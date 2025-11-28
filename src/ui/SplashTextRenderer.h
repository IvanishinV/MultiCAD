#pragma once

#include <array>
#include <chrono>
#include <random>
#include <functional>
#include <windows.h>

class SplashTextRenderer
{
public:
    using FnSetColor = void(__thiscall*)(void*, int, int, int);
    using FnDrawText = void(__thiscall*)(void*, int, int, const char*, int);

    static SplashTextRenderer& Instance() {
        static SplashTextRenderer inst;
        return inst;
    }

    SplashTextRenderer()
        : rng(static_cast<unsigned>(std::chrono::steady_clock::now().time_since_epoch().count()))
    {
    }

    void render(FnSetColor setColorFn, FnDrawText drawTextFn, void* ctx)
    {
        if (!setColorFn || !drawTextFn || !ctx)
            return;

        DWORD now = (DWORD)GetTickCount64();

        if (now - lastShownTime > 100) {
            pickNewPhrase();
        }
        lastShownTime = now;

        float t = static_cast<float>(now % 1000) / 1000.f;
        float brightness = 0.8f + 0.2f * std::sin(t * 3.1415f * 2);

        uint8_t r = static_cast<uint8_t>(0xDE * brightness);
        uint8_t g = static_cast<uint8_t>(0xD7 * brightness);
        uint8_t b = static_cast<uint8_t>(0x42 * brightness);

        __try
        {
            setColorFn(ctx, r, g, b);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            MessageBoxA(NULL, "EXCEPTION in setColorFn()", "FATAL", MB_OK);
        }
        __try
        {
            drawTextFn(ctx, 476, 300, funPhrases[currentIndex], 0);
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

    static constexpr std::array<const char*, 16> funPhrases = {
        "Keep calm and play SS",
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
        "Dad taught me Sudden Strike!",
        "This mod has Dad's approval",
        "Dad's wisdom: always flank!",
        "Dad would approve of this strategy",
        "Minceraft!"
    };

    static_assert(funPhrases[funPhrases.size() - 1] != nullptr, "Last element of funPhrases is nullptr!");
};
