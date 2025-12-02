#include "pch.h"
#include "DllVersionDetector.h"

#include <fstream>
#include <wincrypt.h>
#include <cwctype>
#include <queue>

DllVersionDetector& DllVersionDetector::GetInstance()
{
    static DllVersionDetector instance;
    return instance;
}

void DllVersionDetector::DetectDllVersion(const DllType type, const std::wstring& modulePath, const uintptr_t moduleBase, const size_t imageSize)
{
    auto& state = states_[type];
    state.info.type = type;
    state.info.base = moduleBase;
    state.info.imageSize = imageSize;

    std::array<uint8_t, 32> hash;
    if (!AnalyzeDll(modulePath, hash, state.info))
    {
        state.status = DetectionStatus::NotCalculated;
        state.info.version = GameVersion::UNKNOWN;
        return;
    }

    for (const auto& kv : GetVersions(type))
    {
        if (kv.sha256 == hash)
        {
            state.status = DetectionStatus::Supported;
            state.info.version = kv.version;
            return;
        }
    }

    state.status = DetectionStatus::UnsupportedHash;
    state.info.version = GameVersion::UNKNOWN;
}

DetectionStatus DllVersionDetector::GetDetectionStatus(const DllType type) const
{
    auto it = states_.find(type);
    return it != states_.end() ? it->second.status : DetectionStatus::NotDetected;
}

GameVersion DllVersionDetector::GetGameVersion(const DllType type) const
{
    auto it = states_.find(type);
    return it != states_.end() ? it->second.info.version : GameVersion::UNKNOWN;
}

GameVersion DllVersionDetector::GetOrDetectGameVersion(const DllType type, const std::wstring& modulePath, const uintptr_t moduleBase, const size_t imageSize)
{
    auto it = states_.find(type);
    if (it != states_.end() && it->second.status != DetectionStatus::NotDetected && it->second.status != DetectionStatus::NotCalculated)
    {
        const uintptr_t diff = moduleBase - it->second.info.base;
        it->second.info.relocVA += diff;
        it->second.info.base = moduleBase;
        it->second.info.imageSize = imageSize;
        return it->second.info.version;
    }

    if (it == states_.end() && (moduleBase == 0 || imageSize == 0))
        return GameVersion::UNKNOWN;

    DetectDllVersion(type, modulePath, moduleBase, imageSize);

    return GetGameVersion(type);
}

ModuleInfo DllVersionDetector::GetModuleInfo(const DllType type) const
{
    auto it = states_.find(type);
    return it != states_.end() ? it->second.info : ModuleInfo{};
}

bool DllVersionDetector::AnalyzeDll(const std::wstring& path, std::array<uint8_t, 32>& outHash, ModuleInfo& info)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;

    // Read DOS header
    IMAGE_DOS_HEADER dos{};
    if (!file.read(reinterpret_cast<char*>(&dos), sizeof(dos)))
        return false;
    if (dos.e_magic != IMAGE_DOS_SIGNATURE) // "MZ"
        return false;

    // Go to NT headers
    if (!file.seekg(dos.e_lfanew, std::ios::beg))
        return false;

    // Read PE signature
    DWORD peSig = 0;
    if (!file.read(reinterpret_cast<char*>(&peSig), sizeof(peSig)))
        return false;
    if (peSig != IMAGE_NT_SIGNATURE)
        return false;

    // Read IMAGE_FILE_HEADER
    IMAGE_FILE_HEADER fh = {};
    if (!file.read(reinterpret_cast<char*>(&fh), sizeof(fh)))
        return false;

    // Read Optional Header
    std::vector<char> optionalHeaderData(fh.SizeOfOptionalHeader);
    if (!file.read(optionalHeaderData.data(), fh.SizeOfOptionalHeader))
        return false;

    // Get image size
    auto* opt32 = reinterpret_cast<const IMAGE_OPTIONAL_HEADER32*>(optionalHeaderData.data());
    info.imageSize = opt32->SizeOfImage;

    // Get reloc VA and size
    const IMAGE_DATA_DIRECTORY& relocDir = opt32->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    info.relocVA = relocDir.VirtualAddress ? info.base + relocDir.VirtualAddress : 0;
    info.relocSize = relocDir.Size;

    // Section table begins here
    if (fh.NumberOfSections == 0)
        return false;

    std::vector<IMAGE_SECTION_HEADER> sections(fh.NumberOfSections);
    const std::streamsize secTableBytes = static_cast<std::streamsize>(fh.NumberOfSections) * static_cast<std::streamsize>(sizeof(IMAGE_SECTION_HEADER));
    if (!file.read(reinterpret_cast<char*>(sections.data()), secTableBytes))
        return false;

    for (const auto& sec : sections)
    {
        if (std::strncmp(reinterpret_cast<const char*>(sec.Name), ".text", 5) != 0)
            continue;

        if (sec.SizeOfRawData == 0)
            return false;

        if (!file.seekg(sec.PointerToRawData, std::ios::beg))
            return false;

        std::vector<uint8_t> rawData(sec.SizeOfRawData);
        if (!file.read(reinterpret_cast<char*>(rawData.data()), rawData.size()))
            return false;

        HCRYPTPROV hProv = 0;
        HCRYPTHASH hHash = 0;
        bool success = false;

        if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT))
        {
            if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash))
            {
                if (CryptHashData(hHash, rawData.data(), rawData.size(), 0))
                {
                    DWORD hashLen = 32;
                    if (CryptGetHashParam(hHash, HP_HASHVAL, outHash.data(), &hashLen, 0))
                    {
                        success = true;
                    }
                }
                CryptDestroyHash(hHash);
            }
            CryptReleaseContext(hProv, 0);
        }

        return success;
    }
    return false;
}

static std::wstring GetProcessDirectory()
{
    wchar_t buffer[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, buffer, MAX_PATH);

    if (len == 0 || len == MAX_PATH)
        return L"";

    std::wstring path(buffer);

    size_t pos = path.find_last_of(L"\\/");
    if (pos != std::wstring::npos)
        path.resize(pos);

    return path;
}

bool DllVersionDetector::DetectFileDllVersion(const DllType type, const std::wstring_view& part)
{
    const std::wstring root = GetProcessDirectory();
    std::queue<std::wstring> q;
    q.push(root);

    std::wstring partLower(part.size(), L'\0');
    std::transform(part.begin(), part.end(), partLower.begin(), ::towlower);
    std::wstring_view partView(partLower);

    while (!q.empty())
    {
        std::wstring dir = q.front();
        q.pop();

        WIN32_FIND_DATAW fd;
        std::wstring search = dir + L"\\*";
        HANDLE hFind = FindFirstFileW(search.c_str(), &fd);
        if (hFind == INVALID_HANDLE_VALUE)
            continue;

        do
        {
            const wchar_t* name = fd.cFileName;

            if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                (wcscmp(name, L".") == 0 || wcscmp(name, L"..") == 0))
                continue;

            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                q.push(dir + L"\\" + name);
            }
            else
            {
                bool match = false;
                if (partLower.empty())
                    match = true;
                else
                {
                    for (const wchar_t* p = name; *p && !match; ++p)
                    {
                        size_t i = 0;
                        for (; i < partLower.size() && std::towlower(p[i]) == partLower[i]; ++i);
                        if (i == partLower.size())
                            match = true;
                    }
                }

                if (match)
                {
                    DetectDllVersion(type, dir + L"\\" + name, 0, 0);
                    FindClose(hFind);
                    return true;
                }
            }
        } while (FindNextFileW(hFind, &fd));

        FindClose(hFind);
    }

    return false;
}
