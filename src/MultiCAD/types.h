#pragma once

using U8                = unsigned char;
using U16               = unsigned short;
using U32               = unsigned int;

using S8                = char;
using S16               = short;
using S32               = int;

using F32               = float;

using Addr              = uintptr_t;

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

enum class GameVersion
{
    // Official versions
    SS_RU,              // Sudden Strike (1.0 ru, "Противостояние 3")
    SS_EN,              // Sudden Strike (1.0 en)
    SS_FOREVER,         // Противостояние 3: Война продолжается (2001)
    SS_APRM,            // Противостояние 3: Второе дыхание (2001-2003)
    SS_GOLD_RU,         // Sudden Strike Gold (1.21 ru)
    SS_GOLD_EN,         // Sudden Strike Gold (1.21 en)
    SS_GOLD_DE,         // Sudden Strike Gold (1.21 de)
    SS_GOLD_FR,         // Sudden Strike Gold (1.21 fr)
    SS_2,               // Sudden Strike 2 (de, en, fr, ru) (2002, October)
    SS_RW,              // Sudden Strike: Resource War (2004, October)

    // Official versions from Red Ice
    SS_ASIA_IN_FIRE,    // Противостояние: Азия в огне (2004, "Cold War Conflicts")
    SS_GULF_WAR,        // Противостояние: Война в заливе (2004, "Gulf War")
    SS_BLACK_GOLD,      // Противостояние: Битва за чёрное золото (2005)
    SS_EUROPE_2015,     // Противостояние: Европа 2015 (2008)
    SS_PIECE_2008,      // Противостояние: Принуждение к миру (2008)

    // Known mods
    SS_GOLD_HD_1_2_RU,  // Sudden Strike Gold HD v1.2 (ru)
    SS_GOLD_HD_1_2_INT,  // Sudden Strike Gold HD v1.2 (en, de, fr)
    HS_APRM,            // Hidden Stroke APRM: Allied Power Realism Mod (2003)
    HS_2,               // Hidden Stroke 2 (2005, March)
    HS_2_RW,            // Hidden Stroke 2: Resource War

    RWG_1,              // Real War Game (FMRM 3.0)
    RWG_2,              // ? Real War Game 2.99 (2011)
    RWG_3_5,            // Real War Game 3 by Inductor (2013-2014)
    RWG_ToW_0_9_b,      // Real War Game Truth of War by Zarathustra (2013-2014)

    SS_MW_1,            // Sudden Strike Modern Warfare 1
    SS_MW_2,            // Sudden Strike 2: Modern Warfare 2 (2013)
    SS_MW_3,            // Sudden Strike 2: Modern Warfare 3

    APRM,               // APRM 4.04 (Allied Power Realism Mod)

    // Unknown or not supported
    UNKNOWN
};
