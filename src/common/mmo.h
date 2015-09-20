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
#define MAX_GUILDCASTLE 30
#define MAX_GUILDLEVEL 50
#define MAX_FRIEND 20
#define MAX_STATUSCHANGE 390
#define MAX_PORTAL_MEMO 3
#define MAIL_STORE_MAX 30
#define MAX_HOTKEYS 27

#define MAX_HAIR_STYLE 24
#define MAX_HAIR_COLOR 9
#define MAX_CLOTH_COLOR 5

#define WEDDING_RING_M 2634
#define WEDDING_RING_F 2635

#define MAX_RANKING 4	// �����L���O��
#define MAX_RANKER  10	// �����L���O�l��

#define MAX_HOMUN_DB 16			// �z���̐�
#define HOM_ID 6001			// �z��ID�J�n�l

#define MAX_MERC_DB 30			// �b���̐�
#define MERC_ID 6017			// �b��ID�̊J�n�l
#define MAX_MERC_TYPE 3			// �b���̎��

// �X�L��ID��`
#define HOM_SKILLID    8001		// �z���X�L��ID�̊J�n�l
#define MERC_SKILLID   8201		// �b���X�L��ID�̊J�n�l
#define GUILD_SKILLID 10000		// �M���h�X�L��ID�̊J�n�l

#define MAX_SKILL      1020
#define MAX_HOMSKILL     16
#define MAX_MERCSKILL    37
#define MAX_GUILDSKILL   16

#define MAX_HOM_SKILLID   (HOM_SKILLID+MAX_HOMSKILL)		// �z���X�L��ID�̍ő�l
#define MAX_MERC_SKILLID  (MERC_SKILLID+MAX_MERCSKILL)		// �b���X�L��ID�̍ő�l
#define MAX_GUILD_SKILLID (GUILD_SKILLID+MAX_GUILDSKILL)	// �M���h�X�L��ID�̍ő�l

#define MAX_SKILL_DB (MAX_SKILL+MAX_HOMSKILL+MAX_MERCSKILL+MAX_GUILDSKILL)

// �u���b�NID��`
#define MIN_FLOORITEM            2
#define MAX_FLOORITEM       500000
#define START_ACCOUNT_NUM  2000000
#define END_ACCOUNT_NUM    5000000
#define START_NPC_NUM      5100000
#define END_NPC_NUM       16777215	// SL_SMA�̃G�t�F�N�g�\���\��� = 0x00ffffff

// �E�ƒ�`
#ifdef CLASS_DKDC
	#define MAX_VALID_PC_CLASS 32
#else
	#define MAX_VALID_PC_CLASS 30
#endif

#define PC_CLASS_NV      0  	// �m�r
#define PC_CLASS_NV2  4001  	// �]���m�r
#define PC_CLASS_NV3  4023  	// �{�q�m�r
#define PC_CLASS_SNV    23  	// �X�p�m�r
#define PC_CLASS_SNV3 4045  	// �{�q�X�p�m�r
#define PC_CLASS_TK   4046	// �e�R��
#define PC_CLASS_SG   4047	// ����
#define PC_CLASS_SG2  4048	// ����2
#define PC_CLASS_SL   4049	// �\�E�������J�[
#define PC_CLASS_GS     28	// �K���X�����K�[
#define PC_CLASS_NJ     29	// �E��
#define PC_CLASS_DK   4051	// �f�X�i�C�g
#define PC_CLASS_DC   4052	// �_�[�N�R���N�^�[

#define MAX_PC_CLASS (1+6+6+1+6+1+1+1+1+4+2+2)
#define PC_CLASS_BASE 0
#define PC_CLASS_BASE2 (PC_CLASS_BASE + 4001)
#define PC_CLASS_BASE3 (PC_CLASS_BASE2 + 22)

struct item {
	unsigned int id;
	short nameid;
	short amount;
	unsigned short equip;
	char identify;
	char refine;
	char attribute;
	short card[4];
	unsigned int limit;
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
	short class_;
	unsigned short level;
	short egg_id;
	short equip;
	short intimate;
	short hungry;
	char name[24];
	char rename_flag;
	char incubate;
};

struct friend_data {
	int account_id;
	int char_id;
	char name[24];
};

struct hotkey {
	int id;
	unsigned short lv;
	char type;
};

struct mmo_charstatus {
	int char_id;
	int account_id;
	int partner_id;
	int parent_id[2];
	int baby_id;

	int base_exp,job_exp,zeny;

	short class_;
	short status_point,skill_point;
	int hp,max_hp,sp,max_sp;
	unsigned int option;
	short karma,manner;
	int die_counter;
	short hair,hair_color,clothes_color;
	int party_id,guild_id,pet_id,homun_id,merc_id;

	int merc_fame[MAX_MERC_TYPE],merc_call[MAX_MERC_TYPE];

	short weapon,shield;
	short head_top,head_mid,head_bottom;

	char name[24];
	unsigned short base_level,job_level;
	short str,agi,vit,int_,dex,luk;
	unsigned char char_num;

	struct point last_point, save_point, memo_point[MAX_PORTAL_MEMO];
	char feel_map[3][24];
	struct item inventory[MAX_INVENTORY],cart[MAX_CART];
	struct skill skill[MAX_SKILL];
	int friend_num;
	struct friend_data friend_data[MAX_FRIEND];
	struct hotkey hotkey[MAX_HOTKEYS];
};

struct registry {
	int global_num;
	struct global_reg global[GLOBAL_REG_NUM];
	int account_num;
	struct global_reg account[ACCOUNT_REG_NUM];
	int account2_num;
	struct global_reg account2[ACCOUNT_REG2_NUM];
};

struct mmo_homunstatus {
	int account_id;
	int char_id;
	int homun_id;
	short class_;
	int base_exp;

	short status_point,skill_point;
	int hp,max_hp,sp,max_sp;

	char name[24];
	unsigned short base_level;
	short str,agi,vit,int_,dex,luk;
	short f_str,f_agi,f_vit,f_int,f_dex,f_luk;

	short equip;
	unsigned int option;

	struct skill skill[MAX_HOMSKILL];

	int intimate;	// �y�b�g�ƈႢ�ő�100,000�Ōv�Z
	short hungry;
	char rename_flag;
	char incubate;
};

struct mmo_mercstatus {
	int account_id;
	int char_id;
	int merc_id;
	short class_;
	int hp,max_hp,sp,max_sp;
	char name[24];
	unsigned short base_level;
	short str,agi,vit,int_,dex,luk;
	unsigned int option;
	struct skill skill[MAX_MERCSKILL];
	unsigned int kill_count;
	unsigned int limit;
};

struct storage {
	int account_id;
	char dirty;
	char storage_status;
	short storage_amount;
	unsigned int sortkey;
	struct item store_item[MAX_STORAGE];
};

struct guild_storage {
	int guild_id;
	char dirty;
	char storage_status;
	short storage_amount;
	unsigned int sortkey;
	int last_fd;
	struct item store_item[MAX_GUILD_STORAGE];
};

struct map_session_data;

struct gm_account {
	int account_id;
	int level;
};

struct party_member {
	int account_id;
	int char_id;
	char name[24],map[24];
	unsigned char leader,online;
	unsigned short lv;
	struct map_session_data *sd;
};
struct party {
	int party_id;
	char name[24];
	unsigned char exp;
	unsigned char item;
	struct party_member member[MAX_PARTY];
};

struct guild_member {
	int account_id, char_id;
	short hair,hair_color,gender,class_;
	unsigned short lv;
	int exp,exp_payper;
	unsigned char online;
	short position;
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
	int account_id;
};
struct guild_skill {
	int id,lv;
};
struct guild {
	int guild_id;
	short guild_lv, connect_member, max_member;
	unsigned short average_lv;
	int exp,next_exp,skill_point;
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
	int m;
	char map_name[24];
	char area_name[24];
	char castle_name[32];
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
	struct {
		int visible;
		int id;
	} guardian[8];
};
struct square {
	int val1[5];
	int val2[5];
};

enum {
	GBI_EXP        = 1,	// �M���h��EXP
	GBI_GUILDLV    = 2,	// �M���h��Lv
	GBI_SKILLPOINT = 3,	// �M���h�̃X�L���|�C���g
	GBI_SKILLLV    = 4,	// �M���h�X�L��Lv

	GMI_POSITION   = 0,	// �����o�[�̖�E�ύX
	GMI_EXP        = 1,	// �����o�[��EXP
};

enum {
	GD_APPROVAL = 10000,
	GD_KAFRACONTACT,
	GD_GUARDIANRESEARCH,
	GD_GUARDUP,
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
	GD_ITEMEMERGENCYCALL,
};

struct mail {
	int account_id;
	int char_id;
	unsigned int rates;	// ������
	int store;		// �ۗL����
};
struct mail_data {
	unsigned int mail_num;
	int char_id;
	char char_name[24];
	char receive_name[24];
	int read;
	unsigned int times;
	char title[40];
	char body[35*14];
	unsigned int body_size;
	int zeny;
	struct item item;
};

struct Ranking_Data {
	char name[24];
	int point;
	int char_id;
};

#pragma pack()


#endif	// _MMO_H_
