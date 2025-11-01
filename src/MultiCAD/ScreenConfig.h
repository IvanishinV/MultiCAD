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

    constexpr U32 kMaxWidth = 2560;
    constexpr U32 kMaxHeight = 1440;
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

    static void UpdateResolutionFromIni()
    {
        std::string iniPath = GetIniPath();
        if (iniPath.empty())
            return;

        char buffer[64];
        if (GetPrivateProfileStringA("Game", "Resolution", "", buffer, sizeof(buffer), iniPath.c_str()) == 0)
            return;

        U32 width, height;
        char* xPos = std::strchr(buffer, 'x');

        if (xPos == nullptr)
            return;

        width = std::atol(buffer);
        height = std::atol(xPos + 1);

        if (width < Graphics::kMinWidth || height < Graphics::kMinHeight)
        {
            ShowErrorAsync("The resolution you specified is too small to be supported, so the default one is specified (1920x1080). Minimum supported is 640x480.");
            return;
        }

        if (width > Graphics::kMaxWidth || height > Graphics::kMaxHeight)
        {
            ShowErrorAsync("The resolution you specified is too large to be supported, so the default one is specified (1920x1080). Maximum supported is 2560x1440.");
            return;
        }

        UpdateSize(width, height);
    }

private:
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
};
