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
#include <time.h>

#include "db.h"
#include "malloc.h"
#include "nullpo.h"

#include "battle.h"
#include "booking.h"
#include "clif.h"
#include "map.h"

static struct dbt* booking_db = NULL;
static int booking_mapid[MAX_BOOKING_MAPID];
static int booking_jobid[MAX_BOOKING_JOBID];
static int booking_id = 0;

/*==========================================
 * MAPID���L�����`�F�b�N
 *------------------------------------------
 */
static int booking_search_mapid(int map)
{
	int min = -1;
	int max = MAX_BOOKING_MAPID;

	// binary search
	while(max - min > 1) {
		int mid = (min + max) / 2;
		if(booking_mapid[mid] == map)
			return 0;

		if(booking_mapid[mid] > map)
			max = mid;
		else
			min = mid;
	}
	return 1;
}

/*==========================================
 * JOBID���L�����`�F�b�N
 *------------------------------------------
 */
static int booking_search_jobid(int job)
{
	int min = -1;
	int max = MAX_BOOKING_JOBID;

	if(job == 0)
		return 1;

	// binary search
	while(max - min > 1) {
		int mid = (min + max) / 2;
		if(booking_jobid[mid] == job)
			return 0;

		if(booking_jobid[mid] > job)
			max = mid;
		else
			min = mid;
	}
	return 1;
}

/*==========================================
 * �Y��ID�̃u�b�L���O���X�g����
 *------------------------------------------
 */
static struct booking_data *booking_search(int booking_id)
{
	return (struct booking_data *)numdb_search(booking_db,booking_id);
}

/*==========================================
 * �����Ńu�b�L���O���X�g����
 *------------------------------------------
 */
static int booking_searchcond_sub(void *key,void *data,va_list ap)
{
	struct booking_data *bd=(struct booking_data *)data;
	int lv           = va_arg(ap,int);
	int map          = va_arg(ap,int);
	int job          = va_arg(ap,int);
	int last_index   = va_arg(ap,int);
	int result_count = va_arg(ap,int);
	int *count       = va_arg(ap,int *);
	struct booking_data **list = va_arg(ap,struct booking_data **);
	int i;

	if(lv > 0 && (bd->lv < lv - battle_config.party_booking_lv || bd->lv > lv))	// Lv�������ƍ���Ȃ�
		return 0;

	if(bd->id < last_index)	// last_index���Ⴂ�Ȃ疳��
		return 0;

	if((*count) >= result_count)	// result_count�܂łŌ����I��
		return 1;

	if(map == 0) {	// ��WMAP��������
		if(job == 0xffff) {
			list[(*count)++] = bd;
		} else {
			for(i=0; i<6; i++) {
				if(bd->job[i] == job) {
					list[(*count)++] = bd;
					break;
				}
			}
		}
	} else if(job == 0xffff) {	// ��W�E�Ƃ��w�薳��
		if(bd->map == map)
			list[(*count)++] = bd;
	}

	return 0;
}

void booking_searchcond(struct map_session_data *sd, int lv, int map, int job, int last_index, int result_count)
{
	int flag;
	int count=0;
	struct booking_data *list[MAX_RESULT];	// result_count�ɍő匟�������i�[����Ă��邪�A�l���ǂ�Ȏ��ɕς��̂�(0xa���������Ă��Ȃ�)�s���Ȃ̂ŁA�s���΍�̂��߂Ƃ肠�����萔��p��

	nullpo_retv(sd);

	if(lv > MAX_LEVEL || lv < 0)	// ���x�����s��
		return;
	if(booking_search_mapid(map))	// �L����MAPID�ł͂Ȃ�
		return;
	if(booking_search_jobid(job))	// �L����JOBID�ł͂Ȃ�
		return;

	memset(list,0,sizeof(list));

	flag = numdb_foreach(booking_db,booking_searchcond_sub,lv,map,job,last_index,MAX_RESULT,&count,&list);
	clif_searchbookingack(sd,list,count,flag);

	return;
}

/*==========================================
 * �p�[�e�B�[�u�b�L���O�o�^
 *------------------------------------------
 */
void booking_register(struct map_session_data *sd, int lv, int map, int *job)
{
	struct booking_data *bd;
	int i;

	nullpo_retv(sd);

	if(sd->booking_id)	// ���ɓo�^��
		return;
	if(lv > MAX_LEVEL || lv < 0)	// ���x�����s��
		return;
	if(booking_search_mapid(map))	// �L����MAPID�ł͂Ȃ�
		return;
	for(i=0; i<6; i++) {
		if(booking_search_jobid(job[i]))	// �L����JOBID�ł͂Ȃ�
			return;
	}

	// PT�u�b�L���O���X�g�ɓo�^
	bd = (struct booking_data *)aCalloc(1,sizeof(struct booking_data));
	bd->id = ++booking_id;
	numdb_insert(booking_db,booking_id,bd);
	memcpy(bd->name,sd->status.name,24);
	bd->time = (unsigned int)time(NULL);
	bd->lv = lv;
	bd->map = map;

	for(i=0; i<6; i++) {
		if(job[i] != 0xffff)
			bd->job[i] = job[i];
		else
			bd->job[i] = -1;
	}

	sd->booking_id = bd->id;

	clif_bookingregack(sd,0);
	clif_insertbookinglist(sd,bd);

	return;
}

/*==========================================
 * �p�[�e�B�[�u�b�L���O�o�^���C��
 *------------------------------------------
 */
void booking_update(struct map_session_data *sd, int *job)
{
	struct booking_data *bd;
	int i;

	nullpo_retv(sd);

	bd = booking_search(sd->booking_id);
	if(bd == NULL)
		return;
	
	for(i=0; i<6; i++) {
		if(booking_search_jobid(job[i]))	// �L����JOBID�ł͂Ȃ�
			return;
		if(job[i] != 0xffff)
			bd->job[i] = job[i];
		else
			bd->job[i] = -1;
	}
	bd->time = (unsigned int)time(NULL);

	clif_updatebookinglist(sd,bd);

	return;
}

/*==========================================
 * �p�[�e�B�[�u�b�L���O���X�g�폜
 *------------------------------------------
 */
void booking_delete(struct map_session_data *sd)
{
	struct booking_data *bd;

	nullpo_retv(sd);

	bd = booking_search(sd->booking_id);
	if(bd == NULL)
		return;

	clif_deletebookingack(sd,0);
	clif_deletebooking(sd,bd->id);
	numdb_erase(booking_db,sd->booking_id);
	aFree(bd);
	sd->booking_id = 0;

	return;
}

/*==========================================
 * �p�[�e�B�[�u�b�L���O�f�[�^�ǂݍ���
 *------------------------------------------
 */
static int read_booking_db(void)
{
	FILE *fp;
	char line[1024];
	char *str=NULL;
	int i=0,j=0;

	memset(booking_mapid,0,sizeof(booking_mapid));

	fp=fopen("db/booking_map_db.txt","r");
	if(fp==NULL) {
		printf("can't read db/booking_map_db.txt\n");
		return -1;
	}

	while(fgets(line,sizeof(line),fp) && i < MAX_BOOKING_MAPID) {
		if(line[0] == '\0' || line[0] == '\r' || line[0] == '\n')
			continue;
		if(line[0] == '/' && line[1] == '/')
			continue;

		str=line;
		if(str == NULL)
			continue;
		booking_mapid[i] = atoi(str);
		i++;
	}

	fclose(fp);
	printf("read db/booking_map_db.txt done (count=%d)\n",i);

	memset(booking_jobid,0,sizeof(booking_jobid));

	fp=fopen("db/booking_job_db.txt","r");
	if(fp==NULL) {
		printf("can't read db/booking_job_db.txt\n");
		return -1;
	}

	while(fgets(line,sizeof(line),fp) && j < MAX_BOOKING_JOBID) {
		if(line[0] == '\0' || line[0] == '\r' || line[0] == '\n')
			continue;
		if(line[0] == '/' && line[1] == '/')
			continue;

		str=line;
		if(str == NULL)
			continue;
		booking_jobid[j] = atoi(str);
		j++;
	}

	fclose(fp);
	printf("read db/booking_job_db.txt done (count=%d)\n",j);

	return 0;
}

/*==========================================
 * �I��
 *------------------------------------------
 */
static int booking_db_final(void *key,void *data,va_list ap)
{
	aFree(data);

	return 0;
}

void do_final_booking(void)
{
	if(booking_db)
		numdb_final(booking_db,booking_db_final);

	return;
}

/*==========================================
 * ������
 *------------------------------------------
 */
void do_init_booking(void)
{
	booking_db=numdb_init();
	read_booking_db();

	return;
}
