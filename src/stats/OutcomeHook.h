#pragma once

#include <string>

#include "GameGlobals.h"
#include "MatchStats.h"
#include "types.h"

// Per-player win/lose, captured from the game dll while the match runs.
//
// The menu record carries no outcome - the field its multi.log writer tests is
// never written, which is why the game always says "YOU LOST". The verdict only
// exists as the game dll's per-player announcement, and that dll is gone before
// the results screen appears.
namespace Stats
{
    // Patches the game dll's message poster. Guarded on the version and on the
    // bytes at the target: a blind 5-byte jmp into another build corrupts whatever
    // lives there. Either guard failing costs the outcomes, not the report.
    bool InstallOutcomeHook(GameGlobals& globals, GameVersion version);

    // Called at match start, so a disconnect cannot leak a verdict into the next
    // report.
    void ResetOutcomes();

    // Matched on the id the game record keeps at +0x21, the same one the menu
    // record carries, so players sharing a nickname stay apart. Name is only a
    // fallback; Unknown when neither matches.
    MatchOutcome OutcomeFor(S32 netId, const std::string& name);

    // Separate from OutcomeFor so a quitter still reports the result of the
    // match they walked out of.
    bool LeftEarly(S32 netId, const std::string& name);

    // The sender's own id, which names a record outright. -1 when this machine's
    // own player was never announced.
    S32 LocalPlayerNetId();

    // 0..3, or -1 if never read. Sampled during the match, not at patch time:
    // the global is zero-initialised BSS and 0 is also a valid scheme (summer),
    // so an early read cannot be told apart from a real one.
    S32 MapScheme();
}
