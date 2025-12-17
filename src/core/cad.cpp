#include "pch.h"
#include "cad.h"
#include "renderer.h"
#include "DllVersionDetector.h"

static ModuleStateSSGold_INT  g_moduleStateRelease;
static ModuleStateSSGold_RU   g_moduleStateDebug;

ModuleStateBase* g_moduleState{ nullptr };

inline void InitActionsPrefix(RendererActionsPrefix& a)
{
    a.initValues                                = initValues;
    a.initDxInstance                            = initDxInstance;
    a.restoreDxInstance                         = restoreDxInstance;
    a.initWindowDxSurface                       = initWindowDxSurface;
    a.setPixelColorMasks                        = setPixelColorMasks;
    a.releaseDxSurface                          = releaseDxSurface;
    a.lockDxSurface                             = lockDxSurface;
    a.unlockDxSurface                           = unlockDxSurface;
    a.copyMainBackSurfaces                      = copyMainBackSurfaces;
    a.convertNotMagentaColors                   = convertNotMagentaColors;
    a.convertAllColors                          = convertAllColors;
    a.getTextLength                             = getTextLength;
    a.drawBackSurfaceText                       = drawBackSurfaceText;
    a.drawMainSurfaceText                       = drawMainSurfaceText;
    a.callDrawBackSurfacePaletteRhomb           = callDrawBackSurfacePaletteRhomb;
    a.callDrawBackSurfaceMaskRhomb              = callDrawBackSurfaceMaskRhomb;
    a.drawBackSurfaceRhombsPaletteSprite        = drawBackSurfaceRhombsPaletteSprite;
    a.drawBackSurfaceRhombsPaletteSprite2       = drawBackSurfaceRhombsPaletteSprite2;
    a.drawBackSurfaceRhombsPaletteShadedSprite  = drawBackSurfaceRhombsPaletteShadedSprite;
    a.drawBackSurfacePaletteShadedSprite        = drawBackSurfacePaletteShadedSprite;
    a.drawBackSurfacePaletteSpriteAndStencil    = drawBackSurfacePaletteSpriteAndStencil;
    a.drawBackSurfacePalletteSprite             = drawBackSurfacePalletteSprite;
    a.drawBackSurfaceShadowSprite               = drawBackSurfaceShadowSprite;
    a.copyBackToMainSurfaceRect                 = copyBackToMainSurfaceRect;
    a.drawBackSurfaceColorPoint                 = drawBackSurfaceColorPoint;
    a.callShadeMainSurfaceRhomb                 = callShadeMainSurfaceRhomb;
    a.callCleanMainSurfaceRhomb                 = callCleanMainSurfaceRhomb;
    a.drawMainSurfacePaletteSpriteCompact       = drawMainSurfacePaletteSpriteCompact;
    a.drawMainSurfaceSprite                     = drawMainSurfaceSprite;
    a.drawMainSurfacePaletteSprite              = drawMainSurfacePaletteSprite;
    a.drawMainSurfacePaletteSpriteStencil       = drawMainSurfacePaletteSpriteStencil;
    a.drawMainSurfacePaletteSpriteFrontStencil  = drawMainSurfacePaletteSpriteFrontStencil;
    a.drawMainSurfacePaletteSpriteBackStencil   = drawMainSurfacePaletteSpriteBackStencil;
    a.drawMainSurfaceAnimationSpriteStencil     = drawMainSurfaceAnimationSpriteStencil;
    a.blendMainSurfaceWithWarFog_0              = blendMainSurfaceWithWarFog;
}

inline void InitActionsAnimationSprite(RendererActionsAnimationSprite& a)
{
    a.drawMainSurfaceAnimationSprite = drawMainSurfaceAnimationSprite;
}

inline void InitActionsPostfix(RendererActionsPostfix& p)
{
    p.drawMainSurfaceShadowSprite           = drawMainSurfaceShadowSprite;
    p.drawMainSurfaceActualSprite           = drawMainSurfaceActualSprite;
    p.drawMainSurfaceAdjustedSprite         = drawMainSurfaceAdjustedSprite;
    p.drawMainSurfaceVanishingPaletteSprite = drawMainSurfaceVanishingPaletteSprite;
    p.drawMainSurfaceColorPoint             = drawMainSurfaceColorPoint;
    p.drawMainSurfaceFilledColorRect        = drawMainSurfaceFilledColorRect;
    p.drawMainSurfaceColorRect              = drawMainSurfaceColorRect;
    p.drawMainSurfaceHorLine                = drawMainSurfaceHorLine;
    p.drawMainSurfaceVertLine               = drawMainSurfaceVertLine;
    p.drawMainSurfaceShadeColorRect         = drawMainSurfaceShadeColorRect;
    p.drawMainSurfaceColorOutline           = drawMainSurfaceColorOutline;
    p.drawMainSurfaceColorEllipse           = drawMainSurfaceColorEllipse;
    p.copyMainSurfaceToRendererWithWarFog   = copyMainSurfaceToRendererWithWarFog;
    p.copyMainSurfaceToRenderer             = copyMainSurfaceToRenderer;
    p.readMainSurfaceRect                   = readMainSurfaceRect;
    p.maskStencilSurfaceRect                = maskStencilSurfaceRect;
    p.resetStencilSurface                   = resetStencilSurface;
    p.copyToRendererSurfaceRect             = copyToRendererSurfaceRect;
    p.copyPixelRectFromTo                   = copyPixelRectFromTo;
    p.drawUiSprite                          = drawUiSprite;
    p.drawVanishingUiSprite                 = drawVanishingUiSprite;
    p.markUiWithButtonType                  = markUiWithButtonType;
    p.releaseDxInstance                     = releaseDxInstance;
    p.blendMainSurfaceWithWarFog_1          = blendMainSurfaceWithWarFog;
}

template<typename ModuleState>
void InitModuleState(ModuleState& s)
{
    s.surface.main = g_rendererState.surfaces.main;
    s.surface.back = g_rendererState.surfaces.back;
    s.surface.stencil = g_rendererState.surfaces.stencil;

    initValues();

    InitActionsPrefix(s.actions);

    if constexpr (requires { s.actionsPostfix; })
    {
        InitActionsPostfix(s.actionsPostfix);
    }

    if constexpr (std::derived_from<ModuleState, RendererActionsAnimationSprite>)
    {
        InitActionsAnimationSprite(s.actionsPostfix);
    }
}

void* InitSSGoldRelease()
{
    g_moduleState = &g_moduleStateRelease;

    InitModuleState(g_moduleStateRelease);

    return &g_moduleStateRelease.windowRect;
}

void* InitSSGoldDebug()
{
    g_moduleState = &g_moduleStateDebug;

    InitModuleState(g_moduleStateDebug);

    return &g_moduleStateDebug.windowRect;
}

void* InitializeModule()
{
#pragma comment(linker, "/EXPORT:" "CADraw_Init=" __FUNCDNAME__)

    DllVersionDetector& detector = DllVersionDetector::GetInstance();

    detector.DetectFileDllVersion(DllType::Menu, L"menu_dll");
    const GameVersion menuDllVersion = detector.GetGameVersion(DllType::Menu);

    switch (menuDllVersion)
    {
    case GameVersion::SS_GOLD_RU:
    case GameVersion::SS_RU:
    {
        return InitSSGoldDebug();
    }
    case GameVersion::UNKNOWN:
    {
        ShowErrorNow("MultiCAD couldn't identify menu dll and doesn't fully support this version of Sudden Strike. The mod may not work correctly. \nTo add support, contact the author of the mod.");
        [[fallthrough]];
    }
    default:
        return InitSSGoldRelease();
    }
}
