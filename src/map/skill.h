#ifndef _SKILL_H_
#define _SKILL_H_

#include "map.h"
#include "mmo.h"

#define MAX_SKILL_DB			MAX_SKILL
#define MAX_HOMSKILL_DB			MAX_HOMSKILL
#define MAX_GUILDSKILL_DB		MAX_GUILDSKILL
#define MAX_SKILL_PRODUCE_DB	300
#define MAX_PRODUCE_RESOURCE	10
#define MAX_SKILL_ARROW_DB	 150
#define MAX_SKILL_ABRA_DB	 350

// �X�L���f�[�^�x�[�X
struct skill_db {
	int range[MAX_SKILL_LEVEL],hit,inf,pl,nk,max;
	int num[MAX_SKILL_LEVEL];
	int cast[MAX_SKILL_LEVEL],fixedcast[MAX_SKILL_LEVEL],delay[MAX_SKILL_LEVEL];
	int upkeep_time[MAX_SKILL_LEVEL],upkeep_time2[MAX_SKILL_LEVEL];
	int castcancel,cast_def_rate;
	int inf2,maxcount[MAX_SKILL_LEVEL],skill_type;
	int blewcount[MAX_SKILL_LEVEL];
	int hp[MAX_SKILL_LEVEL],sp[MAX_SKILL_LEVEL],hp_rate[MAX_SKILL_LEVEL],sp_rate[MAX_SKILL_LEVEL],zeny[MAX_SKILL_LEVEL];
	int weapon,state,spiritball[MAX_SKILL_LEVEL],coin[MAX_SKILL_LEVEL],arrow_cost[MAX_SKILL_LEVEL],arrow_type;
	int itemid[10],amount[10];
	int unit_id[4];
	int unit_layout_type[MAX_SKILL_LEVEL];
	int unit_range;
	int unit_interval;
	int unit_target;
	int unit_flag;
	int cloneable;
	int misfire;
	int zone;
	int damage_rate[4];
};

#define MAX_SKILL_UNIT_LAYOUT	50
#define MAX_SQUARE_LAYOUT		5	// 11*11�̃��j�b�g�z�u���ő�
#define MAX_SKILL_UNIT_COUNT ((MAX_SQUARE_LAYOUT*2+1)*(MAX_SQUARE_LAYOUT*2+1))
struct skill_unit_layout {
	int count;
	int dx[MAX_SKILL_UNIT_COUNT];
	int dy[MAX_SKILL_UNIT_COUNT];
};

enum {
	UF_DEFNOTENEMY		= 0x0001,	// defnotenemy �ݒ��BCT_NOENEMY�ɐ؂�ւ�
	UF_NOREITERATION	= 0x0002,	// �d���u���֎~
	UF_NOFOOTSET		= 0x0004,	// �����u���֎~
	UF_NOOVERLAP		= 0x0008,	// ���j�b�g���ʂ��d�����Ȃ�
	UF_PATHCHECK		= 0x0010,	// �I�u�W�F�N�g�������Ɏː��`�F�b�N
	UF_DANCE			= 0x0100,	// �_���X�X�L��
	UF_ENSEMBLE			= 0x0200,	// ���t�X�L��
};

extern struct skill_db skill_db[MAX_SKILL_DB+MAX_HOMSKILL_DB+MAX_GUILDSKILL_DB];

// �A�C�e���쐬�f�[�^�x�[�X
struct skill_produce_db {
	int nameid, trigger;
	int req_skill,req_skilllv,itemlv;
	int mat_id[MAX_PRODUCE_RESOURCE],mat_amount[MAX_PRODUCE_RESOURCE];
};
extern struct skill_produce_db skill_produce_db[MAX_SKILL_PRODUCE_DB];

// ��쐬�f�[�^�x�[�X
struct skill_arrow_db {
	int nameid, trigger;
	int cre_id[5],cre_amount[5];
};
extern struct skill_arrow_db skill_arrow_db[MAX_SKILL_ARROW_DB];

// �A�u���J�_�u���f�[�^�x�[�X
struct skill_abra_db {
	int nameid;
	int req_lv;
	int per;
};
extern struct skill_abra_db skill_abra_db[MAX_SKILL_ABRA_DB];

int do_init_skill(void);

//
int GetSkillStatusChangeTable(int id);
//�}�N������֐���
//�M���hID���g�p�\��
int skill_get_skilldb_id(int id);
int skill_get_hit(int id);
int skill_get_inf(int id);
int skill_get_pl(int id);
int skill_get_nk(int id);
int skill_get_max(int id);
int skill_get_range(int id,int lv);
int skill_get_hp(int id,int lv);
int skill_get_sp(int id,int lv);
int skill_get_zeny(int id,int lv);
int skill_get_num(int id,int lv);
int skill_get_cast(int id,int lv);
int skill_get_fixedcast(int id ,int lv);
int skill_get_delay(int id,int lv);
int skill_get_time(int id ,int lv);
int skill_get_time2(int id,int lv);
int skill_get_castdef(int id);
int skill_get_weapontype(int id);
int skill_get_inf2(int id);
int skill_get_maxcount(int id,int lv);
int skill_get_blewcount(int id,int lv);
int skill_get_unit_id(int id,int flag);
int skill_get_unit_layout_type(int id,int lv);
int skill_get_unit_interval(int id);
int skill_get_unit_range(int id);
int skill_get_unit_target(int id);
int skill_get_unit_flag(int id);
int skill_get_arrow_cost(int id,int lv);
int skill_get_arrow_type(int id);
int skill_get_cloneable(int id);
int skill_get_misfire(int id);
int skill_get_zone(int id);
int skill_get_damage_rate(int id,int type);

// �X�L���̎g�p
void skill_castend_map(struct map_session_data *sd, int skill_num, const char *map);

int skill_cleartimerskill(struct block_list *src);
int skill_addtimerskill(struct block_list *src,unsigned int tick,int target,int x,int y,int skill_id,int skill_lv,int type,int flag);

// �ǉ�����
int skill_additional_effect( struct block_list* src, struct block_list *bl,int skillid,int skilllv,int attack_type,unsigned int tick);

enum {	//������΂��t���O
	SAB_NOMALBLOW   = 0x00000,
	SAB_REVERSEBLOW = 0x10000,
	SAB_NODAMAGE    = 0x20000,
	SAB_NOPATHSTOP  = 0x40000,
};

#define	SAB_NORMAL	0x00010000
#define	SAB_SKIDTRAP	0x00020000
int skill_add_blown( struct block_list *src, struct block_list *target,int skillid,int flag);

//�J�[�h���ʂ̃I�[�g�X�y��
#define AS_ATTACK	0x00050003
#define AS_REVENGE	0x00060003
int skill_use_bonus_autospell(struct block_list * src,struct block_list * bl,int skill_id,int skill_lv,int rate,long skill_flag,int tick,int flag);
int skill_bonus_autospell(struct block_list * src,struct block_list * bl,long mode,int tick,int flag);

// ���j�b�g�X�L��
struct skill_unit *skill_initunit(struct skill_unit_group *group,int idx,int x,int y);
int skill_delunit(struct skill_unit *unit);
struct skill_unit_group *skill_initunitgroup(struct block_list *src,int count,int skillid,int skilllv,int unit_id);
int skill_delunitgroup(struct skill_unit_group *group);
int skill_clear_unitgroup(struct block_list *src);

int skill_unit_ondamaged(struct skill_unit *src,struct block_list *bl,int damage,unsigned int tick);

int skill_castfix( struct block_list *bl, int time );
int skill_delayfix( struct block_list *bl, int time, int cast );
int skill_check_unit_range(int m,int x,int y,int skillid, int skilllv);
int skill_check_unit_range2(int m,int x,int y,int skillid, int skilllv);
int skill_unit_move(struct block_list *bl,unsigned int tick,int flag);
int skill_unit_move_unit_group( struct skill_unit_group *group, int m,int dx,int dy);

int skill_hermode_wp_check(struct block_list *bl,int range);

struct skill_unit_group *skill_check_dancing( struct block_list *src );
void skill_stop_dancing(struct block_list *src, int flag);
void skill_stop_gravitation(struct block_list *src);

// �r���L�����Z��
int skill_castcancel(struct block_list *bl,int type);

int skill_gangsterparadise(struct map_session_data *sd ,int type);
void skill_brandishspear_first(struct square *tc,int dir,int x,int y);
void skill_brandishspear_dir(struct square *tc,int dir,int are);
void skill_autospell(struct map_session_data *sd, int skillid);
void skill_devotion(struct map_session_data *md,int target);
void skill_devotion2(struct block_list *bl,int crusader);
int skill_devotion3(struct map_session_data *sd,int target);
void skill_devotion_end(struct map_session_data *md,struct map_session_data *sd,int target);
int skill_marionette(struct map_session_data *sd,int target);
void skill_marionette2(struct map_session_data *sd,int src);
int skill_tarot_card_of_fate(struct block_list *src,struct block_list *target,int skillid,int skilllv,int tick,int flag,int wheel);

#define skill_calc_heal(bl,skill_lv) (( status_get_lv(bl)+status_get_int(bl) )/8 *(4+ skill_lv*8))
int skill_castend_id( int tid, unsigned int tick, int id,int data );
int skill_castend_pos( int tid, unsigned int tick, int id,int data );

// ���̑�
int skill_check_cloaking(struct block_list *bl);

// �X�L���g�p���ǂ����̔���B

// ����֐��ɓn���\���́B�֐������Ńf�[�^���㏑�������̂ŁA
// �߂�����ɕύX����̂�Y��Ȃ��悤�ɁB
struct skill_condition {
	int id;
	int lv;
	int x;
	int y;
	int target;
};

int skill_check_condition(struct block_list *bl, int type);
int skill_check_condition2(struct block_list *bl, struct skill_condition *sc, int type);

// �A�C�e���쐬
int skill_can_produce_mix( struct map_session_data *sd, int nameid, int trigger );
void skill_produce_mix(struct map_session_data *sd, int nameid, int slot1, int slot2, int slot3);
int skill_am_twilight1(struct map_session_data* sd);
int skill_am_twilight2(struct map_session_data* sd);
int skill_am_twilight3(struct map_session_data* sd);

void skill_arrow_create(struct map_session_data *sd, int nameid);
int skill_can_repair( struct map_session_data *sd, int nameid );
int skill_repair_weapon(struct map_session_data *sd, int idx);

// mob�X�L���̂���
int skill_castend_nodamage_id( struct block_list *src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag );
int skill_castend_damage_id( struct block_list* src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag );
int skill_castend_pos2( struct block_list *src, int x,int y,int skillid,int skilllv,unsigned int tick,int flag);

int skill_cloneable(int skillid);
int skill_upperskill(int skillid);
int skill_mobskill(int skillid);
int skill_abraskill(int skillid);
int skill_clone(struct map_session_data* sd,int skillid,int skilllv);


void skill_weapon_refine(struct map_session_data *sd, int idx);
int skill_success_weaponrefine(struct map_session_data *sd,int idx);
int skill_fail_weaponrefine(struct map_session_data *sd,int idx);

// �X�L���U���ꊇ����
int skill_blown( struct block_list *src, struct block_list *target,int count);

int skill_castend_delay (struct block_list* src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag);

// �o�V���J������~
void skill_basilica_cancel( struct block_list *bl );

void skill_reload(void);

enum {
	ST_NONE,ST_HIDING,ST_CLOAKING,ST_HIDDEN,ST_RIDING,ST_FALCON,ST_CART,ST_SHIELD,ST_SIGHT,ST_EXPLOSIONSPIRITS,
	ST_RECOV_WEIGHT_RATE,ST_MOVE_ENABLE,ST_WATER,
};

enum {
	NV_BASIC = 1,

	SM_SWORD,
	SM_TWOHAND,
	SM_RECOVERY,
	SM_BASH,
	SM_PROVOKE,
	SM_MAGNUM,
	SM_ENDURE,

	MG_SRECOVERY,
	MG_SIGHT,
	MG_NAPALMBEAT,
	MG_SAFETYWALL,
	MG_SOULSTRIKE,
	MG_COLDBOLT,
	MG_FROSTDIVER,
	MG_STONECURSE,
	MG_FIREBALL,
	MG_FIREWALL,
	MG_FIREBOLT,
	MG_LIGHTNINGBOLT,
	MG_THUNDERSTORM,
	AL_DP,
	AL_DEMONBANE,
	AL_RUWACH,
	AL_PNEUMA,
	AL_TELEPORT,
	AL_WARP,
	AL_HEAL,
	AL_INCAGI,
	AL_DECAGI,
	AL_HOLYWATER,
	AL_CRUCIS,
	AL_ANGELUS,
	AL_BLESSING,
	AL_CURE,

	MC_INCCARRY,
	MC_DISCOUNT,
	MC_OVERCHARGE,
	MC_PUSHCART,
	MC_IDENTIFY,
	MC_VENDING,
	MC_MAMMONITE,

	AC_OWL,
	AC_VULTURE,
	AC_CONCENTRATION,
	AC_DOUBLE,
	AC_SHOWER,

	TF_DOUBLE,
	TF_MISS,
	TF_STEAL,
	TF_HIDING,
	TF_POISON,
	TF_DETOXIFY,

	ALL_RESURRECTION,

	KN_SPEARMASTERY,
	KN_PIERCE,
	KN_BRANDISHSPEAR,
	KN_SPEARSTAB,
	KN_SPEARBOOMERANG,
	KN_TWOHANDQUICKEN,
	KN_AUTOCOUNTER,
	KN_BOWLINGBASH,
	KN_RIDING,
	KN_CAVALIERMASTERY,

	PR_MACEMASTERY,
	PR_IMPOSITIO,
	PR_SUFFRAGIUM,
	PR_ASPERSIO,
	PR_BENEDICTIO,
	PR_SANCTUARY,
	PR_SLOWPOISON,
	PR_STRECOVERY,
	PR_KYRIE,
	PR_MAGNIFICAT,
	PR_GLORIA,
	PR_LEXDIVINA,
	PR_TURNUNDEAD,
	PR_LEXAETERNA,
	PR_MAGNUS,

	WZ_FIREPILLAR,
	WZ_SIGHTRASHER,
	WZ_FIREIVY,
	WZ_METEOR,
	WZ_JUPITEL,
	WZ_VERMILION,
	WZ_WATERBALL,
	WZ_ICEWALL,
	WZ_FROSTNOVA,
	WZ_STORMGUST,
	WZ_EARTHSPIKE,
	WZ_HEAVENDRIVE,
	WZ_QUAGMIRE,
	WZ_ESTIMATION,

	BS_IRON,
	BS_STEEL,
	BS_ENCHANTEDSTONE,
	BS_ORIDEOCON,
	BS_DAGGER,
	BS_SWORD,
	BS_TWOHANDSWORD,
	BS_AXE,
	BS_MACE,
	BS_KNUCKLE,
	BS_SPEAR,
	BS_HILTBINDING,
	BS_FINDINGORE,
	BS_WEAPONRESEARCH,
	BS_REPAIRWEAPON,
	BS_SKINTEMPER,
	BS_HAMMERFALL,
	BS_ADRENALINE,
	BS_WEAPONPERFECT,
	BS_OVERTHRUST,
	BS_MAXIMIZE,

	HT_SKIDTRAP,
	HT_LANDMINE,
	HT_ANKLESNARE,
	HT_SHOCKWAVE,
	HT_SANDMAN,
	HT_FLASHER,
	HT_FREEZINGTRAP,
	HT_BLASTMINE,
	HT_CLAYMORETRAP,
	HT_REMOVETRAP,
	HT_TALKIEBOX,
	HT_BEASTBANE,
	HT_FALCON,
	HT_STEELCROW,
	HT_BLITZBEAT,
	HT_DETECTING,
	HT_SPRINGTRAP,

	AS_RIGHT,
	AS_LEFT,
	AS_KATAR,
	AS_CLOAKING,
	AS_SONICBLOW,
	AS_GRIMTOOTH,
	AS_ENCHANTPOISON,
	AS_POISONREACT,
	AS_VENOMDUST,
	AS_SPLASHER,

	NV_FIRSTAID,
	NV_TRICKDEAD,
	SM_MOVINGRECOVERY,
	SM_FATALBLOW,
	SM_AUTOBERSERK,
	AC_MAKINGARROW,
	AC_CHARGEARROW,
	TF_SPRINKLESAND,
	TF_BACKSLIDING,
	TF_PICKSTONE,
	TF_THROWSTONE,
	MC_CARTREVOLUTION,
	MC_CHANGECART,
	MC_LOUD,
	AL_HOLYLIGHT,
	MG_ENERGYCOAT,

	NPC_PIERCINGATT=158,
	NPC_MENTALBREAKER,
	NPC_RANGEATTACK,
	NPC_ATTRICHANGE,
	NPC_CHANGEWATER,
	NPC_CHANGEGROUND,
	NPC_CHANGEFIRE,
	NPC_CHANGEWIND,
	NPC_CHANGEPOISON,
	NPC_CHANGEHOLY,
	NPC_CHANGETELEKINESIS,
	NPC_CHANGEDARKNESS,
	NPC_CRITICALSLASH,
	NPC_COMBOATTACK,
	NPC_GUIDEDATTACK,
	NPC_SELFDESTRUCTION,
	NPC_SPLASHATTACK,
	NPC_SUICIDE,
	NPC_POISON,
	NPC_BLINDATTACK,
	NPC_SILENCEATTACK,
	NPC_STUNATTACK,
	NPC_PETRIFYATTACK,
	NPC_CURSEATTACK,
	NPC_SLEEPATTACK,
	NPC_RANDOMATTACK,
	NPC_WATERATTACK,
	NPC_GROUNDATTACK,
	NPC_FIREATTACK,
	NPC_WINDATTACK,
	NPC_POISONATTACK,
	NPC_HOLYATTACK,
	NPC_DARKNESSATTACK,
	NPC_TELEKINESISATTACK,
	NPC_MAGICALATTACK,
	NPC_METAMORPHOSIS,
	NPC_PROVOCATION,
	NPC_SMOKING,
	NPC_SUMMONSLAVE,
	NPC_EMOTION,
	NPC_TRANSFORMATION,
	NPC_BLOODDRAIN,
	NPC_ENERGYDRAIN,
	NPC_KEEPING,
	NPC_DARKBREATH,
	NPC_DARKBLESSING,
	NPC_BARRIER,
	NPC_DEFENDER,
	NPC_LICK,
	NPC_HALLUCINATION,
	NPC_REBIRTH,
	NPC_SUMMONMONSTER,

	RG_SNATCHER,
	RG_STEALCOIN,
	RG_BACKSTAP,
	RG_TUNNELDRIVE,
	RG_RAID,
	RG_STRIPWEAPON,
	RG_STRIPSHIELD,
	RG_STRIPARMOR,
	RG_STRIPHELM,
	RG_INTIMIDATE,
	RG_GRAFFITI,
	RG_FLAGGRAFFITI,
	RG_CLEANER,
	RG_GANGSTER,
	RG_COMPULSION,
	RG_PLAGIARISM,

	AM_AXEMASTERY,
	AM_LEARNINGPOTION,
	AM_PHARMACY,
	AM_DEMONSTRATION,
	AM_ACIDTERROR,
	AM_POTIONPITCHER,
	AM_CANNIBALIZE,
	AM_SPHEREMINE,
	AM_CP_WEAPON,
	AM_CP_SHIELD,
	AM_CP_ARMOR,
	AM_CP_HELM,
	AM_BIOETHICS,
	AM_BIOTECHNOLOGY,
	AM_CREATECREATURE,
	AM_CULTIVATION,
	AM_FLAMECONTROL,
	AM_CALLHOMUN,
	AM_REST,
	AM_DRILLMASTER,
	AM_HEALHOMUN,
	AM_RESURRECTHOMUN,

	CR_TRUST,
	CR_AUTOGUARD,
	CR_SHIELDCHARGE,
	CR_SHIELDBOOMERANG,
	CR_REFLECTSHIELD,
	CR_HOLYCROSS,
	CR_GRANDCROSS,
	CR_DEVOTION,
	CR_PROVIDENCE,
	CR_DEFENDER,
	CR_SPEARQUICKEN,

	MO_IRONHAND,
	MO_SPIRITSRECOVERY,
	MO_CALLSPIRITS,
	MO_ABSORBSPIRITS,
	MO_TRIPLEATTACK,
	MO_BODYRELOCATION,
	MO_DODGE,
	MO_INVESTIGATE,
	MO_FINGEROFFENSIVE,
	MO_STEELBODY,
	MO_BLADESTOP,
	MO_EXPLOSIONSPIRITS,
	MO_EXTREMITYFIST,
	MO_CHAINCOMBO,
	MO_COMBOFINISH,

	SA_ADVANCEDBOOK,
	SA_CASTCANCEL,
	SA_MAGICROD,
	SA_SPELLBREAKER,
	SA_FREECAST,
	SA_AUTOSPELL,
	SA_FLAMELAUNCHER,
	SA_FROSTWEAPON,
	SA_LIGHTNINGLOADER,
	SA_SEISMICWEAPON,
	SA_DRAGONOLOGY,
	SA_VOLCANO,
	SA_DELUGE,
	SA_VIOLENTGALE,
	SA_LANDPROTECTOR,
	SA_DISPELL,
	SA_ABRACADABRA,
	SA_MONOCELL,
	SA_CLASSCHANGE,
	SA_SUMMONMONSTER,
	SA_REVERSEORCISH,
	SA_DEATH,
	SA_FORTUNE,
	SA_TAMINGMONSTER,
	SA_QUESTION,
	SA_GRAVITY,
	SA_LEVELUP,
	SA_INSTANTDEATH,
	SA_FULLRECOVERY,
	SA_COMA,

	BD_ADAPTATION,
	BD_ENCORE,
	BD_LULLABY,
	BD_RICHMANKIM,
	BD_ETERNALCHAOS,
	BD_DRUMBATTLEFIELD,
	BD_RINGNIBELUNGEN,
	BD_ROKISWEIL,
	BD_INTOABYSS,
	BD_SIEGFRIED,
	BD_RAGNAROK,

	BA_MUSICALLESSON,
	BA_MUSICALSTRIKE,
	BA_DISSONANCE,
	BA_FROSTJOKE,
	BA_WHISTLE,
	BA_ASSASSINCROSS,
	BA_POEMBRAGI,
	BA_APPLEIDUN,

	DC_DANCINGLESSON,
	DC_THROWARROW,
	DC_UGLYDANCE,
	DC_SCREAM,
	DC_HUMMING,
	DC_DONTFORGETME,
	DC_FORTUNEKISS,
	DC_SERVICEFORYOU,

	WE_MALE = 334,
	WE_FEMALE,
	WE_CALLPARTNER,

	NPC_SELFDESTRUCTION2 = 331,
	ITM_TOMAHAWK = 337,
	NPC_DARKCROSS,
	NPC_DARKGRANDCROSS,
	NPC_DARKSOULSTRIKE,
	NPC_DARKJUPITEL,
	NPC_HOLDWEB,
	NPC_BREAKWEAPON,
	NPC_BREAKARMOR,
	NPC_BREAKHELM,
	NPC_BREAKSIELD,
	NPC_UNDEADATTACK,
	NPC_RUNAWAY = 348,
	NPC_EXPLOSIONSPIRITS,
	NPC_INCREASEFLEE,
	NPC_ELEMENTUNDEAD,
	NPC_INVISIBLE,
	NPC_RECALL = 354,

	LK_AURABLADE = 355,
	LK_PARRYING,
	LK_CONCENTRATION,
	LK_TENSIONRELAX,
	LK_BERSERK,
	LK_FURY,
	HP_ASSUMPTIO,
	HP_BASILICA,
	HP_MEDITATIO,
	HW_SOULDRAIN,
	HW_MAGICCRASHER,
	HW_MAGICPOWER,
	PA_PRESSURE,
	PA_SACRIFICE,
	PA_GOSPEL,
	CH_PALMSTRIKE,
	CH_TIGERFIST,
	CH_CHAINCRUSH,
	PF_HPCONVERSION,
	PF_SOULCHANGE,
	PF_SOULBURN,
	ASC_KATAR,
	ASC_HALLUCINATION,
	ASC_EDP,
	ASC_BREAKER,
	SN_SIGHT,
	SN_FALCONASSAULT,
	SN_SHARPSHOOTING,
	SN_WINDWALK,
	WS_MELTDOWN,
	WS_CREATECOIN,
	WS_CREATENUGGET,
	WS_CARTBOOST,
	WS_SYSTEMCREATE,
	ST_CHASEWALK,
	ST_REJECTSWORD,
	ST_STEALBACKPACK,
	CR_ALCHEMY,
	CR_SYNTHESISPOTION,
	CG_ARROWVULCAN,
	CG_MOONLIT,
	CG_MARIONETTE,
	LK_SPIRALPIERCE,
	LK_HEADCRUSH,
	LK_JOINTBEAT,
	HW_NAPALMVULCAN,
	CH_SOULCOLLECT,
	PF_MINDBREAKER,
	PF_MEMORIZE,
	PF_FOGWALL,
	PF_SPIDERWEB,
	ASC_METEORASSAULT,
	ASC_CDP,
	WE_BABY,
	WE_CALLPARENT,
	WE_CALLBABY,
	TK_RUN,
	TK_READYSTORM,
	TK_STORMKICK,
	TK_READYDOWN,
	TK_DOWNKICK,
	TK_READYTURN,
	TK_TURNKICK,
	TK_READYCOUNTER,
	TK_COUNTER,
	TK_DODGE,
	TK_JUMPKICK,
	TK_HPTIME,
	TK_SPTIME,
	TK_POWER,
	TK_SEVENWIND,
	TK_HIGHJUMP,
	SG_FEEL,
	SG_SUN_WARM,
	SG_MOON_WARM,
	SG_STAR_WARM,
	SG_SUN_COMFORT,
	SG_MOON_COMFORT,
	SG_STAR_COMFORT,
	SG_HATE,
	SG_SUN_ANGER,
	SG_MOON_ANGER,
	SG_STAR_ANGER,
	SG_SUN_BLESS,
	SG_MOON_BLESS,
	SG_STAR_BLESS,
	SG_DEVIL,
	SG_FRIEND,
	SG_KNOWLEDGE,
	SG_FUSION,
	SL_ALCHEMIST,
	AM_BERSERKPITCHER,
	SL_MONK,
	SL_STAR,
	SL_SAGE,
	SL_CRUSADER,
	SL_SUPERNOVICE,
	SL_KNIGHT,
	SL_WIZARD,
	SL_PRIEST,
	SL_BARDDANCER,
	SL_ROGUE,
	SL_ASSASIN,
	SL_BLACKSMITH,
	BS_ADRENALINE2,
	SL_HUNTER,
	SL_SOULLINKER,
	SL_KAIZEL,
	SL_KAAHI,
	SL_KAUPE,
	SL_KAITE,
	SL_KAINA,
	SL_STIN,
	SL_STUN,
	SL_SMA,
	SL_SWOO,
	SL_SKE,
	SL_SKA,

	ST_PRESERVE = 475,
	ST_FULLSTRIP,
	WS_WEAPONREFINE,
	CR_SLIMPITCHER,
	CR_FULLPROTECTION,
	PA_SHIELDCHAIN,
	HP_MANARECHARGE,
	PF_DOUBLECASTING,
	HW_GANBANTEIN,
	HW_GRAVITATION,
	WS_CARTTERMINATION,
	WS_OVERTHRUSTMAX,
	CG_LONGINGFREEDOM,
	CG_HERMODE,
	CG_TAROTCARD,
	CR_ACIDDEMONSTRATION,
	CR_CULTIVATION,
	// = 492,
	TK_MISSION		= 493,
	SL_HIGH			= 494,
	KN_ONEHAND		= 495,
	AM_TWILIGHT1	= 496,
	AM_TWILIGHT2	= 497,
	AM_TWILIGHT3	= 498,
	HT_POWER 		= 499,

	GS_GLITTERING   = 500,//#GS_GLITTERING#
	GS_FLING,//#GS_FLING#
	GS_TRIPLEACTION,//#GS_TRIPLEACTION#
	GS_BULLSEYE,//#GS_BULLSEYE#
	GS_MADNESSCANCEL,//#GS_MADNESSCANCEL#
	GS_ADJUSTMENT,//#GS_ADJUSTMENT#
	GS_INCREASING,//#GS_INCREASING#
	GS_MAGICALBULLET,//#GS_MAGICALBULLET#
	GS_CRACKER,//#GS_CRACKER#
	GS_SINGLEACTION,//#GS_SINGLEACTION#
	GS_SNAKEEYE,//#GS_SNAKEEYE#	
	GS_CHAINACTION,//#GS_CHAINACTION#
	GS_TRACKING,//#GS_TRACKING#
	GS_DISARM,//#GS_DISARM#
	GS_PIERCINGSHOT,//#GS_PIERCINGSHOT#
	GS_RAPIDSHOWER,//#GS_RAPIDSHOWER#
	GS_DESPERADO,//#GS_DESPERADO#
	GS_GATLINGFEVER,//#GS_GATLINGFEVER#
	GS_DUST,//#GS_DUST#
	GS_FULLBUSTER,//#GS_FULLBUSTER#
	GS_SPREADATTACK,//#GS_SPREADATTACK#
	GS_GROUNDDRIFT,//#GS_GROUNDDRIFT#

	NJ_TOBIDOUGU,//#NJ_TOBIDOUGU#
	NJ_SYURIKEN,//#NJ_SYURIKEN#
	NJ_KUNAI,//#NJ_KUNAI#
	NJ_HUUMA,//#NJ_HUUMA#
	NJ_ZENYNAGE,//#NJ_ZENYNAGE#
	NJ_TATAMIGAESHI,//#NJ_TATAMIGAESHI#
	NJ_KASUMIKIRI,//#NJ_KASUMIKIRI#
	NJ_SHADOWJUMP,//#NJ_SHADOWJUMP#
	NJ_KIRIKAGE,//#NJ_KIRIKAGE#
	NJ_UTSUSEMI,//#NJ_UTSUSEMI#
	NJ_BUNSINJYUTSU,//#NJ_BUNSINJYUTSU#
	NJ_NINPOU,//#NJ_NINPOU#
	NJ_KOUENKA,//#NJ_KOUENKA#
	NJ_KAENSIN,//#NJ_KAENSIN#
	NJ_BAKUENRYU,//#NJ_BAKUENRYU#
	NJ_HYOUSENSOU,//#NJ_HYOUSENSOU#
	NJ_SUITON,//#NJ_SUITON#
	NJ_HYOUSYOURAKU,//#NJ_HYOUSYOURAKU#
	NJ_HUUJIN,//#NJ_HUUJIN#
	NJ_RAIGEKISAI,//#NJ_RAIGEKISAI#
	NJ_KAMAITACHI,//#NJ_KAMAITACHI#
	NJ_NEN,//#NJ_NEN#
	NJ_ISSEN,//#NJ_ISSEN#
	
	MB_FIGHTING		=545,//#�t�@�C�e�B���O#
	MB_NEUTRAL,//#�j���[�g����,//#
	MB_TAIMING_PUTI,//#�e�C�~���O�v�e�B,//#
	MB_WHITEPOTION,//#�z���C�g�|�[�V����,//#
	MB_CARDPITCHER,//#�J�[�h�s�b�`���[,//#
	MB_MENTAL,//#�����^��,//#
	MB_PETPITCHER,//#�y�b�g�s�b�`���[,//#
	MB_BODYSTUDY,//#�{�f�B�X�^�f�B,//#
	MB_BODYALTER,//#�{�f�B�I���^�[,//#
	MB_PETMEMORY,//#�y�b�g�������[,//#
	MB_M_TELEPORT,//#�e���|�[�g,//#
	MB_B_GAIN,//#�Q�C��,//#
	MB_M_GAIN,//#�Q�C��,//#
	MB_MISSION,//#�~�b�V����,//#
	MB_MUNAKKNOWLEDGE,//#���i�b�N�m�E���b�W,//#
	MB_MUNAKBALL,//#���i�b�N�{�[��,//#
	MB_SCROLL,//#�X�N���[��,//#
	MB_B_GATHERING,//#�M���U�����O,//#
	MB_B_EXCLUDE,//#�G�N�X�N���[�h,//#
	MB_B_DRIFT,//#�h���t�g,//#
	MB_M_DRIFT,//#�h���t�g,//#
	MB_B_WALLRUSH,//#�E�H�[�����b�V��,//#
	MB_M_WALLRUSH,//#�E�H�[�����b�V��,//#
	MB_B_WALLSHIFT,//#�E�H�[���V�t�g,//#
	MB_M_WALLCRASH,//#�E�H�[���N���b�V��,//#
	MB_M_REINCARNATION,//#���C���J�[�l�[�V����,//#
	MB_B_EQUIP,//#�C�N�C�b�v,//#

	SL_DEATHKNIGHT	=572,//#�f�X�i�C�g�̍�,//#
	SL_COLLECTOR,//#�_�[�N�R���N�^�[�̍�,//#
	SL_NINJA,//#�E�҂̍�,//#
	SL_GUNNER,//#�K���X�����K�[�̍�,//#
	AM_TWILIGHT4,//#�g���C���C�g�t�@�[�}�V�[4,//#

	DE_PASSIVE		=577,//#�p�b�V�u,//#
	DE_PATTACK,//#P�A�^�b�N,//#
	DE_PSPEED,//#P�X�s�[�h ,//#
	DE_PDEFENSE,//#P�f�B�t�F���X ,//#
	DE_PCRITICAL,//#P�N���e�B�J��,//#
	DE_PHP,//#PHP,//#
	DE_PSP,//#PSP,//#
	DE_RESET,//#���Z�b�g,//#
	DE_RANKING,//#�����L���O,//#
	DE_PTRIPLE,//#P�g���v��,//#
	DE_ENERGY,//#�G�i�W�[,//#
	DE_NIGHTMARE,//#�i�C�g���A,//#
	DE_SLASH,//#�X���b�V��,//#
	DE_COIL,//#�R�C��,//#
	DE_WAVE,//#�E�F�[�u,//#
	DE_REBIRTH,//#���o�[�X,//#
	DE_AURA,//#�I�[��,//#
	DE_FREEZER,//#�t���[�U�[,//#
	DE_CHANGEATTACK,//#�`�F���W�A�^�b�N,//#
	DE_PUNISH,//#�p�j�b�V��,//#
	DE_POISON,//#�|�C�Y��,//#
	DE_INSTANT,//#�C���X�^���g,//#
	DE_WARNING,//#���[�j���O,//#
	DE_RANKEDKNIFE,//#�����N�h�i�C�t,//#
	DE_RANKEDGRADIUS,//#�����N�h�O���f�B�E�X,//#
	DE_GAUGE,//#G�I�[�W�F,//#
	DE_GTIME,//#G�^�C��,//#
	DE_GPAIN,//#G�y�C��,//#
	DE_GSKILL,//#G�X�L��,//#
	DE_GKILL,//#G�L��,//#
	DE_ACCEL,//#�A�N�Z��,//#
	DE_BLOCKDOUBLE,//#�_�u���u���b�N,//#
	DE_BLOCKMELEE,//#���C���C�u���b�N,//#
	DE_BLOCKFAR,//#�t�@�[�u���b�N,//#
	DE_FRONTATTACK,//#�t�����g�A�^�b�N,//#
	DE_DANGERATTACK,//#�f���W���[�A�^�b�N,//#
	DE_TWINATTACK,//#�c�C���A�^�b�N,//#
	DE_WINDATTACK,//#�E�B���h�A�^�b�N,//#
	DE_WATERATTACK	=	615,//#�E�H�[�^�[�A�^�b�N,//#
	
	KN_CHARGEATK	=	1001,//#�`���[�W�A�^�b�N#
	CR_SHRINK		=	1002,//#�V�������N#
	AS_SONICACCEL	=	1003,//#�\�j�b�N�A�N�Z�����[�V����#
	AS_VENOMKNIFE	=	1004,//#�x�i���i�C�t#
	RG_CLOSECONFINE	=	1005,//#�N���[�Y�R���t�@�C��#
	WZ_SIGHTBLASTER	=	1006,//#�T�C�g�u���X�^�[#
	SA_CREATECON	=	1007,//#�G���������^���R���o�[�^����#
	SA_ELEMENTWATER	=	1008,//#�G���������^���`�F���W�i���j#
	HT_PHANTASMIC	=	1009,//#�t�@���^�X�~�b�N�A���[#
	BA_PANGVOICE	=	1010,//#�p���{�C�X#
	DC_WINKCHARM	=	1011,//#���f�̃E�B���N#
	BS_UNFAIRLYTRICK=	1012,//#�A���t�F�A���[�g���b�N#
	BS_GREED		=	1013,//#�×~#
	PR_REDEMPTIO	=	1014,//#���f���v�e�B�I#
	MO_KITRANSLATION=	1015,//#�����(�U�C����)#
	MO_BALKYOUNG	=	1016,//#����(����)#
	SA_ELEMENTGROUND=	1017,
	SA_ELEMENTFIRE	=	1018,
	SA_ELEMENTWIND	=	1019,

	HM_SKILLBASE	=8001,
	HLIF_HEAL		=8001,//#�����̎菕��(�q�[��)#
	HLIF_AVOID		=8002,//#�ً}���#
	HLIF_BRAIN		=8003,//=//#�]��p#
	HLIF_CHANGE		=8004,//#�����^���`�F���W#
	HAMI_CASTLE		=8005,//#�L���X�g�����O#
	HAMI_DEFENCE	=8006,//#�f�B�t�F���X#
	HAMI_SKIN		=8007,//#�A�_�}���e�B�E���X�L��#
	HAMI_BLOODLUST	=8008,//#�u���b�h���X�g#
	HFLI_MOON		=8009,//#���[�����C�g#
	HFLI_FLEET		=8010,//#�t���[�g���[�u#
	HFLI_SPEED		=8011,//#�I�[�o�[�h�X�s�[�h#
	HFLI_SBR44		=8012,//#S.B.R.44#
	HVAN_CAPRICE	=8013,//#�J�v���X#
	HVAN_CHAOTIC	=8014,//#�J�I�e�B�b�N�x�l�f�B�N�V����#
	HVAN_INSTRUCT	=8015,//#�`�F���W�C���X�g���N�V����#
	HVAN_EXPLOSION	=8016,//#�o�C�I�G�N�X�v���[�W����#

//	move to common/mmo.h
//	GD_APPROVAL=10000,
//	GD_KAFRACONTACT,
//	GD_GUARDIANRESEARCH,
//	GD_CHARISMA,
//	GD_EXTENSION,
	//return from common/mmo.h
	GD_SKILLBASE=10000,
	GD_APPROVAL=10000,
	GD_KAFRACONTACT,
	GD_GUARDIANRESEARCH,
	GD_GUARDUP,
	//GD_CHARISMA, -> GD_GUARDUP�ɕύX����
	// GD_GURADUP,	// GD)CHARISMA�Ɠ�ID
	GD_EXTENSION,
	GD_GLORYGUILD,
	GD_LEADERSHIP,
	GD_GLORYWOUNDS,
	GD_SOULCOLD,
	GD_HAWKEYES,
	GD_BATTLEORDER,
	GD_REGENERATION,
	GD_RESTORE,
	GD_EMERGENCYCALL,
	GD_DEVELOPMENT,

};

extern int SkillStatusChangeTable[];
extern int GuildSkillStatusChangeTable[];

struct skill_unit_group *skill_unitsetting( struct block_list *src, int skillid,int skilllv,int x,int y,int flag);

#endif

