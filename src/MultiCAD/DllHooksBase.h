#pragma once

#include "GameGlobals.h"

template<typename Tag>
class DllHooksBase
{
protected:
    inline static GameGlobals* globals_{ nullptr };

public:
    static void init(uintptr_t moduleBase)
    {
        delete globals_;
        globals_ = new GameGlobals(moduleBase);
    }

    static void initRelocs(std::span<const RelocationHandle::GapRuntime> newGaps)
    {
        if (globals_)
            globals_->addReloc(newGaps);
    }

    static void shutdown()
    {
        delete globals_;
        globals_ = nullptr;
    }
};
