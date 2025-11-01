#pragma once

#include "types.h"

constexpr size_t GRAPHICS_BITS_PER_PIXEL_8 = 8;
constexpr size_t GRAPHICS_BITS_PER_PIXEL_16 = 16;
constexpr size_t GRAPHICS_BITS_PER_PIXEL_24 = 24;
constexpr size_t GRAPHICS_BITS_PER_PIXEL_32 = 32;

constexpr size_t GRAPHICS_SIZE_PALETTE = 256;

constexpr U32 MAX_POSSIBLE_SCREEN_WIDTH = 1920;
constexpr U32 MAX_POSSIBLE_SCREEN_HEIGHT = 1080;

constexpr U32 SCREEN_WIDTH = 1920;
constexpr U32 SCREEN_HEIGHT = 1080;
constexpr U32 SCREEN_SIZE_IN_PIXELS = SCREEN_WIDTH * SCREEN_HEIGHT;         // How many pixels are in the screen
constexpr U32 SCREEN_SIZE_IN_BYTES = SCREEN_SIZE_IN_PIXELS * sizeof(Pixel); // How many bytes are in the screen
constexpr U32 SCREEN_SIZE_IN_DOUBLE_PIXELS = SCREEN_SIZE_IN_PIXELS / 2;     // How many double pixels are in the screen

inline void UpdateScreenSize(const U32 width, const U32 height)
{
    // todo: make dynamic screen resolution

    // SCREEN_WIDTH  = width;
    // SCREEN_HEIGHT = height;

    // SCREEN_SIZE_IN_PIXELS        = width * height;
    // SCREEN_SIZE_IN_BYTES         = SCREEN_SIZE_IN_PIXELS * sizeof(Pixel);
    // SCREEN_SIZE_IN_DOUBLE_PIXELS = SCREEN_SIZE_IN_PIXELS * sizeof(Pixel) * 2
}