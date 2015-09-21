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

#ifndef WINDOWS
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <sys/time.h>
	#include <sys/ioctl.h>
	#include <unistd.h>
	#include <signal.h>
	#include <fcntl.h>
	#include <arpa/inet.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "malloc.h"
#include "nullpo.h"
#include "socket.h"
#include "timer.h"
#include "utils.h"

#include "map.h"
#include "battle.h"
#include "chrif.h"
#include "clif.h"
#include "pc.h"
#include "intif.h"
#include "storage.h"
#include "party.h"
#include "guild.h"
#include "pet.h"
#include "homun.h"
#include "msg.h"
#include "status.h"
#include "mail.h"
#include "npc.h"
#include "merc.h"
#include "elem.h"

static const int packet_len_table[]={
	-1,-1,27, 0, -1, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,	// 3800-
	-1, 7, 0, 0,  0, 0, 0, 0, -1,11,15, 7,  6, 0,  0, 0,	// 3810-
	35,-1,39,13, 38,33, 7,-1, 14, 0, 0, 0,  0, 0,  0, 0,	// 3820-
	10,-1,15, 0, 79,19, 7,-1,  0,-1,-1,-1, 15,67,186,-1,	// 3830-
	 9, 9,-1,-1,  0, 0, 0, 0,  7,-1,-1,-1, 11,-1, -1, 0,	// 3840-
	 0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,	// 3850-
	-1, 7, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,	// 3860-
	-1, 7, 3, 0,  0, 0, 0, 0, -1, 7, 0, 0, -1, 7,  3, 0,	// 3870-
	11,-1, 7, 3,  0, 0, 0, 0, -1, 7, 3, 0,  0, 0,  0, 0,	// 3880-
	31,51,51,-1,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0,  0, 0,	// 3890-
};

extern int char_fd;		// inter server��fd��char_fd���g��
#define inter_fd (char_fd)	// �G�C���A�X

//-----------------------------------------------------------------
// inter server�ւ̑��M

// �y�b�g
void intif_create_pet(int account_id,int char_id,short pet_class,short pet_lv,short pet_egg_id,
	short pet_equip,short intimate,short hungry,char rename_flag,char incubate,const char *pet_name)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd, 0) = 0x3080;
	WFIFOL(inter_fd, 2) = account_id;
	WFIFOL(inter_fd, 6) = char_id;
	WFIFOW(inter_fd,10) = pet_class;
	WFIFOW(inter_fd,12) = pet_lv;
	WFIFOW(inter_fd,14) = pet_egg_id;
	WFIFOW(inter_fd,16) = pet_equip;
	WFIFOW(inter_fd,18) = intimate;
	WFIFOW(inter_fd,20) = hungry;
	WFIFOB(inter_fd,22) = rename_flag;
	WFIFOB(inter_fd,23) = incubate;
	memcpy(WFIFOP(inter_fd,24),pet_name,24);
	WFIFOSET(inter_fd,48);

	return;
}

void intif_request_petdata(int account_id, int char_id, int pet_id)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd, 0) = 0x3081;
	WFIFOL(inter_fd, 2) = account_id;
	WFIFOL(inter_fd, 6) = char_id;
	WFIFOL(inter_fd,10) = pet_id;
	WFIFOSET(inter_fd,14);

	return;
}

void intif_save_petdata(int account_id, struct s_pet *p)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0) = 0x3082;
	WFIFOW(inter_fd,2) = sizeof(struct s_pet) + 8;
	WFIFOL(inter_fd,4) = account_id;
	memcpy(WFIFOP(inter_fd,8),p,sizeof(struct s_pet));
	WFIFOSET(inter_fd,WFIFOW(inter_fd,2));

	return;
}

void intif_delete_petdata(int pet_id)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0) = 0x3083;
	WFIFOL(inter_fd,2) = pet_id;
	WFIFOSET(inter_fd,6);

	return;
}

// �z��
void intif_create_hom(int account_id, int char_id, struct mmo_homunstatus *h)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0) = 0x3088;
	WFIFOW(inter_fd,2) = sizeof(struct mmo_homunstatus) + 12;
	WFIFOL(inter_fd,4) = account_id;
	WFIFOL(inter_fd,8) = char_id;
	memcpy(WFIFOP(inter_fd,12),h,sizeof(struct mmo_homunstatus));
	WFIFOSET(inter_fd,WFIFOW(inter_fd,2));

	return;
}

void intif_request_homdata(int account_id, int char_id, int homun_id)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd, 0) = 0x3089;
	WFIFOL(inter_fd, 2) = account_id;
	WFIFOL(inter_fd, 6) = char_id;
	WFIFOL(inter_fd,10) = homun_id;
	WFIFOSET(inter_fd,14);

	return;
}

void intif_save_homdata(int account_id, struct mmo_homunstatus *h)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0) = 0x308a;
	WFIFOW(inter_fd,2) = sizeof(struct mmo_homunstatus) + 8;
	WFIFOL(inter_fd,4) = account_id;
	memcpy(WFIFOP(inter_fd,8),h,sizeof(struct mmo_homunstatus));
	WFIFOSET(inter_fd,WFIFOW(inter_fd,2));

	return;
}

void intif_delete_homdata(int account_id, int char_id, int homun_id)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd, 0) = 0x308b;
	WFIFOL(inter_fd, 2) = account_id;
	WFIFOL(inter_fd, 6) = char_id;
	WFIFOL(inter_fd,10) = homun_id;
	WFIFOSET(inter_fd,14);

	return;
}

// �b��
void intif_create_merc(int account_id, int char_id, struct mmo_mercstatus *m)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0) = 0x3070;
	WFIFOW(inter_fd,2) = sizeof(struct mmo_mercstatus) + 12;
	WFIFOL(inter_fd,4) = account_id;
	WFIFOL(inter_fd,8) = char_id;
	memcpy(WFIFOP(inter_fd,12),m,sizeof(struct mmo_mercstatus));
	WFIFOSET(inter_fd,WFIFOW(inter_fd,2));

	return;
}

void intif_request_mercdata(int account_id, int char_id, int merc_id)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd, 0) = 0x3071;
	WFIFOL(inter_fd, 2) = account_id;
	WFIFOL(inter_fd, 6) = char_id;
	WFIFOL(inter_fd,10) = merc_id;
	WFIFOSET(inter_fd,14);

	return;
}

void intif_save_mercdata(int account_id, struct mmo_mercstatus *m)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0) = 0x3072;
	WFIFOW(inter_fd,2) = sizeof(struct mmo_mercstatus) + 8;
	WFIFOL(inter_fd,4) = account_id;
	memcpy(WFIFOP(inter_fd,8),m,sizeof(struct mmo_mercstatus));
	WFIFOSET(inter_fd,WFIFOW(inter_fd,2));

	return;
}

void intif_delete_mercdata(int account_id, int char_id, int merc_id)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd, 0) = 0x3073;
	WFIFOL(inter_fd, 2) = account_id;
	WFIFOL(inter_fd, 6) = char_id;
	WFIFOL(inter_fd,10) = merc_id;
	WFIFOSET(inter_fd,14);

	return;
}

// ����
void intif_create_elem(int account_id, int char_id, struct mmo_elemstatus *m)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0) = 0x307c;
	WFIFOW(inter_fd,2) = sizeof(struct mmo_elemstatus) + 12;
	WFIFOL(inter_fd,4) = account_id;
	WFIFOL(inter_fd,8) = char_id;
	memcpy(WFIFOP(inter_fd,12),m,sizeof(struct mmo_elemstatus));
	WFIFOSET(inter_fd,WFIFOW(inter_fd,2));

	return;
}

void intif_request_elemdata(int account_id, int char_id, int elem_id)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd, 0) = 0x307d;
	WFIFOL(inter_fd, 2) = account_id;
	WFIFOL(inter_fd, 6) = char_id;
	WFIFOL(inter_fd,10) = elem_id;
	WFIFOSET(inter_fd,14);

	return;
}

void intif_save_elemdata(int account_id, struct mmo_elemstatus *m)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0) = 0x307e;
	WFIFOW(inter_fd,2) = sizeof(struct mmo_elemstatus) + 8;
	WFIFOL(inter_fd,4) = account_id;
	memcpy(WFIFOP(inter_fd,8),m,sizeof(struct mmo_elemstatus));
	WFIFOSET(inter_fd,WFIFOW(inter_fd,2));

	return;
}

void intif_delete_elemdata(int account_id, int char_id, int elem_id)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd, 0) = 0x307f;
	WFIFOL(inter_fd, 2) = account_id;
	WFIFOL(inter_fd, 6) = char_id;
	WFIFOL(inter_fd,10) = elem_id;
	WFIFOSET(inter_fd,14);

	return;
}

// GM���b�Z�[�W�𑗐M
void intif_GMmessage(const char* mes, size_t len, int flag)
{
	int lp = (flag&0x30)? 4: 0;

	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0) = 0x3000;
	WFIFOW(inter_fd,2) = (unsigned short)(lp + len + 16);
	WFIFOL(inter_fd,4) = 0xFF000000;	// non color�p�_�~�[�R�[�h
	WFIFOW(inter_fd,8) = 0;
	WFIFOW(inter_fd,10) = 0;
	WFIFOW(inter_fd,12) = 0;
	WFIFOW(inter_fd,14) = 0;
	if(flag&0x80)
		memcpy(WFIFOP(inter_fd,16), "micc", 4);
	else if(flag&0x40)
		memcpy(WFIFOP(inter_fd,16), "tool", 4);
	else if(flag&0x20)
		memcpy(WFIFOP(inter_fd,16), "ssss", 4);
	else if(flag&0x10)
		memcpy(WFIFOP(inter_fd,16), "blue", 4);
	memcpy(WFIFOP(inter_fd,16+lp), mes, len);
	WFIFOSET(inter_fd, WFIFOW(inter_fd,2));

	return;
}

// GM���b�Z�[�W�i�}���`�J���[�j�𑗐M
int intif_announce(const char* mes,size_t len,unsigned int color,int type,int size,int align,int pos_y)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3000;
	WFIFOW(inter_fd,2) = (unsigned short)(16+len);
	WFIFOL(inter_fd,4) = color;
	WFIFOW(inter_fd,8) = type;
	WFIFOW(inter_fd,10) = size;
	WFIFOW(inter_fd,12) = align;
	WFIFOW(inter_fd,14) = pos_y;
	memcpy(WFIFOP(inter_fd,16), mes, len);
	WFIFOSET(inter_fd, WFIFOW(inter_fd,2));

	return 0;
}

// Wis�̑��M
void intif_wis_message(struct map_session_data *sd, const char *nick, const char *mes, size_t mes_len)
{
	nullpo_retv(sd);

	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0) = 0x3001;
	WFIFOW(inter_fd,2) = (unsigned short)(mes_len+56);
	WFIFOL(inter_fd,4) = pc_isGM(sd);
	memcpy(WFIFOP(inter_fd,8),sd->status.name,24);
	memcpy(WFIFOP(inter_fd,32),nick,24);
	memcpy(WFIFOP(inter_fd,56),mes,mes_len);
	WFIFOSET(inter_fd, WFIFOW(inter_fd,2) );

	return;
}

// Wis�̕Ԏ�
static int intif_wis_replay(int id,int flag)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3002;
	WFIFOL(inter_fd,2) = id;
	WFIFOB(inter_fd,6) = flag;
	WFIFOSET(inter_fd,7);

	return 0;
}

// �A�J�E���g�ϐ����M
int intif_saveaccountreg(struct map_session_data *sd)
{
	int j,p;

	nullpo_retr(0, sd);

	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3004;
	WFIFOL(inter_fd,4) = sd->bl.id;
	for(j=0,p=8;j<sd->save_reg.account_num;j++,p+=36){
		memcpy(WFIFOP(inter_fd,p),sd->save_reg.account[j].str,32);
		WFIFOL(inter_fd,p+32)=sd->save_reg.account[j].value;
	}
	WFIFOW(inter_fd,2)=p;
	WFIFOSET(inter_fd,p);

	return 0;
}

// �A�J�E���g�ϐ��v��
int intif_request_accountreg(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3005;
	WFIFOL(inter_fd,2) = sd->bl.id;
	WFIFOSET(inter_fd,6);

	return 0;
}

// �q�Ƀf�[�^�v��
int intif_request_storage(int account_id)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3010;
	WFIFOL(inter_fd,2) = account_id;
	WFIFOSET(inter_fd,6);

	return 0;
}

// �q�Ƀf�[�^���M
int intif_send_storage(struct storage *stor)
{
	nullpo_retr(0, stor);

	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3011;
	WFIFOW(inter_fd,2) = sizeof(struct storage)+8;
	WFIFOL(inter_fd,4) = stor->account_id;
	memcpy( WFIFOP(inter_fd,8),stor, sizeof(struct storage) );
	WFIFOSET(inter_fd,WFIFOW(inter_fd,2));

	return 0;
}

// �M���h�q�Ƀf�[�^�v��
int intif_request_guild_storage(int account_id,int guild_id)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3018;
	WFIFOL(inter_fd,2) = account_id;
	WFIFOL(inter_fd,6) = guild_id;
	WFIFOSET(inter_fd,10);

	return 0;
}

// �M���h�q�Ƀf�[�^���M
int intif_send_guild_storage(int account_id,struct guild_storage *gstor)
{
	if (inter_fd < 0)
		return -1;

	WFIFORESERVE( inter_fd, sizeof(struct guild_storage)+12 );
	WFIFOW(inter_fd,0) = 0x3019;
	WFIFOW(inter_fd,2) = sizeof(struct guild_storage)+12;
	WFIFOL(inter_fd,4) = account_id;
	WFIFOL(inter_fd,8) = gstor->guild_id;
	memcpy( WFIFOP(inter_fd,12),gstor, sizeof(struct guild_storage) );
	WFIFOSET(inter_fd,WFIFOW(inter_fd,2));

	return 0;
}

// �M���h�q�Ƀ��b�N�v��
int intif_trylock_guild_storage(struct map_session_data *sd,int npc_id)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0)  = 0x301a;
	WFIFOL(inter_fd,2)  = sd->status.account_id;
	WFIFOL(inter_fd,6)  = sd->status.guild_id;
	WFIFOL(inter_fd,10) = npc_id;
	WFIFOSET(inter_fd,14);

	return 0;
}

// �M���h�q�Ƀ��b�N����
int intif_unlock_guild_storage(int guild_id)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x301b;
	WFIFOL(inter_fd,2) = guild_id;
	WFIFOSET(inter_fd,6);

	return 0;
}

// �M���h�q�Ƀf�b�h���b�N����
int intif_deadlock_guild_storage(int guild_id)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x301c;
	WFIFOL(inter_fd,2) = guild_id;
	WFIFOSET(inter_fd,6);

	return 0;
}

// �p�[�e�B�쐬�v��
void intif_create_party(struct map_session_data *sd, const char *name, int item, int item2)
{
	nullpo_retv(sd);

	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0) = 0x3020;
	WFIFOL(inter_fd,2) = sd->status.account_id;
	WFIFOL(inter_fd,6) = sd->status.char_id;
	memcpy(WFIFOP(inter_fd, 10),name,24);
	WFIFOB(inter_fd,34) = item;
	WFIFOB(inter_fd,35) = item2;
	memcpy(WFIFOP(inter_fd,36),sd->status.name,24);
	memcpy(WFIFOP(inter_fd,60),map[sd->bl.m].name,16);
	WFIFOW(inter_fd,76)= sd->status.base_level;
	WFIFOSET(inter_fd,78);

	return;
}

// �p�[�e�B���v��
int intif_request_partyinfo(int party_id)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3021;
	WFIFOL(inter_fd,2) = party_id;
	WFIFOSET(inter_fd,6);

	return 0;
}

// �p�[�e�B�ǉ��v��
void intif_party_addmember(struct map_session_data *sd)
{
	nullpo_retv(sd);

	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd, 0) = 0x3022;
	WFIFOL(inter_fd, 2) = sd->party_invite;
	WFIFOL(inter_fd, 6) = sd->status.account_id;
	WFIFOL(inter_fd,10) = sd->status.char_id;
	memcpy(WFIFOP(inter_fd,14),sd->status.name,24);
	memcpy(WFIFOP(inter_fd,38),map[sd->bl.m].name,16);
	WFIFOW(inter_fd,54) = sd->status.base_level;
	WFIFOSET(inter_fd,56);

	return;
}

// �p�[�e�B�ݒ�ύX
void intif_party_changeoption(int party_id, int account_id, int baby_id, int exp, int item)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd, 0)=0x3023;
	WFIFOL(inter_fd, 2)=party_id;
	WFIFOL(inter_fd, 6)=account_id;
	WFIFOL(inter_fd,10)=baby_id;
	WFIFOB(inter_fd,14)=exp;
	WFIFOB(inter_fd,15)=item;
	WFIFOSET(inter_fd,16);

	return;
}

// �p�[�e�B�E�ޗv��
void intif_party_leave(int party_id, int account_id, int char_id)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0)=0x3024;
	WFIFOL(inter_fd,2)=party_id;
	WFIFOL(inter_fd,6)=account_id;
	WFIFOL(inter_fd,10)=char_id;
	WFIFOSET(inter_fd,14);

	return;
}

// �p�[�e�B�ړ��v��
void intif_party_changemap(struct map_session_data *sd, unsigned char online)
{
	nullpo_retv(sd);

	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0)=0x3025;
	WFIFOL(inter_fd,2)=sd->status.party_id;
	WFIFOL(inter_fd,6)=sd->status.account_id;
	WFIFOL(inter_fd,10)=sd->status.char_id;
	memcpy(WFIFOP(inter_fd,14),map[sd->bl.m].name,16);
	WFIFOB(inter_fd,30)=online;
	WFIFOW(inter_fd,31)=sd->status.base_level;
	WFIFOSET(inter_fd,33);

	return;
}

// �p�[�e�B�[���U�v��
int intif_break_party(int party_id)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0)=0x3026;
	WFIFOL(inter_fd,2)=party_id;
	WFIFOSET(inter_fd,6);

	return 0;
}

// �p�[�e�B��b���M
int intif_party_message(int party_id,int account_id,const char *mes,size_t len)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0)=0x3027;
	WFIFOW(inter_fd,2)=(unsigned short)(len+12);
	WFIFOL(inter_fd,4)=party_id;
	WFIFOL(inter_fd,8)=account_id;
	memcpy(WFIFOP(inter_fd,12),mes,len);
	WFIFOSET(inter_fd,len+12);

	return 0;
}

// �p�[�e�B�����`�F�b�N�v��
int intif_party_checkconflict(int party_id,int account_id,int char_id)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0)=0x3028;
	WFIFOL(inter_fd,2)=party_id;
	WFIFOL(inter_fd,6)=account_id;
	WFIFOL(inter_fd,10)=char_id;
	WFIFOSET(inter_fd,14);

	return 0;
}

//�p�[�e�B�[���[�_�[�ύX�v��
int intif_party_leaderchange(int party_id,int account_id,int char_id)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0)=0x3029;
	WFIFOL(inter_fd,2)=party_id;
	WFIFOL(inter_fd,6)=account_id;
	WFIFOL(inter_fd,10)=char_id;
	WFIFOSET(inter_fd,14);

	return 0;
}

// �M���h�쐬�v��
void intif_guild_create(const char *name, const struct guild_member *master)
{
	nullpo_retv(master);

	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0)=0x3030;
	WFIFOW(inter_fd,2)=sizeof(struct guild_member)+32;
	WFIFOL(inter_fd,4)=master->account_id;
	memcpy(WFIFOP(inter_fd,8),name,24);
	memcpy(WFIFOP(inter_fd,32),master,sizeof(struct guild_member));
	WFIFOSET(inter_fd,WFIFOW(inter_fd,2));

	return;
}

// �M���h���v��
void intif_guild_request_info(int guild_id)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0) = 0x3031;
	WFIFOL(inter_fd,2) = guild_id;
	WFIFOSET(inter_fd,6);

	return;
}

// �M���h�����o�ǉ��v��
int intif_guild_addmember(int guild_id,struct guild_member *m)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3032;
	WFIFOW(inter_fd,2) = sizeof(struct guild_member)+8;
	WFIFOL(inter_fd,4) = guild_id;
	memcpy(WFIFOP(inter_fd,8),m,sizeof(struct guild_member));
	WFIFOSET(inter_fd,WFIFOW(inter_fd,2));

	return 0;
}

// �M���h�����o�E��/�Ǖ��v��
int intif_guild_leave(int guild_id,int account_id,int char_id,int flag,const char *mes)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd, 0) = 0x3034;
	WFIFOL(inter_fd, 2) = guild_id;
	WFIFOL(inter_fd, 6) = account_id;
	WFIFOL(inter_fd,10) = char_id;
	WFIFOB(inter_fd,14) = flag;
	strncpy(WFIFOP(inter_fd,15),mes,40);
	WFIFOSET(inter_fd,55);

	return 0;
}

// �M���h�����o�̃I�����C����/Lv�X�V�v��
void intif_guild_memberinfoshort(int guild_id,int account_id, int char_id, unsigned char online, int lv, int class_)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd, 0) = 0x3035;
	WFIFOL(inter_fd, 2) = guild_id;
	WFIFOL(inter_fd, 6) = account_id;
	WFIFOL(inter_fd,10) = char_id;
	WFIFOB(inter_fd,14) = online;
	WFIFOW(inter_fd,15) = lv;
	WFIFOW(inter_fd,17) = class_;
	WFIFOSET(inter_fd,19);

	return;
}

// �M���h���U�ʒm
int intif_guild_break(int guild_id)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd, 0) = 0x3036;
	WFIFOL(inter_fd, 2) = guild_id;
	WFIFOSET(inter_fd,6);

	return 0;
}

// �M���h��b���M
int intif_guild_message(int guild_id,int account_id,const char *mes,size_t len)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0)=0x3037;
	WFIFOW(inter_fd,2)=(unsigned short)(len+12);
	WFIFOL(inter_fd,4)=guild_id;
	WFIFOL(inter_fd,8)=account_id;
	memcpy(WFIFOP(inter_fd,12),mes,len);
	WFIFOSET(inter_fd,len+12);

	return 0;
}

// �M���h�����`�F�b�N�v��
int intif_guild_checkconflict(int guild_id,int account_id,int char_id)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd, 0)=0x3038;
	WFIFOL(inter_fd, 2)=guild_id;
	WFIFOL(inter_fd, 6)=account_id;
	WFIFOL(inter_fd,10)=char_id;
	WFIFOSET(inter_fd,14);

	return 0;
}

// �M���h��{���ύX�v��
int intif_guild_change_basicinfo(int guild_id,int type,const void *data,int len)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0)=0x3039;
	WFIFOW(inter_fd,2)=len+10;
	WFIFOL(inter_fd,4)=guild_id;
	WFIFOW(inter_fd,8)=type;
	memcpy(WFIFOP(inter_fd,10),data,len);
	WFIFOSET(inter_fd,len+10);

	return 0;
}

// �M���h�����o���ύX�v��
int intif_guild_change_memberinfo(int guild_id,int account_id,int char_id,int type,const void *data,int len)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd, 0)=0x303a;
	WFIFOW(inter_fd, 2)=len+18;
	WFIFOL(inter_fd, 4)=guild_id;
	WFIFOL(inter_fd, 8)=account_id;
	WFIFOL(inter_fd,12)=char_id;
	WFIFOW(inter_fd,16)=type;
	memcpy(WFIFOP(inter_fd,18),data,len);
	WFIFOSET(inter_fd,len+18);

	return 0;
}

// �M���h��E�ύX�v��
void intif_guild_position(int guild_id, int idx, struct guild_position *p)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0)=0x303b;
	WFIFOW(inter_fd,2)=sizeof(struct guild_position)+12;
	WFIFOL(inter_fd,4)=guild_id;
	WFIFOL(inter_fd,8)=idx;
	memcpy(WFIFOP(inter_fd,12),p,sizeof(struct guild_position));
	WFIFOSET(inter_fd,WFIFOW(inter_fd,2));

	return;
}

// �M���h�X�L���A�b�v�v��
int intif_guild_skillup(int guild_id,int skill_num,int account_id,int level,unsigned char flag)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd, 0)=0x303c;
	WFIFOL(inter_fd, 2)=guild_id;
	WFIFOL(inter_fd, 6)=skill_num;
	WFIFOL(inter_fd,10)=account_id;
	WFIFOL(inter_fd,14)=level;
	WFIFOB(inter_fd,18)=flag;
	WFIFOSET(inter_fd,19);

	return 0;
}

// �M���h����/�G�Ηv��
int intif_guild_alliance(int guild_id1,int guild_id2,int account_id1,int account_id2,int flag)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd, 0)=0x303d;
	WFIFOL(inter_fd, 2)=guild_id1;
	WFIFOL(inter_fd, 6)=guild_id2;
	WFIFOL(inter_fd,10)=account_id1;
	WFIFOL(inter_fd,14)=account_id2;
	WFIFOL(inter_fd,18)=flag;
	WFIFOSET(inter_fd,22);

	return 0;
}

// �M���h���m�ύX�v��
void intif_guild_notice(int guild_id, const char *mes1, const char *mes2)
{
	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0)=0x303e;
	WFIFOL(inter_fd,2)=guild_id;
	memcpy(WFIFOP(inter_fd,6),mes1,60);
	memcpy(WFIFOP(inter_fd,66),mes2,120);
	WFIFOSET(inter_fd,186);

	return;
}

// �M���h�G���u�����ύX�v��
void intif_guild_emblem(int guild_id, unsigned short len, const char *data)
{
	if (guild_id <= 0 || len > 2000) // len always >=0, it's unsigned
		return;

	if (inter_fd < 0)
		return;

	WFIFOW(inter_fd,0)=0x303f;
	WFIFOW(inter_fd,2)=len+12;
	WFIFOL(inter_fd,4)=guild_id;
	WFIFOL(inter_fd,8)=0;
	memcpy(WFIFOP(inter_fd,12),data,len);
	WFIFOSET(inter_fd,len+12);

	return;
}

// ���݂̃M���h���̃M���h�𒲂ׂ�
int intif_guild_castle_dataload(int castle_id, int idx)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0)=0x3040;
	WFIFOW(inter_fd,2)=castle_id;
	WFIFOB(inter_fd,4)=idx;
	WFIFOSET(inter_fd,5);

	return 0;
}

// �M���h���̃M���h�ύX�v��
int intif_guild_castle_datasave(int castle_id, int idx, int value)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0)=0x3041;
	WFIFOW(inter_fd,2)=castle_id;
	WFIFOB(inter_fd,4)=idx;
	WFIFOL(inter_fd,5)=value;
	WFIFOSET(inter_fd,9);

	return 0;
}

/*==========================================
 * �w�肵�����O�̃L�����̏ꏊ�v��
 *------------------------------------------
 */
static int intif_charposreq(int account_id,const char *name,int flag)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0)=0x3090;
	WFIFOL(inter_fd,2)=account_id;
	memcpy(WFIFOP(inter_fd,6),name,24);
	WFIFOB(inter_fd,30)=flag;
	WFIFOSET(inter_fd,31);

	return 0;
}

/*==========================================
 * �w�肵�����O�̃L�����̏ꏊ�Ɉړ�����
 * @jumpto
 *------------------------------------------
 */
int intif_jumpto(int account_id,const char *name)
{
	intif_charposreq(account_id,name,1);

	return 0;
}

/*==========================================
 * �w�肵�����O�̃L�����̏ꏊ�\������
 * @where
 *------------------------------------------
 */
int intif_where(int account_id,const char *name)
{
	intif_charposreq(account_id,name,0);

	return 0;
}

/*==========================================
 * �w�肵�����O�̃L�������Ăъ񂹂�
 * flag=0 ���Ȃ��Ɉ�������
 * flag=1 @recall
 *------------------------------------------
 */
int intif_charmovereq(struct map_session_data *sd,const char *name,int flag)
{
	nullpo_retr(0,sd);

	if(name==NULL)
		return -1;

	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0)=0x3092;
	WFIFOL(inter_fd,2)=sd->status.account_id;
	memcpy(WFIFOP(inter_fd,6),name,24);
	WFIFOB(inter_fd,30)=flag;
	memcpy(WFIFOP(inter_fd,31),sd->mapname,16);
	WFIFOW(inter_fd,47)=sd->bl.x;
	WFIFOW(inter_fd,49)=sd->bl.y;
	WFIFOSET(inter_fd,51);

	return 0;
}

/*==========================================
 * �w�肵�����O�̃L�������w�肵���ꏊ�ɌĂъ񂹂�
 * flag=0 ���Ȃ��Ɉ�������
 * flag=1 @recall
 * flag=2 �{�q�n�Ăяo���X�L��
 *------------------------------------------
 */
int intif_charmovereq2(struct map_session_data *sd,const char *name,const char *mapname,short x, short y,int flag)
{
	nullpo_retr(0,sd);

	if (name == NULL)
		return -1;

	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0)=0x3092;
	WFIFOL(inter_fd,2)=sd->status.account_id;
	memcpy(WFIFOP(inter_fd,6),name,24);
	WFIFOB(inter_fd,30)=flag;
	memcpy(WFIFOP(inter_fd,31),mapname,16);
	WFIFOW(inter_fd,47)=x;
	WFIFOW(inter_fd,49)=y;
	WFIFOSET(inter_fd,51);

	return 0;
}

/*==========================================
 * �Ώ�ID�Ƀ��b�Z�[�W�𑗐M
 *------------------------------------------
 */
int intif_displaymessage(int account_id, const char* mes)
{
	size_t len;

	if (inter_fd < 0)
		return -1;

	len = strlen(mes)+1;
	WFIFOW(inter_fd,0) = 0x3093;
	WFIFOW(inter_fd,2) = (unsigned short)(len + 8);
	WFIFOL(inter_fd,4) = account_id;
	strncpy(WFIFOP(inter_fd,8), mes, len);
	WFIFOSET(inter_fd, WFIFOW(inter_fd,2));

	return 0;
}

/*==========================================
 * ���[��BOX�X�V�v��
 *------------------------------------------
 */
int intif_mailbox(int char_id)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3049;
	WFIFOL(inter_fd,2) = char_id;
	WFIFOSET(inter_fd,6);

	return 0;
}

/*==========================================
 * ���[���̑��M
 *------------------------------------------
 */
int intif_sendmail(struct mail_data *md)
{
	int size;

	if (inter_fd < 0)
		return -1;

	size = sizeof(struct mail_data);
	WFIFOW(inter_fd,0) = 0x304a;
	WFIFOW(inter_fd,2) = 4+size;
	memcpy(WFIFOP(inter_fd,4), md, size);
	WFIFOSET(inter_fd, 4+size);

	return 0;
}

/*==========================================
 * ���[���̍폜�v��
 *------------------------------------------
 */
int intif_deletemail(int char_id,int mail_num)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x304b;
	WFIFOL(inter_fd,2) = char_id;
	WFIFOL(inter_fd,6) = mail_num;
	WFIFOSET(inter_fd, 10);

	return 0;
}

/*==========================================
 * ���[���̑I����M
 *------------------------------------------
 */
int intif_readmail(int char_id,int mail_num)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x304c;
	WFIFOL(inter_fd,2) = char_id;
	WFIFOL(inter_fd,6) = mail_num;
	WFIFOSET(inter_fd, 10);

	return 0;
}

/*==========================================
 * ���[���ɓY�t���ꂽ���̎�M
 *------------------------------------------
 */
int intif_mail_getappend(int char_id,int mail_num)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x304d;
	WFIFOL(inter_fd,2) = char_id;
	WFIFOL(inter_fd,6) = mail_num;
	WFIFOSET(inter_fd, 10);

	return 0;
}

/*==========================================
 * ���[�����󂯎��l�����邩�m�F�v��
 *------------------------------------------
 */
int intif_mail_checkmail(int account_id,struct mail_data *md)
{
	int size;

	if (inter_fd < 0)
		return -1;

	size = sizeof(struct mail_data);
	WFIFOW(inter_fd,0) = 0x304e;
	WFIFOW(inter_fd,2) = 8+size;
	WFIFOL(inter_fd,4) = account_id;
	memcpy(WFIFOP(inter_fd,8), md, size);
	WFIFOSET(inter_fd, 8+size);

	return 0;
}

/*==========================================
 * mail_num�̓Y�t�A�C�e���AZeny�̍폜
 *------------------------------------------
 */
int intif_mail_deleteappend(int char_id, int mail_num)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0)  = 0x304f;
	WFIFOL(inter_fd,2)  = char_id;
	WFIFOL(inter_fd,6)  = mail_num;
	WFIFOSET(inter_fd, 10);

	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�f�[�^�v��
 *------------------------------------------
 */
int intif_request_scdata(int account_id,int char_id)
{
#ifndef NO_SCDATA_SAVING
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3078;
	WFIFOL(inter_fd,2) = account_id;
	WFIFOL(inter_fd,6) = char_id;
	WFIFOSET(inter_fd, 10);
#endif

	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�f�[�^�ۑ�
 *------------------------------------------
 */
int intif_save_scdata(struct map_session_data *sd)
{
#ifndef NO_SCDATA_SAVING
	struct TimerData *td = NULL;
	int i,p;
	unsigned int tick = gettick();

	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3079;
	WFIFOL(inter_fd,4) = sd->status.account_id;
	WFIFOL(inter_fd,8) = sd->status.char_id;

	p=12;
	if(sd->sc.count > 0) {
		for(i=0; i<MAX_STATUSCHANGE; i++) {
			if(sd->sc.data[i].timer != -1 && status_can_save(i))
			{
				td = get_timer(sd->sc.data[i].timer);
				WFIFOW(inter_fd,p)    = i;
				WFIFOL(inter_fd,p+2)  = sd->sc.data[i].val1;
				WFIFOL(inter_fd,p+6)  = sd->sc.data[i].val2;
				WFIFOL(inter_fd,p+10) = sd->sc.data[i].val3;
				WFIFOL(inter_fd,p+14) = sd->sc.data[i].val4;
				WFIFOL(inter_fd,p+18) = DIFF_TICK(td->tick, tick);	// �c�莞��
				p+=22;
			}
		}
	}
	WFIFOW(inter_fd,2) = p;
	WFIFOSET(inter_fd,p);
#endif

	return 0;
}

/*==========================================
 * �N�G�X�g�f�[�^�v��
 *------------------------------------------
 */
int intif_request_quest(int account_id,int char_id)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3060;
	WFIFOL(inter_fd,2) = account_id;
	WFIFOL(inter_fd,6) = char_id;
	WFIFOSET(inter_fd, 10);

	return 0;
}

/*==========================================
 * �N�G�X�g�f�[�^�ۑ�
 *------------------------------------------
 */
int intif_save_quest(struct map_session_data *sd)
{
	int i,p;

	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3061;
	WFIFOL(inter_fd,4) = sd->status.account_id;
	WFIFOL(inter_fd,8) = sd->status.char_id;

	p=12;
	if(sd->questlist > 0) {
		for(i=0; i<sd->questlist; i++) {
			if(sd->quest[i].nameid > 0) {
				WFIFOL(inter_fd,p)    = sd->quest[i].nameid;
				WFIFOB(inter_fd,p+4)  = sd->quest[i].state;
				WFIFOL(inter_fd,p+5)  = (unsigned int)sd->quest[i].limit;
				WFIFOW(inter_fd,p+9)  = (short)sd->quest[i].mob[0].id;
				WFIFOW(inter_fd,p+11) = (short)sd->quest[i].mob[0].max;
				WFIFOW(inter_fd,p+13) = (short)sd->quest[i].mob[0].count;
				WFIFOW(inter_fd,p+15) = (short)sd->quest[i].mob[1].id;
				WFIFOW(inter_fd,p+17) = (short)sd->quest[i].mob[1].max;
				WFIFOW(inter_fd,p+19) = (short)sd->quest[i].mob[1].count;
				WFIFOW(inter_fd,p+21) = (short)sd->quest[i].mob[2].id;
				WFIFOW(inter_fd,p+23) = (short)sd->quest[i].mob[2].max;
				WFIFOW(inter_fd,p+25) = (short)sd->quest[i].mob[2].count;
				p+=27;
			}
		}
	}
	WFIFOW(inter_fd,2) = p;
	WFIFOSET(inter_fd,p);

	return 0;
}

/*==========================================
 * �L�����I�̐����l���̕ύX���M
 *------------------------------------------
 */
int intif_char_connect_limit(int limit)
{
	if (inter_fd < 0)
		return -1;

	WFIFOW(inter_fd,0) = 0x3008;
	WFIFOL(inter_fd,2) = limit;
	WFIFOSET(inter_fd, 6);

	return 0;
}

//-----------------------------------------------------------------
// inter server�����M

// wis��M
static int intif_parse_WisMessage(int fd)
{
	struct map_session_data* sd = map_nick2sd(RFIFOP(fd,36));
	int id = RFIFOL(fd,4);
	int gmlevel = RFIFOL(fd,8);
	int i, j = 0;

	if(sd) {
		for(i=0; i<MAX_WIS_REFUSAL; i++) {	//���ۃ��X�g�ɖ��O�����邩�ǂ������肵�Ă���΋���
			if(strcmp(sd->wis_refusal[i],RFIFOP(fd,12)) == 0) {
				j++;
				break;
			}
		}
		if(sd->state.wis_all) {
			intif_wis_replay(id,3);	// ��M����
		} else if(j > 0) {
			intif_wis_replay(id,2);	// ��M����
		} else {
			clif_wis_message(sd->fd,RFIFOP(fd,12),RFIFOP(fd,60),RFIFOW(fd,2)-60,gmlevel);
			intif_wis_replay(id,0);	// ���M����
		}
	} else {
		intif_wis_replay(id,1);	// ����Ȑl���܂���
	}
	return 0;
}

// wis���M���ʎ�M
static int intif_parse_WisEnd(int fd)
{
	struct map_session_data* sd = map_nick2sd(RFIFOP(fd,2));

	if(sd)
		clif_wis_end(sd->fd,RFIFOB(fd,26));
	return 0;
}

// �A�J�E���g�ϐ��ʒm
static int intif_parse_AccountReg(int fd)
{
	int j,p;
	struct map_session_data *sd = map_id2sd(RFIFOL(fd,4));

	if(sd == NULL)
		return 1;

	for(p=8,j=0; p<RFIFOW(fd,2) && j<ACCOUNT_REG_NUM; p+=36,j++) {
		memcpy(sd->save_reg.account[j].str,RFIFOP(fd,p),32);
		sd->save_reg.account[j].str[31] = '\0';	// force \0 terminal
		sd->save_reg.account[j].value   = RFIFOL(fd,p+32);
	}
	sd->save_reg.account_num = j;

	// �a�������i�[
	sd->deposit = pc_readaccountreg(sd,"#PC_DEPOSIT");

	return 0;
}

// �q�Ƀf�[�^��M
static int intif_parse_LoadStorage(int fd)
{
	if(RFIFOW(fd,2)-8 != sizeof(struct storage)) {
		if(battle_config.error_log)
			printf("intif_parse_LoadStorage: data size error %d %lu\n", RFIFOW(fd,2)-8, (unsigned long)sizeof(struct storage));
		return 1;
	}

	return storage_storageload(RFIFOL(fd,4), (struct storage *)RFIFOP(fd,8));
}

// �q�Ƀf�[�^���M����
static int intif_parse_SaveStorage(int fd)
{
	if(battle_config.save_log)
		printf("intif_savestorage: done %d %d\n",RFIFOL(fd,2),RFIFOB(fd,6) );
	return 0;
}

// �M���h�q�Ƀf�[�^��M
static int intif_parse_LoadGuildStorage(int fd)
{
	if(RFIFOW(fd,2)-12 != sizeof(struct guild_storage)) {
		if(battle_config.error_log)
			printf("intif_parse_LoadGuildStorage: data size error %d %lu\n", RFIFOW(fd,2)-12, (unsigned long)sizeof(struct guild_storage));
		return 1;
	}

	return storage_guild_storageload(RFIFOL(fd,4), RFIFOL(fd,8), (struct guild_storage *)RFIFOP(fd,12));
}

// �M���h�q�Ƀf�[�^���M����
static int intif_parse_SaveGuildStorage(int fd)
{
	if(battle_config.save_log) {
		printf("intif_save_guild_storage: done %d %d %d\n",RFIFOL(fd,2),RFIFOL(fd,6),RFIFOB(fd,10) );
	}
	return 0;
}

// �M���h�q�Ƀ��b�N�v���ԓ�
static int intif_parse_TrylockGuildStorageAck(int fd)
{
	struct map_session_data *sd = map_id2sd(RFIFOL(fd,2));
	int guild_id = RFIFOL(fd,6);
	int flag     = RFIFOB(fd,14);

	// ���ɑ��݂��ĂȂ����M���h���Ⴄ�Ȃ�
	if(sd == NULL || sd->status.guild_id != guild_id) {
		intif_unlock_guild_storage(guild_id);
		return 0;
	}

	if(sd->state.gstorage_lockreq == 1) {	// script
		sd->npc_menu = flag;
		npc_scriptcont(sd,RFIFOL(fd,10));
	}
	else if(sd->state.gstorage_lockreq == 2) {	// atcommand
		if(flag) {
			if(flag == 2) {
				// �L���b�V�����폜���ă����[�h
				storage_guild_delete(guild_id);
			}
			storage_guild_storageopen(sd);
		} else {
			clif_displaymessage(sd->fd, msg_txt(130));
		}
	}
	else {
		// �����O�C�������������ߑq�ɂ��J���K�v���Ȃ�
		intif_unlock_guild_storage(sd->status.guild_id);
	}
	sd->state.gstorage_lockreq = 0;

	return 0;
}

// �M���h�q�Ƀ��b�N�����ԓ�
static int intif_parse_UnlockGuildStorageAck(int fd)
{
	if(battle_config.save_log)
		printf("intif_unlock_guild_storage: done %d %d\n",RFIFOL(fd,2),RFIFOB(fd,6) );
	return 0;
}

// �M���h�q�Ƀf�b�h���b�N�`�F�b�N
static int intif_parse_ChecklockGuildStorage(int fd)
{
	storage_guild_checklock(RFIFOL(fd,2));

	return 0;
}

// �p�[�e�B�쐬��
static int intif_parse_PartyCreated(int fd)
{
	if(battle_config.etc_log)
		printf("intif: party created\n");
	party_created(RFIFOL(fd,2),RFIFOB(fd,6),RFIFOL(fd,7),RFIFOP(fd,11));
	return 0;
}

// �p�[�e�B���
static int intif_parse_PartyInfo(int fd)
{
	if( RFIFOW(fd,2)==8){
		if(battle_config.error_log)
			printf("intif: party noinfo %d\n",RFIFOL(fd,4));
		party_recv_noinfo(RFIFOL(fd,4));
		return 0;
	}

	if( RFIFOW(fd,2)!=sizeof(struct party)+4 ){
		if(battle_config.error_log)
			printf("intif: party info : data size error %d %d %lu\n",RFIFOL(fd,4),RFIFOW(fd,2),(unsigned long)(sizeof(struct party)+4));
	}
	party_recv_info((struct party *)RFIFOP(fd,4));
	return 0;
}

// �p�[�e�B�ǉ��ʒm
static void intif_parse_PartyMemberAdded(int fd)
{
	if(battle_config.etc_log)
		printf("intif: party member added %d %d %d %-24s\n",RFIFOL(fd,2),RFIFOL(fd,6),RFIFOB(fd,14),RFIFOP(fd,15));
	party_member_added(RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOB(fd,14));

	return;
}

// �p�[�e�B�ݒ�ύX�ʒm
static int intif_parse_PartyOptionChanged(int fd)
{
	party_optionchanged(RFIFOL(fd,2),RFIFOL(fd,6),RFIFOB(fd,10),RFIFOB(fd,11),RFIFOB(fd,12));
	return 0;
}

// �p�[�e�B�E�ޒʒm
static int intif_parse_PartyMemberLeaved(int fd)
{
	if(battle_config.etc_log)
		printf("intif: party member leaved %d %d %s\n",RFIFOL(fd,2),RFIFOL(fd,6),RFIFOP(fd,14));
	party_member_leaved(RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10));
	return 0;
}

// �p�[�e�B���U�ʒm
static int intif_parse_PartyBroken(int fd)
{
	party_broken(RFIFOL(fd,2));
	return 0;
}

// �p�[�e�B�ړ��ʒm
static void intif_parse_PartyMove(int fd)
{
	party_recv_movemap(RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOP(fd,14),RFIFOB(fd,30),RFIFOW(fd,31));

	return;
}

// �p�[�e�B���b�Z�[�W
static int intif_parse_PartyMessage(int fd)
{
	party_recv_message(RFIFOL(fd,4),RFIFOL(fd,8),RFIFOP(fd,12),RFIFOW(fd,2)-12);
	return 0;
}

// �p�[�e�B�[���[�_�[�ύX�ʒm
static int intif_parse_PartyLeaderChanged(int fd)
{
	party_leaderchanged(RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10));
	return 0;
}

// �M���h�쐬��
static int intif_parse_GuildCreated(int fd)
{
	guild_created(RFIFOL(fd,2),RFIFOL(fd,6));
	return 0;
}

// �M���h���
static int intif_parse_GuildInfo(int fd)
{
	if( RFIFOW(fd,2)==8){
		if(battle_config.error_log)
			printf("intif: guild noinfo %d\n",RFIFOL(fd,4));
		guild_recv_noinfo(RFIFOL(fd,4));
		return 0;
	}

	if( RFIFOW(fd,2)!=sizeof(struct guild)+4 ){
		if(battle_config.error_log)
			printf("intif: guild info : data size error %d %d %lu\n",RFIFOL(fd,4),RFIFOW(fd,2),(unsigned long)(sizeof(struct guild)+4));
	}
	guild_recv_info((struct guild *)RFIFOP(fd,4));
	return 0;
}

// �M���h�����o�ǉ��ʒm
static int intif_parse_GuildMemberAdded(int fd)
{
	if(battle_config.etc_log)
		printf("intif: guild member added %d %d %d %d\n",RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOB(fd,14));
	guild_member_added(RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOB(fd,14));
	return 0;
}

// �M���h�����o�E��/�Ǖ��ʒm
static int intif_parse_GuildMemberLeaved(int fd)
{
	guild_member_leaved(RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOB(fd,14),
		RFIFOP(fd,55),RFIFOP(fd,15));
	return 0;
}

// �M���h�����o�I�����C�����/Lv�ύX�ʒm
static int intif_parse_GuildMemberInfoShort(int fd)
{
	guild_recv_memberinfoshort(RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOB(fd,14),RFIFOW(fd,15),RFIFOW(fd,17));
	return 0;
}

// �M���h���U�ʒm
static int intif_parse_GuildBroken(int fd)
{
	guild_broken(RFIFOL(fd,2),RFIFOB(fd,6));
	return 0;
}

// �M���h��{���ύX�ʒm
static int intif_parse_GuildBasicInfoChanged(int fd)
{
	int type     = RFIFOW(fd,8);
	void *data   = RFIFOP(fd,10);
	struct guild *g = guild_search(RFIFOL(fd,4));
	short dw = *((short *)data);
	int dd = *((int *)data);

	if(g == NULL)
		return 0;

	switch(type) {
		case GBI_EXP:        g->exp = dd;         break;
		case GBI_GUILDLV:    g->guild_lv = dw;    break;
		case GBI_SKILLPOINT: g->skill_point = dd; break;
	}
	return 0;
}

// �M���h�����o���ύX�ʒm
static int intif_parse_GuildMemberInfoChanged(int fd)
{
	int type       = RFIFOW(fd,16);
	void *data     = RFIFOP(fd,18);
	struct guild *g = guild_search(RFIFOL(fd,4));
	int idx, dd = *((int *)data);

	if(g == NULL)
		return 0;

	idx = guild_getindex(g,RFIFOL(fd,8),RFIFOL(fd,12));
	if(idx < 0)
		return 0;

	switch(type) {
		case GMI_POSITION:
			guild_memberposition_changed(g,idx,dd);
			break;
		case GMI_EXP:
			g->member[idx].exp = dd;
			break;
	}
	return 0;
}

// �M���h��E�ύX�ʒm
static int intif_parse_GuildPosition(int fd)
{
	if( RFIFOW(fd,2)!=sizeof(struct guild_position)+12 ){
		if(battle_config.error_log)
			printf("intif: guild info : data size error %d %d %lu\n",RFIFOL(fd,4),RFIFOW(fd,2),(unsigned long)(sizeof(struct guild_position)+12));
	}
	guild_position_changed(RFIFOL(fd,4),RFIFOL(fd,8),(struct guild_position *)RFIFOP(fd,12));
	return 0;
}

// �M���h�X�L������U��ʒm
static int intif_parse_GuildSkillUp(int fd)
{
	guild_skillupack(RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOB(fd,14));
	return 0;
}

// �M���h����/�G�Βʒm
static int intif_parse_GuildAlliance(int fd)
{
	guild_allianceack(RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOL(fd,14),RFIFOB(fd,18),RFIFOP(fd,19),RFIFOP(fd,43));
	return 0;
}

// �M���h���m�ύX�ʒm
static int intif_parse_GuildNotice(int fd)
{
	guild_notice_changed(RFIFOL(fd,2),RFIFOP(fd,6),RFIFOP(fd,66));
	return 0;
}

// �M���h�G���u�����ύX�ʒm
static int intif_parse_GuildEmblem(int fd)
{
	guild_emblem_changed(RFIFOW(fd,2)-12,RFIFOL(fd,4),RFIFOL(fd,8),RFIFOP(fd,12));
	return 0;
}

// �M���h��b��M
static int intif_parse_GuildMessage(int fd)
{
	guild_recv_message(RFIFOL(fd,4),RFIFOL(fd,8),RFIFOP(fd,12),RFIFOW(fd,2)-12);
	return 0;
}

// �M���h��f�[�^�v���ԐM
static void intif_parse_GuildCastleDataLoad(int fd)
{
	guild_castledataloadack(RFIFOW(fd,2),RFIFOB(fd,4),RFIFOL(fd,5));

	return;
}

// �M���h��f�[�^�ύX�ʒm
static void intif_parse_GuildCastleDataSave(int fd)
{
	guild_castledatasaveack(RFIFOW(fd,2),RFIFOB(fd,4),RFIFOL(fd,5));

	return;
}

// �M���h��f�[�^�ꊇ��M(��������)
static void intif_parse_GuildCastleAllDataLoad(int fd)
{
	guild_castlealldataload(RFIFOW(fd,2),(struct guild_castle *)RFIFOP(fd,4));

	return;
}

// �M���h�X�L���c���[�ő�l��M(��������)
static void intif_parse_GuildSkillMaxLoad(int fd)
{
	guild_skillmax_load(RFIFOW(fd,2),(int *)RFIFOP(fd,4));

	return;
}

// pet
static int intif_parse_CreatePet(int fd)
{
	pet_get_egg(RFIFOL(fd,2),RFIFOL(fd,7),RFIFOB(fd,6));

	return 0;
}

static int intif_parse_RecvPetData(int fd)
{
	struct s_pet p;
	int len=RFIFOW(fd,2);

	if(sizeof(struct s_pet)!=len-9) {
		if(battle_config.etc_log)
			printf("intif: pet data: data size error %lu %d\n",(unsigned long)sizeof(struct s_pet),len-9);
	} else {
		memcpy(&p,RFIFOP(fd,9),sizeof(struct s_pet));
		pet_recv_petdata(RFIFOL(fd,4),&p,RFIFOB(fd,8));
	}

	return 0;
}

static int intif_parse_SavePetOk(int fd)
{
	if(RFIFOB(fd,6) == 1) {
		if(battle_config.error_log)
			printf("pet data save failure\n");
	}

	return 0;
}

static int intif_parse_DeletePetOk(int fd)
{
	if(RFIFOB(fd,2) == false) {
		if(battle_config.error_log)
			printf("pet data delete failure\n");
	}

	return 0;
}

/*==========================================
 * homun
 *------------------------------------------
 */
static int intif_parse_RecvHomData(int fd)
{
	struct mmo_homunstatus p;
	int len=RFIFOW(fd,2);

	if(sizeof(struct mmo_homunstatus)!=len-13) {
		if(battle_config.etc_log)
			printf("intif: hom data: data size error %lu %d\n",(unsigned long)sizeof(struct mmo_homunstatus),len-13);
	} else {
		memcpy(&p,RFIFOP(fd,13),sizeof(struct mmo_homunstatus));
		homun_recv_homdata(RFIFOL(fd,4),RFIFOL(fd,8),&p,RFIFOB(fd,12));
	}

	return 0;
}

static int intif_parse_SaveHomOk(int fd)
{
	if(RFIFOB(fd,6) == 1) {
		if(battle_config.error_log)
			printf("hom data save failure\n");
	}

	return 0;
}

static int intif_parse_DeleteHomOk(int fd)
{
	if(RFIFOB(fd,2) == false) {
		if(battle_config.error_log)
			printf("hom data delete failure\n");
	}

	return 0;
}

/*==========================================
 * �b��
 *------------------------------------------
 */
static int intif_parse_RecvMercData(int fd)
{
	struct mmo_mercstatus p;
	int len=RFIFOW(fd,2);

	if(sizeof(struct mmo_mercstatus)!=len-13) {
		if(battle_config.etc_log)
			printf("intif: merc data: data size error %lu %d\n",(unsigned long)sizeof(struct mmo_mercstatus),len-13);
	} else {
		memcpy(&p,RFIFOP(fd,13),sizeof(struct mmo_mercstatus));
		merc_recv_mercdata(RFIFOL(fd,4),RFIFOL(fd,8),&p,RFIFOB(fd,12));
	}

	return 0;
}

static int intif_parse_SaveMercOk(int fd)
{
	if(RFIFOB(fd,6) == 1) {
		if(battle_config.error_log)
			printf("merc data save failure\n");
	}

	return 0;
}

static int intif_parse_DeleteMercOk(int fd)
{
	if(RFIFOB(fd,2) == false) {
		if(battle_config.error_log)
			printf("merc data delete failure\n");
	}

	return 0;
}

/*==========================================
 * ����
 *------------------------------------------
 */
static int intif_parse_RecvElemData(int fd)
{
	struct mmo_elemstatus p;
	int len=RFIFOW(fd,2);

	if(sizeof(struct mmo_elemstatus)!=len-13) {
		if(battle_config.etc_log)
			printf("intif: elem data: data size error %lu %d\n",(unsigned long)sizeof(struct mmo_elemstatus),len-13);
	} else {
		memcpy(&p,RFIFOP(fd,13),sizeof(struct mmo_elemstatus));
		elem_recv_elemdata(RFIFOL(fd,4),RFIFOL(fd,8),&p,RFIFOB(fd,12));
	}

	return 0;
}

static int intif_parse_SaveElemOk(int fd)
{
	if(RFIFOB(fd,6) == 1) {
		if(battle_config.error_log)
			printf("elem data save failure\n");
	}

	return 0;
}

static int intif_parse_DeleteElemOk(int fd)
{
	if(RFIFOB(fd,2) == false) {
		if(battle_config.error_log)
			printf("elem data delete failure\n");
	}

	return 0;
}

/*==========================================
 * ���[���֘A
 *------------------------------------------
 */
static int intif_parse_MailSendRes(int fd)
{
	struct map_session_data *sd = map_id2sd(RFIFOL(fd,2));
	int fail = RFIFOB(fd,6);

	if(sd) {
		clif_res_sendmail(sd->fd,fail);
		if(fail)
			mail_removeitem(sd,0);
	}
	return 0;
}

static int intif_parse_MailBoxLoad(int fd)
{
	struct map_session_data *sd = map_nick2sd(RFIFOP(fd,8));

	if(sd)
		clif_send_mailbox(sd,RFIFOL(fd,4),(struct mail_data *)RFIFOP(fd,32));

	return 0;
}

static int intif_parse_ArriveNewMail(int fd)
{
	struct map_session_data *sd;
	struct mail_data md;

	memcpy(&md,RFIFOP(fd,4),sizeof(struct mail_data));
	if((sd = map_nick2sd(md.receive_name))==NULL)
		return 0;
	clif_arrive_newmail(sd->fd,&md);
	return 0;
}

static int intif_parse_ReadMail(int fd)
{
	struct map_session_data *sd;
	struct mail_data md;

	memcpy(&md,RFIFOP(fd,4),sizeof(struct mail_data));
	if((sd = map_nick2sd(md.receive_name))==NULL)
		return 0;
	clif_receive_mail(sd,&md);
	return 0;
}

static int intif_parse_MailDeleteRes(int fd)
{
	struct map_session_data *sd = map_id2sd(RFIFOL(fd,2));

	if(sd)
		clif_deletemail_res(sd->fd,RFIFOL(fd,6),RFIFOB(fd,10));
	return 0;
}

static int intif_parse_MailGetAppend(int fd)
{
	mail_getappend(RFIFOL(fd,4),RFIFOL(fd,8),RFIFOL(fd,12),(struct item *)RFIFOP(fd,16));

	return 0;
}

static int intif_parse_MailCheckOK(int fd)
{
	struct map_session_data *sd = map_id2sd(RFIFOL(fd,4));

	if(sd)
		mail_sendmail(sd,(struct mail_data *)RFIFOP(fd,8));

	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�֘A
 *------------------------------------------
 */
static int intif_parse_LoadStatusChange(int fd)
{
	short len = RFIFOW(fd,2);
	struct map_session_data *sd = map_id2sd(RFIFOL(fd,4));
	short type = 0;
	int p,calc_flag=0;

	if(sd == NULL)
		return 0;

	for(p=8; p<len; p+=22) {
		type = RFIFOW(fd,p);
		if(status_can_save(type))
		{
			int tick = (int)RFIFOL(fd,p+18);
			if(tick <= 0)
				continue;
			// �����ł͌��ʎ��ԕ␳����уX�e�[�^�X�Čv�Z���Ȃ��iflag=2+4�j
			status_change_start(&sd->bl, type, RFIFOL(fd,p+2), RFIFOL(fd,p+6), RFIFOL(fd,p+10), RFIFOL(fd,p+14), (unsigned int)tick, 6);
			calc_flag = 1;
		}
	}
	if(calc_flag)
		status_calc_pc(sd,0);

	return 0;
}

static int intif_parse_SaveStatusChange(int fd)
{
	if(RFIFOB(fd,6) == 1) {
		if(battle_config.error_log)
			printf("status change data save failure\n");
	}

	return 0;
}

/*==========================================
 * �N�G�X�g�f�[�^�֘A
 *------------------------------------------
 */
static int intif_parse_LoadQuestList(int fd)
{
	short len = RFIFOW(fd,2);
	struct map_session_data *sd = map_id2sd(RFIFOL(fd,4));
	int i,p;

	if(sd == NULL)
		return 0;

	for(i=0,p=8; p<len; i++,p+=27) {
		sd->quest[i].nameid       = RFIFOL(fd,p);
		sd->quest[i].state        = RFIFOB(fd,p+4);
		sd->quest[i].limit        = (unsigned int)RFIFOL(fd,p+5);
		sd->quest[i].mob[0].id    = RFIFOW(fd,p+9);
		sd->quest[i].mob[0].max   = RFIFOW(fd,p+11);
		sd->quest[i].mob[0].count = RFIFOW(fd,p+13);
		sd->quest[i].mob[1].id    = RFIFOW(fd,p+15);
		sd->quest[i].mob[1].max   = RFIFOW(fd,p+17);
		sd->quest[i].mob[1].count = RFIFOW(fd,p+19);
		sd->quest[i].mob[2].id    = RFIFOW(fd,p+21);
		sd->quest[i].mob[2].max   = RFIFOW(fd,p+23);
		sd->quest[i].mob[2].count = RFIFOW(fd,p+25);
	}
	sd->questlist = i;

	if(i) {
		clif_questlist(sd);
		clif_questlist_info(sd);
	}

	return 0;
}

static int intif_parse_SaveQuestList(int fd)
{
	if(RFIFOB(fd,6) == 1) {
		if(battle_config.error_log)
			printf("quest data save failure\n");
	}

	return 0;
}

/*==========================================
 * �L���������݂�����Inter�ւ��̈ʒu��ԐM
 *------------------------------------------
 */
static int intif_parse_CharPosReq(int fd)
{
	struct map_session_data *sd=map_nick2sd(RFIFOP(fd,6));

	if (inter_fd < 0)
		return -1;

	if (sd == NULL)
		return 0;

	WFIFOW(inter_fd,0)=0x3091;
	memcpy(WFIFOP(inter_fd,2),RFIFOP(fd,2),29);
	memcpy(WFIFOP(inter_fd,31),sd->mapname,16);
	WFIFOW(inter_fd,47)=sd->bl.x;
	WFIFOW(inter_fd,49)=sd->bl.y;
	WFIFOSET(inter_fd,51);

	return 0;
}

/*==========================================
 * Inter����ΏۃL�����̋��ꏊ�������ė���
 * flag=0 @where
 * flag=1 @jumpto
 *------------------------------------------
 */
static int intif_parse_CharPos(int fd)
{
	struct map_session_data *sd=map_id2sd(RFIFOL(fd,2));
	int flag=RFIFOB(fd,30);

	if(sd){
		if(flag==1){
			pc_setpos(sd,RFIFOP(fd,31),RFIFOW(fd,47),RFIFOW(fd,49),3);
			msg_output(sd->fd, msg_txt(4), RFIFOP(fd,6));	// Jump to %s
		}else{
			msg_output(sd->fd, "%s %s %d %d", RFIFOP(fd,6), RFIFOP(fd,31), RFIFOW(fd,47), RFIFOW(fd,49));
		}
	}
	return 0;
}
/*==========================================
 * �L�����N�^�[���w��ʒu�Ɉړ�������
 * flag=0 ���Ȃ����Ɉ��������Ȃ̂Ŏw�ւ̏����`�F�b�N
 * flag=1 @recall�Ȃ̂�GM���x�����ׂ��胁�b�Z�[�W��\��������
 * flag=2 �{�q�n�Ăяo���X�L��
 *------------------------------------------
 */
static int intif_parse_CharMoveReq(int fd)
{
	struct map_session_data *sd=map_nick2sd(RFIFOP(fd,6));
	int flag=RFIFOB(fd,30);
	int account_id=RFIFOL(fd,2);

	printf("intif_parse_CharMoveReq: %d %s %s %d %d\n",RFIFOL(fd,2),RFIFOP(fd,6),RFIFOP(fd,31),RFIFOW(fd,47),RFIFOW(fd,49));

	if(sd){
		if(flag==0){
			// ���Ȃ��Ɉ�������
			int i;
			for( i = 0; i < MAX_INVENTORY; i++ ){
				if( (sd->status.inventory[i].nameid == WEDDING_RING_M || sd->status.inventory[i].nameid == WEDDING_RING_F) && sd->status.inventory[i].equip ){
					pc_setpos(sd,RFIFOP(fd,31),RFIFOW(fd,47),RFIFOW(fd,49),3);
					break;
				}
			}
		}
		else if(flag==1){
			char output[200];
			if(pc_numisGM(account_id) > pc_isGM(sd)){	// @recall���Ăяo����GM���x�����傫��
				pc_setpos(sd,RFIFOP(fd,31),RFIFOW(fd,47),RFIFOW(fd,49),2);
				snprintf(output, sizeof output, msg_txt(46), RFIFOP(fd,6));
				intif_displaymessage(account_id,output);
			}
		}else{
			pc_setpos(sd,RFIFOP(fd,31),RFIFOW(fd,47),RFIFOW(fd,49),3);
		}
	}
	return 0;
}

/*==========================================
 * �Ώ�ID�Ƀ��b�Z�[�W�𑗐M
 *------------------------------------------
 */
static int intif_parse_DisplayMessage(int fd)
{
	struct map_session_data *sd=map_id2sd(RFIFOL(fd,4));

	if(sd){
		clif_displaymessage(sd->fd,RFIFOP(fd,8));
	}
	return 0;
}

//-----------------------------------------------------------------
// inter server����̒ʐM
// �G���[�������0(false)��Ԃ�����
// �p�P�b�g�������ł����1,�p�P�b�g��������Ȃ����2��Ԃ�����
int intif_parse(int fd)
{
	int packet_len;
	int cmd = RFIFOW(fd,0);

	// �p�P�b�g��ID�m�F
	if(cmd < 0x3800 || cmd >= 0x3800 + sizeof(packet_len_table) / sizeof(packet_len_table[0]))
		return 0;

	// �p�P�b�g�̒����m�F
	packet_len = packet_len_table[cmd-0x3800];
	if(packet_len == 0)
		return 0;

	if(packet_len == -1) {
		if(RFIFOREST(fd) < 4)
			return 2;
		packet_len = RFIFOW(fd,2);
	}
	if(RFIFOREST(fd) < packet_len)
		return 2;

	// ��������
	switch(cmd) {
	case 0x3800:
		if(RFIFOL(fd,4) == 0xFF000000)
			clif_GMmessage(NULL,(char*)RFIFOP(fd,16),packet_len-16,0);	// non color
		else
			clif_announce(NULL,(char*)RFIFOP(fd,16),packet_len-16,RFIFOL(fd,4),RFIFOW(fd,8),RFIFOW(fd,10),RFIFOW(fd,12),RFIFOW(fd,14),0);	// multi-color
		break;
	case 0x3801: intif_parse_WisMessage(fd); break;
	case 0x3802: intif_parse_WisEnd(fd); break;
	case 0x3804: intif_parse_AccountReg(fd); break;
	case 0x3810: intif_parse_LoadStorage(fd); break;
	case 0x3811: intif_parse_SaveStorage(fd); break;
	case 0x3818: intif_parse_LoadGuildStorage(fd); break;
	case 0x3819: intif_parse_SaveGuildStorage(fd); break;
	case 0x381a: intif_parse_TrylockGuildStorageAck(fd); break;
	case 0x381b: intif_parse_UnlockGuildStorageAck(fd); break;
	case 0x381c: intif_parse_ChecklockGuildStorage(fd); break;
	case 0x3820: intif_parse_PartyCreated(fd); break;
	case 0x3821: intif_parse_PartyInfo(fd); break;
	case 0x3822: intif_parse_PartyMemberAdded(fd); break;
	case 0x3823: intif_parse_PartyOptionChanged(fd); break;
	case 0x3824: intif_parse_PartyMemberLeaved(fd); break;
	case 0x3825: intif_parse_PartyMove(fd); break;
	case 0x3826: intif_parse_PartyBroken(fd); break;
	case 0x3827: intif_parse_PartyMessage(fd); break;
	case 0x3828: intif_parse_PartyLeaderChanged(fd); break;
	case 0x3830: intif_parse_GuildCreated(fd); break;
	case 0x3831: intif_parse_GuildInfo(fd); break;
	case 0x3832: intif_parse_GuildMemberAdded(fd); break;
	case 0x3834: intif_parse_GuildMemberLeaved(fd); break;
	case 0x3835: intif_parse_GuildMemberInfoShort(fd); break;
	case 0x3836: intif_parse_GuildBroken(fd); break;
	case 0x3837: intif_parse_GuildMessage(fd); break;
	case 0x3839: intif_parse_GuildBasicInfoChanged(fd); break;
	case 0x383a: intif_parse_GuildMemberInfoChanged(fd); break;
	case 0x383b: intif_parse_GuildPosition(fd); break;
	case 0x383c: intif_parse_GuildSkillUp(fd); break;
	case 0x383d: intif_parse_GuildAlliance(fd); break;
	case 0x383e: intif_parse_GuildNotice(fd); break;
	case 0x383f: intif_parse_GuildEmblem(fd); break;
	case 0x3840: intif_parse_GuildCastleDataLoad(fd); break;
	case 0x3841: intif_parse_GuildCastleDataSave(fd); break;
	case 0x3842: intif_parse_GuildCastleAllDataLoad(fd); break;
	case 0x3843: intif_parse_GuildSkillMaxLoad(fd); break;
	case 0x3848: intif_parse_MailSendRes(fd); break;
	case 0x3849: intif_parse_MailBoxLoad(fd); break;
	case 0x384a: intif_parse_ArriveNewMail(fd); break;
	case 0x384b: intif_parse_ReadMail(fd); break;
	case 0x384c: intif_parse_MailDeleteRes(fd); break;
	case 0x384d: intif_parse_MailGetAppend(fd); break;
	case 0x384e: intif_parse_MailCheckOK(fd); break;
	case 0x3860: intif_parse_LoadQuestList(fd); break;
	case 0x3861: intif_parse_SaveQuestList(fd); break;
	case 0x3870: intif_parse_RecvMercData(fd); break;
	case 0x3871: intif_parse_SaveMercOk(fd); break;
	case 0x3872: intif_parse_DeleteMercOk(fd); break;
	case 0x3878: intif_parse_LoadStatusChange(fd); break;
	case 0x3879: intif_parse_SaveStatusChange(fd); break;
	case 0x387c: intif_parse_RecvElemData(fd); break;
	case 0x387d: intif_parse_SaveElemOk(fd); break;
	case 0x387e: intif_parse_DeleteElemOk(fd); break;
	case 0x3880: intif_parse_CreatePet(fd); break;
	case 0x3881: intif_parse_RecvPetData(fd); break;
	case 0x3882: intif_parse_SavePetOk(fd); break;
	case 0x3883: intif_parse_DeletePetOk(fd); break;
	case 0x3888: intif_parse_RecvHomData(fd); break;
	case 0x3889: intif_parse_SaveHomOk(fd); break;
	case 0x388a: intif_parse_DeleteHomOk(fd); break;
	case 0x3890: intif_parse_CharPosReq(fd); break;
	case 0x3891: intif_parse_CharPos(fd); break;
	case 0x3892: intif_parse_CharMoveReq(fd); break;
	case 0x3893: intif_parse_DisplayMessage(fd); break;
	default:
		if(battle_config.error_log)
			printf("intif_parse : unknown packet %d %x\n",fd,RFIFOW(fd,0));
		return 0;
	}
	// �p�P�b�g�ǂݔ�΂�
	RFIFOSKIP(fd,packet_len);

	return 1;
}
