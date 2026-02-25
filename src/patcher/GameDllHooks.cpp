#include "pch.h"
#include "GameDllHooks.h"

int __declspec(noinline) __fastcall GameDllHooks::sub_1001D240(GameData5* self, void* /*dummy*/, int** a2)
{
    if (!self || !a2 || !a2[1] || !self->param_04)
        return 0;

    const int flags = a2[1][8];
    const uint8_t a2_byte28 = *((uint8_t*)a2 + 28);

    if (self->param_B3
        || (self->param_1A & 1) != 0
        || self->param_B8[0]
        || self->param_1C != a2_byte28 && (flags & 0x4000000) == 0
        || !(*(int(__thiscall**)(int*, int*))(*self->param_04 + 48))(self->param_04, a2[1]))
    {
        return 0;
    }

    if ((flags & 0x4200000) != 0)
    {
        if (self->param_17A >= self->param_04[0x51D])
            return 0;
    }
    else if ((flags & 0x1000000) == 0 || self->param_17A + 2 > self->param_04[1309])
    {
        return 0;
    }

    return 1;
}


void __declspec(noinline) __cdecl GameDllHooks::sub_1003E7B0(UnkEntry* a1, int a2, int* a3, int a4)
{
    auto* const g = globals_;

    SomeRandCalcData data
    {
        a1,
        a2,
        a3,
        a4,
        g->getFn<int(__thiscall)(UnkEntry*, int)>(0x2560),
        *g->getPtr<int>(0x295144)
    };

    someRandCalc(data);
}

void __declspec(noinline) __cdecl GameDllHooks::sub_1003E7B0_de(UnkEntry* a1, int a2, int* a3, int a4)
{
    auto* const g = globals_;

    SomeRandCalcData data
    {
        a1,
        a2,
        a3,
        a4,
        g->getFn<int(__thiscall)(UnkEntry*, int)>(0x2650),
        *g->getPtr<int>(0x295110)
    };

    someRandCalc(data);
}

void __declspec(noinline) __cdecl GameDllHooks::sub_1003E7B0_fr(UnkEntry* a1, int a2, int* a3, int a4)
{
    auto* const g = globals_;

    SomeRandCalcData data
    {
        a1,
        a2,
        a3,
        a4,
        g->getFn<int(__thiscall)(UnkEntry*, int)>(0x2650),
        *g->getPtr<int>(0x299130)
    };

    someRandCalc(data);
}


void __declspec(noinline) __fastcall GameDllHooks::sub_10055A20(uint32_t* buffer, void* /*dummy*/, int shiftX, int shiftY)
{
    static constexpr uint32_t BUFFER_OFFSET = 8;

    const int rowBytes = (int)buffer[0];    // number of useful bytes in line
    const int height = (int)buffer[1];      // number of line
    const int colShift = shiftX >> 4;       // how many bytes to move in line
    const int rowShift = shiftY >> 3;       // how many lines to move

    // Quick check: if the shift completely takes the image out of bounds -> zero the entire buffer
    if (colShift > rowBytes || rowShift > height || -colShift > rowBytes || -rowShift > height)
    {
        // Zero out the useful part of each line
        for (int r = 0; r < height; ++r)
        {
            uint8_t* dest = reinterpret_cast<uint8_t*>(buffer) + BUFFER_OFFSET + r * kRowStrideByteSize;
            std::memset(dest, 0, (size_t)rowBytes);
        }
        return;
    }

    // Calculate the copy range inside the line
    const int absColShift = (colShift >= 0) ? colShift : -colShift;
    const int copyLen = rowBytes - absColShift; // Number of bytes to be copied from source to dest

    // If there are no useful bytes to copy, just zero everything within the destination area
    if (copyLen <= 0)
    {
        for (int r = 0; r < height; ++r)
        {
            uint8_t* dest = reinterpret_cast<uint8_t*>(buffer) + BUFFER_OFFSET + r * kRowStrideByteSize;
            std::memset(dest, 0, (size_t)rowBytes);
        }
        return;
    }

    // We choose the direction of traversal of lines so as not to overwrite the original lines before reading:
    // - if dest > src - iterate from bottom to top
    // - otherwise - iterate from top to bottom
    if (rowShift > 0)
    {
        // process from bottom to top
        for (int destR = height - 1; destR >= 0; --destR)
        {
            int srcR = destR - rowShift;

            uint8_t* dest = reinterpret_cast<uint8_t*>(buffer) + BUFFER_OFFSET + destR * kRowStrideByteSize;

            if (srcR < 0 || srcR >= height)
            {
                // No source - just zero the line
                std::memset(dest, 0, (size_t)rowBytes);
                continue;
            }

            uint8_t* src = reinterpret_cast<uint8_t*>(buffer) + BUFFER_OFFSET + srcR * kRowStrideByteSize;

            if (colShift >= 0)
            {
                // Copy src[0 .. copyLen-1] -> dest[colShift .. colShift+copyLen-1]
                std::memmove(dest + colShift, src, (size_t)copyLen);
                // Clear left prefix
                if (colShift > 0)
                    std::memset(dest, 0, (size_t)colShift);
            }
            else
            {
                // colShift < 0: copy src[absColShift .. absColShift+copyLen-1] -> dest[0 .. copyLen-1]
                std::memmove(dest, src + absColShift, (size_t)copyLen);
                // Clear right suffix
                if (absColShift > 0)
                    std::memset(dest + copyLen, 0, (size_t)absColShift);
            }
        }
    }
    else if (rowShift < 0)
    {
        // Process from top to bottom
        for (int destR = 0; destR < height; ++destR)
        {
            int srcR = destR - rowShift; // Since rowShift < 0, srcR > destR

            uint8_t* dest = reinterpret_cast<uint8_t*>(buffer) + BUFFER_OFFSET + destR * kRowStrideByteSize;

            if (srcR < 0 || srcR >= height)
            {
                std::memset(dest, 0, (size_t)rowBytes);
                continue;
            }

            uint8_t* src = reinterpret_cast<uint8_t*>(buffer) + BUFFER_OFFSET + srcR * kRowStrideByteSize;

            if (colShift >= 0)
            {
                std::memmove(dest + colShift, src, (size_t)copyLen);
                if (colShift > 0) std::memset(dest, 0, (size_t)colShift);
            }
            else
            {
                std::memmove(dest, src + absColShift, (size_t)copyLen);
                if (absColShift > 0) std::memset(dest + copyLen, 0, (size_t)absColShift);
            }
        }
    }
    else
    {
        // rowShift == 0: no line shifting - just copy in any direction
        for (int destR = 0; destR < height; ++destR)
        {
            uint8_t* dest = reinterpret_cast<uint8_t*>(buffer) + BUFFER_OFFSET + destR * kRowStrideByteSize;
            uint8_t* src = reinterpret_cast<uint8_t*>(buffer) + BUFFER_OFFSET + destR * kRowStrideByteSize;

            if (colShift >= 0)
            {
                std::memmove(dest + colShift, src, (size_t)copyLen);
                if (colShift > 0) std::memset(dest, 0, (size_t)colShift);
            }
            else
            {
                std::memmove(dest, src + absColShift, (size_t)copyLen);
                if (absColShift > 0) std::memset(dest + copyLen, 0, (size_t)absColShift);
            }
        }
    }
}

void __declspec(noinline) __fastcall GameDllHooks::sub_10055DC0(uint32_t* input)
{
    if (input[1] == 0)
        return;

    for (U32 i = 0, *v2 = &input[2]; i < input[1]; ++i)
    {
        memset(v2, 0, input[0]);
        v2 += kRowStrideDwordSize;
    }
}

void __declspec(noinline) __fastcall GameDllHooks::sub_10055E00(int* input, void* /*dummy*/, char a2, int a3, int a4, int a5, int a6)
{
    int v6 = a3 >> 4;
    int v7 = a6 >> 3;
    int v8 = a5 >> 4;
    int v9 = a4 >> 3;

    if (v6 < 0)
        v6 = 0;
    else if (v6 >= input[0])
        return;

    if (v9 < 0)
        v9 = 0;
    else if (v9 >= input[1])
        return;

    if (v8 < 0)
        return;
    else if (v8 >= input[0])
        v8 = input[0] - 1;

    if (v7 < 0)
        return;
    else if (v7 >= input[1])
        v7 = input[1] - 1;

    int v11 = v7 - v9 + 1;
    if (v11)
    {
        char* v12 = (char*)&input[kRowStrideDwordSize * v9 + 2] + v6;
        const int v13 = v8 - v6 + 1;
        do
        {
            if (v13)
            {
                int v15 = v13;
                do
                {
                    *v12++ |= a2;
                    --v15;
                } while (v15);
            }
            v12 += kRowStrideByteSize - v13;
            --v11;
        } while (v11);
    }
}

void __declspec(noinline) __fastcall GameDllHooks::sub_10055E90(int* input, void* /*dummy*/, char a2, int a3, int a4, __int16* a5)
{
    int v5 = *a5;
    int v6 = a5[1];
    int v7 = v6 + a5[3] + a4 - 1;
    int v8 = v6 + a4;
    int v9 = (v5 + a3) >> 4;
    int v10 = v7 >> 3;
    int v11 = (v5 + a5[2] + a3 - 1) >> 4;
    int v12 = v8 >> 3;

    if (v9 < 0)
        v9 = 0;
    else if (v9 >= *input)
        return;

    if (v12 < 0)
        v12 = 0;
    else if (v12 >= input[1])
        return;

    if (v11 < 0)
        return;
    else if (v11 >= *input)
        v11 = *input - 1;

    int v13 = input[1];
    if (v10 < 0)
        return;
    else if (v10 >= v13) v10 = v13 - 1;

    int v14 = v10 - v12 + 1;
    char* v15 = (char*)&input[kRowStrideDwordSize * v12 + 2] + v9;
    int v16 = v11 - v9 + 1;

    if (v14)
    {
        int v17 = v14;
        do
        {
            if (v16)
            {
                int v18 = v16;
                do
                {
                    *v15++ |= a2;
                    --v18;
                } while (v18);
            }
            v15 += kRowStrideByteSize - v16;
            --v17;
        } while (v17);
    }
}

void __declspec(noinline) __fastcall GameDllHooks::sub_10055F40(int* input, void* /*dummy*/, char a2, int a3, int a4, int a5, int a6)
{
    int v6 = a3 >> 4;
    int v7 = a6 >> 3;
    int v8 = a5 >> 4;
    int v9 = a4 >> 3;

    if (v6 < 0)
        v6 = 0;
    else if (v6 >= *input)
        return;

    if (v9 < 0)
        v9 = 0;
    else if (v9 >= input[1])
        return;

    if (v8 < 0)
        return;
    else if (v8 >= *input)
        v8 = *input - 1;

    int v10 = input[1];
    if (v7 < 0)
        return;
    else if (v7 >= v10)
        v7 = v10 - 1;

    char* v11 = (char*)&input[kRowStrideDwordSize * v9 + 2] + v6;
    int v12 = v8 - v6 + 1;
    int v13 = v7 - v9 + 1;

    if (v13)
    {
        int v14 = v13;
        do
        {
            if (v12)
            {
                int v15 = v12;
                do
                {
                    *v11++ &= -1 - a2;
                    --v15;
                } while (v15);
            }
            v11 += kRowStrideByteSize - v12;
            --v14;
        } while (v14);
    }
}

void __declspec(noinline) __fastcall GameDllHooks::sub_10055FE0(int* input, void* /*dummy*/, char a2)
{
    int v2 = 0;
    if (input[1] > 0)
    {
        int* v4 = input + 2;
        do
        {
            for (int i = 0; i < *input; ++i)
                *((unsigned char*)v4 + i) &= static_cast<unsigned char>(~a2);
            ++v2;
            v4 += kRowStrideDwordSize;
        } while (v2 < input[1]);
    }
}

int  __declspec(noinline) __fastcall GameDllHooks::sub_10056030(uint8_t* input, void* /*dummy*/, int x, int y, GameData* const gd)
{
    const int maxX = gd->maxX;
    const int maxY = gd->maxY;
    const uint8_t mask = gd->mask;
    const uint8_t maskValue = gd->maskValue;

    if (y >= maxY)
        return 0;

    uint8_t* line = &input[kRowStrideByteSize * y + 8];

    while (x >= maxX)
    {
    LABEL_7:
        ++y;
        if (y >= maxY)
            return 0;

        x = gd->x;
        line += kRowStrideByteSize;
    }
    while ((mask & line[x]) == 0)
    {
        if (++x >= maxX)
        {
            goto LABEL_7;
        }
    }

    line[x] &= maskValue;

    int v11 = x + 1;
    if (v11 < maxX)
    {
        do
        {
            uint8_t v13 = line[v11];
            if ((v13 & mask) == 0)
                break;
            line[v11++] = maskValue & v13;
        } while (v11 < maxX);
    }

    int v14 = v11 - 1;
    int v15 = y + 1;

    if (v15 < maxY)
    {
        uint8_t* nextLine = line + kRowStrideByteSize;
        uint8_t* ptrMask = &input[kRowStrideByteSize * v15 + 9 + v14];

        while (v15 < maxY)
        {
            for (int i = x; i <= v14; ++i)
            {
                if ((nextLine[i] & mask) == 0)
                {
                    goto doneMasking;
                }
            }

            bool cond1 = (x <= gd->x) || ((nextLine[x - 1] & mask) == 0);
            bool cond2 = (v14 >= (gd->maxX - 1)) || ((*ptrMask & mask) == 0);

            if (cond1 && cond2)
            {
                for (int j = x; j <= v14; ++j)
                    nextLine[j] &= maskValue;

                ptrMask += kRowStrideByteSize;
                nextLine += kRowStrideByteSize;

                ++v15;
                if (v15 < maxY)
                    continue;
            }

            break;
        }
    }

doneMasking:
    gd->alignX = 16 * x;
    gd->alignY = 8 * y;
    gd->allowX = 16 * v14 + 15;
    gd->allowY = 8 * v15 - 1;

    return 1;
}

int  __declspec(noinline) __fastcall GameDllHooks::sub_10056170(uint8_t* input, void* /*dummy*/, int x, int y, GameData* const gd)
{
    const int maxX = gd->maxX;
    const int maxY = gd->maxY;
    const uint8_t mask = gd->mask;
    const uint8_t maskValue = gd->maskValue;

    if (y >= maxY)
        return 0;

    uint8_t* line = &input[kRowStrideByteSize * y + 8];

    while (x >= maxX)
    {
    LABEL_7:
        ++y;
        if (y >= maxY)
            return 0;

        x = gd->x;
        line += kRowStrideByteSize;
    }
    while ((mask & line[x]) != mask)
    {
        if (++x >= maxX)
        {
            goto LABEL_7;
        }
    }

    line[x] &= maskValue;

    int v10 = x + 1;
    if (v10 < maxX)
    {
        do
        {
            if ((mask & line[v10]) != mask)
                break;
            line[v10++] &= maskValue;
        } while (v10 < maxX);
    }

    int v13 = v10 - 1;
    int v12 = y + 1;

    if (v12 < maxY)
    {
        uint8_t* nextLine = line + kRowStrideByteSize;
        uint8_t* ptrMask = nextLine + 1 + v13;

        while (v12 < maxY)
        {
            for (int idx = x; idx <= v13; ++idx)
            {
                if ((mask & nextLine[idx]) != mask)
                {
                    goto doneMasking;
                }
            }

            bool cond1 = (x <= gd->x) || ((mask & nextLine[x - 1]) != mask);
            bool cond2 = (v13 >= maxX - 1) || ((*ptrMask & mask) != mask);

            if (cond1 && cond2)
            {
                for (int i = x; i <= v13; ++i)
                    nextLine[i] &= maskValue;

                ptrMask += kRowStrideByteSize;
                nextLine += kRowStrideByteSize;

                ++v12;
                continue;
            }
            break;
        }
    }

doneMasking:
    gd->alignX = 16 * x;
    gd->alignY = 8 * y;
    gd->allowX = 16 * v13 + 15;
    gd->allowY = 8 * v12 - 1;

    return 1;
}

int  __declspec(noinline) __fastcall GameDllHooks::sub_100563B0(uint8_t* input, void* /*dummy*/, int x, int y, GameData2* const gd)
{
    const uint8_t mask = gd->mask;
    const uint8_t maskValue = gd->maskValue;
    const uint8_t combinedMask = mask | maskValue;
    const uint8_t clearMask = static_cast<uint8_t>(-1 - combinedMask);

    const int maxX = gd->maxX;
    const int maxY = gd->maxY;

    if (y >= maxY)
        return 0;

    uint8_t* line = &input[kRowStrideByteSize * y + 8];

    while (x >= maxX)
    {
        ++y;
        if (y >= maxY)
            return 0;

        line += kRowStrideByteSize;
        x = gd->x;
    }

    while ((combinedMask & line[x]) == 0)
    {
        if (++x >= maxX)
        {
            ++y;
            if (y >= maxY)
                return 0;

            line += kRowStrideByteSize;
            x = gd->x;
        }
    }

    uint8_t cellMask = line[x] & combinedMask;
    line[x] &= clearMask;

    int vNext = x + 1;
    if (vNext < maxX)
    {
        do
        {
            uint8_t val = line[vNext];
            if ((val & cellMask) == 0)
                break;
            line[vNext++] &= clearMask;
        } while (vNext < maxX);
    }

    int vEndX = vNext - 1;
    int vNextY = y + 1;

    if (vNextY < maxY)
    {
        line += kRowStrideByteSize;
        while (vNextY < maxY)
        {
            for (int idx = x; idx <= vEndX; ++idx)
            {
                if ((cellMask & line[idx]) == 0)
                {
                    goto doneMasking;
                }
            }

            for (int idx = x; idx <= vEndX; ++idx)
                line[idx] &= -1 - combinedMask;

            ++vNextY;
            line += kRowStrideByteSize;
        }
    }

doneMasking:
    gd->alignX = 16 * x;
    gd->alignY = 8 * y;
    gd->allowX = 16 * vEndX + 15;
    gd->allowY = 8 * vNextY - 1;
    gd->cellMask = cellMask;

    return 1;
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1005C170()
{
    auto* const g = globals_;

    int* dword_100ADF88 = g->getPtr<int>(0xADF88);
    int* dword_10299D64 = g->getPtr<int>(0x299D64);
    int* dword_10295C58 = g->getPtr<int>(0x295C58);
    char* byte_10295C5F = g->getPtr<char>(0x295C5F);
    char* byte_10295C60 = g->getPtr<char>(0x295C60);
    char* byte_10296CEE = g->getPtr<char>(0x296CEE);
    char* byte_102996E0 = g->getPtr<char>(0x2996E0);
    int16_t* word_10296C63 = g->getPtr<int16_t>(0x296C63);
    char* byte_102990E0 = g->getPtr<char>(0x2990E0);
    int dword_10383CAC = g->getValue<int>(0x383CAC);

    auto sub_10056740 = g->getFn<void(__cdecl)(void*)>(0x56740);
    auto sub_100592C0 = g->getFn<void(__stdcall)(void*, size_t)>(0x592C0);
    auto sub_1005C0D0 = g->getFn<void(__cdecl)(char*, char*)>(0x5C0D0);

    int& randSeed = *g->getPtr<int>(0x295144);
    auto randNext = [&randSeed]() {
        randSeed = 0x41C64E6D * randSeed + 0x3039;
        return HIWORD(randSeed) & 0x7FFF;
        };

    int count = *dword_10299D64;

    char tempBuffer[128];
    memcpy(tempBuffer, &byte_102990E0[128 * dword_10383CAC], sizeof(tempBuffer));

    size_t writeIndex = 2;
    *(uint16_t*)byte_10295C60 = (uint16_t)randNext();
    *dword_10295C58 = 2;

    int16_t* currentWord = word_10296C63;
    for (int i = 0; i < count; ++i, currentWord = currentWord + 73)
    {
        unsigned char flags = *((unsigned char*)currentWord - 3);
        // Condition when ordering unit to move
        if ((flags & 0x20) == 0)
        {
            int j = 0;
            bool skip = false;
            while (!*((unsigned char*)currentWord + j + 11))
            {
                if (++j >= 128)
                {
                    skip = true;
                    break;
                }
            }
            if (skip)
                continue;

            sub_1005C0D0(tempBuffer, (char*)currentWord + 11);

            writeIndex = *dword_10295C58;
            byte_10295C60[writeIndex++] = flags;
            if (flags & 0x40)
            {
                memcpy(&byte_10295C60[writeIndex], currentWord, 2 * sizeof(int16_t));
                writeIndex += 2 * sizeof(int16_t);
            }

            if ((char)flags < 0)
            {
                memcpy(&byte_10295C60[writeIndex], &currentWord[2], sizeof(int16_t));
                writeIndex += sizeof(int16_t);
            }

            count = *dword_10299D64;

            char lastChar = *((unsigned char*)currentWord + 6);
            byte_10295C60[writeIndex++] = lastChar;

            *dword_10295C58 = writeIndex;
        }
        else
        {
            switch (flags)
            {
            case 33:
            {
                char v10 = *((char*)currentWord + 6);
                byte_10295C60[writeIndex++] = 33;
                byte_10295C60[writeIndex++] = v10;
                *dword_10295C58 = writeIndex;

                if (v10 & 0x80)
                    byte_10295C60[writeIndex++] = *((char*)currentWord + 4);

                if (v10 & 0x40)
                {
                    memcpy(&byte_10295C60[writeIndex], currentWord, 2 * sizeof(int16_t));
                    writeIndex += 2 * sizeof(int16_t);
                }

                *dword_10295C58 = writeIndex;

                break;
            }
            case 34:
            {
                byte_10295C60[writeIndex++] = 34;

                byte_10295C60[writeIndex] = *((char*)currentWord + 6);
                *dword_10295C58 = ++writeIndex;
                break;
            }
            case 35:
            {
                // Case when saving game
                int v5 = *currentWord;
                byte_10295C60[writeIndex++] = 35;
                byte_10295C60[writeIndex++] = static_cast<char>(v5);
                *dword_10295C58 = writeIndex;
                if (v5 == 0)
                {
                    byte_10295C60[writeIndex] = *((char*)currentWord + 2);
                }
                else if (v5 == 2)
                {
                    char* src = *(char**)byte_10296CEE;
                    if (src)
                    {
                        for (int k = 0; k < 16; ++k)
                        {
                            byte_10295C5F[++writeIndex] = *src++;
                            *dword_10295C58 = writeIndex;
                        }
                        while (*src)
                        {
                            byte_10295C60[writeIndex++] = *src++;
                            *dword_10295C58 = writeIndex;
                        }

                        byte_10295C60[writeIndex] = *src;
                    }
                }
                *dword_10295C58 = ++writeIndex;
                break;
            }
            default:
                break;
            }
        }
    }

    memset(byte_102996E0, 0xFF, 0x680);
    byte_102996E0[1664] = -1;

    constexpr size_t kBlockSize = 146;
    for (int i = 0; i < count; ++i)
    {
        char* block = byte_10296CEE + i * kBlockSize;
        void* ptr = *reinterpret_cast<void**>(block);
        if (ptr != nullptr)
        {
            sub_10056740(ptr);
            count = *dword_10299D64;
            *reinterpret_cast<void**>(block) = nullptr;
        }
    }

    *dword_100ADF88 = g->getValue<int>(0x295618);
    *dword_10299D64 = 0;
    sub_100592C0(byte_10295C60, writeIndex);
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1005C170_de()
{
    auto* const g = globals_;

    int* dword_100ADF88 = g->getPtr<int>(0xAFF88);
    int* dword_10299D64 = g->getPtr<int>(0x299D34);
    int* dword_10295C58 = g->getPtr<int>(0x295C28);
    char* byte_10295C5F = g->getPtr<char>(0x295C2B);
    char* byte_10295C60 = g->getPtr<char>(0x295C2C);
    char* byte_10296CEE = g->getPtr<char>(0x296CBE);
    char* byte_102996E0 = g->getPtr<char>(0x2996B0);
    int16_t* word_10296C63 = g->getPtr<int16_t>(0x296C33);
    char* byte_102990E0 = g->getPtr<char>(0x2990B0);
    int dword_10383CAC = g->getValue<int>(0x383C80);

    auto sub_10056740 = g->getFn<void(__cdecl)(void*)>(0x58C30);
    auto sub_100592C0 = g->getFn<void(__stdcall)(void*, size_t)>(0x5B8D0);
    auto sub_1005C0D0 = g->getFn<void(__cdecl)(char*, char*)>(0x5E710);

    int& randSeed = *g->getPtr<int>(0x295110);
    auto randNext = [&randSeed]() {
        randSeed = 0x41C64E6D * randSeed + 0x3039;
        return HIWORD(randSeed) & 0x7FFF;
        };

    int count = *dword_10299D64;

    char tempBuffer[128];
    memcpy(tempBuffer, &byte_102990E0[128 * dword_10383CAC], sizeof(tempBuffer));

    size_t writeIndex = 2;
    *(uint16_t*)byte_10295C60 = (uint16_t)randNext();
    *dword_10295C58 = 2;

    int16_t* currentWord = word_10296C63;
    for (int i = 0; i < count; ++i, currentWord = currentWord + 73)
    {
        unsigned char flags = *((unsigned char*)currentWord - 3);
        // Condition when ordering unit to move
        if ((flags & 0x20) == 0)
        {
            int j = 0;
            bool skip = false;
            while (!*((unsigned char*)currentWord + j + 11))
            {
                if (++j >= 128)
                {
                    skip = true;
                    break;
                }
            }
            if (skip)
                continue;

            sub_1005C0D0(tempBuffer, (char*)currentWord + 11);

            writeIndex = *dword_10295C58;
            byte_10295C60[writeIndex++] = flags;
            if (flags & 0x40)
            {
                memcpy(&byte_10295C60[writeIndex], currentWord, 2 * sizeof(int16_t));
                writeIndex += 2 * sizeof(int16_t);
            }

            if ((char)flags < 0)
            {
                memcpy(&byte_10295C60[writeIndex], &currentWord[2], sizeof(int16_t));
                writeIndex += sizeof(int16_t);
            }

            count = *dword_10299D64;

            char lastChar = *((unsigned char*)currentWord + 6);
            byte_10295C60[writeIndex++] = lastChar;

            *dword_10295C58 = writeIndex;
        }
        else
        {
            switch (flags)
            {
            case 33:
            {
                char v10 = *((char*)currentWord + 6);
                byte_10295C60[writeIndex++] = 33;
                byte_10295C60[writeIndex++] = v10;
                *dword_10295C58 = writeIndex;

                if (v10 & 0x80)
                    byte_10295C60[writeIndex++] = *((char*)currentWord + 4);

                if (v10 & 0x40)
                {
                    memcpy(&byte_10295C60[writeIndex], currentWord, 2 * sizeof(int16_t));
                    writeIndex += 2 * sizeof(int16_t);
                }

                *dword_10295C58 = writeIndex;

                break;
            }
            case 34:
            {
                byte_10295C60[writeIndex++] = 34;

                byte_10295C60[writeIndex] = *((char*)currentWord + 6);
                *dword_10295C58 = ++writeIndex;
                break;
            }
            case 35:
            {
                // Case when saving game
                int v5 = *currentWord;
                byte_10295C60[writeIndex++] = 35;
                byte_10295C60[writeIndex++] = static_cast<char>(v5);
                *dword_10295C58 = writeIndex;
                if (v5 == 0)
                {
                    byte_10295C60[writeIndex] = *((char*)currentWord + 2);
                }
                else if (v5 == 2)
                {
                    char* src = *(char**)byte_10296CEE;
                    if (src)
                    {
                        for (int k = 0; k < 16; ++k)
                        {
                            byte_10295C5F[++writeIndex] = *src++;
                            *dword_10295C58 = writeIndex;
                        }
                        while (*src)
                        {
                            byte_10295C60[writeIndex++] = *src++;
                            *dword_10295C58 = writeIndex;
                        }

                        byte_10295C60[writeIndex] = *src;
                    }
                }
                *dword_10295C58 = ++writeIndex;
                break;
            }
            default:
                break;
            }
        }
    }

    memset(byte_102996E0, 0xFF, 0x680);
    byte_102996E0[1664] = -1;

    constexpr size_t kBlockSize = 146;
    for (int i = 0; i < count; ++i)
    {
        char* block = byte_10296CEE + i * kBlockSize;
        void* ptr = *reinterpret_cast<void**>(block);
        if (ptr != nullptr)
        {
            sub_10056740(ptr);
            count = *dword_10299D64;
            *reinterpret_cast<void**>(block) = nullptr;
        }
    }

    *dword_100ADF88 = g->getValue<int>(0x2955E8);
    *dword_10299D64 = 0;
    sub_100592C0(byte_10295C60, writeIndex);
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1005C170_fr()
{
    auto* const g = globals_;

    int* dword_100ADF88 = g->getPtr<int>(0xB1F88);
    int* dword_10299D64 = g->getPtr<int>(0x29DD54);
    int* dword_10295C58 = g->getPtr<int>(0x299C48);
    char* byte_10295C5F = g->getPtr<char>(0x299C4B);
    char* byte_10295C60 = g->getPtr<char>(0x299C4C);
    char* byte_10296CEE = g->getPtr<char>(0x29ACDE);
    char* byte_102996E0 = g->getPtr<char>(0x29D6D0);
    int16_t* word_10296C63 = g->getPtr<int16_t>(0x29AC53);
    char* byte_102990E0 = g->getPtr<char>(0x29D0D0);
    int dword_10383CAC = g->getValue<int>(0x387CA0);

    auto sub_10056740 = g->getFn<void(__cdecl)(void*)>(0x58BE0);
    auto sub_100592C0 = g->getFn<void(__stdcall)(void*, size_t)>(0x5B880);
    auto sub_1005C0D0 = g->getFn<void(__cdecl)(char*, char*)>(0x5E770);

    int& randSeed = *g->getPtr<int>(0x299130);
    auto randNext = [&randSeed]() {
        randSeed = 0x41C64E6D * randSeed + 0x3039;
        return HIWORD(randSeed) & 0x7FFF;
        };

    int count = *dword_10299D64;

    char tempBuffer[128];
    memcpy(tempBuffer, &byte_102990E0[128 * dword_10383CAC], sizeof(tempBuffer));

    size_t writeIndex = 2;
    *(uint16_t*)byte_10295C60 = (uint16_t)randNext();
    *dword_10295C58 = 2;

    int16_t* currentWord = word_10296C63;
    for (int i = 0; i < count; ++i, currentWord = currentWord + 73)
    {
        unsigned char flags = *((unsigned char*)currentWord - 3);
        // Condition when ordering unit to move
        if ((flags & 0x20) == 0)
        {
            int j = 0;
            bool skip = false;
            while (!*((unsigned char*)currentWord + j + 11))
            {
                if (++j >= 128)
                {
                    skip = true;
                    break;
                }
            }
            if (skip)
                continue;

            sub_1005C0D0(tempBuffer, (char*)currentWord + 11);

            writeIndex = *dword_10295C58;
            byte_10295C60[writeIndex++] = flags;
            if (flags & 0x40)
            {
                memcpy(&byte_10295C60[writeIndex], currentWord, 2 * sizeof(int16_t));
                writeIndex += 2 * sizeof(int16_t);
            }

            if ((char)flags < 0)
            {
                memcpy(&byte_10295C60[writeIndex], &currentWord[2], sizeof(int16_t));
                writeIndex += sizeof(int16_t);
            }

            count = *dword_10299D64;

            char lastChar = *((unsigned char*)currentWord + 6);
            byte_10295C60[writeIndex++] = lastChar;

            *dword_10295C58 = writeIndex;
        }
        else
        {
            switch (flags)
            {
            case 33:
            {
                char v10 = *((char*)currentWord + 6);
                byte_10295C60[writeIndex++] = 33;
                byte_10295C60[writeIndex++] = v10;
                *dword_10295C58 = writeIndex;

                if (v10 & 0x80)
                    byte_10295C60[writeIndex++] = *((char*)currentWord + 4);

                if (v10 & 0x40)
                {
                    memcpy(&byte_10295C60[writeIndex], currentWord, 2 * sizeof(int16_t));
                    writeIndex += 2 * sizeof(int16_t);
                }

                *dword_10295C58 = writeIndex;

                break;
            }
            case 34:
            {
                byte_10295C60[writeIndex++] = 34;

                byte_10295C60[writeIndex] = *((char*)currentWord + 6);
                *dword_10295C58 = ++writeIndex;
                break;
            }
            case 35:
            {
                // Case when saving game
                int v5 = *currentWord;
                byte_10295C60[writeIndex++] = 35;
                byte_10295C60[writeIndex++] = static_cast<char>(v5);
                *dword_10295C58 = writeIndex;
                if (v5 == 0)
                {
                    byte_10295C60[writeIndex] = *((char*)currentWord + 2);
                }
                else if (v5 == 2)
                {
                    char* src = *(char**)byte_10296CEE;
                    if (src)
                    {
                        for (int k = 0; k < 16; ++k)
                        {
                            byte_10295C5F[++writeIndex] = *src++;
                            *dword_10295C58 = writeIndex;
                        }
                        while (*src)
                        {
                            byte_10295C60[writeIndex++] = *src++;
                            *dword_10295C58 = writeIndex;
                        }

                        byte_10295C60[writeIndex] = *src;
                    }
                }
                *dword_10295C58 = ++writeIndex;
                break;
            }
            default:
                break;
            }
        }
    }

    memset(byte_102996E0, 0xFF, 0x680);
    byte_102996E0[1664] = -1;

    constexpr size_t kBlockSize = 146;
    for (int i = 0; i < count; ++i)
    {
        char* block = byte_10296CEE + i * kBlockSize;
        void* ptr = *reinterpret_cast<void**>(block);
        if (ptr != nullptr)
        {
            sub_10056740(ptr);
            count = *dword_10299D64;
            *reinterpret_cast<void**>(block) = nullptr;
        }
    }

    *dword_100ADF88 = g->getValue<int>(0x299608);
    *dword_10299D64 = 0;
    sub_100592C0(byte_10295C60, writeIndex);
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006AD20()
{
    auto* const g = globals_;

    struct GameObject
    {
        void** vtable;
        GameObject* next;
    };
    auto* obj = g->getValue<GameObject*>(0x34FF04);
    for (; obj; obj = obj->next)
    {
        using Fn = void(__thiscall*)(GameObject*);
        Fn fn = reinterpret_cast<Fn>(obj->vtable[1]);
        fn(obj);
    }

    auto sub_1006AEA0 = g->getFn<void(__stdcall)()>(0x6AEA0);
    auto sub_100564F0 = g->getFn<int(__thiscall)(int*, GameData2*)>(0x564F0);
    auto sub_10056530 = g->getFn<int(__thiscall)(int*, GameData2*)>(0x56530);
    auto sub_1006AE80 = g->getFn<void(__thiscall)(int*, int, int, int, int)>(0x6AE80);

    sub_1006AEA0();


    GameData2 gd{};
    gd.x = 0;
    gd.y = 0;
    gd.maxX = 0x7FFFFFFF;
    gd.maxY = 0x7FFFFFFF;
    gd.mask = 16;
    gd.maskValue = 32;


    int* div16Ptr = g->getPtr<int>(0x351728);
    uintptr_t cadObj = g->getValue<uintptr_t>(0x384474);
    auto cad_2B90 = *reinterpret_cast<void(__cdecl**)(int, int, int, int)>(cadObj + 0xAA3C);
    auto cad_2A90 = *reinterpret_cast<void(__cdecl**)(int, int, int, int)>(cadObj + 0xAA40);

    if (sub_100564F0(div16Ptr, &gd))
    {
        do
        {
            if (gd.cellMask == 16)
                cad_2B90(gd.alignX, gd.alignY, gd.allowX, gd.allowY);
            else
                cad_2A90(gd.alignX, gd.alignY, gd.allowX - gd.alignX + 1, gd.allowY - gd.alignY + 1);
        } while (sub_10056530(div16Ptr, &gd));
    }

    int v9[4]{};
    sub_1006AE80(v9, 0, 0, g->getValue<int>(0x34FF10) - 1, g->getValue<int>(0x34FF0C) - 1);

    int x0 = v9[0] >> 4;
    int x1 = v9[2] >> 4;
    int y0 = v9[1] >> 3;
    int y1 = v9[3] >> 3;

    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);

    x1 = std::min(x1, *div16Ptr - 1);
    y1 = std::min(y1, *(div16Ptr + 1) - 1);     // 0x35172C

    for (int row = y0; row <= y1; ++row)
    {
        uint8_t* line = reinterpret_cast<uint8_t*>(div16Ptr + 2) + kRowStrideByteSize * row; // 0x351730
        for (int j = x0; j <= x1; ++j)
        {
            uint8_t val = line[j];
            if (val & 0x40)
                line[j] = (val & 0xBF) | 0x18;
        }
    }
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006AD20_de()
{
    auto* const g = globals_;

    struct GameObject
    {
        void** vtable;
        GameObject* next;
    };
    auto* obj = g->getValue<GameObject*>(0x34FEBC);
    for (; obj; obj = obj->next)
    {
        using Fn = void(__thiscall*)(GameObject*);
        Fn fn = reinterpret_cast<Fn>(obj->vtable[1]);
        fn(obj);
    }

    auto sub_1006AEA0 = g->getFn<void(__stdcall)()>(0x6D8A0);
    auto sub_100564F0 = g->getFn<int(__thiscall)(int*, GameData2*)>(0x58A10);
    auto sub_10056530 = g->getFn<int(__thiscall)(int*, GameData2*)>(0x58A50);
    auto sub_1006AE80 = g->getFn<void(__thiscall)(int*, int, int, int, int)>(0x6D850);

    sub_1006AEA0();


    GameData2 gd{};
    gd.x = 0;
    gd.y = 0;
    gd.maxX = 0x7FFFFFFF;
    gd.maxY = 0x7FFFFFFF;
    gd.mask = 16;
    gd.maskValue = 32;


    int* div16Ptr = g->getPtr<int>(0x351728);
    uintptr_t cadObj = g->getValue<uintptr_t>(0x384474);
    auto cad_2B90 = *reinterpret_cast<void(__cdecl**)(int, int, int, int)>(cadObj + 0xAA3C);
    auto cad_2A90 = *reinterpret_cast<void(__cdecl**)(int, int, int, int)>(cadObj + 0xAA40);

    if (sub_100564F0(div16Ptr, &gd))
    {
        do
        {
            if (gd.cellMask == 16)
                cad_2B90(gd.alignX, gd.alignY, gd.allowX, gd.allowY);
            else
                cad_2A90(gd.alignX, gd.alignY, gd.allowX - gd.alignX + 1, gd.allowY - gd.alignY + 1);
        } while (sub_10056530(div16Ptr, &gd));
    }

    int v9[4]{};
    sub_1006AE80(v9, 0, 0, g->getValue<int>(0x34FF10) - 1, g->getValue<int>(0x34FF0C) - 1);

    int x0 = v9[0] >> 4;
    int x1 = v9[2] >> 4;
    int y0 = v9[1] >> 3;
    int y1 = v9[3] >> 3;

    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);

    x1 = std::min(x1, *div16Ptr - 1);
    y1 = std::min(y1, *(div16Ptr + 1) - 1);     // 0x35172C

    for (int row = y0; row <= y1; ++row)
    {
        uint8_t* line = reinterpret_cast<uint8_t*>(div16Ptr + 2) + kRowStrideByteSize * row; // 0x351730
        for (int j = x0; j <= x1; ++j)
        {
            uint8_t val = line[j];
            if (val & 0x40)
                line[j] = (val & 0xBF) | 0x18;
        }
    }
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006AD20_hd()
{
    auto* const g = globals_;

    struct GameObject
    {
        void** vtable;
        GameObject* next;
    };
    auto* obj = g->getValue<GameObject*>(0x34FF04);
    for (; obj; obj = obj->next)
    {
        using Fn = void(__thiscall*)(GameObject*);
        Fn fn = reinterpret_cast<Fn>(obj->vtable[1]);
        fn(obj);
    }

    auto sub_1006AEA0 = g->getFn<void(__stdcall)()>(0x6AEA0);
    auto sub_100564F0 = g->getFn<int(__thiscall)(int*, GameData2*)>(0x564F0);
    auto sub_10056530 = g->getFn<int(__thiscall)(int*, GameData2*)>(0x56530);
    auto sub_1006AE80 = g->getFn<void(__thiscall)(int*, int, int, int, int)>(0x6AE80);

    sub_1006AEA0();


    GameData2 gd{};
    gd.x = 0;
    gd.y = 0;
    gd.maxX = 0x7FFFFFFF;
    gd.maxY = 0x7FFFFFFF;
    gd.mask = 16;
    gd.maskValue = 32;


    int* div16Ptr = g->getPtr<int>(0x3AD000);
    uintptr_t cadObj = g->getValue<uintptr_t>(0x384474);
    auto cad_2B90 = *reinterpret_cast<void(__cdecl**)(int, int, int, int)>(cadObj + 0xAA3C);
    auto cad_2A90 = *reinterpret_cast<void(__cdecl**)(int, int, int, int)>(cadObj + 0xAA40);

    if (sub_100564F0(div16Ptr, &gd))
    {
        do
        {
            if (gd.cellMask == 16)
                cad_2B90(gd.alignX, gd.alignY, gd.allowX, gd.allowY);
            else
                cad_2A90(gd.alignX, gd.alignY, gd.allowX - gd.alignX + 1, gd.allowY - gd.alignY + 1);
        } while (sub_10056530(div16Ptr, &gd));
    }

    int v9[4]{};
    sub_1006AE80(v9, 0, 0, g->getValue<int>(0x34FF10) - 1, g->getValue<int>(0x34FF0C) - 1);

    int x0 = v9[0] >> 4;
    int x1 = v9[2] >> 4;
    int y0 = v9[1] >> 3;
    int y1 = v9[3] >> 3;

    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);

    x1 = std::min(x1, *div16Ptr - 1);
    y1 = std::min(y1, *(div16Ptr + 1) - 1);     // 0x35172C

    for (int row = y0; row <= y1; ++row)
    {
        uint8_t* line = reinterpret_cast<uint8_t*>(div16Ptr + 2) + kRowStrideByteSize * row; // 0x351730
        for (int j = x0; j <= x1; ++j)
        {
            uint8_t val = line[j];
            if (val & 0x40)
                line[j] = (val & 0xBF) | 0x18;
        }
    }
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006AEA0()
{
    auto* const g = globals_;

    auto sub_10071290 = g->getFn<int(__cdecl)(int*)>(0x71290);
    auto sub_100564F0 = g->getFn<int(__thiscall)(int*, GameData2*)>(0x564F0);
    auto sub_10056530 = g->getFn<int(__thiscall)(int*, GameData2*)>(0x56530);
    auto sub_10071310 = g->getFn<void(__cdecl)(int*)>(0x71310);
    auto sub_10055E00 = g->getFn<void(__thiscall)(int*, int, int, int, int, int)>(0x55E00);
    auto sub_1006B070 = g->getFn<void(__thiscall)(int*, int*)>(0x6B070);

    int v13[4];

    if (!sub_10071290(v13))
        return;

    int x, y, maxX, maxY;
    x = v13[0] & 0xFFFFFFF0;
    y = v13[1] & 0xFFFFFFF8;
    maxX = (v13[2] & 0xFFFFFFF0) + 15;
    maxY = (v13[3] & 0xFFFFFFF8) + 7;

    bool isNegative = v13[0] < 0;
    v13[0] = x;
    v13[1] = y;
    v13[2] = maxX;
    v13[3] = maxY;

    if (isNegative)
        x = v13[0] = 0;
    if (y < 0)
        y = v13[1] = 0;

    maxX = std::min(maxX, g->getValue<int>(0x34FF10) - 1);
    v13[2] = maxX;
    maxY = std::min(maxY, g->getValue<int>(0x34FF0C) - 1);
    v13[3] = maxY;

    GameData2 gd{};
    gd.x = x >> 4;
    gd.y = y >> 3;
    gd.maxX = (x >> 4) + ((maxX - x + 1) >> 4);
    gd.maxY = (y >> 3) + ((maxY - y + 1) >> 3);
    gd.mask = 16;
    gd.maskValue = 32;

    int* div16Ptr = g->getPtr<int>(0x351728);
    uintptr_t cadObj = g->getValue<uintptr_t>(0x384474);
    auto cad_2FB0 = *reinterpret_cast<void(__cdecl**)(int, int, int, int)>(cadObj + 0xA9E8);

    if (sub_100564F0(div16Ptr, &gd))
    {
        do
        {
            if (gd.cellMask == 16)
                cad_2FB0(gd.alignX, gd.alignY, gd.allowX, gd.allowY);

            sub_10071310(&gd.alignX);
            sub_10055E00(div16Ptr, 128, gd.alignX, gd.alignY, gd.allowX, gd.allowY);
        } while (sub_10056530(div16Ptr, &gd));
    }

    int v14[4];
    sub_1006B070(v14, v13);

    int x0 = v14[0] >> 4;
    int x1 = v14[2] >> 4;
    int y0 = v14[1] >> 3;
    int y1 = v14[3] >> 3;

    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);

    x1 = std::min(x1, *div16Ptr - 1);
    y1 = std::min(y1, *(div16Ptr + 1) - 1);    // 0x35172C

    for (int row = y0; row <= y1; ++row)
    {
        uint8_t* line = reinterpret_cast<uint8_t*>(div16Ptr + 2) + kRowStrideByteSize * row; // 0x351730
        for (int j = x0; j <= x1; ++j)
        {
            uint8_t val = line[j];
            if (static_cast<int8_t>(val) < 0)
                line[j] = (val & 0x5D) | 0x22;
        }
    }
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006AEA0_hd()
{
    auto* const g = globals_;

    auto sub_10071290 = g->getFn<int(__cdecl)(int*)>(0x71290);
    auto sub_100564F0 = g->getFn<int(__thiscall)(int*, GameData2*)>(0x564F0);
    auto sub_10056530 = g->getFn<int(__thiscall)(int*, GameData2*)>(0x56530);
    auto sub_10071310 = g->getFn<void(__cdecl)(int*)>(0x71310);
    auto sub_10055E00 = g->getFn<void(__thiscall)(int*, int, int, int, int, int)>(0x55E00);
    auto sub_1006B070 = g->getFn<void(__thiscall)(int*, int*)>(0x6B070);

    int v13[4];

    if (!sub_10071290(v13))
        return;

    int x, y, maxX, maxY;
    x = v13[0] & 0xFFFFFFF0;
    y = v13[1] & 0xFFFFFFF8;
    maxX = (v13[2] & 0xFFFFFFF0) + 15;
    maxY = (v13[3] & 0xFFFFFFF8) + 7;

    bool isNegative = v13[0] < 0;
    v13[0] = x;
    v13[1] = y;
    v13[2] = maxX;
    v13[3] = maxY;

    if (isNegative)
        x = v13[0] = 0;
    if (y < 0)
        y = v13[1] = 0;

    maxX = std::min(maxX, g->getValue<int>(0x34FF10) - 1);
    v13[2] = maxX;
    maxY = std::min(maxY, g->getValue<int>(0x34FF0C) - 1);
    v13[3] = maxY;

    GameData2 gd{};
    gd.x = x >> 4;
    gd.y = y >> 3;
    gd.maxX = (x >> 4) + ((maxX - x + 1) >> 4);
    gd.maxY = (y >> 3) + ((maxY - y + 1) >> 3);
    gd.mask = 16;
    gd.maskValue = 32;

    int* div16Ptr = g->getPtr<int>(0x3AD000);
    uintptr_t cadObj = g->getValue<uintptr_t>(0x384474);
    auto cad_2FB0 = *reinterpret_cast<void(__cdecl**)(int, int, int, int)>(cadObj + 0xA9E8);

    if (sub_100564F0(div16Ptr, &gd))
    {
        do
        {
            if (gd.cellMask == 16)
                cad_2FB0(gd.alignX, gd.alignY, gd.allowX, gd.allowY);

            sub_10071310(&gd.alignX);
            sub_10055E00(div16Ptr, 128, gd.alignX, gd.alignY, gd.allowX, gd.allowY);
        } while (sub_10056530(div16Ptr, &gd));
    }

    int v14[4];
    sub_1006B070(v14, v13);

    int x0 = v14[0] >> 4;
    int x1 = v14[2] >> 4;
    int y0 = v14[1] >> 3;
    int y1 = v14[3] >> 3;

    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);

    x1 = std::min(x1, *div16Ptr - 1);
    y1 = std::min(y1, *(div16Ptr + 1) - 1);    // 0x35172C

    for (int row = y0; row <= y1; ++row)
    {
        uint8_t* line = reinterpret_cast<uint8_t*>(div16Ptr + 2) + kRowStrideByteSize * row; // 0x351730
        for (int j = x0; j <= x1; ++j)
        {
            uint8_t val = line[j];
            if (static_cast<int8_t>(val) < 0)
                line[j] = (val & 0x5D) | 0x22;
        }
    }
}

void __declspec(noinline) __cdecl    GameDllHooks::sub_1006B1C0(char mask, int* a2)
{
    auto* const g = globals_;

    auto sub_10055E00 = g->getFn<void(__thiscall)(int*, int, int, int, int, int)>(0x55E00);
    auto sub_1006B550 = g->getFn<int* (__cdecl)(int)>(0x6B550);
    auto sub_10055F40 = g->getFn<void(__thiscall)(int*, int, int, int, int, int)>(0x55F40);
    auto sub_1006B070 = g->getFn<void(__thiscall)(int*, int*)>(0x6B070);

    int* div16Ptr = g->getPtr<int>(0x351728);

    sub_10055E00(div16Ptr, 128, a2[0], a2[1], a2[2], a2[3]);

    int* node = sub_1006B550(g->getValue<int>(0x37E948));
    while (node)
    {
        sub_10055F40(div16Ptr, 128, node[3], node[4], node[5], node[6]);
        node = reinterpret_cast<int*>(node[1]);
    }

    int v11[4];
    sub_1006B070(v11, a2);

    int x0 = v11[0] >> 4;
    int x1 = v11[2] >> 4;
    int y0 = v11[1] >> 3;
    int y1 = v11[3] >> 3;

    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);

    x1 = std::min(x1, *div16Ptr - 1);
    y1 = std::min(y1, *(div16Ptr + 1) - 1);

    for (int row = y0; row <= y1; ++row)
    {
        uint8_t* line = reinterpret_cast<uint8_t*>(div16Ptr + 2) + kRowStrideByteSize * row; // 0x351730
        for (int j = x0; j <= x1; ++j)
        {
            uint8_t val = line[j];
            if (static_cast<int8_t>(val) < 0)
                line[j] = (val & 0x7F) | mask;
        }
    }
}

void __declspec(noinline) __cdecl    GameDllHooks::sub_1006B1C0_hd(char mask, int* a2)
{
    auto* const g = globals_;

    auto sub_10055E00 = g->getFn<void(__thiscall)(int*, int, int, int, int, int)>(0x55E00);
    auto sub_1006B550 = g->getFn<int* (__cdecl)(int)>(0x6B550);
    auto sub_10055F40 = g->getFn<void(__thiscall)(int*, int, int, int, int, int)>(0x55F40);
    auto sub_1006B070 = g->getFn<void(__thiscall)(int*, int*)>(0x6B070);

    int* div16Ptr = g->getPtr<int>(0x3AD000);

    sub_10055E00(div16Ptr, 128, a2[0], a2[1], a2[2], a2[3]);

    int* node = sub_1006B550(g->getValue<int>(0x37E948));
    while (node)
    {
        sub_10055F40(div16Ptr, 128, node[3], node[4], node[5], node[6]);
        node = reinterpret_cast<int*>(node[1]);
    }

    int v11[4];
    sub_1006B070(v11, a2);

    int x0 = v11[0] >> 4;
    int x1 = v11[2] >> 4;
    int y0 = v11[1] >> 3;
    int y1 = v11[3] >> 3;

    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);

    x1 = std::min(x1, *div16Ptr - 1);
    y1 = std::min(y1, *(div16Ptr + 1) - 1);

    for (int row = y0; row <= y1; ++row)
    {
        uint8_t* line = reinterpret_cast<uint8_t*>(div16Ptr + 2) + kRowStrideByteSize * row; // 0x351730
        for (int j = x0; j <= x1; ++j)
        {
            uint8_t val = line[j];
            if (static_cast<int8_t>(val) < 0)
                line[j] = (val & 0x7F) | mask;
        }
    }
}

char __declspec(noinline) __cdecl    GameDllHooks::sub_1006B2C0(char mask, int* a2, int a3)
{
    auto* const g = globals_;

    auto sub_10055E00 = g->getFn<void(__thiscall)(int*, int, int, int, int, int)>(0x55E00);
    auto sub_1006B550 = g->getFn<int* (__cdecl)(int)>(0x6B550);
    auto sub_10055F40 = g->getFn<void(__thiscall)(int*, int, int, int, int, int)>(0x55F40);
    auto sub_1006B070 = g->getFn<void(__thiscall)(int*, int*)>(0x6B070);

    int* div16Ptr = g->getPtr<int>(0x34FF20);

    sub_10055E00(div16Ptr, 128, a2[0], a2[1], a2[2], a2[3]);

    int* node = sub_1006B550(a3);
    while (node)
    {
        sub_10055F40(div16Ptr, 128, node[3], node[4], node[5], node[6]);
        node = reinterpret_cast<int*>(node[1]);
    }

    int v12[4];
    sub_1006B070(v12, a2);

    int x0 = v12[0] >> 4;
    int x1 = v12[2] >> 4;
    int y0 = v12[1] >> 3;
    int y1 = v12[3] >> 3;

    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);

    x1 = std::min(x1, *div16Ptr - 1);
    y1 = std::min(y1, *(div16Ptr + 1) - 1);

    uint8_t val = static_cast<uint8_t>(y1);
    for (int row = y0; row <= y1; ++row)
    {
        uint8_t* line = reinterpret_cast<uint8_t*>(div16Ptr + 2) + kRowStrideByteSize * row; // 0x34FF28
        for (int j = x0; j <= x1; ++j)
        {
            val = line[j];
            if (static_cast<int8_t>(val) < 0)
            {
                val = (val & 0x7F) | mask;
                line[j] = val;
            }
        }
    }

    return val;
}

char __declspec(noinline) __cdecl    GameDllHooks::sub_1006B2C0_hd(char mask, int* a2, int a3)
{
    auto* const g = globals_;

    auto sub_10055E00 = g->getFn<void(__thiscall)(int*, int, int, int, int, int)>(0x55E00);
    auto sub_1006B550 = g->getFn<int* (__cdecl)(int)>(0x6B550);
    auto sub_10055F40 = g->getFn<void(__thiscall)(int*, int, int, int, int, int)>(0x55F40);
    auto sub_1006B070 = g->getFn<void(__thiscall)(int*, int*)>(0x6B070);

    int* div16Ptr = g->getPtr<int>(0x3A8000);

    sub_10055E00(div16Ptr, 128, a2[0], a2[1], a2[2], a2[3]);

    int* node = sub_1006B550(a3);
    while (node)
    {
        sub_10055F40(div16Ptr, 128, node[3], node[4], node[5], node[6]);
        node = reinterpret_cast<int*>(node[1]);
    }

    int v12[4];
    sub_1006B070(v12, a2);

    int x0 = v12[0] >> 4;
    int x1 = v12[2] >> 4;
    int y0 = v12[1] >> 3;
    int y1 = v12[3] >> 3;

    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);

    x1 = std::min(x1, *div16Ptr - 1);
    y1 = std::min(y1, *(div16Ptr + 1) - 1);

    uint8_t val = static_cast<uint8_t>(y1);
    for (int row = y0; row <= y1; ++row)
    {
        uint8_t* line = reinterpret_cast<uint8_t*>(div16Ptr + 2) + kRowStrideByteSize * row; // 0x34FF28
        for (int j = x0; j <= x1; ++j)
        {
            val = line[j];
            if (static_cast<int8_t>(val) < 0)
            {
                val = (val & 0x7F) | mask;
                line[j] = val;
            }
        }
    }

    return val;
}

void __declspec(noinline) __fastcall GameDllHooks::sub_1006CC60(GameData3* self)
{
    if (!self->param_00)
        return;

    auto* const g = globals_;

    int* g_1037EFE0 = g->getValue<int*>(0x37EFE0);   // get value at 0x37EFE0 as int*
    int16_t* srcRect = reinterpret_cast<int16_t*>(g_1037EFE0[1]);
    if (!srcRect)    // self is always not nullptr
        return;

    Rect dstRect;
    dstRect.x = srcRect[0];
    dstRect.y = srcRect[1];
    dstRect.width = srcRect[2] + srcRect[0] - 1;
    const int y2 = srcRect[3] + srcRect[1] - 1;

    const int verticalOffset = g->getValue<int>(0x34FF0C) - y2 + dstRect.y - 1;
    dstRect.y += verticalOffset;
    dstRect.height = y2 + verticalOffset;

    auto funcArray = reinterpret_cast<void(__thiscall**)(GameData3*, int, Rect*, int, int, uint32_t)>(self->param_00);
    auto func19 = funcArray[19];
    func19(self, 0x50414E4C, &dstRect, 10, -1, 0);

    int* g_10353030 = g->getPtr<int>(0x353030);
    int* g_1037EDE0 = g->getPtr<int>(0x37EDE0);
    int* g_10295434 = g->getValue<int*>(0x295434);

    auto sub_10071850 = g->getFn<void(__thiscall)(GameData3*, int)>(0x71850);
    auto sub_1006BC50 = g->getFn<void(__thiscall)(int*, GameData3*, int, int)>(0x6BC50);
    auto sub_10072980 = g->getFn<void(__thiscall)(GameData3*, int*, int, int, int)>(0x72980);
    auto sub_100718D0 = g->getFn<GameData4 * (__thiscall)(GameData3*, int, int, int, int, int*, int, int*, int*, int)>(0x718D0);
    auto sub_10071FD0 = g->getFn<void(__thiscall)(uint32_t*, uint32_t, uint32_t, uint16_t*, int*)>(0x71FD0);
    auto sub_1006B410 = g->getFn<void(__cdecl)(uint32_t*, int)>(0x6B410);

    sub_10071850(self, *g_1037EFE0);
    sub_1006BC50(g_10353030, self, self->param_60[40], self->param_60[41]);
    sub_10072980(self, g_10353030, 20, 100, 0);
    GameData4* gd4 = sub_100718D0(self, 2, 1, 2, 2, &g_1037EFE0[17], g_1037EFE0[17], g_1037EDE0, g_10295434, 24);
    gd4->param_0D = 10; // if gd4 was not allocated, it will crash in this or another function

    sub_10071FD0(&self->param_64, self->param_0C, self->param_10, reinterpret_cast<uint16_t*>(g_1037EFE0[2]), g_1037EDE0);
    sub_10071FD0(&self->param_94, self->param_0C, self->param_10, reinterpret_cast<uint16_t*>(g_1037EFE0[3]), g_1037EDE0);
    sub_1006B410(&self->param_64, 11);
    sub_1006B410(&self->param_94, 12);
}

void __declspec(noinline) __fastcall GameDllHooks::sub_1006CC60_de(GameData3* self)
{
    if (!self->param_00)
        return;

    auto* const g = globals_;

    int* g_1037EFE0 = g->getValue<int*>(0x37EFA0);   // get value at 0x37EFE0 as int*
    int16_t* srcRect = reinterpret_cast<int16_t*>(g_1037EFE0[1]);
    if (!srcRect)    // self is always not nullptr
        return;

    Rect dstRect;
    dstRect.x = srcRect[0];
    dstRect.y = srcRect[1];
    dstRect.width = srcRect[2] + srcRect[0] - 1;
    const int y2 = srcRect[3] + srcRect[1] - 1;

    const int verticalOffset = g->getValue<int>(0x34FEC4) - y2 + dstRect.y - 1;
    dstRect.y += verticalOffset;
    dstRect.height = y2 + verticalOffset;

    auto funcArray = reinterpret_cast<void(__thiscall**)(GameData3*, int, Rect*, int, int, uint32_t)>(self->param_00);
    auto func19 = funcArray[19];
    func19(self, 0x50414E4C, &dstRect, 10, -1, 0);

    int* g_10353030 = g->getPtr<int>(0x352FE8);
    int* g_1037EDE0 = g->getPtr<int>(0x37EDA0);
    int* g_10295434 = g->getValue<int*>(0x295404);

    auto sub_10071850 = g->getFn<void(__thiscall)(GameData3*, int)>(0x74450);
    auto sub_1006BC50 = g->getFn<void(__thiscall)(int*, GameData3*, int, int)>(0x6E540);
    auto sub_10072980 = g->getFn<void(__thiscall)(GameData3*, int*, int, int, int)>(0x75560);
    auto sub_100718D0 = g->getFn<GameData4 * (__thiscall)(GameData3*, int, int, int, int, int*, int, int*, int*, int)>(0x744D0);
    auto sub_10071FD0 = g->getFn<void(__thiscall)(uint32_t*, uint32_t, uint32_t, uint16_t*, int*)>(0x74BB0);
    auto sub_1006B410 = g->getFn<void(__cdecl)(uint32_t*, int)>(0x6DD10);

    sub_10071850(self, *g_1037EFE0);
    sub_1006BC50(g_10353030, self, self->param_60[40], self->param_60[41]);
    sub_10072980(self, g_10353030, 20, 100, 0);
    GameData4* gd4 = sub_100718D0(self, 2, 1, 2, 2, &g_1037EFE0[17], g_1037EFE0[17], g_1037EDE0, g_10295434, 24);
    gd4->param_0D = 10; // if gd4 was not allocated, it will crash in this or another function

    sub_10071FD0(&self->param_64, self->param_0C, self->param_10, reinterpret_cast<uint16_t*>(g_1037EFE0[2]), g_1037EDE0);
    sub_10071FD0(&self->param_94, self->param_0C, self->param_10, reinterpret_cast<uint16_t*>(g_1037EFE0[3]), g_1037EDE0);
    sub_1006B410(&self->param_64, 11);
    sub_1006B410(&self->param_94, 12);
}

void __declspec(noinline) __fastcall GameDllHooks::sub_1006CC60_fr(GameData3* self)
{
    if (!self->param_00)
        return;

    auto* const g = globals_;

    int* g_1037EFE0 = g->getValue<int*>(0x382FC0);   // get value at 0x37EFE0 as int*
    int16_t* srcRect = reinterpret_cast<int16_t*>(g_1037EFE0[1]);
    if (!srcRect)    // self is always not nullptr
        return;

    Rect dstRect;
    dstRect.x = srcRect[0];
    dstRect.y = srcRect[1];
    dstRect.width = srcRect[2] + srcRect[0] - 1;
    const int y2 = srcRect[3] + srcRect[1] - 1;

    const int verticalOffset = g->getValue<int>(0x353EE4) - y2 + dstRect.y - 1;
    dstRect.y += verticalOffset;
    dstRect.height = y2 + verticalOffset;

    auto funcArray = reinterpret_cast<void(__thiscall**)(GameData3*, int, Rect*, int, int, uint32_t)>(self->param_00);
    auto func19 = funcArray[19];
    func19(self, 0x50414E4C, &dstRect, 10, -1, 0);

    int* g_10353030 = g->getPtr<int>(0x357008);
    int* g_1037EDE0 = g->getPtr<int>(0x382DC0);
    int* g_10295434 = g->getValue<int*>(0x299424);

    auto sub_10071850 = g->getFn<void(__thiscall)(GameData3*, int)>(0x74470);
    auto sub_1006BC50 = g->getFn<void(__thiscall)(int*, GameData3*, int, int)>(0x6E5C0);
    auto sub_10072980 = g->getFn<void(__thiscall)(GameData3*, int*, int, int, int)>(0x755A0);
    auto sub_100718D0 = g->getFn<GameData4 * (__thiscall)(GameData3*, int, int, int, int, int*, int, int*, int*, int)>(0x74500);
    auto sub_10071FD0 = g->getFn<void(__thiscall)(uint32_t*, uint32_t, uint32_t, uint16_t*, int*)>(0x74BE0);
    auto sub_1006B410 = g->getFn<void(__cdecl)(uint32_t*, int)>(0x6DD90);

    sub_10071850(self, *g_1037EFE0);
    sub_1006BC50(g_10353030, self, self->param_60[40], self->param_60[41]);
    sub_10072980(self, g_10353030, 20, 100, 0);
    GameData4* gd4 = sub_100718D0(self, 2, 1, 2, 2, &g_1037EFE0[17], g_1037EFE0[17], g_1037EDE0, g_10295434, 24);
    gd4->param_0D = 10; // if gd4 was not allocated, it will crash in this or another function

    sub_10071FD0(&self->param_64, self->param_0C, self->param_10, reinterpret_cast<uint16_t*>(g_1037EFE0[2]), g_1037EDE0);
    sub_10071FD0(&self->param_94, self->param_0C, self->param_10, reinterpret_cast<uint16_t*>(g_1037EFE0[3]), g_1037EDE0);
    sub_1006B410(&self->param_64, 11);
    sub_1006B410(&self->param_94, 12);
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006D940()
{
    auto* const g = globals_;

    auto sub_1006AE80 = g->getFn<void(__thiscall)(int*, int, int, int, int)>(0x6AE80);
    auto sub_100562F0 = g->getFn<int(__thiscall)(int*, GameData*)>(0x562F0);
    auto sub_10056330 = g->getFn<int(__thiscall)(int*, GameData*)>(0x56330);
    auto sub_1006D2E0 = g->getFn<void()>(0x6D2E0);
    auto sub_1006D300 = g->getFn<void()>(0x6D300);
    auto sub_1006D4D0 = g->getFn<void()>(0x6D4D0);
    auto sub_10049EC0 = g->getFn<void(__cdecl)(int, int, int, int, void*)>(0x49EC0);
    auto sub_10049FF0 = g->getFn<void(__cdecl)(int, int, int, int, void*)>(0x49FF0);
    auto sub_1006D810 = g->getFn<int(__cdecl)(int, int, int, int)>(0x6D810);
    auto sub_1006D8D0 = g->getFn<int(__cdecl)(int, int, int, int)>(0x6D8D0);
    auto sub_1004DDC0 = g->getFn<void()>(0x4DDC0);


    ModuleStateLong* const cadPtr = reinterpret_cast<ModuleStateLong*>(g->getValue<uintptr_t>(0x384474) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites)));
    auto resetStencilSurface = cadPtr->actionsPostfix.resetStencilSurface;

    uint16_t paletteWord = g->getValue<uint16_t>(0x383EFE);

    uint32_t low = 0;
    low |= (cadPtr->actualRedMask & (static_cast<uint32_t>((paletteWord & 0xF800u) >> cadPtr->redOffset)));
    low |= (cadPtr->actualBlueMask & (static_cast<uint32_t>(((paletteWord & 0x001Fu) << 11) >> static_cast<uint8_t>(cadPtr->blueOffset))));
    low |= (cadPtr->actualGreenMask & (static_cast<uint32_t>(((paletteWord & 0x07E0u) << 5) >> static_cast<uint8_t>(cadPtr->greenOffset))));

    cadPtr->backSurfaceShadePixel = (low & 0xFFFFu) | ((low & 0xFFFFu) << 16);


    int v9[4];
    const int xRight = g->getValue<int>(0x37E91C);
    const int yBottom = g->getValue<int>(0x37E918);
    sub_1006AE80(v9, 0, 0, xRight - 1, yBottom - 1);

    int* const div16Ptr = g->getPtr<int>(0x351728);

    int x0 = v9[0] >> 4;
    int x1 = v9[2] >> 4;
    int y0 = v9[1] >> 3;
    int y1 = v9[3] >> 3;

    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);

    x1 = std::min(x1, *div16Ptr - 1);
    y1 = std::min(y1, *(div16Ptr + 1) - 1);


    for (int row = y0; row <= y1; ++row)
    {
        uint8_t* line = reinterpret_cast<uint8_t*>(div16Ptr + 2) + kRowStrideByteSize * row; // 0x351730
        for (int j = x0; j <= x1; ++j)
        {
            uint8_t val = line[j];
            if ((val & 1) != 0)
                line[j] = static_cast<uint8_t>((val & 0xFB) | 0x1A);
        }
    }


    GameData gd{};
    gd.x = 0;
    gd.y = 0;
    gd.maxX = 0x7FFFFFFF;
    gd.maxY = 0x7FFFFFFF;
    gd.mask = 1;
    gd.maskValue = static_cast<uint8_t>(-2);

    if (sub_100562F0(div16Ptr, &gd))
    {
        *g->getPtr<int>(0x37B16C) = g->getValue<int>(0x37E924);
        *g->getPtr<int>(0x37B174) = g->getValue<int>(0x37E924) + xRight - 1;
        *g->getPtr<int>(0x37B170) = g->getValue<int>(0x37E920);
        *g->getPtr<int>(0x37B178) = g->getValue<int>(0x37E920) + yBottom - 1;

        sub_1004DDC0();
        sub_1006D2E0();

        do
        {
            cadPtr->windowRect.x = gd.alignX;
            cadPtr->windowRect.y = gd.alignY;
            cadPtr->windowRect.width = gd.allowX;
            cadPtr->windowRect.height = gd.allowY;

            resetStencilSurface();

            sub_10049EC0(
                gd.alignX + g->getValue<int>(0x37E924),
                gd.alignY + g->getValue<int>(0x37E920),
                g->getValue<int>(0x37E924) + gd.allowX,
                g->getValue<int>(0x37E920) + gd.allowY,
                sub_1006D810);

            sub_1006D300();

            sub_10049FF0(
                gd.alignX + g->getValue<int>(0x37E924),
                gd.alignY + g->getValue<int>(0x37E920),
                g->getValue<int>(0x37E924) + gd.allowX,
                g->getValue<int>(0x37E920) + gd.allowY,
                sub_1006D8D0);

            sub_1006D4D0();
        } while (sub_10056330(div16Ptr, &gd));
    }
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006D940_hd()
{
    auto* const g = globals_;

    auto sub_1006AE80 = g->getFn<void(__thiscall)(int*, int, int, int, int)>(0x6AE80);
    auto sub_100562F0 = g->getFn<int(__thiscall)(int*, GameData*)>(0x562F0);
    auto sub_10056330 = g->getFn<int(__thiscall)(int*, GameData*)>(0x56330);
    auto sub_1006D2E0 = g->getFn<void()>(0x6D2E0);
    auto sub_1006D300 = g->getFn<void()>(0x6D300);
    auto sub_1006D4D0 = g->getFn<void()>(0x6D4D0);
    auto sub_10049EC0 = g->getFn<void(__cdecl)(int, int, int, int, void*)>(0x49EC0);
    auto sub_10049FF0 = g->getFn<void(__cdecl)(int, int, int, int, void*)>(0x49FF0);
    auto sub_1006D810 = g->getFn<int(__cdecl)(int, int, int, int)>(0x6D810);
    auto sub_1006D8D0 = g->getFn<int(__cdecl)(int, int, int, int)>(0x6D8D0);
    auto sub_1004DDC0 = g->getFn<void()>(0x4DDC0);


    ModuleStateLong* const cadPtr = reinterpret_cast<ModuleStateLong*>(g->getValue<uintptr_t>(0x384474) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites)));
    auto resetStencilSurface = cadPtr->actionsPostfix.resetStencilSurface;

    uint16_t paletteWord = g->getValue<uint16_t>(0x383EFE);

    uint32_t low = 0;
    low |= (cadPtr->actualRedMask & (static_cast<uint32_t>((paletteWord & 0xF800u) >> cadPtr->redOffset)));
    low |= (cadPtr->actualBlueMask & (static_cast<uint32_t>(((paletteWord & 0x001Fu) << 11) >> static_cast<uint8_t>(cadPtr->blueOffset))));
    low |= (cadPtr->actualGreenMask & (static_cast<uint32_t>(((paletteWord & 0x07E0u) << 5) >> static_cast<uint8_t>(cadPtr->greenOffset))));

    cadPtr->backSurfaceShadePixel = (low & 0xFFFFu) | ((low & 0xFFFFu) << 16);


    int v9[4];
    const int xRight = g->getValue<int>(0x37E91C);
    const int yBottom = g->getValue<int>(0x37E918);
    sub_1006AE80(v9, 0, 0, xRight - 1, yBottom - 1);

    int* const div16Ptr = g->getPtr<int>(0x3AD000);

    int x0 = v9[0] >> 4;
    int x1 = v9[2] >> 4;
    int y0 = v9[1] >> 3;
    int y1 = v9[3] >> 3;

    x0 = std::max(x0, 0);
    y0 = std::max(y0, 0);

    x1 = std::min(x1, *div16Ptr - 1);
    y1 = std::min(y1, *(div16Ptr + 1) - 1);


    for (int row = y0; row <= y1; ++row)
    {
        uint8_t* line = reinterpret_cast<uint8_t*>(div16Ptr + 2) + kRowStrideByteSize * row; // 0x351730
        for (int j = x0; j <= x1; ++j)
        {
            uint8_t val = line[j];
            if ((val & 1) != 0)
                line[j] = static_cast<uint8_t>((val & 0xFB) | 0x1A);
        }
    }


    GameData gd{};
    gd.x = 0;
    gd.y = 0;
    gd.maxX = 0x7FFFFFFF;
    gd.maxY = 0x7FFFFFFF;
    gd.mask = 1;
    gd.maskValue = static_cast<uint8_t>(-2);

    if (sub_100562F0(div16Ptr, &gd))
    {
        *g->getPtr<int>(0x37B16C) = g->getValue<int>(0x37E924);
        *g->getPtr<int>(0x37B174) = g->getValue<int>(0x37E924) + xRight - 1;
        *g->getPtr<int>(0x37B170) = g->getValue<int>(0x37E920);
        *g->getPtr<int>(0x37B178) = g->getValue<int>(0x37E920) + yBottom - 1;

        sub_1004DDC0();
        sub_1006D2E0();

        do
        {
            cadPtr->windowRect.x = gd.alignX;
            cadPtr->windowRect.y = gd.alignY;
            cadPtr->windowRect.width = gd.allowX;
            cadPtr->windowRect.height = gd.allowY;

            resetStencilSurface();

            sub_10049EC0(
                gd.alignX + g->getValue<int>(0x37E924),
                gd.alignY + g->getValue<int>(0x37E920),
                g->getValue<int>(0x37E924) + gd.allowX,
                g->getValue<int>(0x37E920) + gd.allowY,
                sub_1006D810);

            sub_1006D300();

            sub_10049FF0(
                gd.alignX + g->getValue<int>(0x37E924),
                gd.alignY + g->getValue<int>(0x37E920),
                g->getValue<int>(0x37E924) + gd.allowX,
                g->getValue<int>(0x37E920) + gd.allowY,
                sub_1006D8D0);

            sub_1006D4D0();
        } while (sub_10056330(div16Ptr, &gd));
    }
}

void __declspec(noinline) __fastcall GameDllHooks::sub_1006DC40(int* self, void* /*dummy*/, int a2, int a3, int a4, int a5, uint8_t a6, char a7, char a8)
{
    int v8 = a2 >> 4;
    int v9 = a4 >> 4;
    int v10 = a3 >> 3;
    int v11 = a5 >> 3;
    int* v13;

    if (a2 >> 4 < 0)
        v8 = 0;
    if (v10 < 0)
        v10 = 0;
    if (v9 >= self[0])
        v9 = self[0] - 1;
    if (v11 >= self[1])
        v11 = self[1] - 1;
    if (v10 <= v11)
    {
        v13 = &self[(v10 << (kRowStrideShift - 2)) + 2];
        int a2a = v11 - v10 + 1;
        do
        {
            for (int i = v8; i <= v9; ++i)
            {
                const uint8_t v15 = *((uint8_t*)v13 + i);
                if ((v15 & a6) != 0)
                    *((uint8_t*)v13 + i) = a8 | a7 & v15;
            }
            v13 += kRowStrideByteSize / sizeof(v13);
            --a2a;
        } while (a2a);
    }
}


void __declspec(noinline) __stdcall  GameDllHooks::sub_1006F120()
{
    auto* const g = globals_;

    FogDrawData data
    {
        g->getPtr<uint8_t>(0x1C13FE),
        g->getValue<int>(0xC14EC),
        g->getValue<int>(0xC14F0),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x384474) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites))),

        g->getValue<int>(0x37E920),
        g->getValue<int>(0x37E924),
        g->getValue<int>(0x37E91C),
        g->getValue<int>(0x37E918),

        g->getPtr<int>(0x351728),
        g->getValue<uint8_t>(0x383CB9),
        g->getPtr<uint8_t>(0x37C598)
    };

    drawFogOnWorld(data);
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006F120_de()
{
    auto* const g = globals_;

    FogDrawData data
    {
        g->getPtr<uint8_t>(0x1C13C6),
        g->getValue<int>(0xC14B4),
        g->getValue<int>(0xC14B8),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x38446C) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites))),

        g->getValue<int>(0x37E8E0),
        g->getValue<int>(0x37E8E4),
        g->getValue<int>(0x37E8DC),
        g->getValue<int>(0x37E8D8),

        g->getPtr<int>(0x3516E0),
        g->getValue<uint8_t>(0x383C8D),
        g->getPtr<uint8_t>(0x37C554)
    };

    drawFogOnWorld(data);
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006F120_fr()
{
    auto* const g = globals_;

    FogDrawData data
    {
        g->getPtr<uint8_t>(0x1C53E6),
        g->getValue<int>(0xC54D4),
        g->getValue<int>(0xC54D8),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x384598) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites))),

        g->getValue<int>(0x382900),
        g->getValue<int>(0x382904),
        g->getValue<int>(0x3828FC),
        g->getValue<int>(0x3828F8),

        g->getPtr<int>(0x355700),
        g->getValue<uint8_t>(0x387CAD),
        g->getPtr<uint8_t>(0x380574)
    };

    drawFogOnWorld(data);
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006F120_hd_v1_2()
{
    auto* const g = globals_;

    FogDrawData data
    {
        g->getPtr<uint8_t>(0x1C13FE),
        g->getValue<int>(0xC14EC),
        g->getValue<int>(0xC14F0),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x384474) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites))),

        g->getValue<int>(0x37E920),
        g->getValue<int>(0x37E924),
        g->getValue<int>(0x37E91C),
        g->getValue<int>(0x37E918),

        g->getPtr<int>(0x3AD000),
        g->getValue<uint8_t>(0x383CB9),
        g->getPtr<uint8_t>(0x3B2002)
    };

    drawFogOnWorld(data);
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006F120_v1_0_ru()
{
    auto* const g = globals_;

    FogDrawData data
    {
        g->getPtr<uint8_t>(0x1AEF4E),
        g->getValue<int>(0xAF03C),
        g->getValue<int>(0xAF040),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x370EE4) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites))),

        g->getValue<int>(0x36C020),
        g->getValue<int>(0x36C024),
        g->getValue<int>(0x36C01C),
        g->getValue<int>(0x36C018),

        g->getPtr<int>(0x33EE20),
        g->getValue<uint8_t>(0x37099D),
        g->getPtr<uint8_t>(0x369C94)
    };

    drawFogOnWorld(data);
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006F120_v1_2_en()
{
    auto* const g = globals_;

    FogDrawData data
    {
        g->getPtr<uint8_t>(0x1C1436),
        g->getValue<int>(0xC1524),
        g->getValue<int>(0xC1528),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x3844FC) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites))),

        g->getValue<int>(0x37E978),
        g->getValue<int>(0x37E97C),
        g->getValue<int>(0x37E974),
        g->getValue<int>(0x37E970),

        g->getPtr<int>(0x351778),
        g->getValue<uint8_t>(0x383D1D),
        g->getPtr<uint8_t>(0x37C5EC)
    };

    drawFogOnWorld(data);
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006F120_hd_v1_1()
{
    auto* const g = globals_;

    FogDrawData data
    {
        g->getPtr<uint8_t>(0x1AEF4E),
        g->getValue<int>(0xAF03C),
        g->getValue<int>(0xAF040),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x370EE4) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites))),

        g->getValue<int>(0x36C020),
        g->getValue<int>(0x36C024),
        g->getValue<int>(0x36C01C),
        g->getValue<int>(0x36C018),

        g->getPtr<int>(0x39A000),
        g->getValue<uint8_t>(0x37099D),
        g->getPtr<uint8_t>(0x39F002)
    };

    drawFogOnWorld(data);
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006F120_v2_2()
{
    auto* const g = globals_;

    FogDrawData data
    {
        g->getPtr<uint8_t>(0x64219C),
        g->getValue<int>(0x142384),
        g->getValue<int>(0x142388),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x106F6E4) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites))),

        g->getValue<int>(0x106A130),
        g->getValue<int>(0x106A134),
        g->getValue<int>(0x106A12C),
        g->getValue<int>(0x106A128),

        g->getPtr<int>(0x103CF10),
        g->getValue<uint8_t>(0x106F069),
        g->getPtr<uint8_t>(0x1067DA4)
    };

    drawFogOnWorld_v2(data);
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006F120_rw()
{
    auto* const g = globals_;

    FogDrawData data
    {
        g->getPtr<uint8_t>(0x6311B4),
        g->getValue<int>(0x13139C),
        g->getValue<int>(0x1313A0),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x10AEABC) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites))),

        g->getValue<int>(0x10A9508),
        g->getValue<int>(0x10A950C),
        g->getValue<int>(0x10A9504),
        g->getValue<int>(0x10A9500),

        g->getPtr<int>(0x107C2E8),
        g->getValue<uint8_t>(0x10AE441),
        g->getPtr<uint8_t>(0x10A717C)
    };

    drawFogOnWorld_v2(data);
}

void __declspec(noinline) __stdcall  GameDllHooks::sub_1006F120_bg()
{
    auto* const g = globals_;

    FogDrawData data
    {
        g->getPtr<uint8_t>(0x631364),
        g->getValue<int>(0x13154C),
        g->getValue<int>(0x131550),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x109EC6C) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites))),

        g->getValue<int>(0x10996B8),
        g->getValue<int>(0x10996BC),
        g->getValue<int>(0x10996B4),
        g->getValue<int>(0x10996B0),

        g->getPtr<int>(0x106C498),
        g->getValue<uint8_t>(0x109E5F1),
        g->getPtr<uint8_t>(0x109732C)
    };

    drawFogOnWorld_v2(data);
}


void __declspec(noinline) __cdecl GameDllHooks::sub_10099E01(void* mem)
{
    if (!mem)
        return;

    auto* const g = globals_;

    HANDLE hHeap = g->getValue<HANDLE>(0x3A7FD0);

    __try
    {
        const int dword_103A7FD4 = g->getValue<int>(0x3A7FD4);

        const auto _lock = g->getFn<void(__cdecl)(int)>(0x9C8F4);
        const auto _unlock = g->getFn<void(__cdecl)(int)>(0x9C955);
        const auto __sbh_find_block = g->getFn<void* (__cdecl)(void*)>(0x9D9C8);
        const auto __sbh_free_block = g->getFn<void(__cdecl)(void*, void*)>(0x9D9F3);
        const auto small_find = g->getFn<void* (__cdecl)(void*, uint32_t**, uint32_t*)>(0x9E723);
        const auto small_free = g->getFn<void(__cdecl)(uint32_t*, int, void*)>(0x9E77A);

        void* v1;
        uint32_t* a2;
        uint32_t a3;
        if (dword_103A7FD4 == 3)
        {
            _lock(9);
            v1 = __sbh_find_block(mem);
            if (v1)
                __sbh_free_block(v1, mem);
            _unlock(9);

            if (!v1 && is_valid_ptr(mem))
                HeapFree(hHeap, 0, mem);
        }
        else if (dword_103A7FD4 != 2)
        {
            HeapFree(hHeap, 0, mem);
        }
        else
        {
            _lock(9);
            v1 = small_find(mem, &a2, &a3);
            if (v1)
                small_free(a2, a3, v1);
            _unlock(9);

            if (!v1 && is_valid_ptr(mem))
                HeapFree(hHeap, 0, mem);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void __declspec(noinline) __cdecl GameDllHooks::sub_10099E01_de(void* mem)
{
    if (!mem)
        return;

    auto* const g = globals_;

    HANDLE hHeap = g->getValue<HANDLE>(0x3A7FCC);

    __try
    {
        const auto _lock = g->getFn<void(__cdecl)(int)>(0x9F6C4);
        const auto _unlock = g->getFn<void(__cdecl)(int)>(0x9F725);
        const auto __sbh_find_block = g->getFn<void* (__cdecl)(void*)>(0xA05C5);
        const auto __sbh_free_block = g->getFn<void(__cdecl)(const void*, void*)>(0xA05F0);

        _lock(9);
        const void* v1 = __sbh_find_block(mem);
        if (v1)
        {
            __sbh_free_block(v1, mem);
            _unlock(9);
        }
        else
        {
            _unlock(9);
            if (is_valid_ptr(mem))
                HeapFree(hHeap, 0, mem);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void __declspec(noinline) __cdecl GameDllHooks::sub_10099E01_fr(void* mem)
{
    if (!mem)
        return;

    auto* const g = globals_;

    HANDLE hHeap = g->getValue<HANDLE>(0x3BB0F0);

    __try
    {
        const int dword_103A7FD4 = g->getValue<int>(0x3BB0F4);

        const auto _lock = g->getFn<void(__cdecl)(int)>(0x9FFC4);
        const auto _unlock = g->getFn<void(__cdecl)(int)>(0xA0025);
        const auto __sbh_find_block = g->getFn<void* (__cdecl)(void*)>(0xA1098);
        const auto __sbh_free_block = g->getFn<void(__cdecl)(void*, void*)>(0xA10C3);
        const auto small_find = g->getFn<void* (__cdecl)(void*, uint32_t**, uint32_t*)>(0xA1DF3);
        const auto small_free = g->getFn<void(__cdecl)(uint32_t*, int, void*)>(0xA1E4A);

        void* v1;
        uint32_t* a2;
        uint32_t a3;
        if (dword_103A7FD4 == 3)
        {
            _lock(9);
            v1 = __sbh_find_block(mem);
            if (v1)
                __sbh_free_block(v1, mem);
            _unlock(9);

            if (!v1 && is_valid_ptr(mem))
                HeapFree(hHeap, 0, mem);
        }
        else if (dword_103A7FD4 != 2)
        {
            HeapFree(hHeap, 0, mem);
        }
        else
        {
            _lock(9);
            v1 = small_find(mem, &a2, &a3);
            if (v1)
                small_free(a2, a3, v1);
            _unlock(9);

            if (!v1 && is_valid_ptr(mem))
                HeapFree(hHeap, 0, mem);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

bool GameDllHooks::is_valid_ptr(void* p)
{
    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery(p, &mbi, sizeof(mbi)))
        return false;

    if (mbi.State != MEM_COMMIT)
        return false;

    if (!(mbi.Protect & (PAGE_READWRITE | PAGE_READONLY | PAGE_EXECUTE_READWRITE)))
        return false;

    return true;
}


void __declspec(noinline) __fastcall GameDllHooks::sub_100AC870(MapData* mapData)
{
    auto* const g = globals_;

    ReadStrategicMapData data
    {
        g->getFn<void(__thiscall)(HANDLE*)>(0xCBF50),
        g->getFn<bool(__thiscall)(HANDLE*, const char*, int)>(0xCC010),
        g->getFn<uint32_t(__thiscall)(HANDLE*)>(0xCC240),
        g->getFn<void(__thiscall)(HANDLE*, void*, uint32_t)>(0xCC150),
        g->getFn<void(__thiscall)(HANDLE*)>(0xCC130),
        g->getFn<void(__thiscall)(HANDLE*)>(0xCBF80),
        g->getPtr<char>(0xFC714),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x106F6E4) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites))),
        g->getPtr<void* (__cdecl)(size_t)>(0xD9409)
    };

    readStrategicMapFromFile(mapData, data);
}

void __declspec(noinline) __fastcall GameDllHooks::sub_100A8AF0_v2_3(MapData* mapData)
{
    auto* const g = globals_;

    ReadStrategicMapData data
    {
        g->getFn<void(__thiscall)(HANDLE*)>(0xC6320),
        g->getFn<bool(__thiscall)(HANDLE*, const char*, int)>(0xC63E0),
        g->getFn<uint32_t(__thiscall)(HANDLE*)>(0xC6610),
        g->getFn<void(__thiscall)(HANDLE*, void*, uint32_t)>(0xC6520),
        g->getFn<void(__thiscall)(HANDLE*)>(0xC6500),
        g->getFn<void(__thiscall)(HANDLE*)>(0xC6350),
        g->getPtr<char>(0xEC908),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x10AEABC) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites))),
        g->getPtr<void* (__cdecl)(size_t)>(0xD1829)
    };

    readStrategicMapFromFile(mapData, data);
}

void __declspec(noinline) __fastcall GameDllHooks::sub_100A8AF0_v2_4(MapData* mapData)
{
    auto* const g = globals_;

    ReadStrategicMapData data
    {
        g->getFn<void(__thiscall)(HANDLE*)>(0xC6330),
        g->getFn<bool(__thiscall)(HANDLE*, const char*, int)>(0xC63F0),
        g->getFn<uint32_t(__thiscall)(HANDLE*)>(0xC6620),
        g->getFn<void(__thiscall)(HANDLE*, void*, uint32_t)>(0xC6530),
        g->getFn<void(__thiscall)(HANDLE*)>(0xC6510),
        g->getFn<void(__thiscall)(HANDLE*)>(0xC6360),
        g->getPtr<char>(0xEC908),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x10AEABC) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites))),
        g->getPtr<void* (__cdecl)(size_t)>(0xD1839)
    };

    readStrategicMapFromFile(mapData, data);
}

void __declspec(noinline) __fastcall GameDllHooks::sub_100A8AF0_bg(MapData* mapData)
{
    auto* const g = globals_;

    ReadStrategicMapData data
    {
        g->getFn<void(__thiscall)(HANDLE*)>(0xC6180),
        g->getFn<bool(__thiscall)(HANDLE*, const char*, int)>(0xC6240),
        g->getFn<uint32_t(__thiscall)(HANDLE*)>(0xC6470),
        g->getFn<void(__thiscall)(HANDLE*, void*, uint32_t)>(0xC6380),
        g->getFn<void(__thiscall)(HANDLE*)>(0xC6360),
        g->getFn<void(__thiscall)(HANDLE*)>(0xC61B0),
        g->getPtr<char>(0xECB28),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x109EC6C) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites))),
        g->getPtr<void* (__cdecl)(size_t)>(0xD1689)
    };

    readStrategicMapFromFile(mapData, data);
}


void __declspec(noinline) __fastcall GameDllHooks::sub_100ACDE0(MapData* mapData)
{
    auto* const g = globals_;

    FogOnStrategicMap data
    {
        g->getFn<void(__cdecl)(int*, int, int, int, int)>(0x97740),
        g->getFn<void(__thiscall)(MapData*)>(0xA1110),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int16_t)>(0xAC790),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int16_t)>(0xAC800),
        g->getFn<void(__stdcall)(int, int, int)>(0xC3420),
        g->getValue<UnitData*>(0x10F258),
        105,
        g->getValue<int16_t>(0x106F12E),
        g->getValue<int>(0x142384),
        g->getValue<int>(0x142388),
        g->getValue<int>(0x106A130),
        g->getValue<int>(0x106A134),
        g->getValue<uint8_t>(0x106F069),
        g->getPtr<uint8_t>(0x64219C),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x106F6E4) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites)))
    };

    drawFogOnStrategicMap(mapData, data);
}

void __declspec(noinline) __fastcall GameDllHooks::sub_100A9060(MapData* mapData)
{
    auto* const g = globals_;

    FogOnStrategicMap data
    {
        g->getFn<void(__cdecl)(int*, int, int, int, int)>(0x94A30),
        g->getFn<void(__thiscall)(MapData*)>(0x9DBE0),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int16_t)>(0xA8A10),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int16_t)>(0xA8A80),
        g->getFn<void(__stdcall)(int, int, int)>(0xBE2E0),
        g->getValue<UnitData*>(0xFCC90),
        113,
        g->getValue<int16_t>(0x10AE506),
        g->getValue<int>(0x13139C),
        g->getValue<int>(0x1313A0),
        g->getValue<int>(0x10A9508),
        g->getValue<int>(0x10A950C),
        g->getValue<uint8_t>(0x10AE441),
        g->getPtr<uint8_t>(0x6311B4),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x10AEABC) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites)))
    };

    drawFogOnStrategicMap(mapData, data);
}

void __declspec(noinline) __fastcall GameDllHooks::sub_100A9060_bg(MapData* mapData)
{
    auto* const g = globals_;

    FogOnStrategicMap data
    {
        g->getFn<void(__cdecl)(int*, int, int, int, int)>(0x94A00),
        g->getFn<void(__thiscall)(MapData*)>(0x9DBB0),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int16_t)>(0xA89E0),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int16_t)>(0xA8A50),
        g->getFn<void(__stdcall)(int, int, int)>(0xBE2B0),
        g->getValue<UnitData*>(0xFCE40),
        113,
        g->getValue<int16_t>(0x109E6B6),
        g->getValue<int>(0x13154C),
        g->getValue<int>(0x131550),
        g->getValue<int>(0x10996B8),
        g->getValue<int>(0x10996BC),
        g->getValue<uint8_t>(0x109E5F1),
        g->getPtr<uint8_t>(0x631364),
        reinterpret_cast<ModuleStateBase*>(g->getValue<uintptr_t>(0x109EC6C) - (offsetof(ModuleStateBase, windowRect) - offsetof(ModuleStateBase, fogSprites)))
    };

    drawFogOnStrategicMap(mapData, data);
}


void __declspec(noinline) __fastcall GameDllHooks::sub_100AD2C0(MapData* self, void* /*dummy*/, int offsetX, int offsetY)
{
    auto* const g = globals_;

    ScreenRectDrawData data
    {
        g->getFn<void(__thiscall)(MapData*, int, int)>(0xA11E0),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int)>(0x99880),
        g->getValue<int>(0x142384),
        g->getValue<int>(0x106A130),
        g->getValue<int>(0x106A134)
    };

    drawScreenRectOnStrategicMap(self, offsetX, offsetY, data);
}

void __declspec(noinline) __fastcall GameDllHooks::sub_100A97C0(MapData* self, void* /*dummy*/, int offsetX, int offsetY)
{
    auto* const g = globals_;

    ScreenRectDrawData data
    {
        g->getFn<void(__thiscall)(MapData*, int, int)>(0x9DCB0),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int)>(0x96A00),
        g->getValue<int>(0x13139C),
        g->getValue<int>(0x10A9508),
        g->getValue<int>(0x10A950C)
    };

    drawScreenRectOnStrategicMap(self, offsetX, offsetY, data);
}

void __declspec(noinline) __fastcall GameDllHooks::sub_100A97C0_bg(MapData* self, void* /*dummy*/, int offsetX, int offsetY)
{
    auto* const g = globals_;

    ScreenRectDrawData data
    {
        g->getFn<void(__thiscall)(MapData*, int, int)>(0x9DC80),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int)>(0x969D0),
        g->getValue<int>(0x13154C),
        g->getValue<int>(0x10996B8),
        g->getValue<int>(0x10996BC)
    };

    drawScreenRectOnStrategicMap(self, offsetX, offsetY, data);
}


void __declspec(noinline) __fastcall GameDllHooks::sub_100C3830(PlaneData* self, void* /*dummy*/, int halfScreenWidth, int vertCenterMargin, int scale)
{
    auto* const g = globals_;

    PlaneMapDrawData data
    {
        g->getPtr<MapData>(0x106EC18),
        g->getPtr<uint16_t>(0x106F0EC),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int16_t)>(0xAC790),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int16_t)>(0xAC800),
        halfScreenWidth,
        vertCenterMargin,
        scale
    };

    drawPlaneCrossOnStrategicMap(self, data);
}

void __declspec(noinline) __fastcall GameDllHooks::sub_100BE6F0(PlaneData* self, void* /*dummy*/, int halfScreenWidth, int vertCenterMargin, int scale)
{
    auto* const g = globals_;

    PlaneMapDrawData data
    {
        g->getPtr<MapData>(0x10ADFF0),
        g->getPtr<uint16_t>(0x10AE4C4),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int16_t)>(0xA8A10),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int16_t)>(0xA8A80),
        halfScreenWidth,
        vertCenterMargin,
        scale
    };

    drawPlaneCrossOnStrategicMap(self, data);
}

void __declspec(noinline) __fastcall GameDllHooks::sub_100BE6C0(PlaneData* self, void* /*dummy*/, int halfScreenWidth, int vertCenterMargin, int scale)
{
    auto* const g = globals_;

    PlaneMapDrawData data
    {
        g->getPtr<MapData>(0x109E1A0),
        g->getPtr<uint16_t>(0x109E674),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int16_t)>(0xA89E0),
        g->getFn<void(__thiscall)(MapData*, int, int, int, int16_t)>(0xA8A50),
        halfScreenWidth,
        vertCenterMargin,
        scale
    };

    drawPlaneCrossOnStrategicMap(self, data);
}


void GameDllHooks::someRandCalc(const SomeRandCalcData& data)
{
    int& randSeed = data.randSeed;

    auto randNext = [&randSeed]() {
        randSeed = 0x41C64E6D * randSeed + 0x3039;
        return HIWORD(randSeed) & 0x7FFF;
        };

    UnkEntry* structPtr = &data.a1[data.a2];
    GameObject* objPtr = structPtr->objPtr;

    if (objPtr == nullptr || structPtr->counter == 0)
        return;

    const int subVal = data.fn_2560(structPtr, 0);

    int tmp = randNext()
        * (objPtr->param_173d + ((subVal * objPtr->param_1741) >> 15));
    if (tmp < 0 || (tmp & 0xFFFF8000) == 0)
    {
        const int index = (data.a4 * randNext()) >> 15;
        const int offset = data.a3[index];

        UnkEntry* targetPtr = &data.a1[offset];
        GameObject* targetObj = targetPtr->objPtr;

        if (targetObj == nullptr)
            return;

        if (targetPtr->param_4 < targetObj->param_B8)
        {
            const int limit = targetObj->param_B8;
            const int computed = targetPtr->param_4
                + objPtr->param_1705
                + ((subVal * objPtr->param_1709) >> 15);

            targetPtr->param_4 = static_cast<uint16_t>(limit >= computed ? computed : limit);

            --structPtr->counter;

            int tmp2 = objPtr->param_1735
                + ((subVal * objPtr->param_1739) >> 15);
            const int v12 = std::min(tmp2 + subVal, 0x8000);

            structPtr->value = static_cast<uint16_t>(v12);
        }
    }
}

void GameDllHooks::drawFogOnWorld(const FogDrawData& data)
{
    uint8_t* const fog0 = data.fogBase;
    uint8_t* const fog1 = fog0 + 0xFF;
    uint8_t* const fog2 = fog0 + 0x100;
    uint8_t* const fog3 = fog0 + 0x101;
    uint8_t* const fog4 = fog0 + 0x200;

    const int offsetPow = 8;
    const int offset = 1 << offsetPow;

    std::memset(data.fogBuf, 0x80, sizeof(((ModuleStateBase*)0)->fogSprites));

    const int v54 = data.mapPosY >> 3;
    const int v59 = data.mapPosX >> 4;

    const int tmp = v54 - 3;
    const int v0 = tmp & 1;
    int v1 = (1 - 2 * (v0 ^ (tmp >> 1)) - (data.mapPosX >> 4)) & 3;

    const int v2 = (data.mapPosX + 16 * (v1 - 3)) >> 1;
    const int v3 = 8 * v0 - 24;

    int v51 = (data.mapPosY + v3 - v2) >> 5;
    int v8 = (data.mapPosY + v3 + v2) >> 5;

    const int len_sar_4 = data.screenWidth >> 4;
    const int v6 = (data.screenWidth >> 4) + 11;

    const int v4 = data.screenHeight >> 3;
    const int v5 = (data.screenHeight >> 3) + 11;

    const int v7 = v0 + 5;

    int v52 = v1;

    // 1. Initialize buffer byte_1037C598
    if (v7 <= v5)
    {
        uint8_t* row = &data.fogBuf[kFogLineByteSize * v7];
        unsigned int count = (v5 - v7 + 2) >> 1;
        int x = v1 + 5;

        do
        {
            int tx = x;
            int ty = v51;
            int mx = v8;
            if (x <= v6)
            {
                int idx = v51 << offsetPow;
                do
                {
                    if (mx >= 4
                        && ty >= 4
                        && mx < data.mapHeight - 4
                        && ty < data.mapWidth - 4
                        && (data.fogMask & fog2[idx + mx]) != 0)
                    {
                        if (row[tx] > 0x40)
                        {
                            if (!(data.fogMask & fog4[idx + mx]) ||
                                !(data.fogMask & fog3[idx + mx]) ||
                                !(data.fogMask & fog0[idx + mx]) ||
                                !(data.fogMask & fog1[idx + mx]))
                            {
                                row[tx] = 0x40;
                            }
                        }
                    }
                    else
                    {
                        row[tx] = 0;
                    }

                    tx += 4;
                    ++mx;
                    --ty;
                    idx -= offset;
                } while (tx <= v6);
            }

            if (v52 < 2)
            {
                x += 2;
                ++v8;
            }
            else
            {
                x -= 2;
                ++v51;
            }
            v52 ^= 2;
            row += kFogDoubleLineByteSize;
        } while (--count);
    }

    // 2. Vertical antialiasing
    int step;
    if (v1 >= 2)
    {
        step = 2;
    }
    else
    {
        v1 += 4;
        step = -2;
    }

    const int xEnd = v6 - 1;
    const int yEnd = v5 - 1;
    if (v0 + 7 < yEnd)
    {
        int x = v1 + 5;

        const int v19 = v0 + 7;
        uint8_t* p = &data.fogBuf[kFogLineByteSize * v19 - 2];
        int cnt = (yEnd - v19 + 1) >> 1;

        do
        {
            for (int xi = x; xi < xEnd;)
            {
                uint8_t a = p[xi - (kFogDoubleLineByteSize - 2)];
                uint8_t b = p[xi + 4];
                xi += 4;
                p[xi - 2] = (p[xi + (kFogDoubleLineByteSize - 2)] + p[xi - 4] + b + a) >> 2;
            }

            x += step;
            step = -step;
            p += kFogDoubleLineByteSize;
        } while (--cnt);
    }

    // 3. Horizontal antialiasing
    const int yMax = yEnd - 2;
    const int xMax = xEnd - 2;
    const int startY = (v54 & 1) + 8;

    if (startY <= yMax)
    {
        uint8_t* p = &data.fogBuf[kFogLineByteSize * startY + 1];
        int cnt = (yMax - startY + 2) >> 1;
        do
        {
            int start_i = ((v59 - 1) & 1) + 8;
            for (int i = start_i; i <= xMax; i += 2)
            {
                p[i - 1] = (p[i] + p[i - 2]) >> 1;
            }

            p += kFogDoubleLineByteSize;
        } while (--cnt);
    }

    // 4. Vertical smoothing
    const int startY2 = ((v54 - 1) & 1) + 8;
    if (startY2 <= yMax)
    {
        uint8_t* p = &data.fogBuf[kFogLineByteSize * (startY2 + 1)];
        int cnt = (yMax - startY2 + 2) >> 1;

        do
        {
            for (int j = 8; j <= xMax; ++j)
            {
                p[j - kFogLineByteSize] = (p[j] + p[j - kFogDoubleLineByteSize]) >> 1;
            }
            p += kFogDoubleLineByteSize;
        } while (--cnt);
    }

    // 5. Update fogSprites
    int width = len_sar_4 + 9;
    if (v4 + 9 > 0)
    {
        int y = 0;
        int screenY = -72;
        int rows = v4 + 9;
        do
        {
            if (width > 0)
            {
                int x = 0;
                int screenX = -144;
                do
                {
                    const uint8_t v = data.fogBuf[x + y * kFogLineByteSize];
                    uint8_t* dst = &data.cadPtr->fogSprites[y].unk[x];
                    if (v != *dst)
                    {
                        *dst = v;

                        sub_10055E00(data.div16Ptr, nullptr, 24, screenX, screenY, screenX + 31, screenY + 15);
                        width = len_sar_4 + 9;
                    }
                    ++x;
                    screenX += 16;
                } while (x < width);
            }
            ++y;
            screenY += 8;
        } while (--rows);
    }
}

void GameDllHooks::drawFogOnWorld_v2(const FogDrawData& data)
{
    uint8_t* const fog0 = data.fogBase;
    uint8_t* const fog1 = fog0 + 0x1FF;
    uint8_t* const fog2 = fog0 + 0x200;
    uint8_t* const fog3 = fog0 + 0x201;
    uint8_t* const fog4 = fog0 + 0x400;

    const int offsetPow = 9;
    const int offset = 1 << offsetPow;

    std::memset(data.fogBuf, 0x80, sizeof(((ModuleStateBase*)0)->fogSprites));

    const int v54 = data.mapPosY >> 3;
    const int v59 = data.mapPosX >> 4;

    const int tmp = v54 - 3;
    const int v0 = tmp & 1;
    int v1 = (1 - 2 * (v0 ^ (tmp >> 1)) - (data.mapPosX >> 4)) & 3;

    const int v2 = (data.mapPosX + 16 * (v1 - 3)) >> 1;
    const int v3 = 8 * v0 - 24;

    int v51 = (data.mapPosY + v3 - v2) >> 5;
    int v8 = (data.mapPosY + v3 + v2) >> 5;

    const int len_sar_4 = data.screenWidth >> 4;
    const int v6 = (data.screenWidth >> 4) + 11;

    const int v4 = data.screenHeight >> 3;
    const int v5 = (data.screenHeight >> 3) + 11;

    const int v7 = v0 + 5;

    int v52 = v1;

    // 1. Initialize buffer byte_1037C598
    if (v7 <= v5)
    {
        uint8_t* row = &data.fogBuf[kFogLineByteSize * v7];
        unsigned int count = (v5 - v7 + 2) >> 1;
        int x = v1 + 5;

        do
        {
            int tx = x;
            int ty = v51;
            int mx = v8;
            if (x <= v6)
            {
                int idx = v51 << offsetPow;
                do
                {
                    if (mx >= 4
                        && ty >= 4
                        && mx < data.mapHeight - 4
                        && ty < data.mapWidth - 4
                        && (data.fogMask & fog2[idx + mx]) != 0)
                    {
                        if (row[tx] > 0x40)
                        {
                            if (!(data.fogMask & fog4[idx + mx]) ||
                                !(data.fogMask & fog3[idx + mx]) ||
                                !(data.fogMask & fog0[idx + mx]) ||
                                !(data.fogMask & fog1[idx + mx]))
                            {
                                row[tx] = 0x40;
                            }
                        }
                    }
                    else
                    {
                        row[tx] = 0;
                    }

                    tx += 4;
                    ++mx;
                    --ty;
                    idx -= offset;
                } while (tx <= v6);
            }

            if (v52 < 2)
            {
                x += 2;
                ++v8;
            }
            else
            {
                x -= 2;
                ++v51;
            }
            v52 ^= 2;
            row += kFogDoubleLineByteSize;
        } while (--count);
    }

    // 2. Vertical antialiasing
    int step;
    if (v1 >= 2)
    {
        step = 2;
    }
    else
    {
        v1 += 4;
        step = -2;
    }

    const int xEnd = v6 - 1;
    const int yEnd = v5 - 1;
    if (v0 + 7 < yEnd)
    {
        int x = v1 + 5;

        const int v19 = v0 + 7;
        uint8_t* p = &data.fogBuf[kFogLineByteSize * v19 - 2];
        int cnt = (yEnd - v19 + 1) >> 1;

        do
        {
            for (int xi = x; xi < xEnd;)
            {
                uint8_t a = p[xi - (kFogDoubleLineByteSize - 2)];
                uint8_t b = p[xi + 4];
                xi += 4;
                p[xi - 2] = (p[xi + (kFogDoubleLineByteSize - 2)] + p[xi - 4] + b + a) >> 2;
            }

            x += step;
            step = -step;
            p += kFogDoubleLineByteSize;
        } while (--cnt);
    }

    // 3. Horizontal antialiasing
    const int yMax = yEnd - 2;
    const int xMax = xEnd - 2;
    const int startY = (v54 & 1) + 8;

    if (startY <= yMax)
    {
        uint8_t* p = &data.fogBuf[kFogLineByteSize * startY + 1];
        int cnt = (yMax - startY + 2) >> 1;
        do
        {
            int start_i = ((v59 - 1) & 1) + 8;
            for (int i = start_i; i <= xMax; i += 2)
            {
                p[i - 1] = (p[i] + p[i - 2]) >> 1;
            }

            p += kFogDoubleLineByteSize;
        } while (--cnt);
    }

    // 4. Vertical smoothing
    const int startY2 = ((v54 - 1) & 1) + 8;
    if (startY2 <= yMax)
    {
        uint8_t* p = &data.fogBuf[kFogLineByteSize * (startY2 + 1)];
        int cnt = (yMax - startY2 + 2) >> 1;

        do
        {
            for (int j = 8; j <= xMax; ++j)
            {
                p[j - kFogLineByteSize] = (p[j] + p[j - kFogDoubleLineByteSize]) >> 1;
            }
            p += kFogDoubleLineByteSize;
        } while (--cnt);
    }

    // 5. Update fogSprites
    int width = len_sar_4 + 9;
    if (v4 + 9 > 0)
    {
        int y = 0;
        int screenY = -72;
        int rows = v4 + 9;
        do
        {
            if (width > 0)
            {
                int x = 0;
                int screenX = -144;
                do
                {
                    const uint8_t v = data.fogBuf[x + y * kFogLineByteSize];
                    uint8_t* dst = &data.cadPtr->fogSprites[y].unk[x];
                    if (v != *dst)
                    {
                        *dst = v;

                        sub_10055E00(data.div16Ptr, nullptr, 24, screenX, screenY, screenX + 31, screenY + 15);
                        width = len_sar_4 + 9;
                    }
                    ++x;
                    screenX += 16;
                } while (x < width);
            }
            ++y;
            screenY += 8;
        } while (--rows);
    }
}


void GameDllHooks::readStrategicMapFromFile(MapData* mapData, const ReadStrategicMapData& data)
{
    if (mapData->isMapLoaded)
        return;

    HANDLE miniMapFileHandle[2];
    data.initHandle(miniMapFileHandle);

    int readStatus = 0;

    if (data.createFile(miniMapFileHandle, data.xchngTogameMis, 0) && data.getFileSize(miniMapFileHandle))
    {
        int mapWidth = 0, mapHeight = 0, mapExtraSize = 0;

        data.readFile(miniMapFileHandle, &mapWidth, sizeof(mapWidth));
        data.readFile(miniMapFileHandle, &mapHeight, sizeof(mapHeight));
        data.readFile(miniMapFileHandle, &mapExtraSize, sizeof(mapExtraSize));

        unsigned int mapBufferSize = 3 * mapWidth * mapHeight;  // 3 channels for pixel
        uint8_t* mapBuffer = new uint8_t[mapBufferSize];
        data.readFile(miniMapFileHandle, mapBuffer, mapBufferSize);

        data.closeHandle(miniMapFileHandle);

        const int screenWidth = data.cadPtr->surface.width;
        const int screenHeight = data.cadPtr->surface.height;
        mapData->screenSurfaceWidth = screenWidth;
        mapData->screenSurfaceHeight = screenHeight;

        double scaleX = static_cast<double>(mapWidth) / screenWidth;

        // Calculate real strategic map height
        int miniMapScreenHeight = static_cast<int>(mapHeight / scaleX);
        if (miniMapScreenHeight > screenHeight)
        {
            // If height is too big, calculate it via Y
            scaleX = static_cast<double>(mapHeight) / screenHeight;
            miniMapScreenHeight = screenHeight;
        }
        double scaleY = static_cast<double>(mapHeight) / miniMapScreenHeight;

        // Vertical offset from border
        mapData->verticalCenterMargin = (screenHeight - miniMapScreenHeight) / 2;

        // Buffer for future usage
        mapData->srcBuf = (uint8_t*)data.fnNew(3 * screenWidth * screenHeight);
        memset(mapData->srcBuf, 0, 3 * screenWidth * screenHeight);

        // Bilinear interpolation
        auto bilinearInterpolate = [&](double mapXScaled, double mapYScaled, uint8_t& rOut, uint8_t& gOut, uint8_t& bOut)
            {
                int mapX = std::min(static_cast<int>(mapXScaled), mapWidth - 2);
                int mapY = std::min(static_cast<int>(mapYScaled), mapHeight - 2);

                double fracX = mapXScaled - mapX;
                double fracY = mapYScaled - mapY;
                double invFracX = 1.0 - fracX;
                double invFracY = 1.0 - fracY;

                int idxTL = 3 * (mapY * mapWidth + mapX);
                int idxTR = idxTL + 3;
                int idxBL = 3 * ((mapY + 1) * mapWidth + mapX);
                int idxBR = idxBL + 3;

                double rTL = mapBuffer[idxTL + 0], gTL = mapBuffer[idxTL + 1], bTL = mapBuffer[idxTL + 2];
                double rTR = mapBuffer[idxTR + 0], gTR = mapBuffer[idxTR + 1], bTR = mapBuffer[idxTR + 2];
                double rBL = mapBuffer[idxBL + 0], gBL = mapBuffer[idxBL + 1], bBL = mapBuffer[idxBL + 2];
                double rBR = mapBuffer[idxBR + 0], gBR = mapBuffer[idxBR + 1], bBR = mapBuffer[idxBR + 2];

                double wTL = invFracX * invFracY;
                double wTR = fracX * invFracY;
                double wBL = invFracX * fracY;
                double wBR = fracX * fracY;

                rOut = static_cast<uint8_t>(rTL * wTL + rTR * wTR + rBL * wBL + rBR * wBR);
                gOut = static_cast<uint8_t>(gTL * wTL + gTR * wTR + gBL * wBL + gBR * wBR);
                bOut = static_cast<uint8_t>(bTL * wTL + bTR * wTR + bBL * wBL + bBR * wBR);
            };

        for (int screenY = 0; screenY < miniMapScreenHeight; ++screenY)
        {
            int targetY = screenY + mapData->verticalCenterMargin;
            for (int screenX = 0; screenX < screenWidth; ++screenX)
            {
                double mapXScaled = screenX * scaleX;
                double mapYScaled = screenY * scaleY;

                uint8_t r, g, b;
                bilinearInterpolate(mapXScaled, mapYScaled, r, g, b);

                int screenIndex = screenX + targetY * screenWidth;
                mapData->srcBuf[3 * screenIndex + 0] = r;
                mapData->srcBuf[3 * screenIndex + 1] = g;
                mapData->srcBuf[3 * screenIndex + 2] = b;
            }
        }

        delete[] mapBuffer;

        mapData->isMapLoaded = 1;
    }

    readStatus = -1;
    data.deinitHandle(miniMapFileHandle);
}

void GameDllHooks::drawFogOnStrategicMap(MapData* mapData, const FogOnStrategicMap& data)
{
    const uint8_t* fog0 = data.fogBase;
    const uint8_t* fog1 = fog0 + 0x1FF;
    const uint8_t* fog2 = fog0 + 0x200;
    const uint8_t* fog3 = fog0 + 0x201;
    const uint8_t* fog4 = fog0 + 0x400;

    data.fn_A1110(mapData);

    int clip[4];
    data.fn_97740(clip, mapData->clipLeft, mapData->clipTop, mapData->clipRight, mapData->clipBottom);

    clip[2] += 16;
    clip[3] += 8;

    // Calculate fog
    const double scale = 64.0 * data.mapHeight / mapData->screenSurfaceWidth;
    const double scale2 = scale / 32.0;

    const auto getFogCount = [&](int index) -> int {
        int cnt = 0;
        if (fog4[index] & data.fogMask) ++cnt;
        if (fog0[index] & data.fogMask) ++cnt;
        if (fog3[index] & data.fogMask) ++cnt;
        if (fog1[index] & data.fogMask) ++cnt;
        return cnt;
        };

    const int halfScreenW = mapData->screenSurfaceWidth / 2;
    const int screenH = mapData->screenSurfaceHeight;

    for (int y = clip[1]; y < clip[3]; ++y)
    {
        for (int x = clip[0]; x < clip[2]; ++x)
        {
            const int dx = (x - halfScreenW) >> 1;
            const int mapY1 = static_cast<int>((y + dx - mapData->verticalCenterMargin) * scale2);
            const int mapY2 = static_cast<int>((y - dx - mapData->verticalCenterMargin) * scale2);

            if (mapY1 < 1 || mapY2 < 1 || mapY1 >= data.mapHeight - 1 || mapY2 >= data.mapWidth - 1)
                continue;

            uint8_t* src = &mapData->srcBuf[3 * x + 3 * mapData->screenSurfaceWidth * (screenH - y - 1)];

            int r = src[2];
            int gCol = src[1];
            int b = src[0];

            const int fogIndex = mapY1 + (mapY2 << 9);

            if ((fog2[fogIndex] & data.fogMask) == 0)
            {
                double brightness = (getFogCount(fogIndex) > 1) ? 0.8 : 0.5;
                r = static_cast<int>(r * brightness);
                gCol = static_cast<int>(gCol * brightness);
                b = static_cast<int>(b * brightness);
            }

            const uint16_t gb =
                (data.cadPtr->actualGreenMask & (gCol << 8 >> data.cadPtr->greenOffset)) |
                (data.cadPtr->actualBlueMask & (b << 8 >> data.cadPtr->blueOffset));

            mapData->dstBuf[x + y * mapData->stride] =
                (data.cadPtr->actualRedMask & (r << 8 >> data.cadPtr->redOffset)) | gb;
        }
    }

    // Draw screen rectangle
    int rectWidth = static_cast<int>(mapData->screenSurfaceWidth / scale);
    int rectHeight = static_cast<int>(mapData->screenSurfaceHeight / scale);

    int left = static_cast<int>(mapData->screenSurfaceWidth / 2 + data.mapPosX / scale);
    int right = left + rectWidth - 1;
    int bottom = static_cast<int>(mapData->verticalCenterMargin + data.mapPosY / scale);
    int top = bottom + rectHeight - 1;

    const int w = right - left + 1;
    const int h = top - bottom + 1;

    data.drawHorLine(mapData, left, bottom, w, data.fogBorderColor);
    data.drawHorLine(mapData, left, bottom + h - 1, w, data.fogBorderColor);
    data.drawVertLine(mapData, left, bottom, h, data.fogBorderColor);
    data.drawVertLine(mapData, left + w - 1, bottom, h, data.fogBorderColor);

    // This value (1.2f) was found empirically
    data.drawPlaneCrosses(
        mapData->screenSurfaceWidth / 2,
        static_cast<int>(mapData->verticalCenterMargin * 1.2f),
        static_cast<int>(scale)
    );

    // This code always calls stubs
    /*
    const int* dword_1106F69C = g->getValue<int*>(0x106F69C);

    const uintptr_t objAddr = static_cast<uintptr_t>(*dword_1106F69C);
    const uintptr_t funcAddr = *reinterpret_cast<uintptr_t*>(objAddr + 56);

    const uint64_t bits = *reinterpret_cast<const uint64_t*>(&scale2);
    const uint32_t lo = static_cast<uint32_t>(bits & 0xFFFFFFFF);
    const uint32_t hi = static_cast<uint32_t>(bits >> 32);

    reinterpret_cast<void(__stdcall*)(int, int, uint32_t, uint32_t)>(funcAddr)(
        mapData->screenSurfaceWidth / 2,
        mapData->verticalCenterMargin,
        lo,
        hi
        );
    */

    // Draw units points
    // It also shows building occupied by enemy as red dot even if we don't know that the building is occupied by them
    for (UnitData* obj = data.units; obj; obj = obj->next)
    {
        const int vx = static_cast<int>(((obj->subX >> 8) + 32.0 * obj->tileX) / scale);
        const int vy = static_cast<int>(((obj->subY >> 8) + 32.0 * obj->tileY) / scale);

        auto fn = reinterpret_cast<void(__thiscall*)(UnitData*, int, int)>(obj->vtable[data.unitVtableOffset]);

        fn(
            obj,
            vx + mapData->screenSurfaceWidth / 2 - vy,
            mapData->verticalCenterMargin + ((vx + vy) >> 1)
        );
    }
}

void GameDllHooks::drawScreenRectOnStrategicMap(MapData* mapData, int offsetX, int offsetY, const ScreenRectDrawData& data)
{
    data.fn1(mapData, offsetX, offsetY);

    auto drawRect = [&](int centerX, int centerY)
        {
            const int width = mapData->screenSurfaceWidth;
            const double scale = 64.0 * data.mapHeight / width;

            int rectWidth = static_cast<int>(width / scale);
            int rectHeight = static_cast<int>(mapData->screenSurfaceHeight / scale);

            int left = static_cast<int>(width / 2 + centerX / scale);
            int right = left + rectWidth - 1;
            int top = static_cast<int>(mapData->verticalCenterMargin + centerY / scale);
            int bottom = top + rectHeight - 1;

            data.fn2(mapData, left, top, right, bottom);
        };

    drawRect(data.mapPosX, data.mapPosY);
    drawRect(data.mapPosX + offsetX, data.mapPosY + offsetY);
}

void GameDllHooks::drawPlaneCrossOnStrategicMap(PlaneData* mapData, const PlaneMapDrawData& data)
{
    const float x = mapData->worldX;
    const float y = mapData->worldY;
    const float h = mapData->height;

    // These float values were found empirically
    float isoX = (x - y) / data.scale / 1.07f;
    float isoY = ((x + y) * 0.5f - h) / data.scale / 1.1f;

    int mapX = data.halfScreenWidth + static_cast<int>(isoX);
    int mapY = data.vertCenterMargin + static_cast<int>(isoY);

    data.drawHorLine(data.mapData, mapX - 2, mapY, 5, data.colors[mapData->teamId]);
    data.drawVertLine(data.mapData, mapX, mapY - 2, 5, data.colors[mapData->teamId]);
}
