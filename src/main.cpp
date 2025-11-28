#include "pch.h"
#include "types.h"
#include "ResolutionVerifier.h"
#include "DllMonitor.h"
#include "PatchInstallers.h"

static std::unique_ptr<DllMonitor> g_dllMonitor;

bool APIENTRY DllMain(HMODULE hModule, DWORD fwdReason, LPVOID lpvReserved)
{
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF |
        _CRTDBG_CHECK_ALWAYS_DF |
        _CRTDBG_LEAK_CHECK_DF);
    
    DllMonitor& monitor = GetDllMonitor();

    switch (fwdReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        for (auto& target : CreatePatchTargets())
            monitor.RegisterTarget(std::move(target));

        if (!monitor.Init())
            ShowErrorAsync("Couldn't init dll monitor. HD mod is not working correctly.");

        Screen::UpdateResolutionFromIni();

        break;
    }
    case DLL_PROCESS_DETACH:
    {
        monitor.Shutdown();

        break;
    }
    };
    return true;
}