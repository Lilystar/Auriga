// $Id: login-converter.c,v 1.1 2005/03/21 12:00:14 aboon Exp $
// original : login2.c 2003/01/28 02:29:17 Rev.1.1.1.1
// login data file to mysql conversion utility.
//
#include <stdio.h>
#include <stdlib.h>

#include "../common/mmo.h"
#include "../common/db.h"
#include "../common/malloc.h"
#include "../common/utils.h"
#include "converter.h"

static struct dbt *gm_account_db = NULL;

struct accreg {
	int account_id;
	int reg_num;
	struct global_reg reg[ACCOUNT_REG2_NUM];
};

static int isGM(int account_id)
{
	struct gm_account *p;

	if (gm_account_db == NULL)
		return 0;
	p = (struct gm_account *)numdb_search(gm_account_db, account_id);
	if (p == NULL)
		return 0;

	return p->level;
}

static int gm_account_db_final(void *key, void *data, va_list ap)
{
	struct gm_account *p = (struct gm_account *)data;

	aFree(p);

	return 0;
}

void read_gm_account(void)
{
	char line[8192];
	struct gm_account *p;
	FILE *fp;
	int c, l;
	int account_id, level;
	int i;
	int range, start_range, end_range;

	gm_account_db = numdb_init();

	printf("Starting reading gm_account\n");

	if ((fp = fopen(GM_account_filename, "r")) == NULL) {
		printf("File not found: %s.\n", GM_account_filename);
		return;
	}

	line[sizeof(line)-1] = '\0';
	c = 0;
	l = 0;
	while(fgets(line, sizeof(line)-1, fp)) {
		l++;
		if ((line[0] == '/' && line[1] == '/') || line[0] == '\0' || line[0] == '\n' || line[0] == '\r')
			continue;

		if ((range = sscanf(line, "%d%*[-~]%d %d", &start_range, &end_range, &level)) == 3 ||
		    (range = sscanf(line, "%d%*[-~]%d:%d", &start_range, &end_range, &level)) == 3 ||
		    (range = sscanf(line, "%d %d", &start_range, &level)) == 2 ||
		    (range = sscanf(line, "%d:%d", &start_range, &level)) == 2 ||
		    (range = sscanf(line, "%d: %d", &start_range, &level)) == 2) {
			if (level <= 0) {
				printf("gm_account [%s]: invalid GM level [%ds] line %d\n", GM_account_filename, level, l);
			} else {
				if (level > 99)
					level = 99;
				if (range == 2) {
					end_range = start_range;
				} else if (end_range < start_range) {
					i = end_range;
					end_range = start_range;
					start_range = i;
				}
				for (account_id = start_range; account_id <= end_range; account_id++) {
					if ((p = (struct gm_account *)numdb_search(gm_account_db, account_id)) == NULL) {
						p = (struct gm_account*)aCalloc(1, sizeof(struct gm_account));
						numdb_insert(gm_account_db, account_id, p);
					}
					p->account_id = account_id;
					p->level = level;
					c++;
				}
			}
		} else {
			printf("gm_account: broken data [%s] line %d\n", GM_account_filename, l);
		}
	}
	fclose(fp);
	printf("gm_account: %s read done (%d gm account ID)\n", GM_account_filename, c);

	return;
}

int login_convert(void)
{
	FILE *fp;
	int account_id, logincount, user_level, state, n, n2, i, j, val;
	char line[65536], userid[256], pass[256], lastlogin[256], buf[4][256], sex;
	char input, *p;
	struct accreg dat;

	printf("\nDo you wish to convert your Login Database to SQL? (y/n) : ");
	input = getchar();
	if(input == 'y' || input == 'Y') {
		read_gm_account();
		fp = fopen(account_filename,"r");
		if(fp == NULL) {
			printf("cant't read : %s\n",account_filename);
			return 1;
		}
		while(fgets(line, sizeof(line)-1, fp)) {
			char email[256] = "";

			if(line[0]=='/' && line[1]=='/')
				continue;
			p = line;
			n = -1;
			logincount = 0;
			state = 0;
			i = sscanf(p, "%d\t%255[^\t]\t%255[^\t]\t%255[^\t]\t%c\t%d\t%d\t%n",
				&account_id, userid, pass, lastlogin, &sex, &logincount, &state, &n);

			if(i < 5)		// %newid% �̍s��convert���Ȃ�
				continue;
			if(account_id < 0)	// �A�J�E���gID�����������ꍇ�͔O�̂��ߒe���Ă���
				continue;
			user_level = isGM(account_id);

			// force \0 terminal
			userid[23]    = '\0';
			pass[23]      = '\0';
			lastlogin[23] = '\0';

			// ���[���A�h���X���Ȃ��ꍇ�� "@" �ɒu��
			if( n <= 0 || sscanf(p+n, "%255[^\t]\t%n", email, &n2) != 1 || !strchr(email,'@') ) {
				memset(&email,0,sizeof(email));
				email[0] = '@';
			}
			n = (n2 > 0)? n+n2: 0;

			// �S���[���h���L�A�J�E���g�ϐ� ( ## �ϐ� ) �ǂݍ���
			if(n > 0) {
				char str[256];
				memset(&dat, 0, sizeof(dat));
				for(j=0; j<ACCOUNT_REG2_NUM; j++) {
					p += n;
					if(sscanf(p, "%255[^\t,],%d%n", str, &val, &n)!=2)
						break;
					strncpy(dat.reg[j].str, str, 32);
					dat.reg[j].str[31] = '\0';	// force \0 terminal
					dat.reg[j].value   = val;
					if(p[n] != ' ')
						break;
					n++;
				}
				dat.reg_num = j;
			} else {
				dat.reg_num = 0;
			}

			// �T�[�o�̏ꍇID��0����AUTO_INCREMENT�����̂ŕ␳����
			if(sex == 'S')
				account_id++;

			// �x���}���̂��ߕϊ����Ă���
			if(strcmp(lastlogin,"-") == 0)
				strcpy(lastlogin,"0000-00-00 00:00:00");

			sprintf(tmp_sql, "DELETE FROM `%s` WHERE `%s` = '%d'", login_db, login_db_account_id, account_id);
			if(mysql_query(&mysql_handle, tmp_sql) ) {
				printf("DB server Error (delete `%s`)- %s\n", login_db, mysql_error(&mysql_handle) );
			}
			sprintf(tmp_sql,
				"INSERT INTO `%s` (`%s`, `%s`, `%s`, `lastlogin`, `sex`, `logincount`, `email`, `%s`)"
				" VALUES ('%d', '%s', '%s', '%s', '%c', '%d', '%s', '%d');",
				login_db, login_db_account_id, login_db_userid, login_db_user_pass, login_db_level,
				account_id , strecpy(buf[0],userid), strecpy(buf[1],pass), strecpy(buf[2],lastlogin), sex, logincount, strecpy(buf[3],email), user_level
			);
			if(mysql_query(&mysql_handle, tmp_sql) ) {
				printf("DB server Error (insert `%s`)- %s\n", login_db, mysql_error(&mysql_handle) );
			}

			sprintf(tmp_sql,"DELETE FROM `global_reg_value` WHERE `type`='1' AND `account_id`='%d'",account_id);
			if(mysql_query(&mysql_handle, tmp_sql)) {
				printf("DB server Error (delete `global_reg_value`)- %s\n", mysql_error(&mysql_handle));
			}
			for(i = 0; i < dat.reg_num; i++) {
				sprintf(tmp_sql,
					"INSERT INTO `global_reg_value` (`type`, `account_id`, `str`, `value`) "
					"VALUES ( 1 , '%d' , '%s' , '%d')",
					account_id, strecpy(buf[0],dat.reg[i].str), dat.reg[i].value
				);
				if(mysql_query(&mysql_handle, tmp_sql)) {
					printf("DB server Error (insert `globa_reg_value`)- %s\n", mysql_error(&mysql_handle));
				}
			}
		}
		numdb_final(gm_account_db,gm_account_db_final);
		fclose(fp);
	}
	while(getchar() != '\n');

	return 0;
}
