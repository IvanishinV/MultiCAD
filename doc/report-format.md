# MultiCAD match report format

What the library sends, what each field is worth, and what a server can rely on.
Counterpart to the integration brief; where the two disagree, this one
describes what the code actually does.

## 1. Transport

```
POST <StatsUrl from the game ini>
Content-Type: application/json; charset=utf-8
User-Agent: MultiCAD
```

- The endpoint comes from `[Game] StatsUrl` in the game's ini. No value, no
  reporting - opting in is the act of configuring it.
- `http` and `https` only. No authentication; reports are anonymous.
- Sent when a client closes the multiplayer results screen. A match with N
  players produces at least N reports - see *Duplicates*.
- Bodies over 64 KB are **dropped, never truncated**, so you will never receive a
  half-written JSON document. A 2-player report is about 900 bytes.
- Posted from a detached thread with 5 s timeouts, so the game never waits.

### Retry

The body is written to `multicad_pending\` beside the game ini **before** the
request and deleted once accepted, so a report survives the process being closed
mid-post. Queued reports are retried at the start of the **next match**, and after
any later successful send.

The rule, exactly:

| server answer | client does |
|---|---|
| **2xx** | delivered; queue entry deleted |
| **400, 413, 422** | discarded - the body was refused, resending cannot help |
| **everything else** | kept and retried **indefinitely** |

"Everything else" includes `401`, `403`, `429`, `5xx`, timeouts, DNS failure, an
unparseable `StatsUrl` — **and 3xx**. Redirects are refused rather than followed,
so a `301`/`302` surfaces as the status and is retried forever. If your endpoint
redirects (http→https, or a trailing-slash rewrite), every client will retry that
report for good and never drain its queue. **A 2xx is the only thing that makes a
client stop.**

Queue limits worth planning around:

- It holds 64. When full, the newest report is posted once **unspooled** and lost
  if that fails — so a long outage loses the *newest* matches, not the oldest, and
  a client with a full queue never records new ones.
- A flush stops at the first report that is still undeliverable, so delivery order
  is not chronological.
- **Nothing on the wire distinguishes a retried report from a fresh one.** A
  retried report carries the *original* match's `date`, `startedAt` and
  `durationSeconds` — verified: reports queued at 19:29 and delivered at 19:31
  still carried their 19:28 start time.

### Duplicates

Expect them; deduplicate on `(fingerprint, reporter.netId)`.

- Only a 2xx counts as delivered, so if your response is lost after you committed
  the report, the client still holds the body and resends it.
- The queue entry is deleted *after* the POST returns, so a process killed between
  your 200 and that delete will resend.
- Nothing prevents a second submission: reopening and closing the results screen
  re-reads the session and sends an identical report with a fresh `date`.

## 2. Example

```json
{
  "formatVersion": 1,
  "libraryVersion": "MultiCAD 1.0.0",
  "installId": "550e8400-e29b-41d4-a716-446655440000",
  "mod": "RWG3.6",
  "date": "2026-09-19T16:32:16.332Z",
  "startedAt": "2026-09-19T16:31:19.263Z",
  "map": "(RWG3FE) Граница ver 1.1 (5x5)",
  "mapScheme": 0,
  "durationSeconds": 45,
  "reporter": { "netId": 49249 },
  "players": [
    {
      "name": "Player3-4",
      "netId": 49249,
      "country": 1,
      "team": 1,
      "score": 1205,
      "destroyed": { "infantry": 73, "tanks": 0, "vehicles": 3, "planes": 0,
                     "antiAirs": 0, "artillery": 1, "huges": 0, "miscs": 0 },
      "lost":      { "infantry": 45, "tanks": 0, "vehicles": 1, "planes": 0,
                     "antiAirs": 0, "artillery": 0, "huges": 0, "miscs": 0 },
      "outcome": "won",
      "left": false
    },
    {
      "name": "Player3-5",
      "netId": 6356,
      "country": 0,
      "team": 0,
      "score": 0,
      "destroyed": { "infantry": 45, "tanks": 0, "vehicles": 4, "planes": 0,
                     "antiAirs": 0, "artillery": 0, "huges": 0, "miscs": 0 },
      "lost":      { "infantry": 73, "tanks": 0, "vehicles": 2, "planes": 0,
                     "antiAirs": 0, "artillery": 1, "huges": 0, "miscs": 0 },
      "outcome": "lost",
      "left": false
    }
  ]
}
```

All fields are always present. "Empty" below means the key is there with `""`,
`0` or `-1` — never `null`, never absent.

## 3. Fields

"Agrees" says whether two clients reporting one match should produce the same
value. Read it together with section 7.

| field | type | empty value | agrees | notes |
|---|---|---|---|---|
| `formatVersion` | int | — | yes | `1`. Reject anything you do not know |
| `libraryVersion` | string | — | no | e.g. `"MultiCAD 1.0.0"` |
| `installId` | string | `""` | no | opaque, see 4 |
| `mod` | string | `""` | **usually** | free text, see below |
| `date` | ISO-8601 UTC | `""` | **no** | when *this client* sent it |
| `startedAt` | ISO-8601 UTC | `""` | **no** | match start on *this machine*, see 5 |
| `map` | string | `""` | yes when set | UTF-8, ≤64 bytes |
| `mapScheme` | int | `-1` | yes when set | `0` summer, `1` winter, `2` sea, `3` desert |
| `durationSeconds` | int | `0` | **no** | `0` means unread, not a zero-length match |
| `reporter.netId` | int | `-1` | no - by design | which player sent this, see 8 |
| `players[]` | array | — | **mostly** | 1-12 rows, unordered, see below |

Inside `players[i]`:

| field | type | empty value | agrees | notes |
|---|---|---|---|---|
| `name` | string | `""` | yes | nickname, **not unique**, see 6 |
| `netId` | int | — | **yes** | 2-byte session player id, `0..65535` |
| `country` | int | `-1` | **usually** | nation number `0..254`, meaning is per-`mod` |
| `team` | int | — | yes | opaque grouping key, read raw from the record |
| `score` | int | — | yes | matches the game's own results screen |
| `destroyed` | object | — | yes | 8 branches, all always present |
| `lost` | object | — | yes | same 8 branches |
| `outcome` | enum | `"unknown"` | yes | `won`/`lost`/`draw`/`unknown`, see 8 |
| `left` | bool | — | yes | this player left before the end |

`destroyed` and `lost` always carry all eight of `infantry`, `tanks`, `vehicles`,
`planes`, `antiAirs`, `artillery`, `huges`, `miscs`. Column totals are not sent —
sum them yourself.

**`mod` is not the raw ini value.** It is the text after the last `": "` in
`[StartUp] ProcessName`, or the whole value when there is no separator — so
`"SS2: FMRM 2.1"` arrives as `"FMRM 2.1"`, and `"RWG3.6"` arrives unchanged. It is
free text set by whoever packaged the mod, and can differ between two installs of
the same mod. Treat it as a label, and see the warning in 8 about fingerprinting
on it.

**`players[]` rows are not guaranteed to be human players.** The library drops
rows absent from the lobby snapshot, but when no snapshot was captured it keeps
every occupied record slot — which on the host can include AI and neutral
factions. Such rows show `country: -1` and `outcome: "unknown"` while real rows
carry verdicts; use that as the heuristic if you need one. This also means the
*row set itself* can differ between host and client.

**Timestamps** are always `YYYY-MM-DDTHH:MM:SS.mmmZ` — three-digit milliseconds,
always `Z` — or `""`.

**Lengths.** `map` and `installId` are cut at 64 UTF-8 bytes (`map` on a character
boundary, so it stays valid UTF-8). `name` comes from a 32-byte game field and can
reach ~64 bytes once converted. `mod`, `date` and `startedAt` are cut at 64 bytes
*before* conversion, so their UTF-8 form can exceed 64. Size columns generously.

## 4. `installId`

Stored in the game's own ini as `[Game] InstallId` — the only store, no file or
registry copy. It is therefore **per install, not per machine**: two copies of the
game on one machine each get their own, and can corroborate each other. Copying a
game folder carries the id along, so both copies then look like one install.
Accepted behaviour, per the integration brief, 3.1.

**Do not validate it as a UUID.** A freshly generated one is a v4 UUID, but any
value already in the ini is accepted as long as it is 1-64 printable ASCII
characters. Treat it as an opaque string.

**It identifies an install; it never authenticates one.** The value is generated
on the player's machine and travels in a body that machine writes, so anyone can
send any value. Use it for deduplication, rate limiting and diagnostics. Do not
read "two different `installId`s" as proof that two people played.

If the ini cannot be written (read-only game folder) a throwaway id is generated
**per report**, so that install looks like a stream of distinct ones. Nothing on
the wire flags this, and there is no local log to notice it by — see 10.

## 5. Time

Every timestamp is the reporting machine's own system clock. The game offers no
synchronised time.

Measured, two clients on **one machine** sharing **one clock**: match start
recorded 1.0 s apart, reports arriving 2-3 s apart. That spread is load time,
before any clock skew. Across machines skew is unbounded.

`durationSeconds` is not just clock spread. Each client stops counting when *it*
leaves, so a player who quits early reports a genuinely shorter match: measured
117 s vs 125 s, and again 17 s vs 25 s, the quitter low both times. Take the
**maximum** across reports, ignoring `0`, as the match length.

**Do not group by arrival time.** Two reports of one match usually arrive within
seconds, but a client whose POST failed retries only at the start of its next
match — which may be days later, carrying the original `startedAt`. Group by the
fingerprint (8), use receive time only to separate two matches that share a
fingerprint, and keep a group open for late arrivals.

## 6. Names and ids

`map` is UTF-8 read straight from a game file, unconverted.

`name` is different, in two ways that both matter.

**Encoding.** Nicknames are read out of game memory and converted from the
machine's **ANSI code page** to UTF-8. Two clients with different locales can
produce different bytes for the same non-ASCII nickname.

**Uniqueness.** Nothing stops two players choosing the same nickname, and it
happens. Their rows stay distinct and their `outcome` is still resolved correctly,
because verdicts are matched by `netId`. But `country` is matched to the lobby *by
name*, so both rows get the first match's nation — and if the two players' rows
cannot be told apart by id for some reason, both fall back to `"unknown"`.

**Consequence: never key on a nickname** — not for hashing, not for identifying a
row, not for fingerprinting. Use `netId`.

## 7. What actually agrees between clients

Measured across several 2-client matches on both supported games.

**Reliable:** `netId`, `team`, `score`, `destroyed`, `lost`, `outcome`, `left`.
In matches where both clients saw the same roster, these came back identical every
time.

**Can legitimately differ:**

- **The row set.** Host-only AI/neutral slots have been observed producing a
  4-row report from the host and a 2-row report from the client of the same match.
- `country` — from a per-client lobby snapshot; `-1` when it was missed.
- `map`, `mapScheme` — normally identical, but either can be empty on a client
  that missed its capture window.
- `mod` — free text from each install's ini.
- `date`, `startedAt`, `durationSeconds` — by design.
- `name` bytes, across locales (6).

**`netId` is a session slot, not a player identity.** The same two values recurred
across sessions but bound to the *opposite* players in a later match:
`Player3-4=49249, Player3-5=6356` came back as `Player3-5=49249,
Player3-4=6356`. Safe to fingerprint one match with; useless for tracking a player
across matches.

Only 2-player matches have been played. Larger rosters, and matches with more than
two teams, are untested.

## 8. Suggested server-side validation

1. `formatVersion` is known.
2. `reporter.netId` is not `-1`. A `-1` means the client could not identify its own
   player, and it cannot be the second witness for anything. It *usually* appears
   in `players[]`, but not always — do not reject solely on its absence.
3. 2-12 players; every player has all 8 branches in both counters.
4. All players on one team share an `outcome`, **ignoring `left: true`** — a
   quitter on the winning side legitimately reports `lost`. Also require **at most
   one team `won`**: the per-team rule alone is satisfied by a report where *every*
   team won, which is how a client bug once got past our own test listener.
5. `"unknown"` means no verdict was captured for that row — never a result, and it
   must not satisfy the per-team check. Usually it is all rows or none (the
   outcome hook failed to install on that build), but a **single row** can be
   `unknown`, for example when two players share a nickname.
6. **Fingerprint on `netId`:`team` pairs, sorted.** That set is the only thing
   every client reliably agrees on. Do **not** include timestamps, nicknames, or
   `country`. Including `mod`, `map` or `mapScheme` is tempting but each can be
   empty or `-1` on one client and set on another, which silently prevents
   corroboration — if you use them, treat `""` and `-1` as wildcards.
7. Group by that fingerprint; treat a match as corroborated when two reports carry
   different `reporter.netId` **and** different `installId`.
8. Reconcile small disagreements rather than rejecting: prefer each player's own
   report for their own row. The destroyed/lost mirror between two players is
   expected to be close but not exact — units killed by neither side.
9. Rate-limit per `installId`, and drop duplicate `(fingerprint, reporter.netId)`.
   This rule is mandatory, not an optimisation — see *Duplicates* in 1.

None of this is a security boundary — see 4. It catches broken clients and casual
abuse. An adversary writing their own requests is constrained by none of it; only
server-side signals apply, such as address correlation, implausible statistics,
and rosters that never corroborate elsewhere.

## 9. Not sent

- **`battleId`** — you recompute a fingerprint anyway, so a client-computed hash
  adds no information and one more way to disagree. The library needs no SHA-256.
- **column totals** — derivable.
- **anything identifying the machine** — no IP, MAC or hostname.

## 10. Version support and diagnostics

| version | behaviour |
|---|---|
| Sudden Strike: Resource War v2.4, incl. RWG 3.6 | full report |
| Sudden Strike 2 v2.2, Hidden Stroke 2, FMRM 2.1.5.3 | full report |
| everything else | nothing sent |

Both supported families are confirmed in play: two clients, corroborating reports,
outcomes and sender resolved, including under duplicate nicknames and when a
player quits mid-match.

An unsupported build sends **nothing at all** rather than something wrong. A
supported build whose addresses no longer match is caught by a signature check and
reports the match **without outcomes** — every row `"unknown"` — rather than
patching the wrong bytes. That is the usual cause of an all-`unknown` report.

**A release build produces no local log.** There is no client-side file to
distinguish "the URL is wrong" from "no matches were played"; that has to be
answered from the server side, by whether reports arrive at all. If field
diagnostics are wanted, say so and they can be added deliberately.
