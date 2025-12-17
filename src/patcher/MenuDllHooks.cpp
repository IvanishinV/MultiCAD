#include "pch.h"
#include "MenuDllHooks.h"
#include "resource.h"
#include "version.h"
#include "SplashTextRenderer.h"

void MenuDllHooks::sub_10014B70_common(void* self, const char* versionStr, int x)
{
    auto* g = globals_;

    const auto sub_100020A0 = g->getFn<void(__thiscall)(void*)>(0x20A0);
    const auto sub_100039F0 = g->getFn<void(__thiscall)(void*, int, int, int)>(0x39F0);
    const auto sub_10003D10 = g->getFn<void(__thiscall)(void*, int, int, const char*, int)>(0x3D10);
    int* dword_100590F8 = g->getPtr<int>(0x590F8);

    // Draw main menu
    sub_100020A0(self);

    // And then draw text over the menu
    sub_100039F0(dword_100590F8, 0xDE, 0xD7, 0x42);

    // Last parameter points if the text is written from right to left.
    // Otherwise, it's centered at the specified offset 
    sub_10003D10(dword_100590F8, x, 357, versionStr, 1);
    sub_10003D10(dword_100590F8, x, 370, SS_HD_MOD_TG_LINK, 1);
    sub_10003D10(dword_100590F8, x, 383, SS_HD_MOD_AUTHOR_EMAIL, 1);

    SplashTextRenderer::Instance().render(sub_100039F0, sub_10003D10, dword_100590F8);
}

void __fastcall MenuDllHooks::sub_10014B70(void* self)
{
    sub_10014B70_common(self, "Multi HD mod v" MULTICAD_VERSION_STR " for " SS_GOLD_GAME_STR, 605);
}

void __fastcall MenuDllHooks::sub_10014B70_hd(void* self)
{
    sub_10014B70_common(self, "Multi HD mod v" MULTICAD_VERSION_STR " for " SS_GOLD_HD_GAME_STR, 617);
}

void __fastcall MenuDllHooks::sub_10014B70_fr(void* self)
{
    auto* g = globals_;

    const auto sub_100020A0 = g->getFn<void(__thiscall)(void*)>(0x20D0);
    const auto sub_100039F0 = g->getFn<void(__thiscall)(void*, int, int, int)>(0x3A20);
    const auto sub_10003D10 = g->getFn<void(__thiscall)(void*, int, int, const char*, int)>(0x3D40);
    int* dword_100590F8 = g->getPtr<int>(0x59118);

    sub_100020A0(self);
    sub_100039F0(dword_100590F8, 0xDE, 0xD7, 0x42);

    sub_10003D10(dword_100590F8, 605, 357, "Multi HD mod v" MULTICAD_VERSION_STR " for " SS_GOLD_GAME_STR, 1);
    sub_10003D10(dword_100590F8, 605, 370, SS_HD_MOD_TG_LINK, 1);
    sub_10003D10(dword_100590F8, 605, 383, SS_HD_MOD_AUTHOR_EMAIL, 1);

    SplashTextRenderer::Instance().render(sub_100039F0, sub_10003D10, dword_100590F8);
}

void __fastcall MenuDllHooks::sub_10014B70_ru(void* self)
{
    auto* g = globals_;

    const auto sub_100020A0 = g->getFn<void(__thiscall)(void*)>(0x20D0);
    const auto sub_100039F0 = g->getFn<void(__thiscall)(void*, int, int, int)>(0x3BA0);
    const auto sub_10003D10 = g->getFn<void(__thiscall)(void*, int, int, const char*, int)>(0x3EC0);
    int* dword_100590F8 = g->getPtr<int>(0x580F8);

    sub_100020A0(self);
    sub_100039F0(dword_100590F8, 0xDE, 0xD7, 0x42);

    sub_10003D10(dword_100590F8, 640, 357, "Multi HD mod v" MULTICAD_VERSION_STR " for " SS_GOLD_GAME_STR, 1);
    sub_10003D10(dword_100590F8, 640, 370, SS_HD_MOD_TG_LINK, 1);
    sub_10003D10(dword_100590F8, 640, 383, SS_HD_MOD_AUTHOR_EMAIL, 1);

    SplashTextRenderer::Instance().render(sub_100039F0, sub_10003D10, dword_100590F8);
}

void __fastcall MenuDllHooks::sub_1000E3D0_ru(void* self)
{
    auto* g = globals_;

    const auto sub_100020A0 = g->getFn<void(__thiscall)(void*)>(0x1FE0);
    sub_100020A0(self);

    const auto sub_100039F0 = g->getFn<void(__thiscall)(void*, int, int, int)>(0x3960);
    const auto sub_10003D10 = g->getFn<void(__thiscall)(void*, int, int, const char*, int)>(0x3CA0);
    int* dword_100590F8 = g->getPtr<int>(0x4C268);

    sub_100039F0(dword_100590F8, 0xDE, 0xD7, 0x42);

    sub_10003D10(dword_100590F8, 640, 357, "Multi HD mod v" MULTICAD_VERSION_STR " for " SS_GAME_STR, 1);
    sub_10003D10(dword_100590F8, 640, 370, SS_HD_MOD_TG_LINK, 1);
    sub_10003D10(dword_100590F8, 640, 383, SS_HD_MOD_AUTHOR_EMAIL, 1);

    SplashTextRenderer::Instance().render(sub_100039F0, sub_10003D10, dword_100590F8);
}
