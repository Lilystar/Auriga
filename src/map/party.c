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

#include "party.h"
#include "pc.h"
#include "map.h"
#include "battle.h"
#include "intif.h"
#include "clif.h"
#include "status.h"
#include "skill.h"
#include "unit.h"
#include "atcommand.h"

#define PARTY_SEND_XYHP_INVERVAL	1000	// ���W��g�o���M�̊Ԋu

static struct dbt* party_db = NULL;

/*==========================================
 * �p�[�e�B����
 *------------------------------------------
 */
struct party *party_search(int party_id)
{
	return (struct party *)numdb_search(party_db,party_id);
}

/*==========================================
 * �p�[�e�B������
 *------------------------------------------
 */
static int party_searchname_sub(void *key, void *data, va_list ap)
{
	struct party *p, **dst;
	char *str;

	p   = (struct party *)data;
	str = va_arg(ap,char *);
	dst = va_arg(ap,struct party **);

	if(*dst == NULL) {
		if(strcmp(p->name,str) == 0)
			*dst = p;
	}

	return 0;
}

struct party* party_searchname(char *str)
{
	struct party *p=NULL;

	numdb_foreach(party_db,party_searchname_sub,str,&p);

	return p;
}

/*==========================================
 * �쐬�v��
 *------------------------------------------
 */
void party_create(struct map_session_data *sd, const char *name, int item, int item2)
{
	int i;
	char party_name[24]; // 23 + NULL

	nullpo_retv(sd);

	if (sd->status.party_id == 0 && sd->state.party_creating == 0) {
		strncpy(party_name, name, 23);
		// force '\0' at end (against hacker: normal player can not create a party with a name longer than 23 characters)
		party_name[23] = '\0';

		// if too short name (hacker)
		if (party_name[0] == '\0') {
			clif_party_created(sd, 1); // 0xfa <flag>.B: 0: Party has successfully been organized, 1: That Party Name already exists., 2: The Character is already in a party.
			return;
		}

		// check control chars and del (is checked in char-server, check before to send to char-server to avoid packet transmission)
		for(i = 0; party_name[i]; i++) {
			if (!(party_name[i] & 0xe0) || party_name[i] == 0x7f) {
				clif_party_created(sd, 1); // 0xfa <flag>.B: 0: Party has successfully been organized, 1: That Party Name already exists., 2: The Character is already in a party.
				return;
			}
		}

		// ask char-server for creation
		sd->state.party_creating = 1;
		intif_create_party(sd, party_name, item, item2);
	} else {
		clif_party_created(sd,2); // 0xfa <flag>.B: 0: Party has successfully been organized, 1: That Party Name already exists., 2: The Character is already in a party.
	}

	return;
}

/*==========================================
 * �쐬��
 *------------------------------------------
 */
void party_created(int account_id, unsigned char fail, int party_id, const char *name)
{
	struct map_session_data *sd = map_id2sd(account_id);

	if(sd == NULL)
		return;

	if(fail==0){
		struct party *p;
		if (party_search(party_id) != NULL) {
			printf("party: id already exists!\n");
			exit(1);
		}
		sd->status.party_id=party_id;
		p=(struct party *)aCalloc(1,sizeof(struct party));
		p->party_id=party_id;
		memcpy(p->name,name,24);
		numdb_insert(party_db,party_id,p);
		clif_party_created(sd,0);
	}else{
		clif_party_created(sd,1);
	}
	sd->state.party_creating = 0;

	return;
}

/*==========================================
 * ���v��
 *------------------------------------------
 */
void party_request_info(int party_id)
{
	intif_request_partyinfo(party_id);

	return;
}

/*==========================================
 * �����L�����̊m�F
 *------------------------------------------
 */
static void party_check_member(struct party *p)
{
	int i;
	struct map_session_data *sd;

	nullpo_retv(p);

	for(i=0;i<fd_max;i++){
		if(session[i] && (sd = (struct map_session_data *)session[i]->session_data) && sd->state.auth){
			if(sd->status.party_id==p->party_id){
				int j,f=1;
				for(j=0;j<MAX_PARTY;j++){	// �p�[�e�B�Ƀf�[�^�����邩�m�F
					if(p->member[j].account_id==sd->status.account_id){
						if(p->member[j].char_id == sd->status.char_id)
							f=0;	// �f�[�^������
						else
							p->member[j].sd=NULL;	// ���C�ʃL����������
					}
				}
				if(f){
					sd->status.party_id=0;
					if(battle_config.error_log)
						printf("party: check_member %d[%s] is not member\n",sd->status.account_id,sd->status.name);
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
void party_recv_noinfo(int party_id)
{
	int i;
	struct map_session_data *sd;

	for(i=0;i<fd_max;i++){
		if(session[i] && (sd = (struct map_session_data *)session[i]->session_data) && sd->state.auth){
			if(sd->status.party_id==party_id)
				sd->status.party_id=0;
		}
	}

	return;
}

/*==========================================
 * ���擾
 *------------------------------------------
 */
void party_recv_info(struct party *sp)
{
	struct map_session_data *sd;
	struct party *p;
	int i;

	nullpo_retv(sp);

	if((p = party_search(sp->party_id)) == NULL){
		p = (struct party *)aMalloc(sizeof(struct party));
		numdb_insert(party_db,sp->party_id,p);

		// �ŏ��̃��[�h�Ȃ̂Ń��[�U�[�̃`�F�b�N���s��
		party_check_member(sp);
	}
	memcpy(p,sp,sizeof(struct party));

	for(i=0;i<MAX_PARTY;i++){	// sd�̐ݒ�
		sd = map_id2sd(p->member[i].account_id);
		if( sd != NULL &&
		    sd->status.char_id == p->member[i].char_id &&
		    sd->status.party_id == p->party_id &&
		    !sd->state.waitingdisconnect )
			p->member[i].sd = sd;
		else
			p->member[i].sd = NULL;
	}

	clif_party_main_info(p,-1);
	clif_party_info(p,-1);

	for(i=0;i<MAX_PARTY;i++){	// �ݒ���̑��M
		sd = p->member[i].sd;
		if(sd!=NULL && sd->state.party_sended == 0){
			clif_party_option(p,sd,0x100);
			sd->state.party_sended = 1;
		}
	}

	return;
}

/*==========================================
 * �ʒu�ʒm�N���A
 *------------------------------------------
 */
static void party_send_xy_clear(struct party *p)
{
	int i;

	nullpo_retv(p);

	for(i=0;i<MAX_PARTY;i++){
		struct map_session_data *sd = p->member[i].sd;
		if(sd) {
			sd->party_x  = -1;
			sd->party_y  = -1;
			sd->party_hp = -1;
		}
	}

	return;
}

/*==========================================
 * �p�[�e�B�ւ̊��U
 *------------------------------------------
 */
void party_invite(struct map_session_data *sd, struct map_session_data *tsd)
{
	struct party *p;
	int i, empty = 0;

	nullpo_retv(sd);

	p = party_search(sd->status.party_id);
	if(p == NULL)
		return;

	if(tsd == NULL) {	// ���肪������Ȃ�
		clif_party_inviteack(sd,"",7);
		return;
	}
	if(tsd->state.waitingdisconnect) {	// ���肪�ؒf�҂�
		clif_party_inviteack(sd,tsd->status.name,1);
		return;
	}
	if(!battle_config.invite_request_check) {
		if(tsd->guild_invite > 0 || tsd->trade_partner || tsd->adopt_invite) {	// ���肪��������ǂ���
			clif_party_inviteack(sd,tsd->status.name,1);
			return;
		}
	}
	if(tsd->status.party_id > 0 || tsd->party_invite > 0) {	// ����̏����m�F
		clif_party_inviteack(sd,tsd->status.name,0);
		return;
	}
	for(i=0; i<MAX_PARTY; i++) {
		if(p->member[i].account_id == sd->status.account_id &&
		   p->member[i].char_id == sd->status.char_id &&
		   p->member[i].leader == 0)
			return;		// �v���҂����[�_�[�ł͂Ȃ��ihacker?�j

		if(p->member[i].account_id <= 0) {
			empty = 1;
		} else if(p->member[i].account_id == tsd->status.account_id) {
			if(battle_config.party_join_limit) {
				clif_party_inviteack(sd,tsd->status.name,4);
				return;
			}
			if(p->member[i].char_id == tsd->status.char_id) {
				clif_party_inviteack(sd,tsd->status.name,0);
				return;
			}
		}
	}
	if(!empty) {	// ����I�[�o�[
		clif_party_inviteack(sd,tsd->status.name,3);
		return;
	}
	if(tsd->state.refuse_partyinvite) {	// ���肪���U���ے�
		clif_party_inviteack(sd,tsd->status.name,5);
		return;
	}
	if(battle_config.party_invite_range_check) {	// �����MAP�Ƌ������m�F
		if(sd->bl.m != tsd->bl.m || unit_distance(sd->bl.x,sd->bl.y,tsd->bl.x,tsd->bl.y) > AREA_SIZE) {
			clif_party_inviteack(sd,tsd->status.name,1);
			return;
		}
	}

	tsd->party_invite         = sd->status.party_id;
	tsd->party_invite_account = sd->status.account_id;

	clif_party_invite(sd,tsd,p->name);

	return;
}

/*==========================================
 * �p�[�e�B���U�ւ̕ԓ�
 *------------------------------------------
 */
void party_reply_invite(struct map_session_data *sd, int account_id, int flag)
{
	struct map_session_data *tsd;

	nullpo_retv(sd);

	if(flag == 1) {	// ����
		if(sd->party_invite > 0) {
			// inter�I�֒ǉ��v��
			intif_party_addmember(sd);
			return;
		}
	}

	// ����
	sd->party_invite         = 0;
	sd->party_invite_account = 0;
	tsd = map_id2sd(account_id);
	if(tsd)
		clif_party_inviteack(tsd,sd->status.name,1);

	return;
}

/*==========================================
 * �p�[�e�B�����m�F
 *------------------------------------------
 */
static void party_check_conflict(struct map_session_data *sd)
{
	nullpo_retv(sd);

	//intif_party_checkconflict(sd->status.party_id,sd->status.account_id,sd->status.char_id);

	return;
}

/*==========================================
 * �p�[�e�B���ǉ����ꂽ
 *------------------------------------------
 */
void party_member_added(int party_id, int account_id, int char_id, unsigned char flag)
{
	struct map_session_data *sd, *sd2;
	struct party *p;

	if((p = party_search(party_id)) == NULL)
		return;

	sd = map_id2sd(account_id);
	if(sd == NULL) {
		if(flag == 0) {
			if(battle_config.error_log)
				printf("party: member added error %d is not online\n",account_id);
			intif_party_leave(party_id, account_id, char_id); // �L�������ɓo�^�ł��Ȃ��������ߒE�ޗv�����o��
		}
		return;
	}
	sd2 = map_id2sd(sd->party_invite_account);
	sd->party_invite         = 0;
	sd->party_invite_account = 0;

	if(flag == 1) {	// ���s
		if(sd2)
			clif_party_inviteack(sd2,sd->status.name,0);
		return;
	}

	// ����
	sd->state.party_sended = 0;
	sd->status.party_id    = party_id;

	if(sd2)
		clif_party_inviteack(sd2,sd->status.name,2);

	// �������������m�F
	party_check_conflict(sd);

	// ���W�Ēʒm�v��
	party_send_xy_clear(p);

	return;
}

/*==========================================
 * �p�[�e�B�����v��
 *------------------------------------------
 */
void party_removemember(struct map_session_data *sd, int account_id, const char *name)
{
	struct party *p;
	int i;

	nullpo_retv(sd);

	if( (p = party_search(sd->status.party_id)) == NULL )
		return;

	for(i=0;i<MAX_PARTY;i++){	// ���[�_�[���ǂ����`�F�b�N
		if (p->member[i].account_id == sd->status.account_id &&
		    p->member[i].char_id == sd->status.char_id) {
			if(p->member[i].leader==0)
				return;
		}
	}

	for(i=0;i<MAX_PARTY;i++){	// �������Ă��邩���ׂ�
		if (p->member[i].account_id == account_id &&
		    strncmp(p->member[i].name, name, 24) == 0){
			intif_party_leave(p->party_id, account_id, p->member[i].char_id);
			return;
		}
	}

	return;
}

/*==========================================
 * �p�[�e�B�E�ޗv��
 *------------------------------------------
 */
void party_leave(struct map_session_data *sd)
{
	struct party *p;
	int i;

	nullpo_retv(sd);

	if( (p = party_search(sd->status.party_id)) == NULL )
		return;

	for(i=0;i<MAX_PARTY;i++){	// �������Ă��邩
		if (p->member[i].account_id == sd->status.account_id &&
		    p->member[i].char_id == sd->status.char_id) {
			intif_party_leave(p->party_id, sd->status.account_id, p->member[i].char_id);
			return;
		}
	}

	return;
}

/*==========================================
 * �p�[�e�B�����o���E�ނ���
 *------------------------------------------
 */
void party_member_leaved(int party_id, int account_id, int char_id)
{
	struct map_session_data *sd = map_id2sd(account_id);
	struct party *p = party_search(party_id);

	if(p) {
		int i;
		for(i=0;i<MAX_PARTY;i++) {
			if (p->member[i].account_id == account_id &&
			    p->member[i].char_id == char_id) {
				clif_party_leaved(p,sd,account_id,p->member[i].name,0x00);
				p->member[i].account_id = 0;
				p->member[i].char_id    = 0;
				p->member[i].sd         = NULL;
			}
		}
	}
	if(sd && sd->status.party_id == party_id && sd->status.char_id == char_id) {
		sd->status.party_id    = 0;
		sd->state.party_sended = 0;
	}

	return;
}

/*==========================================
 * �p�[�e�B���U�ʒm
 *------------------------------------------
 */
void party_broken(int party_id)
{
	struct party *p = party_search(party_id);
	int i;

	if(p == NULL)
		return;

	for(i=0;i<MAX_PARTY;i++){
		if(p->member[i].sd!=NULL){
			clif_party_leaved(p, p->member[i].sd, p->member[i].account_id, p->member[i].name, 0x10);
			p->member[i].sd->status.party_id    = 0;
			p->member[i].sd->state.party_sended = 0;
		}
	}
	numdb_erase(party_db,party_id);
	aFree(p);

	return;
}

/*==========================================
 * �Ƒ������ɕK�v�ȍŒ�������`�F�b�N����
 * �{�q�L������ID��Ԃ�
 *------------------------------------------
 */
static int party_check_family_share(struct party *p)
{
	int i, count = 0;
	int p1_idx = -1, p2_idx = -1, baby_idx = -1;
	struct party_member *m;

	nullpo_retr(0, p);

	for(i=0; i<MAX_PARTY; i++) {
		m = &p->member[i];
		if(m->account_id > 0) {
			count++;
			if(m->online && m->sd) {
				if(m->sd->status.baby_id > 0) {
					if(p1_idx <= 0)
						p1_idx = i;
					else
						p2_idx = i;
				}
				if(m->sd->status.parent_id[0] > 0 && m->sd->status.parent_id[1] > 0) {
					baby_idx = i;
				}
			}
		}
	}

	// 3�lPT�ŗ{�q�L���������Ăǂ��炩�̐e�Ɠ���MAP��ɂ���Ȃ狖��
	// �������e�q�֌W�����������ǂ����̃`�F�b�N��int_party.c�ōs��

	if(count == 3 && baby_idx >= 0) {
		m = &p->member[baby_idx];
		if( (p1_idx >= 0 && strcmp(m->map, p->member[p1_idx].map) == 0) ||
		    (p2_idx >= 0 && strcmp(m->map, p->member[p2_idx].map) == 0) )
			return m->sd->status.char_id;
	}
	return 0;
}

/*==========================================
 * �p�[�e�B�̐ݒ�ύX�v��
 *------------------------------------------
 */
void party_changeoption(struct map_session_data *sd, int exp, int item)
{
	struct party *p;
	int i, baby_id = 0;

	nullpo_retv(sd);

	if ((p = party_search(sd->status.party_id)) == NULL)
		return;

	// ONLY the party leader can choose either 'Each Take' or 'Even Share' for experience points.
	for(i = 0; i < MAX_PARTY; i++) {
		if (p->member[i].account_id == sd->status.account_id &&
		    p->member[i].char_id == sd->status.char_id) {
			if (p->member[i].leader) {
				if(exp > 0 && !p->exp) {
					baby_id = party_check_family_share(p);
				}
				if(item < 0) {	// ���̂Ƃ��̓A�C�e�����z�ݒ��ύX���Ȃ�
					item = p->item;
				}
				intif_party_changeoption(sd->status.party_id, sd->status.account_id, baby_id, exp, item);
				return;
			}
		}
	}

	return;
}

/*==========================================
 * �p�[�e�B�̐ݒ�ύX�ʒm
 *------------------------------------------
 */
void party_optionchanged(int party_id, int account_id, unsigned char exp, unsigned char item, int flag)
{
	struct party *p;
	struct map_session_data *sd=map_id2sd(account_id);

	if( (p=party_search(party_id))==NULL)
		return;

	if(!(flag&0x01))
		p->exp = exp;
	if(!(flag&0x10)) {
		int old_item = p->item;
		p->item = item;
		if(old_item != item)
			clif_party_main_info(p,-1);
	}
	clif_party_option(p,sd,flag);

	return;
}

/*==========================================
 * �p�[�e�B�����o�̈ړ��ʒm
 *------------------------------------------
 */
void party_recv_movemap(int party_id, int account_id, int char_id, char *mapname, unsigned char online, unsigned short lv)
{
	struct map_session_data *sd;
	struct party *p;
	int i;

	if( (p=party_search(party_id))==NULL)
		return;

	for(i=0;i<MAX_PARTY;i++){
		struct party_member *m = &p->member[i];
		if (m->account_id == account_id && m->char_id == char_id) {
			memcpy(m->map,mapname,16);
			m->online = online;
			m->lv     = lv;
			break;
		}
	}
	if(i==MAX_PARTY){
		if(battle_config.error_log)
			printf("party: not found member %d on %d[%s]",account_id,party_id,p->name);
		return;
	}

	for(i=0;i<MAX_PARTY;i++){	// sd�Đݒ�
		sd = map_id2sd(p->member[i].account_id);
		if( sd != NULL &&
		    sd->status.char_id == p->member[i].char_id &&
		    sd->status.party_id == p->party_id &&
		    !sd->state.waitingdisconnect )
			p->member[i].sd = sd;
		else
			p->member[i].sd = NULL;
	}

	party_send_xy_clear(p);	// ���W�Ēʒm�v��

	clif_party_main_info(p,-1);
	clif_party_info(p,-1);

	return;
}

/*==========================================
 * �p�[�e�B�����o�̈ړ�
 *------------------------------------------
 */
void party_send_movemap(struct map_session_data *sd)
{
	struct party *p;

	nullpo_retv(sd);

	if( sd->status.party_id==0 )
		return;

	intif_party_changemap(sd,1);

	if( sd->state.party_sended != 0 )	// �����p�[�e�B�f�[�^�͑��M�ς�
		return;

	// �����m�F
	party_check_conflict(sd);

	// ����Ȃ�p�[�e�B��񑗐M
	if( (p=party_search(sd->status.party_id))!=NULL ){
		party_check_member(p);	// �������m�F����
		if(sd->status.party_id==p->party_id){
			clif_party_main_info(p,sd->fd);
			clif_party_info(p,sd->fd);
			clif_party_option(p,sd,0x100);
			sd->state.party_sended = 1;
		}
	}

	return;
}

/*==========================================
 * �p�[�e�B�����o�̃��O�A�E�g
 *------------------------------------------
 */
void party_send_logout(struct map_session_data *sd)
{
	struct party *p;

	nullpo_retv(sd);

	if( sd->status.party_id>0 )
		intif_party_changemap(sd,0);

	// sd�������ɂȂ�̂Ńp�[�e�B��񂩂�폜
	if( (p=party_search(sd->status.party_id))!=NULL ){
		int i;
		for(i=0;i<MAX_PARTY;i++) {
			if(p->member[i].sd==sd)
				p->member[i].sd=NULL;
		}
	}

	return;
}

/*==========================================
 * �p�[�e�B���b�Z�[�W���M
 *------------------------------------------
 */
void party_send_message(struct map_session_data *sd, char *mes, int len)
{
	if(sd->status.party_id==0)
		return;

	intif_party_message(sd->status.party_id,sd->status.account_id,mes,len);

	return;
}

/*==========================================
 * �p�[�e�B���b�Z�[�W��M
 *------------------------------------------
 */
void party_recv_message(int party_id, int account_id, char *mes, int len)
{
	struct party *p;

	if( (p=party_search(party_id))==NULL)
		return;
	clif_party_message(p,account_id,mes,len);

	return;
}

/*==========================================
 * �ʒu��HP�ʒm
 *------------------------------------------
 */
static int party_send_xyhp_timer_sub(void *key, void *data, va_list ap)
{
	struct party *p=(struct party *)data;
	int i;

	nullpo_retr(0, p);

	for(i=0;i<MAX_PARTY;i++){
		struct map_session_data *sd;
		if((sd=p->member[i].sd)!=NULL){
			// ���W�ʒm
			if(sd->party_x!=sd->bl.x || sd->party_y!=sd->bl.y){
				clif_party_xy(sd);
				sd->party_x=sd->bl.x;
				sd->party_y=sd->bl.y;
			}
			// �g�o�ʒm
			if(sd->party_hp!=sd->status.hp){
				clif_party_hp(sd);
				sd->party_hp=sd->status.hp;
			}
		}
	}

	return 0;
}

static int party_send_xyhp_timer(int tid, unsigned int tick, int id, void *data)
{
	numdb_foreach(party_db,party_send_xyhp_timer_sub);

	return 0;
}

/*==========================================
 * HP�ʒm�̕K�v�������p�imap_foreachinmovearea����Ă΂��j
 *------------------------------------------
 */
int party_send_hp_check(struct block_list *bl,va_list ap)
{
	int party_id;
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd = (struct map_session_data *)bl);

	party_id = va_arg(ap,int);

	if(sd->status.party_id == party_id){
		sd->party_hp = -1;
		return 1;
	}

	return 0;
}

/*==========================================
 * �o���l�������z
 *------------------------------------------
 */
void party_exp_share(struct party *p, struct mob_data *md, atn_bignumber base_exp, atn_bignumber job_exp)
{
	struct map_session_data *sd;
	int i,c;
	int base_bonus, job_bonus;
	struct map_session_data *sdlist[MAX_PARTY];

	nullpo_retv(p);
	nullpo_retv(md);

	for(i=c=0;i<MAX_PARTY;i++)
	{
		if((sd = p->member[i].sd) != NULL && sd->bl.prev && sd->bl.m == md->bl.m && !unit_isdead(&sd->bl)) {
			if( (sd->sc.data[SC_TRICKDEAD].timer == -1 || !battle_config.noexp_trickdead) && 	// ���񂾂ӂ肵�Ă��Ȃ�
			    (sd->sc.data[SC_HIDING].timer == -1	   || !battle_config.noexp_hiding   ) )		// �n�C�h���Ă��Ȃ�
				sdlist[c++] = sd;
		}
	}
	if(c==0)
		return;

	base_bonus = 100 + (battle_config.pt_bonus_b*(c-1));
	job_bonus  = 100 + (battle_config.pt_bonus_j*(c-1));
	for(i=0;i<c;i++)
	{
		atn_bignumber bexp = base_exp * base_bonus / 100 / c + 1;
		atn_bignumber jexp = job_exp  * job_bonus  / 100 / c + 1;

		pc_gainexp(sdlist[i],md,bexp,jexp,0);
	}

	return;
}

/*==========================================
 * �A�C�e�����z
 *------------------------------------------
 */
int party_loot_share(struct party *p, struct map_session_data *sd, struct item *item_data, int first)
{
	nullpo_retr(1, sd);
	nullpo_retr(1, item_data);

	if(p && p->item&2 && (first || battle_config.party_item_share_type)) {
		struct map_session_data *psd[MAX_PARTY];
		int i, c=0;

		for(i=0; i<MAX_PARTY; i++) {
			if((psd[c] = p->member[i].sd) && psd[c]->bl.prev && psd[c]->bl.m == sd->bl.m && !unit_isdead(&psd[c]->bl))
				c++;
		}
		while(c > 0) {	// �����_���I��
			i = atn_rand()%c;
			if(pc_additem(psd[i],item_data,item_data->amount)) {
				// �擾���s
				psd[i] = psd[c-1];
				c--;
			} else {
				if(battle_config.party_item_share_show && psd[i] != sd) {
#if PACKETVER < 12
					char output[128];
					snprintf(output, sizeof(output), msg_txt(177), psd[i]->status.name);
					clif_displaymessage(sd->fd,output);
#else
					clif_show_partyshareitem(sd,item_data);
#endif
				}
				return 0;
			}
		}
	}

	// ���z�ł��Ȃ��̂Ō���sd�ɓn��
	return pc_additem(sd,item_data,item_data->amount);
}

/*==========================================
 * �����E�B���h�E�\��
 *------------------------------------------
 */
void party_equip_window(struct map_session_data *sd, int account_id)
{
	struct map_session_data *tsd;

	nullpo_retv(sd);

	if(battle_config.equip_window_type == 0)
		return;

	tsd = map_id2sd(account_id);
	if(tsd == NULL)
		return;

	if(tsd->state.waitingdisconnect)	// ���肪�ؒf�҂�
		return;
	if(battle_config.equip_window_type == 1) {
		if(tsd->status.party_id <= 0 || sd->status.party_id != tsd->status.party_id)	// PT���قȂ�
			return;
	}
	if(sd->bl.m != tsd->bl.m || unit_distance(sd->bl.x, sd->bl.y, tsd->bl.x, tsd->bl.y) > AREA_SIZE)	// ����������
		return;
	if(!tsd->state.show_equip) {	// ���肪���������J���ĂȂ�
		clif_msgstringtable(sd, 0x54d);
		return;
	}

	clif_party_equiplist(sd, tsd);

	return;
}

/*==========================================
 * �p�[�e�B�[���[�_�[�ύX
 *------------------------------------------
 */
void party_changeleader(struct map_session_data *sd, int id)
{
	struct party *p;
	struct map_session_data *tsd;
	int i, t_i;

	nullpo_retv(sd);

	if(sd->status.party_id == 0)
		return;

	p = party_search(sd->status.party_id);
	if(p == NULL)
		return;

	for(i = 0; i < MAX_PARTY && p->member[i].sd != sd; i++);
	if(i == MAX_PARTY)
		return;

	if(!p->member[i].leader)
		return;

	tsd = map_id2sd(id);

	if(tsd == NULL || tsd->status.party_id != sd->status.party_id)
		return;

	for(t_i = 0; t_i < MAX_PARTY && p->member[t_i].sd != tsd; t_i++);
	if(t_i == MAX_PARTY)
		return;

	intif_party_leaderchange(p->party_id,p->member[t_i].account_id,p->member[t_i].char_id);

	// PT���[�_�[�ύX
#if PACKETVER < 25
	p->member[i].leader = 0;
	if(p->member[i].sd->fd)
		clif_displaymessage(p->member[i].sd->fd, msg_txt(194));
	p->member[t_i].leader = 1;
	if(p->member[t_i].sd->fd)
		clif_displaymessage(p->member[t_i].sd->fd, msg_txt(195));
	clif_party_info(p,-1);
#else
	p->member[i].leader = 0;
	p->member[t_i].leader = 1;
	clif_partyleader_info(sd,tsd->status.account_id);
#endif

	return;
}

/*==========================================
 * �����}�b�v�̃p�[�e�B�����o�[�S�̂ɏ�����������
 *------------------------------------------
 */
int party_foreachsamemap(int (*func)(struct block_list*,va_list),struct map_session_data *sd,int range,...)
{
	struct party *p;
	int i, x0, y0, x1, y1;
	struct block_list *list[MAX_PARTY];
	int blockcount = 0;
	int ret = 0;

	nullpo_retr(0, sd);

	if((p = party_search(sd->status.party_id)) == NULL)
		return 0;

	x0 = sd->bl.x - range;
	y0 = sd->bl.y - range;
	x1 = sd->bl.x + range;
	y1 = sd->bl.y + range;

	for(i=0; i<MAX_PARTY; i++) {
		struct party_member *m = &p->member[i];
		if(m != NULL && m->sd != NULL) {
			if( sd->bl.m != m->sd->bl.m )
				continue;
			if( m->sd->bl.x < x0 || m->sd->bl.y < y0 ||
			    m->sd->bl.x > x1 || m->sd->bl.y > y1 )
				continue;
			list[blockcount++] = &m->sd->bl;
		}
	}

	map_freeblock_lock();	// ����������̉�����֎~����

	for(i=0; i<blockcount; i++) {
		if(list[i]->prev) {	// �L�����ǂ����`�F�b�N
			va_list ap;
			va_start(ap, range);
			ret += func(list[i],ap);
			va_end(ap);
		}
	}

	map_freeblock_unlock();	// �����������

	return ret;
}

/*==========================================
 * �����}�b�v�ŃI�����C�����̃p�[�e�B�����o�[�̐���Ԃ�
 * 0:���Ȃ���,PT���Ȃ�
 *------------------------------------------
 */
int party_check_same_map_member_count(struct map_session_data *sd)
{
	int count = 0;
	int i;
	struct party* pt = NULL;

	nullpo_retr(0, sd);

	pt = party_search(sd->status.party_id);

	if(pt == NULL)
		return 0;

	for(i=0;i<MAX_PARTY;i++)
	{
		if(pt->member[i].online && pt->member[i].sd!=NULL)
		{
			if(sd != pt->member[i].sd && sd->bl.m == pt->member[i].sd->bl.m)
				count++;
		}
	}

	return count;
}

/*==========================================
 * �I��
 *------------------------------------------
 */
static int party_db_final(void *key,void *data,va_list ap)
{
	aFree(data);

	return 0;
}

void do_final_party(void)
{
	if(party_db)
		numdb_final(party_db,party_db_final);

	return;
}

/*==========================================
 * ������
 *------------------------------------------
 */
void do_init_party(void)
{
	party_db=numdb_init();

	add_timer_func_list(party_send_xyhp_timer);
	add_timer_interval(gettick()+PARTY_SEND_XYHP_INVERVAL,party_send_xyhp_timer,0,NULL,PARTY_SEND_XYHP_INVERVAL);

	return;
}
