#ifndef _STATUS_H_
#define _STATUS_H_

struct status_pretimer {
	int timer;
	int target_id;
	int map;
	int type;
	int val1,val2,val3,val4;
	int tick;
	int flag;
};

// �X�e�[�^�X�ُ�f�[�^�x�[�X
struct scdata_db {
	short save;
	int releasable;
	int disable;
};

// �p�����[�^�擾�n
int status_get_class(struct block_list *bl);
int status_get_dir(struct block_list *bl);
int status_get_lv(struct block_list *bl);
int status_get_range(struct block_list *bl);
int status_get_group(struct block_list *bl);
int status_get_hp(struct block_list *bl);
int status_get_sp(struct block_list *bl);
int status_get_max_hp(struct block_list *bl);
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

struct status_change *status_get_sc_data(struct block_list *bl);
short *status_get_sc_count(struct block_list *bl);
unsigned short *status_get_opt1(struct block_list *bl);
unsigned short *status_get_opt2(struct block_list *bl);
unsigned int *status_get_opt3(struct block_list *bl);
unsigned int *status_get_option(struct block_list *bl);

int status_get_matk1(struct block_list *bl);
int status_get_matk2(struct block_list *bl);
int status_get_critical(struct block_list *bl);
int status_get_atk_(struct block_list *bl);
int status_get_atk_2(struct block_list *bl);
int status_get_atk2(struct block_list *bl);
int status_get_aspd(struct block_list *bl);

// ��Ԉُ�֘A
int status_can_save(int type);
int status_is_disable(int type,int mask);
int status_change_start(struct block_list *bl,int type,int val1,int val2,int val3,int val4,int tick,int flag);
int status_change_end( struct block_list* bl , int type,int tid );
int status_change_pretimer(struct block_list *bl,int type,int val1,int val2,int val3,int val4,int tick,int flag,int pre_tick);
int status_change_timer(int tid, unsigned int tick, int id, int data);
int status_change_timer_sub(struct block_list *bl, va_list ap );
int status_change_race_end(struct block_list *bl,int type);
int status_change_soulstart(struct block_list *bl,int val1,int val2,int val3,int val4,int tick,int flag);
int status_change_soulclear(struct block_list *bl);
int status_change_resistclear(struct block_list *bl);
int status_enchant_armor_eremental_end(struct block_list *bl,int type);
int status_encchant_eremental_end(struct block_list *bl,int type);
int status_change_clear(struct block_list *bl,int type);
int status_change_release(struct block_list *bl,int mask);
int status_clearpretimer(struct block_list *bl);
int status_change_attacked_end(struct block_list *bl);
int status_change_hidden_end(struct block_list *bl);

// ��ԃ`�F�b�N
int status_check_no_magic_damage(struct block_list *bl);

#ifdef DYNAMIC_SC_DATA
int status_calloc_sc_data(struct block_list *bl);
int status_free_sc_data(struct block_list *bl);
int status_check_dummy_sc_data(struct block_list *bl);
#endif

// �X�e�[�^�X�v�Z
int status_calc_pc(struct map_session_data* sd,int first);
int status_get_overrefine_bonus(int lv);
int status_percentrefinery(struct map_session_data *sd,struct item *item);
int status_percentrefinery_weaponrefine(struct map_session_data *sd,struct item *item);
extern int current_equip_item_index;
extern int current_equip_card_id;

extern struct status_change dummy_sc_data[MAX_STATUSCHANGE];

// DB�ēǍ��p
int status_readdb(void);

int do_init_status(void);

enum {	// struct map_session_data �� status_change�̔ԍ��e�[�u��
	//SC_SENDMAX				=128,

	SC_PROVOKE				= 0,	/* �v���{�b�N */
	SC_ENDURE				= 1,	/* �C���f���A */
	SC_TWOHANDQUICKEN		= 2,	/* �c�[�n���h�N�C�b�P�� */
	SC_CONCENTRATE			= 3,	/* �W���͌��� */
	SC_HIDING				= 4,	/* �n�C�f�B���O */
	SC_CLOAKING				= 5,	/* �N���[�L���O */
	SC_ENCPOISON			= 6,	/* �G���`�����g�|�C�Y�� */
	SC_POISONREACT			= 7,	/* �|�C�Y�����A�N�g */
	SC_QUAGMIRE				= 8,	/* �N�@�O�}�C�A */
	SC_ANGELUS				= 9,	/* �G���W�F���X */
	SC_BLESSING				=10,	/* �u���b�V���O */
	SC_SIGNUMCRUCIS			=11,	/* �V�O�i���N���V�X�H */
	SC_INCREASEAGI			=12,	/*  */
	SC_DECREASEAGI			=13,	/*  */
	SC_SLOWPOISON			=14,	/* �X���[�|�C�Y�� */
	SC_IMPOSITIO			=15,	/* �C���|�V�e�B�I�}�k�X */
	SC_SUFFRAGIUM			=16,	/* �T�t���M�E�� */
	SC_ASPERSIO				=17,	/* �A�X�y���V�I */
	SC_BENEDICTIO			=18,	/* ���̍~�� */
	SC_KYRIE				=19,	/* �L���G�G���C�\�� */
	SC_MAGNIFICAT			=20,	/* �}�O�j�t�B�J�[�g */
	SC_GLORIA				=21,	/* �O�����A */
	SC_AETERNA				=22,	/*  */
	SC_ADRENALINE			=23,	/* �A�h���i�������b�V�� */
	SC_WEAPONPERFECTION		=24,	/* �E�F�|���p�[�t�F�N�V���� */
	SC_OVERTHRUST			=25,	/* �I�[�o�[�g���X�g */
	SC_MAXIMIZEPOWER		=26,	/* �}�L�V�}�C�Y�p���[ */
	//
	SC_TRICKDEAD			=29,	/* ���񂾂ӂ� */
	SC_LOUD					=30,	/* ���E�h�{�C�X */
	SC_ENERGYCOAT			=31,	/* �G�i�W�[�R�[�g */
	SC_PK_PENALTY			=32,	//PK�̃y�i���e�B
	SC_REVERSEORCISH		=33,    //���o�[�X�I�[�L�b�V��
	SC_HALLUCINATION		=34,	/* ���o */
	//
	SC_SPEEDPOTION0			=37,	/* ���x�|�[�V�����H */
	SC_SPEEDPOTION1			=38,	/* �X�s�[�h�A�b�v�|�[�V�����H */
	SC_SPEEDPOTION2			=39,	/* �n�C�X�s�[�h�|�[�V�����H */
	SC_SPEEDPOTION3			=40,	/* �o�[�T�[�N�|�[�V���� */
	SC_ITEM_DELAY			=41,
	//
	//
	//
	//
	//
	//
	//
	//
	SC_STRIPWEAPON			=50,	/* �X�g���b�v�E�F�|�� */
	SC_STRIPSHIELD			=51,	/* �X�g���b�v�V�[���h */
	SC_STRIPARMOR			=52,	/* �X�g���b�v�A�[�}�[ */
	SC_STRIPHELM			=53,	/* �X�g���b�v�w���� */
	SC_CP_WEAPON			=54,	/* �P�~�J���E�F�|���`���[�W */
	SC_CP_SHIELD			=55,	/* �P�~�J���V�[���h�`���[�W */
	SC_CP_ARMOR				=56,	/* �P�~�J���A�[�}�[�`���[�W */
	SC_CP_HELM				=57,	/* �P�~�J���w�����`���[�W */
	SC_AUTOGUARD			=58,	/* �I�[�g�K�[�h */
	SC_REFLECTSHIELD		=59,	/* ���t���N�g�V�[���h */
	SC_DEVOTION				=60,	/* �f�B�{�[�V���� */
	SC_PROVIDENCE			=61,	/* �v�����B�f���X */
	SC_DEFENDER				=62,	/* �f�B�t�F���_�[ */
	SC_SANTA				=63,	//�T���^
	//
	SC_AUTOSPELL			=65,	/* �I�[�g�X�y�� */
	//
	//
	SC_SPEARSQUICKEN		=68,	/* �X�s�A�N�C�b�P�� */
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
	SC_EXPLOSIONSPIRITS		=86,	/* �����g�� */
	SC_STEELBODY			=87,	/* ���� */
	//
	SC_COMBO				=89,
	SC_FLAMELAUNCHER		=90,	/* �t���C�������`���[ */
	SC_FROSTWEAPON			=91,	/* �t���X�g�E�F�|�� */
	SC_LIGHTNINGLOADER		=92,	/* ���C�g�j���O���[�_�[ */
	SC_SEISMICWEAPON		=93,	/* �T�C�Y�~�b�N�E�F�|�� */
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
	SC_AURABLADE			=103,	/* �I�[���u���[�h */
	SC_PARRYING				=104,	/* �p���C���O */
	SC_CONCENTRATION		=105,	/* �R���Z���g���[�V���� */
	SC_TENSIONRELAX			=106,	/* �e���V���������b�N�X */
	SC_BERSERK				=107,	/* �o�[�T�[�N */
	//
	//
	//
	SC_ASSUMPTIO			=110,	/* �A�X���v�e�B�I */
	//
	//
	SC_MAGICPOWER			=113,	/* ���@�͑��� */
	SC_EDP					=114,	/* �G�t�F�N�g������������ړ� */
	SC_TRUESIGHT			=115,	/* �g�D���[�T�C�g */
	SC_WINDWALK				=116,	/* �E�C���h�E�H�[�N */
	SC_MELTDOWN				=117,	/* �����g�_�E�� */
	SC_CARTBOOST			=118,	/* �J�[�g�u�[�X�g */
	SC_CHASEWALK			=119,	/* �`�F�C�X�E�H�[�N */
	SC_REJECTSWORD			=120,	/* ���W�F�N�g�\�[�h */
	SC_MARIONETTE			=121,	/* �}���I�l�b�g�R���g���[�� */ //�����p
	SC_MARIONETTE2			=122,	/* �}���I�l�b�g�R���g���[�� */ //�^�[�Q�b�g�p
	//
	SC_HEADCRUSH			=124,	/* �w�b�h�N���b�V�� */
	SC_JOINTBEAT			=125,	/* �W���C���g�r�[�g */
	//
	//
	SC_STONE				=128,	/* ��Ԉُ�F�Ή� */
	SC_FREEZE				=129,	/* ��Ԉُ�F�X�� */
	SC_STAN					=130,	/* ��Ԉُ�F�X�^�� */
	SC_SLEEP				=131,	/* ��Ԉُ�F���� */
	SC_POISON				=132,	/* ��Ԉُ�F�� */
	SC_CURSE				=133,	/* ��Ԉُ�F�� */
	SC_SILENCE				=134,	/* ��Ԉُ�F���� */
	SC_CONFUSION			=135,	/* ��Ԉُ�F���� */
	SC_BLIND				=136,	/* ��Ԉُ�F�È� */
	SC_BLEED				=137,	/* ��Ԉُ�F�o�� */
	//138
	//139
	SC_SAFETYWALL			=140,	/* �Z�[�t�e�B�[�E�H�[�� */
	SC_PNEUMA				=141,	/* �j���[�} */
	//
	SC_ANKLE				=143,	/* �A���N���X�l�A */
	SC_DANCING				=144,	/*  */
	SC_KEEPING				=145,	/*  */
	SC_BARRIER				=146,	/*  */
	SC_TIGERFIST			=147,	/* ���Ռ� */
	//
	SC_MAGICROD				=149,	/*  */
	SC_SIGHT				=150,	/*  */
	SC_RUWACH				=151,	/*  */
	SC_AUTOCOUNTER			=152,	/*  */
	SC_VOLCANO				=153,	/*  */
	SC_DELUGE				=154,	/*  */
	SC_VIOLENTGALE			=155,	/*  */
	SC_BLADESTOP_WAIT		=156,	/*  */
	SC_BLADESTOP			=157,	/*  */
	SC_EXTREMITYFIST		=158,	/*  */
	SC_GRAFFITI				=159,	/*  */
	SC_LULLABY				=160,	/*  */
	SC_RICHMANKIM			=161,	/*  */
	SC_ETERNALCHAOS			=162,	/*  */
	SC_DRUMBATTLE			=163,	/*  */
	SC_NIBELUNGEN			=164,	/*  */
	SC_ROKISWEIL			=165,	/*  */
	SC_INTOABYSS			=166,	/*  */
	SC_SIEGFRIED			=167,	/*  */
	SC_DISSONANCE			=168,	/*  */
	SC_WHISTLE				=169,	/*  */
	SC_ASSNCROS				=170,	/*  */
	SC_POEMBRAGI			=171,	/*  */
	SC_APPLEIDUN			=172,	/*  */
	SC_UGLYDANCE			=173,	/*  */
	SC_HUMMING				=174,	/*  */
	SC_DONTFORGETME			=175,	/*  */
	SC_FORTUNE				=176,	/*  */
	SC_SERVICE4U			=177,	/*  */
	SC_BASILICA				=178,	/*  */
	SC_MINDBREAKER			=179,	/*  */
	SC_SPIDERWEB			=180,	/* �X�p�C�_�[�E�F�b�u */
	SC_MEMORIZE				=181,	/* �������C�Y */
	SC_DPOISON				=182,	/* �ғ� */
	//
	SC_SACRIFICE			=184,	/* �T�N���t�@�C�X */
	SC_INCATK				=185,	//item 682�p
	SC_INCMATK				=186,	//item 683�p
	SC_WEDDING				=187,	//�����p(�����ߏւɂȂ��ĕ����̂��x���Ƃ�)
	SC_NOCHAT				=188,	//�ԃG�����
	SC_SPLASHER				=189,	/* �x�i���X�v���b�V���[ */
	SC_SELFDESTRUCTION		=190,	/* ���� */
	SC_MAGNUM				=191,	/* �}�O�i���u���C�N */
	SC_GOSPEL				=192,	/* �S�X�y�� */
	SC_INCALLSTATUS			=193,	/* �S�ẴX�e�[�^�X���㏸(���̂Ƃ���S�X�y���p) */
	SC_INCHIT				=194,	/* HIT�㏸(���̂Ƃ���S�X�y���p) */
	SC_INCFLEE				=195,	/* FLEE�㏸(���̂Ƃ���S�X�y���p) */
	SC_INCMHP2				=196,	/* MHP��%�㏸(���̂Ƃ���S�X�y���p) */
	SC_INCMSP2				=197,	/* MSP��%�㏸(���̂Ƃ���S�X�y���p) */
	SC_INCATK2				=198,	/* ATK��%�㏸(���̂Ƃ���S�X�y���p) */
	SC_INCHIT2				=199,	/* HIT��%�㏸(���̂Ƃ���S�X�y���p) */
	SC_INCFLEE2				=200,	/* FLEE��%�㏸(���̂Ƃ���S�X�y���p) */
	SC_PRESERVE				=201,	/* �v���U�[�u */
	SC_OVERTHRUSTMAX		=202,	/* �I�[�o�[�g���X�g�}�b�N�X */
	SC_CHASEWALK_STR		=203,	/*STR�㏸�i�`�F�C�X�E�H�[�N�p�j*/
	SC_WHISTLE_				=204,
	SC_ASSNCROS_			=205,
	SC_POEMBRAGI_			=206,
	SC_APPLEIDUN_			=207,
	SC_HUMMING_				=209,
	SC_DONTFORGETME_		=210,
	SC_FORTUNE_				=211,
	SC_SERVICE4U_			=212,
	SC_BATTLEORDER			=213,//�M���h�X�L��
	SC_REGENERATION			=214,
	//
	SC_POISONPOTION			=219,
	SC_THE_MAGICIAN			=220,
	SC_STRENGTH				=221,
	SC_THE_DEVIL			=222,
	SC_THE_SUN				=223,
	SC_MEAL_INCSTR			=224,//�H���p
	SC_MEAL_INCAGI			=225,
	SC_MEAL_INCVIT			=226,
	SC_MEAL_INCINT			=227,
	SC_MEAL_INCDEX			=228,
	SC_MEAL_INCLUK			=229,
	SC_RUN 					= 230,
	SC_SPURT 				= 231,
	SC_TKCOMBO 				= 232,	//�e�R���̃R���{�p
	SC_DODGE				= 233,
	SC_HERMODE				= 234,
	SC_TRIPLEATTACK_RATE_UP	= 235,	//�O�i�������A�b�v
	SC_COUNTER_RATE_UP		= 236,	//�J�E���^�[�L�b�N�������A�b�v
	SC_WARM					= 237,
	//
	SC_SUN_COMFORT			= 240,
	SC_MOON_COMFORT			= 241,
	SC_STAR_COMFORT			= 242,
	SC_FUSION				= 243,
	SC_ALCHEMIST			= 244,//��
	SC_MONK					= 245,
	SC_STAR					= 246,
	SC_SAGE					= 247,
	SC_CRUSADER				= 248,
	SC_SUPERNOVICE			= 249,
	SC_KNIGHT				= 250,
	SC_WIZARD				= 251,
	SC_PRIEST				= 252,
	SC_BARDDANCER			= 253,
	SC_ROGUE				= 254,
	SC_ASSASIN				= 255,
	SC_BLACKSMITH			= 256,
	SC_HUNTER				= 257,
	SC_SOULLINKER			= 258,
	SC_HIGH					= 259,
	SC_DEATHKINGHT			= 260,
	SC_COLLECTOR			= 261,
	SC_NINJA				= 262,
	SC_GUNNER				= 263,
	SC_KAIZEL				= 264,
	SC_KAAHI				= 265,
	SC_KAUPE				= 266,
	SC_KAITE				= 267,
	SC_SMA					= 268,	//�G�X�}�r���\���ԗp
	SC_SWOO					= 269,
	SC_SKE					= 270,
	SC_SKA					= 271,
	SC_ONEHAND				= 272,
	SC_READYSTORM			= 273,
	SC_READYDOWN			= 274,
	SC_READYTURN			= 275,
	SC_READYCOUNTER			= 276,
	SC_DODGE_DELAY			= 277,
	SC_AUTOBERSERK			= 278,
	//
	SC_DOUBLECASTING 		= 280,//�_�u���L���X�e�B���O
	SC_ELEMENTFIELD			= 281,//������
	SC_DARKELEMENT			= 282,//��
	SC_ATTENELEMENT			= 283,//�O
	SC_MIRACLE				= 284,//���z�ƌ��Ɛ��̊��
	SC_ANGEL				= 285,//���z�ƌ��Ɛ��̓V�g
	SC_FORCEWALKING			= 286,//SC_HIGHJUMP�������ړ���ԂƂ��čė��p
	SC_DOUBLE				= 287,//�_�u���X�g���C�t�B���O���
	SC_ACTION_DELAY			= 288,//
	SC_BABY					= 289,//�p�p�A�}�}�A��D��
	SC_LONGINGFREEDOM		= 290,
	SC_SHRINK				= 291,//#�V�������N#
	SC_CLOSECONFINE			= 292,//#�N���[�Y�R���t�@�C��#
	SC_SIGHTBLASTER			= 293,//#�T�C�g�u���X�^�[#
	SC_ELEMENTWATER			= 294,//#�G���������^���`�F���W��#
	//�H���p2
	SC_MEAL_INCHIT			= 295,
	SC_MEAL_INCFLEE			= 296,
	SC_MEAL_INCFLEE2		= 297,
	SC_MEAL_INCCRITICAL		= 298,
	SC_MEAL_INCDEF			= 299,
	SC_MEAL_INCMDEF			= 300,
	SC_MEAL_INCATK			= 301,
	SC_MEAL_INCMATK			= 302,
	SC_MEAL_INCEXP			= 303,
	SC_MEAL_INCJOB			= 304,
	//
	SC_ELEMENTGROUND		= 305,//�y(�Z)
	SC_ELEMENTFIRE			= 306,//��(�Z)
	SC_ELEMENTWIND			= 307,//��(�Z)
	SC_WINKCHARM			= 308,
	SC_ELEMENTPOISON		= 309,//��(�Z)
	SC_ELEMENTDARK			= 310,//��(�Z)
	SC_ELEMENTELEKINESIS	= 311,//�O(�Z)
	SC_ELEMENTUNDEAD		= 312,//�s��(�Z)
	SC_UNDEADELEMENT		= 313,//�s��(��)
	SC_ELEMENTHOLY			= 314,//��(�Z)
	SC_NPC_DEFENDER			= 315,
	SC_RESISTWATER			= 316,//�ϐ�
	SC_RESISTGROUND			= 317,//�ϐ�
	SC_RESISTFIRE			= 318,//�ϐ�
	SC_RESISTWIND			= 319,//�ϐ�
	SC_RESISTPOISON			= 320,//�ϐ�
	SC_RESISTHOLY			= 321,//�ϐ�
	SC_RESISTDARK			= 322,//�ϐ�
	SC_RESISTTELEKINESIS	= 323,//�ϐ�
	SC_RESISTUNDEAD			= 324,//�ϐ�
	SC_RESISTALL			= 325,//�ϐ�
	//�푰�ύX�H
	SC_RACEUNKNOWN			= 326,//���`
	SC_RACEUNDEAD			= 327,//�s���푰
	SC_RACEBEAST			= 328,
	SC_RACEPLANT			= 329,
	SC_RACEINSECT			= 330,
	SC_RACEFISH				= 332,
	SC_RACEDEVIL			= 333,
	SC_RACEHUMAN			= 334,
	SC_RACEANGEL			= 335,
	SC_RACEDRAGON			= 336,
	SC_TIGEREYE				= 337,
	SC_GRAVITATION_USER		= 338,
	SC_GRAVITATION			= 339,
	SC_FOGWALL				= 340,
	SC_FOGWALLPENALTY		= 341,
	SC_REDEMPTIO			= 342,
	SC_TAROTCARD			= 343,
	SC_HOLDWEB				= 344,
	SC_INVISIBLE			= 345,
	SC_DETECTING			= 346,
	//
	SC_FLING				= 347,
	SC_MADNESSCANCEL		= 348,
	SC_ADJUSTMENT			= 349,
	SC_INCREASING			= 350,
	SC_DISARM				= 351,
	SC_GATLINGFEVER			= 352,
	SC_FULLBUSTER			= 353,
	//�j���W���X�L��
	SC_TATAMIGAESHI			= 354,
	SC_UTSUSEMI				= 355,//#NJ_UTSUSEMI#
	SC_BUNSINJYUTSU			= 356,//#NJ_BUNSINJYUTSU#
	SC_SUITON				= 357,//#NJ_SUITON#
	SC_NEN					= 358,//#NJ_NEN#
	SC_AVOID				= 359,//#�ً}���#
	SC_CHANGE				= 360,//#�����^���`�F���W#
	SC_DEFENCE				= 361,//#�f�B�t�F���X#
	SC_BLOODLUST			= 362,//#�u���b�h���X�g#
	SC_FLEET				= 363,//#�t���[�g���[�u#
	SC_SPEED				= 364,//#�I�[�o�[�h�X�s�[�h#
	//
	SC_ADRENALINE2			= 365,
	SC_STATUS_UNCHANGE		= 366,/* ��Ԉُ�ϐ��i�S�X�y���p�j*/
	SC_INCDAMAGE			= 367,/* ��_���[�W��%�㏸�i�S�X�y���p�j*/
	SC_COMBATHAN			= 368,//�퓬����
	SC_LIFEINSURANCE		= 369,//�����ی���
	//370�F�o�u���K��
	//371�F�ʖʋ�
	SC_MEAL_INCSTR2			= 372,//�H���p(�ۋ��A�C�e��)
	SC_MEAL_INCAGI2			= 373,
	SC_MEAL_INCVIT2			= 374,
	SC_MEAL_INCDEX2			= 375,
	SC_MEAL_INCINT2			= 376,
	SC_MEAL_INCLUK2			= 377,

	//start�ł͎g���Ȃ�resist���A�C�e�����őS�ăN���A���邽�߂̕�
	SC_RESISTCLEAR			= 1001,
	SC_RACECLEAR			= 1002,
	SC_SOUL					= 1003,
	SC_SOULCLEAR			= 1004,
};

//��ԃA�C�R��
//�����ɂ̓L�����N�^�[�̐F�̕ω��Ȃǂ��܂܂�Ă���(�����g���Ȃ�)
enum {
	SI_BLANK				= 43,//�����Ă��󔒂̂���

	SI_PROVOKE				= 0,
	SI_ENDURE				= 1,
	SI_TWOHANDQUICKEN		= 2,
	SI_CONCENTRATE			= 3,
	SI_HIDING				= 4,
	SI_CLOAKING				= 5,
	SI_ENCPOISON			= 6,
	SI_POISONREACT			= 7,
	SI_QUAGMIRE				= 8,
	SI_ANGELUS				= 9,
	SI_BLESSING				=10,
	SI_SIGNUMCRUCIS			=11,
	SI_INCREASEAGI			=12,
	SI_DECREASEAGI			=13,
	SI_SLOWPOISON			=14,
	SI_IMPOSITIO			=15,
	SI_SUFFRAGIUM			=16,
	SI_ASPERSIO				=17,
	SI_BENEDICTIO			=18,
	SI_KYRIE				=19,
	SI_MAGNIFICAT			=20,
	SI_GLORIA				=21,
	SI_AETERNA				=22,
	SI_ADRENALINE			=23,
	SI_WEAPONPERFECTION		=24,
	SI_OVERTHRUST			=25,
	SI_MAXIMIZEPOWER		=26,
	SI_RIDING				=27,
	SI_FALCON				=28,
	SI_TRICKDEAD			=29,
	SI_LOUD					=30,
	SI_ENERGYCOAT			=31,
	SI_BREAKARMOR			=32,	// �h��j��
	SI_BREAKWEAPON			=33,	// ����j��
	SI_HALLUCINATION		=34,
	SI_WEIGHT50				=35,
	SI_WEIGHT90				=36,
	SI_SPEEDPOTION0			=37,	// �X�s�[�h�A�b�v�|�[�V����
	SI_SPEEDPOTION1			=38,	// �n�C�X�s�[�h�|�[�V����
	SI_SPEEDPOTION2			=39,	// �o�[�T�N�|�[�V����
	SI_SPEEDPOTION3			=40,	// �T�[�o�[�N�s�b�`���[�H
	SI_INCREASEAGI2			=41,	// �n�v�H
	SI_INCREASEAGI3			=42,	// �ړ����x�����|�[�V�����H
	SI_STRIPWEAPON			=50,
	SI_STRIPSHIELD			=51,
	SI_STRIPARMOR			=52,
	SI_STRIPHELM			=53,
	SI_CP_WEAPON			=54,
	SI_CP_SHIELD			=55,
	SI_CP_ARMOR				=56,
	SI_CP_HELM				=57,
	SI_AUTOGUARD			=58,
	SI_REFLECTSHIELD		=59,
	SI_DEVOTION				=60,
	SI_PROVIDENCE			=61,
	SI_DEFENDER				=62,
	SI_AUTOSPELL			=65,
	SI_SPEARSQUICKEN		=68,
	SI_EXPLOSIONSPIRITS		=86,	/* �����g�� */
	SI_STEELBODY			=87,	/* �����G�t�F�N�g */
	SI_COMBO				=89,
	SI_FLAMELAUNCHER		=90,	/* �t���C�������`���[ */
	SI_FROSTWEAPON			=91,	/* �t���X�g�E�F�|�� */
	SI_LIGHTNINGLOADER		=92,	/* ���C�g�j���O���[�_�[ */
	SI_SEISMICWEAPON		=93,	/* �T�C�Y�~�b�N�E�F�|�� */
	SI_HOLDWEB				=95,	/* �z�[���h�E�F�u */
	SI_UNDEAD				=97,	/* �s�������t�^ */
	SI_AURABLADE			=103,	/* �I�[���u���[�h */
	SI_PARRYING				=104,	/* �p���C���O */
	SI_CONCENTRATION		=105,	/* �R���Z���g���[�V���� */
	SI_TENSIONRELAX			=106,	/* �e���V���������b�N�X */
	SI_BERSERK				=107,	/* �o�[�T�[�N */
	SI_ASSUMPTIO			=110,	/* �A�X���v�e�B�I */
	SI_ELEMENTFIELD 		=112,	/* ������ */
	SI_MAGICPOWER			=113,	/* ���@�͑��� */
	SI_EDP					=114, 	//
	SI_TRUESIGHT			=115,	/* �g�D���[�T�C�g */
	SI_WINDWALK				=116,	/* �E�C���h�E�H�[�N */
	SI_MELTDOWN				=117,	/* �����g�_�E�� */
	SI_CARTBOOST			=118,	/* �J�[�g�u�[�X�g */
	SI_CHASEWALK			=119,	/* �`�F�C�X�E�H�[�N */
	SI_REJECTSWORD			=120,	/* ���W�F�N�g�\�[�h */
	SI_MARIONETTE			=121,	/* �}���I�l�b�g�R���g���[�� */
	SI_MARIONETTE2			=122,	/* �}���I�l�b�g�R���g���[��2 */
	SI_MOONLIT 				=123,	//��������̉���
	SI_HEADCRUSH			=124,	/* �w�b�h�N���b�V�� */
	SI_JOINTBEAT			=125,	/* �W���C���g�r�[�g */
	SI_BABY					=130,	//�p�p�A�}�}�A��D��
	SI_MAGNUM				=131,	//�}�O�i���u���C�N
	SI_AUTOBERSERK			=132,	//�I�[�g�o�[�T�[�N
	SI_RUN					=133,	//�^�C���M
	SI_RUN_STOP				=134,	//�^�C���M�Փ�
	SI_READYSTORM			=135,	//��������
	SI_READYDOWN			=137,	//���i����
	SI_READYTURN			=139,	//��]����
	SI_READYCOUNTER			=141,	//�J�E���^�[����
	SI_DODGE				=143,	//���@
	SI_SPURT				=145,	//�삯���X�p�[�g
	SI_DARKELEMENT			=146,	//��
	SI_ADRENALINE2			=147,	//�t���A�h���i�������b�V��
	SI_ATTENELEMENT			=148,	//�O
	SI_SOULLINK				=149,	//�����
	SI_DEVIL				=152,	//���z�ƌ��Ɛ��̈���
	SI_KAITE				=153,	//�J�C�g
	SI_KAIZEL				=156,	//�J�C�[��
	SI_KAAHI				=157,	//�J�A�q
	SI_KAUPE				=158,	//�J�E�v
	SI_SMA					=159,	//�G�X�}�\���
	SI_MIRACLE				=160,	//���z�ƌ��Ɛ��̊��
	SI_ONEHAND				=161,	//�����n���h�N�C�b�P��
	SI_WARM					=165,	//������
	//SI_SUN_WARM			=165,	//���z�̉�����
	//SI_MOON_WARM			=166,	//���̉�����
	//SI_STAR_WARM			=167,	//���̉�����
	SI_SUN_COMFORT			=169,	//���z�̈��y
	SI_MOON_COMFORT			=170,	//���̈��y
	SI_STAR_COMFORT			=171,	//���̈��y
	SI_PRESERVE				=181,	//�v���U�[�u
	SI_CHASEWALK_STR		=182,	//�`�F�C�X�E�H�[�N��STR?
	SI_TIGEREYE				=184,	//�^�C�K�[�A�C�i�n�C�h���j��j
	SI_DOUBLECASTING		=186,	//�_�u���L���X�e�B���O
	SI_OVERTHRUSTMAX		=188,	//�I�[�o�[�g���X�g�}�b�N�X
	SI_TAROTCARD			=191,
	SI_SHRINK				=197,	//#�V�������N#
	SI_SIGHTBLASTER			=198,	//#�T�C�g�u���X�^�[#
	SI_CLOSECONFINE			=200,	//#�N���[�Y�R���t�@�C��#
	SI_CLOSECONFINE2		=201,	//#�N���[�Y�R���t�@�C��#
	SI_MADNESSCANCEL		=203,	//#GS_MADNESSCANCEL#
	SI_GATLINGFEVER			=204,	//#GS_GATLINGFEVER#
	SI_UTSUSEMI				=206,	//#NJ_UTSUSEMI#
	SI_BUNSINJYUTSU			=207,	//#NJ_BUNSINJYUTSU#
	SI_NEN					=208,	//#NJ_NEN#
	SI_ADJUSTMENT			=209,	//#GS_ADJUSTMENT#
	SI_INCREASING			=210,	//#GS_INCREASING#
	SI_MEAL_INCSTR			=241,	//�H���p
	SI_MEAL_INCAGI			=242,
	SI_MEAL_INCVIT			=243,
	SI_MEAL_INCDEX			=244,
	SI_MEAL_INCINT			=245,
	SI_MEAL_INCLUK			=246,
	SI_MEAL_INCFLEE			=247,
	SI_MEAL_INCHIT			=248,
	SI_MEAL_INCCRITICAL		=249,
	SI_COMBATHAN			=250,	//�퓬����
	SI_LIFEINSURANCE		=251,	//�����ی���
	//252�F�o�u���K��
	//253�F�ʖʋ�
	SI_MEAL_INCSTR2			=271,	//�H���p(�ۋ��A�C�e��)
	SI_MEAL_INCAGI2			=272,
	SI_MEAL_INCVIT2			=273,
	SI_MEAL_INCDEX2			=274,
	SI_MEAL_INCINT2			=275,
	SI_MEAL_INCLUK2			=276,
	//282�F�r���x����
	//286�F�v����
};

#endif
