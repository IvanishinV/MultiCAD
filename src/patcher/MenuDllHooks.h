#pragma once

#include "types.h"
#include "DllHooksBase.h"

struct MenuTag {};
struct SplashLayout;

namespace Stats { struct StatsLayout; }

class MenuDllHooks : public DllHooksBase<MenuTag>
{
public:
    // Sudden Strike v1.21 based games
    static void __declspec(noinline) __fastcall sub_10014B70(void* self);
    static void __declspec(noinline) __fastcall sub_10014B70_hd(void* self);
    static void __declspec(noinline) __fastcall sub_10014B70_fr(void* self);
    static void __declspec(noinline) __fastcall sub_10014B70_ru(void* self);

    // Sudden Strike v1.0 based games
    static void __declspec(noinline) __fastcall sub_1000E3D0_ru(void* self);
    static void __declspec(noinline) __fastcall sub_1000E3D0_hd_ru(void* self);
    static void __declspec(noinline) __fastcall sub_1000E3D0_hd_en(void* self);

    // Sudden Strike v1.2 based games
    static void __declspec(noinline) __fastcall sub_1000F2D0_en(void* self);

    // Sudden Strike 2
    static void __declspec(noinline) __fastcall sub_1001AC60(void* self);

    // Hidden Stroke 2
    static void __declspec(noinline) __fastcall sub_1001AC60_hs(void* self);

    // Real War Game FMRM
    static void __declspec(noinline) __fastcall sub_1001AC60_fmrm(void* self);

    // Confrontation: Europe 2015
    static void __declspec(noinline) __fastcall sub_1001AC60_bs_eu_2015(void* self);

    // Sudden Strike: Resource War v2.3
    static void __declspec(noinline) __fastcall sub_1001B470(void* self);

    // Sudden Strike: Resource War v2.4
    static void __declspec(noinline) __fastcall sub_1001B380(void* self);

    // Generic composite-screen hide, and where a finished match is reported
    // from - see hideAndReport.
    static void __declspec(noinline) __fastcall sub_1000D190(void* self);   // v2.2 family
    static void __declspec(noinline) __fastcall sub_1000D390(void* self);   // Resource War v2.4

private:
    static void renderGameVersion(void* self, const SplashLayout& layout);
    static void hideAndReport(void* self, uintptr_t baseTeardownRva,
                              const Stats::StatsLayout& layout);
};