#pragma once

using U8                = unsigned char;
using U16               = unsigned short;
using U32               = unsigned int;

using S8                = char;
using S16               = short;
using S32               = int;

using F32               = float;

using Addr              = std::uintptr_t;

using ButtonType        = U8;
using Pixel             = U16;
using DoublePixel       = U32;
using AnimationPixel    = U32;


enum class PixelColor : U16
{
    BLACK = 0x0000,
    BLUE = 0x001F,
    GREEN = 0x07E0,
    CYAN = 0x07FF,
    RED = 0xF800,
    MAGENTA = 0xF81F,
    YELLOW = 0xFFE0,
    WHITE = 0xFFFF,
    DEFAULT_MASK = WHITE
};

inline U32 operator&(const U32 lhs, const PixelColor rhs)
{
    return lhs & static_cast<U32>(rhs);
}

inline U32 operator&(const PixelColor lhs, const U32 rhs)
{
    return static_cast<U32>(lhs) & rhs;
}

inline bool operator==(const U32 lhs, const PixelColor rhs)
{
    return lhs == static_cast<U32>(rhs);
}

inline bool operator==(const PixelColor lhs, const U32 rhs)
{
    return static_cast<U32>(lhs) == rhs;
}
