#pragma once

#include "types.h"
#include "PatchTypes.h"

#include <span>

class IPatchSpecSet
{
public:
    virtual ~IPatchSpecSet() = default;
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

template<auto& Relocs, auto& Hooks, auto& Patches>
class PatchSpecSet final : public IPatchSpecSet
{
public:
    static_assert(check_relocations(Relocs), "RelocateGapSpec: end must be greater or equal than start");

    std::span<const RelocateGapSpec> relocations() const override { return Relocs; }
    std::span<const HookSpec> hooks() const override { return Hooks; }
    std::span<const PatchSpec> patches() const override { return Patches; }
};

class IGameVersionProfile
{
public:
    virtual ~IGameVersionProfile() = default;

    virtual GameVersion version() const = 0;

    virtual const IPatchSpecSet& game() const = 0;
    virtual const IPatchSpecSet& menu() const = 0;
};

template<GameVersion Ver,
    auto& GameRelocs, auto& GameHooks, auto& GamePatches,
    auto& MenuRelocs, auto& MenuHooks, auto& MenuPatches>
class GameVersionProfile final : public IGameVersionProfile
{
public:
    GameVersion version() const override { return Ver; }

    const IPatchSpecSet& game() const { return m_game; }
    const IPatchSpecSet& menu() const { return m_menu; }

    using GameProfileT = PatchSpecSet<GameRelocs, GameHooks, GamePatches>;
    using MenuProfileT = PatchSpecSet<MenuRelocs, MenuHooks, MenuPatches>;

private:
    GameProfileT m_game{};
    MenuProfileT m_menu{};
};
