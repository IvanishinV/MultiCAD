#pragma once

#include "MemoryRelocator.h"
#include "CodePatcher.h"
#include "ProfileBase.h"
#include "LdrNotification.h"
#include "GameDllHooks.h"

#include <memory>
#include <format>

struct PatchSession
{
    RelocationHandle reloc;
    bool             active{ false };
    MemoryRelocator* relocator{};
    CodePatcher*     injector{};

    void Unapply()
    {
        if (!active)
            return;
        if (relocator)
            relocator->revert(reloc);
        active = false;
    }

    ~PatchSession()
    {
        Unapply();
    }
};

class PatchEngine
{
public:
    PatchEngine(std::unique_ptr<MemoryRelocator> relocator,
        std::unique_ptr<CodePatcher> injector)
        :
        relocator_(std::move(relocator)),
        injector_(std::move(injector))
    { }

    bool Apply(const ModuleInfo& mod, const IPatchProfile& profile, PatchSession& outSession)
    {
        outSession = {};
        outSession.relocator = relocator_.get();
        outSession.injector = injector_.get();

        // 1) Relocations first (so hooks see final addresses)
        if (!relocator_->apply(mod, profile.relocations(), outSession.reloc))
        {
#ifdef _DEBUG
            OutputDebugStringA("Couldn't patch relocations.\n");
#endif
            return false;
        }

        GameDllHooks::initRelocs(outSession.reloc.gaps);

        // 2) Hooks
        if (!injector_->apply(mod, profile.hooks(), profile.patches()))
        {
#ifdef _DEBUG
            OutputDebugStringA("Couldn't patch hooks.\n");
#endif
            relocator_->revert(outSession.reloc);
            return false;
        }

#ifdef _DEBUG
        OutputDebugStringA("Applied PatchEngine.\n");
#endif

        outSession.active = true;
        return true;
    }

    void Shutdown() { }

private:
    std::unique_ptr<MemoryRelocator> relocator_;
    std::unique_ptr<CodePatcher> injector_;

    PatchSession currentSession_;
};
