#pragma once

#include "LdrNotification.h"
#include "PatchEngine.h"

class DllMonitor
{
public:
    DllMonitor(const std::wstring& targetNamePart);
    ~DllMonitor();

    bool Init();
    void Shutdown();

private:
    std::wstring m_targetNamePart;
    void* m_dllNotificationCookie = nullptr;

    std::unique_ptr<PatchEngine> m_patchEngine;
    PatchSession m_patchSession;

    pfnLdrUnregisterDllNotification m_pLdrUnregisterDllNotification = nullptr;


    static void CALLBACK DllNotification(
        ULONG notificationReason,
        const LDR_DLL_NOTIFICATION_DATA* notificationData,
        PVOID context);

    bool HandleDllLoaded(const LDR_DLL_LOADED_NOTIFICATION_DATA* data);
    bool HandleDllUnloaded(const LDR_DLL_UNLOADED_NOTIFICATION_DATA* data);

    static bool ContainsIgnoreCase(const std::wstring& str, const std::wstring& part);
};

DllMonitor& GetDllMonitor();
