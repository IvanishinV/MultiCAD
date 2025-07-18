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

struct Surfaces
{
    Pixel main[SCREEN_WIDTH * (SCREEN_HEIGHT + 1)];     // 0x1001d588 Основной буфер кадра (то, что видно на экране).
    Pixel back[SCREEN_WIDTH * (SCREEN_HEIGHT + 1)];     // 0x1019ddac Буфер заднего плана (для двойной буферизации).
    Pixel stencil[SCREEN_WIDTH * (SCREEN_HEIGHT + 1)];  // 0x1031e5d0 Буфер трафарета (для маскирования пикселей).
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

struct Window
{
    S32 x;                      // 0x10010070
    S32 y;                      // 0x10010072
    S32 width;                  // 0x10010074
    S32 height;                 // 0x10010076 0x10010050
};

struct Sprite
{
    Pixel* minX;                // 0x1001005c
    Pixel* maxX;                // 0x10010060
    U32    width;               // 0x10010064
    S32    vanishOffset;        // 0x10010068

    Window  windowRect;         // 0x10010070

    Pixel* x;                   // 0x1001007c

    U32     colorMask;          // 0x10010084
    U32     adjustedColorMask;  // 0x10010088

    S32     height;             // 0x1001009a
    S32     overage;            // 0x1001009e
};

struct Tile
{
    S32 unk01;                  // 0x10010030
    S32 displayedHalfs;         // 0x10010034
    Pixel* stencil;             // 0x10010038
    S8  unk04;                  // 0x1001003c

    S32 diff;                   // 0x1001003d
    S32 tileHeight;             // 0x10010041
    S32 tempTileHeight;         // 0x10010045
    S8  unk08;                  // 0x10010049

    Window windowRect;          // 0x1001004A
};

struct GameUI
{
    Addr offset;                        // 0x100100b6
    U32 stride;                         // 0x100100ba
    Rect windowRect;                    // 0x100100be
    U32 type;                           // 0x100100сe
    ImagePaletteSprite* imageSprite;    // 0x100100d2
};

struct RendererStructTest01
{
    S32 unk01;                  // 1049edd0
    S32 unk02;                  // 1049edd4
    S32 validRowsBlockCount;    // 1049edd8 Количество валидных блоков строк (по 8 строк)..
    U32 unk04;                  // 1049eddc
};

struct RendererStructTest02
{
    S32 unk01;                  // 1001d574
    S32 unk02;                  // 1001d578
    S32 excessRowsBlockCount;   // 1001d57c Количество лишних блоков строк (по 8 строк).
    S32 unk04;                  // 1001d580
    S32 unk05;                  // 1001d584
};

struct RendererStateContainer
{
    Surfaces        surfaces;       // 1001'd5d0
    Outline         outline;        // 1001'd55c
    Sprite          sprite;
    Tile            tile;
    GameUI          gameUI;
    RendererStructTest01 rendererStruct01;
    RendererStructTest02 rendererStruct02;
};

extern RendererStateContainer g_rendererState;



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
 * @param x Start X position.
 * @param y Start Y position.
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
 * @param x Start X position.
 * @param y Start Y position.
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
 * @param x Start X position.
 * @param y Start Y position.
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
 * @param x Start X position.
 * @param y Start Y position.
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
 * @param x Start X position.
 * @param y Start Y position.
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
 *
 * Most likely this function is used for displaying the mission minimap in the corner and in a large size.
 *
 * @param x X position of the point.
 * @param y Y position of the point.
 * @param pixel Preferred color of the point.
 *
 * @return None.
 */
void drawMainSurfaceColorPoint(const S32 x, const S32 y, const Pixel pixel);

// 0x10001840
/**
 * Draws a colored point on back surface.
 *
 * @param x X position of the point.
 * @param y Y position of the point.
 * @param pixel Preferred color of the point.
 *
 * @return None.
 */
void drawBackSurfaceColorPoint(const S32 x, const S32 y, const Pixel pixel);

// 0x100018a0
/**
 * Copies a rectangle from main surface to a provided surface. Used for coping area behind cursor in game.
 *
 * @param sx Source X position of the rectangle.
 * @param sy Source Y position of the rectangle.
 * @param width Width of the rectangle.
 * @param height Height of the rectangle.
 * @param dx Destination X position of the rectangle.
 * @param dy Source Y position of the rectangle.
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
 * @param dx Horizontal offset after camera move.
 * @param dy Vertical offset after camera move.
 *
 * @return None.
 */
void copyMainBackSurfaces(S32 dx, S32 dy);

// 0x10001e90
/**
 * Calls function to draw input rhomb on the back DX surface.
 *
 * @param tx
 * @param ty
 * @param angle_0
 * @param angle_1
 * @param angle_2
 * @param angle_3
 * @param input Input rhomb.
 *
 * @return None.
 */
void callDrawBackSurfaceRhomb(S32 tx, S32 ty, S32 angle_0, S32 angle_1, S32 angle_2, S32 angle_3, ImagePaletteTile* input);

// 0x10001ed0           todo
void FUN_10001ed0(S32 param_1, S32 param_2, S32 param_3, S32 param_4, S32 param_5, S32 param_6);

// 0x10001f10           todo
void FUN_10001f10(S32 param_1, S32 param_2, S32 param_3);

// 0x10001f40           todo
void FUN_10001f40(S32 param_1, S32 param_2, S32 param_3, S32 param_4, S32 param_5, S32 param_6, S32 param_7);

// 0x10001f80
/**
 * Copies rectangle with landscape and static objects from back to main surface.
 *
 * @param x Starting X position of the rectangle.
 * @param y Starting Y position of the rectangle.
 * @param width Width of the rectangle
 * @param height Height of the rectangle
 *
 * @return None.
 */
void copyBackToMainSurfaceRect(const S32 x, const S32 y, const U32 width, const U32 height);

// 0x10002020
/**
 * Draws colored ellipse on main surface.
 *
 * @param x Starting X position of the ellipse.
 * @param y Starting Y position of the ellipse.
 * @param size Size of the ellipse
 * @param pixel Color of the ellipse
 * @param step
 *
 * @return None.
 */
void drawMainSurfaceColorEllipse(const S32 x, const S32 y, S32 size, const Pixel pixel, const S32 step);

// 0x100023e0
/**
 * Draws colored unit selection rectangle.
 *
 * @param x Starting X coordinate.
 * @param y Starting Y coordinate.
 * @param width
 * @param height
 * @param Pixel Preferred color.
 *
 * @return None.
 */
void drawMainSurfaceColorOutline(S32 x, S32 y, S32 width, S32 height, Pixel pixel);

// 0x100026e0
/**
 * Unknown target.
 *
 * @return None.
 */
void drawStencilSurfaceWindowRect();

// 0x10002780
/**
 * Masks a rectangle on stencil surface.
 *
 * @param x Starting X coordinate of the rectangle.
 * @param y Starting Y coordinate of the rectangle.
 * @param width Width of the rectangle.
 * @param height Height of the rectangle.
 *
 * @return None.
 */
void maskStencilSurfaceRect(S32 x, S32 y, S32 width, S32 height);

// 0x10002810
/**
 * Moves stencil surface depending on g_moduleState.surface.offset and other values.
 * x
 * y
 * width
 * height
 * offset
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
 * @param sx Starting X coordinate of the rectangle in source array.
 * @param sy Starting Y coordinate of the rectangle in source array.
 * @param width Width of the rectangle to be copied.
 * @param height Height of the rectangle to be copied.
 * @param dx Starting X coordinate of the rectangle in renderer.
 * @param dy Starting Y coordinate of the rectangle in renderer.
 * @param stride Line pitch. Usually double screen width.
 * @param pixels Source pixel array from which the rectangle will be copied.
 *
 * @return None.
 */
bool copyToRendererSurfaceRect(S32 sx, S32 sy, S32 width, S32 height, S32 dx, S32 dy, S32 stride, Pixel* pixels);

// 0x10002a30
/**
 * Copies a rectangle from source to destination array.
 *
 * @param sx Starting X coordinate of the source array.
 * @param sy Starting Y coordinate of the source array.
 * @param sstr Row stride of the source array.
 * @param input Source array.
 * @param dx Starting X coordinate of the destination array.
 * @param dy Starting Y coordinate of the destination array.
 * @param dstr Row stride of the destination array.
 * @param output Destination array.
 * @param width Width to be copied in pixels.
 * @param height Height to be copied in pixels.
 *
 * @return None.
 */
void copyPixelRectFromTo(S32 sx, S32 sy, S32 sstr, Pixel* input, S32 dx, S32 dy, S32 dstr, Pixel* output, S32 width, S32 height);

// 0x10002a90
/**
 * Copies a rectangle from main surface to renderer.
 *
 * @param x Starting X coordinate of the rectangle.
 * @param y Starting Y coordinate of the rectangle.
 * @param width Width of the rectangle to be copied.
 * @param height Height of the rectangle to be copied.
 *
 * @return True on success, false on any fail.
 */
bool copyMainSurfaceToRenderer(S32 x, S32 y, S32 width, S32 height);

// 0x10002b90
/**
 * Copies a rectangle from main surface to renderer using war fog array.
 *
 * Created by IVA 20.06.2025.
 * 
 * @param x Starting X coordinate of the rectangle.
 * @param y Starting Y coordinate of the rectangle.
 * @param width Width of the rectangle to be copied.
 * @param height Height of the rectangle to be copied.
 *
 * @return None.
 */
void copyMainSurfaceToRendererWithWarFog(S32 x, S32 y, S32 width, S32 height);

// 0x10002fb0           todo
void FUN_10002fb0(S32 x, S32 y, S32 width, S32 height);

// 0x10003320
/**
 * Returns a text length in pixels, Default space between letters is 2 pixels.
 *
 * Created by NR 02.04.2025. Refactored by IVA.
 *
 * @param text Text which length has to be calculated.
 * @param asset Letter assets.
 *
 * @return Length of text in pixels.
 */
S32 getTextLength(const char* text, const AssetCollection* asset);

// 0x10003360           todo
void FUN_10003360(S32 x, S32 y, LPSTR text, AssetCollection* asset, Pixel* palette);

// 0x100033c0           todo
void FUN_100033c0(S32 x, S32 y, LPSTR text, AssetCollection* asset, Pixel* palette);


// 0x10003420
/**
 * Draws a landscape rhomb on back surface. Used for ground.
 * 
 * Created by NR 25.06.2025. Refactored by IVA 27.06.2025.
 *
 * @param angle_0 0th angle of the tile.
 * @param angle_1 1th angle of the tile.
 * @param angle_2 2th angle of the tile.
 * @param angle_3 3th angle of the tile.
 * @param tx X coordinate of the tile.
 * @param ty Y coordinate of the tile.
 * @param stride Line pitch in bytes, most likely double screen width.
 * @param input Input tile pixels.
 * @param output Output pixel array.
 *
 * @return None.
 */
void drawBackSurfaceRhomb(S32 angle_0, S32 angle_1, S32 angle_2, S32 angle_3, S32 tx, S32 ty, S32 stride, ImagePaletteTile* input, Pixel* output);

// 0x10004390           todo
void FUN_10004390(S32 param_1, S32 param_2, LPVOID param_3);

// 0x100046b6           todo
void FUN_100046b6(S32 param_1, S32 param_2, LPVOID param_3);

// 0x100049e6           todo
void FUN_100049e6(S32 param_1, S32 param_2, U16 param_3, LPVOID param_4);

// 0x10004db0           todo
void FUN_10004db0(S32 x, S32 y, U16 param_3, S32 param_4, LPVOID param_5);

// 0x100050df
/**
 * Draws a sprite on main surface, e.g. text.
 * 
 * @param x Starting X position of the sprite.
 * @param y Starting Y position of the sprite.
 * @param palette
 * @param sprite
 *
 * @return None.
 */
void drawMainSurfacePaletteSprite(S32 x, S32 y, const Pixel* palette, const ImagePaletteSprite* const sprite);

// 0x100053c3
/**
 * Draws a sprite with specified vanishing level on main surface.
 * 
 * @param x Starting X position of the sprite.
 * @param y Starting Y position of the sprite.
 * @param vanishOffset Value in [0, 31] responsible for the disappearance of the sprite.
 * @param palette
 * @param sprite
 *
 * @return None.
 */
void drawMainSurfaceVanishingSprite(S32 x, S32 y, const S32 vanishOffset, const Pixel* palette, const ImagePaletteSprite* const sprite);

// 0x1000579c
/**
 * Draws a sprite on back surface, e.g. roads, bridges, cliffs, craters, stumps.
 *
 * Created by IVA 14.07.2025.
 * 
 * @param x Starting X position of the sprite.
 * @param y Starting Y position of the sprite.
 * @param palette
 * @param sprite
 *
 * @return None.
 */
void drawBackSurfacePalletteSprite(S32 x, S32 y, const Pixel* const palette, const ImagePaletteSprite* const sprite);

// 0x10005ac6           todo
void FUN_10005ac6(S32 param_1, S32 param_2, U16 param_3, S32 param_4, LPVOID param_5);

// 0x10005e31
/**
 * Draws a shaded sprite on back surface comparing a specified level with stencil, e.g. trees, fences, some buildings, hedgehogs, etc.
 *
 * Created by IVA 13.07.2025.
 * 
 * @param x Starting X position of the sprite.
 * @param y Starting Y position of the sprite.
 * @param level Displaying level.
 * @param palette
 * @param sprite
 *
 * @return None.
 */
void drawBackSurfacePaletteShadedSprite(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);

// 0x1000618d           todo
void FUN_1000618d(S32 x, S32 y, S32 param_3, LPVOID param_4);

// 0x100064b6
/**
 * Draws a sprite on main surface, e.g. main menu.
 *
 * @param x Starting X position of the sprite.
 * @param y Starting Y position of the sprite.
 * @param palette
 * @param sprite
 *
 * @return None.
 */
void drawMainSurfaceSprite(S32 x, S32 y, ImageSprite* sprite);

// 0x100067ad           todo
void FUN_100067ad(S32 x, S32 y, S32 param_3, LPVOID param_4);

// 0x10006b21
/**
 * Draws an animation sprite via specified level on main surface, e.g. cursor, explodes, etc.
 * 
 * Created by AM. Refactored by IVA 12.07.2025.
 * 
 * @param x Starting X position of the sprite.
 * @param y Starting Y position of the sprite.
 * @param level Displaying level.
 * @param palette
 * @param sprite
 *
 * @return None.
 */
void drawMainSurfaceAnimationSprite(S32 x, S32 y, U16 level, const AnimationPixel* palette, ImagePaletteSprite* sprite);


// 0x10006ef8
/**
 * Draws a sprite via specified level on main surface to foreground before static objects. Draws any units, earth explosion, splash of water.
 * All such objects are considered background within a certain radius from the cursor.
 * 
 * Created by IVA 16.07.2025.
 *
 * @param x Starting X position of the sprite.
 * @param y Starting Y position of the sprite.
 * @param level Displaying level.
 * @param palette
 * @param sprite
 *
 * @return None.
 */
void drawMainSurfaceSpriteFront(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);

// 0x10007292
/**
 * Draws a sprite via specified level on main surface to background behind static objects. Draws any units, earth explosion, splash of water.
 * All such objects are considered background within a certain radius from the cursor.
 * 
 * Created by IVA 16.07.2025.
 *
 * @param x Starting X position of the sprite.
 * @param y Starting Y position of the sprite.
 * @param level Displaying level.
 * @param palette
 * @param sprite
 *
 * @return None.
 */
void drawMainSurfaceSpriteBack(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);

// 0x10007662
/**
 * Draws sprite as shadow on main surface. Draws shadows for vehicle.
 *
 * Created by IVA 15.07.2025.
 *
 * @param x Starting X position of the sprite.
 * @param y Starting Y position of the sprite.
 * @param shadePixel Double pixel for stencil.
 * @param sprite
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
 * @param x Starting X position of the sprite.
 * @param y Starting Y position of the sprite.
 * @param shadePixel Double pixel for stencil.
 * @param sprite
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
 * @param x Starting X position of the sprite.
 * @param y Starting Y position of the sprite.
 * @param level Stencil level.
 * @param sprite
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
 * @param x Starting X position of the sprite.
 * @param y Starting Y position of the sprite.
 * @param level Stencil level.
 * @param palette
 * @param sprite
 *
 * @return None.
 */
void drawMainSurfaceActualSprite(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite);

// 0x10008ecd           todo
/**
 * Draws in-game UI, cursor is displayed always.
 *
 * @return None.
 */
void FUN_10008ecd(S32 param_1, S32 param_2, LPVOID param_3, S32 param_4, LPVOID param_5);

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
void markUiWithButtonType(S32 x, S32 y, ImagePaletteSprite* const sprite, const ButtonType type, const ImageSpriteUI* const uiSprite, const ButtonType* const offset);

// 0x1000a4f3
/**
 * Draws a sprite into a given array using the specified vanishing level.
 *
 * Created by IVA 18.07.2025.
 *
 * @param x Initial X position of the button sprite in given array.
 * @param y Initial Y position of the button sprite in given array.
 * @param vanishOffset Value in [0, 31] responsible for the disappearance of the sprite.
 * @param input The given array to be written.
 * @param sprite Sprite to be drawn.
 * @param uiSprite Total sprite of all UI area. Usually it includes the minimap and buttons area. Its size should be 352x137.
 *
 * @return None.
 */
void drawVanishingUiSprite(S32 x, S32 y, const S32 vanishLevel, const Pixel* palette, ImagePaletteSprite* const sprite, const ImageSpriteUI* const uiSprite);



