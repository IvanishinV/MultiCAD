#include "pch.h"
#include "ResolutionDialog.h"

#include <algorithm>
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib")

HWND CreateSimpleDialog(HINSTANCE hInst, HWND hwndParent)
{
    const int width = 320, height = 300;

    HFONT hFont = CreateFont(
        16, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_SWISS,
        L"Segoe UI");

    HWND hwndDlg = CreateWindowExW(
        WS_EX_DLGMODALFRAME,
        L"#32770",
        L"Select a resolution",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        hwndParent,
        nullptr,
        hInst,
        nullptr);

    if (!hwndDlg)
        return nullptr;

    // Center the window
    RECT rc;
    GetWindowRect(hwndDlg, &rc);
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    int windowWidth = rc.right - rc.left;
    int windowHeight = rc.bottom - rc.top;
    int x = (screenWidth - windowWidth) / 2;
    int y = (screenHeight - windowHeight) / 2;
    SetWindowPos(hwndDlg, nullptr, x, y, 0, 0, SWP_NOZORDER | SWP_NOSIZE);

    // Create controls
    HWND hwndList = CreateWindowExW(0, L"LISTBOX", nullptr,
        WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY | WS_VSCROLL,
        10, 10, width - 35, 230,
        hwndDlg, (HMENU)1001, hInst, nullptr);

    SendMessage(hwndList, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hwndButtonOK = CreateWindowExW(0, L"BUTTON", L"Ok",
        WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
        60, 235, 80, 25,
        hwndDlg, (HMENU)IDOK, hInst, nullptr);

    SendMessage(hwndButtonOK, WM_SETFONT, (WPARAM)hFont, TRUE);

    HWND hwndButtonCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
        WS_CHILD | WS_VISIBLE,
        180, 235, 80, 25,
        hwndDlg, (HMENU)IDCANCEL, hInst, nullptr);

    SendMessage(hwndButtonCancel, WM_SETFONT, (WPARAM)hFont, TRUE);

    return hwndDlg;
}

LRESULT CALLBACK DialogWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    auto* ctx = reinterpret_cast<ResolutionDialogContext*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (uMsg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
        {
            HWND hwndList = GetDlgItem(hwnd, 1001);
            int sel = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0);

            if (sel >= 0 && sel < (int)ctx->availableModes.size())
            {
                ctx->selectedIndex = sel;
                PostQuitMessage(IDOK);
            }
            return 0;
        }
        case IDCANCEL:
        {
            ctx->selectedIndex = -1;
            PostQuitMessage(IDCANCEL);
            return 0;
        }
        case 1001: // listbox
            if (HIWORD(wParam) == LBN_DBLCLK)
            {
                HWND hwndList = GetDlgItem(hwnd, 1001);
                int sel = (int)SendMessage(hwndList, LB_GETCURSEL, 0, 0);

                if (sel >= 0 && sel < (int)ctx->availableModes.size())
                {
                    ctx->selectedIndex = sel;
                    PostQuitMessage(IDOK);
                }
                return 0;
            }
            break;
        }
        break;
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

bool ShowModeSelectionDialog(HINSTANCE hInst, HWND hwndParent, std::shared_ptr<ResolutionDialogContext> ctx)
{
    HWND hWnd = GetForegroundWindow();

    HWND hwndDlg = CreateSimpleDialog(hInst, hwndParent);
    if (!hwndDlg)
        return false;

    HWND hwndList = GetDlgItem(hwndDlg, 1001);

    std::sort(ctx->availableModes.begin(), ctx->availableModes.end(), std::greater<>());

    for (const auto& mode : ctx->availableModes)
    {
        wchar_t buf[64];
        swprintf(buf, 64, L"%dx%d %d-bit", mode.width, mode.height, mode.bits);
        SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)buf);
    }
    SendMessage(hwndList, LB_SETCURSEL, 0, 0);

    SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)ctx.get());
    SetWindowLongPtr(hwndDlg, GWLP_WNDPROC, (LONG_PTR)DialogWndProc);

    ShowWindow(hwndDlg, SW_SHOW);
    PlaySound(TEXT("SystemQuestion"), nullptr, SND_ALIAS | SND_ASYNC);

    if (hWnd && !IsIconic(hWnd))
        ShowWindow(hWnd, SW_MINIMIZE);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (msg.message == WM_QUIT)
            break;

        if (!IsDialogMessage(hwndDlg, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    DestroyWindow(hwndDlg);

    if (hWnd)
    {
        ShowWindow(hWnd, SW_MAXIMIZE);
        SetForegroundWindow(hWnd);
    }

    if (ctx->selectedIndex >= 0 && ctx->selectedIndex < (int)ctx->availableModes.size())
    {
        ctx->selectedMode = ctx->availableModes[ctx->selectedIndex];
        return true;
    }

    return false;
}