#include "pch.h"
#include "types.h"
#include "ResolutionVerifier.h"
#include "DllMonitor.h"

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
        monitor.Init();

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