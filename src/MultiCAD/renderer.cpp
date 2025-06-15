#include "pch.h"
#include "cad.h"
#include "renderer.h"

constexpr S32 DEFAULT_FONT_ASSET_SPACING = 2;

constexpr U32 PIXEL_COLOR_BIT_MASK = 0x8000;
constexpr S32 STENCIL_PIXEL_COLOR_SHIFT = 5;
constexpr S32 STENCIL_PIXEL_COLOR_VALUE = 32;
constexpr S32 STENCIL_PIXEL_MASK_VALUE = 0xFFFB;

#ifdef _DEBUG
#include <format>
#include <cassert>

#define CHECK_ASSERTS
#endif

RendererStateContainer g_rendererState;



// 0x10001000
void initValues()
{
    g_moduleState.surface.offset = 0;
    g_moduleState.surface.y = SCREEN_HEIGHT;
    g_moduleState.surface.width = SCREEN_WIDTH;
    g_moduleState.surface.height = SCREEN_HEIGHT;
    g_moduleState.surface.stride = SCREEN_WIDTH * sizeof(Pixel);

    g_moduleState.windowRect.x = 0;
    g_moduleState.windowRect.y = 0;
    g_moduleState.windowRect.width = SCREEN_WIDTH - 1;
    g_moduleState.windowRect.height = SCREEN_HEIGHT - 1;
}

// 0x10001050
bool initDxInstance(const HWND hwnd, const bool fullscreen)
{
    restoreDxInstance();

    if (FAILED(DirectDrawCreate(NULL, &g_moduleState.directX.instance, NULL)))
    {
        return false;
    }

    if (FAILED(g_moduleState.directX.instance->SetCooperativeLevel(hwnd,
        fullscreen ? (DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN) : DDSCL_NORMAL)))
    {
        return false;
    }

    g_moduleState.isFullScreen = fullscreen;
    g_moduleState.hwnd = hwnd;

    return true;
}

// 0x100010b0
void releaseDxSurface()
{
    dxRelease(g_moduleState.directX.surface);
}

// 0x100010d0
void restoreDxInstance()
{
    releaseDxSurface();

    if (!g_moduleState.directX.instance)
        return;

    if (g_moduleState.isFullScreen)
        g_moduleState.directX.instance->RestoreDisplayMode();

    dxRelease(g_moduleState.directX.instance);
}

// 0x10001110
void releaseDxInstance()
{
    releaseDxSurface();

    dxRelease(g_moduleState.directX.instance);
}

// 0x10001130
void setPixelColorMasks(const U32 r, const U32 g, const U32 b)
{
    g_moduleState.actualRedMask = static_cast<U16>(r);
    g_moduleState.initialRedMask = static_cast<U16>(r);
    g_moduleState.actualGreenMask = static_cast<U16>(g);
    g_moduleState.initialGreenMask = static_cast<U16>(g);
    g_moduleState.actualBlueMask = static_cast<U16>(b);
    g_moduleState.initialBlueMask = static_cast<U16>(b);

    const U32 rm = r & PixelColor::DEFAULT_MASK;
    const U32 gm = g & PixelColor::DEFAULT_MASK;
    const U32 bm = b & PixelColor::DEFAULT_MASK;

    // R
    {
        size_t x = 0;
        U32 mask = PIXEL_COLOR_BIT_MASK;

        for (; x < GRAPHICS_BITS_PER_PIXEL_16; ++x)
        {
            if (mask & rm)
            {
                g_moduleState.actualColorMask = g_moduleState.actualColorMask | static_cast<U16>(mask);
                break;
            }

            mask = mask >> 1;
        }

        g_moduleState.redOffset = static_cast<U16>(x);
    }

    // G
    {
        size_t x = 0;
        U32 mask = PIXEL_COLOR_BIT_MASK;

        for (; x < GRAPHICS_BITS_PER_PIXEL_16; ++x)
        {
            if (mask & gm)
            {
                g_moduleState.actualColorMask = g_moduleState.actualColorMask | static_cast<U16>(mask);
                break;
            }

            mask = mask >> 1;
        }

        g_moduleState.greenOffset = static_cast<U16>(x);
    }

    // B
    {
        size_t x = 0;
        U32 mask = PIXEL_COLOR_BIT_MASK;

        for (; x < GRAPHICS_BITS_PER_PIXEL_16; ++x)
        {
            if (mask & bm)
            {
                g_moduleState.actualColorMask = g_moduleState.actualColorMask | static_cast<U16>(mask);
                break;
            }

            mask = mask >> 1;
        }

        g_moduleState.blueOffset = static_cast<U16>(x);
    }


    // R
    {
        U32 mask = 1;

        for (size_t x = 0; x < GRAPHICS_BITS_PER_PIXEL_16; ++x)
        {
            if (mask & rm)
            {
                g_moduleState.actualColorBits = g_moduleState.actualColorBits | static_cast<U16>(mask);
                break;
            }

            mask = mask << 1;
        }
    }

    // G
    {
        U32 mask = 1;

        for (size_t x = 0; x < GRAPHICS_BITS_PER_PIXEL_16; ++x)
        {
            if (mask & gm)
            {
                g_moduleState.actualColorBits = g_moduleState.actualColorBits | static_cast<U16>(mask);
                break;
            }

            mask = mask << 1;
        }
    }

    // B
    {
        U32 mask = 1;

        for (size_t x = 0; x < GRAPHICS_BITS_PER_PIXEL_16; ++x)
        {
            if (mask & bm)
            {
                g_moduleState.actualColorBits = g_moduleState.actualColorBits | static_cast<U16>(mask);
                break;
            }

            mask = mask << 1;
        }
    }

    g_moduleState.initialColorMask = g_moduleState.actualColorMask;

    g_moduleState.unk23 = ~g_moduleState.actualColorMask;
    g_moduleState.unk24 = ~g_moduleState.actualColorMask;

    g_moduleState.unk18 = g_moduleState.actualColorBits;
    g_moduleState.shadeColorMask = ~g_moduleState.actualColorBits;
    g_moduleState.unk22 = ~g_moduleState.actualColorBits;

    g_moduleState.backSurfaceShadePixel =
        ((5 << (11 - g_moduleState.blueOffset)) + (2 << (11 - g_moduleState.greenOffset))) & PixelColor::DEFAULT_MASK;
    g_moduleState.backSurfaceShadePixel = (g_moduleState.backSurfaceShadePixel << 16) | g_moduleState.backSurfaceShadePixel;

    if (g_moduleState.greenOffset < g_moduleState.redOffset)
    {
        if (g_moduleState.blueOffset < g_moduleState.greenOffset)
        {
            g_moduleState.initialRgbMask = (gm << 16) | bm | rm;
        }
        else if (g_moduleState.blueOffset <= g_moduleState.redOffset)
        {
            g_moduleState.initialRgbMask = (bm << 16) | gm | rm;
        }
        else
        {
            g_moduleState.initialRgbMask = (rm << 16) | bm | gm;
        }
    }
    else
    {
        if (g_moduleState.greenOffset <= g_moduleState.blueOffset)
        {
            g_moduleState.initialRgbMask = (gm << 16) | bm | rm;
        }
        else if (g_moduleState.redOffset < g_moduleState.blueOffset)
        {
            g_moduleState.initialRgbMask = (bm << 16) | gm | rm;
        }
        else
        {
            g_moduleState.initialRgbMask = (rm << 16) | bm | gm;
        }
    }

    g_moduleState.actualRgbMask = ((g_moduleState.actualColorMask << 16) | g_moduleState.actualColorMask) & g_moduleState.initialRgbMask;
}

// 0x10001330
bool initWindowDxSurface(S32 width, S32 height)
{
    releaseDxSurface();

    if (g_moduleState.isFullScreen)
    {
        if (FAILED(g_moduleState.directX.instance->SetDisplayMode(width, height, GRAPHICS_BITS_PER_PIXEL_16)))
        {
            return false;
        }

        SetWindowPos(g_moduleState.hwnd, NULL, 0, 0, width, height, SWP_NOZORDER | SWP_NOMOVE);
    }
    else
    {
        HDC hdc = GetDC(g_moduleState.hwnd);

        const U32 sw = GetDeviceCaps(hdc, HORZRES);
        const U32 sh = GetDeviceCaps(hdc, VERTRES);

        ReleaseDC(g_moduleState.hwnd, hdc);

        SetWindowLongA(g_moduleState.hwnd, GWL_STYLE, WS_CAPTION);

        RECT rect;
        ZeroMemory(&rect, sizeof(RECT));
        AdjustWindowRect(&rect, WS_CAPTION, false);

        width = width + (rect.right - rect.left);
        height = height + (rect.bottom - rect.top);

        SetWindowPos(g_moduleState.hwnd, NULL, (sw - width) / 2, (sh - height) / 2, width, height, SWP_SHOWWINDOW);
    }

    DDSURFACEDESC desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC));

    desc.dwSize = sizeof(DDSURFACEDESC);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    if (FAILED(g_moduleState.directX.instance->CreateSurface(&desc, &g_moduleState.directX.surface, NULL)))
    {
        return false;
    }

    if (FAILED(g_moduleState.directX.surface->GetSurfaceDesc(&desc)))
    {
        return false;
    }

    setPixelColorMasks(desc.ddpfPixelFormat.dwRBitMask, desc.ddpfPixelFormat.dwGBitMask, desc.ddpfPixelFormat.dwBBitMask);

    if (!g_moduleState.isFullScreen)
    {
        LPDIRECTDRAWCLIPPER clipper = NULL;

        if (FAILED(g_moduleState.directX.instance->CreateClipper(0, &clipper, NULL)))
        {
            dxRelease(g_moduleState.directX.surface);
            return false;
        }

        if (FAILED(clipper->SetHWnd(0, g_moduleState.hwnd)))
        {
            dxRelease(clipper);
            dxRelease(g_moduleState.directX.surface);
            return false;
        }

        if (FAILED(g_moduleState.directX.surface->SetClipper(clipper)))
        {
            dxRelease(clipper);
            dxRelease(g_moduleState.directX.surface);
            return false;
        }

        dxRelease(clipper);
    }

    g_moduleState.actions.initValues();

    g_moduleState.windowRect.x = 0;
    g_moduleState.windowRect.y = 0;
    g_moduleState.windowRect.width = width - 1;
    g_moduleState.windowRect.height = height - 1;

    return true;
}



// 0x10001420
void drawMainSurfaceHorLine(const S32 x, const S32 y, const S32 length, const Pixel pixel)
{
    S32 max_x = x + length - 1;
    S32 new_x = x;

    if (y >= g_moduleState.windowRect.y
        && y <= g_moduleState.windowRect.height)
    {
        if (new_x < g_moduleState.windowRect.x)
            new_x = g_moduleState.windowRect.x;
        if (g_moduleState.windowRect.width < max_x)
            max_x = g_moduleState.windowRect.width;

        if (new_x <= max_x)
        {
            Pixel* pixels = (Pixel*)((Addr)g_rendererState.surfaces.main
                + (Addr)((g_moduleState.surface.offset + y * SCREEN_WIDTH + new_x) * sizeof(Pixel)));

            if (g_moduleState.surface.y <= y)
                pixels = (Pixel*)((Addr)pixels - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

            std::fill(pixels, pixels + (max_x - new_x + 1), pixel);
        }
    }
}

// 0x100014b0
void drawMainSurfaceVertLine(const S32 x, const S32 y, const S32 height, const Pixel pixel)
{
    S32 max_y = height + y - 1;
    S32 new_y = y;

    if (x < g_moduleState.windowRect.x
        || g_moduleState.windowRect.width < y)
        return;

    if (y < g_moduleState.windowRect.y)
        new_y = g_moduleState.windowRect.y;
    if (max_y > g_moduleState.windowRect.height)
        max_y = g_moduleState.windowRect.height;

    max_y += 1 - new_y;

    if (max_y < 1)
        return;

    Pixel* pixels = (Pixel*)((Addr)g_rendererState.surfaces.main
        + (Addr)((g_moduleState.surface.offset + new_y * SCREEN_WIDTH + x) * sizeof(Pixel)));

    if (y < g_moduleState.surface.y)
    {
        const S32 delta = new_y + max_y - g_moduleState.surface.y;

        if (delta < 1)
        {
            for (S32 xx = 0; xx < max_y; ++xx)
            {
                pixels[0] = pixel;

                pixels = (Pixel*)((Addr)pixels + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
            }
        }
        else
        {
            for (S32 xx = 0; xx < max_y - delta; ++xx)
            {
                pixels[0] = pixel;

                pixels = (Pixel*)((Addr)pixels + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
            }

            pixels = (Pixel*)((Addr)pixels - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

            for (S32 xx = 0; xx < delta; ++xx)
            {
                pixels[0] = pixel;

                pixels = (Pixel*)((Addr)pixels + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
            }
        }
    }
    else
    {
        pixels = (Pixel*)((Addr)pixels - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

        for (S32 xx = 0; xx < max_y; ++xx)
        {
            pixels[0] = pixel;

            pixels = (Pixel*)((Addr)pixels + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
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
    if (x < g_moduleState.windowRect.x)
    {
        width += x - g_moduleState.windowRect.x;
        x = g_moduleState.windowRect.x;
    }

    if (y < g_moduleState.windowRect.y)
    {
        height += y - g_moduleState.windowRect.y;
        y = g_moduleState.windowRect.y;
    }

    if ((width + x - 1) > g_moduleState.windowRect.width)
        width = g_moduleState.windowRect.width - x + 1;

    if ((height + y - 1) > g_moduleState.windowRect.height)
        height = g_moduleState.windowRect.height - y + 1;

    if (width < 1 || height < 1)
        return;

    Pixel* pixels = (Pixel*)((Addr)g_rendererState.surfaces.main + (Addr)((g_moduleState.surface.offset + y * SCREEN_WIDTH + x) * sizeof(Pixel)));

    if (y < g_moduleState.surface.y)
    {
        const S32 delta = y + height - g_moduleState.surface.y;

        if (y + height < g_moduleState.surface.y || delta == 0)
        {
            for (S32 yy = 0; yy < height; ++yy)
            {
                std::fill(pixels, pixels + width, pixel);

                pixels = (Pixel*)((Addr)pixels + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
            }
        }
        else
        {
            for (S32 yy = 0; yy < height - delta; ++yy)
            {
                std::fill(pixels, pixels + width, pixel);

                pixels = (Pixel*)((Addr)pixels + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
            }

            pixels = (Pixel*)((Addr)pixels - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

            for (S32 yy = 0; yy < delta; ++yy)
            {
                std::fill(pixels, pixels + width, pixel);

                pixels = (Pixel*)((Addr)pixels + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
            }
        }
    }
    else
    {
        pixels = (Pixel*)((Addr)pixels - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

        for (S32 yy = 0; yy < height; ++yy)
        {
            std::fill(pixels, pixels + width, pixel);

            pixels = (Pixel*)((Addr)pixels + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
        }
    }
}

// 0x100016c0 
void drawMainSurfaceShadeColorRect(S32 x, S32 y, S32 width, S32 height, const Pixel pixel)
{
    if (x < g_moduleState.windowRect.x)
    {
        width = width + x - g_moduleState.windowRect.x;
        x = g_moduleState.windowRect.x;

        if (width < 1)
            return;
    }

    if (y < g_moduleState.windowRect.y)
    {
        height = height + y - g_moduleState.windowRect.y;
        y = g_moduleState.windowRect.y;

        if (height < 1)
            return;
    }

    if ((width + x - 1) > g_moduleState.windowRect.width)
        width = g_moduleState.windowRect.width - x + 1;

    if ((height + y - 1) > g_moduleState.windowRect.height)
        height = g_moduleState.windowRect.height - y + 1;

    if (width < 1 || height < 1)
        return;

    Pixel* pixels = (Pixel*)((Addr)g_rendererState.surfaces.main + (Addr)((g_moduleState.surface.offset + y * SCREEN_WIDTH + x) * sizeof(Pixel)));

    const Pixel color = SHADEPIXEL(pixel, g_moduleState.shadeColorMask);

    if (y < g_moduleState.surface.y)
    {
        const S32 delta = y + height - g_moduleState.surface.y;

        if ((y + height) < g_moduleState.surface.y || delta == 0)
        {
            for (S32 yy = 0; yy < height; ++yy)
            {
                for (S32 xx = 0; xx < width; ++xx)
                {
                    pixels[xx] = SHADEPIXEL(pixels[xx], g_moduleState.shadeColorMask) + color;
                }

                pixels = (Pixel*)((Addr)pixels + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
            }
        }
        else
        {
            for (S32 yy = 0; yy < height - delta; ++yy)
            {
                for (S32 xx = 0; xx < width; ++xx)
                {
                    pixels[xx] = SHADEPIXEL(pixels[xx], g_moduleState.shadeColorMask) + color;
                }

                pixels = (Pixel*)((Addr)pixels + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
            }

            pixels = (Pixel*)((Addr)pixels - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

            for (S32 yy = 0; yy < delta; ++yy)
            {
                for (S32 xx = 0; xx < width; ++xx)
                {
                    pixels[xx] = SHADEPIXEL(pixels[xx], g_moduleState.shadeColorMask) + color;
                }

                pixels = (Pixel*)((Addr)pixels + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
            }
        }
    }
    else
    {
        pixels = (Pixel*)((Addr)pixels - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

        for (S32 yy = 0; yy < height; ++yy)
        {
            for (S32 xx = 0; xx < width; ++xx)
            {
                pixels[xx] = SHADEPIXEL(pixels[xx], g_moduleState.shadeColorMask) + color;
            }

            pixels = (Pixel*)((Addr)pixels + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
        }
    }
}

// 0x100017e0
void drawMainSurfaceColorPoint(const S32 x, const S32 y, const Pixel pixel)
{
    if (x > g_moduleState.windowRect.x
        && y > g_moduleState.windowRect.y
        && x <= g_moduleState.windowRect.width
        && y <= g_moduleState.windowRect.height)
    {
        S32 offset = g_moduleState.surface.offset + y * SCREEN_WIDTH + x;

        if (g_moduleState.surface.y <= y)
            offset -= SCREEN_SIZE_IN_PIXELS;

        g_rendererState.surfaces.main[offset] = pixel;
    }
}

// 0x10001840 
void drawBackSurfaceColorPoint(const S32 x, const S32 y, const Pixel pixel)
{
    if (x > g_moduleState.windowRect.x
        && y > g_moduleState.windowRect.y
        && x <= g_moduleState.windowRect.width
        && y <= g_moduleState.windowRect.height)
    {
        S32 offset = g_moduleState.surface.offset + y * SCREEN_WIDTH + x;

        if (g_moduleState.surface.y <= y)
            offset -= SCREEN_SIZE_IN_PIXELS;

        g_rendererState.surfaces.back[offset] = pixel;
    }
}

// 0x100018a0
void readMainSurfaceRect(const S32 sx, const S32 sy, const S32 width, const S32 height, const S32 dx, const S32 dy, const S32 stride, Pixel* surface)
{
    Pixel* src = (Pixel*)((Addr)g_rendererState.surfaces.main + (Addr)((g_moduleState.surface.offset + (sy * SCREEN_WIDTH + sx)) * sizeof(Pixel)));
    Pixel* dst = (Pixel*)((Addr)surface + (Addr)((stride * dy + dx) * sizeof(Pixel)));

    if (sy < g_moduleState.surface.y)
    {
        const S32 delta = sy + height - g_moduleState.surface.y;

        if ((sy + height) < g_moduleState.surface.y || delta == 0)
        {
            for (S32 yy = 0; yy < height; ++yy)
            {
                std::memcpy(dst, src, width);
                src = (Pixel*)((Addr)src + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                dst = (Pixel*)((Addr)dst + (Addr)(stride * sizeof(Pixel)));
            }
        }
        else
        {
            for (S32 yy = 0; yy < height - delta; ++yy)
            {
                std::memcpy(dst, src, width);
                src = (Pixel*)((Addr)src + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                dst = (Pixel*)((Addr)dst + (Addr)(stride * sizeof(Pixel)));
            }

            src = (Pixel*)((Addr)src - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

            for (S32 yy = 0; yy < delta; ++yy)
            {
                std::memcpy(dst, src, width);
                src = (Pixel*)((Addr)src + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                dst = (Pixel*)((Addr)dst + (Addr)(stride * sizeof(Pixel)));
            }
        }
    }
    else
    {
        src = (Pixel*)((Addr)src - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

        for (S32 yy = 0; yy < height; ++yy)
        {
            std::memcpy(dst, src, width);
            src = (Pixel*)((Addr)src + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
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

        const Pixel r = (((pixel & 0xF800) << 0) >> (g_moduleState.redOffset & 0x1F)) & g_moduleState.actualRedMask;
        const Pixel g = (((pixel & 0x07E0) << 5) >> (g_moduleState.greenOffset & 0x1F)) & g_moduleState.actualGreenMask;
        const Pixel b = (((pixel & 0x001F) << 11) >> (g_moduleState.blueOffset & 0x1F)) & g_moduleState.actualBlueMask;

        output[x] = (Pixel)(r | g | b);
    }
}

// 0x10001c80
void convertAllColors(const Pixel* input, Pixel* output, const S32 count)
{
    for (S32 x = 0; x < count; ++x)
    {
        const Pixel pixel = input[x];

        const Pixel r = (((pixel & 0xF800) << 0) >> (g_moduleState.redOffset & 0x1F)) & g_moduleState.actualRedMask;
        const Pixel g = (((pixel & 0x07E0) << 5) >> (g_moduleState.greenOffset & 0x1F)) & g_moduleState.actualGreenMask;
        const Pixel b = (((pixel & 0x001F) << 11) >> (g_moduleState.blueOffset & 0x1F)) & g_moduleState.actualBlueMask;

        output[x] = (Pixel)(r | g | b);
    }
}

// 0x10001d00
void copyMainBackSurfaces(const S32 dx, const S32 dy)
{
    Pixel* src;
    Pixel* dst;

    S32 offset = g_moduleState.surface.offset + dy * SCREEN_WIDTH + dx;
    if (offset < 0)
    {
        do
        {
            offset += SCREEN_SIZE_IN_PIXELS;
        } while (offset < 0);
    }
    else
    {
        while (offset >= SCREEN_SIZE_IN_PIXELS)
            offset -= SCREEN_SIZE_IN_PIXELS;
    }

    S32 x_max = dx + (g_moduleState.surface.offset * sizeof(Pixel) / 2) % SCREEN_WIDTH;
    do
    {
        if (x_max >= 0)
        {
            if (x_max < SCREEN_WIDTH)
                break;

            src = g_moduleState.surface.back + SCREEN_SIZE_IN_PIXELS;
            dst = g_moduleState.surface.back;
            std::memcpy(dst, src, SCREEN_WIDTH * sizeof(Pixel));

            src = g_moduleState.surface.main + SCREEN_SIZE_IN_PIXELS;
            dst = g_moduleState.surface.main;
            std::memcpy(dst, src, SCREEN_WIDTH * sizeof(Pixel));

            src = g_moduleState.surface.stencil + SCREEN_SIZE_IN_PIXELS;
            dst = g_moduleState.surface.stencil;
        }
        else
        {
            src = g_moduleState.surface.back;
            dst = g_moduleState.surface.back + SCREEN_SIZE_IN_PIXELS;
            std::memcpy(dst, src, SCREEN_WIDTH * sizeof(Pixel));

            src = g_moduleState.surface.main;
            dst = g_moduleState.surface.main + SCREEN_SIZE_IN_PIXELS;
            std::memcpy(dst, src, SCREEN_WIDTH * sizeof(Pixel));

            src = g_moduleState.surface.stencil;
            dst = g_moduleState.surface.stencil + SCREEN_SIZE_IN_PIXELS;
        }
        std::memcpy(dst, src, SCREEN_WIDTH * sizeof(Pixel));
    } while (false);

#ifdef CHECK_ASSERTS
    {
        S32 old_offset = g_moduleState.surface.offset + dy * SCREEN_WIDTH + dx;
        // Normalize offset so it is within the expected range.
        if (old_offset < 0)
        {
            while (old_offset < 0)
            {
                old_offset = old_offset + (SCREEN_WIDTH * SCREEN_HEIGHT);
            }
        }
        else
        {
            while (old_offset >= SCREEN_WIDTH * SCREEN_HEIGHT)
            {
                old_offset = old_offset - (SCREEN_WIDTH * SCREEN_HEIGHT);
            }
        }

        S32 length = g_moduleState.surface.offset & (0x80000000 | (SCREEN_WIDTH - 1));

        if (length < 0) { length = ((length - 1) | (U32)(-(S32)SCREEN_WIDTH)) + 1; }

        CONST S32 lines = (old_offset + ((old_offset >> 0x1f) & (SCREEN_WIDTH - 1))) / SCREEN_WIDTH;

        assert(length + dx == x_max && std::format("{} {}", length + dx, x_max).c_str());
        assert(old_offset == offset && std::format("{} {}", old_offset, offset).c_str());
        assert(SCREEN_HEIGHT - lines == SCREEN_HEIGHT - offset / SCREEN_WIDTH && std::format("{} {}", SCREEN_HEIGHT - lines, SCREEN_HEIGHT - offset / SCREEN_WIDTH).c_str());

    }
#endif

    g_moduleState.surface.offset = offset;
    g_moduleState.surface.y = SCREEN_HEIGHT - offset / SCREEN_WIDTH;

    if (dy <= 0)
    {
        if (dy < 0)
        {
            if (dx <= 0)
                moveStencilSurface(-dx, -dy, dx + SCREEN_WIDTH, dy + SCREEN_HEIGHT, -dy);
            else
                moveStencilSurface(0, -dy, SCREEN_WIDTH - dx, dy + SCREEN_HEIGHT, -dy);
        }
    }
    else
    {
        if (dx <= 0)
            moveStencilSurface(-dx, 0, dx + SCREEN_WIDTH, SCREEN_HEIGHT - dy, -dy);
        else
            moveStencilSurface(0, 0, SCREEN_WIDTH - dx, SCREEN_HEIGHT - dy, -dy);
    }
}

// 0x10001e90
void callDrawBackSurfaceRhomb(S32 tx, S32 ty, S32 angle_0, S32 angle_1, S32 angle_2, S32 angle_3, U8* input)
{
    // This check was added since in the original Cad value 2 * g_moduleState.surface.width is passed to the next called function
#ifdef CHECK_ASSERTS
    assert(g_moduleState.surface.width * static_cast<S32>(sizeof(Pixel)) == g_moduleState.surface.stride);
#endif
    drawBackSurfaceRhomb(angle_0, angle_1, angle_2, angle_3, tx, ty, g_moduleState.surface.stride, input, g_rendererState.surfaces.back);
}

// 0x10001ed0
void FUN_10001ed0(S32 param_1, S32 param_2, S32 param_3, S32 param_4, S32 param_5, S32 param_6)
{
}

// 0x10001f10
void FUN_10001f10(S32 param_1, S32 param_2, S32 param_3)
{
}

// 0x10001f40
void FUN_10001f40(S32 param_1, S32 param_2, S32 param_3, S32 param_4, S32 param_5, S32 param_6, S32 param_7)
{
}

// 0x10001f80
void copyBackToMainSurfaceRect(const S32 x, const S32 y, const U32 width, const U32 height)
{
    const Addr offset = (Addr)(x + y * SCREEN_WIDTH);
    Pixel* src = (Pixel*)((Addr)g_rendererState.surfaces.back + (Addr)((g_moduleState.surface.offset + offset) * sizeof(Pixel)));
    Pixel* dst = (Pixel*)((Addr)g_rendererState.surfaces.main + (Addr)((g_moduleState.surface.offset + offset) * sizeof(Pixel)));

    auto copyBlock = [&](Pixel*& s, Pixel*& d, int lines)
        {
            while (lines--)
            {
                std::memcpy(d, s, width * sizeof(Pixel));
                s = (Pixel*)((Addr)s + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                d = (Pixel*)((Addr)d + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
            }
        };

    if (y < g_moduleState.surface.y)
    {
        const S32 delta = y + height - g_moduleState.surface.y;
        if (delta <= 0)
        {
            copyBlock(src, dst, height);
        }
        else
        {
            copyBlock(src, dst, height - delta);
            src = (Pixel*)((Addr)src - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            dst = (Pixel*)((Addr)dst - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            copyBlock(src, dst, delta);
        }
    }
    else
    {
        src = (Pixel*)((Addr)src - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
        dst = (Pixel*)((Addr)dst - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
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
    const S32 offset = g_moduleState.surface.offset % 1024;

    Pixel* src = (Pixel*)((Addr)g_rendererState.surfaces.main
        + (Addr)((offset + SCREEN_SIZE_IN_PIXELS) * sizeof(Pixel)));

    g_rendererState.outline.width = g_moduleState.windowRect.width - g_moduleState.windowRect.x;
    g_rendererState.outline.height = g_moduleState.windowRect.height + 1 - g_moduleState.windowRect.y;
    g_rendererState.outline.options = OUTLINE_SKIP_OPTIONS_NONE;

    x = x - g_moduleState.windowRect.x;
    y = y - g_moduleState.windowRect.y;

    if (y < 0)
    {
        height = height + y;

        if (height <= 0)
            return;

        y = 0;
        g_rendererState.outline.options = (OutlineSkipOptions)(g_rendererState.outline.options | OUTLINE_SKIP_OPTIONS_TOP);
    }

    if (y >= g_rendererState.outline.height)
    {
        height = height + y - (g_rendererState.outline.height - 1);

        if (height >= 0)
            return;

        y = g_rendererState.outline.height - 1;
        g_rendererState.outline.options = (OutlineSkipOptions)(g_rendererState.outline.options | OUTLINE_SKIP_OPTIONS_TOP);
    }

    {
        const S32 max = y + 1 + height;

        if (y + 1 + height < 0 != max < 0)
        {
            height = height - max - 1;
            g_rendererState.outline.options = (OutlineSkipOptions)(g_rendererState.outline.options | OUTLINE_SKIP_OPTIONS_BOTTOM);
        }
    }

    {
        const S32 max = y - 1 + height;

        if (max >= g_rendererState.outline.height)
        {
            height = height + g_rendererState.outline.height - max;
            g_rendererState.outline.options = (OutlineSkipOptions)(g_rendererState.outline.options | OUTLINE_SKIP_OPTIONS_BOTTOM);
        }
    }

    if (x < 1)
    {
        width = width + x;

        if (width < 2)
            return;

        x = 1;
        width = width - 1;

        g_rendererState.outline.options = (OutlineSkipOptions)(g_rendererState.outline.options | OUTLINE_SKIP_OPTIONS_LEFT);
    }

    if (x >= g_rendererState.outline.width + 2)
    {
        width = width + x + 1 - (g_rendererState.outline.width + 2);
        if (width >= 0)
            return;
        x = g_rendererState.outline.width + 1;
        g_rendererState.outline.options = (OutlineSkipOptions)(g_rendererState.outline.options | OUTLINE_SKIP_OPTIONS_LEFT);
    }

    if (x + width <= 0 != x + width < 0)
    {
        width = width - x - width;

        g_rendererState.outline.options = (OutlineSkipOptions)(g_rendererState.outline.options | OUTLINE_SKIP_OPTIONS_RIGHT);
    }

    {
        S32 max = x - 2 + width;

        if (max > g_rendererState.outline.width)
        {
            width = width + g_rendererState.outline.width - max;
            g_rendererState.outline.options = (OutlineSkipOptions)(g_rendererState.outline.options | OUTLINE_SKIP_OPTIONS_RIGHT);
        }
    }

    // Offset in bytes to the next changed pixel. Since we use Pixel*, I changed it to 1
    g_rendererState.outline.horizontalStride = 1;
    if (width < 0)
    {
        g_rendererState.outline.horizontalDirection = -g_rendererState.outline.horizontalDirection;
        width = -width;
    }

    g_rendererState.outline.verticalDirection = 1;
    g_rendererState.outline.stride = g_moduleState.surface.stride;
    if (height < 0)
    {
        g_rendererState.outline.stride = -g_moduleState.surface.stride;
        height = -height;
        g_rendererState.outline.verticalDirection = -g_rendererState.outline.verticalDirection;
    }

    Pixel* dst = (Pixel*)((Addr)g_rendererState.surfaces.main
        + (Addr)(g_moduleState.surface.offset * sizeof(Pixel) + y * g_moduleState.surface.stride + x * sizeof(Pixel))
        + (Addr)(g_moduleState.windowRect.y * g_moduleState.surface.stride + g_moduleState.windowRect.x * sizeof(Pixel)));

    if ((g_rendererState.outline.options & OUTLINE_SKIP_OPTIONS_TOP) == OUTLINE_SKIP_OPTIONS_NONE)
    {
        height = height - 1;

        Pixel* pixels = dst;

        if (src <= dst)
        {
            pixels = (Pixel*)((Addr)dst - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
        }

        for (S32 xx = 0; xx < width; ++xx)
        {
            pixels[g_rendererState.outline.horizontalDirection * xx] = pixel;
        }

        dst = (Pixel*)((Addr)dst + (Addr)g_rendererState.outline.stride);
    }

    if ((g_rendererState.outline.options & OUTLINE_SKIP_OPTIONS_RIGHT) == OUTLINE_SKIP_OPTIONS_NONE)
    {
        S32 off = (width - 1) * sizeof(Pixel);
        if (g_rendererState.outline.horizontalDirection < 0)
        {
            off = -off;
        }

        Pixel* pixels = (Pixel*)((Addr)dst + (Addr)off);

        for (S32 yy = 0; yy < height - 1; ++yy)
        {
            if (src <= pixels)
            {
                pixels = (Pixel*)((Addr)pixels - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            }

            pixels[0] = pixel;

            pixels = (Pixel*)((Addr)pixels + (Addr)g_rendererState.outline.stride);
        }
    }

    if ((g_rendererState.outline.options & OUTLINE_SKIP_OPTIONS_LEFT) == OUTLINE_SKIP_OPTIONS_NONE)
    {
        for (S32 yy = 0; yy < height - 1; ++yy)
        {
            if (src <= dst)
            {
                dst = (Pixel*)((Addr)dst - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            }

            dst[0] = pixel;

            dst = (Pixel*)((Addr)dst + (Addr)g_rendererState.outline.stride);
        }
    }
    else
    {
        dst = (Pixel*)((Addr)dst + (Addr)(g_rendererState.outline.stride * (height - 1)));
    }

    if (height != 0 && (g_rendererState.outline.options & OUTLINE_SKIP_OPTIONS_BOTTOM) == OUTLINE_SKIP_OPTIONS_NONE)
    {
        if (src <= dst)
        {
            dst = (Pixel*)((Addr)dst - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
        }

        for (S32 xx = 0; xx < width; ++xx)
        {
            dst[g_rendererState.outline.horizontalDirection * xx] = pixel;
        }
    }
}

// 0x100026e0
void drawStencilSurfaceWindowRect()
{
    Pixel* pixels = (Pixel*)((Addr)g_rendererState.surfaces.stencil + (Addr)(g_moduleState.surface.offset + g_moduleState.windowRect.y * SCREEN_WIDTH + g_moduleState.windowRect.x) * (Addr)sizeof(Pixel));

    const S32 height = g_moduleState.windowRect.height - g_moduleState.windowRect.y + 1;
    const S32 width = g_moduleState.windowRect.width - g_moduleState.windowRect.x + 1;
    const S32 stride = (SCREEN_WIDTH - width) * sizeof(Pixel);

    Pixel pixel = (Pixel)(g_moduleState.windowRect.y * STENCIL_PIXEL_COLOR_VALUE);

    auto processRows = [&](S32 rows)
        {
            for (S32 i = 0; i < rows; ++i)
            {
                for (S32 j = 0; j < width; ++j)
                    pixels[j] = pixel;

                pixel = pixel + STENCIL_PIXEL_COLOR_VALUE;

                pixels = (Pixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
            }
        };

    if (g_moduleState.windowRect.y < g_moduleState.surface.y)
    {
        const S32 delta = g_moduleState.windowRect.y + height - g_moduleState.surface.y;

        if ((g_moduleState.windowRect.y + height) < g_moduleState.surface.y || delta == 0)
        {
            //processRows(height);          // todo: replace for loops with this lambda function

            for (S32 yy = 0; yy < height; ++yy)
            {
                for (S32 xx = 0; xx < width; ++xx)
                {
                    pixels[xx] = pixel;
                }

                pixel = pixel + STENCIL_PIXEL_COLOR_VALUE;

                pixels = (Pixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
            }
        }
        else
        {
            //processRows(height - delta);
            for (S32 yy = 0; yy < height - delta; ++yy)
            {
                for (S32 xx = 0; xx < width; ++xx) { pixels[xx] = pixel; }

                pixel = pixel + STENCIL_PIXEL_COLOR_VALUE;

                pixels = (Pixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
            }

            pixels = (Pixel*)((Addr)pixels - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

            //processRows(delta);
            for (S32 yy = 0; yy < delta; ++yy)
            {
                for (S32 xx = 0; xx < width; ++xx) { pixels[xx] = pixel; }

                pixel = pixel + STENCIL_PIXEL_COLOR_VALUE;

                pixels = (Pixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
            }
        }
    }
    else
    {
        pixels = (Pixel*)((Addr)pixels - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

        //processRows(height);
        for (S32 yy = 0; yy < height; ++yy)
        {
            for (S32 xx = 0; xx < width; ++xx) { pixels[xx] = pixel; }

            pixel = pixel + STENCIL_PIXEL_COLOR_VALUE;

            pixels = (Pixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
        }
    }
}

// 0x10002780
void maskStencilSurfaceRect(S32 x, S32 y, S32 width, S32 height)
{
    Pixel* pixels = (Pixel*)((Addr)g_rendererState.surfaces.stencil + (Addr)((g_moduleState.surface.offset + y * SCREEN_WIDTH + x) * sizeof(Pixel)));

    const S32 stride = (SCREEN_WIDTH - width) * sizeof(Pixel);

    const Pixel pixel = (Pixel)STENCIL_PIXEL_MASK_VALUE;

    auto processRows = [&](S32 rows)
        {
            for (S32 i = 0; i < rows; ++i)
            {
                for (S32 j = 0; j < width; ++j)
                    pixels[j] = pixels[j] & pixel;

                pixels = (Pixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
            }
        };

    if (y < g_moduleState.surface.y)
    {
        const S32 delta = y + height - g_moduleState.surface.y;

        if ((y + height) < g_moduleState.surface.y || delta == 0)
        {
            // processRows(height);         // todo: replace for loops with this lambda function
            for (S32 yy = 0; yy < height; ++yy)
            {
                for (S32 xx = 0; xx < width; ++xx)
                {
                    pixels[xx] = pixels[xx] & pixel;
                }

                pixels = (Pixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
            }
        }
        else
        {
            // processRows(height - delta);
            for (S32 yy = 0; yy < height - delta; ++yy)
            {
                for (S32 xx = 0; xx < width; ++xx) { pixels[xx] = pixels[xx] & pixel; }

                pixels = (Pixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
            }

            pixels = (Pixel*)((Addr)pixels - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

            // processRows(delta);
            for (S32 yy = 0; yy < delta; ++yy)
            {
                for (S32 xx = 0; xx < width; ++xx) { pixels[xx] = pixels[xx] & pixel; }

                pixels = (Pixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
            }
        }
    }
    else
    {
        pixels = (Pixel*)((Addr)pixels - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

        // processRows(height);
        for (S32 yy = 0; yy < height; ++yy)
        {
            for (S32 xx = 0; xx < width; ++xx) { pixels[xx] = pixels[xx] & pixel; }

            pixels = (Pixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
        }
    }
}

// 0x10002810
void moveStencilSurface(const S32 x, const S32 y, const S32 width, const S32 height, const S32 offset)
{
    DoublePixel* pixels = (DoublePixel*)((Addr)g_rendererState.surfaces.stencil + (Addr)((SCREEN_WIDTH * y + x + g_moduleState.surface.offset) * sizeof(Pixel)));
    const S32 stride = sizeof(Pixel) * (SCREEN_WIDTH - width);

    //const bool addOp = offset >= 0;
    //const Pixel pixel = (Pixel)((addOp ? offset : -offset) << STENCIL_PIXEL_COLOR_SHIFT);

    //DoublePixel pixelValue = ((DoublePixel)(pixel) << GRAPHICS_BITS_PER_PIXEL_16) | (DoublePixel)pixel;
    //if (addOp == false)
    //    pixelValue = (DoublePixel)(-(S32)pixelValue);

    //auto processRows = [&](S32 rows)
    //    {
    //        for (S32 i = 0; i < rows; ++i)
    //        {
    //            for (S32 j = 0; j < (width >> 1); ++j)
    //                *pixels++ += pixelValue;
    //            pixels = (DoublePixel*)((Addr)pixels + (Addr)stride);
    //        }
    //    };

    //if (y < g_moduleState.surface.y)
    //{
    //    if (height + y <= g_moduleState.surface.y)
    //    {
    //        // Entire region is before the surface.y
    //        processRows(height);
    //    }
    //    else
    //    {
    //        S32 beforeY = g_moduleState.surface.y - y;
    //        S32 afterY = (height + y) - g_moduleState.surface.y;

    //        // First process rows before the surface.y
    //        processRows(beforeY);

    //        // Update remaining height and pixels pointer
    //        pixels = (DoublePixel*)((Addr)pixels - (Addr)SCREEN_SIZE_IN_BYTES);
    //        processRows(afterY);
    //    }
    //}
    //else
    //{
    //    // Entire region is after the surface.y, so moving to the array beginning
    //    pixels = (DoublePixel*)((Addr)pixels - (Addr)SCREEN_SIZE_IN_BYTES);
    //    processRows(height);
    //}

    if (-1 < offset)
    {
        CONST Pixel pixel = (Pixel)(offset << STENCIL_PIXEL_COLOR_SHIFT);
        CONST DoublePixel pix = ((DoublePixel)(pixel) << GRAPHICS_BITS_PER_PIXEL_16) | (DoublePixel)pixel;

        if (y < g_moduleState.surface.y)
        {
            CONST S32 delta = y + height - g_moduleState.surface.y;

            if ((y + height) < g_moduleState.surface.y || delta == 0)
            {
                for (S32 yy = 0; yy < height; yy++)
                {
                    for (S32 xx = 0; xx < width / 2; xx++) { pixels[xx] = pixels[xx] + pix; }

                    pixels = (DoublePixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
                }
            }
            else
            {
                for (S32 yy = 0; yy < height - delta; yy++)
                {
                    for (S32 xx = 0; xx < width / 2; xx++) { pixels[xx] = pixels[xx] + pix; }

                    pixels = (DoublePixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
                }

                pixels = (DoublePixel*)((Addr)pixels - (Addr)(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Pixel)));

                for (S32 yy = 0; yy < delta; yy++)
                {
                    for (S32 xx = 0; xx < width / 2; xx++) { pixels[xx] = pixels[xx] + pix; }

                    pixels = (DoublePixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
                }
            }
        }
        else
        {
            pixels = (DoublePixel*)((Addr)pixels - (Addr)(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Pixel)));

            for (S32 yy = 0; yy < height; yy++)
            {
                for (S32 xx = 0; xx < width / 2; xx++) { pixels[xx] = pixels[xx] + pix; }

                pixels = (DoublePixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
            }
        }
    }
    else
    {
        CONST Pixel pixel = (Pixel)(-offset << STENCIL_PIXEL_COLOR_SHIFT);
        CONST DoublePixel pix = ((DoublePixel)(pixel) << GRAPHICS_BITS_PER_PIXEL_16) | (DoublePixel)pixel;

        if (y < g_moduleState.surface.y)
        {
            CONST S32 delta = y + height - g_moduleState.surface.y;

            if ((y + height) < g_moduleState.surface.y || delta == 0)
            {
                for (S32 yy = 0; yy < height; yy++)
                {
                    for (S32 xx = 0; xx < width / 2; xx++) { pixels[xx] = pixels[xx] - pix; }

                    pixels = (DoublePixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
                }
            }
            else
            {
                for (S32 yy = 0; yy < height - delta; yy++)
                {
                    for (S32 xx = 0; xx < width / 2; xx++) { pixels[xx] = pixels[xx] - pix; }

                    pixels = (DoublePixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
                }

                pixels = (DoublePixel*)((Addr)pixels - (Addr)(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Pixel)));

                for (S32 yy = 0; yy < delta; yy++)
                {
                    for (S32 xx = 0; xx < width / 2; xx++) { pixels[xx] = pixels[xx] - pix; }

                    pixels = (DoublePixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
                }
            }
        }
        else
        {
            pixels = (DoublePixel*)((Addr)pixels - (Addr)(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Pixel)));

            for (S32 yy = 0; yy < height; yy++)
            {
                for (S32 xx = 0; xx < width / 2; xx++) { pixels[xx] = pixels[xx] - pix; }

                pixels = (DoublePixel*)((Addr)pixels + (Addr)(width * sizeof(Pixel) + stride));
            }
        }
    }
}

// 0x100028f0
bool lockDxSurface()
{
    DDSURFACEDESC desc;
    ZeroMemory(&desc, sizeof(DDSURFACEDESC));
    desc.dwSize = sizeof(DDSURFACEDESC);

    HRESULT result = g_moduleState.directX.surface->Lock(NULL, &desc, DDLOCK_WAIT, NULL);

    while (true)
    {
        if (SUCCEEDED(result))
        {
            g_moduleState.pitch = desc.lPitch;

            U32 offset = 0;

            if (!g_moduleState.isFullScreen)
            {
                RECT rect;
                ZeroMemory(&rect, sizeof(RECT));
                GetClientRect(g_moduleState.hwnd, &rect);

                POINT point;
                ZeroMemory(&point, sizeof(POINT));
                ClientToScreen(g_moduleState.hwnd, &point);

                OffsetRect(&rect, point.x, point.y);

                offset = desc.lPitch * rect.top + rect.left * sizeof(Pixel);
            }

            g_moduleState.surface.renderer = (LPVOID)((Addr)desc.lpSurface + (Addr)offset);

            return true;
        }

        if (result != DDERR_SURFACEBUSY && result != DDERR_SURFACELOST)
            if (FAILED(g_moduleState.directX.surface->Restore()))
                break;

        result = g_moduleState.directX.surface->Lock(NULL, &desc, DDLOCK_WAIT, NULL);
    }

    return false;
}

// 0x10002970
void unlockDxSurface()
{
    g_moduleState.directX.surface->Unlock(NULL);

    g_moduleState.surface.renderer = NULL;
}

// 0x10002990
bool copyToRendererSurfaceRect(S32 sx, S32 sy, S32 width, S32 height, S32 dx, S32 dy, S32 stride, Pixel* pixels)
{
    bool locked = false;

    if (g_moduleState.surface.renderer == NULL)
    {
        if (!lockDxSurface())
            return false;

        locked = true;
    }

    Pixel* src = (Pixel*)((Addr)pixels + (Addr)((stride * sy + sx) * sizeof(Pixel)));
    Pixel* dst = (Pixel*)((Addr)g_moduleState.surface.renderer + (Addr)(g_moduleState.pitch * dy + dx * sizeof(Pixel)));

    for (S32 yy = 0; yy < height; ++yy)
    {
        std::memcpy(dst, src, width * sizeof(Pixel));
        src = (Pixel*)((Addr)src + (Addr)(stride * sizeof(Pixel)));
        dst = (Pixel*)((Addr)dst + (Addr)g_moduleState.pitch);
    }

    if (locked)
        unlockDxSurface();

    return locked;
}

// 0x10002a30
void copyPixelRectFromTo(S32 sx, S32 sy, S32 sstr, Pixel* input, S32 dx, S32 dy, S32 dstr, Pixel* output, S32 width, S32 height)
{
    Pixel* src = (Pixel*)((Addr)input + (Addr)((sstr * sy + sx) * sizeof(Pixel)));
    Pixel* dst = (Pixel*)((Addr)output + (Addr)((dstr * dy + dx) * sizeof(Pixel)));

    for (S32 yy = 0; yy < height; ++yy)
    {
        std::memcpy(dst, src, width * sizeof(Pixel));
        src = (Pixel*)((Addr)src + (Addr)(sstr * sizeof(Pixel)));
        dst = (Pixel*)((Addr)dst + (Addr)(dstr * sizeof(Pixel)));
    }
}

// 0x10002a90
bool copyMainSurfaceToRenderer(S32 x, S32 y, S32 width, S32 height)
{
    bool locked = false;

    if (g_moduleState.surface.renderer == NULL)
    {
        if (!lockDxSurface())
        {
            return false;
        }

        locked = true;
    }
    Pixel* src = (Pixel*)((Addr)g_rendererState.surfaces.main + (Addr)(g_moduleState.surface.offset + y * SCREEN_WIDTH + x) * sizeof(Pixel));
    LPVOID dst = (LPVOID)((Addr)g_moduleState.surface.renderer + (Addr)(g_moduleState.pitch * y + x * sizeof(Pixel)));

    if (y < g_moduleState.surface.y)
    {
        const S32 delta = y + height - g_moduleState.surface.y;
        if ((y + height) < g_moduleState.surface.y || delta == 0)
        {
            for (S32 vertical = 0; vertical < height; vertical++)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
                src = (Pixel*)((Addr)src + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                dst = (LPVOID)((Addr)dst + (Addr)g_moduleState.pitch);
            }
        }
        else
        {
            for (S32 vertical = 0; vertical < height - delta; vertical++)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
                src = (Pixel*)((Addr)src + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                dst = (LPVOID)((Addr)dst + (Addr)g_moduleState.pitch);
            }

            src = (Pixel*)((Addr)src - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

            for (S32 vertical = 0; vertical < delta; vertical++)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
                src = (Pixel*)((Addr)src + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                dst = (LPVOID)((Addr)dst + (Addr)g_moduleState.pitch);
            }
        }
    }
    else
    {
        src = (Pixel*)((Addr)src - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

        for (S32 yy = 0; yy < height; ++yy)
        {
            std::memcpy(dst, src, width * sizeof(Pixel));
            src = (Pixel*)((Addr)src + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
            dst = (LPVOID)((Addr)dst + (Addr)g_moduleState.pitch);
        }
    }

    if (locked)
    {
        unlockDxSurface();
    }

    return locked;
}

// 0x10002b90
void FUN_10002b90(const S32 x, const S32 y, const S32 endX, const S32 endY)
{
}

// 0x10002fb0
void FUN_10002fb0(S32 x, S32 y, S32 width, S32 height)
{
}

// 0x10003320
S32 getTextLength(const char* text, const AssetCollection* asset)
{
    S32 result = 0;

    for (size_t i = 0; text[i] != ANSI_NULL; ++i)
    {
        const ImagePaletteSprite* image = (ImagePaletteSprite*)((Addr)asset + (Addr)asset->items[text[i]]);

        result += DEFAULT_FONT_ASSET_SPACING + image->width;
    }

    return result;
}

// 0x10003360
void FUN_10003360(S32 x, S32 y, LPSTR text, AssetCollection* asset, Pixel* palette)
{
    U32 offset = 0;

    for (U32 xx = 0; text[xx] != NULL; ++xx)
    {
        ImagePaletteSprite* image = (ImagePaletteSprite*)((Addr)asset + (Addr)asset->items[text[xx]]);

        drawMainSurfacePaletteSprite(x + offset, y, palette, image);

        offset = offset + DEFAULT_FONT_ASSET_SPACING + image->width;
    }
}

// 0x100033c0
void FUN_100033c0(S32 x, S32 y, LPSTR text, AssetCollection* asset, Pixel* palette)
{
    U32 offset = 0;

    for (U32 xx = 0; text[xx] != NULL; ++xx)
    {
        ImagePaletteSprite* image = (ImagePaletteSprite*)((Addr)asset + (Addr)asset->items[text[xx]]);

        drawBackSurfacePaletteShadeSprite(x + offset, y, y, palette, image);

        offset = offset + DEFAULT_FONT_ASSET_SPACING + image->width;
    }
}

// 0x10003420
void drawBackSurfaceRhomb(S32 angle_0, S32 angle_1, S32 angle_2, S32 angle_3, S32 tx, S32 ty, S32 stride, U8* input, Pixel* output)
{
    S32 overage;  //избыток
    S32 overflow; //переполнение

    S32 delta2;
    S32 TileStartDrawLength;


    bool draw;
    S16 uVar4;
    S32 iVar6;
    U16 uVar5;
    //LPVOID content = &input->Pixels;

    /*char debugMsg[256];
    sprintf(debugMsg, "[%s] unk01=%d, unk02=%d, unk03=%d, unk04=%d, tx=%d, ty=%d, y=%d, delta=%d\n", __FUNCTION__, unk01, unk02, unk03, unk04, tx, ty, g_moduleState.windowRect.y, g_moduleState.windowRect.y - ty);
    OutputDebugStringA(debugMsg);*/

    const Addr offset = (Addr)tx - (Addr)output;
    g_rendererState.rendererTile.stencil = (Pixel*)((Addr)g_rendererState.surfaces.stencil + offset);

    g_rendererState.rendererTile.windowRect.x = g_moduleState.windowRect.x + 0x21;        //0 + 33
    g_rendererState.rendererTile.windowRect.y = g_moduleState.windowRect.y;                //0
    g_rendererState.rendererTile.windowRect.width = g_moduleState.windowRect.width + 0x21;     //1023 + 33
    g_rendererState.rendererTile.windowRect.height = g_moduleState.windowRect.height;           //767
    g_rendererState.rendererTile.unk02 = 0;


    if ((((tx <= (g_rendererState.rendererTile.windowRect.width + 1)) && ((g_rendererState.rendererTile.windowRect.x - 64) <= tx)) && (ty <= (g_rendererState.rendererTile.windowRect.height + 1))) && ((g_moduleState.windowRect.y - 32) <= ty))
    {
        U8* src;
        U8* src1;
        Pixel* dst = (Pixel*)((Addr)output + (Addr)(g_moduleState.surface.offset + stride * ty + tx * sizeof(Pixel) - 2));
        Pixel* dst1;
        Pixel* dst2;
        Pixel* dst3;

        S32 TileOffsetTX = tx + 32;//чтото связано с яркостью четверти тайлов
        S32 lerp = (angle_1 - angle_0) * 4; //Осветление/затемнение

        // Пропустите необходимое количество строк от верхней части изображения в случае, если спрайт начинается выше допустимого прямоугольника рисования.
        //if (ty < g_moduleState.windowRect.y)
        //{
        //    g_rendererState.rendererTile.height = g_rendererState.rendererTile.height + ty - g_moduleState.windowRect.y;

        //    if (g_rendererState.rendererTile.height <= 0 || g_rendererState.rendererTile.height == g_moduleState.windowRect.y - ty) { return; }

        //    for (S32 x = 0; x < g_moduleState.windowRect.y - ty; ++x)
        //    {
        //        content = (LPVOID)((Addr)next + (Addr)sizeof(U16));
        //        next = (LPVOID)((Addr)next + (Addr)((reinterpret_cast<U16*>(next))[0] + sizeof(U16)));
        //    }

        //    ty = g_moduleState.windowRect.y;
        //}

        //g_rendererState.rendererTile.height = 32;
        //const S32 overflow2 = g_rendererState.rendererTile.height + ty - g_moduleState.windowRect.height - 1;
        //bool draw = overflow2 == 0 || (g_rendererState.rendererTile.height + ty < g_moduleState.windowRect.height + 1);
        //if (!draw)
        //{
        //    const S32 height = g_rendererState.rendererTile.height; //32
        //    g_rendererState.rendererTile.height = g_rendererState.rendererTile.height - overflow2; // объявляем новую высоту, учитывая сколько вмещается в кадр
        //    draw = g_rendererState.rendererTile.height != 0 && overflow2 <= height;
        //}

        if (ty < (g_moduleState.windowRect.y - 16)) // Фильтр на отображение четверти тайла, в область экрана попадает пирамидка высотой 8 байт ширина этой пирамидки у основания 29 пикскля
        {
            g_rendererState.rendererTile.lerp = (angle_3 - angle_0) * 16;
            TileOffsetTX = angle_0 * 256 + g_rendererState.rendererTile.lerp;

            dst2 = (Pixel*)((Addr)dst + (Addr)(stride * 8 - 29 * sizeof(Pixel)));// выделяет область по четверть тайла предпологаю * это именно его высота
            tx += 3; //шаг между усеченными пирамидками допустим 32+3
            TileStartDrawLength = 61; // стартовая длина строки нижней половины тайла
            input = input + 528; // Пропускаем верхнюю половину тайла для отображения нижней пирамидки, площадь нижней пирамидки 496
            ty += 16;//-24+16 скорей всего высота которая вне экрана -8, возможно удаляем верхнюю половину тайла

            g_rendererState.rendererTile.height = (g_moduleState.windowRect.height + 1) - ty;//Высота экрана + область тайла что будет не видна 768+8=776
            if (15 < g_rendererState.rendererTile.height)
            {
                g_rendererState.rendererTile.height = 16; // высота половины тайла (тайлы в игре рисуются половинками)
            }
            overage = g_moduleState.windowRect.y - ty; //Избыток
            if (ty <= g_moduleState.windowRect.y && overage != 0)
            {
                ty = ty + overage; //8+8 
                g_rendererState.rendererTile.height = g_rendererState.rendererTile.height - overage; // Выравнивание?
                for (S32 y = 0; y < overage; ++y)
                {
                    input = (U8*)((Addr)input + (Addr)TileStartDrawLength);
                    TileOffsetTX = TileOffsetTX + g_rendererState.rendererTile.lerp;
                    TileStartDrawLength -= 4;//для ступечатой отричовки каждай последующая отрисовка пирамидки -4 от ее ширины
                    dst2 = (Pixel*)((Addr)dst2 + (Addr)(stride + 2 * sizeof(Pixel)));// наверно 4 это 2пикселя + мешене для ступечатых операций
                    tx += 2;
                }
            }
        }
        else
        {
            g_rendererState.rendererTile.lerp = (angle_0 - angle_2) * 16;
            overage = angle_2 * 256 + g_rendererState.rendererTile.lerp + lerp;
            g_rendererState.rendererTile.height = (g_moduleState.windowRect.height + 1) - ty;
            if (15 < g_rendererState.rendererTile.height)
            {
                g_rendererState.rendererTile.height = 16;
            }
            TileStartDrawLength = 3;   // стартовая длина строки верхней половины тайла
            delta2 = g_moduleState.windowRect.y - ty;  //Избыток
            tx = TileOffsetTX;
            if (ty <= g_moduleState.windowRect.y && delta2 != 0)
            {
                g_rendererState.rendererTile.height -= delta2;
                ty = ty + delta2;
                for (S32 y = 0; y < delta2; ++y)
                {
                    input = (U8*)((Addr)(input)+(Addr)(TileStartDrawLength));
                    overage = overage + g_rendererState.rendererTile.lerp;
                    TileStartDrawLength += 4;
                    tx -= 2;
                    dst = (Pixel*)((Addr)dst + (Addr)(stride - 2 * sizeof(Pixel)));
                }
            }
            g_rendererState.rendererTile.unk07 = g_rendererState.rendererTile.height;
            if (0 < g_rendererState.rendererTile.height)
            {
                ty = g_rendererState.rendererTile.height + ty; // -16+32
                overflow = ty - g_moduleState.surface.y; //  16-0 вычисляется как разница между новым ty и высотой поверхности (ModuleState.Surface.Y).
                if (ty < g_moduleState.surface.y)
                {
                    overflow = 0;
                }
                draw = g_rendererState.rendererTile.height < overflow;
                g_rendererState.rendererTile.height = g_rendererState.rendererTile.height - overflow;
                dst2 = (Pixel*)dst;
                if (draw || g_rendererState.rendererTile.height == 0) //goto
                {
                    g_rendererState.rendererTile.height = g_rendererState.rendererTile.unk07;
                    ++g_rendererState.rendererTile.unk02;
                    draw = g_rendererState.rendererTile.unk07 != 0;
                    g_rendererState.rendererTile.unk07 = 0;
                    dst2 = (Pixel*)((Addr)dst - (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                    overflow = g_rendererState.rendererTile.unk07;
                }
                do
                {
                    for (U16 yy = 0; yy < g_rendererState.rendererTile.height; ++yy)
                    {
                        g_rendererState.rendererTile.unk07 = overflow;
                        uVar4 = overage;
                        TileOffsetTX = (g_rendererState.rendererTile.windowRect.width + 1) - tx; //767+1
                        if (TileOffsetTX != 0 && tx <= (g_rendererState.rendererTile.windowRect.width + 1)) //767+1
                        {
                            delta2 = TileStartDrawLength;
                            if (TileOffsetTX < TileStartDrawLength)
                            {
                                delta2 = TileOffsetTX;
                            }
                            TileOffsetTX = g_rendererState.rendererTile.windowRect.x - tx;
                            src = input;
                            dst3 = (Pixel*)dst2;
                            if (TileOffsetTX != 0 && tx <= g_rendererState.rendererTile.windowRect.x)
                            {
                                if (delta2 <= TileOffsetTX)//goto
                                {
                                    input = (U8*)((Addr)input + (Addr)TileStartDrawLength);
                                    TileStartDrawLength += 4;
                                    overage = overage + g_rendererState.rendererTile.lerp;
                                    dst = (Pixel*)((Addr)dst2 + (Addr)(stride - 2 * sizeof(Pixel)));
                                    tx -= 2;
                                    dst2 = (Pixel*)dst;
                                    overflow = g_rendererState.rendererTile.unk07;
                                }
                                src = (U8*)((Addr)input + (Addr)TileOffsetTX);
                                dst3 = (Pixel*)((Addr)dst2 + (Addr)TileOffsetTX * sizeof(Pixel));
                                delta2 = delta2 - TileOffsetTX;
                                iVar6 = overage;
                                for (S32 y = 0; y < TileOffsetTX; ++y)
                                {
                                    iVar6 = iVar6 + lerp;
                                    uVar4 = iVar6;
                                }
                            }
                            //if (g_rendererState.rendererTile.stencil <= dst3)
                            //{
                            //    dst3 = (Pixel*)((Addr)dst3 - (Addr)(MAX_RENDERER_WIDTH * sizeof(Pixel)));
                            //}
                            if (dst3 < output)
                            {
                                dst3 = (Pixel*)((Addr)dst3 + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                            }
                            uVar5 = ((U16)((S8)((U16)uVar4 >> 8) ^ g_rendererState.rendererTile.unk08) << 8) | (uVar4 & 0xFF);
                            g_rendererState.rendererTile.unk08 = g_rendererState.rendererTile.unk08 ^ 32;
                            for (S32 y = 0; y < delta2; ++y)
                            {
                                dst3[y] = g_moduleState.rhombsPalette.palette[((uVar5 >> 8) << 8) | src[y]];
                                uVar5 = (uVar5 + (U16)(lerp)) ^ 0x2000;
                            }
                        }
                        //goto
                        input = (U8*)((S32)input + TileStartDrawLength);
                        TileStartDrawLength += 4;
                        overage = overage + g_rendererState.rendererTile.lerp;
                        dst = (Pixel*)((Addr)dst2 + (Addr)(stride - 2 * sizeof(Pixel)));
                        tx -= 2;
                        dst2 = (Pixel*)dst;
                        overflow = g_rendererState.rendererTile.unk07;
                    }
                    //goto
                    g_rendererState.rendererTile.height = g_rendererState.rendererTile.unk07;
                    ++g_rendererState.rendererTile.unk02;
                    draw = g_rendererState.rendererTile.unk07 != 0;
                    g_rendererState.rendererTile.unk07 = 0;
                    dst2 = (Pixel*)((Addr)dst - (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                    overflow = g_rendererState.rendererTile.unk07;
                } while (draw);
            }
            g_rendererState.rendererTile.unk08 = g_rendererState.rendererTile.unk08 ^ 32;
            TileStartDrawLength -= (Addr)(3 * sizeof(Pixel));                   //сдвигаем нюжнюю часть на 3 пикселя
            dst2 = (Pixel*)((Addr)dst + (Addr)(3 * sizeof(Pixel)));             //офссет нижней части ромбика
            tx += 3;
            overage = (g_rendererState.rendererTile.windowRect.height + 1) - ty;
            if ((g_rendererState.rendererTile.windowRect.height + 1) < ty)
            {
                return;
            }
            if (15 < overage)
            {
                overage = 16;
            }
            g_rendererState.rendererTile.lerp = (angle_3 - angle_0) * 16;
            TileOffsetTX = angle_0 * 256 + g_rendererState.rendererTile.lerp;
            g_rendererState.rendererTile.height = overage;
        }
        if (0 < g_rendererState.rendererTile.height)
        {
            overflow = g_rendererState.rendererTile.unk07;
            if (g_rendererState.rendererTile.unk02 < 2)
            {
                g_rendererState.rendererTile.unk07 = g_rendererState.rendererTile.height;
                overflow = (g_rendererState.rendererTile.height + ty) - g_moduleState.surface.y;
                if ((g_rendererState.rendererTile.height + ty) < g_moduleState.surface.y)
                {
                    overflow = 0;
                }
                draw = g_rendererState.rendererTile.height < overflow;
                g_rendererState.rendererTile.height = g_rendererState.rendererTile.height - overflow;
                if (draw || g_rendererState.rendererTile.height == 0)//goto
                {
                    dst2 = (Pixel*)((Addr)dst2 - (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                    g_rendererState.rendererTile.height = g_rendererState.rendererTile.unk07;
                    draw = g_rendererState.rendererTile.unk07 != 0;
                    g_rendererState.rendererTile.unk07 = 0;
                    overflow = g_rendererState.rendererTile.unk07;
                }
            }
            do
            {
                for (U16 yy = 0; yy < g_rendererState.rendererTile.height; ++yy)
                {
                    g_rendererState.rendererTile.unk07 = overflow;
                    uVar4 = TileOffsetTX;
                    overage = (g_rendererState.rendererTile.windowRect.width + 1) - tx;
                    if (overage != 0 && tx <= (g_rendererState.rendererTile.windowRect.width + 1))
                    {
                        delta2 = TileStartDrawLength;
                        if (overage < TileStartDrawLength)
                        {
                            delta2 = overage;
                        }
                        overage = g_rendererState.rendererTile.windowRect.x - tx;
                        src1 = input;///?????? dst3
                        dst1 = dst2;
                        if (overage != 0 && tx <= g_rendererState.rendererTile.windowRect.x)
                        {
                            if (delta2 <= overage)
                            {
                                input = (U8*)((Addr)input + (Addr)TileStartDrawLength);//goto
                                TileStartDrawLength -= 4;
                                TileOffsetTX = TileOffsetTX + g_rendererState.rendererTile.lerp;
                                dst2 = (Pixel*)((Addr)dst2 + (Addr)(stride + 2 * sizeof(Pixel)));
                                tx += 2;
                                overflow = g_rendererState.rendererTile.unk07;
                            }
                            src1 = (U8*)((Addr)input + overage);
                            dst1 = (Pixel*)((Addr)dst2 + (Addr)(overage * sizeof(Pixel))); // Возможно sizeof
                            delta2 = delta2 - overage;
                            iVar6 = TileOffsetTX;
                            for (S32 y = 0; y < overage; ++y)
                            {
                                iVar6 = iVar6 + lerp;
                                uVar4 = iVar6;
                            }
                        }
                        U16 uVar6 = ((U16)((S8)((U16)uVar4 >> 8) ^ g_rendererState.rendererTile.unk08) << 8) | (uVar4 & 0xFF);
                        g_rendererState.rendererTile.unk08 = g_rendererState.rendererTile.unk08 ^ 0x20;
                        for (S32 y = 0; y < delta2; ++y)
                        {
                            dst1[y] = g_moduleState.rhombsPalette.palette[((uVar6 >> 8) << 8) | src1[y]];
                            uVar6 = (uVar6 + (U16)(lerp)) ^ 0x2000;
                        }

                    }
                    input = (U8*)((Addr)input + (Addr)TileStartDrawLength);
                    TileStartDrawLength -= 4;
                    TileOffsetTX = TileOffsetTX + g_rendererState.rendererTile.lerp;
                    dst2 = (Pixel*)((Addr)dst2 + (Addr)(stride + 2 * sizeof(Pixel))); // шаг на новую строку в буфере
                    tx += 2;
                    overflow = g_rendererState.rendererTile.unk07;
                }
                dst2 = (Pixel*)((Addr)dst2 - (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                g_rendererState.rendererTile.height = g_rendererState.rendererTile.unk07;
                draw = g_rendererState.rendererTile.unk07 != 0;
                g_rendererState.rendererTile.unk07 = 0;
                overflow = g_rendererState.rendererTile.unk07;
            } while (draw);
        }
    }
}

// 0x10004390
void FUN_10004390(S32 param_1, S32 param_2, LPVOID param_3)
{
}

// 0x100046b6
void FUN_100046b6(S32 param_1, S32 param_2, LPVOID param_3)
{
}

// 0x100049e6
void FUN_100049e6(S32 param_1, S32 param_2, U16 param_3, LPVOID param_4)
{
}

// 0x10004db0
void FUN_10004db0(S32 x, S32 y, U16 param_3, S32 param_4, LPVOID param_5)
{
}

// 0x100050df
void drawMainSurfacePaletteSprite(S32 x, S32 y, Pixel* palette, ImagePaletteSprite* sprite)
{
    //Здесь копируются параметры окна отрисовки из глобального состояния(ModuleState) 
    // в состояние рендерера(RendererState).
    // Это определяет область, в которой будет выводиться спрайт.
    g_rendererState.sprite.windowRect.x = g_moduleState.windowRect.x;
    g_rendererState.sprite.windowRect.y = g_moduleState.windowRect.y;
    g_rendererState.sprite.windowRect.width = g_moduleState.windowRect.width;
    g_rendererState.sprite.windowRect.height = g_moduleState.windowRect.height;
    // Установка размеров спрайта, при этом ширина увеличивается на +1 хз зачем
    g_rendererState.sprite.height = sprite->height;               //высота
    g_rendererState.sprite.width = sprite->width + 1;            //ширина
    //content    - получает указатель на пиксельные данные спрайта
    //offset_RLE - указатель на строку (используется для RLE - сжатых данных)
    LPVOID content = &sprite->pixels;
    U32* offset_RLE = (U32*)((Addr)content + (Addr)sprite->next);
    //x = x + sprite->x;        // todo: AM doesn't has this line, check
    //смещение спрайта по вертикали
    y = y + sprite->y;
    // Проверка выхода за верхнюю границу окна и если спрайт начинается выше окна, его верхнюю часть нужно обрезать
    if (y < g_moduleState.windowRect.y)
    {
        // Корректировка высоты спрайта
        g_rendererState.sprite.height = g_rendererState.sprite.height + y - g_moduleState.windowRect.y;
        // Проверка на полное отсутствие видимой части
        if (g_rendererState.sprite.height <= 0 || g_rendererState.sprite.height == g_moduleState.windowRect.y - y)
        {
            return;
        }
        // Пропуск невидимых строк спрайта
        for (S32 i = 0; i < g_moduleState.windowRect.y - y; ++i)
        {
            //Каждая строка в RLE - формате начинается с U16(длина данных)
            //Указатели перемещаются на следующую строку, пропуская невидимые
            content = (LPVOID)((Addr)offset_RLE + (Addr)sizeof(U16));
            offset_RLE = (U32*)((Addr)offset_RLE + (Addr)(((U16*)offset_RLE)[0] + sizeof(U16)));
        }
        // Установка новой начальной позиции Y
        y = g_moduleState.windowRect.y;
    }

    const S32 overflow = g_rendererState.sprite.height + y - g_moduleState.windowRect.height - 1;
    bool draw = overflow == 0 || (g_rendererState.sprite.height + y < g_moduleState.windowRect.height + 1);

    if (!draw)
    {
        const S32 height = g_rendererState.sprite.height;

        g_rendererState.sprite.height = g_rendererState.sprite.height - overflow;

        draw = g_rendererState.sprite.height != 0 && overflow <= height;
    }

    if (draw)
    {
        g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + (Addr)(g_moduleState.surface.stride * y) + (Addr)((x + sprite->x) * sizeof(Pixel)));
        g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + (Addr)(g_moduleState.surface.stride * y) + (Addr)(g_moduleState.windowRect.x * sizeof(Pixel)));
        g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + (Addr)(g_moduleState.surface.stride * y) + (Addr)((g_moduleState.windowRect.width + 1) * sizeof(Pixel)));

        //вычисляем вертикальное переполнение(overage) спрайта относительно поверхности экрана и корректирует высоту спрайта для отрисовки только видимой части
        const S32 overage = g_rendererState.sprite.height + y < g_moduleState.surface.y
            ? 0 : (g_rendererState.sprite.height + y - g_moduleState.surface.y);

        // Корректировка высоты спрайта
        g_rendererState.sprite.overage = overage;
        g_rendererState.sprite.height = g_rendererState.sprite.height - overage;

        if (g_rendererState.sprite.height == 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
        }

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                U32 skip = 0;
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Пропустите пиксели слева от области рисования спрайта
                // в случае, если спрайт начинается слева от разрешенного прямоугольника рисования.
                Pixel* sx = g_rendererState.sprite.x;
                // MinX и maxX — задают границы, за которые спрайт не должен выходить(обрезание по окну).
                while (sx < g_rendererState.sprite.minX)
                {
                    const U32 need = (U32)((Addr)g_rendererState.sprite.minX - (Addr)sx) / sizeof(Pixel);
                    // Компактные блоки(IMAGESPRITE_ITEM_COMPACT_MASK) — один цвет повторяется несколько раз (RLE - сжатие).
                    const U32 count = pixels->count & IMAGE_SPRITE_ITEM_COUNT_MASK;

                    if (pixels->count & IMAGE_SPRITE_ITEM_COMPACT_MASK)
                    {
                        if (count <= need)
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                    }
                    else
                    {
                        if (count <= need)
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (count - 1) * sizeof(U8) + sizeof(ImagePaletteSpritePixel));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx = (Pixel*)((Addr)sx + (Addr)(std::min(count, need) * sizeof(Pixel)));
                }

                while (sx < g_rendererState.sprite.maxX && (Addr)pixels < (Addr)offset_RLE)
                {

                    const U32 count = (pixels->count & IMAGE_SPRITE_ITEM_COUNT_MASK);

                    if (count == 0)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)sizeof(ImagePaletteSpritePixel));
                    }
                    else if (pixels->count & IMAGE_SPRITE_ITEM_COMPACT_MASK)
                    {
                        const U8 indx = pixels->pixels[0];

                        if (indx != 0)
                        {
                            const Pixel pixel = palette[indx];

                            for (U32 i = 0; i < count - skip; ++i)
                            {
                                if (((Addr)sx + (Addr)(i * sizeof(Pixel))) < (Addr)g_rendererState.sprite.maxX)
                                {
                                    sx[i] = pixel;
                                }
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)sizeof(ImagePaletteSpritePixel));
                    }
                    else
                    {
                        for (U32 i = 0; i < count - skip; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];

                            if (indx != 0)
                            {
                                if (((Addr)sx + (Addr)(i * sizeof(Pixel))) < (Addr)g_rendererState.sprite.maxX)
                                {
                                    sx[i] = palette[indx];
                                }
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)((count - 1) * sizeof(U8) + sizeof(ImagePaletteSpritePixel)));
                    }

                    sx = (Pixel*)((Addr)sx + (Addr)((count - skip) * sizeof(Pixel)));

                    skip = 0;
                }

                content = (LPVOID)((Addr)offset_RLE + (Addr)sizeof(U16));
                offset_RLE = (U32*)((Addr)offset_RLE + (Addr)(((U16*)offset_RLE)[0] + sizeof(U16)));

                g_rendererState.sprite.height = g_rendererState.sprite.height - 1;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState.surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState.surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState.surface.stride);
            }

            // Обведите по вертикали и нарисуйте излишек в случае, если у спрайта больше
            // содержимого, которое может поместиться в разрешенный прямоугольник для рисования.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
        }
    }
}

// 0x100053c3
void drawMainSurfaceVanishingSprite(S32 x, S32 y, S32 vanishOffset, Pixel* palette, ImagePaletteSprite* sprite)
{
    g_rendererState.sprite.vanishOffset = vanishOffset;
    U32 colorMask = (U32)g_moduleState.actualGreenMask << 16;
    g_rendererState.sprite.colorMask = colorMask | g_moduleState.actualBlueMask | g_moduleState.actualRedMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    g_rendererState.sprite.windowRect.x = g_moduleState.windowRect.x;
    g_rendererState.sprite.windowRect.y = g_moduleState.windowRect.y;
    g_rendererState.sprite.windowRect.width = g_moduleState.windowRect.width;
    g_rendererState.sprite.windowRect.height = g_moduleState.windowRect.height;

    g_rendererState.sprite.width = sprite->width + 1;
    g_rendererState.sprite.height = sprite->height;

    S32 newX = x + sprite->x;
    S32 newY = y + sprite->y;

    LPVOID content = &sprite->pixels;
    U32* offset_RLE = (U32*)((Addr)content + (Addr)sprite->next);

    if (newY < g_moduleState.windowRect.y)
    {
        g_rendererState.sprite.height = g_rendererState.sprite.height + newY - g_moduleState.windowRect.y;

        if (g_rendererState.sprite.height <= 0 || g_rendererState.sprite.height == g_moduleState.windowRect.y - newY)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState.windowRect.y - newY; ++i)
        {
            content = (LPVOID)((Addr)offset_RLE + (Addr)sizeof(U16));
            offset_RLE = (U32*)((Addr)offset_RLE + (Addr)(((U16*)offset_RLE)[0] + sizeof(U16)));
        }

        newY = g_moduleState.windowRect.y;
    }

    const S32 overflow = g_rendererState.sprite.height + newY - g_moduleState.windowRect.height - 1;
    bool draw = overflow == 0 || (g_rendererState.sprite.height + newY < g_moduleState.windowRect.height + 1);

    if (!draw)
    {
        const S32 height = g_rendererState.sprite.height;

        g_rendererState.sprite.height = g_rendererState.sprite.height - overflow;

        draw = g_rendererState.sprite.height != 0 && overflow <= height;
    }

    if (draw)
    {
        g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + (Addr)(g_moduleState.surface.stride * newY) + (Addr)((newX + sprite->x) * sizeof(Pixel)));
        g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + (Addr)(g_moduleState.surface.stride * newY) + (Addr)(g_moduleState.windowRect.x * sizeof(Pixel)));
        g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + (Addr)(g_moduleState.surface.stride * newY) + (Addr)((g_moduleState.windowRect.width + 1) * sizeof(Pixel)));

        const S32 overage = g_rendererState.sprite.height + newY < g_moduleState.surface.y
            ? 0 : (g_rendererState.sprite.height + newY - g_moduleState.surface.y);

        g_rendererState.sprite.overage = overage;
        g_rendererState.sprite.height = g_rendererState.sprite.height - overage;

        if (g_rendererState.sprite.height == 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
        }

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                U32 skip = 0;
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                Pixel* sx = g_rendererState.sprite.x;
                while (sx < g_rendererState.sprite.minX)
                {
                    const U32 need = (U32)((Addr)g_rendererState.sprite.minX - (Addr)sx) / sizeof(Pixel);
                    const U32 count = pixels->count & IMAGE_SPRITE_ITEM_COUNT_MASK;

                    if (pixels->count & IMAGE_SPRITE_ITEM_COMPACT_MASK)
                    {
                        if (count <= need)
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel));
                        }
                    }
                    else
                    {
                        if (count <= need)
                        {
                            pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (count - 1) * sizeof(U8) + sizeof(ImagePaletteSpritePixel));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx = (Pixel*)((Addr)sx + (Addr)(std::min(count, need) * sizeof(Pixel)));
                }

                while (sx < g_rendererState.sprite.maxX && (Addr)pixels < (Addr)offset_RLE)
                {

                    const U32 count = (pixels->count & IMAGE_SPRITE_ITEM_COUNT_MASK);

                    if (count == 0)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)sizeof(ImagePaletteSpritePixel));
                    }
                    else if (pixels->count & IMAGE_SPRITE_ITEM_COMPACT_MASK)
                    {
                        const U8 indx = pixels->pixels[0];

                        if (indx != 0)
                        {
                            const Pixel pixel = palette[indx];

                            for (U32 i = 0; i < count - skip; ++i)
                            {
                                if (((Addr)sx + (Addr)(i * sizeof(Pixel))) < (Addr)g_rendererState.sprite.maxX)
                                {
                                    U32 tempDoublePixel = ((U32)sx[i] << 16) | sx[i];
                                    U32 tempDoublePixel2 = (g_rendererState.sprite.colorMask & (((g_rendererState.sprite.colorMask & tempDoublePixel) * g_rendererState.sprite.vanishOffset) >> 5))
                                        | ((g_rendererState.sprite.colorMask & (((g_rendererState.sprite.colorMask & tempDoublePixel) * g_rendererState.sprite.vanishOffset) >> 5)) >> 16);
                                    U32 tempDoublePixel3 = ((U32)pixel << 16) | pixel;
                                    U32 tempDoublePixel4 = g_rendererState.sprite.colorMask & (((31 - g_rendererState.sprite.vanishOffset) * (g_rendererState.sprite.colorMask & tempDoublePixel3)) >> 5);
                                    U32 tempDoublePixel5 = tempDoublePixel4 | (tempDoublePixel4 >> 16);

                                    sx[i] = tempDoublePixel2 + tempDoublePixel5;        // todo: fix cast from U32 to Pixel, since there is add dx, ax instruction
                                }
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)sizeof(ImagePaletteSpritePixel));
                    }
                    else
                    {
                        for (U32 i = 0; i < count - skip; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];

                            if (indx != 0)
                            {
                                if (((Addr)sx + (Addr)(i * sizeof(Pixel))) < (Addr)g_rendererState.sprite.maxX)
                                {
                                    const Pixel pixel = palette[indx];

                                    U32 tempDoublePixel = ((U32)sx[i] << 16) | sx[i];
                                    U32 tempDoublePixel2 = (g_rendererState.sprite.colorMask & (((g_rendererState.sprite.colorMask & tempDoublePixel) * g_rendererState.sprite.vanishOffset) >> 5))
                                        | ((g_rendererState.sprite.colorMask & (((g_rendererState.sprite.colorMask & tempDoublePixel) * g_rendererState.sprite.vanishOffset) >> 5)) >> 16);
                                    U32 tempDoublePixel3 = ((U32)pixel << 16) | pixel;
                                    U32 tempDoublePixel4 = g_rendererState.sprite.colorMask & (((31 - g_rendererState.sprite.vanishOffset) * (g_rendererState.sprite.colorMask & tempDoublePixel3)) >> 5);
                                    U32 tempDoublePixel5 = tempDoublePixel4 | (tempDoublePixel4 >> 16);

                                    sx[i] = tempDoublePixel2 + tempDoublePixel5;        // todo: the same
                                }
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)((count - 1) * sizeof(U8) + sizeof(ImagePaletteSpritePixel)));
                    }

                    sx = (Pixel*)((Addr)sx + (Addr)((count - skip) * sizeof(Pixel)));

                    skip = 0;
                }

                content = (LPVOID)((Addr)offset_RLE + (Addr)sizeof(U16));
                offset_RLE = (U32*)((Addr)offset_RLE + (Addr)(((U16*)offset_RLE)[0] + sizeof(U16)));

                g_rendererState.sprite.height = g_rendererState.sprite.height - 1;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState.surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState.surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState.surface.stride);
            }

            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
        }
    }
}

// 0x1000579c
void FUN_1000579c(S32 x, S32 y, Pixel* palette, ImagePaletteSprite* sprite)
{
}

// 0x10005ac6
void FUN_10005ac6(S32 param_1, S32 param_2, U16 param_3, S32 param_4, LPVOID param_5)
{
}

// 0x10005e31
void drawBackSurfacePaletteShadeSprite(S32 x, S32 y, U16 level, Pixel* palette, ImagePaletteSprite* sprite)
{
}

// 0x1000618d
void FUN_1000618d(S32 x, S32 y, S32 param_3, LPVOID param_4)
{
}

// 0x100064b6
void drawMainSurfaceSprite(S32 x, S32 y, ImageSprite* sprite)
{
    g_rendererState.sprite.windowRect.x = g_moduleState.windowRect.x;
    g_rendererState.sprite.windowRect.y = g_moduleState.windowRect.y;
    g_rendererState.sprite.windowRect.width = g_moduleState.windowRect.width;
    g_rendererState.sprite.windowRect.height = g_moduleState.windowRect.height;

    g_rendererState.sprite.height = sprite->height;
    g_rendererState.sprite.width = sprite->width + 1;

    LPVOID content = &sprite->pixels;
    LPVOID next = (LPVOID)((Addr)content + (Addr)sprite->next);

    y = y + sprite->y;

    // Пропустите необходимое количество строк от верхней части изображения
    // в случае, если спрайт начинается выше допустимого прямоугольника рисования.
    if (y < g_moduleState.windowRect.y)
    {
        g_rendererState.sprite.height = g_rendererState.sprite.height + y - g_moduleState.windowRect.y;

        if (g_rendererState.sprite.height <= 0 || g_rendererState.sprite.height == g_moduleState.windowRect.y - y) { return; }

        for (S32 i = 0; i < g_moduleState.windowRect.y - y; ++i)
        {
            content = (LPVOID)((Addr)next + (Addr)sizeof(U16));
            next = (LPVOID)((Addr)next + (Addr)(((U16*)(next))[0] + sizeof(U16)));
        }

        y = g_moduleState.windowRect.y;
    }

    const S32 overflow = g_rendererState.sprite.height + y - g_moduleState.windowRect.height - 1;

    bool draw = overflow == 0 || (g_rendererState.sprite.height + y < g_moduleState.windowRect.height + 1);

    if (!draw)
    {
        const S32 height = g_rendererState.sprite.height;

        g_rendererState.sprite.height = g_rendererState.sprite.height - overflow;

        draw = g_rendererState.sprite.height != 0 && overflow <= height;
    }

    if (draw)
    {
        g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + (Addr)(g_moduleState.surface.stride * y) + (Addr)((x + sprite->x) * sizeof(Pixel)));
        g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + (Addr)(g_moduleState.surface.stride * y) + (Addr)(g_moduleState.windowRect.x * sizeof(Pixel)));
        g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + (Addr)(g_moduleState.surface.stride * y) + (Addr)((g_moduleState.windowRect.width + 1) * sizeof(Pixel)));

        const S32 overage = g_rendererState.sprite.height + y < g_moduleState.surface.y
            ? 0 : (g_rendererState.sprite.height + y - g_moduleState.surface.y);

        g_rendererState.sprite.overage = overage;
        g_rendererState.sprite.height = g_rendererState.sprite.height - overage;

        if (g_rendererState.sprite.height == 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
        }

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                U32 skip = 0;
                ImageSpritePixel* pixels = (ImageSpritePixel*)content;

                // Пропустите пиксели слева от области рисования спрайта
                // в случае, если спрайт начинается слева от разрешенного прямоугольника рисования.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX)
                {
                    const U32 need = (U32)((Addr)g_rendererState.sprite.minX - (Addr)sx) / sizeof(Pixel);
                    const U32 count = pixels->count & IMAGE_SPRITE_ITEM_COUNT_MASK;

                    if (pixels->count & IMAGE_SPRITE_ITEM_COMPACT_MASK)
                    {
                        if (count <= need) { pixels = (ImageSpritePixel*)((Addr)pixels + sizeof(ImageSpritePixel)); }
                    }
                    else
                    {
                        if (count <= need) { pixels = (ImageSpritePixel*)((Addr)pixels + (count - 1) * sizeof(Pixel) + sizeof(ImageSpritePixel)); }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx = (Pixel*)((Addr)sx + (Addr)(std::min(count, need) * sizeof(Pixel)));
                }

                while (sx < g_rendererState.sprite.maxX && (Addr)pixels < (Addr)next)
                {
                    const U32 count = (pixels->count & IMAGE_SPRITE_ITEM_COUNT_MASK);

                    if (count == 0)
                    {
                        pixels = (ImageSpritePixel*)((Addr)pixels + (Addr)sizeof(ImageSpritePixel));
                    }
                    else if (pixels->count & IMAGE_SPRITE_ITEM_COMPACT_MASK)
                    {
                        const Pixel pixel = pixels->pixels[0];

                        if (pixel != PixelColor::MAGENTA)
                        {
                            for (U32 i = 0; i < count - skip; ++i)
                            {
                                if (((Addr)sx + (Addr)(i * sizeof(Pixel))) < (Addr)g_rendererState.sprite.maxX) { sx[i] = pixel; }
                            }
                        }

                        pixels = (ImageSpritePixel*)((Addr)pixels + (Addr)sizeof(ImageSpritePixel));
                    }
                    else
                    {
                        for (U32 i = 0; i < count - skip; ++i)
                        {
                            const Pixel pixel = pixels->pixels[skip + i];

                            if (((Addr)sx + (Addr)(i * sizeof(Pixel))) < (Addr)g_rendererState.sprite.maxX) { if (pixel != PixelColor::MAGENTA) { sx[i] = pixel; } }
                        }

                        pixels = (ImageSpritePixel*)((Addr)pixels + (Addr)((count - 1) * sizeof(Pixel) + sizeof(ImageSpritePixel)));
                    }

                    sx = (Pixel*)((Addr)sx + (Addr)((count - skip) * sizeof(Pixel)));

                    skip = 0;
                }

                content = (LPVOID)((Addr)next + (Addr)sizeof(U16));
                next = (LPVOID)((Addr)next + (Addr)(((U16*)(next))[0] + sizeof(U16)));

                g_rendererState.sprite.height = g_rendererState.sprite.height - 1;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState.surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState.surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState.surface.stride);
            }

            // Обведите по вертикали и нарисуйте излишек
            // в случае, если у спрайта больше содержимого, которое может поместиться в разрешенный прямоугольник для рисования.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
        }
    }
}

// 0x100067ad
void FUN_100067ad(S32 x, S32 y, S32 param_3, LPVOID param_4)
{
}

// 0x10006b21
void drawMainSurfaceAnimationSprite(S32 x, S32 y, U16 level, const AnimationPixel* palette, ImagePaletteSprite* sprite)
{
    return;
    g_rendererState.sprite.colorMask =
        (((U32)g_moduleState.actualGreenMask) << 16) | g_moduleState.actualRedMask | g_moduleState.actualBlueMask;
    g_rendererState.sprite.adjustedColorMask = (g_rendererState.sprite.colorMask << 1) | g_rendererState.sprite.colorMask;

    g_rendererState.sprite.windowRect.x = g_moduleState.windowRect.x;
    g_rendererState.sprite.windowRect.y = g_moduleState.windowRect.y;
    g_rendererState.sprite.windowRect.width = g_moduleState.windowRect.width;
    g_rendererState.sprite.windowRect.height = g_moduleState.windowRect.height;

    level = (level + 0x440) * 0x20;
    CONST DoublePixel stencil = (level << 16) | level;

    g_rendererState.sprite.height = sprite->height;
    g_rendererState.sprite.width = sprite->width + 1;

    LPVOID content = &sprite->pixels;
    LPVOID next = (LPVOID)((Addr)content + (Addr)sprite->next);

    y = y + sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState.windowRect.y)
    {
        g_rendererState.sprite.height = g_rendererState.sprite.height + y - g_moduleState.windowRect.y;

        if (g_rendererState.sprite.height <= 0 || g_rendererState.sprite.height == g_moduleState.windowRect.y - y) { return; }

        for (S32 i = 0; i < g_moduleState.windowRect.y - y; ++i)
        {
            content = (LPVOID)((Addr)next + (Addr)sizeof(U16));
            next = (LPVOID)((Addr)next + (Addr)(((U16*)next)[0] + sizeof(U16)));
        }

        y = g_moduleState.windowRect.y;
    }

    CONST S32 overflow = g_rendererState.sprite.height + y - g_moduleState.windowRect.height - 1;

    BOOL draw = overflow == 0 || (g_rendererState.sprite.height + y < g_moduleState.windowRect.height + 1);

    if (!draw)
    {
        CONST S32 height = g_rendererState.sprite.height;

        g_rendererState.sprite.height = g_rendererState.sprite.height - overflow;

        draw = g_rendererState.sprite.height != 0 && overflow <= height;
    }

    if (draw)
    {
        g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + (Addr)(g_moduleState.surface.stride * y) + (Addr)((x + sprite->x) * sizeof(Pixel)));
        g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + (Addr)(g_moduleState.surface.stride * y) + (Addr)(g_moduleState.windowRect.x * sizeof(Pixel)));
        g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + (Addr)(g_moduleState.surface.stride * y) + (Addr)((g_moduleState.windowRect.width + 1) * sizeof(Pixel)));

        CONST S32 overage = g_rendererState.sprite.height + y < g_moduleState.surface.y
            ? 0 : (g_rendererState.sprite.height + y - g_moduleState.surface.y);

        g_rendererState.sprite.overage = overage;
        g_rendererState.sprite.height = g_rendererState.sprite.height - overage;

        if (g_rendererState.sprite.height == 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Pixel)));
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Pixel)));
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Pixel)));
        }

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                U32 skip = 0;
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX)
                {
                    CONST U32 need = (U32)((Addr)g_rendererState.sprite.minX - (Addr)sx) / sizeof(Pixel);
                    CONST U32 count = pixels->count & IMAGE_SPRITE_ITEM_COUNT_MASK;

                    if (pixels->count & IMAGE_SPRITE_ITEM_COMPACT_MASK)
                    {
                        if (count <= need) { pixels = (ImagePaletteSpritePixel*)((Addr)pixels + sizeof(ImagePaletteSpritePixel)); }
                    }
                    else
                    {
                        if (count <= need) { pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (count - 1) * sizeof(U8) + sizeof(ImagePaletteSpritePixel)); }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx = (Pixel*)((Addr)sx + (Addr)(std::min(count, need) * sizeof(Pixel)));
                }

                while (sx < g_rendererState.sprite.maxX && (Addr)pixels < (Addr)next)
                {
                    U32 count = (pixels->count & IMAGE_SPRITE_ITEM_COUNT_MASK);

                    if (count == 0)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)sizeof(ImagePaletteSpritePixel));
                    }
                    else if (pixels->count & IMAGE_SPRITE_ITEM_COMPACT_MASK)
                    {
                        // Skip pixels that are below required stencil value.
                        {
                            CONST Addr offset = (Addr)sx - (Addr)g_rendererState.surfaces.main;
                            while (stencil <= *(DoublePixel*)((Addr)g_rendererState.surfaces.stencil + offset + (Addr)(x + skip) * (Addr)sizeof(Pixel)))
                            {
                                count = count - 1;
                                skip = skip + 1;

                                if (count == 0) { break; }
                            }
                        }

                        // Blend and draw pixels.
                        if (count != 0)
                        {
                            CONST U8 indx = pixels->pixels[0];
                            CONST AnimationPixel pixel = palette[indx];

                            CONST DoublePixel pix = pixel >> 19;

                            if ((pix & 0xFF) != 0x1F)
                            {
                                for (U32 i = 0; i < count - skip; ++i)
                                {
                                    CONST DoublePixel value =
                                        (pix * (((sx[i] << 16) | sx[i]) & g_rendererState.sprite.colorMask) >> 5) & g_rendererState.sprite.colorMask;

                                    sx[i] = (Pixel)((value >> 16) | value) + (Pixel)pixel;
                                }
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)sizeof(ImagePaletteSpritePixel));
                    }
                    else
                    {
                        // Skip pixels that are below required stencil value.
                        {
                            CONST Addr offset = (Addr)sx - (Addr)g_rendererState.surfaces.main;
                            while (stencil <= *(DoublePixel*)((Addr)g_rendererState.surfaces.stencil + offset + (Addr)(x + skip) * (Addr)sizeof(Pixel)))
                            {
                                count = count - 1;
                                skip = skip + 1;

                                if (count == 0) { break; }
                            }
                        }

                        // Blend and draw pixels.
                        if (count != 0)
                        {
                            for (U32 i = 0; i < count - skip; ++i)          // todo: fix bug - in some cases count is less than skip, so i is too big
                            {
                                CONST U8 indx = pixels->pixels[skip + i];
                                CONST AnimationPixel pixel = palette[indx];

                                CONST DoublePixel pix = pixel >> 19;

                                if ((pix & 0xFF) != 0x1F)
                                {
                                    CONST DoublePixel value =
                                        (pix * (((sx[i] << 16) | sx[i]) & g_rendererState.sprite.colorMask) >> 5) & g_rendererState.sprite.colorMask;

                                    sx[i] = (Pixel)((value >> 16) | value) + (Pixel)pixel;
                                }
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)((count - 1) * sizeof(U8) + sizeof(ImagePaletteSpritePixel)));
                    }

                    sx = (Pixel*)((Addr)sx + (Addr)((count - skip) * sizeof(Pixel)));

                    skip = 0;
                }

                content = (LPVOID)((Addr)next + (Addr)sizeof(U16));
                next = (LPVOID)((Addr)next + (Addr)(((U16*)next)[0] + sizeof(U16)));

                g_rendererState.sprite.height = g_rendererState.sprite.height - 1;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState.surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState.surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState.surface.stride);
            }

            // Wrap around vertically, and draw the overage
            // in case the sprite has more content that can fit into the allowed drawing rectangle.
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Pixel)));
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Pixel)));
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(Pixel)));
        }
    }
}

// 0x10006ef8
void drawSurfaceUnitSprite(S32 x, S32 y, U16 level, Pixel* palette, ImagePaletteSprite* sprite)
{
}

// 0x10007292
void FUN_10007292(S32 x, S32 y, U16 param_3, S32 param_4, LPVOID param_5)
{
}

// 0x10007662
void FUN_10007662(S32 x, S32 y, S32 param_3, LPVOID param_4)
{
}

// 0x10007928
void FUN_10007928(S32 param_1, S32 param_2, S32 param_3, LPVOID param_4)
{
}

// 0x10007be8
void FUN_10007be8(S32 x, S32 y, U16 param_3, LPVOID param_4)
{
}

// 0x10007fbc
void FUN_10007fbc(S32 x, S32 y, U16 param_3, S32 param_4, LPVOID param_5)
{
}

// 0x10008ecd
void FUN_10008ecd(S32 param_1, S32 param_2, LPVOID param_3, S32 param_4, LPVOID param_5)
{
}

// 0x10009eb3
void FUN_10009eb3(S32 param_1, S32 param_2, LPVOID param_3, S32 param_4, S32 param_5, S32 param_6)
{
}

// 0x1000a4f3
void FUN_1000a4f3(S32 param_1, S32 param_2, S32 param_3, S32 param_4, LPVOID param_5, LPVOID param_6)
{
}
