#pragma once

#include <array>
#include <chrono>
#include <random>
#include <functional>
#include <windows.h>

#include "SplashConfig.h"

class SplashTextRenderer
{
public:
    using FnSetColor = void(__thiscall*)(void*, int, int, int);
    using FnDrawText = void(__thiscall*)(void*, int, int, const char*, int);

    struct Params
    {
        FnSetColor setColorFn;
        FnDrawText drawTextFn;
        void* ctx;

        uint8_t r = 0xDE;
        uint8_t g = 0xD7;
        uint8_t b = 0x42;

        int x = 476;
        int y = 300;
        bool aligned = true;
    };

    static Params MakeParams(
        const SplashStaticParams& s,
        FnSetColor setColorFn,
        FnDrawText drawTextFn,
        void* ctx
    )
    {
        return {
            setColorFn,
            drawTextFn,
            ctx,
            s.r, s.g, s.b,
            s.x, s.y,
            s.aligned
        };
    }

    static SplashTextRenderer& Instance() {
        static SplashTextRenderer inst;
        return inst;
    }

    SplashTextRenderer()
        : rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()))
    {
    }

    void render(const Params& p)
    {
        if (!p.setColorFn || !p.drawTextFn || !p.ctx)
            return;

        DWORD now = (DWORD)GetTickCount64();

        if (now - lastShownTime > 100) {
            pickNewPhrase();
        }
        lastShownTime = now;

        float t = static_cast<float>(now % 1000) / 1000.f;
        float brightness = 0.8f + 0.2f * std::sin(t * 3.1415f * 2);

        const uint8_t r = static_cast<uint8_t>(p.r * brightness);
        const uint8_t g = static_cast<uint8_t>(p.g * brightness);
        const uint8_t b = static_cast<uint8_t>(p.b * brightness);

        __try
        {
            p.setColorFn(p.ctx, r, g, b);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            MessageBoxA(NULL, "EXCEPTION in setColorFn()", "FATAL", MB_OK);
        }
        __try
        {
            p.drawTextFn(p.ctx, p.x, p.y, funPhrases[currentIndex], p.aligned);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            MessageBoxA(NULL, "EXCEPTION in drawTextFn()", "FATAL", MB_OK);
        }
    }

private:
    void pickNewPhrase()
    {
        std::uniform_int_distribution<size_t> dist(0, funPhrases.size() - 1);
        size_t newIndex;
        do {
            newIndex = dist(rng);
        } while (newIndex == currentIndex);

        currentIndex = newIndex;
    }

    size_t currentIndex = 0;
    DWORD lastShownTime = 0;
    std::mt19937 rng;

    static constexpr std::array<const char*, 18> funPhrases = {
        "Keep calm and play Sudden Strike",
        "Ultimate HD mode activated!",
        "Beware of sneaky tanks!",
        "Victory is near!",
        "Coffee break time!",
        "Tsoi is alive... and commanding tanks",
        "Tanks fear Bulldozer... barely",
        "Bulldozer is a reliable dump provider!",
        "Koteus is taking notes... and tanks!",
        "Watch your back, Pufik is near",
        "Konan is the first tester!",
        "Bulldozer is the second tester!",
        "Dad taught me Sudden Strike!",
        "This mod has Dad's approval",
        "Dad's wisdom: always flank!",
        "Dad approves of this strategy!",
        "Minceraft!",
        "+1",
    };

    static_assert(funPhrases[funPhrases.size() - 1] != nullptr, "Last element of funPhrases is nullptr!");
};
