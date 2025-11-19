#pragma once

#include <format>
#include <span>

#include "PatchTypes.h"
#include "util.h"

struct RelocationHandle
{
    struct GapRuntime
    {
        RelocateGapSpec spec;
        uintptr_t newVA{};   // base of the new buffer
        size_t    copied{};  // bytes copied from old -> new
    };
    std::vector<GapRuntime> gaps;
    char* bigBlock{};
};

class MemoryRelocator final
{
public:
    bool apply(const ModuleInfo& mod, const std::span<const RelocateGapSpec>& gaps, RelocationHandle& out)
    {
        if (!mod.valid() || gaps.empty())
            return true;

        uintptr_t textBase = 0;
        size_t textSize = 0;
        if (!getTextSection(mod, textBase, textSize) || textBase == 0 || textSize == 0)
            return false;

        out.gaps.clear();
        out.gaps.reserve(gaps.size());

        // Calculate total memory size
        size_t totalSize = 0;
        for (const auto& g : gaps)
            totalSize += g.newSize;

        if (totalSize == 0)
            return true;

        // Allocate the whole memory block. This is done due to the problem of needing continuous memory for some global variables
        out.bigBlock = new char[totalSize];
        if (!out.bigBlock)
            return false;

#ifdef _DEBUG
        OutputDebugStringA(std::format("Allocated one big block of size 0x{:x} at 0x{:x}.\n", totalSize, (uintptr_t)out.bigBlock).c_str());
#endif

        uintptr_t currentBase = reinterpret_cast<uintptr_t>(out.bigBlock);
        size_t currentOffset = 0;
        for (const auto& g : gaps)
        {
            if (g.startOffset == 0)
                continue;

            RelocationHandle::GapRuntime rt{};
            rt.spec = g;

            rt.newVA = currentBase + currentOffset;

#ifdef _DEBUG
            OutputDebugStringA(std::format("Allocated memory 0x{:x} for [0x{:x}; 0x{:x}).\n", rt.newVA, g.startOffset, g.endOffset).c_str());
#endif

            size_t length = (g.endOffset > g.startOffset) ? (g.endOffset - g.startOffset) : 0;
            size_t toCopy = std::min(length, g.newSize);

            if (toCopy)
                std::memcpy(reinterpret_cast<void*>(rt.newVA), reinterpret_cast<const void*>(mod.base + g.startOffset), toCopy);

            rt.copied = toCopy;
            out.gaps.push_back(rt);

            currentOffset += g.newSize;
        }

        if (!patchByRelocs(mod, textBase, textSize, out))
        {
            revert(out);
            return false;
        }
#ifdef _DEBUG
        OutputDebugStringA(std::format("Dll located on 0x{:x}\n", mod.base).c_str());
#endif

        return true;
    }

    void revert(RelocationHandle& h)
    {
        if (h.bigBlock)
        {
            delete[] h.bigBlock;
            h.bigBlock = nullptr;
        }
        h.gaps.clear();
    }

private:
    static bool getTextSection(const ModuleInfo& mod, uintptr_t& outBase, size_t& outSize)
    {
        if (!mod.base)
            return false;

        auto dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(mod.base);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return false;

        auto nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(mod.base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE)
            return false;

        auto sec = IMAGE_FIRST_SECTION(nt);
        for (int i = 0; i < nt->FileHeader.NumberOfSections; ++i)
        {
            if (std::memcmp(sec[i].Name, ".text", 5) == 0)
            {
                outBase = mod.base + sec[i].VirtualAddress;
                outSize = sec[i].Misc.VirtualSize;
                return true;
            }
        }
        return false;
    }

    static bool inOldGap(uintptr_t addr, uintptr_t modBase, const RelocationHandle& h, size_t& idxOut, uintptr_t& newAddrOut)
    {
        for (size_t i = 0; i < h.gaps.size(); ++i)
        {
            const auto& gap = h.gaps[i];
            const uintptr_t gapStartAddr = modBase + gap.spec.startOffset;
            const uintptr_t gapEndAddr = modBase + gap.spec.endOffset;

            if (addr >= gapStartAddr && addr < gapEndAddr)
            {
                size_t offset = addr - gapStartAddr;
                newAddrOut = gap.newVA + offset;

                idxOut = i;
                return true;
            }
        }
        return false;
    }

    static bool patchByRelocs(const ModuleInfo& mod, uintptr_t textBase, size_t textSize, const RelocationHandle& h, bool textOnly = true)
    {
        size_t patched{ 0 };
        void* textPtr = reinterpret_cast<void*>(textBase);

        DWORD oldProtect{};
        if (!VirtualProtect(textPtr, textSize, PAGE_EXECUTE_READWRITE, &oldProtect))
        {
            return false;
        }

        uint8_t* cursor = reinterpret_cast<uint8_t*>(mod.relocVA);
        uint8_t* relocEnd = cursor + mod.relocSize;

        while (cursor + sizeof(IMAGE_BASE_RELOCATION) <= relocEnd)
        {
            auto block = reinterpret_cast<const IMAGE_BASE_RELOCATION*>(cursor);
            if (block->SizeOfBlock < sizeof(IMAGE_BASE_RELOCATION) || block->VirtualAddress == 0)
                break;

            size_t entryCount = (block->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
            const WORD* entries = reinterpret_cast<const WORD*>(cursor + sizeof(IMAGE_BASE_RELOCATION));
            uintptr_t pageVA = mod.base + block->VirtualAddress;

            for (size_t i = 0; i < entryCount; ++i)
            {
                WORD e = entries[i];
                WORD type = e >> 12;
                WORD offset = e & 0x0FFF;

                if (type != IMAGE_REL_BASED_HIGHLOW)
                    continue;

                uintptr_t fixupAddr = pageVA + offset;
                if (fixupAddr < textBase || fixupAddr >= textBase + textSize)
                    continue;

                uint32_t* pVal = reinterpret_cast<uint32_t*>(fixupAddr);
                if (reinterpret_cast<uintptr_t>(pVal + 1) > mod.base + mod.imageSize)
                    continue;

                uintptr_t newPtr = 0;
                size_t idx = 0;

                if (inOldGap(static_cast<uintptr_t>(*pVal), mod.base, h, idx, newPtr))
                {
#ifdef _DEBUG
                    OutputDebugStringA(std::format(
                        "Reloc patch at +0x{:x}: 0x{:08x} -> 0x{:08x}\n",
                        fixupAddr - mod.base, *pVal, (uint32_t)newPtr
                    ).c_str());
#endif

                    * pVal = static_cast<uint32_t>(newPtr);
                    ++patched;

                    FlushInstructionCache(GetCurrentProcess(), pVal, sizeof(uint32_t));
                }
            }

            cursor += block->SizeOfBlock;
        }

        VirtualProtect(textPtr, textSize, oldProtect, &oldProtect);

#ifdef _DEBUG
        OutputDebugStringA(std::format("Patched {} refs via .reloc.\n", patched).c_str());
#endif

        return true;
    }
};
