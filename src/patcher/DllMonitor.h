#pragma once

#include "LdrNotification.h"
#include "PatchEngine.h"
#include "DllVersionDetector.h"
#include "ProfileFactory.h"

#include <functional>
#include <queue>

struct TargetState
{
    std::optional<PatchEngine> patchEngine;
    PatchSession patchSession;

    uintptr_t base{ 0 };
    size_t size{ 0 };

    std::wstring fullPath;

    bool active{ false };
    bool packed{ false };
    bool unpacked{ false };
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
    // Called on the loader thread from a packed module's hooked entry point.
    void NotifyUnpacked(uintptr_t base);

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
    // Neither mutex is held across an onLoaded/onUnloaded callback: hashing a
    // module loads the crypto providers, and every load re-enters
    // DllNotification on this thread, where re-taking one would throw.
    //
    // What a callback sees stays consistent because the loader serialises the
    // callers - notifications, ScanLoadedModules and Shutdown all run under the
    // loader lock - and unordered_map keeps element references valid across
    // inserts. A TargetState* may therefore outlive the lock it came from.

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
