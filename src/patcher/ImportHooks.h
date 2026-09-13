#pragma once

#include <cstring>

// Redirects a module's imported functions. Found by name, so unlike the code
// hooks this needs no per-version addresses.
namespace ImportHooks
{
    // Returns the original to forward to, or null if not imported.
    inline void* Replace(const uintptr_t moduleBase,
                         const char* importedDll,
                         const char* function,
                         void* replacement)
    {
        if (moduleBase == 0 || replacement == nullptr)
            return nullptr;

        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(moduleBase);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return nullptr;

        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(moduleBase + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE)
            return nullptr;

        const auto& directory = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
        if (directory.VirtualAddress == 0 || directory.Size == 0)
            return nullptr;

        auto* descriptor = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(moduleBase + directory.VirtualAddress);

        for (; descriptor->Name != 0; ++descriptor)
        {
            const char* name = reinterpret_cast<const char*>(moduleBase + descriptor->Name);
            if (_stricmp(name, importedDll) != 0)
                continue;

            // Names survive here; the loader overwrites the address table.
            // Null when a packer rebuilt the directory - see ReplaceByAddress.
            if (descriptor->OriginalFirstThunk == 0)
                continue;

            auto* named = reinterpret_cast<const IMAGE_THUNK_DATA*>(moduleBase + descriptor->OriginalFirstThunk);
            auto* addresses = reinterpret_cast<IMAGE_THUNK_DATA*>(moduleBase + descriptor->FirstThunk);

            for (; named->u1.AddressOfData != 0; ++named, ++addresses)
            {
                if (IMAGE_SNAP_BY_ORDINAL(named->u1.Ordinal))
                    continue;

                const auto* import =
                    reinterpret_cast<const IMAGE_IMPORT_BY_NAME*>(moduleBase + named->u1.AddressOfData);

                if (std::strcmp(reinterpret_cast<const char*>(import->Name), function) != 0)
                    continue;

                void** slot = reinterpret_cast<void**>(&addresses->u1.Function);

                DWORD oldProtect{};
                if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &oldProtect))
                    return nullptr;

                void* original = *slot;
                *slot = replacement;

                VirtualProtect(slot, sizeof(void*), oldProtect, &oldProtect);

                return original;
            }
        }

        return nullptr;
    }

    // Redirects every slot holding `original`, for modules whose import table a
    // packer rebuilt: ASPack zeroes OriginalFirstThunk, so the loader consumes
    // the names in place, and it mirrors the resolved addresses into the original
    // table that the game calls through. Matching on the address finds both
    // copies. Data sections only - a match inside code would be an instruction.
    inline size_t ReplaceByAddress(const uintptr_t moduleBase, void* original, void* replacement)
    {
        if (moduleBase == 0 || original == nullptr || replacement == nullptr)
            return 0;

        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(moduleBase);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE)
            return 0;

        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(moduleBase + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE)
            return 0;

        const auto* section = IMAGE_FIRST_SECTION(nt);
        size_t replaced = 0;

        for (WORD i = 0; i < nt->FileHeader.NumberOfSections; ++i, ++section)
        {
            if (section->Characteristics & IMAGE_SCN_MEM_EXECUTE)
                continue;

            const size_t size = section->Misc.VirtualSize != 0
                              ? section->Misc.VirtualSize
                              : section->SizeOfRawData;
            if (size < sizeof(void*))
                continue;

            auto* start = reinterpret_cast<void**>(moduleBase + section->VirtualAddress);
            auto* end   = start + (size / sizeof(void*));

            for (void** slot = start; slot < end; ++slot)
            {
                // The section may be larger than what is actually committed.
                if (IsBadReadPtr(slot, sizeof(void*)) || *slot != original)
                    continue;

                DWORD oldProtect{};
                if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &oldProtect))
                    continue;

                *slot = replacement;
                ++replaced;

                VirtualProtect(slot, sizeof(void*), oldProtect, &oldProtect);
            }
        }

        return replaced;
    }
}
