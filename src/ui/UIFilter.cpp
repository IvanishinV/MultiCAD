#include "pch.h"
#include "UiFilter.h"

static UIFilter g_uiFilter;

UIFilter& GetUIFilter()
{
    return g_uiFilter;
}

void UIFilter::setEnabled(const bool enabled)
{
    uiEnabled_ = enabled;
}

bool UIFilter::isEnabled() const
{
    return uiEnabled_;
}

bool UIFilter::shouldIgnore(const int type) const
{
    if (uiEnabled_)
        return false;

    if (isMinimapUi(type))
        return true;
    if (isStatsUi(type))
        return true;
    if (isCrewUi(type))
        return true;

    return false;
}

bool UIFilter::shouldIgnoreDecor(const int type) const
{
    if (uiEnabled_)
        return false;

    if (isDecorUI(type))
        return true;

    return false;
}

bool UIFilter::shouldIgnoreByTag(const int tag) const
{
    if (uiEnabled_)
        return false;

    if ((tag & 0xFFFF0000) == ('Z' << 24 | 'L' << 16))
    {
        return true;
    }

    switch (tag)
    {
    case 'PANL':
    case 'TTWN':
        return true;
    }
    return false;
}

bool UIFilter::isMinimapUi(const int type) const
{
    return type == 10;
}

bool UIFilter::isStatsUi(const int type) const
{
    return type == 20;
}

bool UIFilter::isCrewUi(const int type) const
{
    return type >= 320 && type < 440;
}

bool UIFilter::isChatUI(const int type) const
{
    return type == 601;
}

bool UIFilter::isDecorUI(const int type) const
{
    return type == 11 || type == 12 || type == 21;
}

int UIFilter::getCustomType()
{
    return 0x20;
}

int UIFilter::getCustomTag()
{
    return 'IVAT';
}
