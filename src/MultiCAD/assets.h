#pragma once

#include "types.h"

struct AssetContent
{
    U32 offset[1];
};

struct AssetCollection
{
    U32 offset;
    void* items[1];
};

#pragma pack(push, 1)
struct BinAsset
{
    union
    {
        AssetCollection*    collection;
        AssetContent*       content;
    };
    S32             isCollection;
    char* const     name;
    U8              isImage;
};
#pragma pack(pop)

/**
* The following structures are almost identical, but the difference is in the type used for the pixel.
* In regular structures, pixels are immediately given as values.
* In Palette structures, pixels are specified via the palette index (its size is 256).
*/

#pragma pack(push, 1)
struct ImageSpritePixel
{
    /*
    * Mask 0xC0 - should skip (count & 0x3F) pixels.
    * Mask 0x80 - should draw one pixel (count & 0x3F) times.
    * Mask 0x40 - should blend (count & 0x3F) pixels.
    * Mask 0x00 (no mask) - draw (count & 0x3F) pixels.
    */
    U8      count;
    /*
    * Just a pointer to first pixel. Its size depends on count.
    */
    Pixel   pixels[1];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ImageSprite
{
    S16                 x;
    S16                 y;
    S16                 width;
    S16                 height;
    U8                  type;
    U16                 next;
    ImageSpritePixel    pixels[1];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ImageSpriteUI
{
    Addr                offset;
    U32                 stride;
    S32                 x;
    S32                 y;
    S32                 width;
    S32                 height;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ImagePaletteSpritePixel
{
    /*
    * Mask 0xC0 - should skip (count & 0x3F) pixels.
    * Mask 0x80 - should draw one pixel (count & 0x3F) times.
    * Mask 0x40 - should blend (count & 0x3F) pixels.
    * Mask 0x00 (no mask) - draw (count & 0x3F) pixels.
    */
    U8      count;
    /*
    * Just a pointer to first pixel. Its size depends on count.
    */
    U8      pixels[1];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ImagePaletteTile
{
    U8 pixels[1];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ImagePaletteSprite
{
    S16                     x;
    S16                     y;
    S16                     width;
    S16                     height;
    U8                      type;
    U16                     next;
    ImagePaletteSpritePixel pixels[1];
};
#pragma pack(pop)
