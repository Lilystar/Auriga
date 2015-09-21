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

#ifdef TXT_ONLY

#include <stdio.h>
#include <stdlib.h>

#include "mmo.h"
#include "db.h"
#include "lock.h"
#include "malloc.h"
#include "journal.h"
#include "utils.h"

#include "mercdb.h"
#include "int_merc.h"

static struct dbt *merc_db = NULL;

static char merc_txt[1024]="save/mercenary.txt";
static int merc_newid = 100;

#ifdef TXT_JOURNAL
static int merc_journal_enable = 1;
static struct journal merc_journal;
static char merc_journal_file[1024]="./save/mercenary.journal";
static int merc_journal_cache = 1000;
#endif

int mercdb_txt_config_read_sub(const char* w1,const char *w2)
{
	if(strcmpi(w1,"merc_txt")==0){
		strncpy(merc_txt, w2, sizeof(merc_txt) - 1);
	}
#ifdef TXT_JOURNAL
	else if(strcmpi(w1,"merc_journal_enable")==0){
		merc_journal_enable = atoi( w2 );
	}
	else if(strcmpi(w1,"merc_journal_file")==0){
		strncpy( merc_journal_file, w2, sizeof(merc_journal_file) - 1 );
	}
	else if(strcmpi(w1,"merc_journal_cache_interval")==0){
		merc_journal_cache = atoi( w2 );
	}
#endif
	else {
		return 0;
	}

	return 1;
}

static int merc_tostr(char *str,struct mmo_mercstatus *m)
{
	int i;
	char *str_p = str;
	unsigned short sk_lv;

	if(!m) return 0;

	str_p += sprintf(str,"%d,%d,%s\t%d,%d\t%d,%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%u",
		m->merc_id,m->class_,m->name,
		m->account_id,m->char_id,
		m->base_level,m->max_hp,m->hp,m->max_sp,m->sp,
		m->str,m->agi,m->vit,m->int_,m->dex,m->luk,
		m->kill_count,m->limit);

	*(str_p++)='\t';

	for(i=0;i<MAX_MERCSKILL;i++) {
		if(m->skill[i].id && m->skill[i].flag!=1){
			sk_lv = (m->skill[i].flag==0)? m->skill[i].lv: m->skill[i].flag-2;
			str_p += sprintf(str_p,"%d,%d ",m->skill[i].id,sk_lv);
		}
	}
	*(str_p++)='\t';

	*str_p='\0';
	return 0;
}

static int merc_fromstr(char *str,struct mmo_mercstatus *m)
{
	int i,s,next,set,len;
	int tmp_int[17];
	char tmp_str[256];

	if(!m) return 0;

	memset(m,0,sizeof(struct mmo_mercstatus));

	s=sscanf(str,"%d,%d,%255[^\t]\t%d,%d\t%d,%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%u%n",
		&tmp_int[0],&tmp_int[1],tmp_str,
		&tmp_int[2],&tmp_int[3],
		&tmp_int[4],&tmp_int[5],&tmp_int[6],&tmp_int[7],&tmp_int[8],
		&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],&tmp_int[13],&tmp_int[14],
		&tmp_int[15],&tmp_int[16],&next);

	if(s!=18)
		return 1;

	m->merc_id      = tmp_int[0];
	m->class_       = tmp_int[1];
	strncpy(m->name,tmp_str,24);
	m->name[23] = '\0';	// force \0 terminal
	m->account_id   = tmp_int[2];
	m->char_id      = tmp_int[3];
	m->base_level   = tmp_int[4];
	m->max_hp       = tmp_int[5];
	m->hp           = tmp_int[6];
	m->max_sp       = tmp_int[7];
	m->sp           = tmp_int[8];
	m->str          = tmp_int[9];
	m->agi          = tmp_int[10];
	m->vit          = tmp_int[11];
	m->int_         = tmp_int[12];
	m->dex          = tmp_int[13];
	m->luk          = tmp_int[14];
	m->kill_count   = tmp_int[15];
	m->limit        = (unsigned int)tmp_int[16];
	m->option       = 0;

	if(str[next]=='\n' || str[next]=='\r')
		return 1;	// �X�L�����Ȃ�

	next++;
	for(i=0;str[next] && str[next]!='\t';i++){
		int n;
		set=sscanf(str+next,"%d,%d%n",
			&tmp_int[0],&tmp_int[1],&len);
		if(set!=2)
			return 0;
		n = tmp_int[0]-MERC_SKILLID;
		if(n >= 0 && n < MAX_MERCSKILL) {
			m->skill[n].id = tmp_int[0];
			m->skill[n].lv = tmp_int[1];
		} else {
			printf("merc_fromstr: invaild skill id: %d\n", tmp_int[0]);
		}
		next+=len;
		if(str[next]==' ')
			next++;
	}
	return 0;
}

#ifdef TXT_JOURNAL
// ==========================================
// �b���f�[�^�̃W���[�i���̃��[���t�H���[�h�p�R�[���o�b�N�֐�
// ------------------------------------------
int merc_journal_rollforward( int key, void* buf, int flag )
{
	struct mmo_mercstatus* m = (struct mmo_mercstatus *)numdb_search( merc_db, key );

	// �O�̂��߃`�F�b�N
	if( flag == JOURNAL_FLAG_WRITE && key != ((struct mmo_mercstatus*)buf)->merc_id )
	{
		printf("int_merc: journal: key != merc_id !\n");
		return 0;
	}

	// �f�[�^�̒u������
	if( m )
	{
		if( flag == JOURNAL_FLAG_DELETE ) {
			numdb_erase(merc_db, key);
			aFree(m);
		} else {
			memcpy( m, buf, sizeof(struct mmo_mercstatus) );
		}
		return 1;
	}

	// �ǉ�
	if( flag != JOURNAL_FLAG_DELETE )
	{
		m = (struct mmo_mercstatus*) aCalloc( 1, sizeof( struct mmo_mercstatus ) );
		memcpy( m, buf, sizeof(struct mmo_mercstatus) );
		numdb_insert( merc_db, key, m );
		if( m->merc_id >= merc_newid)
			merc_newid=m->merc_id+1;
		return 1;
	}

	return 0;
}
int mercdb_txt_sync(void);
#endif

bool mercdb_txt_init(void)
{
	char line[8192];
	struct mmo_mercstatus *m;
	FILE *fp;
	int c=0;

	merc_db=numdb_init();

	if( (fp=fopen(merc_txt,"r"))==NULL )
		return 1;
	while(fgets(line,sizeof(line),fp)){
		m=(struct mmo_mercstatus *)aCalloc(1,sizeof(struct mmo_mercstatus));
		if(merc_fromstr(line,m)==0 && m->merc_id>0){
			if( m->merc_id >= merc_newid)
				merc_newid=m->merc_id+1;
			numdb_insert(merc_db,m->merc_id,m);
		}else{
			printf("int_merc: broken data [%s] line %d\n",merc_txt,c);
			aFree(m);
		}
		c++;
	}
	fclose(fp);

#ifdef TXT_JOURNAL
	if( merc_journal_enable )
	{
		// �W���[�i���f�[�^�̃��[���t�H���[�h
		if( journal_load( &merc_journal, sizeof(struct mmo_mercstatus), merc_journal_file ) )
		{
			int c = journal_rollforward( &merc_journal, merc_journal_rollforward );

			printf("int_merc: journal: roll-forward (%d)\n", c );

			// ���[���t�H���[�h�����̂ŁAtxt �f�[�^��ۑ����� ( journal ���V�K�쐬�����)
			mercdb_txt_sync();
		}
		else
		{
			// �W���[�i����V�K�쐬����
			journal_final( &merc_journal );
			journal_create( &merc_journal, sizeof(struct mmo_mercstatus), merc_journal_cache, merc_journal_file );
		}
	}
#endif

	return true;
}

static int mercdb_txt_sync_sub(void *key,void *data,va_list ap)
{
	char line[8192];
	FILE *fp;

	merc_tostr(line,(struct mmo_mercstatus *)data);
	fp=va_arg(ap,FILE *);
	fprintf(fp,"%s" RETCODE,line);
	return 0;
}

int mercdb_txt_sync(void)
{
	FILE *fp;
	int lock;

	if( !merc_db )
		return 1;

	if( (fp=lock_fopen(merc_txt,&lock))==NULL ){
		printf("int_merc: cant write [%s] !!! data is lost !!!\n",merc_txt);
		return 1;
	}
	numdb_foreach(merc_db,mercdb_txt_sync_sub,fp);
	lock_fclose(fp,merc_txt,&lock);

#ifdef TXT_JOURNAL
	if( merc_journal_enable )
	{
		// �R�~�b�g�����̂ŃW���[�i����V�K�쐬����
		journal_final( &merc_journal );
		journal_create( &merc_journal, sizeof(struct mmo_mercstatus), merc_journal_cache, merc_journal_file );
	}
#endif

	return 0;
}

bool mercdb_txt_delete(int merc_id)
{
	struct mmo_mercstatus *p = (struct mmo_mercstatus *)numdb_search(merc_db,merc_id);

	if(p == NULL)
		return false;

	numdb_erase(merc_db,merc_id);
	aFree(p);
	printf("merc_id: %d deleted\n",merc_id);

#ifdef TXT_JOURNAL
	if( merc_journal_enable )
		journal_write( &merc_journal, merc_id, NULL );
#endif

	return true;
}

const struct mmo_mercstatus* mercdb_txt_load(int merc_id)
{
	return (const struct mmo_mercstatus *)numdb_search(merc_db,merc_id);
}

bool mercdb_txt_save(struct mmo_mercstatus* p2)
{
	struct mmo_mercstatus* p1 = (struct mmo_mercstatus *)numdb_search(merc_db,p2->merc_id);

	if(p1 == NULL) {
		p1 = (struct mmo_mercstatus *)aMalloc(sizeof(struct mmo_mercstatus));
		numdb_insert(merc_db,p2->merc_id,p1);
	}
	memcpy(p1,p2,sizeof(struct mmo_mercstatus));

#ifdef TXT_JOURNAL
	if( merc_journal_enable )
		journal_write( &merc_journal, p1->merc_id, p1 );
#endif
	return true;
}

bool mercdb_txt_new(struct mmo_mercstatus *p2)
{
	struct mmo_mercstatus *p1 = (struct mmo_mercstatus *)aMalloc(sizeof(struct mmo_mercstatus));

	p2->merc_id = merc_newid++;
	memcpy(p1,p2,sizeof(struct mmo_mercstatus));
	numdb_insert(merc_db,p2->merc_id,p1);
	return true;
}

static int mercdb_txt_final_sub(void *key,void *data,va_list ap)
{
	struct mmo_mercstatus *p = (struct mmo_mercstatus *)data;

	aFree(p);

	return 0;
}

void mercdb_txt_final(void)
{
	if(merc_db)
		numdb_final(merc_db,mercdb_txt_final_sub);

#ifdef TXT_JOURNAL
	if( merc_journal_enable )
	{
		journal_final( &merc_journal );
	}
#endif
}

#endif
