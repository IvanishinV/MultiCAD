#pragma once

#include "assets.h"
#include "util.h"

constexpr U32 DIRECTDRAW_VERSION = 0x0700;
#include <ddraw.h>

template<typename T>
inline void dxRelease(T*& p)
{
    if (p)
    {
        p->Release();
        p = nullptr;
    }
}

struct DirectX
{
    LPDIRECTDRAW            instance;
    LPDIRECTDRAWSURFACE     surface;
};


using INIT_VALUES_PTR = void(*)(void);
using INIT_DX_INSTANCE_PTR = bool(*)(HWND hwnd, bool fullscreen);
using RELEASE_DX_SURFACE_PTR = void(*)(void);
using RESTORE_DX_INSTANCE_PTR = void(*)(void);
using RELEASE_DX_INSTANCE_PTR = void(*)(void);
using SET_PIXEL_COLOR_MASKS_PTR = void(*)(U32 r, U32 g, U32 b);
using INIT_WINDOW_DX_SURFACE_PTR = bool(*)(S32 width, S32 height);

using DRAW_MAIN_SURFACE_HOR_LINE_PTR = void(*)(S32 x, S32 y, S32 length, Pixel pixel);
using DRAW_MAIN_SURFACE_VERT_COLOR_LINE_PTR = void(*)(S32 x, S32 y, S32 length, Pixel pixel);
using DRAW_MAIN_SURFACE_COLOR_RECT_PTR = void(*)(S32 x, S32 y, S32 width, S32 height, Pixel pixel);
using DRAW_MAIN_SURFACE_FILLED_COLOR_RECT_PTR = void(*)(S32 x, S32 y, S32 width, S32 height, Pixel pixel);
using DRAW_MAIN_SURFACE_SHADED_COLOR_RECT_PTR = void(*)(S32 x, S32 y, S32 width, S32 height, Pixel pixel);
using DRAW_MAIN_SURFACE_COLOR_POINT_PTR = void(*)(S32 x, S32 y, Pixel pixel);
using DRAW_BACK_SURFACE_COLOR_POINT_PTR = void(*)(S32 x, S32 y, Pixel pixel);
using READ_MAIN_SURFACE_RECT_PTR = void(*)(S32 sx, S32 sy, S32 width, S32 height, S32 dx, S32 dy, S32 stride, Pixel* surface);
using CONVERT_NOT_MAGENTA_COLORS_PTR = void(*)(const Pixel* input, Pixel* output, const S32 count);
using CONVERT_ALL_COLORS_PTR = void(*)(const Pixel* input, Pixel* output, const S32 count);
using COPY_MAIN_BACK_SURFACES_PTR = void(*)(S32 x, S32 y);
using CALL_DRAWBACK_SURFACE_RHOMB_PTR = void(*)(S32 tx, S32 ty, S32 param_3, S32 param_4, S32 param_5, S32 param_6, ImagePaletteTile* input);
using FUN_10001ED0_PTR = void(*)(S32 param_1, S32 param_2, S32 param_3, S32 param_4, S32 param_5, S32 param_6);
using FUN_10001F10_PTR = void(*)(S32 param_1, S32 param_2, S32 param_3);
using FUN_10001F40_PTR = void(*)(S32 param_1, S32 param_2, S32 param_3, S32 param_4, S32 param_5, S32 param_6, S32 param_7);

using COPY_BACK_TO_MAIN_SURFACE_RECT_PTR = void(*)(S32 x, S32 y, U32 width, U32 height);
using DRAW_MAIN_SURFACE_COLOR_ELLIPSE_PTR = void(*)(S32 x, S32 y, S32 size, Pixel pixel, S32 step);
using DRAW_MAIN_SURFACE_COLOR_OUTLINE_PTR = void(*)(S32 x, S32 y, S32 width, S32 height, Pixel pixel);
using DRAW_STENCIL_SURFACE_WINDOW_RECT_PTR = void(*)(void);
using MASK_STENCIL_SURFACE_RECT_PTR = void(*)(S32 x, S32 y, S32 width, S32 height);

using LOCK_DX_SURFACE_PTR = bool(*)(void);
using UNLOCK_DX_SURFACE_PTR = void(*)(void);

using COPY_RENDERER_SURFACE_RECT_TO_PTR = bool(*)(S32 sx, S32 sy, S32 width, S32 height, S32 dx, S32 dy, S32 stride, Pixel* pixels);
using COPY_PIXEL_RECT_FROM_TO_PTR = void(*)(S32 sx, S32 sy, S32 sstr, Pixel* input, S32 dx, S32 dy, S32 dstr, Pixel* output, S32 width, S32 height);
using COPY_MAIN_SURFACE_TO_RENDERER_PTR = bool(*)(S32 x, S32 y, S32 width, S32 height);
using COPY_MAIN_SURFACE_TO_RENDERER_WITH_WAR_FOG_PTR = void(*)(S32 x, S32 y, S32 width, S32 height);
using GET_TEXT_LENGTH = S32(*)(const char* text, const AssetCollection* asset);

using FUN_10002FB0_PTR = void(*)(S32 x, S32 y, S32 width, S32 height);
using FUN_10003360_PTR = void(*)(S32 x, S32 y, LPSTR text, AssetCollection* asset, Pixel* palette);
using FUN_100033C0_PTR = void(*)(S32 x, S32 y, LPSTR text, AssetCollection* asset, Pixel* palette);
using FUN_10004390_PTR = void(*)(S32 param_1, S32 param_2, LPVOID param_3);
using FUN_100046B6_PTR = void(*)(S32 param_1, S32 param_2, LPVOID param_3);
using FUN_100049E6_PTR = void(*)(S32 param_1, S32 param_2, U16 param_3, LPVOID param_4);
using FUN_10004DB0_PTR = void(*)(S32 x, S32 y, U16 param_3, S32 param_4, LPVOID param_5);

using DRAW_MAIN_SURFACE_PALETTE_SPRITE_PTR = void(*)(S32 x, S32 y, const Pixel* palette, const ImagePaletteSprite* const sprite);
using DRAW_MAIN_SURFACE_VANISHING_SPRITE_PTR = void(*)(S32 x, S32 y, S32 vanishOffset, const Pixel* palette, const ImagePaletteSprite* const sprite);
using DRAW_BACK_SURFACE_PALETTE_SPRITE_PTR = void(*)(S32 x, S32 y, const Pixel* const palette, const ImagePaletteSprite* const sprite);
using FUN_10005AC6_PTR = void(*)(S32 param_1, S32 param_2, U16 param_3, S32 param_4, LPVOID param_5);
using DRAW_BACK_SURFACE_PALETTE_SHADED_SPRITE_PTR = void(*)(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);

using FUN_1000618D_PTR = void(*)(S32 x, S32 y, S32 param_3, LPVOID param_4);
using FUN_100067AD_PTR = void(*)(S32 x, S32 y, S32 param_3, LPVOID param_4);

using DRAW_MAIN_SURFACE_ANIMATION_SPRITE_PTR = void(*)(S32 x, S32 y, U16 level, const AnimationPixel* palette, ImagePaletteSprite* sprite);
using DRAW_MAIN_SURFACE_PALETTE_PTR = void(*)(S32 x, S32 y, ImageSprite* sprite);
using DRAW_MAIN_SURFACE_SPRITE_FRONT_PTR = void(*)(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);

using DRAW_MAIN_SURFACE_SPRITE_BACK_PTR = void(*)(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);
using DRAW_MAIN_SURFACE_SHADOW_SPRITE_PTR = void(*)(S32 x, S32 y, const DoublePixel shadePixel, const ImagePaletteSprite* const sprite);
using DRAW_BACK_SURFACE_SHADOW_SPRITE_PTR = void(*)(S32 x, S32 y, const DoublePixel shadePixel, const ImagePaletteSprite* const sprite);
using DRAW_MAIN_SURFACE_ADJUSTED_SPRITE_PTR = void(*)(S32 x, S32 y, U16 level, const ImagePaletteSprite* const sprite);
using DRAW_MAIN_SURFACE_ACTUAL_SPRITE_PTR = void(*)(S32 x, S32 y, U16 param_3, const Pixel* const palette, const ImagePaletteSprite* const sprite);
using FUN_10008ECD_PTR = void(*)(S32 param_1, S32 param_2, LPVOID param_3, S32 param_4, LPVOID param_5);
using MARK_UI_WITH_BUTTON_TYPE_PTR = void(*)(S32 param_1, S32 param_2, ImagePaletteSprite* const sprite, const ButtonType type, const ImageSpriteUI* const uiSprite, const ButtonType* const offset);
using DRAW_VANISHING_UI_SPRITE_PTR = void(*)(S32 x, S32 y, const S32 vanishLevel, const Pixel* palette, ImagePaletteSprite* const sprite, const ImageSpriteUI* const uiSprite);

struct RendererActions
{
    INIT_VALUES_PTR                                 initValues;
    INIT_DX_INSTANCE_PTR                            initDxInstance;
    RESTORE_DX_INSTANCE_PTR                         restoreDxInstance;
    INIT_WINDOW_DX_SURFACE_PTR                      initWindowDxSurface;
    SET_PIXEL_COLOR_MASKS_PTR                       setPixelColorMasks;
    RELEASE_DX_SURFACE_PTR                          releaseDxSurface;
    LOCK_DX_SURFACE_PTR                             lockDxSurface;
    UNLOCK_DX_SURFACE_PTR                           unlockDxSurface;
    COPY_MAIN_BACK_SURFACES_PTR                     copyMainBackSurfaces;
    CONVERT_NOT_MAGENTA_COLORS_PTR                  convertNotMagentaColors;
    CONVERT_ALL_COLORS_PTR                          convertAllColors;
    GET_TEXT_LENGTH                                 getTextLength;
    FUN_100033C0_PTR                                FUN_100033c0;
    FUN_10003360_PTR                                FUN_10003360;
    CALL_DRAWBACK_SURFACE_RHOMB_PTR                 callDrawBackSurfaceRhomb;
    FUN_10001F10_PTR                                FUN_10001f10;
    FUN_10004390_PTR                                FUN_10004390;
    FUN_100046B6_PTR                                FUN_100046b6;
    FUN_100049E6_PTR                                FUN_100049e6;
    DRAW_BACK_SURFACE_PALETTE_SHADED_SPRITE_PTR     drawBackSurfacePaletteShadedSprite;
    FUN_10005AC6_PTR                                FUN_10005ac6;
    DRAW_BACK_SURFACE_PALETTE_SPRITE_PTR            drawBackSurfacePalletteSprite;
    DRAW_BACK_SURFACE_SHADOW_SPRITE_PTR             drawBackSurfaceShadowSprite;
    COPY_BACK_TO_MAIN_SURFACE_RECT_PTR              copyBackToMainSurfaceRect;
    DRAW_BACK_SURFACE_COLOR_POINT_PTR               drawBackSurfaceColorPoint;
    FUN_10001ED0_PTR                                FUN_10001ed0;
    FUN_10001F40_PTR                                FUN_10001f40;
    FUN_10002FB0_PTR                                FUN_10002fb0_0;
    DRAW_MAIN_SURFACE_PALETTE_SPRITE_PTR            drawMainSurfacePaletteSprite;
    DRAW_MAIN_SURFACE_PALETTE_PTR                   drawMainSurfaceSprite;
    FUN_1000618D_PTR                                FUN_1000618d;
    FUN_10004DB0_PTR                                FUN_10004db0;
    DRAW_MAIN_SURFACE_SPRITE_FRONT_PTR              drawMainSurfaceSpriteFront;
    DRAW_MAIN_SURFACE_SPRITE_BACK_PTR               drawMainSurfaceSpriteBack;
    DRAW_MAIN_SURFACE_ANIMATION_SPRITE_PTR          drawMainSurfaceAnimationSprite;
    FUN_100067AD_PTR                                FUN_100067ad;
    DRAW_MAIN_SURFACE_SHADOW_SPRITE_PTR             drawMainSurfaceShadowSprite;
    DRAW_MAIN_SURFACE_ACTUAL_SPRITE_PTR             drawMainSurfaceActualSprite;
    DRAW_MAIN_SURFACE_ADJUSTED_SPRITE_PTR           drawMainSurfaceAdjustedSprite;
    DRAW_MAIN_SURFACE_VANISHING_SPRITE_PTR          drawMainSurfaceVanishingSprite;
    DRAW_MAIN_SURFACE_COLOR_POINT_PTR               drawMainSurfaceColorPoint;
    DRAW_MAIN_SURFACE_FILLED_COLOR_RECT_PTR         drawMainSurfaceFilledColorRect;
    DRAW_MAIN_SURFACE_COLOR_RECT_PTR                drawMainSurfaceColorRect;
    DRAW_MAIN_SURFACE_HOR_LINE_PTR                  drawMainSurfaceHorLine;
    DRAW_MAIN_SURFACE_VERT_COLOR_LINE_PTR           drawMainSurfaceVertLine;
    DRAW_MAIN_SURFACE_SHADED_COLOR_RECT_PTR         drawMainSurfaceShadeColorRect;
    DRAW_MAIN_SURFACE_COLOR_OUTLINE_PTR             drawMainSurfaceColorOutline;
    DRAW_MAIN_SURFACE_COLOR_ELLIPSE_PTR             drawMainSurfaceColorEllipse;
    COPY_MAIN_SURFACE_TO_RENDERER_WITH_WAR_FOG_PTR  copyMainSurfaceToRendererWithWarFog;
    COPY_MAIN_SURFACE_TO_RENDERER_PTR               copyMainSurfaceToRenderer;
    FUN_10002FB0_PTR                                FUN_10002fb0_1;
    READ_MAIN_SURFACE_RECT_PTR                      readMainSurfaceRect;
    MASK_STENCIL_SURFACE_RECT_PTR                   maskStencilSurfaceRect;
    DRAW_STENCIL_SURFACE_WINDOW_RECT_PTR            drawStencilSurfaceWindowRect;
    COPY_RENDERER_SURFACE_RECT_TO_PTR               copyToRendererSurfaceRect;
    COPY_PIXEL_RECT_FROM_TO_PTR                     copyPixelRectFromTo;
    FUN_10008ECD_PTR                                FUN_10008ecd;
    DRAW_VANISHING_UI_SPRITE_PTR                    drawVanishingUiSprite;
    MARK_UI_WITH_BUTTON_TYPE_PTR                    markUiWithButtonType;
    RELEASE_DX_INSTANCE_PTR                         releaseDxInstance;
};

struct Rect
{
    S32 x;              //10012af0
    S32 y;              //10012af4
    S32 width;          //10012af8
    S32 height;         //10012afc
};

struct RendererSurface // TODO Реорганизовать структуру.
{
    S32     offset;     //10012b00  // Surface offset in bytes. Depends on screen coordinates, changes when it moves
    S32     y;          //10012b04 эта величина равна высоте экрана, в данном случае всегда 768

    S32     width;      //10012b08  // Width in pixels
    S32     height;     //10012b0c  // Height in pixels

    S32     stride;     //10012b10  // Width * 2 (sizeof Pixel)

    Pixel*  main;        //10012b14 Содержит конечное изображение кадра, исключая пользовательский интерфейс.
    Pixel*  back;        //10012b18 Удерживает фон кадра, включая землю, здания, рельсы, деревья, кусты и т.д.
    Pixel*  stencil;     //10012b1c Содержит буфер трафарета для рамки. Сюда входят здания, заборы, столбы электропередач.

    void*   renderer;   //10012b20 Поверхность DirectDraw.
};

struct ModuleStructTest01
{
    U8* fogPtr;                 //10012ad8    // Pointer to current fog tile. The previous and next fog tiles are used for calculations
    U32 rowAlignmentMask;       //10012adc Битовый флаг для выравнивания строк.
    S32 lineStep;               //10012ae0 Шаг рендеринга(1556512 — возможно, смещение в памяти).
    S32 dstRowStride;           //10012ae4
    S32 actualRgbMask;          //10012ae8
};

struct Fog
{
    U8    unk[0x50]; // TODO
};

struct RhombsPalette
{
    Pixel     palette[GRAPHICS_SIZE_PALETTE * 67];
};

struct Renderer
{
    Rect                   windowRect;
    RendererSurface        surface;

    U16                    actualRedMask;                   //10012b24
    U16                    initialRedMask;                  //10012b26
    U16                    actualGreenMask;                 //10012b28
    U16                    initialGreenMask;                //10012b2a
    U16                    actualBlueMask;                  //10012b2с
    U16                    initialBlueMask;                 //10012b2e

    U16                    redOffset;                       //10012b30
    U16                    greenOffset;                     //10012b32
    U16                    blueOffset;                      //10012b34

    U16                    unk16;                           //10012b38 TODO
    U16                    actualColorBits;                 //10012b3a
    U16                    unk18;                           //10012b3c TODO
    U16                    actualColorMask;                 //10012b3e
    U16                    initialColorMask;                //10012b40
    U16                    shadeColorMask;                  //10012b42
    U16                    shadeColorMask2;                 //10012b44
    U16                    unk23;                           //10012b46 TODO
    U16                    unk24;                           //10012b48 TODO
    U32                    initialRgbMask;                  //10012b4c
    U32                    dummy;                           //Выравнивание без него шиш
    U32                    actualRgbMask;                   //10012b50
    U32                    pitch;                           //10012b54              // Size in bytes of pixel line in DX renderer. Usually is: width * 2
    DoublePixel            backSurfaceShadePixel;           //10012b58
    Fog                    fogSprites[112];                 //10012b5a-10014e5b     // Describes fog of war
    RhombsPalette          rhombsPalette;                   //10014e5c массив палитры ландшафта (rhomb.pl)

    HWND                   hwnd;
    bool                   isFullScreen;                    //1001d478
    DirectX                directX;
    RendererActions        actions;

    U32 dword_10018EA4;

    ModuleStructTest01 moduleStruct01;
};


typedef Renderer*(*RENDERERINITACTIONLAMBDA)(void);

typedef BOOL(*ACQUIRERENDERERSETTINGSVALUELAMBDA)(void);

extern Renderer g_moduleState;