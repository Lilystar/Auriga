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

#define DUMP_UNKNOWN_PACKET	1

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#ifndef WINDOWS
	#include <unistd.h>	// sleep
#else
	#include <windows.h>
#endif

#include "socket.h"
#include "timer.h"
#include "malloc.h"
#include "version.h"
#include "nullpo.h"
#include "httpd.h"
#include "utils.h"
#include "mmo.h"

#include "map.h"
#include "chrif.h"
#include "clif.h"
#include "pc.h"
#include "npc.h"
#include "itemdb.h"
#include "chat.h"
#include "trade.h"
#include "storage.h"
#include "script.h"
#include "skill.h"
#include "atcommand.h"
#include "intif.h"
#include "battle.h"
#include "mob.h"
#include "party.h"
#include "guild.h"
#include "vending.h"
#include "pet.h"
#include "status.h"
#include "friend.h"
#include "unit.h"
#include "mail.h"
#include "homun.h"
#include "ranking.h"
#include "merc.h"
#include "booking.h"

/* �p�P�b�g�f�[�^�x�[�X */
struct packet_db packet_db[MAX_PACKET_DB];

// local define
enum {
	ALL_CLIENT,
	ALL_SAMEMAP,
	AREA,
	AREA_WOS,
	AREA_WOC,
	AREA_WOSC,
	AREA_CHAT_WOC,
	CHAT,
	CHAT_WOS,
	PARTY,
	PARTY_WOS,
	PARTY_SAMEMAP,
	PARTY_SAMEMAP_WOS,
	PARTY_AREA,
	PARTY_AREA_WOS,
	GUILD,
	GUILD_WOS,
	GUILD_SAMEMAP_WOS,
	SELF
};

#define WBUFPOS(p,pos,x,y,dir) \
	{ \
		unsigned char *__p = (p); \
		__p += (pos); \
		__p[0] = (unsigned char)((x)>>2); \
		__p[1] = (unsigned char)(((x)<<6) | (((y)>>4)&0x3f)); \
		__p[2] = (unsigned char)(((y)<<4) | ((dir)&0xf)); \
	}

#define WBUFPOS2(p,pos,x0,y0,x1,y1,sx0,sy0) \
	{ \
		unsigned char *__p = (p); \
		__p += (pos); \
		__p[0] = (unsigned char)((x0)>>2); \
		__p[1] = (unsigned char)(((x0)<<6) | (((y0)>>4)&0x3f)); \
		__p[2] = (unsigned char)(((y0)<<4) | (((x1)>>6)&0x0f)); \
		__p[3] = (unsigned char)(((x1)<<2) | (((y1)>>8)&0x03)); \
		__p[4] = (unsigned char)(y1); \
		__p[5] = (unsigned char)(((sx0)<<4) | ((sy0)&0x0f)); \
	}

#define WFIFOPOS(fd,pos,x,y,dir) { WBUFPOS(WFIFOP(fd,pos),0,x,y,dir); }
#define WFIFOPOS2(fd,pos,x0,y0,x1,y1,sx0,sy0) { WBUFPOS2(WFIFOP(fd,pos),0,x0,y0,x1,y1,sx0,sy0); }

static char map_host[256] = "";
static unsigned long map_ip;
static unsigned short map_port = 5121;
static int map_fd;

static int g_packet_len = 0;
#define GETPACKETPOS(cmd,idx)  ( packet_db[cmd].pos[idx] + ( ( packet_db[cmd].pos[idx] < 0 )? g_packet_len : 0 ) )


/*==========================================
 * map�I�̃z�X�g�ݒ�
 *------------------------------------------
 */
void clif_sethost(const char *host)
{
	size_t len = strlen(host) + 1;

	if(len > sizeof(map_host))
		len = sizeof(map_host);

	memcpy(map_host, host, len);
	map_host[len-1] = '\0';	// force \0 terminal

	return;
}

/*==========================================
 * map�I��ip�ݒ�
 *------------------------------------------
 */
void clif_setip(void)
{
	map_ip = host2ip(map_host, "Map server IP address");

	return;
}

/*==========================================
 * map�I��port�ݒ�
 *------------------------------------------
 */
void clif_setport(unsigned short port)
{
	map_port = port;

	return;
}

/*==========================================
 * map�I��ip�ǂݏo��
 *------------------------------------------
 */
unsigned long clif_getip(void)
{
	return map_ip;
}

/*==========================================
 * map�I��port�ǂݏo��
 *------------------------------------------
 */
unsigned short clif_getport(void)
{
	return map_port;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_countusers(void)
{
	int users=0,i;
	struct map_session_data *sd;

	for(i=0;i<fd_max;i++){
		if( session[i] && (sd = (struct map_session_data *)session[i]->session_data) && sd->state.auth &&
		    !(battle_config.hide_GM_session && pc_isGM(sd)) )
			users++;
	}

	return users;
}

/*==========================================
 * �S�Ă�client�ɑ΂���func()���s
 *------------------------------------------
 */
int clif_foreachclient(int (*func)(struct map_session_data*,va_list),...)
{
	int i, ret = 0;
	struct map_session_data *sd;

	for(i=0;i<fd_max;i++){
		if(
			session[i] && session[i]->func_parse == clif_parse &&
			(sd = (struct map_session_data *)session[i]->session_data) && sd->state.auth &&
			!sd->state.waitingdisconnect
		) {
			va_list ap;
			va_start(ap, func);
			ret += func(sd,ap);
			va_end(ap);
		}
	}

	return ret;
}

/*==========================================
 * clif_send��AREA*�w�莞�p
 *------------------------------------------
 */
static int clif_send_sub(struct block_list *bl,va_list ap)
{
	unsigned char *buf;
	int len;
	struct block_list *src_bl;
	int type, fd;
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd=(struct map_session_data *)bl);

	buf=va_arg(ap,unsigned char*);
	len=va_arg(ap,int);
	nullpo_retr(0, src_bl=va_arg(ap,struct block_list*));
	type=va_arg(ap,int);

	switch(type){
	case AREA_WOS:
		if(bl==src_bl)
			return 0;
		break;
	case AREA_WOC:
		if(sd->chatID || bl==src_bl)
			return 0;
		break;
	case AREA_WOSC:
		if(sd->chatID && src_bl->type == BL_PC && sd->chatID == ((struct map_session_data*)src_bl)->chatID)
			return 0;
		break;
	}

	fd = sd->fd;
	memcpy(WFIFOP(fd,0),buf,len);
	WFIFOSET(fd,len);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_send(unsigned char *buf, int len, struct block_list *bl, int type)
{
	int i;
	struct map_session_data *sd = NULL;
	int x0=0,x1=0,y0=0,y1=0;

	if( type != ALL_CLIENT ){
		nullpo_retv(bl);
	}

	switch(type){
	case ALL_CLIENT:	// �S�N���C�A���g�ɑ��M
		for(i=0;i<fd_max;i++){
			if(session[i] && (sd = (struct map_session_data *)session[i]->session_data) && sd->state.auth){
				memcpy(WFIFOP(i,0),buf,len);
				WFIFOSET(i,len);
			}
		}
		break;
	case ALL_SAMEMAP:	// �����}�b�v�̑S�N���C�A���g�ɑ��M
		for(i=0;i<fd_max;i++){
			if(session[i] && (sd = (struct map_session_data *)session[i]->session_data) && sd->state.auth && sd->bl.m == bl->m){
				memcpy(WFIFOP(i,0),buf,len);
				WFIFOSET(i,len);
			}
		}
		break;
	case AREA:
	case AREA_WOS:
	case AREA_WOC:
	case AREA_WOSC:
		map_foreachinarea(clif_send_sub,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,BL_PC,buf,len,bl,type);
		break;
	case AREA_CHAT_WOC:
		map_foreachinarea(clif_send_sub,bl->m,bl->x-(AREA_SIZE-5),bl->y-(AREA_SIZE-5),bl->x+(AREA_SIZE-5),bl->y+(AREA_SIZE-5),BL_PC,buf,len,bl,AREA_WOC);
		break;
	case CHAT:
	case CHAT_WOS:
		{
			struct chat_data *cd = NULL;
			if(bl->type==BL_PC){
				sd=(struct map_session_data*)bl;
				if(sd->chatID)
					cd=map_id2cd(sd->chatID);
			} else if(bl->type==BL_CHAT) {
				cd=(struct chat_data*)bl;
			}
			if(cd==NULL)
				break;
			for(i=0;i<cd->users;i++){
				if(type==CHAT_WOS && &cd->usersd[i]->bl == bl)
					continue;
				memcpy(WFIFOP(cd->usersd[i]->fd,0),buf,len);
				WFIFOSET(cd->usersd[i]->fd,len);
			}
		}
		break;

	case PARTY_AREA:		// ������ʓ��̑S�p�[�e�B�[�����o�ɑ��M
	case PARTY_AREA_WOS:		// �����ȊO�̓�����ʓ��̑S�p�[�e�B�[�����o�ɑ��M
		x0=bl->x-AREA_SIZE;
		y0=bl->y-AREA_SIZE;
		x1=bl->x+AREA_SIZE;
		y1=bl->y+AREA_SIZE;
	case PARTY:			// �S�p�[�e�B�[�����o�ɑ��M
	case PARTY_WOS:			// �����ȊO�̑S�p�[�e�B�[�����o�ɑ��M
	case PARTY_SAMEMAP:		// �����}�b�v�̑S�p�[�e�B�[�����o�ɑ��M
	case PARTY_SAMEMAP_WOS:		// �����ȊO�̓����}�b�v�̑S�p�[�e�B�[�����o�ɑ��M
		{
			struct party *p = NULL;
			if(bl->type==BL_PC){
				sd=(struct map_session_data *)bl;
				if(sd && sd->status.party_id>0)
					p=party_search(sd->status.party_id);
			}
			if(p == NULL)
				break;
			for(i=0;i<MAX_PARTY;i++){
				if((sd=p->member[i].sd)!=NULL){
					if( sd->bl.id==bl->id && (type==PARTY_WOS ||
						type==PARTY_SAMEMAP_WOS || type==PARTY_AREA_WOS))
						continue;
					if(type!=PARTY && type!=PARTY_WOS && bl->m!=sd->bl.m)	// �}�b�v�`�F�b�N
						continue;
					if((type==PARTY_AREA || type==PARTY_AREA_WOS) &&
						(sd->bl.x<x0 || sd->bl.y<y0 ||
						 sd->bl.x>x1 || sd->bl.y>y1 ) )
						continue;

					memcpy(WFIFOP(sd->fd,0),buf,len);
					WFIFOSET(sd->fd,len);
				}
			}
		}
		break;
	case SELF:
		if(bl->type == BL_PC && (sd=(struct map_session_data *)bl)) {
			memcpy(WFIFOP(sd->fd,0),buf,len);
			WFIFOSET(sd->fd,len);
		}
		break;

	case GUILD:
	case GUILD_WOS:
	case GUILD_SAMEMAP_WOS:
		{
			struct guild *g = NULL;
			if(bl->type==BL_PC){
				sd=(struct map_session_data *)bl;
				if(sd && sd->status.guild_id>0)
					g=guild_search(sd->status.guild_id);
			}
			if(g == NULL)
				break;
			for(i=0;i<g->max_member;i++){
				if((sd=g->member[i].sd)!=NULL){
					if((type==GUILD_WOS || type==GUILD_SAMEMAP_WOS) && sd->bl.id==bl->id)
						continue;
					if(type==GUILD_SAMEMAP_WOS && sd->bl.m!=bl->m)
						continue;
					memcpy(WFIFOP(sd->fd,0),buf,len);
					WFIFOSET(sd->fd,len);
				}
			}
		}
		break;

	default:
		if(battle_config.error_log)
			printf("clif_send �܂�����ĂȂ���[\n");
		return;
	}

	return;
}

//
// �p�P�b�g����đ��M
//
/*==========================================
 *
 *------------------------------------------
 */
void clif_authok(struct map_session_data *sd)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;

#if PACKETVER < 14
	WFIFOW(fd,0)=0x73;
	WFIFOL(fd,2)=gettick();
	WFIFOPOS(fd,6,sd->bl.x,sd->bl.y,sd->dir);
	WFIFOB(fd,9)=5;
	WFIFOB(fd,10)=5;
	WFIFOSET(fd,packet_db[0x73].len);
#else
	WFIFOW(fd,0)=0x2eb;
	WFIFOL(fd,2)=gettick();
	WFIFOPOS(fd,6,sd->bl.x,sd->bl.y,sd->dir);
	WFIFOB(fd,9)=5;
	WFIFOB(fd,10)=5;
	WFIFOW(fd,11)=0;	// font
	WFIFOSET(fd,packet_db[0x2eb].len);
#endif

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_authfail_fd(int fd, unsigned int type)
{
	WFIFOW(fd,0)=0x81;
	WFIFOL(fd,2)=type;
	WFIFOSET(fd,packet_db[0x81].len);

	clif_setwaitclose(fd);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_charselectok(int id)
{
	struct map_session_data *sd;
	int fd;

	if((sd=map_id2sd(id))==NULL)
		return;

	fd=sd->fd;
	WFIFOW(fd,0)=0xb3;
	WFIFOB(fd,2)=1;
	WFIFOSET(fd,packet_db[0xb3].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_dropflooritem(struct flooritem_data *fitem)
{
	int view;
	unsigned char buf[24];

	nullpo_retv(fitem);

	if(fitem->item_data.nameid <= 0)
		return;

	// 009e <ID>.l <name ID>.w <identify flag>.B <X>.w <Y>.w <subX>.B <subY>.B <amount>.w
	WBUFW(buf,0)=0x9e;
	WBUFL(buf,2)=fitem->bl.id;
	if((view = itemdb_viewid(fitem->item_data.nameid)) > 0)
		WBUFW(buf,6)=view;
	else
		WBUFW(buf,6)=fitem->item_data.nameid;
	WBUFB(buf,8)=fitem->item_data.identify;
	WBUFW(buf,9)=fitem->bl.x;
	WBUFW(buf,11)=fitem->bl.y;
	WBUFB(buf,13)=fitem->subx;
	WBUFB(buf,14)=fitem->suby;
	WBUFW(buf,15)=fitem->item_data.amount;
	clif_send(buf,packet_db[0x9e].len,&fitem->bl,AREA);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_clearflooritem(struct flooritem_data *fitem, int fd)
{
	unsigned char buf[8];

	nullpo_retv(fitem);

	WBUFW(buf,0) = 0xa1;
	WBUFL(buf,2) = fitem->bl.id;

	if(fd < 0) {
		clif_send(buf,packet_db[0xa1].len,&fitem->bl,AREA);
	} else {
		memcpy(WFIFOP(fd,0),buf,6);
		WFIFOSET(fd,packet_db[0xa1].len);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_clearchar(struct block_list *bl, int type)
{
	unsigned char buf[8];

	nullpo_retv(bl);

	WBUFW(buf,0) = 0x80;
	WBUFL(buf,2) = bl->id;
	WBUFB(buf,6) = type;
	clif_send(buf,packet_db[0x80].len,bl,(type == 1) ? AREA : AREA_WOS);

	return;
}

static int clif_clearchar_delay_sub(int tid,unsigned int tick,int id,void *data)
{
	struct block_list *bl = (struct block_list *)data;
	unsigned char buf[8];

	WBUFW(buf,0) = 0x80;
	WBUFL(buf,2) = bl->id;
	WBUFB(buf,6) = 0;
	clif_send(buf,packet_db[0x80].len,bl,(id == 1) ? AREA_WOS : ALL_SAMEMAP);

	aFree(bl);
	return 0;
}

int clif_clearchar_delay(unsigned int tick,struct block_list *bl)
{
	struct block_list *tmpbl = (struct block_list *)aMalloc(sizeof(struct block_list));

	memcpy(tmpbl,bl,sizeof(struct block_list));
	add_timer2(tick,clif_clearchar_delay_sub,battle_config.pcview_mob_clear_type,tmpbl);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_clearchar_id(int id, unsigned char type, int fd)
{
	if(fd < 0)
		return;

	WFIFOW(fd,0) = 0x80;
	WFIFOL(fd,2) = id;
	WFIFOB(fd,6) = type;
	WFIFOSET(fd,packet_db[0x80].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set0078(struct map_session_data *sd,unsigned char *buf)
{
	nullpo_retr(0, sd);

#if PACKETVER < 4
	WBUFW(buf,0)=0x78;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->speed;
	WBUFW(buf,8)=sd->sc.opt1;
	WBUFW(buf,10)=sd->sc.opt2;
	WBUFW(buf,12)=sd->sc.option;
	WBUFW(buf,14)=sd->view_class;
	WBUFW(buf,16)=sd->status.hair;
	if(sd->view_class != 22 && sd->view_class != 26 && sd->view_class != 27)
		WBUFW(buf,18)=sd->status.weapon;
	else
		WBUFW(buf,18)=0;
	WBUFW(buf,20)=sd->status.head_bottom;
	WBUFW(buf,22)=sd->status.shield;
	WBUFW(buf,24)=sd->status.head_top;
	WBUFW(buf,26)=sd->status.head_mid;
	WBUFW(buf,28)=sd->status.hair_color;
	WBUFW(buf,30)=sd->status.clothes_color;
	WBUFW(buf,32)=sd->head_dir;
	WBUFL(buf,34)=sd->status.guild_id;
	WBUFL(buf,38)=sd->guild_emblem_id;
	WBUFW(buf,42)=sd->status.manner;
	WBUFB(buf,44)=(unsigned char)sd->status.karma;
	WBUFB(buf,45)=sd->sex;
	WBUFPOS(buf,46,sd->bl.x,sd->bl.y,sd->dir);
	WBUFB(buf,49)=5;
	WBUFB(buf,50)=5;
	WBUFB(buf,51)=sd->state.dead_sit;
	WBUFW(buf,52)=(sd->status.base_level>99)?99:sd->status.base_level;

	return packet_db[0x78].len;
#elif PACKETVER < 7
	WBUFW(buf,0)=0x1d8;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->speed;
	WBUFW(buf,8)=sd->sc.opt1;
	WBUFW(buf,10)=sd->sc.opt2;
	WBUFW(buf,12)=sd->sc.option;
	WBUFW(buf,14)=sd->view_class;
	WBUFW(buf,16)=sd->status.hair;
	if(sd->equip_index[9] >= 0
	&& sd->inventory_data[sd->equip_index[9]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WBUFW(buf,18)=sd->inventory_data[sd->equip_index[9]]->view_id;
		else
			WBUFW(buf,18)=sd->status.inventory[sd->equip_index[9]].nameid;
	}
	else
		WBUFW(buf,18)=0;
	if(sd->equip_index[8] >= 0
	&& sd->equip_index[8] != sd->equip_index[9]
	&& sd->inventory_data[sd->equip_index[8]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WBUFW(buf,20)=sd->inventory_data[sd->equip_index[8]]->view_id;
		else
			WBUFW(buf,20)=sd->status.inventory[sd->equip_index[8]].nameid;
	}
	else
		WBUFW(buf,20)=0;
	WBUFW(buf,22)=sd->status.head_bottom;
	WBUFW(buf,24)=sd->status.head_top;
	WBUFW(buf,26)=sd->status.head_mid;
	WBUFW(buf,28)=sd->status.hair_color;
	WBUFW(buf,30)=sd->status.clothes_color;
	WBUFW(buf,32)=sd->head_dir;
	WBUFL(buf,34)=sd->status.guild_id;
	WBUFW(buf,38)=sd->guild_emblem_id;
	WBUFW(buf,40)=sd->status.manner;
	WBUFW(buf,42)=sd->sc.opt3;
	WBUFB(buf,44)=(unsigned char)sd->status.karma;
	WBUFB(buf,45)=sd->sex;
	WBUFPOS(buf,46,sd->bl.x,sd->bl.y,sd->dir);
	WBUFB(buf,49)=5;
	WBUFB(buf,50)=5;
	WBUFB(buf,51)=sd->state.dead_sit;
	WBUFW(buf,52)=(sd->status.base_level>99)?99:sd->status.base_level;

	return packet_db[0x1d8].len;
#elif PACKETVER < 14
	WBUFW(buf,0)=0x22a;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->speed;
	WBUFW(buf,8)=sd->sc.opt1;
	WBUFW(buf,10)=sd->sc.opt2;
	WBUFL(buf,12)=sd->sc.option;
	WBUFW(buf,16)=sd->view_class;
	WBUFW(buf,18)=sd->status.hair;
	if(sd->equip_index[9] >= 0
	&& sd->inventory_data[sd->equip_index[9]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WBUFW(buf,20)=sd->inventory_data[sd->equip_index[9]]->view_id;
		else
			WBUFW(buf,20)=sd->status.inventory[sd->equip_index[9]].nameid;
	}
	else
		WBUFW(buf,20)=0;
	if(sd->equip_index[8] >= 0
	&& sd->equip_index[8] != sd->equip_index[9]
	&& sd->inventory_data[sd->equip_index[8]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WBUFW(buf,22)=sd->inventory_data[sd->equip_index[8]]->view_id;
		else
			WBUFW(buf,22)=sd->status.inventory[sd->equip_index[8]].nameid;
	}
	else
		WBUFW(buf,22)=0;
	WBUFW(buf,24)=sd->status.head_bottom;
	WBUFW(buf,26)=sd->status.head_top;
	WBUFW(buf,28)=sd->status.head_mid;
	WBUFW(buf,30)=sd->status.hair_color;
	WBUFW(buf,32)=sd->status.clothes_color;
	WBUFW(buf,34)=sd->head_dir;
	WBUFL(buf,36)=sd->status.guild_id;
	WBUFW(buf,40)=sd->guild_emblem_id;
	WBUFW(buf,42)=sd->status.manner;
	WBUFL(buf,44)=sd->sc.opt3;
	WBUFB(buf,48)=(unsigned char)sd->status.karma;
	WBUFB(buf,49)=sd->sex;
	WBUFPOS(buf,50,sd->bl.x,sd->bl.y,sd->dir);
	WBUFB(buf,53)=5;
	WBUFB(buf,54)=5;
	WBUFB(buf,55)=sd->state.dead_sit;
	WBUFW(buf,56)=(sd->status.base_level>99)?99:sd->status.base_level;

	return packet_db[0x22a].len;
#elif PACKETVER < 23
	WBUFW(buf,0)=0x2ee;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->speed;
	WBUFW(buf,8)=sd->sc.opt1;
	WBUFW(buf,10)=sd->sc.opt2;
	WBUFL(buf,12)=sd->sc.option;
	WBUFW(buf,16)=sd->view_class;
	WBUFW(buf,18)=sd->status.hair;
	if(sd->equip_index[9] >= 0
	&& sd->inventory_data[sd->equip_index[9]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WBUFW(buf,20)=sd->inventory_data[sd->equip_index[9]]->view_id;
		else
			WBUFW(buf,20)=sd->status.inventory[sd->equip_index[9]].nameid;
	}
	else
		WBUFW(buf,20)=0;
	if(sd->equip_index[8] >= 0
	&& sd->equip_index[8] != sd->equip_index[9]
	&& sd->inventory_data[sd->equip_index[8]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WBUFW(buf,22)=sd->inventory_data[sd->equip_index[8]]->view_id;
		else
			WBUFW(buf,22)=sd->status.inventory[sd->equip_index[8]].nameid;
	}
	else
		WBUFW(buf,22)=0;
	WBUFW(buf,24)=sd->status.head_bottom;
	WBUFW(buf,26)=sd->status.head_top;
	WBUFW(buf,28)=sd->status.head_mid;
	WBUFW(buf,30)=sd->status.hair_color;
	WBUFW(buf,32)=sd->status.clothes_color;
	WBUFW(buf,34)=sd->head_dir;
	WBUFL(buf,36)=sd->status.guild_id;
	WBUFW(buf,40)=sd->guild_emblem_id;
	WBUFW(buf,42)=sd->status.manner;
	WBUFL(buf,44)=sd->sc.opt3;
	WBUFB(buf,48)=(unsigned char)sd->status.karma;
	WBUFB(buf,49)=sd->sex;
	WBUFPOS(buf,50,sd->bl.x,sd->bl.y,sd->dir);
	WBUFB(buf,53)=5;
	WBUFB(buf,54)=5;
	WBUFB(buf,55)=sd->state.dead_sit;
	WBUFW(buf,56)=(sd->status.base_level>99)?99:sd->status.base_level;
	WBUFW(buf,58)=0;

	return packet_db[0x2ee].len;
#else
	WBUFW(buf,0)=0x7f9;
	WBUFW(buf,2)=(unsigned short)(63 + strlen(sd->status.name));
	WBUFB(buf,4)=0;
	WBUFL(buf,5)=sd->bl.id;
	WBUFW(buf,9)=sd->speed;
	WBUFW(buf,11)=sd->sc.opt1;
	WBUFW(buf,13)=sd->sc.opt2;
	WBUFL(buf,15)=sd->sc.option;
	WBUFW(buf,19)=sd->view_class;
	WBUFW(buf,21)=sd->status.hair;
	if(sd->equip_index[9] >= 0
	&& sd->inventory_data[sd->equip_index[9]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WBUFW(buf,23)=sd->inventory_data[sd->equip_index[9]]->view_id;
		else
			WBUFW(buf,23)=sd->status.inventory[sd->equip_index[9]].nameid;
	}
	else
		WBUFW(buf,23)=0;
	if(sd->equip_index[8] >= 0
	&& sd->equip_index[8] != sd->equip_index[9]
	&& sd->inventory_data[sd->equip_index[8]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WBUFW(buf,25)=sd->inventory_data[sd->equip_index[8]]->view_id;
		else
			WBUFW(buf,25)=sd->status.inventory[sd->equip_index[8]].nameid;
	}
	else
		WBUFW(buf,25)=0;
	WBUFW(buf,27)=sd->status.head_bottom;
	WBUFW(buf,29)=sd->status.head_top;
	WBUFW(buf,31)=sd->status.head_mid;
	WBUFW(buf,33)=sd->status.hair_color;
	WBUFW(buf,35)=sd->status.clothes_color;
	WBUFW(buf,37)=sd->head_dir;
	WBUFL(buf,39)=sd->status.guild_id;
	WBUFW(buf,43)=sd->guild_emblem_id;
	WBUFW(buf,45)=sd->status.manner;
	WBUFL(buf,47)=sd->sc.opt3;
	WBUFB(buf,51)=(unsigned char)sd->status.karma;
	WBUFB(buf,52)=sd->sex;
	WBUFPOS(buf,53,sd->bl.x,sd->bl.y,sd->dir);
	WBUFB(buf,56)=5;
	WBUFB(buf,57)=5;
	WBUFB(buf,58)=sd->state.dead_sit;
	WBUFW(buf,59)=(sd->status.base_level>99)?99:sd->status.base_level;
	WBUFW(buf,61)=0;	// font
	strncpy(WBUFP(buf,63),sd->status.name,24);

	return WBUFW(buf,2);
#endif
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_set007b(struct map_session_data *sd,unsigned char *buf)
{
	nullpo_retr(0, sd);

#if PACKETVER < 4
	WBUFW(buf,0)=0x7b;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->speed;
	WBUFW(buf,8)=sd->sc.opt1;
	WBUFW(buf,10)=sd->sc.opt2;
	WBUFW(buf,12)=sd->sc.option;
	WBUFW(buf,14)=sd->view_class;
	WBUFW(buf,16)=sd->status.hair;
	if(sd->view_class != 22 && sd->view_class != 26 && sd->view_class != 27)
		WBUFW(buf,18)=sd->status.weapon;
	else
		WBUFW(buf,18)=0;
	WBUFW(buf,20)=sd->status.head_bottom;
	WBUFL(buf,22)=gettick();
	WBUFW(buf,26)=sd->status.shield;
	WBUFW(buf,28)=sd->status.head_top;
	WBUFW(buf,30)=sd->status.head_mid;
	WBUFW(buf,32)=sd->status.hair_color;
	WBUFW(buf,34)=sd->status.clothes_color;
	WBUFW(buf,36)=sd->head_dir;
	WBUFL(buf,38)=sd->status.guild_id;
	WBUFL(buf,42)=sd->guild_emblem_id;
	WBUFW(buf,46)=sd->status.manner;
	WBUFB(buf,48)=(unsigned char)sd->status.karma;
	WBUFB(buf,49)=sd->sex;
	WBUFPOS2(buf,50,sd->bl.x,sd->bl.y,sd->ud.to_x,sd->ud.to_y,8,8);
	WBUFB(buf,56)=5;
	WBUFB(buf,57)=5;
	WBUFW(buf,58)=(sd->status.base_level>99)?99:sd->status.base_level;

	return packet_db[0x7b].len;
#elif PACKETVER < 7
	WBUFW(buf,0)=0x1da;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->speed;
	WBUFW(buf,8)=sd->sc.opt1;
	WBUFW(buf,10)=sd->sc.opt2;
	WBUFW(buf,12)=sd->sc.option;
	WBUFW(buf,14)=sd->view_class;
	WBUFW(buf,16)=sd->status.hair;
	if(sd->equip_index[9] >= 0
	&& sd->inventory_data[sd->equip_index[9]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WBUFW(buf,18)=sd->inventory_data[sd->equip_index[9]]->view_id;
		else
			WBUFW(buf,18)=sd->status.inventory[sd->equip_index[9]].nameid;
	}
	else
		WBUFW(buf,18)=0;
	if(sd->equip_index[8] >= 0
	&& sd->equip_index[8] != sd->equip_index[9]
	&& sd->inventory_data[sd->equip_index[8]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WBUFW(buf,20)=sd->inventory_data[sd->equip_index[8]]->view_id;
		else
			WBUFW(buf,20)=sd->status.inventory[sd->equip_index[8]].nameid;
	}
	else
		WBUFW(buf,20)=0;
	WBUFW(buf,22)=sd->status.head_bottom;
	WBUFL(buf,24)=gettick();
	WBUFW(buf,28)=sd->status.head_top;
	WBUFW(buf,30)=sd->status.head_mid;
	WBUFW(buf,32)=sd->status.hair_color;
	WBUFW(buf,34)=sd->status.clothes_color;
	WBUFW(buf,36)=sd->head_dir;
	WBUFL(buf,38)=sd->status.guild_id;
	WBUFW(buf,42)=sd->guild_emblem_id;
	WBUFW(buf,44)=sd->status.manner;
	WBUFW(buf,46)=sd->sc.opt3;
	WBUFB(buf,48)=(unsigned char)sd->status.karma;
	WBUFB(buf,49)=sd->sex;
	WBUFPOS2(buf,50,sd->bl.x,sd->bl.y,sd->ud.to_x,sd->ud.to_y,8,8);
	WBUFB(buf,56)=5;
	WBUFB(buf,57)=5;
	WBUFW(buf,58)=(sd->status.base_level>99)?99:sd->status.base_level;

	return packet_db[0x1da].len;
#elif PACKETVER < 12
	WBUFW(buf,0)=0x22c;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->speed;
	WBUFW(buf,8)=sd->sc.opt1;
	WBUFW(buf,10)=sd->sc.opt2;
	WBUFL(buf,12)=sd->sc.option;
	WBUFW(buf,16)=sd->view_class;
	WBUFW(buf,18)=sd->status.hair;
	if(sd->equip_index[9] >= 0
	&& sd->inventory_data[sd->equip_index[9]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WBUFW(buf,20)=sd->inventory_data[sd->equip_index[9]]->view_id;
		else
			WBUFW(buf,20)=sd->status.inventory[sd->equip_index[9]].nameid;
	}
	else
		WBUFW(buf,20)=0;
	if(sd->equip_index[8] >= 0
	&& sd->equip_index[8] != sd->equip_index[9]
	&& sd->inventory_data[sd->equip_index[8]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WBUFW(buf,22)=sd->inventory_data[sd->equip_index[8]]->view_id;
		else
			WBUFW(buf,22)=sd->status.inventory[sd->equip_index[8]].nameid;
	}
	else
		WBUFW(buf,22)=0;
	WBUFW(buf,24)=sd->status.head_bottom;
	WBUFL(buf,26)=gettick();
	WBUFW(buf,30)=sd->status.head_top;
	WBUFW(buf,32)=sd->status.head_mid;
	WBUFW(buf,34)=sd->status.hair_color;
	WBUFW(buf,36)=sd->status.clothes_color;
	WBUFW(buf,38)=sd->head_dir;
	WBUFL(buf,40)=sd->status.guild_id;
	WBUFW(buf,44)=sd->guild_emblem_id;
	WBUFW(buf,46)=sd->status.manner;
	WBUFL(buf,48)=sd->sc.opt3;
	WBUFB(buf,52)=(unsigned char)sd->status.karma;
	WBUFB(buf,53)=sd->sex;
	WBUFPOS2(buf,54,sd->bl.x,sd->bl.y,sd->ud.to_x,sd->ud.to_y,8,8);
	WBUFB(buf,60)=0;
	WBUFB(buf,61)=0;
	WBUFW(buf,62)=(sd->status.base_level>99)?99:sd->status.base_level;

	return packet_db[0x22c].len;
#elif PACKETVER < 14
	WBUFW(buf,0)=0x22c;
	WBUFB(buf,2)=0;
	WBUFL(buf,3)=sd->bl.id;
	WBUFW(buf,7)=sd->speed;
	WBUFW(buf,9)=sd->sc.opt1;
	WBUFW(buf,11)=sd->sc.opt2;
	WBUFL(buf,13)=sd->sc.option;
	WBUFW(buf,17)=sd->view_class;
	WBUFW(buf,19)=sd->status.hair;
	if(sd->equip_index[9] >= 0
	&& sd->inventory_data[sd->equip_index[9]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WBUFW(buf,21)=sd->inventory_data[sd->equip_index[9]]->view_id;
		else
			WBUFW(buf,21)=sd->status.inventory[sd->equip_index[9]].nameid;
	}
	else
		WBUFW(buf,21)=0;
	if(sd->equip_index[8] >= 0
	&& sd->equip_index[8] != sd->equip_index[9]
	&& sd->inventory_data[sd->equip_index[8]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WBUFW(buf,23)=sd->inventory_data[sd->equip_index[8]]->view_id;
		else
			WBUFW(buf,23)=sd->status.inventory[sd->equip_index[8]].nameid;
	}
	else
		WBUFW(buf,23)=0;
	WBUFW(buf,25)=sd->status.head_bottom;
	WBUFL(buf,27)=gettick();
	WBUFW(buf,31)=sd->status.head_top;
	WBUFW(buf,33)=sd->status.head_mid;
	WBUFW(buf,35)=sd->status.hair_color;
	WBUFW(buf,37)=sd->status.clothes_color;
	WBUFW(buf,39)=sd->head_dir;
	WBUFL(buf,41)=sd->status.guild_id;
	WBUFW(buf,45)=sd->guild_emblem_id;
	WBUFW(buf,47)=sd->status.manner;
	WBUFL(buf,49)=sd->sc.opt3;
	WBUFB(buf,53)=(unsigned char)sd->status.karma;
	WBUFB(buf,54)=sd->sex;
	WBUFPOS2(buf,55,sd->bl.x,sd->bl.y,sd->ud.to_x,sd->ud.to_y,8,8);
	WBUFB(buf,61)=0;
	WBUFB(buf,62)=0;
	WBUFW(buf,63)=(sd->status.base_level>99)?99:sd->status.base_level;

	return packet_db[0x22c].len;
#elif PACKETVER < 23
	WBUFW(buf,0)=0x2ec;
	WBUFB(buf,2)=0;
	WBUFL(buf,3)=sd->bl.id;
	WBUFW(buf,7)=sd->speed;
	WBUFW(buf,9)=sd->sc.opt1;
	WBUFW(buf,11)=sd->sc.opt2;
	WBUFL(buf,13)=sd->sc.option;
	WBUFW(buf,17)=sd->view_class;
	WBUFW(buf,19)=sd->status.hair;
	if(sd->equip_index[9] >= 0
	&& sd->inventory_data[sd->equip_index[9]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WBUFW(buf,21)=sd->inventory_data[sd->equip_index[9]]->view_id;
		else
			WBUFW(buf,21)=sd->status.inventory[sd->equip_index[9]].nameid;
	}
	else
		WBUFW(buf,21)=0;
	if(sd->equip_index[8] >= 0
	&& sd->equip_index[8] != sd->equip_index[9]
	&& sd->inventory_data[sd->equip_index[8]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WBUFW(buf,23)=sd->inventory_data[sd->equip_index[8]]->view_id;
		else
			WBUFW(buf,23)=sd->status.inventory[sd->equip_index[8]].nameid;
	}
	else
		WBUFW(buf,23)=0;
	WBUFW(buf,25)=sd->status.head_bottom;
	WBUFL(buf,27)=gettick();
	WBUFW(buf,31)=sd->status.head_top;
	WBUFW(buf,33)=sd->status.head_mid;
	WBUFW(buf,35)=sd->status.hair_color;
	WBUFW(buf,37)=sd->status.clothes_color;
	WBUFW(buf,39)=sd->head_dir;
	WBUFL(buf,41)=sd->status.guild_id;
	WBUFW(buf,45)=sd->guild_emblem_id;
	WBUFW(buf,47)=sd->status.manner;
	WBUFL(buf,49)=sd->sc.opt3;
	WBUFB(buf,53)=(unsigned char)sd->status.karma;
	WBUFB(buf,54)=sd->sex;
	WBUFPOS2(buf,55,sd->bl.x,sd->bl.y,sd->ud.to_x,sd->ud.to_y,8,8);
	WBUFB(buf,61)=0;
	WBUFB(buf,62)=0;
	WBUFW(buf,63)=(sd->status.base_level>99)?99:sd->status.base_level;
	WBUFW(buf,65)=0;

	return packet_db[0x2ec].len;
#else
	WBUFW(buf,0)=0x7f7;
	WBUFW(buf,2)=(unsigned short)(69 + strlen(sd->status.name));
	WBUFB(buf,4)=0;
	WBUFL(buf,5)=sd->bl.id;
	WBUFW(buf,9)=sd->speed;
	WBUFW(buf,11)=sd->sc.opt1;
	WBUFW(buf,13)=sd->sc.opt2;
	WBUFL(buf,15)=sd->sc.option;
	WBUFW(buf,19)=sd->view_class;
	WBUFW(buf,21)=sd->status.hair;
	if(sd->equip_index[9] >= 0
	&& sd->inventory_data[sd->equip_index[9]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[9]]->view_id > 0)
			WBUFW(buf,23)=sd->inventory_data[sd->equip_index[9]]->view_id;
		else
			WBUFW(buf,23)=sd->status.inventory[sd->equip_index[9]].nameid;
	}
	else
		WBUFW(buf,23)=0;
	if(sd->equip_index[8] >= 0
	&& sd->equip_index[8] != sd->equip_index[9]
	&& sd->inventory_data[sd->equip_index[8]]
	&& sd->view_class != 22
	&& sd->view_class != 26
	&& sd->view_class != 27) {
		if(sd->inventory_data[sd->equip_index[8]]->view_id > 0)
			WBUFW(buf,25)=sd->inventory_data[sd->equip_index[8]]->view_id;
		else
			WBUFW(buf,25)=sd->status.inventory[sd->equip_index[8]].nameid;
	}
	else
		WBUFW(buf,25)=0;
	WBUFW(buf,27)=sd->status.head_bottom;
	WBUFL(buf,29)=gettick();
	WBUFW(buf,33)=sd->status.head_top;
	WBUFW(buf,35)=sd->status.head_mid;
	WBUFW(buf,37)=sd->status.hair_color;
	WBUFW(buf,39)=sd->status.clothes_color;
	WBUFW(buf,41)=sd->head_dir;
	WBUFL(buf,43)=sd->status.guild_id;
	WBUFW(buf,47)=sd->guild_emblem_id;
	WBUFW(buf,49)=sd->status.manner;
	WBUFL(buf,51)=sd->sc.opt3;
	WBUFB(buf,55)=(unsigned char)sd->status.karma;
	WBUFB(buf,56)=sd->sex;
	WBUFPOS2(buf,57,sd->bl.x,sd->bl.y,sd->ud.to_x,sd->ud.to_y,8,8);
	WBUFB(buf,63)=5;
	WBUFB(buf,64)=5;
	WBUFW(buf,65)=(sd->status.base_level>99)?99:sd->status.base_level;
	WBUFW(buf,67)=0;	// font
	strncpy(WBUFP(buf,69),sd->status.name,24);

	return WBUFW(buf,2);
#endif
}

/*==========================================
 * �N���X�`�F���W type��Mob�̏ꍇ��1�ő���0�H
 *------------------------------------------
 */
void clif_class_change(struct block_list *bl,int class_,int type)
{
	unsigned char buf[16];

	nullpo_retv(bl);

	if(class_ >= PC_JOB_MAX) {
		WBUFW(buf,0)=0x1b0;
		WBUFL(buf,2)=bl->id;
		WBUFB(buf,6)=type;
		WBUFL(buf,7)=class_;
		clif_send(buf,packet_db[0x1b0].len,bl,AREA);
	}

	return;
}

/*==========================================
 * MOB�\��1
 *------------------------------------------
 */
static int clif_mob0078(struct mob_data *md,unsigned char *buf)
{
	int len, level;

	nullpo_retr(0, md);

	if(mob_is_pcview(md->class_)) {
#if PACKETVER < 4
		len = packet_db[0x78].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x78;
		WBUFL(buf,2)=md->bl.id;
		WBUFW(buf,6)=status_get_speed(&md->bl);
		WBUFW(buf,8)=md->sc.opt1;
		WBUFW(buf,10)=md->sc.opt2;
		WBUFW(buf,12)=md->sc.option;
		WBUFW(buf,14)=mob_get_viewclass(md->class_);
		WBUFW(buf,16)=mob_get_hair(md->class_);
		WBUFW(buf,18)=mob_get_weapon(md->class_);
		WBUFW(buf,20)=mob_get_head_bottom(md->class_);
		WBUFW(buf,22)=mob_get_shield(md->class_);
		WBUFW(buf,24)=mob_get_head_top(md->class_);
		WBUFW(buf,26)=mob_get_head_mid(md->class_);
		WBUFW(buf,28)=mob_get_hair_color(md->class_);
		WBUFW(buf,30)=mob_get_clothes_color(md->class_);
		if(md->guild_id){
			struct guild *g=guild_search(md->guild_id);
			if(g)
				WBUFL(buf,38)=g->emblem_id;
			WBUFL(buf,34)=md->guild_id;
		}
		WBUFW(buf,42)=md->sc.opt3;
		WBUFB(buf,44)=1;
		WBUFB(buf,45)=mob_get_sex(md->class_);
		WBUFPOS(buf,46,md->bl.x,md->bl.y,md->dir);
		WBUFB(buf,49)=5;
		WBUFB(buf,50)=5;
		WBUFW(buf,52)=((level = status_get_lv(&md->bl))>99)? 99:level;
#elif PACKETVER < 7
		len = packet_db[0x1d8].len;
		memset(buf,0,packet_db[0x1d8].len);

		WBUFW(buf,0)=0x1d8;
		WBUFL(buf,2)=md->bl.id;
		WBUFW(buf,6)=status_get_speed(&md->bl);
		WBUFW(buf,8)=md->sc.opt1;
		WBUFW(buf,10)=md->sc.opt2;
		WBUFW(buf,12)=md->sc.option;
		WBUFW(buf,14)=mob_get_viewclass(md->class_);
		WBUFW(buf,16)=mob_get_hair(md->class_);
		WBUFW(buf,18)=mob_get_weapon(md->class_);
		WBUFW(buf,20)=mob_get_shield(md->class_);
		WBUFW(buf,22)=mob_get_head_bottom(md->class_);
		WBUFW(buf,24)=mob_get_head_top(md->class_);
		WBUFW(buf,26)=mob_get_head_mid(md->class_);
		WBUFW(buf,28)=mob_get_hair_color(md->class_);
		WBUFW(buf,30)=mob_get_clothes_color(md->class_);
		if(md->guild_id){
			struct guild *g=guild_search(md->guild_id);
			if(g)
				WBUFL(buf,38)=g->emblem_id;
			WBUFL(buf,34)=md->guild_id;
		}
		WBUFW(buf,42)=md->sc.opt3;
		WBUFB(buf,44)=1;
		WBUFB(buf,45)=mob_get_sex(md->class_);
		WBUFPOS(buf,46,md->bl.x,md->bl.y,md->dir);
		WBUFB(buf,49)=5;
		WBUFB(buf,50)=5;
		WBUFW(buf,52)=((level = status_get_lv(&md->bl))>99)? 99:level;
#elif PACKETVER < 14
		len = packet_db[0x22a].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x22a;
		WBUFL(buf,2)=md->bl.id;
		WBUFW(buf,6)=status_get_speed(&md->bl);
		WBUFW(buf,8)=md->sc.opt1;
		WBUFW(buf,10)=md->sc.opt2;
		WBUFL(buf,12)=md->sc.option;
		WBUFW(buf,16)=mob_get_viewclass(md->class_);
		WBUFW(buf,18)=mob_get_hair(md->class_);
		WBUFW(buf,20)=mob_get_weapon(md->class_);
		WBUFW(buf,22)=mob_get_shield(md->class_);
		WBUFW(buf,24)=mob_get_head_bottom(md->class_);
		WBUFW(buf,26)=mob_get_head_top(md->class_);
		WBUFW(buf,28)=mob_get_head_mid(md->class_);
		WBUFW(buf,30)=mob_get_hair_color(md->class_);
		WBUFW(buf,32)=mob_get_clothes_color(md->class_);
		if(md->guild_id){
			struct guild *g=guild_search(md->guild_id);
			if(g)
				WBUFW(buf,40)=g->emblem_id;
			WBUFL(buf,36)=md->guild_id;
		}
		WBUFL(buf,44)=md->sc.opt3;
		WBUFB(buf,48)=1;
		WBUFB(buf,49)=mob_get_sex(md->class_);
		WBUFPOS(buf,50,md->bl.x,md->bl.y,md->dir);
		WBUFB(buf,53)=5;
		WBUFB(buf,54)=5;
		WBUFW(buf,56)=((level = status_get_lv(&md->bl))>99)? 99:level;
#elif PACKETVER < 23
		len = packet_db[0x2ee].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x2ee;
		WBUFL(buf,2)=md->bl.id;
		WBUFW(buf,6)=status_get_speed(&md->bl);
		WBUFW(buf,8)=md->sc.opt1;
		WBUFW(buf,10)=md->sc.opt2;
		WBUFL(buf,12)=md->sc.option;
		WBUFW(buf,16)=mob_get_viewclass(md->class_);
		WBUFW(buf,18)=mob_get_hair(md->class_);
		WBUFW(buf,20)=mob_get_weapon(md->class_);
		WBUFW(buf,22)=mob_get_shield(md->class_);
		WBUFW(buf,24)=mob_get_head_bottom(md->class_);
		WBUFW(buf,26)=mob_get_head_top(md->class_);
		WBUFW(buf,28)=mob_get_head_mid(md->class_);
		WBUFW(buf,30)=mob_get_hair_color(md->class_);
		WBUFW(buf,32)=mob_get_clothes_color(md->class_);
		if(md->guild_id){
			struct guild *g=guild_search(md->guild_id);
			if(g)
				WBUFW(buf,40)=g->emblem_id;
			WBUFL(buf,36)=md->guild_id;
		}
		WBUFL(buf,44)=md->sc.opt3;
		WBUFB(buf,48)=1;
		WBUFB(buf,49)=mob_get_sex(md->class_);
		WBUFPOS(buf,50,md->bl.x,md->bl.y,md->dir);
		WBUFB(buf,53)=5;
		WBUFB(buf,54)=5;
		WBUFW(buf,56)=((level = status_get_lv(&md->bl))>99)? 99:level;
		WBUFW(buf,58)=0;
#else
		len = 63 + (int)strlen(md->name);
		memset(buf,0,len);

		WBUFW(buf,0)=0x7f9;
		WBUFW(buf,2)=(unsigned short)len;
		WBUFB(buf,4)=0;
		WBUFL(buf,5)=md->bl.id;
		WBUFW(buf,9)=status_get_speed(&md->bl);
		WBUFW(buf,11)=md->sc.opt1;
		WBUFW(buf,13)=md->sc.opt2;
		WBUFL(buf,15)=md->sc.option;
		WBUFW(buf,19)=mob_get_viewclass(md->class_);
		WBUFW(buf,21)=mob_get_hair(md->class_);
		WBUFW(buf,23)=mob_get_weapon(md->class_);
		WBUFW(buf,25)=mob_get_shield(md->class_);
		WBUFW(buf,27)=mob_get_head_bottom(md->class_);
		WBUFW(buf,29)=mob_get_head_top(md->class_);
		WBUFW(buf,31)=mob_get_head_mid(md->class_);
		WBUFW(buf,33)=mob_get_hair_color(md->class_);
		WBUFW(buf,35)=mob_get_clothes_color(md->class_);
		if(md->guild_id){
			struct guild *g=guild_search(md->guild_id);
			if(g)
				WBUFW(buf,43)=g->emblem_id;
			WBUFL(buf,39)=md->guild_id;
		}
		WBUFL(buf,47)=md->sc.opt3;
		WBUFB(buf,51)=1;
		WBUFB(buf,52)=mob_get_sex(md->class_);
		WBUFPOS(buf,53,md->bl.x,md->bl.y,md->dir);
		WBUFB(buf,56)=5;
		WBUFB(buf,57)=5;
		WBUFW(buf,59)=((level = status_get_lv(&md->bl))>99)? 99:level;
		strncpy(WBUFP(buf,63),md->name,24);
#endif
		return len;
	}
#if PACKETVER < 12
	len = packet_db[0x78].len;
	memset(buf,0,len);

	WBUFW(buf,0)=0x78;
	WBUFL(buf,2)=md->bl.id;
	WBUFW(buf,6)=status_get_speed(&md->bl);
	WBUFW(buf,8)=md->sc.opt1;
	WBUFW(buf,10)=md->sc.opt2;
	WBUFW(buf,12)=md->sc.option;
	WBUFW(buf,14)=mob_get_viewclass(md->class_);
	if((md->class_ == 1285 || md->class_ == 1286 || md->class_ == 1287) && md->guild_id){
		struct guild *g=guild_search(md->guild_id);
		if(g)
			WBUFL(buf,22)=g->emblem_id;
		WBUFL(buf,26)=md->guild_id;
	}
	WBUFW(buf,42)=md->sc.opt3;
	WBUFPOS(buf,46,md->bl.x,md->bl.y,md->dir);
	WBUFB(buf,49)=5;
	WBUFB(buf,50)=5;
	WBUFW(buf,52)=((level = status_get_lv(&md->bl))>99)? 99:level;
#elif PACKETVER < 23
	len = packet_db[0x78].len;
	memset(buf,0,len);

	WBUFW(buf,0)=0x78;
	WBUFB(buf,2)=0;
	WBUFL(buf,3)=md->bl.id;
	WBUFW(buf,7)=status_get_speed(&md->bl);
	WBUFW(buf,9)=md->sc.opt1;
	WBUFW(buf,11)=md->sc.opt2;
	WBUFW(buf,13)=md->sc.option;
	WBUFW(buf,15)=mob_get_viewclass(md->class_);
	if((md->class_ == 1285 || md->class_ == 1286 || md->class_ == 1287) && md->guild_id){
		struct guild *g=guild_search(md->guild_id);
		if(g)
			WBUFL(buf,23)=g->emblem_id;
		WBUFL(buf,27)=md->guild_id;
	}
	WBUFW(buf,43)=md->sc.opt3;
	WBUFPOS(buf,47,md->bl.x,md->bl.y,md->dir);
	WBUFB(buf,50)=5;
	WBUFB(buf,51)=5;
	WBUFW(buf,53)=((level = status_get_lv(&md->bl))>99)? 99:level;
#else
	len = 63 + (int)strlen(md->name);
	memset(buf,0,len);

	WBUFW(buf,0)=0x7f9;
	WBUFW(buf,2)=(unsigned short)len;
	WBUFB(buf,4)=5;
	WBUFL(buf,5)=md->bl.id;
	WBUFW(buf,9)=status_get_speed(&md->bl);
	WBUFW(buf,11)=md->sc.opt1;
	WBUFW(buf,13)=md->sc.opt2;
	WBUFL(buf,15)=md->sc.option;
	WBUFW(buf,19)=mob_get_viewclass(md->class_);
	if(md->guild_id){
		struct guild *g=guild_search(md->guild_id);
		if(g)
			WBUFW(buf,43)=g->emblem_id;
		WBUFL(buf,39)=md->guild_id;
	}
	WBUFL(buf,47)=md->sc.opt3;
	WBUFPOS(buf,53,md->bl.x,md->bl.y,md->dir);
	WBUFW(buf,59)=((level = status_get_lv(&md->bl))>99)? 99:level;
	strncpy(WBUFP(buf,63),md->name,24);
#endif
	return len;
}

/*==========================================
 * MOB�\��2
 *------------------------------------------
 */
static int clif_mob007b(struct mob_data *md,unsigned char *buf)
{
	int len, level;

	nullpo_retr(0, md);

	if(mob_is_pcview(md->class_)) {
#if PACKETVER < 4
		len = packet_db[0x7b].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x7b;
		WBUFL(buf,2)=md->bl.id;
		WBUFW(buf,6)=status_get_speed(&md->bl);
		WBUFW(buf,8)=md->sc.opt1;
		WBUFW(buf,10)=md->sc.opt2;
		WBUFW(buf,12)=md->sc.option;
		WBUFW(buf,14)=mob_get_viewclass(md->class_);
		WBUFW(buf,16)=mob_get_hair(md->class_);
		WBUFW(buf,18)=mob_get_weapon(md->class_);
		WBUFW(buf,20)=mob_get_head_bottom(md->class_);
		WBUFL(buf,22)=gettick();
		WBUFW(buf,26)=mob_get_shield(md->class_);
		WBUFW(buf,28)=mob_get_head_top(md->class_);
		WBUFW(buf,30)=mob_get_head_mid(md->class_);
		WBUFW(buf,32)=mob_get_hair_color(md->class_);
		WBUFW(buf,34)=mob_get_clothes_color(md->class_);
		if(md->guild_id){
			struct guild *g=guild_search(md->guild_id);
			if(g)
				WBUFL(buf,42)=g->emblem_id;
			WBUFL(buf,38)=md->guild_id;
		}
		WBUFW(buf,46)=md->sc.opt3;
		WBUFB(buf,48)=1;
		WBUFB(buf,49)=mob_get_sex(md->class_);
		WBUFPOS2(buf,50,md->bl.x,md->bl.y,md->ud.to_x,md->ud.to_y,8,8);
		WBUFB(buf,56)=5;
		WBUFB(buf,57)=5;
		WBUFW(buf,58)=((level = status_get_lv(&md->bl))>99)? 99:level;
#elif PACKETVER < 7
		len = packet_db[0x1da].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x1da;
		WBUFL(buf,2)=md->bl.id;
		WBUFW(buf,6)=status_get_speed(&md->bl);
		WBUFW(buf,8)=md->sc.opt1;
		WBUFW(buf,10)=md->sc.opt2;
		WBUFW(buf,12)=md->sc.option;
		WBUFW(buf,14)=mob_get_viewclass(md->class_);
		WBUFW(buf,16)=mob_get_hair(md->class_);
		WBUFW(buf,18)=mob_get_weapon(md->class_);
		WBUFW(buf,20)=mob_get_shield(md->class_);
		WBUFW(buf,22)=mob_get_head_bottom(md->class_);
		WBUFL(buf,24)=gettick();
		WBUFW(buf,28)=mob_get_head_top(md->class_);
		WBUFW(buf,30)=mob_get_head_mid(md->class_);
		WBUFW(buf,32)=mob_get_hair_color(md->class_);
		WBUFW(buf,34)=mob_get_clothes_color(md->class_);
		if(md->guild_id){
			struct guild *g=guild_search(md->guild_id);
			if(g)
				WBUFL(buf,42)=g->emblem_id;
			WBUFL(buf,38)=md->guild_id;
		}
		WBUFW(buf,46)=md->sc.opt3;
		WBUFB(buf,48)=1;
		WBUFB(buf,49)=mob_get_sex(md->class_);
		WBUFPOS2(buf,50,md->bl.x,md->bl.y,md->ud.to_x,md->ud.to_y,8,8);
		WBUFB(buf,56)=5;
		WBUFB(buf,57)=5;
		WBUFW(buf,58)=((level = status_get_lv(&md->bl))>99)? 99:level;
#elif PACKETVER < 12
		len = packet_db[0x22c].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x22c;
		WBUFL(buf,2)=md->bl.id;
		WBUFW(buf,6)=md->speed;
		WBUFW(buf,8)=md->sc.opt1;
		WBUFW(buf,10)=md->sc.opt2;
		WBUFL(buf,12)=md->sc.option;
		WBUFW(buf,16)=mob_get_viewclass(md->class_);
		WBUFW(buf,18)=mob_get_hair(md->class_);
		WBUFW(buf,20)=mob_get_weapon(md->class_);
		WBUFW(buf,22)=mob_get_shield(md->class_);
		WBUFW(buf,24)=mob_get_head_bottom(md->class_);
		WBUFL(buf,26)=gettick();
		WBUFW(buf,30)=mob_get_head_top(md->class_);
		WBUFW(buf,32)=mob_get_head_mid(md->class_);
		WBUFW(buf,34)=mob_get_hair_color(md->class_);
		WBUFW(buf,36)=mob_get_clothes_color(md->class_);
		if(md->guild_id){
			struct guild *g=guild_search(md->guild_id);
			if(g)
				WBUFL(buf,44)=g->emblem_id;
			WBUFL(buf,40)=md->guild_id;
		}
		WBUFL(buf,48)=md->sc.opt3;
		WBUFB(buf,52)=1;
		WBUFB(buf,53)=mob_get_sex(md->class_);
		WBUFPOS2(buf,54,md->bl.x,md->bl.y,md->ud.to_x,md->ud.to_y,8,8);
		WBUFB(buf,60)=0;
		WBUFB(buf,61)=0;
		WBUFW(buf,62)=((level = status_get_lv(&md->bl))>99)? 99:level;
#elif PACKETVER < 14
		len = packet_db[0x22c].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x22c;
		WBUFB(buf,2)=0;
		WBUFL(buf,3)=md->bl.id;
		WBUFW(buf,7)=md->speed;
		WBUFW(buf,9)=md->sc.opt1;
		WBUFW(buf,11)=md->sc.opt2;
		WBUFL(buf,13)=md->sc.option;
		WBUFW(buf,17)=mob_get_viewclass(md->class_);
		WBUFW(buf,19)=mob_get_hair(md->class_);
		WBUFW(buf,21)=mob_get_weapon(md->class_);
		WBUFW(buf,23)=mob_get_shield(md->class_);
		WBUFW(buf,25)=mob_get_head_bottom(md->class_);
		WBUFL(buf,27)=gettick();
		WBUFW(buf,31)=mob_get_head_top(md->class_);
		WBUFW(buf,33)=mob_get_head_mid(md->class_);
		WBUFW(buf,35)=mob_get_hair_color(md->class_);
		WBUFW(buf,37)=mob_get_clothes_color(md->class_);
		if(md->guild_id){
			struct guild *g=guild_search(md->guild_id);
			if(g)
				WBUFL(buf,45)=g->emblem_id;
			WBUFL(buf,41)=md->guild_id;
		}
		WBUFL(buf,49)=md->sc.opt3;
		WBUFB(buf,53)=1;
		WBUFB(buf,54)=mob_get_sex(md->class_);
		WBUFPOS2(buf,55,md->bl.x,md->bl.y,md->ud.to_x,md->ud.to_y,8,8);
		WBUFB(buf,61)=0;
		WBUFB(buf,62)=0;
		WBUFW(buf,63)=((level = status_get_lv(&md->bl))>99)? 99:level;
#elif PACKETVER < 23
		len = packet_db[0x2ec].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x2ec;
		WBUFB(buf,2)=0;
		WBUFL(buf,3)=md->bl.id;
		WBUFW(buf,7)=md->speed;
		WBUFW(buf,9)=md->sc.opt1;
		WBUFW(buf,11)=md->sc.opt2;
		WBUFL(buf,13)=md->sc.option;
		WBUFW(buf,17)=mob_get_viewclass(md->class_);
		WBUFW(buf,19)=mob_get_hair(md->class_);
		WBUFW(buf,21)=mob_get_weapon(md->class_);
		WBUFW(buf,23)=mob_get_shield(md->class_);
		WBUFW(buf,25)=mob_get_head_bottom(md->class_);
		WBUFL(buf,27)=gettick();
		WBUFW(buf,31)=mob_get_head_top(md->class_);
		WBUFW(buf,33)=mob_get_head_mid(md->class_);
		WBUFW(buf,35)=mob_get_hair_color(md->class_);
		WBUFW(buf,37)=mob_get_clothes_color(md->class_);
		if(md->guild_id){
			struct guild *g=guild_search(md->guild_id);
			if(g)
				WBUFL(buf,45)=g->emblem_id;
			WBUFL(buf,41)=md->guild_id;
		}
		WBUFL(buf,49)=md->sc.opt3;
		WBUFB(buf,53)=1;
		WBUFB(buf,54)=mob_get_sex(md->class_);
		WBUFPOS2(buf,55,md->bl.x,md->bl.y,md->ud.to_x,md->ud.to_y,8,8);
		WBUFB(buf,61)=0;
		WBUFB(buf,62)=0;
		WBUFW(buf,63)=((level = status_get_lv(&md->bl))>99)? 99:level;
		WBUFW(buf,65)=0;
#else
		len = 69 + (int)strlen(md->name);
		memset(buf,0,len);

		WBUFW(buf,0)=0x7f7;
		WBUFW(buf,2)=(unsigned short)len;
		WBUFB(buf,4)=0;
		WBUFL(buf,5)=md->bl.id;
		WBUFW(buf,9)=md->speed;
		WBUFW(buf,11)=md->sc.opt1;
		WBUFW(buf,13)=md->sc.opt2;
		WBUFL(buf,15)=md->sc.option;
		WBUFW(buf,19)=mob_get_viewclass(md->class_);
		WBUFW(buf,21)=mob_get_hair(md->class_);
		WBUFW(buf,23)=mob_get_weapon(md->class_);
		WBUFW(buf,25)=mob_get_shield(md->class_);
		WBUFW(buf,27)=mob_get_head_bottom(md->class_);
		WBUFL(buf,29)=gettick();
		WBUFW(buf,33)=mob_get_head_top(md->class_);
		WBUFW(buf,35)=mob_get_head_mid(md->class_);
		WBUFW(buf,37)=mob_get_hair_color(md->class_);
		WBUFW(buf,39)=mob_get_clothes_color(md->class_);
		if(md->guild_id){
			struct guild *g=guild_search(md->guild_id);
			if(g)
				WBUFW(buf,47)=g->emblem_id;
			WBUFL(buf,43)=md->guild_id;
		}
		WBUFL(buf,51)=md->sc.opt3;
		WBUFB(buf,55)=1;
		WBUFB(buf,56)=mob_get_sex(md->class_);
		WBUFPOS2(buf,57,md->bl.x,md->bl.y,md->ud.to_x,md->ud.to_y,8,8);
		WBUFB(buf,63)=5;
		WBUFB(buf,64)=5;
		WBUFW(buf,65)=((level = status_get_lv(&md->bl))>99)? 99:level;
		strncpy(WBUFP(buf,69),md->name,24);
#endif
		return len;
	}

#if PACKETVER < 23
	len = packet_db[0x7b].len;
	memset(buf,0,len);

	WBUFW(buf,0)=0x7b;
	WBUFL(buf,2)=md->bl.id;
	WBUFW(buf,6)=status_get_speed(&md->bl);
	WBUFW(buf,8)=md->sc.opt1;
	WBUFW(buf,10)=md->sc.opt2;
	WBUFW(buf,12)=md->sc.option;
	WBUFW(buf,14)=mob_get_viewclass(md->class_);
	WBUFL(buf,22)=gettick();
	if((md->class_ == 1285 || md->class_ == 1286 || md->class_ == 1287) && md->guild_id){
		struct guild *g=guild_search(md->guild_id);
		if(g)
			WBUFL(buf,26)=g->emblem_id;
		WBUFL(buf,30)=md->guild_id;
	}
	WBUFW(buf,46)=md->sc.opt3;
	WBUFPOS2(buf,50,md->bl.x,md->bl.y,md->ud.to_x,md->ud.to_y,8,8);
	WBUFB(buf,56)=5;
	WBUFB(buf,57)=5;
	WBUFW(buf,58)=((level = status_get_lv(&md->bl))>99)? 99:level;
#else
	len = 69 + (int)strlen(md->name);
	memset(buf,0,len);

	WBUFW(buf,0)=0x7f7;
	WBUFW(buf,2)=(unsigned short)len;
	WBUFB(buf,4)=5;
	WBUFL(buf,5)=md->bl.id;
	WBUFW(buf,9)=md->speed;
	WBUFW(buf,11)=md->sc.opt1;
	WBUFW(buf,13)=md->sc.opt2;
	WBUFL(buf,15)=md->sc.option;
	WBUFW(buf,19)=mob_get_viewclass(md->class_);
	WBUFL(buf,29)=gettick();
	if(md->guild_id){
		struct guild *g=guild_search(md->guild_id);
		if(g)
			WBUFW(buf,47)=g->emblem_id;
		WBUFL(buf,43)=md->guild_id;
	}
	WBUFL(buf,51)=md->sc.opt3;
	WBUFPOS2(buf,57,md->bl.x,md->bl.y,md->ud.to_x,md->ud.to_y,8,8);
	WBUFW(buf,65)=((level = status_get_lv(&md->bl))>99)? 99:level;
	strncpy(WBUFP(buf,69),md->name,24);
#endif
	return len;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_npc0078(struct npc_data *nd,unsigned char *buf)
{
	struct guild *g;
	int len;

	nullpo_retr(0, nd);

#if PACKETVER < 12
	len = packet_db[0x78].len;
	memset(buf,0,len);

	WBUFW(buf,0)=0x78;
	WBUFL(buf,2)=nd->bl.id;
	WBUFW(buf,6)=nd->speed;
	WBUFW(buf,12)=nd->option;
	WBUFW(buf,14)=nd->class_;
	if( nd->subtype != WARP &&
	    nd->class_ == WARP_DEBUG_CLASS &&
	    nd->u.scr.guild_id > 0 &&
	    (g = guild_search(nd->u.scr.guild_id)) )
	{
		WBUFL(buf,22)=g->emblem_id;
		WBUFL(buf,26)=g->guild_id;
	}
	WBUFPOS(buf,46,nd->bl.x,nd->bl.y,nd->dir);
	WBUFB(buf,49)=5;
	WBUFB(buf,50)=5;
#elif PACKETVER < 23
	len = packet_db[0x78].len;
	memset(buf,0,len);

	WBUFW(buf,0)=0x78;
	WBUFB(buf,2)=0;
	WBUFL(buf,3)=nd->bl.id;
	WBUFW(buf,7)=nd->speed;
	WBUFW(buf,13)=nd->option;
	WBUFW(buf,15)=nd->class_;
	if( nd->subtype != WARP &&
	    nd->class_ == WARP_DEBUG_CLASS &&
	    nd->u.scr.guild_id > 0 &&
	    (g = guild_search(nd->u.scr.guild_id)) )
	{
		WBUFL(buf,23)=g->emblem_id;
		WBUFL(buf,27)=g->guild_id;
	}
	WBUFPOS(buf,47,nd->bl.x,nd->bl.y,nd->dir);
	WBUFB(buf,50)=5;
	WBUFB(buf,51)=5;
#else
	len = 63 + (int)strlen(nd->name);
	memset(buf,0,len);

	WBUFW(buf,0)=0x7f9;
	WBUFW(buf,2)=(unsigned short)len;
	WBUFB(buf,4)=6;
	WBUFL(buf,5)=nd->bl.id;
	WBUFW(buf,9)=nd->speed;
	WBUFL(buf,15)=nd->option;
	WBUFW(buf,19)=nd->class_;
	if( nd->subtype != WARP &&
	    nd->class_ == WARP_DEBUG_CLASS &&
	    nd->u.scr.guild_id > 0 &&
	    (g = guild_search(nd->u.scr.guild_id)) )
	{
		WBUFW(buf,27)=g->emblem_id;
		WBUFL(buf,31)=g->guild_id;
	}
	WBUFPOS(buf,53,nd->bl.x,nd->bl.y,nd->dir);
	WBUFW(buf,59)=1;
	strncpy(WBUFP(buf,63),nd->name,24);
#endif
	return len;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_pet0078(struct pet_data *pd,unsigned char *buf)
{
	int len, view, level;

	nullpo_retr(0, pd);

	if(mob_is_pcview(pd->class_)) {
#if PACKETVER < 4
		len = packet_db[0x78].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x78;
		WBUFL(buf,2)=pd->bl.id;
		WBUFW(buf,6)=pd->speed;
		WBUFW(buf,12)=mob_db[pd->class_].option;
		WBUFW(buf,14)=mob_get_viewclass(pd->class_);
		WBUFW(buf,16)=mob_get_hair(pd->class_);
		WBUFW(buf,18)=mob_get_weapon(pd->class_);
		WBUFW(buf,20)=mob_get_head_bottom(pd->class_);
		WBUFW(buf,22)=mob_get_shield(pd->class_);
		WBUFW(buf,24)=mob_get_head_top(pd->class_);
		WBUFW(buf,26)=mob_get_head_mid(pd->class_);
		WBUFW(buf,28)=mob_get_hair_color(pd->class_);
		WBUFW(buf,30)=mob_get_clothes_color(pd->class_);
		WBUFB(buf,45)=mob_get_sex(pd->class_);
		WBUFPOS(buf,46,pd->bl.x,pd->bl.y,pd->dir);
		WBUFB(buf,49)=0;
		WBUFB(buf,50)=0;
		WBUFW(buf,52)=((level = status_get_lv(&pd->bl))>99)? 99:level;
#elif PACKETVER < 7
		len = packet_db[0x1d8].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x1d8;
		WBUFL(buf,2)=pd->bl.id;
		WBUFW(buf,6)=pd->speed;
		WBUFW(buf,12)=mob_db[pd->class_].option;
		WBUFW(buf,14)=mob_get_viewclass(pd->class_);
		WBUFW(buf,16)=mob_get_hair(pd->class_);
		WBUFW(buf,18)=mob_get_weapon(pd->class_);
		WBUFW(buf,20)=mob_get_shield(pd->class_);
		WBUFW(buf,22)=mob_get_head_bottom(pd->class_);
		WBUFW(buf,24)=mob_get_head_top(pd->class_);
		WBUFW(buf,26)=mob_get_head_mid(pd->class_);
		WBUFW(buf,28)=mob_get_hair_color(pd->class_);
		WBUFW(buf,30)=mob_get_clothes_color(pd->class_);
		WBUFB(buf,45)=mob_get_sex(pd->class_);
		WBUFPOS(buf,46,pd->bl.x,pd->bl.y,pd->dir);
		WBUFB(buf,49)=0;
		WBUFB(buf,50)=0;
		WBUFW(buf,52)=((level = status_get_lv(&pd->bl))>99)? 99:level;
#elif PACKETVER < 14
		len = packet_db[0x22a].len);
		memset(buf,0,len);

		WBUFW(buf,0)=0x22a;
		WBUFL(buf,2)=pd->bl.id;
		WBUFW(buf,6)=pd->speed;
		WBUFL(buf,12)=mob_db[pd->class_].option;
		WBUFW(buf,16)=mob_get_viewclass(pd->class_);
		WBUFW(buf,18)=mob_get_hair(pd->class_);
		WBUFW(buf,20)=mob_get_weapon(pd->class_);
		WBUFW(buf,22)=mob_get_shield(pd->class_);
		WBUFW(buf,24)=mob_get_head_bottom(pd->class_);
		WBUFW(buf,26)=mob_get_head_top(pd->class_);
		WBUFW(buf,28)=mob_get_head_mid(pd->class_);
		WBUFW(buf,30)=mob_get_hair_color(pd->class_);
		WBUFW(buf,32)=mob_get_clothes_color(pd->class_);
		WBUFB(buf,49)=mob_get_sex(pd->class_);
		WBUFPOS(buf,50,pd->bl.x,pd->bl.y,pd->dir);
		WBUFB(buf,53)=0;
		WBUFB(buf,54)=0;
		WBUFW(buf,56)=((level = status_get_lv(&pd->bl))>99)? 99:level;
#elif PACKETVER < 23
		len = packet_db[0x2ee].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x2ee;
		WBUFL(buf,2)=pd->bl.id;
		WBUFW(buf,6)=pd->speed;
		WBUFL(buf,12)=mob_db[pd->class_].option;
		WBUFW(buf,16)=mob_get_viewclass(pd->class_);
		WBUFW(buf,18)=mob_get_hair(pd->class_);
		WBUFW(buf,20)=mob_get_weapon(pd->class_);
		WBUFW(buf,22)=mob_get_shield(pd->class_);
		WBUFW(buf,24)=mob_get_head_bottom(pd->class_);
		WBUFW(buf,26)=mob_get_head_top(pd->class_);
		WBUFW(buf,28)=mob_get_head_mid(pd->class_);
		WBUFW(buf,30)=mob_get_hair_color(pd->class_);
		WBUFW(buf,32)=mob_get_clothes_color(pd->class_);
		WBUFB(buf,49)=mob_get_sex(pd->class_);
		WBUFPOS(buf,50,pd->bl.x,pd->bl.y,pd->dir);
		WBUFB(buf,53)=0;
		WBUFB(buf,54)=0;
		WBUFW(buf,56)=((level = status_get_lv(&pd->bl))>99)? 99:level;
		WBUFW(buf,58)=0;
#else
		len = 63 + (int)strlen(pd->name);
		memset(buf,0,len);

		WBUFW(buf,0)=0x7f9;
		WBUFW(buf,2)=(unsigned short)len;
		WBUFB(buf,4)=0;
		WBUFL(buf,5)=pd->bl.id;
		WBUFW(buf,9)=pd->speed;
		WBUFL(buf,15)=mob_db[pd->class_].option;
		WBUFW(buf,19)=mob_get_viewclass(pd->class_);
		WBUFW(buf,21)=mob_get_hair(pd->class_);
		WBUFW(buf,23)=mob_get_weapon(pd->class_);
		WBUFW(buf,25)=mob_get_shield(pd->class_);
		WBUFW(buf,27)=mob_get_head_bottom(pd->class_);
		WBUFW(buf,29)=mob_get_head_top(pd->class_);
		WBUFW(buf,31)=mob_get_head_mid(pd->class_);
		WBUFW(buf,33)=mob_get_hair_color(pd->class_);
		WBUFW(buf,35)=mob_get_clothes_color(pd->class_);
		WBUFB(buf,52)=mob_get_sex(pd->class_);
		WBUFPOS(buf,53,pd->bl.x,pd->bl.y,pd->dir);
		WBUFB(buf,56)=5;
		WBUFB(buf,57)=5;
		WBUFW(buf,59)=((level = status_get_lv(&pd->bl))>99)? 99:level;
		strncpy(WBUFP(buf,63),pd->name,24);
#endif
		return len;
	}
#if PACKETVER < 12
	len = packet_db[0x78].len;
	memset(buf,0,len);

	WBUFW(buf,0)=0x78;
	WBUFL(buf,2)=pd->bl.id;
	WBUFW(buf,6)=pd->speed;
	WBUFW(buf,14)=mob_get_viewclass(pd->class_);
	WBUFW(buf,16)=battle_config.pet0078_hair_id;
	if((view = itemdb_viewid(pd->equip)) > 0)
		WBUFW(buf,20)=view;
	else
		WBUFW(buf,20)=pd->equip;
	WBUFPOS(buf,46,pd->bl.x,pd->bl.y,pd->dir);
	WBUFB(buf,49)=0;
	WBUFB(buf,50)=0;
	WBUFW(buf,52)=((level = status_get_lv(&pd->bl))>99)? 99:level;
#elif PACKETVER < 23
	len = packet_db[0x78].len;
	memset(buf,0,len);

	WBUFW(buf,0)=0x78;
	WBUFB(buf,2)=0;
	WBUFL(buf,3)=pd->bl.id;
	WBUFW(buf,7)=pd->speed;
	WBUFW(buf,15)=mob_get_viewclass(pd->class_);
	WBUFW(buf,17)=battle_config.pet0078_hair_id;
	if((view = itemdb_viewid(pd->equip)) > 0)
		WBUFW(buf,21)=view;
	else
		WBUFW(buf,21)=pd->equip;
	WBUFPOS(buf,47,pd->bl.x,pd->bl.y,pd->dir);
	WBUFB(buf,50)=0;
	WBUFB(buf,51)=0;
	WBUFW(buf,53)=((level = status_get_lv(&pd->bl))>99)? 99:level;
#else
	len = 63 + (int)strlen(pd->name);
	memset(buf,0,len);

	WBUFW(buf,0)=0x7f9;
	WBUFW(buf,2)=(unsigned short)len;
	WBUFB(buf,4)=7;
	WBUFL(buf,5)=pd->bl.id;
	WBUFW(buf,9)=pd->speed;
	WBUFW(buf,19)=mob_get_viewclass(pd->class_);
	WBUFW(buf,21)=100;	// ���ׂ�����Œ�
	if((view = itemdb_viewid(pd->equip)) > 0)
		WBUFW(buf,27)=view;
	else
		WBUFW(buf,27)=pd->equip;
	WBUFPOS(buf,53,pd->bl.x,pd->bl.y,pd->dir);
	WBUFW(buf,59)=((level = status_get_lv(&pd->bl))>99)? 99:level;
	strncpy(WBUFP(buf,63),pd->name,24);
#endif
	return len;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_pet007b(struct pet_data *pd,unsigned char *buf)
{
	int len, view, level;

	nullpo_retr(0, pd);

	if(mob_is_pcview(pd->class_)) {
#if PACKETVER < 4
		len = packet_db[0x7b].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x7b;
		WBUFL(buf,2)=pd->bl.id;
		WBUFW(buf,6)=pd->speed;
		WBUFW(buf,12)=mob_db[pd->class_].option;
		WBUFW(buf,14)=mob_get_viewclass(pd->class_);
		WBUFW(buf,16)=mob_get_hair(pd->class_);
		WBUFW(buf,18)=mob_get_weapon(pd->class_);
		WBUFW(buf,20)=mob_get_head_bottom(pd->class_);
		WBUFL(buf,22)=gettick();
		WBUFW(buf,26)=mob_get_shield(pd->class_);
		WBUFW(buf,28)=mob_get_head_top(pd->class_);
		WBUFW(buf,30)=mob_get_head_mid(pd->class_);
		WBUFW(buf,32)=mob_get_hair_color(pd->class_);
		WBUFW(buf,34)=mob_get_clothes_color(pd->class_);
		WBUFB(buf,49)=mob_get_sex(pd->class_);
		WBUFPOS2(buf,50,pd->bl.x,pd->bl.y,pd->ud.to_x,pd->ud.to_y,8,8);
		WBUFB(buf,56)=0;
		WBUFB(buf,57)=0;
		WBUFW(buf,58)=((level = status_get_lv(&pd->bl))>99)? 99:level;
#elif PACKETVER < 7
		len = packet_db[0x1da].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x1da;
		WBUFL(buf,2)=pd->bl.id;
		WBUFW(buf,6)=pd->speed;
		WBUFW(buf,12)=mob_db[pd->class_].option;
		WBUFW(buf,14)=mob_get_viewclass(pd->class_);
		WBUFW(buf,16)=mob_get_hair(pd->class_);
		WBUFW(buf,18)=mob_get_weapon(pd->class_);
		WBUFW(buf,20)=mob_get_shield(pd->class_);
		WBUFW(buf,22)=mob_get_head_bottom(pd->class_);
		WBUFL(buf,24)=gettick();
		WBUFW(buf,28)=mob_get_head_top(pd->class_);
		WBUFW(buf,30)=mob_get_head_mid(pd->class_);
		WBUFW(buf,32)=mob_get_hair_color(pd->class_);
		WBUFW(buf,34)=mob_get_clothes_color(pd->class_);
		WBUFB(buf,49)=mob_get_sex(pd->class_);
		WBUFPOS2(buf,50,pd->bl.x,pd->bl.y,pd->ud.to_x,pd->ud.to_y,8,8);
		WBUFB(buf,56)=0;
		WBUFB(buf,57)=0;
		WBUFW(buf,58)=((level = status_get_lv(&pd->bl))>99)? 99:level;
#elif PACKETVER < 12
		len = packet_db[0x22c].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x22c;
		WBUFL(buf,2)=pd->bl.id;
		WBUFW(buf,6)=pd->speed;
		WBUFL(buf,12)=mob_db[pd->class_].option;
		WBUFW(buf,16)=mob_get_viewclass(pd->class_);
		WBUFW(buf,18)=mob_get_hair(pd->class_);
		WBUFW(buf,20)=mob_get_weapon(pd->class_);
		WBUFW(buf,22)=mob_get_shield(pd->class_);
		WBUFW(buf,24)=mob_get_head_bottom(pd->class_);
		WBUFL(buf,26)=gettick();
		WBUFW(buf,30)=mob_get_head_top(pd->class_);
		WBUFW(buf,32)=mob_get_head_mid(pd->class_);
		WBUFW(buf,34)=mob_get_hair_color(pd->class_);
		WBUFW(buf,36)=mob_get_clothes_color(pd->class_);
		WBUFB(buf,53)=mob_get_sex(pd->class_);
		WBUFPOS2(buf,54,pd->bl.x,pd->bl.y,pd->ud.to_x,pd->ud.to_y,8,8);
		WBUFB(buf,60)=0;
		WBUFB(buf,61)=0;
		WBUFW(buf,62)=((level = status_get_lv(&pd->bl))>99)? 99:level;
#elif PACKETVER < 14
		len = packet_db[0x22c].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x22c;
		WBUFB(buf,2)=0;
		WBUFL(buf,3)=pd->bl.id;
		WBUFW(buf,7)=pd->speed;
		WBUFL(buf,13)=mob_db[pd->class_].option;
		WBUFW(buf,17)=mob_get_viewclass(pd->class_);
		WBUFW(buf,19)=mob_get_hair(pd->class_);
		WBUFW(buf,21)=mob_get_weapon(pd->class_);
		WBUFW(buf,23)=mob_get_shield(pd->class_);
		WBUFW(buf,25)=mob_get_head_bottom(pd->class_);
		WBUFL(buf,27)=gettick();
		WBUFW(buf,31)=mob_get_head_top(pd->class_);
		WBUFW(buf,33)=mob_get_head_mid(pd->class_);
		WBUFW(buf,35)=mob_get_hair_color(pd->class_);
		WBUFW(buf,37)=mob_get_clothes_color(pd->class_);
		WBUFB(buf,54)=mob_get_sex(pd->class_);
		WBUFPOS2(buf,55,pd->bl.x,pd->bl.y,pd->ud.to_x,pd->ud.to_y,8,8);
		WBUFB(buf,61)=0;
		WBUFB(buf,62)=0;
		WBUFW(buf,63)=((level = status_get_lv(&pd->bl))>99)? 99:level;
#elif PACKETVER < 23
		len = packet_db[0x2ec].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x2ec;
		WBUFB(buf,2)=0;
		WBUFL(buf,3)=pd->bl.id;
		WBUFW(buf,7)=pd->speed;
		WBUFL(buf,13)=mob_db[pd->class_].option;
		WBUFW(buf,17)=mob_get_viewclass(pd->class_);
		WBUFW(buf,19)=mob_get_hair(pd->class_);
		WBUFW(buf,21)=mob_get_weapon(pd->class_);
		WBUFW(buf,23)=mob_get_shield(pd->class_);
		WBUFW(buf,25)=mob_get_head_bottom(pd->class_);
		WBUFL(buf,27)=gettick();
		WBUFW(buf,31)=mob_get_head_top(pd->class_);
		WBUFW(buf,33)=mob_get_head_mid(pd->class_);
		WBUFW(buf,35)=mob_get_hair_color(pd->class_);
		WBUFW(buf,37)=mob_get_clothes_color(pd->class_);
		WBUFB(buf,54)=mob_get_sex(pd->class_);
		WBUFPOS2(buf,55,pd->bl.x,pd->bl.y,pd->ud.to_x,pd->ud.to_y,8,8);
		WBUFB(buf,61)=0;
		WBUFB(buf,62)=0;
		WBUFW(buf,63)=((level = status_get_lv(&pd->bl))>99)? 99:level;
		WBUFW(buf,65)=0;
#else
		len = 69 + (int)strlen(pd->name);
		memset(buf,0,len);

		WBUFW(buf,0)=0x7f7;
		WBUFW(buf,2)=(unsigned short)len;
		WBUFB(buf,4)=0;
		WBUFL(buf,5)=pd->bl.id;
		WBUFW(buf,9)=pd->speed;
		WBUFL(buf,15)=mob_db[pd->class_].option;
		WBUFW(buf,19)=mob_get_viewclass(pd->class_);
		WBUFW(buf,21)=mob_get_hair(pd->class_);
		WBUFW(buf,23)=mob_get_weapon(pd->class_);
		WBUFW(buf,25)=mob_get_shield(pd->class_);
		WBUFW(buf,27)=mob_get_head_bottom(pd->class_);
		WBUFL(buf,29)=gettick();
		WBUFW(buf,33)=mob_get_head_top(pd->class_);
		WBUFW(buf,35)=mob_get_head_mid(pd->class_);
		WBUFW(buf,37)=mob_get_hair_color(pd->class_);
		WBUFW(buf,39)=mob_get_clothes_color(pd->class_);
		WBUFB(buf,56)=mob_get_sex(pd->class_);
		WBUFPOS2(buf,57,pd->bl.x,pd->bl.y,pd->ud.to_x,pd->ud.to_y,8,8);
		WBUFB(buf,63)=5;
		WBUFB(buf,64)=5;
		WBUFW(buf,65)=((level = status_get_lv(&pd->bl))>99)? 99:level;
		strncpy(WBUFP(buf,69),pd->name,24);
#endif
		return len;
	}
#if PACKETVER < 23
	len = packet_db[0x7b].len;
	memset(buf,0,len);

	WBUFW(buf,0)=0x7b;
	WBUFL(buf,2)=pd->bl.id;
	WBUFW(buf,6)=pd->speed;
	WBUFW(buf,14)=mob_get_viewclass(pd->class_);
	WBUFW(buf,16)=battle_config.pet0078_hair_id;
	if((view = itemdb_viewid(pd->equip)) > 0)
		WBUFW(buf,20)=view;
	else
		WBUFW(buf,20)=pd->equip;
	WBUFL(buf,22)=gettick();
	WBUFPOS2(buf,50,pd->bl.x,pd->bl.y,pd->ud.to_x,pd->ud.to_y,8,8);
	WBUFB(buf,56)=0;
	WBUFB(buf,57)=0;
	WBUFW(buf,58)=((level = status_get_lv(&pd->bl))>99)? 99:level;
#else
	len = 69 + (int)strlen(pd->name);
	memset(buf,0,len);

	WBUFW(buf,0)=0x7f7;
	WBUFW(buf,2)=(unsigned short)len;
	WBUFB(buf,4)=7;
	WBUFL(buf,5)=pd->bl.id;
	WBUFW(buf,9)=pd->speed;
	WBUFW(buf,19)=mob_get_viewclass(pd->class_);
	WBUFW(buf,21)=100;	// ���ׂ�����ł͌Œ�
	if((view = itemdb_viewid(pd->equip)) > 0)
		WBUFW(buf,27)=view;
	else
		WBUFW(buf,27)=pd->equip;
	WBUFL(buf,29)=gettick();
	WBUFPOS2(buf,57,pd->bl.x,pd->bl.y,pd->ud.to_x,pd->ud.to_y,8,8);
	WBUFW(buf,65)=((level = status_get_lv(&pd->bl))>99)? 99:level;
	strncpy(WBUFP(buf,69),pd->name,24);
#endif
	return len;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_hom0078(struct homun_data *hd,unsigned char *buf)
{
	int len, level;

	nullpo_retr(0, hd);

#if PACKETVER < 12
	len = packet_db[0x78].len;
	memset(buf,0,len);

	WBUFW(buf,0) =0x78;
	WBUFL(buf,2) =hd->bl.id;
	WBUFW(buf,6) =hd->speed;
	WBUFW(buf,8) =hd->sc.opt1;
	WBUFW(buf,10)=hd->sc.opt2;
	WBUFW(buf,12)=hd->sc.option;
	WBUFW(buf,14)=hd->view_class;
	WBUFW(buf,16)=battle_config.pet0078_hair_id;
	WBUFW(buf,20)=0;
	WBUFW(buf,42)=hd->sc.opt3;
	WBUFPOS(buf,46,hd->bl.x,hd->bl.y,hd->dir);
	WBUFB(buf,49)=0;
	WBUFB(buf,50)=0;
	WBUFW(buf,52)=((level = status_get_lv(&hd->bl))>99)? 99:level;
#elif PACKETVER < 23
	len = packet_db[0x78].len;
	memset(buf,0,len);

	WBUFW(buf,0) =0x78;
	WBUFB(buf,2) =0;
	WBUFL(buf,3) =hd->bl.id;
	WBUFW(buf,7) =hd->speed;
	WBUFW(buf,9) =hd->sc.opt1;
	WBUFW(buf,11)=hd->sc.opt2;
	WBUFW(buf,13)=hd->sc.option;
	WBUFW(buf,15)=hd->view_class;
	WBUFW(buf,17)=battle_config.pet0078_hair_id;
	WBUFW(buf,21)=0;
	WBUFW(buf,43)=hd->sc.opt3;
	WBUFPOS(buf,47,hd->bl.x,hd->bl.y,hd->dir);
	WBUFB(buf,50)=0;
	WBUFB(buf,51)=0;
	WBUFW(buf,53)=((level = status_get_lv(&hd->bl))>99)? 99:level;
#else
	len = 63 + (int)strlen(hd->status.name);
	memset(buf,0,len);

	WBUFW(buf,0)=0x7f9;
	WBUFW(buf,2)=(unsigned short)len;
	WBUFB(buf,4)=8;
	WBUFL(buf,5)=hd->bl.id;
	WBUFW(buf,9)=hd->speed;
	WBUFW(buf,11)=hd->sc.opt1;
	WBUFW(buf,13)=hd->sc.opt2;
	WBUFL(buf,15)=hd->sc.option;
	WBUFW(buf,19)=hd->view_class;
	WBUFW(buf,21)=100;
	WBUFL(buf,47)=hd->sc.opt3;
	WBUFPOS(buf,53,hd->bl.x,hd->bl.y,hd->dir);
	WBUFW(buf,59)=((level = status_get_lv(&hd->bl))>99)? 99:level;
	strncpy(WBUFP(buf,63),hd->status.name,24);
#endif
	return len;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_hom007b(struct homun_data *hd,unsigned char *buf)
{
	int len, view, level;

	nullpo_retr(0, hd);

#if PACKETVER < 23
	len = packet_db[0x7b].len;
	memset(buf,0,len);

	WBUFW(buf,0) =0x7b;
	WBUFL(buf,2) =hd->bl.id;
	WBUFW(buf,6) =hd->speed;
	WBUFW(buf,8) =hd->sc.opt1;
	WBUFW(buf,10)=hd->sc.opt2;
	WBUFW(buf,12)=hd->sc.option;
	WBUFW(buf,14)=hd->view_class;
	WBUFW(buf,16)=battle_config.pet0078_hair_id;
	if((view = itemdb_viewid(hd->status.equip)) > 0)
		WBUFW(buf,20)=view;
	else
		WBUFW(buf,20)=hd->status.equip;
	WBUFL(buf,22)=gettick();
	WBUFW(buf,46)=hd->sc.opt3;
	WBUFPOS2(buf,50,hd->bl.x,hd->bl.y,hd->ud.to_x,hd->ud.to_y,8,8);
	WBUFB(buf,56)=0;
	WBUFB(buf,57)=0;
	WBUFW(buf,58)=((level = status_get_lv(&hd->bl))>99)? 99:level;
#else
	len = 69 + (int)strlen(hd->status.name);
	memset(buf,0,len);

	WBUFW(buf,0) =0x7f7;
	WBUFW(buf,2) =(unsigned short)len;
	WBUFB(buf,4) =8;
	WBUFL(buf,5) =hd->bl.id;
	WBUFW(buf,9) =hd->speed;
	WBUFW(buf,11)=hd->sc.opt1;
	WBUFW(buf,13)=hd->sc.opt2;
	WBUFL(buf,15)=hd->sc.option;
	WBUFW(buf,19)=hd->view_class;
	WBUFW(buf,21)=100;
	if((view = itemdb_viewid(hd->status.equip)) > 0)
		WBUFW(buf,23)=view;
	else
		WBUFW(buf,23)=hd->status.equip;
	WBUFL(buf,29)=gettick();
	WBUFL(buf,51)=hd->sc.opt3;
	WBUFPOS2(buf,57,hd->bl.x,hd->bl.y,hd->ud.to_x,hd->ud.to_y,8,8);
	WBUFW(buf,65)=((level = status_get_lv(&hd->bl))>99)? 99:level;
	strncpy(WBUFP(buf,69),hd->status.name,24);
#endif
	return len;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_merc0078(struct merc_data *mcd,unsigned char *buf)
{
	int len, level;

	nullpo_retr(0, mcd);

#if PACKETVER < 12
	len = packet_db[0x78].len;
	memset(buf,0,len);

	WBUFW(buf,0) =0x78;
	WBUFL(buf,2) =mcd->bl.id;
	WBUFW(buf,6) =mcd->speed;
	WBUFW(buf,8) =mcd->sc.opt1;
	WBUFW(buf,10)=mcd->sc.opt2;
	WBUFW(buf,12)=mcd->sc.option;
	WBUFW(buf,14)=mcd->view_class;
	WBUFW(buf,16)=battle_config.pet0078_hair_id;
	WBUFW(buf,20)=0;
	WBUFW(buf,42)=mcd->sc.opt3;
	WBUFPOS(buf,46,mcd->bl.x,mcd->bl.y,mcd->dir);
	WBUFB(buf,49)=0;
	WBUFB(buf,50)=0;
	WBUFW(buf,52)=((level = status_get_lv(&mcd->bl))>99)? 99:level;
#elif PACKETVER < 23
	len = packet_db[0x78].len;
	memset(buf,0,len);

	WBUFW(buf,0) =0x78;
	WBUFB(buf,2) =0;
	WBUFL(buf,3) =mcd->bl.id;
	WBUFW(buf,7) =mcd->speed;
	WBUFW(buf,9) =mcd->sc.opt1;
	WBUFW(buf,11)=mcd->sc.opt2;
	WBUFW(buf,13)=mcd->sc.option;
	WBUFW(buf,15)=mcd->view_class;
	WBUFW(buf,17)=battle_config.pet0078_hair_id;
	WBUFW(buf,21)=0;
	WBUFW(buf,43)=mcd->sc.opt3;
	WBUFPOS(buf,47,mcd->bl.x,mcd->bl.y,mcd->dir);
	WBUFB(buf,50)=0;
	WBUFB(buf,51)=0;
	WBUFW(buf,53)=((level = status_get_lv(&mcd->bl))>99)? 99:level;
#else
	len = 63 + (int)strlen(mcd->status.name);
	memset(buf,0,len);

	WBUFW(buf,0) =0x7f9;
	WBUFW(buf,2) =(unsigned short)len;
	WBUFB(buf,4) =9;
	WBUFL(buf,5) =mcd->bl.id;
	WBUFW(buf,9) =mcd->speed;
	WBUFW(buf,11)=mcd->sc.opt1;
	WBUFW(buf,13)=mcd->sc.opt2;
	WBUFL(buf,15)=mcd->sc.option;
	WBUFW(buf,19)=mcd->view_class;
	WBUFW(buf,21)=100;
	WBUFL(buf,47)=mcd->sc.opt3;
	WBUFPOS(buf,53,mcd->bl.x,mcd->bl.y,mcd->dir);
	WBUFW(buf,59)=((level = status_get_lv(&mcd->bl))>99)? 99:level;
	strncpy(WBUFP(buf,63),mcd->status.name,24);
#endif
	return len;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_merc007b(struct merc_data *mcd,unsigned char *buf)
{
	int len, level;

	nullpo_retr(0, mcd);

#if PACKETVER < 23
	len = packet_db[0x7b].len;
	memset(buf,0,len);

	WBUFW(buf,0) =0x7b;
	WBUFL(buf,2) =mcd->bl.id;
	WBUFW(buf,6) =mcd->speed;
	WBUFW(buf,8) =mcd->sc.opt1;
	WBUFW(buf,10)=mcd->sc.opt2;
	WBUFW(buf,12)=mcd->sc.option;
	WBUFW(buf,14)=mcd->view_class;
	WBUFW(buf,16)=battle_config.pet0078_hair_id;
	WBUFL(buf,22)=gettick();
	WBUFW(buf,46)=mcd->sc.opt3;
	WBUFPOS2(buf,50,mcd->bl.x,mcd->bl.y,mcd->ud.to_x,mcd->ud.to_y,8,8);
	WBUFB(buf,56)=0;
	WBUFB(buf,57)=0;
	WBUFW(buf,58)=((level = status_get_lv(&mcd->bl))>99)? 99:level;
#else
	len = 69 + (int)strlen(mcd->status.name);
	memset(buf,0,len);

	WBUFW(buf,0) =0x7f7;
	WBUFW(buf,2) =(unsigned short)len;
	WBUFB(buf,4) =9;
	WBUFL(buf,5) =mcd->bl.id;
	WBUFW(buf,9) =mcd->speed;
	WBUFW(buf,11)=mcd->sc.opt1;
	WBUFW(buf,13)=mcd->sc.opt2;
	WBUFL(buf,15)=mcd->sc.option;
	WBUFW(buf,19)=mcd->view_class;
	WBUFW(buf,21)=100;
	WBUFL(buf,29)=gettick();
	WBUFL(buf,51)=mcd->sc.opt3;
	WBUFPOS2(buf,57,mcd->bl.x,mcd->bl.y,mcd->ud.to_x,mcd->ud.to_y,8,8);
	WBUFW(buf,65)=((level = status_get_lv(&mcd->bl))>99)? 99:level;
	strncpy(WBUFP(buf,69),mcd->status.name,24);
#endif
	return len;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_set01e1(const int fd, struct map_session_data *dstsd, short num)
{
	nullpo_retv(dstsd);

	WFIFOW(fd,0)=0x1e1;
	WFIFOL(fd,2)=dstsd->bl.id;
	WFIFOW(fd,6)=num;
	WFIFOSET(fd,packet_db[0x1e1].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_set0192(int fd, int m, int x, int y, int type)
{
	WFIFOW(fd,0) = 0x192;
	WFIFOW(fd,2) = x;
	WFIFOW(fd,4) = y;
	WFIFOW(fd,6) = type;
	memcpy(WFIFOP(fd,8),map[m].name,16);
	WFIFOSET(fd,packet_db[0x192].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_spawnpc(struct map_session_data *sd)
{
	int len;

	nullpo_retv(sd);

	clif_set0078(sd,WFIFOP(sd->fd,0));

#if PACKETVER < 4
	len = packet_db[0x79].len;
	WFIFOW(sd->fd,0)=0x79;
	WFIFOW(sd->fd,51)=(sd->status.base_level>99)?99:sd->status.base_level;
#elif PACKETVER < 7
	len = packet_db[0x1d9].len;
	WFIFOW(sd->fd,0)=0x1d9;
	WFIFOW(sd->fd,51)=(sd->status.base_level>99)?99:sd->status.base_level;
#elif PACKETVER < 14
	len = packet_db[0x22b].len;
	WFIFOW(sd->fd,0)=0x22b;
	WFIFOW(sd->fd,55)=(sd->status.base_level>99)?99:sd->status.base_level;
#elif PACKETVER < 23
	len = packet_db[0x2ed].len;
	WFIFOW(sd->fd,0)=0x2ed;
	WFIFOW(sd->fd,55)=(sd->status.base_level>99)?99:sd->status.base_level;
#else
	len = 62 + (int)strlen(sd->status.name);
	WFIFOW(sd->fd,0)=0x7f8;
	WFIFOW(sd->fd,2)=(unsigned short)len;
	WFIFOB(sd->fd,4)=0;
	WFIFOW(sd->fd,58)=(sd->status.base_level>99)?99:sd->status.base_level;
	WFIFOW(sd->fd,60)=0;	// font
	strncpy(WFIFOP(sd->fd,62),sd->status.name,24);
#endif
	clif_send(WFIFOP(sd->fd,0),len,&sd->bl,AREA_WOS);

	if(sd->spiritball.num > 0)
		clif_spiritball(sd);
	if(sd->coin.num > 0)
		clif_coin(sd);
	if(sd->view_size!=0)
		clif_misceffect2(&sd->bl,422+sd->view_size);

	if (map[sd->bl.m].flag.rain)
		clif_misceffect3(sd->fd, sd->bl.id, 161);
	if (map[sd->bl.m].flag.snow)
		clif_misceffect3(sd->fd, sd->bl.id, 162);
	if (map[sd->bl.m].flag.sakura)
		clif_misceffect3(sd->fd, sd->bl.id, 163);
	if (map[sd->bl.m].flag.leaves)
		clif_misceffect3(sd->fd, sd->bl.id, 333);
	if (map[sd->bl.m].flag.fireworks)
		clif_misceffect3(sd->fd, sd->bl.id, 297);
	if (map[sd->bl.m].flag.fireworks)
		clif_misceffect3(sd->fd, sd->bl.id, 299);
	if (map[sd->bl.m].flag.fireworks)
		clif_misceffect3(sd->fd, sd->bl.id, 301);
	if (map[sd->bl.m].flag.cloud1)
		clif_misceffect3(sd->fd, sd->bl.id, 230);
	if (map[sd->bl.m].flag.cloud2)
		clif_misceffect3(sd->fd, sd->bl.id, 233);
	if (map[sd->bl.m].flag.cloud3)
		clif_misceffect3(sd->fd, sd->bl.id, 516);
	if (map[sd->bl.m].flag.fog)
		clif_misceffect3(sd->fd, sd->bl.id, 515);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_spawnnpc(struct npc_data *nd)
{
	unsigned char buf[128];
	int len;

	nullpo_retv(nd);

	if(nd->class_ < 0 || (nd->flag&1 && nd->option != OPTION_HIDE) || nd->class_ == INVISIBLE_CLASS)
		return;

#if PACKETVER < 12
	len = packet_db[0x7c].len;
	memset(buf,0,len);

	WBUFW(buf,0)=0x7c;
	WBUFL(buf,2)=nd->bl.id;
	WBUFW(buf,6)=nd->speed;
	WBUFW(buf,12)=nd->option;
	WBUFW(buf,20)=nd->class_;
	WBUFPOS(buf,36,nd->bl.x,nd->bl.y,nd->dir);
#elif PACKETVER < 23
	len = packet_db[0x7c].len;
	memset(buf,0,len);

	WBUFW(buf,0)=0x7c;
	WBUFB(buf,2)=0;
	WBUFL(buf,3)=nd->bl.id;
	WBUFW(buf,7)=nd->speed;
	WBUFW(buf,13)=nd->option;
	WBUFW(buf,21)=nd->class_;
	WBUFPOS(buf,37,nd->bl.x,nd->bl.y,nd->dir);
#else
	len = 62 + (int)strlen(nd->name);
	memset(buf,0,len);

	WBUFW(buf,0)=0x7f8;
	WBUFW(buf,2)=(unsigned short)len;
	WBUFB(buf,4)=6;
	WBUFL(buf,5)=nd->bl.id;
	WBUFW(buf,9)=nd->speed;
	WBUFL(buf,15)=nd->option;
	WBUFW(buf,19)=nd->class_;
	WBUFW(buf,33)=6;
	WBUFPOS(buf,53,nd->bl.x,nd->bl.y,nd->dir);
	strncpy(WBUFP(buf,62),nd->name,24);
#endif
	clif_send(buf,len,&nd->bl,AREA);

	len = clif_npc0078(nd,buf);
	clif_send(buf,len,&nd->bl,AREA);

	if(nd->view_size!=0)
		clif_misceffect2(&nd->bl,422+nd->view_size);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_spawnmob(struct mob_data *md)
{
	unsigned char buf[128];
	int len;

	nullpo_retv(md);

	if(!mob_is_pcview(md->class_)) {
#if PACKETVER < 12
		len = packet_db[0x7c].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x7c;
		WBUFL(buf,2)=md->bl.id;
		WBUFW(buf,6)=md->speed;
		WBUFW(buf,8)=md->sc.opt1;
		WBUFW(buf,10)=md->sc.opt2;
		WBUFW(buf,12)=md->sc.option;
		WBUFW(buf,20)=mob_get_viewclass(md->class_);
		WBUFPOS(buf,36,md->bl.x,md->bl.y,md->dir);
#elif PACKETVER < 23
		len = packet_db[0x7c].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x7c;
		WBUFB(buf,2)=0;
		WBUFL(buf,3)=md->bl.id;
		WBUFW(buf,7)=md->speed;
		WBUFW(buf,9)=md->sc.opt1;
		WBUFW(buf,11)=md->sc.opt2;
		WBUFW(buf,13)=md->sc.option;
		WBUFW(buf,21)=mob_get_viewclass(md->class_);
		WBUFPOS(buf,37,md->bl.x,md->bl.y,md->dir);
#else
		len = 62 + (int)strlen(md->name);
		memset(buf,0,len);

		WBUFW(buf,0)=0x7f8;
		WBUFW(buf,2)=(unsigned short)len;
		WBUFB(buf,4)=5;
		WBUFL(buf,5)=md->bl.id;
		WBUFW(buf,9)=md->speed;
		WBUFW(buf,11)=md->sc.opt1;
		WBUFW(buf,13)=md->sc.opt2;
		WBUFL(buf,15)=md->sc.option;
		WBUFW(buf,19)=mob_get_viewclass(md->class_);
		WBUFW(buf,33)=5;
		WBUFPOS(buf,53,md->bl.x,md->bl.y,md->dir);
		{
			int level;
			WBUFW(buf,58)=((level = status_get_lv(&md->bl))>99)? 99:level;
		}
		strncpy(WBUFP(buf,62),md->name,24);
#endif
		clif_send(buf,len,&md->bl,AREA);
	}

	len = clif_mob0078(md,buf);
	clif_send(buf,len,&md->bl,AREA);

	clif_send_clothcolor(&md->bl);
	if(md->view_size!=0)
		clif_misceffect2(&md->bl,422+md->view_size);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_spawnpet(struct pet_data *pd)
{
	unsigned char buf[128];
	int len;

	nullpo_retv(pd);

	if(!mob_is_pcview(pd->class_)) {
#if PACKETVER < 12
		len = packet_db[0x7c].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x7c;
		WBUFL(buf,2)=pd->bl.id;
		WBUFW(buf,6)=pd->speed;
		WBUFW(buf,20)=mob_get_viewclass(pd->class_);
		WBUFPOS(buf,36,pd->bl.x,pd->bl.y,pd->dir);
#elif PACKETVER < 23
		len = packet_db[0x7c].len;
		memset(buf,0,len);

		WBUFW(buf,0)=0x7c;
		WBUFB(buf,2)=0;
		WBUFL(buf,3)=pd->bl.id;
		WBUFW(buf,7)=pd->speed;
		WBUFW(buf,21)=mob_get_viewclass(pd->class_);
		WBUFPOS(buf,37,pd->bl.x,pd->bl.y,pd->dir);
#else
		len = 62 + (int)strlen(pd->name);
		memset(buf,0,len);

		WBUFW(buf,0)=0x7f8;
		WBUFW(buf,2)=(unsigned short)len;
		WBUFB(buf,4)=7;
		WBUFL(buf,5)=pd->bl.id;
		WBUFW(buf,9)=pd->speed;
		WBUFW(buf,19)=mob_get_viewclass(pd->class_);
		WBUFW(buf,33)=7;
		WBUFPOS(buf,53,pd->bl.x,pd->bl.y,pd->dir);
		{
			int level;
			WBUFW(buf,58)=((level = status_get_lv(&pd->bl))>99)? 99:level;
		}
		strncpy(WBUFP(buf,62),pd->name,24);
#endif
		clif_send(buf,len,&pd->bl,AREA);
	}

	len = clif_pet0078(pd,buf);
	clif_send(buf,len,&pd->bl,AREA);

	clif_send_clothcolor(&pd->bl);
	if(pd->view_size!=0)
		clif_misceffect2(&pd->bl,422+pd->view_size);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_spawnhom(struct homun_data *hd)
{
	unsigned char buf[128];
	int len;

	nullpo_retv(hd);

#if PACKETVER < 12
	len = packet_db[0x7c].len;
	memset(buf,0,len);

	WBUFW(buf,0) =0x7c;
	WBUFL(buf,2) =hd->bl.id;
	WBUFW(buf,6) =hd->speed;
	WBUFW(buf,20)=hd->view_class;
	WBUFW(buf,28)=8;		// ���ׂ�����Œ�
	WBUFPOS(buf,36,hd->bl.x,hd->bl.y,hd->dir);
#elif PACKETVER < 23
	len = packet_db[0x7c].len;
	memset(buf,0,len);

	WBUFW(buf,0) =0x7c;
	WBUFB(buf,2) =0;
	WBUFL(buf,3) =hd->bl.id;
	WBUFW(buf,7) =hd->speed;
	WBUFW(buf,21)=hd->view_class;
	WBUFW(buf,29)=8;		// ���ׂ�����Œ�
	WBUFPOS(buf,37,hd->bl.x,hd->bl.y,hd->dir);
#else
	len = 62 + (int)strlen(hd->status.name);
	memset(buf,0,len);

	WBUFW(buf,0)=0x7f8;
	WBUFW(buf,2)=(unsigned short)len;
	WBUFB(buf,4)=8;
	WBUFL(buf,5)=hd->bl.id;
	WBUFW(buf,9)=hd->speed;
	WBUFW(buf,19)=hd->view_class;
	WBUFW(buf,33)=8;
	WBUFPOS(buf,53,hd->bl.x,hd->bl.y,hd->dir);
	{
		int level;
		WBUFW(buf,58)=((level = status_get_lv(&hd->bl))>99)? 99:level;
	}
	strncpy(WBUFP(buf,62),hd->status.name,24);
#endif
	clif_send(buf,len,&hd->bl,AREA);

	// �z���ł́A0x78�p�P�b�g��A����ɑ���p�P�b�g�̊m�F�ł���
	//len = clif_hom0078(hd,buf);
	//clif_send(buf,len,&hd->bl,AREA);

	if(hd->view_size!=0)
		clif_misceffect2(&hd->bl,422+hd->view_size);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_spawnmerc(struct merc_data *mcd)
{
	unsigned char buf[128];
	int len;

	nullpo_retv(mcd);

#if PACKETVER < 12
	len = packet_db[0x7c].len;
	memset(buf,0,len);

	WBUFW(buf,0) =0x7c;
	WBUFL(buf,2) =mcd->bl.id;
	WBUFW(buf,6) =mcd->speed;
	WBUFW(buf,20)=mcd->view_class;
	WBUFW(buf,28)=8;
	WBUFPOS(buf,36,mcd->bl.x,mcd->bl.y,mcd->dir);
#elif PACKETVER < 23
	len = packet_db[0x7c].len;
	memset(buf,0,len);

	WBUFW(buf,0) =0x7c;
	WBUFB(buf,2) =0;
	WBUFL(buf,3) =mcd->bl.id;
	WBUFW(buf,7) =mcd->speed;
	WBUFW(buf,21)=mcd->view_class;
	WBUFW(buf,29)=8;
	WBUFPOS(buf,37,mcd->bl.x,mcd->bl.y,mcd->dir);
#else
	len = 62 + (int)strlen(mcd->status.name);
	memset(buf,0,len);

	WBUFW(buf,0)=0x7f8;
	WBUFW(buf,2)=(unsigned short)len;
	WBUFB(buf,4)=9;
	WBUFL(buf,5)=mcd->bl.id;
	WBUFW(buf,9)=mcd->speed;
	WBUFW(buf,19)=mcd->view_class;
	WBUFW(buf,33)=9;
	WBUFPOS(buf,53,mcd->bl.x,mcd->bl.y,mcd->dir);
	{
		int level;
		WBUFW(buf,58)=((level = status_get_lv(&mcd->bl))>99)? 99:level;
	}
	strncpy(WBUFP(buf,62),mcd->status.name,24);
#endif
	clif_send(buf,len,&mcd->bl,AREA);

	if(mcd->view_size!=0)
		clif_misceffect2(&mcd->bl,422+mcd->view_size);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_servertick(struct map_session_data *sd)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x7f;
	WFIFOL(fd,2)=gettick();
	WFIFOSET(fd,packet_db[0x7f].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_walkok(struct map_session_data *sd)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x87;
	WFIFOL(fd,2)=gettick();
	WFIFOPOS2(fd,6,sd->bl.x,sd->bl.y,sd->ud.to_x,sd->ud.to_y,8,8);
	WFIFOSET(fd,packet_db[0x87].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_move_sub(struct block_list *bl)
{
	int len;

	nullpo_retv(bl);

	switch(bl->type) {
		case BL_PC:
		{
			struct map_session_data *sd = (struct map_session_data *)bl;
			len = clif_set007b(sd,WFIFOP(sd->fd,0));
			clif_send(WFIFOP(sd->fd,0),len,&sd->bl,AREA_WOS);
			break;
		}
		case BL_MOB:
		{
			unsigned char buf[128];
			struct mob_data *md = (struct mob_data *)bl;
			len = clif_mob007b(md,buf);
			clif_send(buf,len,&md->bl,AREA_WOS);
			break;
		}
		case BL_PET:
		{
			unsigned char buf[128];
			struct pet_data *pd = (struct pet_data *)bl;
			len = clif_pet007b(pd,buf);
			clif_send(buf,len,&pd->bl,AREA_WOS);
			break;
		}
		case BL_HOM:
		{
			unsigned char buf[128];
			struct homun_data *hd = (struct homun_data *)bl;
			len = clif_hom007b(hd,buf);
			clif_send(buf,len,&hd->bl,AREA_WOS);
			break;
		}
		case BL_MERC:
		{
			unsigned char buf[128];
			struct merc_data *mcd = (struct merc_data *)bl;
			len = clif_merc007b(mcd,buf);
			clif_send(buf,len,&mcd->bl,AREA_WOS);
			break;
		}
		default:
			break;
	}

	return;
}

void clif_move(struct block_list *bl)
{
	nullpo_retv(bl);

	// ���S�ȃC���r�W�u�����[�h�Ȃ瑗�M���Ȃ�
	if(bl->type == BL_PC && battle_config.gm_perfect_hide) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if(sd && pc_isinvisible(sd))
			return;
	}

#if PACKETVER < 23
	{
		struct unit_data *ud = unit_bl2ud(bl);
		if(ud) {
			unsigned char buf[16];

			if(ud->state.change_speed) {
				// ���x���ύX���ꂽ�Ƃ��͌Â����s�p�P�b�g�𑗐M����
				clif_move_sub(bl);
				ud->state.change_speed = 0;
			}

			WBUFW(buf,0)=0x86;
			WBUFL(buf,2)=bl->id;
			WBUFPOS2(buf,6,bl->x,bl->y,ud->to_x,ud->to_y,8,8);
			WBUFL(buf,12)=gettick();
			clif_send(buf, packet_db[0x86].len, bl, AREA_WOS);
		}
	}
#else
	clif_move_sub(bl);
#endif
	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_quitsave(struct map_session_data *sd)
{
	map_quit(sd);
	chrif_chardisconnect(sd);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_waitclose(int tid,unsigned int tick,int id,void *data)
{
	if(session[id])
		session[id]->eof=1;
	close(id);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_setwaitclose(int fd)
{
	add_timer(gettick()+5000,clif_waitclose,fd,NULL);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_changemap(struct map_session_data *sd,char *mapname,int x,int y)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x91;
	memcpy(WFIFOP(fd,2),mapname,16);
	WFIFOW(fd,18)=x;
	WFIFOW(fd,20)=y;
	WFIFOSET(fd,packet_db[0x91].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_changemapserver(struct map_session_data *sd, char *mapname, int x, int y, unsigned long ip, unsigned short port)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x92;
	memcpy(WFIFOP(fd,2),mapname,16);
	WFIFOW(fd,18)=x;
	WFIFOW(fd,20)=y;
	WFIFOL(fd,22)=ip;
	WFIFOW(fd,26)=port;
	WFIFOSET(fd,packet_db[0x92].len);

	return;
}

/*==========================================
 * �ʒu�␳
 *------------------------------------------
 */
int clif_fixpos(struct block_list *bl)
{
	nullpo_retr(0, bl);

	if(battle_config.clif_fixpos_type)
	{
		int x[4], y[4];
		// clif_fixpos2 �p�̃f�[�^���쐬
		x[0] = bl->x - AREA_SIZE;
		x[1] = bl->x + AREA_SIZE;
		x[2] = bl->x - AREA_SIZE;
		x[3] = bl->x + AREA_SIZE;
		y[0] = bl->y - AREA_SIZE;
		y[1] = bl->y + AREA_SIZE;
		y[2] = bl->y - AREA_SIZE;
		y[3] = bl->y + AREA_SIZE;
		clif_fixpos2(bl, x, y);
	}else{
		unsigned char buf[16];
		WBUFW(buf,0)=0x88;
		WBUFL(buf,2)=bl->id;
		WBUFW(buf,6)=bl->x;
		WBUFW(buf,8)=bl->y;
		clif_send(buf,packet_db[0x88].len,bl,AREA);
	}

	return 0;
}

/*==========================================
 * �ʒu�␳2
 *------------------------------------------
 */
static int clif_fixpos2_sub(struct block_list *bl, va_list ap)
{
	struct map_session_data *sd;
	struct block_list *src_bl = va_arg(ap, struct block_list*);
	int len  = va_arg(ap, int);
	unsigned char *buf = va_arg(ap, unsigned char*);

	nullpo_retr(0, bl);
	nullpo_retr(0, sd = (struct map_session_data *)bl);

	if( src_bl->id != sd->bl.id ) { // �����͏��O
		memcpy( WFIFOP(sd->fd, 0), buf, len);
		WFIFOSET( sd->fd, len );
	}

	return 0;
}

int clif_fixpos2(struct block_list *bl, int x[4], int y[4])
{
	int len;
	unsigned char buf[128];

	nullpo_retr(0, bl);

	if(battle_config.clif_fixpos_type)
	{
		// self position is changed
		// ���L�����̐�����΂��p�P���M�i���͎ҕ�W�j
		// ����ς�JT�̃G�t�F�N�g���c���Ă�����Ƀp�P���M���Ă�
		// �ړ����Ȃ��̂͑��̎d�l���ۂ��B�B�B
		// �@�� clif_blown() �𗘗p����悤�ɕύX

		if( bl->type == BL_PC ) {
			struct map_session_data *sd = (struct map_session_data*)bl;
			WFIFOW(sd->fd, 0) = 0x88;
			WFIFOL(sd->fd, 2) = sd->bl.id;
			WFIFOW(sd->fd, 6) = sd->bl.x;
			WFIFOW(sd->fd, 8) = sd->bl.y;
			WFIFOSET(sd->fd, packet_db[0x88].len);
		}

		// sent other players to notify position change
		if( bl->type == BL_PC) {
			struct map_session_data *sd = (struct map_session_data *)bl;
			if(sd->ud.walktimer != -1){
				len = clif_set007b(sd,buf);
			} else {
				len = clif_set0078(sd,buf);
			}
		} else if( bl->type == BL_MOB) {
			struct mob_data *md = (struct mob_data*)bl;
			if(md->ud.walktimer != -1){
				len = clif_mob007b(md,buf);
			} else {
				len = clif_mob0078(md,buf);
			}
		} else if( bl->type == BL_PET ) {
			struct pet_data *pd = (struct pet_data *)bl;
			if(pd->ud.walktimer != -1) {
				len = clif_pet007b(pd,buf);
			} else {
				len = clif_pet0078(pd,buf);
			}
		} else if( bl->type == BL_HOM ) {
			struct homun_data *hd = (struct homun_data *)bl;
			if(hd->ud.walktimer != -1) {
				len = clif_hom007b(hd,buf);
			} else {
				len = clif_hom0078(hd,buf);
			}
		} else if( bl->type == BL_MERC ) {
			struct merc_data *mcd = (struct merc_data *)bl;
			if(mcd->ud.walktimer != -1) {
				len = clif_merc007b(mcd,buf);
			} else {
				len = clif_merc0078(mcd,buf);
			}
		} else {
			WBUFW(buf,0)=0x88;
			WBUFL(buf,2)=bl->id;
			WBUFW(buf,6)=bl->x;
			WBUFW(buf,8)=bl->y;
			len = packet_db[0x88].len;
		}
		map_foreachcommonarea(clif_fixpos2_sub, bl->m, x, y, BL_PC, bl, len, buf);
	}else{
		WBUFW(buf,0)=0x88;
		WBUFL(buf,2)=bl->id;
		WBUFW(buf,6)=bl->x;
		WBUFW(buf,8)=bl->y;
		clif_send(buf,packet_db[0x88].len,bl,AREA);
	}

	return 0;
}

/*==========================================
 * �ړ����̈ʒu�␳
 *------------------------------------------
 */
void clif_fixwalkpos(struct block_list *bl)
{
	unsigned char buf[16];

	nullpo_retv(bl);

	WBUFW(buf,0)=0x88;
	WBUFL(buf,2)=bl->id;
	WBUFW(buf,6)=bl->x;
	WBUFW(buf,8)=bl->y;
	clif_send(buf,packet_db[0x88].len,bl,AREA);

	return;
}

/*==========================================
 * ������΂�
 *------------------------------------------
 */
void clif_blown(struct block_list *bl, int x, int y)
{
	unsigned char buf[10];

	nullpo_retv(bl);

	WBUFW(buf, 0) = 0x1ff;
	WBUFL(buf, 2) = bl->id;
	WBUFW(buf, 6) = x;
	WBUFW(buf, 8) = y;
	clif_send(buf,packet_db[0x1ff].len,bl,AREA);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_npcbuysell(struct map_session_data* sd, int id)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xc4;
	WFIFOL(fd,2)=id;
	WFIFOSET(fd,packet_db[0xc4].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_buylist(struct map_session_data *sd, struct npc_data *nd)
{
	struct item_data *id;
	int fd,i,val;

	nullpo_retv(sd);
	nullpo_retv(nd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xc6;
	for(i=0;nd->u.shop_item[i].nameid > 0;i++){
		id = itemdb_search(nd->u.shop_item[i].nameid);
		val=nd->u.shop_item[i].value;
		WFIFOL(fd,4+i*11)=val;
		if ( !id->flag.value_notdc )
			val=pc_modifybuyvalue(sd,val);
		WFIFOL(fd,8+i*11)=val;
		WFIFOB(fd,12+i*11)=id->type;
		if(id->view_id > 0)
			WFIFOW(fd,13+i*11)=id->view_id;
		else
			WFIFOW(fd,13+i*11)=nd->u.shop_item[i].nameid;
	}
	WFIFOW(fd,2)=i*11+4;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_selllist(struct map_session_data *sd)
{
	int fd,i,c=0,val;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xc7;
	for(i=0;i<MAX_INVENTORY;i++) {
		if(sd->status.inventory[i].nameid > 0 && sd->inventory_data[i]) {
			val=sd->inventory_data[i]->value_sell;
			if(val < 0)
				continue;
			WFIFOW(fd,4+c*10)=i+2;
			WFIFOL(fd,6+c*10)=val;
			if ( !sd->inventory_data[i]->flag.value_notoc )
				val=pc_modifysellvalue(sd,val);
			WFIFOL(fd,10+c*10)=val;
			c++;
		}
	}
	WFIFOW(fd,2)=c*10+4;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 * �X�y�V�����A�C�e���̔����X�g
 *------------------------------------------
 */
void clif_pointshop_list(struct map_session_data *sd, struct npc_data *nd)
{
	struct item_data *id;
	int fd,i,val,len;

	nullpo_retv(sd);
	nullpo_retv(nd);

	fd  = sd->fd;
#if PACKETVER < 10
	len = 8;
#else
	len = 12;
#endif

	WFIFOW(fd,0) = 0x287;
	WFIFOL(fd,4) = sd->shop_point;
	WFIFOL(fd,8) = 0;	// cash?
	for(i=0; nd->u.shop_item[i].nameid > 0; i++) {
		id  = itemdb_search(nd->u.shop_item[i].nameid);
		val = nd->u.shop_item[i].value;
		WFIFOL(fd,len  ) = val;
		WFIFOL(fd,len+4) = val;		// DC�͂Ȃ��̂ŉ��i�͓���
		WFIFOB(fd,len+8) = id->type;
		if(id->view_id > 0)
			WFIFOW(fd,len+9) = id->view_id;
		else
			WFIFOW(fd,len+9) = nd->u.shop_item[i].nameid;
		len += 11;
	}
	WFIFOW(fd,2) = len;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_scriptmes(struct map_session_data *sd, int npcid, char *mes)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xb4;
	WFIFOW(fd,2)=(unsigned short)(strlen(mes)+9);
	WFIFOL(fd,4)=npcid;
	strncpy(WFIFOP(fd,8),mes,strlen(mes)+1);
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_scriptnext(struct map_session_data *sd, int npcid)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xb5;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_db[0xb5].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_scriptclose(struct map_session_data *sd, int npcid)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xb6;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_db[0xb6].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_scriptmenu(struct map_session_data *sd, int npcid, char *mes)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xb7;
	WFIFOW(fd,2)=(unsigned short)(strlen(mes)+9);
	WFIFOL(fd,4)=npcid;
	strncpy(WFIFOP(fd,8),mes,strlen(mes)+1);
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_scriptinput(struct map_session_data *sd, int npcid)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x142;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_db[0x142].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_scriptinputstr(struct map_session_data *sd, int npcid)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x1d4;
	WFIFOL(fd,2)=npcid;
	WFIFOSET(fd,packet_db[0x1d4].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_viewpoint(struct map_session_data *sd, int npc_id, int type, int x, int y, int id, int color)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x144;
	WFIFOL(fd,2)=npc_id;
	WFIFOL(fd,6)=type;
	WFIFOL(fd,10)=x;
	WFIFOL(fd,14)=y;
	WFIFOB(fd,18)=id;
	WFIFOL(fd,19)=color;
	WFIFOSET(fd,packet_db[0x144].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_cutin(struct map_session_data *sd, char *image, int type)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x1b3;
	memcpy(WFIFOP(fd,2),image,64);
	WFIFOB(fd,66)=type;
	WFIFOSET(fd,packet_db[0x1b3].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_additem(struct map_session_data *sd, int n, int amount, unsigned char fail)
{
	int fd,j;

	nullpo_retv(sd);

	fd=sd->fd;

#if PACKETVER < 10
	if(fail) {
		memset(WFIFOP(fd,0), 0, packet_db[0xa0].len);

		WFIFOW(fd,0)=0xa0;
		WFIFOW(fd,2)=n+2;
		WFIFOW(fd,4)=amount;
		WFIFOB(fd,22)=fail;
	} else {
		if(n<0 || n>=MAX_INVENTORY || sd->status.inventory[n].nameid <=0 || sd->inventory_data[n] == NULL)
			return;

		WFIFOW(fd,0)=0xa0;
		WFIFOW(fd,2)=n+2;
		WFIFOW(fd,4)=amount;
		if(sd->inventory_data[n]->view_id > 0)
			WFIFOW(fd,6)=sd->inventory_data[n]->view_id;
		else
			WFIFOW(fd,6)=sd->status.inventory[n].nameid;
		WFIFOB(fd,8)=sd->status.inventory[n].identify;
		WFIFOB(fd,9)=sd->status.inventory[n].attribute;
		WFIFOB(fd,10)=sd->status.inventory[n].refine;
		if(itemdb_isspecial(sd->status.inventory[n].card[0])) {
			if(sd->inventory_data[n]->flag.pet_egg) {
				WFIFOW(fd,11) = 0;
				WFIFOW(fd,13) = 0;
				WFIFOW(fd,15) = 0;
			} else {
				WFIFOW(fd,11) = sd->status.inventory[n].card[0];
				WFIFOW(fd,13) = sd->status.inventory[n].card[1];
				WFIFOW(fd,15) = sd->status.inventory[n].card[2];
			}
			WFIFOW(fd,17) = sd->status.inventory[n].card[3];
		} else {
			if(sd->status.inventory[n].card[0] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[0])) > 0)
				WFIFOW(fd,11)=j;
			else
				WFIFOW(fd,11)=sd->status.inventory[n].card[0];
			if(sd->status.inventory[n].card[1] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[1])) > 0)
				WFIFOW(fd,13)=j;
			else
				WFIFOW(fd,13)=sd->status.inventory[n].card[1];
			if(sd->status.inventory[n].card[2] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[2])) > 0)
				WFIFOW(fd,15)=j;
			else
				WFIFOW(fd,15)=sd->status.inventory[n].card[2];
			if(sd->status.inventory[n].card[3] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[3])) > 0)
				WFIFOW(fd,17)=j;
			else
				WFIFOW(fd,17)=sd->status.inventory[n].card[3];
		}
		WFIFOW(fd,19)=pc_equippoint(sd,n);
		WFIFOB(fd,21)=sd->inventory_data[n]->type;
		WFIFOB(fd,22)=fail;
	}
	WFIFOSET(fd,packet_db[0xa0].len);
#elif PACKETVER < 11
	if(fail) {
		memset(WFIFOP(fd,0), 0, packet_db[0x29a].len);

		WFIFOW(fd,0)=0x29a;
		WFIFOW(fd,2)=n+2;
		WFIFOW(fd,4)=amount;
		WFIFOB(fd,22)=fail;
	} else {
		if(n<0 || n>=MAX_INVENTORY || sd->status.inventory[n].nameid <=0 || sd->inventory_data[n] == NULL)
			return;

		WFIFOW(fd,0)=0x29a;
		WFIFOW(fd,2)=n+2;
		WFIFOW(fd,4)=amount;
		if(sd->inventory_data[n]->view_id > 0)
			WFIFOW(fd,6)=sd->inventory_data[n]->view_id;
		else
			WFIFOW(fd,6)=sd->status.inventory[n].nameid;
		WFIFOB(fd,8)=sd->status.inventory[n].identify;
		WFIFOB(fd,9)=sd->status.inventory[n].attribute;
		WFIFOB(fd,10)=sd->status.inventory[n].refine;
		if(itemdb_isspecial(sd->status.inventory[n].card[0])) {
			if(sd->inventory_data[n]->flag.pet_egg) {
				WFIFOW(fd,11) = 0;
				WFIFOW(fd,13) = 0;
				WFIFOW(fd,15) = 0;
			} else {
				WFIFOW(fd,11) = sd->status.inventory[n].card[0];
				WFIFOW(fd,13) = sd->status.inventory[n].card[1];
				WFIFOW(fd,15) = sd->status.inventory[n].card[2];
			}
			WFIFOW(fd,17) = sd->status.inventory[n].card[3];
		} else {
			if(sd->status.inventory[n].card[0] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[0])) > 0)
				WFIFOW(fd,11)=j;
			else
				WFIFOW(fd,11)=sd->status.inventory[n].card[0];
			if(sd->status.inventory[n].card[1] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[1])) > 0)
				WFIFOW(fd,13)=j;
			else
				WFIFOW(fd,13)=sd->status.inventory[n].card[1];
			if(sd->status.inventory[n].card[2] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[2])) > 0)
				WFIFOW(fd,15)=j;
			else
				WFIFOW(fd,15)=sd->status.inventory[n].card[2];
			if(sd->status.inventory[n].card[3] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[3])) > 0)
				WFIFOW(fd,17)=j;
			else
				WFIFOW(fd,17)=sd->status.inventory[n].card[3];
		}
		WFIFOW(fd,19)=pc_equippoint(sd,n);
		WFIFOB(fd,21)=sd->inventory_data[n]->type;
		WFIFOB(fd,22)=fail;
		WFIFOL(fd,23)=sd->status.inventory[n].limit;
	}
	WFIFOSET(fd,packet_db[0x29a].len);
#else
	if(fail) {
		memset(WFIFOP(fd,0), 0, packet_db[0x2d4].len);

		WFIFOW(fd,0)=0x2d4;
		WFIFOW(fd,2)=n+2;
		WFIFOW(fd,4)=amount;
		WFIFOB(fd,22)=fail;
	} else {
		if(n<0 || n>=MAX_INVENTORY || sd->status.inventory[n].nameid <=0 || sd->inventory_data[n] == NULL)
			return;

		WFIFOW(fd,0)=0x2d4;
		WFIFOW(fd,2)=n+2;
		WFIFOW(fd,4)=amount;
		if(sd->inventory_data[n]->view_id > 0)
			WFIFOW(fd,6)=sd->inventory_data[n]->view_id;
		else
			WFIFOW(fd,6)=sd->status.inventory[n].nameid;
		WFIFOB(fd,8)=sd->status.inventory[n].identify;
		WFIFOB(fd,9)=sd->status.inventory[n].attribute;
		WFIFOB(fd,10)=sd->status.inventory[n].refine;
		if(itemdb_isspecial(sd->status.inventory[n].card[0])) {
			if(sd->inventory_data[n]->flag.pet_egg) {
				WFIFOW(fd,11) = 0;
				WFIFOW(fd,13) = 0;
				WFIFOW(fd,15) = 0;
			} else {
				WFIFOW(fd,11) = sd->status.inventory[n].card[0];
				WFIFOW(fd,13) = sd->status.inventory[n].card[1];
				WFIFOW(fd,15) = sd->status.inventory[n].card[2];
			}
			WFIFOW(fd,17) = sd->status.inventory[n].card[3];
		} else {
			if(sd->status.inventory[n].card[0] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[0])) > 0)
				WFIFOW(fd,11)=j;
			else
				WFIFOW(fd,11)=sd->status.inventory[n].card[0];
			if(sd->status.inventory[n].card[1] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[1])) > 0)
				WFIFOW(fd,13)=j;
			else
				WFIFOW(fd,13)=sd->status.inventory[n].card[1];
			if(sd->status.inventory[n].card[2] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[2])) > 0)
				WFIFOW(fd,15)=j;
			else
				WFIFOW(fd,15)=sd->status.inventory[n].card[2];
			if(sd->status.inventory[n].card[3] > 0 && (j=itemdb_viewid(sd->status.inventory[n].card[3])) > 0)
				WFIFOW(fd,17)=j;
			else
				WFIFOW(fd,17)=sd->status.inventory[n].card[3];
		}
		WFIFOW(fd,19)=pc_equippoint(sd,n);
		WFIFOB(fd,21)=sd->inventory_data[n]->type;
		WFIFOB(fd,22)=fail;
		WFIFOL(fd,23)=sd->status.inventory[n].limit;
		WFIFOW(fd,27)=0;
	}
	WFIFOSET(fd,packet_db[0x2d4].len);
#endif

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_delitem(struct map_session_data *sd, short type, int n, int amount)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;

#if PACKETVER < 24
	WFIFOW(fd,0)=0xaf;
	WFIFOW(fd,2)=n+2;
	WFIFOW(fd,4)=amount;
	WFIFOSET(fd,packet_db[0xaf].len);
#else
	WFIFOW(fd,0)=0x7fa;
	WFIFOW(fd,2)=type;
	WFIFOW(fd,4)=n+2;
	WFIFOW(fd,6)=amount;
	WFIFOSET(fd,packet_db[0x7fa].len);
#endif

	return;
}

/*==========================================
 * �A�C�e���̎��Ԑ؂�폜
 *------------------------------------------
 */
void clif_delitem_timeout(struct map_session_data *sd, int n, int itemid)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x299;
	WFIFOW(fd,2)=n+2;
	WFIFOW(fd,4)=itemid;
	WFIFOSET(fd,packet_db[0x299].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_itemlist(struct map_session_data *sd)
{
	int i,n,fd,arrow=-1;

	nullpo_retv(sd);

	fd=sd->fd;

#if PACKETVER < 5
	WFIFOW(fd,0)=0xa3;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid <=0 || sd->inventory_data[i] == NULL || itemdb_isequip2(sd->inventory_data[i]))
			continue;
		WFIFOW(fd,n*10+4)=i+2;
		if(sd->inventory_data[i]->view_id > 0)
			WFIFOW(fd,n*10+6)=sd->inventory_data[i]->view_id;
		else
			WFIFOW(fd,n*10+6)=sd->status.inventory[i].nameid;
		WFIFOB(fd,n*10+8)=sd->inventory_data[i]->type;
		WFIFOB(fd,n*10+9)=sd->status.inventory[i].identify;
		WFIFOW(fd,n*10+10)=sd->status.inventory[i].amount;
		if(sd->inventory_data[i]->equip == LOC_ARROW){
			WFIFOW(fd,n*10+12)=LOC_ARROW;
			if(sd->status.inventory[i].equip) arrow=i;	// ���łɖ���`�F�b�N
		}
		else
			WFIFOW(fd,n*10+12)=0;
		n++;
	}
	if(n){
		WFIFOW(fd,2)=4+n*10;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#elif PACKETVER < 14
	WFIFOW(fd,0)=0x1ee;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid <=0 || sd->inventory_data[i] == NULL || itemdb_isequip2(sd->inventory_data[i]))
			continue;
		WFIFOW(fd,n*18+4)=i+2;
		if(sd->inventory_data[i]->view_id > 0)
			WFIFOW(fd,n*18+6)=sd->inventory_data[i]->view_id;
		else
			WFIFOW(fd,n*18+6)=sd->status.inventory[i].nameid;
		WFIFOB(fd,n*18+8)=sd->inventory_data[i]->type;
		WFIFOB(fd,n*18+9)=sd->status.inventory[i].identify;
		WFIFOW(fd,n*18+10)=sd->status.inventory[i].amount;
		if(sd->inventory_data[i]->equip == LOC_ARROW){
			WFIFOW(fd,n*18+12)=LOC_ARROW;
			if(sd->status.inventory[i].equip) arrow=i;	// ���łɖ���`�F�b�N
		}
		else
			WFIFOW(fd,n*18+12)=0;
		WFIFOW(fd,n*18+14)=sd->status.inventory[i].card[0];
		WFIFOW(fd,n*18+16)=sd->status.inventory[i].card[1];
		WFIFOW(fd,n*18+18)=sd->status.inventory[i].card[2];
		WFIFOW(fd,n*18+20)=sd->status.inventory[i].card[3];
		n++;
	}
	if(n){
		WFIFOW(fd,2)=4+n*18;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#else
	WFIFOW(fd,0)=0x2e8;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid <=0 || sd->inventory_data[i] == NULL || itemdb_isequip2(sd->inventory_data[i]))
			continue;
		WFIFOW(fd,n*22+4)=i+2;
		if(sd->inventory_data[i]->view_id > 0)
			WFIFOW(fd,n*22+6)=sd->inventory_data[i]->view_id;
		else
			WFIFOW(fd,n*22+6)=sd->status.inventory[i].nameid;
		WFIFOB(fd,n*22+8)=sd->inventory_data[i]->type;
		WFIFOB(fd,n*22+9)=sd->status.inventory[i].identify;
		WFIFOW(fd,n*22+10)=sd->status.inventory[i].amount;
		if(sd->inventory_data[i]->equip == LOC_ARROW){
			WFIFOW(fd,n*22+12)=LOC_ARROW;
			if(sd->status.inventory[i].equip) arrow=i;	// ���łɖ���`�F�b�N
		}
		else
			WFIFOW(fd,n*22+12)=0;
		WFIFOW(fd,n*22+14)=sd->status.inventory[i].card[0];
		WFIFOW(fd,n*22+16)=sd->status.inventory[i].card[1];
		WFIFOW(fd,n*22+18)=sd->status.inventory[i].card[2];
		WFIFOW(fd,n*22+20)=sd->status.inventory[i].card[3];
		WFIFOL(fd,n*22+22)=sd->status.inventory[i].limit;
		n++;
	}
	if(n){
		WFIFOW(fd,2)=4+n*22;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#endif
	if(arrow >= 0)
		clif_arrowequip(sd,arrow);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_equiplist(struct map_session_data *sd)
{
	int i,j,n,fd;

	nullpo_retv(sd);

	fd=sd->fd;

#if PACKETVER < 10
	WFIFOW(fd,0)=0xa4;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid<=0 || sd->inventory_data[i] == NULL || !itemdb_isequip2(sd->inventory_data[i]))
			continue;
		WFIFOW(fd,n*20+4)=i+2;
		if(sd->inventory_data[i]->view_id > 0)
			WFIFOW(fd,n*20+6)=sd->inventory_data[i]->view_id;
		else
			WFIFOW(fd,n*20+6)=sd->status.inventory[i].nameid;
		WFIFOB(fd,n*20+8)=sd->inventory_data[i]->type;
		WFIFOB(fd,n*20+9)=sd->status.inventory[i].identify;
		WFIFOW(fd,n*20+10)=pc_equippoint(sd,i);
		WFIFOW(fd,n*20+12)=sd->status.inventory[i].equip;
		WFIFOB(fd,n*20+14)=sd->status.inventory[i].attribute;
		WFIFOB(fd,n*20+15)=sd->status.inventory[i].refine;
		if(itemdb_isspecial(sd->status.inventory[n].card[0])) {
			if(sd->inventory_data[i]->flag.pet_egg) {
				WFIFOW(fd,n*20+16) = 0;
				WFIFOW(fd,n*20+18) = 0;
				WFIFOW(fd,n*20+20) = 0;
			} else {
				WFIFOW(fd,n*20+16) = sd->status.inventory[i].card[0];
				WFIFOW(fd,n*20+18) = sd->status.inventory[i].card[1];
				WFIFOW(fd,n*20+20) = sd->status.inventory[i].card[2];
			}
			WFIFOW(fd,n*20+22) = sd->status.inventory[i].card[3];
		} else {
			if(sd->status.inventory[i].card[0] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[0])) > 0)
				WFIFOW(fd,n*20+16)=j;
			else
				WFIFOW(fd,n*20+16)=sd->status.inventory[i].card[0];
			if(sd->status.inventory[i].card[1] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[1])) > 0)
				WFIFOW(fd,n*20+18)=j;
			else
				WFIFOW(fd,n*20+18)=sd->status.inventory[i].card[1];
			if(sd->status.inventory[i].card[2] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[2])) > 0)
				WFIFOW(fd,n*20+20)=j;
			else
				WFIFOW(fd,n*20+20)=sd->status.inventory[i].card[2];
			if(sd->status.inventory[i].card[3] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[3])) > 0)
				WFIFOW(fd,n*20+22)=j;
			else
				WFIFOW(fd,n*20+22)=sd->status.inventory[i].card[3];
		}
		n++;
	}
	if(n){
		WFIFOW(fd,2)=4+n*20;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#elif PACKETVER < 11
	WFIFOW(fd,0)=0x295;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid<=0 || sd->inventory_data[i] == NULL || !itemdb_isequip2(sd->inventory_data[i]))
			continue;
		WFIFOW(fd,n*24+4)=i+2;
		if(sd->inventory_data[i]->view_id > 0)
			WFIFOW(fd,n*24+6)=sd->inventory_data[i]->view_id;
		else
			WFIFOW(fd,n*24+6)=sd->status.inventory[i].nameid;
		WFIFOB(fd,n*24+8)=sd->inventory_data[i]->type;
		WFIFOB(fd,n*24+9)=sd->status.inventory[i].identify;
		WFIFOW(fd,n*24+10)=pc_equippoint(sd,i);
		WFIFOW(fd,n*24+12)=sd->status.inventory[i].equip;
		WFIFOB(fd,n*24+14)=sd->status.inventory[i].attribute;
		WFIFOB(fd,n*24+15)=sd->status.inventory[i].refine;
		if(itemdb_isspecial(sd->status.inventory[n].card[0])) {
			if(sd->inventory_data[i]->flag.pet_egg) {
				WFIFOW(fd,n*24+16) = 0;
				WFIFOW(fd,n*24+18) = 0;
				WFIFOW(fd,n*24+20) = 0;
			} else {
				WFIFOW(fd,n*24+16) = sd->status.inventory[i].card[0];
				WFIFOW(fd,n*24+18) = sd->status.inventory[i].card[1];
				WFIFOW(fd,n*24+20) = sd->status.inventory[i].card[2];
			}
			WFIFOW(fd,n*24+22) = sd->status.inventory[i].card[3];
		} else {
			if(sd->status.inventory[i].card[0] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[0])) > 0)
				WFIFOW(fd,n*24+16)=j;
			else
				WFIFOW(fd,n*24+16)=sd->status.inventory[i].card[0];
			if(sd->status.inventory[i].card[1] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[1])) > 0)
				WFIFOW(fd,n*24+18)=j;
			else
				WFIFOW(fd,n*24+18)=sd->status.inventory[i].card[1];
			if(sd->status.inventory[i].card[2] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[2])) > 0)
				WFIFOW(fd,n*24+20)=j;
			else
				WFIFOW(fd,n*24+20)=sd->status.inventory[i].card[2];
			if(sd->status.inventory[i].card[3] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[3])) > 0)
				WFIFOW(fd,n*24+22)=j;
			else
				WFIFOW(fd,n*24+22)=sd->status.inventory[i].card[3];
		}
		WFIFOL(fd,n*24+24)=sd->status.inventory[i].limit;
		n++;
	}
	if(n){
		WFIFOW(fd,2)=4+n*24;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#elif PACKETVER < 28
	WFIFOW(fd,0)=0x2d0;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid<=0 || sd->inventory_data[i] == NULL || !itemdb_isequip2(sd->inventory_data[i]))
			continue;
		WFIFOW(fd,n*26+4)=i+2;
		if(sd->inventory_data[i]->view_id > 0)
			WFIFOW(fd,n*26+6)=sd->inventory_data[i]->view_id;
		else
			WFIFOW(fd,n*26+6)=sd->status.inventory[i].nameid;
		WFIFOB(fd,n*26+8)=sd->inventory_data[i]->type;
		WFIFOB(fd,n*26+9)=sd->status.inventory[i].identify;
		WFIFOW(fd,n*26+10)=pc_equippoint(sd,i);
		WFIFOW(fd,n*26+12)=sd->status.inventory[i].equip;
		WFIFOB(fd,n*26+14)=sd->status.inventory[i].attribute;
		WFIFOB(fd,n*26+15)=sd->status.inventory[i].refine;
		if(itemdb_isspecial(sd->status.inventory[n].card[0])) {
			if(sd->inventory_data[i]->flag.pet_egg) {
				WFIFOW(fd,n*26+16) = 0;
				WFIFOW(fd,n*26+18) = 0;
				WFIFOW(fd,n*26+20) = 0;
			} else {
				WFIFOW(fd,n*26+16) = sd->status.inventory[i].card[0];
				WFIFOW(fd,n*26+18) = sd->status.inventory[i].card[1];
				WFIFOW(fd,n*26+20) = sd->status.inventory[i].card[2];
			}
			WFIFOW(fd,n*26+22) = sd->status.inventory[i].card[3];
		} else {
			if(sd->status.inventory[i].card[0] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[0])) > 0)
				WFIFOW(fd,n*26+16)=j;
			else
				WFIFOW(fd,n*26+16)=sd->status.inventory[i].card[0];
			if(sd->status.inventory[i].card[1] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[1])) > 0)
				WFIFOW(fd,n*26+18)=j;
			else
				WFIFOW(fd,n*26+18)=sd->status.inventory[i].card[1];
			if(sd->status.inventory[i].card[2] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[2])) > 0)
				WFIFOW(fd,n*26+20)=j;
			else
				WFIFOW(fd,n*26+20)=sd->status.inventory[i].card[2];
			if(sd->status.inventory[i].card[3] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[3])) > 0)
				WFIFOW(fd,n*26+22)=j;
			else
				WFIFOW(fd,n*26+22)=sd->status.inventory[i].card[3];
		}
		WFIFOL(fd,n*26+24)=sd->status.inventory[i].limit;
		WFIFOW(fd,n*26+28)=0;
		n++;
	}
	if(n){
		WFIFOW(fd,2)=4+n*26;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#else
	WFIFOW(fd,0)=0x2d0;
	for(i=0,n=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid<=0 || sd->inventory_data[i] == NULL || !itemdb_isequip2(sd->inventory_data[i]))
			continue;
		WFIFOW(fd,n*28+4)=i+2;
		if(sd->inventory_data[i]->view_id > 0)
			WFIFOW(fd,n*28+6)=sd->inventory_data[i]->view_id;
		else
			WFIFOW(fd,n*28+6)=sd->status.inventory[i].nameid;
		WFIFOB(fd,n*28+8)=sd->inventory_data[i]->type;
		WFIFOB(fd,n*28+9)=sd->status.inventory[i].identify;
		WFIFOW(fd,n*28+10)=pc_equippoint(sd,i);
		WFIFOW(fd,n*28+12)=sd->status.inventory[i].equip;
		WFIFOB(fd,n*28+14)=sd->status.inventory[i].attribute;
		WFIFOB(fd,n*28+15)=sd->status.inventory[i].refine;
		if(itemdb_isspecial(sd->status.inventory[n].card[0])) {
			if(sd->inventory_data[i]->flag.pet_egg) {
				WFIFOW(fd,n*28+16) = 0;
				WFIFOW(fd,n*28+18) = 0;
				WFIFOW(fd,n*28+20) = 0;
			} else {
				WFIFOW(fd,n*28+16) = sd->status.inventory[i].card[0];
				WFIFOW(fd,n*28+18) = sd->status.inventory[i].card[1];
				WFIFOW(fd,n*28+20) = sd->status.inventory[i].card[2];
			}
			WFIFOW(fd,n*28+22) = sd->status.inventory[i].card[3];
		} else {
			if(sd->status.inventory[i].card[0] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[0])) > 0)
				WFIFOW(fd,n*28+16)=j;
			else
				WFIFOW(fd,n*28+16)=sd->status.inventory[i].card[0];
			if(sd->status.inventory[i].card[1] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[1])) > 0)
				WFIFOW(fd,n*28+18)=j;
			else
				WFIFOW(fd,n*28+18)=sd->status.inventory[i].card[1];
			if(sd->status.inventory[i].card[2] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[2])) > 0)
				WFIFOW(fd,n*28+20)=j;
			else
				WFIFOW(fd,n*28+20)=sd->status.inventory[i].card[2];
			if(sd->status.inventory[i].card[3] > 0 && (j=itemdb_viewid(sd->status.inventory[i].card[3])) > 0)
				WFIFOW(fd,n*28+22)=j;
			else
				WFIFOW(fd,n*28+22)=sd->status.inventory[i].card[3];
		}
		WFIFOL(fd,n*28+24)=sd->status.inventory[i].limit;
		WFIFOW(fd,n*28+28)=0;
		WFIFOW(fd,n*28+30)=0;
		n++;
	}
	if(n){
		WFIFOW(fd,2)=4+n*28;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#endif

	return;
}

/*==========================================
 * �q�ɂ̏��Օi&���W�i���X�g
 *------------------------------------------
 */
static void clif_storageitemlist_sub(const int fd, struct item *item, int idx, int max)
{
	struct item_data *id;
	int i, len = 4;

	nullpo_retv(item);

#if PACKETVER < 5
	WFIFOW(fd,0)=0xa5;
	for(i = idx; i < max; i++) {
		if(item[i].nameid <= 0)
			continue;
		nullpo_retv(id = itemdb_search(item[i].nameid));
		if(itemdb_isequip2(id))
			continue;
		if(len + 10 > SOCKET_EMPTY_SIZE)
			break;

		WFIFOW(fd,len)=i+1;
		if(id->view_id > 0)
			WFIFOW(fd,len+2)=id->view_id;
		else
			WFIFOW(fd,len+2)=item[i].nameid;
		WFIFOB(fd,len+4)=id->type;
		WFIFOB(fd,len+5)=item[i].identify;
		WFIFOW(fd,len+6)=item[i].amount;
		if(item[i].equip == LOC_ARROW)
			WFIFOW(fd,len+8)=LOC_ARROW;
		else
			WFIFOW(fd,len+8)=0;
		len += 10;
	}
#elif PACKETVER < 14
	WFIFOW(fd,0)=0x1f0;
	for(i = idx; i < max; i++) {
		if(item[i].nameid <= 0)
			continue;
		nullpo_retv(id = itemdb_search(item[i].nameid));
		if(itemdb_isequip2(id))
			continue;
		if(len + 18 > SOCKET_EMPTY_SIZE)
			break;

		WFIFOW(fd,len)=i+1;
		if(id->view_id > 0)
			WFIFOW(fd,len+2)=id->view_id;
		else
			WFIFOW(fd,len+2)=item[i].nameid;
		WFIFOB(fd,len+4)=id->type;
		WFIFOB(fd,len+5)=item[i].identify;
		WFIFOW(fd,len+6)=item[i].amount;
		if(item[i].equip == LOC_ARROW)
			WFIFOW(fd,len+8)=LOC_ARROW;
		else
			WFIFOW(fd,len+8)=0;
		WFIFOW(fd,len+10)=item[i].card[0];
		WFIFOW(fd,len+12)=item[i].card[1];
		WFIFOW(fd,len+14)=item[i].card[2];
		WFIFOW(fd,len+16)=item[i].card[3];
		len += 18;
	}
#else
	WFIFOW(fd,0)=0x2ea;
	for(i = idx; i < max; i++) {
		if(item[i].nameid <= 0)
			continue;
		nullpo_retv(id = itemdb_search(item[i].nameid));
		if(itemdb_isequip2(id))
			continue;
		if(len + 22 > SOCKET_EMPTY_SIZE)
			break;

		WFIFOW(fd,len)=i+1;
		if(id->view_id > 0)
			WFIFOW(fd,len+2)=id->view_id;
		else
			WFIFOW(fd,len+2)=item[i].nameid;
		WFIFOB(fd,len+4)=id->type;
		WFIFOB(fd,len+5)=item[i].identify;
		WFIFOW(fd,len+6)=item[i].amount;
		if(item[i].equip == LOC_ARROW)
			WFIFOW(fd,len+8)=LOC_ARROW;
		else
			WFIFOW(fd,len+8)=0;
		WFIFOW(fd,len+10)=item[i].card[0];
		WFIFOW(fd,len+12)=item[i].card[1];
		WFIFOW(fd,len+14)=item[i].card[2];
		WFIFOW(fd,len+16)=item[i].card[3];
		WFIFOL(fd,len+18)=item[i].limit;
		len += 22;
	}
#endif
	if(len > 4) {
		WFIFOW(fd,2)=len;
		WFIFOSET(fd,len);
		if(i < max) {
			// ���ߕ��̓p�P�b�g����
			clif_storageitemlist_sub(fd, item, i, max);
		}
	}

	return;
}

/*==========================================
 * �q�ɂ̑������X�g
 *------------------------------------------
 */
static void clif_storageequiplist_sub(const int fd, struct item *item, int idx, int max)
{
	struct item_data *id;
	int i, j, len = 4;

	nullpo_retv(item);

#if PACKETVER < 10
	WFIFOW(fd,0)=0xa6;
	for(i = idx; i < max; i++) {
		if(item[i].nameid <= 0)
			continue;
		nullpo_retv(id = itemdb_search(item[i].nameid));
		if(!itemdb_isequip2(id))
			continue;
		if(len + 20 > SOCKET_EMPTY_SIZE)
			break;

		WFIFOW(fd,len)=i+1;
		if(id->view_id > 0)
			WFIFOW(fd,len+2)=id->view_id;
		else
			WFIFOW(fd,len+2)=item[i].nameid;
		WFIFOB(fd,len+4)=id->type;
		WFIFOB(fd,len+5)=item[i].identify;
		WFIFOW(fd,len+6)=id->equip;
		WFIFOW(fd,len+8)=item[i].equip;
		WFIFOB(fd,len+10)=item[i].attribute;
		WFIFOB(fd,len+11)=item[i].refine;
		if(itemdb_isspecial(item[i].card[0])) {
			if(id->flag.pet_egg) {
				WFIFOW(fd,len+12) = 0;
				WFIFOW(fd,len+14) = 0;
				WFIFOW(fd,len+16) = 0;
			} else {
				WFIFOW(fd,len+12) = item[i].card[0];
				WFIFOW(fd,len+14) = item[i].card[1];
				WFIFOW(fd,len+16) = item[i].card[2];
			}
			WFIFOW(fd,len+18) = item[i].card[3];
		} else {
			if(item[i].card[0] > 0 && (j=itemdb_viewid(item[i].card[0])) > 0)
				WFIFOW(fd,len+12)=j;
			else
				WFIFOW(fd,len+12)=item[i].card[0];
			if(item[i].card[1] > 0 && (j=itemdb_viewid(item[i].card[1])) > 0)
				WFIFOW(fd,len+14)=j;
			else
				WFIFOW(fd,len+14)=item[i].card[1];
			if(item[i].card[2] > 0 && (j=itemdb_viewid(item[i].card[2])) > 0)
				WFIFOW(fd,len+16)=j;
			else
				WFIFOW(fd,len+16)=item[i].card[2];
			if(item[i].card[3] > 0 && (j=itemdb_viewid(item[i].card[3])) > 0)
				WFIFOW(fd,len+18)=j;
			else
				WFIFOW(fd,len+18)=item[i].card[3];
		}
		len += 20;
	}
#elif PACKETVER < 11
	WFIFOW(fd,0)=0x296;
	for(i = idx; i < max; i++) {
		if(item[i].nameid <= 0)
			continue;
		nullpo_retv(id = itemdb_search(item[i].nameid));
		if(!itemdb_isequip2(id))
			continue;
		if(len + 24 > SOCKET_EMPTY_SIZE)
			break;

		WFIFOW(fd,len)=i+1;
		if(id->view_id > 0)
			WFIFOW(fd,len+2)=id->view_id;
		else
			WFIFOW(fd,len+2)=item[i].nameid;
		WFIFOB(fd,len+4)=id->type;
		WFIFOB(fd,len+5)=item[i].identify;
		WFIFOW(fd,len+6)=id->equip;
		WFIFOW(fd,len+8)=item[i].equip;
		WFIFOB(fd,len+10)=item[i].attribute;
		WFIFOB(fd,len+11)=item[i].refine;
		if(itemdb_isspecial(item[i].card[0])) {
			if(id->flag.pet_egg) {
				WFIFOW(fd,len+12) = 0;
				WFIFOW(fd,len+14) = 0;
				WFIFOW(fd,len+16) = 0;
			} else {
				WFIFOW(fd,len+12) = item[i].card[0];
				WFIFOW(fd,len+14) = item[i].card[1];
				WFIFOW(fd,len+16) = item[i].card[2];
			}
			WFIFOW(fd,len+18) = item[i].card[3];
		} else {
			if(item[i].card[0] > 0 && (j=itemdb_viewid(item[i].card[0])) > 0)
				WFIFOW(fd,len+12)=j;
			else
				WFIFOW(fd,len+12)=item[i].card[0];
			if(item[i].card[1] > 0 && (j=itemdb_viewid(item[i].card[1])) > 0)
				WFIFOW(fd,len+14)=j;
			else
				WFIFOW(fd,len+14)=item[i].card[1];
			if(item[i].card[2] > 0 && (j=itemdb_viewid(item[i].card[2])) > 0)
				WFIFOW(fd,len+16)=j;
			else
				WFIFOW(fd,len+16)=item[i].card[2];
			if(item[i].card[3] > 0 && (j=itemdb_viewid(item[i].card[3])) > 0)
				WFIFOW(fd,len+18)=j;
			else
				WFIFOW(fd,len+18)=item[i].card[3];
		}
		WFIFOL(fd,len+20)=item[i].limit;
		len += 24;
	}
#elif PACKETVER < 27
	WFIFOW(fd,0)=0x2d1;
	for(i = idx; i < max; i++) {
		if(item[i].nameid <= 0)
			continue;
		nullpo_retv(id = itemdb_search(item[i].nameid));
		if(!itemdb_isequip2(id))
			continue;
		if(len + 26 > SOCKET_EMPTY_SIZE)
			break;

		WFIFOW(fd,len)=i+1;
		if(id->view_id > 0)
			WFIFOW(fd,len+2)=id->view_id;
		else
			WFIFOW(fd,len+2)=item[i].nameid;
		WFIFOB(fd,len+4)=id->type;
		WFIFOB(fd,len+5)=item[i].identify;
		WFIFOW(fd,len+6)=id->equip;
		WFIFOW(fd,len+8)=item[i].equip;
		WFIFOB(fd,len+10)=item[i].attribute;
		WFIFOB(fd,len+11)=item[i].refine;
		if(itemdb_isspecial(item[i].card[0])) {
			if(id->flag.pet_egg) {
				WFIFOW(fd,len+12) = 0;
				WFIFOW(fd,len+14) = 0;
				WFIFOW(fd,len+16) = 0;
			} else {
				WFIFOW(fd,len+12) = item[i].card[0];
				WFIFOW(fd,len+14) = item[i].card[1];
				WFIFOW(fd,len+16) = item[i].card[2];
			}
			WFIFOW(fd,len+18) = item[i].card[3];
		} else {
			if(item[i].card[0] > 0 && (j=itemdb_viewid(item[i].card[0])) > 0)
				WFIFOW(fd,len+12)=j;
			else
				WFIFOW(fd,len+12)=item[i].card[0];
			if(item[i].card[1] > 0 && (j=itemdb_viewid(item[i].card[1])) > 0)
				WFIFOW(fd,len+14)=j;
			else
				WFIFOW(fd,len+14)=item[i].card[1];
			if(item[i].card[2] > 0 && (j=itemdb_viewid(item[i].card[2])) > 0)
				WFIFOW(fd,len+16)=j;
			else
				WFIFOW(fd,len+16)=item[i].card[2];
			if(item[i].card[3] > 0 && (j=itemdb_viewid(item[i].card[3])) > 0)
				WFIFOW(fd,len+18)=j;
			else
				WFIFOW(fd,len+18)=item[i].card[3];
		}
		WFIFOL(fd,len+20)=item[i].limit;
		WFIFOW(fd,len+24)=0;
		len += 26;
	}
#else
	WFIFOW(fd,0)=0x2d1;
	for(i = idx; i < max; i++) {
		if(item[i].nameid <= 0)
			continue;
		nullpo_retv(id = itemdb_search(item[i].nameid));
		if(!itemdb_isequip2(id))
			continue;
		if(len + 28 > SOCKET_EMPTY_SIZE)
			break;

		WFIFOW(fd,len)=i+1;
		if(id->view_id > 0)
			WFIFOW(fd,len+2)=id->view_id;
		else
			WFIFOW(fd,len+2)=item[i].nameid;
		WFIFOB(fd,len+4)=id->type;
		WFIFOB(fd,len+5)=item[i].identify;
		WFIFOW(fd,len+6)=id->equip;
		WFIFOW(fd,len+8)=item[i].equip;
		WFIFOB(fd,len+10)=item[i].attribute;
		WFIFOB(fd,len+11)=item[i].refine;
		if(itemdb_isspecial(item[i].card[0])) {
			if(id->flag.pet_egg) {
				WFIFOW(fd,len+12) = 0;
				WFIFOW(fd,len+14) = 0;
				WFIFOW(fd,len+16) = 0;
			} else {
				WFIFOW(fd,len+12) = item[i].card[0];
				WFIFOW(fd,len+14) = item[i].card[1];
				WFIFOW(fd,len+16) = item[i].card[2];
			}
			WFIFOW(fd,len+18) = item[i].card[3];
		} else {
			if(item[i].card[0] > 0 && (j=itemdb_viewid(item[i].card[0])) > 0)
				WFIFOW(fd,len+12)=j;
			else
				WFIFOW(fd,len+12)=item[i].card[0];
			if(item[i].card[1] > 0 && (j=itemdb_viewid(item[i].card[1])) > 0)
				WFIFOW(fd,len+14)=j;
			else
				WFIFOW(fd,len+14)=item[i].card[1];
			if(item[i].card[2] > 0 && (j=itemdb_viewid(item[i].card[2])) > 0)
				WFIFOW(fd,len+16)=j;
			else
				WFIFOW(fd,len+16)=item[i].card[2];
			if(item[i].card[3] > 0 && (j=itemdb_viewid(item[i].card[3])) > 0)
				WFIFOW(fd,len+18)=j;
			else
				WFIFOW(fd,len+18)=item[i].card[3];
		}
		WFIFOL(fd,len+20)=item[i].limit;
		WFIFOW(fd,len+24)=0;
		WFIFOW(fd,len+26)=0;
		len += 28;
	}
#endif
	if(len > 4) {
		WFIFOW(fd,2)=len;
		WFIFOSET(fd,len);
		if(i < max) {
			// ���ߕ��̓p�P�b�g����
			clif_storageequiplist_sub(fd, item, i, max);
		}
	}

	return;
}

/*==========================================
 * �J�v������ɗa���Ă�����Օi&���W�i���X�g
 *------------------------------------------
 */
void clif_storageitemlist(struct map_session_data *sd, struct storage *stor)
{
	nullpo_retv(sd);
	nullpo_retv(stor);

	clif_storageitemlist_sub(sd->fd, stor->store_item, 0, MAX_STORAGE);

	return;
}

/*==========================================
 * �J�v������ɗa���Ă��鑕�����X�g
 *------------------------------------------
 */
void clif_storageequiplist(struct map_session_data *sd, struct storage *stor)
{
	nullpo_retv(sd);
	nullpo_retv(stor);

	clif_storageequiplist_sub(sd->fd, stor->store_item, 0, MAX_STORAGE);

	return;
}

/*==========================================
 * �M���h�q�ɂɗa���Ă�����Օi&���W�i���X�g
 *------------------------------------------
 */
void clif_guildstorageitemlist(struct map_session_data *sd, struct guild_storage *stor)
{
	nullpo_retv(sd);
	nullpo_retv(stor);

	clif_storageitemlist_sub(sd->fd, stor->store_item, 0, MAX_GUILD_STORAGE);

	return;
}

/*==========================================
 * �M���h�q�ɂɗa���Ă��鑕���i���X�g
 *------------------------------------------
 */
void clif_guildstorageequiplist(struct map_session_data *sd, struct guild_storage *stor)
{
	nullpo_retv(sd);
	nullpo_retv(stor);

	clif_storageequiplist_sub(sd->fd, stor->store_item, 0, MAX_GUILD_STORAGE);

	return;
}

/*==========================================
 * GM�֏ꏊ��HP�ʒm
 *------------------------------------------
 */
static void clif_hpmeter(struct map_session_data *sd)
{
	struct map_session_data *dstsd;
	unsigned char buf[16], buf2[16];
	int i, len;

	nullpo_retv(sd);

	WBUFW(buf,0)=0x107;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->bl.x;
	WBUFW(buf,8)=sd->bl.y;

#if PACKETVER < 26
	WBUFW(buf2,0)=0x106;
	WBUFL(buf2,2)=sd->status.account_id;
	WBUFW(buf2,6)=(sd->status.max_hp > 0x7fff)? (short)((atn_bignumber)sd->status.hp * 0x7fff / sd->status.max_hp): sd->status.hp;
	WBUFW(buf2,8)=(sd->status.max_hp > 0x7fff)? 0x7fff: sd->status.max_hp;
	len = packet_db[0x106].len;
#else
	WBUFW(buf2,0)=0x80e;
	WBUFL(buf2,2)=sd->status.account_id;
	WBUFL(buf2,6)=sd->status.hp;
	WBUFL(buf2,10)=sd->status.max_hp;
	len = packet_db[0x80e].len;
#endif

	for(i=0;i<fd_max;i++){
		if(session[i] && (dstsd = (struct map_session_data *)session[i]->session_data) && dstsd->state.auth &&
		   dstsd->bl.m == sd->bl.m && pc_isGM(dstsd) && sd != dstsd){
			memcpy(WFIFOP(i,0),buf,packet_db[0x107].len);
			WFIFOSET(i,packet_db[0x107].len);
			memcpy(WFIFOP(i,0),buf2,len);
			WFIFOSET(i,len);
		}
	}

	return;
}

/*==========================================
 * �X�e�[�^�X�𑗂����
 * �\����p�����͂��̒��Ōv�Z���đ���
 *------------------------------------------
 */
void clif_updatestatus(struct map_session_data *sd, int type)
{
	int fd,len=8;

	nullpo_retv(sd);

	fd=sd->fd;

	WFIFOW(fd,0)=0xb0;
	WFIFOW(fd,2)=type;
	switch(type){
		// 00b0
	case SP_WEIGHT:
		pc_checkweighticon(sd);
		WFIFOW(fd,0)=0xb0;
		WFIFOW(fd,2)=type;
		WFIFOL(fd,4)=sd->weight;
		break;
	case SP_MAXWEIGHT:
		WFIFOL(fd,4)=sd->max_weight;
		break;
	case SP_SPEED:
		WFIFOL(fd,4)=sd->speed;
		break;
	case SP_BASELEVEL:
		WFIFOL(fd,4)=sd->status.base_level;
		break;
	case SP_JOBLEVEL:
		WFIFOL(fd,4)=sd->status.job_level;
		break;
	case SP_MANNER:
		WFIFOL(fd,4)=sd->status.manner;
		clif_changestatus(&sd->bl,SP_MANNER,sd->status.manner);
		break;
	case SP_STATUSPOINT:
		WFIFOL(fd,4)=sd->status.status_point;
		break;
	case SP_SKILLPOINT:
		WFIFOL(fd,4)=sd->status.skill_point;
		break;
	case SP_HIT:
		WFIFOL(fd,4)=sd->hit;
		break;
	case SP_FLEE1:
		WFIFOL(fd,4)=sd->flee;
		break;
	case SP_FLEE2:
		WFIFOL(fd,4)=sd->flee2/10;
		break;
	case SP_MAXHP:
		WFIFOL(fd,4)=sd->status.max_hp;
		break;
	case SP_MAXSP:
		WFIFOL(fd,4)=sd->status.max_sp;
		break;
	case SP_HP:
		WFIFOL(fd,4)=sd->status.hp;
		if(battle_config.disp_hpmeter)
			clif_hpmeter(sd);
		break;
	case SP_SP:
		WFIFOL(fd,4)=sd->status.sp;
		break;
	case SP_ASPD:
		WFIFOL(fd,4)=sd->amotion;
		break;
	case SP_ATK1:
		WFIFOL(fd,4)=sd->base_atk+sd->watk+sd->watk_;
		break;
	case SP_DEF1:
		WFIFOL(fd,4)=sd->def;
		break;
	case SP_MDEF1:
		WFIFOL(fd,4)=sd->mdef;
		break;
	case SP_ATK2:
		WFIFOL(fd,4)=sd->watk2+sd->watk_2;
		break;
	case SP_DEF2:
		WFIFOL(fd,4)=sd->def2;
		break;
	case SP_MDEF2:
		WFIFOL(fd,4)=sd->mdef2;
		break;
	case SP_CRITICAL:
		WFIFOL(fd,4)=sd->critical/10;
		break;
	case SP_MATK1:
		WFIFOL(fd,4)=sd->matk1;
		break;
	case SP_MATK2:
		WFIFOL(fd,4)=sd->matk2;
		break;

		// 00b1
	case SP_ZENY:
		WFIFOW(fd,0)=0xb1;
		WFIFOL(fd,4)=sd->status.zeny;
		break;
	case SP_BASEEXP:
		WFIFOW(fd,0)=0xb1;
		WFIFOL(fd,4)=sd->status.base_exp;
		break;
	case SP_JOBEXP:
		WFIFOW(fd,0)=0xb1;
		WFIFOL(fd,4)=sd->status.job_exp;
		break;
	case SP_NEXTBASEEXP:
		WFIFOW(fd,0)=0xb1;
		WFIFOL(fd,4)=pc_nextbaseexp(sd);
		break;
	case SP_NEXTJOBEXP:
		WFIFOW(fd,0)=0xb1;
		WFIFOL(fd,4)=pc_nextjobexp(sd);
		break;

		// 00be
	case SP_USTR:
	case SP_UAGI:
	case SP_UVIT:
	case SP_UINT:
	case SP_UDEX:
	case SP_ULUK:
		WFIFOW(fd,0)=0xbe;
		WFIFOB(fd,4)=pc_need_status_point(sd,type-SP_USTR+SP_STR);
		len=5;
		break;

		// 013a
	case SP_ATTACKRANGE:
		WFIFOW(fd,0)=0x13a;
		WFIFOW(fd,2)=sd->range.attackrange;
		len=4;
		break;

		// 0141
	case SP_STR:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.str;
		WFIFOL(fd,10)=sd->paramb[0] + sd->parame[0];
		len=14;
		break;
	case SP_AGI:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.agi;
		WFIFOL(fd,10)=sd->paramb[1] + sd->parame[1];
		len=14;
		break;
	case SP_VIT:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.vit;
		WFIFOL(fd,10)=sd->paramb[2] + sd->parame[2];
		len=14;
		break;
	case SP_INT:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.int_;
		WFIFOL(fd,10)=sd->paramb[3] + sd->parame[3];
		len=14;
		break;
	case SP_DEX:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.dex;
		WFIFOL(fd,10)=sd->paramb[4] + sd->parame[4];
		len=14;
		break;
	case SP_LUK:
		WFIFOW(fd,0)=0x141;
		WFIFOL(fd,2)=type;
		WFIFOL(fd,6)=sd->status.luk;
		WFIFOL(fd,10)=sd->paramb[5] + sd->parame[5];
		len=14;
		break;

	case SP_CARTINFO:
		WFIFOW(fd,0)=0x121;
		WFIFOW(fd,2)=sd->cart_num;
		WFIFOW(fd,4)=sd->cart_max_num;
		WFIFOL(fd,6)=sd->cart_weight;
		WFIFOL(fd,10)=sd->cart_max_weight;
		len=14;
		break;

	default:
		if(battle_config.error_log)
			printf("clif_updatestatus : make %d routine\n",type);
		return;
	}
	WFIFOSET(fd,len);

	return;
}

void clif_changestatus(struct block_list *bl, int type, int val)
{
	unsigned char buf[12];
	struct map_session_data *sd = NULL;

	nullpo_retv(bl);

	if(type == SP_MANNER && battle_config.nomanner_mode)
		return;

	if(bl->type == BL_PC)
		sd = (struct map_session_data *)bl;

	if(sd){
		WBUFW(buf,0)=0x1ab;
		WBUFL(buf,2)=bl->id;
		WBUFW(buf,6)=type;
		switch(type){
		case SP_MANNER:
			WBUFL(buf,8)=val;
			break;
		default:
			if(battle_config.error_log)
				printf("clif_changestatus : make %d routine\n",type);
			return;
		}
		clif_send(buf,packet_db[0x1ab].len,bl,AREA_WOS);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_changelook(struct block_list *bl, int type, int val)
{
	unsigned char buf[16];
	struct map_session_data *sd = NULL;

	nullpo_retv(bl);

	if(bl->type == BL_PC)
		sd = (struct map_session_data *)bl;

#if PACKETVER < 4
	if(sd && (type == LOOK_WEAPON || type == LOOK_SHIELD) && (sd->view_class == 22 || sd->view_class == 26 || view_class == 27))
		val = 0;
	WBUFW(buf,0)=0xc3;
	WBUFL(buf,2)=bl->id;
	WBUFB(buf,6)=type;
	WBUFB(buf,7)=val;
	clif_send(buf,packet_db[0xc3].len,bl,AREA);
#else
	if(bl->type == BL_SKILL && type == LOOK_BASE) {		// �g���b�v�̃��j�b�gID�ύX
		WBUFW(buf,0)=0xc3;
		WBUFL(buf,2)=bl->id;
		WBUFB(buf,6)=type;
		WBUFB(buf,7)=val;
		clif_send(buf,packet_db[0xc3].len,bl,AREA);
	} else {
		int idx=0, shield=0;
		if(sd) {
			switch (type) {
			case LOOK_SHOES:
				val = 0;
				if((idx = sd->equip_index[2]) >= 0 && sd->inventory_data[idx]) {
					if(sd->inventory_data[idx]->view_id > 0)
						val = sd->inventory_data[idx]->view_id;
					else
						val = sd->status.inventory[idx].nameid;
				}
				break;
			case LOOK_WEAPON:
			case LOOK_SHIELD:
				val = 0;
				type = LOOK_WEAPON;
				if(sd->view_class == 22 || sd->view_class == 26 || sd->view_class == 27)
					break;
				if((idx = sd->equip_index[9]) >= 0 && sd->inventory_data[idx]) {
					if(sd->inventory_data[idx]->view_id > 0)
						val = sd->inventory_data[idx]->view_id;
					else
						val = sd->status.inventory[idx].nameid;
				}
				if((idx = sd->equip_index[8]) >= 0 && idx != sd->equip_index[9] && sd->inventory_data[idx]) {
					if(sd->inventory_data[idx]->view_id > 0)
						shield = sd->inventory_data[idx]->view_id;
					else
						shield = sd->status.inventory[idx].nameid;
				}
				break;
			}
		}
		WBUFW(buf,0)=0x1d7;
		WBUFL(buf,2)=bl->id;
		WBUFB(buf,6)=type;
		WBUFW(buf,7)=val;
		WBUFW(buf,9)=shield;
		clif_send(buf,packet_db[0x1d7].len,bl,AREA);
	}
#endif
	return;
}

/*==========================================
 * ���F�p�P�b�g����t���i��{�F�������j
 *------------------------------------------
 */
void clif_send_clothcolor(struct block_list *bl)
{
	short color;
	unsigned char buf[16];

	nullpo_retv(bl);

	color = status_get_clothes_color(bl);
	if(color <= 0)
		return;

#if PACKETVER < 4
	WBUFW(buf,0) = 0xc3;
	WBUFL(buf,2) = bl->id;
	WBUFB(buf,6) = LOOK_CLOTHES_COLOR;
	WBUFB(buf,7) = (unsigned char)color;
	clif_send(buf,packet_db[0xc3].len,bl,AREA);
#else
	WBUFW(buf,0) = 0x1d7;
	WBUFL(buf,2) = bl->id;
	WBUFB(buf,6) = LOOK_CLOTHES_COLOR;
	WBUFW(buf,7) = color;
	WBUFW(buf,9) = 0;
	clif_send(buf,packet_db[0x1d7].len,bl,AREA);
#endif
	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_initialstatus(struct map_session_data *sd)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xbd;
	WFIFOW(fd,2)=sd->status.status_point;
	WFIFOB(fd,4)=(sd->status.str > 255)? 255:sd->status.str;
	WFIFOB(fd,5)=pc_need_status_point(sd,SP_STR);
	WFIFOB(fd,6)=(sd->status.agi > 255)? 255:sd->status.agi;
	WFIFOB(fd,7)=pc_need_status_point(sd,SP_AGI);
	WFIFOB(fd,8)=(sd->status.vit > 255)? 255:sd->status.vit;
	WFIFOB(fd,9)=pc_need_status_point(sd,SP_VIT);
	WFIFOB(fd,10)=(sd->status.int_ > 255)? 255:sd->status.int_;
	WFIFOB(fd,11)=pc_need_status_point(sd,SP_INT);
	WFIFOB(fd,12)=(sd->status.dex > 255)? 255:sd->status.dex;
	WFIFOB(fd,13)=pc_need_status_point(sd,SP_DEX);
	WFIFOB(fd,14)=(sd->status.luk > 255)? 255:sd->status.luk;
	WFIFOB(fd,15)=pc_need_status_point(sd,SP_LUK);

	WFIFOW(fd,16) = sd->base_atk + sd->watk + sd->watk_;
	WFIFOW(fd,18) = sd->watk2 + sd->watk_2;	// atk bonus
	WFIFOW(fd,20) = sd->matk1;
	WFIFOW(fd,22) = sd->matk2;
	WFIFOW(fd,24) = sd->def;	// def
	WFIFOW(fd,26) = sd->def2;
	WFIFOW(fd,28) = sd->mdef;	// mdef
	WFIFOW(fd,30) = sd->mdef2;
	WFIFOW(fd,32) = sd->hit;
	WFIFOW(fd,34) = sd->flee;
	WFIFOW(fd,36) = sd->flee2/10;
	WFIFOW(fd,38) = sd->critical/10;
	WFIFOW(fd,40) = sd->amotion;
	WFIFOW(fd,42) = 0;

	WFIFOSET(fd,packet_db[0xbd].len);

	clif_updatestatus(sd,SP_ATTACKRANGE);

	return;
}

/*==========================================
 * ���
 *------------------------------------------
 */
void clif_arrowequip(struct map_session_data *sd, int idx)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x13c;
	WFIFOW(fd,2)=idx+2;
	WFIFOSET(fd,packet_db[0x13c].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_arrow_fail(struct map_session_data *sd, unsigned short type)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x13b;
	WFIFOW(fd,2)=type;
	WFIFOSET(fd,packet_db[0x13b].len);

	return;
}

/*==========================================
 * �쐬�\ ��X�g���M
 *------------------------------------------
 */
void clif_arrow_create_list(struct map_session_data *sd)
{
	int i, view, idx, c = 0;
	int fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x1ad;

	for(i = 0; i < MAX_SKILL_ARROW_DB; i++) {
		if(skill_arrow_db[i].nameid > 0 &&
		   (idx = pc_search_inventory(sd,skill_arrow_db[i].nameid)) >= 0 &&
		   !sd->status.inventory[idx].equip)
		{
			if((view = itemdb_viewid(skill_arrow_db[i].nameid)) > 0)
				WFIFOW(fd,c*2+4) = view;
			else
				WFIFOW(fd,c*2+4) = skill_arrow_db[i].nameid;
			c++;
		}
	}
	WFIFOW(fd,2) = c * 2 + 4;
	WFIFOSET(fd, WFIFOW(fd,2));

	if(c > 0) {
		sd->skill_menu.id = AC_MAKINGARROW;
		sd->skill_menu.lv = 1;
	}

	return;
}

/*==========================================
 * �|�C�Y�j���O�E�F�|���I��
 *------------------------------------------*/
void clif_poison_list(struct map_session_data *sd, short lv)
{
	int i, view, idx, c = 0;
	int fd;
	static const int poison_list[] = {
		12717, 12718, 12719, 12720, 12721, 12722, 12723, 12724
	};

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x1ad;

	for(i = 0; i < sizeof(poison_list)/sizeof(poison_list[0]); i++) {
		if(poison_list[i] > 0 &&
			(idx = pc_search_inventory(sd, poison_list[i])) >= 0 &&
			!sd->status.inventory[idx].equip && sd->status.inventory[idx].identify)
		{
			if((view = itemdb_viewid(poison_list[i]) > 0))
				WFIFOW(fd,c*2+4) = view;
			else
				WFIFOW(fd,c*2+4) = poison_list[i];
			c++;
		}
	}
	WFIFOW(fd,2) = c * 2 + 4;
	WFIFOSET(fd, WFIFOW(fd,2));

	if(c > 0) {
		sd->skill_menu.id = GC_POISONINGWEAPON;
		sd->skill_menu.lv = lv;
	}

	return;
}

/*==========================================
 * ���[�f�B���O�X�y���u�b�N�I��
 *------------------------------------------*/
void clif_reading_sb_list(struct map_session_data *sd)
{
	int i, view, idx, c = 0;
	int fd;
	static const int sb_list[] = {
		6189, 6190, 6191, 6192, 6193, 6194,
		6195, 6196,	6197, 6198,	6199, 6200,
		6201, 6202, 6203, 6204, 6205
	};

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x1ad;

	for(i = 0; i < sizeof(sb_list)/sizeof(sb_list[0]); i++) {
		if(sb_list[i] > 0 &&
			(idx = pc_search_inventory(sd, sb_list[i])) >= 0 &&
			!sd->status.inventory[idx].equip && sd->status.inventory[idx].identify)
		{
			if((view = itemdb_viewid(sb_list[i]) > 0))
				WFIFOW(fd,c*2+4) = view;
			else
				WFIFOW(fd,c*2+4) = sb_list[i];
			c++;
		}
	}
	WFIFOW(fd,2) = c * 2 + 4;
	WFIFOSET(fd, WFIFOW(fd,2));

	if(c > 0) {
		sd->skill_menu.id = WL_READING_SB;
		sd->skill_menu.lv = 1;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_statusupack(struct map_session_data *sd, int type, int ok, int val)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xbc;
	WFIFOW(fd,2)=type;
	WFIFOB(fd,4)=ok;
	WFIFOB(fd,5)=val;
	WFIFOSET(fd,packet_db[0xbc].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_equipitemack(struct map_session_data *sd, int n, int pos, unsigned char ok)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
#if PACKETVER < 28
	WFIFOW(fd,0)=0xaa;
	WFIFOW(fd,2)=n+2;
	WFIFOW(fd,4)=pos;
	WFIFOB(fd,6)=ok;
	WFIFOSET(fd,packet_db[0xaa].len);
#else
	WFIFOW(fd,0)=0xaa;
	WFIFOW(fd,2)=n+2;
	WFIFOW(fd,4)=pos;
	if(ok && sd->inventory_data[n]->equip&LOC_HEAD_TMB)
		WFIFOW(fd,6)=sd->inventory_data[n]->look;
	else
		WFIFOW(fd,6)=0;
	WFIFOB(fd,8)=ok;
	WFIFOSET(fd,packet_db[0xaa].len);
#endif

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_unequipitemack(struct map_session_data *sd, int n, int pos, unsigned char ok)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xac;
	WFIFOW(fd,2)=n+2;
	WFIFOW(fd,4)=pos;
	WFIFOB(fd,6)=ok;
	WFIFOSET(fd,packet_db[0xac].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_misceffect(struct block_list* bl, int type)
{
	unsigned char buf[16];

	nullpo_retv(bl);

	WBUFW(buf,0) = 0x19b;
	WBUFL(buf,2) = bl->id;
	WBUFL(buf,6) = type;
	clif_send(buf,packet_db[0x19b].len,bl,AREA);

	return;
}

/*==========================================
 * bl���甭������G�t�F�N�g
 *------------------------------------------
 */
void clif_misceffect2(struct block_list *bl,int type)
{
	unsigned char buf[16];

	nullpo_retv(bl);

	WBUFW(buf,0)=0x1f3;
	WBUFL(buf,2)=bl->id;
	WBUFL(buf,6)=type;
	clif_send(buf,packet_db[0x1f3].len,bl,AREA);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_misceffect3(int fd, int id, int type)
{
	WFIFOW(fd,0)=0x1f3;
	WFIFOL(fd,2)=id;
	WFIFOL(fd,6)=type;
	WFIFOSET(fd,packet_db[0x1f3].len);

	return;
}

/*==========================================
 * �\���I�v�V�����ύX
 *------------------------------------------
 */
void clif_changeoption(struct block_list* bl)
{
	unsigned char buf[16];
	struct status_change *sc;
	struct map_session_data *sd = NULL;

	nullpo_retv(bl);

	sc = status_get_sc(bl);

	if(bl->type == BL_PC)
		sd = (struct map_session_data *)bl;
	if(sd)
		clif_changelook(&sd->bl,LOOK_BASE,sd->view_class);

#if PACKETVER < 7
	WBUFW(buf, 0) = 0x119;
	WBUFL(buf, 2) = bl->id;
	WBUFW(buf, 6) = (sc) ? sc->opt1 : OPT1_NORMAL;
	WBUFW(buf, 8) = (sc) ? sc->opt2 : OPT2_NORMAL;
	WBUFW(buf,10) = (sc) ? sc->option : (bl->type == BL_NPC) ? ((struct npc_data *)bl)->option : OPTION_NOTHING;
	WBUFB(buf,12) = (sd) ? (unsigned char)sd->status.karma : 0;
	clif_send(buf,packet_db[0x119].len,bl,AREA);
#else
	WBUFW(buf, 0) = 0x229;
	WBUFL(buf, 2) = bl->id;
	WBUFW(buf, 6) = (sc) ? sc->opt1 : OPT1_NORMAL;
	WBUFW(buf, 8) = (sc) ? sc->opt2 : OPT2_NORMAL;
	WBUFL(buf,10) = (sc) ? sc->option : (bl->type == BL_NPC) ? ((struct npc_data *)bl)->option : OPTION_NOTHING;
	WBUFB(buf,14) = (sd) ? (unsigned char)sd->status.karma : 0;
	clif_send(buf,packet_db[0x229].len,bl,AREA);
#endif

	return;
}

/*==========================================
 * �\���I�v�V�����ύX2
 *------------------------------------------
 */
void clif_changeoption2(struct block_list *bl)
{
	unsigned char buf[24];
	struct status_change *sc;
	int level;

	nullpo_retv(bl);

	sc = status_get_sc(bl);

	WBUFW(buf,0)  = 0x28a;
	WBUFL(buf,2)  = bl->id;
	WBUFL(buf,6)  = (sc) ? sc->option : (bl->type == BL_NPC) ? ((struct npc_data *)bl)->option : OPTION_NOTHING;
	WBUFL(buf,10) = ((level = status_get_lv(bl)) > 99)? 99: level;
	WBUFL(buf,14) = (sc) ? sc->opt3 : OPT3_NORMAL;
	clif_send(buf,packet_db[0x28a].len,bl,AREA);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_useitemack(struct map_session_data *sd, int idx, int amount, unsigned char ok)
{
	nullpo_retv(sd);

	if(!ok) {
		int fd=sd->fd;
		WFIFOW(fd,0)=0xa8;
		WFIFOW(fd,2)=idx+2;
		WFIFOW(fd,4)=amount;
		WFIFOB(fd,6)=ok;
		WFIFOSET(fd,packet_db[0xa8].len);
	} else {
#if PACKETVER < 3
		int fd=sd->fd;

		WFIFOW(fd,0)=0xa8;
		WFIFOW(fd,2)=idx+2;
		WFIFOW(fd,4)=amount;
		WFIFOB(fd,6)=ok;
		WFIFOSET(fd,packet_db[0xa8].len);
#else
		unsigned char buf[16];

		WBUFW(buf,0)=0x1c8;
		WBUFW(buf,2)=idx+2;
		if(sd->inventory_data[idx] && sd->inventory_data[idx]->view_id > 0)
			WBUFW(buf,4)=sd->inventory_data[idx]->view_id;
		else
			WBUFW(buf,4)=sd->status.inventory[idx].nameid;
		WBUFL(buf,6)=sd->bl.id;
		WBUFW(buf,10)=amount;
		WBUFB(buf,12)=ok;
		clif_send(buf,packet_db[0x1c8].len,&sd->bl,AREA);
#endif
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_createchat(struct map_session_data *sd, unsigned char fail)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xd6;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_db[0xd6].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_dispchat(struct chat_data *cd, int fd)
{
	unsigned char buf[80];	// �ő�title(60�o�C�g)+17

	if(cd==NULL || *cd->owner==NULL)
		return;

	WBUFW(buf, 0)=0xd7;
	WBUFW(buf, 2)=(unsigned short)(strlen(cd->title)+18);
	WBUFL(buf, 4)=(*cd->owner)->id;
	WBUFL(buf, 8)=cd->bl.id;
	WBUFW(buf,12)=cd->limit;
	WBUFW(buf,14)=cd->users;
	WBUFB(buf,16)=cd->pub;
	strncpy(WBUFP(buf,17),cd->title,strlen(cd->title)+1);
	if(fd >= 0) {
		memcpy(WFIFOP(fd,0),buf,WBUFW(buf,2));
		WFIFOSET(fd,WBUFW(buf,2));
	} else {
		clif_send(buf,WBUFW(buf,2),*cd->owner,AREA_WOSC);
	}

	return;
}

/*==========================================
 * chat�̏�ԕύX����
 * �O���̐l�p�Ɩ��߃R�[�h(d7->df)���Ⴄ����
 *------------------------------------------
 */
void clif_changechatstatus(struct chat_data *cd)
{
	unsigned char buf[80];	// �ő�title(60�o�C�g)+17

	if(cd==NULL || cd->usersd[0]==NULL)
		return;

	WBUFW(buf, 0)=0xdf;
	WBUFW(buf, 2)=(unsigned short)(strlen(cd->title)+18);
	WBUFL(buf, 4)=cd->usersd[0]->bl.id;
	WBUFL(buf, 8)=cd->bl.id;
	WBUFW(buf,12)=cd->limit;
	WBUFW(buf,14)=cd->users;
	WBUFB(buf,16)=cd->pub;
	strncpy(WBUFP(buf,17),cd->title,strlen(cd->title)+1);
	clif_send(buf,WBUFW(buf,2),&cd->usersd[0]->bl,CHAT);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_clearchat(struct chat_data *cd, int fd)
{
	unsigned char buf[8];

	nullpo_retv(cd);

	WBUFW(buf,0)=0xd8;
	WBUFL(buf,2)=cd->bl.id;
	if(fd >= 0) {
		memcpy(WFIFOP(fd,0),buf,packet_db[0xd8].len);
		WFIFOSET(fd,packet_db[0xd8].len);
	} else {
		clif_send(buf,packet_db[0xd8].len,*cd->owner,AREA_WOSC);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_joinchatfail(struct map_session_data *sd, unsigned char fail)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;

	WFIFOW(fd,0)=0xda;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_db[0xda].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_joinchatok(struct map_session_data *sd, struct chat_data* cd)
{
	int i,fd;

	nullpo_retv(sd);
	nullpo_retv(cd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xdb;
	WFIFOW(fd,2)=8+(28*cd->users);
	WFIFOL(fd,4)=cd->bl.id;
	for(i = 0;i < cd->users;i++){
		WFIFOL(fd,8+i*28) = (i!=0)||((*cd->owner)->type==BL_NPC);
		memcpy(WFIFOP(fd,8+i*28+4),cd->usersd[i]->status.name,24);
	}
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_addchat(struct chat_data* cd, struct map_session_data *sd)
{
	unsigned char buf[32];

	nullpo_retv(sd);
	nullpo_retv(cd);

	WBUFW(buf, 0) = 0xdc;
	WBUFW(buf, 2) = cd->users;
	memcpy(WBUFP(buf, 4),sd->status.name,24);
	clif_send(buf,packet_db[0xdc].len,&sd->bl,CHAT_WOS);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_changechatowner(struct chat_data* cd, struct map_session_data *sd)
{
	unsigned char buf[64];

	nullpo_retv(sd);
	nullpo_retv(cd);

	WBUFW(buf, 0) = 0xe1;
	WBUFL(buf, 2) = 1;
	memcpy(WBUFP(buf,6),cd->usersd[0]->status.name,24);
	WBUFW(buf,30) = 0xe1;
	WBUFL(buf,32) = 0;
	memcpy(WBUFP(buf,36),sd->status.name,24);
	clif_send(buf,packet_db[0xe1].len*2,&sd->bl,CHAT);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_leavechat(struct chat_data* cd, struct map_session_data *sd, unsigned char flag)
{
	unsigned char buf[32];

	nullpo_retv(sd);
	nullpo_retv(cd);

	WBUFW(buf, 0) = 0xdd;
	WBUFW(buf, 2) = cd->users-1;
	memcpy(WBUFP(buf,4),sd->status.name,24);
	WBUFB(buf,28) = flag;
	clif_send(buf,packet_db[0xdd].len,&sd->bl,CHAT);

	return;
}

/*==========================================
 * �������v����
 *------------------------------------------
 */
void clif_traderequest(struct map_session_data *sd, char *name)
{
	int fd;
#if PACKETVER >= 6
	struct map_session_data *target_sd;
#endif

	nullpo_retv(sd);

	fd=sd->fd;
#if PACKETVER < 6
	WFIFOW(fd,0)=0xe5;
	strncpy(WFIFOP(fd,2),name,24);
	WFIFOSET(fd,packet_db[0xe5].len);
#else
	nullpo_retv((target_sd = map_id2sd(sd->trade.partner)));

	WFIFOW(fd,0)=0x1f4;
	strncpy(WFIFOP(fd,2),name,24);
	WFIFOL(fd,26)=target_sd->status.char_id;	// �ǂ�������Ȃ�����Ƃ肠����char_id
	WFIFOW(fd,30)=target_sd->status.base_level;
	WFIFOSET(fd,packet_db[0x1f4].len);
#endif

	return;
}

/*==========================================
 * �������v������
 *------------------------------------------
 */
void clif_tradestart(struct map_session_data *sd, unsigned char type)
{
	int fd;
#if PACKETVER >= 6
	struct map_session_data *target_sd;
#endif

	nullpo_retv(sd);

	fd=sd->fd;
#if PACKETVER < 6
	WFIFOW(fd,0)=0xe7;
	WFIFOB(fd,2)=type;
	WFIFOSET(fd,packet_db[0xe7].len);
#else
	target_sd = map_id2sd(sd->trade.partner);
	WFIFOW(fd,0)=0x1f5;
	WFIFOB(fd,2)=type;
	WFIFOL(fd,3)=(target_sd!=NULL)?target_sd->status.char_id:0;	// �ǂ�������Ȃ�����Ƃ肠����char_id
	WFIFOW(fd,7)=(target_sd!=NULL)?target_sd->status.base_level:0;
	WFIFOSET(fd,packet_db[0x1f5].len);
#endif

	return;
}

/*==========================================
 * ���������̃A�C�e���ǉ�
 *------------------------------------------
 */
void clif_tradeadditem(struct map_session_data *sd,struct map_session_data *tsd, int idx, int amount)
{
	int fd,j;

	nullpo_retv(sd);
	nullpo_retv(tsd);

	fd=tsd->fd;
#if PACKETVER < 26
	WFIFOW(fd,0)=0xe9;
	WFIFOL(fd,2)=amount;
	if(idx==0){
		WFIFOW(fd,6) = 0; // type id
		WFIFOB(fd,8) = 0; // identify flag
		WFIFOB(fd,9) = 0; // attribute
		WFIFOB(fd,10)= 0; // refine
		WFIFOW(fd,11)= 0; // card (4w)
		WFIFOW(fd,13)= 0; // card (4w)
		WFIFOW(fd,15)= 0; // card (4w)
		WFIFOW(fd,17)= 0; // card (4w)
	} else {
		idx -= 2;
		if(sd->inventory_data[idx] && sd->inventory_data[idx]->view_id > 0)
			WFIFOW(fd,6) = sd->inventory_data[idx]->view_id;
		else
			WFIFOW(fd,6) = sd->status.inventory[idx].nameid;
		WFIFOB(fd,8) = sd->status.inventory[idx].identify;
		WFIFOB(fd,9) = sd->status.inventory[idx].attribute;
		WFIFOB(fd,10)= sd->status.inventory[idx].refine;
		if(itemdb_isspecial(sd->status.inventory[idx].card[0])) {
			if(sd->inventory_data[idx]->flag.pet_egg) {
				WFIFOW(fd,11) = 0;
				WFIFOW(fd,13) = 0;
				WFIFOW(fd,15) = 0;
			} else {
				WFIFOW(fd,11) = sd->status.inventory[idx].card[0];
				WFIFOW(fd,13) = sd->status.inventory[idx].card[1];
				WFIFOW(fd,15) = sd->status.inventory[idx].card[2];
			}
			WFIFOW(fd,17) = sd->status.inventory[idx].card[3];
		} else {
			if(sd->status.inventory[idx].card[0] > 0 && (j=itemdb_viewid(sd->status.inventory[idx].card[0])) > 0)
				WFIFOW(fd,11)= j;
			else
				WFIFOW(fd,11)= sd->status.inventory[idx].card[0];
			if(sd->status.inventory[idx].card[1] > 0 && (j=itemdb_viewid(sd->status.inventory[idx].card[1])) > 0)
				WFIFOW(fd,13)= j;
			else
				WFIFOW(fd,13)= sd->status.inventory[idx].card[1];
			if(sd->status.inventory[idx].card[2] > 0 && (j=itemdb_viewid(sd->status.inventory[idx].card[2])) > 0)
				WFIFOW(fd,15)= j;
			else
				WFIFOW(fd,15)= sd->status.inventory[idx].card[2];
			if(sd->status.inventory[idx].card[3] > 0 && (j=itemdb_viewid(sd->status.inventory[idx].card[3])) > 0)
				WFIFOW(fd,17)= j;
			else
				WFIFOW(fd,17)= sd->status.inventory[idx].card[3];
		}
	}
	WFIFOSET(fd,packet_db[0xe9].len);
#else
	WFIFOW(fd,0)=0x80f;
	if(idx==0){
		WFIFOW(fd,2) = 0; // type id
		WFIFOB(fd,4) = 0;	// type
		WFIFOL(fd,5) = amount;	// amount
		WFIFOB(fd,9) = 0; // identify flag
		WFIFOB(fd,10)= 0; // attribute
		WFIFOB(fd,11)= 0; // refine
		WFIFOW(fd,12)= 0; // card (4w)
		WFIFOW(fd,14)= 0; // card (4w)
		WFIFOW(fd,16)= 0; // card (4w)
		WFIFOW(fd,18)= 0; // card (4w)
	} else {
		idx -= 2;
		if(sd->inventory_data[idx] && sd->inventory_data[idx]->view_id > 0)
			WFIFOW(fd,2) = sd->inventory_data[idx]->view_id;
		else
			WFIFOW(fd,2) = sd->status.inventory[idx].nameid;
		WFIFOB(fd,4) = itemdb_type(sd->status.inventory[idx].nameid);
		WFIFOL(fd,5) = amount;
		WFIFOB(fd,9) = sd->status.inventory[idx].identify;
		WFIFOB(fd,10)= sd->status.inventory[idx].attribute;
		WFIFOB(fd,11)= sd->status.inventory[idx].refine;
		if(itemdb_isspecial(sd->status.inventory[idx].card[0])) {
			if(sd->inventory_data[idx]->flag.pet_egg) {
				WFIFOW(fd,12) = 0;
				WFIFOW(fd,14) = 0;
				WFIFOW(fd,16) = 0;
			} else {
				WFIFOW(fd,12) = sd->status.inventory[idx].card[0];
				WFIFOW(fd,14) = sd->status.inventory[idx].card[1];
				WFIFOW(fd,16) = sd->status.inventory[idx].card[2];
			}
			WFIFOW(fd,18) = sd->status.inventory[idx].card[3];
		} else {
			if(sd->status.inventory[idx].card[0] > 0 && (j=itemdb_viewid(sd->status.inventory[idx].card[0])) > 0)
				WFIFOW(fd,12)= j;
			else
				WFIFOW(fd,12)= sd->status.inventory[idx].card[0];
			if(sd->status.inventory[idx].card[1] > 0 && (j=itemdb_viewid(sd->status.inventory[idx].card[1])) > 0)
				WFIFOW(fd,14)= j;
			else
				WFIFOW(fd,14)= sd->status.inventory[idx].card[1];
			if(sd->status.inventory[idx].card[2] > 0 && (j=itemdb_viewid(sd->status.inventory[idx].card[2])) > 0)
				WFIFOW(fd,16)= j;
			else
				WFIFOW(fd,16)= sd->status.inventory[idx].card[2];
			if(sd->status.inventory[idx].card[3] > 0 && (j=itemdb_viewid(sd->status.inventory[idx].card[3])) > 0)
				WFIFOW(fd,18)= j;
			else
				WFIFOW(fd,18)= sd->status.inventory[idx].card[3];
		}
	}
	WFIFOSET(fd,packet_db[0x80f].len);
#endif

	return;
}

/*==========================================
 * �A�C�e���ǉ�����/���s
 *------------------------------------------
 */
void clif_tradeitemok(struct map_session_data *sd, unsigned short idx, unsigned char fail)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xea;
	WFIFOW(fd,2)=idx;
	WFIFOB(fd,4)=fail;
	WFIFOSET(fd,packet_db[0xea].len);

	return;
}

/*==========================================
 * ������ok����
 *------------------------------------------
 */
void clif_tradedeal_lock(struct map_session_data *sd, unsigned char fail)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xec;
	WFIFOB(fd,2)=fail; // 0=you 1=the other person
	WFIFOSET(fd,packet_db[0xec].len);

	return;
}

/*==========================================
 * ���������L�����Z������܂���
 *------------------------------------------
 */
void clif_tradecancelled(struct map_session_data *sd)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xee;
	WFIFOSET(fd,packet_db[0xee].len);

	return;
}

/*==========================================
 * ����������
 *------------------------------------------
 */
void clif_tradecompleted(struct map_session_data *sd, unsigned char fail)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xf0;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_db[0xf0].len);

	return;
}

/*==========================================
 * �J�v���q�ɂ̃A�C�e�������X�V
 *------------------------------------------
 */
void clif_updatestorageamount(struct map_session_data *sd, struct storage *stor)
{
	int fd;

	nullpo_retv(sd);
	nullpo_retv(stor);

	fd=sd->fd;
	WFIFOW(fd,0) = 0xf2;
	WFIFOW(fd,2) = stor->storage_amount;
	WFIFOW(fd,4) = MAX_STORAGE;
	WFIFOSET(fd,packet_db[0xf2].len);

	return;
}

/*==========================================
 * �J�v���q�ɂɃA�C�e����ǉ�����
 *------------------------------------------
 */
void clif_storageitemadded(struct map_session_data *sd, struct storage *stor, int idx, int amount)
{
	struct item_data *id;
	int fd,j;

	nullpo_retv(sd);
	nullpo_retv(stor);

	fd = sd->fd;
	id = itemdb_search(stor->store_item[idx].nameid);

#if PACKETVER < 7
	WFIFOW(fd,0) = 0xf4;
	WFIFOW(fd,2) = idx+1;
	WFIFOL(fd,4) = amount;
	if(id->view_id > 0)
		WFIFOW(fd,8) = id->view_id;
	else
		WFIFOW(fd,8) = stor->store_item[idx].nameid;
	WFIFOB(fd,10) = stor->store_item[idx].identify;
	WFIFOB(fd,11) = stor->store_item[idx].attribute;
	WFIFOB(fd,12) = stor->store_item[idx].refine;
	if(itemdb_isspecial(stor->store_item[idx].card[0])) {
		if(id->flag.pet_egg) {
			WFIFOW(fd,13) = 0;
			WFIFOW(fd,15) = 0;
			WFIFOW(fd,17) = 0;
		} else {
			WFIFOW(fd,13) = stor->store_item[idx].card[0];
			WFIFOW(fd,15) = stor->store_item[idx].card[1];
			WFIFOW(fd,17) = stor->store_item[idx].card[2];
		}
		WFIFOW(fd,19) = stor->store_item[idx].card[3];
	} else {
		if(stor->store_item[idx].card[0] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[0])) > 0)
			WFIFOW(fd,13) = j;
		else
			WFIFOW(fd,13) = stor->store_item[idx].card[0];
		if(stor->store_item[idx].card[1] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[1])) > 0)
			WFIFOW(fd,15) = j;
		else
			WFIFOW(fd,15)= stor->store_item[idx].card[1];
		if(stor->store_item[idx].card[2] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[2])) > 0)
			WFIFOW(fd,17) = j;
		else
			WFIFOW(fd,17)= stor->store_item[idx].card[2];
		if(stor->store_item[idx].card[3] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[3])) > 0)
			WFIFOW(fd,19) = j;
		else
			WFIFOW(fd,19) = stor->store_item[idx].card[3];
	}
	WFIFOSET(fd,packet_db[0xf4].len);
#else
	WFIFOW(fd,0) = 0x1c4;
	WFIFOW(fd,2) = idx+1;
	WFIFOL(fd,4) = amount;
	if(id->view_id > 0)
		WFIFOW(fd,8) = id->view_id;
	else
		WFIFOW(fd,8) = stor->store_item[idx].nameid;
	WFIFOB(fd,10) = id->type;
	WFIFOB(fd,11) = stor->store_item[idx].identify;
	WFIFOB(fd,12) = stor->store_item[idx].attribute;
	WFIFOB(fd,13) = stor->store_item[idx].refine;
	if(itemdb_isspecial(stor->store_item[idx].card[0])) {
		if(id->flag.pet_egg) {
			WFIFOW(fd,14) = 0;
			WFIFOW(fd,16) = 0;
			WFIFOW(fd,18) = 0;
		} else {
			WFIFOW(fd,14) = stor->store_item[idx].card[0];
			WFIFOW(fd,16) = stor->store_item[idx].card[1];
			WFIFOW(fd,18) = stor->store_item[idx].card[2];
		}
		WFIFOW(fd,20) = stor->store_item[idx].card[3];
	} else {
		if(stor->store_item[idx].card[0] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[0])) > 0)
			WFIFOW(fd,14) = j;
		else
			WFIFOW(fd,14) = stor->store_item[idx].card[0];
		if(stor->store_item[idx].card[1] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[1])) > 0)
			WFIFOW(fd,16) = j;
		else
			WFIFOW(fd,16) = stor->store_item[idx].card[1];
		if(stor->store_item[idx].card[2] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[2])) > 0)
			WFIFOW(fd,18) = j;
		else
			WFIFOW(fd,18)= stor->store_item[idx].card[2];
		if(stor->store_item[idx].card[3] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[3])) > 0)
			WFIFOW(fd,20) = j;
		else
			WFIFOW(fd,20) = stor->store_item[idx].card[3];
	}
	WFIFOSET(fd,packet_db[0x1c4].len);
#endif

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_updateguildstorageamount(struct map_session_data *sd, struct guild_storage *stor)
{
	int fd;

	nullpo_retv(sd);
	nullpo_retv(stor);

	fd=sd->fd;
	WFIFOW(fd,0) = 0xf2;
	WFIFOW(fd,2) = stor->storage_amount;
	WFIFOW(fd,4) = MAX_GUILD_STORAGE;
	WFIFOSET(fd,packet_db[0xf2].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_guildstorageitemadded(struct map_session_data *sd, struct guild_storage *stor, int idx, int amount)
{
	struct item_data *id;
	int fd,j;

	nullpo_retv(sd);
	nullpo_retv(stor);

	fd = sd->fd;
	id = itemdb_search(stor->store_item[idx].nameid);

#if PACKETVER < 7
	WFIFOW(fd,0) = 0xf4;
	WFIFOW(fd,2) = idx+1;
	WFIFOL(fd,4) = amount;
	if(id->view_id > 0)
		WFIFOW(fd,8) = id->view_id;
	else
		WFIFOW(fd,8) = stor->store_item[idx].nameid;
	WFIFOB(fd,10) = stor->store_item[idx].identify;
	WFIFOB(fd,11) = stor->store_item[idx].attribute;
	WFIFOB(fd,12) = stor->store_item[idx].refine;
	if(itemdb_isspecial(stor->store_item[idx].card[0])) {
		if(id->flag.pet_egg) {
			WFIFOW(fd,13) = 0;
			WFIFOW(fd,15) = 0;
			WFIFOW(fd,17) = 0;
		} else {
			WFIFOW(fd,13) = stor->store_item[idx].card[0];
			WFIFOW(fd,15) = stor->store_item[idx].card[1];
			WFIFOW(fd,17) = stor->store_item[idx].card[2];
		}
		WFIFOW(fd,19) = stor->store_item[idx].card[3];
	} else {
		if(stor->store_item[idx].card[0] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[0])) > 0)
			WFIFOW(fd,13) = j;
		else
			WFIFOW(fd,13) = stor->store_item[idx].card[0];
		if(stor->store_item[idx].card[1] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[1])) > 0)
			WFIFOW(fd,15) = j;
		else
			WFIFOW(fd,15) = stor->store_item[idx].card[1];
		if(stor->store_item[idx].card[2] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[2])) > 0)
			WFIFOW(fd,17) = j;
		else
			WFIFOW(fd,17) = stor->store_item[idx].card[2];
		if(stor->store_item[idx].card[3] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[3])) > 0)
			WFIFOW(fd,19) = j;
		else
			WFIFOW(fd,19) = stor->store_item[idx].card[3];
	}
	WFIFOSET(fd,packet_db[0xf4].len);
#else
	WFIFOW(fd,0) = 0x1c4;
	WFIFOW(fd,2) = idx+1;
	WFIFOL(fd,4) = amount;
	if(id->view_id > 0)
		WFIFOW(fd,8) = id->view_id;
	else
		WFIFOW(fd,8) = stor->store_item[idx].nameid;
	WFIFOB(fd,10) = id->type;
	WFIFOB(fd,11) = stor->store_item[idx].identify;
	WFIFOB(fd,12) = stor->store_item[idx].attribute;
	WFIFOB(fd,13) = stor->store_item[idx].refine;
	if(itemdb_isspecial(stor->store_item[idx].card[0])) {
		if(id->flag.pet_egg) {
			WFIFOW(fd,14) = 0;
			WFIFOW(fd,16) = 0;
			WFIFOW(fd,18) = 0;
		} else {
			WFIFOW(fd,14) = stor->store_item[idx].card[0];
			WFIFOW(fd,16) = stor->store_item[idx].card[1];
			WFIFOW(fd,18) = stor->store_item[idx].card[2];
		}
		WFIFOW(fd,20) = stor->store_item[idx].card[3];
	} else {
		if(stor->store_item[idx].card[0] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[0])) > 0)
			WFIFOW(fd,14) = j;
		else
			WFIFOW(fd,14) = stor->store_item[idx].card[0];
		if(stor->store_item[idx].card[1] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[1])) > 0)
			WFIFOW(fd,16) = j;
		else
			WFIFOW(fd,16) = stor->store_item[idx].card[1];
		if(stor->store_item[idx].card[2] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[2])) > 0)
			WFIFOW(fd,18) = j;
		else
			WFIFOW(fd,18) = stor->store_item[idx].card[2];
		if(stor->store_item[idx].card[3] > 0 && (j = itemdb_viewid(stor->store_item[idx].card[3])) > 0)
			WFIFOW(fd,20) = j;
		else
			WFIFOW(fd,20) = stor->store_item[idx].card[3];
	}
	WFIFOSET(fd,packet_db[0x1c4].len);
#endif

	return;
}

/*==========================================
 * �J�v���q�ɂ���A�C�e������苎��
 *------------------------------------------
 */
void clif_storageitemremoved(struct map_session_data *sd, int idx, int amount)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xf6;
	WFIFOW(fd,2)=idx+1;
	WFIFOL(fd,4)=amount;
	WFIFOSET(fd,packet_db[0xf6].len);

	return;
}

/*==========================================
 * �J�v���q�ɂ����
 *------------------------------------------
 */
void clif_storageclose(struct map_session_data *sd)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xf8;
	WFIFOSET(fd,packet_db[0xf8].len);

	return;
}

/*==========================================
 * �ʏ�U���G�t�F�N�g���_���[�W
 *------------------------------------------
 */
void clif_damage(struct block_list *src, struct block_list *dst, unsigned int tick, int sdelay, int ddelay, int damage, int div_, int type, int damage2)
{
	unsigned char buf[36];
	struct status_change *sc;

	nullpo_retv(src);
	nullpo_retv(dst);

	sc = status_get_sc(dst);

	if(type != 4 && dst->type == BL_PC) {
		if( ((struct map_session_data *)dst)->special_state.infinite_endure )
			type = 9;
		if( sc && (sc->data[SC_ENDURE].timer != -1 || sc->data[SC_BERSERK].timer != -1) && !map[dst->m].flag.gvg )
			type = 9;
	}
	if(sc && sc->data[SC_HALLUCINATION].timer != -1) {
		damage  = atn_rand() & 0x7fff;
		damage2 = atn_rand() & 0x7fff;
	}

#if PACKETVER < 13
	WBUFW(buf,0)=0x8a;
	WBUFL(buf,2)=src->id;
	WBUFL(buf,6)=dst->id;
	WBUFL(buf,10)=tick;
	WBUFL(buf,14)=sdelay;
	WBUFL(buf,18)=ddelay;
	WBUFW(buf,22)=(damage > 0x7fff)? 0x7fff: damage;
	WBUFW(buf,24)=div_;
	WBUFB(buf,26)=type;
	WBUFW(buf,27)=damage2;
	clif_send(buf,packet_db[0x8a].len,src,AREA);
#else
	WBUFW(buf,0)=0x2e1;
	WBUFL(buf,2)=src->id;
	WBUFL(buf,6)=dst->id;
	WBUFL(buf,10)=tick;
	WBUFL(buf,14)=sdelay;
	WBUFL(buf,18)=ddelay;
	WBUFL(buf,22)=damage;
	WBUFW(buf,26)=div_;
	WBUFB(buf,28)=type;
	WBUFL(buf,29)=damage2;
	clif_send(buf,packet_db[0x2e1].len,src,AREA);
#endif

	return;
}

/*==========================================
 * �A�C�e�����E��
 *------------------------------------------
 */
void clif_takeitem(struct block_list* src, int dst_id)
{
	unsigned char buf[32];

	nullpo_retv(src);

	WBUFW(buf, 0) = 0x8a;
	WBUFL(buf, 2) = src->id;
	WBUFL(buf, 6) = dst_id;
	WBUFB(buf,26) = 1;
	clif_send(buf,packet_db[0x8a].len,src,AREA);

	return;
}

/*==========================================
 * PC�\��
 *------------------------------------------
 */
static void clif_getareachar_pc(struct map_session_data* sd,struct map_session_data* dstsd)
{
	nullpo_retv(sd);
	nullpo_retv(dstsd);

	// ���S�ȃC���r�W�u�����[�h�Ȃ瑗�M���Ȃ�
	if(!battle_config.gm_perfect_hide || !pc_isinvisible(dstsd))
	{
		int len;
		if(dstsd->ud.walktimer != -1) {
			len = clif_set007b(dstsd,WFIFOP(sd->fd,0));
			WFIFOSET(sd->fd,len);
		} else {
			len = clif_set0078(dstsd,WFIFOP(sd->fd,0));
			WFIFOSET(sd->fd,len);
		}
		clif_send_clothcolor(&dstsd->bl);
	}

	if(dstsd->chatID) {
		struct chat_data *cd = map_id2cd(dstsd->chatID);
		if(cd && cd->usersd[0]==dstsd)
			clif_dispchat(cd,sd->fd);
	}
	if(dstsd->vender_id) {
		clif_showvendingboard(&dstsd->bl,dstsd->message,sd->fd);
	}

	if(dstsd->spiritball.num > 0) {
		clif_set01e1(sd->fd,dstsd,dstsd->spiritball.num);
	}

	if(dstsd->coin.num > 0) {
		clif_set01e1(sd->fd,dstsd,dstsd->coin.num);
	}

	if(sd->status.manner < 0)
		clif_changestatus(&sd->bl,SP_MANNER,sd->status.manner);
	if(dstsd->view_size != 0)
		clif_misceffect2(&dstsd->bl,422+dstsd->view_size);

	return;
}

/*==========================================
 * NPC�\��
 *------------------------------------------
 */
static void clif_getareachar_npc(struct map_session_data* sd,struct npc_data* nd)
{
	int len;

	nullpo_retv(sd);
	nullpo_retv(nd);

	if(nd->class_ < 0 ||(nd->flag&1 && nd->option != OPTION_HIDE) || nd->class_ == INVISIBLE_CLASS)
		return;

	len = clif_npc0078(nd,WFIFOP(sd->fd,0));
	WFIFOSET(sd->fd,len);

	if(nd->chat_id){
		clif_dispchat(map_id2cd(nd->chat_id),sd->fd);
	}

	if(nd->view_size!=0)
		clif_misceffect2(&nd->bl,422+nd->view_size);

	return;
}

/*==========================================
 * MOB�\��
 *------------------------------------------
 */
static void clif_getareachar_mob(struct map_session_data* sd, struct mob_data* md)
{
	int len;

	nullpo_retv(sd);
	nullpo_retv(md);

	if(md->ud.walktimer != -1){
		len = clif_mob007b(md,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	} else {
		len = clif_mob0078(md,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	}

	clif_send_clothcolor(&md->bl);
	if(md->view_size!=0)
		clif_misceffect2(&md->bl,422+md->view_size);

	return;
}

/*==========================================
 * PET�\��
 *------------------------------------------
 */
static void clif_getareachar_pet(struct map_session_data* sd, struct pet_data* pd)
{
	int len;

	nullpo_retv(sd);
	nullpo_retv(pd);

	if(pd->ud.walktimer != -1){
		len = clif_pet007b(pd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	} else {
		len = clif_pet0078(pd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	}

	clif_send_clothcolor(&pd->bl);
	if(pd->view_size!=0)
		clif_misceffect2(&pd->bl,422+pd->view_size);

	return;
}

/*==========================================
 * HOM�\��
 *------------------------------------------
 */
static void clif_getareachar_hom(struct map_session_data* sd, struct homun_data* hd)
{
	int len;

	nullpo_retv(sd);
	nullpo_retv(hd);

	if(hd->ud.walktimer != -1){
		len = clif_hom007b(hd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	} else {
		// 0x78���ƍ��W�Y�����N����
		//len = clif_hom0078(hd,WFIFOP(sd->fd,0));
		//WFIFOSET(sd->fd,len);
		len = clif_hom007b(hd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	}
	if(hd->view_size!=0)
		clif_misceffect2(&hd->bl,422+hd->view_size);

	return;
}

/*==========================================
 * MERC�\��
 *------------------------------------------
 */
static void clif_getareachar_merc(struct map_session_data* sd, struct merc_data* mcd)
{
	int len;

	nullpo_retv(sd);
	nullpo_retv(mcd);

	if(mcd->ud.walktimer != -1){
		len = clif_merc007b(mcd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	} else {
		// 0x78���ƍ��W�Y�����N����
		//len = clif_merc0078(mcd,WFIFOP(sd->fd,0));
		//WFIFOSET(sd->fd,len);
		len = clif_merc007b(mcd,WFIFOP(sd->fd,0));
		WFIFOSET(sd->fd,len);
	}
	if(mcd->view_size!=0)
		clif_misceffect2(&mcd->bl,422+mcd->view_size);

	return;
}

/*==========================================
 * ITEM�\��
 *------------------------------------------
 */
static void clif_getareachar_item(struct map_session_data* sd, struct flooritem_data* fitem)
{
	int view,fd;

	nullpo_retv(sd);
	nullpo_retv(fitem);

	// 009d <ID>.l <item ID>.w <identify flag>.B <X>.w <Y>.w <amount>.w <subX>.B <subY>.B
	fd=sd->fd;
	WFIFOW(fd,0)=0x9d;
	WFIFOL(fd,2)=fitem->bl.id;
	if((view = itemdb_viewid(fitem->item_data.nameid)) > 0)
		WFIFOW(fd,6)=view;
	else
		WFIFOW(fd,6)=fitem->item_data.nameid;
	WFIFOB(fd,8)=fitem->item_data.identify;
	WFIFOW(fd,9)=fitem->bl.x;
	WFIFOW(fd,11)=fitem->bl.y;
	WFIFOW(fd,13)=fitem->item_data.amount;
	WFIFOB(fd,15)=fitem->subx;
	WFIFOB(fd,16)=fitem->suby;
	WFIFOSET(fd,packet_db[0x9d].len);

	return;
}

/*==========================================
 * �ꏊ�X�L���G�t�F�N�g�����E�ɓ���
 *------------------------------------------
 */
static void clif_getareachar_skillunit(struct map_session_data *sd, struct skill_unit *unit)
{
	int fd;

	nullpo_retv(sd);
	nullpo_retv(unit);

	fd=sd->fd;

#if PACKETVER >= 3
	if(unit->group->unit_id == UNT_GRAFFITI) {	// �O���t�B�e�B
		WFIFOW(fd, 0)=0x1c9;
		WFIFOL(fd, 2)=unit->bl.id;
		WFIFOL(fd, 6)=unit->group->src_id;
		WFIFOW(fd,10)=unit->bl.x;
		WFIFOW(fd,12)=unit->bl.y;
		WFIFOB(fd,14)=unit->group->unit_id;
		WFIFOB(fd,15)=1;
		WFIFOB(fd,16)=1;
		memcpy(WFIFOP(fd,17),unit->group->valstr,80);
		WFIFOSET(fd,packet_db[0x1c9].len);

		return;
	}
#endif
	WFIFOW(fd, 0)=0x11f;
	WFIFOL(fd, 2)=unit->bl.id;
	WFIFOL(fd, 6)=unit->group->src_id;
	WFIFOW(fd,10)=unit->bl.x;
	WFIFOW(fd,12)=unit->bl.y;
	if(battle_config.trap_is_invisible && skill_unit_istrap(unit->group->unit_id))
		WFIFOB(fd,14)=UNT_ATTACK_SKILLS;
	else
		WFIFOB(fd,14)=unit->group->unit_id;
	WFIFOB(fd,15)=0;
	WFIFOSET(fd,packet_db[0x11f].len);

	if(unit->group->skill_id == WZ_ICEWALL)
		clif_set0192(fd,unit->bl.m,unit->bl.x,unit->bl.y,5);

	return;
}

/*==========================================
 * �ꏊ�X�L���G�t�F�N�g�����E���������
 *------------------------------------------
 */
static void clif_clearchar_skillunit(struct skill_unit *unit, int fd)
{
	nullpo_retv(unit);

	WFIFOW(fd, 0)=0x120;
	WFIFOL(fd, 2)=unit->bl.id;
	WFIFOSET(fd,packet_db[0x120].len);

	if(unit->group->skill_id == WZ_ICEWALL)
		clif_set0192(fd,unit->bl.m,unit->bl.x,unit->bl.y,unit->val2);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_01ac(struct block_list *bl)
{
	unsigned char buf[8];

	nullpo_retv(bl);

	WBUFW(buf, 0) = 0x1ac;
	WBUFL(buf, 2) = bl->id;
	clif_send(buf,packet_db[0x1ac].len,bl,AREA);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static int clif_getareachar(struct block_list* bl, va_list ap)
{
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd = va_arg(ap,struct map_session_data*));

	switch(bl->type) {
	case BL_PC:
		if(&sd->bl != bl)
			clif_getareachar_pc(sd,(struct map_session_data *)bl);
		break;
	case BL_NPC:
		clif_getareachar_npc(sd,(struct npc_data *)bl);
		break;
	case BL_MOB:
		clif_getareachar_mob(sd,(struct mob_data *)bl);
		break;
	case BL_PET:
		clif_getareachar_pet(sd,(struct pet_data *)bl);
		break;
	case BL_ITEM:
		clif_getareachar_item(sd,(struct flooritem_data *)bl);
		break;
	case BL_SKILL:
		clif_getareachar_skillunit(sd,(struct skill_unit *)bl);
		break;
	case BL_HOM:
		clif_getareachar_hom(sd,(struct homun_data *)bl);
		break;
	case BL_MERC:
		clif_getareachar_merc(sd,(struct merc_data *)bl);
		break;
	default:
		if(battle_config.error_log)
			printf("get area char ??? %d\n",bl->type);
		break;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_pcoutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd, *dstsd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd = va_arg(ap,struct map_session_data*));

	switch(bl->type) {
	case BL_PC:
		dstsd = (struct map_session_data *)bl;
		if(dstsd && sd != dstsd) {
			clif_clearchar_id(dstsd->bl.id,0,sd->fd);
			clif_clearchar_id(sd->bl.id,0,dstsd->fd);
			if(dstsd->chatID) {
				struct chat_data *cd = map_id2cd(dstsd->chatID);
				if(cd && cd->usersd[0] == dstsd)
					clif_dispchat(cd,sd->fd);
			}
			if(dstsd->vender_id) {
				clif_closevendingboard(&dstsd->bl,sd->fd);
			}
		}
		break;
	case BL_NPC:
		if( ((struct npc_data *)bl)->class_ != INVISIBLE_CLASS )
			clif_clearchar_id(bl->id,0,sd->fd);
		break;
	case BL_MOB:
	case BL_PET:
	case BL_HOM:
	case BL_MERC:
		clif_clearchar_id(bl->id,0,sd->fd);
		break;
	case BL_ITEM:
		clif_clearflooritem((struct flooritem_data *)bl,sd->fd);
		break;
	case BL_SKILL:
		clif_clearchar_skillunit((struct skill_unit *)bl,sd->fd);
		break;
	}

	return 0;
}

/*==========================================
 * ����
 *------------------------------------------
 */
int clif_pcinsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd, *dstsd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd = va_arg(ap,struct map_session_data*));

	switch(bl->type) {
	case BL_PC:
		dstsd = (struct map_session_data *)bl;
		if(sd != dstsd) {
			clif_getareachar_pc(sd,dstsd);
			clif_getareachar_pc(dstsd,sd);
		}
		break;
	case BL_NPC:
		clif_getareachar_npc(sd,(struct npc_data *)bl);
		break;
	case BL_MOB:
		clif_getareachar_mob(sd,(struct mob_data *)bl);
		break;
	case BL_PET:
		clif_getareachar_pet(sd,(struct pet_data *)bl);
		break;
	case BL_ITEM:
		clif_getareachar_item(sd,(struct flooritem_data *)bl);
		break;
	case BL_SKILL:
		clif_getareachar_skillunit(sd,(struct skill_unit *)bl);
		break;
	case BL_HOM:
		clif_getareachar_hom(sd,(struct homun_data *)bl);
		break;
	case BL_MERC:
		clif_getareachar_merc(sd,(struct merc_data *)bl);
		break;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_mobinsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct mob_data *md;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md = va_arg(ap,struct mob_data*));
	nullpo_retr(0, sd = (struct map_session_data *)bl);

	clif_getareachar_mob(sd,md);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_moboutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct mob_data *md;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md = va_arg(ap,struct mob_data*));
	nullpo_retr(0, sd = (struct map_session_data *)bl);

	clif_clearchar_id(md->bl.id,0,sd->fd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_petinsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct pet_data *pd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, pd = va_arg(ap,struct pet_data*));
	nullpo_retr(0, sd = (struct map_session_data *)bl);

	clif_getareachar_pet(sd,pd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_petoutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct pet_data *pd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, pd = va_arg(ap,struct pet_data*));
	nullpo_retr(0, sd = (struct map_session_data *)bl);

	clif_clearchar_id(pd->bl.id,0,sd->fd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_hominsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct homun_data *hd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, hd = va_arg(ap,struct homun_data*));
	nullpo_retr(0, sd = (struct map_session_data *)bl);

	clif_getareachar_hom(sd,hd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_homoutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct homun_data *hd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, hd = va_arg(ap,struct homun_data*));
	nullpo_retr(0, sd = (struct map_session_data *)bl);

	clif_clearchar_id(hd->bl.id,0,sd->fd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_mercinsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct merc_data *mcd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, mcd = va_arg(ap,struct merc_data*));
	nullpo_retr(0, sd = (struct map_session_data *)bl);

	clif_getareachar_merc(sd,mcd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int clif_mercoutsight(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	struct merc_data *mcd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, mcd = va_arg(ap,struct merc_data*));
	nullpo_retr(0, sd = (struct map_session_data *)bl);

	clif_clearchar_id(mcd->bl.id,0,sd->fd);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_skillinfo(struct map_session_data *sd, int skillid, int type, int range)
{
	int fd,id=0,skill_lv;
	int tk_ranker_bonus = 0;

	nullpo_retv(sd);

	if( skillid!=sd->skill_clone.id && (id=sd->status.skill[skillid].id) <= 0 )
		return;

	if(sd->status.class_==PC_CLASS_TK && pc_checkskill2(sd,TK_MISSION)>0 && sd->status.base_level>=90 &&
			sd->status.skill_point==0 && ranking_get_pc_rank(sd,RK_TAEKWON)>0)
		tk_ranker_bonus=1;

	if(tk_ranker_bonus && sd->status.skill[skillid].flag == 0)
		skill_lv = pc_get_skilltree_max(&sd->s_class,id);
	else if(skillid==sd->skill_clone.id){
		id = skillid;
		skill_lv = sd->status.skill[skillid].lv>sd->skill_clone.lv?sd->status.skill[skillid].lv:sd->skill_clone.lv;
	}else
		skill_lv = sd->status.skill[skillid].lv;

	fd=sd->fd;
	WFIFOW(fd,0)=0x147;
	WFIFOW(fd,2) = id;
	if(type < 0)
		WFIFOL(fd,4) = skill_get_inf(id);
	else
		WFIFOL(fd,4) = type;
	WFIFOW(fd,8) = skill_lv;
	WFIFOW(fd,10) = skill_get_sp(id,skill_lv);
	if(range < 0)
		WFIFOW(fd,12)= skill_get_fixed_range(&sd->bl,id,skill_lv);
	else
		WFIFOW(fd,12)= range;
	memset(WFIFOP(fd,14),0,24);
	if(!(skill_get_inf2(id)&0x01) || battle_config.quest_skill_learn == 1 || (battle_config.gm_allskill > 0 && pc_isGM(sd) >= battle_config.gm_allskill) )
		WFIFOB(fd,38) = (skill_lv < pc_get_skilltree_max(&sd->s_class,id) && skillid!=sd->skill_clone.id && sd->status.skill[skillid].flag == 0)? 1: 0;
	else
		WFIFOB(fd,38) = 0;
	WFIFOSET(fd,packet_db[0x147].len);

	return;
}

/*==========================================
 * �X�L�����X�g�𑗐M����
 * 16KB���߂ɔ�����WFIFORESERVE����
 *------------------------------------------
 */
void clif_skillinfoblock(struct map_session_data *sd)
{
	unsigned char buf[4+37*MAX_PCSKILL];
	int fd;
	int i,len=4,id,skill_lv,tk_ranker_bonus=0;

	nullpo_retv(sd);

	if(sd->status.class_==PC_CLASS_TK && pc_checkskill2(sd,TK_MISSION)>0 && sd->status.base_level>=90 &&
			sd->status.skill_point==0 && ranking_get_pc_rank(sd,RK_TAEKWON)>0)
		tk_ranker_bonus=1;

	fd=sd->fd;
	WBUFW(buf,0)=0x10f;
	for (i=0; i < MAX_PCSKILL; i++){
		if( (i==sd->skill_clone.id && (id=sd->skill_clone.id)!=0) || (i==sd->skill_reproduce.id && (id=sd->skill_reproduce.id)!=0) || (id=sd->status.skill[i].id)!=0 ){
			WBUFW(buf,len  ) = id;
			WBUFL(buf,len+2) = skill_get_inf(id);
			if(tk_ranker_bonus && sd->status.skill[i].flag==0)
				skill_lv = pc_get_skilltree_max(&sd->s_class,id);
			else if(i==sd->skill_clone.id)
				skill_lv = (sd->status.skill[i].lv > sd->skill_clone.lv)? sd->status.skill[i].lv: sd->skill_clone.lv;
			else if(i==sd->skill_reproduce.id)
				skill_lv = (sd->status.skill[i].lv > sd->skill_reproduce.lv)? sd->status.skill[i].lv: sd->skill_reproduce.lv;
			else
				skill_lv = sd->status.skill[i].lv;
			WBUFW(buf,len+6) = skill_lv;
			WBUFW(buf,len+8) = skill_get_sp(id,skill_lv);
			WBUFW(buf,len+10)= skill_get_fixed_range(&sd->bl,id,skill_lv);
			memset(WBUFP(buf,len+12),0,24);
			if(!(skill_get_inf2(id)&0x01) || battle_config.quest_skill_learn == 1 || (battle_config.gm_allskill > 0 && pc_isGM(sd) >= battle_config.gm_allskill) )
				WBUFB(buf,len+36) = (skill_lv < pc_get_skilltree_max(&sd->s_class,id) && i!=sd->skill_clone.id && i!=sd->skill_reproduce.id && sd->status.skill[i].flag == 0)? 1: 0;
			else
				WBUFB(buf,len+36) = 0;
			len+=37;
		}
	}
	WBUFW(buf,2)=len;
	WFIFORESERVE(fd,len);
	memcpy(WFIFOP(fd,0), WBUFP(buf,0), len);
	WFIFOSET(fd,len);

	return;
}

/*==========================================
 * �X�L������U��ʒm
 *------------------------------------------
 */
void clif_skillup(struct map_session_data *sd, int skill_num)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;

#if PACKETVER < 22
	WFIFOW(fd,0) = 0x10e;
	WFIFOW(fd,2) = skill_num;
	WFIFOW(fd,4) = sd->status.skill[skill_num].lv;
	WFIFOW(fd,6) = skill_get_sp(skill_num,sd->status.skill[skill_num].lv);
	WFIFOW(fd,8) = skill_get_fixed_range(&sd->bl,skill_num,sd->status.skill[skill_num].lv);
	WFIFOB(fd,10) = (sd->status.skill[skill_num].lv < pc_get_skilltree_max(&sd->s_class,sd->status.skill[skill_num].id))? 1: 0;
	WFIFOSET(fd,packet_db[0x10e].len);
#else
	WFIFOW(fd,0) = 0x7e1;
	WFIFOW(fd,2) = skill_num;
	WFIFOL(fd,4) = skill_get_inf(skill_num);
	WFIFOW(fd,8) = sd->status.skill[skill_num].lv;
	WFIFOW(fd,10) = skill_get_sp(skill_num,sd->status.skill[skill_num].lv);
	WFIFOW(fd,12) = skill_get_fixed_range(&sd->bl,skill_num,sd->status.skill[skill_num].lv);
	WFIFOB(fd,14) = (sd->status.skill[skill_num].lv < pc_get_skilltree_max(&sd->s_class,sd->status.skill[skill_num].id))? 1: 0;
	WFIFOSET(fd,packet_db[0x7e1].len);
#endif

	return;
}

/*==========================================
 * �X�L���r���G�t�F�N�g�𑗐M����
 *------------------------------------------
 */
void clif_skillcasting(struct block_list* bl,int src_id,int dst_id,int dst_x,int dst_y,int skill_num,int casttime)
{
	unsigned char buf[28];

#if PACKETVER < 24
	WBUFW(buf,0) = 0x13e;
	WBUFL(buf,2) = src_id;
	WBUFL(buf,6) = dst_id;
	WBUFW(buf,10) = dst_x;
	WBUFW(buf,12) = dst_y;
	WBUFW(buf,14) = skill_num;
	WBUFL(buf,16) = skill_get_pl(skill_num);	// ����
	WBUFL(buf,20) = casttime;
	clif_send(buf,packet_db[0x13e].len, bl, AREA);
#else
	WBUFW(buf,0) = 0x7fb;
	WBUFL(buf,2) = src_id;
	WBUFL(buf,6) = dst_id;
	WBUFW(buf,10) = dst_x;
	WBUFW(buf,12) = dst_y;
	WBUFW(buf,14) = skill_num;
	WBUFL(buf,16) = skill_get_pl(skill_num);	// ����
	WBUFL(buf,20) = casttime;
	WBUFB(buf,24) = 0;
	clif_send(buf,packet_db[0x7fb].len, bl, AREA);
#endif

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_skillcastcancel(struct block_list* bl)
{
	unsigned char buf[8];

	nullpo_retv(bl);

	WBUFW(buf,0) = 0x1b9;
	WBUFL(buf,2) = bl->id;
	clif_send(buf,packet_db[0x1b9].len, bl, AREA);

	return;
}

/*==========================================
 * �X�L���r�����s
 *------------------------------------------
 */
void clif_skill_fail(struct map_session_data *sd, int skill_id, unsigned char type, unsigned short btype)
{
	int fd;

	nullpo_retv(sd);

	if(type==0x4 && battle_config.display_delay_skill_fail==0)
		return;

	fd=sd->fd;
	WFIFOW(fd,0) = 0x110;
	WFIFOW(fd,2) = skill_id;
	WFIFOL(fd,4) = btype;
	WFIFOB(fd,8) = 0;
	WFIFOB(fd,9) = type;
	WFIFOSET(fd,packet_db[0x110].len);

	return;
}

/*==========================================
 * �X�L���U���G�t�F�N�g���_���[�W
 *------------------------------------------
 */
void clif_skill_damage(struct block_list *src,struct block_list *dst,
	unsigned int tick,int sdelay,int ddelay,int damage,int div_,int skill_id,int skill_lv,int type)
{
	unsigned char buf[36];
	struct status_change *sc;

	nullpo_retv(src);
	nullpo_retv(dst);

	sc = status_get_sc(dst);

	if(type != 5 && dst->type == BL_PC && ((struct map_session_data *)dst)->special_state.infinite_endure)
		type = 9;
	if(sc) {
		if(type != 5 && (sc->data[SC_ENDURE].timer != -1 || sc->data[SC_BERSERK].timer != -1))
			type = 9;
		if(sc->data[SC_HALLUCINATION].timer != -1)
			damage = atn_rand() & 0x7fff;
	}

#if PACKETVER < 3
	WBUFW(buf,0)=0x114;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFL(buf,8)=dst->id;
	WBUFL(buf,12)=tick;
	WBUFL(buf,16)=sdelay;
	WBUFL(buf,20)=ddelay;
	WBUFW(buf,24)=damage;
	WBUFW(buf,26)=skill_lv;
	WBUFW(buf,28)=div_;
	WBUFB(buf,30)=(type>0)?type:skill_get_hit(skill_id);
	clif_send(buf,packet_db[0x114].len,src,AREA);
#else
	WBUFW(buf,0)=0x1de;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFL(buf,8)=dst->id;
	WBUFL(buf,12)=tick;
	WBUFL(buf,16)=sdelay;
	WBUFL(buf,20)=ddelay;
	WBUFL(buf,24)=damage;
	WBUFW(buf,28)=skill_lv;
	WBUFW(buf,30)=div_;
	WBUFB(buf,32)=(type>0)?type:skill_get_hit(skill_id);
	clif_send(buf,packet_db[0x1de].len,src,AREA);
#endif

	return;
}

/*==========================================
 * ������΂��X�L���U���G�t�F�N�g���_���[�W
 *------------------------------------------
 */
/*void clif_skill_damage2(struct block_list *src,struct block_list *dst,
	unsigned int tick,int sdelay,int ddelay,int damage,int div,int skill_id,int skill_lv,int type)
{
	unsigned char buf[36];
	struct status_change *sc;

	nullpo_retr(0, src);
	nullpo_retr(0, dst);

	sc = status_get_sc(dst);

	if(type != 5 && dst->type == BL_PC && ((struct map_session_data *)dst)->special_state.infinite_endure)
		type = 9;
	if(sc) {
		if(type != 5 && (sc->data[SC_ENDURE].timer != -1 || sc->data[SC_BERSERK].timer != -1))
			type = 9;
		if(sc->data[SC_HALLUCINATION].timer != -1)
			damage = atn_rand() & 0x7fff;
	}

	WBUFW(buf,0)=0x115;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFL(buf,8)=dst->id;
	WBUFL(buf,12)=tick;
	WBUFL(buf,16)=sdelay;
	WBUFL(buf,20)=ddelay;
	WBUFW(buf,24)=dst->x;
	WBUFW(buf,26)=dst->y;
	WBUFW(buf,28)=damage;
	WBUFW(buf,30)=skill_lv;
	WBUFW(buf,32)=div;
	WBUFB(buf,34)=(type>0)?type:skill_get_hit(skill_id);
	clif_send(buf,packet_db[0x115].len,src,AREA);

	return;
}*/

/*==========================================
 * �x��/�񕜃X�L���G�t�F�N�g
 *------------------------------------------
 */
void clif_skill_nodamage(struct block_list *src,struct block_list *dst,int skill_id,int heal,int fail)
{
	unsigned char buf[16];

	nullpo_retv(src);
	nullpo_retv(dst);

	if(heal > 0x7fff && skill_id != NPC_SELFDESTRUCTION && skill_id != NPC_SELFDESTRUCTION2)
		heal=0x7fff;

	WBUFW(buf,0)=0x11a;
	WBUFW(buf,2)=skill_id;
	WBUFW(buf,4)=heal;
	WBUFL(buf,6)=dst->id;
	WBUFL(buf,10)=src->id;
	WBUFB(buf,14)=fail;
	clif_send(buf,packet_db[0x11a].len,src,AREA);

	return;
}

/*==========================================
 * �ꏊ�X�L���G�t�F�N�g
 *------------------------------------------
 */
void clif_skill_poseffect(struct block_list *src,int skill_id,int val,int x,int y,int tick)
{
	unsigned char buf[24];

	nullpo_retv(src);

	WBUFW(buf,0)=0x117;
	WBUFW(buf,2)=skill_id;
	WBUFL(buf,4)=src->id;
	WBUFW(buf,8)=val;
	WBUFW(buf,10)=x;
	WBUFW(buf,12)=y;
	WBUFL(buf,14)=tick;
	clif_send(buf,packet_db[0x117].len,src,AREA);

	return;
}

/*==========================================
 * �ꏊ�X�L���G�t�F�N�g�\��
 *------------------------------------------
 */
void clif_skill_setunit(struct skill_unit *unit)
{
	unsigned char buf[128];

	nullpo_retv(unit);

#if PACKETVER >= 3
	if(unit->group->unit_id == UNT_GRAFFITI) {	// �O���t�B�e�B
		WBUFW(buf, 0)=0x1c9;
		WBUFL(buf, 2)=unit->bl.id;
		WBUFL(buf, 6)=unit->group->src_id;
		WBUFW(buf,10)=unit->bl.x;
		WBUFW(buf,12)=unit->bl.y;
		WBUFB(buf,14)=unit->group->unit_id;
		WBUFB(buf,15)=1;
		WBUFB(buf,16)=1;
		memcpy(WBUFP(buf,17),unit->group->valstr,80);
		clif_send(buf,packet_db[0x1c9].len,&unit->bl,AREA);

		return;
	}
#endif
	WBUFW(buf, 0)=0x11f;
	WBUFL(buf, 2)=unit->bl.id;
	WBUFL(buf, 6)=unit->group->src_id;
	WBUFW(buf,10)=unit->bl.x;
	WBUFW(buf,12)=unit->bl.y;
	if(battle_config.trap_is_invisible && skill_unit_istrap(unit->group->unit_id))
		WBUFB(buf,14)=UNT_ATTACK_SKILLS;
	else
		WBUFB(buf,14)=unit->group->unit_id;
	WBUFB(buf,15)=0;
	clif_send(buf,packet_db[0x11f].len,&unit->bl,AREA);

	return;
}

/*==========================================
 * �ꏊ�X�L���G�t�F�N�g�폜
 *------------------------------------------
 */
void clif_skill_delunit(struct skill_unit *unit)
{
	unsigned char buf[8];

	nullpo_retv(unit);

	WBUFW(buf, 0)=0x120;
	WBUFL(buf, 2)=unit->bl.id;
	clif_send(buf,packet_db[0x120].len,&unit->bl,AREA);

	return;
}

/*==========================================
 * ���[�v�ꏊ�I��
 *------------------------------------------
 */
void clif_skill_warppoint(struct map_session_data *sd,int skill_num,
	const char *map1,const char *map2,const char *map3,const char *map4)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x11c;
	WFIFOW(fd,2)=skill_num;
	strncpy(WFIFOP(fd, 4),map1,16);
	strncpy(WFIFOP(fd,20),map2,16);
	strncpy(WFIFOP(fd,36),map3,16);
	strncpy(WFIFOP(fd,52),map4,16);
	WFIFOSET(fd,packet_db[0x11c].len);

	return;
}

/*==========================================
 * ��������
 *------------------------------------------
 */
void clif_skill_memo(struct map_session_data *sd, unsigned char flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x11e;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_db[0x11e].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_skill_teleportmessage(struct map_session_data *sd, unsigned short flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x189;
	WFIFOW(fd,2)=flag;
	WFIFOSET(fd,packet_db[0x189].len);

	return;
}

/*==========================================
 * �����X�^�[���
 *------------------------------------------
 */
void clif_skill_estimation(struct map_session_data *sd, struct block_list *bl)
{
	unsigned char buf[32];
	struct mob_data *md;
	int i;

	nullpo_retv(sd);
	nullpo_retv(bl);

	if(bl->type != BL_MOB || (md = (struct mob_data *)bl) == NULL)
		return;

	WBUFW(buf, 0)=0x18c;
	WBUFW(buf, 2)=mob_get_viewclass(md->class_);
	WBUFW(buf, 4)=mob_db[md->class_].lv;
	WBUFW(buf, 6)=mob_db[md->class_].size;
	WBUFL(buf, 8)=md->hp;
	WBUFW(buf,12)=status_get_def2(&md->bl);
	WBUFW(buf,14)=mob_db[md->class_].race;
	WBUFW(buf,16)=status_get_mdef2(&md->bl) - (mob_db[md->class_].vit>>1);
	WBUFW(buf,18)=status_get_elem_type(&md->bl);
	for(i=0; i<9 && i<ELE_MAX-1; i++)
		WBUFB(buf,20+i) = battle_attr_fix(100,i+1,md->def_ele);
	for( ; i<9; i++)
		WBUFB(buf,20+i) = 100;	// ELE_MAX��10��菬�����ꍇ

	if(sd->status.party_id>0) {
		clif_send(buf,packet_db[0x18c].len,&sd->bl,PARTY_AREA);
	} else {
		memcpy(WFIFOP(sd->fd,0),buf,packet_db[0x18c].len);
		WFIFOSET(sd->fd,packet_db[0x18c].len);
	}

	return;
}

/*==========================================
 * �A�C�e�������\���X�g
 *------------------------------------------
 */
void clif_skill_produce_mix_list(struct map_session_data *sd, int trigger)
{
	int i,c,view,fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd, 0)=0x18d;

	for(i=0,c=0;i<MAX_SKILL_PRODUCE_DB;i++){
		if( skill_can_produce_mix(sd,i,trigger) ){
			if((view = itemdb_viewid(skill_produce_db[i].nameid)) > 0)
				WFIFOW(fd,c*8+ 4)= view;
			else
				WFIFOW(fd,c*8+ 4)= skill_produce_db[i].nameid;
			WFIFOW(fd,c*8+ 6)= 0x0012;
			WFIFOL(fd,c*8+ 8)= sd->status.char_id;
			c++;
		}
	}
	WFIFOW(fd, 2)=c*8+8;
	WFIFOSET(fd,WFIFOW(fd,2));

	if(c > 0)
		sd->state.produce_flag = 1;

	return;
}

/*==========================================
 * �������X�g
 *------------------------------------------
 */
void clif_making_list(struct map_session_data *sd, int trigger)
{
	int i,c,view,fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd, 0)=0x25a;
	WFIFOW(fd, 4)=1;

	for(i=0,c=0;i<MAX_SKILL_PRODUCE_DB;i++){
		if( skill_can_produce_mix(sd,i,trigger) ){
			if((view = itemdb_viewid(skill_produce_db[i].nameid)) > 0)
				WFIFOW(fd,c*2+ 6)= view;
			else
				WFIFOW(fd,c*2+ 6)= skill_produce_db[i].nameid;
			c++;
		}
	}
	WFIFOW(fd, 2)=c*2+6;
	WFIFOSET(fd,WFIFOW(fd,2));

	if(c > 0)
		sd->state.produce_flag = 1;

	return;
}

/*==========================================
 * ��Ԉُ�A�C�R��/���b�Z�[�W�\��
 *------------------------------------------
 */
void clif_status_load(struct map_session_data *sd, int type, unsigned char flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x196;
	WFIFOW(fd,2)=type;
	WFIFOL(fd,4)=sd->bl.id;
	WFIFOB(fd,8)=flag;
	WFIFOSET(fd,packet_db[0x196].len);

	return;
}

/*==========================================
 * ��Ԉُ�A�C�R��/���b�Z�[�W�\���i�S�́j
 *------------------------------------------
 */
void clif_status_change(struct block_list *bl, int type, unsigned char flag, unsigned int tick, int val)
{
	unsigned char buf[32];

	nullpo_retv(bl);

	switch(type) {
		case SI_BLANK:
		case SI_MAXIMIZEPOWER:
		case SI_RIDING:
		case SI_FALCON:
		case SI_TRICKDEAD:
		case SI_BREAKARMOR:
		case SI_BREAKWEAPON:
		case SI_WEIGHT50:
		case SI_WEIGHT90:
		case SI_TENSIONRELAX:
		case SI_ELEMENTFIELD:
		case SI_AUTOBERSERK:
		case SI_RUN_STOP:
		case SI_READYSTORM:
		case SI_READYDOWN:
		case SI_READYTURN:
		case SI_READYCOUNTER:
		case SI_DODGE:
		case SI_DEVIL:
		case SI_MIRACLE:
		case SI_TIGEREYE:
			tick = 0;
			break;
	}

#if PACKETVER > 17
	WBUFW(buf,0)=0x43f;
	WBUFW(buf,2)=type;
	WBUFL(buf,4)=bl->id;
	WBUFB(buf,8)=flag;
	WBUFL(buf,9)=tick;
	WBUFL(buf,13)=val;
	WBUFL(buf,17)=0;
	WBUFL(buf,21)=0;
	clif_send(buf,packet_db[0x43f].len,bl,AREA);
#else
	WBUFW(buf,0)=0x196;
	WBUFW(buf,2)=type;
	WBUFL(buf,4)=bl->id;
	WBUFB(buf,8)=flag;
	clif_send(buf,packet_db[0x196].len,bl,AREA);
#endif

	return;
}

/*==========================================
 * ���b�Z�[�W�\��
 *------------------------------------------
 */
void clif_displaymessage(const int fd, const char* mes)
{
	size_t len = strlen(mes)+1;

	WFIFOW(fd,0)=0x8e;
	WFIFOW(fd,2)=(unsigned short)(4+len);
	strncpy(WFIFOP(fd,4),mes,len);
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_disp_onlyself(const int fd, const char *mes)
{
	size_t len = strlen(mes)+1;

	WFIFOW(fd,0)=0x17f;
	WFIFOW(fd,2)=(unsigned short)(4+len);
	memcpy(WFIFOP(fd,4),mes,len);
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 * �V�̐��𑗐M����
 *------------------------------------------
 */
void clif_GMmessage(struct block_list *bl, const char* mes, size_t len, int flag)
{
	unsigned char *buf = (unsigned char *)aMalloc(len+8);
	int lp = (flag&0x30)? 8: 4;

	WBUFW(buf,0) = 0x9a;
	WBUFW(buf,2) = (unsigned short)(len+lp);
	if(flag&0x80)
		memcpy(WBUFP(buf,4), "micc", 4);
	else if(flag&0x40)
		memcpy(WBUFP(buf,4), "tool", 4);
	else if(flag&0x20)
		memcpy(WBUFP(buf,4), "ssss", 4);
	else if(flag&0x10)
		memcpy(WBUFP(buf,4), "blue", 4);
	memcpy(WBUFP(buf,lp), mes, len);
	flag&=0x07;
	clif_send(buf, WBUFW(buf,2), bl,
		(flag==1)? ALL_SAMEMAP:
		(flag==2)? AREA:
		(flag==3)? SELF:
		ALL_CLIENT);
	aFree(buf);

	return;
}

/*==========================================
 * �O���[�o�����b�Z�[�W
 *------------------------------------------
 */
void clif_GlobalMessage(struct block_list *bl,const char *message)
{
	unsigned char buf[128];
	size_t len;

	nullpo_retv(bl);

	if(message == NULL)
		return;

	len=strlen(message)+1;
	if(len > sizeof(buf) - 8)
		len = sizeof(buf) - 8;

	WBUFW(buf,0)=0x8d;
	WBUFW(buf,2)=(unsigned short)(len+8);
	WBUFL(buf,4)=bl->id;
	strncpy(WBUFP(buf,8),message,len);
	clif_send(buf,WBUFW(buf,2),bl,AREA_CHAT_WOC);

	return;
}

/*==========================================
 * �V�̐��i�}���`�J���[�j�𑗐M
 *------------------------------------------
 */
void clif_announce(struct block_list *bl, const char* mes, size_t len, unsigned int color, int type, int size, int align, int pos_y, int flag)
{
	unsigned char *buf = (unsigned char *)aMalloc(len+16);

#if PACKETVER < 16
	WBUFW(buf,0) = 0x1c3;
	WBUFW(buf,2) = (unsigned short)(len+16);
	WBUFL(buf,4) = color;
	WBUFW(buf,8) = type;
	WBUFW(buf,10) = size;
	WBUFW(buf,12) = align;
	WBUFW(buf,14) = pos_y;
	memcpy(WBUFP(buf,16), mes, len);

	flag &= 0x07;
	clif_send(buf, WBUFW(buf,2), bl,
	          (flag == 1) ? ALL_SAMEMAP:
	          (flag == 2) ? AREA:
	          (flag == 3) ? SELF:
	          ALL_CLIENT);
#else
	WBUFW(buf,0) = 0x40c;
	WBUFW(buf,2) = (unsigned short)(len+16);
	WBUFL(buf,4) = color;
	WBUFW(buf,8) = type;
	WBUFW(buf,10) = size;
	WBUFW(buf,12) = align;
	WBUFW(buf,14) = pos_y;
	memcpy(WBUFP(buf,16), mes, len);

	flag &= 0x07;
	clif_send(buf, WBUFW(buf,2), bl,
	          (flag == 1) ? ALL_SAMEMAP:
	          (flag == 2) ? AREA:
	          (flag == 3) ? SELF:
	          ALL_CLIENT);
#endif
	aFree(buf);

	return;
}

/*==========================================
 * HPSP�񕜃G�t�F�N�g�𑗐M����
 *------------------------------------------
 */
void clif_heal(int fd, int type, int val)
{
	WFIFOW(fd,0)=0x13d;
	WFIFOW(fd,2)=type;
	WFIFOW(fd,4)=val;
	WFIFOSET(fd,packet_db[0x13d].len);

	return;
}

/*==========================================
 * ��������
 *------------------------------------------
 */
void clif_resurrection(struct block_list *bl, unsigned short type)
{
	unsigned char buf[8];

	nullpo_retv(bl);

	WBUFW(buf,0)=0x148;
	WBUFL(buf,2)=bl->id;
	WBUFW(buf,6)=type;
	clif_send(buf,packet_db[0x148].len,bl,(type == 1) ? AREA : AREA_WOS);

	return;
}

/*==========================================
 * PvP�EGvG����
 *------------------------------------------
 */
void clif_set0199(int fd, unsigned short type)
{
	WFIFOW(fd,0)=0x199;
	WFIFOW(fd,2)=type;
	WFIFOSET(fd,packet_db[0x199].len);

	return;
}

/*==========================================
 * PvP�EGvG�����iMAP�S�́j
 *------------------------------------------
 */
void clif_send0199(int m, unsigned short type)
{
	struct block_list bl;
	unsigned char buf[4];

	memset(&bl,0,sizeof(bl));
	bl.m = m;

	WBUFW(buf,0)=0x199;
	WBUFW(buf,2)=type;
	clif_send(buf,packet_db[0x199].len,&bl,ALL_SAMEMAP);

	return;
}

/*==========================================
 * PVP����
 *------------------------------------------
 */
void clif_pvpset(struct map_session_data *sd, int pvprank, int pvpnum, char type)
{
	nullpo_retv(sd);

	if(type == 2) {
		int fd = sd->fd;

		WFIFOW(fd,0)  = 0x19a;
		WFIFOL(fd,2)  = sd->bl.id;
		WFIFOL(fd,6)  = pvprank;
		WFIFOL(fd,10) = pvpnum;
		WFIFOSET(fd,packet_db[0x19a].len);
	} else {
		unsigned char buf[16];

		WBUFW(buf,0) = 0x19a;
		WBUFL(buf,2) = sd->bl.id;
		if(sd->sc.option&(OPTION_HIDE | OPTION_CLOAKING | OPTION_SPECIALHIDING))
			WBUFL(buf,6) = 0xffffffff;
		else
			WBUFL(buf,6) = pvprank;
		WBUFL(buf,10) = pvpnum;
		clif_send(buf,packet_db[0x19a].len,&sd->bl,(type == 0) ? SELF : ALL_SAMEMAP);
	}

	return;
}

/*==========================================
 * ���B�G�t�F�N�g�𑗐M����
 *------------------------------------------
 */
void clif_refine(int fd, unsigned short fail, int idx, int val)
{
	WFIFOW(fd,0)=0x188;
	WFIFOW(fd,2)=fail;
	WFIFOW(fd,4)=idx+2;
	WFIFOW(fd,6)=val;
	WFIFOSET(fd,packet_db[0x188].len);

	return;
}

/*==========================================
 * Wis�𑗐M����
 *------------------------------------------
 */
void clif_wis_message(int fd,char *nick,char *mes, int mes_len)
{
	WFIFOW(fd,0)=0x97;

#if PACKETVER < 23
	WFIFOW(fd,2)=mes_len + 28;
	memcpy(WFIFOP(fd,4),nick,24);
	memcpy(WFIFOP(fd,28),mes,mes_len);
	WFIFOSET(fd,WFIFOW(fd,2));
#else
	WFIFOW(fd,2)=mes_len + 32;
	memcpy(WFIFOP(fd,4),nick,24);
	WFIFOL(fd,28)=0;	// Unknown
	memcpy(WFIFOP(fd,32),mes,mes_len);
	WFIFOSET(fd,WFIFOW(fd,2));
#endif

	return;
}

/*==========================================
 * Wis�̑��M���ʂ𑗐M����
 *------------------------------------------
 */
void clif_wis_end(int fd, unsigned short flag)
{
	WFIFOW(fd,0)=0x98;
	WFIFOW(fd,2)=flag;
	WFIFOSET(fd,packet_db[0x98].len);

	return;
}

/*==========================================
 * �L����ID���O�������ʂ𑗐M����
 *------------------------------------------
 */
void clif_solved_charname(struct map_session_data *sd, int char_id)
{
	char *p;

	nullpo_retv(sd);

	p = map_charid2nick(char_id);
	if(p!=NULL){
		int fd=sd->fd;
		WFIFOW(fd,0)=0x194;
		WFIFOL(fd,2)=char_id;
		memcpy(WFIFOP(fd,6), p, 24);
		WFIFOSET(fd,packet_db[0x194].len);
	}else{
		map_reqchariddb(sd,char_id,1);
		chrif_searchcharid(char_id);
	}

	return;
}

/*==========================================
 * �J�[�h�̑}���\���X�g��Ԃ�
 *------------------------------------------
 */
static void clif_use_card(struct map_session_data *sd, int idx)
{
	int i, j, c = 0;
	int ep, fd;

	nullpo_retv(sd);

	if (idx < 0 || idx >= MAX_INVENTORY)
		return;

	if(sd->inventory_data[idx] == NULL)
		return;

	ep = sd->inventory_data[idx]->equip;
	fd = sd->fd;

	WFIFOW(fd,0)=0x17b;
	for(i = 0; i < MAX_INVENTORY; i++) {
		if(sd->inventory_data[i] == NULL)
			continue;
		if(!itemdb_isarmor(sd->inventory_data[i]->nameid) && !itemdb_isweapon(sd->inventory_data[i]->nameid))	// ����h���Ȃ�
			continue;
		if(itemdb_isspecial(sd->status.inventory[i].card[0]))
			continue;
		if(sd->status.inventory[i].identify == 0)	// ���Ӓ�
			continue;
		if((sd->inventory_data[i]->equip&ep) == 0)	// ���������Ⴄ
			continue;
		if(itemdb_isweapon(sd->inventory_data[i]->nameid) && ep == LOC_LARM)	// ���J�[�h�Ɨ��蕐��
			continue;

		for(j=0; j<sd->inventory_data[i]->slot; j++) {
			if(sd->status.inventory[i].card[j] == 0) {
				WFIFOW(fd,4+c*2) = i + 2;
				c++;
				break;
			}
		}
	}
	WFIFOW(fd,2)=4+c*2;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 * �J�[�h�̑}���I��
 *------------------------------------------
 */
void clif_insert_card(struct map_session_data *sd, int idx_equip, int idx_card, unsigned char flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x17d;
	WFIFOW(fd,2)=idx_equip+2;
	WFIFOW(fd,4)=idx_card+2;
	WFIFOB(fd,6)=flag;
	WFIFOSET(fd,packet_db[0x17d].len);

	return;
}

/*==========================================
 * �Ӓ�\�A�C�e�����X�g���M
 *------------------------------------------
 */
void clif_item_identify_list(struct map_session_data *sd)
{
	int i,c,fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x177;
	for(i=c=0;i<MAX_INVENTORY;i++){
		if(sd->status.inventory[i].nameid > 0 && sd->status.inventory[i].identify!=1){
			WFIFOW(fd,c*2+4)=i+2;
			c++;
		}
	}
	if(c > 0) {
		WFIFOW(fd,2)=c*2+4;
		WFIFOSET(fd,WFIFOW(fd,2));
	}

	return;
}

/*==========================================
 * �Ӓ茋��
 *------------------------------------------
 */
void clif_item_identified(struct map_session_data *sd, int idx, unsigned char flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd, 0)=0x179;
	WFIFOW(fd, 2)=idx+2;
	WFIFOB(fd, 4)=flag;
	WFIFOSET(fd,packet_db[0x179].len);

	return;
}

/*==========================================
 * �{�q�v���_�C�A���O�\��
 *------------------------------------------
 */
static void clif_baby_req_display(struct map_session_data *dst_sd, struct map_session_data *src_sd, int p_id)
{
	int fd;

	nullpo_retv(dst_sd);

	fd=dst_sd->fd;
	WFIFOW(fd,0) = 0x1f6;
	WFIFOL(fd,2) = src_sd->status.account_id;
	WFIFOL(fd,6) = p_id;
	memcpy(WFIFOP(fd,10), src_sd->status.name, 24);
	WFIFOSET(fd,packet_db[0x1f6].len);

	return;
}

/*==========================================
 * �{�q�`�F�b�N���s
 *------------------------------------------
 */
void clif_baby_req_fail(struct map_session_data *sd, int type)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0) = 0x216;
	WFIFOL(fd,2) = type;
	WFIFOSET(fd,packet_db[0x216].len);

	return;
}

/*==========================================
 * �{�q�^�[�Q�b�g�\��
 *------------------------------------------
 */
// 0x1f8�͎g���ĂȂ������H
/*void clif_baby_target_display(struct map_session_data *sd)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0) = 0x1f8;
	WFIFOSET(fd,packet_db[0x1f8].len);

	return;
}*/

/*==========================================
 * �C���\�A�C�e�����X�g���M
 *------------------------------------------
 */
void clif_item_repair_list(struct map_session_data *sd, struct map_session_data *dstsd)
{
	int i,c,fd;
	int nameid;

	nullpo_retv(sd);
	nullpo_retv(dstsd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x1fc;
	for(i=c=0;i<MAX_INVENTORY;i++){
		if((nameid=dstsd->status.inventory[i].nameid) > 0 && dstsd->status.inventory[i].attribute!=0){
			WFIFOW(fd,c*13+4) = i;
			WFIFOW(fd,c*13+6) = nameid;
			WFIFOL(fd,c*13+8) = sd->status.char_id;
			WFIFOL(fd,c*13+12)= dstsd->status.char_id;
			WFIFOB(fd,c*13+16)= c;
			c++;
		}
	}
	if(c > 0) {
		WFIFOW(fd,2)=c*13+4;
		WFIFOSET(fd,WFIFOW(fd,2));
		sd->state.produce_flag = 1;
		sd->repair_target=dstsd->bl.id;
	}else
		clif_skill_fail(sd,sd->ud.skillid,0,0);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_item_repaireffect(struct map_session_data *sd, unsigned char flag, int nameid)
{
	int view,fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd, 0)=0x1fe;
	if((view = itemdb_viewid(nameid)) > 0)
		WFIFOW(fd, 2)=view;
	else
		WFIFOW(fd, 2)=nameid;
	WFIFOB(fd, 4)=flag;
	WFIFOSET(fd,packet_db[0x1fe].len);

	if(sd->repair_target && sd->bl.id != sd->repair_target && flag==0) {	// ���������瑊��ɂ��ʒm
		struct map_session_data *dstsd = map_id2sd(sd->repair_target);
		sd->repair_target=0;
		if(dstsd)
			clif_item_repaireffect(dstsd,flag,nameid);
	}

	return;
}

/*==========================================
 * ���퐸�B�\�A�C�e�����X�g���M
 *------------------------------------------
 */
void clif_weapon_refine_list(struct map_session_data *sd)
{
	int i,c,fd;
	int skilllv;

	nullpo_retv(sd);

	skilllv = pc_checkskill(sd,WS_WEAPONREFINE);

	fd=sd->fd;
	WFIFOW(fd,0)=0x221;
	for(i=c=0;i<MAX_INVENTORY;i++){
		if( sd->status.inventory[i].nameid > 0 &&
		    sd->status.inventory[i].identify == 1 &&
		    sd->inventory_data[i]->refine &&
		    itemdb_wlv(sd->status.inventory[i].nameid) >=1 &&
		    !(sd->status.inventory[i].equip&0x0022) )
		{
			WFIFOW(fd,c*13+ 4)=i+2;
			WFIFOW(fd,c*13+ 6)=sd->status.inventory[i].nameid;
			WFIFOW(fd,c*13+ 8)=0;
			WFIFOW(fd,c*13+10)=0;
			WFIFOB(fd,c*13+12)=c;
			c++;
		}
	}
	WFIFOW(fd,2)=c*13+4;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_weapon_refine_res(struct map_session_data *sd, int flag, int nameid)
{
	int view,fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd, 0)=0x223;
	WFIFOL(fd, 2)=flag;
	if((view = itemdb_viewid(nameid)) > 0)
		WFIFOW(fd, 6)=view;
	else
		WFIFOW(fd, 6)=nameid;
	WFIFOSET(fd,packet_db[0x223].len);

	return;
}

/*==========================================
 * �A�C�e���ɂ��ꎞ�I�ȃX�L������
 *------------------------------------------
 */
void clif_item_skill(struct map_session_data *sd, int skillid, int skilllv, const char *name)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd, 0)=0x147;
	WFIFOW(fd, 2)=skillid;
	if(skillid == MO_EXTREMITYFIST || skillid == TK_JUMPKICK)
		WFIFOL(fd, 4)=1;
	else
		WFIFOL(fd, 4)=skill_get_inf(skillid);
	WFIFOW(fd, 8)=skilllv;
	WFIFOW(fd,10)=skill_get_sp(skillid,skilllv);
	WFIFOW(fd,12)=skill_get_fixed_range(&sd->bl,skillid,skilllv);
	strncpy(WFIFOP(fd,14),name,24);
	WFIFOB(fd,38)=0;
	WFIFOSET(fd,packet_db[0x147].len);

	return;
}

/*==========================================
 * �J�[�g�ɃA�C�e���ǉ�
 *------------------------------------------
 */
void clif_cart_additem(struct map_session_data *sd, int n, int amount)
{
	struct item_data *id;
	int j,fd;

	nullpo_retv(sd);

	if (n < 0 || n >= MAX_CART || sd->status.cart[n].nameid <= 0)
		return;

	fd = sd->fd;
	id = itemdb_search(sd->status.cart[n].nameid);

#if PACKETVER < 7
	WFIFOW(fd,0)=0x124;
	WFIFOW(fd,2)=n+2;
	WFIFOL(fd,4)=amount;
	if(id->view_id > 0)
		WFIFOW(fd,8)=id->view_id;
	else
		WFIFOW(fd,8)=sd->status.cart[n].nameid;
	WFIFOB(fd,10)=sd->status.cart[n].identify;
	WFIFOB(fd,11)=sd->status.cart[n].attribute;
	WFIFOB(fd,12)=sd->status.cart[n].refine;
	if(itemdb_isspecial(sd->status.cart[n].card[0])) {
		if(id->flag.pet_egg) {
			WFIFOW(fd,13) = 0;
			WFIFOW(fd,15) = 0;
			WFIFOW(fd,17) = 0;
		} else {
			WFIFOW(fd,13) = sd->status.cart[n].card[0];
			WFIFOW(fd,15) = sd->status.cart[n].card[1];
			WFIFOW(fd,17) = sd->status.cart[n].card[2];
		}
		WFIFOW(fd,19) = sd->status.cart[n].card[3];
	} else {
		if(sd->status.cart[n].card[0] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[0])) > 0)
			WFIFOW(fd,13)= j;
		else
			WFIFOW(fd,13)= sd->status.cart[n].card[0];
		if(sd->status.cart[n].card[1] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[1])) > 0)
			WFIFOW(fd,15)= j;
		else
			WFIFOW(fd,15)= sd->status.cart[n].card[1];
		if(sd->status.cart[n].card[2] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[2])) > 0)
			WFIFOW(fd,17)= j;
		else
			WFIFOW(fd,17)= sd->status.cart[n].card[2];
		if(sd->status.cart[n].card[3] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[3])) > 0)
			WFIFOW(fd,19)= j;
		else
			WFIFOW(fd,19)= sd->status.cart[n].card[3];
	}
	WFIFOSET(fd,packet_db[0x124].len);
#else
	WFIFOW(fd,0)=0x1c5;
	WFIFOW(fd,2)=n+2;
	WFIFOL(fd,4)=amount;
	if(id->view_id > 0)
		WFIFOW(fd,8)=id->view_id;
	else
		WFIFOW(fd,8)=sd->status.cart[n].nameid;
	WFIFOB(fd,10)=id->type;
	WFIFOB(fd,11)=sd->status.cart[n].identify;
	WFIFOB(fd,12)=sd->status.cart[n].attribute;
	WFIFOB(fd,13)=sd->status.cart[n].refine;
	if(itemdb_isspecial(sd->status.cart[n].card[0])) {
		if(id->flag.pet_egg) {
			WFIFOW(fd,14) = 0;
			WFIFOW(fd,16) = 0;
			WFIFOW(fd,18) = 0;
		} else {
			WFIFOW(fd,14) = sd->status.cart[n].card[0];
			WFIFOW(fd,16) = sd->status.cart[n].card[1];
			WFIFOW(fd,18) = sd->status.cart[n].card[2];
		}
		WFIFOW(fd,20) = sd->status.cart[n].card[3];
	} else {
		if(sd->status.cart[n].card[0] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[0])) > 0)
			WFIFOW(fd,14)= j;
		else
			WFIFOW(fd,14)= sd->status.cart[n].card[0];
		if(sd->status.cart[n].card[1] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[1])) > 0)
			WFIFOW(fd,16)= j;
		else
			WFIFOW(fd,16)= sd->status.cart[n].card[1];
		if(sd->status.cart[n].card[2] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[2])) > 0)
			WFIFOW(fd,18)= j;
		else
			WFIFOW(fd,18)= sd->status.cart[n].card[2];
		if(sd->status.cart[n].card[3] > 0 && (j=itemdb_viewid(sd->status.cart[n].card[3])) > 0)
			WFIFOW(fd,20)= j;
		else
			WFIFOW(fd,20)= sd->status.cart[n].card[3];
	}
	WFIFOSET(fd,packet_db[0x1c5].len);
#endif

	return;
}

/*==========================================
 * �J�[�g����A�C�e���폜
 *------------------------------------------
 */
void clif_cart_delitem(struct map_session_data *sd, int n, int amount)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x125;
	WFIFOW(fd,2)=n+2;
	WFIFOL(fd,4)=amount;
	WFIFOSET(fd,packet_db[0x125].len);

	return;
}

/*==========================================
 * �J�[�g�̃A�C�e�����X�g
 *------------------------------------------
 */
void clif_cart_itemlist(struct map_session_data *sd)
{
	struct item_data *id;
	int i,n,fd;

	nullpo_retv(sd);

	fd=sd->fd;

#if PACKETVER < 5
	WFIFOW(fd,0)=0x123;
	for(i=0,n=0;i<MAX_CART;i++){
		if(sd->status.cart[i].nameid<=0)
			continue;
		id = itemdb_search(sd->status.cart[i].nameid);
		if(itemdb_isequip2(id))
			continue;
		WFIFOW(fd,n*10+4)=i+2;
		if(id->view_id > 0)
			WFIFOW(fd,n*10+6)=id->view_id;
		else
			WFIFOW(fd,n*10+6)=sd->status.cart[i].nameid;
		WFIFOB(fd,n*10+8)=id->type;
		WFIFOB(fd,n*10+9)=sd->status.cart[i].identify;
		WFIFOW(fd,n*10+10)=sd->status.cart[i].amount;
		if(sd->status.cart[i].equip == LOC_ARROW)
			WFIFOW(fd,n*10+12)=LOC_ARROW;
		else
			WFIFOW(fd,n*10+12)=0;
		n++;
	}
	if(n){
		WFIFOW(fd,2)=4+n*10;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#elif PACKETVER < 14
	WFIFOW(fd,0)=0x1ef;
	for(i=0,n=0;i<MAX_CART;i++){
		if(sd->status.cart[i].nameid<=0)
			continue;
		id = itemdb_search(sd->status.cart[i].nameid);
		if(itemdb_isequip2(id))
			continue;
		WFIFOW(fd,n*18+4)=i+2;
		if(id->view_id > 0)
			WFIFOW(fd,n*18+6)=id->view_id;
		else
			WFIFOW(fd,n*18+6)=sd->status.cart[i].nameid;
		WFIFOB(fd,n*18+8)=id->type;
		WFIFOB(fd,n*18+9)=sd->status.cart[i].identify;
		WFIFOW(fd,n*18+10)=sd->status.cart[i].amount;
		if(sd->status.cart[i].equip == LOC_ARROW)
			WFIFOW(fd,n*18+12)=LOC_ARROW;
		else
			WFIFOW(fd,n*18+12)=0;
		WFIFOW(fd,n*18+14)=sd->status.cart[i].card[0];
		WFIFOW(fd,n*18+16)=sd->status.cart[i].card[1];
		WFIFOW(fd,n*18+18)=sd->status.cart[i].card[2];
		WFIFOW(fd,n*18+20)=sd->status.cart[i].card[3];
		n++;
	}
	if(n){
		WFIFOW(fd,2)=4+n*18;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#else
	WFIFOW(fd,0)=0x2e9;
	for(i=0,n=0;i<MAX_CART;i++){
		if(sd->status.cart[i].nameid<=0)
			continue;
		id = itemdb_search(sd->status.cart[i].nameid);
		if(itemdb_isequip2(id))
			continue;
		WFIFOW(fd,n*22+4)=i+2;
		if(id->view_id > 0)
			WFIFOW(fd,n*22+6)=id->view_id;
		else
			WFIFOW(fd,n*22+6)=sd->status.cart[i].nameid;
		WFIFOB(fd,n*22+8)=id->type;
		WFIFOB(fd,n*22+9)=sd->status.cart[i].identify;
		WFIFOW(fd,n*22+10)=sd->status.cart[i].amount;
		if(sd->status.cart[i].equip == LOC_ARROW)
			WFIFOW(fd,n*22+12)=LOC_ARROW;
		else
			WFIFOW(fd,n*22+12)=0;
		WFIFOW(fd,n*22+14)=sd->status.cart[i].card[0];
		WFIFOW(fd,n*22+16)=sd->status.cart[i].card[1];
		WFIFOW(fd,n*22+18)=sd->status.cart[i].card[2];
		WFIFOW(fd,n*22+20)=sd->status.cart[i].card[3];
		WFIFOL(fd,n*22+22)=sd->status.cart[i].limit;
		n++;
	}
	if(n){
		WFIFOW(fd,2)=4+n*22;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#endif

	return;
}

/*==========================================
 * �J�[�g�̑����i���X�g
 *------------------------------------------
 */
void clif_cart_equiplist(struct map_session_data *sd)
{
	struct item_data *id;
	int i,j,n,fd;

	nullpo_retv(sd);

	fd=sd->fd;

#if PACKETVER < 10
	WFIFOW(fd,0)=0x122;
	for(i=0,n=0;i<MAX_CART;i++){
		if(sd->status.cart[i].nameid<=0)
			continue;
		id = itemdb_search(sd->status.cart[i].nameid);
		if(!itemdb_isequip2(id))
			continue;
		WFIFOW(fd,n*20+4)=i+2;
		if(id->view_id > 0)
			WFIFOW(fd,n*20+6)=id->view_id;
		else
			WFIFOW(fd,n*20+6)=sd->status.cart[i].nameid;
		WFIFOB(fd,n*20+8)=id->type;
		WFIFOB(fd,n*20+9)=sd->status.cart[i].identify;
		WFIFOW(fd,n*20+10)=id->equip;
		WFIFOW(fd,n*20+12)=sd->status.cart[i].equip;
		WFIFOB(fd,n*20+14)=sd->status.cart[i].attribute;
		WFIFOB(fd,n*20+15)=sd->status.cart[i].refine;
		if(itemdb_isspecial(sd->status.cart[i].card[0])) {
			if(id->flag.pet_egg) {
				WFIFOW(fd,n*20+16) = 0;
				WFIFOW(fd,n*20+18) = 0;
				WFIFOW(fd,n*20+20) = 0;
			} else {
				WFIFOW(fd,n*20+16) = sd->status.cart[i].card[0];
				WFIFOW(fd,n*20+18) = sd->status.cart[i].card[1];
				WFIFOW(fd,n*20+20) = sd->status.cart[i].card[2];
			}
			WFIFOW(fd,n*20+22) = sd->status.cart[i].card[3];
		} else {
			if(sd->status.cart[i].card[0] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[0])) > 0)
				WFIFOW(fd,n*20+16)= j;
			else
				WFIFOW(fd,n*20+16)= sd->status.cart[i].card[0];
			if(sd->status.cart[i].card[1] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[1])) > 0)
				WFIFOW(fd,n*20+18)= j;
			else
				WFIFOW(fd,n*20+18)= sd->status.cart[i].card[1];
			if(sd->status.cart[i].card[2] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[2])) > 0)
				WFIFOW(fd,n*20+20)= j;
			else
				WFIFOW(fd,n*20+20)= sd->status.cart[i].card[2];
			if(sd->status.cart[i].card[3] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[3])) > 0)
				WFIFOW(fd,n*20+22)= j;
			else
				WFIFOW(fd,n*20+22)= sd->status.cart[i].card[3];
		}
		n++;
	}
	if(n){
		WFIFOW(fd,2)=4+n*20;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#elif PACKETVER < 11
	WFIFOW(fd,0)=0x297;
	for(i=0,n=0;i<MAX_CART;i++){
		if(sd->status.cart[i].nameid<=0)
			continue;
		id = itemdb_search(sd->status.cart[i].nameid);
		if(!itemdb_isequip2(id))
			continue;
		WFIFOW(fd,n*24+4)=i+2;
		if(id->view_id > 0)
			WFIFOW(fd,n*24+6)=id->view_id;
		else
			WFIFOW(fd,n*24+6)=sd->status.cart[i].nameid;
		WFIFOB(fd,n*24+8)=id->type;
		WFIFOB(fd,n*24+9)=sd->status.cart[i].identify;
		WFIFOW(fd,n*24+10)=id->equip;
		WFIFOW(fd,n*24+12)=sd->status.cart[i].equip;
		WFIFOB(fd,n*24+14)=sd->status.cart[i].attribute;
		WFIFOB(fd,n*24+15)=sd->status.cart[i].refine;
		if(itemdb_isspecial(sd->status.cart[i].card[0])) {
			if(id->flag.pet_egg) {
				WFIFOW(fd,n*24+16) = 0;
				WFIFOW(fd,n*24+18) = 0;
				WFIFOW(fd,n*24+20) = 0;
			} else {
				WFIFOW(fd,n*24+16) = sd->status.cart[i].card[0];
				WFIFOW(fd,n*24+18) = sd->status.cart[i].card[1];
				WFIFOW(fd,n*24+20) = sd->status.cart[i].card[2];
			}
			WFIFOW(fd,n*24+22) = sd->status.cart[i].card[3];
		} else {
			if(sd->status.cart[i].card[0] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[0])) > 0)
				WFIFOW(fd,n*24+16)= j;
			else
				WFIFOW(fd,n*24+16)= sd->status.cart[i].card[0];
			if(sd->status.cart[i].card[1] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[1])) > 0)
				WFIFOW(fd,n*24+18)= j;
			else
				WFIFOW(fd,n*24+18)= sd->status.cart[i].card[1];
			if(sd->status.cart[i].card[2] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[2])) > 0)
				WFIFOW(fd,n*24+20)= j;
			else
				WFIFOW(fd,n*24+20)= sd->status.cart[i].card[2];
			if(sd->status.cart[i].card[3] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[3])) > 0)
				WFIFOW(fd,n*24+22)= j;
			else
				WFIFOW(fd,n*24+22)= sd->status.cart[i].card[3];
		}
		WFIFOL(fd,n*24+24)=sd->status.cart[i].limit;
		n++;
	}
	if(n){
		WFIFOW(fd,2)=4+n*24;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#elif PACKETVER < 27
	WFIFOW(fd,0)=0x2d2;
	for(i=0,n=0;i<MAX_CART;i++){
		if(sd->status.cart[i].nameid<=0)
			continue;
		id = itemdb_search(sd->status.cart[i].nameid);
		if(!itemdb_isequip2(id))
			continue;
		WFIFOW(fd,n*26+4)=i+2;
		if(id->view_id > 0)
			WFIFOW(fd,n*26+6)=id->view_id;
		else
			WFIFOW(fd,n*26+6)=sd->status.cart[i].nameid;
		WFIFOB(fd,n*26+8)=id->type;
		WFIFOB(fd,n*26+9)=sd->status.cart[i].identify;
		WFIFOW(fd,n*26+10)=id->equip;
		WFIFOW(fd,n*26+12)=sd->status.cart[i].equip;
		WFIFOB(fd,n*26+14)=sd->status.cart[i].attribute;
		WFIFOB(fd,n*26+15)=sd->status.cart[i].refine;
		if(itemdb_isspecial(sd->status.cart[i].card[0])) {
			if(id->flag.pet_egg) {
				WFIFOW(fd,n*26+16) = 0;
				WFIFOW(fd,n*26+18) = 0;
				WFIFOW(fd,n*26+20) = 0;
			} else {
				WFIFOW(fd,n*26+16) = sd->status.cart[i].card[0];
				WFIFOW(fd,n*26+18) = sd->status.cart[i].card[1];
				WFIFOW(fd,n*26+20) = sd->status.cart[i].card[2];
			}
			WFIFOW(fd,n*26+22) = sd->status.cart[i].card[3];
		} else {
			if(sd->status.cart[i].card[0] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[0])) > 0)
				WFIFOW(fd,n*26+16)= j;
			else
				WFIFOW(fd,n*26+16)= sd->status.cart[i].card[0];
			if(sd->status.cart[i].card[1] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[1])) > 0)
				WFIFOW(fd,n*26+18)= j;
			else
				WFIFOW(fd,n*26+18)= sd->status.cart[i].card[1];
			if(sd->status.cart[i].card[2] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[2])) > 0)
				WFIFOW(fd,n*26+20)= j;
			else
				WFIFOW(fd,n*26+20)= sd->status.cart[i].card[2];
			if(sd->status.cart[i].card[3] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[3])) > 0)
				WFIFOW(fd,n*26+22)= j;
			else
				WFIFOW(fd,n*26+22)= sd->status.cart[i].card[3];
		}
		WFIFOL(fd,n*26+24)=sd->status.cart[i].limit;
		WFIFOW(fd,n*26+28)=0;
		n++;
	}
	if(n){
		WFIFOW(fd,2)=4+n*26;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#else
	WFIFOW(fd,0)=0x2d2;
	for(i=0,n=0;i<MAX_CART;i++){
		if(sd->status.cart[i].nameid<=0)
			continue;
		id = itemdb_search(sd->status.cart[i].nameid);
		if(!itemdb_isequip2(id))
			continue;
		WFIFOW(fd,n*28+4)=i+2;
		if(id->view_id > 0)
			WFIFOW(fd,n*28+6)=id->view_id;
		else
			WFIFOW(fd,n*28+6)=sd->status.cart[i].nameid;
		WFIFOB(fd,n*28+8)=id->type;
		WFIFOB(fd,n*28+9)=sd->status.cart[i].identify;
		WFIFOW(fd,n*28+10)=id->equip;
		WFIFOW(fd,n*28+12)=sd->status.cart[i].equip;
		WFIFOB(fd,n*28+14)=sd->status.cart[i].attribute;
		WFIFOB(fd,n*28+15)=sd->status.cart[i].refine;
		if(itemdb_isspecial(sd->status.cart[i].card[0])) {
			if(id->flag.pet_egg) {
				WFIFOW(fd,n*28+16) = 0;
				WFIFOW(fd,n*28+18) = 0;
				WFIFOW(fd,n*28+20) = 0;
			} else {
				WFIFOW(fd,n*28+16) = sd->status.cart[i].card[0];
				WFIFOW(fd,n*28+18) = sd->status.cart[i].card[1];
				WFIFOW(fd,n*28+20) = sd->status.cart[i].card[2];
			}
			WFIFOW(fd,n*28+22) = sd->status.cart[i].card[3];
		} else {
			if(sd->status.cart[i].card[0] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[0])) > 0)
				WFIFOW(fd,n*28+16)= j;
			else
				WFIFOW(fd,n*28+16)= sd->status.cart[i].card[0];
			if(sd->status.cart[i].card[1] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[1])) > 0)
				WFIFOW(fd,n*28+18)= j;
			else
				WFIFOW(fd,n*28+18)= sd->status.cart[i].card[1];
			if(sd->status.cart[i].card[2] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[2])) > 0)
				WFIFOW(fd,n*28+20)= j;
			else
				WFIFOW(fd,n*28+20)= sd->status.cart[i].card[2];
			if(sd->status.cart[i].card[3] > 0 && (j=itemdb_viewid(sd->status.cart[i].card[3])) > 0)
				WFIFOW(fd,n*28+22)= j;
			else
				WFIFOW(fd,n*28+22)= sd->status.cart[i].card[3];
		}
		WFIFOL(fd,n*28+24)=sd->status.cart[i].limit;
		WFIFOW(fd,n*28+28)=0;
		WFIFOW(fd,n*28+30)=0;
		n++;
	}
	if(n){
		WFIFOW(fd,2)=4+n*28;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#endif

	return;
}

/*==========================================
 * �J�[�g�̃N���A
 *------------------------------------------
 */
void clif_cart_clear(struct map_session_data *sd)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0) = 0x12b;
	WFIFOSET(fd,packet_db[0x12b].len);

	return;
}

/*==========================================
 * �I�X�J��
 *------------------------------------------
 */
void clif_openvendingreq(struct map_session_data *sd, int num)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x12d;
	WFIFOW(fd,2)=num;
	WFIFOSET(fd,packet_db[0x12d].len);

	return;
}

/*==========================================
 * �I�X�Ŕ\��
 *------------------------------------------
 */
void clif_showvendingboard(struct block_list* bl, const char *shop_title, int fd)
{
	unsigned char buf[96];

	nullpo_retv(bl);

	WBUFW(buf,0)=0x131;
	WBUFL(buf,2)=bl->id;
	strncpy(WBUFP(buf,6), shop_title, 80);
	if(fd >= 0) {
		memcpy(WFIFOP(fd,0),buf,packet_db[0x131].len);
		WFIFOSET(fd,packet_db[0x131].len);
	} else {
		clif_send(buf,packet_db[0x131].len,bl,AREA_WOS);
	}

	return;
}

/*==========================================
 * �I�X�Ŕ���
 *------------------------------------------
 */
void clif_closevendingboard(struct block_list* bl, int fd)
{
	unsigned char buf[8];

	nullpo_retv(bl);

	WBUFW(buf,0)=0x132;
	WBUFL(buf,2)=bl->id;
	if(fd >= 0) {
		memcpy(WFIFOP(fd,0),buf,packet_db[0x132].len);
		WFIFOSET(fd,packet_db[0x132].len);
	} else {
		clif_send(buf,packet_db[0x132].len,bl,AREA_WOS);
	}

	return;
}

/*==========================================
 * �I�X�A�C�e�����X�g
 *------------------------------------------
 */
void clif_vendinglist(struct map_session_data *sd, struct map_session_data *vsd)
{
	struct item_data *data;
	int i, j, n, idx, fd;
	struct vending *vending;

	nullpo_retv(sd);
	nullpo_retv(vsd);
	nullpo_retv(vending = vsd->vending);

	fd=sd->fd;
#if PACKETVER < 25
	WFIFOW(fd,0)=0x133;
	WFIFOL(fd,4)=vsd->status.account_id;
	n = 0;
	for(i = 0; i < vsd->vend_num; i++) {
		if(vending[i].amount<=0)
			continue;
		WFIFOL(fd, 8+n*22)=vending[i].value;
		WFIFOW(fd,12+n*22)=vending[i].amount;
		WFIFOW(fd,14+n*22)=(idx = vending[i].index) + 2;
		if(vsd->status.cart[idx].nameid <= 0 || vsd->status.cart[idx].amount <= 0)
			continue;
		data = itemdb_search(vsd->status.cart[idx].nameid);
		WFIFOB(fd,16+n*22)=data->type;
		if(data->view_id > 0)
			WFIFOW(fd,17+n*22)=data->view_id;
		else
			WFIFOW(fd,17+n*22)=vsd->status.cart[idx].nameid;
		WFIFOB(fd,19+n*22)=vsd->status.cart[idx].identify;
		WFIFOB(fd,20+n*22)=vsd->status.cart[idx].attribute;
		WFIFOB(fd,21+n*22)=vsd->status.cart[idx].refine;
		if(itemdb_isspecial(vsd->status.cart[idx].card[0])) {
			if(data->flag.pet_egg) {
				WFIFOW(fd,22+n*22) = 0;
				WFIFOW(fd,24+n*22) = 0;
				WFIFOW(fd,26+n*22) = 0;
			} else {
				WFIFOW(fd,22+n*22) = vsd->status.cart[idx].card[0];
				WFIFOW(fd,24+n*22) = vsd->status.cart[idx].card[1];
				WFIFOW(fd,26+n*22) = vsd->status.cart[idx].card[2];
			}
			WFIFOW(fd,28+n*22) = vsd->status.cart[idx].card[3];
		} else {
			if(vsd->status.cart[idx].card[0] > 0 && (j=itemdb_viewid(vsd->status.cart[idx].card[0])) > 0)
				WFIFOW(fd,22+n*22)= j;
			else
				WFIFOW(fd,22+n*22)= vsd->status.cart[idx].card[0];
			if(vsd->status.cart[idx].card[1] > 0 && (j=itemdb_viewid(vsd->status.cart[idx].card[1])) > 0)
				WFIFOW(fd,24+n*22)= j;
			else
				WFIFOW(fd,24+n*22)= vsd->status.cart[idx].card[1];
			if(vsd->status.cart[idx].card[2] > 0 && (j=itemdb_viewid(vsd->status.cart[idx].card[2])) > 0)
				WFIFOW(fd,26+n*22)= j;
			else
				WFIFOW(fd,26+n*22)= vsd->status.cart[idx].card[2];
			if(vsd->status.cart[idx].card[3] > 0 && (j=itemdb_viewid(vsd->status.cart[idx].card[3])) > 0)
				WFIFOW(fd,28+n*22)= j;
			else
				WFIFOW(fd,28+n*22)= vsd->status.cart[idx].card[3];
		}
		n++;
	}
	if(n > 0){
		WFIFOW(fd,2)=8+n*22;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#else
	WFIFOW(fd,0)=0x800;
	WFIFOL(fd,4)=vsd->status.account_id;
	WFIFOL(fd,8)=vsd->status.char_id;
	n = 0;
	for(i = 0; i < vsd->vend_num; i++) {
		if(vending[i].amount<=0)
			continue;
		WFIFOL(fd,12+n*22)=vending[i].value;
		WFIFOW(fd,16+n*22)=vending[i].amount;
		WFIFOW(fd,18+n*22)=(idx = vending[i].index) + 2;
		if(vsd->status.cart[idx].nameid <= 0 || vsd->status.cart[idx].amount <= 0)
			continue;
		data = itemdb_search(vsd->status.cart[idx].nameid);
		WFIFOB(fd,20+n*22)=data->type;
		if(data->view_id > 0)
			WFIFOW(fd,21+n*22)=data->view_id;
		else
			WFIFOW(fd,21+n*22)=vsd->status.cart[idx].nameid;
		WFIFOB(fd,23+n*22)=vsd->status.cart[idx].identify;
		WFIFOB(fd,24+n*22)=vsd->status.cart[idx].attribute;
		WFIFOB(fd,25+n*22)=vsd->status.cart[idx].refine;
		if(itemdb_isspecial(vsd->status.cart[idx].card[0])) {
			if(data->flag.pet_egg) {
				WFIFOW(fd,26+n*22) = 0;
				WFIFOW(fd,28+n*22) = 0;
				WFIFOW(fd,30+n*22) = 0;
			} else {
				WFIFOW(fd,26+n*22) = vsd->status.cart[idx].card[0];
				WFIFOW(fd,28+n*22) = vsd->status.cart[idx].card[1];
				WFIFOW(fd,30+n*22) = vsd->status.cart[idx].card[2];
			}
			WFIFOW(fd,32+n*22) = vsd->status.cart[idx].card[3];
		} else {
			if(vsd->status.cart[idx].card[0] > 0 && (j=itemdb_viewid(vsd->status.cart[idx].card[0])) > 0)
				WFIFOW(fd,26+n*22)= j;
			else
				WFIFOW(fd,26+n*22)= vsd->status.cart[idx].card[0];
			if(vsd->status.cart[idx].card[1] > 0 && (j=itemdb_viewid(vsd->status.cart[idx].card[1])) > 0)
				WFIFOW(fd,28+n*22)= j;
			else
				WFIFOW(fd,28+n*22)= vsd->status.cart[idx].card[1];
			if(vsd->status.cart[idx].card[2] > 0 && (j=itemdb_viewid(vsd->status.cart[idx].card[2])) > 0)
				WFIFOW(fd,30+n*22)= j;
			else
				WFIFOW(fd,30+n*22)= vsd->status.cart[idx].card[2];
			if(vsd->status.cart[idx].card[3] > 0 && (j=itemdb_viewid(vsd->status.cart[idx].card[3])) > 0)
				WFIFOW(fd,32+n*22)= j;
			else
				WFIFOW(fd,32+n*22)= vsd->status.cart[idx].card[3];
		}
		n++;
	}
	if(n > 0){
		WFIFOW(fd,2)=12+n*22;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#endif

	return;
}

/*==========================================
 * �I�X�A�C�e���w�����s
 *------------------------------------------
*/
void clif_buyvending(struct map_session_data *sd, int idx, int amount, unsigned char fail)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x135;
	WFIFOW(fd,2)=idx+2;
	WFIFOW(fd,4)=amount;
	WFIFOB(fd,6)=fail;
	WFIFOSET(fd,packet_db[0x135].len);

	return;
}

/*==========================================
 * �I�X�J�ݐ���
 *------------------------------------------
*/
int clif_openvending(struct map_session_data *sd)
{
	struct item_data *data;
	int i, j, n, idx, fd;
	struct vending *vending;

	nullpo_retr(0, sd);
	nullpo_retr(0, vending = sd->vending);

	fd=sd->fd;
	WFIFOW(fd,0)=0x136;
	WFIFOL(fd,4)=sd->bl.id;
	n = 0;
	for(i = 0; i < sd->vend_num; i++) {
		WFIFOL(fd, 8+n*22)=vending[i].value;
		WFIFOW(fd,12+n*22)=(idx = vending[i].index) + 2;
		WFIFOW(fd,14+n*22)=vending[i].amount;
		if(sd->status.cart[idx].nameid <= 0 || sd->status.cart[idx].amount <= 0)
			continue;
		data = itemdb_search(sd->status.cart[idx].nameid);
		WFIFOB(fd,16+n*22)=data->type;
		if(data->view_id > 0)
			WFIFOW(fd,17+n*22)=data->view_id;
		else
			WFIFOW(fd,17+n*22)=sd->status.cart[idx].nameid;
		WFIFOB(fd,19+n*22)=sd->status.cart[idx].identify;
		WFIFOB(fd,20+n*22)=sd->status.cart[idx].attribute;
		WFIFOB(fd,21+n*22)=sd->status.cart[idx].refine;
		if(itemdb_isspecial(sd->status.cart[idx].card[0])) {
			if(data->flag.pet_egg) {
				WFIFOW(fd,22+n*22)=0;
				WFIFOW(fd,24+n*22)=0;
				WFIFOW(fd,26+n*22)=0;
			} else {
				WFIFOW(fd,22+n*22)=sd->status.cart[idx].card[0];
				WFIFOW(fd,24+n*22)=sd->status.cart[idx].card[1];
				WFIFOW(fd,26+n*22)=sd->status.cart[idx].card[2];
			}
			WFIFOW(fd,28+n*22)=sd->status.cart[idx].card[3];
		} else {
			if(sd->status.cart[idx].card[0] > 0 && (j=itemdb_viewid(sd->status.cart[idx].card[0])) > 0)
				WFIFOW(fd,22+n*22)= j;
			else
				WFIFOW(fd,22+n*22)= sd->status.cart[idx].card[0];
			if(sd->status.cart[idx].card[1] > 0 && (j=itemdb_viewid(sd->status.cart[idx].card[1])) > 0)
				WFIFOW(fd,24+n*22)= j;
			else
				WFIFOW(fd,24+n*22)= sd->status.cart[idx].card[1];
			if(sd->status.cart[idx].card[2] > 0 && (j=itemdb_viewid(sd->status.cart[idx].card[2])) > 0)
				WFIFOW(fd,26+n*22)= j;
			else
				WFIFOW(fd,26+n*22)= sd->status.cart[idx].card[2];
			if(sd->status.cart[idx].card[3] > 0 && (j=itemdb_viewid(sd->status.cart[idx].card[3])) > 0)
				WFIFOW(fd,28+n*22)= j;
			else
				WFIFOW(fd,28+n*22)= sd->status.cart[idx].card[3];
		}
		n++;
	}
	if(n > 0){
		WFIFOW(fd,2)=8+n*22;
		WFIFOSET(fd,WFIFOW(fd,2));
	}

	return n;
}

/*==========================================
 * �I�X�A�C�e���̔���
 *------------------------------------------
*/
void clif_vendingreport(struct map_session_data *sd, int idx, int amount)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x137;
	WFIFOW(fd,2)=idx+2;
	WFIFOW(fd,4)=amount;
	WFIFOSET(fd,packet_db[0x137].len);

	return;
}

/*==========================================
 * �p�[�e�B�쐬����
 *------------------------------------------
 */
void clif_party_created(struct map_session_data *sd, unsigned char flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xfa;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_db[0xfa].len);

	return;
}

/*==========================================
 * �p�[�e�B���C�����
 *------------------------------------------
 */
void clif_party_main_info(struct party *p, int fd)
{
	unsigned char buf[96];
	int i;
	struct map_session_data *sd = NULL;

	nullpo_retv(p);

	for(i=0; i<MAX_PARTY && !p->member[i].leader; i++);

	if(i >= MAX_PARTY)
		return;

	sd = p->member[i].sd;
	WBUFW(buf,0)  = 0x1e9;
	WBUFL(buf,2)  = p->member[i].account_id;
	WBUFL(buf,6)  = (p->member[i].leader)? 0: 1;
	WBUFW(buf,10) = (sd)? sd->bl.x: 0;
	WBUFW(buf,12) = (sd)? sd->bl.y: 0;
	WBUFB(buf,14) = (p->member[i].online)? 0: 1;
	memcpy(WBUFP(buf,15), p->name, 24);
	memcpy(WBUFP(buf,39), p->member[i].name, 24);
	memcpy(WBUFP(buf,63), p->member[i].map, 24);
	WBUFB(buf,79) = (p->item&1)? 1: 0;
	WBUFB(buf,80) = (p->item&2)? 1: 0;

	if(fd >= 0){	// fd���ݒ肳��Ă�Ȃ炻��ɑ���
		memcpy(WFIFOP(fd,0),buf,packet_db[0x1e9].len);
		WFIFOSET(fd,packet_db[0x1e9].len);
		return;
	}
	if(!sd) {	// ���[�_�[���I�t���C���Ȃ̂Ń��O�C�����̃����o�[��T��
		for(i=0; i<MAX_PARTY && !p->member[i].sd; i++);
		if(i >= MAX_PARTY)
			return;
		sd = p->member[i].sd;
	}
	clif_send(buf,packet_db[0x1e9].len,&sd->bl,PARTY);

	return;
}

/*==========================================
 * �p�[�e�B��񑗐M
 *------------------------------------------
 */
void clif_party_info(struct party *p, int fd)
{
	unsigned char buf[28+MAX_PARTY*46];
	int i,c;
	struct map_session_data *sd=NULL;

	nullpo_retv(p);

	WBUFW(buf,0)=0xfb;
	memcpy(WBUFP(buf,4),p->name,24);
	for(i=c=0;i<MAX_PARTY;i++){
		struct party_member *m=&p->member[i];
		if(m->account_id>0){
			if(sd==NULL) sd=m->sd;
			WBUFL(buf,28+c*46)=m->account_id;
			memcpy(WBUFP(buf,28+c*46+ 4),m->name,24);
			memcpy(WBUFP(buf,28+c*46+28),m->map,16);
			WBUFB(buf,28+c*46+44)=(m->leader)?0:1;
			WBUFB(buf,28+c*46+45)=(m->online)?0:1;
			c++;
		}
	}
	WBUFW(buf,2)=28+c*46;
	if(fd>=0){	// fd���ݒ肳��Ă�Ȃ炻��ɑ���
		memcpy(WFIFOP(fd,0),buf,WBUFW(buf,2));
		WFIFOSET(fd,WFIFOW(fd,2));
		return;
	}
	if(sd!=NULL)
		clif_send(buf,WBUFW(buf,2),&sd->bl,PARTY);

	return;
}

/*==========================================
 * �p�[�e�B���U
 *------------------------------------------
 */
void clif_party_invite(struct map_session_data *sd, struct map_session_data *tsd, char *name)
{
	int fd;

	nullpo_retv(sd);
	nullpo_retv(tsd);

	fd=tsd->fd;

#if PACKETVER < 11
	WFIFOW(fd,0)=0xfe;
	WFIFOL(fd,2)=sd->status.account_id;
	memcpy(WFIFOP(fd,6),name,24);
	WFIFOSET(fd,packet_db[0xfe].len);
#else
	WFIFOW(fd,0)=0x2c6;
	WFIFOL(fd,2)=sd->status.account_id;
	memcpy(WFIFOP(fd,6),name,24);
	WFIFOSET(fd,packet_db[0x2c6].len);
#endif

	return;
}

/*==========================================
 * �p�[�e�B���U����
 *------------------------------------------
 */
void clif_party_inviteack(struct map_session_data *sd, const char *nick, unsigned char flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;

#if PACKETVER < 11
	WFIFOW(fd, 0)=0xfd;
	strncpy(WFIFOP(fd,2),nick,24);
	WFIFOB(fd,26)=flag;
	WFIFOSET(fd,packet_db[0xfd].len);
#else
	WFIFOW(fd, 0)=0x2c5;
	strncpy(WFIFOP(fd,2),nick,24);
	WFIFOL(fd,26)=flag;
	WFIFOSET(fd,packet_db[0x2c5].len);
#endif

	return;
}

/*==========================================
 * �p�[�e�B�ݒ著�M
 * flag & 0x001=exp�ύX�~�X
 *        0x010=item�ύX�~�X�i���݂͑��݂��Ȃ��j
 *        0x100=��l�ɂ̂ݑ��M
 *------------------------------------------
 */
void clif_party_option(struct party *p, struct map_session_data *sd, int flag)
{
	unsigned char buf[16];

	nullpo_retv(p);

	if(sd==NULL && flag==0){
		int i;
		for(i=0;i<MAX_PARTY;i++)
			if((sd=map_id2sd(p->member[i].account_id))!=NULL)
				break;
	}
	if(sd==NULL)
		return;

#if PACKETVER < 20
	WBUFW(buf,0)=0x101;
	WBUFL(buf,2)=((flag&0x01)?2:p->exp);
	if(flag == 0) {
		clif_send(buf,packet_db[0x101].len,&sd->bl,PARTY);
	} else {
		memcpy(WFIFOP(sd->fd,0),buf,packet_db[0x101].len);
		WFIFOSET(sd->fd,packet_db[0x101].len);
	}
#else
	WBUFW(buf,0)=0x7d8;
	WBUFL(buf,2)=((flag&0x01)?2:p->exp);
	WBUFB(buf,6)=(p->item&1)?1:0;
	WBUFB(buf,7)=(p->item&2)?1:0;
	if(flag == 0) {
		clif_send(buf,packet_db[0x7d8].len,&sd->bl,PARTY);
	} else {
		memcpy(WFIFOP(sd->fd,0),buf,packet_db[0x7d8].len);
		WFIFOSET(sd->fd,packet_db[0x7d8].len);
	}
#endif

	return;
}

/*==========================================
 * �p�[�e�B�E�ށi�E�ޑO�ɌĂԂ��Ɓj
 *------------------------------------------
 */
void clif_party_leaved(struct party *p, struct map_session_data *sd, int account_id, char *name, int flag)
{
	unsigned char buf[32];
	int i;

	nullpo_retv(p);

	WBUFW(buf,0)=0x105;
	WBUFL(buf,2)=account_id;
	memcpy(WBUFP(buf,6),name,24);
	WBUFB(buf,30)=flag&0x0f;

	if((flag&0xf0)==0){
		if(sd==NULL)
			for(i=0;i<MAX_PARTY;i++)
				if((sd=p->member[i].sd)!=NULL)
					break;
		if(sd!=NULL)
			clif_send(buf,packet_db[0x105].len,&sd->bl,PARTY);
	}else if(sd!=NULL){
		memcpy(WFIFOP(sd->fd,0),buf,packet_db[0x105].len);
		WFIFOSET(sd->fd,packet_db[0x105].len);
	}

	return;
}

/*==========================================
 * �p�[�e�B���b�Z�[�W���M
 *------------------------------------------
 */
void clif_party_message(struct party *p, int account_id, char *mes, int len)
{
	unsigned char buf[1024];
	struct map_session_data *sd = NULL;
	int i;

	nullpo_retv(p);

	for(i=0;i<MAX_PARTY;i++){
		if((sd=p->member[i].sd)!=NULL)
			break;
	}
	if(sd==NULL)
		return;

	if(len > sizeof(buf) - 8)
		len = sizeof(buf) - 8;

	WBUFW(buf,0)=0x109;
	WBUFW(buf,2)=len+8;
	WBUFL(buf,4)=account_id;
	memcpy(WBUFP(buf,8),mes,len);
	clif_send(buf,len+8,&sd->bl,PARTY);

	return;
}

/*==========================================
 * �p�[�e�B���W�ʒm
 *------------------------------------------
 */
void clif_party_xy(struct map_session_data *sd)
{
	unsigned char buf[16];

	nullpo_retv(sd);

	WBUFW(buf,0)=0x107;
	WBUFL(buf,2)=sd->status.account_id;
	WBUFW(buf,6)=sd->bl.x;
	WBUFW(buf,8)=sd->bl.y;
	clif_send(buf,packet_db[0x107].len,&sd->bl,PARTY_SAMEMAP_WOS);

	return;
}

/*==========================================
 * �p�[�e�BHP�ʒm
 *------------------------------------------
 */
void clif_party_hp(struct map_session_data *sd)
{
	unsigned char buf[16];

	nullpo_retv(sd);

#if PACKETVER < 26
	WBUFW(buf,0)=0x106;
	WBUFL(buf,2)=sd->status.account_id;
	WBUFW(buf,6)=(sd->status.max_hp > 0x7fff)? (short)((atn_bignumber)sd->status.hp * 0x7fff / sd->status.max_hp): sd->status.hp;
	WBUFW(buf,8)=(sd->status.max_hp > 0x7fff)? 0x7fff: sd->status.max_hp;
	clif_send(buf,packet_db[0x106].len,&sd->bl,PARTY_AREA_WOS);
#else
	WBUFW(buf,0)=0x80e;
	WBUFL(buf,2)=sd->status.account_id;
	WBUFL(buf,6)=sd->status.hp;
	WBUFL(buf,10)=sd->status.max_hp;
	clif_send(buf,packet_db[0x80e].len,&sd->bl,PARTY_AREA_WOS);
#endif

	return;
}

/*==========================================
 * �p�[�e�B�ꏊ�ړ��i���g�p�j
 *------------------------------------------
 */
/*void clif_party_move(struct party *p, struct map_session_data *sd, unsigned char online)
{
	unsigned char buf[96];

	nullpo_retv(sd);
	nullpo_retv(p);

	WBUFW(buf, 0)=0x104;
	WBUFL(buf, 2)=sd->status.account_id;
	WBUFL(buf, 6)=0;
	WBUFW(buf,10)=sd->bl.x;
	WBUFW(buf,12)=sd->bl.y;
	WBUFB(buf,14)=!online;
	memcpy(WBUFP(buf,15),p->name,24);
	memcpy(WBUFP(buf,39),sd->status.name,24);
	memcpy(WBUFP(buf,63),map[sd->bl.m].name,16);
	clif_send(buf,packet_db[0x104].len,&sd->bl,PARTY);

	return;
}*/

/*==========================================
 * �U�����邽�߂Ɉړ����K�v
 *------------------------------------------
 */
void clif_movetoattack(struct map_session_data *sd, struct block_list *bl)
{
	int fd;

	nullpo_retv(sd);
	nullpo_retv(bl);

	fd=sd->fd;
	WFIFOW(fd, 0)=0x139;
	WFIFOL(fd, 2)=bl->id;
	WFIFOW(fd, 6)=bl->x;
	WFIFOW(fd, 8)=bl->y;
	WFIFOW(fd,10)=sd->bl.x;
	WFIFOW(fd,12)=sd->bl.y;
	WFIFOW(fd,14)=sd->range.attackrange;
	WFIFOSET(fd,packet_db[0x139].len);

	return;
}

/*==========================================
 * �����G�t�F�N�g
 *------------------------------------------
 */
void clif_produceeffect(struct map_session_data *sd, unsigned short flag, int nameid)
{
	int view,fd;

	nullpo_retv(sd);

	// ���O�̓o�^�Ƒ��M���ɂ��Ă���
	if( map_charid2nick(sd->status.char_id)==NULL )
		map_addchariddb(sd->status.char_id,sd->status.name,sd->status.account_id,clif_getip(),clif_getport());
	clif_solved_charname(sd,sd->status.char_id);

	fd=sd->fd;
	WFIFOW(fd, 0)=0x18f;
	WFIFOW(fd, 2)=flag;
	if((view = itemdb_viewid(nameid)) > 0)
		WFIFOW(fd, 4)=view;
	else
		WFIFOW(fd, 4)=nameid;
	WFIFOSET(fd,packet_db[0x18f].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_catch_process(struct map_session_data *sd)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x19e;
	WFIFOSET(fd,packet_db[0x19e].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_pet_rulet(struct map_session_data *sd, unsigned char data)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x1a0;
	WFIFOB(fd,2)=data;
	WFIFOSET(fd,packet_db[0x1a0].len);

	return;
}

/*==========================================
 * pet�����X�g�쐬
 *------------------------------------------
 */
void clif_sendegg(struct map_session_data *sd)
{
	int i,n=0,fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x1a6;
	if(sd->status.pet_id <= 0) {
		for(i=0;i<MAX_INVENTORY;i++){
			if(sd->status.inventory[i].nameid<=0 ||
			   sd->inventory_data[i] == NULL ||
			   !sd->inventory_data[i]->flag.pet_egg ||
			   sd->status.inventory[i].amount<=0)
				continue;
			WFIFOW(fd,n*2+4)=i+2;
			n++;
		}
	}
	WFIFOW(fd,2)=4+n*2;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_send_petdata(struct map_session_data *sd, unsigned char type, int param)
{
	int fd;

	nullpo_retv(sd);

	if(!sd->pd)
		return; // �V���x0�ɂȂ��NULL

	fd=sd->fd;
	WFIFOW(fd,0)=0x1a4;
	WFIFOB(fd,2)=type;
	WFIFOL(fd,3)=sd->pd->bl.id;
	WFIFOL(fd,7)=param;
	WFIFOSET(fd,packet_db[0x1a4].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_send_petstatus(struct map_session_data *sd)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x1a2;
	memcpy(WFIFOP(fd,2),sd->pet.name,24);
	WFIFOB(fd,26)=(battle_config.pet_rename == 1)? 0:sd->pet.rename_flag;
	WFIFOW(fd,27)=sd->pet.level;
	WFIFOW(fd,29)=sd->pet.hungry;
	WFIFOW(fd,31)=sd->pet.intimate;
	WFIFOW(fd,33)=sd->pet.equip;
#if PACKETVER > 17
	WFIFOW(fd,35)=sd->pet.class_;
#endif
	WFIFOSET(fd,packet_db[0x1a2].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_pet_emotion(struct pet_data *pd,int param)
{
	unsigned char buf[16];
	struct map_session_data *sd;

	nullpo_retv(pd);
	nullpo_retv(sd = pd->msd);

	WBUFW(buf,0)=0x1aa;
	WBUFL(buf,2)=pd->bl.id;
	if(param >= 100 && sd->petDB->talk_convert_class) {
		if(sd->petDB->talk_convert_class < 0)
			return;
		if(sd->petDB->talk_convert_class > 0) {
			param -= (pd->class_ - 100)*100;
			param += (sd->petDB->talk_convert_class - 100)*100;
		}
	}
	WBUFL(buf,6)=param;
	clif_send(buf,packet_db[0x1aa].len,&pd->bl,AREA);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_pet_performance(struct block_list *bl, int param)
{
	unsigned char buf[16];

	nullpo_retv(bl);

	WBUFW(buf,0)=0x1a4;
	WBUFB(buf,2)=4;
	WBUFL(buf,3)=bl->id;
	WBUFL(buf,7)=param;
	clif_send(buf,packet_db[0x1a4].len,bl,AREA);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_pet_equip(struct pet_data *pd, int nameid)
{
	unsigned char buf[16];
	int view;

	nullpo_retv(pd);

	WBUFW(buf,0)=0x1a4;
	WBUFB(buf,2)=3;
	WBUFL(buf,3)=pd->bl.id;
	if((view = itemdb_viewid(nameid)) > 0)
		WBUFL(buf,7)=view;
	else
		WBUFL(buf,7)=nameid;
	clif_send(buf,packet_db[0x1a4].len,&pd->bl,AREA);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_pet_food(struct map_session_data *sd, int foodid, unsigned char fail)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x1a3;
	WFIFOB(fd,2)=fail;
	WFIFOW(fd,3)=foodid;
	WFIFOSET(fd,packet_db[0x1a3].len);

	return;
}

/*==========================================
 * �I�[�g�X�y�� ���X�g���M
 *------------------------------------------
 */
void clif_autospell(struct map_session_data *sd, int skilllv)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd, 0)=0x1cd;

	if(skilllv>0 && pc_checkskill2(sd,MG_NAPALMBEAT)>0)
		WFIFOL(fd,2)= MG_NAPALMBEAT;
	else
		WFIFOL(fd,2)= 0;
	if(skilllv>1 && pc_checkskill2(sd,MG_COLDBOLT)>0)
		WFIFOL(fd,6)= MG_COLDBOLT;
	else
		WFIFOL(fd,6)= 0;
	if(skilllv>1 && pc_checkskill2(sd,MG_FIREBOLT)>0)
		WFIFOL(fd,10)= MG_FIREBOLT;
	else
		WFIFOL(fd,10)= 0;
	if(skilllv>1 && pc_checkskill2(sd,MG_LIGHTNINGBOLT)>0)
		WFIFOL(fd,14)= MG_LIGHTNINGBOLT;
	else
		WFIFOL(fd,14)= 0;
	if(skilllv>4 && pc_checkskill2(sd,MG_SOULSTRIKE)>0)
		WFIFOL(fd,18)= MG_SOULSTRIKE;
	else
		WFIFOL(fd,18)= 0;
	if(skilllv>7 && pc_checkskill2(sd,MG_FIREBALL)>0)
		WFIFOL(fd,22)= MG_FIREBALL;
	else
		WFIFOL(fd,22)= 0;
	if(skilllv>9 && pc_checkskill2(sd,MG_FROSTDIVER)>0)
		WFIFOL(fd,26)= MG_FROSTDIVER;
	else
		WFIFOL(fd,26)= 0;

	WFIFOSET(fd,packet_db[0x1cd].len);

	return;
}

/*==========================================
 * �f�B�{�[�V�����̐���
 *------------------------------------------
 */
void clif_devotion(struct map_session_data *sd)
{
	unsigned char buf[32];
	int n;

	nullpo_retv(sd);

	WBUFW(buf,0)=0x1cf;
	WBUFL(buf,2)=sd->bl.id;
	for(n=0;n<5;n++)
		WBUFL(buf,6+4*n)=sd->dev.val2[n];
	WBUFW(buf,26)=8;
	clif_send(buf,packet_db[0x1cf].len,&sd->bl,AREA);

	return;
}

/*==========================================
 * ����
 *------------------------------------------
 */
void clif_spiritball(struct map_session_data *sd)
{
	unsigned char buf[8];

	nullpo_retv(sd);

	WBUFW(buf,0)=0x1d0;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->spiritball.num;
	clif_send(buf,packet_db[0x1d0].len,&sd->bl,AREA);

	return;
}

/*==========================================
 * �R�C��
 *------------------------------------------
 */
void clif_coin(struct map_session_data *sd)
{
	unsigned char buf[8];

	nullpo_retv(sd);

	WBUFW(buf,0)=0x1d0;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=sd->coin.num;
	clif_send(buf,packet_db[0x1d0].len,&sd->bl,AREA);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_combo_delay(struct block_list *bl, int wait)
{
	unsigned char buf[16];

	nullpo_retv(bl);

	WBUFW(buf,0)=0x1d2;
	WBUFL(buf,2)=bl->id;
	WBUFL(buf,6)=wait;
	clif_send(buf,packet_db[0x1d2].len,bl,AREA);

	return;
}

/*==========================================
 * ���n���
 *------------------------------------------
 */
void clif_bladestop(struct block_list *src, int dst_id, int flag)
{
	unsigned char buf[16];

	nullpo_retv(src);

	WBUFW(buf,0)=0x1d1;
	WBUFL(buf,2)=src->id;
	WBUFL(buf,6)=dst_id;
	WBUFL(buf,10)=flag;
	clif_send(buf,packet_db[0x1d1].len,src,AREA);

	return;
}

/*==========================================
 * �S�X�y���̌��ʕ\��
 *------------------------------------------
 */
void clif_gospel_message(struct map_session_data *sd, int type)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0) = 0x215;
	WFIFOL(fd,2) = type;
	WFIFOSET(fd,packet_db[0x215].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_changemapcell(int m, int x, int y, int cell_type, int type)
{
	struct block_list bl;
	unsigned char buf[24];

	memset(&bl, 0, sizeof(bl));
	bl.m = m;
	bl.x = x;
	bl.y = y;

	WBUFW(buf,0) = 0x192;
	WBUFW(buf,2) = x;
	WBUFW(buf,4) = y;
	WBUFW(buf,6) = cell_type;
	memcpy(WBUFP(buf,8),map[m].name,16);
	clif_send(buf,packet_db[0x192].len,&bl,(type == 0) ? AREA : ALL_SAMEMAP);

	return;
}

/*==========================================
 * MVP�G�t�F�N�g
 *------------------------------------------
 */
void clif_mvp_effect(struct block_list *bl)
{
	unsigned char buf[8];

	nullpo_retv(bl);

	WBUFW(buf,0)=0x10c;
	WBUFL(buf,2)=bl->id;
	clif_send(buf,packet_db[0x10c].len,bl,AREA);

	return;
}

/*==========================================
 * MVP�A�C�e���擾
 *------------------------------------------
 */
void clif_mvp_item(struct map_session_data *sd, int nameid)
{
	int view,fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x10a;
	if((view = itemdb_viewid(nameid)) > 0)
		WFIFOW(fd,2)=view;
	else
		WFIFOW(fd,2)=nameid;
	WFIFOSET(fd,packet_db[0x10a].len);

	return;
}

/*==========================================
 * MVP�A�C�e���d�ʃI�[�o�[
 *------------------------------------------
 */
void clif_mvp_fail_item(struct map_session_data *sd)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x10d;
	WFIFOSET(fd,packet_db[0x10d].len);

	return;
}

/*==========================================
 * MVP�o���l�擾
 *------------------------------------------
 */
void clif_mvp_exp(struct map_session_data *sd, int exp)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x10b;
	WFIFOL(fd,2)=exp;
	WFIFOSET(fd,packet_db[0x10b].len);

	return;
}

/*==========================================
 * �M���h�쐬�ےʒm
 *------------------------------------------
 */
void clif_guild_created(struct map_session_data *sd, unsigned char flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x167;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_db[0x167].len);

	return;
}

/*==========================================
 * �M���h�����ʒm
 *------------------------------------------
 */
void clif_guild_belonginfo(struct map_session_data *sd, struct guild *g)
{
	int ps,fd;

	nullpo_retv(sd);
	nullpo_retv(g);

	ps=guild_getposition(sd,g);
	if(ps < 0)
		return;

	fd=sd->fd;
	WFIFOW(fd,0)=0x16c;
	WFIFOL(fd,2)=g->guild_id;
	WFIFOL(fd,6)=g->emblem_id;
	WFIFOL(fd,10)=g->position[ps].mode;
	WFIFOB(fd,14)=(strcmp(sd->status.name, g->master))? 0: 1;
	WFIFOL(fd,15)=0;	// unknown
	memcpy(WFIFOP(fd,19),g->name,24);
	WFIFOSET(fd,packet_db[0x16c].len);

	return;
}

/*==========================================
 * �M���h�����o���O�C���ʒm
 *------------------------------------------
 */
void clif_guild_memberlogin_notice(struct guild *g, int idx, unsigned char flag)
{
	unsigned char buf[32];

	nullpo_retv(g);

#if PACKETVER < 8
	WBUFW(buf, 0)=0x16d;
	WBUFL(buf, 2)=g->member[idx].account_id;
	WBUFL(buf, 6)=g->member[idx].char_id;
	WBUFL(buf,10)=flag;
	if(g->member[idx].sd==NULL){
		struct map_session_data *sd=guild_getavailablesd(g);
		if(sd!=NULL)
			clif_send(buf,packet_db[0x16d].len,&sd->bl,GUILD);
	}else
		clif_send(buf,packet_db[0x16d].len,&g->member[idx].sd->bl,GUILD_WOS);
#else
	WBUFW(buf, 0)=0x1f2;
	WBUFL(buf, 2)=g->member[idx].account_id;
	WBUFL(buf, 6)=g->member[idx].char_id;
	WBUFL(buf,10)=flag;
	WBUFW(buf,14)=g->member[idx].gender;
	WBUFW(buf,16)=g->member[idx].hair;
	WBUFW(buf,18)=g->member[idx].hair_color;
	if(g->member[idx].sd==NULL){
		struct map_session_data *sd=guild_getavailablesd(g);
		if(sd!=NULL)
			clif_send(buf,packet_db[0x1f2].len,&sd->bl,GUILD);
	}else
		clif_send(buf,packet_db[0x1f2].len,&g->member[idx].sd->bl,GUILD_WOS);
#endif

	return;
}

/*==========================================
 * �M���h�}�X�^�[�ʒm(14d�ւ̉���)
 *------------------------------------------
 */
static void clif_guild_masterormember(struct map_session_data *sd)
{
	int type=0x57,fd;
	struct guild *g;

	nullpo_retv(sd);

	g=guild_search(sd->status.guild_id);
	if(g!=NULL && strcmp(g->master,sd->status.name)==0)
		type=0xd7;

	fd=sd->fd;
	WFIFOW(fd,0)=0x14e;
	WFIFOL(fd,2)=type;
	WFIFOSET(fd,packet_db[0x14e].len);

	return;
}

/*==========================================
 * �M���h��{���
 *------------------------------------------
 */
void clif_guild_basicinfo(struct map_session_data *sd, struct guild *g)
{
	int fd;

	nullpo_retv(sd);
	nullpo_retv(g);

	fd=sd->fd;
	WFIFOW(fd, 0)=0x1b6;//0x150;
	WFIFOL(fd, 2)=g->guild_id;
	WFIFOL(fd, 6)=g->guild_lv;
	WFIFOL(fd,10)=g->connect_member;
	WFIFOL(fd,14)=g->max_member;
	WFIFOL(fd,18)=g->average_lv;
	WFIFOL(fd,22)=g->exp;
	WFIFOL(fd,26)=g->next_exp;
	WFIFOL(fd,30)=0;	// ��[
	WFIFOL(fd,34)=0;	// VW�i���i�̈����H�F�����O���t���E�j
	WFIFOL(fd,38)=0;	// RF�i���`�̓x�����H�F�����O���t�㉺�j
	WFIFOL(fd,42)=g->emblem_id;
	memcpy(WFIFOP(fd,46),g->name,24);
	memcpy(WFIFOP(fd,70),g->master,24);
	strncpy(WFIFOP(fd,94),"",20);	// �{���n
	WFIFOSET(fd,packet_db[0x1b6].len);

	return;
}

/*==========================================
 * �M���h����/�G�Ώ��
 *------------------------------------------
 */
void clif_guild_allianceinfo(struct map_session_data *sd, struct guild *g)
{
	int fd,i,c;

	nullpo_retv(sd);
	nullpo_retv(g);

	fd=sd->fd;
	WFIFOW(fd, 0)=0x14c;
	c = 0;
	for(i = 0; i < MAX_GUILDALLIANCE; i++) {
		struct guild_alliance *a=&g->alliance[i];
		if(a->guild_id>0){
			WFIFOL(fd,c*32+4)=a->opposition;
			WFIFOL(fd,c*32+8)=a->guild_id;
			memcpy(WFIFOP(fd,c*32+12),a->name,24);
			c++;
		}
	}
	WFIFOW(fd, 2)=c*32+4;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 * �M���h�����o�[���X�g
 *------------------------------------------
 */
void clif_guild_memberlist(struct map_session_data *sd, struct guild *g)
{
	int fd,i,c;

	nullpo_retv(sd);
	nullpo_retv(g);

	fd=sd->fd;
	WFIFOW(fd, 0)=0x154;
	c = 0;
	for(i = 0; i < g->max_member; i++) {
		struct guild_member *m=&g->member[i];
		if(m->account_id==0)
			continue;
		WFIFOL(fd,c*104+ 4)=m->account_id;
		WFIFOL(fd,c*104+ 8)=m->char_id;
		WFIFOW(fd,c*104+12)=m->hair;
		WFIFOW(fd,c*104+14)=m->hair_color;
		WFIFOW(fd,c*104+16)=m->gender;
		WFIFOW(fd,c*104+18)=m->class_;
		WFIFOW(fd,c*104+20)=m->lv;
		WFIFOL(fd,c*104+22)=m->exp;
		WFIFOL(fd,c*104+26)=(int)m->online;
		WFIFOL(fd,c*104+30)=m->position;
		memset(WFIFOP(fd,c*104+34),0,50);	// �����H
		memcpy(WFIFOP(fd,c*104+84),m->name,24);
		c++;
	}
	WFIFOW(fd, 2)=c*104+4;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 * �M���h��E�����X�g
 *------------------------------------------
 */
static void clif_guild_positionnamelist(struct map_session_data *sd, struct guild *g)
{
	int i,fd;

	nullpo_retv(sd);
	nullpo_retv(g);

	fd=sd->fd;
	WFIFOW(fd, 0)=0x166;
	for(i=0;i<MAX_GUILDPOSITION;i++){
		WFIFOL(fd,i*28+4)=i;
		memcpy(WFIFOP(fd,i*28+8),g->position[i].name,24);
	}
	WFIFOW(fd,2)=i*28+4;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 * �M���h��E��񃊃X�g
 *------------------------------------------
 */
static void clif_guild_positioninfolist(struct map_session_data *sd, struct guild *g)
{
	int i,fd;

	nullpo_retv(sd);
	nullpo_retv(g);

	fd=sd->fd;
	WFIFOW(fd,0)=0x160;
	for(i=0;i<MAX_GUILDPOSITION;i++){
		struct guild_position *p=&g->position[i];
		WFIFOL(fd,i*16+ 4)=i;
		WFIFOL(fd,i*16+ 8)=p->mode;
		WFIFOL(fd,i*16+12)=i;
		WFIFOL(fd,i*16+16)=p->exp_mode;
	}
	WFIFOW(fd,2)=i*16+4;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 * �M���h��E�ύX�ʒm
 *------------------------------------------
 */
void clif_guild_positionchanged(struct guild *g, int idx)
{
	struct map_session_data *sd;
	unsigned char buf[48];

	nullpo_retv(g);

	if( (sd = guild_getavailablesd(g)) == NULL )
		return;

	WBUFW(buf, 0)=0x174;
	WBUFW(buf, 2)=44;
	WBUFL(buf, 4)=idx;
	WBUFL(buf, 8)=g->position[idx].mode;
	WBUFL(buf,12)=idx;
	WBUFL(buf,16)=g->position[idx].exp_mode;
	memcpy(WBUFP(buf,20),g->position[idx].name,24);
	clif_send(buf,WBUFW(buf,2),&sd->bl,GUILD);

	return;
}

/*==========================================
 * �M���h�����o�ύX�ʒm
 *------------------------------------------
 */
void clif_guild_memberpositionchanged(struct guild *g, int idx)
{
	struct map_session_data *sd;
	unsigned char buf[16];

	nullpo_retv(g);

	if( (sd = guild_getavailablesd(g)) == NULL )
		return;

	WBUFW(buf, 0)=0x156;
	WBUFW(buf, 2)=16;
	WBUFL(buf, 4)=g->member[idx].account_id;
	WBUFL(buf, 8)=g->member[idx].char_id;
	WBUFL(buf,12)=g->member[idx].position;
	clif_send(buf,WBUFW(buf,2),&sd->bl,GUILD);

	return;
}

/*==========================================
 * �M���h�G���u�������M
 *------------------------------------------
 */
void clif_guild_emblem(struct map_session_data *sd, struct guild *g)
{
	int fd;

	nullpo_retv(sd);
	nullpo_retv(g);

	if(g->emblem_len <= 0)
		return;

	fd=sd->fd;
	WFIFOW(fd,0)=0x152;
	WFIFOW(fd,2)=g->emblem_len+12;
	WFIFOL(fd,4)=g->guild_id;
	WFIFOL(fd,8)=g->emblem_id;
	memcpy(WFIFOP(fd,12),g->emblem_data,g->emblem_len);
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 * �M���h�X�L�����M
 *------------------------------------------
 */
void clif_guild_skillinfo(struct map_session_data *sd, struct guild *g)
{
	int fd,i,id,c = 0;

	nullpo_retv(sd);
	nullpo_retv(g);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x162;
	WFIFOW(fd,4) = g->skill_point;
	for(i = 0; i < MAX_GUILDSKILL; i++) {
		if((id = g->skill[i].id) > 0) {
			WFIFOW(fd,c*37+ 6) = id;
			WFIFOL(fd,c*37+ 8) = guild_skill_get_inf(id);
			WFIFOW(fd,c*37+12) = g->skill[i].lv;
			WFIFOW(fd,c*37+14) = skill_get_sp(id,g->skill[i].lv);
			WFIFOW(fd,c*37+16) = skill_get_range(id,g->skill[i].lv);
			memset(WFIFOP(fd,c*37+18),0,24);
			if(g->skill[i].lv < guild_get_skilltree_max(id))
				WFIFOB(fd,c*37+42) = 1;
			else
				WFIFOB(fd,c*37+42) = 0;
			c++;
		}
	}
	WFIFOW(fd,2) = c*37+6;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 * �M���h���m���M
 *------------------------------------------
 */
void clif_guild_notice(struct map_session_data *sd, struct guild *g)
{
	int fd;

	nullpo_retv(sd);
	nullpo_retv(g);

	if(*g->mes1==0 && *g->mes2==0)
		return;

	fd=sd->fd;
	WFIFOW(fd,0)=0x16f;
	memcpy(WFIFOP(fd,2),g->mes1,60);
	memcpy(WFIFOP(fd,62),g->mes2,120);
	WFIFOSET(fd,packet_db[0x16f].len);

	return;
}

/*==========================================
 * �M���h�����o���U
 *------------------------------------------
 */
void clif_guild_invite(struct map_session_data *sd, struct guild *g)
{
	int fd;

	nullpo_retv(sd);
	nullpo_retv(g);

	fd=sd->fd;
	WFIFOW(fd,0)=0x16a;
	WFIFOL(fd,2)=g->guild_id;
	memcpy(WFIFOP(fd,6),g->name,24);
	WFIFOSET(fd,packet_db[0x16a].len);

	return;
}

/*==========================================
 * �M���h�����o���U����
 *------------------------------------------
 */
void clif_guild_inviteack(struct map_session_data *sd, unsigned char flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x169;
	WFIFOB(fd,2)=flag;	// 0: in an other guild, 1: it was denied, 2: join, 3: guild has no more available place
	WFIFOSET(fd,packet_db[0x169].len);

	return;
}

/*==========================================
 * �M���h�����o�E�ޒʒm
 *------------------------------------------
 */
void clif_guild_leave(struct map_session_data *sd, const char *name, const char *mes)
{
	unsigned char buf[72];

	nullpo_retv(sd);

	WBUFW(buf, 0)=0x15a;
	memcpy(WBUFP(buf, 2),name,24);
	memcpy(WBUFP(buf,26),mes,40);
	clif_send(buf,packet_db[0x15a].len,&sd->bl,GUILD);

	return;
}

/*==========================================
 * �M���h�����o�Ǖ��ʒm
 *------------------------------------------
 */
void clif_guild_explusion(struct map_session_data *sd, const char *name, const char *mes)
{
	unsigned char buf[96];

	nullpo_retv(sd);

	WBUFW(buf, 0)=0x15c;
	memcpy(WBUFP(buf, 2),name,24);
	memcpy(WBUFP(buf,26),mes,40);
	strncpy(WBUFP(buf,66),"dummy",24);
	clif_send(buf,packet_db[0x15c].len,&sd->bl,GUILD);

	return;
}

/*==========================================
 * �M���h�Ǖ������o���X�g
 *------------------------------------------
 */
static void clif_guild_explusionlist(struct map_session_data *sd, struct guild *g)
{
	int fd,i,c=0;

	nullpo_retv(sd);
	nullpo_retv(g);

	fd=sd->fd;
	WFIFOW(fd,0)=0x163;
	for(i = 0; i < MAX_GUILDEXPLUSION; i++) {
		struct guild_explusion *e=&g->explusion[i];
		if(e->account_id>0){
			memcpy(WFIFOP(fd,c*88+ 4),e->name,24);
			strncpy(WFIFOP(fd,c*88+28),"dummy",24);
			memcpy(WFIFOP(fd,c*88+52),e->mes,40);
			c++;
		}
	}
	WFIFOW(fd,2)=c*88+4;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 * �M���h��b
 *------------------------------------------
 */
void clif_guild_message(struct guild *g, const char *mes, int len)
{
	struct map_session_data *sd;
	unsigned char *buf;

	nullpo_retv(g);

	if( (sd = guild_getavailablesd(g)) == NULL )
		return;

	buf = (unsigned char *)aMalloc(len+4);
	WBUFW(buf, 0)=0x17f;
	WBUFW(buf, 2)=len+4;
	memcpy(WBUFP(buf,4),mes,len);
	clif_send(buf,WBUFW(buf,2),&sd->bl,GUILD);

	aFree(buf);

	return;
}

/*==========================================
 * �M���h�X�L������U��ʒm
 *------------------------------------------
 */
void clif_guild_skillup(struct map_session_data *sd, int skill_num, int lv, int flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;

#if PACKETVER < 22
	WFIFOW(fd,0) = 0x10e;
	WFIFOW(fd,2) = skill_num;
	WFIFOW(fd,4) = lv;
	WFIFOW(fd,6) = skill_get_sp(skill_num,lv);
	WFIFOW(fd,8) = skill_get_range(skill_num,lv);
	WFIFOB(fd,10) = flag;
	WFIFOSET(fd,packet_db[0x10e].len);
#else
	WFIFOW(fd,0) = 0x7e1;
	WFIFOW(fd,2) = skill_num;
	WFIFOL(fd,4) = 0;
	WFIFOW(fd,8) = lv;
	WFIFOW(fd,10) = skill_get_sp(skill_num,lv);
	WFIFOW(fd,12) = skill_get_range(skill_num,lv);
	WFIFOB(fd,14) = flag;
	WFIFOSET(fd,packet_db[0x7e1].len);
#endif

	return;
}

/*==========================================
 * �M���h�����v��
 *------------------------------------------
 */
void clif_guild_reqalliance(struct map_session_data *sd, int account_id, const char *name)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x171;
	WFIFOL(fd,2)=account_id;
	memcpy(WFIFOP(fd,6),name,24);
	WFIFOSET(fd,packet_db[0x171].len);

	return;
}

/*==========================================
 * �M���h��������
 *------------------------------------------
 */
void clif_guild_allianceack(struct map_session_data *sd, unsigned int flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x173;
	WFIFOL(fd,2)=flag;
	WFIFOSET(fd,packet_db[0x173].len);

	return;
}

/*==========================================
 * �M���h�֌W�����ʒm
 *------------------------------------------
 */
void clif_guild_delalliance(struct map_session_data *sd, int guild_id, int flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x184;
	WFIFOL(fd,2)=guild_id;
	WFIFOL(fd,6)=flag;
	WFIFOSET(fd,packet_db[0x184].len);

	return;
}

/*==========================================
 * �M���h�G�Ό���
 *------------------------------------------
 */
void clif_guild_oppositionack(struct map_session_data *sd, unsigned char flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x181;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,packet_db[0x181].len);

	return;
}

/*==========================================
 * �M���h�֌W�ǉ�
 *------------------------------------------
 */
/*void clif_guild_allianceadded(struct guild *g,int idx)
{
	struct map_session_data *sd;
	unsigned char buf[36];

	nullpo_retv(g);

	if( (sd = guild_getavailablesd(g)) == NULL )
		return;

	WBUFW(buf,0)=0x185;
	WBUFL(buf,2)=g->alliance[idx].opposition;
	WBUFL(buf,6)=g->alliance[idx].guild_id;
	memcpy(WBUFP(buf,10),g->alliance[idx].name,24);
	clif_send(buf,packet_db[0x185].len,&sd->bl,GUILD);

	return;
}*/

/*==========================================
 * �M���h���U�ʒm
 *------------------------------------------
 */
void clif_guild_broken(struct map_session_data *sd, unsigned int flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x15e;
	WFIFOL(fd,2)=flag;
	WFIFOSET(fd,packet_db[0x15e].len);

	return;
}

/*==========================================
 * �M���h�����o�[�̈ʒu���W���M
 *------------------------------------------
 */
void clif_guild_xy(struct map_session_data *sd)
{
	unsigned char buf[10];

	nullpo_retv(sd);

	WBUFW(buf,0)=0x1eb;
	WBUFL(buf,2)=sd->status.account_id;
	WBUFW(buf,6)=sd->bl.x;
	WBUFW(buf,8)=sd->bl.y;
	clif_send(buf,packet_db[0x1eb].len,&sd->bl,GUILD_SAMEMAP_WOS);

	return;
}

/*==========================================
 * �G���[�V����
 *------------------------------------------
 */
void clif_emotion(struct block_list *bl,int type)
{
	unsigned char buf[8];

	nullpo_retv(bl);

	WBUFW(buf,0)=0xc0;
	WBUFL(buf,2)=bl->id;
	WBUFB(buf,6)=type;
	clif_send(buf,packet_db[0xc0].len,bl,AREA);

	return;
}

/*==========================================
 * �g�[�L�[�{�b�N�X
 *------------------------------------------
 */
void clif_talkiebox(struct block_list *bl,char* talkie)
{
	unsigned char buf[86];

	nullpo_retv(bl);

	WBUFW(buf,0)=0x191;
	WBUFL(buf,2)=bl->id;
	memcpy(WBUFP(buf,6),talkie,80);
	clif_send(buf,packet_db[0x191].len,bl,AREA);

	return;
}

/*==========================================
 * �����G�t�F�N�g
 *------------------------------------------
 */
void clif_wedding_effect(struct block_list *bl)
{
	unsigned char buf[6];

	nullpo_retv(bl);

	WBUFW(buf,0)=0x1ea;
	WBUFL(buf,2)=bl->id;
	clif_send(buf,packet_db[0x1ea].len,bl,AREA);

	return;
}

/*==========================================
 * ���Ȃ��Ɉ��������g�p�����O����
 *------------------------------------------
 */
void clif_callpartner(struct map_session_data *sd)
{
	unsigned char buf[26];
	char *p;

	nullpo_retv(sd);

	if(sd->status.partner_id <= 0)
		return;

	WBUFW(buf,0)=0x1e6;
	p = map_charid2nick(sd->status.partner_id);
	if(p)
		memcpy(WBUFP(buf,2),p,24);
	else
		strncpy(WBUFP(buf,2),"",24);
	clif_send(buf,packet_db[0x1e6].len,&sd->bl,AREA);

	return;
}

/*==========================================
 * ���������ʒm
 *------------------------------------------
 */
void clif_divorced(struct map_session_data *sd, const char *name)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0) = 0x205;
	strncpy(WFIFOP(fd,2),name,24);
	WFIFOSET(fd,packet_db[0x205].len);

	return;
}

/*==========================================
 * ����/����
 *------------------------------------------
 */
void clif_sitting(struct block_list *bl, int sit)
{
	unsigned char buf[32];

	nullpo_retv(bl);

	WBUFW(buf,0)  = 0x8a;
	WBUFL(buf,2)  = bl->id;
	WBUFB(buf,26) = (sit)? 2: 3;
	clif_send(buf,packet_db[0x8a].len,bl,AREA);

	return;
}

/*==========================================
 * ����
 *------------------------------------------
 */
void clif_onlymessage(const char *mes, size_t len)
{
	unsigned char *buf = (unsigned char *)aMalloc(len+8);

	switch (battle_config.mes_send_type) {
		case 0:	// �M���h��b
			WBUFW(buf, 0)=0x17f;
			WBUFW(buf, 2)=(unsigned short)(len+4);
			memcpy(WBUFP(buf,4),mes,len);
			break;
		case 1:	// �I�[�v����b
			WBUFW(buf, 0)=0x8d;
			WBUFW(buf, 2)=(unsigned short)(len+8);
			WBUFL(buf, 4)=0;
			memcpy(WBUFP(buf,8),mes,len);
			break;
		case 2:	// �p�[�e�B��b
			WBUFW(buf, 0)=0x109;
			WBUFW(buf, 2)=(unsigned short)(len+8);
			WBUFL(buf, 4)=0xffffffff;	// account_id��0���Ɩ������Ȃ̂Ń_�~�[�����Ă���
			memcpy(WBUFP(buf,8),mes,len);
			break;
		case 3:	// �I�[�v��GM��b
			WBUFW(buf, 0)=0x8d;
			WBUFW(buf, 2)=(unsigned short)(len+8);
			WBUFL(buf, 4)=pc_get_gm_account_dummy();	// �_�~�[�pGM�A�J�E���g������
			memcpy(WBUFP(buf,8),mes,len);
			break;
		default:
			aFree(buf);
			return;
	}
	clif_send(buf,WBUFW(buf,2),NULL,ALL_CLIENT);
	aFree(buf);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_GM_kickack(struct map_session_data *sd, int id)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xcd;
	WFIFOL(fd,2)=id;
	WFIFOSET(fd,packet_db[0xcd].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_GM_kick(struct map_session_data *sd, struct map_session_data *tsd, int type)
{
	int fd;

	nullpo_retv(tsd);

	if(type)
		clif_GM_kickack(sd,tsd->status.account_id);

	fd = tsd->fd;
	WFIFOW(fd,0)=0x18b;
	WFIFOW(fd,2)=0;
	WFIFOSET(fd,packet_db[0x18b].len);

	clif_setwaitclose(fd);

	return;
}

/*==========================================
 * Wis���ۋ�����
 *------------------------------------------
 */
static void clif_wisexin(struct map_session_data *sd, int type, unsigned char flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xd1;
	WFIFOB(fd,2)=type;
	WFIFOB(fd,3)=flag;
	WFIFOSET(fd,packet_db[0xd1].len);

	return;
}

/*==========================================
 * Wis�S���ۋ�����
 *------------------------------------------
 */
static void clif_wisall(struct map_session_data *sd, int type, unsigned char flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0xd2;
	WFIFOB(fd,2)=type;
	WFIFOB(fd,3)=flag;
	WFIFOSET(fd,packet_db[0xd2].len);

	return;
}

/*==========================================
 * �T�E���h�G�t�F�N�g
 *------------------------------------------
 */
void clif_soundeffect(struct map_session_data *sd,struct block_list *bl,const char *name,int type,int interval)
{
	int fd;

	nullpo_retv(sd);

	if(bl == NULL)
		return;

	fd=sd->fd;
	WFIFOW(fd,0)=0x1d3;
	strncpy(WFIFOP(fd,2),name,24);
	WFIFOB(fd,26)=type;
	WFIFOL(fd,27)=interval;
	WFIFOL(fd,31)=bl->id;
	WFIFOSET(fd,packet_db[0x1d3].len);

	return;
}

/*==========================================
 * �F�B���X�g�̏��ʒm
 *------------------------------------------
 */
void clif_friend_send_info(struct map_session_data *sd)
{
	int len,i,fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd, 0) = 0x201;
	for(i=0, len=4; i<sd->status.friend_num; i++, len+=32) {
		struct friend_data *frd = &sd->status.friend_data[i];
		WFIFOL(fd,len  ) = frd->account_id;
		WFIFOL(fd,len+4) = frd->char_id;
		memcpy(WFIFOP(fd,len+8), frd->name, 24);
	}
	WFIFOW(fd,2) = len;
	WFIFOSET(fd,len);

	return;
}

/*==========================================
 * �F�B���X�g�̃I�����C�����ʒm
 *------------------------------------------
 */
void clif_friend_send_online(const int fd, int account_id, int char_id, int flag)
{
	WFIFOW(fd, 0) = 0x206;
	WFIFOL(fd, 2) = account_id;
	WFIFOL(fd, 6) = char_id;
	WFIFOB(fd,10) = flag;
	WFIFOSET(fd,packet_db[0x206].len);

	return;
}

/*==========================================
 * �F�B���X�g�ǉ��v��
 *------------------------------------------
 */
void clif_friend_add_request(const int fd, struct map_session_data *sd)
{
	nullpo_retv(sd);

	WFIFOW(fd,0) = 0x207;
	WFIFOL(fd,2) = sd->bl.id;
	WFIFOL(fd,6) = sd->status.char_id;
	memcpy(WFIFOP(fd,10), sd->status.name, 24);
	WFIFOSET(fd,packet_db[0x207].len);

	return;
}

/*==========================================
 * �F�B���X�g�ǉ��v���ԓ�
 *------------------------------------------
 */
void clif_friend_add_ack(const int fd, int account_id, int char_id, const char* name, unsigned short flag)
{
	WFIFOW(fd,0) = 0x209;
	WFIFOW(fd,2) = flag;
	WFIFOL(fd,4) = account_id;
	WFIFOL(fd,8) = char_id;
	strncpy(WFIFOP(fd,12), name, 24);
	WFIFOSET(fd,packet_db[0x209].len);

	return;
}

/*==========================================
 * �F�B���X�g�ǉ��폜�ʒm
 *------------------------------------------
 */
void clif_friend_del_ack(const int fd, int account_id, int char_id)
{
	WFIFOW(fd,0) = 0x20a;
	WFIFOL(fd,2) = account_id;
	WFIFOL(fd,6) = char_id;
	WFIFOSET(fd,packet_db[0x20a].len);

	return;
}

/*==========================================
 * BS�����L���O�|�C���g
 *------------------------------------------
 */
void clif_blacksmith_point(const int fd,const int total,const int point)
{
	WFIFOW(fd,0) = 0x21b;
	WFIFOL(fd,2) = point;
	WFIFOL(fd,6) = total;
	WFIFOSET(fd,packet_db[0x21b].len);

	return;
}

/*==========================================
 * BS�����L���O
 *------------------------------------------
 */
void clif_blacksmith_ranking(const int fd,const char *charname[10],const int point[10])
{
	int i;

	WFIFOW(fd,0) = 0x219;
	for(i=0;i<10;i++){
		strncpy(WFIFOP(fd,i*24+2), charname[i], 24);
		WFIFOL(fd,i*4+242) = point[i];
	}
	WFIFOSET(fd,packet_db[0x219].len);

	return;
}

/*==========================================
 * �A���P�~�����L���O�|�C���g
 *------------------------------------------
 */
void clif_alchemist_point(const int fd,const int total,const int point)
{
	WFIFOW(fd,0) = 0x21c;
	WFIFOL(fd,2) = point;
	WFIFOL(fd,6) = total;
	WFIFOSET(fd,packet_db[0x21c].len);

	return;
}

/*==========================================
 * �A���P�~�����L���O
 *------------------------------------------
 */
void clif_alchemist_ranking(const int fd,const char *charname[10],const int point[10])
{
	int i;

	WFIFOW(fd,0) = 0x21a;
	for(i=0;i<10;i++){
		strncpy(WFIFOP(fd,i*24+2), charname[i], 24);
		WFIFOL(fd,i*4+242) = point[i];
	}
	WFIFOSET(fd,packet_db[0x21a].len);

	return;
}

/*==========================================
 * �e�R�������L���O�|�C���g
 *------------------------------------------
 */
void clif_taekwon_point(const int fd,const int total,const int point)
{
	WFIFOW(fd,0) = 0x224;
	WFIFOL(fd,2) = point;
	WFIFOL(fd,6) = total;
	WFIFOSET(fd,packet_db[0x224].len);

	return;
}

/*==========================================
 * �e�R�������L���O
 *------------------------------------------
 */
void clif_taekwon_ranking(const int fd,const char *charname[10],const int point[10])
{
	int i;

	WFIFOW(fd,0) = 0x226;
	for(i=0;i<10;i++){
		strncpy(WFIFOP(fd,i*24+2), charname[i], 24);
		WFIFOL(fd,i*4+242) = point[i];
	}
	WFIFOSET(fd,packet_db[0x226].len);

	return;
}

/*==========================================
 * �s�E�҃����L���O�|�C���g
 *------------------------------------------
 */
void clif_pk_point(const int fd,const int total,const int point)
{
	WFIFOW(fd,0) = 0x236;
	WFIFOL(fd,2) = point;
	WFIFOL(fd,6) = total;
	WFIFOSET(fd,packet_db[0x236].len);

	return;
}

/*==========================================
 * �s�E�҃����L���O
 *------------------------------------------
 */
void clif_pk_ranking(const int fd,const char *charname[10],const int point[10])
{
	int i;

	WFIFOW(fd,0) = 0x238;
	for(i=0;i<10;i++){
		strncpy(WFIFOP(fd,i*24+2), charname[i], 24);
		WFIFOL(fd,i*4+242) = point[i];
	}
	WFIFOSET(fd,packet_db[0x238].len);

	return;
}

/*==========================================
 * ���[��BOX��\��������
 *------------------------------------------
 */
void clif_openmailbox(const int fd)
{
	WFIFOW(fd,0) = 0x260;
	WFIFOL(fd,2) = 0;
	WFIFOSET(fd,packet_db[0x260].len);

	return;
}

/*==========================================
 * ���[���ꗗ�\�iBOX���J���Ă��鎞�ɑ��֑��M�j
 *  0x23f�̉���
 *------------------------------------------
 */
void clif_send_mailbox(struct map_session_data *sd,int store,struct mail_data md[MAIL_STORE_MAX])
{
	int i,fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0) = 0x240;
	WFIFOL(fd,4) = store;	// 0�̂Ƃ��͉�ʂ̃N���A
	for(i=0;i<store;i++){
		WFIFOL(fd,8+73*i)  = md[i].mail_num;
		memcpy(WFIFOP(fd,12+73*i),md[i].title,40);
		WFIFOB(fd,52+73*i) = md[i].read;
		memcpy(WFIFOP(fd,53+73*i),md[i].char_name,24);
		WFIFOL(fd,77+73*i) = md[i].times;
	}
	WFIFOW(fd,2) = 8+73*i;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 * ���[�������M�ł��܂����Ƃ��ł��܂���Ƃ�
 *------------------------------------------
 */
void clif_res_sendmail(const int fd,int fail)
{
	WFIFOW(fd,0) = 0x249;
	WFIFOB(fd,2) = fail;
	WFIFOSET(fd,packet_db[0x249].len);

	return;
}

/*==========================================
 * �A�C�e�����Y�t�ł��܂����Ƃ��ł��܂���Ƃ�
 *------------------------------------------
 */
void clif_res_sendmail_setappend(const int fd,int idx,int flag)
{
	WFIFOW(fd,0) = 0x255;
	WFIFOW(fd,2) = idx + 2;
	WFIFOB(fd,4) = flag;
	WFIFOSET(fd,packet_db[0x256].len);

	return;
}

/*==========================================
 * �Y�t�A�C�e���擾
 *------------------------------------------
 */
void clif_mail_getappend(const int fd,int flag)
{
	WFIFOW(fd,0) = 0x245;
	WFIFOB(fd,2) = flag;
	WFIFOSET(fd,packet_db[0x245].len);

	return;
}

/*==========================================
 * �V�����[�����͂��܂���
 *------------------------------------------
 */
void clif_arrive_newmail(const int fd,struct mail_data *md)
{
	nullpo_retv(md);

	WFIFOW(fd,0) = 0x24a;
	WFIFOL(fd,2) = md->mail_num;
	memcpy(WFIFOP(fd, 6),md->char_name,24);
	memcpy(WFIFOP(fd,30),md->title,40);
	WFIFOSET(fd,packet_db[0x24a].len);

	return;
}

/*==========================================
 * ���[����I����M	Inter���{�l��
 *------------------------------------------
 */
void clif_receive_mail(struct map_session_data *sd,struct mail_data *md)
{
	int fd,view;

	nullpo_retv(sd);
	nullpo_retv(md);

	fd=sd->fd;
	WFIFOW(fd,0)=0x242;
	WFIFOW(fd,2)=md->body_size+99;
	WFIFOL(fd,4)=md->mail_num;
	memcpy(WFIFOP(fd, 8),md->title,40);
	memcpy(WFIFOP(fd,48),md->char_name,24);
	WFIFOL(fd,72)=0x22;	// ���E�H
	WFIFOL(fd,76)=md->zeny;
	WFIFOL(fd,80)=md->item.amount;
	if((view = itemdb_viewid(md->item.nameid)) > 0)
		WFIFOW(fd,84)=view;
	else
		WFIFOW(fd,84)=md->item.nameid;
	WFIFOW(fd,86)=0;
	WFIFOB(fd,88)=md->item.identify;
	WFIFOB(fd,89)=md->item.attribute;
	WFIFOB(fd,90)=md->item.refine;
	WFIFOW(fd,91)=md->item.card[0];
	WFIFOW(fd,93)=md->item.card[1];
	WFIFOW(fd,95)=md->item.card[2];
	WFIFOW(fd,97)=md->item.card[3];
	memcpy(WFIFOP(fd,99),md->body,md->body_size);
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 * ���[�����폜�ł��܂����Ƃ��ł��܂���Ƃ�
 *------------------------------------------
 */
void clif_deletemail_res(const int fd,int mail_num,int flag)
{
	WFIFOW(fd,0) = 0x257;
	WFIFOL(fd,2) = mail_num;
	WFIFOW(fd,6) = flag;
	WFIFOSET(fd,packet_db[0x257].len);

	return;
}

/*==========================================
 * �����]��
 *------------------------------------------
 */
void clif_changedir(struct block_list *bl, int headdir, int dir)
{
	unsigned char buf[16];

	nullpo_retv(bl);

	WBUFW(buf,0)=0x9c;
	WBUFL(buf,2)=bl->id;
	WBUFW(buf,6)=headdir;
	WBUFB(buf,8)=(unsigned char)dir;
	clif_send(buf,packet_db[0x9c].len,bl,AREA_WOS);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_send_homdata(struct map_session_data *sd, int state, int param)
{
	int fd;

	nullpo_retv(sd);
	nullpo_retv(sd->hd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x230;
	WFIFOB(fd,2)=0;
	WFIFOB(fd,3)=state;
	WFIFOL(fd,4)=sd->hd->bl.id;
	WFIFOL(fd,8)=param;
	WFIFOSET(fd,packet_db[0x230].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_send_homstatus(struct map_session_data *sd, int flag)
{
	int fd;
	struct homun_data *hd;

	nullpo_retv(sd);
	nullpo_retv((hd=sd->hd));

	fd=sd->fd;
	WFIFOW(fd,0)=0x22e;
	memcpy(WFIFOP(fd,2),hd->status.name,24);
	WFIFOB(fd,26)=(unit_isdead(&hd->bl))? 2:hd->status.rename_flag;	// ���O�t�����t���O 1�ŕύX�s�� 2�͎��S���
	WFIFOW(fd,27)=hd->status.base_level;	// Lv
	WFIFOW(fd,29)=hd->status.hungry;		// �����x
	WFIFOW(fd,31)=hd->intimate/100;	// �V���x
	WFIFOW(fd,33)=hd->status.equip;			// equip id
	WFIFOW(fd,35)=hd->atk;					// Atk
	WFIFOW(fd,37)=hd->matk;					// MAtk
	WFIFOW(fd,39)=hd->hit;					// Hit
	WFIFOW(fd,41)=hd->critical;				// Cri
	WFIFOW(fd,43)=hd->def;					// Def
	WFIFOW(fd,45)=hd->mdef;					// Mdef
	WFIFOW(fd,47)=hd->flee;					// Flee
	WFIFOW(fd,49)=(flag)?0:status_get_amotion(&hd->bl);	// Aspd
	WFIFOW(fd,51)=hd->status.hp;			// HP
	WFIFOW(fd,53)=hd->max_hp;		// MHp
	WFIFOW(fd,55)=hd->status.sp;			// SP
	WFIFOW(fd,57)=hd->max_sp;		// MSP
	WFIFOL(fd,59)=hd->status.base_exp;		// Exp
	WFIFOL(fd,63)=homun_nextbaseexp(hd);	// NextExp
	WFIFOW(fd,67)=hd->status.skill_point;	// skill point
	WFIFOW(fd,69)=hd->attackable;			// �U���ۃt���O 0:�s��/1:����
	WFIFOSET(fd,packet_db[0x22e].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_hom_food(struct map_session_data *sd, int foodid, unsigned char fail)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x22f;
	WFIFOB(fd,2)=fail;
	WFIFOW(fd,3)=foodid;
	WFIFOSET(fd,packet_db[0x22f].len);

	return;
}

/*==========================================
 * �z���̃X�L�����X�g�𑗐M����
 *------------------------------------------
 */
void clif_homskillinfoblock(struct map_session_data *sd)
{
	int fd;
	int i,len=4,id,skill_lv;
	struct homun_data *hd;

	nullpo_retv(sd);
	nullpo_retv(hd = sd->hd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x235;
	for(i = 0; i < MAX_HOMSKILL; i++) {
		if((id = hd->status.skill[i].id) != 0) {
			skill_lv = hd->status.skill[i].lv;
			WFIFOW(fd,len  )  = id;
			WFIFOL(fd,len+2)  = skill_get_inf(id);
			WFIFOW(fd,len+6)  = skill_lv;
			WFIFOW(fd,len+8)  = skill_get_sp(id,skill_lv);
			WFIFOW(fd,len+10) = skill_get_fixed_range(&hd->bl,id,skill_lv);
			memset(WFIFOP(fd,len+12),0,24);
			if(!(skill_get_inf2(id)&0x01))
				WFIFOB(fd,len+36) = (skill_lv < homun_get_skilltree_max(hd->status.class_,id) && hd->status.skill[i].flag == 0)? 1: 0;
			else
				WFIFOB(fd,len+36) = 0;
			len += 37;
		}
	}
	WFIFOW(fd,2) = len;
	WFIFOSET(fd,len);

	return;
}

/*==========================================
 * �z���̃X�L������U��ʒm
 *------------------------------------------
 */
void clif_homskillup(struct map_session_data *sd, int skill_num)
{
	int fd,skillid;
	struct homun_data *hd;

	nullpo_retv(sd);
	nullpo_retv((hd=sd->hd));

	skillid = skill_num-HOM_SKILLID;

	fd=sd->fd;
	WFIFOW(fd,0) = 0x239;
	WFIFOW(fd,2) = skill_num;
	WFIFOW(fd,4) = hd->status.skill[skillid].lv;
	WFIFOW(fd,6) = skill_get_sp(skill_num,hd->status.skill[skillid].lv);
	WFIFOW(fd,8) = skill_get_fixed_range(&hd->bl,skill_num,hd->status.skill[skillid].lv);
	WFIFOB(fd,10) = (hd->status.skill[skillid].lv < homun_get_skilltree_max(hd->status.class_,hd->status.skill[skillid].id)) ? 1 : 0;
	WFIFOSET(fd,packet_db[0x239].len);

	return;
}

/*==========================================
 * ���z�ƌ��Ɛ��̊���̓o�^����
 *------------------------------------------
 */
static void clif_feel_saveack(struct map_session_data *sd, int lv)
{
	int fd;

	nullpo_retv(sd);

	if(lv < 0 || lv > 2)
		return;

	if(pc_checkskill(sd,SG_FEEL) < lv + 1)
		return;

	if(battle_config.save_feel_map)
		strncpy(sd->status.feel_map[lv], map[sd->bl.m].name, 24);
	sd->feel_index[lv] = sd->bl.m;

	clif_misceffect2(&sd->bl, 432);
	clif_misceffect2(&sd->bl, 543);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x20e;
	memcpy(WFIFOP(fd,2), map[sd->bl.m].name, 24);
	WFIFOL(fd,26) = sd->bl.id;
	WFIFOB(fd,30) = lv;
	WFIFOB(fd,31) = 0;
	WFIFOSET(fd,packet_db[0x20e].len);

	return;
}

/*==========================================
 * ���z�ƌ��Ɛ��̊���̓o�^���
 *------------------------------------------
 */
void clif_feel_info(struct map_session_data *sd, int skilllv)
{
	int fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x20e;
	strncpy(WFIFOP(fd,2),sd->status.feel_map[skilllv-1],24);
	WFIFOL(fd,26) = sd->bl.id;
	WFIFOB(fd,30) = skilllv-1;
	WFIFOB(fd,31) = 1;
	WFIFOSET(fd,packet_db[0x20e].len);

	return;
}

/*==========================================
 * ���z�ƌ��Ɛ��̑����݂̓o�^����
 *------------------------------------------
 */
void clif_hate_mob(struct map_session_data *sd, int skilllv, int id)
{
	int fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x20e;
	if(mobdb_checkid(id) > 0) {
		strncpy(WFIFOP(fd,2),mob_db[id].jname,24);
	} else {
		memset(WFIFOP(fd,2), 0, 24);
	}
	WFIFOL(fd,26) = id;
	WFIFOB(fd,30) = skilllv-1;
	WFIFOB(fd,31) = 10;
	WFIFOSET(fd,packet_db[0x20e].len);

	return;
}

/*==========================================
 * ���z�ƌ��Ɛ��̑����݂̓o�^���
 *------------------------------------------
 */
void clif_hate_info(struct map_session_data *sd, int skilllv, int id)
{
	int fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x20e;
	if(mobdb_checkid(id) > 0) {
		strncpy(WFIFOP(fd,2),mob_db[id].jname,24);
	} else {
		memset(WFIFOP(fd,2), 0, 24);
	}
	WFIFOL(fd,26) = id;
	WFIFOB(fd,30) = skilllv-1;
	WFIFOB(fd,31) = 11;
	WFIFOSET(fd,packet_db[0x20e].len);

	return;
}

/*==========================================
 * �e�R���~�b�V����
 *------------------------------------------
 */
void clif_mission_mob(struct map_session_data *sd, int id, int count)
{
	int fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x20e;
	strncpy(WFIFOP(fd,2),mob_db[id].jname,24);
	WFIFOL(fd,26) = id;
	WFIFOB(fd,30) = count;
	WFIFOB(fd,31) = 20;
	WFIFOSET(fd,packet_db[0x20e].len);

	return;
}

/*==========================================
 * ���z�ƌ��Ɛ��̓V�g
 *------------------------------------------
 */
void clif_angel_message(struct map_session_data *sd)
{
	int fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x20e;
	WFIFOL(fd,26) = sd->bl.id;
	WFIFOB(fd,30) = 0;
	WFIFOB(fd,31) = 30;
	WFIFOSET(fd,packet_db[0x20e].len);

	return;
}

/*==========================================
 * ���z�ƌ��Ɛ��̊���̃_�C�A���O�\��
 *------------------------------------------
 */
void clif_feel_display(struct map_session_data *sd, int skilllv)
{
	int fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x253;
	WFIFOB(fd,2) = skilllv-1;
	WFIFOSET(fd,packet_db[0x253].len);

	return;
}

/*==========================================
 * �}�[�_���[
 *------------------------------------------
 */
void clif_send_murderer(struct map_session_data *sd,int target,int flag)
{
	int fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x220;
	WFIFOL(fd,2) = target;
	WFIFOL(fd,6) = flag;
	WFIFOSET(fd,packet_db[0x220].len);

	return;
}

/*==========================================
 * temper�X�V
 *------------------------------------------
 */
void clif_update_temper(struct map_session_data *sd)
{
	int fd;
	char *nick1 = NULL, *nick2 = NULL;

	nullpo_retv(sd);

	if(sd->kill.char_id > 0) {
		nick1 = map_charid2nick(sd->kill.char_id);
		if(nick1 == NULL) {
			if(map_reqchariddb(sd,sd->kill.char_id,2))
				chrif_searchcharid(sd->kill.char_id);
		}
	}
	if(sd->kill.merderer_char_id > 0) {
		nick2 = map_charid2nick(sd->kill.merderer_char_id);
		if(nick2 == NULL) {
			if(map_reqchariddb(sd,sd->kill.merderer_char_id,2))
				chrif_searchcharid(sd->kill.merderer_char_id);
		}
	}

	fd = sd->fd;
	WFIFOW(fd,0)  = 0x21f;
	WFIFOL(fd,2)  = 0;	// ??
	WFIFOL(fd,6)  = 0;	// ??
	strncpy(WFIFOP(fd,10), (nick1 ? nick1 : ""), 24);
	strncpy(WFIFOP(fd,34), (nick2 ? nick2 : ""), 24);
	WFIFOL(fd,58) = 0;	// ??
	WFIFOL(fd,62) = 0;	// ??
	WFIFOSET(fd,packet_db[0x21f].len);

	return;
}

/*==========================================
 * �z�b�g�L�[�̑��M
 *------------------------------------------
 */
void clif_send_hotkey(struct map_session_data *sd)
{
#if PACKETVER >= 10
	int i, j, fd, hotkeys;

#if PACKETVER >= 21
	hotkeys = 38;
#elif PACKETVER > 20
	hotkeys = 36;
#else
	hotkeys = 27;
#endif

	nullpo_retv(sd);

	fd = sd->fd;

#if PACKETVER < 20
	memset(WFIFOP(fd,0), 0, packet_db[0x2b9].len);

	WFIFOW(fd,0) = 0x2b9;
	for(i = 0, j = sd->hotkey_set * hotkeys; i < hotkeys && j < MAX_HOTKEYS; i++, j++) {
		WFIFOB(fd,7*i+2) = sd->status.hotkey[j].type;
		WFIFOL(fd,7*i+3) = sd->status.hotkey[j].id;
		WFIFOW(fd,7*i+7) = sd->status.hotkey[j].lv;
	}
	WFIFOSET(fd,packet_db[0x2b9].len);
#else
	memset(WFIFOP(fd,0), 0, packet_db[0x7d9].len);

	WFIFOW(fd,0) = 0x7d9;
	for(i = 0, j = sd->hotkey_set * hotkeys; i < hotkeys && j < MAX_HOTKEYS; i++, j++) {
		WFIFOB(fd,7*i+2) = sd->status.hotkey[j].type;
		WFIFOL(fd,7*i+3) = sd->status.hotkey[j].id;
		WFIFOW(fd,7*i+7) = sd->status.hotkey[j].lv;
	}
	WFIFOSET(fd,packet_db[0x7d9].len);
#endif
#endif

	return;
}

/*==========================================
 * MVP�{�X���
 *------------------------------------------
 */
void clif_bossmapinfo(struct map_session_data *sd, const char *name, int x, int y, int tick, int type)
{
	int fd;

	nullpo_retv(sd);

	fd   = sd->fd;
	tick = tick / 1000 / 60;	// minute

	WFIFOW(fd,0)  = 0x293;
	WFIFOB(fd,2)  = type;
	WFIFOL(fd,3)  = x;
	WFIFOL(fd,7)  = y;
	WFIFOW(fd,11) = tick / 60;
	WFIFOW(fd,13) = tick % 60;
	WFIFOL(fd,15) = 0;
	strncpy(WFIFOP(fd,19), name, 50);
	WFIFOB(fd,69) = 0;	// \0 terminal
	WFIFOSET(fd,packet_db[0x293].len);

	return;
}

/*==========================================
 * �Ǐ��E�B���h�E�̕\��
 *------------------------------------------
 */
void clif_openbook(struct map_session_data *sd, int nameid, int page)
{
	int fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0)  = 0x294;
	WFIFOL(fd,2)  = nameid;
	WFIFOL(fd,6)  = page;
	WFIFOSET(fd,packet_db[0x294].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void clif_send_mercstatus(struct map_session_data *sd)
{
	int fd, val;
	struct merc_data *mcd;

	nullpo_retv(sd);
	nullpo_retv(mcd = sd->mcd);

	val = mcd->atk2 - mcd->atk1 + 1;

	fd = sd->fd;
	WFIFOW(fd,0)  = 0x29b;
	WFIFOL(fd,2)  = mcd->bl.id;
	WFIFOW(fd,6)  = (val > 0)? atn_rand() % val + mcd->atk1 : 0;
	WFIFOW(fd,8)  = mcd->matk1;
	WFIFOW(fd,10) = mcd->hit;
	WFIFOW(fd,12) = mcd->critical;
	WFIFOW(fd,14) = mcd->def;
	WFIFOW(fd,16) = mcd->mdef;
	WFIFOW(fd,18) = mcd->flee;
	WFIFOW(fd,20) = mcd->amotion;
	memcpy(WFIFOP(fd,22), mcd->status.name, 24);
	WFIFOW(fd,46) = mcd->status.base_level;
#if PACKETVER < 12
	WFIFOW(fd,48) = mcd->status.hp;
	WFIFOW(fd,50) = mcd->max_hp;
	WFIFOW(fd,52) = mcd->status.sp;
	WFIFOW(fd,54) = mcd->max_sp;
	WFIFOL(fd,56) = mcd->status.limit;	// �ٗp����
	WFIFOW(fd,60) = sd->status.merc_fame[mcd->class_type];	// �����l
	WFIFOL(fd,62) = sd->status.merc_call[mcd->class_type];	// ������
	WFIFOL(fd,66) = mcd->status.kill_count;	// �L���J�E���g
	WFIFOW(fd,70) = mcd->attackrange;	// �U���͈�
#else
	WFIFOL(fd,48) = mcd->status.hp;
	WFIFOL(fd,52) = mcd->max_hp;
	WFIFOL(fd,56) = mcd->status.sp;
	WFIFOL(fd,60) = mcd->max_sp;
	WFIFOL(fd,64) = mcd->status.limit;	// �ٗp����
	WFIFOW(fd,68) = sd->status.merc_fame[mcd->class_type];	// �����l
	WFIFOL(fd,70) = sd->status.merc_call[mcd->class_type];	// ������
	WFIFOL(fd,74) = mcd->status.kill_count;	// �L���J�E���g
	WFIFOW(fd,78) = mcd->attackrange;	// �U���͈�
#endif
	WFIFOSET(fd,packet_db[0x29b].len);

	return;
}

/*==========================================
 * �b���̃X�L�����X�g�𑗐M����
 *------------------------------------------
 */
void clif_mercskillinfoblock(struct map_session_data *sd)
{
	int fd;
	int i,len=4,id,skill_lv;
	struct merc_data *mcd;

	nullpo_retv(sd);
	nullpo_retv(mcd = sd->mcd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x29d;
	for(i = 0; i < MAX_MERCSKILL; i++) {
		if((id = mcd->status.skill[i].id) != 0) {
			skill_lv = mcd->status.skill[i].lv;
			WFIFOW(fd,len  )  = id;
			WFIFOL(fd,len+2)  = skill_get_inf(id);
			WFIFOW(fd,len+6)  = skill_lv;
			WFIFOW(fd,len+8)  = skill_get_sp(id,skill_lv);
			WFIFOW(fd,len+10) = skill_get_fixed_range(&mcd->bl,id,skill_lv);
			memset(WFIFOP(fd,len+12),0,24);
			if(!(skill_get_inf2(id)&0x01))
				WFIFOB(fd,len+36) = (skill_lv < merc_get_skilltree_max(mcd->status.class_,id) && mcd->status.skill[i].flag == 0)? 1: 0;
			else
				WFIFOB(fd,len+36) = 0;
			len += 37;
		}
	}
	WFIFOW(fd,2) = len;
	WFIFOSET(fd,len);

	return;
}

/*==========================================
 * �b���̃X�e�[�^�X�𑗂����
 *------------------------------------------
 */
void clif_mercupdatestatus(struct map_session_data *sd, int type)
{
	int fd;
	struct merc_data *mcd;

	nullpo_retv(sd);
	nullpo_retv(mcd = sd->mcd);

	fd = sd->fd;

	WFIFOW(fd,0)  = 0x2a2;
	WFIFOW(fd,2)  = type;
	switch(type){
	case SP_HP:
		WFIFOL(fd,4) = mcd->status.hp;
		break;
	case SP_MAXHP:
		WFIFOL(fd,4) = mcd->max_hp;
		break;
	case SP_SP:
		WFIFOL(fd,4) = mcd->status.sp;
		break;
	case SP_MAXSP:
		WFIFOL(fd,4) = mcd->max_sp;
		break;
	case SP_ATK1:
		{
			int val = mcd->atk2 - mcd->atk1 + 1;
			WFIFOL(fd,4) = (val > 0)? atn_rand()%val + mcd->atk1 : 0;
		}
		break;
	case SP_MATK1:
		WFIFOL(fd,4) = mcd->matk1;
		break;
	case SP_DEF1:
		WFIFOL(fd,4) = mcd->def;
		break;
	case SP_MDEF1:
		WFIFOL(fd,4) = mcd->mdef;
		break;
	case SP_HIT:
		WFIFOL(fd,4) = mcd->hit;
		break;
	case SP_FLEE1:
	case SP_MERC_FLEE:
		WFIFOL(fd,4) = mcd->flee;
		break;
	case SP_CRITICAL:
		WFIFOL(fd,4) = mcd->critical;
		break;
	case SP_ASPD:
		WFIFOL(fd,4) = mcd->amotion;
		break;
	case SP_MERC_KILLCOUNT:
		WFIFOL(fd,4) = mcd->status.kill_count;
		break;
	case SP_MERC_FAME:
		WFIFOL(fd,4) = sd->status.merc_fame[mcd->class_type];
		break;
	}
	WFIFOSET(fd,packet_db[0x2a2].len);

	return;
}

/*==========================================
 * msgstringtable�\��
 *------------------------------------------
 */
void clif_msgstringtable(struct map_session_data *sd, int line)
{
	int fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x291;
	WFIFOW(fd,2) = line;
	WFIFOSET(fd,packet_db[0x291].len);

	return;
}

/*==========================================
 * �p�[�e�B�[�����o�[�A�C�e���l�����b�Z�[�W
 *------------------------------------------
 */
void clif_show_partyshareitem(struct map_session_data *sd, struct item *item_data)
{
	unsigned char buf[24];
	struct item_data *id;

	nullpo_retv(sd);
	nullpo_retv(item_data);

	id = itemdb_search(item_data->nameid);

	WBUFW(buf,0)=0x2b8;
	WBUFL(buf,2) = sd->status.account_id;
	WBUFW(buf,6) = item_data->nameid;
	WBUFB(buf,8) = item_data->identify;
	WBUFB(buf,9) = item_data->attribute;
	WBUFB(buf,10) = item_data->refine;
	WBUFW(buf,11) = item_data->card[0];
	WBUFW(buf,13) = item_data->card[1];
	WBUFW(buf,15) = item_data->card[2];
	WBUFW(buf,17) = item_data->card[3];
	WBUFW(buf,19) = id->equip;
	WBUFB(buf,21) = id->type;
	clif_send(buf,packet_db[0x2b8].len,&sd->bl,PARTY_SAMEMAP_WOS);

	return;
}

/*==========================================
 * �����j��
 *------------------------------------------
 */
void clif_break_equip(struct map_session_data *sd, int where)
{
#if PACKETVER >= 12
	unsigned char buf[16];

	nullpo_retv(sd);

	WBUFW(buf,0) = 0x2bb;
	WBUFW(buf,2) = where;
	WBUFL(buf,4) = sd->bl.id;
	clif_send(buf,packet_db[0x2bb].len,&sd->bl,PARTY_SAMEMAP_WOS);
#endif

	return;
}

/*==========================================
 * �����E�B���h�E���
 *------------------------------------------
 */
void clif_party_equiplist(struct map_session_data *sd, struct map_session_data *tsd)
{
	int i, j, n, fd;

	nullpo_retv(sd);
	nullpo_retv(tsd);

	fd = sd->fd;

#if PACKETVER < 28
	WFIFOW(fd,0) = 0x2d7;
	memcpy(WFIFOP(fd,4), tsd->status.name, 24);
	if(tsd->status.class_ == PC_CLASS_GS || tsd->status.class_ == PC_CLASS_NJ)
		WFIFOW(fd,28) = tsd->status.class_-4;
	else
		WFIFOW(fd,28) = tsd->status.class_;
	WFIFOW(fd,30) = tsd->status.hair;
	WFIFOW(fd,32) = tsd->status.head_top;
	WFIFOW(fd,34) = tsd->status.head_mid;
	WFIFOW(fd,36) = tsd->status.head_bottom;
	WFIFOW(fd,38) = tsd->status.hair_color;
	WFIFOW(fd,40) = tsd->status.clothes_color;
	WFIFOB(fd,42) = tsd->sex;

	for(i = 0, n = 0; i < MAX_INVENTORY; i++) {
		if(tsd->status.inventory[i].nameid <= 0 || tsd->inventory_data[i] == NULL || !itemdb_isequip2(tsd->inventory_data[i]))
			continue;
		WFIFOW(fd,n*26+43) = i + 2;
		if(tsd->inventory_data[i]->view_id > 0)
			WFIFOW(fd,n*26+45) = tsd->inventory_data[i]->view_id;
		else
			WFIFOW(fd,n*26+45) = tsd->status.inventory[i].nameid;
		WFIFOB(fd,n*26+47) = tsd->inventory_data[i]->type;
		WFIFOB(fd,n*26+48) = tsd->status.inventory[i].identify;
		WFIFOW(fd,n*26+49) = pc_equippoint(tsd,i);
		WFIFOW(fd,n*26+51) = tsd->status.inventory[i].equip;
		WFIFOB(fd,n*26+53) = tsd->status.inventory[i].attribute;
		WFIFOB(fd,n*26+54) = tsd->status.inventory[i].refine;
		if(itemdb_isspecial(tsd->status.inventory[n].card[0])) {
			if(tsd->inventory_data[i]->flag.pet_egg) {
				WFIFOW(fd,n*26+55) = 0;
				WFIFOW(fd,n*26+57) = 0;
				WFIFOW(fd,n*26+59) = 0;
			} else {
				WFIFOW(fd,n*26+55) = tsd->status.inventory[i].card[0];
				WFIFOW(fd,n*26+57) = tsd->status.inventory[i].card[1];
				WFIFOW(fd,n*26+59) = tsd->status.inventory[i].card[2];
			}
			WFIFOW(fd,n*26+61) = tsd->status.inventory[i].card[3];
		} else {
			if(tsd->status.inventory[i].card[0] > 0 && (j = itemdb_viewid(tsd->status.inventory[i].card[0])) > 0)
				WFIFOW(fd,n*26+55) = j;
			else
				WFIFOW(fd,n*26+55) = tsd->status.inventory[i].card[0];
			if(tsd->status.inventory[i].card[1] > 0 && (j = itemdb_viewid(tsd->status.inventory[i].card[1])) > 0)
				WFIFOW(fd,n*26+57) = j;
			else
				WFIFOW(fd,n*26+57) = tsd->status.inventory[i].card[1];
			if(tsd->status.inventory[i].card[2] > 0 && (j = itemdb_viewid(tsd->status.inventory[i].card[2])) > 0)
				WFIFOW(fd,n*26+59) = j;
			else
				WFIFOW(fd,n*26+59) = tsd->status.inventory[i].card[2];
			if(tsd->status.inventory[i].card[3] > 0 && (j = itemdb_viewid(tsd->status.inventory[i].card[3])) > 0)
				WFIFOW(fd,n*26+61) = j;
			else
				WFIFOW(fd,n*26+61) = tsd->status.inventory[i].card[3];
		}
		WFIFOL(fd,n*26+63) = tsd->status.inventory[i].limit;
		WFIFOW(fd,n*26+67) = 0;
		n++;
	}
	if(n) {
		WFIFOW(fd,2) = 43 + n*26;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#else
	WFIFOW(fd,0) = 0x2d7;
	memcpy(WFIFOP(fd,4), tsd->status.name, 24);
	if(tsd->status.class_ == PC_CLASS_GS || tsd->status.class_ == PC_CLASS_NJ)
		WFIFOW(fd,28) = tsd->status.class_-4;
	else
		WFIFOW(fd,28) = tsd->status.class_;
	WFIFOW(fd,30) = tsd->status.hair;
	WFIFOW(fd,32) = tsd->status.head_bottom;
	WFIFOW(fd,34) = tsd->status.head_mid;
	WFIFOW(fd,36) = tsd->status.head_top;
	WFIFOW(fd,38) = tsd->status.hair_color;
	WFIFOW(fd,40) = tsd->status.clothes_color;
	WFIFOB(fd,42) = tsd->sex;

	for(i = 0, n = 0; i < MAX_INVENTORY; i++) {
		if(tsd->status.inventory[i].nameid <= 0 || tsd->inventory_data[i] == NULL || !itemdb_isequip2(tsd->inventory_data[i]))
			continue;
		WFIFOW(fd,n*28+43) = i + 2;
		if(tsd->inventory_data[i]->view_id > 0)
			WFIFOW(fd,n*28+45) = tsd->inventory_data[i]->view_id;
		else
			WFIFOW(fd,n*28+45) = tsd->status.inventory[i].nameid;
		WFIFOB(fd,n*28+47) = tsd->inventory_data[i]->type;
		WFIFOB(fd,n*28+48) = tsd->status.inventory[i].identify;
		WFIFOW(fd,n*28+49) = pc_equippoint(tsd,i);
		WFIFOW(fd,n*28+51) = tsd->status.inventory[i].equip;
		WFIFOB(fd,n*28+53) = tsd->status.inventory[i].attribute;
		WFIFOB(fd,n*28+54) = tsd->status.inventory[i].refine;
		if(itemdb_isspecial(tsd->status.inventory[n].card[0])) {
			if(tsd->inventory_data[i]->flag.pet_egg) {
				WFIFOW(fd,n*28+55) = 0;
				WFIFOW(fd,n*28+57) = 0;
				WFIFOW(fd,n*28+59) = 0;
			} else {
				WFIFOW(fd,n*28+55) = tsd->status.inventory[i].card[0];
				WFIFOW(fd,n*28+57) = tsd->status.inventory[i].card[1];
				WFIFOW(fd,n*28+59) = tsd->status.inventory[i].card[2];
			}
			WFIFOW(fd,n*28+61) = tsd->status.inventory[i].card[3];
		} else {
			if(tsd->status.inventory[i].card[0] > 0 && (j = itemdb_viewid(tsd->status.inventory[i].card[0])) > 0)
				WFIFOW(fd,n*28+55) = j;
			else
				WFIFOW(fd,n*28+55) = tsd->status.inventory[i].card[0];
			if(tsd->status.inventory[i].card[1] > 0 && (j = itemdb_viewid(tsd->status.inventory[i].card[1])) > 0)
				WFIFOW(fd,n*28+57) = j;
			else
				WFIFOW(fd,n*28+57) = tsd->status.inventory[i].card[1];
			if(tsd->status.inventory[i].card[2] > 0 && (j = itemdb_viewid(tsd->status.inventory[i].card[2])) > 0)
				WFIFOW(fd,n*28+59) = j;
			else
				WFIFOW(fd,n*28+59) = tsd->status.inventory[i].card[2];
			if(tsd->status.inventory[i].card[3] > 0 && (j = itemdb_viewid(tsd->status.inventory[i].card[3])) > 0)
				WFIFOW(fd,n*28+61) = j;
			else
				WFIFOW(fd,n*28+61) = tsd->status.inventory[i].card[3];
		}
		WFIFOL(fd,n*28+63) = tsd->status.inventory[i].limit;
		WFIFOW(fd,n*28+67) = 0;
		if(tsd->inventory_data[n]->equip&LOC_HEAD_TMB)
			WFIFOW(fd,n*28+69)=tsd->inventory_data[n]->look;
		else
			WFIFOW(fd,n*28+69)=0;
		n++;
	}
	if(n) {
		WFIFOW(fd,2) = 43 + n*28;
		WFIFOSET(fd,WFIFOW(fd,2));
	}
#endif

	return;
}

/*==========================================
 * ���������J���Ă��邩�ǂ���
 *------------------------------------------
 */
void clif_send_equipopen(struct map_session_data *sd)
{
#if PACKETVER >= 12
	int fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x2da;
	WFIFOB(fd,2) = sd->state.show_equip;
	WFIFOSET(fd,packet_db[0x2da].len);
#endif

	return;
}

/*==========================================
 * �X�L���ŗL�f�B���C�\��
 *------------------------------------------
 */
void clif_skill_cooldown(struct map_session_data *sd, int skillid, unsigned int tick)
{
#if PACKETVER > 16
	int fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x043d;
	WFIFOW(fd,2) = skillid;
	WFIFOL(fd,4) = tick;
	WFIFOSET(fd,packet_db[0x43d].len);
#endif

	return;
}

/*==========================================
 * �~���j�A���V�[���h
 *------------------------------------------
 */
void clif_mshield(struct map_session_data *sd, int num)
{
	unsigned char buf[10];

	nullpo_retv(sd);

	WBUFW(buf,0)=0x440;
	WBUFL(buf,2)=sd->bl.id;
	WBUFW(buf,6)=num;
	WBUFW(buf,8)=0;
	clif_send(buf,packet_db[0x440].len,&sd->bl,AREA);

	return;
}

/*==========================================
 * �I�[�g�V���h�E�X�y��
 *------------------------------------------
 */
void clif_autoshadowspell(struct map_session_data *sd, short lv)
{
	int fd, c = 0;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd, 0)=0x442;

	if(skill_get_skill_type(sd->skill_clone.id) == BF_MAGIC) {
		WFIFOW(fd,8+c*2) = sd->skill_clone.id;
		c++;
	}

	if(skill_get_skill_type(sd->skill_reproduce.id) == BF_MAGIC) {
		WFIFOW(fd,8+c*2) = sd->skill_reproduce.id;
		c++;
	}

	WFIFOW(fd,2) = c * 2 + 8;
	WFIFOSET(fd, WFIFOW(fd,2));

	if(c > 0) {
		sd->skill_menu.id = SC_AUTOSHADOWSPELL;
		sd->skill_menu.lv = lv;
	}

	return;
}

/*==========================================
 * NPC�C�x���g�\��
 *------------------------------------------
 */
void clif_showevent(struct map_session_data *sd, struct block_list *bl, short state, short type)
{
#if PACKETVER > 19
	int fd;

	nullpo_retv(sd);
	nullpo_retv(bl);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x446;
	WFIFOL(fd,2) = bl->id;
	WFIFOW(fd,6) = bl->x;
	WFIFOW(fd,8) = bl->y;
	WFIFOW(fd,10) = state;
	WFIFOW(fd,12) = type;
	WFIFOSET(fd,packet_db[0x446].len);
#endif

	return;
}

/*==========================================
 * �o���l�擾�\��
 *------------------------------------------
 */
void clif_dispexp(struct map_session_data *sd, int exp, short type, short quest)
{
	int fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x7f6;
	WFIFOL(fd,2) = sd->bl.id;
	WFIFOL(fd,6) = exp;
	WFIFOW(fd,10) = type;
	WFIFOW(fd,12) = quest;
	WFIFOSET(fd,packet_db[0x7f6].len);

    return;
}

/*==========================================
 * �p�[�e�B�[���[�_�[�ύX���
 *------------------------------------------
 */
void clif_partyleader_info(struct party *p, int old_account_id, int account_id)
{
	unsigned char buf[16];
	int i;
	struct map_session_data *sd = map_id2sd(account_id);

	nullpo_retv(p);

	WBUFW(buf,0) = 0x7fc;
	WBUFL(buf,2) = old_account_id;
	WBUFL(buf,6) = account_id;

	if(!sd) {	// ���[�_�[���I�t���C���Ȃ̂Ń��O�C�����̃����o�[��T��
		for(i=0; i<MAX_PARTY && !p->member[i].sd; i++);
		if(i >= MAX_PARTY)
			return;
		sd = p->member[i].sd;
	}
	clif_send(buf,packet_db[0x7fc].len,&sd->bl,PARTY);

	return;
}

/*==========================================
 * ���y�t�@�C����炷
 *------------------------------------------
 */
void clif_musiceffect(struct map_session_data *sd, const char *name)
{
#if PACKETVER > 23
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0)=0x7fe;
	strncpy(WFIFOP(fd,2),name,24);
	WFIFOSET(fd,packet_db[0x7fe].len);
#endif

	return;
}

/*==========================================
 * �p�[�e�B�[�u�b�L���O�o�^�v������
 *------------------------------------------
 */
void clif_bookingregack(struct map_session_data *sd, int flag)
{
	int fd;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0) = 0x803;
	WFIFOW(fd,2) = flag;
	WFIFOSET(fd,packet_db[0x803].len);

	return;
}

/*==========================================
 * �p�[�e�B�[�u�b�L���O�����v������
 *------------------------------------------*/
void clif_searchbookingack(struct map_session_data *sd, struct booking_data **list, int count, int flag)
{
	int i,j,fd;
	int n=0;

	nullpo_retv(sd);

	fd=sd->fd;
	WFIFOW(fd,0) = 0x805;
	if(list) {
		for(i=0; i<count; i++) {
			struct booking_data *bd = list[i];
			WFIFOL(fd,n*48+5)=bd->id; 
			memcpy(WFIFOP(fd,n*48+9),bd->name,24);
			WFIFOL(fd,n*48+33)=bd->time;
			WFIFOW(fd,n*48+37)=bd->lv;
			WFIFOW(fd,n*48+39)=bd->map;
			for(j=0; j<6; j++)
				WFIFOW(fd,n*48+41+j*2) = bd->job[j];
			n++;
		}
	}
	WFIFOW(fd,2)=5+n*48;
	WFIFOB(fd,4)=flag;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 * �p�[�e�B�[�u�b�L���O�o�^�폜�v������
 *------------------------------------------*/
void clif_deletebookingack(struct map_session_data* sd, int flag)
{
	int fd;

	nullpo_retv(sd);

	fd = sd->fd;
	WFIFOW(fd,0) = 0x807;
	WFIFOW(fd,2) = flag;
	WFIFOSET(fd,packet_db[0x807].len);

	return;
}

/*==========================================
 * �p�[�e�B�[�u�b�L���O���X�g�ǉ�
 *------------------------------------------*/
void clif_insertbookinglist(struct map_session_data *sd, struct booking_data *bd)
{
	int i;
	unsigned char buf[50];

	nullpo_retv(sd);
	nullpo_retv(bd);

	WBUFW(buf,0) = 0x809;
	WBUFL(buf,2) = bd->id;
	memcpy(WBUFP(buf,6),bd->name,24);
	WBUFL(buf,30) = bd->time;
	WBUFW(buf,34) = bd->lv;
	WBUFW(buf,36) = bd->map;
	for(i=0; i<6; i++)
		WBUFW(buf,38+i*2) = bd->job[i];
	clif_send(buf,packet_db[0x809].len,&sd->bl,ALL_CLIENT);

	return;
}

/*==========================================
 * �p�[�e�B�[�u�b�L���O���X�g�ǉ�
 *------------------------------------------*/
void clif_updatebookinglist(struct map_session_data* sd, struct booking_data *bd)
{
	int i;
	unsigned char buf[18];

	nullpo_retv(sd);
	nullpo_retv(bd);

	WBUFW(buf,0) = 0x80a;
	WBUFL(buf,2) = bd->id;
	for(i=0; i<6; i++)
		WBUFW(buf,6+i*2) = bd->job[i];
	clif_send(buf,packet_db[0x80a].len,&sd->bl,ALL_CLIENT);

	return;
}

/*==========================================
 * �p�[�e�B�[�u�b�L���O�o�^�폜
 *------------------------------------------*/
void clif_deletebooking(struct map_session_data* sd, int id)
{
	unsigned char buf[6];

	nullpo_retv(sd);

	WBUFW(buf,0) = 0x80b;
	WBUFL(buf,2) = id;
	clif_send(buf,packet_db[0x80b].len,&sd->bl,ALL_CLIENT);

	return;
}

/*==========================================
 * send packet �f�o�b�O�p
 *------------------------------------------
 */
void clif_send_packet(struct map_session_data *sd, const char *message)
{
	int fd,n,i;
	int packet,len,db_len;
	int p[256];

	nullpo_retv(sd);

	// �C�ӂ̓ǂݍ��݂�30byte�܂ŋ�����
	n = sscanf(message, "%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
				&packet,&p[2],&p[3],&p[4],&p[5],&p[6],&p[7],&p[8],&p[9],
				&p[10],&p[11],&p[12],&p[13],&p[14],&p[15],&p[16],&p[17],&p[18],&p[19],
				&p[20],&p[21],&p[22],&p[23],&p[24],&p[25],&p[26],&p[27],&p[28],&p[29]);
	if(n == 0)
		return;
	if(packet < 0 || packet >= MAX_PACKET_DB)
		return;
	db_len = packet_db[packet].len;
	if(db_len > 255){
		printf("clif_send_packet_test too long packet!!\n");
		return;
	}

	fd = sd->fd;
	WFIFOW(fd,0) = packet;
	p[0] = packet&0xff;
	p[1] = (packet>>8)&0xff;

	if(db_len >= 0) {
		len = db_len;
		i = 2;
	} else {	// �ϒ�
		if(n == 1 || p[2] <= 0){
			printf("clif_send_packet_test packet:%x len=-1 input len %cpacket len x1 x2...\n", packet, GM_Symbol());
			return;
		}
		WFIFOW(fd,2) = (len=p[2]);
		i = 4;		// p[2]��short�^�Ƃ��Ď�邽��p[3]�͖�������
	}

	for(; i < len; i++) {
		if(i < n+1)
			WFIFOB(fd,i)= p[i];
		else
			WFIFOB(fd,i)= (p[i]=atn_rand()%256);
	}
	printf("clif_send_packet: test packet = 0x%x, len = %d\n", packet, db_len);
	hex_dump(stdout, WFIFOP(fd,0), len);
	printf("\n");

	WFIFOSET(fd,len);

	return;
}

// ------------
// clif_parse_*
// ------------
// �p�P�b�g�ǂݎ���ĐF�X����
/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_WantToConnection(int fd,struct map_session_data *sd, int cmd)
{
	struct map_session_data *old_sd;
	int account_id,char_id,login_id1,sex;
	unsigned int client_tick;

	if(sd) {
		if(battle_config.error_log)
			printf("clif_parse_WantToConnection : invalid request?\n");
		return;
	}

	account_id  = RFIFOL(fd,GETPACKETPOS(cmd,0));
	char_id     = RFIFOL(fd,GETPACKETPOS(cmd,1));
	login_id1   = RFIFOL(fd,GETPACKETPOS(cmd,2));
	client_tick = RFIFOL(fd,GETPACKETPOS(cmd,3));
	sex         = RFIFOB(fd,GETPACKETPOS(cmd,4));

	// �A�J�E���gID�̕s���`�F�b�N
	if(account_id < START_ACCOUNT_NUM || account_id > END_ACCOUNT_NUM) {
		printf("clif_parse_WantToConnection : invalid Account ID !!\n");
		return;
	}
	// Sex�̃`�F�b�N
	if(sex < 0 || sex > 1) {
		printf("clif_parse_WantToConnection : invalid Sex !!\n");
		return;
	}

	session[fd]->session_data = aCalloc(1,sizeof(struct map_session_data));
	sd = (struct map_session_data *)session[fd]->session_data;
	sd->fd = fd;

	pc_setnewpc(sd, account_id, char_id, login_id1, client_tick, sex);
	if((old_sd = map_id2sd(account_id)) != NULL) {
		// 2�dlogin�Ȃ̂Őؒf�p�̃f�[�^��ۑ�����
		old_sd->new_fd=fd;
		sd->new_fd = -1; // �V�����f�[�^�̓Z�[�u���Ȃ��t���O
	} else {
		map_addiddb(&sd->bl);
	}

#if PACKETVER < 9
	WFIFOL(fd,0) = sd->bl.id; // account_id
	WFIFOSET(fd,4);
#else
	WFIFOW(fd,0) = 0x283;
	WFIFOL(fd,2) = sd->bl.id;
	WFIFOSET(fd,packet_db[0x283].len);
#endif

	if(chrif_authreq(sd))
		session[fd]->eof = 1;

	return;
}

/*==========================================
 * 007d �N���C�A���g���}�b�v�ǂݍ��݊���
 * map�N�����ɕK�v�ȃf�[�^��S�đ������
 *------------------------------------------
 */
static void clif_parse_LoadEndAck(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	if(sd->bl.prev != NULL)
		return;

	if(!sd->state.auth) {	// �F�؊������ĂȂ�
		clif_authfail_fd(fd,0);
		return;
	}

	if(sd->npc_id) {
		npc_event_dequeue(sd);
	}

	// item
	pc_checkitem(sd);
	clif_itemlist(sd);
	clif_equiplist(sd);
	// cart
	if(pc_iscarton(sd)){
		clif_cart_itemlist(sd);
		clif_cart_equiplist(sd);
		clif_updatestatus(sd,SP_CARTINFO);
	}
	// weight max , now
	clif_updatestatus(sd,SP_MAXWEIGHT);
	clif_updatestatus(sd,SP_WEIGHT);

	if(battle_config.pc_invincible_time > 0) {
		if(map[sd->bl.m].flag.gvg)
			pc_setinvincibletimer(sd,battle_config.pc_invincible_time<<1);
		else
			pc_setinvincibletimer(sd,battle_config.pc_invincible_time);
	}

	map_addblock(&sd->bl);	// �u���b�N�o�^
	if(!battle_config.gm_perfect_hide || !pc_isinvisible(sd)) {
		clif_spawnpc(sd);	// spawn
	}
	mob_ai_hard_spawn(&sd->bl, 1);

	// party
	party_send_movemap(sd);
	// guild
	guild_send_memberinfoshort(sd,1);
	// friend
	friend_send_info(sd);
	friend_send_online(sd, 0);

	// pvp
	if(sd->pvp_timer != -1)
		delete_timer(sd->pvp_timer,pc_calc_pvprank_timer);

	if(map[sd->bl.m].flag.pk) {
		sd->pvp_timer = -1;
		clif_set0199(sd->fd,4);
		if(battle_config.pk_noshift)
			sd->status.karma = 1;
	} else if(map[sd->bl.m].flag.pvp) {
		sd->pvp_timer     = add_timer(gettick()+200,pc_calc_pvprank_timer,sd->bl.id,NULL);
		sd->pvp_rank      = 0;
		sd->pvp_lastusers = 0;
		sd->pvp_point     = 5;
		clif_set0199(sd->fd,1);
	} else {
		sd->pvp_timer = -1;
	}

	if(map[sd->bl.m].flag.gvg)
		clif_set0199(sd->fd,3);

	if(battle_config.pk_noshift && !map[sd->bl.m].flag.pk)
		sd->status.karma = 0;

	// �y�b�g
	if(sd->status.pet_id > 0 && sd->pd && sd->pet.intimate > 0) {
		map_addblock(&sd->pd->bl);
		clif_spawnpet(sd->pd);
		clif_send_petdata(sd,0,0);
		clif_send_petdata(sd,5,0x14);
		clif_send_petstatus(sd);
	}
	// �z��
	if(sd->hd) {
		map_addblock(&sd->hd->bl);
		mob_ai_hard_spawn(&sd->hd->bl, 1);
		clif_spawnhom(sd->hd);
		clif_send_homdata(sd,0,0);
		clif_send_homstatus(sd,1);
		clif_send_homstatus(sd,0);
	}
	// �b��
	if(sd->mcd) {
		map_addblock(&sd->mcd->bl);
		mob_ai_hard_spawn( &sd->mcd->bl, 1 );
		clif_spawnmerc(sd->mcd);
		clif_send_mercstatus(sd);
		clif_mercskillinfoblock(sd);
	}

	if(sd->state.connect_new) {
		// �X�e�[�^�X�ُ�f�[�^�v��
		// pc_authok() �ŌĂяo���ƃN���C�A���g�ɃG�t�F�N�g�����f����Ȃ�
		intif_request_scdata(sd->status.account_id,sd->status.char_id);

		clif_skillinfoblock(sd);
		clif_send_hotkey(sd);
		clif_send_equipopen(sd);
		clif_updatestatus(sd,SP_NEXTBASEEXP);
		clif_updatestatus(sd,SP_NEXTJOBEXP);
		clif_updatestatus(sd,SP_SKILLPOINT);
		clif_initialstatus(sd);

		// �L���[��񑗐M
		if(battle_config.save_pckiller_type)
			clif_update_temper(sd);

		if(sd->status.class_ != sd->view_class)
			clif_changelook(&sd->bl,LOOK_BASE,sd->view_class);
		if(sd->status.pet_id > 0 && sd->pd && sd->pet.intimate > 900)
			clif_pet_emotion(sd->pd,(sd->pd->class_ - 100)*100 + 50 + pet_hungry_val(sd));
	}

	// param
	clif_updatestatus(sd,SP_STR);
	clif_updatestatus(sd,SP_AGI);
	clif_updatestatus(sd,SP_VIT);
	clif_updatestatus(sd,SP_INT);
	clif_updatestatus(sd,SP_DEX);
	clif_updatestatus(sd,SP_LUK);

	// view equipment item
#if PACKETVER < 4
	clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
	clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);
#else
	clif_changelook(&sd->bl,LOOK_WEAPON,0);
#endif

	// option�n������(�N���C�A���g�ɑ΂���...)
	// ����p�P��2�񑗂邱�Ƃł��\���ł����A�C���������̂�0�ŏ������Ƃ����`��
	clif_changeoption(&sd->bl);
	clif_changeoption(&sd->bl);

	clif_send_clothcolor(&sd->bl);

	// �b��ԃG���h�~����
	if(sd->status.manner < 0) {
		if(battle_config.nomanner_mode)
			sd->status.manner = 0;
		else
			status_change_start(&sd->bl,SC_NOCHAT,0,0,0,0,0,0);
	}

	map_foreachinarea(clif_getareachar,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,BL_ALL,sd);

	if(sd->state.connect_new) {
		sd->state.connect_new = 0;

		if(pc_isriding(sd) || pc_isdragon(sd))
			clif_status_load(sd,SI_RIDING,1);
		if(pc_isfalcon(sd))
			clif_status_load(sd,SI_FALCON,1);

		// OnPCLogin�C�x���g
		if(battle_config.pc_login_script)
			npc_event_doall_id("OnPCLogin",sd->bl.id,sd->bl.m);
	}

	if(map_getcell(sd->bl.m,sd->bl.x,sd->bl.y,CELL_CHKNPC))
		npc_touch_areanpc(sd,sd->bl.m,sd->bl.x,sd->bl.y);
	else
		sd->areanpc_id = 0;

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_TickSend(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	sd->client_tick = RFIFOL(fd,GETPACKETPOS(cmd,0));
	clif_servertick(sd);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_WalkToXY(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	if(unit_isdead(&sd->bl)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}

	if( sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->chatID != 0 )
		return;
	if( sd->state.storage_flag )
		return;
	if( pc_issit(sd) )
		return;

	if( !unit_isrunning(&sd->bl) ) {
		int x = RFIFOB(fd,GETPACKETPOS(cmd,0))*4+(RFIFOB(fd,GETPACKETPOS(cmd,0)+1)>>6);
		int y = ((RFIFOB(fd,GETPACKETPOS(cmd,0)+1)&0x3f)<<4)+(RFIFOB(fd,GETPACKETPOS(cmd,0)+2)>>4);

		if( unit_walktoxy(&sd->bl,x,y) ) {
			unit_stopattack(&sd->bl);
			if(sd->invincible_timer != -1)
				pc_delinvincibletimer(sd);
		}
	}
	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_QuitGame(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	WFIFOW(fd,0)=0x18b;
	if(pc_isquitable(sd)){
		WFIFOW(fd,2)=1; // flag= 0: success, 1: failure (please wait 15 sec...)
		WFIFOSET(fd,packet_db[0x18b].len);
		return;
	}
	WFIFOW(fd,2)=0;
	WFIFOSET(fd,packet_db[0x18b].len);
	clif_setwaitclose(fd);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_GetCharNameRequest(int fd,struct map_session_data *sd, int cmd)
{
	struct block_list *bl;
	int account_id;

	account_id = RFIFOL(fd,GETPACKETPOS(cmd,0));
	bl=map_id2bl(account_id);
	if(bl==NULL)
		return;
	if(sd->bl.m != bl->m || unit_distance2(&sd->bl,bl) > AREA_SIZE)
		return;

	memset(WFIFOP(fd,0), 0, packet_db[0x195].len);
	WFIFOW(fd,0) = 0x95;
	WFIFOL(fd,2) = account_id;

	switch(bl->type){
	case BL_PC:
		{
			struct map_session_data *ssd = (struct map_session_data *)bl;
			struct party *p = NULL;
			struct guild *g = NULL;

			memcpy(WFIFOP(fd,6), ssd->status.name, 24);
			if (ssd->status.party_id > 0)
				p = party_search(ssd->status.party_id);
			if (ssd->status.guild_id > 0)
				g = guild_search(ssd->status.guild_id);

			if ((battle_config.show_always_party_name && p != NULL) || g != NULL) {
				// �M���h�����Ȃ�p�P�b�g0195��Ԃ�
				WFIFOW(fd, 0) = 0x195;
				if (p)
					memcpy(WFIFOP(fd,30), p->name, 24);
				if (g) {
					int i, ps = -1;
					memcpy(WFIFOP(fd,54), g->name, 24);
					for(i = 0; i < g->max_member; i++) {
						if (g->member[i].char_id == ssd->status.char_id) { // char_id is unique, not need to check account_id
							ps = g->member[i].position;
							break;
						}
					}
					if (ps >= 0 && ps < MAX_GUILDPOSITION)
						memcpy(WFIFOP(fd,78), g->position[ps].name, 24);
					else
						strncpy(WFIFOP(fd,78), msg_txt(45), 24); // No Position
				}
				WFIFOSET(fd,packet_db[0x195].len);
			} else {
				WFIFOSET(fd,packet_db[0x95].len);
			}
			// �}�[�_���[
			if(battle_config.pk_murderer_point > 0) {
				if(ranking_get_point(ssd,RK_PK) >= battle_config.pk_murderer_point)
					clif_send_murderer(sd,ssd->bl.id,1);
			}
		}
		break;
	case BL_NPC:
		{
			struct npc_data *snd=(struct npc_data*)bl;
			memcpy(WFIFOP(fd,6),snd->name,24);
			if(snd->position[0]) {
				// �M���h�����Ȃ�p�P�b�g0195��Ԃ�
				WFIFOW(fd, 0)=0x195;
				strncpy(WFIFOP(fd,30),"NPC",24);	// PT��
				strncpy(WFIFOP(fd,54),"NPC",24);	// Guild��
				memcpy(WFIFOP(fd,78),snd->position,24);
				WFIFOSET(fd,packet_db[0x195].len);
				break;
			}
			WFIFOSET(fd,packet_db[0x95].len);
		}
		break;
	case BL_PET:
		memcpy(WFIFOP(fd,6),((struct pet_data*)bl)->name,24);
		WFIFOSET(fd,packet_db[0x95].len);
		break;
	case BL_MOB:
		memcpy(WFIFOP(fd,6),((struct mob_data*)bl)->name,24);
		WFIFOSET(fd,packet_db[0x95].len);
		break;
	case BL_HOM:
		memcpy(WFIFOP(fd,6),((struct homun_data*)bl)->status.name,24);
		WFIFOSET(fd,packet_db[0x95].len);
		break;
	case BL_MERC:
		memcpy(WFIFOP(fd,6),((struct merc_data*)bl)->status.name,24);
		WFIFOSET(fd,packet_db[0x95].len);
		break;
	default:
		if(battle_config.error_log)
			printf("clif_parse_GetCharNameRequest : bad type %d(%d)\n",bl->type,account_id);
		break;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_GlobalMessage(int fd,struct map_session_data *sd, int cmd)
{
	char *message;
	int len, message_size;

	nullpo_retv(sd);

	len = RFIFOW(fd, GETPACKETPOS(cmd,0));
	message = (char *)RFIFOP(fd, GETPACKETPOS(cmd,1));
	message_size = len - GETPACKETPOS(cmd,1); // including NULL

	if (message_size < 8)	// name (mini:4) + " : " (3) + NULL (1) (void mesages are possible for skills)
		return;
	if (message_size > 255)	// too long
		return;

	message[message_size - 1] = 0; // be sure to have a NULL (hacker can send a no-NULL terminated string)

	if (battle_config.check_player_name_global_msg) {
		// structure of message: <player_name> : <message>
		int name_length;
		char *p_message;
		name_length = (int)strlen(sd->status.name);
		if (name_length > 24)
			name_length = 24;
		p_message = message + name_length;
		if (message_size < name_length + 3 + 1 || // check void message (at least 1 char) -> normal client refuse to send void message (but some skills can)
		    memcmp(message, sd->status.name, name_length) != 0 || // check player name
		    *p_message != ' ' || *(p_message + 1) != ':' || *(p_message + 2) != ' ') { // check ' : '
			// add here a message if necessary
			return;
		}
	}

	if (is_atcommand(fd, sd, message) != AtCommand_None)
		return;

	// �o�[�T�[�N�A�`���b�g�֎~��ԂȂ��b�s��
	if (sd->sc.data[SC_BERSERK].timer != -1 || sd->sc.data[SC_NOCHAT].timer != -1)
		return;

	WFIFOW(fd,0) = 0x8d;
	WFIFOW(fd,2) = message_size + 8;
	WFIFOL(fd,4) = sd->bl.id;
	memcpy(WFIFOP(fd,8), message, message_size);
	clif_send(WFIFOP(fd,0), WFIFOW(fd,2), &sd->bl, sd->chatID ? CHAT_WOS : AREA_CHAT_WOC);

	memcpy(WFIFOP(fd,0), RFIFOP(fd,0), len);
	WFIFOW(fd,0) = 0x8e;
	WFIFOSET(fd,WFIFOW(fd,2));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_MapMove(int fd,struct map_session_data *sd, int cmd)
{
	char mapname[17];
	int x,y;

	nullpo_retv(sd);

	if (battle_config.atc_gmonly == 0 ||
	    pc_isGM(sd) >= get_atcommand_level(AtCommand_MapMove)) {
		mapname[16] = 0;
		memcpy(mapname, RFIFOP(fd,GETPACKETPOS(cmd,0)), 16);
		if (mapname[0] != '\0') {
			x = RFIFOW(fd, GETPACKETPOS(cmd,1));
			y = RFIFOW(fd, GETPACKETPOS(cmd,2));
			pc_setpos(sd, mapname, x, y, 2);
		}
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_ChangeDir(int fd,struct map_session_data *sd, int cmd)
{
	int headdir,dir;

	nullpo_retv(sd);

	headdir = RFIFOW(fd,GETPACKETPOS(cmd,0)) & 0x07;
	dir     = RFIFOB(fd,GETPACKETPOS(cmd,1)) & 0x07;
	pc_setdir(sd,dir,headdir);

	clif_changedir(&sd->bl, headdir, dir);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_Emotion(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 2) {
		unsigned int tick = gettick();

		// anti hacker
		if(DIFF_TICK(tick, sd->anti_hacker.emotion_delay_tick) > 0) {
			sd->anti_hacker.emotion_delay_tick = tick + 1000;

			WFIFOW(fd,0)=0xc0;
			WFIFOL(fd,2)=sd->bl.id;
			WFIFOB(fd,6)=RFIFOB(fd,GETPACKETPOS(cmd,0));
			clif_send(WFIFOP(fd,0),packet_db[0xc0].len,&sd->bl,AREA);
		}
	} else {
		clif_skill_fail(sd,1,0,1);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_HowManyConnections(int fd,struct map_session_data *sd, int cmd)
{
	WFIFOW(fd,0)=0xc2;
	WFIFOL(fd,2)=map_getusers();
	WFIFOSET(fd,packet_db[0xc2].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_ActionRequest(int fd,struct map_session_data *sd, int cmd)
{
	unsigned int tick;
	int action_type,target_id;

	nullpo_retv(sd);

	if(unit_isdead(&sd->bl)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	if(sd->npc_id != 0 || (sd->sc.opt1 > OPT1_NORMAL && sd->sc.opt1 != OPT1_BURNNING) || sd->sc.option&OPTION_HIDE || sd->state.storage_flag || sd->state.deal_mode != 0 || sd->chatID || sd->state.mail_appending)
		return;
	if(sd->sc.data[SC_AUTOCOUNTER].timer != -1 ||	// �I�[�g�J�E���^�[
	   sd->sc.data[SC_DEATHBOUND].timer != -1 ||	// �f�X�o�E���h
	   sd->sc.data[SC_BLADESTOP].timer != -1 ||	// ���n���
	   sd->sc.data[SC_RUN].timer != -1 ||		// �^�C���M
	   sd->sc.data[SC_FORCEWALKING].timer != -1 ||	// �����ړ���
	   (sd->sc.data[SC_DANCING].timer != -1 && sd->sc.data[SC_LONGINGFREEDOM].timer == -1))	// �_���X��
		return;

	tick=gettick();

	unit_stop_walking(&sd->bl,0);
	unit_stopattack(&sd->bl);

	target_id   = RFIFOL(fd,GETPACKETPOS(cmd,0));
	action_type = RFIFOB(fd,GETPACKETPOS(cmd,1));

	// decode for jRO 2005-05-09dRagexe
	if( packet_db[cmd].pos[0]==0 )
	{
		int t1[]={ 88, 37 }, t2[]={ 80, 4 };
		int pos = ( ( g_packet_len - t1[g_packet_len&1] ) >> 1 ) + t2[g_packet_len&1];
		target_id = RFIFOL(fd,pos);
	}
	// end decode

	switch(action_type){
	case 0x00:	// once attack
	case 0x07:	// continuous attack
		if(sd->vender_id != 0) return;

		if(!battle_config.sdelay_attack_enable && pc_checkskill(sd,SA_FREECAST) <= 0 ) {
			if(DIFF_TICK(tick , sd->ud.canact_tick) < 0) {
				clif_skill_fail(sd,1,4,0);
				return;
			}
		}
		if(sd->sc.data[SC_WEDDING].timer != -1 ||
		   sd->sc.data[SC_BASILICA].timer != -1 ||
		   sd->sc.data[SC_GOSPEL].timer != -1 ||
		   sd->sc.data[SC_SANTA].timer != -1 ||
		   sd->sc.data[SC_SUMMER].timer != -1 ||
		   (sd->sc.data[SC_GRAVITATION_USER].timer != -1 && battle_config.player_gravitation_type < 2))
			return;
		if(sd->invincible_timer != -1)
			pc_delinvincibletimer(sd);
		unit_attack(&sd->bl,target_id,action_type!=0);
		break;
	case 0x02:	// sitdown
		if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 3) {
			pc_setsit(sd);
			clif_sitting(&sd->bl, 1);
			skill_sit(sd,1);	// �M�����O�X�^�[�p���_�C�X����уe�R���x���ݒ�
		} else {
			clif_skill_fail(sd,1,0,2);
		}
		break;
	case 0x03:	// standup
		pc_setstand(sd);
		clif_sitting(&sd->bl, 0);
		skill_sit(sd,0);	// �M�����O�X�^�[�p���_�C�X����уe�R���x������
		break;
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_Restart(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	switch(RFIFOB(fd,GETPACKETPOS(cmd,0))){ // restarttype
	case 0x00: // 0: restart game when character died
		if(unit_isdead(&sd->bl)) {
			pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,2);
		}
		break;
	case 0x01: // 1: request character select
		if(unit_isdead(&sd->bl))
			pc_setrestartvalue(sd,3);
		if(pc_isquitable(sd)){
			WFIFOW(fd,0)=0x18b;
			WFIFOW(fd,2)=1;
			WFIFOSET(fd,packet_db[0x18b].len);
			return;
		}
		if(sd->pd) {
			pet_lootitem_drop(sd->pd,sd);
			unit_free(&sd->pd->bl, 0);
		}
		if(sd->hd) {
			unit_free(&sd->hd->bl, 0);
		}
		if(sd->mcd) {
			unit_free(&sd->mcd->bl, 0);
		}
		unit_free(&sd->bl, 2);
		chrif_save(sd, 1);
		chrif_charselectreq(sd);
		sd->state.waitingdisconnect = 1;
		break;
	}

	return;
}

/*==========================================
 * Wis�̑��M
 *------------------------------------------
 */
static void clif_parse_Wis(int fd,struct map_session_data *sd, int cmd)
{
	char *message, *name;
	int message_size;

	nullpo_retv(sd);

	message_size = RFIFOW(fd,GETPACKETPOS(cmd,0)) - GETPACKETPOS(cmd,2); // including NULL
	message = (char *)RFIFOP(fd,GETPACKETPOS(cmd,2));

	if (message_size <= 1)	// NULL (1) (no void message)
		return;
	if (message_size > 255)	// too long
		return;

	message[message_size - 1] = 0; // be sure to have a NULL (hacker can send a no-NULL terminated string)

	if(is_atcommand_sub(fd, sd, message, 0) != AtCommand_None)
		return;

	name = (char *)RFIFOP(fd,GETPACKETPOS(cmd,1));
	name[23] = '\0';	// force \0 terminal

	// a normal client can not send a wisp message to yourself (anti-hack)
	if (strncmp(name, sd->status.name, 24) == 0)
		return;

	// �o�[�T�[�N�A�`���b�g�֎~��ԂȂ��b�s��
	if (sd->sc.data[SC_BERSERK].timer != -1 || sd->sc.data[SC_NOCHAT].timer != -1)
		return;

	intif_wis_message(sd, name, message, message_size);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_GMmessage(int fd,struct map_session_data *sd, int cmd)
{
	char *message;
	int message_size;

	message_size = RFIFOW(fd,GETPACKETPOS(cmd,0)) - GETPACKETPOS(cmd,1); // including NULL
	message = (char *)RFIFOP(fd,GETPACKETPOS(cmd,1));

	if (message_size <= 1)	// NULL (1) (no void message)
		return;
	if (message_size > 255)	// too long
		return;

	message[message_size - 1] = 0; // be sure to have a NULL (hacker can send a no-NULL terminated string)

	if (battle_config.atc_gmonly == 0 ||
	    pc_isGM(sd) >= get_atcommand_level(AtCommand_Broadcast))
		intif_GMmessage(message, message_size, 0);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_TakeItem(int fd,struct map_session_data *sd, int cmd)
{
	struct block_list *bl;
	struct flooritem_data *fitem = NULL;

	nullpo_retv(sd);

	if(unit_isdead(&sd->bl)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}

	if( sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || (sd->sc.opt1 > OPT1_NORMAL && sd->sc.opt1 != OPT1_BURNNING) || sd->chatID || sd->state.mail_appending ||
	    sd->state.storage_flag || pc_iscloaking(sd) ||
	    sd->sc.data[SC_AUTOCOUNTER].timer != -1 ||		// �I�[�g�J�E���^�[
	    sd->sc.data[SC_DEATHBOUND].timer != -1 ||	// �f�X�o�E���h
	    sd->sc.data[SC_RUN].timer != -1 ||			// �^�C���M
	    sd->sc.data[SC_FORCEWALKING].timer != -1 ||		// �����ړ����E���Ȃ�
	    sd->sc.data[SC_BLADESTOP].timer != -1 ||		// ���n���
	    sd->sc.data[SC__MANHOLE].timer != -1 )		// �}���z�[��
	{
		clif_additem(sd,0,0,6);
		return;
	}

	bl = map_id2bl(RFIFOL(fd,GETPACKETPOS(cmd,0)));	// map_object_id
	if(bl && bl->type == BL_ITEM)
		fitem = (struct flooritem_data *)bl;

	if(fitem == NULL || fitem->bl.m != sd->bl.m)
		return;

	pc_takeitem(sd,fitem);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_DropItem(int fd,struct map_session_data *sd, int cmd)
{
	int item_index = 0, item_amount = 0;
	unsigned int tick = gettick();

	nullpo_retv(sd);

	item_index  = RFIFOW(fd,GETPACKETPOS(cmd,0)) - 2;
	item_amount = RFIFOW(fd,GETPACKETPOS(cmd,1));

	if(unit_isdead(&sd->bl)) {
		clif_clearchar_area(&sd->bl,1);
		clif_delitem(sd, 0, item_index, 0);	// �N���C�A���g�Ɏ��s�p�P�b�g�𑗐M����K�v������
		return;
	}

	if(++sd->anti_hacker.drop_delay_count == 6) {
		char output[128];
		// %s (ID:%d) �̃A�C�e���h���b�v�Ԋu���ُ�ł��I
		snprintf(output, sizeof(output), msg_txt(47), sd->status.name, sd->status.char_id);
		printf("%s\n", output);
	}

	if( sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || (sd->sc.opt1 > OPT1_NORMAL && sd->sc.opt1 != OPT1_BURNNING) || sd->state.mail_appending || sd->state.storage_flag || sd->chatID ||
	    DIFF_TICK(tick, sd->anti_hacker.drop_delay_tick) < 0 ||
	    sd->sc.data[SC_AUTOCOUNTER].timer != -1 ||		// �I�[�g�J�E���^�[
	    sd->sc.data[SC_DEATHBOUND].timer != -1 ||	// �f�X�o�E���h
	    sd->sc.data[SC_BLADESTOP].timer != -1 ||		// ���n���
	    sd->sc.data[SC_FORCEWALKING].timer != -1 ||		// �����ړ���
	    sd->sc.data[SC_BERSERK].timer != -1 ||		// �o�[�T�[�N
	    sd->sc.data[SC__MANHOLE].timer != -1)		// �}���z�[��
	{
		clif_delitem(sd, 0, item_index, 0);
		return;
	}
	sd->anti_hacker.drop_delay_tick  = tick + 300;
	sd->anti_hacker.drop_delay_count = 0;

	if( pc_dropitem(sd, item_index, item_amount) ) {
		if (battle_config.save_player_when_drop_item) {
			// [Anti-hack] Protection against duplication of items
			// how -> A player drops an item on floor. An other takes the item and disconnects (and the item is saved).
			//        If players know a solution to crash server, they crash it. Then, first player can be not saved and they have duplicated.
			chrif_save(sd, 0);
			if(sd->state.storage_flag == 1)
				storage_storage_save(sd);
		}
	} else {
		clif_delitem(sd, 0, item_index, 0);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_UseItem(int fd,struct map_session_data *sd, int cmd)
{
	int idx;

	nullpo_retv(sd);

	idx = RFIFOW(fd,GETPACKETPOS(cmd,0)) - 2;
	if(idx < 0 || idx >= MAX_INVENTORY)
		return;

	if(unit_isdead(&sd->bl)) {
		clif_clearchar_area(&sd->bl,1);
		clif_useitemack(sd, idx, sd->status.inventory[idx].amount, 0);
		return;
	}
	if( (sd->npc_id != 0 && sd->npc_allowuseitem != 0 && sd->npc_allowuseitem != sd->status.inventory[idx].nameid) ||
	    sd->special_state.item_no_use != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 ||
		(sd->sc.opt1 > OPT1_NORMAL && sd->sc.opt1 != OPT1_STONECURSE_ING && sd->sc.opt1 != OPT1_BURNNING) ||
	    sd->state.storage_flag || sd->chatID || sd->state.mail_appending ||
	    DIFF_TICK(gettick(), sd->item_delay_tick) < 0 || 	// �A�C�e���f�B���C
	    sd->sc.data[SC_TRICKDEAD].timer != -1 ||	// ���񂾂ӂ�
	    sd->sc.data[SC_FORCEWALKING].timer != -1 ||	// �����ړ���
	    sd->sc.data[SC_BERSERK].timer != -1 ||	// �o�[�T�[�N
	    sd->sc.data[SC_FULLBUSTER].timer != -1 ||	// �t���o�X�^�[
	    sd->sc.data[SC_WEDDING].timer != -1 ||	// �����ߑ�
	    sd->sc.data[SC_NOCHAT].timer != -1 ||	// ��b�֎~
	    sd->sc.data[SC_GRAVITATION_USER].timer != -1  ||	// �O���r�e�[�V�����t�B�[���h�g�p��
	    sd->sc.data[SC__SHADOWFORM].timer != -1 ||	// �V���h�E�t�H�[��
	    sd->sc.data[SC__INVISIBILITY].timer != -1 ||	// �C���r�W�r���e�B
	    sd->sc.data[SC__MANHOLE].timer != -1)	// �}���z�[��
	{
		clif_useitemack(sd, idx, sd->status.inventory[idx].amount, 0);
		return;
	}

	pc_useitem(sd, idx);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_EquipItem(int fd,struct map_session_data *sd, int cmd)
{
	int idx;

	nullpo_retv(sd);

	if(unit_isdead(&sd->bl)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}

	idx = RFIFOW(fd,GETPACKETPOS(cmd,0)) - 2;
	if (idx < 0 || idx >= MAX_INVENTORY)
		return;

	if ((sd->npc_id != 0 && sd->npc_allowuseitem != 0 && sd->npc_allowuseitem != sd->status.inventory[idx].nameid) ||
	     sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->state.mail_appending)
		return;
	if (sd->sc.data[SC_BLADESTOP].timer != -1 || sd->sc.data[SC_BERSERK].timer != -1)
		return;

	// ���Ӓ�������͔j�󂳂�Ă���
	if (sd->status.inventory[idx].identify != 1 || sd->status.inventory[idx].attribute != 0) {
		clif_equipitemack(sd, idx, 0, 0);	// fail
		return;
	}

	if (sd->inventory_data[idx]) {
		switch(sd->inventory_data[idx]->type) {
			case ITEMTYPE_ARROW:
			case ITEMTYPE_AMMO:
			case ITEMTYPE_THROWWEAPON:
			case ITEMTYPE_CANNONBALL:
				// ��E�e�ہE�ꖳ�E�藠���E�L���m���{�[��
				pc_equipitem(sd, idx, LOC_ARROW);
				break;
			default:
				if(sd->inventory_data[idx]->flag.pet_acce)	// �y�b�g�p�����i
					pet_equipitem(sd, idx);
				else
					pc_equipitem(sd, idx, RFIFOW(fd,GETPACKETPOS(cmd,1)));
				break;
		}
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_UnequipItem(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	if (unit_isdead(&sd->bl)) {
		clif_clearchar_area(&sd->bl, 1);
		return;
	}

	if (sd->sc.data[SC_BLADESTOP].timer != -1 || sd->sc.data[SC_BERSERK].timer != -1)
		return;
	if (sd->npc_id != 0 || sd->vender_id != 0 || (sd->sc.opt1 > OPT1_NORMAL && sd->sc.opt1 != OPT1_BURNNING) || sd->state.mail_appending)
		return;

	pc_unequipitem(sd, RFIFOW(fd,GETPACKETPOS(cmd,0)) - 2, 0);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_NpcClicked(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	// ����ł�����A�ԃG���̎���NPC���N���b�N�ł��Ȃ�
	if(unit_isdead(&sd->bl)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	if(sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->ud.skilltimer != -1 || sd->status.manner < 0 || sd->state.mail_appending)
		return;
	npc_click(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_NpcBuySellSelected(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	// ����ł�����A�ԃG���̎���NPC���N���b�N�ł��Ȃ�
	if(unit_isdead(&sd->bl)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	if(sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->status.manner < 0 || sd->state.mail_appending)
		return;
	npc_buysellsel(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)),RFIFOB(fd,GETPACKETPOS(cmd,1)));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_NpcBuyListSend(int fd,struct map_session_data *sd, int cmd)
{
	int fail, n;

	nullpo_retv(sd);

	// ����ł�����A�ԃG���̎���NPC���N���b�N�ł��Ȃ�
	if(unit_isdead(&sd->bl)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	if(sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->status.manner < 0 || sd->state.mail_appending)
		return;

	n = (RFIFOW(fd,GETPACKETPOS(cmd,0)) - 4) /4;
	if (n <= 0) // max is checked in npc_buylist function
		return;

	fail = npc_buylist(sd, n, (unsigned short*)RFIFOP(fd,GETPACKETPOS(cmd,1))); // item_list

	WFIFOW(fd,0)=0xca;
	WFIFOB(fd,2)=fail; // 0: The deal has successfully completed., 1: You dont have enough zeny., 2: you are overcharged!, 3: You are over your weight limit.
	WFIFOSET(fd,packet_db[0xca].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_NpcSellListSend(int fd,struct map_session_data *sd, int cmd)
{
	int fail, n;

	nullpo_retv(sd);

	// ����ł�����A�ԃG���̎���NPC���N���b�N�ł��Ȃ�
	if(unit_isdead(&sd->bl)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	if(sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->status.manner < 0 || sd->state.mail_appending)
		return;

	n = (RFIFOW(fd,GETPACKETPOS(cmd,0))-4) /4;
	if (n <= 0 || n > MAX_INVENTORY) // max is checked in npc_selllist function, but normal clients never send more items than total in inventory
		return;

	fail = npc_selllist(sd, n, (unsigned short*)RFIFOP(fd,GETPACKETPOS(cmd,1))); // item_list

	WFIFOW(fd,0)=0xcb;
	WFIFOB(fd,2)=fail;
	WFIFOSET(fd,packet_db[0xcb].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_NpcPointShopBuy(int fd,struct map_session_data *sd, int cmd)
{
	int fail;

	nullpo_retv(sd);

	// ����ł�����A�ԃG���̎���NPC���N���b�N�ł��Ȃ�
	if(unit_isdead(&sd->bl)) {
		clif_clearchar_area(&sd->bl,1);
		return;
	}
	if(sd->npc_id != 0 || sd->vender_id != 0 || sd->status.manner < 0 || sd->state.mail_appending)
		return;

	fail = npc_pointshop_buy(sd, RFIFOW(fd,GETPACKETPOS(cmd,0)), RFIFOW(fd,GETPACKETPOS(cmd,1)));

	WFIFOW(fd,0)  = 0x289;
	WFIFOL(fd,2)  = sd->shop_point;
#if PACKETVER < 10
	WFIFOW(fd,6)  = fail;
#else
	WFIFOL(fd,6)  = 0;	// ??
	WFIFOW(fd,10) = fail;
#endif
	WFIFOSET(fd,packet_db[0x289].len);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_CreateChatRoom(int fd,struct map_session_data *sd, int cmd)
{
	char *chat_title;
	int len;

	nullpo_retv(sd);

	if(sd->status.manner < 0 || (battle_config.basic_skill_check && pc_checkskill(sd,NV_BASIC) < 4)) {
		clif_skill_fail(sd,1,0,3);
		return;
	}

	len = (int)RFIFOW(fd,GETPACKETPOS(cmd,0));
	if (len <= GETPACKETPOS(cmd,4))
		return;
	chat_title = (char *)RFIFOP(fd,GETPACKETPOS(cmd,4)); // don't include \0
	if (chat_title[0] == '\0')
		return;
	chat_createchat(sd, RFIFOW(fd,GETPACKETPOS(cmd,1)), RFIFOB(fd,GETPACKETPOS(cmd,2)), (char *)RFIFOP(fd,GETPACKETPOS(cmd,3)), chat_title, len - GETPACKETPOS(cmd,4));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_ChatAddMember(int fd,struct map_session_data *sd, int cmd)
{
	chat_joinchat(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)),RFIFOP(fd,GETPACKETPOS(cmd,1)));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_ChatRoomStatusChange(int fd,struct map_session_data *sd, int cmd)
{
	char *chat_title;
	int len;

	len = (int)RFIFOW(fd,GETPACKETPOS(cmd,0));
	if (len <= GETPACKETPOS(cmd,4))
		return;
	chat_title = (char *)RFIFOP(fd,GETPACKETPOS(cmd,4)); // don't include \0
	if (chat_title[0] == '\0')
		return;
	chat_changechatstatus(sd, RFIFOW(fd,GETPACKETPOS(cmd,1)), RFIFOB(fd,GETPACKETPOS(cmd,2)), (char *)RFIFOP(fd,GETPACKETPOS(cmd,3)), chat_title, len - GETPACKETPOS(cmd,4));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_ChangeChatOwner(int fd,struct map_session_data *sd, int cmd)
{
	chat_changechatowner(sd, (char *)RFIFOP(fd,GETPACKETPOS(cmd,1)));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_KickFromChat(int fd,struct map_session_data *sd, int cmd)
{
	chat_kickchat(sd, (char *)RFIFOP(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_ChatLeave(int fd,struct map_session_data *sd, int cmd)
{
	chat_leavechat(sd,0);

	return;
}

/*==========================================
 * ����v���𑊎�ɑ���
 *------------------------------------------
 */
static void clif_parse_TradeRequest(int fd,struct map_session_data *sd, int cmd)
{
	int gmlvl = 0;

	nullpo_retv(sd);

	if(map[sd->bl.m].flag.notrade)
		return;
	if(sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->state.mail_appending)
		return;

	gmlvl = pc_isGM(sd);
	if(gmlvl > 0 && battle_config.gm_can_drop_lv > gmlvl)
		return;

	if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 1)
		trade_traderequest(sd,RFIFOL(sd->fd,GETPACKETPOS(cmd,0)));
	else
		clif_skill_fail(sd,1,0,0);

	return;
}

/*==========================================
 * ����v��
 *------------------------------------------
 */
static void clif_parse_TradeAck(int fd,struct map_session_data *sd, int cmd)
{
	int gmlvl = 0;

	nullpo_retv(sd);

	if(map[sd->bl.m].flag.notrade)
		return;

	gmlvl = pc_isGM(sd);
	if(gmlvl > 0 && battle_config.gm_can_drop_lv > gmlvl)
		return;

	trade_tradeack(sd,RFIFOB(sd->fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 * �A�C�e���ǉ�
 *------------------------------------------
 */
static void clif_parse_TradeAddItem(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	trade_tradeadditem(sd,RFIFOW(sd->fd,GETPACKETPOS(cmd,0)),RFIFOL(sd->fd,GETPACKETPOS(cmd,1)));

	return;
}

/*==========================================
 * �A�C�e���ǉ�����(ok����)
 *------------------------------------------
 */
static void clif_parse_TradeOk(int fd,struct map_session_data *sd, int cmd)
{
	trade_tradeok(sd);

	return;
}

/*==========================================
 * ����L�����Z��
 *------------------------------------------
 */
static void clif_parse_TradeCancel(int fd,struct map_session_data *sd, int cmd)
{
	trade_tradecancel(sd);

	return;
}

/*==========================================
 * �������(trade����)
 *------------------------------------------
 */
static void clif_parse_TradeCommit(int fd,struct map_session_data *sd, int cmd)
{
	trade_tradecommit(sd);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_StopAttack(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	unit_stopattack(&sd->bl);

	return;
}

/*==========================================
 * �J�[�g�փA�C�e�����ڂ�
 *------------------------------------------
 */
static void clif_parse_PutItemToCart(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	if (sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->state.mail_appending || pc_iscarton(sd) == 0)
		return;
	pc_putitemtocart(sd,RFIFOW(fd,GETPACKETPOS(cmd,0))-2,RFIFOL(fd,GETPACKETPOS(cmd,1)));

	return;
}

/*==========================================
 * �J�[�g����A�C�e�����o��
 *------------------------------------------
 */
static void clif_parse_GetItemFromCart(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	if (sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->state.mail_appending || pc_iscarton(sd) == 0)
		return;
	pc_getitemfromcart(sd,RFIFOW(fd,GETPACKETPOS(cmd,0))-2,RFIFOL(fd,GETPACKETPOS(cmd,1)));

	return;
}

/*==========================================
 * �t���i(��,�y�R,�J�[�g)���͂���
 *------------------------------------------
 */
static void clif_parse_RemoveOption(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	if (sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->state.mail_appending)
		return;
	pc_setoption(sd,0);

	return;
}

/*==========================================
 * �`�F���W�J�[�g
 *------------------------------------------
 */
static void clif_parse_ChangeCart(int fd,struct map_session_data *sd, int cmd)
{
	pc_setcart(sd, RFIFOW(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 * �X�e�[�^�X�A�b�v
 *------------------------------------------
 */
static void clif_parse_StatusUp(int fd,struct map_session_data *sd, int cmd)
{
	// Is amount RFIFOB(fd,GETPACKETPOS(cmd,1)) always 1?
	pc_statusup(sd, RFIFOW(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 * �X�L�����x���A�b�v
 *------------------------------------------
 */
static void clif_parse_SkillUp(int fd,struct map_session_data *sd, int cmd)
{
	int skill_num = RFIFOW(fd,GETPACKETPOS(cmd,0));

	if( skill_num >= GUILD_SKILLID )	// �M���h�X�L��
		guild_skillup(sd,skill_num,0,0);
	else if( skill_num >= HOM_SKILLID )	// �z�����N���X�X�L��
		homun_skillup(sd,skill_num);
	else
		pc_skillup(sd,skill_num);

	return;
}

/*==========================================
 * �X�L���g�p�iID�w��j
 *------------------------------------------
 */
static void clif_parse_UseSkillToId(int fd, struct map_session_data *sd, int cmd)
{
	int skillnum, skilllv, lv, target_id, skilldb_id, inf, change_inf = 0;
	unsigned int tick = gettick();
	struct block_list *bl;
	struct status_change *sc;

	nullpo_retv(sd);

	if(map[sd->bl.m].flag.noskill)
		return;
	if(sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->chatID || sd->state.storage_flag || sd->state.mail_appending)
		return;
	if(pc_issit(sd))
		return;

	skilllv   = RFIFOW(fd,GETPACKETPOS(cmd,0));
	skillnum  = RFIFOW(fd,GETPACKETPOS(cmd,1));
	target_id = RFIFOL(fd,GETPACKETPOS(cmd,2));

	// decode for jRO 2005-05-09dRagexe
	if(packet_db[cmd].pos[0] == 0 && packet_db[cmd].pos[1] == 0 && packet_db[cmd].pos[2] == 0)
	{
		int tmp = g_packet_len % 3;
		const int t1[] = { 138,43,170 }, t2[] = { 103,4,80 }, t3[] = { 130,33,84 }, t4[] = { 134, 39, 166 };
		int tmp2 = (g_packet_len - t1[tmp]) / 3;
		skilllv   = RFIFOW(fd, tmp2   + t2[tmp] );
		skillnum  = RFIFOW(fd, tmp2*2 + t3[tmp] );
		target_id = RFIFOL(fd, tmp2*3 + t4[tmp] );
	}
	// end decode

	bl = map_id2bl(target_id);
	if(bl == NULL)
		return;

	inf = skill_get_inf(skillnum);
	if(inf == INF_PASSIVE || inf & INF_TOGROUND) {
		// anti hacker
		return;
	}

	if(skillnum >= HOM_SKILLID && skillnum < MAX_HOM_SKILLID) {
		// �z���X�L��
		struct homun_data *hd = sd->hd;
		if( hd && (lv = homun_checkskill(hd,skillnum)) ) {
			if(skilllv > lv)
				skilllv = lv;
			if(hd->ud.skilltimer != -1)
				return;
			if(DIFF_TICK(tick, hd->ud.canact_tick) < 0)
				return;
			if(DIFF_TICK(tick, hd->skillstatictimer[skillnum-HOM_SKILLID]) < 0)
				return;

			if(inf & INF_TOME)	// �������Ώ�
				unit_skilluse_id(&hd->bl,hd->bl.id,skillnum,skilllv);
			else
				unit_skilluse_id(&hd->bl,target_id,skillnum,skilllv);
		}
		return;
	}
	if(skillnum >= MERC_SKILLID && skillnum < MAX_MERC_SKILLID) {
		// �b���X�L��
		struct merc_data *mcd = sd->mcd;
		if( mcd && (lv = merc_checkskill(mcd,skillnum)) ) {
			if(skilllv > lv)
				skilllv = lv;
			if(mcd->ud.skilltimer != -1)
				return;
			if(DIFF_TICK(tick, mcd->ud.canact_tick) < 0)
				return;
			if(DIFF_TICK(tick, mcd->skillstatictimer[skillnum-MERC_SKILLID]) < 0)
				return;

			if(inf & INF_TOME)	// �������Ώ�
				unit_skilluse_id(&mcd->bl,mcd->bl.id,skillnum,skilllv);
			else
				unit_skilluse_id(&mcd->bl,target_id,skillnum,skilllv);
		}
		return;
	}
	if(skillnum >= GUILD_SKILLID) {
		struct guild *g = guild_search(sd->status.guild_id);

		// �M���h�X�L���̓M���}�X�̂�
		if(g == NULL || sd != guild_get_guildmaster_sd(g))
			return;
		// skilllv�͏��0�Ȃ̂Ō��݂̃X�L��Lv�ŕ␳����
		skilllv = guild_checkskill(g, skillnum);
	}

	// inf�́u���������v���u�G�v�ɕς��ꍇ
	if(skillnum == MO_EXTREMITYFIST) {
		if(sd->sc.data[SC_COMBO].timer == -1 || (sd->sc.data[SC_COMBO].val1 != MO_COMBOFINISH && sd->sc.data[SC_COMBO].val1 != CH_CHAINCRUSH))
			change_inf = INF_TOCHARACTER;
	} else if(skillnum == TK_JUMPKICK) {
		if(sd->sc.data[SC_DODGE_DELAY].timer == -1)
			change_inf = INF_TOCHARACTER;
	}

	if(inf & INF_TOME && !change_inf) {
		// ���������Ȃ�^�[�Q�b�g���������g�ɂ���
		target_id = sd->bl.id;
	}

	if(sd->ud.skilltimer != -1) {
		if(skillnum != SA_CASTCANCEL)
			return;
	} else if(DIFF_TICK(tick, sd->ud.canact_tick) < 0) {
		clif_skill_fail(sd,skillnum,4,0);
		return;
	}

	// static timer�̔���
	skilldb_id = skill_get_skilldb_id(skillnum);
	if(skilldb_id <= 0)
		return;
	if(DIFF_TICK(tick, sd->skillstatictimer[skilldb_id]) < 0) {
		clif_skill_fail(sd,skillnum,4,0);
		return;
	}
	if(skillnum >= THIRD_SKILLID && skillnum < MAX_THIRD_SKILLID) {
		// 3���E�X�L��
		if(DIFF_TICK(tick, sd->skillcooldown[skillnum - THIRD_SKILLID]) < 0)
			return;
	}

	if( (sd->sc.data[SC_TRICKDEAD].timer != -1 && skillnum != NV_TRICKDEAD) ||
	    sd->sc.data[SC_NOCHAT].timer != -1 ||
	    sd->sc.data[SC_WEDDING].timer != -1 ||
	    sd->sc.data[SC_FORCEWALKING].timer != -1 ||
	    sd->sc.data[SC_SANTA].timer != -1 ||
	    sd->sc.data[SC_SUMMER].timer != -1 )
		return;
	if(sd->sc.data[SC_BASILICA].timer != -1 && (skillnum != HP_BASILICA || sd->sc.data[SC_BASILICA].val2 != sd->bl.id))
		return;
	if(sd->sc.data[SC_GOSPEL].timer != -1 && (skillnum != PA_GOSPEL || sd->sc.data[SC_GOSPEL].val2 != sd->bl.id))
		return;

	sc = status_get_sc(bl);
	if(sc && sc->option & (OPTION_HIDE | OPTION_CLOAKING | OPTION_FOOTPRINT) && ((&sd->bl) != bl))
	{
		// �����ƈ����͍U���\�H
		if(sd->race != RCT_INSECT && sd->race != RCT_DEMON)
			return;
	}

	if(sd->invincible_timer != -1)
		pc_delinvincibletimer(sd);

	if(sd->skill_item.id >= 0 && sd->skill_item.id == skillnum) {
		if(skilllv != sd->skill_item.lv)
			skilllv = sd->skill_item.lv;
		if( !unit_skilluse_id(&sd->bl,target_id,skillnum,skilllv) ) {
			sd->skill_item.id      = -1;
			sd->skill_item.lv      = -1;
			sd->skill_item.flag    = 0;
		}
	} else {
		sd->skill_item.id      = -1;
		sd->skill_item.lv      = -1;
		sd->skill_item.flag    = 0;

		if(change_inf) {
			if(!sd->state.skill_flag) {
				sd->state.skill_flag = 1;
				clif_skillinfo(sd,skillnum,1,-1);
				return;
			}
			if(sd->bl.id == target_id) {
				clif_skillinfo(sd,skillnum,1,-1);
				return;
			}
		}
		if((lv = pc_checkskill(sd,skillnum)) > 0) {
			if(skilllv > lv)
				skilllv = lv;
			unit_skilluse_id(&sd->bl,target_id,skillnum,skilllv);
			if(sd->state.skill_flag)
				sd->state.skill_flag = 0;
		}
	}

	return;
}

/*==========================================
 * �X�L���g�p�i�ꏊ�w��j
 *------------------------------------------
 */
static void clif_parse_UseSkillToPos(int fd, struct map_session_data *sd, int cmd)
{
	int skillnum, skilllv, lv, x, y, skilldb_id;
	unsigned int tick = gettick();

	nullpo_retv(sd);

	if(map[sd->bl.m].flag.noskill)
		return;
	if(sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->chatID || sd->state.storage_flag || sd->state.mail_appending)
		return;

	skilllv  = RFIFOW(fd,GETPACKETPOS(cmd,0));
	skillnum = RFIFOW(fd,GETPACKETPOS(cmd,1));
	x        = RFIFOW(fd,GETPACKETPOS(cmd,2));
	y        = RFIFOW(fd,GETPACKETPOS(cmd,3));

	if(!(skill_get_inf(skillnum) & INF_TOGROUND)) {
		// anti hacker
		return;
	}

	if(skillnum >= HOM_SKILLID && skillnum < MAX_HOM_SKILLID) {
		// �z���X�L��
		struct homun_data *hd = sd->hd;
		if( hd && (lv = homun_checkskill(hd,skillnum)) ) {
			if(skilllv > lv)
				skilllv = lv;
			if(hd->ud.skilltimer != -1)
				return;
			if(DIFF_TICK(tick, hd->ud.canact_tick) < 0)
				return;
			if(DIFF_TICK(tick, hd->skillstatictimer[skillnum-HOM_SKILLID]) < 0)
				return;

			unit_skilluse_pos(&hd->bl,x,y,skillnum,skilllv);
		}
		return;
	}
	if(skillnum >= MERC_SKILLID && skillnum < MAX_MERC_SKILLID) {
		// �b���X�L��
		struct merc_data *mcd = sd->mcd;
		if( mcd && (lv = merc_checkskill(mcd,skillnum)) ) {
			if(skilllv > lv)
				skilllv = lv;
			if(mcd->ud.skilltimer != -1)
				return;
			if(DIFF_TICK(tick, mcd->ud.canact_tick) < 0)
				return;
			if(DIFF_TICK(tick, mcd->skillstatictimer[skillnum-MERC_SKILLID]) < 0)
				return;

			unit_skilluse_pos(&mcd->bl,x,y,skillnum,skilllv);
		}
		return;
	}

	if(sd->ud.skilltimer != -1)
		return;
	if(DIFF_TICK(tick, sd->ud.canact_tick) < 0) {
		clif_skill_fail(sd,skillnum,4,0);
		return;
	}

	// static timer�̔���
	skilldb_id = skill_get_skilldb_id(skillnum);
	if(skilldb_id <= 0)
		return;
	if(DIFF_TICK(tick, sd->skillstatictimer[skilldb_id]) < 0) {
		clif_skill_fail(sd,skillnum,4,0);
		return;
	}
	if(skillnum >= THIRD_SKILLID && skillnum < MAX_THIRD_SKILLID) {
		// 3���E�X�L��
		if(DIFF_TICK(tick, sd->skillcooldown[skillnum - THIRD_SKILLID]) < 0)
			return;
	}

	if(GETPACKETPOS(cmd,4)) {
		if(pc_issit(sd)) {
			clif_skill_fail(sd,skillnum,0,0);
			return;
		}
		memcpy(sd->message,RFIFOP(fd,GETPACKETPOS(cmd,4)),80);
		sd->message[79] = '\0'; // message includes NULL, but add it against hacker
	}
	else if(pc_issit(sd)) {
		return;
	}

	if( (sd->sc.data[SC_TRICKDEAD].timer != -1 && skillnum != NV_TRICKDEAD) ||
	    sd->sc.data[SC_NOCHAT].timer != -1 ||
	    sd->sc.data[SC_WEDDING].timer != -1 ||
	    sd->sc.data[SC_GOSPEL].timer != -1 ||
	    sd->sc.data[SC_FORCEWALKING].timer != -1 ||
	    sd->sc.data[SC_SANTA].timer != -1 ||
	    sd->sc.data[SC_SUMMER].timer != -1 )
		return;
	if(sd->sc.data[SC_BASILICA].timer != -1 && (skillnum != HP_BASILICA || sd->sc.data[SC_BASILICA].val2 != sd->bl.id))
		return;

	if(sd->invincible_timer != -1)
		pc_delinvincibletimer(sd);

	if(sd->skill_item.id >= 0 && sd->skill_item.id == skillnum) {
		if(skilllv != sd->skill_item.lv)
			skilllv = sd->skill_item.lv;
		if( !unit_skilluse_pos(&sd->bl,x,y,skillnum,skilllv) ) {
			sd->skill_item.id      = -1;
			sd->skill_item.lv      = -1;
			sd->skill_item.flag    = 0;
		}
	} else {
		sd->skill_item.id      = -1;
		sd->skill_item.lv      = -1;
		sd->skill_item.flag    = 0;
		if((lv = pc_checkskill(sd,skillnum)) > 0) {
			if(skilllv > lv)
				skilllv = lv;
			unit_skilluse_pos(&sd->bl,x,y,skillnum,skilllv);
		}
	}

	return;
}

/*==========================================
 * �X�L���g�p�imap�w��j
 *------------------------------------------
 */
static void clif_parse_UseSkillMap(int fd, struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	if(map[sd->bl.m].flag.noskill)
		return;
	if(sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->chatID || sd->state.storage_flag || sd->state.mail_appending)
		return;

	if(sd->sc.data[SC_TRICKDEAD].timer != -1 ||
	   sd->sc.data[SC_NOCHAT].timer != -1 ||
	   sd->sc.data[SC_WEDDING].timer != -1 ||
	   sd->sc.data[SC_BASILICA].timer != -1 ||
	   sd->sc.data[SC_GOSPEL].timer != -1 ||
	   sd->sc.data[SC_FORCEWALKING].timer != -1 ||
	   sd->sc.data[SC_SANTA].timer != -1 ||
	   sd->sc.data[SC_SUMMER].timer != -1)
		return;

	if(sd->invincible_timer != -1)
		pc_delinvincibletimer(sd);

	RFIFOB(fd,GETPACKETPOS(cmd,1)+15) = '\0'; // be sure to have NULL
	skill_castend_map(sd,RFIFOW(fd,GETPACKETPOS(cmd,0)),RFIFOP(fd,GETPACKETPOS(cmd,1)));

	return;
}

/*==========================================
 * �����v��
 *------------------------------------------
 */
static void clif_parse_RequestMemo(int fd,struct map_session_data *sd, int cmd)
{
	pc_memo(sd,-1);

	return;
}

/*==========================================
 * �A�C�e������
 *------------------------------------------
 */
static void clif_parse_ProduceMix(int fd,struct map_session_data *sd, int cmd)
{
	int nameid, slot1, slot2, slot3;

	nullpo_retv(sd);

	sd->state.produce_flag = 0;

	nameid = RFIFOW(fd,GETPACKETPOS(cmd,0));
	slot1  = RFIFOW(fd,GETPACKETPOS(cmd,1));
	slot2  = RFIFOW(fd,GETPACKETPOS(cmd,2));
	slot3  = RFIFOW(fd,GETPACKETPOS(cmd,3));

	skill_produce_mix(sd,nameid,slot1,slot2,slot3);

	return;
}

/*==========================================
 * ����C��
 *------------------------------------------
 */
static void clif_parse_RepairItem(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	sd->state.produce_flag = 0;
	skill_repair_weapon(sd,RFIFOW(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 * ���퐸�B
 *------------------------------------------
 */
static void clif_parse_WeaponRefine(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	skill_weapon_refine(sd, RFIFOW(fd,GETPACKETPOS(cmd,0)) - 2); // index

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_NpcSelectMenu(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	sd->npc_menu=RFIFOB(fd,GETPACKETPOS(cmd,1));
	npc_scriptcont(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_NpcNextClicked(int fd,struct map_session_data *sd, int cmd)
{
	npc_scriptcont(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_NpcAmountInput(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	sd->npc_amount = (int)RFIFOL(fd,GETPACKETPOS(cmd,1));
	npc_scriptcont(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_NpcStringInput(int fd,struct map_session_data *sd, int cmd)
{
	int len;

	nullpo_retv(sd);

	len = (int)RFIFOW(fd,GETPACKETPOS(cmd,0));

	// check validity of length (against hacker)
	if (len < (int)GETPACKETPOS(cmd,2) + 1)
		return;
	// normal client send NULL -> force it (against hacker)
	RFIFOB(fd,len - 1) = '\0';

	if (len - (int)GETPACKETPOS(cmd,2) >= sizeof(sd->npc_str)) { // message includes NULL
		//printf("clif: input string too long !\n"); // no display, it's not an error, just a check against overflow (and hacker)
		memcpy(sd->npc_str, RFIFOP(fd,GETPACKETPOS(cmd,2)), sizeof(sd->npc_str));
	} else {
		strncpy(sd->npc_str, RFIFOP(fd,GETPACKETPOS(cmd,2)), sizeof(sd->npc_str));
	}
	sd->npc_str[sizeof(sd->npc_str) - 1] = 0;
	npc_scriptcont(sd,RFIFOL(fd,GETPACKETPOS(cmd,1)));

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_NpcCloseClicked(int fd,struct map_session_data *sd, int cmd)
{
	npc_scriptcont(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 * �A�C�e���Ӓ�
 *------------------------------------------
 */
static void clif_parse_ItemIdentify(int fd,struct map_session_data *sd, int cmd)
{
	pc_item_identify(sd,RFIFOW(fd,GETPACKETPOS(cmd,0))-2);

	return;
}

/*==========================================
 * �A�C�e�����X�g�I����M
 *------------------------------------------
 */
static void clif_parse_SelectItem(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	switch(sd->skill_menu.id)
	{
		case AC_MAKINGARROW:		/* ��쐬 */
			skill_arrow_create(sd,RFIFOW(fd,GETPACKETPOS(cmd,0)));
			break;

		case GC_POISONINGWEAPON:	/* �|�C�Y�j���O�E�F�|�� */
			skill_poisoning_weapon(sd,RFIFOW(fd,GETPACKETPOS(cmd,0)));
			break;

		case WL_READING_SB:			/* ���[�f�B���O�X�y���u�b�N */
			skill_reading_sb(sd,RFIFOW(fd,GETPACKETPOS(cmd,0)));
			break;
	}

	sd->skill_menu.id = 0;
	sd->skill_menu.lv = 0;

	return;
}

/*==========================================
 * �I�[�g�X�y����M
 *------------------------------------------
 */
static void clif_parse_AutoSpell(int fd,struct map_session_data *sd, int cmd)
{
	skill_autospell(sd,RFIFOW(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 * �J�[�h�g�p
 *------------------------------------------
 */
static void clif_parse_UseCard(int fd,struct map_session_data *sd, int cmd)
{
	clif_use_card(sd,RFIFOW(fd,GETPACKETPOS(cmd,0))-2);

	return;
}

/*==========================================
 * �J�[�h�}�������I��
 *------------------------------------------
 */
static void clif_parse_InsertCard(int fd,struct map_session_data *sd, int cmd)
{
	int idx;

	nullpo_retv(sd);

	idx = RFIFOW(fd,GETPACKETPOS(cmd,0)) - 2;
	if (idx < 0 || idx >= MAX_INVENTORY)
		return;

	if (sd->npc_id != 0 && sd->npc_allowuseitem != 0 && sd->npc_allowuseitem != sd->status.inventory[idx].nameid)
		return;
	if (sd->vender_id != 0 || sd->state.deal_mode != 0 || (sd->sc.opt1 > OPT1_NORMAL && sd->sc.opt1 < OPT1_STONECURSE_ING) || sd->state.storage_flag || sd->chatID || sd->state.mail_appending)
		return;
	if (unit_isdead(&sd->bl))
		return;

	if (sd->sc.data[SC_TRICKDEAD].timer != -1 ||	// ���񂾂ӂ�
	    sd->sc.data[SC_BLADESTOP].timer != -1 ||	// ���n���
	    sd->sc.data[SC_FORCEWALKING].timer != -1 ||	// �����ړ�
	    sd->sc.data[SC_BERSERK].timer != -1 ||	// �o�[�T�[�N
	    sd->sc.data[SC_WEDDING].timer != -1 ||	// �����ߑ�
	    sd->sc.data[SC_NOCHAT].timer != -1)		// ��b�֎~
		return;

	pc_insert_card(sd, idx, RFIFOW(fd,GETPACKETPOS(cmd,1))-2);

	return;
}

/*==========================================
 * 0193 �L����ID���O����
 *------------------------------------------
 */
static void clif_parse_SolveCharName(int fd,struct map_session_data *sd, int cmd)
{
	clif_solved_charname(sd, RFIFOL(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 * 0197 /resetskill /resetstate
 *------------------------------------------
 */
static void clif_parse_ResetChar(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	if (battle_config.atc_gmonly == 0 ||
	    pc_isGM(sd) >= get_atcommand_level(AtCommand_ResetState)) {
		switch(RFIFOW(fd,GETPACKETPOS(cmd,0))){
		case 0:
			pc_resetstate(sd);
			break;
		case 1:
			pc_resetskill(sd, -1);
			break;
		}
	}

	return;
}

/*==========================================
 * 019c /lb (with GM name), /nlb, /mb (in blue)
 *------------------------------------------
 */
static void clif_parse_LGMmessage(int fd,struct map_session_data *sd, int cmd)
{
	int len;
	int message_size;

	nullpo_retv(sd);

	len = RFIFOW(fd,GETPACKETPOS(cmd,0));
	message_size = len - GETPACKETPOS(cmd,1);
	if (message_size <= 0)	// must have at least \0
		return;
	if (message_size > 255)	// too long
		return;

	// force \0 against hack
	RFIFOB(fd, len-1) = '\0';

	if (battle_config.atc_gmonly == 0 ||
	    pc_isGM(sd) >= get_atcommand_level(AtCommand_LocalBroadcast)) {
		WFIFOW(fd,0)=0x9a;
		WFIFOW(fd,2)=len;
		memcpy(WFIFOP(fd,4), RFIFOP(fd, GETPACKETPOS(cmd,1)), message_size);
		clif_send(WFIFOP(fd,0), len, &sd->bl, ALL_SAMEMAP);
	}

	return;
}

/*==========================================
 * �J�v���q�ɂ֓����
 *------------------------------------------
 */
static void clif_parse_MoveToKafra(int fd,struct map_session_data *sd, int cmd)
{
	int idx, amount;

	nullpo_retv(sd);

	if(sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->state.mail_appending || sd->sc.data[SC_BERSERK].timer != -1)
		return;

	idx    = RFIFOW(fd,GETPACKETPOS(cmd,0))-2;
	amount = RFIFOL(fd,GETPACKETPOS(cmd,1));

	if(sd->state.storage_flag==2)
		storage_guild_storageadd(sd, idx, amount);
	else if(sd->state.storage_flag==1)
		storage_storageadd(sd, idx, amount);

	return;
}

/*==========================================
 * �J�v���q�ɂ���o��
 *------------------------------------------
 */
static void clif_parse_MoveFromKafra(int fd,struct map_session_data *sd, int cmd)
{
	int idx, amount;

	nullpo_retv(sd);

	if(sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->state.mail_appending || sd->sc.data[SC_BERSERK].timer != -1)
		return;

	idx    = RFIFOW(fd,GETPACKETPOS(cmd,0))-1;
	amount = RFIFOL(fd,GETPACKETPOS(cmd,1));

	if(sd->state.storage_flag==2)
		storage_guild_storageget(sd, idx, amount);
	else if(sd->state.storage_flag==1)
		storage_storageget(sd, idx, amount);

	return;
}

/*==========================================
 * �J�v���q�ɂփJ�[�g��������
 *------------------------------------------
 */
static void clif_parse_MoveToKafraFromCart(int fd,struct map_session_data *sd, int cmd)
{
	int idx, amount;

	nullpo_retv(sd);

	if(sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->state.mail_appending ||
	   pc_iscarton(sd) == 0 || sd->sc.data[SC_BERSERK].timer != -1)
		return;

	idx    = RFIFOW(fd,GETPACKETPOS(cmd,0))-2;
	amount = RFIFOL(fd,GETPACKETPOS(cmd,1));

	if(sd->state.storage_flag==2)
		storage_guild_storageaddfromcart(sd, idx, amount);
	else if(sd->state.storage_flag==1)
		storage_storageaddfromcart(sd, idx, amount);

	return;
}

/*==========================================
 * �J�v���q�ɂ���J�[�g�֏o��
 *------------------------------------------
 */
static void clif_parse_MoveFromKafraToCart(int fd,struct map_session_data *sd, int cmd)
{
	int idx, amount;

	nullpo_retv(sd);

	if(sd->npc_id != 0 || sd->vender_id != 0 || sd->state.deal_mode != 0 || sd->state.mail_appending ||
	   pc_iscarton(sd) == 0 || sd->sc.data[SC_BERSERK].timer != -1)
		return;

	idx    = RFIFOW(fd,GETPACKETPOS(cmd,0))-1;
	amount = RFIFOL(fd,GETPACKETPOS(cmd,1));

	if(sd->state.storage_flag==2)
		storage_guild_storagegettocart(sd, idx, amount);
	else if(sd->state.storage_flag==1)
		storage_storagegettocart(sd, idx, amount);

	return;
}

/*==========================================
 * �J�v���q�ɂ����
 *------------------------------------------
 */
static void clif_parse_CloseKafra(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	if(sd->state.storage_flag==2) {
		storage_guild_storageclose(sd);
	} else if(sd->state.storage_flag==1) {
		storage_storageclose(sd);
	} else {
		sd->state.storage_flag = 0;
		clif_storageclose(sd);
	}

	return;
}

/*==========================================
 * �p�[�e�B�����
 *------------------------------------------
 */
static void clif_parse_CreateParty(int fd,struct map_session_data *sd, int cmd)
{
	if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 7)
		party_create(sd,RFIFOP(fd,GETPACKETPOS(cmd,0)),0,0);
	else
		clif_skill_fail(sd,1,0,4);

	return;
}

/*==========================================
 * �p�[�e�B�����2
 *------------------------------------------
 */
static void clif_parse_CreateParty2(int fd,struct map_session_data *sd, int cmd)
{
	int item1 = (int)RFIFOB(fd,GETPACKETPOS(cmd,1));
	int item2 = (int)RFIFOB(fd,GETPACKETPOS(cmd,2));

	if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 7)
		party_create(sd,RFIFOP(fd,GETPACKETPOS(cmd,0)),item1,item2);
	else
		clif_skill_fail(sd,1,0,4);

	return;
}

/*==========================================
 * �p�[�e�B�Ɋ��U
 *------------------------------------------
 */
static void clif_parse_PartyInvite(int fd,struct map_session_data *sd, int cmd)
{
	struct map_session_data *tsd = map_id2sd(RFIFOL(fd,GETPACKETPOS(cmd,0)));

	party_invite(sd,tsd);

	return;
}

/*==========================================
 * �p�[�e�B�Ɋ��U2
 *------------------------------------------
 */
static void clif_parse_PartyInvite2(int fd,struct map_session_data *sd, int cmd)
{
	char *name = (char *)RFIFOP(fd,GETPACKETPOS(cmd,0));
	struct map_session_data *tsd = map_nick2sd(name);

	party_invite(sd,tsd);

	return;
}

/*==========================================
 * �p�[�e�B���U�ԓ�
 *------------------------------------------
 */
static void clif_parse_ReplyPartyInvite(int fd,struct map_session_data *sd, int cmd)
{
	if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 5) {
		party_reply_invite(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)),RFIFOL(fd,GETPACKETPOS(cmd,1)));
	} else {
		party_reply_invite(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)),-1);
		clif_skill_fail(sd,1,0,4);
	}

	return;
}

/*==========================================
 * �p�[�e�B���U�ԓ�2
 *------------------------------------------
 */
static void clif_parse_ReplyPartyInvite2(int fd,struct map_session_data *sd, int cmd)
{
	if(battle_config.basic_skill_check == 0 || pc_checkskill(sd,NV_BASIC) >= 5) {
		party_reply_invite(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)),RFIFOB(fd,GETPACKETPOS(cmd,1)));
	} else {
		party_reply_invite(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)),-1);
		clif_skill_fail(sd,1,0,8);
	}

	return;
}

/*==========================================
 * /accept, /refuse
 *------------------------------------------
 */
static void clif_parse_RefusePartyInvite(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	sd->state.refuse_partyinvite = RFIFOB(fd,GETPACKETPOS(cmd,0));

	WFIFOW(fd,0) = 0x2c9;
	WFIFOB(fd,2) = sd->state.refuse_partyinvite;
	WFIFOSET(fd,packet_db[0x2c9].len);

	return;
}

/*==========================================
 * �p�[�e�B�E�ޗv��
 *------------------------------------------
 */
static void clif_parse_LeaveParty(int fd,struct map_session_data *sd, int cmd)
{
	party_leave(sd);

	return;
}

/*==========================================
 * �p�[�e�B�����v��
 *------------------------------------------
 */
static void clif_parse_RemovePartyMember(int fd,struct map_session_data *sd, int cmd)
{
	party_removemember(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)),RFIFOP(fd,GETPACKETPOS(cmd,1)));

	return;
}

/*==========================================
 * �p�[�e�B�ݒ�ύX�v��
 *------------------------------------------
 */
static void clif_parse_PartyChangeOption(int fd,struct map_session_data *sd, int cmd)
{
	party_changeoption(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)),-1);

	return;
}

/*==========================================
 * �p�[�e�B�ݒ�ύX�v��2
 *------------------------------------------
 */
static void clif_parse_PartyChangeOption2(int fd,struct map_session_data *sd, int cmd)
{
	int item = (int)RFIFOB(fd,GETPACKETPOS(cmd,1));
	int item2 = (int)RFIFOB(fd,GETPACKETPOS(cmd,1)+1);

	party_changeoption(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)),(item ? 1 : 0) | (item2 ? 2 : 0));

	return;
}

/*==========================================
 * �p�[�e�B���b�Z�[�W���M�v��
 *------------------------------------------
 */
static void clif_parse_PartyMessage(int fd,struct map_session_data *sd, int cmd)
{
	char *message;
	int message_size;

	nullpo_retv(sd);

	message = (char *)RFIFOP(fd, GETPACKETPOS(cmd,1));
	message_size = RFIFOW(fd, GETPACKETPOS(cmd,0)) - GETPACKETPOS(cmd,1);

	if (message_size <= 0)	// against hack
		return;
	if (message_size > 255)	// too long
		return;

	// normal client send NULL -> force it (against hacker)
	message[message_size - 1] = '\0';

	if (battle_config.check_player_name_party_msg) {
		// structure of message: <player_name> : <message>
		char *p_message;
		int name_length;
		name_length = (int)strlen(sd->status.name);
		if (name_length > 24)
			name_length = 24;
		p_message = message + name_length;
		if (message_size < name_length + 3 + 1 || // check void message (at least 1 char) -> normal client refuse to send void message
		    memcmp(message, sd->status.name, name_length) != 0 || // check player name
		    *p_message != ' ' || *(p_message + 1) != ':' || *(p_message + 2) != ' ') { // check ' : '
			// add here a message if necessary
			return;
		}
	}

	if (is_atcommand(fd, sd, message) != AtCommand_None)
		return;

	// �o�[�T�[�N�A�`���b�g�֎~��ԂȂ��b�s��
	if(sd->sc.data[SC_BERSERK].timer != -1 || sd->sc.data[SC_NOCHAT].timer != -1)
		return;

	party_send_message(sd, message, message_size);

	return;
}

/*==========================================
 * �I�X��
 *------------------------------------------
 */
static void clif_parse_CloseVending(int fd,struct map_session_data *sd, int cmd)
{
	vending_closevending(sd);

	return;
}

/*==========================================
 * �I�X�A�C�e�����X�g�v��
 *------------------------------------------
 */
static void clif_parse_VendingListReq(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	vending_vendinglistreq(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)));
	if(sd->npc_id)
		npc_event_dequeue(sd);

	return;
}

/*==========================================
 * �I�X�A�C�e���w��
 *------------------------------------------
 */
static void clif_parse_PurchaseReq(int fd,struct map_session_data *sd, int cmd)
{
	vending_purchasereq(sd,RFIFOW(fd,GETPACKETPOS(cmd,0)),RFIFOL(fd,GETPACKETPOS(cmd,1)),-1,RFIFOP(fd,GETPACKETPOS(cmd,2)));

	return;
}

/*==========================================
 * �I�X�A�C�e���w��2
 *------------------------------------------
 */
static void clif_parse_PurchaseReq2(int fd,struct map_session_data *sd, int cmd)
{
	vending_purchasereq(sd,RFIFOW(fd,GETPACKETPOS(cmd,0)),RFIFOL(fd,GETPACKETPOS(cmd,1)),RFIFOL(fd,GETPACKETPOS(cmd,2)),RFIFOP(fd,GETPACKETPOS(cmd,3)));

	return;
}

/*==========================================
 * �I�X�J��
 *------------------------------------------
 */
static void clif_parse_OpenVending(int fd,struct map_session_data *sd, int cmd)
{
	vending_openvending(sd, RFIFOW(fd,GETPACKETPOS(cmd,0)), RFIFOP(fd,GETPACKETPOS(cmd,1)), RFIFOB(fd,GETPACKETPOS(cmd,2)), RFIFOP(fd,GETPACKETPOS(cmd,3)));

	return;
}

/*==========================================
 * �M���h�����
 *------------------------------------------
 */
static void clif_parse_CreateGuild(int fd,struct map_session_data *sd, int cmd)
{
	guild_create(sd,RFIFOP(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 * �M���h�}�X�^�[���ǂ����m�F
 *------------------------------------------
 */
static void clif_parse_GuildCheckMaster(int fd,struct map_session_data *sd, int cmd)
{
	clif_guild_masterormember(sd);

	return;
}

/*==========================================
 * �M���h���v��
 *------------------------------------------
 */
static void clif_parse_GuildRequestInfo(int fd,struct map_session_data *sd, int cmd)
{
	struct guild *g;

	nullpo_retv(sd);

	// only a guild member can ask for guild info
	if(sd->status.guild_id == 0 || (g = guild_search(sd->status.guild_id)) == NULL)
		return;

	switch(RFIFOL(fd,GETPACKETPOS(cmd,0))) {
	case 0:	// �M���h��{���A�����G�Ώ��
		clif_guild_basicinfo(sd, g);
		clif_guild_allianceinfo(sd, g);
		clif_guild_emblem(sd, g);
		break;
	case 1:	// �����o�[���X�g�A��E�����X�g
		clif_guild_positionnamelist(sd, g);
		clif_guild_memberlist(sd, g);
		break;
	case 2:	// ��E�����X�g�A��E��񃊃X�g
		clif_guild_positionnamelist(sd, g);
		clif_guild_positioninfolist(sd, g);
		break;
	case 3:	// �X�L�����X�g
		clif_guild_skillinfo(sd, g);
		break;
	case 4:	// �Ǖ����X�g
		clif_guild_explusionlist(sd, g);
		break;
	default:
		if(battle_config.error_log)
			printf("clif: guild request info: unknown type %d\n", RFIFOL(fd,GETPACKETPOS(cmd,0)));
	}

	return;
}

/*==========================================
 * �M���h��E�ύX
 *------------------------------------------
 */
static void clif_parse_GuildChangePositionInfo(int fd,struct map_session_data *sd, int cmd)
{
	struct guild *g;
	int i, n, len;

	nullpo_retv(sd);

	// only guild master can change position.
	if (sd->status.guild_id == 0 || (g = guild_search(sd->status.guild_id)) == NULL)
		return;
	if (strcmp(sd->status.name, g->master))
		return;

	len = GETPACKETPOS(cmd,1);
	n   = (RFIFOW(fd,GETPACKETPOS(cmd,0)) - len) / 40;
	if(n <= 0)
		return;

	for(i = 0; i < n; i++) {	// {<index>.l <mode>.l <index>.l <exp_mode>.l <nickname>.24B}.40B*
		guild_change_position(sd->status.guild_id, RFIFOL(fd,len), RFIFOL(fd,len+4), RFIFOL(fd,len+12), RFIFOP(fd,len+16));
		len += 40;
	}

	return;
}

/*==========================================
 * �M���h�����o��E�ύX
 *------------------------------------------
 */
static void clif_parse_GuildChangeMemberPosition(int fd,struct map_session_data *sd, int cmd)
{
	struct guild *g;
	int i, n, len;

	nullpo_retv(sd);

	// only guild master can change a member position
	if (sd->status.guild_id == 0 || (g = guild_search(sd->status.guild_id)) == NULL)
		return;
	if (strcmp(sd->status.name, g->master))
		return;

	len = GETPACKETPOS(cmd,1);
	n   = (RFIFOW(fd,GETPACKETPOS(cmd,0)) - len) / 12;
	if(n <= 0)
		return;

	for(i = 0; i < n; i++) {	// {<accID>.l <charaID>.l <index>.l}.12B*
		guild_change_memberposition(sd->status.guild_id, RFIFOL(fd,len), RFIFOL(fd,len+4), RFIFOL(fd,len+8));
		len += 12;
	}

	return;
}

/*==========================================
 * �M���h�G���u�����v��
 *------------------------------------------
 */
static void clif_parse_GuildRequestEmblem(int fd,struct map_session_data *sd, int cmd)
{
	struct guild *g = guild_search(RFIFOL(fd,GETPACKETPOS(cmd,0)));

	if(g)
		clif_guild_emblem(sd,g);

	return;
}

/*==========================================
 * �M���h�G���u�����ύX
 *------------------------------------------
 */
static void clif_parse_GuildChangeEmblem(int fd,struct map_session_data *sd, int cmd)
{
	int offset;
	unsigned int len;
	struct guild *g;

	// only guild master can change emblem.
	if (sd->status.guild_id == 0 || (g = guild_search(sd->status.guild_id)) == NULL)
		return;
	if (strcmp(sd->status.name, g->master))
		return;

	offset = GETPACKETPOS(cmd,1);
	len    = RFIFOW(fd,GETPACKETPOS(cmd,0)) - offset;

	guild_change_emblem(sd->status.guild_id,len,RFIFOP(fd,offset));
}

/*==========================================
 * �M���h���m�ύX
 *------------------------------------------
 */
static void clif_parse_GuildChangeNotice(int fd,struct map_session_data *sd, int cmd)
{
	struct guild *g;
	int guild_id;

	nullpo_retv(sd);

	// only guild master can change guild message
	if (sd->status.guild_id == 0 || (g = guild_search(sd->status.guild_id)) == NULL)
		return;
	if (strcmp(sd->status.name, g->master))
		return;

	// you can not change message of another guild!
	guild_id = RFIFOL(fd,GETPACKETPOS(cmd,0));
	if (sd->status.guild_id != RFIFOL(fd,GETPACKETPOS(cmd,0)))
		return;

	// client sends NULL -> force it (angainst hack)
	RFIFOB(fd, GETPACKETPOS(cmd,1) + 59)  = '\0';
	RFIFOB(fd, GETPACKETPOS(cmd,2) + 119) = '\0';

	guild_change_notice(guild_id, RFIFOP(fd,GETPACKETPOS(cmd,1)), RFIFOP(fd,GETPACKETPOS(cmd,2)));

	return;
}

/*==========================================
 * �M���h���U
 *------------------------------------------
 */
static void clif_parse_GuildInvite(int fd,struct map_session_data *sd, int cmd) // S 0x0168 <TargetAccID>.l <sourceAccID>.l <myCharactorID>.l
{
	nullpo_retv(sd);

	// check to be sure that is original packet (not need to have these values after)
	if (RFIFOL(fd,GETPACKETPOS(cmd,1)) != sd->status.account_id ||
	    RFIFOL(fd,GETPACKETPOS(cmd,2)) != sd->status.char_id)
		return;

	guild_invite(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 * �M���h���U�ԐM
 *------------------------------------------
 */
static void clif_parse_GuildReplyInvite(int fd,struct map_session_data *sd, int cmd)
{
	guild_reply_invite(sd, RFIFOL(fd,GETPACKETPOS(cmd,0)), (unsigned char)RFIFOL(fd,GETPACKETPOS(cmd,1)));

	return;
}

/*==========================================
 * �M���h�E��
 *------------------------------------------
 */
static void clif_parse_GuildLeave(int fd,struct map_session_data *sd, int cmd)
{
	// client sends NULL -> force it (against hack)
	RFIFOB(fd, GETPACKETPOS(cmd,3) + 39) = '\0';

	guild_leave(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)),RFIFOL(fd,GETPACKETPOS(cmd,1)),RFIFOL(fd,GETPACKETPOS(cmd,2)),RFIFOP(fd,GETPACKETPOS(cmd,3)));

	return;
}

/*==========================================
 * �M���h�Ǖ�
 *------------------------------------------
 */
static void clif_parse_GuildExplusion(int fd,struct map_session_data *sd, int cmd)
{
	// client sends NULL -> force it (against hack)
	RFIFOB(fd, GETPACKETPOS(cmd,3) + 39) = '\0';

	guild_explusion(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)),RFIFOL(fd,GETPACKETPOS(cmd,1)),RFIFOL(fd,GETPACKETPOS(cmd,2)),RFIFOP(fd,GETPACKETPOS(cmd,3)));

	return;
}

/*==========================================
 * �M���h��b
 *------------------------------------------
 */
static void clif_parse_GuildMessage(int fd,struct map_session_data *sd, int cmd)
{
	char *message;
	int message_size;

	nullpo_retv(sd);

	message = (char *)RFIFOP(fd, GETPACKETPOS(cmd,1));
	message_size = RFIFOW(fd, GETPACKETPOS(cmd,0)) - GETPACKETPOS(cmd,1);

	if (message_size <= 0)	// against hack
		return;
	if (message_size > 255)	// too long
		return;

	// normal client send NULL -> force it (against hacker)
	message[message_size - 1] = '\0';

	if (battle_config.check_player_name_guild_msg) {
		// structure of message: <player_name> : <message>
		char *p_message;
		int name_length;
		name_length = (int)strlen(sd->status.name);
		if (name_length > 24)
			name_length = 24;
		p_message = message + name_length;
		if (message_size < name_length + 3 + 1 || // check void message (at least 1 char) -> normal client refuse to send void message
		    memcmp(message, sd->status.name, name_length) != 0 || // check player name
		    *p_message != ' ' || *(p_message + 1) != ':' || *(p_message + 2) != ' ') { // check ' : '
			// add here a message if necessary
			return;
		}
	}

	if (is_atcommand(fd, sd, message) != AtCommand_None)
		return;

	// �o�[�T�[�N�A�`���b�g�֎~��ԂȂ��b�s��
	if (sd->sc.data[SC_BERSERK].timer != -1 || sd->sc.data[SC_NOCHAT].timer != -1)
		return;

	guild_send_message(sd, message, message_size);

	return;
}

/*==========================================
 * �M���h�����v��
 *------------------------------------------
 */
static void clif_parse_GuildRequestAlliance(int fd,struct map_session_data *sd, int cmd)
{
	guild_reqalliance(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)));
}

/*==========================================
 * �M���h�����v���ԐM
 *------------------------------------------
 */
static void clif_parse_GuildReplyAlliance(int fd,struct map_session_data *sd, int cmd)
{
	guild_reply_reqalliance(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)),RFIFOL(fd,GETPACKETPOS(cmd,1)));
}

/*==========================================
 * �M���h�֌W����
 *------------------------------------------
 */
static void clif_parse_GuildDelAlliance(int fd,struct map_session_data *sd, int cmd)
{
	guild_delalliance(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)),RFIFOL(fd,GETPACKETPOS(cmd,1)));
}

/*==========================================
 * �M���h�G��
 *------------------------------------------
 */
static void clif_parse_GuildOpposition(int fd,struct map_session_data *sd, int cmd)
{
	guild_opposition(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)));
}

/*==========================================
 * �M���h���U
 *------------------------------------------
 */
static void clif_parse_GuildBreak(int fd,struct map_session_data *sd, int cmd)
{
	guild_break(sd,RFIFOP(fd,GETPACKETPOS(cmd,0)));
}

/*==========================================
 * �M���h�����o�[���i���g�p�j
 *------------------------------------------
 */
static void clif_parse_GuildMemberInfo(int fd,struct map_session_data *sd, int cmd)
{
	//int account_id = GETPACKETPOS(cmd,0);
	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_PetMenu(int fd,struct map_session_data *sd, int cmd)
{
	pet_menu(sd,RFIFOB(fd,GETPACKETPOS(cmd,0)));
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_CatchPet(int fd,struct map_session_data *sd, int cmd)
{
	pet_catch_process2(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)));
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_SelectEgg(int fd,struct map_session_data *sd, int cmd)
{
	pet_select_egg(sd,(short)(RFIFOW(fd,GETPACKETPOS(cmd,0))-2));
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_SendEmotion(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	if(sd->pd)
		clif_pet_emotion(sd->pd,RFIFOL(fd,GETPACKETPOS(cmd,0)));
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_ChangePetName(int fd,struct map_session_data *sd, int cmd)
{
	pet_change_name(sd,RFIFOP(fd,GETPACKETPOS(cmd,0)));
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_GMKick(int fd,struct map_session_data *sd, int cmd)
{
	struct block_list *target;
	int tid = RFIFOL(fd,GETPACKETPOS(cmd,0));

	nullpo_retv(sd);

	if(pc_isGM(sd) >= get_atcommand_level(AtCommand_Kick)) {
		target = map_id2bl(tid);
		if(target) {
			if(target->type == BL_PC) {
				struct map_session_data *tsd = (struct map_session_data *)target;
				if(pc_isGM(sd) > pc_isGM(tsd))
					clif_GM_kick(sd,tsd,1);
				else
					clif_GM_kickack(sd,0);
			} else if(target->type == BL_MOB) {
				struct mob_data *md = (struct mob_data *)target;
				sd->state.attack_type = 0;
				mob_damage(&sd->bl,md,md->hp,2);
			} else {
				clif_GM_kickack(sd,0);
			}
		} else {
			clif_GM_kickack(sd,0);
		}
	}
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_GMHide(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	if(pc_isGM(sd) >= get_atcommand_level(AtCommand_Hide)) {
		if(sd->sc.option&OPTION_SPECIALHIDING) {
			sd->sc.option &= ~OPTION_SPECIALHIDING;
			clif_displaymessage(fd, msg_txt(10)); // invisible off!
		} else {
			sd->sc.option |= OPTION_SPECIALHIDING;
			clif_displaymessage(fd, msg_txt(11)); // invisible!
		}
		clif_changeoption(&sd->bl);
		clif_send_clothcolor(&sd->bl);
	}
}

/*==========================================
 * GM�ɂ��`���b�g�֎~���ԕt�^
 *------------------------------------------
 */
static void clif_parse_GMReqNoChat(int fd,struct map_session_data *sd, int cmd)
{
	int tid = RFIFOL(fd,GETPACKETPOS(cmd,0));
	int type = RFIFOB(fd,GETPACKETPOS(cmd,1));
	int limit = RFIFOW(fd,GETPACKETPOS(cmd,2));
	struct map_session_data *dstsd = NULL;

	nullpo_retv(sd);

	if(battle_config.nomanner_mode)
		return;

	// �^�C���M���͐ԃG���ɂȂ�Ȃ�
	if(sd->sc.data[SC_RUN].timer != -1)
		return;
	// �O�̂��ߋ����ړ�����
	if(sd->sc.data[SC_FORCEWALKING].timer != -1)
		return;

	dstsd = map_id2sd(tid);
	if(dstsd == NULL)
		return;

	if( (type == 2 && !pc_isGM(sd) && sd->bl.id == tid) ||
	    (pc_isGM(sd) > pc_isGM(dstsd) && pc_isGM(sd) >= battle_config.gm_nomanner_lv) )
	{
		int dstfd = dstsd->fd;

		WFIFOW(dstfd,0)=0x14b;
		WFIFOB(dstfd,2)=(type==2)? 1: type;
		memcpy(WFIFOP(dstfd,3),sd->status.name,24);
		WFIFOSET(dstfd,packet_db[0x14b].len);

		if(!battle_config.nomanner_mode)
			dstsd->status.manner += (type == 0)? limit: -limit;
		if(dstsd->status.manner < 0) {
			status_change_start(&dstsd->bl,SC_NOCHAT,0,0,0,0,0,0);
		} else {
			dstsd->status.manner = 0;
			status_change_end(&dstsd->bl,SC_NOCHAT,-1);
		}
	}

	return;
}

/*==========================================
 * GM�ɂ��`���b�g�֎~���ԎQ�Ɓi�H�j
 *------------------------------------------
 */
static void clif_parse_GMReqNoChatCount(int fd,struct map_session_data *sd, int cmd)
{
	int tid = RFIFOL(fd,GETPACKETPOS(cmd,0));

	WFIFOW(fd,0)=0x1e0;
	WFIFOL(fd,2)=tid;
	snprintf(WFIFOP(fd,6),24,"%d",tid);
	WFIFOSET(fd,packet_db[0x1e0].len);

	return;
}

/*==========================================
 * /doridori�ɂ��SPR2�{
 *------------------------------------------
 */
static void clif_parse_doridori(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	if(sd->s_class.job == PC_JOB_SNV) {
		sd->state.sn_doridori = 1;
	}
	else if(sd->state.taekwonrest && sd->s_class.job >= PC_JOB_TK && sd->s_class.job <= PC_JOB_SL) {
		sd->state.tk_doridori_hp = 1;
		sd->state.tk_doridori_sp = 1;
	}

	return;
}

/*==========================================
 * �X�p�m�r�̔����g��
 *------------------------------------------
 */
static void clif_parse_sn_explosionspirits(int fd,struct map_session_data *sd, int cmd)
{
	int next, rate = 0;

	nullpo_retv(sd);

	if(sd->s_class.job != PC_JOB_SNV)
		return;

	next = pc_nextbaseexp(sd);
	if( (next > 0 && (rate = (int)((atn_bignumber)sd->status.base_exp * 1000 / next)) > 0 && rate % 100 == 0) ||
	    (next <= 0 && sd->status.base_exp >= battle_config.snovice_maxexp_border) )
	{
		if(battle_config.etc_log) {
			printf(
				"SuperNovice explosionspirits!! %d %d %d %d\n",
				sd->bl.id, sd->s_class.job, sd->status.base_exp, rate
			);
		}
		clif_skill_nodamage(&sd->bl,&sd->bl,MO_EXPLOSIONSPIRITS,5,1);
		status_change_start(&sd->bl,SC_EXPLOSIONSPIRITS,5,0,0,0,skill_get_time(MO_EXPLOSIONSPIRITS,5),0);
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static int pstrcmp(const void *a, const void *b)
{
	return strcmp((char *)a, (char *)b);
}

/*==========================================
 * Wis���ۋ��� /ex /in
 *------------------------------------------
 */
static void clif_parse_wisexin(int fd,struct map_session_data *sd, int cmd)
{
	char *name = RFIFOP(fd,GETPACKETPOS(cmd,0));
	int type   = RFIFOB(fd,GETPACKETPOS(cmd,1));
	int i;
	unsigned char flag = 1;

	nullpo_retv(sd);

	qsort(sd->wis_refusal[0], MAX_WIS_REFUSAL, sizeof(sd->wis_refusal[0]), pstrcmp);
	if(type == 0) {	// ex
		for(i=0; i<MAX_WIS_REFUSAL; i++) {	// ���łɒǉ�����Ă���Ή������Ȃ�
			if(strcmp(sd->wis_refusal[i], name) == 0) {
				flag = 0;
				clif_wisexin(sd, type, flag);
				return;
			}
		}
		for(i=0; i<MAX_WIS_REFUSAL; i++) {	// ��̋��ۃ��X�g�ɒǉ�(�Ƃ肠����)
			if(sd->wis_refusal[i][0] == 0) {
				memcpy(sd->wis_refusal[i], name, 24);
				sd->wis_refusal[i][23] = '\0';
				flag = 0;
				break;
			}
		}
		if(flag == 1) {
			memcpy(sd->wis_refusal[MAX_WIS_REFUSAL-1], name, 24);
			sd->wis_refusal[MAX_WIS_REFUSAL-1][23] = '\0';
			flag = 0;
		}
	} else {		// in
		for(i=0; i<MAX_WIS_REFUSAL; i++) {	// ��v���鋑�ۃ��X�g�����
			if(strcmp(sd->wis_refusal[i], name) == 0) {
				sd->wis_refusal[i][0] = 0;
				flag = 0;
			}
		}
		sd->state.wis_all = 0;
	}
	clif_wisexin(sd, type, flag);

	return;
}

/*==========================================
 * Wis���ۃ��X�g
 *------------------------------------------
 */
static void clif_parse_wisexlist(int fd,struct map_session_data *sd, int cmd)
{
	int i, j = 0;

	nullpo_retv(sd);

	qsort(sd->wis_refusal[0], MAX_WIS_REFUSAL, sizeof(sd->wis_refusal[0]), pstrcmp);

	WFIFOW(fd,0)=0xd4;
	for(i=0; i<MAX_WIS_REFUSAL; i++) {
		if(sd->wis_refusal[i][0] != 0) {
			memcpy(WFIFOP(fd,4+j*24),sd->wis_refusal[i],24);
			j++;
		}
	}
	WFIFOW(fd,2) = 4+j*24;
	WFIFOSET(fd,WFIFOW(fd,2));

	if(j >= MAX_WIS_REFUSAL)	// ���^���Ȃ�Ō��1������
		sd->wis_refusal[MAX_WIS_REFUSAL-1][0] = 0;

	return;
}

/*==========================================
 * Wis�S���ۋ��� /exall /inall
 *------------------------------------------
 */
static void clif_parse_wisall(int fd,struct map_session_data *sd, int cmd)
{
	int type = RFIFOB(fd,GETPACKETPOS(cmd,0));

	nullpo_retv(sd);

	if(type==0) {	// exall
		sd->state.wis_all = 1;
	} else {	// inall
		int i;
		for(i=0;i<MAX_WIS_REFUSAL;i++)	// ���ۃ��X�g�����
			sd->wis_refusal[i][0]=0;
		sd->state.wis_all = 0;
	}
	clif_wisall(sd, type, 0);
}

/*==========================================
 * GM�R�}���h /killall
 *------------------------------------------
 */
static void clif_parse_GMkillall(int fd,struct map_session_data *sd, int cmd)
{
	char str[40];

	nullpo_retv(sd);

	sprintf(str, "%ckickall", GM_Symbol());
	is_atcommand_sub(fd, sd, str, 0);

	return;
}

/*==========================================
 * GM�R�}���h /summon
 *------------------------------------------
 */
static void clif_parse_GMsummon(int fd,struct map_session_data *sd, int cmd)
{
	char str[64];

	nullpo_retv(sd);

	sprintf(str, "%csummon ", GM_Symbol());
	strncat(str, RFIFOP(fd,GETPACKETPOS(cmd,0)), 24);
	is_atcommand_sub(fd, sd, str, 0);

	return;
}

/*==========================================
 * GM�R�}���h /item /monster
 *------------------------------------------
 */
static void clif_parse_GMitemmonster( int fd, struct map_session_data *sd, int cmd )
{
	char str[64];

	nullpo_retv(sd);

	sprintf(str, "%cim ", GM_Symbol());
	strncat(str, RFIFOP(fd, GETPACKETPOS(cmd, 0)), 24);
	is_atcommand_sub(fd, sd, str, 0);

	return;
}

/*==========================================
 * GM�R�}���h /shift
 *------------------------------------------
 */
static void clif_parse_GMshift(int fd,struct map_session_data *sd, int cmd)
{
	char str[64];

	nullpo_retv(sd);

	sprintf(str, "%cjumpto ", GM_Symbol());
	strncat(str, RFIFOP(fd,GETPACKETPOS(cmd,0)), 24);
	is_atcommand_sub(fd, sd, str, 0);

	return;
}

/*==========================================
 * GM�R�}���h /recall
 *------------------------------------------
 */
static void clif_parse_GMrecall(int fd,struct map_session_data *sd, int cmd)
{
	char str[64];

	nullpo_retv(sd);

	sprintf(str, "%crecall ", GM_Symbol());
	strncat(str, RFIFOP(fd,GETPACKETPOS(cmd,0)), 24);
	is_atcommand_sub(fd, sd, str, 0);

	return;
}

/*==========================================
 * GM�R�}���h /remove
 *------------------------------------------
 */
static void clif_parse_GMremove(int fd,struct map_session_data *sd, int cmd)
{
	// ���ʕs��
	return;
}

/*==========================================
 * GM�R�}���h /changemaptype
 *------------------------------------------
 */
static void clif_parse_GMchangemaptype(int fd,struct map_session_data *sd, int cmd)
{
	int x,y,type;
	char str[64];

	nullpo_retv(sd);

	x    = RFIFOW(fd, GETPACKETPOS(cmd,0));
	y    = RFIFOW(fd, GETPACKETPOS(cmd,1));
	type = RFIFOW(fd, GETPACKETPOS(cmd,2));	// 0��1�̂�

	sprintf(str, "%cchangemaptype %d %d %d", GM_Symbol(), x, y, type&1);
	is_atcommand_sub(fd, sd, str, 0);

	return;
}

/*==========================================
 * GM�R�}���h /rc
 *------------------------------------------
 */
static void clif_parse_GMrc(int fd,struct map_session_data *sd, int cmd)
{
	// ���ʕs��
	return;
}

/*==========================================
 * GM�R�}���h /check
 *------------------------------------------
 */
static void clif_parse_GMcheck(int fd,struct map_session_data *sd, int cmd)
{
	// ���ʕs��
	return;
}

/*==========================================
 * GM�R�}���h /���
 *------------------------------------------
 */
static void clif_parse_GMcharge(int fd,struct map_session_data *sd, int cmd)
{
	// ���ʕs��
	return;
}

/*==========================================
 * �F�B���X�g�ǉ��v��
 *------------------------------------------
 */
static void clif_parse_FriendAddRequest(int fd,struct map_session_data *sd, int cmd)
{
	friend_add_request(sd, RFIFOP(fd,GETPACKETPOS(cmd,0)));
	return;
}

/*==========================================
 * �F�B���X�g�ǉ��v���ԓ�
 *------------------------------------------
 */
static void clif_parse_FriendAddReply(int fd,struct map_session_data *sd, int cmd)
{
	friend_add_reply(sd, RFIFOL(fd,GETPACKETPOS(cmd,0)), RFIFOL(fd,GETPACKETPOS(cmd,1)), RFIFOL(fd,GETPACKETPOS(cmd,2)));
	return;
}

/*==========================================
 * �F�B���X�g�폜�v��
 *------------------------------------------
 */
static void clif_parse_FriendDeleteRequest(int fd,struct map_session_data *sd, int cmd)
{
	friend_del_request(sd, RFIFOL(fd,GETPACKETPOS(cmd,0)), RFIFOL(fd,GETPACKETPOS(cmd,1)));
	return;
}

/*==========================================
 * /pvpinfo
 *------------------------------------------
 */
static void clif_parse_PvPInfo(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	if(!map[sd->bl.m].flag.pk)
		return;

	WFIFOW(fd,0)  = 0x210;
	WFIFOL(fd,2)  = sd->status.account_id;
	WFIFOL(fd,6)  = sd->status.char_id;
	WFIFOL(fd,10) = 0;	// times won
	WFIFOL(fd,14) = 0;	// times lost
	WFIFOL(fd,18) = ranking_get_point(sd, RK_PK);
	WFIFOSET(fd, packet_db[0x210].len);

	return;
}

/*==========================================
 * BS�����L���O	/blacksmith
 *------------------------------------------
 */
static void clif_parse_RankingBlacksmith(int fd,struct map_session_data *sd, int cmd)
{
	ranking_clif_display(sd,RK_BLACKSMITH);
	return;
}

/*==========================================
 * �A���P�~�����L���O	/alchemist
 *------------------------------------------
 */
static void clif_parse_RankingAlchemist(int fd,struct map_session_data *sd, int cmd)
{
	ranking_clif_display(sd,RK_ALCHEMIST);
	return;
}

/*==========================================
 * �e�R�������L���O	/taekwon
 *------------------------------------------
 */
static void clif_parse_RankingTaekwon(int fd,struct map_session_data *sd, int cmd)
{
	ranking_clif_display(sd,RK_TAEKWON);
	return;
}

/*==========================================
 * �s�E�҃����L���O	/pk
 *------------------------------------------
 */
static void clif_parse_RankingPk(int fd,struct map_session_data *sd, int cmd)
{
	ranking_clif_display(sd,RK_PK);
	return;
}

/*==========================================
 * ���[�����M
 *------------------------------------------
 */
static void clif_parse_SendMail(int fd,struct map_session_data *sd, int cmd)
{
	int bodylen;

	nullpo_retv(sd);

	if(!battle_config.romail) {	// ���[�����M���֎~
		clif_res_sendmail(fd,1);
		return;
	}

	bodylen = RFIFOW(fd,GETPACKETPOS(cmd,0)) - GETPACKETPOS(cmd,3) - 1;	// \0�͍��
	mail_checkmail(sd, RFIFOP(fd,GETPACKETPOS(cmd,1)), RFIFOP(fd,GETPACKETPOS(cmd,2)), RFIFOP(fd,GETPACKETPOS(cmd,3)), bodylen);

	return;
}

/*==========================================
 * ���[���̑I����M
 *------------------------------------------
 */
static void clif_parse_ReadMail(int fd,struct map_session_data *sd, int cmd)
{
	int mail_num = RFIFOL(fd,GETPACKETPOS(cmd,0));

	nullpo_retv(sd);

	intif_readmail(sd->status.char_id,mail_num);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_MailGetAppend(int fd,struct map_session_data *sd, int cmd)
{
	int mail_num = RFIFOL(fd,GETPACKETPOS(cmd,0));

	nullpo_retv(sd);

	intif_mail_getappend(sd->status.char_id,mail_num);
	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_MailWinOpen(int fd,struct map_session_data *sd, int cmd)
{
	int flag;

	nullpo_retv(sd);

	// 0: �S�ĕԋp 1: �A�C�e���̂ݕԋp 2: Zeny�̂ݕԋp
	flag = RFIFOW(fd,GETPACKETPOS(cmd,0));

	mail_removeitem(sd, flag&3);

	return;
}

/*==========================================
 * ���[��BOX�̍X�V�v��
 *------------------------------------------
 */
static void clif_parse_RefleshMailBox(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	intif_mailbox(sd->status.char_id);

	return;
}

/*==========================================
 * ���[���ɓY�t
 *------------------------------------------
 */
static void clif_parse_SendMailSetAppend(int fd,struct map_session_data *sd, int cmd)
{
	int idx    = RFIFOW(fd,GETPACKETPOS(cmd,0));
	int amount = RFIFOL(fd,GETPACKETPOS(cmd,1));

	nullpo_retv(sd);

	if (idx < 0 || amount < 0)
		return;

	mail_setitem(sd, idx, amount);
	return;
}

/*==========================================
 * ���[���폜
 *------------------------------------------
 */
static void clif_parse_DeleteMail(int fd,struct map_session_data *sd, int cmd)
{
	int mail_num = RFIFOL(fd,GETPACKETPOS(cmd,0));

	nullpo_retv(sd);

	intif_deletemail(sd->status.char_id,mail_num);
	return;
}

/*==========================================
 * ���[���ԑ�
 *------------------------------------------
 */
static void clif_parse_ReturnMail(int fd,struct map_session_data *sd, int cmd)
{
	//int mail_num = RFIFOL(fd,GETPACKETPOS(cmd,0));
	//char *name = (char*)RFIFOP(fd,GETPACKETPOS(cmd,1));

	nullpo_retv(sd);

	// �Ƃ肠����������
	return;
}

/*==========================================
 * �z�����N���X
 *------------------------------------------
 */
static void clif_parse_HomMenu(int fd,struct map_session_data *sd, int cmd)
{
	homun_menu(sd,RFIFOB(fd,GETPACKETPOS(cmd,0)));
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_HomMercWalkMaster(int fd,struct map_session_data *sd, int cmd)
{
	struct block_list *bl = map_id2bl(RFIFOL(fd,GETPACKETPOS(cmd,0)));

	nullpo_retv(sd);

	if(bl == NULL || bl->prev == NULL)
		return;

	if(bl->type == BL_HOM) {
		if(sd->hd && bl == &sd->hd->bl) {
			homun_return_master(sd);
		}
	} else if(bl->type == BL_MERC) {
		if(sd->mcd && bl == &sd->mcd->bl) {
			merc_return_master(sd);
		}
	}
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_HomMercWalkToXY(int fd,struct map_session_data *sd, int cmd)
{
	struct block_list *bl = map_id2bl(RFIFOL(fd,GETPACKETPOS(cmd,0)));

	nullpo_retv(sd);

	if(bl == NULL || bl->prev == NULL)
		return;

	if( (bl->type == BL_HOM && sd->hd && bl == &sd->hd->bl) ||
	    (bl->type == BL_MERC && sd->mcd && bl == &sd->mcd->bl) )
	{
		int x = RFIFOB(fd,GETPACKETPOS(cmd,1))*4+(RFIFOB(fd,GETPACKETPOS(cmd,1)+1)>>6);
		int y = ((RFIFOB(fd,GETPACKETPOS(cmd,1)+1)&0x3f)<<4)+(RFIFOB(fd,GETPACKETPOS(cmd,1)+2)>>4);

		unit_walktoxy(bl,x,y);
	}
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_HomMercActionRequest(int fd,struct map_session_data *sd, int cmd)
{
	struct block_list *src = map_id2bl(RFIFOL(fd,GETPACKETPOS(cmd,0)));
	struct status_change *sc;
	int action_type,target_id;

	nullpo_retv(sd);

	if(src == NULL || src->prev == NULL)
		return;

	if(src->type == BL_HOM && (!sd->hd || src != &sd->hd->bl))
		return;
	else if(src->type == BL_MERC && (!sd->mcd || src != &sd->mcd->bl))
		return;

	if(unit_isdead(src)) {
		clif_clearchar_area(src,1);
		return;
	}

	sc = status_get_sc(src);
	if(sc && sc->option&OPTION_HIDE)
		return;

	unit_stop_walking(src,1);
	unit_stopattack(src);

	target_id   = RFIFOL(fd,GETPACKETPOS(cmd,1));
	action_type = RFIFOB(fd,GETPACKETPOS(cmd,2));

	// �v���C���[�ƈႢ once attack �����Ȃ�
	if(action_type == 0) {
		struct block_list *bl = map_id2bl(target_id);
		if(bl && mob_gvmobcheck(sd,bl) == 0)
			return;
		unit_attack(src,target_id,0);
	}
	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static void clif_parse_ChangeHomName(int fd,struct map_session_data *sd, int cmd)
{
	homun_change_name(sd,RFIFOP(fd,GETPACKETPOS(cmd,0)));
}

/*==========================================
 * /effect�Ƃ�
 *------------------------------------------
 */
static void clif_parse_clientsetting(int fd,struct map_session_data *sd, int cmd)
{
	// RFIFOL(fd,GETPACKETPOS(cmd,0))	effect��؂��Ă邩�ǂ���
	return;
}

/*==========================================
 * �{�q�v��
 *------------------------------------------
 */
static void clif_parse_BabyRequest(int fd,struct map_session_data *sd, int cmd)
{
	struct map_session_data* baby_sd = NULL;

	nullpo_retv(sd);

	baby_sd = map_id2sd(RFIFOL(fd,GETPACKETPOS(cmd,0)));

	if(baby_sd) {
		struct map_session_data *p_sd = pc_get_partner(sd);

		if(p_sd && pc_check_adopt_condition(baby_sd, sd, p_sd, 1)) {
			baby_sd->adopt_invite = sd->status.account_id;
			clif_baby_req_display(baby_sd, sd, p_sd->status.account_id);
		}
	}
	return;
}

/*==========================================
 * �{�q�ԓ�
 *------------------------------------------
 */
static void clif_parse_BabyReply(int fd,struct map_session_data *sd, int cmd)
{
	pc_adopt_reply(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)),RFIFOL(fd,GETPACKETPOS(cmd,1)),RFIFOL(fd,GETPACKETPOS(cmd,2)));
	return;
}

/*==========================================
 * ���z�ƌ��Ɛ��̊���̃Z�[�u�ԓ�
 *------------------------------------------
 */
static void clif_parse_FeelSaveAck(int fd,struct map_session_data *sd, int cmd)
{
	clif_feel_saveack(sd,(int)RFIFOB(fd,GETPACKETPOS(cmd,0)));
	return;
}

/*==========================================
 * �����v������
 *------------------------------------------
 */
static void clif_parse_Making(int fd,struct map_session_data *sd, int cmd)
{
	int nameid;

	nullpo_retv(sd);

	sd->state.produce_flag = 0;

	nameid = RFIFOW(fd,GETPACKETPOS(cmd,0));
	skill_produce_mix(sd,nameid,0,0,0);

	return;
}

/*==========================================
 * �z�b�g�L�[�̃Z�[�u
 *------------------------------------------
 */
static void clif_parse_HotkeySave(int fd,struct map_session_data *sd, int cmd)
{
	int idx, hotkeys;
#if PACKETVER >= 21
	hotkeys = 38;
#elif PACKETVER >= 20
	hotkeys = 36;
#else
	hotkeys = 27;
#endif

	nullpo_retv(sd);

	idx = sd->hotkey_set * hotkeys + RFIFOW(fd,GETPACKETPOS(cmd,0));
	if(idx < 0 || idx >= MAX_HOTKEYS)
		return;

	sd->status.hotkey[idx].type = RFIFOB(fd,GETPACKETPOS(cmd,1));
	sd->status.hotkey[idx].id   = RFIFOL(fd,GETPACKETPOS(cmd,2));
	sd->status.hotkey[idx].lv   = RFIFOW(fd,GETPACKETPOS(cmd,3));

	return;
}

/*==========================================
 * �W�[�N�t���[�h�̏؂ɂ�镜��
 *------------------------------------------
 */
static void clif_parse_Revive(int fd,struct map_session_data *sd, int cmd)
{
	int idx;

	nullpo_retv(sd);

	if(map[sd->bl.m].flag.pvp && sd->pvp_point < 0)	// PVP�ŕ����s�\���
		return;
	if(!unit_isdead(&sd->bl))
		return;
	if((idx = pc_search_inventory(sd,7621)) < 0)	// �W�[�N�t���[�h�̏؂��������Ă��Ȃ�
		return;

	pc_delitem(sd,idx,1,0,1);
	sd->status.hp = sd->status.max_hp;
	sd->status.sp = sd->status.max_sp;
	clif_updatestatus(sd,SP_HP);
	clif_updatestatus(sd,SP_SP);
	pc_setstand(sd);
	if(battle_config.pc_invincible_time > 0)
		pc_setinvincibletimer(sd,battle_config.pc_invincible_time);
	clif_resurrection(&sd->bl,1);
	clif_misceffect2(&sd->bl,77);	// ���U�̃G�t�F�N�g��t���Ă݂�

	return;
}

/*==========================================
 * �b�����j���[
 *------------------------------------------
 */
static void clif_parse_MercMenu(int fd,struct map_session_data *sd, int cmd)
{
	merc_menu(sd,RFIFOB(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 * �����E�B���h�E�\���v��
 *------------------------------------------
 */
static void clif_parse_PartyEquipWindow(int fd,struct map_session_data *sd, int cmd)
{
	party_equip_window(sd, RFIFOL(fd,GETPACKETPOS(cmd,0)));

	return;
}

/*==========================================
 * �������J�v��
 *------------------------------------------
 */
static void clif_parse_PartyEquipOpen(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	sd->state.show_equip = RFIFOL(fd,GETPACKETPOS(cmd,0));

	WFIFOW(fd,0) = 0x2d9;
	WFIFOL(fd,2) = 0;
	WFIFOL(fd,6) = sd->state.show_equip;
	WFIFOSET(fd,packet_db[0x2d9].len);

	return;
}

/*==========================================
 * �n���e�B���O���X�g
 * �ڍוs���Ȃ��ߖ�����
 *------------------------------------------
 */
static void clif_parse_HuntingList(int fd,struct map_session_data *sd, int cmd)
{
/*
	char buf[] = "";

	nullpo_retv(sd);

	WFIFOW(fd,0) = 0x27a;
	WFIFOW(fd,2) = strlen(buf) + 1;
	memcpy(WFIFOP(fd,4), buf, sizeof(buf));
	WFIFOSET(fd,WFIFOW(fd,2));
*/
	return;
}

/*==========================================
 * ���`���b�g���M�v��
 * �ڍוs���Ȃ��ߖ�����
 *------------------------------------------
 */
static void clif_parse_BattleMessage(int fd,struct map_session_data *sd, int cmd)
{
	char *message;
	int len, message_size;

	nullpo_retv(sd);

	len = RFIFOW(fd, GETPACKETPOS(cmd,0));
	message = (char *)RFIFOP(fd, GETPACKETPOS(cmd,1));
	message_size = len - GETPACKETPOS(cmd,1); // including NULL

	if (message_size < 8)	// name (mini:4) + " : " (3) + NULL (1) (void mesages are possible for skills)
		return;
	if (message_size > 255)	// too long
		return;

	message[message_size - 1] = 0; // be sure to have a NULL (hacker can send a no-NULL terminated string)

	if (battle_config.check_player_name_battle_msg) {
		// structure of message: <player_name> : <message>
		int name_length;
		char *p_message;
		name_length = (int)strlen(sd->status.name);
		if (name_length > 24)
			name_length = 24;
		p_message = message + name_length;
		if (message_size < name_length + 3 + 1 || // check void message (at least 1 char) -> normal client refuse to send void message (but some skills can)
		    memcmp(message, sd->status.name, name_length) != 0 || // check player name
		    *p_message != ' ' || *(p_message + 1) != ':' || *(p_message + 2) != ' ') { // check ' : '
			// add here a message if necessary
			return;
		}
	}

	if (is_atcommand(fd, sd, message) != AtCommand_None)
		return;

	// �o�[�T�[�N�A�`���b�g�֎~��ԂȂ��b�s��
	if (sd->sc.data[SC_BERSERK].timer != -1 || sd->sc.data[SC_NOCHAT].timer != -1)
		return;

/*
	WFIFOW(fd,0) = 0x2dc;
	WFIFOW(fd,2) = message_size + 32;
	WFIFOL(fd,4) = sd->bl.id;
	memcpy(WFIFOP(fd,8), sd->status.name, 24);
	memcpy(WFIFOP(fd,32), message, message_size);
	clif_send(WFIFOP(fd,0), WFIFOW(fd,2), &sd->bl, AREA);
*/

	return;
}

/*==========================================
* �p�[�e�B�[���[�_�[�`�F���W
*------------------------------------------*/
static void clif_parse_PartyChangeLeader(int fd,struct map_session_data *sd, int cmd)
{
	party_changeleader(sd,RFIFOL(fd,GETPACKETPOS(cmd,0)));

	return; 
}

/*==========================================
* GM�ɂ�鑕������
*------------------------------------------*/
static void clif_parse_GmFullstrip(int fd,struct map_session_data *sd, int cmd)
{
	int id,lv,i;
	struct map_session_data *tsd = NULL;

	nullpo_retv(sd);

	id = RFIFOL(fd, GETPACKETPOS(cmd,0));
	lv = pc_isGM(sd);
	tsd = map_id2sd(id);
	if(tsd == NULL)
		return;

	if(lv <= 0 || lv < pc_isGM(tsd))
		return;

	for(i=0; i<MAX_INVENTORY; i++) {
		if(tsd->status.inventory[i].equip) {
			pc_unequipitem(tsd,i,0);
		}
	}

	return;
}

/*==========================================
* �p�[�e�B�[�u�b�L���O�o�^
*------------------------------------------*/
static void clif_parse_PartyBookingRegisterReq(int fd,struct map_session_data *sd, int cmd)
{
	int i,lv,map;
	int job[6];

	lv = RFIFOW(fd,GETPACKETPOS(cmd,0));
	map = RFIFOW(fd,GETPACKETPOS(cmd,1));
	for(i=0; i<6; i++)
		job[i] = RFIFOW(fd,GETPACKETPOS(cmd,2+i));

	booking_register(sd,lv,map,job);

	return;
}

/*==========================================
* �p�[�e�B�[�u�b�L���O�����v��
*------------------------------------------*/
static void clif_parse_PartyBookingSearchReq(int fd,struct map_session_data *sd, int cmd)
{
	booking_searchcond(sd,RFIFOW(fd,GETPACKETPOS(cmd,0)),RFIFOW(fd,GETPACKETPOS(cmd,1)),RFIFOW(fd,GETPACKETPOS(cmd,2)),RFIFOL(fd,GETPACKETPOS(cmd,3)),RFIFOW(fd,GETPACKETPOS(cmd,4)));

	return;
}

/*==========================================
* �p�[�e�B�[�u�b�L���O�폜�v��
*------------------------------------------*/
static void clif_parse_PartyBookingDeleteReq(int fd,struct map_session_data *sd, int cmd)
{
	booking_delete(sd);

	return;
}

/*==========================================
* �p�[�e�B�[�u�b�L���O�A�b�v�f�[�g�v��
*------------------------------------------*/
static void clif_parse_PartyBookingUpdateReq(int fd,struct map_session_data *sd, int cmd)
{
	int i;
	int job[6];
	
	for(i=0; i<6; i++)
		job[i] = RFIFOW(fd,GETPACKETPOS(cmd,0+i));

	booking_update(sd,job);

	return;
}

/*==========================================
 * �X�L�����X�g�I����M
 *------------------------------------------
 */
static void clif_parse_SelectSkill(int fd,struct map_session_data *sd, int cmd)
{
	nullpo_retv(sd);

	switch(sd->skill_menu.id)
	{
		case SC_AUTOSHADOWSPELL:		/* �V���h�E�I�[�g�X�y�� */
			skill_autoshadowspell(sd,RFIFOW(fd,GETPACKETPOS(cmd,1)));
			break;
	}

	sd->skill_menu.id = 0;
	sd->skill_menu.lv = 0;

	return;
}

/*==========================================
 * �N���C�A���g�̃f�X�g���N�^
 *------------------------------------------
 */
static int clif_disconnect(int fd)
{
	struct map_session_data *sd = (struct map_session_data *)session[fd]->session_data;

	if(sd && sd->state.auth) {
		clif_quitsave(sd);
	}
	close(fd);

	if(sd) {
		struct map_session_data *tmpsd = map_id2sd(sd->bl.id);
		if(tmpsd == sd)
			map_deliddb(&sd->bl);
		if(sd->bl.prev)
			map_delblock(&sd->bl);
	}

	return 0;
}

/*==========================================
 * �N���C�A���g����̃p�P�b�g���
 * socket.c��do_parsepacket����Ăяo�����
 *------------------------------------------
 */
int clif_parse(int fd)
{
	int cmd, packet_len = 0;
	struct map_session_data *sd;

	// char�I�Ɍq�����ĂȂ��Ԃ͐ڑ��֎~
	if(!chrif_isconnect())
		session[fd]->eof = 1;

	while(session[fd] != NULL && !session[fd]->eof) {
		sd = (struct map_session_data *)session[fd]->session_data;

		if(RFIFOREST(fd) < 2)
			return 0;
		cmd = RFIFOW(fd,0);

		// �Ǘ��p�p�P�b�g����
		if(cmd >= 30000) {
			switch(cmd) {
			case 0x7530:	// Auriga���擾
				WFIFOW(fd,0)=0x7531;
				WFIFOB(fd,2)=AURIGA_MAJOR_VERSION;
				WFIFOB(fd,3)=AURIGA_MINOR_VERSION;
				WFIFOW(fd,4)=AURIGA_REVISION;
				WFIFOB(fd,6)=AURIGA_RELEASE_FLAG;
				WFIFOB(fd,7)=AURIGA_OFFICIAL_FLAG;
				WFIFOB(fd,8)=AURIGA_SERVER_MAP;
				WFIFOW(fd,9)=get_current_version();
				WFIFOSET(fd,11);
				RFIFOSKIP(fd,2);
				break;
			case 0x7532:	// �ڑ��̐ؒf
				close(fd);
				session[fd]->eof=1;
				break;
			}
			return 0;
		}

		// �Q�[���p�ȊO�̃p�P�b�g�Ȃ̂Őؒf
		if(cmd >= MAX_PACKET_DB || packet_db[cmd].len == 0) {
			printf("clif_parse: unsupported packet 0x%04x disconnect %d\n", cmd, fd);
			close(fd);
			session[fd]->eof = 1;
			return 0;
		}

		// �F�؂��I����O��0072�ȊO�������̂Őؒf
		if((!sd || sd->state.auth == 0) && packet_db[cmd].func != clif_parse_WantToConnection) {
			close(fd);
			session[fd]->eof = 1;
			return 0;
		}

		// �p�P�b�g�����v�Z
		packet_len = packet_db[cmd].len;
		if(packet_len == -1) {
			if(RFIFOREST(fd) < 4)
				return 0;	// �ϒ��p�P�b�g�Œ����̏��܂Ńf�[�^�����ĂȂ�
			packet_len = RFIFOW(fd,2);
			if(packet_len < 4 || packet_len > 0x8000) {
				printf("clif_parse: invalid packet 0x%04x length %d disconnect %d\n", cmd, packet_len, fd);
				close(fd);
				session[fd]->eof = 1;
				return 0;
			}
		}
		if(RFIFOREST(fd) < packet_len)
			return 0;	// �܂�1�p�P�b�g���f�[�^�������ĂȂ�

		if(sd && sd->state.auth == 1 && sd->state.waitingdisconnect) {
			;	// �ؒf�҂��̏ꍇ�p�P�b�g���������Ȃ�
		} else if(sd && sd->bl.prev == NULL && packet_db[cmd].func != clif_parse_LoadEndAck) {
			;	// �u���b�N�Ɍq�����Ă��Ȃ��Ƃ���007d�ȊO�������珈�����Ȃ�
		} else if(packet_db[cmd].func) {
			g_packet_len = packet_len;	// GETPACKETPOS �p�ɕۑ�
			// �p�P�b�g����
			packet_db[cmd].func(fd,sd,cmd);
		} else {
			// �s���ȃp�P�b�g
			if(battle_config.error_log) {
				printf("clif_parse: unknown packet 0x%04x %d\n", cmd, fd);
#ifdef DUMP_UNKNOWN_PACKET
				hex_dump(stdout, RFIFOP(fd,0), packet_len);
				printf("\n");
#endif
			}
		}
		RFIFOSKIP(fd,packet_len);
	}

	return 0;
}

/*==========================================
 * �p�P�b�g�f�[�^�x�[�X�ǂݍ���
 *------------------------------------------
 */
static void packetdb_readdb(void)
{
	FILE *fp;
	char line[1024];
	int ln=0;
	int cmd,j;
	char *str[32],*p,*str2[32],*p2;
	struct {
		void (*func)(int fd,struct map_session_data *sd, int cmd);
		const char *name;
	} clif_parse_func[] = {
		{ clif_parse_WantToConnection,          "wanttoconnection"          },
		{ clif_parse_LoadEndAck,                "loadendack"                },
		{ clif_parse_TickSend,                  "ticksend"                  },
		{ clif_parse_WalkToXY,                  "walktoxy"                  },
		{ clif_parse_QuitGame,                  "quitgame"                  },
		{ clif_parse_GetCharNameRequest,        "getcharnamerequest"        },
		{ clif_parse_GlobalMessage,             "globalmessage"             },
		{ clif_parse_MapMove,                   "mapmove"                   },
		{ clif_parse_ChangeDir,                 "changedir"                 },
		{ clif_parse_Emotion,                   "emotion"                   },
		{ clif_parse_HowManyConnections,        "howmanyconnections"        },
		{ clif_parse_ActionRequest,             "actionrequest"             },
		{ clif_parse_Restart,                   "restart"                   },
		{ clif_parse_Wis,                       "wis"                       },
		{ clif_parse_GMmessage,                 "gmmessage"                 },
		{ clif_parse_TakeItem,                  "takeitem"                  },
		{ clif_parse_DropItem,                  "dropitem"                  },
		{ clif_parse_UseItem,                   "useitem"                   },
		{ clif_parse_EquipItem,                 "equipitem"                 },
		{ clif_parse_UnequipItem,               "unequipitem"               },
		{ clif_parse_NpcClicked,                "npcclicked"                },
		{ clif_parse_NpcBuySellSelected,        "npcbuysellselected"        },
		{ clif_parse_NpcBuyListSend,            "npcbuylistsend"            },
		{ clif_parse_NpcSellListSend,           "npcselllistsend"           },
		{ clif_parse_NpcPointShopBuy,           "npcpointshopbuy"           },
		{ clif_parse_CreateChatRoom,            "createchatroom"            },
		{ clif_parse_ChatAddMember,             "chataddmember"             },
		{ clif_parse_ChatRoomStatusChange,      "chatroomstatuschange"      },
		{ clif_parse_ChangeChatOwner,           "changechatowner"           },
		{ clif_parse_KickFromChat,              "kickfromchat"              },
		{ clif_parse_ChatLeave,                 "chatleave"                 },
		{ clif_parse_TradeRequest,              "traderequest"              },
		{ clif_parse_TradeAck,                  "tradeack"                  },
		{ clif_parse_TradeAddItem,              "tradeadditem"              },
		{ clif_parse_TradeOk,                   "tradeok"                   },
		{ clif_parse_TradeCancel,               "tradecancel"               },
		{ clif_parse_TradeCommit,               "tradecommit"               },
		{ clif_parse_StopAttack,                "stopattack"                },
		{ clif_parse_PutItemToCart,             "putitemtocart"             },
		{ clif_parse_GetItemFromCart,           "getitemfromcart"           },
		{ clif_parse_RemoveOption,              "removeoption"              },
		{ clif_parse_ChangeCart,                "changecart"                },
		{ clif_parse_StatusUp,                  "statusup"                  },
		{ clif_parse_SkillUp,                   "skillup"                   },
		{ clif_parse_UseSkillToId,              "useskilltoid"              },
		{ clif_parse_UseSkillToPos,             "useskilltopos"             },
		{ clif_parse_UseSkillMap,               "useskillmap"               },
		{ clif_parse_RequestMemo,               "requestmemo"               },
		{ clif_parse_ProduceMix,                "producemix"                },
		{ clif_parse_RepairItem,                "repairitem"                },
		{ clif_parse_WeaponRefine,              "weaponrefine"              },
		{ clif_parse_NpcSelectMenu,             "npcselectmenu"             },
		{ clif_parse_NpcNextClicked,            "npcnextclicked"            },
		{ clif_parse_NpcAmountInput,            "npcamountinput"            },
		{ clif_parse_NpcStringInput,            "npcstringinput"            },
		{ clif_parse_NpcCloseClicked,           "npccloseclicked"           },
		{ clif_parse_ItemIdentify,              "itemidentify"              },
		{ clif_parse_SelectItem,                "selectitem"                },
		{ clif_parse_AutoSpell,                 "autospell"                 },
		{ clif_parse_UseCard,                   "usecard"                   },
		{ clif_parse_InsertCard,                "insertcard"                },
		{ clif_parse_SolveCharName,             "solvecharname"             },
		{ clif_parse_ResetChar,                 "resetchar"                 },
		{ clif_parse_LGMmessage,                "lgmmessage"                },
		{ clif_parse_MoveToKafra,               "movetokafra"               },
		{ clif_parse_MoveFromKafra,             "movefromkafra"             },
		{ clif_parse_MoveToKafraFromCart,       "movetokafrafromcart"       },
		{ clif_parse_MoveFromKafraToCart,       "movefromkafratocart"       },
		{ clif_parse_CloseKafra,                "closekafra"                },
		{ clif_parse_CreateParty,               "createparty"               },
		{ clif_parse_CreateParty2,              "createparty2"              },
		{ clif_parse_PartyInvite,               "partyinvite"               },
		{ clif_parse_PartyInvite2,              "partyinvite2"              },
		{ clif_parse_ReplyPartyInvite,          "replypartyinvite"          },
		{ clif_parse_ReplyPartyInvite2,         "replypartyinvite2"         },
		{ clif_parse_RefusePartyInvite,         "refusepartyinvite"         },
		{ clif_parse_LeaveParty,                "leaveparty"                },
		{ clif_parse_RemovePartyMember,         "removepartymember"         },
		{ clif_parse_PartyChangeOption,         "partychangeoption"         },
		{ clif_parse_PartyChangeOption2,        "partychangeoption2"        },
		{ clif_parse_PartyMessage,              "partymessage"              },
		{ clif_parse_CloseVending,              "closevending"              },
		{ clif_parse_VendingListReq,            "vendinglistreq"            },
		{ clif_parse_PurchaseReq,               "purchasereq"               },
		{ clif_parse_PurchaseReq2,              "purchasereq2"              },
		{ clif_parse_OpenVending,               "openvending"               },
		{ clif_parse_CreateGuild,               "createguild"               },
		{ clif_parse_GuildCheckMaster,          "guildcheckmaster"          },
		{ clif_parse_GuildRequestInfo,          "guildrequestinfo"          },
		{ clif_parse_GuildChangePositionInfo,   "guildchangepositioninfo"   },
		{ clif_parse_GuildChangeMemberPosition, "guildchangememberposition" },
		{ clif_parse_GuildRequestEmblem,        "guildrequestemblem"        },
		{ clif_parse_GuildChangeEmblem,         "guildchangeemblem"         },
		{ clif_parse_GuildChangeNotice,         "guildchangenotice"         },
		{ clif_parse_GuildInvite,               "guildinvite"               },
		{ clif_parse_GuildReplyInvite,          "guildreplyinvite"          },
		{ clif_parse_GuildLeave,                "guildleave"                },
		{ clif_parse_GuildExplusion,            "guildexplusion"            },
		{ clif_parse_GuildMessage,              "guildmessage"              },
		{ clif_parse_GuildRequestAlliance,      "guildrequestalliance"      },
		{ clif_parse_GuildReplyAlliance,        "guildreplyalliance"        },
		{ clif_parse_GuildDelAlliance,          "guilddelalliance"          },
		{ clif_parse_GuildOpposition,           "guildopposition"           },
		{ clif_parse_GuildBreak,                "guildbreak"                },
		{ clif_parse_GuildMemberInfo,           "guildmemberinfo"           },
		{ clif_parse_PetMenu,                   "petmenu"                   },
		{ clif_parse_CatchPet,                  "catchpet"                  },
		{ clif_parse_SelectEgg,                 "selectegg"                 },
		{ clif_parse_SendEmotion,               "sendemotion"               },
		{ clif_parse_ChangePetName,             "changepetname"             },
		{ clif_parse_GMKick,                    "gmkick"                    },
		{ clif_parse_GMHide,                    "gmhide"                    },
		{ clif_parse_GMReqNoChat,               "gmreqnochat"               },
		{ clif_parse_GMReqNoChatCount,          "gmreqnochatcount"          },
		{ clif_parse_doridori,                  "doridori"                  },
		{ clif_parse_sn_explosionspirits,       "snexplosionspirits"        },
		{ clif_parse_wisexin,                   "wisexin"                   },
		{ clif_parse_wisexlist,                 "wisexlist"                 },
		{ clif_parse_wisall,                    "wisall"                    },
		{ clif_parse_GMkillall,                 "killall"                   },
		{ clif_parse_GMsummon,                  "summon"                    },
		{ clif_parse_GMitemmonster,             "itemmonster"               },
		{ clif_parse_GMshift,                   "shift"                     },
		{ clif_parse_GMrecall,                  "recall"                    },
		{ clif_parse_GMremove,                  "gmremove"                  },
		{ clif_parse_GMchangemaptype,           "changemaptype"             },
		{ clif_parse_GMrc,                      "gmrc"                      },
		{ clif_parse_GMcheck,                   "gmcheck"                   },
		{ clif_parse_GMcharge,                  "gmcharge"                  },
		{ clif_parse_FriendAddRequest,          "friendaddrequest"          },
		{ clif_parse_FriendAddReply,            "friendaddreply"            },
		{ clif_parse_FriendDeleteRequest,       "frienddeleterequest"       },
		{ clif_parse_clientsetting,             "clientsetting"             },
		{ clif_parse_BabyRequest,               "babyrequest"               },
		{ clif_parse_BabyReply,                 "babyreply"                 },
		{ clif_parse_PvPInfo,                   "pvpinfo"                   },
		{ clif_parse_RankingBlacksmith,         "rankingblacksmith"         },
		{ clif_parse_RankingAlchemist,          "rankingalchemist"          },
		{ clif_parse_RankingTaekwon,            "rankingtaekwon"            },
		{ clif_parse_RankingPk,                 "rankingpk"                 },
		{ clif_parse_HomMenu,                   "hommenu"                   },
		{ clif_parse_HomMercWalkMaster,         "hommercwalkmaster"         },
		{ clif_parse_HomMercWalkToXY,           "hommercwalktoxy"           },
		{ clif_parse_HomMercActionRequest,      "hommercactionrequest"      },
		{ clif_parse_ChangeHomName,             "changehomname"             },
		{ clif_parse_MailWinOpen,               "mailwinopen"               },
		{ clif_parse_ReadMail,                  "readmail"                  },
		{ clif_parse_MailGetAppend,             "mailgetappend"             },
		{ clif_parse_SendMail,                  "sendmail"                  },
		{ clif_parse_RefleshMailBox,            "refleshmailbox"            },
		{ clif_parse_SendMailSetAppend,         "sendmailsetappend"         },
		{ clif_parse_DeleteMail,                "deletemail"                },
		{ clif_parse_ReturnMail,                "returnmail"                },
		{ clif_parse_FeelSaveAck,               "feelsaveack"               },
		{ clif_parse_Making,                    "making"                    },
		{ clif_parse_HotkeySave,                "hotkeysave"                },
		{ clif_parse_Revive,                    "revive"                    },
		{ clif_parse_MercMenu,                  "mercmenu"                  },
		{ clif_parse_PartyEquipWindow,          "partyequipwindow"          },
		{ clif_parse_PartyEquipOpen,            "partyequipopen"            },
		{ clif_parse_HuntingList,               "huntinglist"               },
		{ clif_parse_BattleMessage,             "battlemessage"             },
		{ clif_parse_PartyChangeLeader,         "partychangeleader"         },
		{ clif_parse_GmFullstrip,               "gmfullstrip"               },
		{ clif_parse_PartyBookingRegisterReq,   "bookingregreq"             },
		{ clif_parse_PartyBookingSearchReq,     "bookingsearchreq"          },
		{ clif_parse_PartyBookingUpdateReq,     "bookingupdatereq"          },
		{ clif_parse_PartyBookingDeleteReq,     "bookingdelreq"             },
		{ clif_parse_SelectSkill,               "selectskill"               },
		{ NULL,                                 NULL                        },
	};

	memset(packet_db,0,sizeof(packet_db));

	if( (fp=fopen("db/packet_db.txt","r"))==NULL ){
		printf("can't read db/packet_db.txt\n");
		exit(1);
	}
	while(fgets(line,1020,fp)){
		ln++;
		if(line[0] == '\0' || line[0] == '\r' || line[0] == '\n')
			continue;
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		for(j=0,p=line;j<4 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(str[0]==NULL)
			continue;

		cmd=strtol(str[0],(char **)NULL,0);
		if(cmd<=0 || cmd>=MAX_PACKET_DB)
			continue;

		if(str[1]==NULL){
			printf("packet_db: 0x%x packet len error (line %d)\n", cmd, ln);
			exit(1);
		}
		packet_db[cmd].len = atoi(str[1]);

		if(str[2]==NULL){
			ln++;
			continue;
		}
		for(j=0;j<sizeof(clif_parse_func)/sizeof(clif_parse_func[0]);j++){
			if(clif_parse_func[j].name == NULL){
				printf("packet_db: 0x%x no func %s (line %d)\n", cmd, str[2], ln);
				exit(1);
			}
			if(strcmp(str[2],clif_parse_func[j].name) == 0){
				packet_db[cmd].func=clif_parse_func[j].func;
				break;
			}
		}
		if(str[3]==NULL){
			printf("packet_db: 0x%x packet error (line %d)\n", cmd, ln);
			exit(1);
		}
		for(j=0,p2=str[3];p2;j++){
			if(j >= sizeof(packet_db[0].pos)/sizeof(packet_db[0].pos[0])) {
				printf("packet_db: 0x%x pos overflow (line %d)\n", cmd, ln);
				exit(1);
			}
			str2[j]=p2;
			p2=strchr(p2,':');
			if(p2) *p2++=0;
			packet_db[cmd].pos[j]=atoi(str2[j]);
		}

		ln++;
		//if(packet_db[cmd].len > 2 && packet_db[cmd].pos[0] == 0)
		//	printf("packet_db:? %d 0x%x %d %s %p\n",ln,cmd,packet_db[cmd].len,str[2],packet_db[cmd].func);
	}
	fclose(fp);
	printf("read db/packet_db.txt done (count=%d)\n",ln);

	return;
}

/*==========================================
 * Web�`���b�g�V�X�e��
 *------------------------------------------
 */
#define MAX_CHAT_MESSAGE 15

static char webchat_message[MAX_CHAT_MESSAGE][256*4+1];
static int webchat_pos = 0;

void clif_webchat_message(const char* head,const char *mes1,const char *mes2)
{
	int  i;
	char *p;
	char temp[512];

	snprintf(temp, sizeof(temp), "%s %s : %s", head, mes1, mes2);
	p = httpd_quote_meta(temp);

	// �d�����b�Z�[�W������ꍇ����������
	for(i = 0; i < MAX_CHAT_MESSAGE; i++) {
		if(!strcmp(p, webchat_message[i])) {
			aFree(p);
			return;
		}
	}
	clif_onlymessage(temp,strlen(temp) + 1);

	strncpy(webchat_message[webchat_pos], p, sizeof(webchat_message[0]));
	webchat_message[webchat_pos][sizeof(webchat_message[0])-1] = '\0';
	if(++webchat_pos >= MAX_CHAT_MESSAGE)
		webchat_pos = 0;

	aFree(p);

	return;
}

void clif_webchat(struct httpd_session_data* sd,const char* url)
{
	char *name1 = httpd_get_value(sd,"name");
	char *name2 = httpd_binary_encode(name1);
	char *name3 = httpd_quote_meta(name1);
	char *mes   = httpd_get_value(sd,"mes");
	const char *err = NULL;
	char buf[8192];
	char *p = buf;

	do {
		int i, j;
		if(get_atcommand_level(AtCommand_MesWeb) > 0) {
			err = "Chat system disabled."; break;
		} else if(!name1[0]) {
			err = "Please input your name."; break;
		} else if(strlen(name1) > 24) {
			err = "Name is too long."; break;
		} else if(strlen(mes) > 100) {
			err = "Message is too long."; break;
		} else if(mes[0]) {
			if(httpd_get_method(sd) != HTTPD_METHOD_POST) {
				// POST�ȊO�̏������݂͂��f��
				err = "Illegal request."; break;
			}
			clif_webchat_message("[web]",name1,mes);
		}
		// print form
		// �R�O�b���ƂɍX�V
		p += sprintf(p,"<html><head>\n");
		p += sprintf(p,"<meta http-equiv=\"Refresh\" content=\"30;URL=/chat?name=%s\">\n",name2);
		p += sprintf(p,"<title>Auriga Chat</title></head>\n\n<body>\n");
		p += sprintf(p,"<form action=\"/chat\" method=\"POST\">\n");
		p += sprintf(p,"%s : <input type=\"text\" name=\"mes\" size=\"32\">\n",name3);
		p += sprintf(p,"<input type=\"hidden\" name=\"name\" value=\"%s\">\n",name3);
		p += sprintf(p,"<input type=\"submit\" value=\"Send!\">\n");
		p += sprintf(p,"</form>\n");

		i = webchat_pos;
		for(j = 0; j < MAX_CHAT_MESSAGE; j++) {
			if(webchat_message[i][0]) {
				p += sprintf(p,"%s<br>\n",webchat_message[i]);
			}
			if(++i >= MAX_CHAT_MESSAGE)
				i = 0;
		}
		p += sprintf(p,"</body></html>\n");
		httpd_send(sd,200,"text/html",(int)(p - buf),buf);
	} while(0);

	if(err != NULL) {
		httpd_send(sd,200,"text/html",(int)strlen(err),err);
	}
	aFree(name1);
	aFree(name2);
	aFree(name3);
	aFree(mes);

	return;
}

/*==========================================
 * �I��
 *------------------------------------------
 */
void do_final_clif(void)
{
	delete_session(map_fd);

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
void do_init_clif(void)
{
	int i;

	packetdb_readdb();
	set_defaultparse(clif_parse);
	set_sock_destruct(clif_disconnect);
	for(i=0;i<10;i++){
		if ((map_fd = make_listen_port(map_port, listen_ip)))
			break;
#ifdef WINDOWS
		Sleep(20);
#else
		sleep(20);
#endif
	}
	if(i==10){
		printf("can't bind game port\n");
		exit(1);
	}
	memset(webchat_message, 0, sizeof(webchat_message));

	add_timer_func_list(clif_waitclose);
	add_timer_func_list(clif_clearchar_delay_sub);

	return;
}
