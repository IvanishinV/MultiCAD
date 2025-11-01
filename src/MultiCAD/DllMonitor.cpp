#include "pch.h"
#include "DllMonitor.h"
#include "GameDllVersionDetector.h"
#include "ProfileFactory.h"
#include "MemoryRelocator.h"
#include "CodePatcher.h"
#include "GameGlobals.h"
#include "util.h"

#include <algorithm>

/**
    DllMonitor -> Detector -> GameVersion + ModuleInfo
             \                    |
              \            ProfileFactory + IPatchProfile
               \          /
               PatchEngine
              /           \
    MemoryRelocator       CodePatcher
* */

DllMonitor& GetDllMonitor()
{
    static DllMonitor instance(L"game");
    return instance;
}

DllMonitor::DllMonitor(const std::wstring& targetNamePart)
    : m_targetNamePart(targetNamePart)
{
}

DllMonitor::~DllMonitor()
{
    Shutdown();
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
        return false;
    }

    NTSTATUS status = pLdrRegisterDllNotification(0, DllNotification, this, &m_dllNotificationCookie);
    if (!NT_SUCCESS(status))
    {
        m_dllNotificationCookie = nullptr;
        return false;
    }
    return true;
}

void DllMonitor::Shutdown()
{
    if (m_dllNotificationCookie)
    {
        m_pLdrUnregisterDllNotification(m_dllNotificationCookie);
        m_dllNotificationCookie = nullptr;
    }

    if (m_patchEngine)
    {
        m_patchSession.Unapply();
        m_patchEngine.reset();
    }
}

void CALLBACK DllMonitor::DllNotification(
    ULONG notificationReason,
    const LDR_DLL_NOTIFICATION_DATA* notificationData,
    PVOID context)
{
    if (!context || !notificationData)
        return;

    auto* self = reinterpret_cast<DllMonitor*>(context);

    switch (notificationReason)
    {
    case LDR_DLL_NOTIFICATION_REASON_LOADED:
        self->HandleDllLoaded(&notificationData->Loaded);
        break;
    case LDR_DLL_NOTIFICATION_REASON_UNLOADED:
        self->HandleDllUnloaded(&notificationData->Unloaded);
        break;
    }
}

bool DllMonitor::HandleDllLoaded(const LDR_DLL_LOADED_NOTIFICATION_DATA* data)
{
    if (!data || !data->BaseDllName || !data->BaseDllName->Buffer || !data->FullDllName || !data->FullDllName->Buffer)
        return false;

    const std::wstring dllName(data->BaseDllName->Buffer, data->BaseDllName->Length / sizeof(WCHAR));
    if (!ContainsIgnoreCase(dllName, m_targetNamePart))
        return false;

    const std::wstring dllPath(data->FullDllName->Buffer, data->FullDllName->Length / sizeof(WCHAR));

    GameDllVersionDetector detector((uintptr_t)data->DllBase, data->SizeOfImage);
    detector.DetectGameDll(dllPath);


    switch (detector.GetDetectionStatus())
    {
    case DetectionStatus::UnsupportedHash:
    {
        ShowErrorAsync("MultiCAD couldn't identify and doesn't fully support this version of Sudden Strike. The mod may not work correctly. \nTo add support, contact the author of the mod.");
        break;
    }
    case DetectionStatus::Supported:
    {
        ProfileFactory factory;
        auto profile = factory.create(detector.GetGameVersion());
        if (!profile)
            break;

        auto relocator = std::make_unique<MemoryRelocator>();
        auto injector = std::make_unique<CodePatcher>();
        
        GameDllHooks::init(detector.GetModuleInfo().base);
        m_patchEngine = std::make_unique<PatchEngine>(std::move(relocator), std::move(injector));

        if (!m_patchEngine->Apply(detector.GetModuleInfo(), *profile, m_patchSession))
        {
            GameDllHooks::shutdown();

            ShowErrorAsync("Couldn't patch game dll due to some error. Contact the author.");
            return false;
        }


        return true;
    }
    }

    return false;
}

bool DllMonitor::HandleDllUnloaded(const LDR_DLL_UNLOADED_NOTIFICATION_DATA* data)
{
    if (!data || !data->BaseDllName || !data->BaseDllName->Buffer)
        return false;

    const std::wstring dllName(data->BaseDllName->Buffer, data->BaseDllName->Length / sizeof(WCHAR));
    if (!ContainsIgnoreCase(dllName, m_targetNamePart))
        return false;

    if (m_patchEngine)
    {
        m_patchSession.Unapply();
        m_patchEngine.reset();
    }

    GameDllHooks::shutdown();

    return true;
}

bool DllMonitor::ContainsIgnoreCase(const std::wstring& str, const std::wstring& part)
{
    if (part.empty() || str.empty())
        return false;

    auto it = std::search(
        str.begin(), str.end(),
        part.begin(), part.end(),
        [](wchar_t ch1, wchar_t ch2)
        {
            return towlower(ch1) == towlower(ch2);
        });

    return (it != str.end());
}