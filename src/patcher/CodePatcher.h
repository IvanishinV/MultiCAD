#pragma once

#include <vector>

class CodePatcher final
{
public:
    bool apply(const ModuleInfo& mod, const std::span<const HookSpec>& hooks, const std::span<const PatchSpec>& patches)
    {
        return applyCodeHooks(mod, hooks) && applyMemoryPatches(mod, patches);
    }

private:
    static constexpr size_t jumpSize{ 5 };
    bool applyCodeHooks(const ModuleInfo& mod, const std::span<const HookSpec>& hooks)
    {
        for (const auto& h : hooks)
        {
            if (h.targetRva == 0 || h.detour == 0)
                continue;

            uint8_t* target = reinterpret_cast<uint8_t*>(mod.base + h.targetRva);
            uint8_t* detour = reinterpret_cast<uint8_t*>(h.detour);

            DWORD oldProtect{};
            if (!VirtualProtect(target, jumpSize, PAGE_EXECUTE_READWRITE, &oldProtect))
                return false;

            intptr_t relAddr = reinterpret_cast<intptr_t>(detour) -
                reinterpret_cast<intptr_t>(target) - jumpSize;

            target[0] = 0xE9; // jmp rel32
            *reinterpret_cast<int32_t*>(target + 1) = static_cast<int32_t>(relAddr);

            VirtualProtect(target, jumpSize, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), target, jumpSize);
        }
        return true;
    }

    bool applyMemoryPatches(const ModuleInfo& mod, const std::span<const PatchSpec>& patches)
    {
        for (const auto& p : patches)
        {
            if (p.targetRva == 0)
                continue;

            if (p.srcRva != 0)
            {
                if (p.size == 0)
                    continue;

                uint8_t* target = reinterpret_cast<uint8_t*>(mod.base + p.targetRva);
                uint8_t* source = reinterpret_cast<uint8_t*>(mod.base + p.srcRva);

                DWORD oldProtect{};
                if (!VirtualProtect(target, p.size, PAGE_EXECUTE_READWRITE, &oldProtect))
                    return false;

                std::memmove(target, source, p.size);

                VirtualProtect(target, p.size, oldProtect, &oldProtect);
                FlushInstructionCache(GetCurrentProcess(), target, p.size);
            }
            else
            {
                if (p.data.empty())
                    continue;

                uint8_t* target = reinterpret_cast<uint8_t*>(mod.base + p.targetRva);

                DWORD oldProtect{};
                if (!VirtualProtect(target, p.data.size(), PAGE_EXECUTE_READWRITE, &oldProtect))
                    return false;

                std::copy(p.data.begin(), p.data.end(), target);

                VirtualProtect(target, p.data.size(), oldProtect, &oldProtect);
                FlushInstructionCache(GetCurrentProcess(), target, p.data.size());
            }
        }
        return true;
    }
};