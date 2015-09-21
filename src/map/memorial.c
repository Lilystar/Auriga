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
#include <time.h>

#include "db.h"
#include "timer.h"
#include "socket.h"
#include "nullpo.h"
#include "malloc.h"
#include "utils.h"
#include "mmo.h"

#include "memorial.h"
#include "map.h"
#include "npc.h"
#include "clif.h"
#include "party.h"
#include "pc.h"

#define MAX_MEMORIAL_DB		10		// �������A���_���W�����ő�DB��
#define MEMORIAL_INVERVAL	60000	// �������A���_���W�����\�񏈗��Ԋu(ms)
#define MEMORIAL_LIMIT		300		// �������A���_���W��������E���l��������(�b)

struct memorial_data memorial_data[MAX_MEMORIAL_DATA];

static struct memorial_db{
	short type;
	char name[61];
	int limit;
	struct {
		char mapname[24];
		short x, y;
	} enter;
	char mapname[MAX_MEMORIAL_SEGMAP][24];
} memorial_db[MAX_MEMORIAL_DB];

static struct {
	int id[MAX_MEMORIAL_DATA];
	int count;
	int timer;
} memorial_wait;

/*==========================================
 * TYPE���烁�����A��DB�̃f�[�^������
 *------------------------------------------
 */
static struct memorial_db *memorial_searchtype_db(short memorial_type)
{
	int i;

	for(i=0; i < MAX_MEMORIAL_DB; i++) {
		if(memorial_db[i].type == memorial_type)
			return &memorial_db[i];
	}

	return NULL;
}

/*==========================================
 * ���̂��烁�����A��DB�̃f�[�^������
 *------------------------------------------
 */
static struct memorial_db *memorial_searchname_db(const char *memorial_name)
{
	int i;

	for(i=0; i < MAX_MEMORIAL_DB; i++) {
		if(strcmp(memorial_db[i].name, memorial_name) == 0)
			return &memorial_db[i];
	}

	return NULL;
}

/*==========================================
 * �������A���_���W�����폜�^�C�}�[
 *------------------------------------------
 */
static int memorial_delete_timer(int tid, unsigned int tick, int id, void *data)
{
	memorial_delete(id);

	return 0;
}

/*==========================================
 * �������A���_���W����NPC�ǉ�
 *------------------------------------------
 */
static int memorial_addnpc(struct block_list *bl, va_list ap)
{
	struct npc_data* nd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, nd = (struct npc_data *)bl);

	return npc_addmdnpc(nd, va_arg(ap, int));
}

/*==========================================
 * �������A���_���W����MAP�ǉ�
 *------------------------------------------
 */
static int memorial_addmap(int memorial_id)
{
	int i, m;
	int cnt_map = 0, cnt_npc = 0;
	struct memorial_data *md;
	struct memorial_db *db;
	struct party *pt;

	if(memorial_id <= 0 || memorial_id > MAX_MEMORIAL_DATA)
		return 0;

	md = &memorial_data[memorial_id-1];

	// �\��ҋ@���łȂ�
	if(md->state != MDSTATE_IDLE)
		return 0;

	if((db = memorial_searchtype_db(md->type)) == NULL)
		return 0;

	// �������A���_���W�������ݒ�
	md->state = MDSTATE_BUSY;
	md->idle_limit = (unsigned int)time(NULL) + MEMORIAL_LIMIT;
	md->idle_timer = add_timer(gettick()+MEMORIAL_LIMIT*1000, memorial_delete_timer, memorial_id, NULL);

	// �}�b�v�ǉ�
	for(i = 0; i < MAX_MEMORIAL_SEGMAP; i++) {
		m = map_addmdmap(db->mapname[i], memorial_id);
		if(m < 0)
			continue;
		md->map[cnt_map].m = m;
		md->map[cnt_map].src_m = map_mapname2mapid(db->mapname[i]);
		cnt_map++;
	}

	// �}�b�v�ǉ����NPC�ǉ�
	for(i = 0; i < cnt_map; i++) {
		// ���}�b�v����NPC��T��
		cnt_npc += map_foreachinarea(memorial_addnpc, md->map[i].src_m, 0, 0, map[md->map[i].src_m].xs, map[md->map[i].src_m].ys, BL_NPC, md->map[i].m);
	}

	// �ǉ�NPC��OnInit�C�x���g���s
	if(cnt_npc) {
		for(i = 0; i < cnt_map; i++) {
			npc_event_doall_map("OnInit", md->map[i].m);
		}
	}

	// �p�[�e�B�[�ɐi���ʒm
	if((pt = party_search(md->party_id)) != NULL) {
		for(i = 0; i < MAX_PARTY; i++) {
			clif_memorial_status(pt->member[i].sd, db->name, md->keep_limit, md->idle_limit, 1);
			break;
		}
	}

	printf("memorial_addmap:[%s] memorial_id=%03d map_count=%d npc_count=%d\n",db->name, memorial_id, cnt_map, cnt_npc);

	return 1;
}

/*==========================================
 * �������A���_���W�����\��^�C�}�[
 *------------------------------------------
 */
static int memorial_subscription_timer(int tid, unsigned int tick, int id, void *data)
{
	int i, j, ret;
	int memorial_id = memorial_wait.id[0];	// �擪�̗\��ID�̂ݏ���
	struct party *pt;

	// �\��Ȃ�
	if(memorial_wait.count == 0)
		return 0;
	if(memorial_id <= 0 || memorial_id > MAX_MEMORIAL_DATA)
		return 0;

	// �}�b�v�쐬
	ret = memorial_addmap(memorial_id);

	// ���s���̃p�[�e�B�[�\��������ʒm
	if(ret == 0) {
		if((pt = party_search(memorial_data[memorial_id - 1].party_id)) != NULL) {
			for(i = 0; i < MAX_PARTY; i++) {
				if(pt->member[i].sd) {
					clif_memorial_changewait(pt->member[i].sd, 0xffff, 1);
					break;
				}
			}
		}
	}

	// �\�񏇏��ύX
	memorial_wait.count--;
	memmove(&memorial_wait.id[0],&memorial_wait.id[1],sizeof(memorial_wait.id[0])*memorial_wait.count);
	memset(&memorial_wait.id[memorial_wait.count], 0, sizeof(memorial_wait.id[0]));

	// �\�񏇏��ʒm
	for(i = 0; i < memorial_wait.count; i++) {
		if(memorial_data[memorial_wait.id[i]-1].state == MDSTATE_IDLE) {
			if((pt = party_search(memorial_data[memorial_wait.id[i]-1].party_id)) != NULL) {
				for(j = 0; j < MAX_PARTY; j++) {
					if(pt->member[j].sd) {
						clif_memorial_changewait(pt->member[j].sd, i+1, 1);
						break;
					}
				}
			}
		}
	}

	// �\��^�C�}�[�ĊJ�E��~
	if(memorial_wait.count)
		memorial_wait.timer = add_timer(gettick()+MEMORIAL_INVERVAL, memorial_subscription_timer, 0, NULL);
	else
		memorial_wait.timer = -1;

	return 0;
}

/*==========================================
 * �ێ������^�C�}�[�N��
 *------------------------------------------
 */
static int memorial_startkeeptimer(struct memorial_data *md, int memorial_id)
{
	struct memorial_db *db;
	struct party *pt;
	int i;

	nullpo_retr(0, md);

	// ���ɋN����
	if(md->keep_timer != -1)
		return 1;

	if((db = memorial_searchtype_db(md->type)) == NULL)
		return 1;

	// �^�C�}�[�J�n
	md->keep_limit = (unsigned int)time(NULL) + db->limit;
	md->keep_timer = add_timer(gettick()+db->limit*1000, memorial_delete_timer, memorial_id, NULL);

	// �p�[�e�B�[�ɏ��ʒm
	if((pt = party_search(md->party_id)) != NULL) {
		for(i = 0; i < MAX_PARTY; i++) {
			if(pt->member[i].sd) {
				clif_memorial_status(pt->member[i].sd, db->name, md->keep_limit, md->idle_limit, 1);
				break;
			}
		}
	}

	return 0;
}

/*==========================================
 * ���l�����^�C�}�[�N��
 *------------------------------------------
 */
static int memorial_startidletimer(struct memorial_data *md, int memorial_id)
{
	struct memorial_db *db;
	struct party *pt;
	int i;

	nullpo_retr(1, md);

	// ���ɋN����
	if(md->idle_timer != -1)
		return 1;

	// �^�C�}�[�J�n
	md->idle_limit = (unsigned int)time(NULL) + MEMORIAL_LIMIT;
	md->idle_timer = add_timer(gettick()+MEMORIAL_LIMIT*1000, memorial_delete_timer, memorial_id, NULL);

	// �p�[�e�B�[�ɏ��ʒm
	if((pt = party_search(md->party_id)) && (db = memorial_searchtype_db(md->type))) {
		for(i = 0; i < MAX_PARTY; i++) {
			if(pt->member[i].sd) {
				clif_memorial_status(pt->member[i].sd, db->name, md->keep_limit, md->idle_limit, 1);
				break;
			}
		}
	}

	return 0;
}

/*==========================================
 * ���l�����^�C�}�[��~
 *------------------------------------------
 */
static int memorial_stopidletimer(struct memorial_data *md)
{
	struct party *pt;
	int i;

	nullpo_retr(0, md);
	
	// ���ɒ�~��
	if(md->idle_timer == -1)
		return 1;

	// �^�C�}�[��~
	md->idle_limit = 0;
	delete_timer(md->idle_timer, memorial_delete_timer);
	md->idle_timer = -1;

	// �p�[�e�B�[�ɏ��ʒm
	if((pt = party_search(md->party_id)) != NULL) {
		for(i = 0; i < MAX_PARTY; i++) {
			if(pt->member[i].sd) {
				clif_memorial_changestatus(pt->member[i].sd, 0, md->idle_limit, 1);
				break;
			}
		}
	}

	return 0;
}

/*==========================================
 * �������A���_���W��������
 *------------------------------------------
 */
int memorial_create(const char *memorial_name, int party_id)
{
	int i;
	struct memorial_db *db = memorial_searchname_db(memorial_name);
	struct party *pt = party_search(party_id);

	if(db == NULL)
		return MDCREATE_ERROR;

	// �p�[�e�B�[������
	if(pt == NULL)
		return MDCREATE_PERMISSION;

	// ���ɐ����ς�
	if(pt->memorial_id) {
		// ���ɗ\��ς�
		if(memorial_data[pt->memorial_id-1].state == MDSTATE_IDLE)
			return MDCREATE_RESERVED;
		return MDCREATE_EXISTS;
	}

	for(i = 0; i < MAX_MEMORIAL_DATA; i++) {
		if(memorial_data[i].state == MDSTATE_FREE)
			break;
	}
	// �������A���_���W������������MAX
	if(i >= MAX_MEMORIAL_DATA)
		return MDCREATE_ERROR;

	// �\�񐔂��ő�
	if(memorial_wait.count >= MAX_MEMORIAL_DATA)
		return MDCREATE_ERROR;

	// �������A���_���W�������ݒ�
	memorial_data[i].type = db->type;
	memorial_data[i].state = MDSTATE_IDLE;
	memorial_data[i].party_id = pt->party_id;
	memorial_data[i].keep_limit = 0;
	memorial_data[i].keep_timer = -1;
	memorial_data[i].idle_limit = 0;
	memorial_data[i].idle_timer = -1;
	memorial_data[i].users = 0;
	memset(memorial_data[i].map, 0, sizeof(memorial_data[i].map));

	// �p�[�e�B�[���ݒ�
	pt->memorial_id = i + 1;

	// �\��ǉ�
	memorial_wait.id[memorial_wait.count++] = pt->memorial_id;
	if(memorial_wait.timer == -1)
		memorial_wait.timer = add_timer(gettick()+MEMORIAL_INVERVAL, memorial_subscription_timer, 0, NULL);

	// �p�[�e�B�[�ɏ��ʒm
	for(i = 0; i < MAX_PARTY; i++) {
		if(pt->member[i].sd) {
			clif_memorial_create(pt->member[i].sd, memorial_name, memorial_wait.count, 1);
			break;
		}
	}

	return MDCREATE_NOERROR;
}

/*==========================================
 * �������A���_���W�����폜
 *------------------------------------------
 */
int memorial_delete(int memorial_id)
{
	struct memorial_data *md;
	struct party *pt;
	int i, j, type = 0, count = 0;
	unsigned int now = (unsigned int)time(NULL);

	if(memorial_id <= 0 || memorial_id > MAX_MEMORIAL_DATA)
		return 1;

	md = &memorial_data[memorial_id-1];

	if(md->state == MDSTATE_FREE) {
		return 1;
	}

	// �\��
	if(md->state == MDSTATE_IDLE) {
		// �\��e�[�u������폜
		for(i = 0; i < memorial_wait.count; i++) {
			if(memorial_wait.id[i] == memorial_id) {
				// �\�񏇏��ύX
				memorial_wait.count--;
				memmove(&memorial_wait.id[i],&memorial_wait.id[i+1],sizeof(memorial_wait.id[0])*(memorial_wait.count-i));
				memset(&memorial_wait.id[memorial_wait.count], 0, sizeof(memorial_wait.id[0]));

				// �\�񏇏��ʒm
				for(i = 0; i < memorial_wait.count; i++) {
					if(memorial_data[memorial_wait.id[i]-1].state == MDSTATE_IDLE) {
						if((pt = party_search(memorial_data[memorial_wait.id[i]-1].party_id)) != NULL) {
							for(j = 0; j < MAX_PARTY; j++) {
								if(pt->member[j].sd) {
									clif_memorial_changewait(pt->member[j].sd, i+1, 1);
									break;
								}
							}
						}
					}
				}

				// �\��^�C�}�[�ĊJ�E��~
				if(memorial_wait.count)
					memorial_wait.timer = add_timer(gettick()+MEMORIAL_INVERVAL, memorial_subscription_timer, 0, NULL);
				else
					memorial_wait.timer = -1;
				type = 0;	// �������A���_���W�����\�񂪎�������܂����B
				break;
			}
		}
	}
	// �i�s��
	else {
		if(md->keep_limit && md->keep_limit <= now)
			type = 1;	// �������A���_���W�������������Ԓ��߂ɂ����ł��܂����B
		else if(md->idle_limit && md->idle_limit <= now)
			type = 2;	// �������A���_���W���������l��Ԉێ����Ԑ����ɂ����ł��܂����B
		else
			type = 3;	// �������A���_���W���������ł��܂����B

		// �}�b�v�폜
		for(i = 0; i < MAX_MEMORIAL_SEGMAP; i++)
			count += map_delmdmap(md->map[i].m);
	}

	// �^�C�}�[��~
	if(md->keep_timer != -1) {
		delete_timer(md->keep_timer, memorial_delete_timer);
		md->keep_timer = -1;
	}
	if(md->idle_timer != -1) {
		delete_timer(md->idle_timer, memorial_delete_timer);
		md->idle_timer = -1;
	}

	// �p�[�e�B�[�ɏ��ʒm
	pt = party_search(md->party_id);
	if(pt) {
		pt->memorial_id = 0;
		for(j = 0; j < MAX_PARTY; j++) {
			if(pt->member[j].sd) {
				if(type)
					clif_memorial_changestatus(pt->member[j].sd, type, 0, 1);
				else
					clif_memorial_changewait(pt->member[j].sd, 0xffff, 1);
				break;
			}
		}
	}

	// �������A���_���W������񏉊���
	md->type = 0;
	md->state = MDSTATE_FREE;
	md->party_id = 0;
	md->keep_limit = 0;
	md->idle_limit = 0;
	md->users = 0;
	memset(memorial_data[i].map, 0, sizeof(memorial_data[i].map));

	printf("memorial_delete: memorial_id=%03d count=%d\n",memorial_id, count);

	return 0;
}

/*==========================================
 * �������A���_���W��������
 *------------------------------------------
 */
int memorial_enter(struct map_session_data *sd, const char *memorial_name)
{
	struct memorial_data *md;
	struct memorial_db *db = memorial_searchname_db(memorial_name);
	struct party *pt;
	int m;

	nullpo_retr(MDENTER_ERROR, sd);

	if(db == NULL)
		return MDENTER_ERROR;

	// �p�[�e�B�[������
	if(sd->status.party_id == 0)
		return MDENTER_NOPARTY;
	if((pt = party_search(sd->status.party_id)) == NULL)
		return MDENTER_NOPARTY;

	// �������A���_���W����������
	if(pt->memorial_id == 0)
		return MDENTER_NOCREATE;

	md = &memorial_data[pt->memorial_id-1];
	if(md->party_id != pt->party_id)
		return MDENTER_NOCREATE;
	if(md->state != MDSTATE_BUSY)
		return MDENTER_NOCREATE;
	if(md->type != db->type)
		return MDENTER_NOCREATE;

	// �}�b�v�ړ�
	if((m = memorial_mapname2mapid(db->enter.mapname, pt->memorial_id)) < 0)
		return MDENTER_ERROR;
	if(pc_setpos(sd, map[m].name, db->enter.x, db->enter.y, 0))
		return MDENTER_ERROR;

	// ���l�����^�C�}�[����
	memorial_stopidletimer(md);

	// �ێ������^�C�}�[�J�n
	memorial_startkeeptimer(md, pt->memorial_id);

	return MDENTER_NOERROR;
}

/*==========================================
 * �������A���_���W�����̏��v��
 *------------------------------------------
 */
int memorial_reqinfo(struct map_session_data *sd, int memorial_id)
{
	struct memorial_data *md;
	struct memorial_db *db;
	int i;

	nullpo_retr(1, sd);

	if(memorial_id <= 0 || memorial_id > MAX_MEMORIAL_DATA)
		return 1;

	md = &memorial_data[memorial_id-1];

	if((db = memorial_searchtype_db(md->type)) == NULL)
		return 1;

	// �\����
	if(md->state == MDSTATE_IDLE) {
		for(i = 0; i < memorial_wait.count; i++) {
			if(memorial_wait.id[i] == memorial_id) {
				clif_memorial_create(sd, db->name, i+1, 0);
				break;
			}
		}
	}
	// �i�s���
	else if(md->state == MDSTATE_BUSY) {
		clif_memorial_status(sd, db->name, md->keep_limit, md->idle_limit, 0);
	}

	return 0;
}

/*==========================================
 * �������A���_���W�����̃��[�U�[�ǉ�
 *------------------------------------------
 */
int memorial_addusers(int memorial_id)
{
	struct memorial_data *md;

	if(memorial_id <= 0 || memorial_id > MAX_MEMORIAL_DATA)
		return 1;

	md = &memorial_data[memorial_id-1];
	if(md->state != MDSTATE_BUSY)
		return 1;

	// �������A���_���W�����̃��[�U�[���ǉ�
	md->users++;

	// ���l�����^�C�}�[���쓮���Ă������~
	memorial_stopidletimer(md);

	// �ێ������^�C�}�[����~���Ă�����쓮
	memorial_startkeeptimer(md, memorial_id);

	return 0;
}

/*==========================================
 * �������A���_���W�����̃��[�U�[�폜
 *------------------------------------------
 */
int memorial_delusers(int memorial_id)
{
	struct memorial_data *md;

	if(memorial_id <= 0 || memorial_id > MAX_MEMORIAL_DATA)
		return 1;

	md = &memorial_data[memorial_id-1];
	if(md->state != MDSTATE_BUSY)
		return 1;

	// �������A���_���W�����̃��[�U�[���폜
	md->users--;

	// ���l��ԂŖ��l�����^�C�}�[����~���Ă�����쓮
	if(md->users <= 0)
		memorial_startidletimer(md, memorial_id);

	return 0;
}

/*==========================================
 * �}�b�v���ƃ������A��ID����}�b�v�ԍ��擾
 *------------------------------------------
 */
int memorial_mapname2mapid(const char *name, int memorial_id)
{
	struct memorial_data *md;
	int m = map_mapname2mapid(name);
	int i;

	if(memorial_id <= 0 || memorial_id > MAX_MEMORIAL_DATA)
		return m;

	md = &memorial_data[memorial_id-1];
	if(md->state != MDSTATE_BUSY)
		return m;

	for(i = 0; i < MAX_MEMORIAL_SEGMAP; i++) {
		if(md->map[i].src_m == m)
			return md->map[i].m;
	}

	return m;
}

//
// ��������
//
/*==========================================
 * �������A���_���W������{�f�[�^�ǂݍ���
 *------------------------------------------
 */
static int read_memorial_db(void)
{
	int i = 0, j, k;
	FILE *fp;
	char line[1024],*p;

	memset(&memorial_db, 0, sizeof(memorial_db));

	fp=fopen("db/memorial_db.txt","r");
	if(fp==NULL){
		printf("can't read db/memorial_db.txt\n");
		return 1;
	}
	while(fgets(line,1020,fp)){
		char *split[6+MAX_MEMORIAL_SEGMAP];
		if(line[0] == '\0' || line[0] == '\r' || line[0] == '\n')
			continue;
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<6+MAX_MEMORIAL_SEGMAP && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(j < 6+MAX_MEMORIAL_SEGMAP)
			continue;

		memorial_db[i].type = (short)atoi(split[0]);
		strncpy(memorial_db[i].name,split[1],61);
		memorial_db[i].limit = atoi(split[2]);
		strncpy(memorial_db[i].enter.mapname,split[3],24);
		memorial_db[i].enter.x = (short)atoi(split[4]);
		memorial_db[i].enter.y = (short)atoi(split[5]);

		for(k = 0; k < MAX_MEMORIAL_SEGMAP; k++) {
			strncpy(memorial_db[i].mapname[k],split[6+k],24);
			memorial_db[i].mapname[k][23] = '\0';
		}

		// force \0 terminal
		memorial_db[i].name[60] = '\0';
		memorial_db[i].enter.mapname[23] = '\0';

		if(++i >= MAX_MEMORIAL_DB)
			break;
	}
	fclose(fp);
	printf("read db/memorial_db.txt done (count=%d)\n",i);

	return 0;
}

/*==========================================
 * ����������
 *------------------------------------------
 */
int do_init_memorial(void)
{
	read_memorial_db();
	memset(memorial_data, 0, sizeof(memorial_data));
	memset(&memorial_wait, 0, sizeof(memorial_wait));
	memorial_wait.timer = -1;

	add_timer_func_list(memorial_delete_timer);
	add_timer_func_list(memorial_subscription_timer);

	return 0;
}

/*==========================================
 * �I��
 *------------------------------------------
 */
int do_final_memorial(void)
{
	int i;

	for(i = 1; i <= MAX_MEMORIAL_DATA; i++)
		memorial_delete(i);

	return 0;
}
