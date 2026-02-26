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
        unsigned char param_1A;
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


    enum class TeamType : uint8_t
    {
        Player = 0,
        Enemy = 1,
        Static = 12,
    };

#pragma pack(push, 1)
    struct UnitData
    {
        int* vtable;
        int var_04;
        int16_t var_08;
        UnitData* next;
        int var_0E[3];
        uint16_t flag_1A;
        uint8_t teamId;
        bool var_1D;
        int var_1E[1];
        bool var_22;
        bool flag_23;
        bool var_24;
        bool var_25;
        int var_26[3];
        bool var_32;
        int16_t tileX;
        int16_t tileY;
        int16_t subX;
        int16_t subY;
        int var_3B[142];
        int16_t fogFlag;
    };
#pragma pack(pop)

    static_assert(offsetof(UnitData, teamId) == 0x1C, "teamId offset mismatch");
    static_assert(offsetof(UnitData, tileX) == 0x33, "tileX offset mismatch");
    static_assert(offsetof(UnitData, fogFlag) == 0x273, "fogFlag offset mismatch");

#pragma pack(push, 1)
    struct PlaneData
    {
        int* vtable;
        int16_t** pivots;
        PlaneData* next;
        int teamId;
        int var_010;
        int var_014;
        int16_t var_18;
        float worldX;
        float worldY;
        float height;
        float rotAngle;
    };

#pragma pack(pop)

    static_assert(offsetof(PlaneData, worldX) == 0x1A, "worldX offset mismatch");
    static_assert(offsetof(PlaneData, worldY) == 0x1E, "worldY offset mismatch");
    static_assert(offsetof(PlaneData, height) == 0x22, "height offset mismatch");

#pragma endregion Helper_Structs

#pragma region UI_Helper_Structs
    // Structures used in UI hooks
    struct UiEvent
    {
        int eventTag;
        int flags;
        int x;
        int y;
    };


    struct ZoneHandler;

    struct ZoneHandlerVtable
    {
        void(__thiscall* fn1)(ZoneHandler*, int);
        void(__thiscall* fn2)(ZoneHandler*, int);
        void(__thiscall* fn3)(ZoneHandler*);
        void(__thiscall* fn4)(ZoneHandler*, int, int);
        void(__thiscall* fn5)(ZoneHandler*);
        void*            fn6;
        void(__thiscall* onMouseEvent)(ZoneHandler*, UiEvent*);
        int(__thiscall*  fn8)(ZoneHandler*, int);
        int(__thiscall*  fn9)(ZoneHandler*, ZoneHandler*);
        void(__thiscall* fn10)(ZoneHandler*, ZoneHandler*);
        void(__thiscall* fn11)(ZoneHandler*, ZoneHandler*);
        void(__thiscall* calculateCursorType)(ZoneHandler*, int, int, int*);
        int(__thiscall*  fn13)(ZoneHandler*, int, int, int);
        int(__thiscall*  fn14)(ZoneHandler*, int, int, int, int);
        void*            fn15;
    };

    struct ZoneHandler
    {
        ZoneHandlerVtable* vtable;
        int var1;
        ZoneHandler* next;
        uint8_t zoneId;
    };


    struct UiElementBase;

    struct UiElementVtable
    {
        void(__thiscall* fn1)(UiElementBase*, int, int);
        void(__thiscall* fn2)(UiElementBase*);
        void(__thiscall* onAdd)(UiElementBase*);
        void(__thiscall* onRemoveFn)(UiElementBase*);
        void(__thiscall* handleCursorClick)(UiElementBase*, UiEvent*);
        int(__thiscall*  fn6)(UiElementBase*, int, int, int, int);
        int(__thiscall*  handlePressButton)(UiElementBase*, UiEvent*);
        void(__thiscall* onLoseFocus)(UiElementBase*);
        void(__thiscall* fn9)(UiElementBase*);
        void(__thiscall* fn_A1000)(UiElementBase*);
        void(__thiscall* drawUiElement)(UiElementBase*);
        void(__thiscall* fn10)(UiElementBase*);
        void(__thiscall* fn11)(UiElementBase*);
        void(__thiscall* fn12)(UiElementBase*, int, int);
        void(__thiscall* calculateClosedArea)(UiElementBase*);
        int(__thiscall*  calculateCursorType)(UiElementBase*, int, int, int*);
        int(__thiscall*  fn13)(UiElementBase*, int, int, char*);
        void(__thiscall* addCtlEventToRingBuffer)(UiElementBase*, int, int, int);
        void(__thiscall* addCtlEventToRingBuffer_2)(UiElementBase*, int, int, int);
        void(__thiscall* addEventToRingBuffer)(UiElementBase*, int, Rect*, int, int, int);
        void(__thiscall* removeUiElement)(UiElementBase*);
    };

    static_assert(offsetof(UiElementVtable, onAdd) == 0x8, "onAdd offset mismatch");
    static_assert(offsetof(UiElementVtable, handleCursorClick) == 0x10, "handleCursorClick offset mismatch");
    static_assert(offsetof(UiElementVtable, onLoseFocus) == 0x1C, "onLoseFocus offset mismatch");


    struct UiEventArea
    {
        int tag;
        int x;
        int y;
        int width;
        int height;
        int flags;
        int flags_2;
        UiEventArea* next;
    };

    static_assert(offsetof(UiEventArea, next) == 0x1C, "next offset mismatch");


    struct UiElementBase
    {
        UiElementVtable* vtable;
        UiElementBase* next;
        UiElementBase* prev;
        int leftX;
        int topY;
        int rightX;
        int bottomY;
        UiEventArea* uiEventArea;
        int type;
        int var_24;
        int mouseFlags;
        int var_2C;
        Pixel* sprites;
        int stride;
        int clipLeft;
        int clipTop;
        int clipRight;
        int clipBottom;
        uint16_t* dstBuf;
        uint8_t* zoneBuffer;
        ZoneHandler* zoneList;
        ZoneHandler* var_54;
        ZoneHandler* forced;
        ZoneHandler* current;
    };

    static_assert(offsetof(UiElementBase, uiEventArea) == 0x1C, "uiEventArea offset mismatch");
    static_assert(offsetof(UiElementBase, type) == 0x20, "type offset mismatch");
    static_assert(offsetof(UiElementBase, sprites) == 0x30, "sprites offset mismatch");
    static_assert(offsetof(UiElementBase, stride) == 0x34, "stride offset mismatch");
    static_assert(offsetof(UiElementBase, zoneBuffer) == 0x4C, "zoneBuffer offset mismatch");
    static_assert(offsetof(UiElementBase, zoneList) == 0x50, "zoneList offset mismatch");
    static_assert(offsetof(UiElementBase, forced) == 0x58, "forced offset mismatch");
    static_assert(offsetof(UiElementBase, current) == 0x5C, "isLoaded offset mismatch");


    enum UiEventFlags
    {
        UI_MOUSE_MOVE = 0x1,
        UI_MOUSE_ENTER = 0x2,
        UI_MOUSE_LEAVE = 0x4,
        UI_DISABLED = 0x200,
        UI_STOP_PROPAGATION = 0x400
    };


#pragma pack(push, 1)
    struct UiStrategicMapElement : UiElementBase
    {
        bool var_060;
        bool isMapLoaded;
        int screenSurfaceWidth;
        int screenSurfaceHeight;
        int verticalCenterMargin;
        bool var_06E;
        uint8_t* srcBuf;
    };
#pragma pack(pop)

    static_assert(sizeof(UiStrategicMapElement) == 0x73, "UiStrategicMapElement size mismatch");
    static_assert(offsetof(UiStrategicMapElement, dstBuf) == 0x48, "dstBuf offset mismatch");
    static_assert(offsetof(UiStrategicMapElement, isMapLoaded) == 0x61, "isMapLoaded offset mismatch");
    static_assert(offsetof(UiStrategicMapElement, srcBuf) == 0x6F, "srcBuf offset mismatch");
#pragma endregion

#pragma region UI_Common_Function_Structs
    struct AddUiElementData
    {
        int type;
        UiElementBase** pointed;
        int* updatedUiFlag;
        int screenHeight;
        int screenWidth;
    };

    struct DrawUiElementData
    {
        int* dword_103B708;
        int(__thiscall* fn_79900)(int*, GameData*);
        int(__thiscall* fn_79950)(int*, GameData*);
        void(__cdecl* fn_98410)(int, int, int, int, int, int, void*);
    };

    struct CalculateClosedAreaData
    {
        int* dword_103CF10;
        void(__thiscall*fn_794B0)(int*, char, int, int, int, int);
    };

    struct DispatchMouseButtonEventData
    {
        int eventTag;
        int mouseX;
        int mouseY;
        UiEventArea* uiEventAreas;
        int(__cdecl* writeEventToRingBuffer)(int, int, int, int);
    };

    struct DispatchMouseMoveEventData
    {
        int prevMouseX;
        int prevMouseY;
        int mouseX;
        int mouseY;
        UiEventArea* uiEventAreas;
        int(__cdecl* writeEventToRingBuffer)(int, int, int, int);
    };

    struct DispatchWndMessageData
    {
        int a2;
        int a3;
        int a4;
        int* dword_1106F6F0;

        int(__cdecl* dispatchMouseButtonEvent)(int);
        int(__cdecl* dispatchMouseMoveEvent)(int, int, int, int);
        int(__cdecl* writeEventToRingBuffer)(int, int, int, int);
        int(__cdecl* multiByteToWideCharOr)(int);

        int screenHeight;
        int screenWidth;

        int(__cdecl* addUiEventArea)(UiEventArea*);
        int(__cdecl* removeUiEventAreaSafe)(UiEventArea*);
        void(__cdecl* addUiElement)(UiElementBase*, int);
        void(__thiscall* removeUiElement)(UiElementBase*);

        UiElementVtable* strategicMapUiVtable;
    };
#pragma endregion

#pragma region Common_Function_Structs
    // Structures used in common patterns
    struct SomeRandCalcData
    {
        UnkEntry* a1;
        int a2;
        int* a3;
        int a4;

        int(__thiscall* fn_2560)(UnkEntry*, int);
        int& randSeed;
    };

    struct ReadStrategicMapData
    {
        void(__thiscall* initHandle)(HANDLE*);
        bool(__thiscall* createFile)(HANDLE*, const char*, int);
        uint32_t(__thiscall* getFileSize)(HANDLE*);
        void(__thiscall* readFile)(HANDLE*, void*, uint32_t);
        void(__thiscall* closeHandle)(HANDLE*);
        void(__thiscall* deinitHandle)(HANDLE*);
        char* xchngTogameMis;
        ModuleStateBase* cadPtr;
        void* (__cdecl* fnNew)(size_t);
    };

    struct FogDrawData
    {
        uint8_t* fogBase;
        int mapHeight;
        int mapWidth;
        ModuleStateBase* cadPtr;

        int mapPosY;
        int mapPosX;
        int screenWidth;
        int screenHeight;

        int* div16Ptr;
        uint8_t fogMask;
        uint8_t* fogBuf;
    };

    struct FogOnStrategicMap
    {
        void(__cdecl* fn_97740)(int*, int, int, int, int);
        void(__thiscall* fn_A1110)(UiStrategicMapElement*);

        void(__thiscall* drawHorLine)(UiElementBase*, int, int, int, int16_t);
        void(__thiscall* drawVertLine)(UiElementBase*, int, int, int, int16_t);
        void(__stdcall* drawPlaneCrosses)(int, int, int);

        UnitData* units;
        int unitVtableOffset;
        int16_t fogBorderColor;

        int mapHeight;
        int mapWidth;
        int mapPosY;
        int mapPosX;

        uint8_t fogMask;
        uint8_t* fogBase;

        ModuleStateBase* cadPtr;
    };

    struct ScreenRectDrawData
    {
        void(__thiscall* fn1)(UiElementBase*, int, int);
        void(__thiscall* fn2)(UiElementBase*, int, int, int, int);
        int mapHeight;
        int mapPosY;
        int mapPosX;
    };

    struct PlaneMapDrawData
    {
        UiElementBase* mapData;
        uint16_t* colors;
        void(__thiscall* drawHorLine)(UiElementBase*, int, int, int, int16_t);
        void(__thiscall* drawVertLine)(UiElementBase*, int, int, int, int16_t);
        int halfScreenWidth;
        int vertCenterMargin;
        int scale;
    };

#pragma endregion Common_Function_Structs

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
    // This method exists only in: SS en/ru, SS Gold de/fr/ru, SS2, SS:RW
    static void __declspec(noinline) __fastcall sub_1006DC40(int* self, void* /*dummy*/, int a2, int a3, int a4, int a5, uint8_t a6, char a7, char a8);
    static void __declspec(noinline) __stdcall  sub_1006F120();
    static void __declspec(noinline) __stdcall  sub_1006F120_de();
    static void __declspec(noinline) __stdcall  sub_1006F120_fr();
    static void __declspec(noinline) __stdcall  sub_1006F120_hd_v1_2();
    static void __declspec(noinline) __stdcall  sub_1006F120_v1_0_ru();
    static void __declspec(noinline) __stdcall  sub_1006F120_v1_2_en();
    static void __declspec(noinline) __stdcall  sub_1006F120_hd_v1_1();
    static void __declspec(noinline) __stdcall  sub_1006F120_v2_2();    // SS 2
    static void __declspec(noinline) __stdcall  sub_1006F120_rw();      // SS:RW
    static void __declspec(noinline) __stdcall  sub_1006F120_bg();      // Black Gold
    static void __declspec(noinline) __cdecl    sub_10099E01(void* mem);
    static void __declspec(noinline) __cdecl    sub_10099E01_de(void* mem);
    static void __declspec(noinline) __cdecl    sub_10099E01_fr(void* mem);

    // These methods are related to strategic map view and exist only in SS2 and SS:RW
    // Fixes strategic map loading from mis_mini file
    static void __declspec(noinline) __fastcall sub_100AC870(UiStrategicMapElement* self);        // SS 2
    static void __declspec(noinline) __fastcall sub_100A8AF0_v2_3(UiStrategicMapElement* self);   // SS:RW v2.3
    static void __declspec(noinline) __fastcall sub_100A8AF0_v2_4(UiStrategicMapElement* self);   // SS:RW v2.4
    static void __declspec(noinline) __fastcall sub_100A8AF0_bg(UiStrategicMapElement* self);     // Black Gold
    // Fixes original bug with white rectangle in strategic view
    static void __declspec(noinline) __fastcall sub_100ACDE0(UiStrategicMapElement* self);        // SS 2
    static void __declspec(noinline) __fastcall sub_100A9060_v2_3(UiStrategicMapElement* self);   // SS:RW v2.3
    static void __declspec(noinline) __fastcall sub_100A9060_v2_4(UiStrategicMapElement* self);   // SS:RW v2.4
    static void __declspec(noinline) __fastcall sub_100A9060_bg(UiStrategicMapElement* self);     // Black Gold
    // Fixes the same original bug with copying rectangle in strategic view. Related to the previous one
    static void __declspec(noinline) __fastcall sub_100AD2C0(UiStrategicMapElement* self, void* /*dummy*/, int offsetX, int offsetY);    // SS 2
    static void __declspec(noinline) __fastcall sub_100A97C0(UiStrategicMapElement* self, void* /*dummy*/, int offsetX, int offsetY);    // SS:RW
    static void __declspec(noinline) __fastcall sub_100A97C0_bg(UiStrategicMapElement* self, void* /*dummy*/, int offsetX, int offsetY); // Black Gold
    // Draws plane cross on strategic map. Originally it draws cross incorrectly since it doesn't use height
    static void __declspec(noinline) __fastcall sub_100C3830(PlaneData* self, void* /*dummy*/, int halfScreenWidth, int vertCenterMargin, int scale);   // SS 2
    static void __declspec(noinline) __fastcall sub_100BE6F0(PlaneData* self, void* /*dummy*/, int halfScreenWidth, int vertCenterMargin, int scale);   // SS:RW
    static void __declspec(noinline) __fastcall sub_100BE6C0(PlaneData* self, void* /*dummy*/, int halfScreenWidth, int vertCenterMargin, int scale);   // Black Gold

    // These methods are used to add opportunity to disable/enable in-game UI

    // SS 2
    static void __declspec(noinline) __cdecl    addUiElement_v2_2(UiElementBase* self, int type);
    static void __declspec(noinline) __fastcall drawUiElement_v2_2(UiElementBase* self);
    static void __declspec(noinline) __fastcall calculateClosedArea_v2_2(UiElementBase* self);
    static void __declspec(noinline) __cdecl    dispatchMouseButtonEvent_v2_2(int eventTag);
    static void __declspec(noinline) __cdecl    dispatchMouseMoveEvent_v2_2(int prevMouseX, int prevMouseY, int mouseX, int mouseY);
    static int  __declspec(noinline) __cdecl    dispatchWndMessage_v2_2(int a1, int a2, int a3, int a4);

    // SS:RW v2.3/v2.4
    static void __declspec(noinline) __cdecl    addUiElement_rw(UiElementBase* self, int type);
    static void __declspec(noinline) __fastcall drawUiElement_rw(UiElementBase* self);
    static void __declspec(noinline) __fastcall calculateClosedArea_rw(UiElementBase* self);

    static void __declspec(noinline) __cdecl    dispatchMouseButtonEvent_rw_v2_3(int eventTag);
    static void __declspec(noinline) __cdecl    dispatchMouseButtonEvent_rw_v2_4(int eventTag);
    static void __declspec(noinline) __cdecl    dispatchMouseMoveEvent_rw_v2_3(int prevMouseX, int prevMouseY, int mouseX, int mouseY);
    static void __declspec(noinline) __cdecl    dispatchMouseMoveEvent_rw_v2_4(int prevMouseX, int prevMouseY, int mouseX, int mouseY);
    static int  __declspec(noinline) __cdecl    dispatchWndMessage_rw_v2_3(int a1, int a2, int a3, int a4);
    static int  __declspec(noinline) __cdecl    dispatchWndMessage_rw_v2_4(int a1, int a2, int a3, int a4);

    // Black Gold
    static void __declspec(noinline) __cdecl    addUiElement_bg(UiElementBase* self, int type);
    static void __declspec(noinline) __fastcall drawUiElement_bg(UiElementBase* self);
    static void __declspec(noinline) __fastcall calculateClosedArea_bg(UiElementBase* self);
    static void __declspec(noinline) __cdecl    dispatchMouseButtonEvent_bg(int eventTag);
    static void __declspec(noinline) __cdecl    dispatchMouseMoveEvent_bg(int prevMouseX, int prevMouseY, int mouseX, int mouseY);
    static int  __declspec(noinline) __cdecl    dispatchWndMessage_bg(int a1, int a2, int a3, int a4);

    // Common function to disable/enable in-game UI
    static int  __declspec(noinline) __fastcall calculateCursorType(UiElementBase* self, void* /*dummy*/, int x, int y, int* a4);
private:
    static bool is_valid_ptr(void* p);

    static void someRandCalc(const SomeRandCalcData& data);

    static void drawFogOnWorld(const FogDrawData& data);
    static void drawFogOnWorld_v2(const FogDrawData& data);

    // Common functions to process strategic map
    static void readStrategicMapFromFile(UiStrategicMapElement* mapData, const ReadStrategicMapData& data);
    static void drawFogOnStrategicMap(UiStrategicMapElement* mapData, const FogOnStrategicMap& data);
    static void drawScreenRectOnStrategicMap(UiStrategicMapElement* mapData, int offsetX, int offsetY, const ScreenRectDrawData& data);
    static void drawPlaneCrossOnStrategicMap(PlaneData* mapData, const PlaneMapDrawData& data);

    // Common functions to disable/enable in-game UI
    static void addUiElement(UiElementBase* elem, const AddUiElementData& data);
    static void drawUiElement(UiElementBase* self, const DrawUiElementData& data);
    static void calculateClosedArea(UiElementBase* self, const CalculateClosedAreaData& data);
    static void dispatchMouseButtonEvent(const DispatchMouseButtonEventData& data);
    static void dispatchMouseMoveEvent(const DispatchMouseMoveEventData& data);
    static int  dispatchWndMessage(const DispatchWndMessageData& data);
};