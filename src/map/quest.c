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

#include "nullpo.h"

#include "map.h"
#include "quest.h"
#include "clif.h"
#include "chrif.h"
#include "intif.h"
#include "pc.h"

/* �N�G�X�g�f�[�^�x�[�X */
struct quest_db quest_db[MAX_QUEST_DB];
/* �����Ώۃf�[�^�x�[�X */
int quest_killdb[MAX_QUEST_DB];

/*==========================================
 * �N�G�X�gDB�̃f�[�^������
 *------------------------------------------
 */
int quest_search_db(int quest_id)
{
	int i;

	for(i=0; i < MAX_QUEST_DB; i++) {
		if(quest_db[i].nameid == quest_id)
			return i;
	}

	return -1;
}

/*==========================================
 * �N�G�X�gID����C���f�b�N�X��Ԃ�
 *------------------------------------------
 */
int quest_search_index(struct map_session_data *sd, int quest_id)
{
	int i;

	nullpo_retr(-1, sd);

	for(i=0; i < sd->questlist; i++) {
		if(sd->quest[i].nameid == quest_id)
			return i;
	}

	return -1;
}

/*==========================================
 * �N�G�X�gID����\���̂�Ԃ�
 *------------------------------------------
 */
struct quest_data *quest_get_data(struct map_session_data *sd, int quest_id)
{
	int idx = quest_search_index(sd, quest_id);

	if(idx >= 0)
		return &sd->quest[idx];

	return NULL;
}

/*==========================================
 * �����Ώۂ��`�F�b�N
 *------------------------------------------
 */
int quest_search_mobid(int mob_id)
{
	int min = -1;
	int max = MAX_QUEST_DB;

	// binary search
	while(max - min > 1) {
		int mid = (min + max) / 2;
		if(quest_killdb[mid] == mob_id)
			return 1;

		if(quest_killdb[mid] > mob_id)
			max = mid;
		else
			min = mid;
	}
	return 0;
}

/*==========================================
 * �N�G�X�g���X�g�ǉ�
 *------------------------------------------
 */
int quest_addlist(struct map_session_data *sd, int quest_id)
{
	struct quest_data qd;
	int i, qid, idx;

	nullpo_retr(1, sd);

	qid = quest_search_db(quest_id);
	if(qid < 0)
		return 1;

	memset(&qd, 0, sizeof(struct quest_data));
	qd.nameid = quest_db[qid].nameid;
	if(quest_db[qid].limit)
		qd.limit = (unsigned int)time(NULL) + quest_db[qid].limit;
	for(i = 0; i < sizeof(qd.mob)/sizeof(qd.mob[0]); i++) {
		qd.mob[i].id  = quest_db[qid].mob[i].id;
		qd.mob[i].max = quest_db[qid].mob[i].count;
	}
	qd.state = 1;

	// ���ɃN�G�X�g���擾���Ă邩����
	if((idx = quest_search_index(sd, quest_id)) >= 0) {
		// ���ɃN�G�X�g���X�g�ɂ���ꍇ�͍X�V����
		memcpy(&sd->quest[idx], &qd, sizeof(struct quest_data));
	} else {
		if(sd->questlist >= MAX_QUESTLIST) {
			// �e�ʃI�[�o�[
			return 1;
		}
		memcpy(&sd->quest[sd->questlist], &qd, sizeof(struct quest_data));
		sd->questlist++;
	}

	clif_add_questlist(sd, quest_id);
	clif_questlist(sd);
	clif_questlist_info(sd);
	intif_save_quest(sd);

	return 0;
}

/*==========================================
 * �N�G�X�g���X�g�X�V
 *------------------------------------------
 */
int quest_updatelist(struct map_session_data *sd, int old_id, int new_id)
{
	struct quest_data qd;
	int i, qid, old_idx, new_idx;

	nullpo_retr(1, sd);

	qid = quest_search_db(new_id);
	if(qid < 0)
		return 1;

	// �N�G�X�g���擾���Ă邩����
	old_idx = quest_search_index(sd, old_id);
	new_idx = quest_search_index(sd, new_id);

	if(old_idx >= 0) {
		if(new_idx >= 0) {
			// �V�N�G�X�g�����ɃN�G�X�g���X�g�ɂ���ꍇ�͋��N�G�X�g���폜���邾��
			quest_dellist(sd, old_id);
			return 0;
		}
	} else {
		if(new_idx >= 0) {
			// �V�N�G�X�g�����ɃN�G�X�g���X�g�ɂ���ꍇ�͉������Ȃ�
			return 0;
		}
		if(sd->questlist >= MAX_QUESTLIST) {
			// �e�ʃI�[�o�[
			return 1;
		}
	}

	memset(&qd, 0, sizeof(struct quest_data));
	qd.nameid = quest_db[qid].nameid;
	if(quest_db[qid].limit)
		qd.limit = (unsigned int)time(NULL) + quest_db[qid].limit;
	for(i = 0; i < sizeof(qd.mob)/sizeof(qd.mob[0]); i++) {
		qd.mob[i].id  = quest_db[qid].mob[i].id;
		qd.mob[i].max = quest_db[qid].mob[i].count;
	}
	qd.state = 1;

	if(old_idx >= 0) {
		// ���N�G�X�g���擾���Ă�ꍇ�͏㏑���ōX�V����
		memcpy(&sd->quest[old_idx], &qd, sizeof(struct quest_data));
	} else {
		memcpy(&sd->quest[sd->questlist], &qd, sizeof(struct quest_data));
		sd->questlist++;
	}

	clif_add_questlist(sd, new_id);
	clif_questlist(sd);
	clif_questlist_info(sd);
	intif_save_quest(sd);

	return 0;
}

/*==========================================
 * �N�G�X�g���X�g�폜
 *------------------------------------------
 */
int quest_dellist(struct map_session_data *sd, int quest_id)
{
	int idx;

	nullpo_retr(1, sd);

	idx = quest_search_index(sd, quest_id);
	if(idx >= MAX_QUESTLIST || idx < 0)
		return 1;

	sd->questlist--;
	memmove(&sd->quest[idx],&sd->quest[idx+1],sizeof(struct quest_data) * (sd->questlist - idx));
	memset(&sd->quest[sd->questlist], 0, sizeof(struct quest_data));

	clif_del_questlist(sd, quest_id);
	clif_questlist(sd);
	clif_questlist_info(sd);
	intif_save_quest(sd);

	return 0;
}

int quest_killcount_sub(struct block_list *tbl, va_list ap)
{
	struct map_session_data *sd;
	int mob_id, party_id;

	nullpo_retr(0, tbl);
	nullpo_retr(0, sd = (struct map_session_data *)tbl);

	party_id = va_arg(ap,int);
	mob_id = va_arg(ap,int);

	if(!sd->questlist)
		return 0;
	if(sd->status.party_id != party_id)
		return 0;

	quest_killcount(sd, mob_id);

	return 1;
}

/*==========================================
 * �N�G�X�g���X�g�������X�V
 *------------------------------------------
 */
int quest_killcount(struct map_session_data *sd, int mob_id)
{
	struct quest_data *qd;
	int i, j;

	nullpo_retr(1, sd);

	for(i = 0; i < sd->questlist; i++) {
		qd = &sd->quest[i];
		if(qd->nameid > 0) {
			for(j = 0; j < sizeof(qd->mob)/sizeof(qd->mob[0]); j++) {
				if(qd->mob[j].id == mob_id && qd->mob[j].count < qd->mob[j].max) {
					qd->mob[j].count++;
					clif_update_questcount(sd, qd->nameid);
				}
			}
		}
	}

	return 0;
}

/*==========================================
 * �����f�[�^�x�[�X�̃\�[�g
 *------------------------------------------
 */
static int quest_sort_id(const void *_i1, const void *_i2)
{
	int i1 = *((int *)_i1);
	int i2 = *((int *)_i2);

	return (i1 > i2)? 1 : (i1 < i2)? -1 : 0;
}

/*==========================================
 * �N�G�X�g�ݒ�t�@�C���ǂݍ���
 * quest_db.txt �N�G�X�g�f�[�^
 *------------------------------------------
 */
static int quest_readdb(void)
{
	int i,j,k;
	FILE *fp;
	char line[1024],*p;

	memset(&quest_db, 0, sizeof(quest_db));
	memset(&quest_killdb, 0, sizeof(quest_killdb));

	fp=fopen("db/quest_db.txt","r");
	if(fp==NULL){
		printf("can't read db/quest_db.txt\n");
		return 1;
	}
	i=0;
	k=-1;
	while(fgets(line,1020,fp)){
		char *split[9];
		if(line[0] == '\0' || line[0] == '\r' || line[0] == '\n')
			continue;
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<9 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(j < 9)
			continue;
		if(i < 0 || i >= MAX_QUEST_DB)
			continue;

		quest_db[i].nameid = atoi(split[0]);
		quest_db[i].limit  = atoi(split[2]);
		for(j = 0; j < sizeof(quest_db[0].mob)/sizeof(quest_db[0].mob[0]); j++) {
			quest_db[i].mob[j].id    = (short)atoi(split[3+j*2]);
			quest_db[i].mob[j].count = (short)atoi(split[4+j*2]);

			if(++k >= MAX_QUEST_DB)
				continue;
			quest_killdb[k]          = quest_db[i].mob[j].id;
		}

		if(++i >= MAX_QUEST_DB)
			break;
	}
	// �����f�[�^�x�[�X�̃\�[�g
	qsort(quest_killdb, MAX_QUEST_DB, sizeof(int), quest_sort_id);

	fclose(fp);
	printf("read db/quest_db.txt done (count=%d)\n",i);

	return 0;
}

/*==========================================
 * �����[�h
 *------------------------------------------
 */
void quest_reload(void)
{
	quest_readdb();
}

/*==========================================
 * �N�G�X�g����������
 *------------------------------------------
 */
int do_init_quest(void)
{
	quest_readdb();

	return 0;
}
