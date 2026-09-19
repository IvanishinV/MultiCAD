#pragma once

#include <string>
#include <vector>

#include "types.h"

// One finished multiplayer match, in the vocabulary of the game's own statistics
// screen. Filled by the per-version menu hook; everything here is version
// independent.

enum class MatchOutcome : U8
{
    Unknown = 0,
    Won,
    Lost,
    Draw,
};

// The statistics screen is a matrix: one row per unit branch, two columns headed
// MNTXT_UNITS_DESTROYED and MNTXT_UNITS_LOST, so each player has two of these.
struct UnitCounts
{
    S32 infantry{ 0 };
    S32 tanks{ 0 };
    S32 vehicles{ 0 };
    S32 planes{ 0 };
    S32 antiAirs{ 0 };
    S32 artillery{ 0 };
    S32 huges{ 0 };
    S32 miscs{ 0 };
};

struct PlayerStats
{
    // Off the wire from other players: never trusted, always escaped.
    std::string  name;

    // The session's 2-byte player id. Every client must agree on it to route
    // traffic and it carries no code page, which makes it the key for matching
    // one client's report against another's - unlike a nickname. It is a slot,
    // though, not a person: the same id belongs to someone else next session.
    S32          netId{ -1 };

    // Nation number. The display names live in the mod's lng files, so the
    // number travels and the server maps it per mod.
    S32          country{ -1 };

    S32          team{ -1 };
    S32          score{ 0 };

    // Column totals are derivable, so they are not sent.
    UnitCounts   destroyed;
    UnitCounts   lost;

    MatchOutcome outcome{ MatchOutcome::Unknown };

    // Reported beside the outcome rather than folded into it: a quitter on the
    // winning side would otherwise make that team's results disagree, and a
    // server checking "one outcome per team" would discard the whole match.
    bool         left{ false };
};

struct MatchStats
{
    // From [StartUp] ProcessName, not version detection - RWG 3.6 ships the
    // stock Resource War 2.4 dlls, so the hash cannot tell them apart.
    std::string              mod;

    // ISO 8601 UTC. `date` is when this client reported, `startedAt` when the
    // match began here. Neither is comparable across machines, and even two
    // clients on one machine start a second apart - never a match identity.
    std::string              date;
    std::string              startedAt;

    std::string              map;

    // 0 summer, 1 winter, 2 sea, 3 desert; -1 unread. From the map, so every
    // client agrees.
    S32                      mapScheme{ -1 };

    // This client's view: each stops counting when it leaves, so a player who
    // quit early reports a genuinely shorter match.
    U32                      durationSeconds{ 0 };

    // Which player sent this report. Points into `players`.
    S32                      reporterNetId{ -1 };

    std::vector<PlayerStats> players;
};
