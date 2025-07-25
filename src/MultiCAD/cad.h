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


using INIT_VALUES_PTR               = void(*)();
using INIT_DX_INSTANCE_PTR          = bool(*)(HWND hwnd, bool fullscreen);
using RELEASE_DX_SURFACE_PTR        = void(*)();
using RESTORE_DX_INSTANCE_PTR       = void(*)();
using RELEASE_DX_INSTANCE_PTR       = void(*)();
using SET_PIXEL_COLOR_MASKS_PTR     = void(*)(U32 r, U32 g, U32 b);
using INIT_WINDOW_DX_SURFACE_PTR    = bool(*)(S32 width, S32 height);

using DRAW_MAIN_SURFACE_HOR_LINE_PTR            = void(*)(S32 x, S32 y, S32 length, Pixel pixel);
using DRAW_MAIN_SURFACE_VERT_COLOR_LINE_PTR     = void(*)(S32 x, S32 y, S32 length, Pixel pixel);
using DRAW_MAIN_SURFACE_COLOR_RECT_PTR          = void(*)(S32 x, S32 y, S32 width, S32 height, Pixel pixel);
using DRAW_MAIN_SURFACE_FILLED_COLOR_RECT_PTR   = void(*)(S32 x, S32 y, S32 width, S32 height, Pixel pixel);
using DRAW_MAIN_SURFACE_SHADED_COLOR_RECT_PTR   = void(*)(S32 x, S32 y, S32 width, S32 height, Pixel pixel);
using DRAW_MAIN_SURFACE_COLOR_POINT_PTR         = void(*)(S32 x, S32 y, Pixel pixel);
using DRAW_BACK_SURFACE_COLOR_POINT_PTR         = void(*)(S32 x, S32 y, Pixel pixel);

using READ_MAIN_SURFACE_RECT_PTR        = void(*)(S32 sx, S32 sy, S32 width, S32 height, S32 dx, S32 dy, S32 stride, Pixel* surface);
using CONVERT_NOT_MAGENTA_COLORS_PTR    = void(*)(const Pixel* input, Pixel* output, S32 count);
using CONVERT_ALL_COLORS_PTR            = void(*)(const Pixel* input, Pixel* output, S32 count);

using COPY_MAIN_BACK_SURFACES_PTR               = void(*)(S32 x, S32 y);
using CALL_DRAW_BACK_SURFACE_PALETTE_RHOMB_PTR  = void(*)(S32 tx, S32 ty, S32 angle_0, S32 angle_1, S32 angle_2, S32 angle_3, const ImagePaletteTile* const tile);
using CALL_SHADE_MAIN_SURFACE_RHOMB_PTR         = void(*)(S32 x, S32 y, S32 angle_0, S32 angle_1, S32 angle_2, S32 angle_3);
using CALL_DRAW_BACK_SURFACE_MASK_RHOMB_PTR     = void(*)(S32 param_1, S32 param_2, S32 param_3);
using CALL_CLEAN_MAIN_SURFACE_RHOMB_PTR         = void(*)(S32 tx, S32 ty, S32 angle_0, S32 angle_1, S32 angle_2, S32 angle_3, const ImagePaletteTile* const tile);

using COPY_BACK_TO_MAIN_SURFACE_RECT_PTR    = void(*)(S32 x, S32 y, U32 width, U32 height);
using DRAW_MAIN_SURFACE_COLOR_ELLIPSE_PTR   = void(*)(S32 x, S32 y, S32 size, Pixel pixel, S32 step);
using DRAW_MAIN_SURFACE_COLOR_OUTLINE_PTR   = void(*)(S32 x, S32 y, S32 width, S32 height, Pixel pixel);

using RESET_STENCIL_SURFACE_PTR         = void(*)();
using MASK_STENCIL_SURFACE_RECT_PTR     = void(*)(S32 x, S32 y, S32 width, S32 height);

using LOCK_DX_SURFACE_PTR       = bool(*)();
using UNLOCK_DX_SURFACE_PTR     = void(*)();

using COPY_TO_RENDERER_SURFACE_RECT_TO_PTR              = bool(*)(S32 sx, S32 sy, S32 width, S32 height, S32 dx, S32 dy, S32 stride, const Pixel* const pixels);
using COPY_PIXEL_RECT_FROM_TO_PTR                       = void(*)(S32 sx, S32 sy, S32 sstr, const Pixel* const input, S32 dx, S32 dy, S32 dstr, Pixel* const output, S32 width, S32 height);
using COPY_MAIN_SURFACE_TO_RENDERER_PTR                 = bool(*)(S32 x, S32 y, S32 width, S32 height);
using COPY_MAIN_SURFACE_TO_RENDERER_WITH_WAR_FOG_PTR    = void(*)(S32 x, S32 y, S32 width, S32 height);
using BLEND_MAIN_SURFACE_WITH_WAR_FOG_PTR               = void(*)(S32 x, S32 y, S32 width, S32 height);

using GET_TEXT_LENGTH_PTR           = S32(*)(const char* const str, const AssetCollection* const asset);
using DRAW_MAIN_SURFACE_TEXT_PTR    = void(*)(S32 x, S32 y, const char* const str, const AssetCollection* const asset, const Pixel* const palette);
using DRAW_BACK_SURFACE_TEXT_PTR    = void(*)(S32 x, S32 y, const char* const str, const AssetCollection* const asset, const Pixel* const palette);

using DRAW_BACK_SURFACE_RHOMBS_PALETTE_SPRITE_PTR           = void(*)(S32 x, S32 y, const ImagePaletteSprite* const sprite);
using DRAW_BACK_SURFACE_RHOMBS_PALETTE_SPRITE2_PTR          = void(*)(S32 x, S32 y, const ImagePaletteSprite* const sprite);
using DRAW_BACK_SURFACE_RHOMBS_PALETTE_SHADED_SPRTITE_PTR   = void(*)(S32 x, S32 y, U16 level, const ImagePaletteSprite* const sprite);

using DRAW_MAIN_SURFACE_PALETTE_SPRITE_STENCIL_PTR      = void(*)(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);
using DRAW_MAIN_SURFACE_PALETTE_SPRITE_COMPACT_PTR      = void(*)(S32 x, S32 y, const Pixel* palette, const ImagePaletteSprite* const sprite);
using DRAW_MAIN_SURFACE_VANISHING_PALETTE_SPRITE_PTR    = void(*)(S32 x, S32 y, S32 vanishOffset, const Pixel* palette, const ImagePaletteSprite* const sprite);
using DRAW_BACK_SURFACE_PALETTE_SPRITE_PTR              = void(*)(S32 x, S32 y, const Pixel* const palette, const ImagePaletteSprite* const sprite);
using DRAW_BACK_SURFACE_PALETTE_SPRITE_AND_STENCIL_PTR  = void(*)(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);
using DRAW_BACK_SURFACE_PALETTE_SHADED_SPRITE_PTR       = void(*)(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);

using DRAW_MAIN_SURFACE_PALETTE_SPRITE_PTR                  = void(*)(S32 x, S32 y, const Pixel* const palette, const ImagePaletteSprite* const sprite);
using DRAW_MAIN_SURFACE_SPRITE_PTR                          = void(*)(S32 x, S32 y, const ImageSprite* const sprite);
using DRAW_MAIN_SURFACE_ANIMATION_SPRITE_PTR                = void(*)(S32 x, S32 y, const AnimationPixel* palette, const ImagePaletteSprite* const sprite);
using DRAW_MAIN_SURFACE_ANIMATION_SPRITE_STENCIL_PTR        = void(*)(S32 x, S32 y, U16 level, const AnimationPixel* palette, ImagePaletteSprite* sprite);
using DRAW_MAIN_SURFACE_PALETTE_SPRITE_FRONT_STENCIL_PTR    = void(*)(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);
using DRAW_MAIN_SURFACE_PALETTE_SPRITE_BACK_STENCIL_PTR     = void(*)(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);

using DRAW_MAIN_SURFACE_SHADOW_SPRITE_PTR       = void(*)(S32 x, S32 y, DoublePixel shadePixel, const ImagePaletteSprite* const sprite);
using DRAW_BACK_SURFACE_SHADOW_SPRITE_PTR       = void(*)(S32 x, S32 y, DoublePixel shadePixel, const ImagePaletteSprite* const sprite);
using DRAW_MAIN_SURFACE_ADJUSTED_SPRITE_PTR     = void(*)(S32 x, S32 y, U16 level, const ImagePaletteSprite* const sprite);
using DRAW_MAIN_SURFACE_ACTUAL_SPRITE_PTR       = void(*)(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);

using DRAW_UI_SPRITE_PTR                = void(*)(S32 x, S32 y, const ImagePaletteSprite* const sprite, const void* palette, const ImageSpriteUI* const uiSprite);
using MARK_UI_WITH_BUTTON_TYPE_PTR      = void(*)(S32 x, S32 y, const ImagePaletteSprite* const sprite, ButtonType type, const ImageSpriteUI* const uiSprite, const ButtonType* const offset);
using DRAW_VANISHING_UI_SPRITE_PTR      = void(*)(S32 x, S32 y, S32 vanishLevel, const Pixel* palette, ImagePaletteSprite* const sprite, const ImageSpriteUI* const uiSprite);

struct RendererActions
{
    INIT_VALUES_PTR                                         initValues;
    INIT_DX_INSTANCE_PTR                                    initDxInstance;
    RESTORE_DX_INSTANCE_PTR                                 restoreDxInstance;
    INIT_WINDOW_DX_SURFACE_PTR                              initWindowDxSurface;
    SET_PIXEL_COLOR_MASKS_PTR                               setPixelColorMasks;
    RELEASE_DX_SURFACE_PTR                                  releaseDxSurface;
    LOCK_DX_SURFACE_PTR                                     lockDxSurface;
    UNLOCK_DX_SURFACE_PTR                                   unlockDxSurface;
    COPY_MAIN_BACK_SURFACES_PTR                             copyMainBackSurfaces;
    CONVERT_NOT_MAGENTA_COLORS_PTR                          convertNotMagentaColors;
    CONVERT_ALL_COLORS_PTR                                  convertAllColors;
    GET_TEXT_LENGTH_PTR                                     getTextLength;
    DRAW_BACK_SURFACE_TEXT_PTR                              drawBackSurfaceText;
    DRAW_MAIN_SURFACE_TEXT_PTR                              drawMainSurfaceText;
    CALL_DRAW_BACK_SURFACE_PALETTE_RHOMB_PTR                callDrawBackSurfacePaletteRhomb;
    CALL_DRAW_BACK_SURFACE_MASK_RHOMB_PTR                   callDrawBackSurfaceMaskRhomb;
    DRAW_BACK_SURFACE_RHOMBS_PALETTE_SPRITE_PTR             drawBackSurfaceRhombsPaletteSprite;
    DRAW_BACK_SURFACE_RHOMBS_PALETTE_SPRITE2_PTR            drawBackSurfaceRhombsPaletteSprite2;
    DRAW_BACK_SURFACE_RHOMBS_PALETTE_SHADED_SPRTITE_PTR     drawBackSurfaceRhombsPaletteShadedSprite;
    DRAW_BACK_SURFACE_PALETTE_SHADED_SPRITE_PTR             drawBackSurfacePaletteShadedSprite;
    DRAW_BACK_SURFACE_PALETTE_SPRITE_AND_STENCIL_PTR        drawBackSurfacePaletteSpriteAndStencil;
    DRAW_BACK_SURFACE_PALETTE_SPRITE_PTR                    drawBackSurfacePalletteSprite;
    DRAW_BACK_SURFACE_SHADOW_SPRITE_PTR                     drawBackSurfaceShadowSprite;
    COPY_BACK_TO_MAIN_SURFACE_RECT_PTR                      copyBackToMainSurfaceRect;
    DRAW_BACK_SURFACE_COLOR_POINT_PTR                       drawBackSurfaceColorPoint;
    CALL_SHADE_MAIN_SURFACE_RHOMB_PTR                       callShadeMainSurfaceRhomb;
    CALL_CLEAN_MAIN_SURFACE_RHOMB_PTR                       callCleanMainSurfaceRhomb;
    BLEND_MAIN_SURFACE_WITH_WAR_FOG_PTR                     blendMainSurfaceWithWarFog_0;
    DRAW_MAIN_SURFACE_PALETTE_SPRITE_COMPACT_PTR            drawMainSurfacePaletteSpriteCompact;
    DRAW_MAIN_SURFACE_SPRITE_PTR                            drawMainSurfaceSprite;
    DRAW_MAIN_SURFACE_PALETTE_SPRITE_PTR                    drawMainSurfacePaletteSprite;
    DRAW_MAIN_SURFACE_PALETTE_SPRITE_STENCIL_PTR            drawMainSurfacePaletteSpriteStencil;
    DRAW_MAIN_SURFACE_PALETTE_SPRITE_FRONT_STENCIL_PTR      drawMainSurfacePaletteSpriteFrontStencil;
    DRAW_MAIN_SURFACE_PALETTE_SPRITE_BACK_STENCIL_PTR       drawMainSurfacePaletteSpriteBackStencil;
    DRAW_MAIN_SURFACE_ANIMATION_SPRITE_STENCIL_PTR          drawMainSurfaceAnimationSpriteStencil;
    DRAW_MAIN_SURFACE_ANIMATION_SPRITE_PTR                  drawMainSurfaceAnimationSprite;
    DRAW_MAIN_SURFACE_SHADOW_SPRITE_PTR                     drawMainSurfaceShadowSprite;
    DRAW_MAIN_SURFACE_ACTUAL_SPRITE_PTR                     drawMainSurfaceActualSprite;
    DRAW_MAIN_SURFACE_ADJUSTED_SPRITE_PTR                   drawMainSurfaceAdjustedSprite;
    DRAW_MAIN_SURFACE_VANISHING_PALETTE_SPRITE_PTR          drawMainSurfaceVanishingPaletteSprite;
    DRAW_MAIN_SURFACE_COLOR_POINT_PTR                       drawMainSurfaceColorPoint;
    DRAW_MAIN_SURFACE_FILLED_COLOR_RECT_PTR                 drawMainSurfaceFilledColorRect;
    DRAW_MAIN_SURFACE_COLOR_RECT_PTR                        drawMainSurfaceColorRect;
    DRAW_MAIN_SURFACE_HOR_LINE_PTR                          drawMainSurfaceHorLine;
    DRAW_MAIN_SURFACE_VERT_COLOR_LINE_PTR                   drawMainSurfaceVertLine;
    DRAW_MAIN_SURFACE_SHADED_COLOR_RECT_PTR                 drawMainSurfaceShadeColorRect;
    DRAW_MAIN_SURFACE_COLOR_OUTLINE_PTR                     drawMainSurfaceColorOutline;
    DRAW_MAIN_SURFACE_COLOR_ELLIPSE_PTR                     drawMainSurfaceColorEllipse;
    COPY_MAIN_SURFACE_TO_RENDERER_WITH_WAR_FOG_PTR          copyMainSurfaceToRendererWithWarFog;
    COPY_MAIN_SURFACE_TO_RENDERER_PTR                       copyMainSurfaceToRenderer;
    BLEND_MAIN_SURFACE_WITH_WAR_FOG_PTR                     blendMainSurfaceWithWarFog_1;
    READ_MAIN_SURFACE_RECT_PTR                              readMainSurfaceRect;
    MASK_STENCIL_SURFACE_RECT_PTR                           maskStencilSurfaceRect;
    RESET_STENCIL_SURFACE_PTR                               resetStencilSurface;
    COPY_TO_RENDERER_SURFACE_RECT_TO_PTR                    copyToRendererSurfaceRect;
    COPY_PIXEL_RECT_FROM_TO_PTR                             copyPixelRectFromTo;
    DRAW_UI_SPRITE_PTR                                      drawUiSprite;
    DRAW_VANISHING_UI_SPRITE_PTR                            drawVanishingUiSprite;
    MARK_UI_WITH_BUTTON_TYPE_PTR                            markUiWithButtonType;
    RELEASE_DX_INSTANCE_PTR                                 releaseDxInstance;
};

struct Rect
{
    S32 x;              //10012af0
    S32 y;              //10012af4
    S32 width;          //10012af8
    S32 height;         //10012afc
};

struct RendererSurface
{
    S32     offset;     //10012b00      // Surface offset in bytes. Depends on screen coordinates, changes when it moves
    S32     y;          //10012b04      // Usually it is height of the window screen

    S32     width;      //10012b08      // Width in pixels
    S32     height;     //10012b0c      // Height in pixels

    S32     stride;     //10012b10      // Width * 2 (sizeof Pixel)

    Pixel*  main;        //10012b14     // Contains the final image of the frame, excluding the user interface
    Pixel*  back;        //10012b18     // Holds the background of the frame, including the ground, buildings, rails, trees, bushes, etc
    Pixel*  stencil;     //10012b1c     // Contains the stencil buffer for the frame. This includes buildings, fences, utility poles

    void*   renderer;   //10012b20      // DirectDraw surface
};

struct ModuleStructTest01
{
    U8* fogPtr;                 //10012ad8      // Pointer to current fog tile. The previous and next fog tiles are used for calculations
    U32 blocksCount;            //10012adc      // Number of blocks to be copied
    S32 lineStep;               //10012ae0      // Size in bytes between two lines
    S32 dstRowStride;           //10012ae4
    S32 actualRgbMask;          //10012ae8
};

struct Fog
{
    U8    unk[0x50];        // Fog array for visible screen
};

struct RhombsPalette
{
    Pixel palette[GRAPHICS_SIZE_PALETTE * 67];
};

struct Renderer
{
    Rect                   windowRect;
    RendererSurface        surface;

    U16                    actualRedMask;                   //10012b24
    U16                    initialRedMask;                  //10012b26
    U16                    actualGreenMask;                 //10012b28
    U16                    initialGreenMask;                //10012b2a
    U16                    actualBlueMask;                  //10012b2ñ
    U16                    initialBlueMask;                 //10012b2e

    U16                    redOffset;                       //10012b30
    U16                    greenOffset;                     //10012b32
    U16                    blueOffset;                      //10012b34

    U16                    pad_1;                           //10012b38              // Not used
    U16                    actualColorBits;                 //10012b3a
    U16                    actualColorBits2;                //10012b3c              // Just for actualColorBits mask of size of DoublePixel
    U16                    actualColorMask;                 //10012b3e
    U16                    initialColorMask;                //10012b40
    U16                    shadeColorMask;                  //10012b42
    U16                    shadeColorMask2;                 //10012b44              // Just for shadeColorMask mask of size of DoublePixel
    U16                    invActualColorBits;              //10012b46
    U16                    invActualColorBits2;             //10012b48              // Just for invActualColorBits mask of size of DoublePixel
    U32                    initialRgbMask;                  //10012b4c
    U32                    pad_2;                                                   // Used for padding. Without it game_dll won't work correctly
    U32                    actualRgbMask;                   //10012b50
    U32                    pitch;                           //10012b54              // Size in bytes of pixel line in DX renderer. Usually is: width * 2
    DoublePixel            backSurfaceShadePixel;           //10012b58
    Fog                    fogSprites[112];                 //10012b5a-10014e5b     // Describes fog of war
    RhombsPalette          rhombsPalette;                   //10014e5c              // Landscape palette array (rhomb.pl)

    HWND                   hwnd;
    bool                   isFullScreen;                    //1001d478
    DirectX                directX;
    RendererActions        actions;

    U32 dword_10018EA4;

    ModuleStructTest01 moduleStruct01;
};

extern Renderer g_moduleState;