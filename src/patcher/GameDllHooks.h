#pragma once

#include "types.h"
#include "cad.h"
#include "DllHooksBase.h"

#include <vector>

template<GameVersion Version>
struct UiTraits;

struct UiAddresses
{
    // addUiElement
    uintptr_t updateUiFlag;
    uintptr_t pointedUiElem;
    uintptr_t screenHeight;
    uintptr_t screenWidth;
    
    // drawUiElement
    uintptr_t uiGameDataArray;
    uintptr_t fnDrawUiSprite;
    uintptr_t fnGetFirstGameData;
    uintptr_t fnGetNextGameData;

    // calculateClosedArea
    uintptr_t closedAreaGameDataArray;
    uintptr_t fnFillClosedAreaGameData;

    // dispatchMouseButtonEvent
    uintptr_t mouseX;
    uintptr_t mouseY;
    uintptr_t uiEventAreas;
    uintptr_t fnWriteEventToRingBuffer;

    // dispatchWndMessage
    uintptr_t wndGlobalVariables;
    uintptr_t fnDispatchMouseButtonEvent;
    uintptr_t fnDispatchMouseMoveEvent;
    uintptr_t fnMultiByteToWideCharOr;
    uintptr_t fnAddUiEventArea;
    uintptr_t fnRemoveUiEventAreaSafe;
    uintptr_t fnAddUiElement;
    uintptr_t fnRemoveUiElement;
    uintptr_t strategicMapUiVtable;

    // drawDecorUiElement
    uintptr_t cadPtr;
    uintptr_t fnBlendMainWithWarFog;
    uintptr_t fnGetFirstDecorUi;
    uintptr_t fnGetNextDecorUi;
};

template<>
struct UiTraits<GameVersion::SS_V1_0>
{
    static constexpr UiAddresses addresses
    {
        0x33D5E4,
        0x33D5F0,
        0x36C018,
        0x36C01C,

        0x33D618,
        0x5EE60,
        0x4A170,
        0x4A1C0,

        0x33EE20,
        0x49DF0,

        0x370EF4,
        0x370EF8,
        0x371F0C,
        0x80C20,

        0x370EF0,
        0x80FA0,
        0x81020,
        0x0,
        0x80AB0,
        0x80AF0,
        0x5F2C0,
        0x661C0,
        0x98F24,

        0x370EE4,
        0x5ED00,
        0x4A380,
        0x4A3D0
    };
};

template<>
struct UiTraits<GameVersion::SS_HD_V1_1_RU>
{
    static constexpr UiAddresses addresses
    {
        0x33D5E4,
        0x33D5F0,
        0x36C018,
        0x36C01C,

        0x395000,
        0x5EE60,
        0x4A170,
        0x4A1C0,

        0x39A000,
        0x49DF0,

        0x370EF4,
        0x370EF8,
        0x371F0C,
        0x80C20,

        0x370EF0,
        0x80FA0,
        0x81020,
        0x0,
        0x80AB0,
        0x80AF0,
        0x5F2C0,
        0x661C0,
        0x98F24,

        0x370EE4,
        0x5ED00,
        0x4A380,
        0x4A3D0
    };
};

template<>
struct UiTraits<GameVersion::SS_V1_2>
{
    static constexpr UiAddresses addresses
    {
        0x34FF3C,
        0x34FF48,
        0x37E970,
        0x37E974,

        0x34FF70,
        0x6D850,
        0x58550,
        0x585A0,

        0x351778,
        0x581D0,

        0x38450C,
        0x384510,
        0x385524,
        0x913E0,

        0x384508,
        0x914E0,
        0x91560,
        0x91D50,
        0x91270,
        0x912B0,
        0x6DCB0,
        0x74E10,
        0xAA6B0,

        0x3844FC,
        0x6D6F0,
        0x58760,
        0x587B0
    };
};

template<>
struct UiTraits<GameVersion::SS_GOLD_DE>
{
    static constexpr UiAddresses addresses
    {
        0x34FEA4,
        0x34FEB0,
        0x37E8D8,
        0x37E8DC,

        0x34FED8,
        0x6DA00,
        0x58820,
        0x58860,

        0x3516E0,
        0x584B0,

        0x38447C,
        0x384480,
        0x385494,
        0x91660,

        0x384478,
        0x91760,
        0x917E0,
        0x91F10,
        0x914F0,
        0x91530,
        0x6DE60,
        0x74F40,
        0xAA6B0,

        0x38446C,
        0x6D8A0,
        0x58A10,
        0x58A50
    };
};

template<>
struct UiTraits<GameVersion::SS_GOLD_EN>
{
    static constexpr UiAddresses addresses
    {
        0x34FEEC,
        0x34FEF8,
        0x37E918,
        0x37E91C,

        0x34FF20,
        0x6B0A0,
        0x562F0,
        0x56330,

        0x351728,
        0x55F40,

        0x384484,
        0x384488,
        0x38549C,
        0x8E7D0,

        0x384480,
        0x8E8D0,
        0x8E960,
        0x8F080,
        0x8E660,
        0x8E6A0,
        0x6B570,
        0x72340,
        0xA8740,

        0x384474,
        0x6AEA0,
        0x564F0,
        0x56530
    };
};

template<>
struct UiTraits<GameVersion::SS_GOLD_FR>
{
    static constexpr UiAddresses addresses
    {
        0x353EC4,
        0x353ED0,
        0x3828F8,
        0x3828FC,

        0x353EF8,
        0x6DA80,
        0x587C0,
        0x58800,

        0x355700,
        0x58410,

        0x3885A4,
        0x3885A8,
        0x3895BC,
        0x91BC0,

        0x3885A0,
        0x91CC0,
        0x91D40,
        0x92480,
        0x91A50,
        0x91A90,
        0x6DEE0,
        0x74F80,
        0xAC6B8,

        0x388598,
        0x6D8F0,
        0x589C0,
        0x58A00
    };
};

template<>
struct UiTraits<GameVersion::SS_GOLD_HD_1_2_INT>
{
    static constexpr UiAddresses addresses
    {
        0x34FEEC,
        0x34FEF8,
        0x37E918,
        0x37E91C,

        0x3A8000,
        0x6B0A0,
        0x562F0,
        0x56330,

        0x3AD000,
        0x55F40,

        0x384484,
        0x384488,
        0x38549C,
        0x8E7D0,

        0x384480,
        0x8E8D0,
        0x8E960,
        0x8F080,
        0x8E660,
        0x8E6A0,
        0x6B570,
        0x72340,
        0xA8740,

        0x384474,
        0x6AEA0,
        0x564F0,
        0x56530
    };
};

template<>
struct UiTraits<GameVersion::SS_2>
{
    static constexpr UiAddresses addresses
    {
        0x103B6D4,
        0x103B6E0,
        0x106A128,
        0x106A12C,

        0x103B708,
        0x98410,
        0x79900,
        0x79950,

        0x103CF10,
        0x794B0,

        0x106F6F4,
        0x106F6F8,
        0x107070C,
        0xCAAE0,

        0x106F6F0,
        0xCACD0,
        0xCAD50,
        0xCB5E0,
        0xCA810,
        0xCA850,
        0x988C0,
        0xA0DF0,
        0xEFF2C,

        0x106F6E4,
        0x982B0,
        0x79B10,
        0x79B60
    };
};

template<>
struct UiTraits<GameVersion::SS_RW_V2_3>
{
    static constexpr UiAddresses addresses
    {
        0x107AAAC,
        0x107AAB8,
        0x10A9500,
        0x10A9504,

        0x107AAE0,
        0x955F0,
        0x78E90,
        0x78EE0,

        0x107C2E8,
        0x78B10,

        0x10AEACC,
        0x10AEAD0,
        0x10AFAE4,
        0xC5500,

        0x10AEAC8,
        0xC5600,
        0xC5680,
        0xC5F70,
        0xC5390,
        0xC53D0,
        0x95AA0,
        0x9D8C0,
        0xE2FAC,

        0x10AEABC,
        0x95490,
        0x790A0,
        0x790F0
    };
};

template<>
struct UiTraits<GameVersion::SS_RW_V2_4>
{
    static constexpr UiAddresses addresses
    {
        0x107AAAC,
        0x107AAB8,
        0x10A9500,
        0x10A9504,

        0x107AAE0,
        0x955F0,
        0x78E90,
        0x78EE0,

        0x107C2E8,
        0x78B10,

        0x10AEACC,
        0x10AEAD0,
        0x10AFAE4,
        0xC5510,

        0x10AEAC8,
        0xC5610,
        0xC5690,
        0xC5F80,
        0xC53A0,
        0xC53E0,
        0x95AA0,
        0x9D8C0,
        0xE2FAC,

        0x10AEABC,
        0x95490,
        0x790A0,
        0x790F0
    };
};

template<>
struct UiTraits<GameVersion::SS_BLACK_GOLD>
{
    static constexpr UiAddresses addresses
    {
        0x106AC5C,
        0x106AC68,
        0x10996B0,
        0x10996B4,

        0x106AC90,
        0x955C0,
        0x78E70,
        0x78EC0,

        0x106C498,
        0x78AF0,

        0x109EC7C,
        0x109EC80,
        0x109FC94,
        0xC5430,

        0x109EC78,
        0xC5530,
        0xC55B0,
        0xC5DD0,
        0xC52C0,
        0xC5300,
        0x95A70,
        0x9D890,
        0xE2FAC,

        0x109EC6C,
        0x95460,
        0x79080,
        0x790D0
    };
};


template<GameVersion V, typename = void>
struct HasUiTraits : std::false_type {};

template<GameVersion V>
struct HasUiTraits<V, std::void_t<
    decltype(UiTraits<V>::addresses)
    >> : std::true_type {};

template<GameVersion V>
constexpr bool ValidateUiTraits()
{
    constexpr auto& A = UiTraits<V>::addresses;

    return
        A.pointedUiElem &&
        A.updateUiFlag &&
        A.screenHeight &&
        A.screenWidth;
}


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

    struct UIRenderElement
    {
        void** vtable;
        UIRenderElement* prev;
        UIRenderElement* next;
        Rect rect;
        int x;
        int y;
        int* scale;
        int some_ui_param;
        int type;
        int* var_30;
        int parent;
        int flags;
    };

    static_assert(sizeof(UIRenderElement) == 0x3C, "UIRenderElement size mismatch");
#pragma endregion

#pragma region UI_Common_Function_Structs
    struct DrawDecorUiElementData
    {
        UIRenderElement* uiRenderElem;
        int* closedAreaGameDataArray;
        uintptr_t cadPtr;

        int surfaceHeight;
        int surfaceWidth;

        void(__stdcall* blendMainWithWarFog)();
        int(__thiscall* getFirstDecorUi)(int*, GameData2*);
        int(__thiscall* getNextDecorUi)(int*, GameData2*);
    };

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
        void(__cdecl* fnWriteClipRect)(int*, int, int, int, int);
        void(__thiscall* fnResetUiImage)(UiStrategicMapElement*);

        void(__thiscall* drawHorLine)(UiElementBase*, int, int, int, int16_t);
        void(__thiscall* drawVertLine)(UiElementBase*, int, int, int, int16_t);
        void(__stdcall* drawPlaneCrosses)(int, int, int);
        uintptr_t* strategicMapOverlay;

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
    template<GameVersion V>
    static void __declspec(noinline) __stdcall  drawDecorUiElements_ver()
    {
        static_assert(HasUiTraits<V>::value, "UiTraits specialization missing");
        static_assert(ValidateUiTraits<V>(), "One or more UiTraits addresses are zero");

        auto* const g = globals_;
        constexpr auto& A = UiTraits<V>::addresses;
        
        DrawDecorUiElementData data
        {
            g->getValue<UIRenderElement*>(A.pointedUiElem + 0xC),
            g->getPtr<int>(A.closedAreaGameDataArray),
            g->getValue<uintptr_t>(A.cadPtr),

            g->getValue<int>(A.pointedUiElem + 0x14),
            g->getValue<int>(A.pointedUiElem + 0x18),

            g->getFn<void(__stdcall)()>(A.fnBlendMainWithWarFog),
            g->getFn<int(__thiscall)(int*, GameData2*)>(A.fnGetFirstDecorUi),
            g->getFn<int(__thiscall)(int*, GameData2*)>(A.fnGetNextDecorUi),
        };

        drawDecorUiElements(data);
    }
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

    // Template functions to disable/enable in-game UI
    template<GameVersion V>
    static void __declspec(noinline) __cdecl    addUiElement_ver(UiElementBase* self, int type)
    {
        static_assert(HasUiTraits<V>::value, "UiTraits specialization missing");
        static_assert(ValidateUiTraits<V>(), "One or more UiTraits addresses are zero");

        auto* const g = globals_;
        constexpr auto& A = UiTraits<V>::addresses;

        AddUiElementData data
        {
            type,
            g->getPtr<UiElementBase*>(A.pointedUiElem),
            g->getPtr<int>(A.updateUiFlag),
            g->getValue<int>(A.screenHeight),
            g->getValue<int>(A.screenWidth)
        };

        addUiElement(self, data);
    }
    template<GameVersion V>
    static void __declspec(noinline) __fastcall drawUiElement_ver(UiElementBase* self)
    {
        static_assert(HasUiTraits<V>::value, "UiTraits specialization missing");
        static_assert(ValidateUiTraits<V>(), "One or more UiTraits addresses are zero");

        auto* const g = globals_;
        constexpr auto& A = UiTraits<V>::addresses;

        DrawUiElementData data
        {
            g->getPtr<int>(A.uiGameDataArray),
            g->getFn<int(__thiscall)(int*, GameData*)>(A.fnGetFirstGameData),
            g->getFn<int(__thiscall)(int*, GameData*)>(A.fnGetNextGameData),
            g->getFn<void(__cdecl)(int, int, int, int, int, int, void*)>(A.fnDrawUiSprite)
        };

        drawUiElement(self, data);
    }
    template<GameVersion V>
    static void __declspec(noinline) __fastcall calculateClosedArea_ver(UiElementBase* self)
    {
        static_assert(HasUiTraits<V>::value, "UiTraits specialization missing");
        static_assert(ValidateUiTraits<V>(), "One or more UiTraits addresses are zero");

        auto* const g = globals_;
        constexpr auto& A = UiTraits<V>::addresses;

        CalculateClosedAreaData data
        {
            g->getPtr<int>(A.closedAreaGameDataArray),
            g->getFn<void(__thiscall)(int*, char, int, int, int, int)>(A.fnFillClosedAreaGameData)
        };

        calculateClosedArea(self, data);
    }
    static int  __declspec(noinline) __fastcall calculateCursorType(UiElementBase* self, void* /*dummy*/, int x, int y, int* a4);
    template<GameVersion V>
    static void __declspec(noinline) __cdecl    dispatchMouseButtonEvent_ver(int eventTag)
    {
        static_assert(HasUiTraits<V>::value, "UiTraits specialization missing");
        static_assert(ValidateUiTraits<V>(), "One or more UiTraits addresses are zero");

        auto* const g = globals_;
        constexpr auto& A = UiTraits<V>::addresses;

        DispatchMouseButtonEventData data
        {
            eventTag,
            g->getValue<int>(A.mouseX),
            g->getValue<int>(A.mouseY),
            g->getValue<UiEventArea*>(A.uiEventAreas),
            g->getFn<int(__cdecl)(int, int, int, int)>(A.fnWriteEventToRingBuffer)
        };

        dispatchMouseButtonEvent(data);
    }
    template<GameVersion V>
    static void __declspec(noinline) __cdecl    dispatchMouseMoveEvent_ver(int prevMouseX, int prevMouseY, int mouseX, int mouseY)
    {
        static_assert(HasUiTraits<V>::value, "UiTraits specialization missing");
        static_assert(ValidateUiTraits<V>(), "One or more UiTraits addresses are zero");

        auto* const g = globals_;
        constexpr auto& A = UiTraits<V>::addresses;

        DispatchMouseMoveEventData data
        {
            prevMouseX,
            prevMouseY,
            mouseX,
            mouseY,
            g->getValue<UiEventArea*>(A.uiEventAreas),
            g->getFn<int(__cdecl)(int, int, int, int)>(A.fnWriteEventToRingBuffer)
        };

        dispatchMouseMoveEvent(data);
    }
    template<GameVersion V>
    static int __declspec(noinline) __cdecl     dispatchWndMessage_ver(int a1, int a2, int a3, int a4)
    {
        static_assert(HasUiTraits<V>::value, "UiTraits specialization missing");
        static_assert(ValidateUiTraits<V>(), "One or more UiTraits addresses are zero");

        auto* const g = globals_;
        constexpr auto& A = UiTraits<V>::addresses;

        DispatchWndMessageData data
        {
            a2,
            a3,
            a4,
            g->getPtr<int>(A.wndGlobalVariables),
            g->getFn<int(__cdecl)(int)>(A.fnDispatchMouseButtonEvent),
            g->getFn<int(__cdecl)(int, int, int, int)>(A.fnDispatchMouseMoveEvent),
            g->getFn<int(__cdecl)(int, int, int, int)>(A.fnWriteEventToRingBuffer),
            A.fnMultiByteToWideCharOr != 0 ? g->getFn<int(__cdecl)(int)>(A.fnMultiByteToWideCharOr) : 0,
            g->getValue<int>(A.screenHeight),
            g->getValue<int>(A.screenWidth),
            g->getFn<int(__cdecl)(UiEventArea*)>(A.fnAddUiEventArea),
            g->getFn<int(__cdecl)(UiEventArea*)>(A.fnRemoveUiEventAreaSafe),
            g->getFn<void(__cdecl)(UiElementBase*, int)>(A.fnAddUiElement),
            g->getFn<void(__thiscall)(UiElementBase*)>(A.fnRemoveUiElement),
            g->getPtr<UiElementVtable>(A.strategicMapUiVtable)
        };

        return dispatchWndMessage(data);
    }
private:
    static void drawDecorUiElements(const DrawDecorUiElementData& data);

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
