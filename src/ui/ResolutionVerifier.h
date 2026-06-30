#pragma once

#include "util.h"
#include <vector>

class ResolutionVerifier
{
public:
    static ResolutionVerifier& GetInstance();

    void Initialize(LPDIRECTDRAW ddraw, int bits);

    ResolutionVerifier(const ResolutionVerifier&) = delete;
    ResolutionVerifier& operator=(const ResolutionVerifier&) = delete;
    ~ResolutionVerifier() = default;

    bool IsSupported(int width, int height, int bits) const;
    bool ChooseResolution(int& width, int& height) const;

    // Snaps width/height to the closest supported mode (by squared distance) for
    // the given bit depth. Returns false when no supported mode matches the depth.
    bool FindNearest(int& width, int& height, int bits) const;

    struct Resolution
    {
        int width;
        int height;
        int bits;

        auto operator<=>(const Resolution&) const = default;
    };

private:
    ResolutionVerifier();

    static HRESULT WINAPI EnumModesCallback(LPDDSURFACEDESC lpDDSurfaceDesc, VOID* lpContext);
    void InitSupportedResolutions(int bitsFilter);

    LPDIRECTDRAW ddraw_{ nullptr };
    std::vector<Resolution> supportedResolutions_;
    int bitsFilter_{ 0 };

    static ResolutionVerifier* instance_;
};
