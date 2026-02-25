#pragma once

// There are the following event tags (just usual int):
// PANL - minimap
// TTWN - stats
// ZL00 - vehicle
// ZL** - crew and passengers
// TMAP - strategic map
// FILD - ground
// /KBD - keyboard
// /UTF - pressed symbol
// MENU - in-game menu
// mbEX - confirmation window

// If I disable UI, I need to ignore the following tags during some calculations:
// PANL, TTWN, ZL**

// There are also the following UI types:
// 10      - minimap
// 20      - stats
// 320-440 - crew and passengers
// 601     - chat

class UIFilter
{
public:
    void setEnabled(const bool enabled);
    bool isEnabled() const;

    bool shouldIgnore(const int type) const;
    bool shouldIgnoreByTag(const int tag) const;

    bool isMinimapUi(const int type) const;
    bool isStatsUi(const int type) const;
    bool isCrewUi(const int type) const;
    bool isChatUI(const int type) const;

    static int getCustomType();
    static int getCustomTag();

private:
    bool uiEnabled_{ true };
};

UIFilter& GetUIFilter();
