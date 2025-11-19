#pragma once

#include <vector>
#include <string>

#include "types.h"

enum class DetectionStatus
{
    NotDetected,
    NotCalculated,
    UnsupportedHash,
    Supported
};

struct HookSpec
{
    uintptr_t targetRva;
    uintptr_t detour;
};

struct PatchSpec
{
    uintptr_t targetRva;
    std::vector<uint8_t> data{};
    uintptr_t srcRva{ 0 };
    size_t size{ 0 };

    template <typename T>
    static std::vector<uint8_t> to_bytes(const T& value)
    {
        static_assert(std::is_trivially_copyable_v<T>, "T must be trivially copyable");
        const uint8_t* begin = reinterpret_cast<const uint8_t*>(&value);
        return std::vector<uint8_t>(begin, begin + sizeof(T));
    }

    template <typename... Parts>
    static std::vector<uint8_t> concat(std::initializer_list<uint8_t> prefix, Parts&&... parts)
    {
        std::vector<uint8_t> result(prefix);
        (append(result, std::forward<Parts>(parts)), ...);
        return result;
    }

private:
    static void append(std::vector<uint8_t>& dst, const std::vector<uint8_t>& src)
    {
        dst.insert(dst.end(), src.begin(), src.end());
    }

    static void append(std::vector<uint8_t>& dst, std::initializer_list<uint8_t> src)
    {
        dst.insert(dst.end(), src.begin(), src.end());
    }

    template <typename T>
    static void append(std::vector<uint8_t>& dst, const T& value)
    {
        auto bytes = to_bytes(value);
        dst.insert(dst.end(), bytes.begin(), bytes.end());
    }
};

struct RelocateGapSpec
{
    uintptr_t   startOffset;    // offset from the mobule base
    uintptr_t   endOffset;      // offset from the module base, not included
    size_t      newSize;
};

enum class DllType
{
    Game,
    Menu,
    Unknown
};

struct ModuleInfo
{
    uintptr_t   base{};
    size_t      imageSize{};  // from PE header
    uintptr_t   relocVA{};
    size_t      relocSize{};

    GameVersion  version{ GameVersion::UNKNOWN };
    DllType     type{ DllType::Unknown };

    bool valid() const
    {
        return base != 0 && version != GameVersion::UNKNOWN;
    }
};
