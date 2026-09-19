#include "pch.h"
#include "MatchStatsReader.h"

#include "OutcomeHook.h"
#include "StatsReporter.h"

#include <algorithm>
#include <mutex>
#include <utility>
#include <vector>

namespace
{
    // Record layout, identical in both menu dlls; only the array address moves.
    constexpr size_t    kRecordStride  = 0x7C;
    // The name ends where the network id begins. Reading further swallows the
    // id into the nickname, and the result no longer matches the name the
    // outcome hook stored - so the player reports `unknown`.
    constexpr size_t    kNameCapacity  = 0x20;

    constexpr uintptr_t kOffName       = 0x00;
    constexpr uintptr_t kOffNetId      = 0x20;   // 2 bytes, session player id
    constexpr uintptr_t kOffTeam       = 0x30;
    constexpr uintptr_t kOffDestroyed  = 0x34;   // 8 dwords
    constexpr uintptr_t kOffLost       = 0x54;   // 8 dwords
    constexpr uintptr_t kOffScore      = 0x74;

    // The screen lists the branches in a different order from the array. Its
    // render loop switches row -> dword index, and these are those indices:
    // reading the group in array order reports tanks as infantry.
    constexpr size_t kIdxInfantry  = 2;
    constexpr size_t kIdxTanks     = 0;
    constexpr size_t kIdxVehicles  = 4;
    constexpr size_t kIdxPlanes    = 3;
    constexpr size_t kIdxAntiAirs  = 6;
    constexpr size_t kIdxArtillery = 1;
    constexpr size_t kIdxHuges     = 5;
    constexpr size_t kIdxMiscs     = 7;

    // Multiplayer setup screen's player table. Same free-slot convention as the
    // statistics array: an empty name means the slot is unused.
    constexpr size_t    kLobbyStride    = 0x7B;
    constexpr size_t    kLobbyNameCap   = 0x20;
    constexpr uintptr_t kOffLobbyName   = 0x00;
    constexpr uintptr_t kOffLobbyNation = 0x76;
    constexpr uintptr_t kOffLobbyTeam   = 0x78;

    // Nothing chosen; the setup screen tests both the nation and team bytes
    // for it.
    constexpr U8 kUnset = 0xFF;

    constexpr size_t kMapNameCap = 64;

    UnitCounts ReadCounts(const S32* group)
    {
        UnitCounts counts;
        counts.infantry  = group[kIdxInfantry];
        counts.tanks     = group[kIdxTanks];
        counts.vehicles  = group[kIdxVehicles];
        counts.planes    = group[kIdxPlanes];
        counts.antiAirs  = group[kIdxAntiAirs];
        counts.artillery = group[kIdxArtillery];
        counts.huges     = group[kIdxHuges];
        counts.miscs     = group[kIdxMiscs];

        return counts;
    }

    // A free slot is an empty name, which is how the screen's own list decides
    // whether to draw a row.
    std::string ReadName(const char* name, size_t capacity)
    {
        size_t length = 0;
        while (length < capacity && name[length] != '\0')
            ++length;

        return std::string(name, length);
    }

    struct LobbyEntry
    {
        std::string name;
        S32         country{ -1 };
        S32         team{ -1 };
    };

    std::mutex              g_lobbyMutex;
    std::vector<LobbyEntry> g_lobby;

    bool LobbyCaptured()
    {
        std::lock_guard<std::mutex> lock(g_lobbyMutex);
        return !g_lobby.empty();
    }

    // Leaves the nation unset rather than guessing when the player was not in
    // the captured lobby. Team comes from the record, not from here.
    bool FindLobbyEntry(const std::string& name, S32& country)
    {
        std::lock_guard<std::mutex> lock(g_lobbyMutex);

        for (const LobbyEntry& entry : g_lobby)
        {
            if (entry.name != name)
                continue;

            country = entry.country;

            return true;
        }

        return false;
    }
}

namespace Stats
{
    const StatsLayout* LayoutFor(const GameVersion version)
    {
        switch (version)
        {
        case GameVersion::SS_2:
        case GameVersion::HS_2:
        case GameVersion::FMRM_2_1_5_3:
            return &kLayoutSs2V22;

        case GameVersion::SS_RW_V2_4:
            return &kLayoutRwV24;

        default:
            return nullptr;
        }
    }

    const U8* RecordsBase(GameGlobals& globals, const StatsLayout& layout)
    {
        // Records live in the session object. `firstRecordRva` is the saved .rts
        // table, which ReadMatch refuses before it ever gets here.
        if (layout.sessionPtrRva != 0)
        {
            const auto* session = *globals.getPtr<const U8*>(layout.sessionPtrRva);

            return session != nullptr ? session + layout.sessionRecordsOffset : nullptr;
        }

        return layout.firstRecordRva != 0 ? globals.getPtr<const U8>(layout.firstRecordRva) : nullptr;
    }

    bool ReadMatch(GameGlobals& globals, const StatsLayout& layout, MatchStats& out)
    {
        // Without a session pointer the only readable table is the saved per-map
        // top ten from a .rts file - old names and scores, not a match anyone
        // just played. Reporting it would invent a match that never happened.
        if (layout.sessionPtrRva == 0)
            return false;

        const auto* records = RecordsBase(globals, layout);
        if (records == nullptr || layout.slotCount == 0)
            return false;

        MatchStats match;
        S32 longestSeconds = 0;
        S32 localIndex     = -1;   // reported only if the sender cannot be named

        const bool haveLobby = LobbyCaptured();

        for (size_t slot = 0; slot < layout.slotCount && match.players.size() < kMaxPlayers; ++slot)
        {
            const auto* record = records + slot * kRecordStride;

            std::string name = ReadName(reinterpret_cast<const char*>(record + kOffName), kNameCapacity);
            if (name.empty())
                continue;   // unused slot

            const auto readS32 = [record](uintptr_t offset)
                {
                    return *reinterpret_cast<const S32*>(record + offset);
                };

            PlayerStats player;
            player.name      = std::move(name);
            player.score     = readS32(kOffScore);
            player.destroyed = ReadCounts(reinterpret_cast<const S32*>(record + kOffDestroyed));
            player.lost      = ReadCounts(reinterpret_cast<const S32*>(record + kOffLost));
            player.team      = readS32(kOffTeam);

            // 2 bytes by the notes; the upper half has been zero in every record
            // dumped, so this is the conservative reading.
            player.netId     = *reinterpret_cast<const U16*>(record + kOffNetId);

            // Not in the record: the field the results screen tests is always
            // zero, which is why multi.log always says "YOU LOST". See OutcomeHook.
            player.outcome   = OutcomeFor(player.netId, player.name);
            player.left      = LeftEarly(player.netId, player.name);

            // The lobby lists humans and nothing else, so it decides who belongs
            // here: AI and neutral factions hold record slots too, and on the
            // host only, which made the two clients disagree. Without a snapshot
            // everything is kept rather than dropping the match.
            if (!FindLobbyEntry(player.name, player.country) && haveLobby)
            {
                continue;
            }

            match.players.push_back(std::move(player));
        }

        if (match.players.empty())
            return false;

        // All three were captured while the match ran; nothing here survives it.
        match.map       = MapName();
        match.mapScheme = MapScheme();
        match.startedAt = MatchStartedAt();

        // Elapsed seconds and the local player both live in the session; the
        // per-record time field stays zero for a multiplayer match.
        if (layout.sessionPtrRva != 0)
        {
            if (const auto* session = *globals.getPtr<const U8*>(layout.sessionPtrRva))
            {
                if (layout.sessionDurationOffset != 0)
                {
                    const S32 seconds = *reinterpret_cast<const S32*>(session + layout.sessionDurationOffset);
                    if (seconds > 0)
                        longestSeconds = seconds;
                }

                // One-based: the two clients of a two-player match read 1 and
                // 2, never 0. As a subscript it names the wrong record on one
                // client and runs off the roster on the other.
                if (layout.sessionLocalIndexOffset != 0)
                {
                    const S32 local =
                        *reinterpret_cast<const S32*>(session + layout.sessionLocalIndexOffset);
                    const S32 slot = local - 1;

                    localIndex = local;

                    if (slot >= 0 && static_cast<size_t>(slot) < layout.slotCount)
                    {
                        const auto* self = records + static_cast<size_t>(slot) * kRecordStride;
                        if (self[kOffName] != 0)
                            match.reporterNetId = *reinterpret_cast<const U16*>(self + kOffNetId);
                    }
                }
            }
        }

        match.durationSeconds = longestSeconds > 0 ? static_cast<U32>(longestSeconds) : 0;

        // Preferred over the session index: it names a record by id rather than
        // by slot arithmetic.
        if (const S32 selfNetId = LocalPlayerNetId(); selfNetId >= 0)
        {
            for (const PlayerStats& player : match.players)
            {
                if (player.netId == selfNetId)
                {
                    match.reporterNetId = player.netId;
                    break;
                }
            }
        }

        if (match.reporterNetId < 0)
        {
#ifdef _DEBUG
            OutputDebugStringA(("[MultiCAD] reporter unresolved, session local index "
                                + std::to_string(localIndex) + "\n").c_str());
#else
            (void)localIndex;
#endif
        }

        out = std::move(match);

        return true;
    }

    void CaptureLobby(GameGlobals& globals, const StatsLayout& layout)
    {
        if (layout.lobbyFirstRecordRva == 0 || layout.lobbySlotCount == 0)
            return;

        std::vector<LobbyEntry> captured;

        for (size_t slot = 0; slot < layout.lobbySlotCount; ++slot)
        {
            const auto* record =
                globals.getPtr<const U8>(layout.lobbyFirstRecordRva + slot * kLobbyStride);

            LobbyEntry entry;
            entry.name = ReadName(reinterpret_cast<const char*>(record + kOffLobbyName), kLobbyNameCap);
            if (entry.name.empty())
                continue;

            const U8 nation = record[kOffLobbyNation];
            const U8 side   = record[kOffLobbyTeam];

            entry.country = nation == kUnset ? -1 : static_cast<S32>(nation);
            entry.team    = side   == kUnset ? -1 : static_cast<S32>(side);

            captured.push_back(std::move(entry));
        }

        // An empty table is the post-reload state, not a lobby anyone sat in.
        if (captured.empty())
            return;

        std::lock_guard<std::mutex> lock(g_lobbyMutex);
        g_lobby = std::move(captured);
    }
}
