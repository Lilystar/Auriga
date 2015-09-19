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

#define _INT_GUILD_C_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "mmo.h"
#include "socket.h"
#include "db.h"
#include "lock.h"
#include "malloc.h"
#include "journal.h"
#include "utils.h"
#include "sqldbs.h"

#include "char.h"
#include "inter.h"
#include "int_guild.h"
#include "int_storage.h"

static int guild_exp[MAX_GUILDLEVEL];

static int guild_join_limit = 0;
static int guild_extension_increment = 4;

static struct dbt *guild_db = NULL;
static struct guild_castle castle_db[MAX_GUILDCASTLE];

struct {
	int id;
	int max;
	struct{
		short id;
		short lv;
	} need[5];
} guild_skill_tree[MAX_GUILDSKILL];

int mapif_guild_broken(int guild_id,int flag);
int guild_check_empty(const struct guild *g);
void guild_calc_skilltree(struct guild *g);
int guild_calcinfo(struct guild *g);
int mapif_guild_info(int fd,const struct guild *g);

// �M���h�֘A�f�[�^�x�[�X�ǂݍ���
static int guild_readdb(void)
{
	int i;
	FILE *fp;
	char line[1024],*p;

	// �M���h�o���l
	fp=fopen("db/exp_guild.txt","r");
	if(fp==NULL){
		printf("can't read db/exp_guild.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)) {
		if(line[0] == '\0' || line[0] == '\r' || line[0] == '\n')
			continue;
		if(line[0] == '/' && line[1] == '/')
			continue;
		guild_exp[i]=atoi(line);
		i++;
		if(i >= MAX_GUILDLEVEL)
			break;
	}
	fclose(fp);
	printf("read db/exp_guild.txt done.\n");

	// �M���h�X�L���c���[
	memset(guild_skill_tree,0,sizeof(guild_skill_tree));

	fp=fopen("db/guild_skill_tree.txt","r");
	if(fp==NULL){
		printf("can't read db/guild_skill_tree.txt\n");
		return 1;
	}

	while(fgets(line,1020,fp)){
		int skillid, id, k;
		char *split[12];
		if(line[0] == '\0' || line[0] == '\r' || line[0] == '\n')
			continue;
		if(line[0] == '/' && line[1] == '/')
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

		for(k=0;k<5;k++){
			guild_skill_tree[id].need[k].id=atoi(split[k*2+2]);
			guild_skill_tree[id].need[k].lv=atoi(split[k*2+3]);
		}
	}
	fclose(fp);
	printf("read db/guild_skill_tree.txt done\n");

	return 0;
}

#ifdef TXT_ONLY

static int guildcastle_txt_init(void);
static int guildcastle_txt_sync(void);

static int guild_newid=10000;
static char guild_txt[1024]  = "save/guild.txt";
static char castle_txt[1024] = "save/castle.txt";


#ifdef TXT_JOURNAL
static int guild_journal_enable = 1;
static struct journal guild_journal;
static char guild_journal_file[1024]="./save/guild.journal";
static int guild_journal_cache = 1000;

static int guildcastle_journal_enable = 1;
static struct journal guildcastle_journal;
static char guildcastle_journal_file[1024]="./save/castle.journal";
static int guildcastle_journal_cache = 1000;

#endif

// �M���h�f�[�^�̕�����ւ̕ϊ�
static int guild_tostr(char *str,struct guild *g)
{
	int i,c,len;

	// ��{�f�[�^
	len=sprintf(str,"%d\t%s\t%s\t%d,%d,%d,%d\t%s#\t%s#\t",
		g->guild_id,g->name,g->master,
		g->guild_lv,g->max_member,g->exp,g->skill_point,
		g->mes1,g->mes2);

	// �����o�[
	for(i=0;i<g->max_member;i++){
		struct guild_member *m = &g->member[i];
		len+=sprintf(str+len,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\t%s\t",
			m->account_id,m->char_id,
			m->hair,m->hair_color,m->gender,
			m->class_,m->lv,m->exp,m->exp_payper,m->position,
			((m->account_id>0)? m->name: "-"));
	}

	// ��E
	for(i=0;i<MAX_GUILDPOSITION;i++){
		struct guild_position *p = &g->position[i];
		len+=sprintf(str+len,"%d,%d\t%s#\t",
			p->mode,p->exp_mode,p->name);
	}

	// �G���u����
	len+=sprintf(str+len,"%d,%d,",g->emblem_len,g->emblem_id);
	for(i=0;i<g->emblem_len;i++){
		len+=sprintf(str+len,"%02x",(unsigned char)(g->emblem_data[i]));
	}
	len+=sprintf(str+len,"$\t");

	// ����/�G�΃��X�g
	for(i=0,c=0;i<MAX_GUILDALLIANCE;i++) {
		if(g->alliance[i].guild_id>0)
			c++;
	}
	len+=sprintf(str+len,"%d\t",c);
	for(i=0;i<MAX_GUILDALLIANCE;i++){
		struct guild_alliance *a=&g->alliance[i];
		if(a->guild_id>0)
			len+=sprintf(str+len,"%d,%d\t%s\t",
				a->guild_id,a->opposition,a->name);
	}

	// �Ǖ����X�g
	for(i=0,c=0;i<MAX_GUILDEXPLUSION;i++) {
		if(g->explusion[i].account_id>0)
			c++;
	}
	len+=sprintf(str+len,"%d\t",c);
	for(i=0;i<MAX_GUILDEXPLUSION;i++){
		struct guild_explusion *e=&g->explusion[i];
		if(e->account_id>0)
			len+=sprintf(str+len,"%d\t%s\t%s#\t",
				e->account_id,e->name,e->mes );
	}

	// �M���h�X�L��
	for(i=0;i<MAX_GUILDSKILL;i++){
		len+=sprintf(str+len,"%d,%d ",g->skill[i].id,g->skill[i].lv);
	}
	len+=sprintf(str+len,"\t");
	return 0;
}

// �M���h�f�[�^�̕����񂩂�̕ϊ�
static int guild_fromstr(char *str,struct guild *g)
{
	int i,j,c;
	int tmp_int[10];
	char tmp_str[4][256];
	char tmp_str2[4097];	// binary data + 1
	char *pstr;

	// ��{�f�[�^
	memset(g,0,sizeof(struct guild));

	if( sscanf(str,"%d\t%255[^\t]\t%255[^\t]\t%d,%d,%d,%d,%d\t%255[^\t]\t%255[^\t]\t",
		&tmp_int[0],tmp_str[0],tmp_str[1],
		&tmp_int[1],&tmp_int[2],&tmp_int[3],&tmp_int[4],&tmp_int[5],
		tmp_str[2],tmp_str[3]) != 10)
	{
		// Auriga-0177�ȍ~�̌`��
		if( sscanf(str,"%d\t%255[^\t]\t%255[^\t]\t%d,%d,%d,%d\t%255[^\t]\t%255[^\t]\t",
			&tmp_int[0],tmp_str[0],tmp_str[1],
			&tmp_int[1],&tmp_int[2],&tmp_int[3],&tmp_int[4],
			tmp_str[2],tmp_str[3]) != 9)
			return 1;
	}

	g->guild_id    = tmp_int[0];
	g->guild_lv    = tmp_int[1];
	g->max_member  = tmp_int[2];
	g->exp         = tmp_int[3];
	g->skill_point = tmp_int[4];
	strncpy(g->name,tmp_str[0],24);
	strncpy(g->master,tmp_str[1],24);
	tmp_str[2][strlen(tmp_str[2])-1] = 0;
	strncpy(g->mes1,tmp_str[2],60);
	tmp_str[3][strlen(tmp_str[3])-1] = 0;
	strncpy(g->mes2,tmp_str[3],120);

	// force \0 terminal
	g->name[23]   = '\0';
	g->master[23] = '\0';
	g->mes1[59]   = '\0';
	g->mes2[119]  = '\0';

	for(j=0;j<6 && str!=NULL;j++)	// �ʒu�X�L�b�v
		str=strchr(str+1,'\t');
	//printf("GuildBaseInfo OK\n");

	// �����o�[
	for(i=0;i<g->max_member && i<MAX_GUILD;i++){
		struct guild_member *m = &g->member[i];
		if( sscanf(str+1,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\t%255[^\t]\t",
			&tmp_int[0],&tmp_int[1],&tmp_int[2],&tmp_int[3],&tmp_int[4],
			&tmp_int[5],&tmp_int[6],&tmp_int[7],&tmp_int[8],&tmp_int[9],
			tmp_str[0]) <11)
			return 1;
		m->account_id=tmp_int[0];
		m->char_id=tmp_int[1];
		m->hair=tmp_int[2];
		m->hair_color=tmp_int[3];
		m->gender=tmp_int[4];
		m->class_=tmp_int[5];
		m->lv=tmp_int[6];
		m->exp=tmp_int[7];
		m->exp_payper=tmp_int[8];
		m->position=tmp_int[9];
		strncpy(m->name,tmp_str[0],24);
		m->name[23] = '\0';	// force \0 terminal

		for(j=0;j<2 && str!=NULL;j++)	// �ʒu�X�L�b�v
			str=strchr(str+1,'\t');
	}
	if(g->max_member != i) {
		printf("int_guild: max guild member changed %d -> %d\n", g->max_member, i);
		g->max_member = i;
	}

	// ��E
	for(i=0;i<MAX_GUILDPOSITION;i++){
		struct guild_position *p = &g->position[i];
		if( sscanf(str+1,"%d,%d\t%255[^\t]\t",
			&tmp_int[0],&tmp_int[1],tmp_str[0]) < 3)
			return 1;
		p->mode     = tmp_int[0];
		p->exp_mode = tmp_int[1];
		tmp_str[0][strlen(tmp_str[0])-1] = 0;
		strncpy(p->name,tmp_str[0],24);
		p->name[23] = '\0';	// force \0 terminal

		for(j=0;j<2 && str!=NULL;j++)	// �ʒu�X�L�b�v
			str=strchr(str+1,'\t');
	}

	// �G���u����
	tmp_int[1]=0;
	if( sscanf(str+1,"%d,%d,%4096[^\t]\t",&tmp_int[0],&tmp_int[1],tmp_str2) < 3 )
		return 1;
	g->emblem_len = tmp_int[0];
	g->emblem_id  = tmp_int[1];
	if(g->emblem_len > sizeof(tmp_str2) / 2) {
		// �G���u�����̃f�[�^�����傫������
		return 1;
	}
	for(i=0,pstr=tmp_str2;i<g->emblem_len;i++,pstr+=2){
		int c1=pstr[0],c2=pstr[1],x1=0,x2=0;
		if(c1>='0' && c1<='9')x1=c1-'0';
		if(c1>='a' && c1<='f')x1=c1-'a'+10;
		if(c1>='A' && c1<='F')x1=c1-'A'+10;
		if(c2>='0' && c2<='9')x2=c2-'0';
		if(c2>='a' && c2<='f')x2=c2-'a'+10;
		if(c2>='A' && c2<='F')x2=c2-'A'+10;
		g->emblem_data[i]=(x1<<4)|x2;
	}
	//printf("GuildEmblemInfo OK\n");
	str=strchr(str+1,'\t');	// �ʒu�X�L�b�v

	// ����/�G�΃��X�g
	if( sscanf(str+1,"%d\t",&c)< 1)
		return 1;
	str=strchr(str+1,'\t');	// �ʒu�X�L�b�v
	for(i=0;i<c && i<MAX_GUILDALLIANCE;i++){
		struct guild_alliance *a = &g->alliance[i];
		if( sscanf(str+1,"%d,%d\t%255[^\t]\t",
			&tmp_int[0],&tmp_int[1],tmp_str[0]) < 3)
			return 1;
		a->guild_id   = tmp_int[0];
		a->opposition = tmp_int[1];
		strncpy(a->name,tmp_str[0],24);
		a->name[23] = '\0';	// force \0 terminal

		for(j=0;j<2 && str!=NULL;j++)	// �ʒu�X�L�b�v
			str=strchr(str+1,'\t');
	}
	//printf("GuildAllianceInfo OK\n");

	// �Ǖ����X�g
	if( sscanf(str+1,"%d\t",&c)< 1)
		return 1;
	str=strchr(str+1,'\t');	// �ʒu�X�L�b�v
	for(i=0;i<c && i<MAX_GUILDEXPLUSION;i++){
		struct guild_explusion *e = &g->explusion[i];
		int ret,step;

		ret = sscanf(str+1,"%d,%d,%d,%d\t%255[^\t]\t%255[^\t]\t%255[^\t]\t",
			&tmp_int[0],&tmp_int[1],&tmp_int[2],&tmp_int[3],
			tmp_str[0],tmp_str[2],tmp_str[1]);
		if(ret != 7) {
			// Auriga-0142�ȍ~�̌`��
			ret = sscanf(str+1,"%d\t%255[^\t]\t%255[^\t]\t",
				&tmp_int[0],tmp_str[0],tmp_str[1]);
			if(ret != 3)
				return 1;
			step = 3;
		} else {
			step = 4;
		}
		e->account_id = tmp_int[0];
		strncpy(e->name,tmp_str[0],24);
		tmp_str[1][strlen(tmp_str[1])-1] = 0;
		strncpy(e->mes,tmp_str[1],40);

		// force \0 terminal
		e->name[23] = '\0';
		e->mes[39]  = '\0';

		for(j=0;j<step && str!=NULL;j++)	// �ʒu�X�L�b�v
			str=strchr(str+1,'\t');
	}
	//printf("GuildExplusionInfo OK\n");

	// �M���h�X�L��
	for(i=0;i<MAX_GUILDSKILL;i++){
		int n;
		if( sscanf(str+1,"%d,%d ",&tmp_int[0],&tmp_int[1]) <2)
			break;
		n = tmp_int[0] - GUILD_SKILLID;
		if(n >= 0 && n < MAX_GUILDSKILL) {
			g->skill[n].id = tmp_int[0];
			g->skill[n].lv = tmp_int[1];
		}
		str=strchr(str+1,' ');
	}

	// �X�L���c���[���͏�����
	for(i=0;i<MAX_GUILDSKILL;i++)
		g->skill[i].id = 0;

	str=strchr(str+1,'\t');
	//printf("GuildSkillInfo OK\n");

	return 0;
}

#ifdef TXT_JOURNAL
// ==========================================
// �M���h�f�[�^�̃W���[�i���̃��[���t�H���[�h�p�R�[���o�b�N�֐�
// ------------------------------------------
int guild_journal_rollforward( int key, void* buf, int flag )
{
	struct guild* g = (struct guild *)numdb_search( guild_db, key );

	// �O�̂��߃`�F�b�N
	if( flag == JOURNAL_FLAG_WRITE && key != ((struct guild*)buf)->guild_id )
	{
		printf("int_guild: journal: key != guild_id !\n");
		return 0;
	}

	// �f�[�^�̒u������
	if( g )
	{
		if(flag == JOURNAL_FLAG_DELETE) {
			numdb_erase( guild_db, key );
			aFree(g);
		} else {
			memcpy( g, buf, sizeof(struct guild) );
		}
		return 1;
	}

	// �ǉ�
	if( flag != JOURNAL_FLAG_DELETE )
	{
		g = (struct guild*) aCalloc( 1, sizeof( struct guild ) );
		memcpy( g, buf, sizeof(struct guild) );
		numdb_insert( guild_db, key, g );
		if( g->guild_id >= guild_newid)
			guild_newid=g->guild_id+1;
		return 1;
	}

	return 0;
}
int guild_txt_sync(void);
#endif

// �M���h�f�[�^�̓ǂݍ���
int guild_txt_init(void)
{
	char line[16384];
	struct guild *g;
	FILE *fp;
	int c=0;

	guild_readdb();

	guild_db=numdb_init();

	if( (fp=fopen(guild_txt,"r"))==NULL )
		return 1;
	while(fgets(line,sizeof(line),fp)){
		g=(struct guild *)aCalloc(1,sizeof(struct guild));
		if(guild_fromstr(line,g)==0 && g->guild_id>0){
			if(g->guild_id >= guild_newid)
				guild_newid=g->guild_id+1;
			numdb_insert(guild_db,g->guild_id,g);
			guild_calc_skilltree(g);
			guild_calcinfo(g);
		}else{
			printf("int_guild: broken data [%s] line %d\n",guild_txt,c);
			aFree(g);
		}
		c++;
	}
	fclose(fp);
	//printf("int_guild: %s read done (%d guilds)\n",guild_txt,c);

#ifdef TXT_JOURNAL
	if( guild_journal_enable )
	{
		// �W���[�i���f�[�^�̃��[���t�H���[�h
		if( journal_load( &guild_journal, sizeof(struct guild), guild_journal_file ) )
		{
			int c = journal_rollforward( &guild_journal, guild_journal_rollforward );

			printf("int_guild: guild_journal: roll-forward (%d)\n", c );

			// ���[���t�H���[�h�����̂ŁAtxt �f�[�^��ۑ����� ( journal ���V�K�쐬�����)
			guild_txt_sync();
		}
		else
		{
			// �W���[�i����V�K�쐬����
			journal_final( &guild_journal );
			journal_create( &guild_journal, sizeof(struct guild), guild_journal_cache, guild_journal_file );
		}
	}
#endif

	guildcastle_txt_init();
	return 0;
}

const struct guild *guild_txt_load_num(int guild_id)
{
	struct guild *g = (struct guild *)numdb_search(guild_db,guild_id);

	if(g)
		guild_calcinfo(g);

	return g;
}

// �M���h�f�[�^�̃Z�[�u�p
static int guild_txt_sync_sub(void *key,void *data,va_list ap)
{
	char line[16384];
	FILE *fp;

	guild_tostr(line,(struct guild *)data);
	fp=va_arg(ap,FILE *);
	fprintf(fp,"%s" RETCODE,line);
	return 0;
}

// �M���h�f�[�^�̃Z�[�u
int guild_txt_sync(void)
{
	FILE *fp;
	int lock;

	if( !guild_db )
		return 1;

	if( (fp=lock_fopen(guild_txt,&lock))==NULL ){
		printf("int_guild: cant write [%s] !!! data is lost !!!\n",guild_txt);
		return 1;
	}
	numdb_foreach(guild_db,guild_txt_sync_sub,fp);
	lock_fclose(fp,guild_txt,&lock);

#ifdef TXT_JOURNAL
	if( guild_journal_enable )
	{
		// �R�~�b�g�����̂ŃW���[�i����V�K�쐬����
		journal_final( &guild_journal );
		journal_create( &guild_journal, sizeof(struct guild), guild_journal_cache, guild_journal_file );
	}
#endif

	guildcastle_txt_sync();
	return 0;
}

// �M���h�������p
static int guild_txt_load_str_sub(void *key,void *data,va_list ap)
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

// �M���h������
const struct guild* guild_txt_load_str(char *str)
{
	struct guild *g=NULL;

	numdb_foreach(guild_db,guild_txt_load_str_sub,str,&g);
	return g;
}

// �M���h���U�����p�i����/�G�΂������j
static int guild_txt_delete_sub(void *key,void *data,va_list ap)
{
	struct guild *g=(struct guild *)data;
	int guild_id=va_arg(ap,int);
	int i;

	for(i=0;i<MAX_GUILDALLIANCE;i++){
		if(g->alliance[i].guild_id==guild_id)
		{
			g->alliance[i].guild_id=0;
#ifdef TXT_JOURNAL
			if( guild_journal_enable )
				journal_write( &guild_journal, g->guild_id, g );
#endif
		}
	}
	return 0;
}

void guild_txt_delete(int guild_id)
{
	struct guild *g = (struct guild *)numdb_search(guild_db,guild_id);

	if(g) {
		numdb_foreach(guild_db,guild_txt_delete_sub,g->guild_id);
		numdb_erase(guild_db,g->guild_id);
		gstorage_delete(g->guild_id);
		mapif_guild_broken(g->guild_id,0);
		aFree(g);

#ifdef TXT_JOURNAL
		if( guild_journal_enable )
			journal_write( &guild_journal, guild_id, NULL );
#endif
	}
}

void guild_txt_new(struct guild *g2)
{
	struct guild* g1 = (struct guild *)aMalloc(sizeof(struct guild));

	g2->guild_id = guild_newid++;
	memcpy(g1,g2,sizeof(struct guild));
	numdb_insert(guild_db,g2->guild_id,g1);
#ifdef TXT_JOURNAL
	if( guild_journal_enable )
		journal_write( &guild_journal, g1->guild_id, g1 );
#endif
}

void guild_txt_config_read_sub(const char* w1,const char *w2)
{
	if(strcmpi(w1,"guild_txt")==0){
		strncpy(guild_txt,w2,sizeof(guild_txt) - 1);
	}
	else if(strcmpi(w1,"castle_txt")==0){
		strncpy(castle_txt,w2,sizeof(castle_txt) - 1);
	}
#ifdef TXT_JOURNAL
	else if(strcmpi(w1,"guild_journal_enable")==0){
		guild_journal_enable = atoi( w2 );
	}
	else if(strcmpi(w1,"guild_journal_file")==0){
		strncpy( guild_journal_file, w2, sizeof(guild_journal_file) - 1 );
	}
	else if(strcmpi(w1,"guild_journal_cache_interval")==0){
		guild_journal_cache = atoi( w2 );
	}
	else if(strcmpi(w1,"castle_journal_enable")==0){
		guildcastle_journal_enable = atoi( w2 );
	}
	else if(strcmpi(w1,"castle_journal_file")==0){
		strncpy( guildcastle_journal_file, w2, sizeof(guildcastle_journal_file) - 1 );
	}
	else if(strcmpi(w1,"castle_journal_cache_interval")==0){
		guildcastle_journal_cache = atoi( w2 );
	}
#endif
}

static int guild_txt_db_final(void *key,void *data,va_list ap)
{
	struct guild *g = (struct guild *)data;

	aFree(g);

	return 0;
}

void guild_txt_final(void)
{
	if(guild_db)
		numdb_final(guild_db,guild_txt_db_final);

#ifdef TXT_JOURNAL
	if( guild_journal_enable )
	{
		journal_final( &guild_journal );
	}
	if( guildcastle_journal_enable )
	{
		journal_final( &guildcastle_journal );
	}
#endif
}

int guild_txt_save(struct guild* g2)
{
	struct guild *g1 = (struct guild *)numdb_search(guild_db,g2->guild_id);

	if(g1 == NULL) {
		g1 = (struct guild *)aMalloc(sizeof(struct guild));
		numdb_insert(guild_db,g2->guild_id,g1);
	}
	memcpy(g1,g2,sizeof(struct guild));
#ifdef TXT_JOURNAL
	if( guild_journal_enable )
		journal_write( &guild_journal, g1->guild_id, g1 );
#endif
	return 1;
}

// �M���h��f�[�^�̕�����ւ̕ϊ�
static int guildcastle_tostr(char *str,struct guild_castle *gc)
{
	sprintf(str,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		gc->castle_id,gc->guild_id,gc->economy,gc->defense,gc->triggerE,
		gc->triggerD,gc->nextTime,gc->payTime,gc->createTime,gc->visibleC,
		gc->visibleG0,gc->visibleG1,gc->visibleG2,gc->visibleG3,gc->visibleG4,
		gc->visibleG5,gc->visibleG6,gc->visibleG7);

	return 0;
}

// �M���h��f�[�^�̕����񂩂�̕ϊ�
static int guildcastle_fromstr(char *str,struct guild_castle *gc)
{
	int tmp_int[18];

	memset(gc,0,sizeof(struct guild_castle));
	if( sscanf(str,"%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
		&tmp_int[0],&tmp_int[1],&tmp_int[2],&tmp_int[3],&tmp_int[4],&tmp_int[5],&tmp_int[6],
		&tmp_int[7],&tmp_int[8],&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],&tmp_int[13],
		&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17]) <18 )
		return 1;
	gc->castle_id  = tmp_int[0];
	gc->guild_id   = tmp_int[1];
	gc->economy    = tmp_int[2];
	gc->defense    = tmp_int[3];
	gc->triggerE   = tmp_int[4];
	gc->triggerD   = tmp_int[5];
	gc->nextTime   = tmp_int[6];
	gc->payTime    = tmp_int[7];
	gc->createTime = tmp_int[8];
	gc->visibleC   = tmp_int[9];
	gc->visibleG0  = tmp_int[10];
	gc->visibleG1  = tmp_int[11];
	gc->visibleG2  = tmp_int[12];
	gc->visibleG3  = tmp_int[13];
	gc->visibleG4  = tmp_int[14];
	gc->visibleG5  = tmp_int[15];
	gc->visibleG6  = tmp_int[16];
	gc->visibleG7  = tmp_int[17];

	return 0;
}

#ifdef TXT_JOURNAL
// ==========================================
// �M���h��f�[�^�̃W���[�i���̃��[���t�H���[�h�p�R�[���o�b�N�֐�
// ------------------------------------------
int guildcastle_journal_rollforward( int key, void* buf, int flag )
{
	// �O�̂��߃`�F�b�N
	if( flag == JOURNAL_FLAG_WRITE && key != ((struct guild_castle*)buf)->castle_id )
	{
		printf("int_guild: castle_journal: key != castle_id !\n");
		return 0;
	}

	// �f�[�^�̒u������
	if( key >= 0 && key < MAX_GUILDCASTLE )
	{
		if( flag == JOURNAL_FLAG_DELETE ) {
			memset( &castle_db[key], 0, sizeof(castle_db[0]) );
			castle_db[key].castle_id = key;
		} else {
			memcpy( &castle_db[key], buf, sizeof(castle_db[0]) );
		}
		return 1;
	}

	// �ǉ�
	if( flag != JOURNAL_FLAG_DELETE )
	{
		memcpy( &castle_db[key], buf, sizeof(castle_db[0]) );
		return 1;
	}

	return 0;
}
#endif

static int guildcastle_txt_init(void)
{
	char line[1024];
	struct guild_castle gc;
	FILE *fp;
	int i,c=0;

	// �f�t�H���g�f�[�^���쐬
	memset(castle_db,0,sizeof(castle_db));
	for(i=0; i<MAX_GUILDCASTLE; i++)
		castle_db[i].castle_id = i;

	if( (fp=fopen(castle_txt,"r"))==NULL ){
		return 1;
	}

	while(fgets(line,sizeof(line),fp)){
		if(guildcastle_fromstr(line,&gc)==0 && gc.castle_id >= 0 && gc.castle_id < MAX_GUILDCASTLE) {
			memcpy(&castle_db[gc.castle_id], &gc, sizeof(gc));
		} else {
			printf("int_guild: broken data [%s] line %d\n",castle_txt,c);
		}
		c++;
	}
	fclose(fp);

#ifdef TXT_JOURNAL
	if( guildcastle_journal_enable )
	{
		// �W���[�i���f�[�^�̃��[���t�H���[�h
		if( journal_load( &guildcastle_journal, sizeof(struct guild_castle), guildcastle_journal_file ) )
		{
			int c = journal_rollforward( &guildcastle_journal, guildcastle_journal_rollforward );

			printf("int_guild: castle_journal: roll-forward (%d)\n", c );

			// ���[���t�H���[�h�����̂ŁAtxt �f�[�^��ۑ����� ( journal ���V�K�쐬�����)
			guildcastle_txt_sync();
		}
		else
		{
			// �W���[�i����V�K�쐬����
			journal_final( &guildcastle_journal );
			journal_create( &guildcastle_journal, sizeof(struct guild_castle), guildcastle_journal_cache, guildcastle_journal_file );
		}
	}
#endif
	return 0;
}

// �M���h��f�[�^�̃Z�[�u�p
static int guildcastle_txt_sync(void)
{
	FILE *fp;
	int i,lock;
	char line[1024];

	if( (fp=lock_fopen(castle_txt,&lock))==NULL ){
		printf("int_guild: cant write [%s] !!! data is lost !!!\n",castle_txt);
		return 1;
	}

	for(i=0; i<MAX_GUILDCASTLE; i++) {
		guildcastle_tostr(line,&castle_db[i]);
		fprintf(fp,"%s" RETCODE,line);
	}
	lock_fclose(fp,castle_txt,&lock);

#ifdef TXT_JOURNAL
	if( guildcastle_journal_enable )
	{
		// �R�~�b�g�����̂ŃW���[�i����V�K�쐬����
		journal_final( &guildcastle_journal );
		journal_create( &guildcastle_journal, sizeof(struct guild_castle), guildcastle_journal_cache, guildcastle_journal_file );
	}
#endif

	return 0;
}

#define guild_new      guild_txt_new
#define guild_final    guild_txt_final
#define guild_init     guild_txt_init
#define guild_save     guild_txt_save
#define guild_sync     guild_txt_sync
#define guild_load_num guild_txt_load_num
#define guild_load_str guild_txt_load_str
#define guild_delete   guild_txt_delete
#define guild_config_read_sub guild_txt_config_read_sub

#else /* TXT_ONLY */

static int guildcastle_sql_init(void);

int guild_sql_init(void)
{
	guild_readdb();
	guildcastle_sql_init();
	guild_db=numdb_init();
	return 0;
}

static int guildcastle_sql_init(void)
{
	MYSQL_RES* sql_res;
	MYSQL_ROW  sql_row = NULL;
	struct guild_castle *gc;
	int i,id;

	// �f�t�H���g�f�[�^���쐬
	memset(castle_db,0,sizeof(castle_db));
	for(i=0; i<MAX_GUILDCASTLE; i++)
		castle_db[i].castle_id = i;

	sqldbs_query(&mysql_handle, "SELECT "
		"`castle_id`, `guild_id`, `economy`, `defense`, `triggerE`, `triggerD`, `nextTime`, `payTime`, `createTime`,"
		"`visibleC`, `visibleG0`, `visibleG1`, `visibleG2`, `visibleG3`, `visibleG4`, `visibleG5`, `visibleG6`, `visibleG7`"
		" FROM `" GUILD_CASTLE_TABLE "`"
	);

	sql_res = sqldbs_store_result(&mysql_handle);

	if(sql_res) {
		for(i=0; (sql_row = sqldbs_fetch(sql_res)); i++)
		{
			id = atoi(sql_row[0]);
			if(id < 0 || id >= MAX_GUILDCASTLE)
				continue;
			gc = &castle_db[id];

			gc->guild_id   = atoi(sql_row[1]);
			gc->economy    = atoi(sql_row[2]);
			gc->defense    = atoi(sql_row[3]);
			gc->triggerE   = atoi(sql_row[4]);
			gc->triggerD   = atoi(sql_row[5]);
			gc->nextTime   = atoi(sql_row[6]);
			gc->payTime    = atoi(sql_row[7]);
			gc->createTime = atoi(sql_row[8]);
			gc->visibleC   = atoi(sql_row[9]);
			gc->visibleG0  = atoi(sql_row[10]);
			gc->visibleG1  = atoi(sql_row[11]);
			gc->visibleG2  = atoi(sql_row[12]);
			gc->visibleG3  = atoi(sql_row[13]);
			gc->visibleG4  = atoi(sql_row[14]);
			gc->visibleG5  = atoi(sql_row[15]);
			gc->visibleG6  = atoi(sql_row[16]);
			gc->visibleG7  = atoi(sql_row[17]);
		}
		sqldbs_free_result(sql_res);
	}
	return 0;
}

static int guildcastle_tosql(int castle_id)
{
	int rc;
	struct guild_castle *gc = &castle_db[castle_id];

	rc = sqldbs_query(&mysql_handle,
		"UPDATE `" GUILD_CASTLE_TABLE "` SET guild_id = %d,economy = %d,"
		"defense = %d,triggerE = %d,"
		"triggerD = %d,nextTime = %d,"
		"payTime = %d,createTime = %d,"
		"visibleC = %d,visibleG0 = %d,"
		"visibleG1 = %d,visibleG2 = %d,"
		"visibleG3 = %d,visibleG4 = %d,"
		"visibleG5 = %d,visibleG6 = %d,"
		"visibleG7 = %d WHERE castle_id = %d;",
		gc->guild_id,
		gc->economy,gc->defense,
		gc->triggerE,gc->triggerD,
		gc->nextTime,gc->payTime,
		gc->createTime,gc->visibleC,
		gc->visibleG0,gc->visibleG1,
		gc->visibleG2,gc->visibleG3,
		gc->visibleG4,gc->visibleG5,
		gc->visibleG6,gc->visibleG7,
		gc->castle_id
	);

	return (rc)? 1: 0;
}

static int guild_guildcastle_save(void)
{
	int i;

	for(i=0;i<MAX_GUILDCASTLE;i++)
		guildcastle_tosql(i);
	return 0;
}

int guild_sql_sync(void)
{
	guild_guildcastle_save();
	return 0;
}

const struct guild *guild_sql_load_num(int guild_id)
{
	int rc;
	char emblem_data[4096];
	char *pstr;
	struct guild *g;
	MYSQL_RES* sql_res;
	MYSQL_ROW  sql_row = NULL;

	g = (struct guild*)numdb_search(guild_db,guild_id);
	// �L���b�V��������
	if (g && g->guild_id == guild_id) {
		return g;
	}
	if (g == NULL) {
		g = (struct guild *)aMalloc(sizeof(struct guild));
		numdb_insert(guild_db,guild_id,g);
	}
	memset(g,0,sizeof(struct guild));

	// ��{�f�[�^
	rc = sqldbs_query(
		&mysql_handle,
		"SELECT `name`,`master`,`guild_lv`,`connect_member`,`max_member`,"
		"`average_lv`,`exp`,`next_exp`,`skill_point`,`mes1`,`mes2`,`emblem_len`,"
		"`emblem_id`,`emblem_data` FROM `" GUILD_TABLE "` WHERE `guild_id`='%d'", guild_id
	);
	if(rc) {
		g->guild_id = -1;
		return NULL;
	}

	sql_res = sqldbs_store_result(&mysql_handle);
	if (sql_res!=NULL && sqldbs_num_rows(sql_res)>0) {
		int i;
		sql_row = sqldbs_fetch(sql_res);
		if (sql_row==NULL) {
			sqldbs_free_result(sql_res);
			g->guild_id = -1;
			return NULL;
		}

		g->guild_id  = guild_id;
		strncpy(g->name,sql_row[0],24);
		strncpy(g->master,sql_row[1],24);
		g->guild_lv       = atoi(sql_row[2]);
		g->connect_member = atoi(sql_row[3]);
		if (atoi(sql_row[4]) > MAX_GUILD)
			g->max_member = MAX_GUILD;
		else
			g->max_member = atoi(sql_row[4]);
		g->average_lv  = atoi(sql_row[5]);
		g->exp         = atoi(sql_row[6]);
		g->next_exp    = atoi(sql_row[7]);
		g->skill_point = atoi(sql_row[8]);
		strncpy(g->mes1,sql_row[9],60);
		strncpy(g->mes2,sql_row[10],120);
		g->emblem_len = atoi(sql_row[11]);
		g->emblem_id  = atoi(sql_row[12]);
		strncpy(emblem_data,sql_row[13],4096);
		for(i=0,pstr=emblem_data;i<g->emblem_len;i++,pstr+=2){
			int c1=pstr[0],c2=pstr[1],x1=0,x2=0;
			if(c1>='0' && c1<='9')x1=c1-'0';
			if(c1>='a' && c1<='f')x1=c1-'a'+10;
			if(c1>='A' && c1<='F')x1=c1-'A'+10;
			if(c2>='0' && c2<='9')x2=c2-'0';
			if(c2>='a' && c2<='f')x2=c2-'a'+10;
			if(c2>='A' && c2<='F')x2=c2-'A'+10;
			g->emblem_data[i]=(x1<<4)|x2;
		}
		// force \0 terminal
		g->name[23]   = '\0';
		g->master[23] = '\0';
		g->mes1[59]   = '\0';
		g->mes2[119]  = '\0';
	} else {
		if( sql_res ) sqldbs_free_result(sql_res);
		g->guild_id = -1;
		return NULL;
	}
	sqldbs_free_result(sql_res);

	// �����o�[
	rc = sqldbs_query(
		&mysql_handle,
		"SELECT `account_id`,`char_id`,`hair`,`hair_color`,`gender`,`class`,`lv`,"
		"`exp`,`exp_payper`,`online`,`position`,`name` FROM `" GUILD_MEMBER_TABLE "` "
		"WHERE `guild_id`='%d' ORDER BY `position`", guild_id
	);
	if(rc) {
		g->guild_id = -1;
		return NULL;
	}
	sql_res = sqldbs_store_result(&mysql_handle);
	if (sql_res!=NULL && sqldbs_num_rows(sql_res)>0) {
		int i;
		for(i=0; (sql_row = sqldbs_fetch(sql_res)) && i<MAX_GUILD ;i++){
			struct guild_member *m = &g->member[i];
			m->account_id = atoi(sql_row[0]);
			m->char_id    = atoi(sql_row[1]);
			m->hair       = atoi(sql_row[2]);
			m->hair_color = atoi(sql_row[3]);
			m->gender     = atoi(sql_row[4]);
			m->class_     = atoi(sql_row[5]);
			m->lv         = atoi(sql_row[6]);
			m->exp        = atoi(sql_row[7]);
			m->exp_payper = atoi(sql_row[8]);
			m->online     = (unsigned char)atoi(sql_row[9]);
			if (atoi(sql_row[10]) >= MAX_GUILDPOSITION)
				m->position = MAX_GUILDPOSITION - 1;
			else
				m->position = atoi(sql_row[10]);
			strncpy(m->name,sql_row[11],24);
			m->name[23] = '\0';	//  force \0 terminal
		}
	}
	sqldbs_free_result(sql_res);

	// ��E
	rc = sqldbs_query(
		&mysql_handle,
		"SELECT `position`,`name`,`mode`,`exp_mode` FROM `" GUILD_POSITION_TABLE "` WHERE `guild_id`='%d'",
		guild_id
	);
	if(rc) {
		g->guild_id = -1;
		return NULL;
	}
	sql_res = sqldbs_store_result(&mysql_handle);
	if (sql_res!=NULL && sqldbs_num_rows(sql_res)>0) {
		int i;
		for(i=0;((sql_row = sqldbs_fetch(sql_res))&&i<MAX_GUILDPOSITION);i++){
			int position = atoi(sql_row[0]);
			struct guild_position *p = &g->position[position];
			strncpy(p->name,sql_row[1],24);
			p->name[23] = '\0';	// force \0 terminal
			p->mode     = atoi(sql_row[2]);
			p->exp_mode = atoi(sql_row[3]);
		}
	}
	sqldbs_free_result(sql_res);

	// ����/�G�΃��X�g
	rc = sqldbs_query(
		&mysql_handle,
		"SELECT `opposition`,`alliance_id`,`name` FROM `" GUILD_ALLIANCE_TABLE "` WHERE `guild_id`='%d'",
		guild_id
	);
	if(rc) {
		g->guild_id = -1;
		return NULL;
	}
	sql_res = sqldbs_store_result(&mysql_handle);
	if (sql_res!=NULL && sqldbs_num_rows(sql_res)>0) {
		int i;
		for(i=0;((sql_row = sqldbs_fetch(sql_res))&&i<MAX_GUILDALLIANCE);i++){
			struct guild_alliance *a = &g->alliance[i];
			a->opposition = atoi(sql_row[0]);
			a->guild_id   = atoi(sql_row[1]);
			strncpy(a->name,sql_row[2],24);
			a->name[23] = '\0';	// force \0 terminal
		}
	}
	sqldbs_free_result(sql_res);

	// �Ǖ����X�g
	rc = sqldbs_query(&mysql_handle, "SELECT `name`,`mes`,`account_id` FROM `" GUILD_EXPULSION_TABLE "` WHERE `guild_id`='%d'", guild_id);
	if(rc) {
		g->guild_id = -1;
		return NULL;
	}
	sql_res = sqldbs_store_result(&mysql_handle);
	if (sql_res!=NULL && sqldbs_num_rows(sql_res)>0) {
		int i;
		for(i=0;((sql_row = sqldbs_fetch(sql_res))&&i<MAX_GUILDEXPLUSION);i++){
			struct guild_explusion *e = &g->explusion[i];
			strncpy(e->name,sql_row[0],24);
			strncpy(e->mes,sql_row[1],40);
			e->account_id = atoi(sql_row[2]);

			// force \0 terminal
			e->name[23] = '\0';
			e->mes[39]  = '\0';
		}
	}
	sqldbs_free_result(sql_res);

	// �M���h�X�L��
	rc = sqldbs_query(&mysql_handle, "SELECT `id`,`lv` FROM `" GUILD_SKILL_TABLE "` WHERE `guild_id`='%d'", guild_id);
	if(rc) {
		g->guild_id = -1;
		return NULL;
	}
	sql_res = sqldbs_store_result(&mysql_handle);
	if (sql_res!=NULL && sqldbs_num_rows(sql_res)>0) {
		int i,n;
		for(i=0;((sql_row = sqldbs_fetch(sql_res))&&i<MAX_GUILDSKILL);i++){
			n = atoi(sql_row[0]) - GUILD_SKILLID;
			if(n >= 0 && n < MAX_GUILDSKILL) {
				g->skill[n].id = atoi(sql_row[0]);
				g->skill[n].lv = atoi(sql_row[1]);
			}
		}

		// �X�L���c���[���͏�����
		for(i = 0; i< MAX_GUILDSKILL;i++)
			g->skill[i].id = 0;
	}
	sqldbs_free_result(sql_res);

	// ���̊֐������Ń����������̃M���h�f�[�^�������������邪�A
	// �n���f�[�^�������Ȃ�A���Ă���f�[�^�������ɂȂ�̂ŁA
	// ���u���邱�Ƃɂ���
	guild_calc_skilltree(g);
	guild_calcinfo(g);

	return g;
}

const struct guild* guild_sql_load_str(char *str)
{
	int  id_num = -1;
	char buf[256];
	MYSQL_RES* sql_res;
	MYSQL_ROW  sql_row = NULL;

	sqldbs_query(
		&mysql_handle,
		"SELECT `guild_id` FROM `" GUILD_TABLE "` WHERE `name` = '%s'",
		strecpy(buf,str)
	);
	sql_res = sqldbs_store_result(&mysql_handle);
	if (sql_res) {
		sql_row = sqldbs_fetch(sql_res);
		if(sql_row) {
			id_num = atoi(sql_row[0]);
		}
		sqldbs_free_result(sql_res);
	}
	if(id_num >= 0) {
		return guild_sql_load_num(id_num);
	}
	return NULL;
}

#define UPDATE_NUM(val,sql) \
	if(g1->val != g2->val) {\
		p += sprintf(p,"%c`"sql"` = '%d'",sep,g2->val); sep = ',';\
	}
#define UPDATE_STR(val,sql) \
	if(strcmp(g1->val,g2->val)) {\
		p += sprintf(p,"%c`"sql"` = '%s'",sep,strecpy(buf,g2->val)); sep = ',';\
	}

int guild_sql_save(struct guild* g2)
{
	int  i;
	char buf[256],buf2[256];
	char sep;
	char *p;
	const struct guild* g1 = guild_sql_load_num(g2->guild_id);

	if (g1 == NULL)
		return 0;

	// ��{���
	sep = ' ';
	p = tmp_sql;
	strcpy(p, "UPDATE `" GUILD_TABLE "` SET");
	p += strlen(p);

	UPDATE_STR(name          ,"name");
	UPDATE_STR(master        ,"master");
	UPDATE_NUM(guild_lv      ,"guild_lv");
	UPDATE_NUM(connect_member,"connect_member");
	UPDATE_NUM(max_member    ,"max_member");
	UPDATE_NUM(average_lv    ,"average_lv");
	UPDATE_NUM(exp           ,"exp");
	UPDATE_NUM(next_exp      ,"next_exp");
	UPDATE_NUM(skill_point   ,"skill_point");
	UPDATE_STR(mes1          ,"mes1");
	UPDATE_STR(mes2          ,"mes2");
	UPDATE_NUM(emblem_len    ,"emblem_len");
	UPDATE_NUM(emblem_id     ,"emblem_id");
	if(g1->emblem_len != g2->emblem_len || memcmp(g1->emblem_data,g2->emblem_data,g1->emblem_len)) {
		p += sprintf(p,"%c`emblem_data` = '",sep);
		for(i = 0; i < g2->emblem_len ; i++) {
			p += sprintf(p,"%02x",(unsigned char)g2->emblem_data[i]);
		}
		p += sprintf(p,"'");
		sep = ',';
	}

	if(sep == ',') {
		sprintf(p," WHERE `guild_id` = '%d'",g2->guild_id);
		sqldbs_query(&mysql_handle, tmp_sql);
	}

	// �����o�[
	if(memcmp(g1->member,g2->member,sizeof(g1->member))) {
		sqldbs_query(&mysql_handle, "DELETE FROM `" GUILD_MEMBER_TABLE "` WHERE `guild_id`='%d'", g2->guild_id);

		for(i=0;i < g2->max_member;i++) {
			if (g2->member[i].account_id>0){
				struct guild_member *m = &g2->member[i];
				sqldbs_query(
					&mysql_handle,
					"INSERT INTO `" GUILD_MEMBER_TABLE "` (`guild_id`,`account_id`,`char_id`,`hair`,`hair_color`,`gender`,"
					"`class`,`lv`,`exp`,`exp_payper`,`online`,`position`,`name`) VALUES "
					"('%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%d','%s')",
					g2->guild_id,m->account_id,m->char_id,m->hair,m->hair_color,m->gender,
					m->class_,m->lv,m->exp,m->exp_payper,(int)m->online,m->position,strecpy(buf,m->name)
				);
			}
		}
	}

	// ��E
	if(memcmp(g1->position,g2->position,sizeof(g1->position))) {
		sqldbs_query(&mysql_handle, "DELETE FROM `" GUILD_POSITION_TABLE "` WHERE `guild_id`='%d'", g2->guild_id);

		for(i=0;i<MAX_GUILDPOSITION;i++){
			struct guild_position *pos = &g2->position[i];
			sqldbs_query(
				&mysql_handle,
				"INSERT INTO `" GUILD_POSITION_TABLE "` (`guild_id`,`position`,`name`,`mode`,`exp_mode`) VALUES "
				"('%d','%d','%s','%d','%d')",
				g2->guild_id,i,strecpy(buf,pos->name),pos->mode,pos->exp_mode
			);
		}
	}

	// ����/�G�΃��X�g
	if(memcmp(g1->alliance,g2->alliance,sizeof(g1->alliance))) {
		sqldbs_query(&mysql_handle, "DELETE FROM `" GUILD_ALLIANCE_TABLE "` WHERE `guild_id`='%d'", g2->guild_id);

		for(i=0;i<MAX_GUILDALLIANCE;i++){
			struct guild_alliance *a = &g2->alliance[i];
			if(a->guild_id>0){
				sqldbs_query(
					&mysql_handle,
					"INSERT INTO `" GUILD_ALLIANCE_TABLE "` (`guild_id`,`opposition`,`alliance_id`,`name`) VALUES "
					"('%d','%d','%d','%s')",
					g2->guild_id,a->opposition,a->guild_id,strecpy(buf,a->name)
				);
			}
		}
	}

	// �Ǖ����X�g
	if(memcmp(g1->explusion,g2->explusion,sizeof(g1->explusion))) {
		sqldbs_query(&mysql_handle, "DELETE FROM `" GUILD_EXPULSION_TABLE "` WHERE `guild_id`='%d'", g2->guild_id);

		for(i=0;i<MAX_GUILDEXPLUSION;i++) {
			struct guild_explusion *e = &g2->explusion[i];
			if(e->account_id>0) {
				sqldbs_query(
					&mysql_handle,
					"INSERT INTO `" GUILD_EXPULSION_TABLE "` (`guild_id`,`name`,`mes`,`account_id`) VALUES "
					"('%d','%s','%s','%d')",
					g2->guild_id,strecpy(buf,e->name),strecpy(buf2,e->mes),e->account_id
				);
			}
		}
	}

	// �M���h�X�L��
	if(memcmp(g1->skill,g2->skill,sizeof(g1->skill))) {
		sqldbs_query(&mysql_handle, "DELETE FROM `" GUILD_SKILL_TABLE "` WHERE `guild_id`='%d'", g2->guild_id);

		for(i=0;i<MAX_GUILDSKILL;i++) {
			if (g2->skill[i].id > 0) {
				sqldbs_query(
					&mysql_handle,
					"INSERT INTO `" GUILD_SKILL_TABLE "` (`guild_id`,`id`,`lv`) VALUES "
					"('%d','%d','%d')",
					g2->guild_id,g2->skill[i].id,g2->skill[i].lv
				);
			}
		}
	}

	{
		struct guild* g3 = (struct guild *)numdb_search(guild_db,g2->guild_id);
		if(g3)
			memcpy(g3,g2,sizeof(struct guild));
	}
	guild_guildcastle_save();

	return 1;
}

// �M���h���U�����p�i����/�G�΂������j
// SQL �ォ������Ȃ�A��������̃M���h�f�[�^�������Ȃ��Ƃ����Ȃ�
static int guild_sql_delete_sub(void *key,void *data,va_list ap)
{
	struct guild *g=(struct guild *)data;
	int guild_id=va_arg(ap,int);
	int i;

	for(i=0;i<MAX_GUILDALLIANCE;i++){
		if(g->alliance[i].guild_id==guild_id)
			g->alliance[i].guild_id=0;
	}
	return 0;
}

void guild_sql_delete(int guild_id)
{
	int i;
	struct guild* g = (struct guild *)numdb_search(guild_db,guild_id);

	if(g) {
		numdb_erase(guild_db,g->guild_id);
		aFree(g);
	}
	numdb_foreach(guild_db,guild_sql_delete_sub,guild_id);
	gstorage_delete(guild_id);
	mapif_guild_broken(guild_id,0);

	sqldbs_query(&mysql_handle, "DELETE FROM `" GUILD_TABLE "` WHERE `guild_id`='%d'", guild_id);
	sqldbs_query(&mysql_handle, "DELETE FROM `" GUILD_MEMBER_TABLE "` WHERE `guild_id`='%d'", guild_id);
	sqldbs_query(&mysql_handle, "DELETE FROM `" GUILD_POSITION_TABLE "` WHERE `guild_id`='%d'", guild_id);
	sqldbs_query(&mysql_handle, "DELETE FROM `" GUILD_ALLIANCE_TABLE "` WHERE `guild_id`='%d' OR `alliance_id`='%d'", guild_id, guild_id);
	sqldbs_query(&mysql_handle, "DELETE FROM `" GUILD_EXPULSION_TABLE "` WHERE `guild_id`='%d'", guild_id);
	sqldbs_query(&mysql_handle, "DELETE FROM `" GUILD_SKILL_TABLE "` WHERE `guild_id`='%d'", guild_id);

	for(i=0;i<MAX_GUILDCASTLE;i++) {
		if(castle_db[i].guild_id == guild_id) {
			memset(&castle_db[i],0,sizeof(castle_db[0]));
			castle_db[i].castle_id = i;
		}
	}
}

int guild_sql_new(struct guild *g)
{
	int rc;
	char t_name[64];
	MYSQL_RES* sql_res;
	MYSQL_ROW  sql_row = NULL;

	// �M���hID��ǂݏo��
	rc = sqldbs_query(&mysql_handle, "SELECT MAX(`guild_id`) FROM `" GUILD_TABLE "`");
	if(rc)
		return 0;

	sql_res = sqldbs_store_result(&mysql_handle);
	if(!sql_res)
		return 0;

	sql_row = sqldbs_fetch(sql_res);
	if(sql_row[0]) {
		g->guild_id = atoi(sql_row[0]) + 1;
	} else {
		g->guild_id = 10000;
	}
	sqldbs_free_result(sql_res);

	// DB�ɑ}��
	sqldbs_query(
		&mysql_handle,
		"INSERT INTO `" GUILD_TABLE "` (`guild_id`,`name`,`guild_lv`,`max_member`,`emblem_data`) VALUES ('%d','%s','1','%d','')",
		g->guild_id,strecpy(t_name,g->name),g->max_member
	);

	guild_sql_save(g);
	return 1;
}

void guild_sql_config_read_sub(const char *w1,const char* w2)
{
	// nothing to do
}

static int guild_sql_db_final(void *key,void *data,va_list ap)
{
	struct guild *g = (struct guild *)data;

	aFree(g);
	return 0;
}

void guild_sql_final(void)
{
	if(guild_db)
		numdb_final(guild_db,guild_sql_db_final);
}

#define guild_final    guild_sql_final
#define guild_new      guild_sql_new
#define guild_save     guild_sql_save
#define guild_init     guild_sql_init
#define guild_sync     guild_sql_sync
#define guild_load_num guild_sql_load_num
#define guild_load_str guild_sql_load_str
#define guild_delete   guild_sql_delete
#define guild_config_read_sub guild_sql_config_read_sub

#endif /* TXT_ONLY */


// �M���h���󂩂ǂ����`�F�b�N
int guild_check_empty(const struct guild *g)
{
	int i;

	for(i=0;i<g->max_member;i++){
		if(g->member[i].account_id>0){
			return 0;
		}
	}
	return 1;
}

// �L�����̋������Ȃ����`�F�b�N
// �Ƃ肠�����ȗ�
int guild_check_conflict(int guild_id,int account_id,int char_id)
{
	return 0;
}

int guild_nextexp(int level)
{
	if(level == 0) return 1;

	if(level < MAX_GUILDLEVEL && level > 0)
		return guild_exp[level-1];

	return 0;
}

// �M���h�X�L�������邩�m�F
int guild_checkskill(const struct guild *g,int id)
{
	int idx = id - GUILD_SKILLID;

	if (idx < 0 || idx >= MAX_GUILDSKILL)
		return 0;
	return g->skill[idx].lv;
}

// �M���h�X�L���c���[�v�Z
void guild_calc_skilltree(struct guild *g)
{
	int i, id, flag;

	if(g == NULL)
		return;

	do {
		flag = 0;
		for(i = 0; i < MAX_GUILDSKILL && (id = guild_skill_tree[i].id) > 0; i++) {
			if(g->skill[i].id <= 0) {
				int j, skillid, fail = 0;
				for(j = 0; j < 5 && (skillid = guild_skill_tree[i].need[j].id) > 0; j++) {
					if(guild_checkskill(g,skillid) < guild_skill_tree[i].need[j].lv) {
						fail = 1;
						break;
					}
				}
				if(!fail) {
					g->skill[i].id = id;
					flag = 1;
				}
			}
		}
	} while(flag);

	return;
}

// �M���h�̏��̍Čv�Z
int guild_calcinfo(struct guild *g)
{
	int i,c,nextexp;
	int sum = 0;
	struct guild before = *g;

	// �M���h���x��
	if(g->guild_lv <= 0)
		g->guild_lv = 1;
	if(g->guild_lv > MAX_GUILDLEVEL)
		g->guild_lv = MAX_GUILDLEVEL;
	nextexp = guild_nextexp(g->guild_lv);

	while(nextexp > 0 && g->exp >= nextexp){	// ���x���A�b�v����
		g->exp-=nextexp;
		g->guild_lv++;
		g->skill_point++;
		nextexp = guild_nextexp(g->guild_lv);
	}

	// �M���h�̎��̌o���l
	g->next_exp = nextexp;

	// �����o����i�M���h�g���K�p�j
	g->max_member = 16 + guild_checkskill(g, GD_EXTENSION) * guild_extension_increment;
	if(g->max_member > MAX_GUILD)
		g->max_member = MAX_GUILD;

	// ���σ��x���ƃI�����C���l��
	g->average_lv=0;
	g->connect_member=0;
	for(i=c=0;i<g->max_member;i++){
		if(g->member[i].account_id>0){
			sum += g->member[i].lv;
			c++;

			if(g->member[i].online>0)
				g->connect_member++;
		}
	}
	if(c != 0) {
		g->average_lv = (unsigned short)(sum / c);
	}

	// �S�f�[�^�𑗂�K�v�����肻��
	if( g->max_member!=before.max_member ||
	    g->guild_lv!=before.guild_lv ||
	    g->skill_point!=before.skill_point )
	{
		mapif_guild_info(-1,g);
		return 1;
	}

	return 0;
}

//-------------------------------------------------------------------
// map server�ւ̒ʐM

// �M���h�쐬��
int mapif_guild_created(int fd,int account_id,const struct guild *g)
{
	WFIFOW(fd,0)=0x3830;
	WFIFOL(fd,2)=account_id;
	if(g!=NULL){
		WFIFOL(fd,6)=g->guild_id;
		printf("int_guild: created! %d %s\n",g->guild_id,g->name);
	}else{
		WFIFOL(fd,6)=0;
	}
	WFIFOSET(fd,10);
	return 0;
}

// �M���h��񌩂��炸
int mapif_guild_noinfo(int fd,int guild_id)
{
	WFIFOW(fd,0)=0x3831;
	WFIFOW(fd,2)=8;
	WFIFOL(fd,4)=guild_id;
	WFIFOSET(fd,8);
	printf("int_guild: info not found %d\n",guild_id);
	return 0;
}

// �M���h���܂Ƃߑ���
int mapif_guild_info(int fd,const struct guild *g)
{
	unsigned char *buf = (unsigned char *)aMalloc(4+sizeof(struct guild));

	WBUFW(buf,0)=0x3831;
	memcpy(buf+4,g,sizeof(struct guild));
	WBUFW(buf,2)=4+sizeof(struct guild);
	if(fd<0)
		mapif_sendall(buf,WBUFW(buf,2));
	else
		mapif_send(fd,buf,WBUFW(buf,2));
	aFree(buf);
	return 0;
}

// �����o�ǉ���
int mapif_guild_memberadded(int fd,int guild_id,int account_id,int char_id,int flag)
{
	WFIFOW(fd,0)=0x3832;
	WFIFOL(fd,2)=guild_id;
	WFIFOL(fd,6)=account_id;
	WFIFOL(fd,10)=char_id;
	WFIFOB(fd,14)=flag;
	WFIFOSET(fd,15);
	return 0;
}

// �E��/�Ǖ��ʒm
int mapif_guild_leaved(int guild_id,int account_id,int char_id,int flag,const char *name,const char *mes)
{
	unsigned char buf[128];

	WBUFW(buf, 0)=0x3834;
	WBUFL(buf, 2)=guild_id;
	WBUFL(buf, 6)=account_id;
	WBUFL(buf,10)=char_id;
	WBUFB(buf,14)=flag;
	strncpy(WBUFP(buf,15),mes,40);
	strncpy(WBUFP(buf,55),name,24);
	mapif_sendall(buf,79);
	printf("int_guild: guild leaved %d %d %s %s\n",guild_id,account_id,name,mes);
	return 0;
}

// �I�����C����Ԃ�Lv�X�V�ʒm
int mapif_guild_memberinfoshort(struct guild *g,int idx)
{
	unsigned char buf[32];

	WBUFW(buf, 0)=0x3835;
	WBUFL(buf, 2)=g->guild_id;
	WBUFL(buf, 6)=g->member[idx].account_id;
	WBUFL(buf,10)=g->member[idx].char_id;
	WBUFB(buf,14)=(unsigned char)g->member[idx].online;
	WBUFW(buf,15)=g->member[idx].lv;
	WBUFW(buf,17)=g->member[idx].class_;
	mapif_sendall(buf,19);
	return 0;
}

// ���U�ʒm
int mapif_guild_broken(int guild_id,int flag)
{
	unsigned char buf[16];

	WBUFW(buf,0)=0x3836;
	WBUFL(buf,2)=guild_id;
	WBUFB(buf,6)=flag;
	mapif_sendall(buf,7);
	printf("int_guild: broken %d\n",guild_id);
	return 0;
}

// �M���h������
int mapif_guild_message(int guild_id,int account_id,char *mes,int len)
{
	unsigned char buf[512];

	WBUFW(buf,0)=0x3837;
	WBUFW(buf,2)=len+12;
	WBUFL(buf,4)=guild_id;
	WBUFL(buf,8)=account_id;
	memcpy(WBUFP(buf,12),mes,len);
	mapif_sendall(buf,len+12);
	return 0;
}

// �M���h��{���ύX�ʒm
int mapif_guild_basicinfochanged(int guild_id,int type,const void *data,int len)
{
	unsigned char buf[2048];

	WBUFW(buf, 0)=0x3839;
	WBUFW(buf, 2)=len+10;
	WBUFL(buf, 4)=guild_id;
	WBUFW(buf, 8)=type;
	memcpy(WBUFP(buf,10),data,len);
	mapif_sendall(buf,len+10);
	return 0;
}

// �M���h�����o���ύX�ʒm
int mapif_guild_memberinfochanged(int guild_id,int account_id,int char_id,int type,const void *data,int len)
{
	unsigned char buf[2048];

	WBUFW(buf, 0)=0x383a;
	WBUFW(buf, 2)=len+18;
	WBUFL(buf, 4)=guild_id;
	WBUFL(buf, 8)=account_id;
	WBUFL(buf,12)=char_id;
	WBUFW(buf,16)=type;
	memcpy(WBUFP(buf,18),data,len);
	mapif_sendall(buf,len+18);
	return 0;
}

// �M���h�X�L���A�b�v�ʒm
int mapif_guild_skillupack(int guild_id,int skill_num,int account_id,int flag)
{
	unsigned char buf[16];

	WBUFW(buf, 0)=0x383c;
	WBUFL(buf, 2)=guild_id;
	WBUFL(buf, 6)=skill_num;
	WBUFL(buf,10)=account_id;
	WBUFB(buf,14)=flag;
	mapif_sendall(buf,15);
	return 0;
}

// �M���h����/�G�Βʒm
int mapif_guild_alliance(int guild_id1,int guild_id2,int account_id1,int account_id2,
	int flag,const char *name1,const char *name2)
{
	unsigned char buf[128];

	WBUFW(buf, 0)=0x383d;
	WBUFL(buf, 2)=guild_id1;
	WBUFL(buf, 6)=guild_id2;
	WBUFL(buf,10)=account_id1;
	WBUFL(buf,14)=account_id2;
	WBUFB(buf,18)=flag;
	memcpy(WBUFP(buf,19),name1,24);
	memcpy(WBUFP(buf,43),name2,24);
	mapif_sendall(buf,67);
	return 0;
}

// �M���h��E�ύX�ʒm
int mapif_guild_position(struct guild *g,int idx)
{
	unsigned char buf[128];

	WBUFW(buf,0)=0x383b;
	WBUFW(buf,2)=sizeof(struct guild_position)+12;
	WBUFL(buf,4)=g->guild_id;
	WBUFL(buf,8)=idx;
	memcpy(WBUFP(buf,12),&g->position[idx],sizeof(struct guild_position));
	mapif_sendall(buf,WBUFW(buf,2));
	return 0;
}

// �M���h���m�ύX�ʒm
int mapif_guild_notice(struct guild *g)
{
	unsigned char buf[256];

	WBUFW(buf,0)=0x383e;
	WBUFL(buf,2)=g->guild_id;
	memcpy(WBUFP(buf,6),g->mes1,60);
	memcpy(WBUFP(buf,66),g->mes2,120);
	mapif_sendall(buf,186);
	return 0;
}

// �M���h�G���u�����ύX�ʒm
int mapif_guild_emblem(struct guild *g)
{
	unsigned char buf[2048];

	WBUFW(buf,0)=0x383f;
	WBUFW(buf,2)=g->emblem_len+12;
	WBUFL(buf,4)=g->guild_id;
	WBUFL(buf,8)=g->emblem_id;
	memcpy(WBUFP(buf,12),g->emblem_data,g->emblem_len);
	mapif_sendall(buf,WBUFW(buf,2));
	return 0;
}

int mapif_guild_castle_dataload(int castle_id,int idx,int value)
{
	unsigned char buf[16];

	WBUFW(buf, 0)=0x3840;
	WBUFW(buf, 2)=castle_id;
	WBUFB(buf, 4)=idx;
	WBUFL(buf, 5)=value;
	mapif_sendall(buf,9);
	return 0;
}

int mapif_guild_castle_datasave(int castle_id,int idx,int value)
{
	unsigned char buf[16];

	WBUFW(buf, 0)=0x3841;
	WBUFW(buf, 2)=castle_id;
	WBUFB(buf, 4)=idx;
	WBUFL(buf, 5)=value;
	mapif_sendall(buf,9);
	return 0;
}

int mapif_guild_castle_alldataload(int fd)
{
	WFIFOW(fd,0)=0x3842;
	WFIFOW(fd,2)=4+sizeof(castle_db);
	memcpy(WFIFOP(fd,4), castle_db, sizeof(castle_db));
	WFIFOSET(fd,WFIFOW(fd,2));
	return 0;
}

int mapif_guild_skillmax_load(int fd)
{
	int i, len = 4;

	WFIFOW(fd,0) = 0x3843;
	for(i = 0; i < MAX_GUILDSKILL; i++) {
		WFIFOL(fd,len) = guild_skill_tree[i].max;
		len += 4;
	}
	WFIFOW(fd,2) = len;
	WFIFOSET(fd,len);
	return 0;
}

//-------------------------------------------------------------------
// map server����̒ʐM


// �M���h�쐬�v��
int mapif_parse_CreateGuild(int fd,int account_id,char *name,struct guild_member *master)
{
	struct guild g;
	int i;

	for(i=0;i<24 && name[i];i++){
		if( !(name[i]&0xe0) || name[i]==0x7f){
			printf("int_guild: illegal guild name [%s]\n",name);
			mapif_guild_created(fd,account_id,NULL);
			return 0;
		}
	}

	if(guild_load_str(name) !=NULL) {
		printf("int_guild: same name guild exists [%s]\n",name);
		mapif_guild_created(fd,account_id,NULL);
		return 0;
	}
	memset(&g,0,sizeof(struct guild));
	memcpy(g.name,name,24);
	memcpy(g.master,master->name,24);
	memcpy(&g.member[0],master,sizeof(struct guild_member));

	g.position[0].mode=0x11;

	strncpy(g.position[0].name,"GuildMaster",24);
	for(i=1;i<MAX_GUILDPOSITION-1;i++) {
		sprintf(g.position[i].name,"Position %d",i+1);
	}
	strncpy(g.position[MAX_GUILDPOSITION-1].name,"Newbie",24);

	// �����ŃM���h���v�Z���K�v�Ǝv����
	g.max_member = (MAX_GUILD > 16)? 16: MAX_GUILD;
	g.average_lv = master->lv;
	g.guild_lv   = 1;

	guild_calc_skilltree(&g);
	guild_new(&g);

	mapif_guild_created(fd,account_id,&g);
	mapif_guild_info(-1,&g);

	inter_log("guild %s (id=%d) created by master %s (id=%d)",
		name, g.guild_id, master->name, master->account_id);

	return 0;
}

// �M���h���v��
int mapif_parse_GuildInfo(int fd,int guild_id)
{
	const struct guild *g = guild_load_num(guild_id);

	if(g == NULL){
		// ���݂��Ȃ��M���h
		mapif_guild_noinfo(fd,guild_id);
	} else if(guild_check_empty(g)) {
		// �����o�[�����Ȃ��̂ŉ��U����
		guild_delete(guild_id); // �c�[�폜
		mapif_guild_noinfo(fd,guild_id);
		return 0;
	} else {
		// �M���h��񑗐M
		mapif_guild_info(fd,g);
	}
	return 0;
}

// �M���h�����o�ǉ��v��
int mapif_parse_GuildAddMember(int fd,int guild_id,struct guild_member *m)
{
	const struct guild *g1 = guild_load_num(guild_id);
	struct guild g2;
	int i;

	if(g1 == NULL){
		mapif_guild_memberadded(fd,guild_id,m->account_id,m->char_id,1);
		return 0;
	}

	memcpy(&g2,g1,sizeof(struct guild));
	for(i=0;i<g2.max_member;i++){
		if(guild_join_limit && g2.member[i].account_id==m->account_id)
			break;
		if(g2.member[i].account_id==0){
			memcpy(&g2.member[i],m,sizeof(struct guild_member));
			mapif_guild_memberadded(fd,guild_id,m->account_id,m->char_id,0);
			if(guild_calcinfo(&g2) == 0)
				mapif_guild_info(-1,&g2);
			guild_save(&g2);
			return 0;
		}
	}
	mapif_guild_memberadded(fd,guild_id,m->account_id,m->char_id,1);
	return 0;
}

// �M���h�E��/�Ǖ��v��
int mapif_parse_GuildLeave(int fd,int guild_id,int account_id,int char_id,int flag,const char *mes)
{
	const struct guild *g1 = guild_load_num(guild_id);
	struct guild g2;
	int i;

	if(g1 == NULL)
		return 0;

	memcpy(&g2,g1,sizeof(struct guild));
	for(i=0;i<MAX_GUILD;i++){
		if(g2.member[i].account_id == account_id && g2.member[i].char_id == char_id)
		{
			if(flag) {	// �Ǖ��̏ꍇ�Ǖ����X�g�ɓ����
				int j;
				for(j=0;j<MAX_GUILDEXPLUSION;j++){
					if(g2.explusion[j].account_id==0)
						break;
				}
				if(j>=MAX_GUILDEXPLUSION) {	// ��t�Ȃ̂ŌÂ��̂�����
					j=MAX_GUILDEXPLUSION-1;
					memmove(&g2.explusion[0],&g2.explusion[1],j*sizeof(g2.explusion[0]));
				}
				g2.explusion[j].account_id=account_id;
				strncpy(g2.explusion[j].name,g2.member[i].name,24);
				strncpy(g2.explusion[j].mes,mes,40);
			}

			mapif_guild_leaved(guild_id,account_id,char_id,flag,g2.member[i].name,mes);
			memset(&g2.member[i],0,sizeof(struct guild_member));

			if(fd >= 0) {
				// �L�����폜�łȂ��ꍇ�̓M���hID��0�ɏ�����
				const struct mmo_chardata *cd = char_load(char_id);
				if(cd) {
					struct mmo_charstatus st;
					memcpy(&st, &cd->st, sizeof(st));
					st.guild_id = 0;
					char_save(&st);
				}
			}

			if( guild_check_empty(&g2) ) {
				// ��f�[�^
				guild_delete(g2.guild_id);
			} else {
				guild_save(&g2);
				mapif_guild_info(-1,&g2);	// �܂��l������̂Ńf�[�^���M
			}
			return 0;
		}
	}

	return 0;
}

// �I�����C��/Lv�X�V
static int mapif_parse_GuildChangeMemberInfoShort(int fd,int guild_id,int account_id,int char_id,unsigned char online,int lv,int class_)
{
	const struct guild *g1 = guild_load_num(guild_id);
	int i,alv,c;
	struct guild g2;

	if(g1 == NULL){
		return 0;
	}
	memcpy(&g2,g1,sizeof(struct guild));

	g2.connect_member=0;

	for(i=0,alv=0,c=0;i<MAX_GUILD;i++){
		if( g2.member[i].account_id==account_id && g2.member[i].char_id==char_id ) {
			g2.member[i].online=online;
			g2.member[i].lv=lv;
			g2.member[i].class_=class_;
			mapif_guild_memberinfoshort(&g2,i);
		}
		if( g2.member[i].account_id>0 ){
			alv+=g2.member[i].lv;
			c++;
		}
		if( g2.member[i].online )
			g2.connect_member++;
	}
	// ���σ��x��
	if(c != 0) {
		g2.average_lv=alv/c;
	}
	guild_save(&g2);
	return 0;
}

// �M���h���U�v��
int mapif_parse_BreakGuild(int fd,int guild_id)
{
	guild_delete(guild_id);
	inter_log("guild (id=%d) broken",guild_id);
	return 0;
}

// �M���h���b�Z�[�W���M
int mapif_parse_GuildMessage(int fd,int guild_id,int account_id,char *mes,int len)
{
	return mapif_guild_message(guild_id,account_id,mes,len);
}

// �M���h��{�f�[�^�ύX�v��
int mapif_parse_GuildBasicInfoChange(int fd,int guild_id,int type,const char *data,int len)
{
	const struct guild *g1 = guild_load_num(guild_id);
	struct guild g2;

	if(g1 == NULL){
		return 0;
	}
	memcpy(&g2,g1,sizeof(struct guild));

	switch(type) {
	case GBI_GUILDLV:
		{
			short dw = *((short *)data);
			if(dw > 0 && g2.guild_lv + dw <= MAX_GUILDLEVEL) {
				g2.guild_lv    += dw;
				g2.skill_point += dw;
			} else if(dw < 0 && g2.guild_lv + dw >= 1) {
				g2.guild_lv += dw;
			}
		}
		break;
	case GBI_SKILLPOINT:
		g2.skill_point += *((int *)data);
		break;
	default:
		printf("int_guild: GuildBasicInfoChange: Unknown type %d\n",type);
		return 0;
	}
	mapif_guild_info(-1,&g2);
	guild_save(&g2);

	return 0;
}

// �M���h�����o�f�[�^�ύX�v��
int mapif_parse_GuildMemberInfoChange(int fd,int guild_id,int account_id,int char_id,int type,const char *data,int len)
{
	int i;
	const struct guild *g1 = guild_load_num(guild_id);
	struct guild g2;
	const void *p = NULL;

	if(g1 == NULL){
		return 0;
	}
	memcpy(&g2,g1,sizeof(struct guild));
	for(i=0;i<g2.max_member;i++) {
		if(g2.member[i].account_id==account_id && g2.member[i].char_id==char_id)
			break;
	}
	if(i == g2.max_member){
		printf("int_guild: GuildMemberChange: Not found %d,%d in %d[%s]\n",
			account_id,char_id,guild_id,g2.name);
		return 0;
	}
	switch(type){
	case GMI_POSITION:	// ��E
		g2.member[i].position=*((int *)data);
		break;
	case GMI_EXP:
		{	// EXP
			atn_bignumber tmp;
			int exp = *((int *)data);

			tmp = (atn_bignumber)g2.member[i].exp + exp;
			g2.member[i].exp = (tmp > 0x7fffffff)? 0x7fffffff: (tmp < 0)? 0: (int)tmp;

			tmp = (atn_bignumber)g2.exp + exp;
			g2.exp = (tmp > 0x7fffffff)? 0x7fffffff: (tmp < 0)? 0: (int)tmp;

			guild_calcinfo(&g2);	// Lv�A�b�v���f
			mapif_guild_basicinfochanged(guild_id,GBI_EXP,&g2.exp,sizeof(g2.exp));
			p = &g2.member[i].exp;
		}
		break;
	default:
		printf("int_guild: GuildMemberInfoChange: Unknown type %d\n",type);
		return 0;
	}
	mapif_guild_memberinfochanged(guild_id,account_id,char_id,type,(p == NULL)? data: p,len);
	guild_save(&g2);
	return 0;
}

// �M���h��E���ύX�v��
int mapif_parse_GuildPosition(int fd,int guild_id,int idx,struct guild_position *p)
{
	const struct guild *g1 = guild_load_num(guild_id);
	struct guild g2;

	if(g1 == NULL || idx<0 || idx>=MAX_GUILDPOSITION){
		return 0;
	}
	memcpy(&g2,g1,sizeof(struct guild));
	memcpy(&g2.position[idx],p,sizeof(struct guild_position));
	{
		unsigned char *p2 = g2.position[idx].name;
		int limit = sizeof(g2.position[0].name);
		while(*p2 && --limit) {
			if(*p2 < 0x20) *p2 = '.';
			p2++;
		}
	}
	mapif_guild_position(&g2,idx);
	guild_save(&g2);
	printf("int_guild: position changed %d\n",idx);
	return 0;
}

// �M���h�X�L���A�b�v�v��
int mapif_parse_GuildSkillUp(int fd,int guild_id,int skill_num,int account_id,int level,unsigned char flag)
{
	const struct guild *g1 = guild_load_num(guild_id);
	struct guild g2;
	int idx = skill_num - GUILD_SKILLID;
	int succeed = 0;

	if (g1 == NULL || idx < 0 || idx >= MAX_GUILDSKILL)
		return 0;

	memcpy(&g2,g1,sizeof(struct guild));

	if(g2.skill[idx].id > 0) {
		if(level < 0) {
			if(!(flag&1)) {
				g2.skill_point += g2.skill[idx].lv;
			}
			g2.skill[idx].lv = 0;
			succeed = 1;
		} else {
			if((g2.skill_point > 0 || flag&1) && g2.skill[idx].id > 0) {
				if(level == 0)
					level = g2.skill[idx].lv + 1;
				if(level > guild_skill_tree[idx].max)
					level = guild_skill_tree[idx].max;

				if(!(flag&1)) {
					g2.skill_point -= level - g2.skill[idx].lv;
				}
				g2.skill[idx].lv = level;
				succeed = 1;
			}
		}
	}

	if(succeed) {
		guild_calc_skilltree(&g2);
		if(guild_calcinfo(&g2) == 0)
			mapif_guild_info(-1, &g2);
		mapif_guild_skillupack(guild_id,skill_num,account_id,1);
		guild_save(&g2);

		if(level >= 0)
			printf("int_guild: %d skill %d up %d\n", guild_id, skill_num, level);
		else
			printf("int_guild: %d skill %d lost\n", guild_id, skill_num);
	} else {
		mapif_guild_skillupack(guild_id,skill_num,account_id,0);
	}
	return 0;
}

// �M���h�����v��
int mapif_parse_GuildAlliance(int fd,int guild_id1,int guild_id2,int account_id1,int account_id2,int flag)
{
	const struct guild *g1[2];
	struct guild g2[2];
	int j,i;

	g1[0] = guild_load_num(guild_id1);
	g1[1] = guild_load_num(guild_id2);
	if(g1[0]==NULL || g1[1]==NULL) return 0;

	memcpy(&g2[0],g1[0],sizeof(struct guild));
	memcpy(&g2[1],g1[1],sizeof(struct guild));

	if(!(flag&0x8)){
		for(i=0;i<2-(flag&1);i++){
			for(j=0;j<MAX_GUILDALLIANCE;j++) {
				if(g2[i].alliance[j].guild_id==0) {
					g2[i].alliance[j].guild_id = g2[1-i].guild_id;
					memcpy(g2[i].alliance[j].name,g2[1-i].name,24);
					g2[i].alliance[j].opposition = flag&1;
					break;
				}
			}
		}
	} else {	// �֌W����
		for(i=0;i<2-(flag&1);i++){
			for(j=0;j<MAX_GUILDALLIANCE;j++) {
				if( g2[i].alliance[j].guild_id == g2[1-i].guild_id &&
				    g2[i].alliance[j].opposition == (flag&1))
				{
					g2[i].alliance[j].guild_id=0;
					break;
				}
			}
		}
	}
	mapif_guild_alliance(guild_id1,guild_id2,account_id1,account_id2,flag,g2[0].name,g2[1].name);
	guild_save(&g2[0]);
	guild_save(&g2[1]);
	return 0;
}

// �M���h���m�ύX�v��
int mapif_parse_GuildNotice(int fd,int guild_id,const char *mes1,const char *mes2)
{
	const struct guild *g1 = guild_load_num(guild_id);
	struct guild g2;
	unsigned char *p2;
	int limit;

	if(g1 == NULL)
		return 0;
	memcpy(&g2,g1,sizeof(struct guild));
	memcpy(g2.mes1,mes1,60);
	memcpy(g2.mes2,mes2,120);

	p2    = g2.mes1;
	limit = sizeof(g2.mes1);

	while(*p2 && --limit) {
		if(*p2 < 0x20) *p2 = '.';
		p2++;
	}
	p2 = g2.mes2;
	limit = sizeof(g2.mes2);
	while(*p2 && --limit) {
		if(*p2 < 0x20) *p2 = '.';
		p2++;
	}
	guild_save(&g2);
	return mapif_guild_notice(&g2);
}

// �M���h�G���u�����ύX�v��
int mapif_parse_GuildEmblem(int fd,int len,int guild_id,int dummy,const char *data)
{
	const struct guild *g1 = guild_load_num(guild_id);
	struct guild g2;

	if(g1 == NULL)
		return 0;
	memcpy(&g2,g1,sizeof(struct guild));
	memcpy(g2.emblem_data,data,len);
	g2.emblem_len=len;
	g2.emblem_id++;
	guild_save(&g2);
	return mapif_guild_emblem(&g2);
}

int mapif_parse_GuildCastleDataLoad(int fd,int castle_id,int idx)
{
	struct guild_castle *gc;

	if(castle_id < 0 || castle_id >= MAX_GUILDCASTLE) {
		return mapif_guild_castle_dataload(castle_id,0,0);
	}
	gc = &castle_db[castle_id];

	switch(idx){
		case 1:  return mapif_guild_castle_dataload(gc->castle_id,idx,gc->guild_id);   break;
		case 2:  return mapif_guild_castle_dataload(gc->castle_id,idx,gc->economy);    break;
		case 3:  return mapif_guild_castle_dataload(gc->castle_id,idx,gc->defense);    break;
		case 4:  return mapif_guild_castle_dataload(gc->castle_id,idx,gc->triggerE);   break;
		case 5:  return mapif_guild_castle_dataload(gc->castle_id,idx,gc->triggerD);   break;
		case 6:  return mapif_guild_castle_dataload(gc->castle_id,idx,gc->nextTime);   break;
		case 7:  return mapif_guild_castle_dataload(gc->castle_id,idx,gc->payTime);    break;
		case 8:  return mapif_guild_castle_dataload(gc->castle_id,idx,gc->createTime); break;
		case 9:  return mapif_guild_castle_dataload(gc->castle_id,idx,gc->visibleC);   break;
		case 10: return mapif_guild_castle_dataload(gc->castle_id,idx,gc->visibleG0);  break;
		case 11: return mapif_guild_castle_dataload(gc->castle_id,idx,gc->visibleG1);  break;
		case 12: return mapif_guild_castle_dataload(gc->castle_id,idx,gc->visibleG2);  break;
		case 13: return mapif_guild_castle_dataload(gc->castle_id,idx,gc->visibleG3);  break;
		case 14: return mapif_guild_castle_dataload(gc->castle_id,idx,gc->visibleG4);  break;
		case 15: return mapif_guild_castle_dataload(gc->castle_id,idx,gc->visibleG5);  break;
		case 16: return mapif_guild_castle_dataload(gc->castle_id,idx,gc->visibleG6);  break;
		case 17: return mapif_guild_castle_dataload(gc->castle_id,idx,gc->visibleG7);  break;
		default:
			printf("mapif_parse_GuildCastleDataLoad ERROR!! (Not found index=%d)\n", idx);
			break;
	}
	return 0;
}

int mapif_parse_GuildCastleDataSave(int fd,int castle_id,int idx,int value)
{
	struct guild_castle *gc;

	if(castle_id < 0 || castle_id >= MAX_GUILDCASTLE) {
		return mapif_guild_castle_datasave(castle_id,idx,value);
	}
	gc = &castle_db[castle_id];

	switch(idx){
		case 1:
			if( gc->guild_id != value ) {
				inter_log(
					"guild id=%d %s castle id=%d",
					((value)? value: gc->guild_id), ((value)? "occupy": "abandon"), idx
				);
			}
			gc->guild_id = value;
			break;
		case 2:  gc->economy    = value; break;
		case 3:  gc->defense    = value; break;
		case 4:  gc->triggerE   = value; break;
		case 5:  gc->triggerD   = value; break;
		case 6:  gc->nextTime   = value; break;
		case 7:  gc->payTime    = value; break;
		case 8:  gc->createTime = value; break;
		case 9:  gc->visibleC   = value; break;
		case 10: gc->visibleG0  = value; break;
		case 11: gc->visibleG1  = value; break;
		case 12: gc->visibleG2  = value; break;
		case 13: gc->visibleG3  = value; break;
		case 14: gc->visibleG4  = value; break;
		case 15: gc->visibleG5  = value; break;
		case 16: gc->visibleG6  = value; break;
		case 17: gc->visibleG7  = value; break;
		default:
			printf("mapif_parse_GuildCastleDataSave ERROR!! (Not found index=%d)\n", idx);
			return 0;
	}
#if defined(TXT_ONLY) && defined(TXT_JOURNAL)
	if( guildcastle_journal_enable )
		journal_write( &guildcastle_journal, gc->castle_id, gc );
#endif
	return mapif_guild_castle_datasave(gc->castle_id,idx,value);
}

// �M���h�`�F�b�N�v��
int mapif_parse_GuildCheck(int fd,int guild_id,int account_id,int char_id)
{
	return guild_check_conflict(guild_id,account_id,char_id);
}


// map server ����̒ʐM
// �E�P�p�P�b�g�̂݉�͂��邱��
// �E�p�P�b�g���f�[�^��inter.c�ɃZ�b�g���Ă�������
// �E�p�P�b�g���`�F�b�N��ARFIFOSKIP�͌Ăяo�����ōs����̂ōs���Ă͂Ȃ�Ȃ�
// �E�G���[�Ȃ�0(false)�A�����łȂ��Ȃ�1(true)���������Ȃ���΂Ȃ�Ȃ�
int inter_guild_parse_frommap(int fd)
{
	switch(RFIFOW(fd,0)){
	case 0x3030: mapif_parse_CreateGuild(fd,RFIFOL(fd,4),RFIFOP(fd,8),(struct guild_member *)RFIFOP(fd,32)); break;
	case 0x3031: mapif_parse_GuildInfo(fd,RFIFOL(fd,2)); break;
	case 0x3032: mapif_parse_GuildAddMember(fd,RFIFOL(fd,4),(struct guild_member *)RFIFOP(fd,8)); break;
	case 0x3034: mapif_parse_GuildLeave(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOB(fd,14),RFIFOP(fd,15)); break;
	case 0x3035: mapif_parse_GuildChangeMemberInfoShort(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOB(fd,14),RFIFOW(fd,15),RFIFOW(fd,17)); break;
	case 0x3036: mapif_parse_BreakGuild(fd,RFIFOL(fd,2)); break;
	case 0x3037: mapif_parse_GuildMessage(fd,RFIFOL(fd,4),RFIFOL(fd,8),RFIFOP(fd,12),RFIFOW(fd,2)-12); break;
	case 0x3038: mapif_parse_GuildCheck(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10)); break;
	case 0x3039: mapif_parse_GuildBasicInfoChange(fd,RFIFOL(fd,4),RFIFOW(fd,8),RFIFOP(fd,10),RFIFOW(fd,2)-10); break;
	case 0x303A: mapif_parse_GuildMemberInfoChange(fd,RFIFOL(fd,4),RFIFOL(fd,8),RFIFOL(fd,12),RFIFOW(fd,16),RFIFOP(fd,18),RFIFOW(fd,2)-18); break;
	case 0x303B: mapif_parse_GuildPosition(fd,RFIFOL(fd,4),RFIFOL(fd,8),(struct guild_position *)RFIFOP(fd,12)); break;
	case 0x303C: mapif_parse_GuildSkillUp(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOL(fd,14),RFIFOB(fd,18)); break;
	case 0x303D: mapif_parse_GuildAlliance(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOL(fd,10),RFIFOL(fd,14),RFIFOL(fd,18)); break;
	case 0x303E: mapif_parse_GuildNotice(fd,RFIFOL(fd,2),RFIFOP(fd,6),RFIFOP(fd,66)); break;
	case 0x303F: mapif_parse_GuildEmblem(fd,RFIFOW(fd,2)-12,RFIFOL(fd,4),RFIFOL(fd,8),RFIFOP(fd,12)); break;
	case 0x3040: mapif_parse_GuildCastleDataLoad(fd,RFIFOW(fd,2),RFIFOB(fd,4)); break;
	case 0x3041: mapif_parse_GuildCastleDataSave(fd,RFIFOW(fd,2),RFIFOB(fd,4),RFIFOL(fd,5)); break;

	default:
		return 0;
	}
	return 1;
}

// �}�b�v�T�[�o�[�̐ڑ�������
int inter_guild_mapif_init(int fd)
{
	mapif_guild_castle_alldataload(fd);
	mapif_guild_skillmax_load(fd);
	return 0;
}

// �T�[�o�[����E�ޗv���i�L�����폜�p�j
int inter_guild_leave(int guild_id,int account_id,int char_id)
{
	return mapif_parse_GuildLeave(-1,guild_id,account_id,char_id,0,"**�T�[�o�[����**");
}

// �M���h�ݒ�ǂݍ���
void guild_config_read(const char *w1,const char* w2)
{
	if(strcmpi(w1,"guild_extension_increment")==0)
	{
		guild_extension_increment = atoi(w2);
		if( guild_extension_increment<0 ) guild_extension_increment=0;
		if( guild_extension_increment>6 ) guild_extension_increment=6;
	}
	else if(strcmpi(w1,"guild_join_limit")==0)
	{
		guild_join_limit = atoi(w2);
	}
	else
	{
		guild_config_read_sub(w1,w2);
	}
}
