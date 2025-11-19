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
        case GameVersion::SS_GOLD_EN: return std::make_unique<Profile_SS_GOLD_EN>();
        default: return nullptr;
        }
    }
};