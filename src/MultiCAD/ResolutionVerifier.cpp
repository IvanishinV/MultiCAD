#include "pch.h"
#include "ResolutionVerifier.h"
#include "ResolutionDialog.h"


ResolutionVerifier& ResolutionVerifier::GetInstance()
{
    static ResolutionVerifier instance;
    return instance;
}

ResolutionVerifier::ResolutionVerifier()
    : ddraw_(nullptr), bitsFilter_(0)
{
    supportedResolutions_.clear();
}

void ResolutionVerifier::Initialize(LPDIRECTDRAW ddraw, int bits)
{
    if (ddraw_ == nullptr && ddraw != nullptr)
    {
        ddraw_ = ddraw;
        InitSupportedResolutions(bits);
    }
}


void ResolutionVerifier::InitSupportedResolutions(int bitsFilter)
{
    bitsFilter_ = bitsFilter;
    supportedResolutions_.clear();

    if (!ddraw_)
        return;

    ddraw_->EnumDisplayModes(0, nullptr, this, EnumModesCallback);
}

HRESULT WINAPI ResolutionVerifier::EnumModesCallback(LPDDSURFACEDESC lpDDSurfaceDesc, VOID* lpContext)
{
    ResolutionVerifier* verifier = reinterpret_cast<ResolutionVerifier*>(lpContext);

    const int bits = lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount;

    if (verifier->bitsFilter_ != 0 && bits != verifier->bitsFilter_)
    {
        return DDENUMRET_OK;
    }

    Resolution res{
        static_cast<int>(lpDDSurfaceDesc->dwWidth),
        static_cast<int>(lpDDSurfaceDesc->dwHeight),
        bits
    };

    for (const auto& existingRes : verifier->supportedResolutions_)
    {
        if (existingRes == res)
        {
            return DDENUMRET_OK;
        }
    }

    verifier->supportedResolutions_.push_back(res);

    return DDENUMRET_OK;
}

bool ResolutionVerifier::IsSupported(int& width, int& height, int bits, bool allowChoose) const
{
    for (const auto& res : supportedResolutions_)
    {
        if (res.width == width && res.height == height && res.bits == bits)
        {
            return true;
        }
    }

    if (!allowChoose)
        return false;

    HINSTANCE hInstance = GetModuleHandle(NULL);

    auto ctx = std::make_shared<ResolutionDialogContext>();
    ctx->availableModes = supportedResolutions_;

    if (ShowModeSelectionDialog(hInstance, NULL, ctx))
    {
        width = ctx->selectedMode.width;
        height = ctx->selectedMode.height;

        return true;
    }

    return false;
}
