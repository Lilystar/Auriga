// Original : mmo.h 2003/03/14 12:07:02 Rev.1.7

#ifndef	_MMO_H_
#define	_MMO_H_

#include <stdio.h>
#include <string.h>

#pragma pack(4)

#define AUTH_FIFO_SIZE 256

#define MAX_INVENTORY 100
#define MAX_AMOUNT 30000
#define MAX_ZENY 1000000000	// 1G zeny
#define MAX_CART 100
#define MAX_SKILL 1020
#define MAX_HOMSKILL 16
#define GLOBAL_REG_NUM 96
#define ACCOUNT_REG_NUM 16
#define ACCOUNT_REG2_NUM 16
#define DEFAULT_WALK_SPEED 150
#define MIN_WALK_SPEED 0
#define MAX_WALK_SPEED 1000
#define MAX_STORAGE 300
#define MAX_GUILD_STORAGE 1000
#define MAX_PARTY 12
#define MAX_GUILD 76
#define MAX_GUILDPOSITION 20
#define MAX_GUILDEXPLUSION 32
#define MAX_GUILDALLIANCE 16
#define MAX_GUILDSKILL  15
#define MAX_GUILDCASTLE 20
#define MAX_GUILDLEVEL 50
#define MAX_FRIEND 20
#define MAX_STATUSCHANGE 370

#define MAX_HAIR_STYLE 24
#define MAX_HAIR_COLOR 9
#define MAX_CLOTH_COLOR 5

// for produce
#define MIN_ATTRIBUTE 0
#define MAX_ATTRIBUTE 4
#define ATTRIBUTE_NORMAL 0
#define MIN_STAR 0
#define MAX_STAR 3

#define MAX_PORTAL_MEMO 3

#define WEDDING_RING_M 2634
#define WEDDING_RING_F 2635

#define MAIL_STORE_MAX 30

#define MAX_HOMUN_DB 16			// �z���̐�
#define HOM_ID 6001				// ID�J�n�l
#define HOM_SKILLID 8001		// �z���X�L��ID�̊J�n�l
#define MAX_HOM_SKILLID (HOM_SKILLID+MAX_HOMSKILL)	// �z���X�L��ID�̍ő�l

#define GRF_PATH_FILENAME "conf/grf-files.txt"

// �u���b�NID��`
#define MIN_FLOORITEM            2
#define MAX_FLOORITEM       500000
#define START_ACCOUNT_NUM  2000000
#define END_ACCOUNT_NUM    5000000
#define START_NPC_NUM      5100000
#define END_NPC_NUM       16777215	// SL_SMA�̃G�t�F�N�g�\���\��� = 0x00ffffff

struct item {
	int id;
	short nameid;
	short amount;
	unsigned short equip;
	char identify;
	char refine;
	char attribute;
	short card[4];
};
struct point{
	char map[24];
	short x,y;
};
struct skill {
	unsigned short id,lv,flag;
};
struct global_reg {
	char str[32];
	int value;
};
struct s_pet {
	int account_id;
	int char_id;
	int pet_id;
	short class;
	short level;
	short egg_id;//pet egg id
	short equip;//pet equip name_id
	short intimate;//pet friendly
	short hungry;//pet hungry
	char name[24];
	char rename_flag;
	char incubate;
};

struct friend_data {
	int account_id;
	int char_id;
	char name[24];
};

struct mmo_charstatus {
	int char_id;
	int account_id;
	int partner_id;
	int parent_id[2];
	int baby_id;

	int base_exp,job_exp,zeny;

	short class;
	short status_point,skill_point;
	int hp,max_hp,sp,max_sp;
	short option,karma,manner;
	short hair,hair_color,clothes_color;
	int party_id,guild_id,pet_id,homun_id;

	short weapon,shield;
	short head_top,head_mid,head_bottom;

	char name[24];
	unsigned char base_level,job_level;
	short str,agi,vit,int_,dex,luk;
	unsigned char char_num;

	struct point last_point, save_point, memo_point[MAX_PORTAL_MEMO];
	char feel_map[3][24];
	struct item inventory[MAX_INVENTORY],cart[MAX_CART];
	struct skill skill[MAX_SKILL];
	int friend_num;
	struct friend_data friend_data[MAX_FRIEND];
};

struct registry {
	int global_num;
	struct global_reg global[GLOBAL_REG_NUM];
	int account_num;
	struct global_reg account[ACCOUNT_REG_NUM];
	int account2_num;
	struct global_reg account2[ACCOUNT_REG2_NUM];
};

struct charid2nick {
	char nick[24];
	int req_id;
	int account_id;
	unsigned long ip;
	unsigned int port;
};

struct mmo_homunstatus {
	int account_id;
	int char_id;
	int homun_id;
	short class;
	int base_exp;

	short status_point,skill_point;
	int hp,max_hp,sp,max_sp;

	char name[24];
	unsigned char base_level;
	short str,agi,vit,int_,dex,luk;

	short option;
	short equip;

	struct skill skill[MAX_HOMSKILL];

	int intimate;	// �y�b�g�ƈႢ�A�ő�100,000�Ōv�Z
	short hungry;
	char rename_flag;
	short incubate;
};

struct storage {
	int account_id;
	short storage_status;
	short storage_amount;
	struct item storage[MAX_STORAGE];
};

struct guild_storage {
	int guild_id;
	short storage_status;
	short storage_amount;
	struct item storage[MAX_GUILD_STORAGE];
};

struct map_session_data;

struct gm_account {
	int account_id;
	int level;
};

struct party_member {
	int account_id;
	char name[24],map[24];
	int leader,online,lv;
	struct map_session_data *sd;
};
struct party {
	int party_id;
	char name[24];
	int exp;
	int item;
	struct party_member member[MAX_PARTY];
};

struct guild_member {
	int account_id, char_id;
	short hair,hair_color,gender,class,lv;
	int exp,exp_payper;
	unsigned char online;
	short position;
	int rsv1,rsv2;
	char name[24];
	struct map_session_data *sd;
};
struct guild_position {
	char name[24];
	int mode;
	int exp_mode;
};
struct guild_alliance {
	int opposition;
	int guild_id;
	char name[24];
};
struct guild_explusion {
	char name[24];
	char mes[40];
	char acc[40];
	int account_id;
	int rsv1,rsv2,rsv3;
};
struct guild_skill {
	int id,lv;
};
struct guild {
	int guild_id;
	short guild_lv, connect_member, max_member, average_lv;
	int exp,next_exp,skill_point,castle_id;
	char name[24],master[24];
	struct guild_member member[MAX_GUILD];
	struct guild_position position[MAX_GUILDPOSITION];
	char mes1[60],mes2[120];
	int emblem_len,emblem_id;
	char emblem_data[2048];
	struct guild_alliance alliance[MAX_GUILDALLIANCE];
	struct guild_explusion explusion[MAX_GUILDEXPLUSION];
	struct guild_skill skill[MAX_GUILDSKILL];
};
struct guild_castle {
	int castle_id;
	char map_name[24];
	char castle_name[24];
	char castle_event[24];
	int guild_id;
	int economy;
	int defense;
	int triggerE;
	int triggerD;
	int nextTime;
	int payTime;
	int createTime;
	int visibleC;
	int visibleG0;
	int visibleG1;
	int visibleG2;
	int visibleG3;
	int visibleG4;
	int visibleG5;
	int visibleG6;
	int visibleG7;
};
struct square {
	int val1[5];
	int val2[5];
};

enum {
	GBI_EXP			=1,		// �M���h��EXP
	GBI_GUILDLV		=2,		// �M���h��Lv
	GBI_SKILLPOINT	=3,		// �M���h�̃X�L���|�C���g
	GBI_SKILLLV		=4,		// �M���h�X�L��Lv

	GMI_POSITION	=0,		// �����o�[�̖�E�ύX
	GMI_EXP			=1,		// �����o�[��EXP

};
struct mail {
	int account_id;
	int char_id;
	int rates;	// ������
	int store;	// �ۗL����
};
struct mail_data {
	int mail_num;
	//���M��
	int char_id;
	char char_name[24];
	//���l
	int receive_id;
	char receive_name[24];
	int read;
	unsigned int times;
	char title[40];
	char body[35*14];
	unsigned int body_size;
	int zeny;
	struct item item;
};

#pragma pack()


#endif	// _MMO_H_
