#pragma once

#include "ProfileBase.h"
#include "GameDllHooks.h"
#include "MenuDllHooks.h"
#include "cad.h"
#include "ScreenConfig.h"

#include <array>

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
    RelocateGapSpec{ 0x00351728, 0x00352F30, 8 + sizeof(uint32_t) * kRowStrideDwordSize * ((Graphics::kMaxHeight + 7) >> 3) },
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
    RelocateGapSpec{ 0x0037C596, 0x0037E898, 2 + sizeof(((ModuleStateBase*)0)->fogSprites) },
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
    HookSpec{0x1D240, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1001D240)},
    HookSpec{0x6CC60, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006CC60)},
    HookSpec{0x5C170, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1005C170)},
    HookSpec{0x99E01, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10099E01)},
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
    PatchSpec{0x71CE0, {0x78}},             // jg 71E5C
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


constexpr std::array relocs_game_ss_gold_de_ru
{
    RelocateGapSpec{ 0x0034FED8, 0x003516E0, 8 + sizeof(uint32_t) * kRowStrideDwordSize * ((Graphics::kMaxHeight + 7) >> 3) },
    RelocateGapSpec{ 0x003516E0, 0x00352EE8, 8 + sizeof(uint32_t) * kRowStrideDwordSize * ((Graphics::kMaxHeight + 7) >> 3) },
    RelocateGapSpec{ 0x0037B53C, 0x0037C53C, ARRAY_37B588_BYTE_SIZE },
    RelocateGapSpec{ 0x0037C53C, 0x0037C552, 0x10 },
    RelocateGapSpec{ 0x0037C552, 0x0037E858, 2 + sizeof(((ModuleStateBase*)0)->fogSprites) },
};

const std::array hooks_game_ss_gold_de_ru
{
    HookSpec{0x57F70, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055A20)},
    HookSpec{0x58330, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055DC0)},
    HookSpec{0x58370, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055E00)},
    HookSpec{0x58400, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055E90)},
    HookSpec{0x584B0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055F40)},
    HookSpec{0x58450, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055FE0)},
    HookSpec{0x58580, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10056030)},
    HookSpec{0x586B0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10056170)},
    HookSpec{0x588E0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_100563B0)},
    //HookSpec{0x6AD20, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006AD20)},
    //HookSpec{0x6AEA0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006AEA0)},
    //HookSpec{0x6B1C0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006B1C0)},
    //HookSpec{0x6B2C0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006B2C0)},
    //HookSpec{0x6D940, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006D940)},
    HookSpec{0x71A00, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006F120_de)},

    // Fixes a null pointer dereference (issue reported via dump from Иван 'Alee' Петров)
    HookSpec{0x3FB20, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1003E7B0_de)},
    // Fixes a null pointer dereferences (issues reported via dumps from Eugin 'Bulldozer' Banks)
    HookSpec{0x1D9E0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1001D240)},
    HookSpec{0x6F530, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006CC60_de)},
    HookSpec{0x5C170, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1005C170_de)},
    HookSpec{0x9CD23, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10099E01_de)},
};

const std::array patches_game_ss_gold_de_ru
{
    // Fixes bug where enemies inside buildings were revealed by your supply trucks
    PatchSpec{0x20601, {0x75, 0xB4}},   // jmp 20601 -> 205B7
    PatchSpec{0x205B7,
    {
        0x80, 0x7E, 0x00, 0x68,         // cmp byte ptr [esi+0], 68h
        0x74, 0x46,                     // jz 20603
        0xEB, 0x74                      // jmp 20633
    }},

    // Sets the screen height at which units are displayed
    PatchSpec{0x6FA17, PatchSpec::to_bytes(SCREEN_HEIGHT_TO_SHOW_UNITS)},

    // Fixes building selection to account changes in CAD
    PatchSpec{0x70868, {kStencilPixelColorShift}},
    PatchSpec{0x70869, PatchSpec::concat(
        {0x81, 0xEF}, kStencilPixelOffset       // sub edi, 440h
    )},
    PatchSpec{0x7086F, {0xEB, 0x81}},           // jmp 7086F -> 707F2
    PatchSpec{0x707F2, PatchSpec::concat(
        {0x81, 0xE7}, kStencilPixelSmallMask,   // and edi, 7FFh
        std::array<uint8_t, 2>{0xEB, 0x78}      // jmp 707F8 -> 70872
    )},

    // Fixes due to changed array sizes
    PatchSpec{0x70CBC, PatchSpec::to_bytes(ARRAY_37B588_DWORD_SIZE)},
    PatchSpec{0x70D02, PatchSpec::to_bytes(ARRAY_37B588_DWORD_SIZE)},
    PatchSpec{0x73C2E, PatchSpec::to_bytes(Graphics::kMaxHeight)},
    PatchSpec{0x6DC81, {0x42}},             // jg 6DCC1
    PatchSpec{0x6DC8C, {kRowStrideShift}},  // shl esi, 6
    PatchSpec{0x6DCB7, PatchSpec::concat(
        {0x81, 0xC6}, kRowStrideByteSize,   // add esi, 40h
        std::array<uint8_t, 13>
        {
            0x48,                   // dec eax
            0x89, 0x44, 0x24, 0x10, // mov [esp+0Ch+arg_0], eax
            0x75, 0xD2,             // jnz 6DC96
            0x5F, 0x5E, 0x5D,       // pop edi, esi, ebp
            0xC2, 0x1C, 0x00        // retn 1Ch
        }
    )},

    // Fixes unit selection when double-clicking
    PatchSpec{0x82AA6, {}, 0x82AA7, sizeof(uint32_t)},
    PatchSpec{0x82AA5, {0xBA}},
    PatchSpec{0x82AAA, {0x3B, 0x4A, 0xFC}},
    PatchSpec{0x82AAF, {0x3B, 0x02}},
};


constexpr std::array relocs_menu_ss_gold_ru
{
    RelocateGapSpec{0x0, 0x0, 0x0}
};

const std::array hooks_menu_ss_gold_ru
{
    HookSpec{0x0, 0x0},
    HookSpec{0x15540, reinterpret_cast<uintptr_t>(&MenuDllHooks::sub_10014B70_ru)},
};

const std::array patches_menu_ss_gold_ru
{
    PatchSpec{0x0, {}},
};


constexpr std::array relocs_game_ss_gold_fr
{
    RelocateGapSpec{ 0x00353EF8, 0x00355700, 8 + sizeof(uint32_t) * kRowStrideDwordSize * ((Graphics::kMaxHeight + 7) >> 3) },
    RelocateGapSpec{ 0x00355700, 0x00356F08, 8 + sizeof(uint32_t) * kRowStrideDwordSize * ((Graphics::kMaxHeight + 7) >> 3) },
    RelocateGapSpec{ 0x0037F55C, 0x0038055C, ARRAY_37B588_BYTE_SIZE },
    RelocateGapSpec{ 0x0038055C, 0x00380752, 0x10 },
    RelocateGapSpec{ 0x00380752, 0x00382878, 2 + sizeof(((ModuleStateBase*)0)->fogSprites) },
};

const std::array hooks_game_ss_gold_fr
{
    HookSpec{0x0, 0x0},
    HookSpec{0x57EF0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055A20)},
    HookSpec{0x58290, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055DC0)},
    HookSpec{0x582D0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055E00)},
    HookSpec{0x58360, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055E90)},
    HookSpec{0x58410, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055F40)},
    HookSpec{0x584B0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055FE0)},
    HookSpec{0x58500, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10056030)},
    HookSpec{0x58640, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10056170)},
    HookSpec{0x58880, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_100563B0)},
    //HookSpec{0x6AD20, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006AD20)},
    //HookSpec{0x6AEA0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006AEA0)},
    //HookSpec{0x6B1C0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006B1C0)},
    //HookSpec{0x6B2C0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006B2C0)},
    //HookSpec{0x6D940, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006D940)},
    HookSpec{0x71A10, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006F120_fr)},

    // Fixes a null pointer dereference (issue reported via dump from Иван 'Alee' Петров)
    HookSpec{0x3FB20, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1003E7B0_fr)},
    // Fixes a null pointer dereferences (issues reported via dumps from Eugin 'Bulldozer' Banks)
    HookSpec{0x1D930, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1001D240)},
    HookSpec{0x6F540, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006CC60_fr)},
    HookSpec{0x5E810, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1005C170_fr)},
    HookSpec{0x9D611, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10099E01_fr)},
};

const std::array patches_game_ss_gold_fr
{
    // Fixes bug where enemies inside buildings were revealed by your supply trucks
    PatchSpec{0x20551, {0x75, 0xB4}},   // jmp 20507
    PatchSpec{0x20507,
    {
        0x80, 0x7E, 0x00, 0x68,         // cmp byte ptr [esi+0], 68h
        0x74, 0x46,                     // jz 20553
        0xEB, 0x74                      // jmp 20583
    }},

    // Sets the screen height at which units are displayed
    PatchSpec{0x6FA27, PatchSpec::to_bytes(SCREEN_HEIGHT_TO_SHOW_UNITS)},

    // Fixes building selection to account changes in CAD
    PatchSpec{0x70878, {kStencilPixelColorShift}},
    PatchSpec{0x70879, PatchSpec::concat(
        {0x81, 0xEF}, kStencilPixelOffset       // sub edi, 440h
    )},
    PatchSpec{0x7087F, {0xEB, 0x81}},           // jmp 7087F -> 70802
    PatchSpec{0x70802, PatchSpec::concat(
        {0x81, 0xE7}, kStencilPixelSmallMask,   // and edi, 7FFh
        std::array<uint8_t, 2>{0xEB, 0x78}      // 70808 -> 70882
    )},

    // Fixes due to changed array sizes
    PatchSpec{0x70CCC, PatchSpec::to_bytes(ARRAY_37B588_DWORD_SIZE)},
    PatchSpec{0x70D12, PatchSpec::to_bytes(ARRAY_37B588_DWORD_SIZE)},
    PatchSpec{0x73C4E, PatchSpec::to_bytes(Graphics::kMaxHeight)},
    PatchSpec{0x6DD01, {0x42}},            // jg 6DD41
    PatchSpec{0x6DD0C, {kRowStrideShift}}, // shl esi, 6
    PatchSpec{0x6DD37, PatchSpec::concat(
        {0x81, 0xC6}, kRowStrideByteSize,  // add esi, 40h
        std::array<uint8_t, 14>
        {
            0x48,                   // dec eax
            0x89, 0x44, 0x24, 0x10, // mov [esp+0Ch+arg_0], eax
            0x75, 0xD2,             // jnz 71E35
            0x5F, 0x5E, 0x5D,       // pop edi, esi, ebp
            0xC2, 0x1C, 0x00        // retn 1Ch
        }
    )},

    // Fixes unit selection when double-clicking
    PatchSpec{0x82EB6, {}, 0x82EB7, sizeof(uint32_t)},  // copy already calculated global variable address
    PatchSpec{0x82EB5, {0xBA}},             // mov edx, offset yBottom
    PatchSpec{0x82EBA, {0x3B, 0x4A, 0xFC}}, // cmp ecx, [edx-4]     // screen height
    PatchSpec{0x82EBF, {0x3B, 0x02}},       // cmp eax, [edx]       // screen width
};

constexpr std::array relocs_menu_ss_gold_fr
{
    RelocateGapSpec{0x0, 0x0, 0x0}
};

const std::array hooks_menu_ss_gold_fr
{
    HookSpec{0x14CB0, reinterpret_cast<uintptr_t>(&MenuDllHooks::sub_10014B70_fr)},
};

const std::array patches_menu_ss_gold_fr
{
    PatchSpec{0x0, {}},
};


constexpr std::array relocs_game_ss_gold_hd_1_2
{
    RelocateGapSpec{ 0x003A8000, 0x003AD000, 8 + sizeof(uint32_t) * kRowStrideDwordSize * ((Graphics::kMaxHeight + 7) >> 3) },
    RelocateGapSpec{ 0x003AD000, 0x003B2000, 8 + sizeof(uint32_t) * kRowStrideDwordSize * ((Graphics::kMaxHeight + 7) >> 3) },
    RelocateGapSpec{ 0x003BC000, 0x003BD800, ARRAY_37B588_BYTE_SIZE },
    RelocateGapSpec{ 0x003BD800, 0x003BD80E, 0x10 },
    RelocateGapSpec{ 0x003B2000, 0x003BC000, 2 + sizeof(((ModuleStateBase*)0)->fogSprites) },
};

const std::array hooks_game_ss_gold_hd_1_2
{
    HookSpec{0x55A20, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055A20)},
    HookSpec{0x55DC0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055DC0)},
    HookSpec{0x55E00, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055E00)},
    HookSpec{0x55E90, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055E90)},
    HookSpec{0x55F40, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055F40)},
    HookSpec{0x55FE0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055FE0)},
    HookSpec{0x56030, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10056030)},
    HookSpec{0x56170, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10056170)},
    HookSpec{0x563B0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_100563B0)},
    HookSpec{0x6AD20, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006AD20_hd)},
    HookSpec{0x6AEA0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006AEA0_hd)},
    HookSpec{0x6B1C0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006B1C0_hd)},
    HookSpec{0x6B2C0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006B2C0_hd)},
    HookSpec{0x6D940, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006D940_hd)},
    HookSpec{0x6F120, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006F120_hd)},

    //HookSpec{0x3E7B0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1003E7B0)},
    HookSpec{0x1D240, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1001D240)},
    HookSpec{0x6CC60, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006CC60)},
    HookSpec{0x5C170, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1005C170)},
    HookSpec{0x99E01, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10099E01)},
};

const std::array patches_game_ss_gold_hd_1_2
{
    PatchSpec{0x6D147, PatchSpec::to_bytes(SCREEN_HEIGHT_TO_SHOW_UNITS)},

    PatchSpec{0x6E008, {kStencilPixelColorShift}},
    PatchSpec{0x6E009, PatchSpec::concat(
        {0x81, 0xEE}, kStencilPixelOffset
    )},
    PatchSpec{0x6E00F, {0xEB, 0x81}},
    PatchSpec{0x6DF92, PatchSpec::concat(
        {0x81, 0xE6}, kStencilPixelSmallMask,
        std::array<uint8_t, 2>{0xEB, 0x78}
    )},

    // Fixes due to changed array sizes
    PatchSpec{0x6E42C, PatchSpec::to_bytes(ARRAY_37B588_DWORD_SIZE)},
    PatchSpec{0x6E472, PatchSpec::to_bytes(ARRAY_37B588_DWORD_SIZE)},
    PatchSpec{0x713CE, PatchSpec::to_bytes(Graphics::kMaxHeight)},
    // Part fix since the original HD mod already implemented this
    PatchSpec{0x71E29, {kRowStrideShift}},
    PatchSpec{0x71E55, PatchSpec::to_bytes(kRowStrideByteSize)},
};

constexpr std::array relocs_menu_ss_gold_hd_1_2
{
    RelocateGapSpec{0x0, 0x0, 0x0}
};

const std::array hooks_menu_ss_gold_hd_1_2
{
    HookSpec{0x14B70, reinterpret_cast<uintptr_t>(&MenuDllHooks::sub_10014B70_hd)},
};

const std::array patches_menu_ss_gold_hd_1_2
{
    // Remove the displayed text on main menu from the old HD mod
    PatchSpec{0x13931, { 0x8B, 0xF1, 0xE8, 0xB8, 0xE0, 0xFE, 0xFF, 0x8B, 0x4E, 0x05, 0x85, 0xC9, 0x5E, 0x74, 0x05, 0x8B, 0x01, 0xFF, 0x60, 0x0C, 0xC3 }},
};


constexpr std::array relocs_game_ss_ru
{
    RelocateGapSpec{ 0x0033D618, 0x0033EE20, 8 + sizeof(uint32_t) * kRowStrideDwordSize * ((Graphics::kMaxHeight + 7) >> 3) },
    RelocateGapSpec{ 0x0033EE20, 0x00340628, 8 + sizeof(uint32_t) * kRowStrideDwordSize * ((Graphics::kMaxHeight + 7) >> 3) },
    RelocateGapSpec{ 0x00368C7C, 0x00369C7C, ARRAY_37B588_BYTE_SIZE },
    RelocateGapSpec{ 0x00369C7C, 0x00369C92, 0x10 },
    RelocateGapSpec{ 0x00369C92, 0x0036BF98, 2 + sizeof(((ModuleStateBase*)0)->fogSprites) },
};

const std::array hooks_game_ss_ru
{
    HookSpec{0x498A0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055A20)},
    HookSpec{0x49C60, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055DC0)},
    HookSpec{0x49CA0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055E00)},
    HookSpec{0x49D40, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055E90)},
    HookSpec{0x49DF0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055F40)},
    HookSpec{0x49E80, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055FE0)},
    HookSpec{0x49EC0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10056030)},
    HookSpec{0x4A000, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10056170)},
    HookSpec{0x4A250, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_100563B0)},
    //HookSpec{0x6AD20, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006AD20)},
    //HookSpec{0x6AEA0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006AEA0)},
    //HookSpec{0x6B1C0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006B1C0)},
    //HookSpec{0x6B2C0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006B2C0)},
    //HookSpec{0x6D940, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006D940)},
    //HookSpec{0x6F120, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006F120)},
    HookSpec{0x5F0A0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006DC40)},
    HookSpec{0x62DC0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006F120_v1_0_ru)},
};

const std::array patches_game_ss_ru
{
    // Fixes bug where enemies inside buildings were revealed by your supply trucks
    PatchSpec{0x19E7E, {0x75, 0xB7}},   // jmp 19E37
    PatchSpec{0x19E37,
    {
        0x80, 0x7E, 0x00, 0x68,         // cmp byte ptr [esi+0], 68h
        0x74, 0x43,                     // jz 19E80
        0xEB, 0x67                      // jmp 19EA6
    }},

    // Sets the screen height at which units are displayed
    PatchSpec{0x60E59, PatchSpec::to_bytes(SCREEN_HEIGHT_TO_SHOW_UNITS)},

    // Fixes building selection to account changes in CAD
    PatchSpec{0x61C98, {kStencilPixelColorShift}},
    PatchSpec{0x61C99, PatchSpec::concat(
        {0x81, 0xEF}, kStencilPixelOffset       // sub edi, 440h
    )},
    PatchSpec{0x61C9F, {0xEB, 0x82}},           // jmp 61C9F -> 61C23
    PatchSpec{0x61C23, PatchSpec::concat(
        {0x81, 0xE7}, kStencilPixelSmallMask,   // and edi, 7FFh
        std::array<uint8_t, 2>{0xEB, 0x77}      // jmp 61C29 -> 61CA2
    )},

    // Fixes due to changed array sizes
    PatchSpec{0x620C2, PatchSpec::to_bytes(ARRAY_37B588_DWORD_SIZE)},
    PatchSpec{0x62112, PatchSpec::to_bytes(ARRAY_37B588_DWORD_SIZE)},
    PatchSpec{0x64FAE, PatchSpec::to_bytes(Graphics::kMaxHeight)},

    // Fixes unit selection when double-clicking
    PatchSpec{0x73158, {}, 0x73159, sizeof(uint32_t)},
    PatchSpec{0x73157, {0xBA}},
    PatchSpec{0x7315C, {0x3B, 0x4A, 0xFC}},
    PatchSpec{0x73161, {0x3B, 0x02}},
};

constexpr std::array relocs_menu_ss_ru
{
    RelocateGapSpec{0x0, 0x0, 0x0}
};

const std::array hooks_menu_ss_ru
{
    HookSpec{0x0, 0x0},
    HookSpec{0xE3D0, reinterpret_cast<uintptr_t>(&MenuDllHooks::sub_1000E3D0_ru)},
};

const std::array patches_menu_ss_ru
{
    PatchSpec{0x0, {}},
};


constexpr std::array relocs_game_ss_hd_v1_1
{
    RelocateGapSpec{ 0x00395000, 0x0039A000, 8 + sizeof(uint32_t) * kRowStrideDwordSize * ((Graphics::kMaxHeight + 7) >> 3) },
    RelocateGapSpec{ 0x0039A000, 0x0039F000, 8 + sizeof(uint32_t) * kRowStrideDwordSize * ((Graphics::kMaxHeight + 7) >> 3) },
    RelocateGapSpec{ 0x003A9000, 0x003AA800, ARRAY_37B588_BYTE_SIZE },
    RelocateGapSpec{ 0x003AA800, 0x003AA80E, 0x10 },
    RelocateGapSpec{ 0x0039F000, 0x003A9000, 2 + sizeof(((ModuleStateBase*)0)->fogSprites) },
};

const std::array hooks_game_ss_hd_v1_1
{
    HookSpec{0x498A0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055A20)},
    HookSpec{0x49C60, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055DC0)},
    HookSpec{0x49CA0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055E00)},
    HookSpec{0x49D40, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055E90)},
    HookSpec{0x49DF0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055F40)},
    HookSpec{0x49E80, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10055FE0)},
    HookSpec{0x49EC0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10056030)},
    HookSpec{0x4A000, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_10056170)},
    HookSpec{0x4A250, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_100563B0)},
    //HookSpec{0x6AD20, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006AD20)},
    //HookSpec{0x6AEA0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006AEA0)},
    //HookSpec{0x6B1C0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006B1C0)},
    //HookSpec{0x6B2C0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006B2C0)},
    //HookSpec{0x6D940, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006D940)},
    //HookSpec{0x6F120, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006F120)},
    HookSpec{0x62DC0, reinterpret_cast<uintptr_t>(&GameDllHooks::sub_1006F120_v1_0_hd)},
};

const std::array patches_game_ss_hd_v1_1
{
    // Sets the screen height at which units are displayed
    PatchSpec{0x60E59, PatchSpec::to_bytes(SCREEN_HEIGHT_TO_SHOW_UNITS)},

    // Fixes building selection to account changes in CAD
    PatchSpec{0x61C98, {kStencilPixelColorShift}},
    PatchSpec{0x61C99, PatchSpec::concat(
        {0x81, 0xEF}, kStencilPixelOffset       // sub edi, 440h
    )},
    PatchSpec{0x61C9F, {0xEB, 0x82}},           // jmp 61C9F -> 61C23
    PatchSpec{0x61C23, PatchSpec::concat(
        {0x81, 0xE7}, kStencilPixelSmallMask,   // and edi, 7FFh
        std::array<uint8_t, 2>{0xEB, 0x77}      // jmp 61C29 -> 61CA2
    )},

    // Fixes due to changed array sizes
    PatchSpec{0x620C2, PatchSpec::to_bytes(ARRAY_37B588_DWORD_SIZE)},
    PatchSpec{0x62112, PatchSpec::to_bytes(ARRAY_37B588_DWORD_SIZE)},
    PatchSpec{0x64FAE, PatchSpec::to_bytes(Graphics::kMaxHeight)},
    // Part fix since the original HD mod already implemented this
    PatchSpec{0x94672, {kRowStrideShift}},
    PatchSpec{0x946A1, PatchSpec::to_bytes(kRowStrideByteSize)},
};

constexpr std::array relocs_menu_ss_hd_v1_1
{
    RelocateGapSpec{0x0, 0x0, 0x0}
};

const std::array hooks_menu_ss_hd_v1_1_ru
{
    HookSpec{0xE3D0, reinterpret_cast<uintptr_t>(&MenuDllHooks::sub_1000E3D0_hd_ru)},
};

const std::array hooks_menu_ss_hd_v1_1_en
{
    HookSpec{0xE3D0, reinterpret_cast<uintptr_t>(&MenuDllHooks::sub_1000E3D0_hd_en)},
};

const std::array patches_menu_ss_hd_v1_1
{
    // Remove the displayed text on main menu from the old HD mod
    PatchSpec{0xD2F0, { 0x56, 0x8B, 0xF1, 0x8B, 0x4E, 0x05, 0x5E, 0x85, 0xC9, 0x74, 0x05, 0x8B, 0x01, 0xFF, 0x60, 0x0C, 0xC3 }},
};
