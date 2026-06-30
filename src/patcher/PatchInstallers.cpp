#include "pch.h"
#include "PatchInstallers.h"
#include "AudioHelper.h"
#include "UIFilter.h"

bool InstallGamePatches(TargetState& state, uintptr_t base, size_t size, const std::wstring& path)
{
    std::thread([] { AudioHelper::EnsureMaxVolume(); }).detach();

    DllVersionDetector& detector = DllVersionDetector::GetInstance();
    GameVersion version = detector.GetOrDetectGameVersion(DllType::Game, path, base, size);
    DetectionStatus status = detector.GetDetectionStatus(DllType::Game);

    ProfileFactory factory;
    auto profile = factory.create(version);

    switch (status)
    {
    case DetectionStatus::NotDetected:
    {
        ShowErrorAsync("MultiCAD couldn't detect game dll hash for some reason.");
        Screen::UpdateToOrigSize();
        return false;
    }
    case DetectionStatus::NotCalculated:
    {
        ShowErrorAsync("MultiCAD couldn't calculate game dll hash for some reason.");
        Screen::UpdateToOrigSize();
        return false;
    }
    case DetectionStatus::Supported:
    {
        // I check profile version separately, because dll can be identified, but there can be no profile for this version
        if (!profile->isUnknown())
            break;
        ShowErrorAsync("MultiCAD identified game dll, but doesn't have patches for it. The game will NOT work correctly. \nTo add support, contact the author of the mod.");
        Screen::UpdateToOrigSize();
        return false;
    }
    case DetectionStatus::UnsupportedHash:
    {
        ShowErrorAsync("MultiCAD couldn't identify game dll and doesn't try to patch it. The game will NOT work correctly. \nTo add support, contact the author of the mod.");
        Screen::UpdateToOrigSize();
        return false;
    }
    }

    // Resolve the game resolution here (and only here): native desktop by
    // default, overridden by sudtest.ini. The menu always runs at its own size,
    // so the high resolution must not be applied before the game dll loads.
    Screen::ApplyGameResolution();

    const auto& module = detector.GetModuleInfo(DllType::Game);
    GameDllHooks::init(module.base);
    state.patchEngine.emplace(
        std::make_unique<MemoryRelocator>(),
        std::make_unique<CodePatcher>()
    );

    if (!state.patchEngine->Apply(module, profile->game(), state.patchSession))
    {
        GameDllHooks::shutdown();
        state.patchEngine.reset();

        GetUIFilter().setEnabled(true);

        ShowErrorAsync("Couldn't patch game dll due to some error. Contact the author.");
        Screen::UpdateToOrigSize();
        return false;
    }

    return true;
}

bool UninstallGamePatches(TargetState& state)
{
    if (state.active)
    {
        state.patchSession.Unapply();
        state.patchEngine.reset();
    }

    GameDllHooks::shutdown();

    GetUIFilter().setEnabled(true);

    return true;
}

bool InstallMenuPatches(TargetState& state, uintptr_t base, size_t size, const std::wstring& path)
{
    std::thread([] { AudioHelper::EnsureMaxVolume(); }).detach();

    DllVersionDetector& detector = DllVersionDetector::GetInstance();
    GameVersion version = detector.GetOrDetectGameVersion(DllType::Menu, path, base, size);
    DetectionStatus status = detector.GetDetectionStatus(DllType::Menu);

    ProfileFactory factory;
    auto profile = factory.create(version);

    switch (status)
    {
    case DetectionStatus::NotDetected:
    {
        ShowErrorAsync("MultiCAD couldn't detect menu dll hash for some reason.");
        return false;
    }
    case DetectionStatus::NotCalculated:
    {
        ShowErrorAsync("MultiCAD couldn't calculate menu dll hash for some reason.");
        return false;
    }
    case DetectionStatus::Supported:
    {
        // I check profile version separately, because dll can be identified, but there can be no profile for this version
        if (!profile->isUnknown())
            break;
        ShowErrorAsync("MultiCAD identified menu dll, but doesn't have patches for it. \nTo add support, contact the author of the mod.");
        return false;
    }
    case DetectionStatus::UnsupportedHash:
    {
        ShowErrorAsync("MultiCAD couldn't identify menu dll and doesn't try to patch it. \nTo add support, contact the author of the mod.");
        return false;
    }
    }

    const auto& module = detector.GetModuleInfo(DllType::Menu);
    MenuDllHooks::init(module.base);
    state.patchEngine.emplace(
        std::make_unique<MemoryRelocator>(),
        std::make_unique<CodePatcher>()
    );

    if (!state.patchEngine->Apply(module, profile->menu(), state.patchSession))
    {
        MenuDllHooks::shutdown();
        state.patchEngine.reset();

        ShowErrorAsync("Couldn't patch game dll due to some error. Contact the author.");
        return false;
    }

    return true;
}

bool UninstallMenuPatches(TargetState& state)
{
    if (state.active)
    {
        state.patchSession.Unapply();
        state.patchEngine.reset();
    }

    MenuDllHooks::shutdown();

    return true;
}

std::vector<TargetInfo> CreatePatchTargets()
{
    std::vector<TargetInfo> targets;
    
    // Default Sudden Strike dll files with "game" and "menu" in its names
    {
        TargetInfo gameTarget;
        gameTarget.namePart = ToDllName(DllType::Game);
        gameTarget.onLoaded = InstallGamePatches;
        gameTarget.onUnloaded = UninstallGamePatches;
        targets.push_back(std::move(gameTarget));

        TargetInfo menuTarget;
        menuTarget.namePart = ToDllName(DllType::Menu);
        menuTarget.onLoaded = InstallMenuPatches;
        menuTarget.onUnloaded = UninstallMenuPatches;
        targets.push_back(std::move(menuTarget));
    }

    // Confrontation: Gulf War
    {
        TargetInfo gameTargetGulfWar;
        gameTargetGulfWar.namePart = ToDllName(DllType::GameGulfWar);
        gameTargetGulfWar.onLoaded = InstallGamePatches;
        gameTargetGulfWar.onUnloaded = UninstallGamePatches;
        targets.push_back(std::move(gameTargetGulfWar));

        TargetInfo menuTargetGulfWar;
        menuTargetGulfWar.namePart = ToDllName(DllType::MenuGulfWar);
        menuTargetGulfWar.onLoaded = InstallMenuPatches;
        menuTargetGulfWar.onUnloaded = UninstallMenuPatches;
        targets.push_back(std::move(menuTargetGulfWar));
    }

    // Confrontation: Black Gold
    {
        TargetInfo gameTargetBlackGold;
        gameTargetBlackGold.namePart = ToDllName(DllType::GameBlackGold);
        gameTargetBlackGold.onLoaded = InstallGamePatches;
        gameTargetBlackGold.onUnloaded = UninstallGamePatches;
        targets.push_back(std::move(gameTargetBlackGold));

        TargetInfo menuTargetBlackGold;
        menuTargetBlackGold.namePart = ToDllName(DllType::MenuBlackGold);
        menuTargetBlackGold.onLoaded = InstallMenuPatches;
        menuTargetBlackGold.onUnloaded = UninstallMenuPatches;
        targets.push_back(std::move(menuTargetBlackGold));
    }

    // Confrontation: Europe 2015
    {
        TargetInfo gameTargetEurope2015;
        gameTargetEurope2015.namePart = ToDllName(DllType::GameEurope2015);
        gameTargetEurope2015.onLoaded = InstallGamePatches;
        gameTargetEurope2015.onUnloaded = UninstallGamePatches;
        targets.push_back(std::move(gameTargetEurope2015));

        TargetInfo menuTargetEurope2015;
        menuTargetEurope2015.namePart = ToDllName(DllType::MenuEurope2015);
        menuTargetEurope2015.onLoaded = InstallMenuPatches;
        menuTargetEurope2015.onUnloaded = UninstallMenuPatches;
        targets.push_back(std::move(menuTargetEurope2015));
    }

    // Confrontation: Black Sea
    {
        TargetInfo gameTargetBlackSea;
        gameTargetBlackSea.namePart = ToDllName(DllType::GameBlackSea);
        gameTargetBlackSea.onLoaded = InstallGamePatches;
        gameTargetBlackSea.onUnloaded = UninstallGamePatches;
        targets.push_back(std::move(gameTargetBlackSea));

        TargetInfo menuTargetBlackSea;
        menuTargetBlackSea.namePart = ToDllName(DllType::MenuBlackSea);
        menuTargetBlackSea.onLoaded = InstallMenuPatches;
        menuTargetBlackSea.onUnloaded = UninstallMenuPatches;
        targets.push_back(std::move(menuTargetBlackSea));
    }

    return targets;
}
