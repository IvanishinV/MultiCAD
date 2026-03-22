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

// Additionally, there is a decorative UI near minimap area to make it a bit more beautiful.
// It uses the same structure as a chat text on the left. Since it doesn't provide any handlers
// for mouse, the easiest way is just to skip its rendering. See isDecorUI method usage

// There are the following types:
// 11 - horizontal decor above the minimap
// 12 - vertical dark line to the right of minimap
// 21 - horizontal decor above stats

class UIFilter
{
public:
    void setEnabled(const bool enabled);
    bool isEnabled() const;

    /**
     * Checks UI type, if it should be skipped on rendering.
     *
     * @return True if shouldn't be rendered, false if should be visible.
     */
    bool shouldIgnore(const int type) const;
    bool shouldIgnoreByTag(const int tag) const;
    bool shouldIgnoreDecor(const int type) const;

    bool isMinimapUi(const int type) const;
    bool isStatsUi(const int type) const;
    bool isCrewUi(const int type) const;
    bool isChatUI(const int type) const;
    bool isDecorUI(const int type) const;

    static int getCustomType();
    static int getCustomTag();

private:
    bool uiEnabled_{ true };
};

UIFilter& GetUIFilter();
