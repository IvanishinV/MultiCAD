#include "pch.h"
#include "util.h"
#include <thread>

void ShowErrorNow(const std::string_view& message, bool isCritical)
{
    MessageBoxA(NULL, message.data(), "MultiCAD error", MB_OK | MB_ICONERROR);
    if (isCritical)
        ExitProcess(EXIT_FAILURE);
}

void ShowErrorAsync(const std::string_view& message, bool isCritical)
{
    std::thread([message, isCritical]() {
        ShowErrorNow(message, isCritical);
        }).detach();
}
