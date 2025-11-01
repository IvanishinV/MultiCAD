#pragma once

#include "types.h"
#include <string>

constexpr U32 DIRECTDRAW_VERSION = 0x0700;
#include <ddraw.h>

const U8 IMAGE_SPRITE_ITEM_SMALL_PIXEL_MASK     = 0x1F;
const U8 IMAGE_SPRITE_ITEM_SHORT_COUNT_MASK     = 0x3F;
const U8 IMAGE_SPRITE_ITEM_SHORT_COMPACT_MASK   = 0x40;
const U8 IMAGE_SPRITE_ITEM_COUNT_MASK           = 0x7F;
const U8 IMAGE_SPRITE_ITEM_COMPACT_MASK         = 0x80;
const U8 IMAGE_SPRITE_ITEM_EXTENDED_MASK        = 0xC0;

#define SHADEPIXEL(pixel, mask) (((pixel) & (mask)) >> 1)

void ShowErrorNow(const std::string_view& message, bool isCritical = false);

void ShowErrorAsync(const std::string_view& message, bool isCritical = false);

