#include "pch.h"
#include "MenuDllHooks.h"
#include "resource.h"
#include "version.h"
#include "SplashTextRenderer.h"
#include "ScreenConfig.h"
#include "MatchStatsReader.h"
#include "OutcomeHook.h"
#include "StatsReporter.h"

#include <cstdio>
#include <cstring>

struct SplashLayout
{
    int titleX;
    int titleY;

    const char* gameStr1;
    const char* gameStr2;

    SplashVariant variant;

    int flagPtr;
    int resetPtr1;
    int resetPtr2;

    int initFnPtr;
    int selfFnPtr;
    int setColorFnPtr;
    int writeTextFnPtr;

    int surfacePtr;

    // Top-right position of the "WIDTHxHEIGHT" label.
    int resX;
    int resY;
};

void MenuDllHooks::renderGameVersion(void* self, const SplashLayout& layout)
{
    auto* g = globals_;

    if (layout.flagPtr)
    {
        int* flagPtr = g->getPtr<int>(layout.flagPtr);
        const auto initFn = g->getFn<void(__thiscall)()>(layout.initFnPtr);

        if (*flagPtr != 0)
        {
            initFn();
            *flagPtr = 1;

            *g->getPtr<int>(layout.resetPtr1) = 0;
            *g->getPtr<int>(layout.resetPtr2) = 0;
        }
    }

    g->getFn<void(__thiscall)(void*)>(layout.selfFnPtr)(self);

    int* surface = g->getPtr<int>(layout.surfacePtr);
    const auto setColorFn = g->getFn<void(__thiscall)(void*, int, int, int)>(layout.setColorFnPtr);
    const auto writeTextFn = g->getFn<void(__thiscall)(void*, int, int, const char*, int)>(layout.writeTextFnPtr);

    const auto& splashCfg = g_splashTable[(int)layout.variant];
    setColorFn(surface, splashCfg.r, splashCfg.g, splashCfg.b);

    int y = layout.titleY;
    writeTextFn(surface, layout.titleX, y, "Multi HD mod v" MULTICAD_VERSION_STR " for", 1);
    y += 13;
    writeTextFn(surface, layout.titleX, y, layout.gameStr1, 1);

    if (layout.gameStr2)
    {
        y += 13;
        writeTextFn(surface, layout.titleX, y, layout.gameStr2, 1);
    }

    y += 26;
    writeTextFn(surface, layout.titleX, y, SS_HD_MOD_TG_LINK, 1);
    y += 13;
    writeTextFn(surface, layout.titleX, y, SS_HD_MOD_AUTHOR_EMAIL, 1);

    // Show active game resolution
    if (layout.resX != 0 || layout.resY != 0)
    {
        S32 resWidth, resHeight;
        Screen::ResolveTargetResolution(resWidth, resHeight);

        char resBuf[32];
        std::snprintf(resBuf, sizeof(resBuf), "%dx%d", resWidth, resHeight);
        writeTextFn(surface, layout.resX, layout.resY, resBuf, 0);
    }

    const SplashTextRenderer::Params splash =
        SplashTextRenderer::MakeParams(splashCfg, setColorFn, writeTextFn, surface);

    SplashTextRenderer::Instance().render(splash);
}

void __fastcall MenuDllHooks::sub_10014B70(void* self)
{
    static const SplashLayout layout
    {
        605,
        344,
        SS_GOLD_GAME_STR,
        nullptr,
        SplashVariant::SS,
        0,
        0,
        0,
        0,
        0x20A0,
        0x39F0,
        0x3D10,
        0x590F8,
        553,
        125,
    };

    renderGameVersion(self, layout);
}

void __fastcall MenuDllHooks::sub_10014B70_hd(void* self)
{
    static const SplashLayout layout
    {
        617,
        344,
        SS_GOLD_HD_GAME_STR,
        nullptr,
        SplashVariant::SS,
        0,
        0,
        0,
        0,
        0x20A0,
        0x39F0,
        0x3D10,
        0x590F8,
        553,
        125,
    };

    renderGameVersion(self, layout);
}

void __fastcall MenuDllHooks::sub_10014B70_fr(void* self)
{
    static const SplashLayout layout
    {
        605,
        344,
        SS_GOLD_GAME_STR,
        nullptr,
        SplashVariant::SS,
        0,
        0,
        0,
        0,
        0x20D0,
        0x3A20,
        0x3D40,
        0x59118,
        553,
        125,
    };

    renderGameVersion(self, layout);
}

void __fastcall MenuDllHooks::sub_10014B70_ru(void* self)
{
    static const SplashLayout layout
    {
        640,
        344,
        SS_GOLD_GAME_STR,
        nullptr,
        SplashVariant::SS,
        0,
        0,
        0,
        0,
        0x20D0,
        0x3BA0,
        0x3EC0,
        0x580F8,
        553,
        125,
    };

    renderGameVersion(self, layout);
}

void __fastcall MenuDllHooks::sub_1000E3D0_ru(void* self)
{
    static const SplashLayout layout
    {
        640,
        344,
        SS_GAME_STR,
        nullptr,
        SplashVariant::SS,
        0,
        0,
        0,
        0,
        0x1FE0,
        0x3960,
        0x3CA0,
        0x4C268,
        553,
        125,
    };

    renderGameVersion(self, layout);
}

void __fastcall MenuDllHooks::sub_1000E3D0_hd_ru(void* self)
{
    static const SplashLayout layout
    {
        640,
        344,
        SS_HD_GAME_STR,
        nullptr,
        SplashVariant::SS,
        0,
        0,
        0,
        0,
        0x1FE0,
        0x3960,
        0x3CA0,
        0x4C268,
        553,
        125,
    };

    renderGameVersion(self, layout);
}

void __fastcall MenuDllHooks::sub_1000E3D0_hd_en(void* self)
{
    static const SplashLayout layout
    {
        617,
        344,
        SS_HD_GAME_STR,
        nullptr,
        SplashVariant::SS,
        0,
        0,
        0,
        0,
        0x1FE0,
        0x3960,
        0x3CA0,
        0x4C268,
        553,
        125,
    };

    renderGameVersion(self, layout);
}

void __fastcall MenuDllHooks::sub_1000F2D0_en(void* self)
{
    static const SplashLayout layout
    {
        605,
        344,
        SS_V1_2_GAME_STR,
        nullptr,
        SplashVariant::SS,
        0,
        0,
        0,
        0,
        0x1F80,
        0x39F0,
        0x3D10,
        0x4FAA8,
        553,
        125,
    };

    renderGameVersion(self, layout);
}

void __fastcall MenuDllHooks::sub_1001AC60(void* self)
{
    static const SplashLayout layout
    {
        780,
        498,
        SS_2_V2_2_GAME_STR,
        nullptr,
        SplashVariant::SS_2,
        0xA06A0,
        0xB6F3C,
        0xB6F40,
        0x75F80,
        0xD1B0,
        0x2C00,
        0x2FA0,
        0xB4B48,
        750,
        30,
    };

    renderGameVersion(self, layout);
}

void __fastcall MenuDllHooks::sub_1001AC60_hs(void* self)
{
    static const SplashLayout layout
    {
        780,
        498,
        SS_HS_GAME_STR,
        nullptr,
        SplashVariant::SS_2,
        0xA06A0,
        0xB6F3C,
        0xB6F40,
        0x75F80,
        0xD1B0,
        0x2C00,
        0x2FA0,
        0xB4B48,
        750,
        30,
    };

    renderGameVersion(self, layout);
}

void __fastcall MenuDllHooks::sub_1001AC60_fmrm(void* self)
{
    static const SplashLayout layout
    {
        780,
        498,
        FMRM_GAME_STR,
        nullptr,
        SplashVariant::SS_2,
        0xA06A0,
        0xB6F3C,
        0xB6F40,
        0x75F80,
        0xD1B0,
        0x2C00,
        0x2FA0,
        0xB4B48,
        750,
        30,
    };

    renderGameVersion(self, layout);
}

// Vtable of the multiplayer results screen, the same RVA in both menu builds.
constexpr uintptr_t kMultiplayerResultsVtableRva = 0x000905DC;

// No-op unless StatsUrl is set.
static void TryReportMatch(GameGlobals& globals, const Stats::StatsLayout& layout)
{
    if (!Stats::IsReportingEnabled())
        return;

    MatchStats match;
    if (Stats::ReadMatch(globals, layout, match))
        Stats::Submit(match);
}

// Stands in for the generic "hide" composite screens use, transcribed from the
// original (0xD190 v2.2, 0xD390 RW2.4): base teardown, then fan slot 2 to each
// child. The additions are the lobby snapshot and the match report.
void MenuDllHooks::hideAndReport(void* self, const uintptr_t baseTeardownRva,
                                 const Stats::StatsLayout& layout)
{
    auto* g = globals_;

    g->getFn<void(__fastcall)(void*)>(baseTeardownRva)(self);

    const auto moduleBase = reinterpret_cast<uintptr_t>(g->getPtr<const void>(0));
    const auto screen = *reinterpret_cast<uintptr_t*>(self) - moduleBase;

    // Leaving a screen is the last chance to read the lobby before the match
    // unloads this dll and takes the table with it.
    Stats::CaptureLobby(*g, layout);

    // The results screen closes through here. Its own slot 2 is a shared thunk
    // that cannot be hooked without catching other screens, hence the vtable
    // test. The session outlives the base teardown above; the read only has
    // to come before the children are hidden below.
    if (screen == kMultiplayerResultsVtableRva)
        TryReportMatch(*g, layout);

    // The original's own work: walk the child list and fan slot 2 to each child.
    // Unaligned by design - the list head is at +5, each node's successor at +4.
    auto* node = *reinterpret_cast<U8**>(static_cast<U8*>(self) + 5);

    for (; node != nullptr; node = *reinterpret_cast<U8**>(node + 4))
    {
        void* child = *reinterpret_cast<void**>(node);
        if (child == nullptr)
            continue;

        const auto vtable = *reinterpret_cast<uintptr_t*>(child);

        (*reinterpret_cast<void(__fastcall**)(void*)>(vtable + 8))(child);
    }
}

void __fastcall MenuDllHooks::sub_1000D190(void* self)
{
    hideAndReport(self, 0x0000BC20, Stats::kLayoutSs2V22);
}

void __fastcall MenuDllHooks::sub_1000D390(void* self)
{
    hideAndReport(self, 0x0000BDF0, Stats::kLayoutRwV24);
}

void __fastcall MenuDllHooks::sub_1001AC60_bs_eu_2015(void* self)
{
    static const SplashLayout layout
    {
        780,
        485,
        SS_EUROPE_2015_V1_0_GAME_STR,
        SS_BLACK_SEA_V1_2_GAME_STR,
        SplashVariant::SS_BS,
        0xA06A0,
        0xB6F3C,
        0xB6F40,
        0x75F80,
        0xD1B0,
        0x2C00,
        0x2FA0,
        0xB4B48,
        750,
        30,
    };

    renderGameVersion(self, layout);
}

void __fastcall MenuDllHooks::sub_1001B470(void* self)
{
    static const SplashLayout layout
    {
        780,
        498,
        SS_RW_V2_3_GAME_STR,
        nullptr,
        SplashVariant::SS_2,
        0x9E740,
        0xB4FFC,
        0xB5000,
        0x76F60,
        0xD460,
        0x2BD0,
        0x2F70,
        0xB2BE8,
        750,
        30,
    };

    renderGameVersion(self, layout);
}

void __fastcall MenuDllHooks::sub_1001B380(void* self)
{
    static const SplashLayout layout
    {
        780,
        498,
        SS_RW_V2_4_GAME_STR,
        nullptr,
        SplashVariant::SS_2,
        0x9E7C0,
        0xB507C,
        0xB5080,
        0x76E90,
        0xD3B0,
        0x2BC0,
        0x2F60,
        0xB2C68,
        750,
        30,
    };

    renderGameVersion(self, layout);
}
