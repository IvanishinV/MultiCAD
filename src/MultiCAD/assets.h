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
        AssetCollection* collection;
        AssetContent* content;
    };
    S32                                isCollection;
    char* const                              name;
    U8                                  isImage;
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ImageSpritePixel
{
    U8      count;
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
    U8                  unk04; // TODO
    U16                 next;
    ImageSpritePixel    pixels[1];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct ImagePaletteSpritePixel
{
    U8      count;          // If 0x80 bit is set - amount of the same pixel. If not - count of unique pixels.
    U8      pixels[1];      // One index of unique pixel if 0x80 was set. Count pixels if not.
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
    U8                      unk04; // TODO
    U16                     next;
    ImagePaletteSpritePixel pixels[1];
};
#pragma pack(pop)

#pragma pack(push, 1)
struct AnimationSpriteHeader
{
    U32 magic;
    U32 count;
    U32 offsets[1];
};
#pragma pack(pop)