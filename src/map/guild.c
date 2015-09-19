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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "timer.h"
#include "socket.h"
#include "nullpo.h"
#include "malloc.h"
#include "utils.h"
#include "grfio.h"

#include "guild.h"
#include "storage.h"
#include "battle.h"
#include "npc.h"
#include "pc.h"
#include "map.h"
#include "skill.h"
#include "mob.h"
#include "intif.h"
#include "clif.h"
#include "status.h"
#include "atcommand.h"
#include "unit.h"

static struct dbt *guild_db = NULL;
static struct dbt *guild_expcache_db = NULL;
static struct dbt *guild_infoevent_db = NULL;
static struct dbt *guild_castleinfoevent_db = NULL;

static struct guild_castle castle_db[MAX_GUILDCASTLE];

struct eventlist {
	char name[50];
	struct eventlist *next;
};

#define GUILD_PAYEXP_INVERVAL 10000	// �M���hEXP�L���b�V���̊Ԋu(�ő吶�����ԁA�~���b)
#define GUILD_SEND_XY_INVERVAL 1000	// ���W���M�̊Ԋu

// �M���h��EXP�L���b�V��
struct guild_expcache {
	int guild_id;
	int account_id;
	int char_id;
	int exp;
};

struct {
	int id;
	int max;
	struct{
		short id;
		short lv;
	} need[5];
} guild_skill_tree[MAX_GUILDSKILL];

/*==========================================
 * �M���h�X�L��db�̃A�N�Z�T�i���͒��ł��ő�p�j
 *------------------------------------------
 */
int guild_skill_get_inf(int id)
{
	return skill_get_inf(id);
}

int guild_skill_get_sp(int id,int lv)
{
	return skill_get_sp(id,lv);
}

int guild_skill_get_range(int id,int lv)
{
	return skill_get_range(id,lv);
}

int guild_skill_get_max(int id)
{
	return skill_get_max(id);
}

int guild_skill_get_lv(struct guild *g,int id)
{
	int idx = id-GUILD_SKILLID;

	if(g == NULL) return 0;

	if (idx < 0 || idx >= MAX_GUILDSKILL)
		return 0;

	return g->skill[id-GUILD_SKILLID].lv;
}

/*==========================================
 * �X�L����MaxLv��Ԃ�
 *------------------------------------------
 */
int guild_get_skilltree_max(int id)
{
	int idx = id - GUILD_SKILLID;

	if(idx < 0 || idx >= MAX_GUILDSKILL)
		return 0;

	return guild_skill_tree[idx].max;
}

/*==========================================
 * �M���h�X�L���c���[�`�F�b�N
 *------------------------------------------
 */
int guild_check_skill_require(struct guild *g,int id)
{
	int i,skillid;
	int idx = id - GUILD_SKILLID;

	if(g == NULL)
		return 0;

	if(idx < 0 || idx >= MAX_GUILDSKILL)
		return 0;
	if(guild_skill_tree[idx].id <= 0)
		return 0;
	if(guild_skill_tree[idx].max <= 0)
		return 0;

	for(i=0; i<5 && (skillid = guild_skill_tree[idx].need[i].id) > 0; i++)
	{
		if(guild_skill_tree[idx].need[i].lv > guild_checkskill(g,skillid))
			return 0;
	}
	return 1;
}

/*==========================================
 * �M���h�X�L�������邩�m�F
 *------------------------------------------
 */
int guild_checkskill(struct guild *g,int id)
{
	int idx = id-GUILD_SKILLID;

	if (idx < 0 || idx >= MAX_GUILDSKILL)
		return 0;
	return g->skill[idx].lv;
}

/*==========================================
 * �M���h�}�X�^�[��sd��Ԃ�
 *------------------------------------------
 */
struct map_session_data *guild_get_guildmaster_sd(struct guild *g)
{
	if(g == NULL)
		return NULL;

	return g->member[0].sd;
}

/*==========================================
 * �M���h����
 *------------------------------------------
 */
struct guild *guild_search(int guild_id)
{
	return (struct guild *)numdb_search(guild_db,guild_id);
}

/*==========================================
 * �M���h������
 *------------------------------------------
 */
static int guild_searchname_sub(void *key,void *data,va_list ap)
{
	struct guild *g, **dst;
	char *str;

	g   = (struct guild *)data;
	str = va_arg(ap,char *);
	dst = va_arg(ap,struct guild **);

	if(*dst == NULL) {
		if(strcmp(g->name,str) == 0)
			*dst = g;
	}

	return 0;
}

struct guild* guild_searchname(char *str)
{
	struct guild *g = NULL;

	numdb_foreach(guild_db,guild_searchname_sub,str,&g);

	return g;
}

/*==========================================
 * �M���h�錟��
 *------------------------------------------
 */
struct guild_castle *guild_castle_search(int gcid)
{
	if(gcid < 0 || gcid >= MAX_GUILDCASTLE)
		return NULL;

	return &castle_db[gcid];
}

/*==========================================
 * mapname�ɑΉ������A�W�g��gc��Ԃ�
 *------------------------------------------
 */
struct guild_castle *guild_mapname2gc(char *mapname)
{
	int i;
	struct guild_castle *gc=NULL;

	for(i=0;i<MAX_GUILDCASTLE;i++){
		gc=guild_castle_search(i);
		if(gc && strcmp(gc->map_name,mapname)==0)
			return gc;
	}

	return NULL;
}

/*==========================================
 * ���O�C�����̃M���h�����o�[�̂P�l��sd��Ԃ�
 *------------------------------------------
 */
struct map_session_data *guild_getavailablesd(struct guild *g)
{
	int i;

	nullpo_retr(NULL, g);

	for(i=0;i<g->max_member;i++) {
		if(g->member[i].sd!=NULL)
			return g->member[i].sd;
	}

	return NULL;
}

/*==========================================
 * �M���h�����o�[�̃C���f�b�N�X��Ԃ�
 *------------------------------------------
 */
int guild_getindex(struct guild *g,int account_id,int char_id)
{
	int i;

	if(g==NULL)
		return -1;

	for(i=0;i<g->max_member;i++) {
		if( g->member[i].account_id==account_id &&
		    g->member[i].char_id==char_id )
			return i;
	}

	return -1;
}

/*==========================================
 * �M���h�����o�[�̖�E��Ԃ�
 *------------------------------------------
 */
int guild_getposition(struct map_session_data *sd,struct guild *g)
{
	int i;

	nullpo_retr(-1, sd);

	if(g==NULL && (g=guild_search(sd->status.guild_id))==NULL)
		return -1;
	for(i=0;i<g->max_member;i++) {
		if (g->member[i].char_id == sd->status.char_id) // not need to check accoun_id. char_id is unique
			return g->member[i].position;
	}

	return -1;
}

/*==========================================
 * �����o�[���̍쐬
 *------------------------------------------
 */
static void guild_makemember(struct guild_member *m,struct map_session_data *sd)
{
	nullpo_retv(sd);

	memset(m,0,sizeof(struct guild_member));

	m->account_id = sd->status.account_id;
	m->char_id    = sd->status.char_id;
	m->hair       = sd->status.hair;
	m->hair_color = sd->status.hair_color;
	m->gender     = sd->sex;
	m->class_     = sd->status.class_;
	m->lv         = sd->status.base_level;
	m->online     = 1;
	m->position   = MAX_GUILDPOSITION-1;
	memcpy(m->name,sd->status.name,24);

	return;
}

/*==========================================
 * �M���h�����m�F
 *------------------------------------------
 */
static void guild_check_conflict(struct map_session_data *sd)
{
	nullpo_retv(sd);

	intif_guild_checkconflict(sd->status.guild_id,sd->status.account_id,sd->status.char_id);

	return;
}

/*==========================================
 * �M���h��EXP�L���b�V����inter�I�Ƀt���b�V��
 *------------------------------------------
 */
static int guild_payexp_timer_sub(void *key,void *data,va_list ap)
{
	int i;
	struct guild_expcache *c;
	struct guild *g;
	atn_bignumber tmp;

	nullpo_retr(0, c = (struct guild_expcache *)data);

	if( (g = guild_search(c->guild_id)) == NULL )
		return 0;
	if( (i = guild_getindex(g,c->account_id,c->char_id)) < 0 )
		return 0;

	tmp = (atn_bignumber)g->member[i].exp + c->exp;
	g->member[i].exp = (tmp > 0x7fffffff)? 0x7fffffff: (tmp < 0)? 0: (int)tmp;

	// �����o�[��Exp��MAX�l�ł���[�͍s��
	intif_guild_change_memberinfo(g->guild_id,c->account_id,c->char_id,GMI_EXP,&c->exp,sizeof(c->exp));

	aFree(c);

	return 0;
}

void guild_flush_expcache(void)
{
	numdb_clear(guild_expcache_db,guild_payexp_timer_sub);
	return;
}

static int guild_payexp_timer(int tid,unsigned int tick,int id,int data)
{
	guild_flush_expcache();
	return 0;
}

/*==========================================
 * �M���h��EXP��[
 *------------------------------------------
 */
atn_bignumber guild_payexp(struct map_session_data *sd,atn_bignumber exp)
{
	struct guild *g;
	struct guild_expcache *c;
	int pos,per;
	atn_bignumber exp2;

	nullpo_retr(0, sd);

	if(sd->status.guild_id==0 || (g=guild_search(sd->status.guild_id))==NULL )
		return 0;
	if( (pos=guild_getposition(sd,g)) < 0)
		return 0;
	if( (per=g->position[pos].exp_mode)<=0 )
		return 0;
	if( per>100 )
		per=100;
	if( (exp2=exp*per/100)<=0 )
		return 0;

	if(battle_config.guild_exp_rate!=100)
		exp2 = exp2*battle_config.guild_exp_rate/100;

	if( (c = (struct guild_expcache *)numdb_search(guild_expcache_db,sd->status.char_id)) == NULL ){
		c=(struct guild_expcache *)aCalloc(1,sizeof(struct guild_expcache));
		c->guild_id=sd->status.guild_id;
		c->account_id=sd->status.account_id;
		c->char_id=sd->status.char_id;
		numdb_insert(guild_expcache_db,c->char_id,c);
	}

	if(0x7fffffff - c->exp <= (int)exp2)
		c->exp = 0x7fffffff;
	else
		c->exp += (int)exp2;

	return exp2;
}

/*==========================================
 * �M���hEXP�擾
 *------------------------------------------
 */
void guild_getexp(struct map_session_data *sd, int exp)
{
	struct guild *g;
	struct guild_expcache *c;
	atn_bignumber tmp;

	nullpo_retv(sd);

	if (sd->status.guild_id == 0 || (g = guild_search(sd->status.guild_id)) == NULL)
		return;

	if ((c = (struct guild_expcache *)numdb_search(guild_expcache_db,sd->status.char_id)) == NULL) {
		c = (struct guild_expcache *)aCalloc(1,sizeof(struct guild_expcache));
		c->guild_id = sd->status.guild_id;
		c->account_id = sd->status.account_id;
		c->char_id = sd->status.char_id;
		numdb_insert(guild_expcache_db,c->char_id,c);
	}
	tmp = (atn_bignumber)c->exp + exp;
	c->exp = (tmp > 0x7fffffff) ? 0x7fffffff : (int)tmp;

	return;
}

/*==========================================
 * �쐬�v��
 *------------------------------------------
 */
void guild_create(struct map_session_data *sd, char *name)
{
	nullpo_retv(sd);

	if(sd->status.guild_id == 0 && sd->state.guild_creating == 0) {
		if(!battle_config.guild_emperium_check || pc_search_inventory(sd,714) >= 0) {
			struct guild_member m;
			// check no guild name! (hacker)
			if (name[0] == '\0') {
				clif_guild_created(sd, 2); // 0x167 <flag>.b: 0: Guild has been created., 1: You are already in a guild., 2: That Guild Name already exists., 3: You need the necessary item to create a Guild.
				return;
			}
			// force NULL in guild name (hacker or incorrect at command)
			// normal client doesn't send more than 23 char, and it always sends NULL
			name[23] = '\0';
			// check guild name with " -> use a " in a guild name disables /breakguild command (it doesn't work)
			if (strchr(name, '\"') != NULL) {
				clif_guild_created(sd, 2); // 0x167 <flag>.b: 0: Guild has been created., 1: You are already in a guild., 2: That Guild Name already exists., 3: You need the necessary item to create a Guild.
				return;
			}
			guild_makemember(&m, sd);
			m.position=0;
			sd->state.guild_creating = 1;
			intif_guild_create(name, &m);
		} else {
			clif_guild_created(sd,3);	// �G���y���E�����Ȃ�
		}
	} else {
		clif_guild_created(sd,1);	// ���łɏ������Ă���
	}
	return;
}

/*==========================================
 * �쐬��
 *------------------------------------------
 */
void guild_created(int account_id, int guild_id)
{
	struct map_session_data *sd=map_id2sd(account_id);

	if(sd==NULL)
		return;

	if(guild_id>0) {
		sd->status.guild_id    = guild_id;
		sd->state.guild_sended = 0;
		clif_guild_created(sd,0);
		if(battle_config.guild_emperium_check) {
			int idx = pc_search_inventory(sd,714);
			if(idx >= 0)
				pc_delitem(sd,idx,1,0);	// �G���y���E������
		}
	} else {
		clif_guild_created(sd,2);	// �쐬���s�i�����M���h���݁j
	}
	sd->state.guild_creating = 0;

	return;
}

/*==========================================
 * ���v��
 *------------------------------------------
 */
void guild_request_info(int guild_id)
{
	intif_guild_request_info(guild_id);

	return;
}

/*==========================================
 * �C�x���g�t�����v��
 *------------------------------------------
 */
void guild_npc_request_info(int guild_id,const char *event)
{
	struct eventlist *ev;

	if( guild_search(guild_id) ){
		if(event && *event)
			npc_event_do(event);
		return;
	}

	if(event==NULL || *event==0) {
		guild_request_info(guild_id);
		return;
	}

	ev=(struct eventlist *)aCalloc(1,sizeof(struct eventlist));
	strncpy(ev->name,event,sizeof(ev->name));
	ev->name[sizeof(ev->name)-1] = '\0';	// force \0 terminal
	ev->next=(struct eventlist *)numdb_insert(guild_infoevent_db,guild_id,ev);
	guild_request_info(guild_id);

	return;
}

/*==========================================
 * �����L�����̊m�F
 *------------------------------------------
 */
static void guild_check_member(struct guild *g)
{
	int i;
	struct map_session_data *sd;

	nullpo_retv(g);

	for(i=0;i<fd_max;i++){
		if(session[i] && (sd = (struct map_session_data *)session[i]->session_data) && sd->state.auth && !sd->state.waitingdisconnect){
			if(sd->status.guild_id==g->guild_id){
				int j,f=1;
				for(j=0;j<MAX_GUILD;j++){	// �f�[�^�����邩
					if( g->member[j].account_id==sd->status.account_id &&
					    g->member[j].char_id==sd->status.char_id )
					{
						struct guild_member *m=&g->member[j];
						m->hair=sd->status.hair;
						m->hair_color=sd->status.hair_color;
						m->gender=sd->sex;
						f=0;
					}
				}
				if(f){
					sd->status.guild_id    = 0;
					sd->state.guild_sended = 0;
					sd->guild_emblem_id    = 0;
					if(battle_config.error_log)
						printf("guild: check_member %d[%s] is not member\n",sd->status.account_id,sd->status.name);
				}
			}
		}
	}

	return;
}

/*==========================================
 * ���擾���s�i����ID�̃L������S���������ɂ���j
 *------------------------------------------
 */
void guild_recv_noinfo(int guild_id)
{
	int i;
	struct map_session_data *sd;

	for(i=0;i<fd_max;i++){
		if(session[i] && (sd = (struct map_session_data *)session[i]->session_data) && sd->state.auth && !sd->state.waitingdisconnect){
			if(sd->status.guild_id==guild_id)
				sd->status.guild_id=0;
		}
	}

	return;
}

/*==========================================
 * ���擾
 *------------------------------------------
 */
void guild_recv_info(struct guild *sg)
{
	struct map_session_data *sd;
	struct guild *g;
	struct guild before;
	struct eventlist *ev, *ev2;
	int i, bm = 0, m = 0;

	nullpo_retv(sg);

	if((g = (struct guild *)numdb_search(guild_db,sg->guild_id)) == NULL) {
		g = (struct guild *)aCalloc(1,sizeof(struct guild));
		numdb_insert(guild_db,sg->guild_id,g);
		before = *sg;

		// �ŏ��̃��[�h�Ȃ̂Ń��[�U�[�̃`�F�b�N���s��
		guild_check_member(sg);
	} else {
		before = *g;
	}

	memcpy(g,sg,sizeof(struct guild));

	for(i=0; i<g->max_member; i++) {	// sd�̐ݒ�Ɛl���̊m�F
		if(g->member[i].account_id > 0) {
			sd = map_id2sd(g->member[i].account_id);
			if( sd != NULL &&
			    sd->status.char_id == g->member[i].char_id &&
			    sd->status.guild_id == g->guild_id &&
			    !sd->state.waitingdisconnect )
				g->member[i].sd = sd;
			else
				g->member[i].sd = NULL;
			m++;
		} else {
			g->member[i].sd = NULL;
		}
		if(before.member[i].account_id > 0)
			bm++;
	}

	for(i=0; i<g->max_member; i++) {		// ���̑��M
		sd = g->member[i].sd;
		if(sd == NULL)
			continue;

		if(before.guild_lv != g->guild_lv || bm != m || before.max_member != g->max_member) {
			clif_guild_basicinfo(sd, g);	// ��{��񑗐M
			clif_guild_emblem(sd, g);	// �G���u�������M
		}

		if(bm != m) {	// �����o�[��񑗐M
			clif_guild_memberlist(g->member[i].sd, g);
		}

		if(before.skill_point != g->skill_point)
			clif_guild_skillinfo(sd, g);	// �X�L����񑗐M

		if(sd->state.guild_sended == 0) {	// �����M�Ȃ珊����������
			clif_guild_belonginfo(sd,g);
			clif_guild_notice(sd,g);
			sd->guild_emblem_id    = g->emblem_id;
			sd->state.guild_sended = 1;
		}
	}

	// �C�x���g�̔���
	if((ev = (struct eventlist *)numdb_search(guild_infoevent_db,sg->guild_id)) != NULL) {
		numdb_erase(guild_infoevent_db,sg->guild_id);
		while(ev) {
			npc_event_do(ev->name);
			ev2 = ev->next;
			aFree(ev);
			ev = ev2;
		}
	}

	return;
}

/*==========================================
 * �M���h�ւ̊��U
 *------------------------------------------
 */
void guild_invite(struct map_session_data *sd, int account_id)
{
	struct map_session_data *tsd;
	struct guild *g;
	int i, ps;

	nullpo_retv(sd);

	g = guild_search(sd->status.guild_id);
	if (g == NULL)
		return;

	tsd = map_id2sd(account_id);
	if (tsd == NULL)
		return;

	// player must have ability to invite...
	if ((ps = guild_getposition(sd, g)) < 0 || !(g->position[ps].mode & 0x0001))
		return;

	// player must be on same map... (how to use mouse menu otherwise?)
	if (sd->bl.m != tsd->bl.m)
		return;
	// and in visible area of member
	if (unit_distance(sd->bl.x,sd->bl.y,tsd->bl.x,tsd->bl.y) > AREA_SIZE)
		return;

	// GVG�ł͊��U�ł��Ȃ�
	if (!battle_config.allow_guild_invite_in_gvg) {
		if (agit_flag && map[sd->bl.m].flag.gvg)
			return;
	}

	if(tsd->state.waitingdisconnect) {	// ���肪�ؒf�҂�
		clif_guild_inviteack(sd,1);
		return;
	}
	if(!battle_config.invite_request_check) {
		if (tsd->party_invite > 0 || tsd->trade_partner || tsd->adopt_invite) {	// ���肪��������ǂ���
			clif_guild_inviteack(sd,1);
			return;
		}
	}
	if(tsd->status.guild_id > 0 || tsd->guild_invite > 0) {	// ����̏����m�F
		clif_guild_inviteack(sd,0);
		return;
	}

	// ����m�F
	for(i=0; i<g->max_member; i++) {
		if(g->member[i].account_id == 0)
			break;
	}
	if(i >= g->max_member) {
		clif_guild_inviteack(sd,3);
		return;
	}

	tsd->guild_invite         = sd->status.guild_id;
	tsd->guild_invite_account = sd->status.account_id;

	clif_guild_invite(tsd,g);

	return;
}

/*==========================================
 * �M���h���U�ւ̕ԓ�
 *------------------------------------------
 */
void guild_reply_invite(struct map_session_data *sd, int guild_id, unsigned char flag)
{
	struct map_session_data *tsd;

	nullpo_retv(sd);

	if(sd->guild_invite != guild_id)	// ���U�ƃM���hID���Ⴄ
		return;

	tsd = map_id2sd(sd->guild_invite_account);

	if(flag == 1) {	// ����
		struct guild_member m;
		struct guild *g;
		int i;

		// ����m�F
		if(tsd == NULL || (g = guild_search(tsd->status.guild_id)) == NULL) {
			sd->guild_invite         = 0;
			sd->guild_invite_account = 0;
			return;
		}
		for(i=0; i<g->max_member; i++) {
			if(g->member[i].account_id == 0)
				break;
		}
		if(i >= g->max_member) {
			sd->guild_invite         = 0;
			sd->guild_invite_account = 0;
			clif_guild_inviteack(tsd,3);
			return;
		}

		// inter�I�֒ǉ��v��
		guild_makemember(&m,sd);
		intif_guild_addmember(sd->guild_invite, &m);
	} else {		// ����
		sd->guild_invite         = 0;
		sd->guild_invite_account = 0;
		if(tsd)
			clif_guild_inviteack(tsd,1);
	}

	return;
}

/*==========================================
 * �M���h�����o���ǉ����ꂽ
 *------------------------------------------
 */
void guild_member_added(int guild_id, int account_id, int char_id, unsigned char flag)
{
	struct map_session_data *sd, *sd2;
	struct guild *g;

	if((g = guild_search(guild_id)) == NULL)
		return;

	sd = map_id2sd(account_id);
	if(sd == NULL || sd->guild_invite == 0) {
		if(flag == 0) {
			// �L�������ɓo�^�ł��Ȃ��������ߒE�ޗv�����o��
			char mes[40];
			if(battle_config.error_log)
				printf("guild: member added error %d is not online\n",account_id);

			strncpy(mes, msg_txt(176), 40); // **�o�^���s**
			mes[39] = '\0';			// force \0 terminal

			intif_guild_leave(guild_id, account_id,char_id, 0, mes);
		}
		return;
	}
	sd->guild_invite         = 0;
	sd->guild_invite_account = 0;

	sd2 = map_id2sd(sd->guild_invite_account);

	if(flag == 1) {	// ���s
		if(sd2)
			clif_guild_inviteack(sd2,3);
		return;
	}

	// ����
	sd->state.guild_sended = 0;
	sd->status.guild_id    = guild_id;

	if(sd2)
		clif_guild_inviteack(sd2,2);

	// �������������m�F
	guild_check_conflict(sd);

	return;
}

/*==========================================
 * �M���h�E�ޗv��
 *------------------------------------------
 */
void guild_leave(struct map_session_data *sd, int guild_id, int account_id, int char_id, const char *mes)
{
	struct guild *g;
	int i;

	nullpo_retv(sd);

	if (sd->status.account_id != account_id || sd->status.char_id != char_id || sd->status.guild_id != guild_id)
		return;

	g = guild_search(sd->status.guild_id);
	if (g == NULL)
		return;

	if (!battle_config.allow_guild_leave_in_gvg) {
		if (agit_flag && map[sd->bl.m].flag.gvg)
			return;
	}

	for(i=0;i<g->max_member;i++){	// �������Ă��邩
		if(g->member[i].char_id == sd->status.char_id) { // char_id is unique. don't check account_id
			intif_guild_leave(g->guild_id,sd->status.account_id,sd->status.char_id,0,mes);
			return;
		}
	}

	return;
}

/*==========================================
 * �M���h�Ǖ��v��
 *------------------------------------------
 */
void guild_explusion(struct map_session_data *sd, int guild_id, int account_id, int char_id, const char *mes)
{
	struct guild *g;
	int i,ps;

	nullpo_retv(sd);

	g = guild_search(sd->status.guild_id);
	if(g==NULL)
		return;

	if (sd->status.guild_id != guild_id)
		return;

	if( (ps=guild_getposition(sd,g))<0 || !(g->position[ps].mode&0x0010) )
		return;	// ������������

	for(i=0;i<g->max_member;i++){	// �������Ă��邩
		if (g->member[i].char_id == char_id) { // check only char_id (it's unique). account_id is not necessary
			// �V�[�Y���[�h�ł͔�����Ȃ�
			if(!battle_config.allow_guild_leave_in_gvg)
			{
				if(g->member[i].sd){
					if(agit_flag && map[g->member[i].sd->bl.m].flag.gvg)
						return;
				}
			}
			intif_guild_leave(g->guild_id,account_id,char_id,1,mes);
			return;
		}
	}

	return;
}

/*==========================================
 * �M���h�����o���E�ނ���
 *------------------------------------------
 */
void guild_member_leaved(int guild_id, int account_id, int char_id, unsigned char flag, const char *name, const char *mes)
{
	struct map_session_data *sd=map_id2sd(account_id);
	struct guild *g=guild_search(guild_id);

	if(g!=NULL){
		int i;
		for(i=0;i<g->max_member;i++) {
			if(g->member[i].account_id==account_id && g->member[i].char_id==char_id) {
				struct map_session_data *sd2=sd;
				if(sd2==NULL)
					sd2=guild_getavailablesd(g);
				if(sd2!=NULL){
					if(flag==0)
						clif_guild_leave(sd2,name,mes);
					else
						clif_guild_explusion(sd2,name,mes);
				}
				g->member[i].account_id=0;
				g->member[i].sd=NULL;
			}
		}
		// �����o�[���X�g��S���ɍĒʒm
		for(i=0;i<g->max_member;i++){
			if( g->member[i].sd!=NULL )
				clif_guild_memberlist(g->member[i].sd, g);
		}
	}
	if(sd!=NULL && sd->status.char_id==char_id && sd->status.guild_id==guild_id){
		if(sd->state.storage_flag == 2)
			storage_guild_storageclose(sd);
		sd->status.guild_id    = 0;
		sd->guild_emblem_id    = 0;
		sd->state.guild_sended = 0;
	}

	return;
}

/*==========================================
 * �ʒu�ʒm�N���A
 *------------------------------------------
 */
static void guild_send_xy_clear(struct guild *g)
{
	int i;

	nullpo_retv(g);

	for(i=0; i < g->max_member; i++) {
		struct map_session_data *sd;
		if((sd = g->member[i].sd) != NULL) {
			sd->guild_x = -1;
			sd->guild_y = -1;
		}
	}

	return;
}

/*==========================================
 * �ʒu�̒ʒm�^�C�}�[
 *------------------------------------------
 */
static int guild_send_xy_timer_sub(void *key,void *data,va_list ap)
{
	struct guild *g = (struct guild *)data;
	int i;

	nullpo_retr(0, g);

	for(i=0; i < g->max_member; i++) {
		struct map_session_data *sd;
		if((sd = g->member[i].sd) != NULL) {
			if(sd->guild_x != sd->bl.x || sd->guild_y != sd->bl.y) {
				clif_guild_xy(sd);
				sd->guild_x = sd->bl.x;
				sd->guild_y = sd->bl.y;
			}
		}
	}

	return 0;
}

static int guild_send_xy_timer(int tid,unsigned int tick,int id,int data)
{
	numdb_foreach(guild_db,guild_send_xy_timer_sub);

	return 0;
}

/*==========================================
 * �M���h�����o�̃I�����C�����/Lv�X�V���M
 *------------------------------------------
 */
void guild_send_memberinfoshort(struct map_session_data *sd, unsigned char online)
{
	struct guild *g;

	nullpo_retv(sd);

	if(sd->status.guild_id<=0)
		return;
	g=guild_search(sd->status.guild_id);
	if(g==NULL)
		return;

	intif_guild_memberinfoshort(g->guild_id,
		sd->status.account_id,sd->status.char_id,online,sd->status.base_level,sd->status.class_);

	if( !online ){	// ���O�A�E�g����Ȃ�sd���N���A���ďI��
		int i=guild_getindex(g,sd->status.account_id,sd->status.char_id);
		if(i>=0)
			g->member[i].sd=NULL;

		if(i==0)	// �}�X�^�[��������
		{
			struct map_session_data *member = NULL;
			for(i=1;i<g->max_member;i++){
				member = g->member[i].sd;
				if(member && member->under_the_influence_of_the_guild_skill > 0)
					status_calc_pc(sd,0);
			}
		}
		return;
	}

	if( sd->state.guild_sended != 0 )	// �M���h�������M�f�[�^�͑��M�ς�
		return;

	// �����m�F
	guild_check_conflict(sd);

	// �M���h�������M�f�[�^���M
	guild_check_member(g);	// �������m�F����
	if(sd->status.guild_id==g->guild_id){
		clif_guild_belonginfo(sd,g);
		clif_guild_notice(sd,g);
		sd->state.guild_sended = 1;
		sd->guild_emblem_id    = g->emblem_id;
	}

	/*
	//guild_recv_memberinfoshort�ōX�V����邩�炢��Ȃ��H
	if(g)
	{
		struct map_session_data *gmsd = g->member[0].sd;
		if(gmsd && battle_config.guild_hunting_skill_available
			&& (pc_checkskill(gmsd,GD_LEADERSHIP) > 0 || pc_checkskill(gmsd,GD_SOULCOLD) > 0
					||pc_checkskill(gmsd,GD_GLORYWOUNDS) > 0 ||pc_checkskill(gmsd,GD_HAWKEYES) > 0))
		{
			if(sd == gmsd && battle_config.allow_me_guild_skill)
				status_calc_pc(gmsd,0);
			pc_check_guild_skill_effective_range(gmsd);
		}
	}
	*/

	return;
}

/*==========================================
 * �M���h�����o�̃I�����C�����/Lv�X�V�ʒm
 *------------------------------------------
 */
void guild_recv_memberinfoshort(int guild_id, int account_id, int char_id, unsigned char online, int lv, int class_)
{
	int i, alv = 0, c = 0, idx = -1, om = 0;
	unsigned char oldonline = !online;
	struct guild *g=guild_search(guild_id);
	struct map_session_data *sd;

	if(g==NULL)
		return;

	for(i=0;i<g->max_member;i++){
		struct guild_member *m=&g->member[i];
		if(!m) continue;
		if(m->account_id==account_id && m->char_id==char_id ){
			oldonline = m->online;
			m->online = online;
			m->lv     = lv;
			m->class_ = class_;
			idx       = i;
		}
		if(m->account_id>0){
			alv+=m->lv;
			c++;
		}
		if(m->online)
			om++;
	}
	if(idx == -1 || c == 0) {
		// �M���h�̃����o�[�O�Ȃ̂ŒǕ���������
		sd = map_id2sd(account_id);
		if(sd && sd->status.char_id == char_id) {
			sd->status.guild_id    = 0;
			sd->guild_emblem_id    = 0;
			sd->state.guild_sended = 0;
		}
		if(battle_config.error_log)
			printf("guild: not found member %d,%d on %d[%s]\n",account_id,char_id,guild_id,g->name);
		return;
	}
	g->average_lv=alv/c;
	g->connect_member=om;

	for(i=0;i<g->max_member;i++){	// sd�Đݒ�
		sd = map_id2sd(g->member[i].account_id);
		g->member[i].sd=(sd!=NULL &&
			sd->status.char_id==g->member[i].char_id &&
			sd->status.guild_id==guild_id &&
			!sd->state.waitingdisconnect)?sd:NULL;
	}

	if(oldonline!=online)	// �I�����C����Ԃ��ς�����̂Œʒm
		clif_guild_memberlogin_notice(g,idx,online);

	// �}�b�v�ړ�,�ڑ��ȂǂŃM���h�X�L���̗L���`�F�b�N���X�e�[�^�X�X�V
	if(g->member[0].sd)
		pc_check_guild_skill_effective_range(g->member[0].sd);
	// �����ɃN���C�A���g�ɑ��M�������K�v

	guild_send_xy_clear(g);	// ���W�Ēʒm�v��

	return;
}

/*==========================================
 * �M���h��b���M
 *------------------------------------------
 */
void guild_send_message(struct map_session_data *sd, char *mes, int len)
{
	nullpo_retv(sd);

	if(sd->status.guild_id==0)
		return;
	intif_guild_message(sd->status.guild_id,sd->status.account_id,mes,len);

	return;
}

/*==========================================
 * �M���h��b��M
 *------------------------------------------
 */
void guild_recv_message(int guild_id, int account_id, char *mes, int len)
{
	struct guild *g = guild_search(guild_id);

	if(g)
		clif_guild_message(g,mes,len);

	return;
}

/*==========================================
 * �M���h�����o�̖�E�ύX
 *------------------------------------------
 */
void guild_change_memberposition(int guild_id, int account_id, int char_id, int idx)
{
	if(idx < 0 || idx >= MAX_GUILDPOSITION)
		return;

	intif_guild_change_memberinfo(guild_id, account_id, char_id, GMI_POSITION, &idx, sizeof(idx));

	return;
}

/*==========================================
 * �M���h�����o�̖�E�ύX�ʒm
 *------------------------------------------
 */
void guild_memberposition_changed(struct guild *g, int idx, int pos)
{
	nullpo_retv(g);

	g->member[idx].position=pos;
	clif_guild_memberpositionchanged(g,idx);

	return;
}

/*==========================================
 * �M���h��E�ύX
 *------------------------------------------
 */
void guild_change_position(int guild_id, int idx, int mode, int exp_mode, const char *name)
{
	struct guild_position p;

	if (idx < 0 || idx >= MAX_GUILDPOSITION)
		return;

	memset(&p, 0, sizeof(struct guild_position));

	strncpy(p.name, name, 24);
	p.name[23] = '\0';	// force \0 terminal
	p.mode = mode;
	if (exp_mode < 0)
		p.exp_mode = 0;
	else if (exp_mode > battle_config.guild_exp_limit)
		p.exp_mode = battle_config.guild_exp_limit;
	else
		p.exp_mode = exp_mode;

	intif_guild_position(guild_id, idx, &p);

	return;
}

/*==========================================
 * �M���h��E�ύX�ʒm
 *------------------------------------------
 */
void guild_position_changed(int guild_id, int idx, struct guild_position *p)
{
	struct guild *g = guild_search(guild_id);

	if(g == NULL)
		return;
	if(idx < 0 || idx >= MAX_GUILDPOSITION)
		return;

	memcpy(&g->position[idx],p,sizeof(struct guild_position));
	clif_guild_positionchanged(g,idx);

	return;
}

/*==========================================
 * �M���h���m�ύX
 *------------------------------------------
 */
void guild_change_notice(int guild_id, const char *mes1, const char *mes2)
{
	intif_guild_notice(guild_id, mes1, mes2);

	return;
}

/*==========================================
 * �M���h���m�ύX�ʒm
 *------------------------------------------
 */
void guild_notice_changed(int guild_id, const char *mes1, const char *mes2)
{
	int i;
	struct map_session_data *sd;
	struct guild *g=guild_search(guild_id);

	if(g==NULL)
		return;

	memcpy(g->mes1,mes1,60);
	memcpy(g->mes2,mes2,120);
	g->mes1[59]  = '\0';
	g->mes2[119] = '\0';

	for(i=0;i<g->max_member;i++){
		if((sd=g->member[i].sd)!=NULL)
			clif_guild_notice(sd,g);
	}

	return;
}

/*==========================================
 * �M���h�G���u�����ύX
 *------------------------------------------
 */
void guild_change_emblem(int guild_id, int zipbitmap_len, const char *data)
{
	char dest_bitmap[4100]; // max possible (16/24 bits): 4086 (windows)-> (header1)14 + (header2)40 + (576 colors)2304 + (bitmap:24x24)1728 (no compression with palette)
	unsigned long dest_bitmap_len;
	unsigned int ncol;

	// length of zipbitmap (client doesn't send bmp structure, but a zipped BMP)
	if (zipbitmap_len < 0 || zipbitmap_len > 2000)
		return;

	// analyse of bmp
	dest_bitmap_len = sizeof(dest_bitmap);
	if (decode_zip(dest_bitmap, &dest_bitmap_len, data, zipbitmap_len) != 0) { // Z_OK = 0
		return;
	}
	if (dest_bitmap_len < 14 + 40) { // strict minimum -> windows: header1(14) header2(40), os/2 v1 header1(14) header2(12) 256colors(768)
		return;
	}

	/*Structure BITMAPFILEHEADER (14 bytes)
	bfType       Word  (2 bytes)  0 Magic identifier (should be 'BM'=0x4D 0x4E)
	bfSize       Dword (4 bytes)  2 File size in bytes
	bfReserved1  Word  (2 bytes)  6 always 0
	bfReserved2  Word  (2 bytes)  8 always 0
	bfOffBits    Dword (4 bytes) 10 Offset to image data, bytes*/
	if(strncmp(dest_bitmap, "BM", 2) != 0 ||
	   RBUFL(dest_bitmap, 2) != dest_bitmap_len ||
	   RBUFW(dest_bitmap, 6) != 0 ||
	   RBUFW(dest_bitmap, 8) != 0) {
		// check of Offset is done with colors.
		return;
	}

	// Structure BITMAPINFOHEADER (40 bytes: Windows 3.1x, 95, NT, ..., 12 bytes: OS/2 1.x, 64 bytes: OS/2 2.x, ?? bytes: OS/2 3.x)
	switch(RBUFL(dest_bitmap, 14)) {
	case 40: // windows version
		/*biSize           DWord (4 bytes) 14 Header size in bytes (must be: 40)
			biWidth          Dword (4 bytes) 18 Width of image
			biHeight         Dword (4 bytes) 22 Height of image
			biPlanes         Word  (2 bytes) 26 Number of colour planes (always 1)
			biBitCount       Word  (2 bytes) 28 Bits per pixel (1, 4, 8, 16, 24 or 32)
			biCompression    Dword (4 bytes) 30 Compression type (0 - no compression, 1 - 8 bit run length encoding, 2 - 4 bit run length encoding, 3 - RGB bitmap with mask)
			biSizeImage      Dword (4 bytes) 34 Image size in bytes (usefull for compression)
			biXpelsPerMeter  Dword (4 bytes) 38 Pixels per meter
			biYpelsPerMeter  Dword (4 bytes) 42 Pixels per meter
			biClrUsed        Dword (4 bytes) 46 Number of used colours (number of colors in palette)
			biClrImportant   Dword (4 bytes) 50 Number of important colours (0: all)*/
		if (RBUFL(dest_bitmap, 18) != 24 || // ragnarok condition
		    RBUFL(dest_bitmap, 22) != 24 || // ragnarok condition
		    RBUFW(dest_bitmap, 26) != 1 ||
		    // number of colors is checked after
		    (RBUFL(dest_bitmap, 30) != 0 && RBUFL(dest_bitmap, 30) != 1 && RBUFL(dest_bitmap, 30) != 2 && RBUFL(dest_bitmap, 30) != 3)) {
			return;
		}
		/* color structure (8 bits, always exist)
		Windows RGBQUAD color map entry
		0       rgbBlue     (1 bytes)  Blue value for color map entry.
		1       rgbGreen    (1 bytes)  Green value for color map entry.
		2       rgbRed      (1 bytes)  Red value for color map entry.
		3       rgbReserved (1 bytes)  Zero*/
		if (RBUFL(dest_bitmap, 28) == 8) { // 256 colors (ragnarok condition)
			ncol = (RBUFL(dest_bitmap, 10) - 54) / 4; // compute number of colors recorded
			// check offset of bitmap and number of colors
			if (ncol > 256 || // 8 bits colored bitmap, used colors must be at maximum 256
			    (RBUFL(dest_bitmap, 46) == 0 && ncol != 256) || // biClrUsed not used
			    (RBUFL(dest_bitmap, 46) != 0 && ncol != RBUFL(dest_bitmap, 46))) { // biClrUsed used
				return;
			}
			// check size of file (when no compression)
			if (RBUFL(dest_bitmap, 10) < 14 + 40 + ncol * 4 || // check minimum position of bitmap data
			    (RBUFL(dest_bitmap, 30) == 0 && (dest_bitmap_len < 14 + 40 + ncol * 4 + 576 || // no compression (check minimum)
			                                     dest_bitmap_len - RBUFL(dest_bitmap, 10) < 576)) ||
			    (RBUFL(dest_bitmap, 30) != 0 && dest_bitmap_len <= 14 + 40 + ncol * 4)) { // with compression (check minimum)
				return;
			}
		// 16-24 bits colors (Additional ragnarok conditions)
		} else if (((battle_config.guild_emblem_colors & 1) == 1 && RBUFL(dest_bitmap, 28) == 16) ||
		           ((battle_config.guild_emblem_colors & 2) == 2 && RBUFL(dest_bitmap, 28) == 24)) {
			ncol = RBUFL(dest_bitmap, 46); // compute number of colors recorded (could be 0 if not important colors)
			// check offset of bitmap and number of colors
			if (ncol > 576) { // can not have more colors than number of used pixels
				return;
			}
			// check size of file (when no compression)
			if (RBUFL(dest_bitmap, 10) < 14 + 40 + ncol * 4 || // check minimum position of bitmap data
			    (RBUFL(dest_bitmap, 30) == 0 && (dest_bitmap_len < 14 + 40 + ncol * 4 + 576 * 3 || // no compression (check minimum)
			                                     dest_bitmap_len - RBUFL(dest_bitmap, 10) < 576 * 3)) ||
			    (RBUFL(dest_bitmap, 30) != 0 && dest_bitmap_len <= 14 + 40 + ncol * 4)) { // with compression (check minimum)
				return;
			}
		} else {	// non supported number of colors
			return;
		}
		break;

	case 12: // os/2 v1.x version
		/*biSize           DWord (4 bytes) 14 Header size in bytes (must be: 12)
			biWidth          Word  (2 bytes) 18 Width of image
			biHeight         Word  (2 bytes) 20 Height of image
			biPlanes         Word  (2 bytes) 22 Number of colour planes (always 1)
			biBitCount       Word  (2 bytes) 24 Bits per pixel (1, 4, 8, 24) -> color palette size: 2, 16, 256, 0
			no compression*/
		if (RBUFW(dest_bitmap, 18) != 24 || // ragnarok condition
		    RBUFW(dest_bitmap, 20) != 24 || // ragnarok condition
		    RBUFW(dest_bitmap, 22) != 1
		    // number of colors is checked after
		    ) {
			return;
		}
		/* color structure (8 bits, always exist)
		OS/2 RGBTRIPLE color map entry
		0       rgbtBlue  (1 bytes)  Blue value for color map entry.
		1       rgbtGreen (1 bytes)  Green value for color map entry.
		2       rgbtRed   (1 bytes)  Red value for color map entry.*/
		if (RBUFW(dest_bitmap, 24) == 8) { // 256 colors (ragnarok condition)
			ncol = (RBUFL(dest_bitmap, 10) - 26) / 3; // compute number of colors recorded
			// check offset of bitmap
			if (ncol != 256) { // bmp start after the 2 headers and color structure
				return;
			}
			// check size of file
			if (dest_bitmap_len != 14 + 12 + ncol * 3 + 576) {
				return;
			}
		// 24 bits colors (Additional ragnarok condition)
		} else if ((battle_config.guild_emblem_colors & 2) == 2 && RBUFW(dest_bitmap, 24) == 24) {
			// check size of file
			if (dest_bitmap_len != 14 + 12 + 1728) { // bmp start after the 2 headers
				return;
			}
		} else {	// non supported number of colors
			return;
		}
		break;

	default: // other -> not supported
		/* note: Ragnarok run under windows, client don't support OS/2 1.x, 2.x or 3.x versions. OS/2 1.x version is here for example */
		return;
	}

	intif_guild_emblem(guild_id, (unsigned short)zipbitmap_len, data);

	return;
}

/*==========================================
 * �M���h�G���u�����ύX�ʒm
 *------------------------------------------
 */
void guild_emblem_changed(int len, int guild_id, int emblem_id, const char *data)
{
	int i;
	struct map_session_data *sd;
	struct guild *g=guild_search(guild_id);

	if(g==NULL)
		return;

	memcpy(g->emblem_data,data,len);
	g->emblem_len=len;
	g->emblem_id=emblem_id;

	for(i=0;i<g->max_member;i++){
		if((sd=g->member[i].sd)!=NULL){
			sd->guild_emblem_id=emblem_id;
			clif_guild_belonginfo(sd,g);
			clif_guild_emblem(sd,g);
		}
	}

	return;
}

/*==========================================
 * �X�L���|�C���g����U��
 *------------------------------------------
 */
void guild_skillup(struct map_session_data *sd, int skill_num, int flag)
{
	struct guild *g;
	int idx;

	nullpo_retv(sd);

	if(sd->status.guild_id==0 || (g=guild_search(sd->status.guild_id))==NULL)
		return;
	if(strcmp(sd->status.name,g->master))
		return;

	idx = skill_num - GUILD_SKILLID;
	if (idx < 0 || idx >= MAX_GUILDSKILL)
		return;

	if( (g->skill_point > 0 || flag&1) &&
	    g->skill[idx].id > 0 &&
	    guild_check_skill_require(g,skill_num) &&
	    g->skill[idx].lv < guild_skill_tree[idx].max )
	{
		// ���X�V
		intif_guild_skillup(g->guild_id,skill_num,sd->status.account_id,flag);
		clif_guild_skillinfo(sd, g);
	}

	return;
}

/*==========================================
 * �X�L���|�C���g����U��ʒm
 *------------------------------------------
 */
void guild_skillupack(int guild_id, int skill_num, int account_id)
{
	struct map_session_data *sd=map_id2sd(account_id);
	struct guild *g=guild_search(guild_id);
	int i;

	if(g==NULL)
		return;

	if(sd!=NULL)
		clif_guild_skillup(sd,skill_num,g->skill[skill_num-GUILD_SKILLID].lv);

	// �S���ɒʒm
	for(i=0;i<g->max_member;i++) {
		if((sd=g->member[i].sd)!=NULL)
			clif_guild_skillinfo(sd, g);
	}

	return;
}

/*==========================================
 * �M���h�������擾
 *------------------------------------------
 */
static int guild_get_alliance_count(struct guild *g,int flag)
{
	int i,c;

	nullpo_retr(0, g);

	for(i=c=0;i<MAX_GUILDALLIANCE;i++){
		if( g->alliance[i].guild_id>0 && g->alliance[i].opposition==flag )
			c++;
	}

	return c;
}

/*==========================================
 * �����֌W���ǂ����`�F�b�N
 * �����Ȃ�1�A����ȊO��0
 *------------------------------------------
 */
int guild_check_alliance(int guild_id1, int guild_id2, int flag)
{
	struct guild *g;
	int i;

	g = guild_search(guild_id1);
	if (g == NULL)
		return 0;

	for (i=0; i<MAX_GUILDALLIANCE; i++)
	{
		if (g->alliance[i].guild_id == guild_id2 && g->alliance[i].opposition == flag)
			return 1;
	}

	return 0;
}

/*==========================================
 * �M���h�����v��
 *------------------------------------------
 */
void guild_reqalliance(struct map_session_data *sd, int account_id)
{
	struct map_session_data *tsd;
	struct guild *g[2];
	int i;

	nullpo_retv(sd);

	tsd = map_id2sd(account_id);
	if(tsd == NULL || tsd->status.guild_id <= 0)
		return;

	// �����M���h�ɂ͂ł��Ȃ�
	if(sd->status.guild_id == tsd->status.guild_id)
		return;

	// player must be on same map... (how to use mouse menu otherwise?)
	if (sd->bl.m != tsd->bl.m)
		return;
	// and in visible area of member
	if (unit_distance(sd->bl.x,sd->bl.y,tsd->bl.x,tsd->bl.y) > AREA_SIZE)
		return;

	// �U��풆�ł͓����ł��Ȃ�
	if (!battle_config.allow_guild_alliance_in_agit) {
		if (agit_flag)
			return;
	}

	g[0] = guild_search(sd->status.guild_id);
	g[1] = guild_search(tsd->status.guild_id);

	if(g[0] == NULL || g[1] == NULL)
		return;

	if(strcmp(sd->status.name, g[0]->master))	// �}�X�^�[���ǂ����m�F
		return;

	if(guild_get_alliance_count(g[0],0) > 3)	// �������m�F
		clif_guild_allianceack(sd,4);
	if(guild_get_alliance_count(g[1],0) > 3)
		clif_guild_allianceack(sd,3);

	if(tsd->guild_alliance > 0) {	// ���肪�����v����Ԃ��ǂ����m�F
		clif_guild_allianceack(sd,1);
		return;
	}

	for(i=0; i<MAX_GUILDALLIANCE; i++) {	// ���łɓ�����Ԃ��m�F
		if(g[0]->alliance[i].guild_id == tsd->status.guild_id && g[0]->alliance[i].opposition == 0) {
			clif_guild_allianceack(sd,0);
			return;
		}
	}

	tsd->guild_alliance         = sd->status.guild_id;
	tsd->guild_alliance_account = sd->status.account_id;

	clif_guild_reqalliance(tsd,sd->status.account_id,g[0]->name);

	return;
}

/*==========================================
 * �M���h�������U�ւ̕ԓ�
 *------------------------------------------
 */
void guild_reply_reqalliance(struct map_session_data *sd, int account_id, int flag)
{
	struct map_session_data *tsd;

	nullpo_retv(sd);

	if((tsd = map_id2sd(account_id)) == NULL)
		return;
	if(sd->guild_alliance != tsd->status.guild_id)	// ���U�ƃM���hID���Ⴄ
		return;

	if(flag == 1) {
		// ����
		int i;
		struct guild *g;

		// �������Ċm�F
		if((g = guild_search(sd->status.guild_id)) == NULL || guild_get_alliance_count(g,0) > 3) {
			clif_guild_allianceack(sd,4);
			clif_guild_allianceack(tsd,3);
			return;
		}
		if((g = guild_search(tsd->status.guild_id)) == NULL || guild_get_alliance_count(g,0) > 3) {
			clif_guild_allianceack(sd,3);
			clif_guild_allianceack(tsd,4);
			return;
		}

		// �G�Ί֌W�Ȃ�G�΂��~�߂�
		if((g = guild_search(sd->status.guild_id)) == NULL)
			return;
		for(i=0; i<MAX_GUILDALLIANCE; i++) {
			if(g->alliance[i].guild_id == tsd->status.guild_id && g->alliance[i].opposition == 1) {
				intif_guild_alliance( sd->status.guild_id,tsd->status.guild_id,
					sd->status.account_id,tsd->status.account_id,9 );
			}
		}
		if((g = guild_search(tsd->status.guild_id)) == NULL)
			return;
		for(i=0; i<MAX_GUILDALLIANCE; i++) {
			if(g->alliance[i].guild_id == sd->status.guild_id && g->alliance[i].opposition == 1) {
				intif_guild_alliance( tsd->status.guild_id,sd->status.guild_id,
					tsd->status.account_id,sd->status.account_id,9 );
			}
		}

		// inter�I�֓����v��
		intif_guild_alliance( sd->status.guild_id,tsd->status.guild_id,
			sd->status.account_id,tsd->status.account_id,0 );
	} else {
		// ����
		sd->guild_alliance         = 0;
		sd->guild_alliance_account = 0;
		if(tsd)
			clif_guild_allianceack(tsd,3);
	}

	return;
}

/*==========================================
 * �M���h�֌W����
 *------------------------------------------
 */
void guild_delalliance(struct map_session_data *sd, int guild_id, int flag)
{
	nullpo_retv(sd);

	// �U��풆�ł͉����ł��Ȃ�
	if(!battle_config.allow_guild_alliance_in_agit) {
		if(agit_flag)
			return;
	}

	intif_guild_alliance(sd->status.guild_id,guild_id,sd->status.account_id,0,flag|8);

	return;
}

/*==========================================
 * �M���h�G��
 *------------------------------------------
 */
void guild_opposition(struct map_session_data *sd, int account_id)
{
	struct map_session_data *tsd;
	struct guild *g;
	int i;

	nullpo_retv(sd);

	tsd = map_id2sd(account_id);
	if(tsd == NULL || tsd->status.guild_id <= 0)
		return;

	// �����M���h�ɂ͂ł��Ȃ�
	if(sd->status.guild_id == tsd->status.guild_id)
		return;

	if(sd->bl.m != tsd->bl.m)
		return;
	if(unit_distance(sd->bl.x,sd->bl.y,tsd->bl.x,tsd->bl.y) > AREA_SIZE)
		return;

	// �U��풆�ł͓G�΂ł��Ȃ�
	if(!battle_config.allow_guild_alliance_in_agit) {
		if(agit_flag)
			return;
	}

	if((g = guild_search(sd->status.guild_id)) == NULL)
		return;
	if(strcmp(sd->status.name, g->master))	// �}�X�^�[���ǂ����m�F
		return;

	if(guild_get_alliance_count(g,1) > 3)	// �G�ΐ��m�F
		clif_guild_oppositionack(sd,1);

	for(i=0; i<MAX_GUILDALLIANCE; i++) {	// ���łɊ֌W�������Ă��邩�m�F
		if(g->alliance[i].guild_id == tsd->status.guild_id) {
			if(g->alliance[i].opposition == 1) {	// ���łɓG��
				clif_guild_oppositionack(sd,2);
				return;
			}
			// �����j��
			intif_guild_alliance( sd->status.guild_id,tsd->status.guild_id,
				sd->status.account_id,tsd->status.account_id,8 );
		}
	}

	// inter�I�ɓG�Ηv��
	intif_guild_alliance( sd->status.guild_id,tsd->status.guild_id,
			sd->status.account_id,tsd->status.account_id,1 );

	return;
}

/*==========================================
 * �M���h����/�G�Βʒm
 *------------------------------------------
 */
void guild_allianceack(int guild_id1, int guild_id2, int account_id1, int account_id2,
	unsigned char flag, const char *name1, const char *name2)
{
	struct guild *g[2];
	int guild_id[2];
	const char *guild_name[2];
	struct map_session_data *sd[2];
	int j,i;

	guild_id[0]   = guild_id1;
	guild_id[1]   = guild_id2;
	guild_name[0] = name1;
	guild_name[1] = name2;
	sd[0]         = map_id2sd(account_id1);
	sd[1]         = map_id2sd(account_id2);

	g[0] = guild_search(guild_id1);
	g[1] = guild_search(guild_id2);

	if(sd[0]!=NULL && (flag&0x0f)==0){
		sd[0]->guild_alliance=0;
		sd[0]->guild_alliance_account=0;
	}

	if(flag&0x70){	// ���s
		for(i=0;i<2-(flag&1);i++)
			if( sd[i]!=NULL )
				clif_guild_allianceack(sd[i],((flag>>4)==i+1)?3:4);
		return;
	}

	if(!(flag&0x08)) {	// �֌W�ǉ�
		for(i=0;i<2-(flag&1);i++) {
			if(g[i]!=NULL) {
				for(j=0;j<MAX_GUILDALLIANCE;j++) {
					if(g[i]->alliance[j].guild_id==0){
						g[i]->alliance[j].guild_id=guild_id[1-i];
						memcpy(g[i]->alliance[j].name,guild_name[1-i],24);
						g[i]->alliance[j].opposition=flag&1;
						break;
					}
				}
			}
		}
	} else {		// �֌W����
		for(i=0;i<2-(flag&1);i++) {
			if(g[i]!=NULL) {
				for(j=0;j<MAX_GUILDALLIANCE;j++) {
					if(g[i]->alliance[j].guild_id==guild_id[1-i] &&
					   g[i]->alliance[j].opposition==(flag&1)){
						g[i]->alliance[j].guild_id=0;
						break;
					}
				}
			}
			if( sd[i]!=NULL )	// �����ʒm
				clif_guild_delalliance(sd[i],guild_id[1-i],(flag&1));
		}
	}

	if((flag&0x0f)==0){			// �����ʒm
		if( sd[1]!=NULL )
			clif_guild_allianceack(sd[1],2);
	}else if((flag&0x0f)==1){	// �G�Βʒm
		if( sd[0]!=NULL )
			clif_guild_oppositionack(sd[0],0);
	}


	for(i=0;i<2-(flag&1);i++) {	// ����/�G�΃��X�g�̍đ��M
		struct map_session_data *tmpsd;
		if(g[i]!=NULL) {
			for(j=0;j<g[i]->max_member;j++) {
				if((tmpsd=g[i]->member[j].sd)!=NULL)
					clif_guild_allianceinfo(tmpsd, g[i]);
			}
		}
	}

	return;
}

/*==========================================
 * �M���h���U�ʒm
 *------------------------------------------
 */
static int guild_broken_sub(void *key,void *data,va_list ap)
{
	struct guild *g=(struct guild *)data;
	int guild_id=va_arg(ap,int);
	int i,j;
	struct map_session_data *sd=NULL;

	nullpo_retr(0,g);

	for(i=0;i<MAX_GUILDALLIANCE;i++){	// �֌W��j��
		if(g->alliance[i].guild_id==guild_id){
			for(j=0;j<g->max_member;j++) {
				if( (sd=g->member[j].sd)!=NULL )
					clif_guild_delalliance(sd,guild_id,g->alliance[i].opposition);
			}
			g->alliance[i].guild_id=0;
		}
	}

	return 0;
}

void guild_broken(int guild_id, unsigned char flag)
{
	struct guild *g;
	struct guild_castle *gc;
	struct map_session_data *sd;
	int i;

	if(flag != 0)
		return;
	if((g = guild_search(guild_id)) == NULL)
		return;

	// ���L�Ԃ̔j��
	for(i=0; i<MAX_GUILDCASTLE; i++) {
		if((gc = guild_castle_search(i)) != NULL) {
			if(gc->guild_id == guild_id) {
				char name[50];
				memcpy(name,gc->castle_event,24);
				npc_event_do(strcat(name,"::OnGuildBreak"));
			}
		}
	}

	for(i=0; i<g->max_member; i++) {	// �M���h���U��ʒm
		if((sd = g->member[i].sd) != NULL) {
			if(sd->state.storage_flag == 2) {
				// �M���h�q�ɂ��J���Ă���Ȃ����
				sd->state.storage_flag = 0;
				clif_storageclose(sd);
			}
			sd->status.guild_id    = 0;
			sd->state.guild_sended = 0;
			clif_guild_broken(g->member[i].sd,0);
		}
	}

	numdb_foreach(guild_db,guild_broken_sub,guild_id);
	numdb_erase(guild_db,guild_id);
	storage_guild_delete(guild_id);
	aFree(g);

	return;
}

/*==========================================
 * �M���h���U
 *------------------------------------------
 */
void guild_break(struct map_session_data *sd, char *name)
{
	struct guild *g;
	int i;

	nullpo_retv(sd);

	if( (g=guild_search(sd->status.guild_id))==NULL )
		return;
	if(strcmp(g->name,name)!=0)
		return;
	if(strcmp(sd->status.name,g->master)!=0)
		return;
	for(i=0;i<g->max_member;i++){
		if( g->member[i].account_id>0 ) {
			if( g->member[i].account_id!=sd->status.account_id ||
			    g->member[i].char_id!=sd->status.char_id )
				break;
		}
	}
	if(i<g->max_member){
		clif_guild_broken(sd,2);
		return;
	}

	intif_guild_break(g->guild_id);

	return;
}

/*==========================================
 * �M���h��f�[�^�v��
 *------------------------------------------
 */
void guild_castledataload(int castle_id, int idx)
{
	intif_guild_castle_dataload(castle_id, idx);

	return;
}

/*==========================================
 * �M���h����擾���C�x���g�ǉ�
 *------------------------------------------
 */
void guild_addcastleinfoevent(int castle_id, int idx, const char *name)
{
	struct eventlist *ev;
	int code;

	if( name==NULL || *name==0 )
		return;

	code = castle_id | (idx << 16);
	ev = (struct eventlist *)aCalloc(1,sizeof(struct eventlist));
	memcpy(ev->name,name,sizeof(ev->name));
	ev->name[sizeof(ev->name)-1] = '\0';	// force \0 terminal
	ev->next = (struct eventlist *)numdb_insert(guild_castleinfoevent_db,code,ev);

	return;
}

/*==========================================
 * �M���h��f�[�^�v���ԐM
 *------------------------------------------
 */
void guild_castledataloadack(int castle_id, int idx, int value)
{
	struct guild_castle *gc=guild_castle_search(castle_id);
	int code;
	struct eventlist *ev,*ev2;

	if(gc==NULL){
		return;
	}
	code = castle_id | (idx << 16);
	switch(idx) {
		case  1: gc->guild_id   = value; break;
		case  2: gc->economy    = value; break;
		case  3: gc->defense    = value; break;
		case  4: gc->triggerE   = value; break;
		case  5: gc->triggerD   = value; break;
		case  6: gc->nextTime   = value; break;
		case  7: gc->payTime    = value; break;
		case  8: gc->createTime = value; break;
		case  9: gc->visibleC   = value; break;
		case 10: gc->visibleG0  = value; break;
		case 11: gc->visibleG1  = value; break;
		case 12: gc->visibleG2  = value; break;
		case 13: gc->visibleG3  = value; break;
		case 14: gc->visibleG4  = value; break;
		case 15: gc->visibleG5  = value; break;
		case 16: gc->visibleG6  = value; break;
		case 17: gc->visibleG7  = value; break;
		default:
			printf("guild_castledataloadack ERROR!! (Not found index=%d)\n", idx);
			return;
	}
	if( (ev = (struct eventlist *)numdb_search(guild_castleinfoevent_db,code)) != NULL ){
		numdb_erase(guild_castleinfoevent_db,code);
		while(ev){
			npc_event_do(ev->name);
			ev2=ev->next;
			aFree(ev);
			ev=ev2;
		}
	}

	return;
}

/*==========================================
 * �M���h��f�[�^�ύX�v��
 *------------------------------------------
 */
void guild_castledatasave(int castle_id, int idx, int value)
{
	intif_guild_castle_datasave(castle_id, idx, value);

	return;
}

/*==========================================
 * �M���h��f�[�^�ύX�ʒm
 *------------------------------------------
 */
void guild_castledatasaveack(int castle_id, int idx, int value)
{
	struct guild_castle *gc=guild_castle_search(castle_id);

	if(gc==NULL){
		return;
	}
	switch(idx) {
		case  1: gc->guild_id   = value; break;
		case  2: gc->economy    = value; break;
		case  3: gc->defense    = value; break;
		case  4: gc->triggerE   = value; break;
		case  5: gc->triggerD   = value; break;
		case  6: gc->nextTime   = value; break;
		case  7: gc->payTime    = value; break;
		case  8: gc->createTime = value; break;
		case  9: gc->visibleC   = value; break;
		case 10: gc->visibleG0  = value; break;
		case 11: gc->visibleG1  = value; break;
		case 12: gc->visibleG2  = value; break;
		case 13: gc->visibleG3  = value; break;
		case 14: gc->visibleG4  = value; break;
		case 15: gc->visibleG5  = value; break;
		case 16: gc->visibleG6  = value; break;
		case 17: gc->visibleG7  = value; break;
		default:
			printf("guild_castledatasaveack ERROR!! (Not found index=%d)\n", idx);
			return;
	}

	return;
}

/*==========================================
 * �M���h�f�[�^�ꊇ��M�i���������j
 *------------------------------------------
 */
void guild_castlealldataload(int len, struct guild_castle *gc)
{
	int i;
	int n=(len-4)/sizeof(struct guild_castle),ev=-1;

	nullpo_retv(gc);

	// �C�x���g�t���ŗv������f�[�^�ʒu��T��(�Ō�̐苒�f�[�^)
	for(i=0;i<n;i++){
		if( (gc+i)->guild_id )
			ev=i;
	}

	// ��f�[�^�i�[�ƃM���h���v��
	for(i=0;i<n;i++,gc++){
		struct guild_castle *c=guild_castle_search(gc->castle_id);
		if(!c){
			printf("guild_castlealldataload ??\n");
			continue;
		}
		memcpy(&c->guild_id,&gc->guild_id,
			sizeof(struct guild_castle) - ((int)&c->guild_id - (int)c) );
		if( c->guild_id ){
			if(i!=ev)
				guild_request_info(c->guild_id);
			else
				guild_npc_request_info(c->guild_id,"::OnAgitInit");
		}
	}
	if(ev==-1)
		npc_event_doall("OnAgitInit");

	return;
}

/*==========================================
 * �V�[�Y�J�n�iOnAgitStart�j
 *------------------------------------------
 */
void guild_agit_start(void)
{
	int c = npc_event_doall("OnAgitStart");

	printf("NPC_Event:[OnAgitStart] Run (%d) Events by @AgitStart.\n",c);
	if(battle_config.allow_guild_skill_in_gvgtime_only)
		battle_config.guild_skill_available = 1;

	return;
}

/*==========================================
 * �V�[�Y�I���iOnAgitEnd�j
 *------------------------------------------
 */
void guild_agit_end(void)
{
	int c = npc_event_doall("OnAgitEnd");

	printf("NPC_Event:[OnAgitEnd] Run (%d) Events by @AgitEnd.\n",c);
	if(battle_config.allow_guild_skill_in_gvgtime_only)
		battle_config.guild_skill_available = 0;

	return;
}

/*==========================================
 * �r���^�C�}�[�iOnAgitEliminate�j
 *------------------------------------------
 */
static int guild_gvg_eliminate_timer(int tid,unsigned int tick,int id,int data)
{
	char *evname = (char *)data;

	if(evname) {
		if(agit_flag) {
			int c = npc_event_do(evname);
			printf("NPC_Event:[%s] Run (%d) Events.\n",evname,c);
		}
		aFree(evname);
	}

	return 0;
}

/*==========================================
 * �G���y�j��iOnAgitBreak�j
 *------------------------------------------
 */
void guild_agit_break(struct mob_data *md)
{
	size_t len;
	char *evname;

	nullpo_retv(md);

	if(!agit_flag)	// Agit already End
		return;

	len = strlen(md->npc_event);
	evname = (char *)aCalloc(len + 5, sizeof(char));
	memcpy(evname, md->npc_event, len - 5);
	strcat(evname, "Eliminate");

	add_timer2(gettick()+battle_config.gvg_eliminate_time,guild_gvg_eliminate_timer,
		md->bl.m,(int)evname,TIMER_FREE_DATA);

	return;
}

/*==========================================
 * �I��
 *------------------------------------------
 */
static int guild_db_final(void *key,void *data,va_list ap)
{
	struct guild *g = (struct guild *)data;

	aFree(g);

	return 0;
}

static int guild_expcache_db_final(void *key,void *data,va_list ap)
{
	struct guild_expcache *c = (struct guild_expcache *)data;

	aFree(c);

	return 0;
}

static int guild_infoevent_db_final(void *key,void *data,va_list ap)
{
	struct eventlist *ev = (struct eventlist *)data;

	aFree(ev);

	return 0;
}

void do_final_guild(void)
{
	if(guild_db)
		numdb_final(guild_db,guild_db_final);
	if(guild_expcache_db)
		numdb_final(guild_expcache_db,guild_expcache_db_final);
	if(guild_infoevent_db)
		numdb_final(guild_infoevent_db,guild_infoevent_db_final);
	if(guild_castleinfoevent_db)
		numdb_final(guild_castleinfoevent_db,guild_infoevent_db_final);

	return;
}

/*==========================================
 * �M���h��f�[�^�x�[�X�ǂݍ���
 *------------------------------------------
 */
static void guild_read_castledb(void)
{
	FILE *fp;
	char line[1024];
	int j,ln=0;
	char *str[4],*p;

	// �f�t�H���g�f�[�^���쐬
	memset(castle_db,0,sizeof(castle_db));
	for(j=0; j<MAX_GUILDCASTLE; j++)
		castle_db[j].castle_id = j;

	if ((fp=fopen("db/castle_db.txt","r"))==NULL){
		printf("can't read db/castle_db.txt\n");
		return;
	}

	while(fgets(line,1020,fp)){
		int id;

		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));
		for(j=0,p=line;j<4 && p;j++){
			str[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		id = atoi(str[0]);
		if(id < 0 || id >= MAX_GUILDCASTLE)
			continue;

		memcpy(castle_db[id].map_name,str[1],24);
		memcpy(castle_db[id].castle_name,str[2],24);
		memcpy(castle_db[id].castle_event,str[3],24);

		// force \0 terminal
		castle_db[id].map_name[23]     = '\0';
		castle_db[id].castle_name[23]  = '\0';
		castle_db[id].castle_event[23] = '\0';
		ln++;
	}
	fclose(fp);
	printf("read db/castle_db.txt done (count=%d)\n",ln);

	return;
}

/*==========================================
 * �M���h�X�L��tree�ǂݍ���
 *------------------------------------------
 */
static void guild_read_guildskill_tree_db(void)
{
	int i,k,id,skillid;
	FILE *fp;
	char line[1024],*p;

	memset(guild_skill_tree,0,sizeof(guild_skill_tree));

	fp=fopen("db/guild_skill_tree.txt","r");
	if(fp==NULL){
		printf("can't read db/guild_skill_tree.txt\n");
		return;
	}

	id = 0;
	while(fgets(line,1020,fp)){
		char *split[12];
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(i=0,p=line;i<12 && p;i++){
			split[i]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(i<12)
			continue;

		skillid = atoi(split[0]);
		id = skillid - GUILD_SKILLID;
		if(id < 0 || id >= MAX_GUILDSKILL)
			continue;
		guild_skill_tree[id].id  = skillid;
		guild_skill_tree[id].max = atoi(split[1]);

		if(guild_skill_tree[id].max > guild_skill_get_max(skillid))
			guild_skill_tree[id].max = guild_skill_get_max(skillid);

		for(k=0;k<5;k++){
			guild_skill_tree[id].need[k].id=atoi(split[k*2+2]);
			guild_skill_tree[id].need[k].lv=atoi(split[k*2+3]);
		}
	}
	fclose(fp);
	printf("read db/guild_skill_tree.txt done\n");

	return;
}

/*==========================================
 * ������
 *------------------------------------------
 */
void do_init_guild(void)
{
	guild_db=numdb_init();
	guild_expcache_db=numdb_init();
	guild_infoevent_db=numdb_init();
	guild_castleinfoevent_db=numdb_init();

	guild_read_castledb();
	guild_read_guildskill_tree_db();

	add_timer_func_list(guild_gvg_eliminate_timer);
	add_timer_func_list(guild_payexp_timer);
	add_timer_func_list(guild_send_xy_timer);
	add_timer_interval(gettick()+GUILD_PAYEXP_INVERVAL,guild_payexp_timer,0,0,GUILD_PAYEXP_INVERVAL);
	add_timer_interval(gettick()+GUILD_SEND_XY_INVERVAL,guild_send_xy_timer,0,0,GUILD_SEND_XY_INVERVAL);

	return;
}
