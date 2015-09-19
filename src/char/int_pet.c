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

#define _INT_PET_C_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mmo.h"
#include "socket.h"
#include "db.h"
#include "lock.h"
#include "malloc.h"
#include "journal.h"
#include "sqldbs.h"

#include "char.h"
#include "inter.h"
#include "int_pet.h"

static struct dbt *pet_db = NULL;

#ifdef TXT_ONLY

static char pet_txt[1024]="save/pet.txt";
static int pet_newid = 100;

#ifdef TXT_JOURNAL
static int pet_journal_enable = 1;
static struct journal pet_journal;
static char pet_journal_file[1024]="./save/pet.journal";
static int pet_journal_cache = 1000;
#endif

void pet_txt_config_read_sub(const char* w1,const char *w2)
{
	if(strcmpi(w1,"pet_txt")==0){
		strncpy(pet_txt, w2, sizeof(pet_txt) - 1);
	}
#ifdef TXT_JOURNAL
	else if(strcmpi(w1,"pet_journal_enable")==0){
		pet_journal_enable = atoi( w2 );
	}
	else if(strcmpi(w1,"pet_journal_file")==0){
		strncpy( pet_journal_file, w2, sizeof(pet_journal_file) - 1 );
	}
	else if(strcmpi(w1,"pet_journal_cache_interval")==0){
		pet_journal_cache = atoi( w2 );
	}
#endif
}

static int pet_tostr(char *str,struct s_pet *p)
{
	if(p->hungry < 0)
		p->hungry = 0;
	else if(p->hungry > 100)
		p->hungry = 100;
	if(p->intimate < 0)
		p->intimate = 0;
	else if(p->intimate > 1000)
		p->intimate = 1000;

	sprintf(str,"%d,%d,%s\t%d,%d,%d,%d,%d,%d,%d,%d,%d",
		p->pet_id,p->class_,p->name,p->account_id,p->char_id,p->level,p->egg_id,
		p->equip,p->intimate,p->hungry,p->rename_flag,p->incubate);

	return 0;
}

static int pet_fromstr(char *str,struct s_pet *p)
{
	int s;
	int tmp_int[11];
	char tmp_str[256];

	memset(p,0,sizeof(struct s_pet));

	s=sscanf(str,"%d,%d,%255[^\t]\t%d,%d,%d,%d,%d,%d,%d,%d,%d",
		&tmp_int[0],&tmp_int[1],tmp_str,&tmp_int[2],
		&tmp_int[3],&tmp_int[4],&tmp_int[5],&tmp_int[6],&tmp_int[7],&tmp_int[8],&tmp_int[9],&tmp_int[10]);

	if(s!=12)
		return 1;

	p->pet_id      = tmp_int[0];
	p->class_      = tmp_int[1];
	memcpy(p->name,tmp_str,24);
	p->name[23] = '\0';	// force \0 terminal
	p->account_id  = tmp_int[2];
	p->char_id     = tmp_int[3];
	p->level       = tmp_int[4];
	p->egg_id      = tmp_int[5];
	p->equip       = tmp_int[6];
	p->intimate    = tmp_int[7];
	p->hungry      = tmp_int[8];
	p->rename_flag = tmp_int[9];
	p->incubate    = tmp_int[10];

	if(p->hungry < 0)
		p->hungry = 0;
	else if(p->hungry > 100)
		p->hungry = 100;
	if(p->intimate < 0)
		p->intimate = 0;
	else if(p->intimate > 1000)
		p->intimate = 1000;

	return 0;
}

#ifdef TXT_JOURNAL
// ==========================================
// �y�b�g�f�[�^�̃W���[�i���̃��[���t�H���[�h�p�R�[���o�b�N�֐�
// ------------------------------------------
int pet_journal_rollforward( int key, void* buf, int flag )
{
	struct s_pet* p = (struct s_pet *)numdb_search( pet_db, key );

	// �O�̂��߃`�F�b�N
	if( flag == JOURNAL_FLAG_WRITE && key != ((struct s_pet*)buf)->pet_id )
	{
		printf("int_pet: journal: key != pet_id !\n");
		return 0;
	}

	// �f�[�^�̒u������
	if( p )
	{
		if( flag == JOURNAL_FLAG_DELETE ) {
			numdb_erase(pet_db, key);
			aFree(p);
		} else {
			memcpy( p, buf, sizeof(struct s_pet) );
		}
		return 1;
	}

	// �ǉ�
	if( flag != JOURNAL_FLAG_DELETE )
	{
		p = (struct s_pet*)aCalloc( 1, sizeof( struct s_pet ) );
		memcpy( p, buf, sizeof(struct s_pet) );
		numdb_insert( pet_db, key, p );
		if( p->pet_id >= pet_newid)
			pet_newid=p->pet_id+1;
		return 1;
	}

	return 0;
}
int pet_txt_sync(void);
#endif

int pet_txt_init(void)
{
	char line[8192];
	struct s_pet *p;
	FILE *fp;
	int c=0;

	pet_db=numdb_init();

	if( (fp=fopen(pet_txt,"r"))==NULL )
		return 1;
	while(fgets(line,sizeof(line),fp)){
		p=(struct s_pet *)aCalloc(1,sizeof(struct s_pet));
		if(pet_fromstr(line,p)==0 && p->pet_id>0){
			if( p->pet_id >= pet_newid)
				pet_newid=p->pet_id+1;
			numdb_insert(pet_db,p->pet_id,p);
		}else{
			printf("int_pet: broken data [%s] line %d\n",pet_txt,c);
			aFree(p);
		}
		c++;
	}
	fclose(fp);

#ifdef TXT_JOURNAL
	if( pet_journal_enable )
	{
		// �W���[�i���f�[�^�̃��[���t�H���[�h
		if( journal_load( &pet_journal, sizeof(struct s_pet), pet_journal_file ) )
		{
			int c = journal_rollforward( &pet_journal, pet_journal_rollforward );

			printf("int_pet: journal: roll-forward (%d)\n", c );

			// ���[���t�H���[�h�����̂ŁAtxt �f�[�^��ۑ����� ( journal ���V�K�쐬�����)
			pet_txt_sync();
		}
		else
		{
			// �W���[�i����V�K�쐬����
			journal_final( &pet_journal );
			journal_create( &pet_journal, sizeof(struct s_pet), pet_journal_cache, pet_journal_file );
		}
	}
#endif

	return 0;
}

static int pet_txt_sync_sub(void *key,void *data,va_list ap)
{
	char line[8192];
	FILE *fp;

	pet_tostr(line,(struct s_pet *)data);
	fp=va_arg(ap,FILE *);
	fprintf(fp,"%s" RETCODE,line);
	return 0;
}

int pet_txt_sync(void)
{
	FILE *fp;
	int lock;

	if( !pet_db )
		return 1;

	if( (fp=lock_fopen(pet_txt,&lock))==NULL ){
		printf("int_pet: cant write [%s] !!! data is lost !!!\n",pet_txt);
		return 1;
	}
	numdb_foreach(pet_db,pet_txt_sync_sub,fp);
	lock_fclose(fp,pet_txt,&lock);

#ifdef TXT_JOURNAL
	if( pet_journal_enable )
	{
		// �R�~�b�g�����̂ŃW���[�i����V�K�쐬����
		journal_final( &pet_journal );
		journal_create( &pet_journal, sizeof(struct s_pet), pet_journal_cache, pet_journal_file );
	}
#endif

	return 0;
}

int pet_txt_delete(int pet_id)
{
	struct s_pet *p = (struct s_pet *)numdb_search(pet_db,pet_id);

	if(p == NULL)
		return 1;

	numdb_erase(pet_db,pet_id);
	aFree(p);
	printf("pet_id: %d deleted\n",pet_id);

#ifdef TXT_JOURNAL
	if( pet_journal_enable )
		journal_write( &pet_journal, pet_id, NULL );
#endif

	return 0;
}

const struct s_pet* pet_txt_load(int pet_id)
{
	return (const struct s_pet *)numdb_search(pet_db,pet_id);
}

int pet_txt_save(struct s_pet* p2)
{
	struct s_pet* p1 = (struct s_pet *)numdb_search(pet_db,p2->pet_id);

	if(p1 == NULL) {
		p1 = (struct s_pet *)aMalloc(sizeof(struct s_pet));
		numdb_insert(pet_db,p1,p2->pet_id);
	}
	memcpy(p1,p2,sizeof(struct s_pet));

#ifdef TXT_JOURNAL
	if( pet_journal_enable )
		journal_write( &pet_journal, p1->pet_id, p1 );
#endif
	return 1;
}

int pet_txt_new(struct s_pet *p)
{
	p->pet_id = pet_newid++;
	numdb_insert(pet_db,p->pet_id,p);
	return 0;
}

static int pet_txt_final_sub(void *key,void *data,va_list ap)
{
	struct s_pet *p = (struct s_pet *)data;

	aFree(p);

	return 0;
}

void pet_txt_final(void)
{
	if(pet_db)
		numdb_final(pet_db,pet_txt_final_sub);

#ifdef TXT_JOURNAL
	if( pet_journal_enable )
	{
		journal_final( &pet_journal );
	}
#endif
}

#define pet_init   pet_txt_init
#define pet_sync   pet_txt_sync
#define pet_delete pet_txt_delete
#define pet_load   pet_txt_load
#define pet_save   pet_txt_save
#define pet_new    pet_txt_new
#define pet_final  pet_txt_final

#else /* TXT_ONLY */

int pet_sql_init(void)
{
	pet_db = numdb_init();
	return 0;
}

int pet_sql_sync(void)
{
	// nothing to do
	return 0;
}

int pet_sql_delete(int pet_id)
{
	struct s_pet *p;

	sqldbs_query(&mysql_handle, "DELETE FROM `" PET_TABLE "` WHERE `pet_id`='%d'", pet_id);

	p = (struct s_pet *)numdb_search(pet_db,pet_id);
	if(p) {
		numdb_erase(pet_db,p->pet_id);
		aFree(p);
	} else {
		if(sqldbs_affected_rows(&mysql_handle) <= 0)
			return 1;	// SQL����폜�ł��Ȃ��Ƃ�����fail
	}

	printf("pet_id: %d deleted\n",pet_id);

	return 0;
}

const struct s_pet* pet_sql_load(int pet_id)
{
	int rc;
	MYSQL_RES* sql_res;
	MYSQL_ROW  sql_row = NULL;
	struct s_pet *p = (struct s_pet *)numdb_search(pet_db,pet_id);

	if(p && p->pet_id == pet_id) {
		return p;
	}
	if(p == NULL) {
		p = (struct s_pet *)aMalloc(sizeof(struct s_pet));
		numdb_insert(pet_db,pet_id,p);
	}
	memset(p, 0, sizeof(struct s_pet));

	rc = sqldbs_query(
		&mysql_handle,
		"SELECT `class`,`name`,`account_id`,`char_id`,`level`,`egg_id`,`equip`,"
		"`intimate`,`hungry`,`rename_flag`,`incubate` FROM `" PET_TABLE "` WHERE `pet_id`='%d'",
		pet_id
	);
	if(rc) {
		p->pet_id = -1;
		return NULL;
	}

	sql_res = sqldbs_store_result(&mysql_handle);
	if (sql_res!=NULL && sqldbs_num_rows(sql_res)>0) {
		sql_row = sqldbs_fetch(sql_res);

		p->pet_id      = pet_id;
		p->class_      = atoi(sql_row[0]);
		strncpy(p->name, sql_row[1], 24);
		p->name[23] = '\0';	// force \0 terminal
		p->account_id  = atoi(sql_row[2]);
		p->char_id     = atoi(sql_row[3]);
		p->level       = atoi(sql_row[4]);
		p->egg_id      = atoi(sql_row[5]);
		p->equip       = atoi(sql_row[6]);
		p->intimate    = atoi(sql_row[7]);
		p->hungry      = atoi(sql_row[8]);
		p->rename_flag = atoi(sql_row[9]);
		p->incubate    = atoi(sql_row[10]);
	} else {
		p->pet_id = -1;
		if( sql_res ) sqldbs_free_result(sql_res);
		return NULL;
	}
	if(p->hungry < 0)
		p->hungry = 0;
	else if(p->hungry > 100)
		p->hungry = 100;
	if(p->intimate < 0)
		p->intimate = 0;
	else if(p->intimate > 1000)
		p->intimate = 1000;

	sqldbs_free_result(sql_res);

	return p;
}

int pet_sql_save(struct s_pet* p2)
{
	char t_name[64];
	const struct s_pet *p1 = pet_sql_load(p2->pet_id);

	if(p1 == NULL)
		return 0;

	if(memcmp(p1,p2,sizeof(struct s_pet))) {
		sqldbs_query(
			&mysql_handle,
			"UPDATE `" PET_TABLE "` SET `class`='%d',`name`='%s',`account_id`='%d',`char_id`='%d',`level`='%d',`egg_id`='%d',"
			"`equip`='%d',`intimate`='%d',`hungry`='%d',`rename_flag`='%d',`incubate`='%d' WHERE `pet_id`='%d'",
			p2->class_, strecpy(t_name, p2->name), p2->account_id, p2->char_id, p2->level, p2->egg_id,
			p2->equip, p2->intimate, p2->hungry, p2->rename_flag, p2->incubate, p2->pet_id
		);
	}

	{
		struct s_pet *p3 = (struct s_pet *)numdb_search(pet_db,p2->pet_id);
		if(p3)
			memcpy(p3,p2,sizeof(struct s_pet));
	}
	return 1;
}

int pet_sql_new(struct s_pet *p)
{
	// �y�b�gID��ǂݏo��
	int rc;
	char t_name[64];

	rc = sqldbs_query(
		&mysql_handle,
		"INSERT INTO `" PET_TABLE "` (`class`,`name`,`account_id`,`char_id`,`level`,`egg_id`,"
		"`equip`,`intimate`,`hungry`,`rename_flag`,`incubate`) VALUES ('%d', '%s', '%d',"
		"'%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d')",
		p->class_, strecpy(t_name, p->name), p->account_id, p->char_id, p->level, p->egg_id,
		p->equip, p->intimate, p->hungry, p->rename_flag, p->incubate
	);
	if(rc){
		aFree(p);
		return 1;
	}

	p->pet_id = (int)sqldbs_insert_id(&mysql_handle);

	numdb_insert(pet_db,p->pet_id,p);
	return 0;
}

static int pet_sql_final_sub(void *key,void *data,va_list ap)
{
	struct s_pet *p = (struct s_pet *)data;

	aFree(p);

	return 0;
}

void pet_sql_final(void)
{
	if(pet_db)
		numdb_final(pet_db,pet_sql_final_sub);
}

void pet_sql_config_read_sub(const char* w1,const char *w2)
{
	// nothing to do
	return;
}

#define pet_init   pet_sql_init
#define pet_sync   pet_sql_sync
#define pet_delete pet_sql_delete
#define pet_load   pet_sql_load
#define pet_save   pet_sql_save
#define pet_new    pet_sql_new
#define pet_final  pet_sql_final

#endif

int mapif_pet_created(int fd,int account_id,struct s_pet *p)
{
	WFIFOW(fd,0)=0x3880;
	WFIFOL(fd,2)=account_id;
	if(p!=NULL){
		WFIFOB(fd,6)=0;
		WFIFOL(fd,7)=p->pet_id;
		printf("int_pet: created! %d %s\n",p->pet_id,p->name);
	}else{
		WFIFOB(fd,6)=1;
		WFIFOL(fd,7)=0;
	}
	WFIFOSET(fd,11);

	return 0;
}

int mapif_pet_info(int fd,int account_id,const struct s_pet *p)
{
	WFIFOW(fd,0)=0x3881;
	WFIFOW(fd,2)=sizeof(struct s_pet) + 9;
	WFIFOL(fd,4)=account_id;
	WFIFOB(fd,8)=0;
	memcpy(WFIFOP(fd,9),p,sizeof(struct s_pet));
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

int mapif_pet_noinfo(int fd,int account_id)
{
	WFIFOW(fd,0)=0x3881;
	WFIFOW(fd,2)=sizeof(struct s_pet) + 9;
	WFIFOL(fd,4)=account_id;
	WFIFOB(fd,8)=1;
	memset(WFIFOP(fd,9),0,sizeof(struct s_pet));
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

int mapif_save_pet_ack(int fd,int account_id,int flag)
{
	WFIFOW(fd,0)=0x3882;
	WFIFOL(fd,2)=account_id;
	WFIFOB(fd,6)=flag;
	WFIFOSET(fd,7);

	return 0;
}

int mapif_delete_pet_ack(int fd,int flag)
{
	WFIFOW(fd,0)=0x3883;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,3);

	return 0;
}

int mapif_create_pet(int fd,int account_id,int char_id,short pet_class,unsigned short pet_lv,short pet_egg_id,
	short pet_equip,short intimate,short hungry,char rename_flag,char incubate,char *pet_name)
{
	struct s_pet *p;

	p=(struct s_pet *)aCalloc(1,sizeof(struct s_pet));
	memcpy(p->name,pet_name,24);
	if(incubate == 1) {
		p->account_id = 0;
		p->char_id    = 0;
	} else {
		p->account_id = account_id;
		p->char_id    = char_id;
	}
	p->class_      = pet_class;
	p->level       = pet_lv;
	p->egg_id      = pet_egg_id;
	p->equip       = pet_equip;
	p->intimate    = intimate;
	p->hungry      = hungry;
	p->rename_flag = rename_flag;
	p->incubate    = incubate;

	if(p->hungry < 0)
		p->hungry = 0;
	else if(p->hungry > 100)
		p->hungry = 100;
	if(p->intimate < 0)
		p->intimate = 0;
	else if(p->intimate > 1000)
		p->intimate = 1000;

	if(pet_new(p) == 0) {
		mapif_pet_created(fd,account_id,p);
	} else {
		aFree(p);
	}
	return 0;
}

int mapif_load_pet(int fd,int account_id,int char_id,int pet_id)
{
	const struct s_pet *p = pet_load(pet_id);

	if(p!=NULL) {
		if(p->incubate == 1)
			mapif_pet_info(fd,account_id,p);
		else if(account_id == p->account_id && char_id == p->char_id)
			mapif_pet_info(fd,account_id,p);
		else
			mapif_pet_noinfo(fd,account_id);
	} else {
		mapif_pet_noinfo(fd,account_id);
	}

	return 0;
}

int mapif_save_pet(int fd,int account_id,struct s_pet *data)
{
	if(sizeof(struct s_pet) != RFIFOW(fd,2) - 8) {
		printf("inter pet: data size error %d %d\n",sizeof(struct s_pet),RFIFOW(fd,2)-8);
		return 0;
	}

	if(pet_load(data->pet_id)) {
		if(data->hungry < 0)
			data->hungry = 0;
		else if(data->hungry > 100)
			data->hungry = 100;
		if(data->intimate < 0)
			data->intimate = 0;
		else if(data->intimate > 1000)
			data->intimate = 1000;
		if(data->incubate == 1)
			data->account_id = data->char_id = 0;
		pet_save(data);
		mapif_save_pet_ack(fd,account_id,0);
	}

	return 0;
}

int mapif_delete_pet(int fd,int pet_id)
{
	mapif_delete_pet_ack(fd,pet_delete(pet_id));

	return 0;
}

int mapif_parse_CreatePet(int fd)
{
	mapif_create_pet(
		fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOW(fd,10),RFIFOW(fd,12),RFIFOW(fd,14),
		RFIFOW(fd,16),RFIFOW(fd,18),RFIFOW(fd,20),RFIFOB(fd,22),RFIFOB(fd,23),RFIFOP(fd,24)
	);
	return 0;
}

int mapif_parse_LoadPet(int fd)
{
	mapif_load_pet(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10));
	return 0;
}

int mapif_parse_SavePet(int fd)
{
	mapif_save_pet(fd,RFIFOL(fd,4),(struct s_pet *)RFIFOP(fd,8));
	return 0;
}

int mapif_parse_DeletePet(int fd)
{
	mapif_delete_pet(fd,RFIFOL(fd,2));
	return 0;
}

// map server ����̒ʐM
// �E�P�p�P�b�g�̂݉�͂��邱��
// �E�p�P�b�g���f�[�^��inter.c�ɃZ�b�g���Ă�������
// �E�p�P�b�g���`�F�b�N��ARFIFOSKIP�͌Ăяo�����ōs����̂ōs���Ă͂Ȃ�Ȃ�
// �E�G���[�Ȃ�0(false)�A�����łȂ��Ȃ�1(true)���������Ȃ���΂Ȃ�Ȃ�
int inter_pet_parse_frommap(int fd)
{
	switch(RFIFOW(fd,0)){
	case 0x3080: mapif_parse_CreatePet(fd); break;
	case 0x3081: mapif_parse_LoadPet(fd); break;
	case 0x3082: mapif_parse_SavePet(fd); break;
	case 0x3083: mapif_parse_DeletePet(fd); break;
	default:
		return 0;
	}
	return 1;
}
