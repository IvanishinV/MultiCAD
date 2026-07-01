#pragma once

#include "types.h"
#include "util.h"

namespace Graphics
{
    constexpr size_t kBitsPerPixel8 = 8;
    constexpr size_t kBitsPerPixel16 = 16;
    constexpr size_t kBitsPerPixel24 = 24;
    constexpr size_t kBitsPerPixel32 = 32;

    constexpr size_t kPaletteSize = 256;

    constexpr U32 kMinWidth = 640;
    constexpr U32 kMinHeight = 480;

    constexpr U32 kDefaultWidth = 1920;
    constexpr U32 kDefaultHeight = 1080;

    constexpr U32 kMaxWidth = 3840;
    constexpr U32 kMaxHeight = 2160;
}

class Screen
{
public:
    static S32 width_;              // Width in pixels
    static S32 height_;             // Height in pixels
    static S32 widthInBytes_;       // Width in bytes
    static S32 heightInBytes_;      // Height in bytes
    static S32 sizeInPixels_;       // Number of pixels are in the screen
    static S32 sizeInBytes_;        // Number of bytes are in the screen
    static S32 sizeInDoublePixels_; // Number of double pixels are in the screen

    static bool resolutionFromIni_;

    static void UpdateSize(S32 width, S32 height)
    {
        width_ = width;
        height_ = height;

        widthInBytes_ = width * 2;
        heightInBytes_ = height * 2;

        sizeInPixels_ = width * height;
        sizeInBytes_ = sizeInPixels_ * sizeof(Pixel);
        sizeInDoublePixels_ = sizeInPixels_ * sizeof(Pixel) * 2;
    }

    static void UpdateToOrigSize()
    {
        constexpr S32 width = 1024;
        constexpr S32 height = 768;

        width_ = width;
        height_ = height;

        widthInBytes_ = width * 2;
        heightInBytes_ = height * 2;

        sizeInPixels_ = width * height;
        sizeInBytes_ = sizeInPixels_ * sizeof(Pixel);
        sizeInDoublePixels_ = sizeInPixels_ * sizeof(Pixel) * 2;
    }

    static void GetNativeResolution(S32& width, S32& height)
    {
        width = Graphics::kDefaultWidth;
        height = Graphics::kDefaultHeight;

        // Registry, not current: the menu sets a temporary 640x480 fullscreen mode;
        // only the registry mode keeps the real desktop resolution.
        DEVMODEA dm{};
        dm.dmSize = sizeof(dm);
        if (EnumDisplaySettingsA(nullptr, ENUM_REGISTRY_SETTINGS, &dm))
        {
            width = static_cast<S32>(dm.dmPelsWidth);
            height = static_cast<S32>(dm.dmPelsHeight);
        }

        if (width < static_cast<S32>(Graphics::kMinWidth))   width = Graphics::kMinWidth;
        if (width > static_cast<S32>(Graphics::kMaxWidth))   width = Graphics::kMaxWidth;
        if (height < static_cast<S32>(Graphics::kMinHeight)) height = Graphics::kMinHeight;
        if (height > static_cast<S32>(Graphics::kMaxHeight)) height = Graphics::kMaxHeight;

        // Renderer requires a height divisible by 8.
        height &= ~7;
        if (height < static_cast<S32>(Graphics::kMinHeight))
            height = Graphics::kMinHeight;
    }

    // Native desktop resolution, overridden by an explicit ini value. Cached.
    static void ResolveTargetResolution(S32& width, S32& height)
    {
        if (!targetResolved_)
        {
            GetNativeResolution(targetWidth_, targetHeight_);
            resolutionFromIni_ = ApplyIniResolution(targetWidth_, targetHeight_);
            targetResolved_ = true;
        }

        width = targetWidth_;
        height = targetHeight_;
    }

    // Call when the game dll loads; the menu runs at its own fixed size.
    static void ApplyGameResolution()
    {
        S32 width, height;
        ResolveTargetResolution(width, height);
        UpdateSize(width, height);
    }

    static void SaveResolutionToIni(S32 width, S32 height)
    {
        const std::string iniPath = GetIniPath();
        if (iniPath.empty())
            return;

        const std::string value = std::to_string(width) + "x" + std::to_string(height);
        WritePrivateProfileStringA("Game", "Resolution", value.c_str(), iniPath.c_str());
    }

private:

    static bool targetResolved_;
    static S32 targetWidth_;
    static S32 targetHeight_;

    // Reads "[Game] Resolution=WIDTHxHEIGHT" from the ini. True if a valid value was found.
    static bool ApplyIniResolution(S32& outWidth, S32& outHeight)
    {
        std::string iniPath = GetIniPath();
        if (iniPath.empty())
            return false;

        char buffer[64];
        if (GetPrivateProfileStringA("Game", "Resolution", "", buffer, sizeof(buffer), iniPath.c_str()) == 0)
            return false;

        char* xPos = std::strchr(buffer, 'x');
        if (xPos == nullptr)
            return false;

        S32 width = std::atol(buffer);
        S32 height = std::atol(xPos + 1);

        if (width < static_cast<S32>(Graphics::kMinWidth) || height < static_cast<S32>(Graphics::kMinHeight))
        {
            ShowErrorAsync("The resolution specified in sudtest.ini is too small and will be ignored. Minimum supported is 640x480.");
            return false;
        }

        if (width > static_cast<S32>(Graphics::kMaxWidth) || height > static_cast<S32>(Graphics::kMaxHeight))
        {
            ShowErrorAsync("The resolution specified in sudtest.ini is too large and will be ignored. Maximum supported is 3840x2160.");
            return false;
        }

        outWidth = width;
        outHeight = height;
        return true;
    }

    static std::string ToLowerAscii(std::string s)
    {
        for (char& c : s)
            if (c >= 'A' && c <= 'Z')
                c = static_cast<char>(c + ('a' - 'A'));
        return s;
    }

    // True when [Game] has an SSDraw* entry referencing dllName, i.e. this ini loaded us.
    static bool IniLoadsDll(const std::string& iniPath, const std::string& dllName)
    {
        char section[8192] = { 0 };
        if (GetPrivateProfileSectionA("Game", section, sizeof(section), iniPath.c_str()) == 0)
            return false;

        const std::string needle = ToLowerAscii(dllName);

        // section is a run of "key=value\0" entries ending with an extra '\0'.
        for (const char* entry = section; *entry; entry += std::strlen(entry) + 1)
        {
            if (_strnicmp(entry, "SSDraw", 6) != 0)
                continue;

            if (ToLowerAscii(entry).find(needle) != std::string::npos)
                return true;
        }

        return false;
    }

    // Directory of the given module (nullptr = the game exe), without trailing slash.
    static std::string ModuleDir(HMODULE mod)
    {
        char path[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(mod, path, MAX_PATH) == 0)
            return {};

        char* lastSlash = strrchr(path, '\\');
        if (!lastSlash)
            return {};

        *lastSlash = '\0';
        return path;
    }

    static std::string WorkingDir()
    {
        char path[MAX_PATH] = { 0 };
        DWORD n = GetCurrentDirectoryA(MAX_PATH, path);
        if (n == 0 || n >= MAX_PATH)
            return {};

        return path; // no trailing slash
    }

    static std::string GetIniPath()
    {
        // Resolve our own module from an in-module address (name-agnostic).
        HMODULE self = nullptr;
        if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&WorkingDir),
            &self))
            return {};

        char selfPath[MAX_PATH] = { 0 };
        if (GetModuleFileNameA(self, selfPath, MAX_PATH) == 0)
            return {};

        const char* slash = strrchr(selfPath, '\\');
        const std::string dllName = slash ? std::string(slash + 1) : std::string(selfPath);

        // The ini lives next to the exe, which may not be the dll's folder.
        const std::string searchDirs[] = { ModuleDir(nullptr), ModuleDir(self), WorkingDir() };

        // Ini name differs per game version; pick the one that references us.
        static constexpr const char* kCandidateInis[] = {
            "sudtest.ini",   // base Sudden Strike
            "gulfwar.ini",   // addons
            "blackgold.ini",
            "blacksea.ini",
            "euro2015.ini",
        };

        std::string firstExisting;

        for (const std::string& dir : searchDirs)
        {
            if (dir.empty())
                continue;

            for (const char* name : kCandidateInis)
            {
                std::string path = dir + "\\" + name;

                if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES)
                    continue;

                if (firstExisting.empty())
                    firstExisting = path;

                if (!dllName.empty() && IniLoadsDll(path, dllName))
                    return path;
            }
        }

        // No SSDraw match: fall back to the first existing ini.
        return firstExisting;
    }
};
