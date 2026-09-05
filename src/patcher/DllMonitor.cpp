#include "pch.h"
#include "DllMonitor.h"
#include "DllVersionDetector.h"
#include "ProfileFactory.h"
#include "MemoryRelocator.h"
#include "CodePatcher.h"
#include "GameGlobals.h"
#include "util.h"

#include <algorithm>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <TlHelp32.h>

/**
                                 DllMonitor
                                /          \
                Game dll GameVersion      Menu dll GameVersion
                             /                \
   ProfileFactory + GameVersionProfile  ProfileFactory + GameVersionProfile
                                \           /
                                 PatchEngine
                                /           \
                      MemoryRelocator       CodePatcher
* */

namespace
{
    constexpr size_t kPrologueSize = 5;

    // One RWX block per hooked module; the stubs address it by absolute
    // immediate, so hooked modules share no state.
    //
    //   +0x00  trampoline : original prologue, then jmp EP+5
    //   +0x10  enterStub  : swaps the loader return address for afterStub
    //   +0x20  afterStub  : calls OnUnpacked, then returns to the loader
    //   +0x40  retAddr    : loader return address of the in-flight EP call
    constexpr size_t kOffTrampoline = 0x00;
    constexpr size_t kOffEnterStub  = 0x10;
    constexpr size_t kOffAfterStub  = 0x20;
    constexpr size_t kOffRetAddr    = 0x40;
    constexpr size_t kHookBlockSize = 0x50;

    struct EntryHook
    {
        uint8_t   original[kPrologueSize]{};
        uint8_t*  block{ nullptr };
        void*     entry{ nullptr };
        uintptr_t base{ 0 };
        bool      hooked{ false };
    };

    std::mutex g_hooksMutex;
    std::unordered_map<uintptr_t, EntryHook> g_hooks;   // module base -> hook

    void Emit8(uint8_t*& p, uint8_t v) { *p++ = v; }

    void Emit32(uint8_t*& p, uint32_t v) { memcpy(p, &v, sizeof(v)); p += sizeof(v); }

    // rel32 displacements are measured from the end of the instruction
    void EmitRel32(uint8_t*& p, const void* target)
    {
        const uintptr_t end = reinterpret_cast<uintptr_t>(p) + sizeof(uint32_t);
        Emit32(p, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(target) - end));
    }

    uint32_t Imm(const void* p) { return static_cast<uint32_t>(reinterpret_cast<uintptr_t>(p)); }
}

// Called from afterStub, on the loader thread, with the loader lock held: the
// packer stub has just decompressed the image, so this is the only point where
// the real code is patchable and has not yet run.
//
// Runs inline on purpose. The loader thread blocks until patching finishes
// either way, so a worker buys no concurrency - and it turns the loader-lock
// calls on the patch path (GetIniPath -> GetModuleHandleExA) from a safe
// recursive acquire into a cross-thread deadlock.
extern "C" void __cdecl OnUnpacked(uintptr_t base)
{
    GetDllMonitor().NotifyUnpacked(base);
}

void* GetDllEntryPoint(uintptr_t base)
{
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return nullptr;

    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE)
        return nullptr;

    return reinterpret_cast<void*>(base + nt->OptionalHeader.AddressOfEntryPoint);
}

namespace
{
    // The 5-byte jmp that redirects the entry point at enterStub. Also used to
    // recognise our own patch when unhooking.
    void MakeEntryPatch(uint8_t (&out)[kPrologueSize], const uint8_t* ep, const uint8_t* enterStub)
    {
        uint8_t* p = out;
        Emit8(p, 0xE9);                                     // jmp rel32
        Emit32(p, static_cast<uint32_t>(enterStub - (ep + kPrologueSize)));
    }

    bool HookEntryPoint(void* entryPoint, uintptr_t base)
    {
        auto* ep = static_cast<uint8_t*>(entryPoint);

        {
            std::lock_guard lk(g_hooksMutex);
            if (g_hooks.count(base))
                return true;   // re-hooking would save our own jmp as the prologue
        }

        auto* block = static_cast<uint8_t*>(VirtualAlloc(
            nullptr, kHookBlockSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        if (!block)
            return false;

        EntryHook hook;
        hook.block = block;
        hook.entry = entryPoint;
        hook.base = base;
        memcpy(hook.original, ep, kPrologueSize);

        uint8_t* const trampoline = block + kOffTrampoline;
        uint8_t* const enterStub  = block + kOffEnterStub;
        uint8_t* const afterStub  = block + kOffAfterStub;
        uint8_t* const retAddr    = block + kOffRetAddr;

        memset(retAddr, 0, sizeof(void*));

        // trampoline: run the displaced prologue, then rejoin the entry point
        {
            uint8_t* p = trampoline;
            memcpy(p, hook.original, kPrologueSize);
            p += kPrologueSize;
            Emit8(p, 0xE9);                                 // jmp rel32
            EmitRel32(p, ep + kPrologueSize);
        }

        // enterStub: stash the loader's return address, substitute afterStub
        {
            uint8_t* p = enterStub;
            Emit8(p, 0x58);                                 // pop eax
            Emit8(p, 0xA3); Emit32(p, Imm(retAddr));        // mov [retAddr], eax
            Emit8(p, 0x68); Emit32(p, Imm(afterStub));      // push afterStub
            Emit8(p, 0xE9); EmitRel32(p, trampoline);       // jmp trampoline
        }

        // afterStub: entry point returned, image is unpacked. eax holds
        // DllMain's return value and is preserved by pushad/popad.
        {
            uint8_t* p = afterStub;
            Emit8(p, 0x60);                                 // pushad
            Emit8(p, 0x9C);                                 // pushfd
            Emit8(p, 0x68); Emit32(p, static_cast<uint32_t>(base));   // push base
            Emit8(p, 0xE8);                                 // call rel32
            EmitRel32(p, reinterpret_cast<const void*>(&OnUnpacked));
            Emit8(p, 0x83); Emit8(p, 0xC4); Emit8(p, 0x04); // add esp, 4  (cdecl)
            Emit8(p, 0x9D);                                 // popfd
            Emit8(p, 0x61);                                 // popad
            Emit8(p, 0xFF); Emit8(p, 0x25); Emit32(p, Imm(retAddr));  // jmp [retAddr]
        }

        FlushInstructionCache(GetCurrentProcess(), block, kHookBlockSize);

        DWORD oldProt;
        if (!VirtualProtect(ep, kPrologueSize, PAGE_EXECUTE_READWRITE, &oldProt))
        {
            VirtualFree(block, 0, MEM_RELEASE);
            return false;
        }

        uint8_t patch[kPrologueSize];
        MakeEntryPatch(patch, ep, enterStub);
        memcpy(ep, patch, kPrologueSize);

        VirtualProtect(ep, kPrologueSize, oldProt, &oldProt);
        FlushInstructionCache(GetCurrentProcess(), ep, kPrologueSize);

        hook.hooked = true;

        std::lock_guard lk(g_hooksMutex);
        g_hooks[base] = hook;

        return true;
    }

    // Only restores when our jmp is still there: if the packer rewrote its own
    // prologue, the saved bytes are stale. VirtualProtect runs first so an
    // unmapped module fails cleanly instead of faulting in the memcmp.
    bool UnhookEntryPoint(EntryHook& hook)
    {
        if (!hook.hooked)
            return false;

        auto* ep = static_cast<uint8_t*>(hook.entry);

        uint8_t expected[kPrologueSize];
        MakeEntryPatch(expected, ep, hook.block + kOffEnterStub);

        DWORD oldProt;
        if (!VirtualProtect(ep, kPrologueSize, PAGE_EXECUTE_READWRITE, &oldProt))
            return false;

        const bool ours = memcmp(ep, expected, kPrologueSize) == 0;
        if (ours)
            memcpy(ep, hook.original, kPrologueSize);

        VirtualProtect(ep, kPrologueSize, oldProt, &oldProt);

        if (ours)
            FlushInstructionCache(GetCurrentProcess(), ep, kPrologueSize);

        hook.hooked = false;
        return ours;
    }

    // Keeps the block mapped: afterStub is still executing out of it.
    void UnhookEntryPointFor(uintptr_t base)
    {
        std::lock_guard lk(g_hooksMutex);

        auto it = g_hooks.find(base);
        if (it != g_hooks.end())
            UnhookEntryPoint(it->second);
    }

    // Safe only once no stub can be in flight: module gone, or process detach.
    void ReleaseHook(uintptr_t base, bool restoreEntryPoint)
    {
        std::lock_guard lk(g_hooksMutex);

        auto it = g_hooks.find(base);
        if (it == g_hooks.end())
            return;

        if (restoreEntryPoint)
            UnhookEntryPoint(it->second);

        VirtualFree(it->second.block, 0, MEM_RELEASE);
        g_hooks.erase(it);
    }

    void ReleaseAllHooks()
    {
        std::lock_guard lk(g_hooksMutex);

        for (auto& kv : g_hooks)
        {
            UnhookEntryPoint(kv.second);
            VirtualFree(kv.second.block, 0, MEM_RELEASE);
        }

        g_hooks.clear();
    }
}


bool IsPackedModule(uintptr_t base)
{
    auto* dos = (IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE)
        return false;

    auto* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    auto* sec = IMAGE_FIRST_SECTION(nt);

    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++)
    {
        if (memcmp(sec[i].Name, ".petite", 7) == 0
            || (memcmp(sec[i].Name, ".\0\0\0\0\0\0\0", 8) == 0 && i == 0)
            || (memcmp(sec[i].Name, "\0\0\0\0\0\0\0\0", 8) == 0 && i == 0))
        {
#ifdef _DEBUG
            OutputDebugStringA("The DLL is packed.\n");
#endif
            return true;
        }
    }

#ifdef _DEBUG
    OutputDebugStringA("The DLL is not packed.\n");
#endif
    return false;
}


DllMonitor& GetDllMonitor()
{
    static DllMonitor instance;
    return instance;
}

DllMonitor::~DllMonitor()
{
    Shutdown();
}

void DllMonitor::RegisterTarget(const TargetInfo& info)
{
    std::lock_guard lk(m_targetsMutex);

    std::wstring key = info.namePart;
    std::transform(key.begin(), key.end(), key.begin(), ::towlower);

    m_targets.emplace(key, info);
}

bool DllMonitor::Init()
{
    if (m_dllNotificationCookie)
        return false;

    HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
    if (!hNtdll)
        return false;

    auto pLdrRegisterDllNotification =
        reinterpret_cast<pfnLdrRegisterDllNotification>(
            GetProcAddress(hNtdll, "LdrRegisterDllNotification"));

    m_pLdrUnregisterDllNotification =
        reinterpret_cast<pfnLdrUnregisterDllNotification>(
            GetProcAddress(hNtdll, "LdrUnregisterDllNotification"));

    if (!pLdrRegisterDllNotification || !m_pLdrUnregisterDllNotification)
    {
        Shutdown();
        return false;
    }

    NTSTATUS status = pLdrRegisterDllNotification(0, DllNotification, this, &m_dllNotificationCookie);
    if (!NT_SUCCESS(status))
    {
        m_dllNotificationCookie = nullptr;
        Shutdown();
        return false;
    }

    // No worker thread: OnUnpacked services unpack notifications inline.
    ScanLoadedModules();

    return true;
}

void DllMonitor::Shutdown()
{
    if (m_dllNotificationCookie && m_pLdrUnregisterDllNotification)
    {
        m_pLdrUnregisterDllNotification(m_dllNotificationCookie);
        m_dllNotificationCookie = nullptr;
    }

    std::lock_guard lkStates(m_statesMutex);
    for (auto& kv : m_states)
    {
        auto& state = kv.second;
        if (state.active && !kv.first.empty())
        {
            std::lock_guard lk2(m_targetsMutex);
            auto it = m_targets.find(kv.first);
            if (it != m_targets.end() && it->second.onUnloaded)
            {
                it->second.onUnloaded(state);
            }
        }
    }
    m_states.clear();

    ReleaseAllHooks();

    {
        std::lock_guard lkTargets(m_targetsMutex);
        m_targets.clear();
    }
}

void DllMonitor::HandleLoad(const std::wstring& matched, uintptr_t base, size_t size, const std::wstring& fullPath)
{
#ifdef _DEBUG
    OutputDebugStringW(std::format(L"HandleLoad for {}\n", fullPath).c_str());
#endif

    {
        std::lock_guard lk(m_statesMutex);
        auto it = m_states.find(matched);
        if (it != m_states.end() && it->second.base == base)
            return;   // ScanLoadedModules and the notification can both report it

        // The name part is a substring: "game" also matches
        // gameoverlayrenderer.dll. Patching a second claimant would rebase the
        // cached ModuleInfo and write the patches into the wrong module.
        if (it != m_states.end() && it->second.active)
            return;
    }

    TargetInfo target;
    {
        std::lock_guard lk(m_targetsMutex);
        auto it = m_targets.find(matched);
        if (it == m_targets.end())
            return;   // unregistered between the match and here: never throw out of DllMain

        target = it->second;
    }

    TargetState st;
    st.packed = IsPackedModule(base);
    st.base = base;
    st.size = size;
    st.fullPath = fullPath;

    bool shouldTrack{ false };
    if (!st.packed)
    {
        st.unpacked = true;
        if (target.onLoaded && target.onLoaded(st, base, size, fullPath))
        {
            st.active = true;
            shouldTrack = true;
        }
    }
    else
    {
        void* ep = GetDllEntryPoint(base);
        if (ep && HookEntryPoint(ep, base))
            shouldTrack = true;
    }

    if (shouldTrack)
    {
        std::lock_guard lk(m_statesMutex);
        m_states[matched] = std::move(st);
    }
}

void DllMonitor::HandleUnload(const std::wstring& matched)
{
#ifdef _DEBUG
    OutputDebugStringW(std::format(L"HandleUnload for {}\n", matched).c_str());
#endif

    TargetState* state = nullptr;
    {
        std::lock_guard lk(m_statesMutex);
        auto it = m_states.find(matched);
        if (it == m_states.end())
            return;

        state = &it->second;
    }

    TargetInfo target;
    {
        std::lock_guard lk(m_targetsMutex);
        auto it = m_targets.find(matched);
        if (it != m_targets.end())
            target = it->second;
    }

    const uintptr_t base = state->base;

    if (target.onUnloaded)
        target.onUnloaded(*state);

    // The module is on its way out, so don't write its entry point back.
    ReleaseHook(base, false);

    std::lock_guard lk(m_statesMutex);
    m_states.erase(matched);
}

void DllMonitor::NotifyUnpacked(uintptr_t base)
{
    std::wstring matched;
    TargetState* state = nullptr;

    {
        std::lock_guard lk(m_statesMutex);

        for (auto& [key, st] : m_states)
        {
            if (st.base != base || !st.packed || st.unpacked)
                continue;

            st.unpacked = true;
            matched = key;
            state = &st;

            break;
        }
    }

    if (!state)
        return;

    TargetInfo target;
    {
        std::lock_guard lk(m_targetsMutex);
        auto it = m_targets.find(matched);
        if (it != m_targets.end())
            target = it->second;
    }

    if (target.onLoaded && target.onLoaded(*state, state->base, state->size, state->fullPath))
        state->active = true;

    // The entry point is DllMain, re-entered on every DLL_THREAD_ATTACH. It
    // only needs unpacking once, so let every later call go straight through.
    UnhookEntryPointFor(base);
}

bool DllMonitor::TryMatchTargets(const std::wstring& moduleBaseName, std::wstring& outMatchedPart)
{
    std::wstring lowerW = moduleBaseName;
    std::transform(lowerW.begin(), lowerW.end(), lowerW.begin(), ::towlower);

    if (!lowerW.ends_with(L".dll"))
        return false;

    std::lock_guard lk(m_targetsMutex);
    for (const auto& kv : m_targets)
    {
        const std::wstring& targetKey = kv.first;
        if (lowerW.find(targetKey) != std::wstring::npos)
        {
            outMatchedPart = kv.first;
            return true;
        }
    }
    return false;
}

void CALLBACK DllMonitor::DllNotification(ULONG reason, const LDR_DLL_NOTIFICATION_DATA* data, PVOID ctx)
{
    if (!ctx || !data)
        return;

    auto* self = reinterpret_cast<DllMonitor*>(ctx);

    if (reason == LDR_DLL_NOTIFICATION_REASON_LOADED)
    {
        const auto& d = data->Loaded;
        if (!d.BaseDllName || !d.BaseDllName->Buffer)
            return;

        std::wstring base(d.BaseDllName->Buffer, d.BaseDllName->Length / sizeof(WCHAR));
        std::wstring full;

        if (d.FullDllName && d.FullDllName->Buffer)
            full.assign(d.FullDllName->Buffer, d.FullDllName->Length / sizeof(WCHAR));

        std::wstring matched;
        if (self->TryMatchTargets(base, matched))
            self->HandleLoad(matched, reinterpret_cast<uintptr_t>(d.DllBase), d.SizeOfImage, full);
    }
    else if (reason == LDR_DLL_NOTIFICATION_REASON_UNLOADED)
    {
        const auto& d = data->Unloaded;
        if (!d.BaseDllName || !d.BaseDllName->Buffer)
            return;

        std::wstring base(d.BaseDllName->Buffer, d.BaseDllName->Length / sizeof(WCHAR));
        std::wstring matched;

        if (self->TryMatchTargets(base, matched))
            self->HandleUnload(matched);
    }
}

void DllMonitor::ScanLoadedModules()
{
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (snap == INVALID_HANDLE_VALUE) return;

    MODULEENTRY32W me;
    ZeroMemory(&me, sizeof(me));
    me.dwSize = sizeof(me);
    if (Module32FirstW(snap, &me))
    {
        do
        {
            std::wstring key;
            if (!TryMatchTargets(me.szModule, key))
                continue;

            HandleLoad(key, reinterpret_cast<uintptr_t>(me.modBaseAddr), me.modBaseSize, me.szExePath);
        } while (Module32NextW(snap, &me));
    }

    CloseHandle(snap);
}
