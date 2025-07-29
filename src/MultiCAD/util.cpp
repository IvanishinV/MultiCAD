#include "pch.h"
#include "util.h"

void ShowErrorMessage(const std::string_view& message, bool isCritical)
{
    MessageBoxA(NULL, message.data(), "MultiCAD error", MB_OK | MB_ICONERROR);
    if (isCritical)
        ExitProcess(EXIT_FAILURE);
}
