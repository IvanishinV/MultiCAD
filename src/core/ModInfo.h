#pragma once

#include <string>

#include "ScreenConfig.h"

// A mod declares its own name and version here:
//
//   [Launcher]
//   ModName=FMRM
//   ModVer=2.1.5.4
//
// Used for the splash line and the reported mod, so both follow the mod's
// releases rather than whatever this dll was built against.
namespace ModInfo
{
    // False unless both keys are set; the caller then falls back to its own
    // default.
    inline bool FromLauncher(std::string& name, std::string& version)
    {
        const std::string iniPath = Screen::IniPath();
        if (iniPath.empty())
            return false;

        const auto read = [&iniPath](const char* key, std::string& out)
            {
                char buffer[64] = { 0 };
                const DWORD length = GetPrivateProfileStringA(
                    "Launcher", key, "", buffer, sizeof(buffer), iniPath.c_str());

                out.assign(buffer, length);

                return !out.empty();
            };

        return read("ModName", name) && read("ModVer", version);
    }
}
