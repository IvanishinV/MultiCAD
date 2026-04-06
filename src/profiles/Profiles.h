#pragma once

#include "ProfileBase.h"
#include "ProfileSpecs.h"

using Profile_Empty = GameVersionProfile<
    GameVersion::UNKNOWN,
    relocs_empty,
    hooks_empty,
    patches_empty,
    relocs_empty,
    hooks_empty,
    patches_empty
>;

using Profile_SS_v1_0 = GameVersionProfile<
    GameVersion::SS_V1_0,
    relocs_game_ss_ru,
    hooks_game_ss_ru<GameVersion::SS_V1_0>,
    patches_game_ss_ru,
    relocs_empty,
    hooks_menu_ss_ru,
    patches_empty
>;

using Profile_SS_v1_2 = GameVersionProfile<
    GameVersion::SS_V1_2,
    relocs_game_ss_cd_en,
    hooks_game_ss_cd_en<GameVersion::SS_V1_2>,
    patches_game_ss_cd_en,
    relocs_empty,
    hooks_menu_ss_cd_en,
    patches_empty
>;

using Profile_SS_GOLD_EN = GameVersionProfile<
    GameVersion::SS_GOLD_EN,
    relocs_game_ss_gold_en,
    hooks_game_ss_gold_en<GameVersion::SS_GOLD_EN>,
    patches_game_ss_gold_en,
    relocs_empty,
    hooks_menu_ss_gold_en,
    patches_empty
>;

using Profile_SS_GOLD_DE = GameVersionProfile<
    GameVersion::SS_GOLD_DE,
    relocs_game_ss_gold_de_ru,
    hooks_game_ss_gold_de_ru<GameVersion::SS_GOLD_DE>,
    patches_game_ss_gold_de_ru,
    relocs_empty,
    hooks_menu_ss_gold_en,
    patches_empty
>;

using Profile_SS_GOLD_FR = GameVersionProfile<
    GameVersion::SS_GOLD_FR,
    relocs_game_ss_gold_fr,
    hooks_game_ss_gold_fr<GameVersion::SS_GOLD_FR>,
    patches_game_ss_gold_fr,
    relocs_empty,
    hooks_menu_ss_gold_fr,
    patches_empty
>;

using Profile_SS_GOLD_RU = GameVersionProfile<
    GameVersion::SS_GOLD_RU,
    relocs_game_ss_gold_de_ru,
    hooks_game_ss_gold_de_ru<GameVersion::SS_GOLD_DE>,
    patches_game_ss_gold_de_ru,
    relocs_empty,
    hooks_menu_ss_gold_ru,
    patches_empty
>;

using Profile_SS_HD_v1_1_RU = GameVersionProfile<
    GameVersion::SS_HD_V1_1_RU,
    relocs_game_ss_hd_v1_1,
    hooks_game_ss_hd_v1_1<GameVersion::SS_HD_V1_1_RU>,
    patches_game_ss_hd_v1_1,
    relocs_empty,
    hooks_menu_ss_hd_v1_1_ru,
    patches_menu_ss_hd_v1_1
>;

using Profile_SS_HD_v1_1_EN = GameVersionProfile<
    GameVersion::SS_HD_V1_1_RU,
    relocs_game_ss_hd_v1_1,
    hooks_game_ss_hd_v1_1<GameVersion::SS_HD_V1_1_RU>,
    patches_game_ss_hd_v1_1,
    relocs_empty,
    hooks_menu_ss_hd_v1_1_en,
    patches_menu_ss_hd_v1_1
>;

using Profile_SS_GOLD_HD_v1_2_RU = GameVersionProfile<
    GameVersion::SS_GOLD_HD_1_2_RU,
    relocs_game_ss_gold_hd_v1_2,
    hooks_game_ss_gold_hd_v1_2<GameVersion::SS_GOLD_HD_1_2_INT>,
    patches_game_ss_gold_hd_v1_2,
    relocs_empty,
    hooks_menu_ss_gold_hd_v1_2,
    patches_menu_ss_gold_hd_v1_2
>;

using Profile_SS_GOLD_HD_v1_2_INT = GameVersionProfile<
    GameVersion::SS_GOLD_HD_1_2_INT,
    relocs_game_ss_gold_hd_v1_2,
    hooks_game_ss_gold_hd_v1_2<GameVersion::SS_GOLD_HD_1_2_INT>,
    patches_game_ss_gold_hd_v1_2,
    relocs_empty,
    hooks_menu_ss_gold_hd_v1_2,
    patches_menu_ss_gold_hd_v1_2
>;

using Profile_SS_2 = GameVersionProfile<
    GameVersion::SS_2,
    relocs_game_ss_2_v2_2,
    hooks_game_ss_2_v2_2<GameVersion::SS_2>,
    patches_game_ss_2_v2_2,
    relocs_empty,
    hooks_menu_ss_2_v2_2,
    patches_empty
>;

using Profile_HS_2 = GameVersionProfile<
    GameVersion::HS_2,
    relocs_game_ss_2_v2_2,
    hooks_game_ss_2_v2_2<GameVersion::SS_2>,
    patches_game_ss_2_v2_2,
    relocs_empty,
    hooks_menu_hs,
    patches_empty
>;

using Profile_SS_RW_v2_3 = GameVersionProfile<
    GameVersion::SS_RW_V2_3,
    relocs_game_ss_rw_v2_4,
    hooks_game_ss_rw_v2_3<GameVersion::SS_RW_V2_3>,
    patches_game_ss_rw_v2_4,
    relocs_empty,
    hooks_menu_ss_rw_v2_3,
    patches_empty
>;

using Profile_SS_RW_v2_4 = GameVersionProfile<
    GameVersion::SS_RW_V2_4,
    relocs_game_ss_rw_v2_4,
    hooks_game_ss_rw_v2_4<GameVersion::SS_RW_V2_4>,
    patches_game_ss_rw_v2_4,
    relocs_empty,
    hooks_menu_ss_rw_v2_4,
    patches_empty
>;

using Profile_SS_BLACK_GOLD = GameVersionProfile<
    GameVersion::SS_BLACK_GOLD,
    relocs_game_black_gold,
    hooks_game_black_gold<GameVersion::SS_BLACK_GOLD>,
    patches_game_black_gold,
    relocs_empty,
    hooks_menu_ss_rw_v2_3,
    patches_empty
>;

using Profile_SS_EUROPE_2015 = GameVersionProfile<
    GameVersion::SS_EUROPE_2015,
    relocs_game_ss_rw_v2_4,
    hooks_game_ss_rw_v2_4<GameVersion::SS_RW_V2_4>,
    patches_game_ss_rw_v2_4,
    relocs_empty,
    hooks_menu_ss_black_sea_europe_2015,
    patches_empty
>;
