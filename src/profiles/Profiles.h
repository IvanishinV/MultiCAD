#pragma once

#include "ProfileBase.h"
#include "ProfileSSGoldEn.h"

using Profile_SS_RU = GameVersionProfile<
    GameVersion::SS_RU,
    relocs_game_ss_ru,
    hooks_game_ss_ru,
    patches_game_ss_ru,
    relocs_empty,
    hooks_menu_ss_ru,
    patches_empty
>;

using Profile_SS_HD_v1_1_RU = GameVersionProfile<
    GameVersion::SS_HD_V1_1_RU,
    relocs_game_ss_hd_v1_1,
    hooks_game_ss_hd_v1_1,
    patches_game_ss_hd_v1_1,
    relocs_empty,
    hooks_menu_ss_hd_v1_1_ru,
    patches_menu_ss_hd_v1_1
>;

using Profile_SS_HD_v1_1_EN = GameVersionProfile<
    GameVersion::SS_HD_V1_1_RU,
    relocs_game_ss_hd_v1_1,
    hooks_game_ss_hd_v1_1,
    patches_game_ss_hd_v1_1,
    relocs_empty,
    hooks_menu_ss_hd_v1_1_en,
    patches_menu_ss_hd_v1_1
>;

using Profile_SS_GOLD_EN = GameVersionProfile<
    GameVersion::SS_GOLD_EN,
    relocs_game_ss_gold_en,
    hooks_game_ss_gold_en,
    patches_game_ss_gold_en,
    relocs_empty,
    hooks_menu_ss_gold_en,
    patches_empty
>;

using Profile_SS_GOLD_DE = GameVersionProfile<
    GameVersion::SS_GOLD_DE,
    relocs_game_ss_gold_de_ru,
    hooks_game_ss_gold_de_ru,
    patches_game_ss_gold_de_ru,
    relocs_empty,
    hooks_menu_ss_gold_en,
    patches_empty
>;

using Profile_SS_GOLD_FR = GameVersionProfile<
    GameVersion::SS_GOLD_FR,
    relocs_game_ss_gold_fr,
    hooks_game_ss_gold_fr,
    patches_game_ss_gold_fr,
    relocs_empty,
    hooks_menu_ss_gold_fr,
    patches_empty
>;

using Profile_SS_GOLD_RU = GameVersionProfile<
    GameVersion::SS_GOLD_RU,
    relocs_game_ss_gold_de_ru,
    hooks_game_ss_gold_de_ru,
    patches_game_ss_gold_de_ru,
    relocs_empty,
    hooks_menu_ss_gold_ru,
    patches_empty
>;

using Profile_SS_GOLD_HD_1_2_RU = GameVersionProfile<
    GameVersion::SS_GOLD_HD_1_2_RU,
    relocs_game_ss_gold_hd_1_2,
    hooks_game_ss_gold_hd_1_2,
    patches_game_ss_gold_hd_1_2,
    relocs_empty,
    hooks_menu_ss_gold_hd_1_2,
    patches_menu_ss_gold_hd_1_2
>;

using Profile_SS_GOLD_HD_1_2_INT = GameVersionProfile<
    GameVersion::SS_GOLD_HD_1_2_INT,
    relocs_game_ss_gold_hd_1_2,
    hooks_game_ss_gold_hd_1_2,
    patches_game_ss_gold_hd_1_2,
    relocs_empty,
    hooks_menu_ss_gold_hd_1_2,
    patches_menu_ss_gold_hd_1_2
>;