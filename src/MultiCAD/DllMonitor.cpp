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
}

void DllMonitor::HandleLoad(const std::wstring& matched, uintptr_t base, size_t size, const std::wstring& fullPath)
{
    TargetInfo target;
    {
        std::lock_guard lk(m_targetsMutex);
        target = m_targets.at(matched);
    }

    TargetState st;
    const bool ok = target.onLoaded ? target.onLoaded(st, base, size, fullPath) : false;

    if (ok)
    {
        st.active = true;
        std::lock_guard lk(m_statesMutex);
        m_states[matched] = std::move(st);
    }
}

void DllMonitor::HandleUnload(const std::wstring& matched)
{
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

bool DllMonitor::TryMatchTargets(const std::wstring& moduleBaseName, std::wstring& outMatchedPart)
{
    std::string lowerMod;
    lowerMod.reserve(moduleBaseName.size());

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
            const std::wstring baseName = me.szModule;
            std::wstring matchedPart;
            if (!TryMatchTargets(baseName, matchedPart))
                continue;

            TargetInfo target;
            {
                std::lock_guard lk(m_targetsMutex);
                target = m_targets.at(matchedPart);
            }

            TargetState state;
            const bool ok = target.onLoaded ? target.onLoaded(state,
                reinterpret_cast<uintptr_t>(me.modBaseAddr),
                me.modBaseSize,
                me.szExePath) : false;

            if (ok)
            {
                state.active = true;
                std::lock_guard lk(m_statesMutex);
                m_states[matchedPart] = std::move(state);
            }
        } while (Module32NextW(snap, &me));
    }

    CloseHandle(snap);
}
