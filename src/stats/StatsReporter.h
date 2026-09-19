#pragma once

#include <string>

#include "MatchStats.h"

// Posts a finished match to [Game] StatsUrl in the game ini. No StatsUrl, no
// reporting - a record carries player names, so it must not leave the machine
// unless someone configured it to.
namespace Stats
{
    // Names arrive over the network from other players, so everything is bounded.
    constexpr size_t kMaxPlayers    = 16;
    constexpr size_t kMaxNameLength = 64;
    constexpr size_t kMaxBodyBytes  = 64 * 1024;

    // The map is named only in XCHNG\ToGame\mis_desc, which the menu deletes at
    // match end, so it has to be read while the match runs - from this dll,
    // which outlives the menu dll. Already UTF-8, unlike anything read from
    // memory.
    void CaptureMapName();
    std::string MapName();

    // This machine's clock. Two clients enter one match a second apart even on
    // one machine, so it is for display, never a match identity.
    void MarkMatchStart();
    std::string MatchStartedAt();

    // Kept in [Game] InstallId and nowhere else, which makes it per install
    // rather than per machine - two copies of the game can corroborate each
    // other. Identifies an install, never authenticates one: the player's own
    // machine writes both the id and the body it travels in.
    std::string InstallId();

    bool IsReportingEnabled();

    // Serialises here, posts from a detached thread. Silent by design - a report
    // must never interrupt the game.
    void Submit(const MatchStats& match);

    // Retries `multicad_pending` beside the game ini. A report is dropped only
    // when the server understood the body and rejected it (400, 413, 422), or
    // when it is unreadable. Everything else is kept - including a 404, and a
    // StatsUrl too malformed to parse, both being settings the player can fix.
    void FlushPending();

    std::string ToJson(const MatchStats& match);

    // Submit fills these in when the caller left them empty.
    std::string CurrentUtcTimestamp();
    std::string DetectModName();
}
