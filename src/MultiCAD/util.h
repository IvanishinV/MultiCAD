#pragma once

#include "types.h"
#include <string>

#define DIRECTDRAW_VERSION 0x0700
#include <ddraw.h>

const U8 kImageSpriteItemSmallPixelMask     = 0x1F;
const U8 kImageSpriteItemShortCountMask     = 0x3F;
const U8 kImageSpriteItemShortCompactMask   = 0x40;
const U8 kImageSpriteItemCountMask          = 0x7F;
const U8 kImageSpriteItemCompactMask        = 0x80;
const U8 kImageSpriteItemExtendedMask       = 0xC0;

#define SHADEPIXEL(pixel, mask) (((pixel) & (mask)) >> 1)

void ShowErrorNow(const std::string_view& message, bool isCritical = false);

void ShowErrorAsync(const std::string_view& message, bool isCritical = false);

