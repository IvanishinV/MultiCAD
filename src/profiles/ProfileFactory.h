#pragma once

#include "Profiles.h"

#include <memory>

class ProfileFactory final
{
public:
    std::unique_ptr<IGameVersionProfile> create(GameVersion v)
    {
        switch (v)
        {
        case GameVersion::SS_RU: return std::make_unique<Profile_SS_RU>();
        case GameVersion::SS_GOLD_EN: return std::make_unique<Profile_SS_GOLD_EN>();
        case GameVersion::SS_GOLD_DE: return std::make_unique<Profile_SS_GOLD_DE>();
        case GameVersion::SS_GOLD_FR: return std::make_unique<Profile_SS_GOLD_FR>();
        case GameVersion::SS_GOLD_RU: return std::make_unique<Profile_SS_GOLD_RU>();
        case GameVersion::SS_GOLD_HD_1_2_RU: return std::make_unique<Profile_SS_GOLD_HD_1_2_RU>();
        case GameVersion::SS_GOLD_HD_1_2_INT: return std::make_unique<Profile_SS_GOLD_HD_1_2_INT>();
        default: return nullptr;
        }
    }
};