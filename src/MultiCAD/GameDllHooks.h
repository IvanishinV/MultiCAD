#pragma once

#include "types.h"
#include "GameGlobals.h"
#include "cad.h"

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

constexpr uint32_t kRowStrideOldDwordSize = 0x10;
constexpr uint32_t kRowStrideOldShift = 6;

constexpr uint32_t kRowStrideDwordSize = kRowStrideOldDwordSize * 4;   // increase by 4 to handle bigger resolution
constexpr uint32_t kRowStrideByteSize = kRowStrideDwordSize * sizeof(uint32_t);
constexpr uint8_t  kRowStrideShift = kRowStrideOldShift + 2;     // + sqrt(4)

constexpr uint32_t kFogLineByteSize = sizeof(Fog);
constexpr uint32_t kFogDoubleLineByteSize = kFogLineByteSize * 2;

class GameDllHooks
{
private:

    inline static GameGlobals* globals_{ nullptr };

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

#pragma endregion Helper_Structs

public:
    static void init(uintptr_t moduleBase)
    {
        delete globals_;
        globals_ = new GameGlobals(moduleBase);
    }

    static void initRelocs(std::span<const RelocationHandle::GapRuntime> newGaps)
    {
        if (globals_)
            globals_->addReloc(newGaps);
    }

    static void shutdown()
    {
        delete globals_;
        globals_ = nullptr;
    }

    static void __declspec(noinline) __cdecl    sub_1003E7B0(UnkEntry* a1, int a2, int* a3, int a4);
    static void __declspec(noinline) __fastcall sub_10055A20(uint32_t* self, void* /*dummy*/, int a2, int a3);
    static void __declspec(noinline) __fastcall sub_10055DC0(uint32_t* self);
    static void __declspec(noinline) __fastcall sub_10055E00(int* self, void* /*dummy*/, char a2, int a3, int a4, int a5, int a6);
    static void __declspec(noinline) __fastcall sub_10055E90(int* self, void* /*dummy*/, char a2, int a3, int a4, __int16* a5);
    static void __declspec(noinline) __fastcall sub_10055F40(int* self, void* /*dummy*/, char a2, int a3, int a4, int a5, int a6);
    static void __declspec(noinline) __fastcall sub_10055FE0(int* self, void* /*dummy*/, char a2);
    static int  __declspec(noinline) __fastcall sub_10056030(uint8_t* self, void* /*dummy*/, int a2, int a3, GameData* const a4);
    static int  __declspec(noinline) __fastcall sub_10056170(uint8_t* self, void* /*dummy*/, int a2, int a3, GameData* const a4);
    static int  __declspec(noinline) __fastcall sub_100563B0(uint8_t* self, void* /*dummy*/, int a2, int a3, GameData2* const a4);
    static void __declspec(noinline) __stdcall  sub_1006AD20();
    static void __declspec(noinline) __stdcall  sub_1006AEA0();
    static void __declspec(noinline) __cdecl    sub_1006B1C0(char mask, int* a2);
    static char __declspec(noinline) __cdecl    sub_1006B2C0(char mask, int* a2, int a3);
    static void __declspec(noinline) __stdcall  sub_1006D940();
    static void __declspec(noinline) __stdcall  sub_1006F120();

};