#include "pch.h"
#include "PatchInstallers.h"
#include "AudioHelper.h"

bool InstallGamePatches(TargetState& state, uintptr_t base, size_t size, const std::wstring& path)
{
    DllVersionDetector& detector = DllVersionDetector::GetInstance();
    GameVersion version = detector.GetOrDetectGameVersion(DllType::Game, path, base, size);
    DetectionStatus status = detector.GetDetectionStatus(DllType::Game);

    if (status == DetectionStatus::UnsupportedHash)
    {
        ShowErrorAsync("MultiCAD couldn't identify game dll and doesn't fully support this version of Sudden Strike. The mod may not work correctly. \nTo add support, contact the author of the mod.");
        return false;
    }

    if (status != DetectionStatus::Supported)
    {
        return false;
    }

    ProfileFactory factory;
    auto profile = factory.create(version);
    if (!profile)
    {
        ShowErrorAsync("This is most likely a build error. MultiCAD identified game dll, but couldn't find relevant patches. Contact the author.");
        return false;
    }

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

        ShowErrorAsync("Couldn't patch game dll due to some error. Contact the author.");
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

    return true;
}

bool InstallMenuPatches(TargetState& state, uintptr_t base, size_t size, const std::wstring& path)
{
    std::thread([] { AudioHelper::EnsureMaxVolume(); }).detach();

    DllVersionDetector& detector = DllVersionDetector::GetInstance();
    GameVersion version = detector.GetOrDetectGameVersion(DllType::Menu, path, base, size);
    DetectionStatus status = detector.GetDetectionStatus(DllType::Menu);

    if (status == DetectionStatus::UnsupportedHash)
    {
        ShowErrorAsync("MultiCAD couldn't identify menu dll and doesn't fully support this version of Sudden Strike. The mod may not work correctly. \nTo add support, contact the author of the mod.");
        return false;
    }

    if (status != DetectionStatus::Supported)
    {
        return false;
    }

    ProfileFactory factory;
    auto profile = factory.create(version);
    if (!profile)
    {
        ShowErrorAsync("This is most likely a build error. MultiCAD identified menu dll, but couldn't find relevant patches. Contact the author.");
        return false;
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

    TargetInfo gameTarget;
    gameTarget.namePart = L"game_dll";
    gameTarget.onLoaded = InstallGamePatches;
    gameTarget.onUnloaded = UninstallGamePatches;
    targets.push_back(std::move(gameTarget));

    TargetInfo menuTarget;
    menuTarget.namePart = L"menu_dll";
    menuTarget.onLoaded = InstallMenuPatches;
    menuTarget.onUnloaded = UninstallMenuPatches;
    targets.push_back(std::move(menuTarget));

    return targets;
}
