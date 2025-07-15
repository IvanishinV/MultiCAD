#include "pch.h"
#include "cad.h"
#include "renderer.h"

Renderer g_moduleState;

Renderer* InitializeModule()
{
    g_moduleState.surface.main = g_rendererState.surfaces.main;
    g_moduleState.surface.back = g_rendererState.surfaces.back;
    g_moduleState.surface.stencil = g_rendererState.surfaces.stencil;

    initValues();

    g_moduleState.actions.FUN_10002fb0_0                        = FUN_10002fb0;
    g_moduleState.actions.FUN_10002fb0_1                        = FUN_10002fb0;
    g_moduleState.actions.initValues                            = initValues;
    g_moduleState.actions.initDxInstance                        = initDxInstance;
    g_moduleState.actions.restoreDxInstance                     = restoreDxInstance;
    g_moduleState.actions.initWindowDxSurface                   = initWindowDxSurface;
    g_moduleState.actions.setPixelColorMasks                    = setPixelColorMasks;
    g_moduleState.actions.releaseDxSurface                      = releaseDxSurface;
    g_moduleState.actions.lockDxSurface                         = lockDxSurface;
    g_moduleState.actions.unlockDxSurface                       = unlockDxSurface;
    g_moduleState.actions.copyMainBackSurfaces                  = copyMainBackSurfaces;
    g_moduleState.actions.convertNotMagentaColors               = convertNotMagentaColors;
    g_moduleState.actions.convertAllColors                      = convertAllColors;
    g_moduleState.actions.getTextLength                         = getTextLength;
    g_moduleState.actions.FUN_100033c0                          = FUN_100033c0;
    g_moduleState.actions.FUN_10003360                          = FUN_10003360;
    g_moduleState.actions.callDrawBackSurfaceRhomb              = callDrawBackSurfaceRhomb;
    g_moduleState.actions.FUN_10001f10                          = FUN_10001f10;
    g_moduleState.actions.FUN_10004390                          = FUN_10004390;
    g_moduleState.actions.FUN_100046b6                          = FUN_100046b6;
    g_moduleState.actions.FUN_100049e6                          = FUN_100049e6;
    g_moduleState.actions.drawBackSurfacePaletteShadedSprite    = drawBackSurfacePaletteShadedSprite;
    g_moduleState.actions.FUN_10005ac6                          = FUN_10005ac6;
    g_moduleState.actions.drawBackSurfacePalletteSprite         = drawBackSurfacePalletteSprite;
    g_moduleState.actions.drawBackSurfaceShadowSprite           = drawBackSurfaceShadowSprite;
    g_moduleState.actions.copyBackToMainSurfaceRect             = copyBackToMainSurfaceRect;
    g_moduleState.actions.drawBackSurfaceColorPoint             = drawBackSurfaceColorPoint;
    g_moduleState.actions.FUN_10001ed0                          = FUN_10001ed0;
    g_moduleState.actions.FUN_10001f40                          = FUN_10001f40;
    g_moduleState.actions.drawMainSurfacePaletteSprite          = drawMainSurfacePaletteSprite;
    g_moduleState.actions.drawMainSurfaceSprite                 = drawMainSurfaceSprite;
    g_moduleState.actions.FUN_1000618d                          = FUN_1000618d;
    g_moduleState.actions.FUN_10004db0                          = FUN_10004db0;
    g_moduleState.actions.FUN_10006ef8                          = drawSurfaceUnitSprite;
    g_moduleState.actions.FUN_10007292                          = FUN_10007292;
    g_moduleState.actions.drawMainSurfaceAnimationSprite        = drawMainSurfaceAnimationSprite;
    g_moduleState.actions.FUN_100067ad                          = FUN_100067ad;
    g_moduleState.actions.FUN_10007662                          = FUN_10007662;
    g_moduleState.actions.FUN_10007fbc                          = FUN_10007fbc;
    g_moduleState.actions.FUN_10007be8                          = FUN_10007be8;
    g_moduleState.actions.drawMainSurfaceVanishingSprite        = drawMainSurfaceVanishingSprite;
    g_moduleState.actions.drawMainSurfaceColorPoint             = drawMainSurfaceColorPoint;
    g_moduleState.actions.drawMainSurfaceFilledColorRect        = drawMainSurfaceFilledColorRect;
    g_moduleState.actions.drawMainSurfaceColorRect              = drawMainSurfaceColorRect;
    g_moduleState.actions.drawMainSurfaceHorLine                = drawMainSurfaceHorLine;
    g_moduleState.actions.drawMainSurfaceVertLine               = drawMainSurfaceVertLine;
    g_moduleState.actions.drawMainSurfaceShadeColorRect         = drawMainSurfaceShadeColorRect;
    g_moduleState.actions.drawMainSurfaceColorOutline           = drawMainSurfaceColorOutline;
    g_moduleState.actions.drawMainSurfaceColorEllipse           = drawMainSurfaceColorEllipse;
    g_moduleState.actions.copyMainSurfaceToRendererWithWarFog   = copyMainSurfaceToRendererWithWarFog;
    g_moduleState.actions.copyMainSurfaceToRenderer             = copyMainSurfaceToRenderer;
    g_moduleState.actions.readMainSurfaceRect                   = readMainSurfaceRect;
    g_moduleState.actions.maskStencilSurfaceRect                = maskStencilSurfaceRect;
    g_moduleState.actions.drawStencilSurfaceWindowRect          = drawStencilSurfaceWindowRect;
    g_moduleState.actions.copyToRendererSurfaceRect             = copyToRendererSurfaceRect;
    g_moduleState.actions.copyPixelRectFromTo                   = copyPixelRectFromTo;
    g_moduleState.actions.FUN_10008ecd                          = FUN_10008ecd;
    g_moduleState.actions.FUN_1000a4f3                          = FUN_1000a4f3;
    g_moduleState.actions.FUN_10009eb3                          = FUN_10009eb3;
    g_moduleState.actions.releaseDxInstance                     = releaseDxInstance;

    return &g_moduleState;
}
