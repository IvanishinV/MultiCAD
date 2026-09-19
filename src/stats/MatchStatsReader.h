#pragma once

#include "GameGlobals.h"
#include "MatchStats.h"
#include "types.h"

// Reads the per-player records the menu dll renders on its end-of-game screen.
// Nothing here draws or patches - the array is live while the screen is up, so a
// hook only has to say when to look.
namespace Stats
{
    struct StatsLayout
    {
        uintptr_t firstRecordRva{ 0 };
        size_t    slotCount{ 0 };

        // The lobby table is the only source of nation. It is not in the same
        // order as the records (those are sorted by score), so the two are
        // matched by player name.
        uintptr_t lobbyFirstRecordRva{ 0 };
        size_t    lobbySlotCount{ 0 };

        // A finished match has no fixed address; the records live in the session
        // object, which the results screen reaches as:
        //
        //     mov eax, [sessionPtrRva]
        //     lea ebx, [eax + sessionRecordsOffset]
        //
        // `firstRecordRva` is a different table - saved per-map records from a
        // .rts file - and is never reported, see ReadMatch.
        uintptr_t sessionPtrRva{ 0 };
        uintptr_t sessionRecordsOffset{ 0 };

        // Elapsed seconds, in the dword after the record array. The records
        // carry no time; the field the saved table uses stays zero here.
        uintptr_t sessionDurationOffset{ 0 };

        // Index of the player at this machine, one-based: subtract one for a slot.
        uintptr_t sessionLocalIndexOffset{ 0 };
    };

    // Sudden Strike 2 v2.2 - shared by stock v2.2, Hidden Stroke 2 and FMRM.
    // Twelve slots, same `cmp eax, 0Ch` as RW2.4; the session layout is
    // identical too, only the pointer moves.
    inline constexpr StatsLayout kLayoutSs2V22{ 0x0009F9C8, 12, 0x000A9B24, 12, 0x000B5828, 0x48, 0x618, 0x3C };

    // Resource War v2.4 - shared by RWG 3.6. Twelve slots, matching the
    // `cmp eax, 0Ch` in the results screen's own loop.
    inline constexpr StatsLayout kLayoutRwV24{ 0x0009DAE8, 12, 0x000A7C44, 12, 0x000B3958, 0x48, 0x618, 0x3C };

    // Null for a version whose addresses were never verified.
    const StatsLayout* LayoutFor(GameVersion version);

    // False, leaving `out` untouched, when no session is mapped or no slot holds
    // a player.
    bool ReadMatch(GameGlobals& globals, const StatsLayout& layout, MatchStats& out);

    // The menu dll is unloaded for the whole match, so by report time its lobby
    // table has been reinitialised. This dll stays loaded and keeps the last
    // populated copy. Cheap enough to call on every screen change.
    void CaptureLobby(GameGlobals& globals, const StatsLayout& layout);
}
