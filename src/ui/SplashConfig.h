#pragma once
#include <cstdint>

struct SplashStaticParams
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    int x;
    int y;
    bool aligned;
};

enum class SplashVariant : uint8_t
{
    SS,
    SS2,
};

constexpr SplashStaticParams g_splashTable[] =
{
    { 0xDE, 0xD7, 0x42, 476, 300, false },  // SS
    { 0xAA, 0xAA, 0x55, 780, 470, true },   // SS2
};
