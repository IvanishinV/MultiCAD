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

    // True when the active game resolution came from an explicit sudtest.ini entry
    // rather than the auto-detected native one. The renderer uses this to decide
    // whether to prompt the user or silently snap to the nearest supported mode.
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

    // Reads the current desktop resolution, clamped to the supported range and
    // with height snapped down to a multiple of 8 (renderer requirement, see
    // ResolutionVerifier). Falls back to the default if the query fails.
    static void GetNativeResolution(S32& width, S32& height)
    {
        width = Graphics::kDefaultWidth;
        height = Graphics::kDefaultHeight;

        // Registry, not current: the menu's 640x480 exclusive-fullscreen mode would
        // otherwise be reported as "current". The desktop mode persists in registry.
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

        // Renderer cannot handle a height that isn't divisible by 8.
        height &= ~7;
        if (height < static_cast<S32>(Graphics::kMinHeight))
            height = Graphics::kMinHeight; // 480 is divisible by 8
    }

    // Computes the resolution the game should run at: the native desktop one by
    // default, overridden by an explicit sudtest.ini entry when present and valid.
    // The result is cached, so the desktop and ini are only read once.
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

    // Applies the resolved target resolution to the screen geometry. Must be
    // called when the game dll loads (the menu always runs at its own size).
    static void ApplyGameResolution()
    {
        S32 width, height;
        ResolveTargetResolution(width, height);
        UpdateSize(width, height);
    }

private:
    // Reads "[Game] Resolution = WIDTHxHEIGHT" from sudtest.ini into the supplied
    // out-params. Returns true only when a valid in-range value was found; an
    // out-of-range value is reported and ignored (out-params left untouched).
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

    static std::string GetIniPath()
    {
        char dllPath[MAX_PATH] = { 0 };
        HMODULE hModule = nullptr;
        if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            "cadMulti.dll",
            &hModule))
            return {};

        if (GetModuleFileNameA(hModule, dllPath, MAX_PATH) == 0)
            return {};

        char* lastSlash = strrchr(dllPath, '\\');
        if (lastSlash)
            *lastSlash = '\0';

        std::string iniPath = std::string(dllPath) + "\\sudtest.ini";
        return iniPath;
    }

    static bool targetResolved_;
    static S32 targetWidth_;
    static S32 targetHeight_;
};
