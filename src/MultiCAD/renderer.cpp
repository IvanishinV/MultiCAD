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
        || g_moduleState.windowRect.width < x)
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

        if (delta <= 0)
        {
            for (S32 yy = 0; yy < height; ++yy)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
                src = (Pixel*)((Addr)src + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                dst = (Pixel*)((Addr)dst + (Addr)(stride * sizeof(Pixel)));
            }
        }
        else
        {
            for (S32 yy = 0; yy < height - delta; ++yy)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
                src = (Pixel*)((Addr)src + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                dst = (Pixel*)((Addr)dst + (Addr)(stride * sizeof(Pixel)));
            }

            src = (Pixel*)((Addr)src - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

            for (S32 yy = 0; yy < delta; ++yy)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
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
            std::memcpy(dst, src, width * sizeof(Pixel));
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

        const S32 lines = (old_offset + ((old_offset >> 0x1f) & (SCREEN_WIDTH - 1))) / SCREEN_WIDTH;

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
void callDrawBackSurfaceRhomb(S32 tx, S32 ty, S32 angle_0, S32 angle_1, S32 angle_2, S32 angle_3, ImagePaletteTile* input)
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
    const S32 offset = g_moduleState.surface.offset % SCREEN_WIDTH;

    Pixel* src = (Pixel*)((Addr)g_rendererState.surfaces.main
        + (Addr)((offset + SCREEN_SIZE_IN_PIXELS) * sizeof(Pixel)));

    g_rendererState.outline.width = g_moduleState.windowRect.width - g_moduleState.windowRect.x;
    g_rendererState.outline.height = g_moduleState.windowRect.height + 1 - g_moduleState.windowRect.y;
    g_rendererState.outline.options = OUTLINE_DRAW_OPTION_NONE;

    x = x - g_moduleState.windowRect.x;
    y = y - g_moduleState.windowRect.y;

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
    g_rendererState.outline.stride = g_moduleState.surface.stride;
    if (height < 0)
    {
        g_rendererState.outline.stride = -g_moduleState.surface.stride;
        height = -height;
        g_rendererState.outline.verticalStride = -g_rendererState.outline.verticalStride;
    }

    Pixel* dst = (Pixel*)((Addr)g_rendererState.surfaces.main
        + (Addr)(g_moduleState.surface.offset * sizeof(Pixel) + y * g_moduleState.surface.stride + x * sizeof(Pixel))
        + (Addr)(g_moduleState.windowRect.y * g_moduleState.surface.stride + g_moduleState.windowRect.x * sizeof(Pixel)));

    if ((g_rendererState.outline.options & OUTLINE_DRAW_OPTION_TOP) == OUTLINE_DRAW_OPTION_NONE)
    {
        height = height - 1;

        Pixel* pixels = dst;

        if (src <= dst)
        {
            pixels = (Pixel*)((Addr)dst - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
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
                pixels = (Pixel*)((Addr)pixels - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
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

    if (height != 0 && (g_rendererState.outline.options & OUTLINE_DRAW_OPTION_BOTTOM) == OUTLINE_DRAW_OPTION_NONE)
    {
        if (src <= dst)
        {
            dst = (Pixel*)((Addr)dst - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));
        }

        for (S32 xx = 0; xx < width; ++xx)
        {
            dst[g_rendererState.outline.horizontalStride * xx] = pixel;
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
        const Pixel pixel = (Pixel)(offset << STENCIL_PIXEL_COLOR_SHIFT);
        const DoublePixel pix = ((DoublePixel)(pixel) << GRAPHICS_BITS_PER_PIXEL_16) | (DoublePixel)pixel;

        if (y < g_moduleState.surface.y)
        {
            const S32 delta = y + height - g_moduleState.surface.y;

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
        const Pixel pixel = (Pixel)(-offset << STENCIL_PIXEL_COLOR_SHIFT);
        const DoublePixel pix = ((DoublePixel)(pixel) << GRAPHICS_BITS_PER_PIXEL_16) | (DoublePixel)pixel;

        if (y < g_moduleState.surface.y)
        {
            const S32 delta = y + height - g_moduleState.surface.y;

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
    void* dst = (void*)((Addr)g_moduleState.surface.renderer + (Addr)(g_moduleState.pitch * y + x * sizeof(Pixel)));

    if (y < g_moduleState.surface.y)
    {
        const S32 delta = y + height - g_moduleState.surface.y;
        if (delta <= 0)
        {
            for (S32 vertical = 0; vertical < height; vertical++)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
                src = (Pixel*)((Addr)src + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                dst = (void*)((Addr)dst + (Addr)g_moduleState.pitch);
            }
        }
        else
        {
            for (S32 vertical = 0; vertical < height - delta; vertical++)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
                src = (Pixel*)((Addr)src + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                dst = (void*)((Addr)dst + (Addr)g_moduleState.pitch);
            }

            src = (Pixel*)((Addr)src - (Addr)(SCREEN_SIZE_IN_PIXELS * sizeof(Pixel)));

            for (S32 vertical = 0; vertical < delta; vertical++)
            {
                std::memcpy(dst, src, width * sizeof(Pixel));
                src = (Pixel*)((Addr)src + (Addr)(SCREEN_WIDTH * sizeof(Pixel)));
                dst = (void*)((Addr)dst + (Addr)g_moduleState.pitch);
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
            dst = (void*)((Addr)dst + (Addr)g_moduleState.pitch);
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

    if (g_moduleState.surface.renderer == NULL)
    {
        if (!lockDxSurface())
        {
            return;
}

        locked = true;
    }

    S32 delta = (endY + 1) - y;  
    g_rendererState.rendererStruct02.unk04 = 0;
    g_moduleState.moduleStruct01.actualRgbMask = g_moduleState.initialRgbMask;
    g_moduleState.moduleStruct01.dstRowStride = -8 * g_moduleState.pitch + 32;
    g_moduleState.moduleStruct01.lineStep = -(S32)SCREEN_WIDTH * 16 + 32;

    U8* fogSrc = &g_moduleState.fogSprites[(y >> 3) + 8].unk[(x >> 4) + 8];
    DoublePixel* src = (DoublePixel*)((Addr)g_rendererState.surfaces.main + (SCREEN_WIDTH * y + x + g_moduleState.surface.offset) * sizeof(Pixel));
    DoublePixel* dst = (DoublePixel*)((Addr)g_moduleState.surface.renderer + x * sizeof(Pixel) + y * g_moduleState.pitch);

    if (y >= g_moduleState.surface.y)
        src = (DoublePixel*)((Addr)src - SCREEN_SIZE_IN_BYTES);

    if (y >= g_moduleState.surface.y || y + delta <= g_moduleState.surface.y)
    {
        g_rendererState.rendererStruct01.validRowsBlockCount = delta >> 3;
        g_rendererState.rendererStruct01.unk02 = 0;
        g_rendererState.rendererStruct02.excessRowsBlockCount = 0;  
        
        g_moduleState.moduleStruct01.rowAlignmentMask = SCREEN_WIDTH * sizeof(Pixel);
    }
    else
    {
        g_rendererState.rendererStruct01.validRowsBlockCount = (g_moduleState.surface.y - y) >> 3;
        g_rendererState.rendererStruct02.excessRowsBlockCount = (delta + y - g_moduleState.surface.y) >> 3;
        U32 v7 = ((U8)g_moduleState.surface.y - (U8)y) & 7;
        v7 = v7 | ((8 - v7) << 8);
        v7 = v7 << 8;               // v7 now is 0000 ' 8 - v7 ' v7 ' 0000, so the summ of all its bytes is always 8
        g_rendererState.rendererStruct01.unk02 = (S32)v7;

        if (g_rendererState.rendererStruct01.validRowsBlockCount == 0)
        {
            g_moduleState.moduleStruct01.lineStep += SCREEN_SIZE_IN_BYTES;
            g_moduleState.moduleStruct01.rowAlignmentMask = v7;
            g_rendererState.rendererStruct01.validRowsBlockCount = 1;
            g_rendererState.rendererStruct01.unk02 = 0;
        }
        else
            g_moduleState.moduleStruct01.rowAlignmentMask = SCREEN_WIDTH * sizeof(Pixel);
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

                g_moduleState.moduleStruct01.fogPtr = someFog;
                g_rendererState.rendererStruct01.unk04 = ((endX + 1) - x) >> 4;

                do {
                    DoublePixel fogPixel = *(DoublePixel*)((Addr)g_moduleState.moduleStruct01.fogPtr - 2);
                    fogPixel = (fogPixel & 0xFFFF'0000) | (*(Pixel*)((Addr)g_moduleState.moduleStruct01.fogPtr + sizeof(Fog)));

                    if (fogPixel)
                    {
                        if (fogPixel == 0x80808080)
                        {
                            U32 j = g_moduleState.moduleStruct01.rowAlignmentMask >> 8;
                            ++g_moduleState.moduleStruct01.fogPtr;
                            while (true)
                            {
                                do
                                {
                                    dst[0] = src[0];
                                    dst[1] = src[1];
                                    dst[2] = src[2];
                                    dst[3] = src[3];
                                    dst[4] = src[4];
                                    dst[5] = src[5];
                                    dst[6] = src[6];
                                    dst[7] = src[7];
                                    src = (DoublePixel*)((Addr)src + (Addr)SCREEN_WIDTH * sizeof(Pixel));
                                    dst = (DoublePixel*)((Addr)dst + (Addr)g_moduleState.pitch);

                                    --j;
                                } while (j & 0xFF);
                                j >>= 8;
                                if (j == 0)
                                    break;
                                src = (DoublePixel*)((Addr)src - (Addr)SCREEN_SIZE_IN_BYTES);
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
                            g_rendererState.rendererStruct01.unk01 = v27 >> 2;

                            // Compute v28 (difference between fogPixelTopByte and fogPixelHighByte)
                            S32 v28 = -(S32)(fogPixelTopByte < fogPixelHighByte);
                            fogPixelTopByte -= fogPixelHighByte;
                            v28 = (v28 & 0xFFFF'FF00) | fogPixelTopByte;
                            g_rendererState.rendererStruct02.unk01 = 2 * v28;

                            // Compute v30 (difference between fogPixelLowByte and fogPixelLow)
                            S32 v30 = -(S32)(fogPixelLowByte < fogPixelHighByte);
                            fogPixelLowByte -= fogPixelHighByte;
                            v30 = (v30 & 0xFFFF'FF00) | fogPixelLowByte;

                            S32 v31 = 4 * v30;
                            g_rendererState.rendererStruct02.unk02 = v31;

                            U32 fogOffset = ((S32)fogPixelHighByte + 0x7F) << 5;

                            const U32 mask = g_moduleState.moduleStruct01.actualRgbMask;
                            U32 j = g_moduleState.moduleStruct01.rowAlignmentMask >> 8;
                            ++g_moduleState.moduleStruct01.fogPtr;

                            while (true)
                            {
                                g_rendererState.rendererStruct02.unk04 = 0;
                                do {
                                    U8 k = 0x10;
                                    S32 v39 = fogOffset;
                                    do {
                                        while ((U8)(fogOffset >> 8) != 0x20)
                                        {
                                            const U32 srcPixel = (*src & 0xFFFF) | (*src << 16);
                                            const U32 v35 = g_rendererState.rendererStruct02.unk04 + (mask & srcPixel) * (U8)(fogOffset >> 8);
                                            fogOffset += g_rendererState.rendererStruct02.unk01;

                                            const U32 v37 = mask & (v35 >> 5);
                                            g_rendererState.rendererStruct02.unk04 = mask & v35;

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

                                        fogOffset += g_rendererState.rendererStruct02.unk01;
                                        *(Pixel*)dst = (Pixel)v31;

                                        dst = (DoublePixel*)((Addr)dst + sizeof(Pixel));
                                        src = (DoublePixel*)((Addr)src + sizeof(Pixel));
                                        --k;
                                    } while (k);

                                    fogOffset = g_rendererState.rendererStruct02.unk02 + v39;
                                    src = (DoublePixel*)((Addr)src + SCREEN_WIDTH * sizeof(Pixel) - 32);
                                    dst = (DoublePixel*)((Addr)dst + g_moduleState.pitch - 32);
                                    g_rendererState.rendererStruct02.unk01 += g_rendererState.rendererStruct01.unk01;

                                    --j;
                                } while (j & 0xFF);
                                j >>= 8;
                                if (j == 0)
                                    break;
                                src = (DoublePixel*)((Addr)src - (Addr)SCREEN_SIZE_IN_BYTES);
                            }
                        }
                    }
                    else
                    {
                        U32 j = g_moduleState.moduleStruct01.rowAlignmentMask >> 8;
                        ++g_moduleState.moduleStruct01.fogPtr;
                        const DoublePixel mask = (g_moduleState.unk23 << 16) | g_moduleState.unk23;
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
                                src = (DoublePixel*)((Addr)src + (Addr)SCREEN_WIDTH * sizeof(Pixel));
                                dst = (DoublePixel*)((Addr)dst + (Addr)g_moduleState.pitch);

                                --j;
                            } while (j & 0xFF);
                            j >>= 8;
                            if (j == 0)
                                break;
                            src = (DoublePixel*)((Addr)src - (Addr)SCREEN_SIZE_IN_BYTES);
                        }
                    }

                    src = (DoublePixel*)((Addr)src + (Addr)g_moduleState.moduleStruct01.lineStep);
                    dst = (DoublePixel*)((Addr)dst + (Addr)g_moduleState.moduleStruct01.dstRowStride);
                    --g_rendererState.rendererStruct01.unk04;
                } while (g_rendererState.rendererStruct01.unk04);

                src = (DoublePixel*)((Addr)srcTemp + (Addr)SCREEN_WIDTH * sizeof(Pixel) * 8);
                dst = (DoublePixel*)((Addr)dstTemp + (Addr)g_moduleState.pitch * 8);
                --g_rendererState.rendererStruct01.validRowsBlockCount;
            } while (g_rendererState.rendererStruct01.validRowsBlockCount);

            if ((g_rendererState.rendererStruct01.unk02 & 0xFF00) == 0)
                break;

            g_moduleState.moduleStruct01.rowAlignmentMask = g_rendererState.rendererStruct01.unk02;
            g_moduleState.moduleStruct01.lineStep = SCREEN_SIZE_IN_BYTES - SCREEN_WIDTH * sizeof(Pixel) * 8 + 32;
            g_rendererState.rendererStruct01.validRowsBlockCount = 1;
            g_rendererState.rendererStruct01.unk02 = 0;
        }

        g_moduleState.moduleStruct01.lineStep = -(S32)SCREEN_WIDTH * 16 + 32;
        remainingExcessRows = g_rendererState.rendererStruct02.excessRowsBlockCount;
        g_rendererState.rendererStruct01.validRowsBlockCount = remainingExcessRows;
        g_rendererState.rendererStruct02.excessRowsBlockCount = 0;
        g_moduleState.moduleStruct01.rowAlignmentMask = SCREEN_WIDTH * sizeof(Pixel);
        src = (DoublePixel*)((Addr)src - (Addr)SCREEN_SIZE_IN_BYTES);
    } while (remainingExcessRows);

    if (locked)
    {
        unlockDxSurface();
    }

    return;
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
void drawBackSurfaceRhomb(S32 angle_0, S32 angle_1, S32 angle_2, S32 angle_3, S32 tx, S32 ty, S32 stride, ImagePaletteTile* input, Pixel* output)
{
    // Tile height: 32
    // Tile width: 63

    g_rendererState.tile.stencil = (Pixel*)((Addr)output + (g_moduleState.surface.offset % SCREEN_WIDTH) * sizeof(Pixel) + SCREEN_SIZE_IN_BYTES);
    g_rendererState.tile.windowRect.x = g_moduleState.windowRect.x + TILE_SIZE_HEIGHT + 1;
    g_rendererState.tile.windowRect.y = g_moduleState.windowRect.y;
    g_rendererState.tile.windowRect.width = g_moduleState.windowRect.width + TILE_SIZE_HEIGHT + 1;
    g_rendererState.tile.windowRect.height = g_moduleState.windowRect.height;
    g_rendererState.tile.displayedHalfs = 0;

    // Проверка видимости тайла
    if (tx > g_rendererState.tile.windowRect.width + 1
        || tx < g_rendererState.tile.windowRect.x - TILE_SIZE_WIDTH - 1
        || ty > g_rendererState.tile.windowRect.height + 1
        || ty < g_rendererState.tile.windowRect.y - TILE_SIZE_HEIGHT)
        return;

    S32 tileStartDrawLength;

    // Настройка рендеринга
    U8* srcInput = input->pixels;
    Pixel* dst = (Pixel*)((Addr)output + (g_moduleState.surface.offset * sizeof(Pixel)) + stride * ty + tx * sizeof(Pixel) - 2);
        Pixel* dst2;
    S32 txDelta = tx + TILE_SIZE_HEIGHT;
    S32 diff = (angle_1 - angle_0) << 2;
    bool isUpperPart = (ty > (g_moduleState.windowRect.y - 16)) ? true : false; // 1 остальное 0 четверь

    /*OutputDebugStringA(std::format("[{}] tx={}, ty={}, X={}, Y={}, Width={}, Height={}\n"
            , __FUNCTION__
            , tx
            , ty
            , g_moduleState.windowRect.x
            , g_moduleState.windowRect.x
            , g_moduleState.windowRect.width
            , g_moduleState.windowRect.height
        ).c_str());*/

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
        g_rendererState.tile.tileHeight = std::min((g_moduleState.windowRect.height + 1) - ty, 16);

        // Если тайл по высоте торчит за пределы экрана, то уменьшаем отображаемую высоту тайла
        const S32 overage = g_moduleState.windowRect.y - ty;
        if (overage >= 0)
        {
            ty = g_rendererState.tile.windowRect.y;
            g_rendererState.tile.tileHeight -= overage;

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

        g_rendererState.tile.tileHeight = std::min((g_moduleState.windowRect.height + 1) - ty, 16);

        S32 overage = g_moduleState.windowRect.y - ty;
        if (overage >= 0)
        {
            ty += overage;
            g_rendererState.tile.tileHeight -= overage;

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
        if (g_rendererState.tile.tileHeight > 0)
            {
            ty += g_rendererState.tile.tileHeight;
            S32 overflow = std::max(ty - g_moduleState.surface.y, 0);

            g_rendererState.tile.tempTileHeight = g_rendererState.tile.tileHeight;
            g_rendererState.tile.tileHeight -= overflow;

            dst2 = dst;
            if (g_rendererState.tile.tileHeight <= 0)
            {
                g_rendererState.tile.tileHeight = g_rendererState.tile.tempTileHeight;
                g_rendererState.tile.tempTileHeight = 0;
                g_rendererState.tile.displayedHalfs++;

                overflow = 0;

                dst2 = (Pixel*)((Addr)dst2 - (Addr)SCREEN_SIZE_IN_BYTES);
            }

            while (g_rendererState.tile.tileHeight > 0)
                {
                for (S32 yy = 0; yy < g_rendererState.tile.tileHeight; ++yy)
                    {
                    g_rendererState.tile.tempTileHeight = overflow;

                    S32 totalTxOffset = txDelta;

                    const S32 delta = (g_rendererState.tile.windowRect.width + 1) - tx;
                    S32 delta2 = std::min(delta, tileStartDrawLength);
                    const S32 delta3 = g_rendererState.tile.windowRect.x - tx;
                    if (delta > 0 && delta2 > delta3)
                        {
                        U8* srcTemp = srcInput;
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
                            dstTemp = (Pixel*)((Addr)dstTemp - (Addr)SCREEN_SIZE_IN_BYTES);
                        }
                        if (dstTemp < output)
                            {
                            dstTemp = (Pixel*)((Addr)dstTemp + (Addr)SCREEN_SIZE_IN_BYTES);
                            }

                        U16 uVar5 = ((U16)((totalTxOffset >> 8) ^ g_rendererState.tile.unk08) << 8) | (totalTxOffset & 0xFF);
                        g_rendererState.tile.unk08 ^= 0x20;

                        for (S32 y = 0; y < delta2; ++y)
                            {
                            dstTemp[y] = g_moduleState.rhombsPalette.palette[(uVar5 & 0xFF00) | srcTemp[y]];
                            uVar5 = uVar5 + (U16)(diff) ^ 0x2000;
                            }
                        }

                    srcInput = (U8*)((Addr)srcInput + tileStartDrawLength);
                    tileStartDrawLength += 4;

                    txDelta += g_rendererState.tile.diff;
                    overflow = g_rendererState.tile.tempTileHeight;
                        tx -= 2;

                    dst2 = dst = (Pixel*)((Addr)dst2 + (Addr)(stride - 4));
                    }

                // Отрисовываем оставшуюся часть высоты тайла
                g_rendererState.tile.tileHeight = g_rendererState.tile.tempTileHeight;
                g_rendererState.tile.tempTileHeight = 0;
                g_rendererState.tile.displayedHalfs++;

                overflow = 0;

                dst2 = (Pixel*)((Addr)dst - (Addr)SCREEN_SIZE_IN_BYTES);
            }
        }

        if (ty > g_rendererState.tile.windowRect.height + 1) 
                return;

        g_rendererState.tile.unk08 ^= 0x20;
        tileStartDrawLength -= 6;                                   // Сдвигаем нюжнюю часть на 3 пикселя
        dst2 = (Pixel*)((Addr)dst + (Addr)(3 * sizeof(Pixel)));     // Офссет нижней части ромбика
        tx += 3;

        g_rendererState.tile.tileHeight = std::min((g_rendererState.tile.windowRect.height + 1) - ty, 16);
        g_rendererState.tile.diff = (angle_3 - angle_0) << 4;
        txDelta = (angle_0 << 8) + g_rendererState.tile.diff;
            }

    // Рендеринг нижней части
    if (g_rendererState.tile.tileHeight > 0)
        {
        S32 overflow = g_rendererState.tile.tempTileHeight;

        if (g_rendererState.tile.displayedHalfs < 2)
            {
            overflow = std::max(g_rendererState.tile.tileHeight + ty - g_moduleState.surface.y, 0);

            g_rendererState.tile.tempTileHeight = g_rendererState.tile.tileHeight;
            g_rendererState.tile.tileHeight -= overflow;

            if (g_rendererState.tile.tileHeight <= 0)
                {
                g_rendererState.tile.tileHeight = g_rendererState.tile.tempTileHeight;
                g_rendererState.tile.tempTileHeight = 0;

                overflow = g_rendererState.tile.tempTileHeight;

                dst2 = (Pixel*)((Addr)dst2 - (Addr)SCREEN_SIZE_IN_BYTES);
                }
                }

        while (g_rendererState.tile.tileHeight > 0)
            {
            for (U16 yy = 0; yy < g_rendererState.tile.tileHeight; ++yy)
                {
                g_rendererState.tile.tempTileHeight = overflow;

                S32 totalTxOffset = txDelta;

                S32 delta = (g_rendererState.tile.windowRect.width + 1) - tx;
                S32 delta2 = std::min(delta, tileStartDrawLength);
                const S32 delta3 = g_rendererState.tile.windowRect.x - tx;
                if (delta > 0 && delta2 > delta3)
                        {
                    U8* srcTemp = srcInput;
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
                        dstTemp[y] = g_moduleState.rhombsPalette.palette[(uVar5 & 0xFF00) | srcTemp[y]];
                        uVar5 = uVar5 + (U16)(diff) ^ 0x2000;
                        }

                    }

                srcInput = (U8*)((Addr)srcInput + tileStartDrawLength);
                tileStartDrawLength -= 4;

                txDelta += g_rendererState.tile.diff;
                overflow = g_rendererState.tile.tempTileHeight;
                    tx += 2;

                dst2 = (Pixel*)((Addr)dst2 + (Addr)(stride + 2 * sizeof(Pixel)));
                }

            g_rendererState.tile.tileHeight = g_rendererState.tile.tempTileHeight;
            g_rendererState.tile.tempTileHeight = 0;

            overflow = g_rendererState.tile.tempTileHeight;

            dst2 = (Pixel*)((Addr)dst2 - (Addr)SCREEN_SIZE_IN_BYTES);
        };
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
void drawMainSurfacePaletteSprite(S32 x, S32 y, const Pixel* palette, const ImagePaletteSprite* const sprite)
{
    // Копируем глобальные параметры окна отрисовки в отдельную переменную          // todo: check windowRect initialization, because looks like it's not used anywhere
    g_rendererState.sprite.windowRect.x = g_moduleState.windowRect.x;
    g_rendererState.sprite.windowRect.y = g_moduleState.windowRect.y;
    g_rendererState.sprite.windowRect.width = g_moduleState.windowRect.width;
    g_rendererState.sprite.windowRect.height = g_moduleState.windowRect.height;

    // Копируем высоту и ширину спрайта для дальнейшего изменения в случае необходимости
    g_rendererState.sprite.height = sprite->height;
    g_rendererState.sprite.width = sprite->width + 1;

    // content - указатель на пиксели спрайта
    // offset_RLE - указатель на строку (используется для RLE - сжатых данных)
    const void* content = &sprite->pixels;
    U32* offset_RLE = (U32*)((Addr)content + (Addr)sprite->next);

    x = x + sprite->x;
    y = y + sprite->y;

    // Проверка выхода за верхнюю границу окна и если спрайт начинается выше окна, то его верхнюю часть нужно обрезать
    if (y < g_moduleState.windowRect.y)
    {
        // Корректировка высоты спрайта по разнице между высотами
        g_rendererState.sprite.height = g_rendererState.sprite.height - (g_moduleState.windowRect.y - y);

        // Проверка на полное отсутствие видимой части
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        // Пропуск невидимых строк спрайта
        for (S32 i = 0; i < g_moduleState.windowRect.y - y; ++i)
        {
            //Каждая строка в RLE - формате начинается с U16(длина данных)
            //Указатели перемещаются на следующую строку, пропуская невидимые
            content = (void*)((Addr)offset_RLE + (Addr)sizeof(U16));
            offset_RLE = (U32*)((Addr)offset_RLE + (Addr)(((U16*)offset_RLE)[0] + sizeof(U16)));
        }

        y = g_moduleState.windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState.windowRect.height + 1);
    bool draw = overflow <= 0;

    // Если есть overflow, то мы проверяем, можно ли уменьшить высоту отображаемого спрайта на overflow
    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = (Addr)(g_moduleState.surface.stride * y);

        g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)(x * sizeof(Pixel)));
        g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)(g_moduleState.windowRect.x * sizeof(Pixel)));
        g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)((g_moduleState.windowRect.width + 1) * sizeof(Pixel)));


        // Ещё раз проверяем переполнение высоты спрайта, но уже относительно surface, а не windowRect
        const S32 overage = y + g_rendererState.sprite.height < g_moduleState.surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState.surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        // Корректировка высоты спрайта. Если переполнения не было, то высота спрайта не изменится.
        // Однако в таком случае нужно будет отрисовать спрайт дважды (до переполнения и после, т.е. sprite->height - overage и overage)
        g_rendererState.sprite.height -= overage;

        // Если нужно отрисовывать только переполнение
        if (g_rendererState.sprite.height < 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)SCREEN_SIZE_IN_BYTES);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)SCREEN_SIZE_IN_BYTES);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)SCREEN_SIZE_IN_BYTES);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                U32 skip = 0;
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Если левый край спрайта находится левее прямоугольника для рисования, то пропускаем часть спрайта
                Pixel* sx = g_rendererState.sprite.x;
                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)offset_RLE)
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

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)offset_RLE)
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

                content = (void*)((Addr)offset_RLE + (Addr)sizeof(U16));
                offset_RLE = (U32*)((Addr)offset_RLE + (Addr)(((U16*)offset_RLE)[0] + sizeof(U16)));

                g_rendererState.sprite.height = g_rendererState.sprite.height - 1;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState.surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState.surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState.surface.stride);
            }

            // Если было также и переполнение, то отрисовываем и его
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)SCREEN_SIZE_IN_BYTES);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)SCREEN_SIZE_IN_BYTES);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)SCREEN_SIZE_IN_BYTES);
        }
    }
}

// 0x100053c3
void drawMainSurfaceVanishingSprite(S32 x, S32 y, const S32 vanishOffset, const Pixel* palette, const ImagePaletteSprite* const sprite)
{
    const U32 colorMask = ((U32)g_moduleState.actualGreenMask << 16) | g_moduleState.actualBlueMask | g_moduleState.actualRedMask;
    g_rendererState.sprite.vanishOffset = vanishOffset;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    g_rendererState.sprite.windowRect.x = g_moduleState.windowRect.x;
    g_rendererState.sprite.windowRect.y = g_moduleState.windowRect.y;
    g_rendererState.sprite.windowRect.width = g_moduleState.windowRect.width;
    g_rendererState.sprite.windowRect.height = g_moduleState.windowRect.height;

    g_rendererState.sprite.width = sprite->width + 1;
    g_rendererState.sprite.height = sprite->height;

    x = x + sprite->x;
    y = y + sprite->y;

    const void* content = &sprite->pixels;
    U32* offset_RLE = (U32*)((Addr)content + (Addr)sprite->next);

    if (y < g_moduleState.windowRect.y)
    {
        g_rendererState.sprite.height = g_rendererState.sprite.height - (g_moduleState.windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState.windowRect.y - y; ++i)
        {
            content = (void*)((Addr)offset_RLE + (Addr)sizeof(U16));
            offset_RLE = (U32*)((Addr)offset_RLE + (Addr)(((U16*)offset_RLE)[0] + sizeof(U16)));
        }

        y = g_moduleState.windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState.windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = (Addr)(g_moduleState.surface.stride * y);

        g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)(x * sizeof(Pixel)));
        g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)(g_moduleState.windowRect.x * sizeof(Pixel)));
        g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)((g_moduleState.windowRect.width + 1) * sizeof(Pixel)));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState.surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState.surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height < 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)SCREEN_SIZE_IN_BYTES);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)SCREEN_SIZE_IN_BYTES);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)SCREEN_SIZE_IN_BYTES);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                U32 skip = 0;
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                Pixel* sx = g_rendererState.sprite.x;
                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)offset_RLE)
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

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)offset_RLE)
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

                                    sx[i] = (U16)(tempDoublePixel2 + tempDoublePixel5);
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

                                    sx[i] = (U16)(tempDoublePixel2 + tempDoublePixel5);
                                }
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)((count - 1) * sizeof(U8) + sizeof(ImagePaletteSpritePixel)));
                    }

                    sx = (Pixel*)((Addr)sx + (Addr)((count - skip) * sizeof(Pixel)));

                    skip = 0;
                }

                content = (void*)((Addr)offset_RLE + (Addr)sizeof(U16));
                offset_RLE = (U32*)((Addr)offset_RLE + (Addr)(((U16*)offset_RLE)[0] + sizeof(U16)));

                g_rendererState.sprite.height = g_rendererState.sprite.height - 1;

                g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x + (Addr)g_moduleState.surface.stride);
                g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX + (Addr)g_moduleState.surface.stride);
                g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX + (Addr)g_moduleState.surface.stride);
            }

            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)SCREEN_SIZE_IN_BYTES);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)SCREEN_SIZE_IN_BYTES);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)SCREEN_SIZE_IN_BYTES);
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
void drawBackSurfacePaletteShadeSprite(S32 x, S32 y, U16 level, const Pixel* const palette, const ImagePaletteSprite* const sprite)
{
    const U32 colorMask = ((U32)g_moduleState.actualGreenMask << 16) | g_moduleState.actualBlueMask | g_moduleState.actualRedMask;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    g_rendererState.sprite.windowRect.x = g_moduleState.windowRect.x;
    g_rendererState.sprite.windowRect.y = g_moduleState.windowRect.y;
    g_rendererState.sprite.windowRect.width = g_moduleState.windowRect.width;
    g_rendererState.sprite.windowRect.height = g_moduleState.windowRect.height;

    level = (level + 0x440) * 0x20;

    g_rendererState.sprite.height = sprite->height;
    g_rendererState.sprite.width = sprite->width + 1;

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x = x + sprite->x;
    y = y + sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState.windowRect.y)
    {
        g_rendererState.sprite.height = g_rendererState.sprite.height - (g_moduleState.windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState.windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + (Addr)sizeof(U16));
            next = (U32*)((Addr)next + (Addr)(((U16*)next)[0] + sizeof(U16)));
        }

        y = g_moduleState.windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState.windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = (Addr)(g_moduleState.surface.stride * y);

        g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.surfaces.back
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)(x * sizeof(Pixel)));
        g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.surfaces.back
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)(g_moduleState.windowRect.x * sizeof(Pixel)));
        g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.surfaces.back
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)((g_moduleState.windowRect.width + 1) * sizeof(Pixel)));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState.surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState.surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)SCREEN_SIZE_IN_BYTES);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)SCREEN_SIZE_IN_BYTES);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)SCREEN_SIZE_IN_BYTES);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                U32 skip = 0;       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
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

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    U32 count = (pixels->count & IMAGE_SPRITE_ITEM_COUNT_MASK);
                    const U32 availCount = std::min(count - skip, (U32)(g_rendererState.sprite.maxX - sx));

                    if (count == 0)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)sizeof(ImagePaletteSpritePixel));
                    }
                    else if (pixels->count & IMAGE_SPRITE_ITEM_COMPACT_MASK)
                    {
                        const U8 indx = pixels->pixels[0];

                        if (indx != 0)
                        {
                            const ptrdiff_t offset = sx - g_rendererState.surfaces.back;
                            Pixel* stencil = g_rendererState.surfaces.stencil + offset;

                            Pixel pixel = palette[indx];

                            for (U32 i = 0; i < availCount; ++i)
                            {
                                const DoublePixel sten = stencil[i];
                                stencil[i] = level | (sten & 3);

                                if (sten & 2)
                                    pixel = (Pixel)(g_moduleState.backSurfaceShadePixel + ((*(DoublePixel*)g_moduleState.shadeColorMask & pixel) >> 1));

                                sx[i] = pixel;
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)sizeof(ImagePaletteSpritePixel));
                    }
                    else
                    {
                        const ptrdiff_t offset = sx - g_rendererState.surfaces.back;
                        Pixel* stencil = g_rendererState.surfaces.stencil + offset;

                        for (U32 i = 0; i < availCount; ++i)
                        {
                            const U8 indx = pixels->pixels[skip + i];

                            if (indx != 0)
{
                                const Pixel sten = stencil[skip + i];
                                stencil[skip + i] = level | (sten & 3);

                                Pixel pixel = palette[indx];
                                if (sten & 2)
                                    pixel = (Pixel)(g_moduleState.backSurfaceShadePixel + ((*(DoublePixel*)g_moduleState.shadeColorMask & pixel) >> 1));

                                sx[i] = pixel;
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)((count - 1) * sizeof(U8) + sizeof(ImagePaletteSpritePixel)));
                    }

                    sx = (Pixel*)((Addr)sx + (Addr)((count - skip) * sizeof(Pixel)));

                    skip = 0;
                }

                content = (void*)((Addr)next + (Addr)sizeof(U16));
                next = (void*)((Addr)next + (Addr)(((U16*)next)[0] + sizeof(U16)));

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

    const void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x = x + sprite->x;
    y = y + sprite->y;

    if (y < g_moduleState.windowRect.y)
    {
        g_rendererState.sprite.height = g_rendererState.sprite.height - (g_moduleState.windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState.windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + (Addr)sizeof(U16));
            next = (U32*)((Addr)next + (Addr)(((U16*)next)[0] + sizeof(U16)));
        }

        y = g_moduleState.windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState.windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = (Addr)(g_moduleState.surface.stride * y);

        g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)(x * sizeof(Pixel)));
        g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)(g_moduleState.windowRect.x * sizeof(Pixel)));
        g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)((g_moduleState.windowRect.width + 1) * sizeof(Pixel)));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState.surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState.surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height < 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)SCREEN_SIZE_IN_BYTES);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)SCREEN_SIZE_IN_BYTES);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)SCREEN_SIZE_IN_BYTES);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                U32 skip = 0;
                ImageSpritePixel* pixels = (ImageSpritePixel*)content;

                Pixel* sx = g_rendererState.sprite.x;
                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    const U32 need = (U32)((Addr)g_rendererState.sprite.minX - (Addr)sx) / sizeof(Pixel);
                    const U32 count = pixels->count & IMAGE_SPRITE_ITEM_COUNT_MASK;

                    if (pixels->count & IMAGE_SPRITE_ITEM_COMPACT_MASK)
                    {
                        if (count <= need) {
                            pixels = (ImageSpritePixel*)((Addr)pixels + sizeof(ImageSpritePixel));
                        }
                    }
                    else
                    {
                        if (count <= need)
                        {
                            pixels = (ImageSpritePixel*)((Addr)pixels + (count - 1) * sizeof(Pixel) + sizeof(ImageSpritePixel));
                        }
                    }

                    skip = count == need ? 0 : std::min(count, need);
                    sx = (Pixel*)((Addr)sx + (Addr)(std::min(count, need) * sizeof(Pixel)));
                }

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
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
                                if (((Addr)sx + (Addr)(i * sizeof(Pixel))) < (Addr)g_rendererState.sprite.maxX)
                                {
                                    sx[i] = pixel;
                                }
                            }
                        }

                        pixels = (ImageSpritePixel*)((Addr)pixels + (Addr)sizeof(ImageSpritePixel));
                    }
                    else
                    {
                        for (U32 i = 0; i < count - skip; ++i)
                        {
                            const Pixel pixel = pixels->pixels[skip + i];

                            if (((Addr)sx + (Addr)(i * sizeof(Pixel))) < (Addr)g_rendererState.sprite.maxX)
                            {
                                if (pixel != PixelColor::MAGENTA)
                                {
                                    sx[i] = pixel;
                                }
                            }
                        }

                        pixels = (ImageSpritePixel*)((Addr)pixels + (Addr)((count - 1) * sizeof(Pixel) + sizeof(ImageSpritePixel)));
                    }

                    sx = (Pixel*)((Addr)sx + (Addr)((count - skip) * sizeof(Pixel)));

                    skip = 0;
                }

                content = (void*)((Addr)next + (Addr)sizeof(U16));
                next = (void*)((Addr)next + (Addr)(((U16*)(next))[0] + sizeof(U16)));

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
    const U32 colorMask = ((U32)g_moduleState.actualGreenMask << 16) | g_moduleState.actualBlueMask | g_moduleState.actualRedMask;
    g_rendererState.sprite.colorMask = colorMask;
    g_rendererState.sprite.adjustedColorMask = colorMask | (colorMask << 1);

    g_rendererState.sprite.windowRect.x = g_moduleState.windowRect.x;
    g_rendererState.sprite.windowRect.y = g_moduleState.windowRect.y;
    g_rendererState.sprite.windowRect.width = g_moduleState.windowRect.width;
    g_rendererState.sprite.windowRect.height = g_moduleState.windowRect.height;

    level = (level + 0x440) * 0x20;
    const DoublePixel stencil = (level << 16) | level;

    g_rendererState.sprite.height = sprite->height;
    g_rendererState.sprite.width = sprite->width + 1;

    void* content = &sprite->pixels;
    void* next = (void*)((Addr)content + (Addr)sprite->next);

    x = x + sprite->x;
    y = y + sprite->y;

    // Skip the necessary number of rows from the top of the image
    // in case the sprite starts above the allowed drawing rectangle.
    if (y < g_moduleState.windowRect.y)
    {
        g_rendererState.sprite.height = g_rendererState.sprite.height - (g_moduleState.windowRect.y - y);
        if (g_rendererState.sprite.height <= 0)
        {
            return;
        }

        for (S32 i = 0; i < g_moduleState.windowRect.y - y; ++i)
        {
            content = (void*)((Addr)next + (Addr)sizeof(U16));
            next = (U32*)((Addr)next + (Addr)(((U16*)next)[0] + sizeof(U16)));
        }

        y = g_moduleState.windowRect.y;
    }

    const S32 overflow = y + g_rendererState.sprite.height - (g_moduleState.windowRect.height + 1);
    bool draw = overflow <= 0;

    if (!draw)
    {
        draw = !(g_rendererState.sprite.height <= overflow);
        g_rendererState.sprite.height -= overflow;
    }

    if (draw)
    {
        const Addr linesStride = (Addr)(g_moduleState.surface.stride * y);

        g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)(x * sizeof(Pixel)));
        g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)(g_moduleState.windowRect.x * sizeof(Pixel)));
        g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.surfaces.main
            + (Addr)(g_moduleState.surface.offset * sizeof(Pixel)) + linesStride + (Addr)((g_moduleState.windowRect.width + 1) * sizeof(Pixel)));


        const S32 overage = y + g_rendererState.sprite.height < g_moduleState.surface.y
            ? 0 : y + g_rendererState.sprite.height - g_moduleState.surface.y;

        g_rendererState.sprite.overage = g_rendererState.sprite.height;
        g_rendererState.sprite.height -= overage;

        if (g_rendererState.sprite.height <= 0)
        {
            g_rendererState.sprite.height = g_rendererState.sprite.overage;
            g_rendererState.sprite.overage = 0;

            g_rendererState.sprite.x = (Pixel*)((Addr)g_rendererState.sprite.x - (Addr)SCREEN_SIZE_IN_BYTES);
            g_rendererState.sprite.minX = (Pixel*)((Addr)g_rendererState.sprite.minX - (Addr)SCREEN_SIZE_IN_BYTES);
            g_rendererState.sprite.maxX = (Pixel*)((Addr)g_rendererState.sprite.maxX - (Addr)SCREEN_SIZE_IN_BYTES);
        }
        else
            g_rendererState.sprite.overage = overage;

        while (g_rendererState.sprite.height > 0)
        {
            while (g_rendererState.sprite.height > 0)
            {
                U32 skip = 0;       // How many pixels we should skip if pixels->count was bigger than diff between minX and sx
                ImagePaletteSpritePixel* pixels = (ImagePaletteSpritePixel*)content;

                // Skip the pixels to the left of the sprite drawing area
                // in case the sprite starts to the left of allowed drawing rectangle.
                Pixel* sx = g_rendererState.sprite.x;

                // There was a bug, that 'pixels' was bigger than 'next'. According to IDA, there is a separate loop "pixels < next"
                // Also, there was a bug that 'i < count - skip', but count has been reduced by skip
                // And also, there was a bug that sx[i] >= maxX
                while (sx < g_rendererState.sprite.minX && (std::uintptr_t)pixels < (std::uintptr_t)next)
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

                while (sx < g_rendererState.sprite.maxX && (std::uintptr_t)pixels < (std::uintptr_t)next)
                {
                    U32 count = (pixels->count & IMAGE_SPRITE_ITEM_COUNT_MASK);
                    const U32 availCount = std::min(count - skip, (U32)(g_rendererState.sprite.maxX - sx));

                    if (count == 0)
                    {
                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)sizeof(ImagePaletteSpritePixel));
                    }
                    else if (pixels->count & IMAGE_SPRITE_ITEM_COMPACT_MASK)
                    {
                        // Blend and draw pixels.
                        if (count != 0)
                        {
                            const U8 indx = pixels->pixels[0];
                            const AnimationPixel pixel = palette[indx];

                            const DoublePixel pix = pixel >> 19;

                            if ((pix & 0xFF) != 0x1F)
                            {
                                const ptrdiff_t offset = sx - g_rendererState.surfaces.main;
                                Pixel* pStencil = g_rendererState.surfaces.stencil + offset;

                                for (U32 i = 0; i < availCount; ++i)
                                {
                                    if (stencil <= *(DoublePixel*)((Addr)pStencil + (Addr)(i - 1) * (Addr)sizeof(Pixel)))
                                        continue;

                                    const DoublePixel value =
                                        (pix * (((sx[i] << 16) | sx[i]) & g_rendererState.sprite.colorMask) >> 5) & g_rendererState.sprite.colorMask;

                                    sx[i] = (Pixel)((value >> 16) | value) + (Pixel)pixel;
                                }
                            }
                        }

                        pixels = (ImagePaletteSpritePixel*)((Addr)pixels + (Addr)sizeof(ImagePaletteSpritePixel));
                    }
                    else
                    {
                        // Blend and draw pixels.
                        if (count != 0)
                        {
                            const ptrdiff_t offset = sx - g_rendererState.surfaces.main;
                            Pixel* pStencil = g_rendererState.surfaces.stencil + offset;

                            for (U32 i = 0; i < availCount; ++i)
                            {
                                const U8 indx = pixels->pixels[skip + i];
                                const AnimationPixel pixel = palette[indx];

                                const DoublePixel pix = pixel >> 19;

                                if ((pix & 0xFF) != 0x1F)
                                {
                                    if (stencil <= *(DoublePixel*)((Addr)pStencil + (Addr)(i - 1) * (Addr)sizeof(Pixel)))
                                        continue;

                                    const DoublePixel value =
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

                content = (void*)((Addr)next + (Addr)sizeof(U16));
                next = (void*)((Addr)next + (Addr)(((U16*)next)[0] + sizeof(U16)));

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
