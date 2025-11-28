#pragma once

#include "LdrNotification.h"
#include "PatchEngine.h"
#include "DllVersionDetector.h"
#include "ProfileFactory.h"

#include <functional>
#include <queue>

class DllMonitor;

struct TargetState
{
    std::optional<PatchEngine> patchEngine;
    PatchSession patchSession;
    bool active{ false };
};

struct TargetInfo
{
    std::wstring namePart;
    std::function<bool(TargetState& state, uintptr_t base, size_t size, const std::wstring& path)> onLoaded;
    std::function<void(TargetState& state)> onUnloaded;
};

class DllMonitor
{
public:
    DllMonitor() = default;
    ~DllMonitor();

    bool Init();
    void Shutdown();

    void RegisterTarget(const TargetInfo& info);

private:
    void HandleLoad(const std::wstring& matched, uintptr_t base, size_t size, const std::wstring& fullPath);
    void HandleUnload(const std::wstring& matched);

    bool TryMatchTargets(const std::wstring& moduleBaseName, std::wstring& outMatchedPart);
    // This function doesn't stop threads when injecting functions
    void ScanLoadedModules();

    static void CALLBACK DllNotification(
        ULONG notificationReason,
        const LDR_DLL_NOTIFICATION_DATA* notificationData,
        PVOID context);

private:
    // lowercase name part -> TargetInfo
    std::mutex m_targetsMutex;
    std::unordered_map<std::wstring, TargetInfo> m_targets;

    // lowercase name part -> TargetState
    std::mutex m_statesMutex;
    std::unordered_map<std::wstring, TargetState> m_states;

    pfnLdrRegisterDllNotification m_pLdrRegisterDllNotification{ nullptr };
    pfnLdrUnregisterDllNotification m_pLdrUnregisterDllNotification{ nullptr };
    void* m_dllNotificationCookie{ nullptr };
};

DllMonitor& GetDllMonitor();
