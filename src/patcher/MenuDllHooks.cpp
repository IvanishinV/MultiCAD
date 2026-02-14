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

    constexpr auto& splashCfg = g_splashTable[(int)SplashVariant::SS];
    const SplashTextRenderer::Params splash = SplashTextRenderer::MakeParams(splashCfg, sub_100039F0, sub_10003D10, dword_100590F8);

    SplashTextRenderer::Instance().render(splash);
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

    constexpr auto& splashCfg = g_splashTable[(int)SplashVariant::SS];
    const SplashTextRenderer::Params splash = SplashTextRenderer::MakeParams(splashCfg, sub_100039F0, sub_10003D10, dword_100590F8);

    SplashTextRenderer::Instance().render(splash);
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

    constexpr auto& splashCfg = g_splashTable[(int)SplashVariant::SS];
    const SplashTextRenderer::Params splash = SplashTextRenderer::MakeParams(splashCfg, sub_100039F0, sub_10003D10, dword_100590F8);

    SplashTextRenderer::Instance().render(splash);
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

    constexpr auto& splashCfg = g_splashTable[(int)SplashVariant::SS];
    const SplashTextRenderer::Params splash = SplashTextRenderer::MakeParams(splashCfg, sub_100039F0, sub_10003D10, dword_100590F8);

    SplashTextRenderer::Instance().render(splash);
}

void MenuDllHooks::sub_1000E3D0_hd_common(void* self, int x)
{
    auto* g = globals_;

    const auto sub_100020A0 = g->getFn<void(__thiscall)(void*)>(0x1FE0);
    sub_100020A0(self);

    const auto sub_100039F0 = g->getFn<void(__thiscall)(void*, int, int, int)>(0x3960);
    const auto sub_10003D10 = g->getFn<void(__thiscall)(void*, int, int, const char*, int)>(0x3CA0);
    int* dword_100590F8 = g->getPtr<int>(0x4C268);

    sub_100039F0(dword_100590F8, 0xDE, 0xD7, 0x42);

    sub_10003D10(dword_100590F8, x, 357, "Multi HD mod v" MULTICAD_VERSION_STR " for " SS_HD_GAME_STR, 1);
    sub_10003D10(dword_100590F8, x, 370, SS_HD_MOD_TG_LINK, 1);
    sub_10003D10(dword_100590F8, x, 383, SS_HD_MOD_AUTHOR_EMAIL, 1);

    constexpr auto& splashCfg = g_splashTable[(int)SplashVariant::SS];
    const SplashTextRenderer::Params splash = SplashTextRenderer::MakeParams(splashCfg, sub_100039F0, sub_10003D10, dword_100590F8);

    SplashTextRenderer::Instance().render(splash);
}

void __fastcall MenuDllHooks::sub_1000E3D0_hd_ru(void* self)
{
    sub_1000E3D0_hd_common(self, 640);
}

void __fastcall MenuDllHooks::sub_1000E3D0_hd_en(void* self)
{
    sub_1000E3D0_hd_common(self, 617);
}

void __fastcall MenuDllHooks::sub_1000F2D0_en(void* self)
{
    auto* g = globals_;

    const auto sub_100020A0 = g->getFn<void(__thiscall)(void*)>(0x1F80);
    sub_100020A0(self);

    const auto sub_100039F0 = g->getFn<void(__thiscall)(void*, int, int, int)>(0x39F0);
    const auto sub_10003D10 = g->getFn<void(__thiscall)(void*, int, int, const char*, int)>(0x3D10);
    int* dword_100590F8 = g->getPtr<int>(0x4FAA8);

    sub_100039F0(dword_100590F8, 0xDE, 0xD7, 0x42);

    sub_10003D10(dword_100590F8, 605, 357, "Multi HD mod v" MULTICAD_VERSION_STR " for " SS_V1_2_GAME_STR, 1);
    sub_10003D10(dword_100590F8, 605, 370, SS_HD_MOD_TG_LINK, 1);
    sub_10003D10(dword_100590F8, 605, 383, SS_HD_MOD_AUTHOR_EMAIL, 1);

    constexpr auto& splashCfg = g_splashTable[(int)SplashVariant::SS];
    const SplashTextRenderer::Params splash = SplashTextRenderer::MakeParams(splashCfg, sub_100039F0, sub_10003D10, dword_100590F8);

    SplashTextRenderer::Instance().render(splash);
}

void __fastcall MenuDllHooks::sub_1001AC60(void* self)
{
    auto* g = globals_;

    int* dword_100A06A0 = g->getPtr<int>(0xA06A0);
    const auto sub_10075F80 = g->getFn<void(__thiscall)()>(0x75F80);
    if (*dword_100A06A0 != 0)
    {
        sub_10075F80();
        *dword_100A06A0 = 1;

        int* dword_100B6F40 = g->getPtr<int>(0xB6F40);
        int* dword_100B6F3C = g->getPtr<int>(0xB6F3C);
        *dword_100B6F3C = 0;
        *dword_100B6F40 = 0;
    }

    const auto sub_1000D1B0 = g->getFn<void(__thiscall)(void*)>(0xD1B0);
    sub_1000D1B0(self);

    const auto sub_10002C00 = g->getFn<void(__thiscall)(void*, int, int, int)>(0x2C00);
    const auto sub_10002FA0 = g->getFn<void(__thiscall)(void*, int, int, const char*, int)>(0x2FA0);
    int* dword_100B4B48 = g->getPtr<int>(0xB4B48);

    sub_10002C00(dword_100B4B48, 0xAA, 0xAA, 0x55);

    sub_10002FA0(dword_100B4B48, 780, 498, "Multi HD mod v" MULTICAD_VERSION_STR " for", 1);
    sub_10002FA0(dword_100B4B48, 780, 511, SS_2_V2_2_GAME_STR, 1);
    sub_10002FA0(dword_100B4B48, 780, 537, SS_HD_MOD_TG_LINK, 1);
    sub_10002FA0(dword_100B4B48, 780, 550, SS_HD_MOD_AUTHOR_EMAIL, 1);

    constexpr auto& splashCfg = g_splashTable[(int)SplashVariant::SS2];
    const SplashTextRenderer::Params splash =
        SplashTextRenderer::MakeParams(splashCfg, sub_10002C00, sub_10002FA0, dword_100B4B48);

    SplashTextRenderer::Instance().render(splash);
}

void __fastcall MenuDllHooks::sub_1001AC60_eu_2015(void* self)
{
    auto* g = globals_;

    int* dword_100A06A0 = g->getPtr<int>(0xA06A0);
    const auto sub_10075F80 = g->getFn<void(__thiscall)()>(0x75F80);
    if (*dword_100A06A0 != 0)
    {
        sub_10075F80();
        *dword_100A06A0 = 1;

        int* dword_100B6F40 = g->getPtr<int>(0xB6F40);
        int* dword_100B6F3C = g->getPtr<int>(0xB6F3C);
        *dword_100B6F3C = 0;
        *dword_100B6F40 = 0;
    }

    const auto sub_1000D1B0 = g->getFn<void(__thiscall)(void*)>(0xD1B0);
    sub_1000D1B0(self);

    const auto sub_10002C00 = g->getFn<void(__thiscall)(void*, int, int, int)>(0x2C00);
    const auto sub_10002FA0 = g->getFn<void(__thiscall)(void*, int, int, const char*, int)>(0x2FA0);
    int* dword_100B4B48 = g->getPtr<int>(0xB4B48);

    sub_10002C00(dword_100B4B48, 0xAA, 0xAA, 0x55);

    sub_10002FA0(dword_100B4B48, 780, 498, "Multi HD mod v" MULTICAD_VERSION_STR " for", 1);
    sub_10002FA0(dword_100B4B48, 780, 511, SS_EUROPE_2015_V1_0_GAME_STR, 1);
    sub_10002FA0(dword_100B4B48, 780, 537, SS_HD_MOD_TG_LINK, 1);
    sub_10002FA0(dword_100B4B48, 780, 550, SS_HD_MOD_AUTHOR_EMAIL, 1);

    constexpr auto& splashCfg = g_splashTable[(int)SplashVariant::SS2];
    const SplashTextRenderer::Params splash =
        SplashTextRenderer::MakeParams(splashCfg, sub_10002C00, sub_10002FA0, dword_100B4B48);

    SplashTextRenderer::Instance().render(splash);
}

void __fastcall MenuDllHooks::sub_1001B470(void* self)
{
    auto* g = globals_;

    int* dword_1009E740 = g->getPtr<int>(0x9E740);
    const auto sub_10076F60 = g->getFn<void(__thiscall)()>(0x76F60);
    if (*dword_1009E740 != 0)
    {
        sub_10076F60();
        *dword_1009E740 = 1;

        int* dword_100B4FFC = g->getPtr<int>(0xB4FFC);
        int* dword_100B5000 = g->getPtr<int>(0xB5000);
        *dword_100B4FFC = 0;
        *dword_100B5000 = 0;
    }

    const auto sub_1000D460 = g->getFn<void(__thiscall)(void*)>(0xD460);
    sub_1000D460(self);

    const auto sub_10002BD0 = g->getFn<void(__thiscall)(void*, int, int, int)>(0x2BD0);
    const auto sub_10002F70 = g->getFn<void(__thiscall)(void*, int, int, const char*, int)>(0x2F70);
    int* dword_100B2BE8 = g->getPtr<int>(0xB2BE8);

    sub_10002BD0(dword_100B2BE8, 0xAA, 0xAA, 0x55);

    sub_10002F70(dword_100B2BE8, 780, 498, "Multi HD mod v" MULTICAD_VERSION_STR " for", 1);
    sub_10002F70(dword_100B2BE8, 780, 511, SS_RW_V2_3_GAME_STR, 1);
    sub_10002F70(dword_100B2BE8, 780, 537, SS_HD_MOD_TG_LINK, 1);
    sub_10002F70(dword_100B2BE8, 780, 550, SS_HD_MOD_AUTHOR_EMAIL, 1);

    constexpr auto& splashCfg = g_splashTable[(int)SplashVariant::SS2];
    const SplashTextRenderer::Params splash =
        SplashTextRenderer::MakeParams(splashCfg, sub_10002BD0, sub_10002F70, dword_100B2BE8);

    SplashTextRenderer::Instance().render(splash);
}

void __fastcall MenuDllHooks::sub_1001B380(void* self)
{
    auto* g = globals_;

    int* dword_1009E7C0 = g->getPtr<int>(0x9E7C0);
    const auto sub_10076E90 = g->getFn<void(__thiscall)()>(0x76E90);
    if (*dword_1009E7C0 != 0)
    {
        sub_10076E90();
        *dword_1009E7C0 = 1;

        int* dword_100B507C = g->getPtr<int>(0xB507C);
        int* dword_100B5080 = g->getPtr<int>(0xB5080);
        *dword_100B507C = 0;
        *dword_100B5080 = 0;
    }

    const auto sub_1000D3B0 = g->getFn<void(__thiscall)(void*)>(0xD3B0);
    sub_1000D3B0(self);

    const auto sub_10002BC0 = g->getFn<void(__thiscall)(void*, int, int, int)>(0x2BC0);
    const auto sub_10002F60 = g->getFn<void(__thiscall)(void*, int, int, const char*, int)>(0x2F60);
    int* dword_100B2C68 = g->getPtr<int>(0xB2C68);

    sub_10002BC0(dword_100B2C68, 0xAA, 0xAA, 0x55);

    sub_10002F60(dword_100B2C68, 780, 498, "Multi HD mod v" MULTICAD_VERSION_STR " for", 1);
    sub_10002F60(dword_100B2C68, 780, 511, SS_RW_V2_4_GAME_STR, 1);
    sub_10002F60(dword_100B2C68, 780, 537, SS_HD_MOD_TG_LINK, 1);
    sub_10002F60(dword_100B2C68, 780, 550, SS_HD_MOD_AUTHOR_EMAIL, 1);

    constexpr auto& splashCfg = g_splashTable[(int)SplashVariant::SS2];
    const SplashTextRenderer::Params splash =
        SplashTextRenderer::MakeParams(splashCfg, sub_10002BC0, sub_10002F60, dword_100B2C68);

    SplashTextRenderer::Instance().render(splash);
}
