#include "pch.h"
#include "cad.h"
#include "renderer.h"
#include "ResolutionVerifier.h"

#include "DllVersionDetector.h"
#include "PatchEngine.h"
#include "ProfileFactory.h"
#include "MemoryRelocator.h"
#include "CodePatcher.h"

#ifdef _DEBUG
#include <format>
#include <cassert>

#define CHECK_ASSERTS
#endif

RendererState g_rendererState;

// 0x10001000
void initValues()
{
    g_moduleState->surface.offset = 0;
    g_moduleState->surface.y = Screen::height_;
    g_moduleState->surface.width = Screen::width_;
    g_moduleState->surface.height = Screen::height_;
    g_moduleState->surface.stride = Screen::widthInBytes_;

    g_moduleState->windowRect.x = 0;
    g_moduleState->windowRect.y = 0;
    g_moduleState->windowRect.width = Screen::width_ - 1;
    g_moduleState->windowRect.height = Screen::height_ - 1;
}

// 0x10001050
bool initDxInstance(const HWND hwnd, const bool fullscreen)
{
    restoreDxInstance();

    if (FAILED(DirectDrawCreate(NULL, &g_moduleState->directX.instance, NULL)))
    {
        return false;
    }

    if (FAILED(g_moduleState->directX.instance->SetCooperativeLevel(hwnd,
        fullscreen ? (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN) : DDSCL_NORMAL)))
    {
        return false;
    }

    ResolutionVerifier::GetInstance().Initialize(g_moduleState->directX.instance, Graphics::kBitsPerPixel16);

    g_moduleState->isFullScreen = fullscreen;
    g_moduleState->hwnd = hwnd;

    return true;
}

// 0x100010b0
void releaseDxSurface()
{
    dxRelease(g_moduleState->directX.surface);
}

// 0x100010d0
void restoreDxInstance()
{
    releaseDxSurface();

    if (!g_moduleState->directX.instance)
        return;

    if (g_moduleState->isFullScreen)
        g_moduleState->directX.instance->RestoreDisplayMode();

    dxRelease(g_moduleState->directX.instance);
}

// 0x10001110
void releaseDxInstance()
{
    releaseDxSurface();

    dxRelease(g_moduleState->directX.instance);
}

// 0x10001130
void setPixelColorMasks(const U32 r, const U32 g, const U32 b)
{
    g_moduleState->actualRedMask = static_cast<U16>(r);
    g_moduleState->initialRedMask = static_cast<U16>(r);
    g_moduleState->actualGreenMask = static_cast<U16>(g);
    g_moduleState->initialGreenMask = static_cast<U16>(g);
    g_moduleState->actualBlueMask = static_cast<U16>(b);
    g_moduleState->initialBlueMask = static_cast<U16>(b);

    const U32 rm = r & PixelColor::DEFAULT_MASK;
    const U32 gm = g & PixelColor::DEFAULT_MASK;
    const U32 bm = b & PixelColor::DEFAULT_MASK;

    // R
    {
        size_t x = 0;
        U32 mask = kPixelColorBitMask;

        for (; x < Graphics::kBitsPerPixel16; ++x)
        {
            if (mask & rm)
            {
                g_moduleState->actualColorMask = g_moduleState->actualColorMask | static_cast<U16>(mask);
                break;
            }

            mask = mask >> 1;
        }

        g_moduleState->redOffset = static_cast<U16>(x);
    }

    // G
    {
        size_t x = 0;
        U32 mask = kPixelColorBitMask;

        for (; x < Graphics::kBitsPerPixel16; ++x)
        {
            if (mask & gm)
            {
                g_moduleState->actualColorMask = g_moduleState->actualColorMask | static_cast<U16>(mask);
                break;
            }

            mask = mask >> 1;
        }

        g_moduleState->greenOffset = static_cast<U16>(x);
    }

    // B
    {
        size_t x = 0;
        U32 mask = kPixelColorBitMask;

        for (; x < Graphics::kBitsPerPixel16; ++x)
        {
            if (mask & bm)
            {
                g_moduleState->actualColorMask = g_moduleState->actualColorMask | static_cast<U16>(mask);
                break;
            }

            mask = mask >> 1;
        }

        g_moduleState->blueOffset = static_cast<U16>(x);
    }


    // R
    {
        U32 mask = 1;

        for (size_t x = 0; x < Graphics::kBitsPerPixel16; ++x)
        {
            if (mask & rm)
            {
                g_moduleState->actualColorBits = g_moduleState->actualColorBits | static_cast<U16>(mask);
                break;
            }

            mask = mask << 1;
        }
    }

    // G
    {
        U32 mask = 1;

        for (size_t x = 0; x < Graphics::kBitsPerPixel16; ++x)
        {
            if (mask & gm)
            {
                g_moduleState->actualColorBits = g_moduleState->actualColorBits | static_cast<U16>(mask);
                break;
            }

            mask = mask << 1;
        }
    }

    // B
    {
        U32 mask = 1;

        for (size_t x = 0; x < Graphics::kBitsPerPixel16; ++x)
        {
            if (mask & bm)
            {
                g_moduleState->actualColorBits = g_moduleState->actualColorBits | static_cast<U16>(mask);
                break;
            }

            mask = mask << 1;
        }
    }

    g_moduleState->initialColorMask = g_moduleState->actualColorMask;

    g_moduleState->invActualColorBits = ~g_moduleState->actualColorMask;
    g_moduleState->invActualColorBits2 = ~g_moduleState->actualColorMask;

    g_moduleState->actualColorBits2 = g_moduleState->actualColorBits;
    g_moduleState->shadeColorMask = ~g_moduleState->actualColorBits;
    g_moduleState->shadeColorMask2 = ~g_moduleState->actualColorBits;

    g_moduleState->backSurfaceShadePixel =
        ((5 << (11 - g_moduleState->blueOffset)) + (2 << (11 - g_moduleState->greenOffset))) & PixelColor::DEFAULT_MASK;
    g_moduleState->backSurfaceShadePixel = (g_moduleState->backSurfaceShadePixel << 16) | g_moduleState->backSurfaceShadePixel;

    if (g_moduleState->greenOffset < g_moduleState->redOffset)
    {
        if (g_moduleState->blueOffset < g_moduleState->greenOffset)
        {
            g_moduleState->initialRgbMask = (gm << 16) | bm | rm;
        }
        else if (g_moduleState->blueOffset <= g_moduleState->redOffset)
        {
            g_moduleState->initialRgbMask = (bm << 16) | gm | rm;
        }
        else
        {
            g_moduleState->initialRgbMask = (rm << 16) | bm | gm;
        }
    }
    else
    {
        if (g_moduleState->greenOffset <= g_moduleState->blueOffset)
        {
            g_moduleState->initialRgbMask = (gm << 16) | bm | rm;
        }
        else if (g_moduleState->redOffset < g_moduleState->blueOffset)
        {
            g_moduleState->initialRgbMask = (bm << 16) | gm | rm;
        }
        else
        {
            g_moduleState->initialRgbMask = (rm << 16) | bm | gm;
        }
    }

    g_moduleState->actualRgbMask = ((g_moduleState->actualColorMask << 16) | g_moduleState->actualColorMask) & g_moduleState->initialRgbMask;
}

// 0x10001330
bool initWindowDxSurface(S32 width, S32 height)
{
    releaseDxSurface();

    if (g_moduleState->isFullScreen)
    {
        const bool isSupported = ResolutionVerifier::GetInstance().IsSupported(width, height, Graphics::kBitsPerPixel16);
        if (!isSupported)
            if (ResolutionVerifier::GetInstance().ChooseResolution(width, height))
                Screen::UpdateSize(width, height);

        if (FAILED(g_moduleState->directX.instance->SetDisplayMode(width, height, Graphics::kBitsPerPixel16)))
        {
            char buf[100];
            std::snprintf(buf, sizeof(buf), "Display mode %dx%dx%d is not supported by system. Terminating.", width, height, Graphics::kBitsPerPixel16);
            ShowErrorNow(buf);
            return false;
        }
        
        SetWindowPos(g_moduleState->hwnd, NULL, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE);
    }
    else
    {
        HDC hdc = GetDC(g_moduleState->hwnd);

        const U32 sw = GetDeviceCaps(hdc, HORZRES);
        const U32 sh = GetDeviceCaps(hdc, VERTRES);

        ReleaseDC(g_moduleState->hwnd, hdc);

        SetWindowLongA(g_moduleState->hwnd, GWL_STYLE, WS_CAPTION);

        RECT rect;
        ZeroMemory(&rect, sizeof(RECT));
        AdjustWindowRect(&rect, WS_CAPTION, false);

        width = width + (rect.right - rect.left);
        height = height + (rect.bottom - rect.top);

        SetWindowPos(g_moduleState->hwnd, NULL, (sw - width) / 2, (sh - height) / 2, width, height, SWP_SHOWWINDOW);
    }

    DDSURFACEDESC desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC));

    desc.dwSize = sizeof(DDSURFACEDESC);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    if (FAILED(g_moduleState->directX.instance->CreateSurface(&desc, &g_moduleState->directX.surface, NULL)))
    {
        return false;
    }

    if (FAILED(g_moduleState->directX.surface->GetSurfaceDesc(&desc)))
    {
        return false;
    }

    setPixelColorMasks(desc.ddpfPixelFormat.dwRBitMask, desc.ddpfPixelFormat.dwGBitMask, desc.ddpfPixelFormat.dwBBitMask);

    if (!g_moduleState->isFullScreen)
    {
        LPDIRECTDRAWCLIPPER clipper = NULL;

        if (FAILED(g_moduleState->directX.instance->CreateClipper(0, &clipper, NULL)))
        {
            dxRelease(g_moduleState->directX.surface);
            return false;
        }

        if (FAILED(clipper->SetHWnd(0, g_moduleState->hwnd)))
        {
            dxRelease(clipper);
            dxRelease(g_moduleState->directX.surface);
            return false;
        }

        if (FAILED(g_moduleState->directX.surface->SetClipper(clipper)))
        {
            dxRelease(clipper);
            dxRelease(g_moduleState->directX.surface);
            return false;
        }

        dxRelease(clipper);
    }

    g_moduleState->actions.initValues();

    g_moduleState->windowRect.x = 0;
    g_moduleState->windowRect.y = 0;
    g_moduleState->windowRect.width = width - 1;
    g_moduleState->windowRect.height = height - 1;

    return true;
}


// 0x10001420
void drawMainSurfaceHorLine(const S32 x, const S32 y, const S32 length, const Pixel pixel)
{
    S32 max_x = x + length - 1;
    S32 new_x = x;

    if (y >= g_moduleState->windowRect.y
        && y <= g_moduleState->windowRect.height)
    {
        if (new_x < g_moduleState->windowRect.x)
            new_x = g_moduleState->windowRect.x;
        if (g_moduleState->windowRect.width < max_x)
            max_x = g_moduleState->windowRect.width;

        if (new_x <= max_x)
        {
            Pixel* pixels = (Pixel*)((Addr)g_rendererState.surfaces.main
                + g_moduleState->surface.offset + (y * Screen::width_ + new_x) * sizeof(Pixel));

            if (g_moduleState->surface.y <= y)
                pixels = (Pixel*)((Addr)pixels - Screen::sizeInBytes_);

            std::fill_n(pixels, max_x - new_x + 1, pixel);
        }
    }
}

// 0x100014b0
void drawMainSurfaceVertLine(const S32 x, const S32 y, const S32 height, const Pixel pixel)
{
    S32 max_y = height + y - 1;
    S32 new_y = y;

    if (x < g_moduleState->windowRect.x
        || g_moduleState->windowRect.width < x)
        return;

    if (y < g_moduleState->windowRect.y)
        new_y = g_moduleState->windowRect.y;
    if (max_y > g_moduleState->windowRect.height)
        max_y = g_moduleState->windowRect.height;

    max_y += 1 - new_y;

    if (max_y < 1)
        return;

    Pixel* pixels = (Pixel*)((Addr)g_rendererState.surfaces.main
        + g_moduleState->surface.offset + (Addr)(new_y * Screen::width_ + x) * sizeof(Pixel));
    const Addr screenWidthInBytes = Screen::widthInBytes_;

    if (y < g_moduleState->surface.y)
    {
        const S32 delta = new_y + max_y - g_moduleState->surface.y;

        if (delta < 1)
        {
            for (S32 xx = 0; xx < max_y; ++xx)
            {
                pixels[0] = pixel;

                pixels = (Pixel*)((Addr)pixels + screenWidthInBytes);
            }
        }
        else
        {
            for (S32 xx = 0; xx < max_y - delta; ++xx)
            {
                pixels[0] = pixel;

                pixels = (Pixel*)((Addr)pixels + screenWidthInBytes);
            }

            pixels = (Pixel*)((Addr)pixels - Screen::sizeInBytes_);

            for (S32 xx = 0; xx < delta; ++xx)
            {
                pixels[0] = pixel;

                pixels = (Pixel*)((Addr)pixels + screenWidthInBytes);
            }
        }
    }
    else
    {
        pixels = (Pixel*)((Addr)pixels - Screen::sizeInBytes_);

        for (S32 xx = 0; xx < max_y; ++xx)
        {
            pixels[0] = pixel;

            pixels = (Pixel*)((Addr)pixels + screenWidthInBytes);
        }
    }
}

// 0x10001570
void drawMainSurfaceColorRect(const S32 x, const S32 y, const S32 width, const S32 height, const Pixel pixel)
{
    drawMainSurfaceHorLine(x, y, width, pixel);
    drawMainSurfaceHorLine(x, y + height - 1, width, pixel);
    drawMainSurfaceVertLine(x, y, height, pixel);
    drawMainSurfaceVertLine(x + width - 1, y, height, pixel);
}

// 0x100015d0
void drawMainSurfaceFilledColorRect(S32 x, S32 y, S32 width, S32 height, const Pixel pixel)
{
    if (x < g_moduleState->windowRect.x)
    {
        width += x - g_moduleState->windowRect.x;
        x = g_moduleState->windowRect.x;
    }

    if (y < g_moduleState->windowRect.y)
    {
        height += y - g_moduleState->windowRect.y;
        y = g_moduleState->windowRect.y;
    }

    if ((width + x - 1) > g_moduleState->windowRect.width)
        width = g_moduleState->windowRect.width - x + 1;

    if ((height + y - 1) > g_moduleState->windowRect.height)
        height = g_moduleState->windowRect.height - y + 1;

    if (width < 1 || height < 1)
        return;

    Pixel* pixels = (Pixel*)((Addr)g_rendererState.surfaces.main + g_moduleState->surface.offset + (Addr)(y * Screen::width_ + x) * sizeof(Pixel));
    const Addr widthInBytes = Screen::widthInBytes_;

    if (y < g_moduleState->surface.y)
    {
        const S32 delta = y + height - g_moduleState->surface.y;

        if (y + height < g_moduleState->surface.y || delta == 0)
        {
            for (S32 yy = 0; yy < height; ++yy)
            {
                std::fill_n(pixels, width, pixel);

                pixels = (Pixel*)((Addr)pixels + widthInBytes);
            }
        }
        else
        {
            for (S32 yy = 0; yy < height - delta; ++yy)
            {
                std::fill_n(pixels, width, pixel);

                pixels = (Pixel*)((Addr)pixels + widthInBytes);
            }

            pixels = (Pixel*)((Addr)pixels - Screen::sizeInBytes_);

            for (S32 yy = 0; yy < delta; ++yy)
            {
                std::fill_n(pixels, width, pixel);

                pixels = (Pixel*)((Addr)pixels + widthInBytes);
            }
        }
    }
    else
    {
        pixels = (Pixel*)((Addr)pixels - Screen::sizeInBytes_);

        for (S32 yy = 0; yy < height; ++yy)
        {
            std::fill_n(pixels, width, pixel);

            pixels = (Pixel*)((Addr)pixels + widthInBytes);
        }
    }
}

// 0x100016c0 
void drawMainSurfaceShadeColorRect(S32 x, S32 y, S32 width, S32 height, const Pixel pixel)
{
    if (x < g_moduleState->windowRect.x)
    {
        width = width + x - g_moduleState->windowRect.x;
        x = g_moduleState->windowRect.x;

        if (width < 1)
            return;
    }

    if (y < g_moduleState->windowRect.y)
    {
        height = height + y - g_moduleState->windowRect.y;
        y = g_moduleState->windowRect.y;

        if (height < 1)
            return;
    }

    if ((width + x - 1) > g_moduleState->windowRect.width)
        width = g_moduleState->windowRect.width - x + 1;

    if ((height + y - 1) > g_moduleState->windowRect.height)
        height = g_moduleState->windowRect.height - y + 1;

    if (width < 1 || height < 1)
        return;

    Pixel* pixels = (Pixel*)((Addr)g_rendererState.surfaces.main + g_moduleState->surface.offset + (Addr)(y * Screen::width_ + x) * sizeof(Pixel));
    const Addr widthInBytes = Screen::widthInBytes_;

    const Pixel color = SHADEPIXEL(pixel, g_moduleState->shadeColorMask);

    if (y < g_moduleState->surface.y)
    {
        const S32 delta = y + height - g_moduleState->surface.y;

        if ((y + height) < g_moduleState->surface.y || delta == 0)
        {
            for (S32 yy = 0; yy < height; ++yy)
            {
                for (S32 xx = 0; xx < width; ++xx)
                {
                    pixels[xx] = SHADEPIXEL(pixels[xx], g_moduleState->shadeColorMask) + color;
                }

                pixels = (Pixel*)((Addr)pixels + widthInBytes);
            }
        }
        else
        {
            for (S32 yy = 0; yy < height - delta; ++yy)
            {
                for (S32 xx = 0; xx < width; ++xx)
                {
                    pixels[xx] = SHADEPIXEL(pixels[xx], g_moduleState->shadeColorMask) + color;
                }

                pixels = (Pixel*)((Addr)pixels + widthInBytes);
            }

            pixels = (Pixel*)((Addr)pixels - Screen::sizeInBytes_);

            for (S32 yy = 0; yy < delta; ++yy)
            {
                for (S32 xx = 0; xx < width; ++xx)
                {
                    pixels[xx] = SHADEPIXEL(pixels[xx], g_moduleState->shadeColorMask) + color;
                }

                pixels = (Pixel*)((Addr)pixels + widthInBytes);
            }
        }
    }
    else
    {
        pixels = (Pixel*)((Addr)pixels - Screen::sizeInBytes_);

        for (S32 yy = 0; yy < height; ++yy)
        {
            for (S32 xx = 0; xx < width; ++xx)
            {
                pixels[xx] = SHADEPIXEL(pixels[xx], g_moduleState->shadeColorMask) + color;
            }

            pixels = (Pixel*)((Addr)pixels + widthInBytes);
        }
    }
}

// 0x100017e0
void drawMainSurfaceColorPoint(const S32 x, const S32 y, const Pixel pixel)
{
    if (x > g_moduleState->windowRect.x
        && y > g_moduleState->windowRect.y
        && x <= g_moduleState->windowRect.width
        && y <= g_moduleState->windowRect.height)
    {
        S32 offset = g_moduleState->surface.offset / sizeof(Pixel) + y * Screen::width_ + x;

        if (g_moduleState->surface.y <= y)
            offset -= Screen::sizeInPixels_;

        g_rendererState.surfaces.main[offset] = pixel;
    }
}

// 0x10001840 
void drawBackSurfaceColorPoint(const S32 x, const S32 y, const Pixel pixel)
{
    if (x > g_moduleState->windowRect.x
        && y > g_moduleState->windowRect.y
        && x <= g_moduleState->windowRect.width
        && y <= g_moduleState->windowRect.height)
    {
        S32 offset = g_moduleState->surface.offset / sizeof(Pixel) + y * Screen::width_ + x;

        if (g_moduleState->surface.y <= y)
            offset -= Screen::sizeInPixels_;

        g_rendererState.surfaces.back[offset] = pixel;
    }
}


// 0x100018a0
void readMainSurfaceRect(const S32 sx, const S32 sy, const S32 width, const S32 height, const S32 dx, const S32 dy, const S32 stride, Pixel* surface)
{
    Pixel* src = (Pixel*)((Addr)g_rendererState.surfaces.main + g_moduleState->surface.offset + (sy * Screen::width_ + sx) * sizeof(Pixel));
    Pixel* dst = (Pixel*)((Addr)surface + (stride * dy + dx) * sizeof(Pixel));
    const Addr widthInBytes = Screen::widthInBytes_;

    if (sy < g_moduleState->surface.y)
    {
        const S32 delta = sy + height - g_moduleState->surface.y;

        if (delta <= 0)
        {
            for (S32 yy = 0; yy < height; ++yy)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
                src = (Pixel*)((Addr)src + widthInBytes);
                dst = (Pixel*)((Addr)dst + (Addr)(stride * sizeof(Pixel)));
            }
        }
        else
        {
            for (S32 yy = 0; yy < height - delta; ++yy)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
                src = (Pixel*)((Addr)src + widthInBytes);
                dst = (Pixel*)((Addr)dst + (Addr)(stride * sizeof(Pixel)));
            }

            src = (Pixel*)((Addr)src - Screen::sizeInBytes_);

            for (S32 yy = 0; yy < delta; ++yy)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
                src = (Pixel*)((Addr)src + widthInBytes);
                dst = (Pixel*)((Addr)dst + (Addr)(stride * sizeof(Pixel)));
            }
        }
    }
    else
    {
        src = (Pixel*)((Addr)src - Screen::sizeInBytes_);

        for (S32 yy = 0; yy < height; ++yy)
        {
            std::memcpy(dst, src, width * sizeof(Pixel));
            src = (Pixel*)((Addr)src + widthInBytes);
            dst = (Pixel*)((Addr)dst + (Addr)(stride * sizeof(Pixel)));
        }
    }
}

// 0x10001be0
void convertNotMagentaColors(const Pixel* input, Pixel* output, const S32 count)
{
    for (S32 x = 0; x < count; ++x)
    {
        const Pixel pixel = input[x];

        if (pixel == PixelColor::MAGENTA)
        {
            output[x] = static_cast<U16>(PixelColor::MAGENTA);
            continue;
        }

        const Pixel r = (((pixel & 0xF800) << 0) >> (g_moduleState->redOffset & 0x1F)) & g_moduleState->actualRedMask;
        const Pixel g = (((pixel & 0x07E0) << 5) >> (g_moduleState->greenOffset & 0x1F)) & g_moduleState->actualGreenMask;
        const Pixel b = (((pixel & 0x001F) << 11) >> (g_moduleState->blueOffset & 0x1F)) & g_moduleState->actualBlueMask;

        output[x] = (Pixel)(r | g | b);
    }
}

// 0x10001c80
void convertAllColors(const Pixel* input, Pixel* output, const S32 count)
{
    for (S32 x = 0; x < count; ++x)
    {
        const Pixel pixel = input[x];

        const Pixel r = (((pixel & 0xF800) << 0) >> (g_moduleState->redOffset & 0x1F)) & g_moduleState->actualRedMask;
        const Pixel g = (((pixel & 0x07E0) << 5) >> (g_moduleState->greenOffset & 0x1F)) & g_moduleState->actualGreenMask;
        const Pixel b = (((pixel & 0x001F) << 11) >> (g_moduleState->blueOffset & 0x1F)) & g_moduleState->actualBlueMask;

        output[x] = (Pixel)(r | g | b);
    }
}


// 0x10001d00
void copyMainBackSurfaces(const S32 dx, const S32 dy)
{
    const S32 screenWidth = Screen::width_;
    const S32 screenHeight = Screen::height_;
    const S32 screenSizeInPixels = Screen::sizeInPixels_;
    Pixel* src;
    Pixel* dst;

    S32 offset = g_moduleState->surface.offset / sizeof(Pixel) + dy * screenWidth + dx;
    if (offset < 0)
    {
        do
        {
            offset += screenSizeInPixels;
        } while (offset < 0);
    }
    else
    {
        while (offset >= screenSizeInPixels)
            offset -= screenSizeInPixels;
    }

    S32 x_max = dx + (g_moduleState->surface.offset / sizeof(Pixel)) % screenWidth;
    do
    {
        if (x_max >= 0)
        {
            if (x_max < screenWidth)
                break;

            src = g_moduleState->surface.back + screenSizeInPixels;
            dst = g_moduleState->surface.back;
            std::memcpy(dst, src, screenWidth * sizeof(Pixel));

            src = g_moduleState->surface.main + screenSizeInPixels;
            dst = g_moduleState->surface.main;
            std::memcpy(dst, src, screenWidth * sizeof(Pixel));

            src = g_moduleState->surface.stencil + screenSizeInPixels;
            dst = g_moduleState->surface.stencil;
        }
        else
        {
            src = g_moduleState->surface.back;
            dst = g_moduleState->surface.back + screenSizeInPixels;
            std::memcpy(dst, src, screenWidth * sizeof(Pixel));

            src = g_moduleState->surface.main;
            dst = g_moduleState->surface.main + screenSizeInPixels;
            std::memcpy(dst, src, screenWidth * sizeof(Pixel));

            src = g_moduleState->surface.stencil;
            dst = g_moduleState->surface.stencil + screenSizeInPixels;
        }
        std::memcpy(dst, src, screenWidth * sizeof(Pixel));
    } while (false);

    g_moduleState->surface.offset = offset * sizeof(Pixel);
    g_moduleState->surface.y = screenHeight - offset / screenWidth;

    if (dy <= 0)
    {
        if (dy < 0)
        {
            if (dx <= 0)
                moveStencilSurface(-dx, -dy, dx + screenWidth, dy + screenHeight, -dy);
            else
                moveStencilSurface(0, -dy, screenWidth - dx, dy + screenHeight, -dy);
        }
    }
    else
    {
        if (dx <= 0)
            moveStencilSurface(-dx, 0, dx + screenWidth, screenHeight - dy, -dy);
        else
            moveStencilSurface(0, 0, screenWidth - dx, screenHeight - dy, -dy);
    }
}

// 0x10001e90
void callDrawBackSurfacePaletteRhomb(const S32 tx, const S32 ty, const S32 angle_0, const S32 angle_1, const S32 angle_2, const S32 angle_3, const ImagePaletteTile* const tile)
{
    // This check was added since in the original Cad value 2 * g_moduleState->surface.width is passed to the next called function
#ifdef CHECK_ASSERTS
    assert(g_moduleState->surface.width * static_cast<S32>(sizeof(Pixel)) == g_moduleState->surface.stride);
#endif
    drawSurfacePaletteRhomb(angle_0, angle_1, angle_2, angle_3, tx, ty, g_moduleState->surface.stride, tile, g_rendererState.surfaces.back);
}

// 0x10001ed0
void callShadeMainSurfaceRhomb(const S32 x, const S32 y, const S32 angle_0, const S32 angle_1, const S32 angle_2, const S32 angle_3)
{
    shadeSurfaceRhomb(angle_0, angle_1, angle_2, angle_3, x, y, g_moduleState->surface.stride, g_rendererState.surfaces.main);
}

// 0x10001f10
void callDrawBackSurfaceMaskRhomb(const S32 x, const S32 y, const S32 color)
{
    drawSurfaceMaskRhomb(x, y, g_moduleState->surface.stride, color, g_rendererState.surfaces.main);
}

// 0x10001f40
void callCleanMainSurfaceRhomb(const S32 x, const S32 y, const S32 angle_0, const S32 angle_1, const S32 angle_2, const S32 angle_3, const ImagePaletteTile* const tile)
{
    cleanSurfaceRhomb(angle_0, angle_1, angle_2, angle_3, x, y, g_moduleState->surface.stride, tile, g_rendererState.surfaces.main);
}


// 0x10001f80
void copyBackToMainSurfaceRect(const S32 x, const S32 y, const U32 width, const U32 height)
{
    const Addr offset = (Addr)(x + y * Screen::width_);
    Pixel* src = (Pixel*)((Addr)g_rendererState.surfaces.back + g_moduleState->surface.offset + offset * sizeof(Pixel));
    Pixel* dst = (Pixel*)((Addr)g_rendererState.surfaces.main + g_moduleState->surface.offset + offset * sizeof(Pixel));
    const Addr widthInBytes = Screen::widthInBytes_;

    auto copyBlock = [&](Pixel*& s, Pixel*& d, int lines)
        {
            while (lines--)
            {
                std::memcpy(d, s, width * sizeof(Pixel));
                s = (Pixel*)((Addr)s + widthInBytes);
                d = (Pixel*)((Addr)d + widthInBytes);
            }
        };

    if (y < g_moduleState->surface.y)
    {
        const S32 delta = y + height - g_moduleState->surface.y;
        if (delta <= 0)
        {
            copyBlock(src, dst, height);
        }
        else
        {
            copyBlock(src, dst, height - delta);
            src = (Pixel*)((Addr)src - Screen::sizeInBytes_);
            dst = (Pixel*)((Addr)dst - Screen::sizeInBytes_);
            copyBlock(src, dst, delta);
        }
    }
    else
    {
        src = (Pixel*)((Addr)src - Screen::sizeInBytes_);
        dst = (Pixel*)((Addr)dst - Screen::sizeInBytes_);
        copyBlock(src, dst, height);
    }
}

// 0x10002020
void drawMainSurfaceColorEllipse(const S32 x, const S32 y, S32 size, const Pixel pixel, const S32 step)
{
    S32 yy = 0;
    S32 current = 1 - size;
    S32 distance = step;
    S32 xx = size;
    S32 end = 0;
    S32 start = -size;
    S32 offset = start;

    size = size * 2;

    do {
        if (step < distance) {
            distance = distance - step;
            drawMainSurfaceColorPoint(xx + x, yy + y, pixel);
            start = offset;
        }
        if (-1 < current) {
            size = size + -2;
            xx = xx + -1;
            start = start + 1;
            distance = distance + 0x3989;
            current = current - size;
            offset = start;
        }
        end = end + -4;
        yy = yy + 1;
        distance = distance + 0x10000;
        current = current + 1 + yy * 8;
    } while (start <= end);

    size = yy * 8;
    current = -current;

    do {
        if (step < distance) {
            distance = distance - step;
            drawMainSurfaceColorPoint(xx + x, yy + y, pixel);
        }
        if (-1 < current) {
            size = size + 8;
            yy = yy + 1;
            distance = distance + 0x3989;
            current = current - size;
        }
        xx = xx + -1;
        distance = distance + 0x10000;
        current = current + -1 + xx * 2;
    } while (-1 < xx);

    offset = xx * 2;
    start = yy * -4;
    size = yy * 8;
    end = start;

    do {
        if (step < distance) {
            distance = distance - step;
            drawMainSurfaceColorPoint(xx + x, yy + y, pixel);
            start = end;
        }
        if (-1 < current) {
            size = size + -8;
            start = start + 4;
            yy = yy + -1;
            distance = distance + 0x3989;
            current = current - size;
            end = start;
        }
        offset = offset + -2;
        xx = xx + -1;
        distance = distance + 0x10000;
        current = current + (1 - offset);
    } while (start <= xx);

    current = -current;

    do {
        if (step < distance) {
            distance = distance - step;
            drawMainSurfaceColorPoint(xx + x, yy + y, pixel);
        }
        if (-1 < current) {
            xx = xx + -1;
            distance = distance + 0x3989;
            current = current + xx * 2;
        }
        yy = yy + -1;
        distance = distance + 0x10000;
        current = current + 1 + yy * 8;
    } while (-1 < yy);

    size = yy * 4;
    end = yy * 8;

    do {
        if (step < distance) {
            distance = distance - step;
            drawMainSurfaceColorPoint(xx + x, yy + y, pixel);
        }
        if (-1 < current) {
            xx = xx + 1;
            distance = distance + 0x3989;
            current = current + xx * 2;
        }
        size = size + -4;
        end = end + -8;
        yy = yy + -1;
        distance = distance + 0x10000;
        current = current + (-1 - end);
    } while (xx <= size);

    current = -current;
    start = -xx;
    size = xx * 2;

    do {
        if (step < distance) {
            distance = distance - step;
            drawMainSurfaceColorPoint(xx + x, yy + y, pixel);
        }
        if (-1 < current) {
            yy = yy + -1;
            distance = distance + 0x3989;
            current = current + yy * 8;
        }
        size = size + 2;
        xx = xx + 1;
        start = start + -1;
        distance = distance + 0x10000;
        current = current + (1 - size);
    } while (-1 < start);

    size = yy * 4;
    start = -xx;

    do {
        if (step < distance) {
            distance = distance - step;
            drawMainSurfaceColorPoint(xx + x, yy + y, pixel);
        }
        if (-1 < current) {
            yy = yy + 1;
            size = size + 4;
            distance = distance + 0x3989;
            current = current + yy * 8;
        }
        xx = xx + 1;
        start = start + -1;
        distance = distance + 0x10000;
        current = current + -1 + xx * 2;
    } while (size <= start);

    end = yy * 8;
    current = -current;
    size = xx * 2;
    xx = xx + x;
    start = yy * -4;
    yy = yy + y;

    do {
        if (step < distance) {
            distance = distance - step;
            drawMainSurfaceColorPoint(xx, yy, pixel);
        }
        if (-1 < current) {
            size = size + 2;
            xx = xx + 1;
            distance = distance + 0x3989;
            current = current - size;
        }
        end = end + 8;
        start = start + -4;
        current = current + (-1 - end);
        yy = yy + 1;
        distance = distance + 0x10000;
    } while (-1 < start);
}

// 0x100023e0
void drawMainSurfaceColorOutline(S32 x, S32 y, S32 width, S32 height, const Pixel pixel)
{
    const S32 offset = (g_moduleState->surface.offset / sizeof(Pixel)) % Screen::width_;

    Pixel* src = (Pixel*)((Addr)g_rendererState.surfaces.main
        + (Addr)((offset + Screen::sizeInPixels_) * sizeof(Pixel)));

    g_rendererState.outline.width = g_moduleState->windowRect.width - g_moduleState->windowRect.x;
    g_rendererState.outline.height = g_moduleState->windowRect.height + 1 - g_moduleState->windowRect.y;
    g_rendererState.outline.options = OUTLINE_DRAW_OPTION_NONE;

    x = x - g_moduleState->windowRect.x;
    y = y - g_moduleState->windowRect.y;

    if (y < 0)
    {
        height = height + y;

        if (height <= 0)
            return;

        y = 0;
        g_rendererState.outline.options = (OutlintDrawOption)(g_rendererState.outline.options | OUTLINE_DRAW_OPTION_TOP);
    }

    if (y >= g_rendererState.outline.height)
    {
        height = height + y - (g_rendererState.outline.height - 1);

        if (height >= 0)
            return;

        y = g_rendererState.outline.height - 1;
        g_rendererState.outline.options = (OutlintDrawOption)(g_rendererState.outline.options | OUTLINE_DRAW_OPTION_TOP);
    }

    {
        const S32 max = y + 1 + height;

        if (y + 1 + height < 0 != max < 0)
        {
            height = height - max - 1;
            g_rendererState.outline.options = (OutlintDrawOption)(g_rendererState.outline.options | OUTLINE_DRAW_OPTION_BOTTOM);
        }
    }

    {
        const S32 max = y - 1 + height;

        if (max >= g_rendererState.outline.height)
        {
            height = height + g_rendererState.outline.height - max;
            g_rendererState.outline.options = (OutlintDrawOption)(g_rendererState.outline.options | OUTLINE_DRAW_OPTION_BOTTOM);
        }
    }

    if (x < 1)
    {
        width = width + x;

        if (width < 2)
            return;

        x = 1;
        width = width - 1;

        g_rendererState.outline.options = (OutlintDrawOption)(g_rendererState.outline.options | OUTLINE_DRAW_OPTION_LEFT);
    }

    if (x >= g_rendererState.outline.width + 2)
    {
        width = width + x + 1 - (g_rendererState.outline.width + 2);
        if (width >= 0)
            return;
        x = g_rendererState.outline.width + 1;
        g_rendererState.outline.options = (OutlintDrawOption)(g_rendererState.outline.options | OUTLINE_DRAW_OPTION_LEFT);
    }

    if (x + width <= 0 != x + width < 0)
    {
        width = width - x - width;

        g_rendererState.outline.options = (OutlintDrawOption)(g_rendererState.outline.options | OUTLINE_DRAW_OPTION_RIGHT);
    }

    {
        S32 max = x - 2 + width;

        if (max > g_rendererState.outline.width)
        {
            width = width + g_rendererState.outline.width - max;
            g_rendererState.outline.options = (OutlintDrawOption)(g_rendererState.outline.options | OUTLINE_DRAW_OPTION_RIGHT);
        }
    }

    // Offset in bytes to the next changed pixel. Since we use Pixel*, I changed it to 1
    g_rendererState.outline.horizontalStride = 1;
    if (width < 0)
    {
        g_rendererState.outline.horizontalStride = -g_rendererState.outline.horizontalStride;
        width = -width;
    }

    g_rendererState.outline.verticalStride = 1;
    g_rendererState.outline.stride = g_moduleState->surface.stride;
    if (height < 0)
    {
        g_rendererState.outline.stride = -g_moduleState->surface.stride;
        height = -height;
        g_rendererState.outline.verticalStride = -g_rendererState.outline.verticalStride;
    }

    Pixel* dst = (Pixel*)((Addr)g_rendererState.surfaces.main
        + g_moduleState->surface.offset + y * g_moduleState->surface.stride + x * sizeof(Pixel))
        + (Addr)(g_moduleState->windowRect.y * g_moduleState->surface.stride + g_moduleState->windowRect.x * sizeof(Pixel));

    if ((g_rendererState.outline.options & OUTLINE_DRAW_OPTION_TOP) == OUTLINE_DRAW_OPTION_NONE)
    {
        height = height - 1;

        Pixel* pixels = dst;

        if (src <= dst)
        {
            pixels = (Pixel*)((Addr)dst - Screen::sizeInBytes_);
        }

        for (S32 xx = 0; xx < width; ++xx)
        {
            pixels[g_rendererState.outline.horizontalStride * xx] = pixel;
        }

        dst = (Pixel*)((Addr)dst + (Addr)g_rendererState.outline.stride);
    }

    if ((g_rendererState.outline.options & OUTLINE_DRAW_OPTION_RIGHT) == OUTLINE_DRAW_OPTION_NONE)
    {
        S32 off = (width - 1) * sizeof(Pixel);
        if (g_rendererState.outline.horizontalStride < 0)
        {
            off = -off;
        }

        Pixel* pixels = (Pixel*)((Addr)dst + (Addr)off);

        for (S32 yy = 0; yy < height - 1; ++yy)
        {
            if (src <= pixels)
            {
                pixels = (Pixel*)((Addr)pixels - Screen::sizeInBytes_);
            }

            pixels[0] = pixel;

            pixels = (Pixel*)((Addr)pixels + (Addr)g_rendererState.outline.stride);
        }
    }

    if ((g_rendererState.outline.options & OUTLINE_DRAW_OPTION_LEFT) == OUTLINE_DRAW_OPTION_NONE)
    {
        for (S32 yy = 0; yy < height - 1; ++yy)
        {
            if (src <= dst)
            {
                dst = (Pixel*)((Addr)dst - Screen::sizeInBytes_);
            }

            dst[0] = pixel;

            dst = (Pixel*)((Addr)dst + (Addr)g_rendererState.outline.stride);
        }
    }
    else
    {
        dst = (Pixel*)((Addr)dst + (Addr)(g_rendererState.outline.stride * (height - 1)));
    }

    if (height != 0 && (g_rendererState.outline.options & OUTLINE_DRAW_OPTION_BOTTOM) == OUTLINE_DRAW_OPTION_NONE)
    {
        if (src <= dst)
        {
            dst = (Pixel*)((Addr)dst - Screen::sizeInBytes_);
        }

        for (S32 xx = 0; xx < width; ++xx)
        {
            dst[g_rendererState.outline.horizontalStride * xx] = pixel;
        }
    }
}


// 0x100026e0
void resetStencilSurface()
{
    Pixel* pixels = (Pixel*)((Addr)g_rendererState.surfaces.stencil + g_moduleState->surface.offset + (g_moduleState->windowRect.y * Screen::width_ + g_moduleState->windowRect.x) * sizeof(Pixel));

    const S32 height = g_moduleState->windowRect.height - g_moduleState->windowRect.y + 1;
    const S32 width = g_moduleState->windowRect.width - g_moduleState->windowRect.x + 1;
    const S32 stride = (Screen::width_ - width) * sizeof(Pixel);

    Pixel pixel = (Pixel)(g_moduleState->windowRect.y << kStencilPixelColorShift);

    auto processRows = [&](S32 rows)
        {
            for (S32 i = 0; i < rows; ++i)
            {
                std::fill_n(pixels, width, pixel);

                pixel += kStencilPixelColorValue;

                pixels = (Pixel*)((Addr)pixels + width * sizeof(Pixel) + stride);
            }
        };

    if (g_moduleState->windowRect.y < g_moduleState->surface.y)
    {
        const S32 delta = g_moduleState->windowRect.y + height - g_moduleState->surface.y;

        if ((g_moduleState->windowRect.y + height) < g_moduleState->surface.y || delta == 0)
        {
            processRows(height);
        }
        else
        {
            processRows(height - delta);

            pixels = (Pixel*)((Addr)pixels - Screen::sizeInBytes_);

            processRows(delta);
        }
    }
    else
    {
        pixels = (Pixel*)((Addr)pixels - Screen::sizeInBytes_);

        processRows(height);
    }
}

// 0x10002780
void maskStencilSurfaceRect(S32 x, S32 y, S32 width, S32 height)
{
    Pixel* pixels = (Pixel*)((Addr)g_rendererState.surfaces.stencil + g_moduleState->surface.offset + (y * Screen::width_ + x) * sizeof(Pixel));

    const S32 stride = (Screen::width_ - width) * sizeof(Pixel);

    const Pixel pixel = (Pixel)kStencilPixelBigMask;

    auto processRows = [&](S32 rows)
        {
            for (S32 i = 0; i < rows; ++i)
            {
                for (S32 j = 0; j < width; ++j)
                    pixels[j] = pixels[j] & pixel;

                pixels = (Pixel*)((Addr)pixels + width * sizeof(Pixel) + stride);
            }
        };

    if (y < g_moduleState->surface.y)
    {
        const S32 delta = y + height - g_moduleState->surface.y;

        if ((y + height) < g_moduleState->surface.y || delta == 0)
        {
            processRows(height);
        }
        else
        {
            processRows(height - delta);

            pixels = (Pixel*)((Addr)pixels - Screen::sizeInBytes_);

            processRows(delta);
        }
    }
    else
    {
        pixels = (Pixel*)((Addr)pixels - Screen::sizeInBytes_);

        processRows(height);
    }
}

// 0x10002810
void moveStencilSurface(const S32 x, const S32 y, const S32 width, const S32 height, const S32 offset)
{
    DoublePixel* pixels = (DoublePixel*)((Addr)g_rendererState.surfaces.stencil + g_moduleState->surface.offset + (Screen::width_ * y + x) * sizeof(Pixel));
    const S32 stride = sizeof(Pixel) * (Screen::width_ - width);

    const bool addOp = offset >= 0;
    const Pixel pixel = (Pixel)((addOp ? offset : -offset) << kStencilPixelColorShift);

    DoublePixel pix = ((DoublePixel)(pixel) << Graphics::kBitsPerPixel16) | (DoublePixel)pixel;

    // Imitate "pixels[i] - pix"
    if (addOp == false)
        pix = (DoublePixel)(-(S32)pix);

    // SSE2 is not needed here. For full HD scalar time is 0.18ms, SSE2 time is 0.20ms
    auto processRows = [&](S32 rows)
        {
            for (S32 i = 0; i < rows; ++i)
            {
                for (S32 j = 0; j < (width >> 1); ++j)
                    *pixels++ += pix;

                if (width & 1)
                    *(Pixel*)pixels += (Pixel)pix;

                pixels = (DoublePixel*)((Addr)pixels + stride);
            }
        };

    if (y < g_moduleState->surface.y)
    {
        const S32 delta = y + height - g_moduleState->surface.y;
        if (delta <= 0)
        {
            // Entire region is before the surface.y
            processRows(height);
        }
        else
        {
            // First process rows before the surface.y
            processRows(height - delta);

            // Update remaining height and pixels pointer
            pixels = (DoublePixel*)((Addr)pixels - Screen::sizeInBytes_);
            processRows(delta);
        }
    }
    else
    {
        // Entire region is after the surface.y, so moving to the array beginning
        pixels = (DoublePixel*)((Addr)pixels - Screen::sizeInBytes_);
        processRows(height);
    }
}


// 0x100028f0
bool lockDxSurface()
{
    DDSURFACEDESC desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC));
    desc.dwSize = sizeof(DDSURFACEDESC);

    HRESULT result = g_moduleState->directX.surface->Lock(NULL, &desc, DDLOCK_WAIT, NULL);

    while (true)
    {
        if (SUCCEEDED(result))
        {
            g_moduleState->pitch = desc.lPitch;

            U32 offset = 0;

            if (!g_moduleState->isFullScreen)
            {
                RECT rect;
                ZeroMemory(&rect, sizeof(RECT));
                GetClientRect(g_moduleState->hwnd, &rect);

                POINT point;
                ZeroMemory(&point, sizeof(POINT));
                ClientToScreen(g_moduleState->hwnd, &point);

                OffsetRect(&rect, point.x, point.y);

                offset = desc.lPitch * rect.top + rect.left * sizeof(Pixel);
            }

            g_moduleState->surface.renderer = (void*)((Addr)desc.lpSurface + (Addr)offset);

            return true;
        }

        if (result != DDERR_SURFACEBUSY && result != DDERR_SURFACELOST)
            if (FAILED(g_moduleState->directX.surface->Restore()))
                break;

        result = g_moduleState->directX.surface->Lock(NULL, &desc, DDLOCK_WAIT, NULL);
    }

    return false;
}

// 0x10002970
void unlockDxSurface()
{
    g_moduleState->directX.surface->Unlock(NULL);

    g_moduleState->surface.renderer = NULL;
}


// 0x10002990
bool copyToRendererSurfaceRect(S32 sx, S32 sy, S32 width, S32 height, S32 dx, S32 dy, S32 stride, const Pixel* const pixels)
{
    bool locked = false;

    if (g_moduleState->surface.renderer == NULL)
    {
        if (!lockDxSurface())
            return false;

        locked = true;
    }

    Pixel* src = (Pixel*)((Addr)pixels + (stride * sy + sx) * sizeof(Pixel));
    Pixel* dst = (Pixel*)((Addr)g_moduleState->surface.renderer + g_moduleState->pitch * dy + dx * sizeof(Pixel));

    for (S32 yy = 0; yy < height; ++yy)
    {
        std::memcpy(dst, src, width * sizeof(Pixel));
        src = (Pixel*)((Addr)src + stride * sizeof(Pixel));
        dst = (Pixel*)((Addr)dst + g_moduleState->pitch);
    }

    if (locked)
        unlockDxSurface();

    return locked;
}

// 0x10002a30
void copyPixelRectFromTo(S32 sx, S32 sy, S32 sstr, const Pixel* const input, S32 dx, S32 dy, S32 dstr, Pixel* const output, S32 width, S32 height)
{
    Pixel* src = (Pixel*)((Addr)input + (sstr * sy + sx) * sizeof(Pixel));
    Pixel* dst = (Pixel*)((Addr)output + (dstr * dy + dx) * sizeof(Pixel));

    for (S32 yy = 0; yy < height; ++yy)
    {
        std::memcpy(dst, src, width * sizeof(Pixel));
        src = (Pixel*)((Addr)src + sstr * sizeof(Pixel));
        dst = (Pixel*)((Addr)dst + dstr * sizeof(Pixel));
    }
}

// 0x10002a90
bool copyMainSurfaceToRenderer(S32 x, S32 y, S32 width, S32 height)
{
    bool locked = false;

    if (g_moduleState->surface.renderer == NULL)
    {
        if (!lockDxSurface())
        {
            return false;
        }

        locked = true;
    }
    Pixel* src = (Pixel*)((Addr)g_rendererState.surfaces.main + g_moduleState->surface.offset + (y * Screen::width_ + x) * sizeof(Pixel));
    void* dst = (void*)((Addr)g_moduleState->surface.renderer + g_moduleState->pitch * y + x * sizeof(Pixel));
    const Addr widthInBytes = Screen::widthInBytes_;

    if (y < g_moduleState->surface.y)
    {
        const S32 delta = y + height - g_moduleState->surface.y;
        if (delta <= 0)
        {
            for (S32 vertical = 0; vertical < height; vertical++)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
                src = (Pixel*)((Addr)src + widthInBytes);
                dst = (void*)((Addr)dst + g_moduleState->pitch);
            }
        }
        else
        {
            for (S32 vertical = 0; vertical < height - delta; vertical++)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
                src = (Pixel*)((Addr)src + widthInBytes);
                dst = (void*)((Addr)dst + g_moduleState->pitch);
            }

            src = (Pixel*)((Addr)src - Screen::sizeInBytes_);

            for (S32 vertical = 0; vertical < delta; vertical++)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
                src = (Pixel*)((Addr)src + widthInBytes);
                dst = (void*)((Addr)dst + g_moduleState->pitch);
            }
        }
    }
    else
    {
        src = (Pixel*)((Addr)src - Screen::sizeInBytes_);

        for (S32 yy = 0; yy < height; ++yy)
        {
            std::memcpy(dst, src, width * sizeof(Pixel));
            src = (Pixel*)((Addr)src + widthInBytes);
            dst = (void*)((Addr)dst + (Addr)g_moduleState->pitch);
        }
    }

    if (locked)
    {
        unlockDxSurface();
    }

    return locked;
}

// 0x10002b90
void copyMainSurfaceToRendererWithWarFog(const S32 x, const S32 y, const S32 endX, const S32 endY)
{
    bool locked = false;

    if (g_moduleState->surface.renderer == NULL)
    {
        if (!lockDxSurface())
        {
            return;
        }

        locked = true;
    }

    constexpr S32 blockSize = 8 * sizeof(DoublePixel);  // 32 bytes
    constexpr U32 blocksNumber = 8;

    S32 delta = (endY + 1) - y;
    g_rendererState.fogBlockParams2.unk04 = 0;
    g_rendererState.fogRenderParams.actualRgbMask = g_moduleState->initialRgbMask;
    g_rendererState.fogRenderParams.dstRowStride = -8 * g_moduleState->pitch + blockSize;
    g_rendererState.fogRenderParams.lineStep = -(S32)Screen::width_ * 16 + blockSize;

    U8* fogSrc = &g_moduleState->fogSprites[(y >> 3) + 8].unk[(x >> 4) + 8];
    DoublePixel* src = (DoublePixel*)((Addr)g_rendererState.surfaces.main + g_moduleState->surface.offset + (Screen::width_ * y + x) * sizeof(Pixel));
    DoublePixel* dst = (DoublePixel*)((Addr)g_moduleState->surface.renderer + x * sizeof(Pixel) + y * g_moduleState->pitch);
    const Addr screenSizeInBytes = Screen::sizeInBytes_;
    const Addr screenWidthInBytes = Screen::widthInBytes_;

    if (y >= g_moduleState->surface.y)
        src = (DoublePixel*)((Addr)src - screenSizeInBytes);

    if (y >= g_moduleState->surface.y || y + delta <= g_moduleState->surface.y)
    {
        g_rendererState.fogBlockParams.validRowsBlockCount = delta >> 3;
        g_rendererState.fogBlockParams.tempBlocksCount = 0;
        g_rendererState.fogBlockParams2.excessRowsBlockCount = 0;

        g_rendererState.fogRenderParams.blocksCount = blocksNumber;
    }
    else
    {
        g_rendererState.fogBlockParams.validRowsBlockCount = (g_moduleState->surface.y - y) >> 3;
        g_rendererState.fogBlockParams2.excessRowsBlockCount = (delta + y - g_moduleState->surface.y) >> 3;
        U32 v7 = ((U8)g_moduleState->surface.y - (U8)y) & 7;
        v7 = v7 | ((8 - v7) << 8);      // v7 now is 0000 ' 0000 ' 8 - v7 ' v7, so the summ of all its bytes is always 8    
        g_rendererState.fogBlockParams.tempBlocksCount = v7;

        if (g_rendererState.fogBlockParams.validRowsBlockCount == 0)
        {
            g_rendererState.fogRenderParams.lineStep += screenSizeInBytes;
            g_rendererState.fogRenderParams.blocksCount = v7;
            g_rendererState.fogBlockParams.validRowsBlockCount = 1;
            g_rendererState.fogBlockParams.tempBlocksCount = 0;
        }
        else
            g_rendererState.fogRenderParams.blocksCount = blocksNumber;
    }

    S32 remainingExcessRows;
    DoublePixel* srcTemp;
    DoublePixel* dstTemp;
    do
    {
        while (true)
        {
            do {
                srcTemp = src;
                dstTemp = dst;

                U8* someFog = fogSrc;
                fogSrc = someFog + sizeof(Fog);

                g_rendererState.fogRenderParams.fogPtr = someFog;
                g_rendererState.fogBlockParams.unk04 = ((endX + 1) - x) >> 4;

                do {
                    DoublePixel fogPixel = *(DoublePixel*)((Addr)g_rendererState.fogRenderParams.fogPtr - 2);
                    fogPixel = (fogPixel & 0xFFFF'0000) | (*(Pixel*)((Addr)g_rendererState.fogRenderParams.fogPtr + sizeof(Fog)));

                    if (fogPixel)
                    {
                        if (fogPixel == 0x80808080)
                        {
                            U32 j = g_rendererState.fogRenderParams.blocksCount;
                            ++g_rendererState.fogRenderParams.fogPtr;
                            while (true)
                            {
                                do
                                {
                                    memcpy(dst, src, 8 * sizeof(*src));
                                    src = (DoublePixel*)((Addr)src + screenWidthInBytes);
                                    dst = (DoublePixel*)((Addr)dst + (Addr)g_moduleState->pitch);

                                    --j;
                                } while (j & 0xFF);
                                j >>= 8;
                                if (j == 0)
                                    break;
                                src = (DoublePixel*)((Addr)src - screenSizeInBytes);
                            }
                        }
                        else
                        {
                            // Break fogPixel into bytes
                            U8 fogPixelLowByte = fogPixel & 0xFF;          // Byte 0
                            U8 fogValueMidByte = (fogPixel >> 8) & 0xFF;   // Byte 1
                            U8 fogPixelHighByte = (fogPixel >> 16) & 0xFF; // Byte 2
                            U8 fogPixelTopByte = (fogPixel >> 24) & 0xFF;  // Byte 3

                            // Step 1: fogValueMidByte -= fogPixelTopByte, track borrow
                            bool borrow1 = fogValueMidByte < fogPixelTopByte;
                            fogValueMidByte -= fogPixelTopByte;

                            // Step 2: fogValueMidByte -= fogPixelLowByte, track another borrow
                            bool borrow2 = fogValueMidByte < fogPixelLowByte;
                            fogValueMidByte -= fogPixelLowByte;

                            // Step 3: Compute carry of (fogPixelHighByte + fogValueMidByte)
                            bool carry = ((uint16_t)fogPixelHighByte + (uint16_t)fogValueMidByte) > 0xFF;
                            fogValueMidByte += fogPixelHighByte;

                            // Final v27 computation
                            S32 v27 = carry + (-(S32)borrow1) - (S32)borrow2;
                            v27 = (v27 & 0xFFFF'FF00) | fogValueMidByte;
                            g_rendererState.fogBlockParams.unk01 = v27 >> 2;

                            // Compute v28 (difference between fogPixelTopByte and fogPixelHighByte)
                            S32 v28 = -(S32)(fogPixelTopByte < fogPixelHighByte);
                            fogPixelTopByte -= fogPixelHighByte;
                            v28 = (v28 & 0xFFFF'FF00) | fogPixelTopByte;
                            g_rendererState.fogBlockParams2.unk01 = 2 * v28;

                            // Compute v30 (difference between fogPixelLowByte and fogPixelLow)
                            S32 v30 = -(S32)(fogPixelLowByte < fogPixelHighByte);
                            fogPixelLowByte -= fogPixelHighByte;
                            v30 = (v30 & 0xFFFF'FF00) | fogPixelLowByte;

                            S32 v31 = 4 * v30;
                            g_rendererState.fogBlockParams2.unk02 = v31;

                            U32 fogOffset = ((S32)fogPixelHighByte + 0x7F) << 5;

                            const U32 mask = g_rendererState.fogRenderParams.actualRgbMask;
                            U32 j = g_rendererState.fogRenderParams.blocksCount;
                            ++g_rendererState.fogRenderParams.fogPtr;

                            while (true)
                            {
                                g_rendererState.fogBlockParams2.unk04 = 0;
                                do {
                                    U8 k = 0x10;
                                    S32 v39 = fogOffset;
                                    do {
                                        while ((U8)(fogOffset >> 8) != 0x20)
                                        {
                                            const U32 srcPixel = (*src & 0xFFFF) | (*src << 16);
                                            const U32 v35 = g_rendererState.fogBlockParams2.unk04 + (mask & srcPixel) * (U8)(fogOffset >> 8);
                                            fogOffset += g_rendererState.fogBlockParams2.unk01;

                                            const U32 v37 = mask & (v35 >> 5);
                                            g_rendererState.fogBlockParams2.unk04 = mask & v35;

                                            v31 = v37 >> 16;
                                            *(Pixel*)dst = (Pixel)(v31 | v37);

                                            dst = (DoublePixel*)((Addr)dst + sizeof(Pixel));
                                            src = (DoublePixel*)((Addr)src + sizeof(Pixel));
                                            --k;
                                            if (k == 0)
                                                break;
                                        }
                                        if (k == 0)
                                            break;

                                        fogOffset += g_rendererState.fogBlockParams2.unk01;
                                        *(Pixel*)dst = (Pixel)v31;

                                        dst = (DoublePixel*)((Addr)dst + sizeof(Pixel));
                                        src = (DoublePixel*)((Addr)src + sizeof(Pixel));
                                        --k;
                                    } while (k);

                                    fogOffset = g_rendererState.fogBlockParams2.unk02 + v39;
                                    src = (DoublePixel*)((Addr)src + screenWidthInBytes - blockSize);
                                    dst = (DoublePixel*)((Addr)dst + g_moduleState->pitch - blockSize);
                                    g_rendererState.fogBlockParams2.unk01 += g_rendererState.fogBlockParams.unk01;

                                    --j;
                                } while (j & 0xFF);
                                j >>= 8;
                                if (j == 0)
                                    break;
                                src = (DoublePixel*)((Addr)src - screenSizeInBytes);
                            }
                        }
                    }
                    else
                    {
                        U32 j = g_rendererState.fogRenderParams.blocksCount;
                        ++g_rendererState.fogRenderParams.fogPtr;
                        const DoublePixel mask = *(DoublePixel*)&g_moduleState->invActualColorBits;
                        while (true)
                        {
                            do
                            {
                                dst[0] = mask & (src[0] >> 1);
                                dst[1] = mask & (src[1] >> 1);
                                dst[2] = mask & (src[2] >> 1);
                                dst[3] = mask & (src[3] >> 1);
                                dst[4] = mask & (src[4] >> 1);
                                dst[5] = mask & (src[5] >> 1);
                                dst[6] = mask & (src[6] >> 1);
                                dst[7] = mask & (src[7] >> 1);
                                src = (DoublePixel*)((Addr)src + screenWidthInBytes);
                                dst = (DoublePixel*)((Addr)dst + (Addr)g_moduleState->pitch);

                                --j;
                            } while (j & 0xFF);
                            j >>= 8;
                            if (j == 0)
                                break;
                            src = (DoublePixel*)((Addr)src - screenSizeInBytes);
                        }
                    }

                    src = (DoublePixel*)((Addr)src + (Addr)g_rendererState.fogRenderParams.lineStep);
                    dst = (DoublePixel*)((Addr)dst + (Addr)g_rendererState.fogRenderParams.dstRowStride);
                    --g_rendererState.fogBlockParams.unk04;
                } while (g_rendererState.fogBlockParams.unk04);

                src = (DoublePixel*)((Addr)srcTemp + screenWidthInBytes * 8);
                dst = (DoublePixel*)((Addr)dstTemp + (Addr)g_moduleState->pitch * 8);
                --g_rendererState.fogBlockParams.validRowsBlockCount;
            } while (g_rendererState.fogBlockParams.validRowsBlockCount);

            if ((g_rendererState.fogBlockParams.tempBlocksCount & 0xFF) == 0)
                break;

            g_rendererState.fogRenderParams.blocksCount = g_rendererState.fogBlockParams.tempBlocksCount;
            g_rendererState.fogRenderParams.lineStep = screenSizeInBytes - screenWidthInBytes * 8 + blockSize;
            g_rendererState.fogBlockParams.validRowsBlockCount = 1;
            g_rendererState.fogBlockParams.tempBlocksCount = 0;
        }

        g_rendererState.fogRenderParams.lineStep = -(S32)screenWidthInBytes * 8 + blockSize;
        remainingExcessRows = g_rendererState.fogBlockParams2.excessRowsBlockCount;
        g_rendererState.fogBlockParams.validRowsBlockCount = remainingExcessRows;
        g_rendererState.fogBlockParams2.excessRowsBlockCount = 0;
        g_rendererState.fogRenderParams.blocksCount = blocksNumber;
        src = (DoublePixel*)((Addr)src - screenSizeInBytes);
    } while (remainingExcessRows);

    if (locked)
    {
        unlockDxSurface();
    }
}

// 0x10002fb0
void blendMainSurfaceWithWarFog(const S32 x, const S32 y, const S32 endX, const S32 endY)
{
    const Addr screenSizeInBytes = Screen::sizeInBytes_;
    const Addr screenWidthInBytes = Screen::widthInBytes_;

    constexpr S32 blockSize = 8 * sizeof(DoublePixel);  // 32 bytes
    constexpr U32 blocksNumber = 8;

    S32 delta = (endY + 1) - y;
    g_rendererState.fogBlockParams2.unk04 = 0;
    g_rendererState.fogRenderParams.actualRgbMask = g_moduleState->initialRgbMask;
    g_rendererState.fogRenderParams.dstRowStride = -8 * g_moduleState->pitch + blockSize;
    g_rendererState.fogRenderParams.lineStep = -(S32)Screen::width_ * 16 + blockSize;

    U8* fogSrc = &g_moduleState->fogSprites[(y >> 3) + 8].unk[(x >> 4) + 8];
    DoublePixel* src = (DoublePixel*)((Addr)g_rendererState.surfaces.main + g_moduleState->surface.offset + (Screen::width_ * y + x) * sizeof(Pixel));

    if (y >= g_moduleState->surface.y)
        src = (DoublePixel*)((Addr)src - screenSizeInBytes);

    if (y >= g_moduleState->surface.y || y + delta <= g_moduleState->surface.y)
    {
        g_rendererState.fogBlockParams.validRowsBlockCount = delta >> 3;
        g_rendererState.fogBlockParams.tempBlocksCount = 0;
        g_rendererState.fogBlockParams2.excessRowsBlockCount = 0;

        g_rendererState.fogRenderParams.blocksCount = blocksNumber;
    }
    else
    {
        g_rendererState.fogBlockParams.validRowsBlockCount = (g_moduleState->surface.y - y) >> 3;
        g_rendererState.fogBlockParams2.excessRowsBlockCount = (delta + y - g_moduleState->surface.y) >> 3;
        U32 v7 = ((U8)g_moduleState->surface.y - (U8)y) & 7;
        v7 = v7 | ((8 - v7) << 8);      // v7 now is 0000 ' 0000 ' 8 - v7 ' v7, so the summ of all its bytes is always 8    
        g_rendererState.fogBlockParams.tempBlocksCount = v7;

        if (g_rendererState.fogBlockParams.validRowsBlockCount == 0)
        {
            g_rendererState.fogRenderParams.lineStep += screenSizeInBytes;
            g_rendererState.fogRenderParams.blocksCount = v7;
            g_rendererState.fogBlockParams.validRowsBlockCount = 1;
            g_rendererState.fogBlockParams.tempBlocksCount = 0;
        }
        else
            g_rendererState.fogRenderParams.blocksCount = blocksNumber;
    }

    S32 remainingExcessRows;
    DoublePixel* srcTemp;
    do
    {
        while (true)
        {
            do {
                srcTemp = src;

                U8* someFog = fogSrc;
                fogSrc = someFog + sizeof(Fog);

                g_rendererState.fogRenderParams.fogPtr = someFog;
                g_rendererState.fogBlockParams.unk04 = ((endX + 1) - x) >> 4;

                do {
                    DoublePixel fogPixel = *(DoublePixel*)((Addr)g_rendererState.fogRenderParams.fogPtr - 2);
                    fogPixel = (fogPixel & 0xFFFF'0000) | (*(Pixel*)((Addr)g_rendererState.fogRenderParams.fogPtr + sizeof(Fog)));

                    if (fogPixel)
                    {
                        if (fogPixel == 0x80808080)
                        {
                            ++g_rendererState.fogRenderParams.fogPtr;
                            src = (DoublePixel*)((Addr)src + blockSize);
                        }
                        else
                        {
                            // Break fogPixel into bytes
                            U8 fogPixelLowByte = fogPixel & 0xFF;          // Byte 0
                            U8 fogValueMidByte = (fogPixel >> 8) & 0xFF;   // Byte 1
                            U8 fogPixelHighByte = (fogPixel >> 16) & 0xFF; // Byte 2
                            U8 fogPixelTopByte = (fogPixel >> 24) & 0xFF;  // Byte 3

                            // Step 1: fogValueMidByte -= fogPixelTopByte, track borrow
                            bool borrow1 = fogValueMidByte < fogPixelTopByte;
                            fogValueMidByte -= fogPixelTopByte;

                            // Step 2: fogValueMidByte -= fogPixelLowByte, track another borrow
                            bool borrow2 = fogValueMidByte < fogPixelLowByte;
                            fogValueMidByte -= fogPixelLowByte;

                            // Step 3: Compute carry of (fogPixelHighByte + fogValueMidByte)
                            bool carry = ((uint16_t)fogPixelHighByte + (uint16_t)fogValueMidByte) > 0xFF;
                            fogValueMidByte += fogPixelHighByte;

                            // Final v27 computation
                            S32 v27 = carry + (-(S32)borrow1) - (S32)borrow2;
                            v27 = (v27 & 0xFFFF'FF00) | fogValueMidByte;
                            g_rendererState.fogBlockParams.unk01 = v27 >> 2;

                            // Compute v28 (difference between fogPixelTopByte and fogPixelHighByte)
                            S32 v28 = -(S32)(fogPixelTopByte < fogPixelHighByte);
                            fogPixelTopByte -= fogPixelHighByte;
                            v28 = (v28 & 0xFFFF'FF00) | fogPixelTopByte;
                            g_rendererState.fogBlockParams2.unk01 = 2 * v28;

                            // Compute v30 (difference between fogPixelLowByte and fogPixelLow)
                            S32 v30 = -(S32)(fogPixelLowByte < fogPixelHighByte);
                            fogPixelLowByte -= fogPixelHighByte;
                            v30 = (v30 & 0xFFFF'FF00) | fogPixelLowByte;

                            const S32 v31 = 4 * v30;
                            g_rendererState.fogBlockParams2.unk02 = v31;

                            U32 fogOffset = ((S32)fogPixelHighByte + 0x7F) << 5;

                            const U32 mask = g_rendererState.fogRenderParams.actualRgbMask;
                            U32 j = g_rendererState.fogRenderParams.blocksCount;
                            ++g_rendererState.fogRenderParams.fogPtr;

                            while (true)
                            {
                                g_rendererState.fogBlockParams2.unk04 = 0;
                                do {
                                    U8 k = 0x10;
                                    S32 v39 = fogOffset;
                                    do {
                                        while ((U8)(fogOffset >> 8) != 0x20)
                                        {
                                            const U32 srcPixel = (*src & 0xFFFF) | (*src << 16);
                                            const U32 v35 = g_rendererState.fogBlockParams2.unk04 + (mask & srcPixel) * (U8)(fogOffset >> 8);
                                            fogOffset += g_rendererState.fogBlockParams2.unk01;

                                            const U32 v37 = mask & (v35 >> 5);
                                            g_rendererState.fogBlockParams2.unk04 = mask & v35;

                                            *(Pixel*)src = (Pixel)((v37 >> 16) | v37);

                                            src = (DoublePixel*)((Addr)src + sizeof(Pixel));
                                            --k;
                                            if (k == 0)
                                                break;
                                        }
                                        if (k == 0)
                                            break;

                                        fogOffset += g_rendererState.fogBlockParams2.unk01;

                                        src = (DoublePixel*)((Addr)src + sizeof(Pixel));
                                        --k;
                                    } while (k);

                                    fogOffset = g_rendererState.fogBlockParams2.unk02 + v39;
                                    src = (DoublePixel*)((Addr)src + screenWidthInBytes - blockSize);
                                    g_rendererState.fogBlockParams2.unk01 += g_rendererState.fogBlockParams.unk01;

                                    --j;
                                } while (j & 0xFF);
                                j >>= 8;
                                if (j == 0)
                                    break;
                                src = (DoublePixel*)((Addr)src - screenSizeInBytes);
                            }
                            src = (DoublePixel*)((Addr)src + (Addr)g_rendererState.fogRenderParams.lineStep);
                        }
                    }
                    else
                    {
                        U32 j = g_rendererState.fogRenderParams.blocksCount;
                        ++g_rendererState.fogRenderParams.fogPtr;
                        const DoublePixel mask = (*(DoublePixel*)&g_moduleState->invActualColorBits) & 0x7FFF7FFF;
                        while (true)
                        {
                            do
                            {
                                src[0] = mask & (src[0] >> 1);
                                src[1] = mask & (src[1] >> 1);
                                src[2] = mask & (src[2] >> 1);
                                src[3] = mask & (src[3] >> 1);
                                src[4] = mask & (src[4] >> 1);
                                src[5] = mask & (src[5] >> 1);
                                src[6] = mask & (src[6] >> 1);
                                src[7] = mask & (src[7] >> 1);
                                src = (DoublePixel*)((Addr)src + screenWidthInBytes);

                                --j;
                            } while (j & 0xFF);
                            j >>= 8;
                            if (j == 0)
                                break;
                            src = (DoublePixel*)((Addr)src - screenSizeInBytes);
                        }
                        src = (DoublePixel*)((Addr)src + (Addr)g_rendererState.fogRenderParams.lineStep);
                    }

                    --g_rendererState.fogBlockParams.unk04;
                } while (g_rendererState.fogBlockParams.unk04);

                src = (DoublePixel*)((Addr)srcTemp + screenWidthInBytes * 8);
                --g_rendererState.fogBlockParams.validRowsBlockCount;
            } while (g_rendererState.fogBlockParams.validRowsBlockCount);

            if ((g_rendererState.fogBlockParams.tempBlocksCount & 0xFF) == 0)
                break;

            g_rendererState.fogRenderParams.blocksCount = g_rendererState.fogBlockParams.tempBlocksCount;
            g_rendererState.fogRenderParams.lineStep = screenSizeInBytes - screenWidthInBytes * 8 + blockSize;
            g_rendererState.fogBlockParams.validRowsBlockCount = 1;
            g_rendererState.fogBlockParams.tempBlocksCount = 0;
        }

        g_rendererState.fogRenderParams.lineStep = -(S32)screenWidthInBytes * 8 + blockSize;
        remainingExcessRows = g_rendererState.fogBlockParams2.excessRowsBlockCount;
        g_rendererState.fogBlockParams.validRowsBlockCount = remainingExcessRows;
        g_rendererState.fogBlockParams2.excessRowsBlockCount = 0;
        g_rendererState.fogRenderParams.blocksCount = blocksNumber;
        src = (DoublePixel*)((Addr)src - screenSizeInBytes);
    } while (remainingExcessRows);
}


// 0x10003320
S32 getTextLength(const char* const str, const AssetCollection* const asset)
{
    S32 result = 0;

    for (const char* s = str; *s; ++s)
    {
        const ImagePaletteSprite* image = (ImagePaletteSprite*)((Addr)asset + (Addr)asset->items[*s]);

        result += DEFAULT_FONT_ASSET_SPACING + image->width;
    }

    return result;
}

// 0x10003360
void drawMainSurfaceText(const S32 x, const S32 y, const char* const str, const AssetCollection* const asset, const Pixel* const palette)
{
    U32 offset = 0;

    for (const char* s = str; *s; ++s)
    {
        ImagePaletteSprite* image = (ImagePaletteSprite*)((Addr)asset + (Addr)asset->items[*s]);

        drawMainSurfacePaletteSpriteCompact(x + offset, y, palette, image);

        offset += DEFAULT_FONT_ASSET_SPACING + image->width;
    }
}

// 0x100033c0
void drawBackSurfaceText(const S32 x, const S32 y, const char* const str, const AssetCollection* const asset, const Pixel* const palette)
{
    U32 offset = 0;

    for (const char* s = str; *s; ++s)
    {
        ImagePaletteSprite* image = (ImagePaletteSprite*)((Addr)asset + (Addr)asset->items[*s]);

        drawBackSurfacePaletteShadedSprite(x + offset, y, (U16)y, palette, image);

        offset += DEFAULT_FONT_ASSET_SPACING + image->width;
    }
}


// 0x10003420
void drawSurfacePaletteRhomb(const S32 angle_0, const S32 angle_1, const S32 angle_2, const S32 angle_3, S32 tx, S32 ty, const S32 stride, const ImagePaletteTile* const tile, Pixel* const output)
{
    // Tile height: 32
    // Tile width: 63

    const Addr screenSizeInBytes = Screen::sizeInBytes_;

    g_rendererState.tile.stencil = (Pixel*)((Addr)output + g_moduleState->surface.offset % Screen::widthInBytes_ + screenSizeInBytes);
    g_rendererState.tile.rect.x = g_moduleState->windowRect.x + TILE_SIZE_HEIGHT + 1;
    g_rendererState.tile.rect.y = g_moduleState->windowRect.y;
    g_rendererState.tile.rect.width = g_moduleState->windowRect.width + TILE_SIZE_HEIGHT + 1;
    g_rendererState.tile.rect.height = g_moduleState->windowRect.height;
    g_rendererState.tile.displayedHalfs = 0;

    // Проверка видимости тайла
    if (tx > g_rendererState.tile.rect.width + 1
        || tx < g_rendererState.tile.rect.x - TILE_SIZE_WIDTH - 1
        || ty > g_rendererState.tile.rect.height + 1
        || ty < g_rendererState.tile.rect.y - TILE_SIZE_HEIGHT)
        return;

    S32 tileStartDrawLength;

    // Настройка рендеринга
    const U8* srcInput = tile->pixels;
    Pixel* dst = (Pixel*)((Addr)output + g_moduleState->surface.offset + stride * ty + tx * sizeof(Pixel) - 2);
    Pixel* dst2;
    S32 txDelta = tx + TILE_SIZE_HEIGHT;
    S32 diff = (angle_1 - angle_0) << 2;
    bool isUpperPart = (ty > (g_moduleState->windowRect.y - 16)) ? true : false; // 1 остальное 0 четверь

    if (!isUpperPart)
    {
        // Инициализация параметров рендеринга нижней четверти
        g_rendererState.tile.diff = (angle_3 - angle_0) << 4;
        txDelta = (angle_0 << 8) + g_rendererState.tile.diff;
        dst2 = (Pixel*)((Addr)dst + (Addr)(16 * stride - 29 * sizeof(Pixel)));
        tx += 3; // Шаг между усеченными пирамидками допустим 32+3

        tileStartDrawLength = 61; // Стартовая длина строки нижней половины тайла
        srcInput += 528; // Пропускаем верхнюю половину тайла для отображения нижней половины тайла
        ty += 16; // -24+16 скорей всего высота которая вне экрана -8, возможно удаляем верхнюю половину тайла

        // Вычисление высоты
        g_rendererState.tile.height = std::min((g_moduleState->windowRect.height + 1) - ty, 16);

        // Если тайл по высоте торчит за пределы экрана, то уменьшаем отображаемую высоту тайла
        const S32 overage = g_moduleState->windowRect.y - ty;
        if (overage >= 0)
        {
            ty = g_rendererState.tile.rect.y;
            g_rendererState.tile.height -= overage;

            for (S32 y = 0; y < overage; ++y)
            {
                srcInput += tileStartDrawLength;
                txDelta += g_rendererState.tile.diff;
                tileStartDrawLength -= 4; // Для ступечатой отрисовки каждай последующая отрисовка пирамидки -4 от ее ширины
                dst2 = (Pixel*)((Addr)dst2 + (Addr)(stride + 2 * sizeof(Pixel)));   // наверно 4 это 2 пикселя + мешене для ступечатых операций
                tx += 2;
            }
        }
    }
    else
    {
        tileStartDrawLength = 3;   // Стартовая длина строки верхней половины тайла
        tx = txDelta;

        // Инициализация параметров рендеринга
        g_rendererState.tile.diff = (angle_0 - angle_2) << 4;
        txDelta = (angle_2 << 8) + g_rendererState.tile.diff + diff;

        g_rendererState.tile.height = std::min((g_moduleState->windowRect.height + 1) - ty, 16);

        S32 overage = g_moduleState->windowRect.y - ty;
        if (overage >= 0)
        {
            ty += overage;
            g_rendererState.tile.height -= overage;

            for (S32 y = 0; y < overage; ++y)
            {
                srcInput += tileStartDrawLength;
                txDelta += g_rendererState.tile.diff;
                tileStartDrawLength += 4;
                dst = (Pixel*)((Addr)dst + (Addr)(stride - 2 * sizeof(Pixel)));
                tx -= 2;
            }
        }

        // Отрисовка части тайла
        if (g_rendererState.tile.height > 0)
        {
            ty += g_rendererState.tile.height;
            S32 overflow = std::max(ty - g_moduleState->surface.y, 0);

            g_rendererState.tile.tempHeight = g_rendererState.tile.height;
            g_rendererState.tile.height -= overflow;

            dst2 = dst;
            if (g_rendererState.tile.height <= 0)
            {
                g_rendererState.tile.height = g_rendererState.tile.tempHeight;
                g_rendererState.tile.tempHeight = 0;
                g_rendererState.tile.displayedHalfs++;

                overflow = 0;

                dst2 = (Pixel*)((Addr)dst2 - screenSizeInBytes);
            }

            while (g_rendererState.tile.height > 0)
            {
                for (S32 yy = 0; yy < g_rendererState.tile.height; ++yy)
                {
                    g_rendererState.tile.tempHeight = overflow;

                    S32 totalTxOffset = txDelta;

                    const S32 delta = (g_rendererState.tile.rect.width + 1) - tx;
                    S32 delta2 = std::min(delta, tileStartDrawLength);
                    const S32 delta3 = g_rendererState.tile.rect.x - tx;
                    if (delta > 0 && delta2 > delta3)
                    {
                        const U8* srcTemp = srcInput;
                        Pixel* dstTemp = dst2;

                        if (delta3 > 0)
                        {
                            srcTemp = (U8*)((Addr)srcTemp + (Addr)delta3);
                            dstTemp = (Pixel*)((Addr)dstTemp + (Addr)delta3 * sizeof(Pixel));

                            delta2 -= delta3;
                            totalTxOffset = txDelta + delta3 * diff;
                        }

                        // Glitch
                        if (g_rendererState.tile.stencil <= dstTemp)
                        {
                            dstTemp = (Pixel*)((Addr)dstTemp - screenSizeInBytes);
                        }
                        if (dstTemp < output)
                        {
                            dstTemp = (Pixel*)((Addr)dstTemp + screenSizeInBytes);
                        }

                        U16 uVar5 = ((U16)((totalTxOffset >> 8) ^ g_rendererState.tile.unk08) << 8) | (totalTxOffset & 0xFF);
                        g_rendererState.tile.unk08 ^= 0x20;

                        for (S32 y = 0; y < delta2; ++y)
                        {
                            dstTemp[y] = g_moduleState->rhombsPalette.palette[(uVar5 & 0xFF00) | srcTemp[y]];
                            uVar5 = uVar5 + (U16)(diff) ^ 0x2000;
                        }
                    }

                    srcInput = (U8*)((Addr)srcInput + tileStartDrawLength);
                    tileStartDrawLength += 4;

                    txDelta += g_rendererState.tile.diff;
                    overflow = g_rendererState.tile.tempHeight;
                    tx -= 2;

                    dst2 = dst = (Pixel*)((Addr)dst2 + (Addr)(stride - 4));
                }

                // Отрисовываем оставшуюся часть высоты тайла
                g_rendererState.tile.height = g_rendererState.tile.tempHeight;
                g_rendererState.tile.tempHeight = 0;
                g_rendererState.tile.displayedHalfs++;

                overflow = 0;

                dst2 = (Pixel*)((Addr)dst - screenSizeInBytes);
            }
        }

        if (ty > g_rendererState.tile.rect.height + 1)
            return;

        g_rendererState.tile.unk08 ^= 0x20;
        tileStartDrawLength -= 6;                                   // Сдвигаем нюжнюю часть на 3 пикселя
        dst2 = (Pixel*)((Addr)dst + (Addr)(3 * sizeof(Pixel)));     // Офссет нижней части ромбика
        tx += 3;

        g_rendererState.tile.height = std::min((g_rendererState.tile.rect.height + 1) - ty, 16);
        g_rendererState.tile.diff = (angle_3 - angle_0) << 4;
        txDelta = (angle_0 << 8) + g_rendererState.tile.diff;
    }

    // Рендеринг нижней части
    if (g_rendererState.tile.height > 0)
    {
        S32 overflow = g_rendererState.tile.tempHeight;

        if (g_rendererState.tile.displayedHalfs < 2)
        {
            overflow = std::max(g_rendererState.tile.height + ty - g_moduleState->surface.y, 0);

            g_rendererState.tile.tempHeight = g_rendererState.tile.height;
            g_rendererState.tile.height -= overflow;

            if (g_rendererState.tile.height <= 0)
            {
                g_rendererState.tile.height = g_rendererState.tile.tempHeight;
                g_rendererState.tile.tempHeight = 0;

                overflow = g_rendererState.tile.tempHeight;

                dst2 = (Pixel*)((Addr)dst2 - screenSizeInBytes);
            }
        }

        while (g_rendererState.tile.height > 0)
        {
            for (U16 yy = 0; yy < g_rendererState.tile.height; ++yy)
            {
                g_rendererState.tile.tempHeight = overflow;

                S32 totalTxOffset = txDelta;

                S32 delta = (g_rendererState.tile.rect.width + 1) - tx;
                S32 delta2 = std::min(delta, tileStartDrawLength);
                const S32 delta3 = g_rendererState.tile.rect.x - tx;
                if (delta > 0 && delta2 > delta3)
                {
                    const U8* srcTemp = srcInput;
                    Pixel* dstTemp = dst2;
                    if (delta3 > 0)
                    {
                        srcTemp = (U8*)((Addr)srcTemp + delta3);
                        dstTemp = (Pixel*)((Addr)dstTemp + (Addr)(delta3 * sizeof(Pixel)));

                        delta2 -= delta3;
                        totalTxOffset = txDelta + delta3 * diff;
                    }

                    U16 uVar5 = ((U16)((totalTxOffset >> 8) ^ g_rendererState.tile.unk08) << 8) | (totalTxOffset & 0xFF);
                    g_rendererState.tile.unk08 ^= 0x20;

                    for (S32 y = 0; y < delta2; y++)
                    {
                        dstTemp[y] = g_moduleState->rhombsPalette.palette[(uVar5 & 0xFF00) | srcTemp[y]];
                        uVar5 = uVar5 + (U16)(diff) ^ 0x2000;
                    }

                }

                srcInput = (U8*)((Addr)srcInput + tileStartDrawLength);
                tileStartDrawLength -= 4;

                txDelta += g_rendererState.tile.diff;
                overflow = g_rendererState.tile.tempHeight;
                tx += 2;

                dst2 = (Pixel*)((Addr)dst2 + (Addr)(stride + 2 * sizeof(Pixel)));
            }

            g_rendererState.tile.height = g_rendererState.tile.tempHeight;
            g_rendererState.tile.tempHeight = 0;

            overflow = g_rendererState.tile.tempHeight;

            dst2 = (Pixel*)((Addr)dst2 - screenSizeInBytes);
        };
    }
}

// 0x1000381e
void shadeSurfaceRhomb(const S32 angle_0, const S32 angle_1, const S32 angle_2, const S32 angle_3, S32 tx, S32 ty, const S32 stride, Pixel* const output)
{
    const Addr screenSizeInBytes = Screen::sizeInBytes_;

    const U32 colorMask = ((U32)g_moduleState->actualGreenMask << 16) | g_moduleState->actualBlueMask | g_moduleState->actualRedMask;
    g_rendererState.tile.stencil = (Pixel*)((Addr)output + g_moduleState->surface.offset % Screen::widthInBytes_ + screenSizeInBytes);
    g_rendererState.tile.rect.x = g_moduleState->windowRect.x + TILE_SIZE_HEIGHT + 1;
    g_rendererState.tile.rect.y = g_moduleState->windowRect.y;
    g_rendererState.tile.rect.width = g_moduleState->windowRect.width + TILE_SIZE_HEIGHT + 1;
    g_rendererState.tile.rect.height = g_moduleState->windowRect.height;
    g_rendererState.tile.displayedHalfs = 0;

    if (tx > g_rendererState.tile.rect.width + 1
        || tx < g_rendererState.tile.rect.x - TILE_SIZE_WIDTH - 1
        || ty > g_rendererState.tile.rect.height + 1
        || ty < g_rendererState.tile.rect.y - TILE_SIZE_HEIGHT)
        return;

    S32 tileStartDrawLength;

    Pixel* dst = (Pixel*)((Addr)output + g_moduleState->surface.offset + stride * ty + tx * sizeof(Pixel) - 2);
    Pixel* dst2;
    S32 txDelta = tx + TILE_SIZE_HEIGHT;
    S32 diff = (angle_1 - angle_0) << 2;

    if (ty <= g_moduleState->windowRect.y - 16)
    {
        g_rendererState.tile.diff = (angle_3 - angle_0) << 4;
        txDelta = (angle_0 << 8) + g_rendererState.tile.diff;
        dst2 = (Pixel*)((Addr)dst + (Addr)(16 * stride - 29 * sizeof(Pixel)));
        tx += 3;

        tileStartDrawLength = 61;
        ty += 16;

        // Вычисление высоты
        g_rendererState.tile.height = std::min((g_moduleState->windowRect.height + 1) - ty, 16);

        // Если тайл по высоте торчит за пределы экрана, то уменьшаем отображаемую высоту тайла
        const S32 overage = g_moduleState->windowRect.y - ty;
        if (overage >= 0)
        {
            ty = g_rendererState.tile.rect.y;
            g_rendererState.tile.height -= overage;

            for (S32 y = 0; y < overage; ++y)
            {
                txDelta += g_rendererState.tile.diff;
                tileStartDrawLength -= 4; // Для ступечатой отрисовки каждай последующая отрисовка пирамидки -4 от ее ширины
                dst2 = (Pixel*)((Addr)dst2 + (Addr)(stride + 2 * sizeof(Pixel)));   // наверно 4 это 2 пикселя + мешене для ступечатых операций
                tx += 2;
            }
        }
    }
    else
    {
        tileStartDrawLength = 3;   // Стартовая длина строки верхней половины тайла
        tx = txDelta;

        // Инициализация параметров рендеринга
        g_rendererState.tile.diff = (angle_0 - angle_2) << 4;
        txDelta = (angle_2 << 8) + g_rendererState.tile.diff + diff;

        g_rendererState.tile.height = std::min((g_moduleState->windowRect.height + 1) - ty, 16);

        S32 overage = g_moduleState->windowRect.y - ty;
        if (overage >= 0)
        {
            ty += overage;
            g_rendererState.tile.height -= overage;

            for (S32 y = 0; y < overage; ++y)
            {
                txDelta += g_rendererState.tile.diff;
                tileStartDrawLength += 4;
                dst = (Pixel*)((Addr)dst + (Addr)(stride - 2 * sizeof(Pixel)));
                tx -= 2;
            }
        }

        // Отрисовка части тайла
        if (g_rendererState.tile.height > 0)
        {
            ty += g_rendererState.tile.height;
            S32 overflow = std::max(ty - g_moduleState->surface.y, 0);

            g_rendererState.tile.tempHeight = g_rendererState.tile.height;
            g_rendererState.tile.height -= overflow;

            dst2 = dst;
            if (g_rendererState.tile.height <= 0)
            {
                g_rendererState.tile.height = g_rendererState.tile.tempHeight;
                g_rendererState.tile.tempHeight = 0;
                g_rendererState.tile.displayedHalfs++;

                overflow = 0;

                dst2 = (Pixel*)((Addr)dst2 - screenSizeInBytes);
            }

            while (g_rendererState.tile.height > 0)
            {
                for (S32 yy = 0; yy < g_rendererState.tile.height; ++yy)
                {
                    g_rendererState.tile.tempHeight = overflow;

                    S32 totalTxOffset = txDelta;

                    const S32 delta = (g_rendererState.tile.rect.width + 1) - tx;
                    S32 delta2 = std::min(delta, tileStartDrawLength);
                    const S32 delta3 = g_rendererState.tile.rect.x - tx;
                    if (delta > 0 && delta2 > delta3)
                    {
                        Pixel* dstTemp = dst2;

                        if (delta3 > 0)
                        {
                            dstTemp = (Pixel*)((Addr)dstTemp + (Addr)delta3 * sizeof(Pixel));

                            delta2 -= delta3;
                            totalTxOffset = txDelta + delta3 * diff;
                        }

                        // Glitch
                        if (g_rendererState.tile.stencil <= dstTemp)
                        {
                            dstTemp = (Pixel*)((Addr)dstTemp - screenSizeInBytes);
                        }
                        if (dstTemp < output)
                        {
                            dstTemp = (Pixel*)((Addr)dstTemp + screenSizeInBytes);
                        }

                        for (S32 j = 0; j < delta2; ++j)
                        {
                            const U8 byte1 = (U8)((Addr)txDelta >> 8);
                            if (byte1 >= 0x20)
                                continue;

                            Pixel res = (Pixel)byte1;
                            if (byte1)
                            {
                                const DoublePixel val = (dstTemp[j] << 16) | dstTemp[j];
                                const DoublePixel mask = colorMask & (((colorMask & val) * byte1) >> 5);
                                res = (Pixel)((mask >> 16) | mask);
                            }
                            dstTemp[j] = res;
                        }
                    }

                    tileStartDrawLength += 4;

                    txDelta += g_rendererState.tile.diff;
                    overflow = g_rendererState.tile.tempHeight;
                    tx -= 2;

                    dst2 = dst = (Pixel*)((Addr)dst2 + (Addr)(stride - 4));
                }

                // Отрисовываем оставшуюся часть высоты тайла
                g_rendererState.tile.height = g_rendererState.tile.tempHeight;
                g_rendererState.tile.tempHeight = 0;
                g_rendererState.tile.displayedHalfs++;

                overflow = 0;

                dst2 = (Pixel*)((Addr)dst - screenSizeInBytes);
            }
        }

        if (ty > g_rendererState.tile.rect.height + 1)
            return;

        tileStartDrawLength -= 6;                                   // Сдвигаем нюжнюю часть на 3 пикселя
        dst2 = (Pixel*)((Addr)dst + (Addr)(3 * sizeof(Pixel)));     // Офссет нижней части ромбика
        tx += 3;

        g_rendererState.tile.height = std::min((g_rendererState.tile.rect.height + 1) - ty, 16);
        g_rendererState.tile.diff = (angle_3 - angle_0) << 4;
        txDelta = (angle_0 << 8) + g_rendererState.tile.diff;
    }

    // Рендеринг нижней части
    if (g_rendererState.tile.height > 0)
    {
        S32 overflow = g_rendererState.tile.tempHeight;

        if (g_rendererState.tile.displayedHalfs < 2)
        {
            overflow = std::max(g_rendererState.tile.height + ty - g_moduleState->surface.y, 0);

            g_rendererState.tile.tempHeight = g_rendererState.tile.height;
            g_rendererState.tile.height -= overflow;

            if (g_rendererState.tile.height <= 0)
            {
                g_rendererState.tile.height = g_rendererState.tile.tempHeight;
                g_rendererState.tile.tempHeight = 0;

                overflow = g_rendererState.tile.tempHeight;

                dst2 = (Pixel*)((Addr)dst2 - screenSizeInBytes);
            }
        }

        while (g_rendererState.tile.height > 0)
        {
            for (U16 yy = 0; yy < g_rendererState.tile.height; ++yy)
            {
                g_rendererState.tile.tempHeight = overflow;

                S32 totalTxOffset = txDelta;

                S32 delta = (g_rendererState.tile.rect.width + 1) - tx;
                S32 delta2 = std::min(delta, tileStartDrawLength);
                const S32 delta3 = g_rendererState.tile.rect.x - tx;
                if (delta > 0 && delta2 > delta3)
                {
                    Pixel* dstTemp = dst2;
                    if (delta3 > 0)
                    {
                        dstTemp = (Pixel*)((Addr)dstTemp + (Addr)(delta3 * sizeof(Pixel)));

                        delta2 -= delta3;
                        totalTxOffset = txDelta + delta3 * diff;
                    }

                    for (S32 j = 0; j < delta2; j++)
                    {
                        const U8 byte1 = (U8)((Addr)txDelta >> 8);
                        if (byte1 >= 0x20)
                            continue;

                        Pixel res = (Pixel)byte1;
                        if (byte1)
                        {
                            const DoublePixel val = (dstTemp[j] << 16) | dstTemp[j];
                            const DoublePixel mask = colorMask & (((colorMask & val) * byte1) >> 5);
                            res = (Pixel)((mask >> 16) | mask);
                        }
                        dstTemp[j] = res;
                    }

                }

                tileStartDrawLength -= 4;

                txDelta += g_rendererState.tile.diff;
                overflow = g_rendererState.tile.tempHeight;
                tx += 2;

                dst2 = (Pixel*)((Addr)dst2 + (Addr)(stride + 2 * sizeof(Pixel)));
            }

            g_rendererState.tile.height = g_rendererState.tile.tempHeight;
            g_rendererState.tile.tempHeight = 0;

            overflow = g_rendererState.tile.tempHeight;

            dst2 = (Pixel*)((Addr)dst2 - screenSizeInBytes);
        };
    }
}

// 0x10003C48
void cleanSurfaceRhomb(const S32 angle_0, const S32 angle_1, const S32 angle_2, const S32 angle_3, S32 tx, S32 ty, const S32 stride, const ImagePaletteTile* const tile, Pixel* const output)
{
    const Addr screenSizeInBytes = Screen::sizeInBytes_;

    g_rendererState.tile.stencil = (Pixel*)((Addr)output + g_moduleState->surface.offset % Screen::widthInBytes_ + screenSizeInBytes);
    g_rendererState.tile.rect.x = g_moduleState->windowRect.x + TILE_SIZE_HEIGHT + 1;
    g_rendererState.tile.rect.y = g_moduleState->windowRect.y;
    g_rendererState.tile.rect.width = g_moduleState->windowRect.width + TILE_SIZE_HEIGHT + 1;
    g_rendererState.tile.rect.height = g_moduleState->windowRect.height;
    g_rendererState.tile.displayedHalfs = 0;

    if (tx > g_rendererState.tile.rect.width + 1
        || tx < g_rendererState.tile.rect.x - TILE_SIZE_WIDTH - 1
        || ty > g_rendererState.tile.rect.height + 1
        || ty < g_rendererState.tile.rect.y - TILE_SIZE_HEIGHT)
        return;

    S32 tileStartDrawLength;

    const U8* srcInput = tile->pixels;
    Pixel* dst = (Pixel*)((Addr)output + g_moduleState->surface.offset + stride * ty + tx * sizeof(Pixel) - 2);
    Pixel* dst2;

    if (ty <= g_moduleState->windowRect.y - 16)
    {
        tileStartDrawLength = 61;
        tx += 3;

        dst2 = (Pixel*)((Addr)dst + (Addr)(16 * stride - 29 * sizeof(Pixel)));
        srcInput += 528;
        ty += 16;

        g_rendererState.tile.height = std::min((g_moduleState->windowRect.height + 1) - ty, 16);

        const S32 overage = g_moduleState->windowRect.y - ty;
        if (overage >= 0)
        {
            ty = g_rendererState.tile.rect.y;
            g_rendererState.tile.height -= overage;

            for (S32 y = 0; y < overage; ++y)
            {
                srcInput += tileStartDrawLength;
                tileStartDrawLength -= 4;
                dst2 = (Pixel*)((Addr)dst2 + (Addr)(stride + 2 * sizeof(Pixel)));
                tx += 2;
            }
        }
    }
    else
    {
        tileStartDrawLength = 3;
        tx += TILE_SIZE_HEIGHT;

        g_rendererState.tile.height = std::min((g_moduleState->windowRect.height + 1) - ty, 16);

        S32 overage = g_moduleState->windowRect.y - ty;
        if (overage >= 0)
        {
            ty += overage;
            g_rendererState.tile.height -= overage;

            for (S32 y = 0; y < overage; ++y)
            {
                srcInput += tileStartDrawLength;
                tileStartDrawLength += 4;
                dst = (Pixel*)((Addr)dst + (Addr)(stride - 2 * sizeof(Pixel)));
                tx -= 2;
            }
        }

        if (g_rendererState.tile.height > 0)
        {
            ty += g_rendererState.tile.height;
            S32 overflow = std::max(ty - g_moduleState->surface.y, 0);

            g_rendererState.tile.tempHeight = g_rendererState.tile.height;
            g_rendererState.tile.height -= overflow;

            dst2 = dst;
            if (g_rendererState.tile.height <= 0)
            {
                g_rendererState.tile.height = g_rendererState.tile.tempHeight;
                g_rendererState.tile.tempHeight = 0;
                g_rendererState.tile.displayedHalfs++;

                overflow = 0;

                dst2 = (Pixel*)((Addr)dst2 - screenSizeInBytes);
            }

            while (g_rendererState.tile.height > 0)
            {
                for (S32 yy = 0; yy < g_rendererState.tile.height; ++yy)
                {
                    g_rendererState.tile.tempHeight = overflow;

                    const S32 delta = (g_rendererState.tile.rect.width + 1) - tx;
                    S32 delta2 = std::min(delta, tileStartDrawLength);
                    const S32 delta3 = g_rendererState.tile.rect.x - tx;
                    if (delta > 0 && delta2 > delta3)
                    {
                        const U8* srcTemp = srcInput;
                        Pixel* dstTemp = dst2;

                        if (delta3 > 0)
                        {
                            srcTemp = (U8*)((Addr)srcTemp + (Addr)delta3);
                            dstTemp = (Pixel*)((Addr)dstTemp + (Addr)delta3 * sizeof(Pixel));

                            delta2 -= delta3;
                        }

                        if (g_rendererState.tile.stencil <= dstTemp)
                        {
                            dstTemp = (Pixel*)((Addr)dstTemp - screenSizeInBytes);
                        }
                        if (dstTemp < output)
                        {
                            dstTemp = (Pixel*)((Addr)dstTemp + screenSizeInBytes);
                        }

                        for (S32 y = 0; y < delta2; ++y)
                        {
                            if (srcTemp[y])
                                dstTemp[y] = 0;
                        }
                    }

                    srcInput = (U8*)((Addr)srcInput + tileStartDrawLength);
                    tileStartDrawLength += 4;

                    overflow = g_rendererState.tile.tempHeight;
                    tx -= 2;

                    dst2 = dst = (Pixel*)((Addr)dst2 + (Addr)(stride - 4));
                }

                g_rendererState.tile.height = g_rendererState.tile.tempHeight;
                g_rendererState.tile.tempHeight = 0;
                g_rendererState.tile.displayedHalfs++;

                overflow = 0;

                dst2 = (Pixel*)((Addr)dst - screenSizeInBytes);
            }
        }

        if (ty > g_rendererState.tile.rect.height + 1)
            return;

        tileStartDrawLength -= 6;
        dst2 = (Pixel*)((Addr)dst + (Addr)(3 * sizeof(Pixel)));
        tx += 3;

        g_rendererState.tile.height = std::min((g_rendererState.tile.rect.height + 1) - ty, 16);
    }

    if (g_rendererState.tile.height > 0)
    {
        S32 overflow = g_rendererState.tile.tempHeight;

        if (g_rendererState.tile.displayedHalfs < 2)
        {
            overflow = std::max(g_rendererState.tile.height + ty - g_moduleState->surface.y, 0);

            g_rendererState.tile.tempHeight = g_rendererState.tile.height;
            g_rendererState.tile.height -= overflow;

            if (g_rendererState.tile.height <= 0)
            {
                g_rendererState.tile.height = g_rendererState.tile.tempHeight;
                g_rendererState.tile.tempHeight = 0;

                overflow = g_rendererState.tile.tempHeight;

                dst2 = (Pixel*)((Addr)dst2 - screenSizeInBytes);
            }
        }

        while (g_rendererState.tile.height > 0)
        {
            for (U16 yy = 0; yy < g_rendererState.tile.height; ++yy)
            {
                g_rendererState.tile.tempHeight = overflow;

                S32 delta = (g_rendererState.tile.rect.width + 1) - tx;
                S32 delta2 = std::min(delta, tileStartDrawLength);
                const S32 delta3 = g_rendererState.tile.rect.x - tx;
                if (delta > 0 && delta2 > delta3)
                {
                    const U8* srcTemp = srcInput;
                    Pixel* dstTemp = dst2;
                    if (delta3 > 0)
                    {
                        srcTemp = (U8*)((Addr)srcTemp + delta3);
                        dstTemp = (Pixel*)((Addr)dstTemp + (Addr)(delta3 * sizeof(Pixel)));

                        delta2 -= delta3;
                    }

                    for (S32 y = 0; y < delta2; y++)
                    {
                        if (srcTemp[y])
                            dstTemp[y] = 0;
                    }

                }

                srcInput = (U8*)((Addr)srcInput + tileStartDrawLength);
                tileStartDrawLength -= 4;

                overflow = g_rendererState.tile.tempHeight;
                tx += 2;

                dst2 = (Pixel*)((Addr)dst2 + (Addr)(stride + 2 * sizeof(Pixel)));
            }

            g_rendererState.tile.height = g_rendererState.tile.tempHeight;
            g_rendererState.tile.tempHeight = 0;

            overflow = g_rendererState.tile.tempHeight;

            dst2 = (Pixel*)((Addr)dst2 - screenSizeInBytes);
        };
    }
}

// 0x10004016
void drawSurfaceMaskRhomb(S32 x, S32 y, const S32 stride, const S32 mask, Pixel* const surface)
{
    const Addr screenSizeInBytes = Screen::sizeInBytes_;

    g_rendererState.tile.stencil = (Pixel*)((Addr)surface + g_moduleState->surface.offset % Screen::widthInBytes_ + screenSizeInBytes);
    g_rendererState.tile.rect.x = g_moduleState->windowRect.x + TILE_SIZE_HEIGHT + 1;
    g_rendererState.tile.rect.y = g_moduleState->windowRect.y;
    g_rendererState.tile.rect.width = g_moduleState->windowRect.width + TILE_SIZE_HEIGHT + 1;
    g_rendererState.tile.rect.height = g_moduleState->windowRect.height;
    g_rendererState.tile.displayedHalfs = 0;

    if (x > g_rendererState.tile.rect.width + 1
        || x < g_rendererState.tile.rect.x - TILE_SIZE_WIDTH - 1
        || y > g_rendererState.tile.rect.height + 1
        || y < g_rendererState.tile.rect.y - TILE_SIZE_HEIGHT)
        return;

    S32 tileStartDrawLength;
    Pixel* dst = (Pixel*)((Addr)surface + x * sizeof(Pixel) + stride * y + g_moduleState->surface.offset - 2);
    Pixel* dst2;

    if (g_rendererState.tile.rect.y - 16 > y)
    {
        dst2 = dst + 8 * stride - 29;
        x += TILE_SIZE_HEIGHT - 29;
        y += 16;
        tileStartDrawLength = 61;
        g_rendererState.tile.height = std::min((g_moduleState->windowRect.height + 1) - y, 16);

        S32 overage = g_moduleState->windowRect.y - y;
        if (overage > 0)
        {
            g_rendererState.tile.height -= overage;
            y = g_rendererState.tile.rect.y;
            do
            {
                tileStartDrawLength -= 4;
                dst2 = (Pixel*)((Addr)dst2 + stride + 4);
                x += 2;
            } while (--overage);
        }
    }
    else
    {
        x += TILE_SIZE_HEIGHT;
        tileStartDrawLength = 3;
        g_rendererState.tile.height = std::min((g_moduleState->windowRect.height + 1) - y, 16);

        S32 overage = g_rendererState.tile.rect.y - y;
        if (overage > 0)
        {
            g_rendererState.tile.height -= overage;
            y = g_rendererState.tile.rect.y;
            do
            {
                tileStartDrawLength += 4;
                x -= 2;
                dst = (Pixel*)((Addr)dst + (stride - 4));
            } while (--overage);
        }


        if (g_rendererState.tile.height > 0)
        {
            y += g_rendererState.tile.height;
            S32 overflow = std::max(y - g_moduleState->surface.y, 0);

            g_rendererState.tile.tempHeight = g_rendererState.tile.height;
            g_rendererState.tile.height -= overflow;

            dst2 = dst;
            if (g_rendererState.tile.height <= 0)
            {
                g_rendererState.tile.height = g_rendererState.tile.tempHeight;
                g_rendererState.tile.tempHeight = 0;
                g_rendererState.tile.displayedHalfs++;

                overflow = 0;

                dst2 = (Pixel*)((Addr)dst2 - screenSizeInBytes);
            }

            while (g_rendererState.tile.height > 0)
            {
                for (S32 i = 0; i < g_rendererState.tile.height; ++i)
                {
                    g_rendererState.tile.tempHeight = overflow;

                    const S32 delta = (g_rendererState.tile.rect.width + 1) - x;
                    S32 delta2 = std::min(delta, tileStartDrawLength);
                    const S32 delta3 = g_rendererState.tile.rect.x - x;
                    if (delta > 0 && delta2 > delta3)
                    {
                        Pixel* dstTemp = dst2;

                        if (delta3 > 0)
                        {
                            dstTemp += delta3;
                            delta2 -= delta3;
                        }

                        if (dstTemp >= g_rendererState.tile.stencil)
                        {
                            dstTemp = (Pixel*)((Addr)dstTemp - screenSizeInBytes);
                        }
                        if (dstTemp < surface)
                        {
                            dstTemp = (Pixel*)((Addr)dstTemp + screenSizeInBytes);
                        }

                        for (S32 j = 0; j < delta2; ++j)
                        {
                            dstTemp[j] = (Pixel)mask + ((g_moduleState->shadeColorMask & dstTemp[j]) >> 1);
                        }
                    }

                    tileStartDrawLength += 4;

                    overflow = g_rendererState.tile.tempHeight;
                    x -= 2;

                    dst2 = dst = (Pixel*)((Addr)dst2 + (stride - 4));
                }

                g_rendererState.tile.height = g_rendererState.tile.tempHeight;
                g_rendererState.tile.tempHeight = 0;
                g_rendererState.tile.displayedHalfs++;

                overflow = 0;

                dst2 = (Pixel*)((Addr)dst - screenSizeInBytes);
            }
        }

        if (y > g_rendererState.tile.rect.height + 1)
            return;

        tileStartDrawLength -= 3 * sizeof(Pixel);
        dst2 = dst + 3;
        x += 3;

        g_rendererState.tile.height = std::min((g_rendererState.tile.rect.height + 1) - y, 16);
    }

    if (g_rendererState.tile.height > 0)
    {
        S32 overflow = g_rendererState.tile.tempHeight;

        if (g_rendererState.tile.displayedHalfs < 2)
        {
            overflow = std::max(g_rendererState.tile.height + y - g_moduleState->surface.y, 0);

            g_rendererState.tile.tempHeight = g_rendererState.tile.height;
            g_rendererState.tile.height -= overflow;

            if (g_rendererState.tile.height <= 0)
            {
                g_rendererState.tile.height = g_rendererState.tile.tempHeight;
                g_rendererState.tile.tempHeight = 0;

                overflow = g_rendererState.tile.tempHeight;

                dst2 = (Pixel*)((Addr)dst2 - screenSizeInBytes);
            }
        }

        while (g_rendererState.tile.height > 0)
        {
            for (U16 yy = 0; yy < g_rendererState.tile.height; ++yy)
            {
                g_rendererState.tile.tempHeight = overflow;

                S32 delta = (g_rendererState.tile.rect.width + 1) - x;
                S32 delta2 = std::min(delta, tileStartDrawLength);
                const S32 delta3 = g_rendererState.tile.rect.x - x;
                if (delta > 0 && delta2 > delta3)
                {
                    Pixel* dstTemp = dst2;
                    if (delta3 > 0)
                    {
                        dstTemp += delta3;
                        delta2 -= delta3;
                    }

                    for (S32 j = 0; j < delta2; ++j)
                    {
                        dstTemp[j] = (Pixel)mask + ((g_moduleState->shadeColorMask & dstTemp[j]) >> 1);
                    }

                }

                tileStartDrawLength -= 4;

                overflow = g_rendererState.tile.tempHeight;
                x += 2;

                dst2 = (Pixel*)((Addr)dst2 + (Addr)(stride + 2 * sizeof(Pixel)));
            }

            g_rendererState.tile.height = g_rendererState.tile.tempHeight;
            g_rendererState.tile.tempHeight = 0;

            overflow = g_rendererState.tile.tempHeight;

            dst2 = (Pixel*)((Addr)dst2 - screenSizeInBytes);
        };
    }
}


// 0x10004390
void drawBackSurfaceRhombsPaletteSprite(S32 x, S32 y, const ImagePaletteSprite* const sprite)
{
    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.back;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x    = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemShortCountMask;

                    if (count <= need)
                    {
                        if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                        {
                            // Mask 0xC0 -> only count
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        }
                        else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> count and one pixel
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            // Mask 0x40 and 0x00 -> count and few pixels
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemShortCountMask);

                    if (count == 0)
                    {
                        // Count 0 -> no pixels to process
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                    if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                    {
                        // Mask 0xC0 -> skip pixels
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                    {
                        // Mask 0x80 -> repeat one pixel
                        const U8 indx = pixels->pixels[0];
                        const Pixel pixel = g_moduleState->rhombsPalette.palette[indx + 0x4100];

                        std::fill_n(sx, availCount, pixel);

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else if ((pixels->count & kImageSpriteItemShortCompactMask) == kImageSpriteItemShortCompactMask)
                    {
                        // Mask 0x40 -> repeat and blend pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];
                            Pixel pixel = g_moduleState->rhombsPalette.palette[indx + 0x4100];
                            pixel = (sx[i] + pixel - (g_moduleState->actualColorBits & (sx[i] ^ pixel))) >> 1;

                            sx[i] = pixel;
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == 0)
                    {
                        // Mask 0x00 -> draw pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];
                            const Pixel pixel = g_moduleState->rhombsPalette.palette[indx + 0x4100];

                            sx[i] = pixel;
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x100046b6
void drawBackSurfaceRhombsPaletteSprite2(S32 x, S32 y, const ImagePaletteSprite* const sprite)
{
    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.back;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x    = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemShortCountMask;

                    if (count <= need)
                    {
                        if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                        {
                            // Mask 0xC0 -> only count
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        }
                        else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> count and one pixel
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            // Mask 0x40 and 0x00 -> count and few pixels
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemShortCountMask);

                    if (count == 0)
                    {
                        // Count 0 -> no pixels to process
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                    if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                    {
                        // Mask 0xC0 -> skip pixels
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                    {
                        // Mask 0x80 -> repeat one pixel
                        const U8 indx = pixels->pixels[0];
                        const Pixel pixel = g_moduleState->rhombsPalette.palette[indx + 0x4200];

                        std::fill_n(sx, availCount, pixel);

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else if ((pixels->count & kImageSpriteItemShortCompactMask) == kImageSpriteItemShortCompactMask)
                    {
                        // Mask 0x40 -> repeat and blend pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];
                            Pixel pixel = g_moduleState->rhombsPalette.palette[indx + 0x4200];
                            pixel = (sx[i] + pixel - (g_moduleState->actualColorBits & (sx[i] ^ pixel))) >> 1;

                            sx[i] = pixel;
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == 0)
                    {
                        // Mask 0x00 -> draw pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];
                            const Pixel pixel = g_moduleState->rhombsPalette.palette[indx + 0x4200];

                            sx[i] = pixel;
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x100049e6
void drawBackSurfaceRhombsPaletteShadedSprite(S32 x, S32 y, U16 level, const ImagePaletteSprite* const sprite)
{
    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    level = (level + kStencilPixelOffset) << kStencilPixelColorShift;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.back;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x    = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemShortCountMask;

                    if (count <= need)
                    {
                        if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                        {
                            // Mask 0xC0 -> only count
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        }
                        else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> count and one pixel
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            // Mask 0x40 and 0x00 -> count and few pixels
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemShortCountMask);

                    if (count == 0)
                    {
                        // Count 0 -> no pixels to process
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                    const ptrdiff_t offset = sx - surfaceOffset;
                    Pixel* const stencil = g_rendererState.surfaces.stencil + offset;

                    if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                    {
                        // Mask 0xC0 -> skip pixels
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                    {
                        // Mask 0x80 -> repeat one pixel
                        const U8 indx = pixels->pixels[0];
                        Pixel pixel = g_moduleState->rhombsPalette.palette[indx + 0x4000];

                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const Pixel sten = level | (stencil[i] & 3);
                            if (sten & 2)
                                pixel = (Pixel)(g_moduleState->backSurfaceShadePixel + ((*(DoublePixel*)g_moduleState->shadeColorMask & pixel) >> 1));

                            stencil[i] = sten;
                            sx[i] = pixel;
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else if ((pixels->count & kImageSpriteItemShortCompactMask) == kImageSpriteItemShortCompactMask)
                    {
                        // Mask 0x40 -> repeat and blend pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];
                            Pixel pixel = g_moduleState->rhombsPalette.palette[indx + 0x4000];
                            pixel = (sx[i] + pixel - (g_moduleState->actualColorBits & (sx[i] ^ pixel))) >> 1;

                            const Pixel sten = level | (stencil[i] & 3);
                            if (sten & 2)
                                pixel = (Pixel)(g_moduleState->backSurfaceShadePixel + ((*(DoublePixel*)g_moduleState->shadeColorMask & pixel) >> 1));

                            stencil[i - 1] = sten;
                            sx[i] = pixel;
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == 0)
                    {
                        // Mask 0x00 -> draw pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];
                            Pixel pixel = g_moduleState->rhombsPalette.palette[indx + 0x4000];

                            const Pixel sten = level | (stencil[i] & 3);
                            if (sten & 2)
                                pixel = (Pixel)(g_moduleState->backSurfaceShadePixel + ((*(DoublePixel*)g_moduleState->shadeColorMask & pixel) >> 1));

                            stencil[i] = sten;
                            sx[i] = pixel;
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x10004db0
void drawMainSurfacePaletteSpriteStencil(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite)
{
    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    level = (level + kStencilPixelOffset) << kStencilPixelColorShift;
    const U32 stencilLevel = (level << 16) | level;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.main;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x    = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                    if (count <= need)
                    {
                        if (pixels->count & kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> count and one pixel
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            // Mask 0x40 and 0x00 -> count and few pixels
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemCompactMask);

                    if (count == 0)
                    {
                        // Count 0 -> no pixels to process
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                    const ptrdiff_t offset = sx - surfaceOffset;
                    Pixel* const stencil = g_rendererState.surfaces.stencil + offset;

                    if (pixels->count & kImageSpriteItemCompactMask)
                    {
                        // Mask 0x80 -> repeat one pixel
                        const U8 indx = pixels->pixels[0];
                        const Pixel pixel = palette[indx];

                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            if (stencilLevel <= *(DoublePixel*)((Addr)stencil + (i - 1) * sizeof(Pixel)))
                                continue;

                            sx[i] = pixel;
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else
                    {
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            if (stencilLevel <= *(DoublePixel*)((Addr)stencil + (i - 1) * sizeof(Pixel)))
                                continue;

                            const U8 indx = pixels->pixels[skip + i];
                            const Pixel pixel = palette[indx];

                            sx[i] = pixel;
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x100050df
void drawMainSurfacePaletteSpriteCompact(S32 x, S32 y, const Pixel* palette, const ImagePaletteSprite* const sprite)
{
    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;

    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);

        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.main;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x    = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                Pixel* sx = g_rendererState.sprite.x;
                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                    if (count <= need)
                    {
                        if (pixels->count & kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> count and one pixel
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemCountMask);

                    if (count == 0)
                    {
                        // Count 0 -> no pixels to process
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                    if (pixels->count & kImageSpriteItemCompactMask)
                    {
                        // Mask 0x80 -> repeat one pixel
                        const U8 indx = pixels->pixels[0];

                        if (indx != 0)
                        {
                            const Pixel pixel = palette[indx];

                            std::fill_n(sx, availCount, pixel);
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else
                    {
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];

                            sx[i] = palette[indx];
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x100053c3
void drawMainSurfaceVanishingPaletteSprite(S32 x, S32 y, const S32 vanishOffset, const Pixel* palette, const ImagePaletteSprite* const sprite)
{
    const U32 colorMask = ((U32)g_moduleState->actualGreenMask << 16) | g_moduleState->actualBlueMask | g_moduleState->actualRedMask;
    g_rendererState.sprite.vanishOffset = vanishOffset;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    //g_rendererState.sprite.width = sprite->width + 1;
    g_rendererState.sprite.height = sprite->height;

    x += sprite->x;
    y += sprite->y;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height = g_rendererState.sprite.height - (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.main;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x    = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x    = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                Pixel* sx = g_rendererState.sprite.x;
                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                    if (count <= need)
                    {
                        if (pixels->count & kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> count and one pixel
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemCountMask);

                    if (count == 0)
                    {
                        // Count 0 -> no pixels to process
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                    if (pixels->count & kImageSpriteItemCompactMask)
                    {
                        const U8 indx = pixels->pixels[0];

                        if (indx != 0)
                        {
                            const Pixel pixel = palette[indx];

                            for (ptrdiff_t i = 0; i < availCount; ++i)
                            {
                                U32 tempDoublePixel = ((U32)sx[i] << 16) | sx[i];
                                U32 tempDoublePixel2 = (g_rendererState.sprite.colorMask & (((g_rendererState.sprite.colorMask & tempDoublePixel) * g_rendererState.sprite.vanishOffset) >> 5))
                                    | ((g_rendererState.sprite.colorMask & (((g_rendererState.sprite.colorMask & tempDoublePixel) * g_rendererState.sprite.vanishOffset) >> 5)) >> 16);
                                U32 tempDoublePixel3 = ((U32)pixel << 16) | pixel;
                                U32 tempDoublePixel4 = g_rendererState.sprite.colorMask & (((31 - g_rendererState.sprite.vanishOffset) * (g_rendererState.sprite.colorMask & tempDoublePixel3)) >> 5);
                                U32 tempDoublePixel5 = tempDoublePixel4 | (tempDoublePixel4 >> 16);

                                sx[i] = (U16)(tempDoublePixel2 + tempDoublePixel5);
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else
                    {
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];

                            const Pixel pixel = palette[indx];

                            U32 tempDoublePixel = ((U32)sx[i] << 16) | sx[i];
                            U32 tempDoublePixel2 = (g_rendererState.sprite.colorMask & (((g_rendererState.sprite.colorMask & tempDoublePixel) * g_rendererState.sprite.vanishOffset) >> 5))
                                | ((g_rendererState.sprite.colorMask & (((g_rendererState.sprite.colorMask & tempDoublePixel) * g_rendererState.sprite.vanishOffset) >> 5)) >> 16);
                            U32 tempDoublePixel3 = ((U32)pixel << 16) | pixel;
                            U32 tempDoublePixel4 = g_rendererState.sprite.colorMask & (((31 - g_rendererState.sprite.vanishOffset) * (g_rendererState.sprite.colorMask & tempDoublePixel3)) >> 5);
                            U32 tempDoublePixel5 = tempDoublePixel4 | (tempDoublePixel4 >> 16);

                            sx[i] = (U16)(tempDoublePixel2 + tempDoublePixel5);
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x1000579c
void drawBackSurfacePalletteSprite(S32 x, S32 y, const Pixel* const palette, const ImagePaletteSprite* const sprite)
{
    const U32 colorMask = ((U32)g_moduleState->actualGreenMask << 16) | g_moduleState->actualBlueMask | g_moduleState->actualRedMask;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.back;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x    = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemShortCountMask;

                    if (count <= need)
                    {
                        if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                        {
                            // Mask 0xC0 -> only count
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        }
                        else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> count and one pixel
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            // Mask 0x40 and 0x00 -> count and few pixels
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemShortCountMask);

                    if (count == 0)
                    {
                        // Count 0 -> no pixels to process
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                    if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                    {
                        // Mask 0xC0 -> skip pixels
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                    {
                        // Mask 0x80 -> repeat one pixel
                        const U8 indx = pixels->pixels[0];

                        if (indx != 0)
                        {
                            const Pixel pixel = palette[indx];

                            std::fill_n(sx, availCount, pixel);
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else if ((pixels->count & kImageSpriteItemShortCompactMask) == kImageSpriteItemShortCompactMask)
                    {
                        // Mask 0x40 -> repeat and blend pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];

                            const Pixel pixel = palette[indx];
                            sx[i] = (sx[i] + pixel - (g_moduleState->actualColorBits & (sx[i] ^ pixel))) >> 1;
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == 0)
                    {
                        // Mask 0x00 -> draw pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];

                            const Pixel pixel = palette[indx];
                            sx[i] = pixel;
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x10005ac6
void drawBackSurfacePaletteSpriteAndStencil(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite)
{
    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    level = (level + kStencilPixelOffset) << kStencilPixelColorShift;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.back;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemShortCountMask;

                    if (count <= need)
                    {
                        if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                        {
                            // Mask 0xC0 -> only count
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        }
                        else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> count and one pixel
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            // Mask 0x40 and 0x00 -> count and few pixels
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemShortCountMask);

                    if (count == 0)
                    {
                        // Count 0 -> no pixels to process
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);
                    Pixel* const stencil = g_rendererState.surfaces.stencil + (sx - g_rendererState.surfaces.back);

                    if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                    {
                        // Mask 0xC0 -> skip pixels
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                    {
                        // Mask 0x80 -> repeat one pixel
                        const U8 indx = pixels->pixels[0];
                        const Pixel pixel = palette[indx];

                        std::fill_n(sx, availCount, pixel);
                        std::fill_n(stencil, availCount, level);

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else if ((pixels->count & kImageSpriteItemShortCompactMask) == kImageSpriteItemShortCompactMask)
                    {
                        // Mask 0x40 -> repeat and blend pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];
                            const Pixel pixel = palette[indx];

                            sx[i] = (sx[i] + pixel - (g_moduleState->actualColorBits & (sx[i] ^ pixel))) >> 1;
                        }

                        std::fill_n(stencil, availCount, level);

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == 0)
                    {
                        // Mask 0x00 -> draw pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];
                            const Pixel pixel = palette[indx];

                            sx[i] = pixel;
                        }

                        std::fill_n(stencil, availCount, level);

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x10005e31
void drawBackSurfacePaletteShadedSprite(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite)
{
    const U32 colorMask = ((U32)g_moduleState->actualGreenMask << 16) | g_moduleState->actualBlueMask | g_moduleState->actualRedMask;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    level = (level + kStencilPixelOffset) << kStencilPixelColorShift;

    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x = x + sprite->x;
    y = y + sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height = g_rendererState.sprite.height - (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.back;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                    if (count <= need)
                    {
                        if (pixels->count & kImageSpriteItemCompactMask)
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemCountMask);

                    if (count == 0)
                    {
                        // Count 0 -> no pixels to process
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);
                    Pixel* const stencil = g_rendererState.surfaces.stencil + (sx - g_rendererState.surfaces.back);

                    if (pixels->count & kImageSpriteItemCompactMask)
                    {
                        // Mask 0x80 -> count and one pixel
                        const U8 indx = pixels->pixels[0];

                        if (indx != 0)
                        {
                            Pixel pixel = palette[indx];

                            for (ptrdiff_t i = 0; i < availCount; ++i)
                            {
                                const DoublePixel sten = stencil[i];
                                stencil[i] = level | (sten & 3);

                                if (sten & 2)
                                    pixel = (Pixel)(g_moduleState->backSurfaceShadePixel + ((*(DoublePixel*)g_moduleState->shadeColorMask & pixel) >> 1));

                                sx[i] = pixel;
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else
                    {
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];

                            const Pixel sten = stencil[i];
                            stencil[i] = level | (sten & 3);

                            Pixel pixel = palette[indx];
                            if (sten & 2)
                                pixel = (Pixel)(g_moduleState->backSurfaceShadePixel + ((*(DoublePixel*)g_moduleState->shadeColorMask & pixel) >> 1));

                            sx[i] = pixel;
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}


// 0x1000618d
void drawMainSurfacePaletteSprite(S32 x, S32 y, const Pixel* const palette, const ImagePaletteSprite* const sprite)
{
    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.main;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemShortCountMask;

                    if (count <= need)
                    {
                        if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                        {
                            // Mask 0xC0 -> only count
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        }
                        else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> count and one pixel
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            // Mask 0x40 and 0x00 -> count and few pixels
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemShortCountMask);

                    if (count == 0)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                    if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                    {
                        // Mask 0xC0 -> skip pixels
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                    {
                        // Mask 0x80 -> repeat one pixel
                        const U8 indx = pixels->pixels[0];
                        const Pixel pixel = palette[indx];

                        std::fill_n(sx, availCount, pixel);

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else if ((pixels->count & kImageSpriteItemShortCompactMask) == kImageSpriteItemShortCompactMask)
                    {
                        // Mask 0x40 -> repeat and blend pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];
                            const Pixel pixel = palette[indx];
                            sx[i] = (sx[i] + pixel - (g_moduleState->actualColorBits & (sx[i] ^ pixel))) >> 1;
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == 0)
                    {
                        // Mask 0x00 -> draw pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];
                            const Pixel pixel = palette[indx];
                            sx[i] = pixel;
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x100064b6
void drawMainSurfaceSprite(S32 x, S32 y, const ImageSprite* const sprite)
{
    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x = x + sprite->x;
    y = y + sprite->y;

    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height = g_rendererState.sprite.height - (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.main;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };
                ImageSpritePixel* pixels = (ImageSpritePixel*)content;

                Pixel* sx = g_rendererState.sprite.x;
                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                    if (count <= need)
                    {
                        if (pixels->count & kImageSpriteItemCompactMask)
                        {
                            pixels = (ImageSpritePixel*)((Addr)pixels + sizeof(ImageSpritePixel));
                        }
                        else
                        {
                            pixels = (ImageSpritePixel*)((Addr)pixels + sizeof(ImageSpritePixel) + (count - 1) * sizeof(Pixel));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                    if (count == 0)
                    {
                        // Count 0 -> no pixels to process
                        pixels = (ImageSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                    if (pixels->count & kImageSpriteItemCompactMask)
                    {
                        const Pixel pixel = pixels->pixels[0];

                        if (pixel != PixelColor::MAGENTA)
                        {
                            std::fill_n(sx, availCount, pixel);
                        }

                        pixels = (ImageSpritePixel*)((Addr)pixels + sizeof(ImageSpritePixel));
                    }
                    else
                    {
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const Pixel pixel = pixels->pixels[skip + i];

                            sx[i] = pixel;
                        }

                        pixels = (ImageSpritePixel*)((Addr)pixels + sizeof(ImageSpritePixel) + (count - 1) * sizeof(Pixel));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Обведите по вертикали и нарисуйте излишек
            // в случае, если у спрайта больше содержимого, которое может поместиться в разрешенный прямоугольник для рисования.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x100067ad
void drawMainSurfaceAnimationSprite(S32 x, S32 y, const AnimationPixel* palette, const ImagePaletteSprite* const sprite)
{
    const U32 colorMask = ((U32)g_moduleState->actualGreenMask << 16) | g_moduleState->actualBlueMask | g_moduleState->actualRedMask;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.main;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                    if (count <= need)
                    {
                        if (pixels->count & kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> count and one pixel
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            // Mask 0x40 and 0x00 -> count and few pixels
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemCountMask);

                    if (count == 0)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                    if (pixels->count & kImageSpriteItemCompactMask)
                    {
                        const U8 indx = pixels->pixels[0];
                        const AnimationPixel pixel = palette[indx];

                        const DoublePixel pix = pixel >> 19;

                        if ((pix & 0xFF) != 0x1F)
                        {
                            for (ptrdiff_t i = 0; i < availCount; ++i)
                            {
                                const DoublePixel res = ((DoublePixel)sx[i] << 16) | sx[i];
                                const DoublePixel value = g_rendererState.sprite.colorMask & (((g_rendererState.sprite.colorMask & res) * pix) >> 5);

                                sx[i] = (Pixel)pixel + (Pixel)((value >> 16) | value);
                            }
                        }
                    }
                    else
                    {
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];
                            const AnimationPixel pixel = palette[indx];

                            const DoublePixel pix = pixel >> 19;

                            if ((pix & 0xFF) != 0x1F)
                            {
                                const DoublePixel res = ((DoublePixel)sx[i] << 16) | sx[i];
                                const DoublePixel value = g_rendererState.sprite.colorMask & (((g_rendererState.sprite.colorMask & res) * pix) >> 5);

                                sx[i] = (Pixel)pixel + (Pixel)((value >> 16) | value);
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + ((count - 1) * sizeof(U8) + sizeof(ImagePaletteSpritePixel)));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x10006b21
void drawMainSurfaceAnimationSpriteStencil(S32 x, S32 y, U16 level, const AnimationPixel* palette, ImagePaletteSprite* sprite)
{
    const U32 colorMask = ((U32)g_moduleState->actualGreenMask << 16) | g_moduleState->actualBlueMask | g_moduleState->actualRedMask;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    level = (level + kStencilPixelOffset) << kStencilPixelColorShift;
    const DoublePixel stencilLevel = (level << 16) | level;

    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x = x + sprite->x;
    y = y + sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height = g_rendererState.sprite.height - (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.main;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                // There was a bug, that 'pixels' was bigger than 'next'. According to IDA, there is a separate loop "pixels < next"
                // Also, there was a bug that 'i < count - skip', but count has been reduced by skip
                // And also, there was a bug that sx[i] >= maxX
                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                    if (count <= need)
                    {
                        if (pixels->count & kImageSpriteItemCompactMask)
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemCountMask);

                    if (count == 0)
                    {
                        // Count 0 -> no pixels to process
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);
                    Pixel* const stencil = g_rendererState.surfaces.stencil + (sx - g_rendererState.surfaces.main);

                    if (pixels->count & kImageSpriteItemCompactMask)
                    {
                        const U8 indx = pixels->pixels[0];
                        const AnimationPixel pixel = palette[indx];

                        const DoublePixel pix = pixel >> 19;

                        if ((pix & 0xFF) != 0x1F)
                        {
                            for (ptrdiff_t i = 0; i < availCount; ++i)
                            {
                                if (stencilLevel <= *(DoublePixel*)(stencil + i - 1))
                                    continue;

                                const DoublePixel value =
                                    (pix * (((sx[i] << 16) | sx[i]) & g_rendererState.sprite.colorMask) >> 5) & g_rendererState.sprite.colorMask;

                                sx[i] = (Pixel)((value >> 16) | value) + (Pixel)pixel;
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)sizeof(ImagePaletteSpritePixel));
                    }
                    else
                    {
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];
                            const AnimationPixel pixel = palette[indx];

                            const DoublePixel pix = pixel >> 19;

                            if ((pix & 0xFF) != 0x1F)
                            {
                                if (stencilLevel <= *(DoublePixel*)(stencil + i - 1))
                                    continue;

                                const DoublePixel value =
                                    (pix * (((sx[i] << 16) | sx[i]) & g_rendererState.sprite.colorMask) >> 5) & g_rendererState.sprite.colorMask;

                                sx[i] = (Pixel)((value >> 16) | value) + (Pixel)pixel;
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)((count - 1) * sizeof(U8) + sizeof(ImagePaletteSpritePixel)));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x10006ef8
void drawMainSurfacePaletteSpriteFrontStencil(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite)
{
    const U32 colorMask = ((U32)g_moduleState->actualGreenMask << 16) | g_moduleState->actualBlueMask | g_moduleState->actualRedMask;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    level = (level + kStencilPixelOffset) << kStencilPixelColorShift;
    const DoublePixel stencilLevel = (level << 16) | level;

    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.main;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemShortCountMask;

                    if (count <= need)
                    {
                        if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                        {
                            // Mask 0xC0 -> only count
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        }
                        else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> count and one pixel
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            // Mask 0x40 and 0x00 -> count and few pixels
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemShortCountMask);

                    if (count == 0)
                    {
                        // Count 0 -> no pixels to process
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);
                    Pixel* const stencil = g_rendererState.surfaces.stencil + (sx - g_rendererState.surfaces.main);

                    if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                    {
                        // Mask 0xC0 -> skip pixels
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                    {
                        // Mask 0x80 -> repeat one pixel
                        const U8 indx = pixels->pixels[0];
                        const Pixel pixel = palette[indx];

                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            if (*(DoublePixel*)(stencil + i - 1) < stencilLevel)
                            {
                                sx[i] = pixel;
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else if ((pixels->count & kImageSpriteItemShortCompactMask) == kImageSpriteItemShortCompactMask)
                    {
                        // Mask 0x40 -> repeat and blend pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            if (*(DoublePixel*)(stencil + i - 1) < stencilLevel)
                            {
                                const U8 indx = pixels->pixels[skip + i];
                                const Pixel pixel = palette[indx];

                                sx[i] = (sx[i] + pixel - (g_moduleState->actualColorBits & (sx[i] ^ pixel))) >> 1;
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == 0)
                    {
                        // Mask 0x00 -> draw pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            if (*(DoublePixel*)(stencil + i - 1) < stencilLevel)
                            {
                                const U8 indx = pixels->pixels[skip + i];
                                const Pixel pixel = palette[indx];

                                sx[i] = pixel;
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x10007292
void drawMainSurfacePaletteSpriteBackStencil(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite)
{
    const U32 colorMask = ((U32)g_moduleState->actualGreenMask << 16) | g_moduleState->actualBlueMask | g_moduleState->actualRedMask;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    level = (level + kStencilPixelOffset) << kStencilPixelColorShift;
    const DoublePixel stencilLevel = (level << 16) | level;

    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;
    S32 chessPixel = y;    // Used to display units behind objects in a checkerboard pattern

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        chessPixel += g_moduleState->windowRect.y - y;
        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = (Addr)(g_moduleState->surface.stride * y);
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.surfaces.main
            + g_moduleState->surface.offset + linesStride + (Addr)(x * sizeof(Pixel)));
        g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + g_moduleState->surface.offset + linesStride + (Addr)(g_moduleState->windowRect.x * sizeof(Pixel)));
        g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + g_moduleState->surface.offset + linesStride + (Addr)((g_moduleState->windowRect.width + 1) * sizeof(Pixel)));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                chessPixel ^= 1;
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemShortCountMask;

                    if (count <= need)
                    {
                        if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                        {
                            // Mask 0xC0 -> only count
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        }
                        else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> count and one pixel
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            // Mask 0x40 and 0x00 -> count and few pixels
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemShortCountMask);

                    if (count == 0)
                    {
                        // Count 0 -> no pixels to process
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);
                    Pixel* const stencil = g_rendererState.surfaces.stencil + (sx - g_rendererState.surfaces.main);

                    if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                    {
                        // Mask 0xC0 -> skip pixels
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                    {
                        // Mask 0x80 -> repeat one pixel
                        const U8 indx = pixels->pixels[0];
                        const Pixel pixel = palette[indx];

                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            if ((((Addr)(sx + i) / 2 ^ chessPixel) & 1) != 0
                                || *(DoublePixel*)(stencil + i - 1) < stencilLevel)
                            {
                                sx[i] = pixel;
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else if ((pixels->count & kImageSpriteItemShortCompactMask) == kImageSpriteItemShortCompactMask)
                    {
                        // Mask 0x40 -> repeat and blend pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            if ((((Addr)(sx + i) / 2 ^ chessPixel) & 1) != 0
                                || *(DoublePixel*)(stencil + i - 1) < stencilLevel)
                            {
                                const U8 indx = pixels->pixels[skip + i];

                                const Pixel pixel = palette[indx];
                                sx[i] = (sx[i] + pixel - (g_moduleState->actualColorBits & (sx[i] ^ pixel))) >> 1;
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemExtendedMask) == 0)
                    {
                        // Mask 0x00 -> draw pixels
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            if ((((Addr)(sx + i) / 2 ^ chessPixel) & 1) != 0
                                || *(DoublePixel*)(stencil + i - 1) < stencilLevel)
                            {
                                const U8 indx = pixels->pixels[skip + i];
                                const Pixel pixel = palette[indx];
                                sx[i] = pixel;
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}


// 0x10007662
void drawMainSurfaceShadowSprite(S32 x, S32 y, const DoublePixel shadePixel, const ImagePaletteSprite* const sprite)
{
    const U32 colorMask = ((U32)g_moduleState->actualGreenMask << 16) | g_moduleState->actualBlueMask | g_moduleState->actualRedMask;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.main;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                // This function uses 0x80 RLE mask to skip pixels, not to draw one pixel few times.
                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                    if (count <= need)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    ptrdiff_t count = (pixels->count & kImageSpriteItemCountMask);

                    if (count == 0)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);
                    Pixel* const stencil = g_rendererState.surfaces.stencil + (sx - g_rendererState.surfaces.main);

                    if ((pixels->count & kImageSpriteItemCompactMask) == 0)
                    {
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const DoublePixel sten = *(DoublePixel*)(stencil + i);
                                if ((sten & kStencilPixelShadowMask) == 0)
                                {
                                    *(DoublePixel*)(stencil + i) = shadePixel | sten;

                                    const Pixel pixel = (Pixel)(g_moduleState->backSurfaceShadePixel + SHADEPIXEL(*(DoublePixel*)(sx + i), *(DoublePixel*)&g_moduleState->shadeColorMask));
                                    sx[i] = pixel;
                                }
                        }
                    }

                    pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x10007928
void drawBackSurfaceShadowSprite(S32 x, S32 y, const DoublePixel shadePixel, const ImagePaletteSprite* const sprite)
{
    const U32 colorMask = ((U32)g_moduleState->actualGreenMask << 16) | g_moduleState->actualBlueMask | g_moduleState->actualRedMask;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.back;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                // This function uses 0x80 RLE mask to skip pixels, not to draw one pixel few times.
                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                    if (count <= need)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    ptrdiff_t count = (pixels->count & kImageSpriteItemCountMask);

                    if (count == 0)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);
                    Pixel* const stencil = g_rendererState.surfaces.stencil + (sx - g_rendererState.surfaces.back);

                    if ((pixels->count & kImageSpriteItemCompactMask) == 0)
                    {
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const DoublePixel sten = *(DoublePixel*)(stencil + i);
                            if ((sten & kStencilPixelShadowMask) == 0)
                            {
                                *(DoublePixel*)(stencil + i) = shadePixel | sten;

                                const Pixel pixel = (Pixel)(g_moduleState->backSurfaceShadePixel + SHADEPIXEL(*(DoublePixel*)(sx + i), *(DoublePixel*)&g_moduleState->shadeColorMask));
                                sx[i] = pixel;
                            }
                        }
                    }

                    pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x10007be8
void drawMainSurfaceAdjustedSprite(S32 x, S32 y, U16 level, const ImagePaletteSprite* const sprite)
{
    const U32 colorMask = ((U32)g_moduleState->actualGreenMask << 16) | g_moduleState->actualBlueMask | g_moduleState->actualRedMask;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    level = (level + kStencilPixelOffset) << kStencilPixelColorShift;
    const DoublePixel stencilLevel = (level << 16) | level;

    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.main;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                    if (count <= need)
                    {
                        if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                        {
                            // Mask 0xC0 -> only count
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        }
                        else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> count and one pixel
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            // Mask 0x40 and 0x00 -> count and few pixels
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    ptrdiff_t count = (pixels->count & kImageSpriteItemCountMask);

                    if (count == 0)
                    {
                        // Count 0 -> no pixels to process
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);
                    Pixel* const stencil = g_rendererState.surfaces.stencil + (sx - g_rendererState.surfaces.main);

                    if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                    {
                        // Mask 0xC0 -> skip pixels
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemCompactMask) == kImageSpriteItemCompactMask)
                    {
                        // Mask 0x80 -> repeat one pixel
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            if (*(DoublePixel*)(stencil + i - 1) < stencilLevel)
                            {
                                DoublePixel pixel = g_rendererState.sprite.colorMask & (sx[i] | ((DoublePixel)sx[i] << 16));
                                pixel = g_rendererState.sprite.adjustedColorMask & ((pixel * (pixels->pixels[0] & kImageSpriteItemSmallPixelMask)) >> 4);
                                pixel = g_rendererState.sprite.colorMask &
                                    (((g_rendererState.sprite.colorMask - pixel) >> 5) | pixel);

                                sx[i] = (Pixel)((pixel >> 16) | pixel);
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else
                    {
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            if (*(DoublePixel*)(stencil + i - 1) < stencilLevel)
                            {
                                DoublePixel pixel = g_rendererState.sprite.colorMask & (sx[i] | ((DoublePixel)sx[i] << 16));
                                pixel = g_rendererState.sprite.adjustedColorMask & ((pixel * (pixels->pixels[skip + i] & kImageSpriteItemSmallPixelMask)) >> 4);
                                pixel = g_rendererState.sprite.colorMask &
                                    (((g_rendererState.sprite.colorMask - pixel) >> 5) | pixel);

                                sx[i] = (Pixel)((pixel >> 16) | pixel);
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}

// 0x10007fbc
void drawMainSurfaceActualSprite(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite)
{
    const U32 colorMask = ((U32)g_moduleState->actualGreenMask << 16) | g_moduleState->actualBlueMask | g_moduleState->actualRedMask;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    level = (level + kStencilPixelOffset) << kStencilPixelColorShift;
    const DoublePixel stencilLevel = (level << 16) | level;

    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x += sprite->x;
    y += sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState->windowRect.y)
    {
        g_rendererState.sprite.height -= (g_moduleState->windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState->windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_moduleState->windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState->windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = g_moduleState->surface.stride * y;
        const Pixel* surfaceOffset = g_rendererState.surfaces.main;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * g_moduleState->windowRect.x);
        g_rendererState.sprite.maxX = (Pixel*)((Addr)surfaceOffset + g_moduleState->surface.offset + linesStride + sizeof(Pixel) * (g_moduleState->windowRect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState->surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState->surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                    if (count <= need)
                    {
                        if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                        {
                            // Mask 0xC0 -> only count
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        }
                        else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> count and one pixel
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            // Mask 0x40 and 0x00 -> count and few pixels
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    ptrdiff_t count = (pixels->count & kImageSpriteItemCountMask);

                    if (count == 0)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);
                    Pixel* const stencil = g_rendererState.surfaces.stencil + (sx - g_rendererState.surfaces.main);

                    if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                    {
                        // Mask 0xC0 -> skip pixels
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                    }
                    else if ((pixels->count & kImageSpriteItemCompactMask) == kImageSpriteItemCompactMask)
                    {
                        // Mask 0x80 -> repeat one pixel
                        const U8 indx = pixels->pixels[0];
                        const Pixel pixel = palette[indx];

                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            if (*(DoublePixel*)(stencil + i - 1) < stencilLevel)
                            {
                                const Pixel summ = pixel + sx[i];
                                const Pixel colorPixel = g_moduleState->actualColorMask & ((summ ^ pixel ^ sx[i]) >> 1);
                                Pixel out = summ - colorPixel;
                                if (colorPixel & g_moduleState->actualRedMask)
                                    out |= g_moduleState->actualRedMask;
                                if (colorPixel & g_moduleState->actualGreenMask)
                                    out |= g_moduleState->actualGreenMask;
                                if (colorPixel & g_moduleState->actualBlueMask)
                                    out |= g_moduleState->actualGreenMask;
                                sx[i] = out;
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else
                    {
                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            if (*(DoublePixel*)(stencil + i - 1) < stencilLevel)
                            {
                                const U8 indx = pixels->pixels[skip + i];
                                const Pixel pixel = palette[indx];

                                const Pixel summ = pixel + sx[i];
                                const Pixel colorPixel = g_moduleState->actualColorMask & ((summ ^ pixel ^ sx[i]) >> 1);
                                Pixel out = summ - colorPixel;
                                if (colorPixel & g_moduleState->actualRedMask)
                                    out |= g_moduleState->actualRedMask;
                                if (colorPixel & g_moduleState->actualGreenMask)
                                    out |= g_moduleState->actualGreenMask;
                                if (colorPixel & g_moduleState->actualBlueMask)
                                    out |= g_moduleState->actualGreenMask;
                                sx[i] = out;
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState->surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState->surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}


// 0x10008ecd
void drawUiSprite(S32 x, S32 y, const ImagePaletteSprite* const sprite, const void* pal, const ImageSpriteUI* const uiSprite)
{
    // Early exit just in case
    switch (sprite->type)
    {
    case SPRITE_TYPE_STATIC:
    case SPRITE_TYPE_DYNAMIC:
    case SPRITE_TYPE_ANIMATION:
    case SPRITE_TYPE_ALPHA:
        break;
    default:
        return;
    }

    g_rendererState.gameUI.offset = uiSprite->offset;
    g_rendererState.gameUI.stride = uiSprite->stride * 2;
    g_rendererState.gameUI.rect.x = uiSprite->x;
    g_rendererState.gameUI.rect.y = uiSprite->y;
    g_rendererState.gameUI.rect.width = uiSprite->width;
    g_rendererState.gameUI.rect.height = uiSprite->height;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    y += sprite->y;
    x += sprite->x;

    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    if (y < g_rendererState.gameUI.rect.y)
    {
        g_rendererState.sprite.height -= (g_rendererState.gameUI.rect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_rendererState.gameUI.rect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_rendererState.gameUI.rect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_rendererState.gameUI.rect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        Addr offset = g_rendererState.gameUI.offset;
        const Addr linesStride = g_rendererState.gameUI.stride * y;
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)(offset + linesStride + x * sizeof(Pixel));
        g_rendererState.sprite.minX = (Pixel*)(offset + linesStride + g_rendererState.gameUI.rect.x * sizeof(Pixel));
        g_rendererState.sprite.maxX = (Pixel*)(offset + linesStride + (g_rendererState.gameUI.rect.width + 1) * sizeof(Pixel));

        const S32 overage = y + g_rendererState.sprite.height < Screen::height_ ? 0 : y + g_rendererState.sprite.height - Screen::height_;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        switch (sprite->type)
        {
        case SPRITE_TYPE_STATIC:
        {
            Pixel* palette = (Pixel*)pal;

            while (g_rendererState.sprite.height > 0)
            {
                while (g_rendererState.sprite.height > 0)
                {
                    ptrdiff_t skip{ 0 };
                    ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                    Pixel* sx = g_rendererState.sprite.x;
                    while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                    {
                        const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                        const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                        if (count <= need)
                        {
                            if (pixels->count & kImageSpriteItemCompactMask)
                            {
                                // Mask 0x80 -> count and one pixel
                                pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                            }
                            else
                            {
                                // Mask 0x40 and 0x00 -> count and few pixels
                                pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(pixels->pixels));
                            }
                        }

                        skip = count == need ? 0 : std::min(count, need);
                        sx += std::min(count, need);
                    }

                    while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                    {
                        const ptrdiff_t count = (pixels->count & kImageSpriteItemCountMask);

                        if (count == 0)
                        {
                            // Count 0 -> no pixels to process
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                            return;
                        }

                        const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                        if (pixels->count & kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> repeat one pixel
                            const U8 indx = pixels->pixels[0];

                            if (indx != 0)
                            {
                                const Pixel pixel = palette[indx];

                                std::fill_n(sx, availCount, pixel);
                            }

                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            for (ptrdiff_t i = 0; i < availCount; ++i)
                            {
                                const U8 indx = pixels->pixels[skip + i];
                                const Pixel pixel = palette[indx];

                                sx[i] = pixel;
                            }

                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(pixels->pixels));
                        }

                        sx += availCount;

                        skip = 0;
                    }

                    content = (void*)((Addr)next + sizeof(U16));
                    next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                    --g_rendererState.sprite.height;

                    g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_rendererState.gameUI.stride);
                    g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_rendererState.gameUI.stride);
                    g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_rendererState.gameUI.stride);
                }

                // Wrap around vertically, and draw the overage
                // in case the sprite has more content that can fit into the allowed drawing rectangle.
                g_rendererState.sprite.height = g_rendererState.sprite.overage;
                g_rendererState.sprite.overage = 0;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
            }

            break;
        }
        case SPRITE_TYPE_DYNAMIC:
        {
            Pixel* palette = (Pixel*)pal;

            while (g_rendererState.sprite.height > 0)
            {
                while (g_rendererState.sprite.height > 0)
                {
                    ptrdiff_t skip{ 0 };
                    ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                    Pixel* sx = g_rendererState.sprite.x;
                    while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                    {
                        const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                        const ptrdiff_t count = pixels->count & kImageSpriteItemShortCountMask;

                        if (count <= need)
                        {
                            if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                            {
                                // Mask 0x80 -> count and one pixel
                                pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                            }
                            else
                            {
                                // Mask 0x40 and 0x00 -> count and few pixels
                                pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(pixels->pixels));
                            }
                        }

                        skip = count == need ? 0 : std::min(count, need);
                        sx += std::min(count, need);
                    }

                    while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                    {
                        const ptrdiff_t count = (pixels->count & kImageSpriteItemShortCountMask);

                        if (count == 0)
                        {
                            // Count 0 -> no pixels to process
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                            continue;
                        }

                        const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                        if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                        {
                            // Mask 0xC0 -> skip pixels
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        }
                        else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> repeat one pixel
                            const U8 indx = pixels->pixels[0];
                            const Pixel pixel = palette[indx];

                            std::fill_n(sx, availCount, pixel);

                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else if ((pixels->count & kImageSpriteItemShortCompactMask) == kImageSpriteItemShortCompactMask)
                        {
                            // Mask 0x40 -> repeat and blend pixels
                            for (ptrdiff_t i = 0; i < availCount; ++i)
                            {
                                const U8 indx = pixels->pixels[skip + i];
                                const Pixel pixel = palette[indx];

                                sx[i] = (sx[i] + pixel - (g_moduleState->actualColorBits & (sx[i] ^ pixel))) >> 1;
                            }

                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(pixels->pixels));
                        }
                        else if ((pixels->count & kImageSpriteItemExtendedMask) == 0)
                        {
                            // Mask 0x00 -> draw pixels
                            for (ptrdiff_t i = 0; i < availCount; ++i)
                            {
                                const U8 indx = pixels->pixels[skip + i];
                                const Pixel pixel = palette[indx];

                                sx[i] = pixel;
                            }

                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(pixels->pixels));
                        }

                        sx += availCount;

                        skip = 0;
                    }

                    content = (void*)((Addr)next + sizeof(U16));
                    next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                    --g_rendererState.sprite.height;

                    g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_rendererState.gameUI.stride);
                    g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_rendererState.gameUI.stride);
                    g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_rendererState.gameUI.stride);
                }

                // Wrap around vertically, and draw the overage
                // in case the sprite has more content that can fit into the allowed drawing rectangle.
                g_rendererState.sprite.height = g_rendererState.sprite.overage;
                g_rendererState.sprite.overage = 0;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
            }

            break;
        }
        case SPRITE_TYPE_ALPHA:
        {
            while (g_rendererState.sprite.height > 0)
            {
                while (g_rendererState.sprite.height > 0)
                {
                    ptrdiff_t skip{ 0 };
                    ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                    Pixel* sx = g_rendererState.sprite.x;
                    while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                    {
                        const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                        const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                        if (count <= need)
                        {
                            if (pixels->count & kImageSpriteItemCompactMask)
                            {
                                // Mask 0x80 -> count and one pixel
                                pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                            }
                            else
                            {
                                // Mask 0x40 and 0x00 -> count and few pixels
                                pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(pixels->pixels));
                            }
                        }

                        skip = count == need ? 0 : std::min(count, need);
                        sx += std::min(count, need);
                    }

                    while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                    {
                        ptrdiff_t count = (pixels->count & kImageSpriteItemCountMask);

                        if (count == 0)
                        {
                            // Count 0 -> no pixels to process
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                            continue;
                        }

                        const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                        if ((pixels->count & kImageSpriteItemCompactMask) == 0)
                        {
                            for (ptrdiff_t i = 0; i < availCount; ++i)
                            {
                                if ((sx[i] & kStencilPixelShadowMask) == 0)
                                {
                                    const Pixel pixel = (Pixel)(g_moduleState->backSurfaceShadePixel + SHADEPIXEL(*(DoublePixel*)(sx + i), *(DoublePixel*)&g_moduleState->shadeColorMask));

                                    sx[i] = pixel;
                                }
                            }
                        }
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        sx += availCount;

                        skip = 0;
                    }

                    content = (void*)((Addr)next + sizeof(U16));
                    next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                    --g_rendererState.sprite.height;

                    g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_rendererState.gameUI.stride);
                    g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_rendererState.gameUI.stride);
                    g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_rendererState.gameUI.stride);
                }

                // Wrap around vertically, and draw the overage
                // in case the sprite has more content that can fit into the allowed drawing rectangle.
                g_rendererState.sprite.height = g_rendererState.sprite.overage;
                g_rendererState.sprite.overage = 0;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
            }

            break;
        }
        case SPRITE_TYPE_ANIMATION:
        {
            AnimationPixel* palette = (AnimationPixel*)pal;

            const U32 colorMask = ((U32)g_moduleState->actualGreenMask << 16) | g_moduleState->actualBlueMask | g_moduleState->actualRedMask;
            g_rendererState.sprite.colorMask = colorMask;
            g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

            while (g_rendererState.sprite.height > 0)
            {
                while (g_rendererState.sprite.height > 0)
                {
                    ptrdiff_t skip{ 0 };
                    ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                    // Skip the pixels to the left of the sprite drawing area
                    // in case the sprite starts to the left of allowed drawing rectangle.
                    Pixel* sx = g_rendererState.sprite.x;

                    while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                    {
                        const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                        const ptrdiff_t count = pixels->count & kImageSpriteItemShortCountMask;

                        if (count <= need)
                        {
                            if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemExtendedMask)
                            {
                                // Mask 0xC0 -> only count
                                pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                            }
                            else if ((pixels->count & kImageSpriteItemExtendedMask) == kImageSpriteItemCompactMask)
                            {
                                // Mask 0x80 -> count and one pixel
                                pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                            }
                            else
                            {
                                // Mask 0x40 and 0x00 -> count and few pixels
                                pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(pixels->pixels));
                            }
                        }

                        skip = count == need ? 0 : std::min(count, need);
                        sx += std::min(count, need);
                    }

                    while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                    {
                        const ptrdiff_t count = (pixels->count & kImageSpriteItemCountMask);

                        if (count == 0)
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                            continue;
                        }

                        const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                        if (pixels->count & kImageSpriteItemCompactMask)
                        {
                            // Mask 0x80 -> repeat one pixel
                            const U8 indx = pixels->pixels[0];
                            const AnimationPixel pixel = palette[indx];
                            const DoublePixel pix = pixel >> 19;

                            if ((pix & 0xFF) != 0x1F)
                            {
                                for (ptrdiff_t i = 0; i < availCount; ++i)
                                {
                                    const DoublePixel value =
                                        (pix * (((sx[i] << 16) | sx[i]) & g_rendererState.sprite.colorMask) >> 5) & g_rendererState.sprite.colorMask;

                                    sx[i] = (Pixel)((value >> 16) | value) + (Pixel)pixel;
                                }
                            }

                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            for (ptrdiff_t i = 0; i < availCount; ++i)
                            {
                                const U8 indx = pixels->pixels[skip + i];
                                const AnimationPixel pixel = palette[indx];
                                const DoublePixel pix = pixel >> 19;

                                if ((pix & 0xFF) != 0x1F)
                                {
                                    const DoublePixel value = (pix * (((sx[i] << 16) | sx[i]) & g_rendererState.sprite.colorMask) >> 5) & g_rendererState.sprite.colorMask;

                                    sx[i] = (Pixel)((value >> 16) | value) + (Pixel)pixel;
                                }
                            }

                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(pixels->pixels));
                        }

                        sx += availCount;

                        skip = 0;
                    }

                    content = (void*)((Addr)next + sizeof(U16));
                    next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                    --g_rendererState.sprite.height;

                    g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_rendererState.gameUI.stride);
                    g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_rendererState.gameUI.stride);
                    g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_rendererState.gameUI.stride);
                }

                // Wrap around vertically, and draw the overage
                // in case the sprite has more content that can fit into the allowed drawing rectangle.
                g_rendererState.sprite.height = g_rendererState.sprite.overage;
                g_rendererState.sprite.overage = 0;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
            }

            break;
        }
        }
    }
}

// 0x10009eb3
void markUiWithButtonType(S32 x, S32 y, const ImagePaletteSprite* const sprite, const ButtonType type, const ImageSpriteUI* const uiSprite, const ButtonType* const offset)
{
    g_rendererState.gameUI.stride = uiSprite->stride;
    g_rendererState.gameUI.rect.x = uiSprite->x;
    g_rendererState.gameUI.rect.y = uiSprite->y;
    g_rendererState.gameUI.rect.width = uiSprite->width;
    g_rendererState.gameUI.rect.height = uiSprite->height;

    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    x += sprite->x;
    y += sprite->y;

    const void* content = &sprite->pixels[0];
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_rendererState.gameUI.rect.y)
    {
        g_rendererState.sprite.height -= (g_rendererState.gameUI.rect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_rendererState.gameUI.rect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_rendererState.gameUI.rect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_rendererState.gameUI.rect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = (Addr)(g_rendererState.gameUI.stride * y);
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)(offset + linesStride + x);
        g_rendererState.sprite.minX = (Pixel*)(offset + linesStride + g_rendererState.gameUI.rect.x);
        g_rendererState.sprite.maxX = (Pixel*)(offset + linesStride + g_rendererState.gameUI.rect.width + 1);


        const S32 overage = y + g_rendererState.sprite.height < Screen::height_
            ? 0 : y + g_rendererState.sprite.height - Screen::height_;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)Screen::sizeInPixels_);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)Screen::sizeInPixels_);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many elements we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the elements to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                ButtonType* sx = (ButtonType*)g_rendererState.sprite.x;

                while (sx < (ButtonType*)g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = (ButtonType*)g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                    if (count <= need)
                    {
                        if (pixels->count & kImageSpriteItemCompactMask)
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(U8));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < (ButtonType*)g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemCountMask);

                    if (count == 0)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, (ButtonType*)g_rendererState.sprite.maxX - sx);

                    std::memset(sx, type, availCount * sizeof(*sx));
                    if (pixels->count & kImageSpriteItemCompactMask)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + ((count - 1) * sizeof(U8) + sizeof(ImagePaletteSpritePixel)));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + g_rendererState.gameUI.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + g_rendererState.gameUI.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + g_rendererState.gameUI.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)Screen::sizeInPixels_);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)Screen::sizeInPixels_);
        }
    }
}

// 0x1000a4f3
void drawVanishingUiSprite(S32 x, S32 y, const S32 vanishLevel, const Pixel* palette, ImagePaletteSprite* const sprite, const ImageSpriteUI* const uiSprite)
{
    const U32 colorMask = ((U32)g_moduleState->actualGreenMask << 16) | g_moduleState->actualBlueMask | g_moduleState->actualRedMask;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    g_rendererState.gameUI.offset = uiSprite->offset;
    g_rendererState.gameUI.stride = uiSprite->stride * sizeof(Pixel);
    g_rendererState.gameUI.rect.x = uiSprite->x;
    g_rendererState.gameUI.rect.y = uiSprite->y;
    g_rendererState.gameUI.rect.width = uiSprite->width;
    g_rendererState.gameUI.rect.height = uiSprite->height;

    g_rendererState.sprite.height = sprite->height;
    //g_rendererState.sprite.width = sprite->width + 1;

    x += sprite->x;
    y += sprite->y;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_rendererState.gameUI.rect.y)
    {
        g_rendererState.sprite.height -= (g_rendererState.gameUI.rect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_rendererState.gameUI.rect.y - y; ++i)
        {
            content = (void*)((Addr)next + sizeof(U16));
            next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);
        }

        y = g_rendererState.gameUI.rect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_rendererState.gameUI.rect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        Addr offset = g_rendererState.gameUI.offset;
        const Addr linesStride = (Addr)(g_rendererState.gameUI.stride * y);
        const Addr screenSizeInBytes = Screen::sizeInBytes_;

        g_rendererState.sprite.x = (Pixel*)(offset + linesStride + sizeof(Pixel) * x);
        g_rendererState.sprite.minX = (Pixel*)(offset + linesStride + sizeof(Pixel) * g_rendererState.gameUI.rect.x);
        g_rendererState.sprite.maxX = (Pixel*)(offset + linesStride + sizeof(Pixel) * (g_rendererState.gameUI.rect.width + 1));


        const S32 overage = y + g_rendererState.sprite.height < Screen::height_
            ? 0 : y + g_rendererState.sprite.height - Screen::height_;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                ptrdiff_t skip{ 0 };       // How many elements we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the elements to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t need = g_rendererState.sprite.minX - sx;
                    const ptrdiff_t count = pixels->count & kImageSpriteItemCountMask;

                    if (count <= need)
                    {
                        if (pixels->count & kImageSpriteItemCompactMask)
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                        else
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(pixels->pixels));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx += std::min(count, need);
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const ptrdiff_t count = (pixels->count & kImageSpriteItemCountMask);

                    if (count == 0)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(U8));
                        continue;
                    }

                    const ptrdiff_t availCount = std::min(count - skip, g_rendererState.sprite.maxX - sx);

                    if (pixels->count & kImageSpriteItemCompactMask)
                    {
                        const U8 indx = pixels->pixels[0];
                        const Pixel pixel = palette[indx];

                        const DoublePixel res2 = ((DoublePixel)pixel << 16) | pixel;
                        const DoublePixel mask2 = g_rendererState.sprite.colorMask & (((g_rendererState.sprite.colorMask & res2) * (31 - vanishLevel)) >> 5);

                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const DoublePixel res = ((DoublePixel)sx[i] << 16) | sx[i];
                            const DoublePixel mask = g_rendererState.sprite.colorMask & (((g_rendererState.sprite.colorMask & res) * vanishLevel) >> 5);

                            sx[i] = (Pixel)((mask | (mask >> 16)) + (mask2 | (mask >> 16)));
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                    }
                    else
                    {

                        for (ptrdiff_t i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[0];
                            const Pixel pixel = palette[indx];

                            const DoublePixel res2 = ((DoublePixel)pixel << 16) | pixel;
                            const DoublePixel mask2 = g_rendererState.sprite.colorMask & (((g_rendererState.sprite.colorMask & res2) * (31 - vanishLevel)) >> 5);

                            const DoublePixel res = ((DoublePixel)sx[i] << 16) | sx[i];
                            const DoublePixel mask = g_rendererState.sprite.colorMask & (((g_rendererState.sprite.colorMask & res) * vanishLevel) >> 5);

                            sx[i] = (Pixel)((mask | (mask >> 16)) + (mask2 | (mask >> 16)));
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel) + (count - 1) * sizeof(pixels->pixels));
                    }

                    sx += availCount;

                    skip = 0;
                }

                content = (void*)((Addr)next + sizeof(U16));
                next = (void*)((Addr)next + sizeof(U16) + ((U16*)next)[0]);

                --g_rendererState.sprite.height;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + g_rendererState.gameUI.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + g_rendererState.gameUI.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + g_rendererState.gameUI.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - screenSizeInBytes);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - screenSizeInBytes);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - screenSizeInBytes);
        }
    }
}
