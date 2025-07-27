#pragma once

#include "types.h"
#include <string>

constexpr U32 DIRECTDRAW_VERSION = 0x0700;
#include <ddraw.h>

constexpr size_t  GRAPHICS_BITS_PER_PIXEL_8		= 8;
constexpr size_t  GRAPHICS_BITS_PER_PIXEL_16	= 16;
constexpr size_t  GRAPHICS_BITS_PER_PIXEL_24	= 24;
constexpr size_t  GRAPHICS_BITS_PER_PIXEL_32	= 32;

constexpr size_t  GRAPHICS_SIZE_PALETTE         = 256;

constexpr U32  GRAPHICS_RESOLUTION_480			= 480;
constexpr U32  GRAPHICS_RESOLUTION_600			= 600;
constexpr U32  GRAPHICS_RESOLUTION_640			= 640;
constexpr U32  GRAPHICS_RESOLUTION_768			= 768;
constexpr U32  GRAPHICS_RESOLUTION_800			= 800;
constexpr U32  GRAPHICS_RESOLUTION_1024			= 1024;

constexpr U32  SCREEN_WIDTH				    = GRAPHICS_RESOLUTION_1024;
constexpr U32  SCREEN_HEIGHT			    = GRAPHICS_RESOLUTION_768;
constexpr U32  SCREEN_SIZE_IN_PIXELS        = SCREEN_WIDTH * SCREEN_HEIGHT;             // How many pixels are in the screen
constexpr U32  SCREEN_SIZE_IN_BYTES         = SCREEN_SIZE_IN_PIXELS * sizeof(Pixel);    // How many bytes are in the screen
constexpr U32  SCREEN_SIZE_IN_DOUBLE_PIXELS = SCREEN_SIZE_IN_PIXELS / 2;                // How many double pixels are in the screen

const U8 IMAGE_SPRITE_ITEM_SMALL_PIXEL_MASK     = 0x1F;
const U8 IMAGE_SPRITE_ITEM_SHORT_COUNT_MASK     = 0x3F;
const U8 IMAGE_SPRITE_ITEM_SHORT_COMPACT_MASK   = 0x40;
const U8 IMAGE_SPRITE_ITEM_COUNT_MASK           = 0x7F;
const U8 IMAGE_SPRITE_ITEM_COMPACT_MASK         = 0x80;
const U8 IMAGE_SPRITE_ITEM_EXTENDED_MASK        = 0xC0;

#define SHADEPIXEL(pixel, mask) (((pixel) & (mask)) >> 1)

void ShowErrorMessage(const std::string_view& message, bool isCritical = false);
