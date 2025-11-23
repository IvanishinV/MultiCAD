#pragma once

#include "ProfileBase.h"
#include "GameDllHooks.h"
#include "MenuDllHooks.h"
#include "cad.h"
#include "ScreenConfig.h"

constexpr uint16_t SCREEN_HEIGHT_TO_SHOW_UNITS = Graphics::kMaxHeight;   // to show top part of units below the screen
constexpr uint32_t ARRAY_37B588_DWORD_SIZE = Graphics::kMaxWidth;
constexpr uint32_t ARRAY_37B588_BYTE_SIZE = ARRAY_37B588_DWORD_SIZE * sizeof(uint32_t);

constexpr std::array relocs_game_ss_gold_en
{
    RelocateGapSpec{0x0, 0x0, 0x0},
    /*
    * (width + 7) >> 4, (height + 7) >> 3 and array 0x34FF20 with size:
    * 1024 * 768:  1800h = 4 * kRowStrideOldDwordSize * ((300h + 7) >> 3)
    * 1080 * 1920: 21C0h = 4 * kRowStrideDwordSize * ((438h + 7) >> 3)
    * 2560 * 1440: 2D00h = 4 * kRowStrideDwordSize * ((5A0h + 7) >> 3)
    */
    RelocateGapSpec{ 0x0034FF20, 0x00351728, 8 + sizeof(uint32_t) * kRowStrideDwordSize * ((Graphics::kMaxHeight + 7) >> 3) },
    /*
    * (width + 7) / 16, (height + 7) / 8 and array 0x351728 with size:
    * 1024 * 768:  1800h = 4 * kRowStrideOldDwordSize * ((300h + 7) >> 3)
    * 1080 * 1920: 21C0h = 4 * kRowStrideDwordSize * ((438h + 7) >> 3)
    * 2560 * 1440: 2D00h = 4 * kRowStrideDwordSize * ((5A0h + 7) >> 3)
    */
    RelocateGapSpec{ 0x00351728, 0x00352F2A, 8 + sizeof(uint32_t) * kRowStrideDwordSize * ((Graphics::kMaxHeight + 7) >> 3) },
    /*
    * Helper array with size:
    * 1024: 1000h = 400h * 4 = 1024 * 4
    * 1920: 1E00h = 780h * 4 = 1920 * 4
    */
    RelocateGapSpec{ 0x0037B588, 0x0037C588, ARRAY_37B588_BYTE_SIZE },
    /*
    * Variables needed for previous helper array.
    * This memory should come immediately after the previous one.
    */
    RelocateGapSpec{ 0x0037C588, 0x0037C596, 0x10 },
    /*
    * Helper array for fog calculation with size of ModuleState.fogSprites + 2 bytes.
    * This memory starts from 96, but previous array has int starting on 94. May be the developers
    * wanted to use the high word of 94 int value in helper array. So I left the same.
    */
    RelocateGapSpec{ 0x0037C596, 0x0037E898, 2 + sizeof(((ModuleState*)0)->fogSprites) },
};

const std::array hooks_game_ss_gold_en
{
    HookSpec{0x0, 0x0},
    HookSpec{0x55A20, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055A20)},
    HookSpec{0x55DC0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055DC0)},
    HookSpec{0x55E00, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055E00)},
    HookSpec{0x55E90, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055E90)},
    HookSpec{0x55F40, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055F40)},
    HookSpec{0x55FE0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055FE0)},
    HookSpec{0x56030, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10056030)},
    HookSpec{0x56170, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10056170)},
    HookSpec{0x563B0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_100563B0)},
    HookSpec{0x6AD20, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006AD20)},
    HookSpec{0x6AEA0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006AEA0)},
    HookSpec{0x6B1C0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006B1C0)},
    HookSpec{0x6B2C0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006B2C0)},
    HookSpec{0x6D940, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006D940)},
    HookSpec{0x6F120, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006F120)},

    // Fixes a null pointer dereference (issue reported via dump from Иван 'Alee' Петров)
    HookSpec{0x3E7B0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1003E7B0)},
    // Fixes a null pointer dereferences (issues reported via dumps from Eugin 'Bulldozer' Banks)
    HookSpec{0x6CC60, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006CC60)},
    HookSpec{0x5C170, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1005C170)},
};

const std::array patches_game_ss_gold_en
{
    PatchSpec{0x0, {}},

    // Fixes bug where enemies inside buildings were revealed by your supply trucks
    PatchSpec{0x1FE41, {0x75, 0xB1}},   // jmp 1FDF4
    PatchSpec{0x1FDF4,
    {
        0x80, 0x7E, 0x00, 0x68,         // cmp byte ptr [esi+0], 68h
        0x74, 0x49,                     // jz 1FE43
        0xEB, 0x77                      // jmp 1FE73
    }},

    // Sets the screen height at which units are displayed
    PatchSpec{0x6D147, PatchSpec::to_bytes(SCREEN_HEIGHT_TO_SHOW_UNITS)},

    // Fixes building selection to account changes in CAD
    PatchSpec{0x6E008, {kStencilPixelColorShift}},
    PatchSpec{0x6E009, PatchSpec::concat(
        {0x81, 0xEE}, kStencilPixelOffset       // sub esi, 440h
    )},
    PatchSpec{0x6E00F, {0xEB, 0x81}},           // jmp 6E00F -> 6DF92
    PatchSpec{0x6DF92, PatchSpec::concat(
        {0x81, 0xE6}, kStencilPixelSmallMask,   // and esi, 7FFh
        std::array<uint8_t, 2>{0xEB, 0x78}      // jmp 6DF98 -> 6E012
    )},

    // Fixes due to changed array sizes
    PatchSpec{0x6E42C, PatchSpec::to_bytes(ARRAY_37B588_DWORD_SIZE)},
    PatchSpec{0x6E472, PatchSpec::to_bytes(ARRAY_37B588_DWORD_SIZE)},
    PatchSpec{0x713CE, PatchSpec::to_bytes(Graphics::kMaxHeight)},
    PatchSpec{0x71E24, {0x37}},             // jg 71E5C
    PatchSpec{0x71E29, {kRowStrideShift}}, // shl esi, 6
    PatchSpec{0x71E53, PatchSpec::concat(
        {0x81, 0xC6}, kRowStrideByteSize, // add esi, 40h
        std::array<uint8_t, 11>
        {
            0x4A,                   // dec edx
            0x75, 0xD9,             // jnz 71E35
            0x5F, 0x5E, 0x5D, 0x5B, // pop edi, esi, ebp, ebx
            0x83, 0xC4, 0x34,       // add esp, 34h
            0xC3                    // retn
        }
    )},

    // Fixes unit selection when double-clicking
    PatchSpec{0x7FE64, {}, 0x7FE65, sizeof(uint32_t)},  // copy already calculated global variable address
    PatchSpec{0x7FE63, {0xBA}},             // mov edx, offset yBottom
    PatchSpec{0x7FE68, {0x3B, 0x4A, 0xFC}}, // cmp ecx, [edx-4]     // screen height
    PatchSpec{0x7FE6D, {0x3B, 0x02}},       // cmp eax, [edx]       // screen width
};

constexpr std::array relocs_menu_ss_gold_en
{
    RelocateGapSpec{0x0, 0x0, 0x0}
};

const std::array hooks_menu_ss_gold_en
{
    HookSpec{0x0, 0x0},
    HookSpec{0x14B70, reinterpret_cast<uintptr_t>(&MenuDllHooks::sub_10014B70)},
};

const std::array patches_menu_ss_gold_en
{
    PatchSpec{0x0, {}},
};

using Profile_SS_GOLD_EN = GameVersionProfile<
    GameVersion::SS_GOLD_EN,
    relocs_game_ss_gold_en,
    hooks_game_ss_gold_en,
    patches_game_ss_gold_en,
    relocs_menu_ss_gold_en,
    hooks_menu_ss_gold_en,
    patches_menu_ss_gold_en
>;
