#include "pch.h"
#include "DllVersionDetector.h"

#include <fstream>
#include <wincrypt.h>
#include <span>

void DllVersionDetector::DetectDll(DllType type, const std::wstring& modulePath)
{
    moduleInfo_.type = type;

    std::array<uint8_t, 32> hash;
    if (!AnalyzeDll(modulePath, hash))
    {
        detectionStatus_ = DetectionStatus::NotCalculated;
        moduleInfo_.version = GameVersion::UNKNOWN;
        return;
    }

    std::span<const DllVersion> versions =
        (type == DllType::Game)
        ? std::span<const DllVersion>(gameDllVersions_)
        : std::span<const DllVersion>(menuDllVersions_);

    for (const auto& kv : versions)
    {
        if (kv.sha256 == hash)
        {
            detectionStatus_ = DetectionStatus::Supported;
            moduleInfo_.version = kv.version;
            return;
        }
    }

    detectionStatus_ = DetectionStatus::UnsupportedHash;
    moduleInfo_.version = GameVersion::UNKNOWN;
}

void DllVersionDetector::DetectGameDll(const std::wstring& modulePath)
{
    std::array<uint8_t, 32> hash;
    if (!AnalyzeDll(modulePath, hash))
    {
        detectionStatus_ = DetectionStatus::NotCalculated;
        moduleInfo_.version = GameVersion::UNKNOWN;
        return;
    }

    for (const auto& kv : gameDllVersions_)
    {
        if (kv.sha256 == hash)
        {
            detectionStatus_ = DetectionStatus::Supported;
            moduleInfo_.version = kv.version;
            return;
        }
    }

    detectionStatus_ = DetectionStatus::UnsupportedHash;
    moduleInfo_.version = GameVersion::UNKNOWN;
}

void DllVersionDetector::DetectMenuDll(const std::wstring& modulePath)
{
    std::array<uint8_t, 32> hash;
    if (!AnalyzeDll(modulePath, hash))
    {
        detectionStatus_ = DetectionStatus::NotCalculated;
        moduleInfo_.version = GameVersion::UNKNOWN;
        return;
    }

    for (const auto& kv : menuDllVersions_)
    {
        if (kv.sha256 == hash)
        {
            detectionStatus_ = DetectionStatus::Supported;
            moduleInfo_.version = kv.version;
            return;
        }
    }

    detectionStatus_ = DetectionStatus::UnsupportedHash;
    moduleInfo_.version = GameVersion::UNKNOWN;
}

DetectionStatus DllVersionDetector::GetDetectionStatus() const
{
    return detectionStatus_;
}

GameVersion DllVersionDetector::GetGameVersion() const
{
    return moduleInfo_.version;
}

ModuleInfo DllVersionDetector::GetModuleInfo() const
{
    return moduleInfo_;
}

bool DllVersionDetector::AnalyzeDll(const std::wstring& path, std::array<uint8_t, 32>& outHash)
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
    moduleInfo_.imageSize = opt32->SizeOfImage;

    // Get reloc VA and size
    const IMAGE_DATA_DIRECTORY& relocDir = opt32->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC];
    moduleInfo_.relocVA = relocDir.VirtualAddress ? moduleInfo_.base + relocDir.VirtualAddress : 0;
    moduleInfo_.relocSize = relocDir.Size;

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