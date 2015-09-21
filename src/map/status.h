/*
 * Copyright (C) 2002-2007  Auriga
 *
 * This file is part of Auriga.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street - Fifth Floor, Boston, MA 02110-1301, USA.
 *
 */

#ifndef _STATUS_H_
#define _STATUS_H_

struct status_pretimer {
	int timer;
	int target_id;
	short m;
	short type;
	int val1,val2,val3,val4;
	int tick;
	int flag;
};

// ステータス異常データベース
struct scdata_db {
	short save;
	int releasable;
	int disable;
};

// パラメータ取得系
int status_get_class(struct block_list *bl);
int status_get_dir(struct block_list *bl);
int status_get_lv(struct block_list *bl);
int status_get_range(struct block_list *bl);
int status_get_group(struct block_list *bl);
int status_get_hp(struct block_list *bl);
int status_get_sp(struct block_list *bl);
int status_get_max_hp(struct block_list *bl);
int status_get_max_sp(struct block_list *bl);
int status_get_str(struct block_list *bl);
int status_get_agi(struct block_list *bl);
int status_get_vit(struct block_list *bl);
int status_get_int(struct block_list *bl);
int status_get_dex(struct block_list *bl);
int status_get_luk(struct block_list *bl);
int status_get_hit(struct block_list *bl);
int status_get_flee(struct block_list *bl);
int status_get_def(struct block_list *bl);
int status_get_mdef(struct block_list *bl);
int status_get_flee2(struct block_list *bl);
int status_get_def2(struct block_list *bl);
int status_get_mdef2(struct block_list *bl);
int status_get_baseatk(struct block_list *bl);
int status_get_atk(struct block_list *bl);
int status_get_atk2(struct block_list *bl);
int status_get_speed(struct block_list *bl);
int status_get_adelay(struct block_list *bl);
int status_get_amotion(struct block_list *bl);
int status_get_dmotion(struct block_list *bl);
int status_get_element(struct block_list *bl);
int status_get_attack_element(struct block_list *bl);
int status_get_attack_element2(struct block_list *bl);
int status_get_attack_element_nw(struct block_list *bl);
#define status_get_elem_type(bl)	(status_get_element(bl)%20)
#define status_get_elem_level(bl)	(status_get_element(bl)/20)
int status_get_party_id(struct block_list *bl);
int status_get_guild_id(struct block_list *bl);
int status_get_race(struct block_list *bl);
int status_get_size(struct block_list *bl);
int status_get_mode(struct block_list *bl);
int status_get_mexp(struct block_list *bl);
int status_get_enemy_type(struct block_list *bl);
short status_get_clothes_color(struct block_list *bl);

struct status_change *status_get_sc(struct block_list *bl);

int status_get_matk1(struct block_list *bl);
int status_get_matk2(struct block_list *bl);
int status_get_critical(struct block_list *bl);
int status_get_atk_(struct block_list *bl);
int status_get_atk_2(struct block_list *bl);
int status_get_aspd(struct block_list *bl);

// 状態異常関連
int status_can_save(int type);
int status_is_disable(int type,int mask);
int status_change_start(struct block_list *bl,int type,int val1,int val2,int val3,int val4,int tick,int flag);
int status_change_end(struct block_list* bl, int type, int tid);
int status_change_pretimer(struct block_list *bl,int type,int val1,int val2,int val3,int val4,int tick,int flag,int pre_tick);
int status_change_timer(int tid, unsigned int tick, int id, void *data);
int status_change_timer_sub(struct block_list *bl, va_list ap);
int status_change_race_end(struct block_list *bl,int type);
int status_change_soulstart(struct block_list *bl,int val1,int val2,int val3,int val4,int tick,int flag);
int status_change_soulclear(struct block_list *bl);
int status_change_resistclear(struct block_list *bl);
int status_enchant_armor_elemental_end(struct block_list *bl,int type);
int status_enchant_elemental_end(struct block_list *bl,int type);
int status_change_clear(struct block_list *bl,int type);
int status_change_release(struct block_list *bl,int mask);
int status_clearpretimer(struct block_list *bl);
int status_change_attacked_end(struct block_list *bl);
int status_change_hidden_end(struct block_list *bl);
int status_change_removemap_end(struct block_list *bl);
int status_change_rate(struct block_list *bl,int type,int rate,int src_level);

// 状態チェック
int status_check_no_magic_damage(struct block_list *bl);

#ifdef DYNAMIC_SC_DATA
int status_calloc_sc_data(struct status_change *sc);
int status_free_sc_data(struct status_change *sc);
extern struct status_change_data dummy_sc_data[MAX_STATUSCHANGE];
#endif

// ステータス計算
int status_calc_pc(struct map_session_data* sd,int first);
int status_calc_pc_stop_begin(struct block_list *bl);
int status_calc_pc_stop_end(struct block_list *bl);
int status_get_overrefine_bonus(int lv);
int status_percentrefinery(struct map_session_data *sd,struct item *item);
int status_percentrefinery_weaponrefine(struct map_session_data *sd,struct item *item);
extern int current_equip_item_index;
extern int current_equip_name_id;

// DB再読込用
int status_readdb(void);

int do_init_status(void);

// ステータス異常番号テーブル
enum {
	SC_PROVOKE              = 0,
	SC_ENDURE               = 1,
	SC_TWOHANDQUICKEN       = 2,
	SC_CONCENTRATE          = 3,
	SC_HIDING               = 4,
	SC_CLOAKING             = 5,
	SC_ENCPOISON            = 6,
	SC_POISONREACT          = 7,
	SC_QUAGMIRE             = 8,
	SC_ANGELUS              = 9,
	SC_BLESSING             = 10,
	SC_SIGNUMCRUCIS         = 11,
	SC_INCREASEAGI          = 12,
	SC_DECREASEAGI          = 13,
	SC_SLOWPOISON           = 14,
	SC_IMPOSITIO            = 15,
	SC_SUFFRAGIUM           = 16,
	SC_ASPERSIO             = 17,
	SC_BENEDICTIO           = 18,
	SC_KYRIE                = 19,
	SC_MAGNIFICAT           = 20,
	SC_GLORIA               = 21,
	SC_AETERNA              = 22,
	SC_ADRENALINE           = 23,
	SC_WEAPONPERFECTION     = 24,
	SC_OVERTHRUST           = 25,
	SC_MAXIMIZEPOWER        = 26,
	//
	SC_TRICKDEAD            = 29,
	SC_LOUD                 = 30,
	SC_ENERGYCOAT           = 31,
	SC_PK_PENALTY           = 32,
	SC_REVERSEORCISH        = 33,
	SC_HALLUCINATION        = 34,
	//
	SC_SPEEDPOTION0         = 37,
	SC_SPEEDPOTION1         = 38,
	SC_SPEEDPOTION2         = 39,
	SC_SPEEDPOTION3         = 40,
	//
	//
	//
	//
	//
	//
	//
	//
	//
	SC_STRIPWEAPON          = 50,
	SC_STRIPSHIELD          = 51,
	SC_STRIPARMOR           = 52,
	SC_STRIPHELM            = 53,
	SC_CP_WEAPON            = 54,
	SC_CP_SHIELD            = 55,
	SC_CP_ARMOR             = 56,
	SC_CP_HELM              = 57,
	SC_AUTOGUARD            = 58,
	SC_REFLECTSHIELD        = 59,
	SC_DEVOTION             = 60,
	SC_PROVIDENCE           = 61,
	SC_DEFENDER             = 62,
	SC_SANTA                = 63,
	SC_SUMMER               = 64,
	SC_AUTOSPELL            = 65,
	//
	//
	SC_SPEARQUICKEN         = 68,
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	//
	SC_EXPLOSIONSPIRITS     = 86,
	SC_STEELBODY            = 87,
	//
	SC_COMBO                = 89,
	SC_FLAMELAUNCHER        = 90,
	SC_FROSTWEAPON          = 91,
	SC_LIGHTNINGLOADER      = 92,
	SC_SEISMICWEAPON        = 93,
	//
	//
	//
	//
	//
	//
	//
	//
	//
	SC_WE_FEMALE            = 102,
	SC_AURABLADE            = 103,
	SC_PARRYING             = 104,
	SC_CONCENTRATION        = 105,
	SC_TENSIONRELAX         = 106,
	SC_BERSERK              = 107,
	//
	//
	//
	SC_ASSUMPTIO            = 110,
	//
	//
	SC_MAGICPOWER           = 113,
	SC_EDP                  = 114,
	SC_TRUESIGHT            = 115,
	SC_WINDWALK             = 116,
	SC_MELTDOWN             = 117,
	SC_CARTBOOST            = 118,
	SC_CHASEWALK            = 119,
	SC_REJECTSWORD          = 120,
	SC_MARIONETTE           = 121,
	SC_MARIONETTE2          = 122,
	//
	SC_HEADCRUSH            = 124,
	SC_JOINTBEAT            = 125,
	//
	//
	SC_STONE                = 128,
	SC_FREEZE               = 129,
	SC_STUN                 = 130,
	SC_SLEEP                = 131,
	SC_POISON               = 132,
	SC_CURSE                = 133,
	SC_SILENCE              = 134,
	SC_CONFUSION            = 135,
	SC_BLIND                = 136,
	SC_BLEED                = 137,
	//
	//
	SC_SAFETYWALL           = 140,
	SC_PNEUMA               = 141,
	//
	SC_ANKLE                = 143,
	SC_DANCING              = 144,
	SC_KEEPING              = 145,
	SC_BARRIER              = 146,
	SC_TIGERFIST            = 147,
	//
	SC_MAGICROD             = 149,
	SC_SIGHT                = 150,
	SC_RUWACH               = 151,
	SC_AUTOCOUNTER          = 152,
	SC_VOLCANO              = 153,
	SC_DELUGE               = 154,
	SC_VIOLENTGALE          = 155,
	SC_BLADESTOP_WAIT       = 156,
	SC_BLADESTOP            = 157,
	SC_EXTREMITYFIST        = 158,
	SC_GRAFFITI             = 159,
	SC_LULLABY              = 160,
	SC_RICHMANKIM           = 161,
	SC_ETERNALCHAOS         = 162,
	SC_DRUMBATTLE           = 163,
	SC_NIBELUNGEN           = 164,
	SC_ROKISWEIL            = 165,
	SC_INTOABYSS            = 166,
	SC_SIEGFRIED            = 167,
	SC_DISSONANCE           = 168,
	SC_WHISTLE              = 169,
	SC_ASSNCROS             = 170,
	SC_POEMBRAGI            = 171,
	SC_APPLEIDUN            = 172,
	SC_UGLYDANCE            = 173,
	SC_HUMMING              = 174,
	SC_DONTFORGETME         = 175,
	SC_FORTUNE              = 176,
	SC_SERVICE4U            = 177,
	SC_BASILICA             = 178,
	SC_MINDBREAKER          = 179,
	SC_SPIDERWEB            = 180,
	SC_MEMORIZE             = 181,
	SC_DPOISON              = 182,
	//
	SC_SACRIFICE            = 184,
	SC_INCATK               = 185,
	SC_INCMATK              = 186,
	SC_WEDDING              = 187,
	SC_NOCHAT               = 188,
	SC_SPLASHER             = 189,
	SC_SELFDESTRUCTION      = 190,
	SC_MAGNUM               = 191,
	SC_GOSPEL               = 192,
	SC_INCALLSTATUS         = 193,
	SC_INCHIT               = 194,
	SC_INCFLEE              = 195,
	SC_INCMHP2              = 196,
	SC_INCMSP2              = 197,
	SC_INCATK2              = 198,
	SC_INCHIT2              = 199,
	SC_INCFLEE2             = 200,
	SC_PRESERVE             = 201,
	SC_OVERTHRUSTMAX        = 202,
	SC_CHASEWALK_STR        = 203,
	SC_WHISTLE_             = 204,
	SC_ASSNCROS_            = 205,
	SC_POEMBRAGI_           = 206,
	SC_APPLEIDUN_           = 207,
	SC_HUMMING_             = 209,
	SC_DONTFORGETME_        = 210,
	SC_FORTUNE_             = 211,
	SC_SERVICE4U_           = 212,
	SC_BATTLEORDER          = 213,
	SC_REGENERATION         = 214,
	SC_BATTLEORDER_DELAY	= 215,
	SC_REGENERATION_DELAY	= 216,
	SC_RESTORE_DELAY		= 217,
	SC_EMERGENCYCALL_DELAY	= 218,
	SC_POISONPOTION         = 219,
	SC_THE_MAGICIAN         = 220,
	SC_STRENGTH             = 221,
	SC_THE_DEVIL            = 222,
	SC_THE_SUN              = 223,
	SC_MEAL_INCSTR          = 224,
	SC_MEAL_INCAGI          = 225,
	SC_MEAL_INCVIT          = 226,
	SC_MEAL_INCINT          = 227,
	SC_MEAL_INCDEX          = 228,
	SC_MEAL_INCLUK          = 229,
	SC_RUN                  = 230,
	SC_SPURT                = 231,
	SC_TKCOMBO              = 232,
	SC_DODGE                = 233,
	SC_HERMODE              = 234,
	SC_TRIPLEATTACK_RATE_UP = 235,
	SC_COUNTER_RATE_UP      = 236,
	SC_WARM                 = 237,
	SC_SEVENWIND            = 238,
	//
	SC_SUN_COMFORT          = 240,
	SC_MOON_COMFORT         = 241,
	SC_STAR_COMFORT         = 242,
	SC_FUSION               = 243,
	SC_ALCHEMIST            = 244,
	SC_MONK                 = 245,
	SC_STAR                 = 246,
	SC_SAGE                 = 247,
	SC_CRUSADER             = 248,
	SC_SUPERNOVICE          = 249,
	SC_KNIGHT               = 250,
	SC_WIZARD               = 251,
	SC_PRIEST               = 252,
	SC_BARDDANCER           = 253,
	SC_ROGUE                = 254,
	SC_ASSASIN              = 255,
	SC_BLACKSMITH           = 256,
	SC_HUNTER               = 257,
	SC_SOULLINKER           = 258,
	SC_HIGH                 = 259,
	SC_DEATHKINGHT          = 260,
	SC_COLLECTOR            = 261,
	SC_NINJA                = 262,
	SC_GUNNER               = 263,
	SC_KAIZEL               = 264,
	SC_KAAHI                = 265,
	SC_KAUPE                = 266,
	SC_KAITE                = 267,
	SC_SMA                  = 268,
	SC_SWOO                 = 269,
	SC_SKE                  = 270,
	SC_SKA                  = 271,
	SC_ONEHAND              = 272,
	SC_READYSTORM           = 273,
	SC_READYDOWN            = 274,
	SC_READYTURN            = 275,
	SC_READYCOUNTER         = 276,
	SC_DODGE_DELAY          = 277,
	SC_AUTOBERSERK          = 278,
	//
	SC_DOUBLECASTING        = 280,
	SC_ELEMENTFIELD         = 281,
	SC_DARKELEMENT          = 282,
	SC_ATTENELEMENT         = 283,
	SC_MIRACLE              = 284,
	SC_ANGEL                = 285,
	SC_FORCEWALKING         = 286,
	SC_DOUBLE               = 287,
	//
	SC_BABY                 = 289,
	SC_LONGINGFREEDOM       = 290,
	SC_SHRINK               = 291,
	SC_CLOSECONFINE         = 292,
	SC_SIGHTBLASTER         = 293,
	SC_ELEMENTWATER         = 294,
	SC_MEAL_INCHIT          = 295,
	SC_MEAL_INCFLEE         = 296,
	SC_MEAL_INCFLEE2        = 297,
	SC_MEAL_INCCRITICAL     = 298,
	SC_MEAL_INCDEF          = 299,
	SC_MEAL_INCMDEF         = 300,
	SC_MEAL_INCATK          = 301,
	SC_MEAL_INCMATK         = 302,
	SC_MEAL_INCEXP          = 303,
	SC_MEAL_INCJOB          = 304,
	SC_ELEMENTGROUND        = 305,
	SC_ELEMENTFIRE          = 306,
	SC_ELEMENTWIND          = 307,
	SC_WINKCHARM            = 308,
	SC_ELEMENTPOISON        = 309,
	SC_ELEMENTDARK          = 310,
	SC_ELEMENTELEKINESIS    = 311,
	SC_ELEMENTUNDEAD        = 312,
	SC_UNDEADELEMENT        = 313,
	SC_ELEMENTHOLY          = 314,
	SC_NPC_DEFENDER         = 315,
	SC_RESISTWATER          = 316,
	SC_RESISTGROUND         = 317,
	SC_RESISTFIRE           = 318,
	SC_RESISTWIND           = 319,
	SC_RESISTPOISON         = 320,
	SC_RESISTHOLY           = 321,
	SC_RESISTDARK           = 322,
	SC_RESISTTELEKINESIS    = 323,
	SC_RESISTUNDEAD         = 324,
	SC_RESISTALL            = 325,
	SC_RACEUNKNOWN          = 326,
	SC_RACEUNDEAD           = 327,
	SC_RACEBEAST            = 328,
	SC_RACEPLANT            = 329,
	SC_RACEINSECT           = 330,
	SC_RACEFISH             = 332,
	SC_RACEDEVIL            = 333,
	SC_RACEHUMAN            = 334,
	SC_RACEANGEL            = 335,
	SC_RACEDRAGON           = 336,
	SC_TIGEREYE             = 337,
	SC_GRAVITATION_USER     = 338,
	SC_GRAVITATION          = 339,
	SC_FOGWALL              = 340,
	SC_FOGWALLPENALTY       = 341,
	//
	//
	SC_STOP                 = 344,
	SC_INVISIBLE            = 345,
	SC_MODECHANGE           = 346,
	SC_FLING                = 347,
	SC_MADNESSCANCEL        = 348,
	SC_ADJUSTMENT           = 349,
	SC_INCREASING           = 350,
	SC_DISARM               = 351,
	SC_GATLINGFEVER         = 352,
	SC_FULLBUSTER           = 353,
	SC_TATAMIGAESHI         = 354,
	SC_UTSUSEMI             = 355,
	SC_BUNSINJYUTSU         = 356,
	SC_SUITON               = 357,
	SC_NEN                  = 358,
	SC_AVOID                = 359,
	SC_CHANGE               = 360,
	SC_DEFENCE              = 361,
	SC_BLOODLUST            = 362,
	SC_FLEET                = 363,
	SC_SPEED                = 364,
	SC_ADRENALINE2          = 365,
	SC_STATUS_UNCHANGE      = 366,
	SC_INCDAMAGE            = 367,
	SC_COMBATHAN            = 368,
	SC_LIFEINSURANCE        = 369,
	SC_ITEMDROPRATE         = 370,
	SC_BOSSMAPINFO          = 371,
	SC_MEAL_INCSTR2         = 372,
	SC_MEAL_INCAGI2         = 373,
	SC_MEAL_INCVIT2         = 374,
	SC_MEAL_INCINT2         = 375,
	SC_MEAL_INCDEX2         = 376,
	SC_MEAL_INCLUK2         = 377,
	SC_SLOWCAST             = 378,
	SC_CRITICALWOUND        = 379,
	SC_MAGICMIRROR          = 380,
	SC_STONESKIN            = 381,
	SC_ANTIMAGIC            = 382,
	SC_WEAPONQUICKEN        = 383,
	SC_HAPPY                = 384,
	SC_NATURAL_HEAL_STOP    = 385,
	SC_REBIRTH              = 386,
	SC_HELLPOWER			= 387,
	SC_ENCHANTBLADE			= 388,
	SC_BERKANA				= 389,
	SC_NAUTHIZ				= 390,
	SC_TURISUSS				= 391,
	SC_HAGALAZ				= 392,
	SC_ISHA					= 393,
	SC_EISIR				= 394,
	SC_URUZ					= 395,
	SC_DEATHBOUND			= 396,
	SC_FEAR					= 397,
	SC_VENOMIMPRESS			= 398,
	SC_POISONINGWEAPON		= 399,
	SC_WEAPONBLOCKING		= 400,
	SC_WEAPONBLOCKING2		= 401,
	SC_CLOAKINGEXCEED		= 402,
	SC_HALLUCINATIONWALK	= 403,
	SC_HALLUCINATIONWALK2	= 404,
	SC_ROLLINGCUTTER		= 405,
	SC_TOXIN				= 406,
	SC_PARALIZE				= 407,
	SC_VENOMBLEED			= 408,
	SC_MAGICMUSHROOM		= 409,
	SC_DEATHHURT			= 410,
	SC_PYREXIA				= 411,
	SC_OBLIVIONCURSE		= 412,
	SC_LEECHEND				= 413,
	SC_EPICLESIS			= 414,
	SC_ORATIO				= 415,
	SC_LAUDAAGNUS			= 416,
	SC_LAUDARAMUS			= 417,
	SC_RENOVATIO			= 418,
	SC_EXPIATIO				= 419,
	SC_DUPLELIGHT			= 420,
	SC_SACRAMENT			= 421,
	SC_WHITEIMPRISON		= 422,
	SC_FROSTMISTY			= 423,
	SC_MARSHOFABYSS			= 424,
	SC_RECOGNIZEDSPELL		= 425,
	SC_STASIS				= 426,
	SC_HELLINFERNO			= 427,
	SC_SUMMONBALL1			= 428,
	SC_SUMMONBALL2			= 429,
	SC_SUMMONBALL3			= 430,
	SC_SUMMONBALL4			= 431,
	SC_SUMMONBALL5			= 432,
	SC_SPELLBOOK			= 433,
	SC_FEARBREEZE			= 434,
	SC_ELECTRICSHOCKER		= 435,
	SC_WUGDASH				= 436,
	SC_WUGBITE				= 437,
	SC_CAMOUFLAGE			= 438,
	SC_ACCELERATION			= 439,
	SC_HOVERING				= 440,
	SC_OVERHEAT				= 441,
	SC_SHAPESHIFT			= 442,
	SC_INFRAREDSCAN			= 443,
	SC_ANALYZE				= 444,
	SC_MAGNETICFIELD		= 445,
	SC_NEUTRALBARRIER_USER	= 446,
	SC_NEUTRALBARRIER		= 447,
	SC_STEALTHFIELD_USER	= 448,
	SC_STEALTHFIELD			= 449,
	SC_MANU_ATK				= 450,
	SC_MANU_DEF				= 451,
	SC_MANU_MATK			= 452,
	SC_SPL_ATK				= 453,
	SC_SPL_DEF				= 454,
	SC_SPL_MATK				= 455,

	// startでは使えないresistをアイテム側で全てクリアするための物
	SC_RESISTCLEAR          = 1001,
	SC_RACECLEAR            = 1002,
	SC_SOUL                 = 1003,
	SC_SOULCLEAR            = 1004,
};

// 状態アイコン
// 厳密にはキャラクターの色の変化なども含まれている(爆裂波動など)
enum {
	SI_BLANK            = 43,

	SI_PROVOKE          = 0,
	SI_ENDURE           = 1,
	SI_TWOHANDQUICKEN   = 2,
	SI_CONCENTRATE      = 3,
	SI_HIDING           = 4,
	SI_CLOAKING         = 5,
	SI_ENCPOISON        = 6,
	SI_POISONREACT      = 7,
	SI_QUAGMIRE         = 8,
	SI_ANGELUS          = 9,
	SI_BLESSING         = 10,
	SI_SIGNUMCRUCIS     = 11,
	SI_INCREASEAGI      = 12,
	SI_DECREASEAGI      = 13,
	SI_SLOWPOISON       = 14,
	SI_IMPOSITIO        = 15,
	SI_SUFFRAGIUM       = 16,
	SI_ASPERSIO         = 17,
	SI_BENEDICTIO       = 18,
	SI_KYRIE            = 19,
	SI_MAGNIFICAT       = 20,
	SI_GLORIA           = 21,
	SI_AETERNA          = 22,
	SI_ADRENALINE       = 23,
	SI_WEAPONPERFECTION = 24,
	SI_OVERTHRUST       = 25,
	SI_MAXIMIZEPOWER    = 26,
	SI_RIDING           = 27,
	SI_FALCON           = 28,
	SI_TRICKDEAD        = 29,
	SI_LOUD             = 30,
	SI_ENERGYCOAT       = 31,
	SI_BREAKARMOR       = 32,
	SI_BREAKWEAPON      = 33,
	SI_HALLUCINATION    = 34,
	SI_WEIGHT50         = 35,
	SI_WEIGHT90         = 36,
	SI_SPEEDPOTION0     = 37,
	SI_SPEEDPOTION1     = 38,
	SI_SPEEDPOTION2     = 39,
	SI_SPEEDPOTION3     = 40,
	SI_INCREASEAGI2     = 41,
	SI_INCREASEAGI3     = 42,
	//SI_AUTOCOUNTER    = 43,
	//SI_SPLASHER       = 44,
	//SI_ANKLESNARE     = 45,
	SI_ACTIONDELAY      = 46,
	//SI_NOACTION       = 47,
	//SI_IMPOSSIBLEPICKUP=48,
	//SI_BARRIER        = 49,
	SI_STRIPWEAPON      = 50,
	SI_STRIPSHIELD      = 51,
	SI_STRIPARMOR       = 52,
	SI_STRIPHELM        = 53,
	SI_CP_WEAPON        = 54,
	SI_CP_SHIELD        = 55,
	SI_CP_ARMOR         = 56,
	SI_CP_HELM          = 57,
	SI_AUTOGUARD        = 58,
	SI_REFLECTSHIELD    = 59,
	SI_DEVOTION         = 60,
	SI_PROVIDENCE       = 61,
	SI_DEFENDER         = 62,
	//SI_MAGICROD       = 63,
	//SI_WEAPONPROPERTY = 64,
	SI_AUTOSPELL        = 65,
	//SI_SPECIALZONE    = 66,
	//SI_MASK           = 67,
	SI_SPEARQUICKEN     = 68,
	//SI_BDPLAYING      = 69,
	//SI_WHISTLE        = 70,
	//SI_ASSASSINCROSS  = 71,
	//SI_POEMBRAGI      = 72,
	//SI_APPLEIDUN      = 73,
	//SI_HUMMING        = 74,
	//SI_DONTFORGETME   = 75,
	//SI_FORTUNEKISS    = 76,
	//SI_SERVICEFORYOU  = 77,
	//SI_RICHMANKIM     = 78,
	//SI_ETERNALCHAOS   = 79,
	//SI_DRUMBATTLEFIELD= 80,
	//SI_RINGNIBELUNGEN = 81,
	//SI_ROKISWEIL      = 82,
	//SI_INTOABYSS      = 83,
	//SI_SIEGFRIED      = 84,
	//SI_BLADESTOP      = 85,
	SI_EXPLOSIONSPIRITS = 86,
	SI_STEELBODY        = 87,
	SI_COMBO            = 89,
	SI_FLAMELAUNCHER    = 90,
	SI_FROSTWEAPON      = 91,
	SI_LIGHTNINGLOADER  = 92,
	SI_SEISMICWEAPON    = 93,
	//SI_MAGICATTACK    = 94,
	SI_STOP             = 95,
	//SI_WEAPONBRAKER   = 96,
	SI_UNDEAD           = 97,
	//SI_POWERUP        = 98,
	//SI_AGIUP          = 99,
	//SI_SIEGEMODE      = 100,
	//SI_INVISIBLE      = 101,
	SI_WE_FEMALE        = 102,
	SI_AURABLADE        = 103,
	SI_PARRYING         = 104,
	SI_CONCENTRATION    = 105,
	SI_TENSIONRELAX     = 106,
	SI_BERSERK          = 107,
	//SI_SACRIFICE      = 108,
	//SI_GOSPEL         = 109,
	SI_ASSUMPTIO        = 110,
	//SI_BASILICA       = 111,
	SI_ELEMENTFIELD     = 112,
	SI_MAGICPOWER       = 113,
	SI_EDP              = 114,
	SI_TRUESIGHT        = 115,
	SI_WINDWALK         = 116,
	SI_MELTDOWN         = 117,
	SI_CARTBOOST        = 118,
	SI_CHASEWALK        = 119,
	SI_REJECTSWORD      = 120,
	SI_MARIONETTE       = 121,
	SI_MARIONETTE2      = 122,
	SI_MOONLIT          = 123,
	SI_HEADCRUSH        = 124,
	SI_JOINTBEAT        = 125,
	//SI_MINDBREAKER    = 126,
	//SI_MEMORIZE       = 127,
	//SI_FOGWALL        = 128,
	//SI_SPIDERWEB      = 129,
	SI_BABY             = 130,
	SI_MAGNUM           = 131,
	SI_AUTOBERSERK      = 132,
	SI_RUN              = 133,
	SI_RUN_STOP         = 134,
	SI_READYSTORM       = 135,
	//SI_STORMKICK_READY= 136,
	SI_READYDOWN        = 137,
	//SI_DOWNKICK_READY = 138,
	SI_READYTURN        = 139,
	//SI_TURNKICK_READY = 140,
	SI_READYCOUNTER     = 141,
	//SI_COUNTER_READY  = 142,
	SI_DODGE            = 143,
	//SI_DODGE_READY    = 144,
	SI_SPURT            = 145,
	SI_DARKELEMENT      = 146,
	SI_ADRENALINE2      = 147,
	SI_ATTENELEMENT     = 148,
	SI_SOULLINK         = 149,
	//SI_PLUSATTACKPOWER= 150,
	//SI_PLUSMAGICPOWER = 151,
	SI_DEVIL            = 152,
	SI_KAITE            = 153,
	//SI_SWOO           = 154,
	//SI_STAR2          = 155,
	SI_KAIZEL           = 156,
	SI_KAAHI            = 157,
	SI_KAUPE            = 158,
	SI_SMA              = 159,
	SI_MIRACLE          = 160,
	SI_ONEHAND          = 161,
	//SI_FRIEND         = 162,
	//SI_FRIENDUP       = 163,
	//SI_SG_WARM        = 164,
	SI_WARM             = 165,
	//SI_WARM           = 166,
	//SI_WARM           = 167,
	//SI_EMOTION        = 168,
	SI_SUN_COMFORT      = 169,
	SI_MOON_COMFORT     = 170,
	SI_STAR_COMFORT     = 171,
	//SI_EXPUP          = 172,
	//SI_G_BATTLEORDER  = 173,
	//SI_G_REGENERATION = 174,
	//SI_G_POSTDELAY    = 175,
	//SI_RESISTHANDICAP = 176,
	//SI_MAXHPPERCENT   = 177,
	//SI_MAXSPPERCENT   = 178,
	//SI_DEFENCE        = 179,
	//SI_SLOWDOWN       = 180,
	SI_PRESERVE         = 181,
	SI_CHASEWALK_STR    = 182,
	//SI_NOTEXTREMITYFIST=183,
	SI_TIGEREYE         = 184,
	//SI_MOVESLOW_POTION= 185,
	SI_DOUBLECASTING    = 186,
	//SI_GRAVITATION    = 187,
	SI_OVERTHRUSTMAX    = 188,
	//SI_LONGING        = 189,
	//SI_HERMODE        = 190,
	SI_TAROTCARD        = 191,
	//SI_HLIF_AVOID     = 192,
	//SI_HFLI_FLEET     = 193,
	//SI_HFLI_SPEED     = 194,
	//SI_HLIF_CHANGE    = 195,
	//SI_HAMI_BLOODLUST = 196,
	SI_SHRINK           = 197,
	SI_SIGHTBLASTER     = 198,
	//SI_WINKCHARM      = 199,
	SI_CLOSECONFINE     = 200,
	SI_CLOSECONFINE2    = 201,
	//SI_DISABLEMOVE    = 202,
	SI_MADNESSCANCEL    = 203,
	SI_GATLINGFEVER     = 204,
	SI_HAPPY            = 205,
	SI_UTSUSEMI         = 206,
	SI_BUNSINJYUTSU     = 207,
	SI_NEN              = 208,
	SI_ADJUSTMENT       = 209,
	SI_INCREASING       = 210,
	//SI_NJ_SUITON      = 211,
	//SI_PET            = 212,
	//SI_MENTAL         = 213,
	//SI_EXPMEMORY      = 214,
	//SI_PERFORMANCE    = 215,
	//SI_GAIN           = 216,
	//SI_GRIFFON        = 217,
	//SI_DRIFT          = 218,
	//SI_WALLSHIFT      = 219,
	//SI_REINCARNATION  = 220,
	//SI_PATTACK        = 221,
	//SI_PSPEED         = 222,
	//SI_PDEFENSE       = 223,
	//SI_PCRITICAL      = 224,
	//SI_RANKING        = 225,
	//SI_PTRIPLE        = 226,
	//SI_DENERGY        = 227,
	//SI_WAVE1          = 228,
	//SI_WAVE2          = 229,
	//SI_WAVE3          = 230,
	//SI_WAVE4          = 231,
	//SI_DAURA          = 232,
	//SI_DFREEZER       = 233,
	//SI_DPUNISH        = 234,
	//SI_DBARRIER       = 235,
	//SI_DWARNING       = 236,
	//SI_MOUSEWHEEL     = 237,
	//SI_DGAUGE         = 238,
	//SI_DACCEL         = 239,
	//SI_DBLOCK         = 240,
	SI_MEAL_INCSTR      = 241,
	SI_MEAL_INCAGI      = 242,
	SI_MEAL_INCVIT      = 243,
	SI_MEAL_INCDEX      = 244,
	SI_MEAL_INCINT      = 245,
	SI_MEAL_INCLUK      = 246,
	SI_MEAL_INCFLEE     = 247,
	SI_MEAL_INCHIT      = 248,
	SI_MEAL_INCCRITICAL = 249,
	SI_COMBATHAN        = 250,
	SI_LIFEINSURANCE    = 251,
	SI_ITEMDROPRATE     = 252,
	SI_BOSSMAPINFO      = 253,
	//SI_DA_ENERGY      = 254,
	//SI_DA_FIRSTSLOT   = 255,
	//SI_DA_HEADDEF     = 256,
	//SI_DA_SPACE       = 257,
	//SI_DA_TRANSFORM   = 258,
	//SI_DA_ITEMREBUILD = 259,
	//SI_DA_ILLUSION    = 260,
	//SI_DA_DARKPOWER   = 261,
	//SI_DA_EARPLUG     = 262,
	//SI_DA_CONTRACT    = 263,
	//SI_DA_BLACK       = 264,
	//SI_DA_MAGICCART   = 265,
	//SI_CRYSTAL        = 266,
	//SI_DA_REBUILD     = 267,
	//SI_DA_EDARKNESS   = 268,
	//SI_DA_EGUARDIAN   = 269,
	//SI_DA_TIMEOUT     = 270,
	SI_MEAL_INCSTR2     = 271,
	SI_MEAL_INCAGI2     = 272,
	SI_MEAL_INCVIT2     = 273,
	SI_MEAL_INCINT2     = 274,
	SI_MEAL_INCDEX2     = 275,
	SI_MEAL_INCLUK2     = 276,
	//SI_MER_FLEE       = 277,
	//SI_MER_ATK        = 278,
	//SI_MER_HP         = 279,
	//SI_MER_SP         = 280,
	//SI_MER_HIT        = 281,
	SI_SLOWCAST         = 282,
	//SI_MAGICMIRROR    = 283,
	//SI_STONESKIN      = 284,
	//SI_ANTIMAGIC      = 285,
	SI_CRITICALWOUND    = 286,
	//SI_NPC_DEFENDER   = 287,
	//SI_NOACTION_WAIT  = 288,
	//SI_MOVHASTE_HORSE = 289,
	//SI_PROTECT_DEF    = 290,
	//SI_PROTECT_MDEF   = 291,
	//SI_HEALPLUS       = 292,
	//SI_S_LIFEPOTION   = 293,
	//SI_L_LIFEPOTION   = 294,
	//SI_CRITICALPERCENT= 295,
	//SI_PLUSAVOIDVALUE = 296,
	//SI_ATKER_ASPD     = 297,
	//SI_TARGET_ASPD    = 298,
	//SI_ATKER_MOVESPEED= 299,
	//SI_ATKER_BLOOD    = 300,
	//SI_TARGET_BLOOD   = 301,
	//SI_ARMOR_PROPERTY = 302,
	//SI_REUSE_LIMIT_A  = 303,
	SI_HELLPOWER        = 304,
	//SI_STEAMPACK      = 305,
	//SI_REUSE_LIMIT_B  = 306,
	//SI_REUSE_LIMIT_C  = 307,
	//SI_REUSE_LIMIT_D  = 308,
	//SI_REUSE_LIMIT_E  = 309,
	//SI_REUSE_LIMIT_F  = 310,
	//SI_INVINCIBLE     = 311,
	//SI_CPLUSONLYJOBEXP= 312,
	//SI_PARTYFLEE      = 313,
	//SI_ANGEL_PROTECT  = 314,
	//SI_ENDURE_MDEF    = 315,
	SI_ENCHANTBLADE		= 316,
	//SI_DEATHBOUND     = 317,
	SI_NAUTHIZ			= 318,
	SI_THURISSUS		= 319,
	SI_HAGALAZ			= 320,
	SI_ISA				= 321,
	SI_OTHILA			= 322,
	SI_URUZ				= 323,
	//
	SI_VENOMIMPRESS		= 328,
	//
	SI_ORATIO			= 330,
	//
	SI_POISONINGWEAPON	= 333,
	SI_HALLUCINATIONWALK= 334,
	//
	SI_RENOVATIO		= 336,
	SI_WEAPONBLOCKING	= 337,
	//
	SI_ROLLINGCUTTER	= 339,
	//
	SI_TOXIN			= 342,
	SI_PARALIZE			= 343,
	SI_VENOMBLEED		= 344,
	SI_MAGICMUSHROOM	= 345,
	SI_DEATHHURT		= 346,
	SI_PYREXIA			= 347,
	SI_OBLIVIONCURSE	= 348,
	SI_LEECHEND			= 349,
	SI_DUPLELIGHT		= 350,
	SI_FROSTMISTY		= 351,
	SI_FEARBREEZE		= 352,
	//
	SI_MARSHOFABYSS		= 354,
	SI_RECOGNIZEDSPELL	= 355,
	//
	SI_WUGRIDER			= 357,
	SI_WUGDASH			= 358,
	//
	SI_CAMOUFLAGE		= 360,
	SI_ACCELERATION		= 361,
	SI_HOVERING			= 362,
	SI_SUMMONBALL1		= 363,
	SI_SUMMONBALL2		= 364,
	SI_SUMMONBALL3		= 365,
	SI_SUMMONBALL4		= 366,
	SI_SUMMONBALL5		= 367,
	//
	SI_OVERHEAT			= 373,
	SI_SHAPESHIFT		= 374,
	SI_INFRAREDSCAN		= 375,
	//
	SI_NEUTRALBARRIER	= 377,
	//
	SI_STEALTHFIELD		= 379,
	//
	SI_MANU_ATK			= 381,
	SI_MANU_DEF			= 382,
	SI_SPL_ATK			= 383,
	SI_SPL_DEF			= 384,
	//
	SI_MANU_MATK		= 386,
	SI_SPL_MATK			= 387,
	//
	SI_EXPIATIO			= 414,
	//
	SI_ANALYZE			= 450,
	//
	SI_SACRAMENT		= 472
};

// opt1テーブル
enum {
	OPT1_NORMAL			= 0x0,
	OPT1_STONECURSE 	= 0x1,
	OPT1_FREEZING		= 0x2,
	OPT1_STUN			= 0x3,
	OPT1_SLEEP			= 0x4,
	OPT1_UNDEAD			= 0x5,
	OPT1_STONECURSE_ING	= 0x6,
	OPT1_BURNNING		= 0x7,
	OPT1_IMPRISON		= 0x8,
};

// opt2テーブル
enum {
	OPT2_NORMAL			= 0x0,
	OPT2_POISON			= 0x1,
	OPT2_CURSE			= 0x2,
	OPT2_SILENCE		= 0x4,
	OPT2_CONFUSION		= 0x8,
	OPT2_BLIND			= 0x10,
	OPT2_ANGELUS		= 0x20,
	OPT2_BLOODING		= 0x40,
	OPT2_HEAVYPOISON	= 0x80,
	OPT2_FEAR			= 0x100,
};

// opt3テーブル
enum {
	OPT3_NORMAL				= 0x0,
	OPT3_QUICKEN			= 0x1,
	OPT3_OVERTHRUST			= 0x2,
	OPT3_ENERGYCOAT			= 0x4,
	OPT3_EXPLOSIONSPIRITS	= 0x8,
	OPT3_STEELBODY			= 0x10,
	OPT3_BLADESTOP			= 0x20,
	OPT3_AURABLADE			= 0x40,
	OPT3_REDBODY			= 0x80,
	OPT3_LIGHTBLADE			= 0x100,
	OPT3_MOON				= 0x200,
	OPT3_PINKBODY			= 0x400,
	OPT3_ASSUMPTIO			= 0x800,
	OPT3_SUN_WARM			= 0x1000,
	OPT3_REFLECT			= 0x2000,
	OPT3_BUNSIN				= 0x4000,
	OPT3_SOULLINK			= 0x8000,
	OPT3_UNDEAD				= 0x10000,
	OPT3_CONTRACT			= 0x20000,
};

// オプションテーブル
enum {
	OPTION_NOTHING			= 0x0,
	OPTION_SIGHT			= 0x1,
	OPTION_HIDE				= 0x2,
	OPTION_CLOAKING			= 0x4,
	OPTION_PUSHCART			= 0x8,
	OPTION_BIRD				= 0x10,
	OPTION_CHICKEN			= 0x20,
	OPTION_SPECIALHIDING	= 0x40,
	OPTION_PUSHCART2		= 0x80,
	OPTION_PUSHCART3		= 0x100,
	OPTION_PUSHCART4		= 0x200,
	OPTION_PUSHCART5		= 0x400,
	OPTION_CARTMASK			= 0x788,
	OPTION_ORCFACE			= 0x800,
	OPTION_MARRIED			= 0x1000,
	OPTION_RUWACH			= 0x2000,
	OPTION_FOOTPRINT		= 0x4000,
	OPTION_STAR2			= 0x8000,
	OPTION_SANTA			= 0x10000,
	OPTION_TRANSFORM		= 0x20000,
	OPTION_SUMMER			= 0x40000,
	OPTION_DRAGON			= 0x80000,
	OPTION_WUG				= 0x100000,
	OPTION_WUGRIDER			= 0x200000,
	OPTION_MADOGEAR			= 0x400000,
	OPTION_DRAGON2			= 0x800000,
	OPTION_DRAGON3			= 0x1000000,
	OPTION_DRAGON4			= 0x2000000,
	OPTION_DRAGON5			= 0x4000000,
	OPTION_DRAGONMASK		= 0x7880000,
	OPTION_MASK				= 0x7f8d7b8,
};

#endif
