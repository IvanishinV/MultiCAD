#include "pch.h"
#include "DllMonitor.h"
#include "DllVersionDetector.h"
#include "ProfileFactory.h"
#include "MemoryRelocator.h"
#include "CodePatcher.h"
#include "GameGlobals.h"
#include "util.h"

#include <algorithm>
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

struct EntryHook
{
    uint8_t original[5];
    void* entry;
    void* trampoline;
    uintptr_t base;
};

EntryHook g_entryHook;
void* g_originalReturn{ nullptr };
std::atomic<bool> g_stopWatcher{ false };

std::atomic<int> g_syncState{ 0 };  // 0 = wait, 1 = ready, 2 = done, 3 = shutdown

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

#pragma warning(push)
#pragma warning(disable: 4740)
__declspec(naked) void AfterEntryPoint()
{
    __asm
    {
        pushad
        pushfd
    }

    g_syncState.store(1, std::memory_order_release);

    while (g_syncState.load(std::memory_order_acquire) != 2)
    {
        if (g_stopWatcher.load(std::memory_order_acquire))
            break;
        _mm_pause();
    }

    __asm
    {
        popfd
        popad

        jmp g_originalReturn
    }
}
#pragma warning(pop)

__declspec(naked) void EntryPointThunk()
{
    __asm
    {
        // stack on input:
        // [esp] = loader return address

        pop eax                         // eax = loader retaddr
        mov g_originalReturn, eax       // save it to global
        push offset AfterEntryPoint     // swap return address

        // call original entry point
        jmp g_entryHook.trampoline
    }
}

bool HookEntryPoint(void* entryPoint)
{
    g_entryHook.entry = entryPoint;

    DWORD oldProt;
    if (!VirtualProtect(entryPoint, 5, PAGE_EXECUTE_READWRITE, &oldProt))
        return false;

    memcpy(g_entryHook.original, entryPoint, 5);

    uint8_t* tramp = (uint8_t*)VirtualAlloc(
        nullptr,
        16,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE
    );
    if (!tramp)
        return false;

    g_entryHook.trampoline = tramp;

    // Original bytes
    memcpy(tramp, g_entryHook.original, 5);

    // jmp back to EP+5
    tramp[5] = 0xE9;
    *(int32_t*)(tramp + 6) = (int32_t)((uint8_t*)entryPoint + 5 - (tramp + 10));

    // Patch EP jmp EntryPointThunk
    uint8_t patch[5];
    patch[0] = 0xE9;
    *(int32_t*)(patch + 1) = (int32_t)((uint8_t*)&EntryPointThunk - ((uint8_t*)entryPoint + 5));

    memcpy(entryPoint, patch, 5);

    VirtualProtect(entryPoint, 5, oldProt, &oldProt);
    FlushInstructionCache(GetCurrentProcess(), entryPoint, 5);

    return true;
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

    std::thread([this]() {
        while (!g_stopWatcher.load(std::memory_order_acquire))
        {
            // Wait signal from AfterEntryPoint
            while (g_syncState.load(std::memory_order_acquire) != 1)
            {
                if (g_stopWatcher.load(std::memory_order_acquire))
                    return; // Shutdown
                _mm_pause();
            }
            
            // Patch dll
            NotifyUnpacked();

            // Signal AfterEntryPoint to continue
            g_syncState.store(2, std::memory_order_release);
        }
        }).detach();

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

    g_syncState.store(3, std::memory_order_release);
}

void DllMonitor::HandleLoad(const std::wstring& matched, uintptr_t base, size_t size, const std::wstring& fullPath)
{
#ifdef _DEBUG
    OutputDebugStringW(std::format(L"HandleLoad for {}", fullPath).c_str());
#endif

    TargetInfo target;
    {
        std::lock_guard lk(m_targetsMutex);
        target = m_targets.at(matched);
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
        if (ep)
        {
            g_entryHook.base = base;
            HookEntryPoint(ep);
            shouldTrack = true;
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
    OutputDebugStringW(std::format(L"HandleUnload for {}", matched).c_str());
#endif

    std::lock_guard lk(m_statesMutex);
    auto it = m_states.find(matched);
    if (it == m_states.end())
        return;

    TargetState& st = it->second;
    {
        std::lock_guard lk2(m_targetsMutex);
        auto it2 = m_targets.find(matched);
        if (it2 != m_targets.end() && it2->second.onUnloaded)
        {
            it2->second.onUnloaded(st);
        }
    }

    m_states.erase(it);
}

void DllMonitor::NotifyUnpacked()
{
    std::lock_guard lk(m_statesMutex);

    for (auto& [key, st] : m_states)
    {
        if (st.base == g_entryHook.base && st.packed && !st.unpacked)
        {
            st.unpacked = true;

            std::lock_guard lk2(m_targetsMutex);
            auto it = m_targets.find(key);
            if (it != m_targets.end() && it->second.onLoaded)
            {
                if (it->second.onLoaded(st, st.base, st.size, st.fullPath))
                {
                    st.active = true;

                    return;
                }
            }
        }
    }
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
