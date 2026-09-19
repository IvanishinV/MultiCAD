#include "pch.h"
#include "OutcomeHook.h"

#include "StatsReporter.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <mutex>
#include <vector>

namespace
{
    // One address set per game dll build.
    //
    // The poster is hooked, not the emitter that calls it: the patcher has no
    // trampoline, so taking the emitter would mean transcribing its score loops
    // whole. Nothing is lost by dropping down one level - the emitter writes the
    // player index to V1 and passes table[ITXT_OBR + outcome] straight through,
    // so both are readable from inside the poster.
    struct OutcomeLayout
    {
        uintptr_t posterRva;        // hooked
        uintptr_t postInnerRva;     // what the poster forwards to
        uintptr_t textTableRva;     // char**, resolved from dat/lng
        uintptr_t playerTableRva;   // player records, stride 0xB5
        uintptr_t var1Rva;          // token variable V1

        // Index of the player at this machine, which the emitter compares the
        // announced player against.
        uintptr_t localPlayerRva;

        // Landscape scheme: the sixth dword of XCHNG\ToGame\map_info, clamped by
        // the loader to 0..3. Zero-initialised BSS also reads as 0 (summer), so
        // it only means anything once a map is loaded - hence sampling it from
        // the hook rather than at patch time.
        uintptr_t mapSchemeRva;
    };

    // Resource War v2.4, shared by RWG 3.6.
    constexpr OutcomeLayout kOutcomeRwV24
    {
        0x00046830, 0x0009F400, 0x00E04864, 0x008C2248,
        0x010ADDA8, 0x010AE434, 0x001313A4,
    };

    // Stock v2.2, Hidden Stroke 2 and FMRM 2.1.5.3. FMRM ships this ASPack-packed,
    // but the patcher runs against the unpacked image, so the RVAs still hold.
    constexpr OutcomeLayout kOutcomeSs2V22
    {
        0x00046D10, 0x000A2930, 0x00DD54F0, 0x00892FB0,
        0x0106E9D0, 0x0106F05C, 0x0014238C,
    };

    const OutcomeLayout* OutcomeLayoutFor(const GameVersion version)
    {
        switch (version)
        {
        case GameVersion::SS_RW_V2_4:
            return &kOutcomeRwV24;

        case GameVersion::SS_2:
        case GameVersion::HS_2:
        case GameVersion::FMRM_2_1_5_3:
            return &kOutcomeSs2V22;

        default:
            return nullptr;
        }
    }

    constexpr size_t kPlayerStride  = 0xB5;
    constexpr size_t kPlayerSlots   = 13;
    constexpr size_t kPlayerNameCap = 0x20;

    // The same 2-byte id the menu record keeps at +0x20. The mis_players parser
    // copies it in at this offset, after the name and the status byte.
    constexpr size_t kPlayerNetId   = 0x21;

    // ITXT_OBR and its variants, verified against Dat/lng: 46 tie, 47 won,
    // 48 lost, 49 left the game, 50..51 empty but still reachable.
    constexpr size_t kItxtObr    = 46;
    constexpr int    kOutcomeMax = 5;

    // First 13 bytes of the poster. All esp-relative, so no relocation rewrites
    // them and they can be compared against the loaded image directly.
    constexpr U8 kPosterSignature[]
    {
        0x8B, 0x4C, 0x24, 0x10,   // mov ecx, [esp+0x10]
        0x8B, 0x54, 0x24, 0x0C,   // mov edx, [esp+0x0C]
        0x8D, 0x44, 0x24, 0x14,   // lea eax, [esp+0x14]
        0x50,                     // push eax
    };

    // The poster is a thunk onto this. __thiscall in the original: self in ecx,
    // everything else on the stack - which is what __fastcall produces once a
    // second register argument is declared and left unused.
    using PostInnerFn = void(__fastcall)(void* self, int edxUnused,
                                         int a2, int a3, const char* text, void* varargs);

    PostInnerFn*        g_inner{ nullptr };
    const char* const** g_textTable{ nullptr };
    const char*         g_playerTable{ nullptr };
    const S32*          g_var1{ nullptr };
    const S32*          g_localPlayer{ nullptr };
    const S32*          g_mapScheme{ nullptr };

    // One per announced player. Keyed by the game dll's own slot index, which
    // is unique - a nickname is not.
    struct Capture
    {
        S32         index{ -1 };
        S32         netId{ -1 };
        std::string name;
        int         code{ 0 };
    };

    std::mutex           g_outcomeMutex;
    std::vector<Capture> g_outcomes;
    S32                  g_scheme{ -1 };
    S32                  g_localIndex{ -1 };

    bool CodeForNetId(const S32 netId, int& code)
    {
        if (netId < 0)
            return false;

        for (const Capture& capture : g_outcomes)
        {
            if (capture.netId != netId)
                continue;

            code = capture.code;
            return true;
        }

        return false;
    }

    // Two players may share a nickname. If their verdicts differ nothing says
    // which row is which, so the caller is told nothing rather than something
    // wrong.
    bool CodeForName(const std::string& name, int& code)
    {
        bool found = false;

        for (const Capture& capture : g_outcomes)
        {
            if (capture.name != name)
                continue;

            if (!found)
            {
                code  = capture.code;
                found = true;
            }
            else if (code != capture.code)
            {
                return false;
            }
        }

        return found;
    }

    std::string ReadName(const char* name, size_t capacity)
    {
        size_t length = 0;
        while (length < capacity && name[length] != '\0')
            ++length;

        return std::string(name, length);
    }

    const char* PlayerRecord(S32 index)
    {
        if (g_playerTable == nullptr || index < 0 || static_cast<size_t>(index) >= kPlayerSlots)
            return nullptr;

        return g_playerTable + static_cast<size_t>(index) * kPlayerStride;
    }

    // Recognised by pointer identity against the table, not by contents: the text
    // arriving here is still the unsubstituted template, and this way a wrong
    // table address never matches rather than matching the wrong thing.
    bool OutcomeOfText(const char* text, int& outcome)
    {
        if (text == nullptr || g_textTable == nullptr)
            return false;

        const char* const* table = *g_textTable;
        if (table == nullptr)
            return false;

        for (int i = 0; i <= kOutcomeMax; ++i)
        {
            if (table[kItxtObr + i] == text)
            {
                outcome = i;
                return true;
            }
        }

        return false;
    }

    void NoteOutcome(const char* text)
    {
        int outcome = 0;
        if (!OutcomeOfText(text, outcome))
            return;

        const S32   index  = g_var1 != nullptr ? *g_var1 : -1;
        const char* record = PlayerRecord(index);

        if (record == nullptr)
            return;

        std::string name = ReadName(record, kPlayerNameCap);
        if (name.empty())
            return;

        const S32 netId = *reinterpret_cast<const U16*>(record + kPlayerNetId);

        const S32 local = g_localPlayer != nullptr ? *g_localPlayer : -1;

        {
            std::lock_guard<std::mutex> lock(g_outcomeMutex);

            auto existing = std::find_if(g_outcomes.begin(), g_outcomes.end(),
                                         [index](const Capture& c) { return c.index == index; });

            if (existing != g_outcomes.end())
                existing->code = outcome;
            else
                g_outcomes.push_back(Capture{ index, netId, name, outcome });

            // The emitter compares the announced player against the game dll's
            // own local index, so this is where the sender is known - and it
            // still knows after that player has walked out.
            if (index == local)
                g_localIndex = index;
        }
    }

    // A posted message means the match is running, so the map is loaded and the
    // scheme global means something. Cheaper than being told when loading ended.
    void SampleMapScheme()
    {
        if (g_mapScheme == nullptr)
            return;

        const S32 scheme = *g_mapScheme;
        if (scheme < 0 || scheme > 3)
            return;

        std::lock_guard<std::mutex> lock(g_outcomeMutex);
        g_scheme = scheme;
    }

    // Stands in for the poster, transcribed from it. va_start supplies the
    // `lea eax,[esp+0x14]` the original passes as its varargs pointer.
    void __cdecl PostMessageDetour(void* self, int a2, int a3, const char* text, ...)
    {
        SampleMapScheme();
        NoteOutcome(text);

        va_list args;
        va_start(args, text);
        g_inner(self, 0, a2, a3, text, args);
        va_end(args);
    }

    bool WriteJump(U8* target, const void* detour)
    {
        constexpr size_t kJumpSize = 5;

        DWORD oldProtect{};
        if (!VirtualProtect(target, kJumpSize, PAGE_EXECUTE_READWRITE, &oldProtect))
            return false;

        const intptr_t rel = reinterpret_cast<intptr_t>(detour)
                           - reinterpret_cast<intptr_t>(target) - static_cast<intptr_t>(kJumpSize);

        target[0] = 0xE9;
        *reinterpret_cast<int32_t*>(target + 1) = static_cast<int32_t>(rel);

        VirtualProtect(target, kJumpSize, oldProtect, &oldProtect);
        FlushInstructionCache(GetCurrentProcess(), target, kJumpSize);

        return true;
    }
}

namespace Stats
{
    bool InstallOutcomeHook(GameGlobals& globals, const GameVersion version)
    {
        // Guessing at a build whose addresses were never read would patch
        // whatever happens to sit at that offset.
        const OutcomeLayout* layout = OutcomeLayoutFor(version);
        if (layout == nullptr)
        {
#ifdef _DEBUG
            OutputDebugStringA(("[MultiCAD] outcome hook: no addresses for version "
                                 + std::to_string(static_cast<int>(version)) + "\n").c_str());
#endif
            return false;
        }

        auto* poster = globals.getPtr<U8>(layout->posterRva);

        if (std::memcmp(poster, kPosterSignature, sizeof(kPosterSignature)) != 0)
        {
            std::string got;
            for (size_t i = 0; i < sizeof(kPosterSignature); ++i)
            {
                char byte[8];
                std::snprintf(byte, sizeof(byte), "%02X", poster[i]);
                got += byte;
            }

#ifdef _DEBUG
            OutputDebugStringA(("[MultiCAD] outcome hook: signature mismatch, got " + got + "\n").c_str());
#endif
            return false;
        }

        g_inner       = globals.getFn<PostInnerFn>(layout->postInnerRva);
        g_textTable   = globals.getPtr<const char* const*>(layout->textTableRva);
        g_playerTable = globals.getPtr<const char>(layout->playerTableRva);
        g_var1        = globals.getPtr<const S32>(layout->var1Rva);
        g_localPlayer = globals.getPtr<const S32>(layout->localPlayerRva);
        g_mapScheme   = globals.getPtr<const S32>(layout->mapSchemeRva);

        if (!WriteJump(poster, reinterpret_cast<const void*>(&PostMessageDetour)))
        {
#ifdef _DEBUG
            OutputDebugStringA("[MultiCAD] outcome hook: could not write the jump\n");
#endif
            return false;
        }

        return true;
    }

    void ResetOutcomes()
    {
        std::lock_guard<std::mutex> lock(g_outcomeMutex);
        g_outcomes.clear();
        g_scheme     = -1;
        g_localIndex = -1;
    }

    S32 LocalPlayerNetId()
    {
        std::lock_guard<std::mutex> lock(g_outcomeMutex);

        for (const Capture& capture : g_outcomes)
        {
            if (capture.index == g_localIndex)
                return capture.netId;
        }

        return -1;
    }

    bool LeftEarly(const S32 netId, const std::string& name)
    {
        std::lock_guard<std::mutex> lock(g_outcomeMutex);

        // 3 is the announcement that names leaving explicitly. 4 and 5 exist in
        // the switch but have no text of their own, so nothing is claimed about
        // them here.
        int code = 0;

        return (CodeForNetId(netId, code) || CodeForName(name, code)) && code == 3;
    }

    S32 MapScheme()
    {
        std::lock_guard<std::mutex> lock(g_outcomeMutex);
        return g_scheme;
    }

    MatchOutcome OutcomeFor(const S32 netId, const std::string& name)
    {
        std::lock_guard<std::mutex> lock(g_outcomeMutex);

        // By id first: it is unique where a nickname is not.
        int code = 0;
        if (!CodeForNetId(netId, code) && !CodeForName(name, code))
            return MatchOutcome::Unknown;

        // 3 is "left the game" and 4..5 have no text of their own. The game
        // itself folds all three into a defeat for the local player, so they are
        // reported the same way here.
        switch (code)
        {
        case 0:  return MatchOutcome::Draw;
        case 1:  return MatchOutcome::Won;
        default: return MatchOutcome::Lost;
        }
    }
}
