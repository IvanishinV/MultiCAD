#pragma once

#include "types.h"
#include "DllHooksBase.h"

struct MenuTag {};

class MenuDllHooks : public DllHooksBase<MenuTag>
{
public:
    static void __declspec(noinline) __fastcall sub_10014B70(void* self);
};