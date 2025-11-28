#pragma once

#include "ProfileBase.h"
#include "ProfileSSGoldEn.h"

using Profile_SS_GOLD_EN = GameVersionProfile<
    GameVersion::SS_GOLD_EN,
    relocs_game_ss_gold_en,
    hooks_game_ss_gold_en,
    patches_game_ss_gold_en,
    relocs_menu_ss_gold_en,
    hooks_menu_ss_gold_en,
    patches_menu_ss_gold_en
>;

using Profile_SS_GOLD_HD_1_2_RU = GameVersionProfile<
    GameVersion::SS_GOLD_HD_1_2_RU,
    relocs_game_ss_gold_hd_1_2,
    hooks_game_ss_gold_hd_1_2,
    patches_game_ss_gold_hd_1_2,
    relocs_menu_ss_gold_hd_1_2,
    hooks_menu_ss_gold_hd_1_2,
    patches_menu_ss_gold_hd_1_2
>;

using Profile_SS_GOLD_HD_1_2_INT = GameVersionProfile<
    GameVersion::SS_GOLD_HD_1_2_INT,
    relocs_game_ss_gold_hd_1_2,
    hooks_game_ss_gold_hd_1_2,
    patches_game_ss_gold_hd_1_2,
    relocs_menu_ss_gold_hd_1_2,
    hooks_menu_ss_gold_hd_1_2,
    patches_menu_ss_gold_hd_1_2
>;