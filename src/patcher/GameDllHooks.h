#pragma once

#include "types.h"
#include "cad.h"
#include "DllHooksBase.h"

#include <vector>

// Replace and increase arrays
// 1034FF20-10351728
// 10351728-10352F2A
// 1037B588-1037C588
// 1037C588-1037C596        // Only move
// 1037C596-1037E894

// Full inject for CAD due to array increases
// 55A20    done
// 55DC0    done
// 55E00    done
// 55E90    done
// 55F40    done
// 55FE0    done
// 56030    done
// 56170    done
// 563B0    done
// 6AD20    done
// 6AEA0    done
// 6B1C0    done
// 6B2C0    done
// 6D940    done
// 6F120    done

// Only replace/move values due to array increases
// 6E3D0    done
// 6E460    done
// 71310    done
// 71CD0    done

// Additional fixes
// 6D0A0    done    // for correct objects displaying
// 6DFA0    done    // for correct selection of buildings

// Fix existing bugs
// 71310    done    // don't remember what exactly bug
// 7FD20    done    // selecting units under the screen by double click
// 1FE00    done    // bug with supply trucks and enemy buildings
// 3E7B0    done    // no nullptr check (dump sent by Alee)
// 5C170    done    // no nullptr check (dump sent by Bulldozer)
// 6CC60    done    // no nullptr check (dump sent by Bulldozer)
// 99E01    done    // freeing bad memory (dump sent by Bulldozer)

constexpr uint32_t kRowStrideOldDwordSize = 0x10;
constexpr uint32_t kRowStrideOldShift = 6;

constexpr uint32_t kRowStrideDwordSize = kRowStrideOldDwordSize * 4;   // increase by 4 to handle bigger resolution
constexpr uint32_t kRowStrideByteSize = kRowStrideDwordSize * sizeof(uint32_t);
constexpr uint8_t  kRowStrideShift = kRowStrideOldShift + 2;     // + sqrt(4)

constexpr uint32_t kFogLineByteSize = sizeof(Fog);
constexpr uint32_t kFogDoubleLineByteSize = kFogLineByteSize * 2;

struct GameTag {};

class GameDllHooks : public DllHooksBase<GameTag>
{
private:

#pragma region Helper_Structs

#pragma pack(push, 1)
    struct GameData
    {
        int alignX;
        int alignY;
        int allowX;
        int allowY;
        uint8_t mask;
        uint8_t maskValue;
        int x;
        int y;
        int maxX;
        int maxY;
    };
#pragma pack(pop)

    static_assert(offsetof(GameData, alignX) == 0, "alignX offset mismatch");
    static_assert(offsetof(GameData, alignY) == 4, "alignY offset mismatch");
    static_assert(offsetof(GameData, allowX) == 8, "allowX offset mismatch");
    static_assert(offsetof(GameData, allowY) == 12, "allowY offset mismatch");
    static_assert(offsetof(GameData, mask) == 16, "mask offset mismatch");
    static_assert(offsetof(GameData, maskValue) == 17, "maskValue offset mismatch");
    static_assert(offsetof(GameData, x) == 18, "x offset mismatch");
    static_assert(offsetof(GameData, y) == 22, "y offset mismatch");
    static_assert(offsetof(GameData, maxX) == 26, "maxX offset mismatch");
    static_assert(offsetof(GameData, maxY) == 30, "maxY offset mismatch");

    static_assert(sizeof(GameData) == 34, "GameData size mismatch");


#pragma pack(push, 1)
    struct GameData2
    {
        int alignX;
        int alignY;
        int allowX;
        int allowY;
        uint8_t cellMask;
        uint8_t mask;
        uint8_t maskValue;
        int x;
        int y;
        int maxX;
        int maxY;
    };
#pragma pack(pop)

    static_assert(offsetof(GameData2, alignX) == 0, "alignX offset mismatch");
    static_assert(offsetof(GameData2, alignY) == 4, "alignY offset mismatch");
    static_assert(offsetof(GameData2, allowX) == 8, "allowX offset mismatch");
    static_assert(offsetof(GameData2, allowY) == 12, "allowY offset mismatch");
    static_assert(offsetof(GameData2, cellMask) == 16, "cellMask offset mismatch");
    static_assert(offsetof(GameData2, mask) == 17, "mask offset mismatch");
    static_assert(offsetof(GameData2, maskValue) == 18, "maskValue offset mismatch");
    static_assert(offsetof(GameData2, x) == 19, "x offset mismatch");
    static_assert(offsetof(GameData2, y) == 23, "y offset mismatch");
    static_assert(offsetof(GameData2, maxX) == 27, "maxX offset mismatch");
    static_assert(offsetof(GameData2, maxY) == 31, "maxY offset mismatch");

    static_assert(sizeof(GameData2) == 35, "GameData2 size mismatch");


#pragma pack(push, 1)
    struct GameObject
    {
        char pad_1[32];
        int param_20;
        char pad_4[148];
        int param_B8;
        char pad_2[5475];
        int param_161F;
        char pad_5[226];
        int param_1705;
        int param_1709;
        char pad_3[40];
        int param_1735;
        int param_1739;
        int param_173d;
        int param_1741;
    };

    struct UnkEntry
    {
        GameObject* objPtr;
        uint16_t param_4;
        uint16_t param_6;
        uint16_t counter;
        uint16_t value;
        char pad[7];
    };
#pragma pack(pop)

    static_assert(sizeof(GameObject) == 5957, "GameObject size mismatch");
    static_assert(sizeof(UnkEntry) == 19, "UnkEntry size mismatch");


#pragma pack(push, 1)
    struct GameData3
    {
        int* param_00;
        int param_04;
        int param_08;
        int param_0C;
        int param_10;
        int param_14;
        int param_18;
        unsigned int* param_1C;
        int param_20;
        int param_24;
        int param_28;
        int param_2C;
        int param_30;
        int param_34;
        int param_38;
        int param_3C;
        int param_40;
        int param_44;
        int param_48;
        int param_4C;
        int param_50;
        int param_54;
        int param_58;
        int param_5C;
        int* param_60;
        unsigned int param_64;
        int param_68;
        int param_6C;
        int param_70;
        int param_74;
        int param_78;
        int param_7C;
        int param_80;
        int param_84;
        int param_88;
        int param_8C;
        int param_90;
        unsigned int param_94;
    };

    struct GameData4
    {
        int* param_00;
        int param_04;
        int param_08;
        char pad_1;
        int param_0D;
        int param_11;
        int param_15;
        int param_19;
        int param_1D;
        int param_21;
        char pad_2;
        int param_26;
        int param_2A;
        int param_2E;
        char pad_3;
        int param_33;
    };

    struct GameData5
    {
        int param_00;
        int* param_04;
        char param_08;
        char param_09;
        char param_0A;
        char param_0B;
        int param_0C;
        int param_10;
        int param_14;
        char param_18;
        char param_19;
        bool param_1A;
        char param_1B;
        char param_1C;
        char param_1D;
        char param_1E;
        char param_1F;
        int param_20;
        int param_24;
        int param_28;
        int param_2C;
        int param_30[32];
        char param_B0;
        char param_B1;
        char param_B2;
        char param_B3;
        int param_B4;
        int param_B8[48];
        char param_178;
        char param_179;
        unsigned char param_17A;
        char param_17B;
    };

#pragma pack(pop)

    static_assert(sizeof(GameData4) == 0x37, "GameData4 size mismatch");
    static_assert(sizeof(GameData5) == 0x17C, "GameData5 size mismatch");

#pragma endregion Helper_Structs

public:
    static int  __declspec(noinline) __fastcall sub_1001D240(GameData5* self, void* /*dummy*/, int** a2);
    static void __declspec(noinline) __cdecl    sub_1003E7B0(UnkEntry* a1, int a2, int* a3, int a4);
    static void __declspec(noinline) __cdecl    sub_1003E7B0_de(UnkEntry* a1, int a2, int* a3, int a4);
    static void __declspec(noinline) __cdecl    sub_1003E7B0_fr(UnkEntry* a1, int a2, int* a3, int a4);
    static void __declspec(noinline) __fastcall sub_10055A20(uint32_t* self, void* /*dummy*/, int a2, int a3);
    static void __declspec(noinline) __fastcall sub_10055DC0(uint32_t* self);
    static void __declspec(noinline) __fastcall sub_10055E00(int* self, void* /*dummy*/, char a2, int a3, int a4, int a5, int a6);
    static void __declspec(noinline) __fastcall sub_10055E90(int* self, void* /*dummy*/, char a2, int a3, int a4, __int16* a5);
    static void __declspec(noinline) __fastcall sub_10055F40(int* self, void* /*dummy*/, char a2, int a3, int a4, int a5, int a6);
    static void __declspec(noinline) __fastcall sub_10055FE0(int* self, void* /*dummy*/, char a2);
    static int  __declspec(noinline) __fastcall sub_10056030(uint8_t* self, void* /*dummy*/, int a2, int a3, GameData* const a4);
    static int  __declspec(noinline) __fastcall sub_10056170(uint8_t* self, void* /*dummy*/, int a2, int a3, GameData* const a4);
    static int  __declspec(noinline) __fastcall sub_100563B0(uint8_t* self, void* /*dummy*/, int a2, int a3, GameData2* const a4);
    static void __declspec(noinline) __stdcall  sub_1005C170();
    static void __declspec(noinline) __stdcall  sub_1005C170_de();
    static void __declspec(noinline) __stdcall  sub_1005C170_fr();
    static void __declspec(noinline) __stdcall  sub_1006AD20();
    static void __declspec(noinline) __stdcall  sub_1006AD20_de();
    static void __declspec(noinline) __stdcall  sub_1006AD20_hd();
    static void __declspec(noinline) __stdcall  sub_1006AEA0();
    static void __declspec(noinline) __stdcall  sub_1006AEA0_hd();
    static void __declspec(noinline) __cdecl    sub_1006B1C0(char mask, int* a2);
    static void __declspec(noinline) __cdecl    sub_1006B1C0_hd(char mask, int* a2);
    static char __declspec(noinline) __cdecl    sub_1006B2C0(char mask, int* a2, int a3);
    static char __declspec(noinline) __cdecl    sub_1006B2C0_hd(char mask, int* a2, int a3);
    static void __declspec(noinline) __fastcall sub_1006CC60(GameData3* self);
    static void __declspec(noinline) __fastcall sub_1006CC60_de(GameData3* self);
    static void __declspec(noinline) __fastcall sub_1006CC60_fr(GameData3* self);
    static void __declspec(noinline) __stdcall  sub_1006D940();
    static void __declspec(noinline) __stdcall  sub_1006D940_hd();
    static void __declspec(noinline) __stdcall  sub_1006F120();
    static void __declspec(noinline) __stdcall  sub_1006F120_de();
    static void __declspec(noinline) __stdcall  sub_1006F120_fr();
    static void __declspec(noinline) __stdcall  sub_1006F120_hd();
    static void __declspec(noinline) __cdecl    sub_10099E01(void* mem);
    static void __declspec(noinline) __cdecl    sub_10099E01_de(void* mem);
    static void __declspec(noinline) __cdecl    sub_10099E01_fr(void* mem);

private:
    static bool is_valid_ptr(void* p);
};