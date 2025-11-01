#pragma once

#include "types.h"
#include "PatchTypes.h"

#include <span>
#include <array>

class IPatchProfile
{
public:
    virtual ~IPatchProfile() = default;
    virtual GameVersion version() const = 0;
    virtual std::span<const RelocateGapSpec> relocations() const = 0;
    virtual std::span<const HookSpec> hooks() const = 0;
    virtual std::span<const PatchSpec> patches() const = 0;
};

constexpr bool check_relocations(auto const& reloc)
{
    for (auto const& r : reloc)
    {
        if (!(r.endOffset >= r.startOffset))
        {
            return false;
        }
    }
    return true;
}

template<GameVersion Ver,
    auto& Relocs,
    auto& Hooks,
    auto& Patches>
class PatchProfile final : public IPatchProfile
{
public:
    static_assert(check_relocations(Relocs), "RelocateGapSpec: end must be greater or equal than start");

    GameVersion version() const override { return Ver; }
    std::span<const RelocateGapSpec> relocations() const override { return Relocs; }
    std::span<const HookSpec> hooks() const override { return Hooks; }
    std::span<const PatchSpec> patches() const override { return Patches; }
};
