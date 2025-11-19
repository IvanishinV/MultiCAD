#pragma once

#include <Windows.h>
#include <vector>
#include <memory>
#include "ResolutionVerifier.h"

struct ResolutionDialogContext
{
    std::vector<ResolutionVerifier::Resolution> availableModes;
    ResolutionVerifier::Resolution selectedMode{};
    int selectedIndex{ -1 };
};

HWND CreateSimpleDialog(HINSTANCE hInst, HWND hwndParent);
LRESULT CALLBACK DialogWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool ShowModeSelectionDialog(HINSTANCE hInst, HWND hwndParent, std::shared_ptr<ResolutionDialogContext> ctx);