#include "pch.h"
#include "types.h"
#include "ResolutionVerifier.h"

bool APIENTRY DllMain(HMODULE hModule, DWORD fwdReason, LPVOID lpvReserved)
{
    switch (fwdReason)
    {
    case DLL_PROCESS_ATTACH:
    {
        ResolutionVerifier::CreateInstance();

        break;
    }
    case DLL_PROCESS_DETACH:
    {
        ResolutionVerifier::DestroyInstance();

        break;
    }
    };
    return true;
}