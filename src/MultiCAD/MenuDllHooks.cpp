#include "pch.h"
#include "MenuDllHooks.h"
#include "resource.h"
#include "version.h"

void __fastcall MenuDllHooks::sub_10014B70(void* self)
{
    auto* g = globals_;

    const auto sub_100020A0 = g->getFn<void(__thiscall)(void*)>(0x20A0);
    const auto sub_100039F0 = g->getFn<void(__thiscall)(void*, int, int, int)>(0x39F0);
    const auto sub_10017480 = g->getFn<void(__thiscall)(void*)>(0x17480);
    const auto sub_10003D10 = g->getFn<void(__thiscall)(void*, int, int, const char*, int)>(0x3D10);
    int* dword_100590F8 = g->getPtr<int>(0x590F8);

    sub_100020A0(self);
    sub_100039F0(dword_100590F8, 0xDE, 0xD7, 0x42);

    // Last parameter points if the text is written from right to left.
    // Otherwise, it's centered at the specified offset 
    sub_10003D10(dword_100590F8, 605, 357, "Multi HD mod v" MULTICAD_VERSION_STR " for " SS_GOLD_GAME_STR, 1);
    sub_10003D10(dword_100590F8, 605, 370, SS_HD_MOD_TG_LINK, 1);
    sub_10003D10(dword_100590F8, 605, 383, SS_HD_MOD_AUTHOR_EMAIL, 1);

    sub_10017480(&self);
}
