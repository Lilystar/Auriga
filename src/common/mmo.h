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
#define MAX_STORAGE 600
#define MAX_GUILD_STORAGE 1000
#define MAX_PARTY 12
#define MAX_GUILD 76
#define MAX_GUILDPOSITION 20
#define MAX_GUILDEXPLUSION 32
#define MAX_GUILDALLIANCE 16
#define MAX_GUILDCASTLE 30
#define MAX_GUILDLEVEL 50
#define MAX_FRIEND 20
#define MAX_STATUSCHANGE 600
#define MAX_PORTAL_MEMO 3
#define MAIL_STORE_MAX 30

#if PACKETVER >= 21
	#define MAX_HOTKEYS 38
#elif PACKETVER >= 20
	#define MAX_HOTKEYS 36
#else
	#define MAX_HOTKEYS 27
#endif

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
#define SECOND_SKILLID 1001		// 2���E�N�G�X�g�X�L��ID�̊J�n�l
#define THIRD_SKILLID  2001		// 3���E�X�L��ID�̊J�n�l
#define HOM_SKILLID    8001		// �z���X�L��ID�̊J�n�l
#define MERC_SKILLID   8201		// �b���X�L��ID�̊J�n�l
#define GUILD_SKILLID 10000		// �M���h�X�L��ID�̊J�n�l

#define MAX_SKILL       700
#define MAX_SECONDSKILL  19
#define MAX_THIRDSKILL  518
#define MAX_HOMSKILL     16
#define MAX_MERCSKILL    37
#define MAX_GUILDSKILL   16

#define MAX_SECOND_SKILLID (SECOND_SKILLID+MAX_SECONDSKILL)	// 2���E�N�G�X�g�X�L��ID�̍ő�l
#define MAX_THIRD_SKILLID  (THIRD_SKILLID+MAX_THIRDSKILL)	// 3���E�X�L��ID�̍ő�l
#define MAX_HOM_SKILLID    (HOM_SKILLID+MAX_HOMSKILL)		// �z���X�L��ID�̍ő�l
#define MAX_MERC_SKILLID   (MERC_SKILLID+MAX_MERCSKILL)		// �b���X�L��ID�̍ő�l
#define MAX_GUILD_SKILLID  (GUILD_SKILLID+MAX_GUILDSKILL)	// �M���h�X�L��ID�̍ő�l

#define MAX_SKILL_DB (MAX_SKILL+MAX_SECONDSKILL+MAX_THIRDSKILL+MAX_HOMSKILL+MAX_MERCSKILL+MAX_GUILDSKILL)
#define MAX_PCSKILL  MAX_THIRD_SKILLID		// PC���g�p�\�̍ő�̃X�L��ID

// �u���b�NID��`
#define MIN_FLOORITEM            2
#define MAX_FLOORITEM       500000
#define START_ACCOUNT_NUM  2000000
#define END_ACCOUNT_NUM    5000000
#define START_NPC_NUM      5100000
#define END_NPC_NUM       16777215	// SL_SMA�̃G�t�F�N�g�\���\��� = 0x00ffffff

// �N���XID��`
enum {
	PC_CLASS_NV = 0,		// �m�[�r�X
	PC_CLASS_SM,			// �\�[�h�}��
	PC_CLASS_MG,			// �}�W�V����
	PC_CLASS_AC,			// �A�[�`���[
	PC_CLASS_AL,			// �A�R���C�g
	PC_CLASS_MC,			// �}�[�`�����g
	PC_CLASS_TF,			// �V�[�t
	PC_CLASS_KN,			// �i�C�g
	PC_CLASS_PR,			// �v���[�X�g
	PC_CLASS_WZ,			// �E�B�U�[�h
	PC_CLASS_BS,			// �u���b�N�X�~�X
	PC_CLASS_HT,			// �n���^�[
	PC_CLASS_AS,			// �A�T�V��
	PC_CLASS_KN2,			// �i�C�g(�R��)
	PC_CLASS_CR,			// �N���Z�C�_�[
	PC_CLASS_MO,			// �����N
	PC_CLASS_SA,			// �Z�[�W
	PC_CLASS_RG,			// ���[�O
	PC_CLASS_AM,			// �A���P�~�X�g
	PC_CLASS_BA,			// �o�[�h
	PC_CLASS_DC,			// �_���T�[
	PC_CLASS_CR2,			// �N���Z�C�_�[(�R��)
	PC_CLASS_WE,			// �E�F�f�B���O
	PC_CLASS_SNV,			// �X�[�p�[�m�[�r�X
	PC_CLASS_GS,			// �K���X�����K�[
	PC_CLASS_NJ,			// �E��
	PC_CLASS_NV_H = 4001,	// �]���m�[�r�X
	PC_CLASS_SM_H,			// �]���\�[�h�}��
	PC_CLASS_MG_H,			// �]���}�W�V����
	PC_CLASS_AC_H,			// �]���A�[�`���[
	PC_CLASS_AL_H,			// �]���A�R���C�g
	PC_CLASS_MC_H,			// �]���}�[�`�����g
	PC_CLASS_TF_H,			// �]���V�[�t
	PC_CLASS_KN_H,			// ���[�h�i�C�g
	PC_CLASS_PR_H,			// �n�C�v���[�X�g
	PC_CLASS_WZ_H,			// �n�C�E�B�U�[�h
	PC_CLASS_BS_H,			// �z���C�g�X�~�X
	PC_CLASS_HT_H,			// �X�i�C�p�[
	PC_CLASS_AS_H,			// �A�T�V���N���X
	PC_CLASS_KN2_H,			// ���[�h�i�C�g(�R��)
	PC_CLASS_CR_H,			// �p���f�B��
	PC_CLASS_MO_H,			// �`�����s�I��
	PC_CLASS_SA_H,			// �v���t�F�b�T�[
	PC_CLASS_RG_H,			// �`�F�C�T�[
	PC_CLASS_AM_H,			// �N���G�C�^�[
	PC_CLASS_BA_H,			// �N���E��
	PC_CLASS_DC_H,			// �W�v�V�[
	PC_CLASS_CR2_H,			// �p���f�B��(�R��)
	PC_CLASS_NV_B,			// �{�q�m�[�r�X
	PC_CLASS_SM_B,			// �{�q�\�[�h�}��
	PC_CLASS_MG_B,			// �{�q�}�W�V����
	PC_CLASS_AC_B,			// �{�q�A�[�`���[
	PC_CLASS_AL_B,			// �{�q�A�R���C�g
	PC_CLASS_MC_B,			// �{�q�}�[�`�����g
	PC_CLASS_TF_B,			// �{�q�V�[�t
	PC_CLASS_KN_B,			// �{�q�i�C�g
	PC_CLASS_PR_B,			// �{�q�v���[�X�g
	PC_CLASS_WZ_B,			// �{�q�E�B�U�[�h
	PC_CLASS_BS_B,			// �{�q�u���b�N�X�~�X
	PC_CLASS_HT_B,			// �{�q�n���^�[
	PC_CLASS_AS_B,			// �{�q�A�T�V��
	PC_CLASS_KN2_B,			// �{�q�i�C�g(�R��)
	PC_CLASS_CR_B,			// �{�q�N���Z�C�_�[
	PC_CLASS_MO_B,			// �{�q�����N
	PC_CLASS_SA_B,			// �{�q�Z�[�W
	PC_CLASS_RG_B,			// �{�q���[�O
	PC_CLASS_AM_B,			// �{�q�A���P�~�X�g
	PC_CLASS_BA_B,			// �{�q�o�[�h
	PC_CLASS_DC_B,			// �{�q�_���T�[
	PC_CLASS_CR2_B,			// �{�q�N���Z�C�_�[(�R��)
	PC_CLASS_SNV_B,			// �{�q�X�[�p�[�m�[�r�X
	PC_CLASS_TK,			// �e�R���L�b�h
	PC_CLASS_SG,			// ����
	PC_CLASS_SG2,			// ����(�Z��)
	PC_CLASS_SL,			// �\�E�������J�[
	PC_CLASS_MB,			// �L�����V�[
	PC_CLASS_DK,			// �f�X�i�C�g
	PC_CLASS_DA,			// �_�[�N�R���N�^�[
	// 4053
	PC_CLASS_RK = 4054,		// ���[���i�C�g
	PC_CLASS_WL,			// �E�H�[���b�N
	PC_CLASS_RA,			// �����W���[
	PC_CLASS_AB,			// �A�[�N�r�V���b�v
	PC_CLASS_NC,			// ���J�j�b�N
	PC_CLASS_GC,			// �M���`���N���X
	PC_CLASS_RK_H,			// �]�����[���i�C�g
	PC_CLASS_WL_H,			// �]���E�H�[���b�N
	PC_CLASS_RA_H,			// �]�������W���[
	PC_CLASS_AB_H,			// �]���A�[�N�r�V���b�v
	PC_CLASS_NC_H,			// �]�����J�j�b�N
	PC_CLASS_GC_H,			// �]���M���`���N���X
	PC_CLASS_LG,			// ���C�����K�[�h
	PC_CLASS_SO,			// �\�[�T���[
	PC_CLASS_MI,			// �~���X�g����
	PC_CLASS_WA,			// �����_���[
	PC_CLASS_SR,			// �C��
	PC_CLASS_GN,			// �W�F�l�e�B�b�N
	PC_CLASS_SC,			// �V���h�E�`�F�C�T�[
	PC_CLASS_LG_H,			// �]�����C�����K�[�h
	PC_CLASS_SO_H,			// �]���\�[�T���[
	PC_CLASS_MI_H,			// �]���~���X�g����
	PC_CLASS_WA_H,			// �]�������_���[
	PC_CLASS_SR_H,			// �]���C��
	PC_CLASS_GN_H,			// �]���W�F�l�e�B�b�N
	PC_CLASS_SC_H,			// �]���V���h�E�`�F�C�T�[
	PC_CLASS_RK2,			// ���[���i�C�g(�R��)
	PC_CLASS_RK2_H,			// �]�����[���i�C�g(�R��)
	PC_CLASS_LG2,			// ���C�����K�[�h(�R��)
	PC_CLASS_LG2_H,			// �]�����C�����K�[�h(�R��)
	PC_CLASS_RA2,			// �����W���[(�R��)
	PC_CLASS_RA2_H,			// �]�������W���[(�R��)
	PC_CLASS_NC2,			// ���J�j�b�N(�R��)
	PC_CLASS_NC2_H,			// �]�����J�j�b�N(�R��)
	PC_CLASS_RK3,			// ���[���i�C�g(�R��)
	PC_CLASS_RK3_H,			// �]�����[���i�C�g(�R��)
	PC_CLASS_RK4,			// ���[���i�C�g(�R��)
	PC_CLASS_RK4_H,			// �]�����[���i�C�g(�R��)
	PC_CLASS_RK5,			// ���[���i�C�g(�R��)
	PC_CLASS_RK5_H,			// �]�����[���i�C�g(�R��)
	PC_CLASS_RK6,			// ���[���i�C�g(�R��)
	PC_CLASS_RK6_H,			// �]�����[���i�C�g(�R��)
	PC_CLASS_RK_B,			// �{�q���[���i�C�g
	PC_CLASS_WL_B,			// �{�q�E�H�[���b�N
	PC_CLASS_RA_B,			// �{�q�����W���[
	PC_CLASS_AB_B,			// �{�q�A�[�N�r�V���b�v
	PC_CLASS_NC_B,			// �{�q���J�j�b�N
	PC_CLASS_GC_B,			// �{�q�M���`���N���X
	PC_CLASS_LG_B,			// �{�q���C�����K�[�h
	PC_CLASS_SO_B,			// �{�q�\�[�T���[
	PC_CLASS_MI_B,			// �{�q�~���X�g����
	PC_CLASS_WA_B,			// �{�q�����_���[
	PC_CLASS_SR_B,			// �{�q�C��
	PC_CLASS_GN_B,			// �{�q�W�F�l�e�B�b�N
	PC_CLASS_SC_B,			// �{�q�V���h�E�`�F�C�T�[
	PC_CLASS_RK2_B,			// �{�q���[���i�C�g(�R��)
	PC_CLASS_LG2_B,			// �{�q���C�����K�[�h(�R��)
	PC_CLASS_RA2_B,			// �{�q�����W���[(�R��)
	PC_CLASS_NC2_B,			// �{�q���J�j�b�N(�R��)
	PC_CLASS_MAX
};

// �E�ƒ�`
enum {
	PC_JOB_NV = 0,	// �m�[�r�X
	PC_JOB_SM,		// �\�[�h�}��
	PC_JOB_MG,		// �}�W�V����
	PC_JOB_AC,		// �A�[�`���[
	PC_JOB_AL,		// �A�R���C�g
	PC_JOB_MC,		// �}�[�`�����g
	PC_JOB_TF,		// �V�[�t
	PC_JOB_KN,		// �i�C�g
	PC_JOB_PR,		// �v���[�X�g
	PC_JOB_WZ,		// �E�B�U�[�h
	PC_JOB_BS,		// �u���b�N�X�~�X
	PC_JOB_HT,		// �n���^�[
	PC_JOB_AS,		// �A�T�V��
	PC_JOB_CR,		// �N���Z�C�_�[
	PC_JOB_MO,		// �����N
	PC_JOB_SA,		// �Z�[�W
	PC_JOB_RG,		// ���[�O
	PC_JOB_AM,		// �A���P�~�X�g
	PC_JOB_BA,		// �o�[�h
	PC_JOB_DC,		// �_���T�[
	PC_JOB_SNV,		// �X�[�p�[�m�[�r�X
	PC_JOB_TK,		// �e�R���L�b�h
	PC_JOB_SG,		// ����
	PC_JOB_SL,		// �\�E�������J�[
	PC_JOB_GS,		// �K���X�����K�[
	PC_JOB_NJ,		// �E��
	PC_JOB_MB,		// �L�����V�[
	PC_JOB_DK,		// �f�X�i�C�g
	PC_JOB_DA,		// �_�[�N�R���N�^�[
	PC_JOB_RK,		// ���[���i�C�g
	PC_JOB_WL,		// �E�H�[���b�N
	PC_JOB_RA,		// �����W���[
	PC_JOB_AB,		// �A�[�N�r�V���b�v
	PC_JOB_NC,		// ���J�j�b�N
	PC_JOB_GC,		// �M���`���N���X
	PC_JOB_LG,		// ���C�����K�[�h
	PC_JOB_SO,		// �\�[�T���[
	PC_JOB_MI,		// �~���X�g����
	PC_JOB_WA,		// �����_���[
	PC_JOB_SR,		// �C��
	PC_JOB_GN,		// �W�F�l�e�B�b�N
	PC_JOB_SC,		// �V���h�E�`�F�C�T�[
	PC_JOB_MAX
};

enum {
	PC_UPPER_NORMAL = 0,	// ���]��
	PC_UPPER_HIGH,			// �]��
	PC_UPPER_BABY,			// �{�q
	PC_UPPER_MAX
};

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
	struct skill skill[MAX_PCSKILL];
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
