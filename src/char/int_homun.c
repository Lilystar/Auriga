
#define _INT_HOM_C_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mmo.h"
#include "inter.h"
#include "int_homun.h"
#include "char.h"
#include "socket.h"
#include "db.h"
#include "lock.h"
#include "malloc.h"
#include "journal.h"

#ifdef TXT_ONLY

static char homun_txt[1024]="save/homun.txt";
static struct dbt *homun_db;
static int homun_newid = 100;

#ifdef TXT_JOURNAL
static int homun_journal_enable = 1;
static struct journal homun_journal;
static char homun_journal_file[1024]="./save/homun.journal";
static int homun_journal_cache = 1000;
#endif

void homun_txt_config_read_sub(const char* w1,const char *w2) {
	if(strcmpi(w1,"homun_txt")==0){
		strncpy(homun_txt,w2,sizeof(homun_txt));
	}
#ifdef TXT_JOURNAL
	else if(strcmpi(w1,"homun_journal_enable")==0){
		homun_journal_enable = atoi( w2 );
	}
	else if(strcmpi(w1,"homun_journal_file")==0){
		strncpy( homun_journal_file, w2, sizeof(homun_journal_file) );
	}
	else if(strcmpi(w1,"homun_journal_cache_interval")==0){
		homun_journal_cache = atoi( w2 );
	}
#endif
}

int homun_tostr(char *str,struct mmo_homunstatus *h)
{
	int i;
	char *str_p = str;
	unsigned short sk_lv;

	if(!h) return 0;

	if(h->hungry < 0)
		h->hungry = 0;
	else if(h->hungry > 100)
		h->hungry = 100;
	if(h->intimate < 0)
		h->intimate = 0;
	else if(h->intimate > 100000)
		h->intimate = 100000;

	str_p += sprintf(str,"%d,%d,%s\t%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d\t%d,%d,%d,%d,%d",
		h->homun_id,h->class,h->name,
		h->account_id,h->char_id,
		h->base_level,h->base_exp,h->max_hp,h->hp,h->max_sp,h->sp,
		h->str,h->agi,h->vit,h->int_,h->dex,h->luk,
		h->status_point,h->skill_point,
		h->equip,h->intimate,h->hungry,h->rename_flag,h->incubate);

	*(str_p++)='\t';

	for(i=0;i<MAX_HOMSKILL;i++)
		if(h->skill[i].id && h->skill[i].flag!=1){
			sk_lv = (h->skill[i].flag==0)?h->skill[i].lv:h->skill[i].flag-2;
			str_p += sprintf(str_p,"%d,%d ",h->skill[i].id,sk_lv);
		}
	*(str_p++)='\t';

	*str_p='\0';
	return 0;
}

int homun_fromstr(char *str,struct mmo_homunstatus *h)
{
	int i,s,next,set,len;
	int tmp_int[23];
	char tmp_str[256];

	if(!h) return 0;
	memset(h,0,sizeof(struct mmo_homunstatus));

//	printf("sscanf homun main info\n");
	s=sscanf(str,"%d,%d,%[^\t]\t%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d\t%d,%d,%d,%d,%d%n",
		&tmp_int[0],&tmp_int[1],tmp_str,
		&tmp_int[2],&tmp_int[3],
		&tmp_int[4],&tmp_int[5],&tmp_int[6],&tmp_int[7],&tmp_int[8],&tmp_int[9],
		&tmp_int[10],&tmp_int[11],&tmp_int[12],&tmp_int[13],&tmp_int[14],&tmp_int[15],
		&tmp_int[16],&tmp_int[17],
		&tmp_int[18],&tmp_int[19],&tmp_int[20],&tmp_int[21],&tmp_int[22],&next);

	if(s!=24)
		return 1;

	h->homun_id = tmp_int[0];
	h->class = tmp_int[1];
	memcpy(h->name,tmp_str,24);
	h->account_id = tmp_int[2];
	h->char_id = tmp_int[3];
	h->base_level = tmp_int[4];
	h->base_exp = tmp_int[5];
	h->max_hp = tmp_int[6];
	h->hp = tmp_int[7];
	h->max_sp = tmp_int[8];
	h->sp = tmp_int[9];
	h->str = tmp_int[10];
	h->agi = tmp_int[11];
	h->vit = tmp_int[12];
	h->int_= tmp_int[13];
	h->dex = tmp_int[14];
	h->luk = tmp_int[15];
	h->status_point = tmp_int[16];
	h->skill_point = tmp_int[17];
	h->equip = tmp_int[18];
	h->intimate = tmp_int[19];
	h->hungry = tmp_int[20];
	h->rename_flag = tmp_int[21];
	h->incubate = tmp_int[22];
	h->option = 0;

	if(h->hungry < 0)
		h->hungry = 0;
	else if(h->hungry > 100)
		h->hungry = 100;
	if(h->intimate < 0)
		h->intimate = 0;
	else if(h->intimate > 100000)
		h->intimate = 100000;

	if(str[next]=='\n' || str[next]=='\r')
		return 1;	// �X�L�����Ȃ�

	next++;
	for(i=0;str[next] && str[next]!='\t';i++){
		set=sscanf(str+next,"%d,%d%n",
		&tmp_int[0],&tmp_int[1],&len);
		if(set!=2)
			return 0;
		tmp_int[2] = tmp_int[0]-HOM_SKILLID;
		if(tmp_int[2] >= 0 && tmp_int[2] < MAX_HOMSKILL) {
			h->skill[tmp_int[2]].id = tmp_int[0];
			h->skill[tmp_int[2]].lv = tmp_int[1];
		} else {
			printf("homun_fromstr: invaild skill id: %d\n", tmp_int[0]);
		}
		next+=len;
		if(str[next]==' ')
			next++;
	}
	return 0;
}

#ifdef TXT_JOURNAL
// ==========================================
// �z���f�[�^�̃W���[�i���̃��[���t�H���[�h�p�R�[���o�b�N�֐�
// ------------------------------------------
int homun_journal_rollforward( int key, void* buf, int flag )
{
	struct mmo_homunstatus* h = numdb_search( homun_db, key );

	// �O�̂��߃`�F�b�N
	if( flag == JOURNAL_FLAG_WRITE && key != ((struct mmo_homunstatus*)buf)->homun_id )
	{
		printf("int_homun: journal: key != homun_id !\n");
		return 0;
	}

	// �f�[�^�̒u������
	if( h )
	{
		if( flag == JOURNAL_FLAG_DELETE ) {
			numdb_erase(homun_db, key);
			aFree(h);
		} else {
			memcpy( h, buf, sizeof(struct mmo_homunstatus) );
		}
		return 1;
	}

	// �ǉ�
	if( flag != JOURNAL_FLAG_DELETE )
	{
		h = (struct mmo_homunstatus*) aCalloc( 1, sizeof( struct mmo_homunstatus ) );
		memcpy( h, buf, sizeof(struct mmo_homunstatus) );
		numdb_insert( homun_db, key, h );
		if( h->homun_id >= homun_newid)
			homun_newid=h->homun_id+1;
		return 1;
	}

	return 0;
}
int homun_txt_sync(void);
#endif

int homun_txt_init(void)
{
	char line[8192];
	struct mmo_homunstatus *h;
	FILE *fp;
	int c=0;

	homun_db=numdb_init();

	if( (fp=fopen(homun_txt,"r"))==NULL )
		return 1;
	while(fgets(line,sizeof(line),fp)){
		h=(struct mmo_homunstatus *)aCalloc(1,sizeof(struct mmo_homunstatus));
		if(homun_fromstr(line,h)==0 && h->homun_id>0){
			if( h->homun_id >= homun_newid)
				homun_newid=h->homun_id+1;
			numdb_insert(homun_db,h->homun_id,h);
		}else{
			printf("int_homun: broken data [%s] line %d\n",homun_txt,c);
			free(h);
		}
		c++;
	}
	fclose(fp);
//	printf("int_homun: %s read done (%d homs)\n",homun_txt,c);

#ifdef TXT_JOURNAL
	if( homun_journal_enable )
	{
		// �W���[�i���f�[�^�̃��[���t�H���[�h
		if( journal_load( &homun_journal, sizeof(struct mmo_homunstatus), homun_journal_file ) )
		{
			int c = journal_rollforward( &homun_journal, homun_journal_rollforward );

			printf("int_homun: journal: roll-forward (%d)\n", c );

			// ���[���t�H���[�h�����̂ŁAtxt �f�[�^��ۑ����� ( journal ���V�K�쐬�����)
			homun_txt_sync();
		}
		else
		{
			// �W���[�i����V�K�쐬����
			journal_final( &homun_journal );
			journal_create( &homun_journal, sizeof(struct mmo_homunstatus), homun_journal_cache, homun_journal_file );
		}
	}
#endif

	return 0;
}

int homun_txt_sync_sub(void *key,void *data,va_list ap)
{
	char line[8192];
	FILE *fp;
	homun_tostr(line,(struct mmo_homunstatus *)data);
	fp=va_arg(ap,FILE *);
	fprintf(fp,"%s" RETCODE,line);
	return 0;
}

int homun_txt_sync(void) {
	FILE *fp;
	int lock;

	if( !homun_db )
		return 1;

	if( (fp=lock_fopen(homun_txt,&lock))==NULL ){
		printf("int_homun: cant write [%s] !!! data is lost !!!\n",homun_txt);
		return 1;
	}
	numdb_foreach(homun_db,homun_txt_sync_sub,fp);
	lock_fclose(fp,homun_txt,&lock);
//	printf("int_homun: %s saved.\n",homun_txt);

#ifdef TXT_JOURNAL
	if( homun_journal_enable )
	{
		// �R�~�b�g�����̂ŃW���[�i����V�K�쐬����
		journal_final( &homun_journal );
		journal_create( &homun_journal, sizeof(struct mmo_homunstatus), homun_journal_cache, homun_journal_file );
	}
#endif

	return 0;
}

int homun_txt_delete(int homun_id) {
	struct mmo_homunstatus *p;
	p = numdb_search(homun_db,homun_id);
	if( p == NULL)
		return 1;
	else {
		numdb_erase(homun_db,homun_id);
		aFree(p);
		printf("homun_id: %d deleted\n",homun_id);

#ifdef TXT_JOURNAL
		if( homun_journal_enable )
			journal_write( &homun_journal, homun_id, NULL );
#endif
	}
	return 0;
}

const struct mmo_homunstatus* homun_txt_load(int homun_id) {
	return numdb_search(homun_db,homun_id);
}

int homun_txt_save(struct mmo_homunstatus* p2) {
	struct mmo_homunstatus* p1 = numdb_search(homun_db,p2->homun_id);
	if(p1 == NULL) {
		p1 = aMalloc(sizeof(struct mmo_homunstatus));
		numdb_insert(homun_db,p2->homun_id,p1);
	}
	memcpy(p1,p2,sizeof(struct mmo_homunstatus));

#ifdef TXT_JOURNAL
	if( homun_journal_enable )
		journal_write( &homun_journal, p1->homun_id, p1 );
#endif
	return 1;
}

int homun_txt_new(struct mmo_homunstatus *p2,int account_id,int char_id) {
	struct mmo_homunstatus *p1 = aMalloc(sizeof(struct mmo_homunstatus));
	p2->homun_id = homun_newid++;
	memcpy(p1,p2,sizeof(struct mmo_homunstatus));
	numdb_insert(homun_db,p2->homun_id,p1);
	return 0;
}

static int homun_txt_final_sub(void *key,void *data,va_list ap)
{
	struct mmo_homunstatus *p=data;

	free(p);

	return 0;
}

void homun_txt_final(void)
{
	if(homun_db)
		numdb_final(homun_db,homun_txt_final_sub);

#ifdef TXT_JOURNAL
	if( homun_journal_enable )
	{
		journal_final( &homun_journal );
	}
#endif
}

#define homun_init   homun_txt_init
#define homun_sync   homun_txt_sync
#define homun_delete homun_txt_delete
#define homun_load   homun_txt_load
#define homun_save   homun_txt_save
#define homun_new    homun_txt_new
#define homun_final  homun_txt_final

#else /* TXT_ONLY */
static char homun_db_[256]      = "homunculus";
static char homun_skill_db[256] = "homunculus_skill";
static struct dbt *homun_db;

int  homun_sql_init(void) {
	homun_db = numdb_init();
	return 0;
}

int  homun_sql_sync(void) {
	// nothing to do
	return 0;
}

int  homun_sql_delete(int homun_id) {
	struct mmo_homunstatus *p = numdb_search(homun_db,homun_id);
	if(p) {
		numdb_erase(homun_db,p->homun_id);
		aFree(p);
	}
	// printf("Request del  hom  (%6d)[",homun_id);
	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `homun_id`='%d'",homun_db_, homun_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle) );
	}
	sprintf(tmp_sql,"DELETE FROM `%s` WHERE `homun_id`='%d'",homun_skill_db, homun_id);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error - %s\n", mysql_error(&mysql_handle) );
	}
	// printf("]\n");
	return 0;
}

const struct mmo_homunstatus* homun_sql_load(int homun_id) {
	MYSQL_RES* sql_res;
	MYSQL_ROW  sql_row = NULL;
	struct mmo_homunstatus *p = numdb_search(homun_db,homun_id);

	if(p && p->homun_id == homun_id) {
		return p;
	}
	if(p == NULL) {
		p = aMalloc(sizeof(struct mmo_homunstatus));
		numdb_insert(homun_db,homun_id,p);
	}

	// printf("Request load hom  (%6d)[",homun_id);
	memset(p, 0, sizeof(struct mmo_homunstatus));

	//`hom` (`homun_id`, `class`,`name`,`account_id`,`char_id`,`base_level`,`base_exp`,
	//	`max_hp`,`hp`,`max_sp`,`sp`,`str`,`agi`,`vit`,`int`,`dex`,`luk`,
	//	`status_point`,`skill_point`,`equip`,`intimate`,`hungry`,`rename_flag`,`incubate`)
	sprintf(
		tmp_sql,
		"SELECT `homun_id`, `class`,`name`,`account_id`,`char_id`,`base_level`,`base_exp`,"
		"`max_hp`,`hp`,`max_sp`,`sp`,`str`,`agi`,`vit`,`int`,`dex`,`luk`,"
		"`status_point`,`skill_point`,`equip`,`intimate`,`hungry`,`rename_flag`,`incubate` "
		"FROM `%s` WHERE `homun_id`='%d'",
		homun_db_, homun_id
	);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error (select `hom`)- %s\n", mysql_error(&mysql_handle) );
		p->homun_id = -1;
		return NULL;
	}
	sql_res = mysql_store_result(&mysql_handle) ;
	if (sql_res!=NULL && mysql_num_rows(sql_res)>0) {
		sql_row = mysql_fetch_row(sql_res);

		p->homun_id = homun_id;
		p->class = atoi(sql_row[1]);
		memcpy(p->name,sql_row[2],24);
		p->account_id = atoi(sql_row[3]);
		p->char_id = atoi(sql_row[4]);
		p->base_level = atoi(sql_row[5]);
		p->base_exp = atoi(sql_row[6]);
		p->max_hp = atoi(sql_row[7]);
		p->hp = atoi(sql_row[8]);
		p->max_sp = atoi(sql_row[9]);
		p->sp = atoi(sql_row[10]);
		p->str = atoi(sql_row[11]);
		p->agi = atoi(sql_row[12]);
		p->vit = atoi(sql_row[13]);
		p->int_= atoi(sql_row[14]);
		p->dex = atoi(sql_row[15]);
		p->luk = atoi(sql_row[16]);
		p->status_point = atoi(sql_row[17]);
		p->skill_point = atoi(sql_row[18]);
		p->equip = atoi(sql_row[19]);
		p->intimate = atoi(sql_row[20]);
		p->hungry = atoi(sql_row[21]);
		p->rename_flag = atoi(sql_row[22]);
		p->incubate = atoi(sql_row[23]);
	} else {
		p->homun_id = -1;
		if( sql_res ) mysql_free_result(sql_res);
		return NULL;
	}
	mysql_free_result(sql_res);

	sprintf(
		tmp_sql,"SELECT `homun_id`,`id`,`lv` FROM `%s` WHERE `homun_id`='%d'",
		homun_skill_db, homun_id
	);
	if(mysql_query(&mysql_handle, tmp_sql) ) {
		printf("DB server Error (select `homun_skill`)- %s\n", mysql_error(&mysql_handle) );
		p->homun_id = -1;
		return NULL;
	}
	sql_res = mysql_store_result(&mysql_handle) ;
	if (sql_res!=NULL && mysql_num_rows(sql_res)>0) {
		int i;
		for(i=0;((sql_row = mysql_fetch_row(sql_res))&&i<MAX_HOMSKILL);i++){
			int id = atoi(sql_row[1]);
			if( id < HOM_SKILLID || id >= HOM_SKILLID + MAX_HOMSKILL ) {
				// DB���삵�ĕςȃX�L�����o����������\��������̂Ń`�F�b�N

				// unit.c�̂��A�łقƂ�ǂ̃X�L�����g�p���Ă����S�Ȃ͂������ǁA
				// �f�[�^�\���̕ύX�͖{�̂�����܂ŉ������Ă����܂��B
				printf("homun_sql_load: invaild skill id: %d\n", id);
			} else {
				p->skill[id-HOM_SKILLID].id = id;
				p->skill[id-HOM_SKILLID].lv = atoi(sql_row[2]);
			}
		}
	}
	mysql_free_result(sql_res);

	p->option = 0;
	if(p->hungry < 0)
		p->hungry = 0;
	else if(p->hungry > 100)
		p->hungry = 100;
	if(p->intimate < 0)
		p->intimate = 0;
	else if(p->intimate > 100000)
		p->intimate = 100000;

	 //printf("]\n");
	return p;
}

#define UPDATE_NUM(val,sql) \
	if(p1->val != p2->val) {\
		p += sprintf(p,"%c`"sql"` = '%d'",sep,p2->val); sep = ',';\
	}
#define UPDATE_STR(val,sql) \
	if(strcmp(p1->val,p2->val)) {\
		p += sprintf(p,"%c`"sql"` = '%s'",sep,strecpy(buf,p2->val)); sep = ',';\
	}

int  homun_sql_save(struct mmo_homunstatus* p2) {
	//`hom` (`homun_id`, `class`,`name`,`account_id`,`char_id`,`base_level`,`base_exp`,
	//	`max_hp`,`hp`,`max_sp`,`sp`,`str`,`agi`,`vit`,`int`,`dex`,`luk`,
	//	`status_point`,`skill_point`,`equip`,`intimate`,`hungry`,`rename_flag`,`incubate`)
	int  i;
	char sep, *p, buf[100];
	const struct mmo_homunstatus *p1 = homun_sql_load(p2->homun_id);
	if(p1 == NULL) return 0;

	// printf("Request save hom  (%6d)[",p2->homun_id);
	sep = ' ';
	// basic information
	p =  tmp_sql;
	p += sprintf(p,"UPDATE `%s` SET",homun_db_);
	UPDATE_NUM(class       ,"class");
	UPDATE_STR(name        ,"name");
	UPDATE_NUM(account_id  ,"account_id");
	UPDATE_NUM(char_id     ,"char_id");
	UPDATE_NUM(base_level  ,"base_level");
	UPDATE_NUM(base_exp    ,"base_exp");
	UPDATE_NUM(max_hp      ,"max_hp");
	UPDATE_NUM(hp          ,"hp");
	UPDATE_NUM(max_sp      ,"max_sp");
	UPDATE_NUM(sp          ,"sp");
	UPDATE_NUM(str         ,"str");
	UPDATE_NUM(agi         ,"agi");
	UPDATE_NUM(vit         ,"vit");
	UPDATE_NUM(int_        ,"int");
	UPDATE_NUM(dex         ,"dex");
	UPDATE_NUM(luk         ,"luk");
	UPDATE_NUM(status_point,"status_point");
	UPDATE_NUM(skill_point ,"skill_point");
	UPDATE_NUM(equip       ,"equip");
	UPDATE_NUM(intimate    ,"intimate");
	UPDATE_NUM(hungry      ,"hungry");
	UPDATE_NUM(rename_flag ,"rename_flag");
	UPDATE_NUM(incubate    ,"incubate");

	if(sep == ',') {
		sprintf(p," WHERE `homun_id` = '%d'",p2->homun_id);
		if (mysql_query(&mysql_handle, tmp_sql)) {
			printf("DB server Error - %s\n", mysql_error(&mysql_handle));
		}
		// printf("basic ");
	}

	if(memcmp(p1->skill, p2->skill, sizeof(p1->skill)) ) {
		sprintf(tmp_sql,"DELETE FROM `%s` WHERE `homun_id`='%d'",homun_skill_db,p2->homun_id);
		if(mysql_query(&mysql_handle, tmp_sql) ) {
			printf("DB server Error (delete `homun_id_skill`)- %s\n", mysql_error(&mysql_handle) );
		}
		p  = tmp_sql;
		p += sprintf(tmp_sql, "INSERT INTO `%s` (`homun_id`,`id`,`lv`) VALUES", homun_skill_db);
		sep = ' ';
		for(i=0;i<MAX_HOMSKILL;i++) {
			if(p2->skill[i].id && p2->skill[i].flag!=1){
				int lv = (p2->skill[i].flag==0)?p2->skill[i].lv:p2->skill[i].flag-2;
				p += sprintf(p,"%c('%d','%d','%d')", sep,p2->homun_id,p2->skill[i].id,lv);
				sep = ',';
			}
		}
		if(sep == ',') {
			if(mysql_query(&mysql_handle, tmp_sql) ) {
				printf("DB server Error (insert `homun_skill`)- %s\n", mysql_error(&mysql_handle) );
			}
		}
		// printf("skill ");
	}
	// printf("]\n");
	{
		struct mmo_homunstatus *p3 = numdb_search(homun_db,p2->homun_id);
		if(p3)
			memcpy(p3,p2,sizeof(struct mmo_homunstatus));
	}
	return 1;
}

int  homun_sql_new(struct mmo_homunstatus *p,int account_id,int char_id) {
	// �z��ID��ǂݏo��
	int i;
	char t_name[100], sep, *buf;
	struct mmo_homunstatus *p2;
	// printf("Request make hom  (------)[");
	sprintf(
		tmp_sql,
		"INSERT INTO `%s` (`class`,`name`,`account_id`,`char_id`,`base_level`,`base_exp`,"
		"`max_hp`,`hp`,`max_sp`,`sp`,`str`,`agi`,`vit`,`int`,`dex`,`luk`,"
		"`status_point`,`skill_point`,`equip`,`intimate`,`hungry`,`rename_flag`,`incubate`) "
		"VALUES ('%d', '%s', '%d', '%d',"
		"'%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d', '%d',"
		"'%d', '%d', '%d', '%d', '%d', '%d', '%d')",
		homun_db_, p->class, strecpy(t_name, p->name), p->account_id, p->char_id, p->base_level,
		p->base_exp, p->max_hp, p->hp, p->max_sp, p->sp, p->str, p->agi, p->vit, p->int_, p->dex, p->luk,
		p->status_point, p->skill_point, p->equip, p->intimate,
		p->hungry, p->rename_flag, p->incubate
	);
	if(mysql_query(&mysql_handle, tmp_sql)){
		printf("failed (insert hom), SQL error: %s\n", mysql_error(&mysql_handle));
		aFree(p);
		return 1;
	}

	p->homun_id = (int)mysql_insert_id(&mysql_handle);

	// skill
	buf  = tmp_sql;
	buf += sprintf(tmp_sql, "INSERT INTO `%s` (`homun_id`,`id`,`lv`) VALUES", homun_skill_db);
	sep = ' ';
	for(i=0;i<MAX_HOMSKILL;i++) {
		if(p->skill[i].id && p->skill[i].flag!=1){
			int lv = (p->skill[i].flag==0)?p->skill[i].lv:p->skill[i].flag-2;
			buf += sprintf(buf,"%c('%d','%d','%d')", sep,p->homun_id,p->skill[i].id,lv);
			sep = ',';
		}
	}
	if(sep == ',') {
		if(mysql_query(&mysql_handle, tmp_sql) ) {
			printf("DB server Error (insert `homun_skill`)- %s\n", mysql_error(&mysql_handle) );
		}
	}

	p2 = (struct mmo_homunstatus*)aMalloc( sizeof( struct mmo_homunstatus ) );
	memcpy( p2, p, sizeof( struct mmo_homunstatus ) );
	numdb_insert(homun_db,p->homun_id,p2);

	return 0;
}

static int homun_sql_final_sub(void *key,void *data,va_list ap)
{
	struct mmo_homunstatus *p=data;

	free(p);

	return 0;
}

void homun_sql_final(void)
{
	if(homun_db)
		numdb_final(homun_db,homun_sql_final_sub);
}

void homun_sql_config_read_sub(const char* w1,const char *w2) {
	// nothing to do
	return;
}

#define homun_init   homun_sql_init
#define homun_sync   homun_sql_sync
#define homun_delete homun_sql_delete
#define homun_load   homun_sql_load
#define homun_save   homun_sql_save
#define homun_new    homun_sql_new
#define homun_final  homun_sql_final

#endif

int mapif_hom_info(int fd,int account_id,const struct mmo_homunstatus *h)
{
	if(!h) return 0;
	WFIFOW(fd,0)=0x3888;
	WFIFOW(fd,2)=sizeof(struct mmo_homunstatus) + 9;
	WFIFOL(fd,4)=account_id;
	WFIFOB(fd,8)=(unsigned char)h->incubate;
	memcpy(WFIFOP(fd,9),h,sizeof(struct mmo_homunstatus));
	WFIFOSET(fd,WFIFOW(fd,2));

	return 0;
}

int mapif_save_hom_ack(int fd,int account_id,int flag)
{
	WFIFOW(fd,0)=0x3889;
	WFIFOL(fd,2)=account_id;
	WFIFOB(fd,6)=flag;
	WFIFOSET(fd,7);

	return 0;
}

int mapif_delete_hom_ack(int fd,int flag)
{
	WFIFOW(fd,0)=0x388a;
	WFIFOB(fd,2)=flag;
	WFIFOSET(fd,3);

	return 0;
}

int mapif_create_hom(int fd,int account_id,int char_id,struct mmo_homunstatus *h)
{
	if(!h || account_id <= 0 || char_id <= 0)
		return 0;
	if(homun_new(h,account_id,char_id) == 0) {
		mapif_hom_info(fd,account_id,h);
	}
	return 0;
}

int mapif_load_hom(int fd,int account_id,int char_id,int homun_id)
{
	const struct mmo_homunstatus *h = homun_load(homun_id);
	if(h!=NULL) {
		mapif_hom_info(fd,account_id,h);
	}else
		printf("inter hom: data load error %d %d %d\n",account_id,char_id,homun_id);
	return 0;
}

int mapif_save_hom(int fd,int account_id,struct mmo_homunstatus *data)
{
	if(!data || sizeof(struct mmo_homunstatus) != RFIFOW(fd,2) - 8) {
		printf("inter hom: data size error %d %d\n",sizeof(struct mmo_homunstatus),RFIFOW(fd,2)-8);
	} else if(homun_load(data->homun_id)) {
		if(data->hungry < 0)
			data->hungry = 0;
		else if(data->hungry > 100)
			data->hungry = 100;
		if(data->intimate < 0)
			data->intimate = 0;
		else if(data->intimate > 100000)
			data->intimate = 100000;
		homun_save(data);
		mapif_save_hom_ack(fd,account_id,0);
	}

	return 0;
}

int mapif_delete_hom(int fd,int homun_id)
{
	mapif_delete_hom_ack(fd,homun_delete(homun_id));

	return 0;
}
//---------------------------------------------------------------------------------------
// �z����V�K�쐬
int mapif_parse_CreateHom(int fd)
{
	int size = sizeof(struct mmo_homunstatus);
	int account_id   = RFIFOL(fd,4);
	int char_id      = RFIFOL(fd,8);
	struct mmo_homunstatus h;

	memset(&h,0,size);
	memcpy(&h,RFIFOP(fd,12),size);

	mapif_create_hom(fd,account_id,char_id,&h);
	return 0;
}
// �z���̃f�[�^���M
int mapif_parse_LoadHom(int fd)
{
	mapif_load_hom(fd,RFIFOL(fd,2),RFIFOL(fd,6),RFIFOW(fd,10));
	return 0;
}
// �z���̃f�[�^��M���ۑ�
int mapif_parse_SaveHom(int fd)
{
	mapif_save_hom(fd,RFIFOL(fd,4),(struct mmo_homunstatus *)RFIFOP(fd,8));
	return 0;
}
// �z���폜
int mapif_parse_DeleteHom(int fd)
{
	mapif_delete_hom(fd,RFIFOW(fd,10));
	return 0;
}

// map server ����̒ʐM
// �E�P�p�P�b�g�̂݉�͂��邱��
// �E�p�P�b�g���f�[�^��inter.c�ɃZ�b�g���Ă�������
// �E�p�P�b�g���`�F�b�N��ARFIFOSKIP�͌Ăяo�����ōs����̂ōs���Ă͂Ȃ�Ȃ�
// �E�G���[�Ȃ�0(false)�A�����łȂ��Ȃ�1(true)���������Ȃ���΂Ȃ�Ȃ�
int inter_hom_parse_frommap(int fd)
{
	switch(RFIFOW(fd,0)){
	case 0x3088: mapif_parse_CreateHom(fd); break;
	case 0x3089: mapif_parse_LoadHom(fd); break;
	case 0x308a: mapif_parse_SaveHom(fd); break;
	case 0x308b: mapif_parse_DeleteHom(fd); break;
	default:
		return 0;
	}
	return 1;
}

