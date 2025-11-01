#pragma once

#include <cstdint>
#include <span>
#include <unordered_map>
#include <mutex>

#include "MemoryRelocator.h"

class GameGlobals
{
public:
    explicit GameGlobals(const uintptr_t moduleBase) : moduleBase_(moduleBase) {}

    void addReloc(std::span<const RelocationHandle::GapRuntime> newGaps)
    {
        relocs_.insert(relocs_.end(), newGaps.begin(), newGaps.end());
    }


    /**
     * Returns the value of type T stored at the given RVA in the module memory.
     *
     * @tparam T Type of the value to retrieve (cannot be a function type).
     * @param rva Relative virtual address of the value in the module.
     *
     * @return Value of type T located at the specified address.
     */
    template<typename T>
    T getValue(uintptr_t rva)
    {
        static_assert(!std::is_function_v<T>, "getValue cannot be used with function types");
        return *resolve<T*>(rva);
    }

    /**
     * Returns a pointer to the object of type T stored at the given RVA in the module memory.
     *
     * @tparam T Type of the object to retrieve.
     * @param rva Relative virtual address of the object in the module.
     *
     * @return Pointer to the object of type T located at the specified address.
     */
    template<typename T>
    T* getPtr(uintptr_t rva)
    {
        return resolve<T*>(rva);
    }

    /**
     * Returns a function pointer of the specified type stored at the given RVA in the module memory.
     *
     * @tparam FuncType Type of the function pointer to retrieve.
     * @param rva Relative virtual address of the function in the module.
     *
     * @return Function pointer of type FuncType pointing to the specified address.
     */
    template<typename FuncType>
    FuncType* getFn(uintptr_t rva)
    {
        return resolve<FuncType*>(rva);
    }

private:
    uintptr_t moduleBase_;
    std::unordered_map<uintptr_t, uintptr_t> cache_;
    std::vector<RelocationHandle::GapRuntime> relocs_;
    std::mutex cache_mutex_;

    template<typename T>
    T resolve(uintptr_t rva)
    {
        {
            std::lock_guard<std::mutex> lg(cache_mutex_);
            auto it = cache_.find(rva);
            if (it != cache_.end())
                return reinterpret_cast<T>(it->second);
        }

        uintptr_t ptr = resolveReloc(rva);

        {
            std::lock_guard<std::mutex> lg(cache_mutex_);
            cache_[rva] = ptr;
        }

        return reinterpret_cast<T>(ptr);
    }

    uintptr_t resolveReloc(uintptr_t rva)
    {
        for (const auto& gap : relocs_)
        {
            const uintptr_t gapStart = gap.spec.startOffset;
            const uintptr_t gapEnd = gap.spec.endOffset;

            if (rva >= gapStart && rva < gapEnd)
                return gap.newVA + (rva - gapStart);
        }

        return moduleBase_ + rva;
    }
};