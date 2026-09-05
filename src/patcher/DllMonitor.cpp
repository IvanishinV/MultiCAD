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
    // Our entry-point patch is always a 5-byte jmp rel32.
    constexpr size_t kPatchSize = 5;

    // The displaced prologue is whole instructions, so it can be longer than the
    // patch: ASPack opens with pushad + jmp rel32, which is 6.
    constexpr size_t kMaxPrologue = 16;

    // One RWX block per hooked module; the stubs address it by absolute
    // immediate, so hooked modules share no state.
    //
    //   +0x00  trampoline : unpatches the entry point, then the displaced prologue
    //   +0x40  enterStub  : swaps the loader return address for afterStub
    //   +0x80  afterStub  : calls OnUnpacked, then returns to the loader
    //   +0xC0  retAddr    : loader return address of the in-flight EP call
    constexpr size_t kOffTrampoline = 0x00;
    constexpr size_t kOffEnterStub  = 0x40;
    constexpr size_t kOffAfterStub  = 0x80;
    constexpr size_t kOffRetAddr    = 0xC0;
    constexpr size_t kHookBlockSize = 0x100;

    struct EntryHook
    {
        uint8_t   original[kMaxPrologue]{};
        uint8_t*  block{ nullptr };
        void*     entry{ nullptr };
        uintptr_t base{ 0 };
        DWORD     oldProtect{ 0 };
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

    // mov dword ptr [addr], imm32
    void EmitStoreDword(uint8_t*& p, const void* addr, uint32_t value)
    {
        Emit8(p, 0xC7); Emit8(p, 0x05);
        Emit32(p, Imm(addr));
        Emit32(p, value);
    }

    // mov byte ptr [addr], imm8
    void EmitStoreByte(uint8_t*& p, const void* addr, uint8_t value)
    {
        Emit8(p, 0xC6); Emit8(p, 0x05);
        Emit32(p, Imm(addr));
        Emit8(p, value);
    }
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
    void MakeEntryPatch(uint8_t (&out)[kPatchSize], const uint8_t* ep, const uint8_t* enterStub)
    {
        uint8_t* p = out;
        Emit8(p, 0xE9);                                     // jmp rel32
        Emit32(p, static_cast<uint32_t>(enterStub - (ep + kPatchSize)));
    }

    // Bytes taken by a ModRM operand: the byte itself, an optional SIB, and
    // whatever displacement the addressing mode implies.
    size_t ModRmSize(const uint8_t* p)
    {
        const uint8_t modrm = *p;
        const uint8_t mod   = static_cast<uint8_t>(modrm >> 6);
        const uint8_t rm    = static_cast<uint8_t>(modrm & 0x07);

        if (mod == 3)
            return 1;                       // register direct

        size_t n = 1;
        if (rm == 4)                        // a SIB byte follows
        {
            ++n;
            if (mod == 0 && (p[1] & 0x07) == 5)
                return n + 4;               // disp32, no base register
        }
        else if (mod == 0 && rm == 5)
            return n + 4;                   // absolute disp32

        if (mod == 1) return n + 1;
        if (mod == 2) return n + 4;
        return n;
    }

    // What the trampoline has to reproduce: whole instructions covering at least
    // the 5 bytes the patch overwrites.
    struct Prologue
    {
        size_t         size{ 0 };       // bytes displaced at the entry point
        size_t         copySize{ 0 };   // bytes copied verbatim into the trampoline
        const uint8_t* resume{ nullptr };   // where the trampoline jumps when done
    };

    // A relative jmp cannot be copied verbatim - the trampoline sits at another
    // address, so the displacement would land somewhere else. It is re-emitted
    // against its original target instead, which also ends the prologue.
    // An opcode this does not know is a refusal to hook, never a split instruction.
    bool DecodePrologue(const uint8_t* ep, Prologue& out)
    {
        size_t n = 0;

        while (n < kPatchSize)
        {
            const uint8_t  op    = ep[n];
            const uint8_t* modrm = ep + n + 1;
            size_t len = 0;

            if (op == 0x60 || op == 0x9C || op == 0x90 || (op >= 0x50 && op <= 0x57))
                len = 1;                                    // pushad/pushfd/nop/push r32
            else if (op == 0x6A)
                len = 2;                                    // push imm8
            else if (op == 0x68 || (op >= 0xB8 && op <= 0xBF))
                len = 5;                                    // push imm32 / mov r32, imm32
            else if (op == 0x89 || op == 0x8B || op == 0x33 || op == 0x31
                     || op == 0x03 || op == 0x2B || op == 0x85 || op == 0x39)
                len = 1 + ModRmSize(modrm);                 // mov/xor/add/sub/test/cmp r/m32
            else if (op == 0x83)
                len = 1 + ModRmSize(modrm) + 1;             // grp1 r/m32, imm8
            else if (op == 0x81)
                len = 1 + ModRmSize(modrm) + 4;             // grp1 r/m32, imm32
            else if (op == 0xE9 || op == 0xEB)
            {
                const size_t jmpLen = (op == 0xE9) ? 5 : 2;
                const ptrdiff_t rel = (op == 0xE9)
                    ? static_cast<ptrdiff_t>(*reinterpret_cast<const int32_t*>(ep + n + 1))
                    : static_cast<ptrdiff_t>(*reinterpret_cast<const int8_t*>(ep + n + 1));

                out.copySize = n;
                out.size     = n + jmpLen;
                out.resume   = ep + n + jmpLen + rel;

                // A rel8 jmp can end the prologue in fewer bytes than the patch
                // overwrites. Refuse those: the saved prologue has to cover every
                // byte the patch touches, or restoring it writes zeros over code
                // that was never ours.
                return out.size >= kPatchSize && out.size <= kMaxPrologue;
            }
            else
                return false;

            if (n + len > kMaxPrologue)
                return false;

            n += len;
        }

        // Reaching here means n >= kPatchSize, so the saved prologue always
        // covers the patch.
        out.size     = n;
        out.copySize = n;
        out.resume   = ep + n;
        return true;
    }

    bool HookEntryPoint(void* entryPoint, uintptr_t base)
    {
        auto* ep = static_cast<uint8_t*>(entryPoint);

        {
            std::lock_guard lk(g_hooksMutex);
            if (g_hooks.count(base))
                return true;   // re-hooking would save our own jmp as the prologue
        }

        Prologue prologue;
        if (!DecodePrologue(ep, prologue))
            return false;   // unknown entry shape: leave the module alone

        auto* block = static_cast<uint8_t*>(VirtualAlloc(
            nullptr, kHookBlockSize, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
        if (!block)
            return false;

        EntryHook hook;
        hook.block = block;
        hook.entry = entryPoint;
        hook.base = base;
        memcpy(hook.original, ep, prologue.size);

        uint8_t* const trampoline = block + kOffTrampoline;
        uint8_t* const enterStub  = block + kOffEnterStub;
        uint8_t* const afterStub  = block + kOffAfterStub;
        uint8_t* const retAddr    = block + kOffRetAddr;

        memset(retAddr, 0, sizeof(void*));

        // trampoline: put the entry point back before anything else, then run the
        // displaced prologue and rejoin the original flow.
        //
        // Unpatching here rather than after the fact is what makes this safe with
        // a self-modifying stub. ASPack finishes by zeroing the displacement of
        // the jmp at its entry point so later thread attaches fall through to an
        // already-unpacked path - and that write lands inside our 5-byte patch,
        // leaving a jmp to nowhere. Restoring first means the packer only ever
        // sees, and rewrites, its own bytes. The page is left writable until
        // UnhookEntryPoint runs, since these stores execute before it.
        {
            uint8_t* p = trampoline;

            uint32_t head;
            memcpy(&head, hook.original, sizeof(head));
            EmitStoreDword(p, ep, head);
            EmitStoreByte(p, ep + sizeof(head), hook.original[sizeof(head)]);

            memcpy(p, hook.original, prologue.copySize);
            p += prologue.copySize;
            Emit8(p, 0xE9);                                 // jmp rel32
            EmitRel32(p, prologue.resume);
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
        if (!VirtualProtect(ep, kPatchSize, PAGE_EXECUTE_READWRITE, &oldProt))
        {
            VirtualFree(block, 0, MEM_RELEASE);
            return false;
        }

        uint8_t patch[kPatchSize];
        MakeEntryPatch(patch, ep, enterStub);
        memcpy(ep, patch, kPatchSize);

        // Deliberately left writable: the trampoline restores these bytes itself.
        FlushInstructionCache(GetCurrentProcess(), ep, kPatchSize);

        hook.oldProtect = oldProt;
        hook.hooked = true;

        std::lock_guard lk(g_hooksMutex);
        g_hooks[base] = hook;

        return true;
    }

    // By now the trampoline has usually put the entry point back itself, so this
    // only rewrites bytes when the entry point never ran - a module unloaded
    // before initialisation. Whatever the packer has since written there is left
    // alone. VirtualProtect runs first so an unmapped module fails cleanly
    // instead of faulting in the memcmp, and the page protection the hook left
    // open is closed again here.
    bool UnhookEntryPoint(EntryHook& hook)
    {
        if (!hook.hooked)
            return false;

        auto* ep = static_cast<uint8_t*>(hook.entry);

        uint8_t expected[kPatchSize];
        MakeEntryPatch(expected, ep, hook.block + kOffEnterStub);

        DWORD oldProt;
        if (!VirtualProtect(ep, kPatchSize, PAGE_EXECUTE_READWRITE, &oldProt))
            return false;

        const bool ours = memcmp(ep, expected, kPatchSize) == 0;

        if (ours)
            memcpy(ep, hook.original, kPatchSize);

        VirtualProtect(ep, kPatchSize, hook.oldProtect, &oldProt);

        if (ours)
            FlushInstructionCache(GetCurrentProcess(), ep, kPatchSize);

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
            || memcmp(sec[i].Name, ".aspack", 7) == 0
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

    // Take both maps first, so the callbacks below run unlocked like the ones
    // in HandleLoad, HandleUnload and NotifyUnpacked.
    std::unordered_map<std::wstring, TargetState> states;
    {
        std::lock_guard lkStates(m_statesMutex);
        states.swap(m_states);
    }

    std::unordered_map<std::wstring, TargetInfo> targets;
    {
        std::lock_guard lkTargets(m_targetsMutex);
        targets.swap(m_targets);
    }

    for (auto& [key, state] : states)
    {
        if (!state.active || key.empty())
            continue;

        auto it = targets.find(key);
        if (it != targets.end() && it->second.onUnloaded)
        {
            it->second.onUnloaded(state);
        }
    }

    states.clear();   // ~PatchSession unapplies whatever onUnloaded left behind

    ReleaseAllHooks();
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
        {
            shouldTrack = true;
        }
        else
        {
            // Nothing downstream runs for this module - no unpack callback, so no
            // installer and none of its error paths. Report it here or the failure
            // is silent.
#ifdef _DEBUG
            OutputDebugStringA("Couldn't hook the packed dll entry point.\n");
#endif
            ShowErrorAsync("MultiCAD couldn't hook the packed dll entry point and doesn't try to patch it. The game will NOT work correctly. \nTo add support, contact the author of the mod.");
        }
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

    // The entry point is DllMain, re-entered on every DLL_THREAD_ATTACH. It
    // only needs unpacking once, so let every later call go straight through.
    // Done before the state check so a module without one still releases its
    // hook instead of leaking it until shutdown.
    UnhookEntryPointFor(base);

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
