#pragma once

#include "util.h"


enum OutlintDrawOption
{
    OUTLINE_DRAW_OPTION_NONE = 0,
    OUTLINE_DRAW_OPTION_TOP = 1,
    OUTLINE_DRAW_OPTION_BOTTOM = 2,
    OUTLINE_DRAW_OPTION_LEFT = 4,
    OUTLINE_DRAW_OPTION_RIGHT = 8,
    OUTLINE_DRAW_OPTION_MAX = 0x7FFFFFFF
};

enum TileSize
{
    TILE_SIZE_HEIGHT = 32,
    TILE_SIZE_WIDTH = 63
};

enum SpriteType
{
    SPRITE_TYPE_STATIC = 0xA1,
    SPRITE_TYPE_DYNAMIC = 0xA2,
    SPRITE_TYPE_ALPHA = 0xA3,
    SPRITE_TYPE_ANIMATION = 0xA9,
};

struct Surfaces
{
    Pixel main[SCREEN_WIDTH * (SCREEN_HEIGHT + 1)];     // 0x1001d588   // Main buffer. Contains the final image. Used for dynamic objects like units, explosions
    Pixel back[SCREEN_WIDTH * (SCREEN_HEIGHT + 1)];     // 0x1019ddac   // Secondary buffer. Used mainly for static objects like ground, trees, buildings, roads
    Pixel stencil[SCREEN_WIDTH * (SCREEN_HEIGHT + 1)];  // 0x1031e5d0   // Stencil buffer. Used for calculating frame depth for units, buildings, trees, poles
};

struct Outline
{
    OutlintDrawOption   options;                // 0x1001d55c       // Which rectangle sides to draw
    S32                 horizontalStride;       // 0x1001d560       // Number of bytes to skip on horizontal lines
    S32                 stride;                 // 0x1001d564       // Stride between two lines, usually: width * sizeof(Pixel)
    S32                 verticalStride;         // 0x1001d568       // Number of lines to skip on vertical lines
    S32                 height;                 // 0x1001d56c       // Height of the rectangle
    S32                 width;                  // 0x1001d570       // Width of the rectangle
};

struct Sprite
{
    Pixel*  minX;               // 0x1001005c
    Pixel*  maxX;               // 0x10010060
    U32     width;              // 0x10010064
    S32     vanishOffset;       // 0x10010068

    //Rect    windowRect;         // 0x10010070

    Pixel*  x;                  // 0x1001007c

    U32     colorMask;          // 0x10010084
    U32     adjustedColorMask;  // 0x10010088

    S32     height;             // 0x1001009a
    S32     overage;            // 0x1001009e
};

struct Tile
{
    S32 colorMask;              // 0x10010030
    S32 displayedHalfs;         // 0x10010034
    Pixel* stencil;             // 0x10010038
    S8  unk04;                  // 0x1001003c

    S32 diff;                   // 0x1001003d
    S32 height;                 // 0x10010041
    S32 tempHeight;             // 0x10010045
    S8  unk08;                  // 0x10010049

    Rect rect;                  // 0x1001004A
};

struct GameUI
{
    Addr offset;                        // 0x100100b6
    U32 stride;                         // 0x100100ba
    Rect rect;                          // 0x100100be
    U32 type;                           // 0x100100ñe
    ImagePaletteSprite* imageSprite;    // 0x100100d2
};

struct FogBlockParams
{
    S32 unk01;                  // 1049edd0
    U32 tempBlocksCount;        // 1049edd4
    S32 validRowsBlockCount;    // 1049edd8     // Number of valid row blocks (8 rows each)
    U32 unk04;                  // 1049eddc
};

struct FogBlockParams2
{
    S32 unk01;                  // 1001d574
    S32 unk02;                  // 1001d578
    S32 excessRowsBlockCount;   // 1001d57c     // Number of extra row blocks (8 rows each)
    S32 unk04;                  // 1001d580
};

struct RendererState
{
    FogRenderParams fogRenderParams;    // 1001'2ad8
    Surfaces        surfaces;           // 1001'd5d0
    Outline         outline;            // 1001'd55c
    Sprite          sprite;
    Tile            tile;
    GameUI          gameUI;
    FogBlockParams  fogBlockParams;
    FogBlockParams2 fogBlockParams2;
};

extern RendererState g_rendererState;



// 0x10001000
/**
 * Initializes screen contstants for surface and window.
 *
 * @return None.
 */
void initValues();

// 0x10001050
/**
 * Restores and creates a new DX instance, sets cooperative level.
 * 
 * @param hwnd Window handle to be associtaed with the device.
 * @param fullscreen If the game was run in full screen.
 *
 * @return None.
 */
bool initDxInstance(const HWND hwnd, const bool fullscreen);

// 0x100010b0
/**
 * Releases DX surface.
 *
 * @return None.
 */
void releaseDxSurface();

// 0x100010d0
/**
 * Releases DX surface and restores DX instance.
 *
 * @return None.
 */
void restoreDxInstance();

// 0x10001110
/**
 * Releases DX instance.
 *
 * @return None.
 */
void releaseDxInstance();

// 0x10001130
/**
 * Sets r, g, b color masks.
 *
 * @param r Mask for red color.
 * @param g Mask for green color.
 * @param b Mask for blue color.
 *
 * @return None.
 */
void setPixelColorMasks(const U32 r, const U32 g, const U32 b);

// 0x10001330
/**
 * Sets Window position, sets display mode for DX instance, creates DX surface.
 *
 * @param width Width of the game window.
 * @param height Height of the game window.
 *
 * @return False if failed, true on success.
 */
bool initWindowDxSurface(const S32 width, const S32 height);



// 0x10001420
/**
 * Draws a horizontal line on main surface, e.g. unit bars like health bar, experience bar.
 *
 * Created by AM.
 *
 * @param x Initial X coordinate of the line.
 * @param y Initial Y coordinate of the line.
 * @param length Length of the line.
 * @param pixel Color of the line.
 *
 * @return None.
 */
void drawMainSurfaceHorLine(const S32 x, const S32 y, const S32 length, const Pixel pixel);

// 0x100014b0
/**
 * Draws a vertical line on main surface, e.g. partially passenger cells.
 *
 * Created by AM.
 *
 * @param x Initial X coordinate of the line.
 * @param y Initial Y coordinate of the line.
 * @param length Length of the line.
 * @param pixel Color of the line.
 *
 * @return None.
 */
void drawMainSurfaceVertLine(const S32 x, const S32 y, const S32 height, const Pixel pixel);

// 0x10001570
/**
 * Draws a rectangle on main surface.
 *
 * Created by AM.
 *
 * @param x Initial X coordinate of the rectangle.
 * @param y Initial Y coordinate of the rectangle.
 * @param width Width of the rectangle.
 * @param height Height of the rectangle.
 * @param pixel Color of the rectangle.
 *
 * @return None.
 */
void drawMainSurfaceColorRect(S32 x, S32 y, S32 width, S32 height, const Pixel pixel);

// 0x100015d0
/**
 * Draws a filled rectangle on main surface.
 *
 * Created by AM.
 *
 * @param x Initial X coordinate of the rectangle.
 * @param y Initial Y coordinate of the rectangle.
 * @param width Width of the rectangle.
 * @param height Height of the rectangle.
 * @param pixel Color of the rectangle.
 *
 * @return None.
 */
void drawMainSurfaceFilledColorRect(S32 x, S32 y, S32 width, S32 height, const Pixel pixel);

// 0x100016c0
/**
 * Draws a shaded colored rectangle on main surface.
 *
 * Created by AM.
 *
 * @param x Initial X coordinate of the rectangle.
 * @param y Initial Y coordinate of the rectangle.
 * @param width Width of the rectangle.
 * @param height Height of the rectangle.
 * @param pixel Color of the rectangle.
 *
 * @return None.
 */
void drawMainSurfaceShadeColorRect(S32 x, S32 y, S32 width, S32 height, const Pixel pixel);

// 0x100017e0
/**
 * Draws a colored point on main surface.
 * This function is used for displaying the mission minimap in the corner and in a large size.
 * 
 * Created by AM.
 *
 * @param x X coordinate of the point.
 * @param y Y coordinate of the point.
 * @param pixel Preferred color of the point.
 *
 * @return None.
 */
void drawMainSurfaceColorPoint(const S32 x, const S32 y, const Pixel pixel);

// 0x10001840
/**
 * Draws a colored point on back surface.
 * 
 * Created by AM.
 *
 * @param x X coordinate of the point.
 * @param y Y coordinate of the point.
 * @param pixel Preferred color of the point.
 *
 * @return None.
 */
void drawBackSurfaceColorPoint(const S32 x, const S32 y, const Pixel pixel);

// 0x100018a0
/**
 * Copies a rectangle from main surface to a provided surface. Used for coping area behind cursor in game.
 * 
 * Created by AM.
 *
 * @param sx Source X coordinate of the rectangle.
 * @param sy Source Y coordinate of the rectangle.
 * @param width Width of the rectangle.
 * @param height Height of the rectangle.
 * @param dx Destination X coordinate of the rectangle.
 * @param dy Source Y coordinate of the rectangle.
 * @param surface Destination surface which the rectangle is copied to.
 *
 * @return None.
 */
void readMainSurfaceRect(const S32 sx, const S32 sy, const S32 width, const S32 height, const S32 dx, const S32 dy, const S32 stride, Pixel* surface);

// 0x10001be0
/**
 * Transforms each not magenta pixel from the input array into a new format. Pixel should be a 16-bit value.
 * The funciton transform it according to the settings store in g_moduleState.
 * Magenta color is 0xF81F.
 * 
 * Created by AM.
 *
 * @param input Pointer to the start of the input pixel array.
 * @param output Pointer to the start of the output pixel array.
 * @param count Number of pixels to transform.
 *
 * @return None.
 */
void convertNotMagentaColors(const Pixel* input, Pixel* output, const S32 count);

// 0x10001c80
/**
 * Transforms each pixel from the input array into a new format. Pixel should be a 16-bit value.
 * The funciton transform it according to the settings store in g_moduleState.
 * 
 * Created by AM.
 *
 * @param input Pointer to the start of the input pixel array.
 * @param output Pointer to the start of the output pixel array.
 * @param count Number of pixels to transform.
 *
 * @return None.
 */
void convertAllColors(const Pixel* input, Pixel* output, const S32 count);



// 0x10001d00
/**
 * Moves main, back and stencil surfaces according to dx and dy offsets.
 * 
 * Created by IVA 05.2025.
 *
 * @param dx Horizontal offset after camera move. To the right - positive, to the left - negative.
 * @param dy Vertical offset after camera move. To the bottom - positive, to the top - negative.
 *
 * @return None.
 */
void copyMainBackSurfaces(const S32 dx, const S32 dy);

// 0x10001e90
/**
 * Calls function to draw a rhomb on back surface using the specified palette.
 * 
 * Created by AM.
 *
 * @param x Initial X coordinate of the rhomb.
 * @param y Initial Y coordinate of the rhomb.
 * @param angle_0 0th angle of the rhomb.
 * @param angle_1 1th angle of the rhomb.
 * @param angle_2 2th angle of the rhomb.
 * @param angle_3 3th angle of the rhomb.
 * @param tile Tile sprite to be drawn.
 *
 * @return None.
 */
void callDrawBackSurfacePaletteRhomb(const S32 x, const S32 y, const S32 angle_0, const S32 angle_1, const S32 angle_2, const S32 angle_3, const ImagePaletteTile* const tile);

// 0x10001ed0
/**
 * Calls function to blend a rhomb on main surface using the color mask.
 *
 * Created by IVA 19.07.2025.
 *
 * @param x Initial X coordinate of the rhomb.
 * @param y Initial Y coordinate of the rhomb.
 * @param angle_0 0th angle of the rhomb.
 * @param angle_1 1th angle of the rhomb.
 * @param angle_2 2th angle of the rhomb.
 * @param angle_3 3th angle of the rhomb.
 *
 * @return None.
 */
void callShadeMainSurfaceRhomb(const S32 x, const S32 y, const S32 angle_0, const S32 angle_1, const S32 angle_2, const S32 angle_3);

// 0x10001f10
/**
 * Calls function to draw a blended rhomb on main surface using the specified color.
 *
 * Create by IVA 18.07.2025.
 * 
 * @param x Initial X coordinate of the rhomb.
 * @param y Initial Y coordinate of the rhomb.
 * @param color Color with which the source will be combined.
 *
 * @return None.
 */
void callDrawBackSurfaceMaskRhomb(const S32 x, const S32 y, const S32 color);

// 0x10001f40
/**
 * Calls function to clean a rhomb on main surface.
 *
 * Create by IVA 19.07.2025.
 * 
 * @param x Initial X coordinate of the rhomb.
 * @param y Initial Y coordinate of the rhomb.
 * @param angle_0 0th angle of the rhomb.
 * @param angle_1 1th angle of the rhomb.
 * @param angle_2 2th angle of the rhomb.
 * @param angle_3 3th angle of the rhomb.
 * @param tile Tile sprite to be drawn.
 *
 * @return None.
 */
void callCleanMainSurfaceRhomb(const S32 x, const S32 y, const S32 angle_0, const S32 angle_1, const S32 angle_2, const S32 angle_3, const ImagePaletteTile* const tile);

// 0x10001f80
/**
 * Copies rectangle with landscape and static objects from back to main surface.
 *
 * Created by AM. Refactored by IVA.
 * 
 * @param x Initial X coordinate of the rectangle.
 * @param y Initial Y coordinate of the rectangle.
 * @param width Width of the rectangle.
 * @param height Height of the rectangle.
 *
 * @return None.
 */
void copyBackToMainSurfaceRect(const S32 x, const S32 y, const U32 width, const U32 height);

// 0x10002020
/**
 * Draws colored ellipse on main surface.
 *
 * Created by AM.
 * 
 * @param x Initial X coordinate of the ellipse.
 * @param y Initial Y coordinate of the ellipse.
 * @param size Size of the ellipse.
 * @param pixel Color of the ellipse.
 * @param step Smoothing step.
 *
 * @return None.
 */
void drawMainSurfaceColorEllipse(const S32 x, const S32 y, S32 size, const Pixel pixel, const S32 step);

// 0x100023e0
/**
 * Draws colored rectangle on main surface. Draws unit selection rectangle.
 * 
 * Created by AM. Refactored by IVA.
 *
 * @param x Initial X coordinate of the rectangle.
 * @param y Initial Y coordinate of the rectangle.
 * @param width Width of the rectangle.
 * @param height Height of the rectangle.
 * @param Pixel Preferred color.
 *
 * @return None.
 */
void drawMainSurfaceColorOutline(S32 x, S32 y, S32 width, S32 height, const Pixel pixel);

// 0x100026e0
/**
 * Resets stencil surface with values depending on pixel's Y coordinate.
 * 
 * Created by AM.
 *
 * @return None.
 */
void resetStencilSurface();

// 0x10002780
/**
 * Masks a rectangle on stencil surface.
 * 
 * Created by AM.
 *
 * @param x Initial X coordinate of the rectangle.
 * @param y Initial Y coordinate of the rectangle.
 * @param width Width of the rectangle.
 * @param height Height of the rectangle.
 *
 * @return None.
 */
void maskStencilSurfaceRect(S32 x, S32 y, S32 width, S32 height);

// 0x10002810
/**
 * Moves stencil surface vertically depending on global offset.
 * 
 * @param x Initial X coordinate of the rectangle to be moved.
 * @param y Initial Y coordinate of the rectangle to be moved.
 * @param width Width of the rectangle to be moved.
 * @param height Height of the rectangle to be moved.
 * @param offset Vertical offset
 *
 * @return None.
 */
void moveStencilSurface(const S32 x, const S32 y, const S32 width, const S32 height, const S32 offset);

// 0x100028f0
/**
 * Locks DX surface.
 *
 * @return True on success, false on any fail.
 */
bool lockDxSurface();

// 0x10002970
/**
 * Unlocks DX surface.
 *
 * @return None.
 */
void unlockDxSurface();

// 0x10002990
/**
 * Copies a rectangle from the provided pixel array to renderer. Displays minimap, toolbar, menu.
 * 
 * Created by IVA 06.2025.
 *
 * @param sx Initial X coordinate of the rectangle in source array.
 * @param sy Initial Y coordinate of the rectangle in source array.
 * @param width Width of the rectangle to be copied.
 * @param height Height of the rectangle to be copied.
 * @param dx Initial X coordinate of the rectangle in renderer.
 * @param dy Initial Y coordinate of the rectangle in renderer.
 * @param stride Line pitch. Usually double screen width.
 * @param pixels Source pixel array from which the rectangle will be copied.
 *
 * @return None.
 */
bool copyToRendererSurfaceRect(S32 sx, S32 sy, S32 width, S32 height, S32 dx, S32 dy, S32 stride, const Pixel* const pixels);

// 0x10002a30
/**
 * Copies a rectangle from source to destination array.
 * 
 * Created by IVA 06.2025.
 *
 * @param sx Initial X coordinate of the source array.
 * @param sy Initial Y coordinate of the source array.
 * @param sstr Row stride of the source array.
 * @param input Source array.
 * @param dx Initial X coordinate of the destination array.
 * @param dy Initial Y coordinate of the destination array.
 * @param dstr Row stride of the destination array.
 * @param output Destination array.
 * @param width Width to be copied in pixels.
 * @param height Height to be copied in pixels.
 *
 * @return None.
 */
void copyPixelRectFromTo(S32 sx, S32 sy, S32 sstr, const Pixel* const input, S32 dx, S32 dy, S32 dstr, Pixel* const output, S32 width, S32 height);

// 0x10002a90
/**
 * Copies a rectangle from main surface to renderer.
 * 
 * Created by IVA 06.2025.
 *
 * @param x Initial X coordinate of the rectangle.
 * @param y Initial Y coordinate of the rectangle.
 * @param width Width of the rectangle to be copied.
 * @param height Height of the rectangle to be copied.
 *
 * @return True on success, false on any fail.
 */
bool copyMainSurfaceToRenderer(S32 x, S32 y, S32 width, S32 height);

// 0x10002b90
/**
 * Copies a rectangle from main surface to renderer using war fog array.
 * Copies tiles for the main part of the screen (except the area near the minimap due to the UI template).
 *
 * Created by IVA 20.06.2025.
 * 
 * @param x Initial X coordinate of the rectangle.
 * @param y Initial Y coordinate of the rectangle.
 * @param width Width of the rectangle to be copied.
 * @param height Height of the rectangle to be copied.
 *
 * @return None.
 */
void copyMainSurfaceToRendererWithWarFog(const S32 x, const S32 y, const S32 width, const S32 height);

// 0x10002fb0
/**
 * Blends main surface with war fog array.
 * Used to blend a light area near minimap.
 *
 * Created by IVA 19.07.2025.
 *
 * @param x Initial X coordinate of the rectangle.
 * @param y Initial Y coordinate of the rectangle.
 * @param width Width of the rectangle to be copied.
 * @param height Height of the rectangle to be copied.
 *
 * @return None.
 */
void blendMainSurfaceWithWarFog(const S32 x, const S32 y, const S32 width, const S32 height);

// 0x10003320
/**
 * Returns a text length in pixels.
 * Default space between letters is 2 pixels.
 *
 * Created by NR 02.04.2025. Refactored by IVA.
 *
 * @param str Null-terminated ansi string which length should be calculated.
 * @param asset Letter assets.
 *
 * @return Length of the string in pixels.
 */
S32 getTextLength(const char* const str, const AssetCollection* const asset);

// 0x10003360
/**
 * Draws an ansi string on main surface.
 *
 * Created by AM. Refactored by IVA 19.07.2025.
 *
 * @param x Initial X coordinate of the text.
 * @param y Initial Y coordinate of the text.
 * @param str Null-terminated ansi string to be drawn.
 * @param asset Letter assets.
 * @param palette Pixel palette.
 *
 * @return None.
 */
void drawMainSurfaceText(const S32 x, const S32 y, const char* const str, const AssetCollection* const asset, const Pixel* const palette);

// 0x100033c0
/**
 * Draws an ansi string on back surface.
 *
 * Created by AM. Refactored by IVA 19.07.2025.
 *
 * @param x Initial X coordinate of the text.
 * @param y Initial Y coordinate of the text.
 * @param str Null-terminated ansi string to be drawn.
 * @param asset Letter assets.
 * @param palette Pixel palette.
 *
 * @return None.
 */
void drawBackSurfaceText(const S32 x, const S32 y, const char* const str, const AssetCollection* const asset, const Pixel* const palette);


// 0x10003420
/**
 * Draws a palette rhomb on given surface. Draws ground tiles.
 * 
 * Created by NR 25.06.2025. Refactored by IVA.
 *
 * @param angle_0 0th angle of the rhomb.
 * @param angle_1 1th angle of the rhomb.
 * @param angle_2 2th angle of the rhomb.
 * @param angle_3 3th angle of the rhomb.
 * @param x Initial X coordinate of the rhomb.
 * @param y Initial Y coordinate of the rhomb.
 * @param stride Line pitch in bytes, more often double screen width.
 * @param tile Input tile struct.
 * @param output Pixel array to be changed.
 *
 * @return None.
 */
void drawSurfacePaletteRhomb(const S32 angle_0, const S32 angle_1, const S32 angle_2, const S32 angle_3, S32 tx, S32 ty, const S32 stride, const ImagePaletteTile* const tile, Pixel* const output);

// 0x1000381e
/**
 * Shades a rhomb on given surface using color mask.
 * It's unknown for which sprites it is used.
 *
 * Created by by IVA 19.07.2025.
 *
 * @param angle_0 0th angle of the rhomb.
 * @param angle_1 1th angle of the rhomb.
 * @param angle_2 2th angle of the rhomb.
 * @param angle_3 3th angle of the rhomb.
 * @param x Initial X coordinate of the rhomb.
 * @param y Initial Y coordinate of the rhomb.
 * @param stride Line pitch in bytes, more often double screen width.
 * @param tile Input tile struct.
 * @param output Surface pixel array to be changed.
 *
 * @return None.
 */
void shadeSurfaceRhomb(const S32 angle_0, const S32 angle_1, const S32 angle_2, const S32 angle_3, S32 tx, S32 ty, const S32 stride, Pixel* const output);

// 0x10003C48
/**
 * Cleans a rhomb on given surface.
 * It's unknown for which sprites it is used.
 *
 * Created by by IVA 19.07.2025.
 *
 * @param angle_0 0th angle of the rhomb.
 * @param angle_1 1th angle of the rhomb.
 * @param angle_2 2th angle of the rhomb.
 * @param angle_3 3th angle of the rhomb.
 * @param x Initial X coordinate of the rhomb.
 * @param y Initial Y coordinate of the rhomb.
 * @param stride Line pitch in bytes, more often double screen width.
 * @param tile Input tile struct.
 * @param output Surface pixel array to be changed.
 *
 * @return None.
 */
void cleanSurfaceRhomb(const S32 angle_0, const S32 angle_1, const S32 angle_2, const S32 angle_3, S32 tx, S32 ty, const S32 stride, const ImagePaletteTile* const tile, Pixel* const output);

// 0x10004016
/**
 * Draws a rhomb on back surface using specified color mask. Draws pontoones' blue tiles.
 *
 * Created by IVA 18.07.2025.
 *
 * @param x Initial X coordinate of the rhomb.
 * @param y Initial Y coordinate of the rhomb.
 * @param stride Line pitch in bytes, more often double screen width.
 * @param mask Preferred color to be added.
 * @param surface Pixel array to be changed.
 *
 * @return None.
 */
void drawSurfaceMaskRhomb(S32 x, S32 y, const S32 stride, const S32 mask, Pixel* const surface);

// 0x10004390
/**
 * Draws a rhombs palette sprite on back surface using palette offset 0x4100.
 * It's unknown for which sprites it is used.
 *
 * Created by IVA 18.07.2025.
 *
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param level Stencil level.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawBackSurfaceRhombsPaletteSprite(S32 x, S32 y, const ImagePaletteSprite* const sprite);

// 0x100046b6
/**
 * Draws a rhombs palette sprite on back surface using palette offset 0x4200.
 * It's unknown for which sprites it is used.
 *
 * Created by IVA 18.07.2025.
 *
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param level Stencil level.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawBackSurfaceRhombsPaletteSprite2(S32 x, S32 y, const ImagePaletteSprite* const sprite);

// 0x100049e6
/**
 * Draws a rhombs palette sprite on back surface comparing a specified level with stencil and writing new stencil value.
 * It's unknown for which sprites it is used.
 *
 * Created by IVA 18.07.2025.
 *
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param level Stencil level.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawBackSurfaceRhombsPaletteShadedSprite(S32 x, S32 y, U16 level, const ImagePaletteSprite* const sprite);

// 0x10004db0
/**
 * Draws a palette sprite on main surface comparing a specified level with stencil.
 * It's unknown for which sprites it is used.
 *
 * Created by IVA 18.07.2025.
 *
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param level Stencil level.
 * @param palette Pixel palette used with RLE encoding.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawMainSurfacePaletteSpriteStencil(S32 x, S32 y, U16 param_3, const Pixel* const palette, const ImagePaletteSprite* const sprite);

// 0x100050df
/**
 * Draws a sprite on main surface using palette with only 0x80 and 0x00 count mask. Draws text.
 * 
 * Created by IVA 18.07.2025.
 * 
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param palette Pixel palette used with RLE encoding.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawMainSurfacePaletteSpriteCompact(S32 x, S32 y, const Pixel* palette, const ImagePaletteSprite* const sprite);

// 0x100053c3
/**
 * Draws a sprite with specified vanishing level on main surface.
 * 
 * Create by IVA 06.2025.
 * 
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param vanishOffset Value in [0, 31] responsible for the disappearance of the sprite.
 * @param palette Pixel palette used with RLE encoding.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawMainSurfaceVanishingPaletteSprite(S32 x, S32 y, const S32 vanishOffset, const Pixel* palette, const ImagePaletteSprite* const sprite);

// 0x1000579c
/**
 * Draws a sprite on back surface, e.g. roads, bridges, pontoons, cliffs, craters, stumps.
 *
 * Created by IVA 14.07.2025.
 * 
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param palette Pixel palette used with RLE encoding.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawBackSurfacePalletteSprite(S32 x, S32 y, const Pixel* const palette, const ImagePaletteSprite* const sprite);

// 0x10005ac6
/**
 * Draws a sprite on back surface and writes stencil value to stencil surface.
 * It's unknown for which sprites it is used.
 *
 * Created by IVA 18.07.2025.
 *
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param level Stencil level.
 * @param palette Pixel palette used with RLE encoding.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawBackSurfacePaletteSpriteAndStencil(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);

// 0x10005e31
/**
 * Draws a shaded sprite on back surface comparing a specified level with stencil, e.g. trees, fences, some buildings, hedgehogs, etc.
 *
 * Created by IVA 13.07.2025.
 * 
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param level Displaying level.
 * @param palette
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawBackSurfacePaletteShadedSprite(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);


// 0x1000618d
/**
 * Draws a sprite on main surface using palette with full RLE encoding.
 * It's unknown for which sprites it is used.
 *
 * Created by IVA 18.07.2025.
 * 
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param palette Pixel palette used with RLE encoding.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawMainSurfacePaletteSprite(S32 x, S32 y, const Pixel* const palette, const ImagePaletteSprite* const sprite);

// 0x100064b6
/**
 * Draws a sprite on main surface, e.g. main menu.
 * 
 * Create by AM. Refactored by IVA.
 *
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param palette Pixel palette used with RLE encoding.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawMainSurfaceSprite(S32 x, S32 y, const ImageSprite* const sprite);

// 0x100067ad
/**
 * Draws an animation sprite on main surface.
 * It's unknown for which sprites it is used.
 *
 * Created by IVA 18.07.2025.
 *
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param palette Pixel palette used with RLE encoding.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawMainSurfaceAnimationSprite(S32 x, S32 y, const AnimationPixel* palette, const ImagePaletteSprite* const sprite);

// 0x10006b21
/**
 * Draws an animation sprite on main surface comparing a specified level with stencil. Draws cursor in motion or pressed, explodes, etc.
 * 
 * Created by AM. Refactored by IVA 12.07.2025.
 * 
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param level Displaying level.
 * @param palette Pixel palette used with RLE encoding.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawMainSurfaceAnimationSpriteStencil(S32 x, S32 y, U16 level, const AnimationPixel* palette, ImagePaletteSprite* sprite);

// 0x10006ef8
/**
 * Draws a sprite on main surface to foreground before static objects via specified stencil level . Draws any units, earth explosion, splash of water.
 * All such objects are considered background within a certain radius from the cursor.
 * 
 * Created by IVA 16.07.2025.
 *
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param level Displaying level.
 * @param palette Pixel palette used with RLE encoding.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawMainSurfacePaletteSpriteFrontStencil(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);

// 0x10007292
/**
 * Draws a sprite on main surface to background behind static objects via specified stencil level. Draws any units, earth explosion, splash of water.
 * All such objects are considered background within a certain radius from the cursor.
 * 
 * Created by IVA 16.07.2025.
 *
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param level Displaying level.
 * @param palette Pixel palette used with RLE encoding.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawMainSurfacePaletteSpriteBackStencil(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);


// 0x10007662
/**
 * Draws sprite as shadow on main surface. Draws shadows for vehicle.
 *
 * Created by IVA 15.07.2025.
 *
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param shadePixel Double pixel for stencil.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawMainSurfaceShadowSprite(S32 x, S32 y, const DoublePixel shadePixel, const ImagePaletteSprite* const sprite);

// 0x10007928
/**
 * Draws sprite as shadow on back surface. Draws shadows for all static objects like trees, buildings, poles, bushes, hedgehogs, etc.
 *
 * Created by IVA 15.07.2025.
 *
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param shadePixel Double pixel for stencil.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawBackSurfaceShadowSprite(S32 x, S32 y, const DoublePixel shadePixel, const ImagePaletteSprite* const sprite);

// 0x10007be8
/**
 * Draws a sprite on main surface comparing a specified level with stencil. Uses adjusted color mask.
 * This function is called, but its purpose is unknown.
 *
 * Created by IVA 16.07.2025.
 *
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param level Stencil level.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawMainSurfaceAdjustedSprite(S32 x, S32 y, U16 level, const ImagePaletteSprite* const sprite);

// 0x10007fbc
/**
 * Draws a sprite on main surface comparing a specified level with stencil. Uses actual color mask.
 * This function is called, but its purpose is unknown.
 *
 * Created by IVA 16.07.2025.
 *
 * @param x Initial X coordinate of the sprite.
 * @param y Initial Y coordinate of the sprite.
 * @param level Stencil level.
 * @param palette Pixel palette used with RLE encoding.
 * @param sprite Sprite to be drawn.
 *
 * @return None.
 */
void drawMainSurfaceActualSprite(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);


// 0x10008ecd
/**
 * Draws a sprite into a given sprite. Draws all UI near minimap, in-game menu, static cursor, crew and passengers icons.
 * 
 * Created by NR 22.07.2025. Refactored by IVA 23.07.2025.
 * 
 * @param x Initial X position of the sprite in given array.
 * @param y Initial Y position of the sprite in given array.
 * @param sprite Sprite to be drawn.
 * @param palette Pixel palette used with RLE encoding.
 * @param uiSprite Destination sprite.
 *
 * @return None.
 */
void drawUiSprite(S32 x, S32 y, const ImagePaletteSprite* const sprite, const void* palette, const ImageSpriteUI* const uiSprite);

// 0x10009eb3
/**
 * Marks a given array with button type based on given button sprite. It marks array for both drawn and missing pixels.
 * The passed array is used by the cursor to determine whether the button is hovered up and pressed.
 *
 * Created by IVA 17.07.2025.
 *
 * @param x Initial X position of the button sprite in given array.
 * @param y Initial Y position of the button sprite in given array.
 * @param sprite Button sprite used for its drawing.
 * @param type Button type used for recognition by cursor.
 * @param uiSprite Total sprite of all UI area. Includes the minimap and buttons area. Its size is 352x137.
 * @param offset The given array of button types.
 *
 * @return None.
 */
void markUiWithButtonType(S32 x, S32 y, const ImagePaletteSprite* const sprite, const ButtonType type, const ImageSpriteUI* const uiSprite, const ButtonType* const offset);

// 0x1000a4f3
/**
 * Draws a sprite into a given sprite using the specified vanishing level.
 *
 * Created by IVA 18.07.2025.
 *
 * @param x Initial X position of the button sprite in given array.
 * @param y Initial Y position of the button sprite in given array.
 * @param vanishLevel Value in [0, 31] responsible for the disappearance of the sprite.
 * @param palette Pixel palette used with RLE encoding.
 * @param sprite Sprite to be drawn.
 * @param uiSprite Destination sprite. This is a total sprite for all UI area. Usually it includes the minimap and buttons area. Its size should be 352x137.
 *
 * @return None.
 */
void drawVanishingUiSprite(S32 x, S32 y, const S32 vanishLevel, const Pixel* palette, ImagePaletteSprite* const sprite, const ImageSpriteUI* const uiSprite);



