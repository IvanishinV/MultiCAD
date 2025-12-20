#pragma once

#include "types.h"
#include "DllHooksBase.h"

struct MenuTag {};

class MenuDllHooks : public DllHooksBase<MenuTag>
{
public:
    // Sudden Strike v1.21 based games
    static void __declspec(noinline) __fastcall sub_10014B70(void* self);
    static void __declspec(noinline) __fastcall sub_10014B70_hd(void* self);
    static void __declspec(noinline) __fastcall sub_10014B70_fr(void* self);
    static void __declspec(noinline) __fastcall sub_10014B70_ru(void* self);

    // Sudden Strike v1.0 based games
    static void __declspec(noinline) __fastcall sub_1000E3D0_ru(void* self);
    static void __declspec(noinline) __fastcall sub_1000E3D0_hd_ru(void* self);
    static void __declspec(noinline) __fastcall sub_1000E3D0_hd_en(void* self);

    // Sudden Strike v1.2 based games
    static void __declspec(noinline) __fastcall sub_1000F2D0_en(void* self);

private:
    static void sub_10014B70_common(void* self, const char* versionStr, int x);
    static void sub_1000E3D0_hd_common(void* self, int x);
};