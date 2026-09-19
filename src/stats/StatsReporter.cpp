#include "pch.h"
#include "StatsReporter.h"

#include "ModInfo.h"
#include "ScreenConfig.h"
#include "version.h"

#include <winhttp.h>
#include <bcrypt.h>

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <map>
#include <mutex>
#include <thread>
#include <utility>
#include <vector>

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "bcrypt.lib")

namespace
{
    constexpr int kResolveTimeoutMs = 5000;
    constexpr int kConnectTimeoutMs = 5000;
    constexpr int kSendTimeoutMs    = 5000;
    constexpr int kReceiveTimeoutMs = 5000;

    std::string ReadIniValue(const char* section, const char* key)
    {
        const std::string iniPath = Screen::IniPath();
        if (iniPath.empty())
            return {};

        char buffer[512] = { 0 };
        const DWORD length = GetPrivateProfileStringA(
            section, key, "", buffer, sizeof(buffer), iniPath.c_str());

        return std::string(buffer, length);
    }

    std::string Trim(const std::string& raw)
    {
        const size_t first = raw.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
            return {};

        const size_t last = raw.find_last_not_of(" \t\r\n");

        return raw.substr(first, last - first + 1);
    }

    // Trimmed: WinHttpCrackUrl rejects a trailing space, which reads as a bad
    // setting and drops the report rather than queueing it.
    std::string ReadStatsUrl()
    {
        return Trim(ReadIniValue("Game", "StatsUrl"));
    }

    std::wstring Widen(const std::string& text, UINT codePage)
    {
        if (text.empty())
            return {};

        const int size = MultiByteToWideChar(codePage, 0, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (size <= 0)
            return {};

        std::wstring wide(static_cast<size_t>(size), L'\0');
        MultiByteToWideChar(codePage, 0, text.data(), static_cast<int>(text.size()), wide.data(), size);

        return wide;
    }

    // The game hands us text in the local ANSI code page - Cyrillic player names
    // in the Russian builds - and JSON has to be UTF-8.
    std::string ToUtf8(const std::string& ansi)
    {
        const std::wstring wide = Widen(ansi, CP_ACP);
        if (wide.empty())
            return {};

        const int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                                             nullptr, 0, nullptr, nullptr);
        if (size <= 0)
            return {};

        std::string utf8(static_cast<size_t>(size), '\0');
        WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()),
                            utf8.data(), size, nullptr, nullptr);

        return utf8;
    }

    std::string JsonEscape(const std::string& utf8)
    {
        std::string out;
        out.reserve(utf8.size() + 8);

        for (const unsigned char c : utf8)
        {
            switch (c)
            {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b";  break;
            case '\f': out += "\\f";  break;
            case '\n': out += "\\n";  break;
            case '\r': out += "\\r";  break;
            case '\t': out += "\\t";  break;
            default:
                if (c < 0x20)
                {
                    char escaped[7] = { 0 };
                    std::snprintf(escaped, sizeof(escaped), "\\u%04X", c);
                    out += escaped;
                }
                else
                {
                    out += static_cast<char>(c);
                }
                break;
            }
        }

        return out;
    }

    // Largest cut at or below `maxLength` that lands between characters. Cutting
    // UTF-8 by byte count leaves a lone lead byte and a strict parser rejects the
    // whole body. Continuation bytes are 10xxxxxx, so backing up over them finds
    // the character the cut fell inside.
    size_t Utf8Boundary(const std::string& utf8, size_t maxLength)
    {
        if (utf8.size() <= maxLength)
            return utf8.size();

        size_t end = maxLength;
        while (end > 0 && (static_cast<unsigned char>(utf8[end]) & 0xC0) == 0x80)
            --end;

        return end;
    }

    // For text that is already UTF-8 - the map comes straight out of a game file
    // rather than out of memory in the local code page.
    std::string JsonStringUtf8(const std::string& utf8, size_t maxLength)
    {
        return "\"" + JsonEscape(utf8.substr(0, Utf8Boundary(utf8, maxLength))) + "\"";
    }

    std::string JsonString(const std::string& raw, size_t maxLength)
    {
        std::string text = raw;
        if (text.size() > maxLength)
            text.resize(maxLength);

        return "\"" + JsonEscape(ToUtf8(text)) + "\"";
    }

    std::string BranchesToJson(const UnitCounts& c)
    {
        std::string json = "{";
        json += "\"infantry\":"   + std::to_string(c.infantry);
        json += ",\"tanks\":"     + std::to_string(c.tanks);
        json += ",\"vehicles\":"  + std::to_string(c.vehicles);
        json += ",\"planes\":"    + std::to_string(c.planes);
        json += ",\"antiAirs\":"  + std::to_string(c.antiAirs);
        json += ",\"artillery\":" + std::to_string(c.artillery);
        json += ",\"huges\":"     + std::to_string(c.huges);
        json += ",\"miscs\":"     + std::to_string(c.miscs);
        json += "}";

        return json;
    }

    const char* OutcomeName(MatchOutcome outcome)
    {
        switch (outcome)
        {
        case MatchOutcome::Won:  return "won";
        case MatchOutcome::Lost: return "lost";
        case MatchOutcome::Draw: return "draw";
        default:                 return "unknown";
        }
    }

    // --- install id -------------------------------------------------------

    constexpr size_t kInstallIdMaxLength = 64;

    // An identifier, not a credential: a hand-edited value is fine as long as it
    // is printable and bounded.
    bool PlausibleInstallId(const std::string& id)
    {
        if (id.empty() || id.size() > kInstallIdMaxLength)
            return false;

        return std::all_of(id.begin(), id.end(),
                           [](unsigned char c) { return c > 0x20 && c < 0x7F; });
    }

    std::string NewInstallId()
    {
        // Random, with no machine data in it: IP, MAC and nickname all collide
        // between players behind one VPN.
        unsigned char bytes[16] = { 0 };
        if (BCryptGenRandom(nullptr, bytes, sizeof(bytes), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
            return {};

        // UUID v4 version and variant nibbles.
        bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0F) | 0x40);
        bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3F) | 0x80);

        char text[37] = { 0 };
        std::snprintf(text, sizeof(text),
                      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                      bytes[0], bytes[1], bytes[2],  bytes[3],  bytes[4],  bytes[5],
                      bytes[6], bytes[7], bytes[8],  bytes[9],  bytes[10], bytes[11],
                      bytes[12], bytes[13], bytes[14], bytes[15]);

        return text;
    }

    bool WriteInstallIdIni(const std::string& id)
    {
        const std::string iniPath = Screen::IniPath();
        if (iniPath.empty())
            return false;

        return WritePrivateProfileStringA("Game", "InstallId", id.c_str(), iniPath.c_str()) != FALSE;
    }

    // `retryable` separates "could not reach the endpoint" from "the endpoint
    // refused this body" - the first is what the spool exists for, the second
    // would pile up in it forever.
    struct PostResult
    {
        bool  delivered{ false };
        bool  retryable{ false };
        DWORD error{ 0 };
        DWORD status{ 0 };
    };

    std::string DescribePost(const PostResult& result)
    {
        if (result.delivered)
            return "ok (HTTP " + std::to_string(result.status) + ")";

        if (result.status != 0)
            return "REFUSED HTTP " + std::to_string(result.status)
                 + (result.retryable ? " - will retry" : " - body rejected, discarded");

        return "NO RESPONSE err=" + std::to_string(result.error) + " - will retry";
    }

    PostResult PostJson(const std::string& url, const std::string& body)
    {
        // Kept unless the server rejects the body itself. A url that will not
        // parse is a setting the player can still correct, so the report waits
        // for that rather than being thrown away.
        PostResult result;
        result.retryable = true;

        const std::wstring wideUrl = Widen(url, CP_ACP);
        if (wideUrl.empty())
            return result;

        wchar_t host[256] = { 0 };
        wchar_t path[1024] = { 0 };
        wchar_t extra[1024] = { 0 };

        URL_COMPONENTS parts{};
        parts.dwStructSize = sizeof(parts);
        parts.lpszHostName = host;
        parts.dwHostNameLength = ARRAYSIZE(host);
        parts.lpszUrlPath = path;
        parts.dwUrlPathLength = ARRAYSIZE(path);
        parts.lpszExtraInfo = extra;
        parts.dwExtraInfoLength = ARRAYSIZE(extra);

        if (!WinHttpCrackUrl(wideUrl.c_str(), 0, 0, &parts))
            return result;

        // Only the two web schemes: WinHttpCrackUrl happily parses others.
        if (parts.nScheme != INTERNET_SCHEME_HTTP && parts.nScheme != INTERNET_SCHEME_HTTPS)
            return result;

        const bool secure = parts.nScheme == INTERNET_SCHEME_HTTPS;

        // WinHttpCrackUrl splits the query off the path; the request object is
        // both halves together.
        const std::wstring object = std::wstring(path) + extra;

        HINTERNET session = WinHttpOpen(L"MultiCAD",
                                        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        if (!session)
        {
            result.error = GetLastError();
            return result;
        }

        WinHttpSetTimeouts(session, kResolveTimeoutMs, kConnectTimeoutMs, kSendTimeoutMs, kReceiveTimeoutMs);

        if (HINTERNET connection = WinHttpConnect(session, host, parts.nPort, 0))
        {
            HINTERNET request = WinHttpOpenRequest(connection, L"POST", object.c_str(), nullptr,
                                                    WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                                    secure ? WINHTTP_FLAG_SECURE : 0);
            if (request)
            {
                // A redirect could quietly move the endpoint somewhere the ini
                // never named, so let the configured URL be the only one used.
                DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
                WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));

                static constexpr wchar_t kHeaders[] = L"Content-Type: application/json; charset=utf-8\r\n";

                const bool exchanged =
                    WinHttpSendRequest(request, kHeaders, static_cast<DWORD>(-1),
                                       const_cast<char*>(body.data()), static_cast<DWORD>(body.size()),
                                       static_cast<DWORD>(body.size()), 0) != FALSE
                    && WinHttpReceiveResponse(request, nullptr) != FALSE;

                if (!exchanged)
                {
                    result.error = GetLastError();
                }
                else
                {
                    // Until this was read, a 404 from a mistyped path counted as
                    // a successful delivery.
                    DWORD status = 0;
                    DWORD size    = sizeof(status);

                    if (WinHttpQueryHeaders(request,
                                            WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                                            WINHTTP_HEADER_NAME_BY_INDEX, &status, &size,
                                            WINHTTP_NO_HEADER_INDEX))
                    {
                        result.status    = status;
                        result.delivered = status >= 200 && status < 300;

                        // A 404 is usually a mistyped StatsUrl, which the
                        // player can still fix - so it is kept. Only a body the
                        // server understood and rejected is hopeless.
                        result.retryable = !result.delivered
                                        && status != 400 && status != 413 && status != 422;
                    }
                    else
                    {
                        result.error = GetLastError();
                    }
                }

                WinHttpCloseHandle(request);
            }

            WinHttpCloseHandle(connection);
        }

        WinHttpCloseHandle(session);

        return result;
    }

    // --- delivery spool ---------------------------------------------------
    //
    // Undelivered reports wait in `multicad_pending` beside the game ini. Per
    // install, like the install id: two installs must not share a queue.

    constexpr size_t kMaxPendingReports = 64;

    std::string PendingDir()
    {
        const std::string iniPath = Screen::IniPath();
        const size_t slash = iniPath.find_last_of('\\');
        if (slash == std::string::npos)
            return {};

        return iniPath.substr(0, slash) + "\\multicad_pending";
    }

    std::vector<std::string> PendingFiles(const std::string& dir)
    {
        std::vector<std::string> names;

        WIN32_FIND_DATAA found{};
        HANDLE search = FindFirstFileA((dir + "\\*.json").c_str(), &found);
        if (search == INVALID_HANDLE_VALUE)
            return names;

        do
        {
            if ((found.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
                names.emplace_back(found.cFileName);
        }
        while (FindNextFileA(search, &found));

        FindClose(search);

        return names;
    }

    bool ReadWholeFile(const std::string& path, std::string& out)
    {
        FILE* file = nullptr;
        if (fopen_s(&file, path.c_str(), "rb") != 0 || file == nullptr)
            return false;

        char buffer[4096];
        out.clear();

        for (size_t read = 0; (read = std::fread(buffer, 1, sizeof(buffer), file)) > 0; )
        {
            out.append(buffer, read);

            if (out.size() > Stats::kMaxBodyBytes)
                break;
        }

        std::fclose(file);

        return true;
    }

    // The caller spools BEFORE posting and deletes on success. Spooling on failure
    // would never run in the case that matters: closing the game at the results
    // screen kills the posting thread, so there is no failure to react to.
    std::string SpoolReport(const std::string& body)
    {
        const std::string dir = PendingDir();
        if (dir.empty())
            return {};

        CreateDirectoryA(dir.c_str(), nullptr);   // harmless when it exists

        if (PendingFiles(dir).size() >= kMaxPendingReports)
        {
            return {};
        }

        // Unique without consulting the folder, even if the install runs twice.
        static std::atomic<unsigned> sequence{ 0 };

        char name[64];
        std::snprintf(name, sizeof(name), "%08lX_%04X_%03u.json",
                      static_cast<unsigned long>(GetTickCount()),
                      static_cast<unsigned>(GetCurrentProcessId() & 0xFFFF),
                      sequence++ & 0xFFu);

        const std::string path = dir + "\\" + name;

        FILE* file = nullptr;
        if (fopen_s(&file, path.c_str(), "wb") != 0 || file == nullptr)
        {
            return {};
        }

        const size_t written = std::fwrite(body.data(), 1, body.size(), file);
        std::fclose(file);

        if (written != body.size())
        {
            DeleteFileA(path.c_str());   // a half-written body would never parse
            return {};
        }

        return path;
    }

    // Without this a match-start flush could pick up the report a Submit worker
    // is posting and send it twice.
    std::mutex g_deliveryMutex;

    // Caller holds g_deliveryMutex. Stops at the first report still undeliverable
    // rather than walking the queue against a server that is down.
    void FlushPendingLocked(const std::string& url)
    {
        const std::string dir = PendingDir();
        if (dir.empty())
            return;

        const std::vector<std::string> names = PendingFiles(dir);
        if (names.empty())
            return;

        for (const std::string& name : names)
        {
            const std::string path = dir + "\\" + name;

            std::string body;
            if (!ReadWholeFile(path, body) || body.empty() || body.size() > Stats::kMaxBodyBytes)
            {
                // Unreadable or absurd: it will never be sent, so stop carrying it.
                DeleteFileA(path.c_str());
                continue;
            }

            const PostResult result = PostJson(url, body);

            if (result.delivered || !result.retryable)
            {
                DeleteFileA(path.c_str());
                continue;
            }

            break;
        }
    }

    void FlushPendingNow(const std::string& url)
    {
        std::lock_guard<std::mutex> lock(g_deliveryMutex);
        FlushPendingLocked(url);
    }
}


namespace Stats
{
    static std::mutex  g_mapMutex;
    static std::string g_mapName;
    static std::string g_matchStartedAt;

    void CaptureMapName()
    {
        // Every failure path below returns without setting one, and `map` is a
        // fingerprint input - a stale name breaks corroboration silently.
        {
            std::lock_guard<std::mutex> lock(g_mapMutex);
            g_mapName.clear();
        }

        const std::string iniPath = Screen::IniPath();
        const size_t slash = iniPath.find_last_of('\\');
        if (slash == std::string::npos)
            return;

        const std::string path = iniPath.substr(0, slash) + "\\XCHNG\\ToGame\\mis_desc";

        FILE* file = nullptr;
        if (fopen_s(&file, path.c_str(), "rb") != 0 || file == nullptr)
        {
            // The menu writes this when the match starts and empties the folder
            // when it ends, so a miss here means we looked too early - and there
            // is no second attempt, the name is gone for the whole match.
            return;
        }

        char line[256] = { 0 };
        const size_t read = std::fread(line, 1, sizeof(line) - 1, file);
        std::fclose(file);

        // The first line names the map; the rest is the briefing.
        std::string name(line, read);
        const size_t end = name.find_first_of("\r\n");
        if (end != std::string::npos)
            name.resize(end);

        if (name.empty())
        {
            return;
        }

        std::lock_guard<std::mutex> lock(g_mapMutex);
        g_mapName = std::move(name);
    }

    std::string MapName()
    {
        std::lock_guard<std::mutex> lock(g_mapMutex);
        return g_mapName;
    }

    void MarkMatchStart()
    {
        const std::string now = CurrentUtcTimestamp();

        {
            std::lock_guard<std::mutex> lock(g_mapMutex);
            g_matchStartedAt = now;
        }
    }

    std::string MatchStartedAt()
    {
        std::lock_guard<std::mutex> lock(g_mapMutex);
        return g_matchStartedAt;
    }

    std::string InstallId()
    {
        static std::mutex  mutex;
        static std::string cached;

        std::lock_guard<std::mutex> lock(mutex);

        if (!cached.empty())
            return cached;

        // The ini is the only store, so an id written by an earlier run is the
        // one that keeps being used rather than a second appearing beside it.
        if (const std::string found = Trim(ReadIniValue("Game", "InstallId"));
            PlausibleInstallId(found))
        {
            cached = found;
            return cached;
        }

        const std::string fresh = NewInstallId();
        if (fresh.empty())
        {
            return {};
        }

        if (!WriteInstallIdIni(fresh))
        {
            // Not cached, so this is a new id per report, not per launch - the
            // server sees one install as a stream of them. A read-only game
            // folder is how this happens.
            return fresh;
        }

        cached = fresh;
        return cached;
    }

    bool IsReportingEnabled()
    {
        return !ReadStatsUrl().empty();
    }

    std::string CurrentUtcTimestamp()
    {
        SYSTEMTIME utc{};
        GetSystemTime(&utc);

        char buffer[32] = { 0 };
        std::snprintf(buffer, sizeof(buffer), "%04u-%02u-%02uT%02u:%02u:%02u.%03uZ",
                      utc.wYear, utc.wMonth, utc.wDay,
                      utc.wHour, utc.wMinute, utc.wSecond, utc.wMilliseconds);

        return buffer;
    }

    std::string DetectModName()
    {
        // What the mod declares about itself, where it does.
        std::string launcherName, launcherVersion;
        if (ModInfo::FromLauncher(launcherName, launcherVersion))
            return launcherName + " " + launcherVersion;

        std::string name = ReadIniValue("StartUp", "ProcessName");

        // Mods tend to prefix the base game they run on - FMRM ships
        // "SS2: FMRM 2.1" - and only the mod half is worth reporting. Names
        // without a separator, like "RWG3.6", are left alone.
        const size_t separator = name.rfind(": ");
        if (separator != std::string::npos && separator + 2 < name.size())
            name.erase(0, separator + 2);

        return name;
    }

    std::string ToJson(const MatchStats& match)
    {
        std::string json;
        json.reserve(1024);

        json += "{\"formatVersion\":1";
        json += ",\"libraryVersion\":\"MultiCAD " MULTICAD_VERSION_STR "\"";
        json += ",\"installId\":" + JsonStringUtf8(InstallId(), kInstallIdMaxLength);
        json += ",\"mod\":" + JsonString(match.mod, kMaxNameLength);
        json += ",\"date\":" + JsonString(match.date, kMaxNameLength);
        json += ",\"startedAt\":" + JsonString(match.startedAt, kMaxNameLength);
        json += ",\"map\":" + JsonStringUtf8(match.map, kMaxNameLength);
        json += ",\"mapScheme\":" + std::to_string(match.mapScheme);
        json += ",\"durationSeconds\":" + std::to_string(match.durationSeconds);
        json += ",\"reporter\":{\"netId\":" + std::to_string(match.reporterNetId) + "}";
        json += ",\"players\":[";

        const size_t count = std::min(match.players.size(), kMaxPlayers);
        for (size_t i = 0; i < count; ++i)
        {
            const PlayerStats& p = match.players[i];

            if (i != 0)
                json += ',';

            json += "{\"name\":" + JsonString(p.name, kMaxNameLength);
            json += ",\"netId\":" + std::to_string(p.netId);
            json += ",\"country\":" + std::to_string(p.country);
            json += ",\"team\":" + std::to_string(p.team);
            json += ",\"score\":" + std::to_string(p.score);
            json += ",\"destroyed\":" + BranchesToJson(p.destroyed);
            json += ",\"lost\":" + BranchesToJson(p.lost);
            json += ",\"outcome\":\"";
            json += OutcomeName(p.outcome);
            json += "\"";
            json += ",\"left\":";
            json += p.left ? "true" : "false";
            json += "}";
        }

        json += "]}";

        return json;
    }

    void Submit(const MatchStats& match)
    {
        const std::string url = ReadStatsUrl();
        if (url.empty())
            return;   // no endpoint configured: reporting stays off

        MatchStats record = match;
        if (record.date.empty())
            record.date = CurrentUtcTimestamp();
        if (record.mod.empty())
            record.mod = DetectModName();

        std::string body = ToJson(record);
        if (body.empty() || body.size() > kMaxBodyBytes)
            return;

        std::thread([url, body = std::move(body)]() mutable
            {
                std::lock_guard<std::mutex> lock(g_deliveryMutex);

                // On disk first: the report has to survive the process dying
                // anywhere in the exchange below.
                const std::string spooled = SpoolReport(body);

                const PostResult result = PostJson(url, body);
#ifdef _DEBUG
                OutputDebugStringA(("[MultiCAD] report: " + DescribePost(result) + "\n").c_str());
#endif

                // A rejected body is as finished as a delivered one.
                if (!spooled.empty() && (result.delivered || !result.retryable))
                    DeleteFileA(spooled.c_str());

                // The endpoint is up: drain anything an earlier run left behind.
                if (result.delivered)
                    FlushPendingLocked(url);
            }).detach();
    }

    void FlushPending()
    {
        const std::string url = ReadStatsUrl();
        if (url.empty())
            return;   // reporting is off; the queue keeps until it is turned on

        // Runs at match start; the game must not wait on a server that is down.
        std::thread([url]() { FlushPendingNow(url); }).detach();
    }
}
