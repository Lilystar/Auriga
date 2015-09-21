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

#ifndef NO_SCDATA_SAVING

#include <stdio.h>
#include <stdlib.h>

#include "mmo.h"
#include "db.h"
#include "malloc.h"
#include "sqldbs.h"
#include "utils.h"
#include "nullpo.h"

#include "statusdb_sql.h"

static struct dbt *scdata_db = NULL;

/*==========================================
 * �ݒ�t�@�C���̓ǂݍ���
 *------------------------------------------
 */
int statusdb_sql_config_read_sub(const char *w1, const char *w2)
{
	return 0;
}

/*==========================================
 * ����
 *------------------------------------------
 */
int statusdb_sql_sync(void)
{
	// nothing to do
	return 0;
}

/*==========================================
 * �X�e�[�^�X�f�[�^�폜
 *------------------------------------------
 */
bool statusdb_sql_delete(int char_id)
{
	struct scdata *sc;

	if( sqldbs_query(&mysql_handle, "DELETE FROM `" SCDATA_TABLE "` WHERE `char_id`='%d'", char_id) == false )
		return false;

	sc = (struct scdata *)numdb_search(scdata_db, char_id);

	if(sc && sc->char_id == char_id) {
		numdb_erase(scdata_db, char_id);
		aFree(sc);
	}
	return true;
}

/*==========================================
 * �L����ID����X�e�[�^�X�f�[�^�����[�h
 * ���׌y����D�悵��const��t���Ȃ�
 *------------------------------------------
 */
struct scdata *statusdb_sql_load(int char_id)
{
	bool result;
	struct scdata *sc = (struct scdata *)numdb_search(scdata_db, char_id);

	if(sc && sc->char_id == char_id) {
		// ���ɃL���b�V�������݂���
		return sc;
	}
	if(sc == NULL) {
		sc = (struct scdata *)aMalloc(sizeof(struct scdata));
		numdb_insert(scdata_db, char_id, sc);
	}
	memset(sc, 0, sizeof(struct scdata));

	sc->char_id = char_id;

	result = sqldbs_query(&mysql_handle,
		"SELECT `account_id`, `type`, `val1`, `val2`, `val3`, `val4`, `tick` "
		"FROM `" SCDATA_TABLE "` WHERE `char_id`='%d'", char_id
	);
	if(result == false) {
		sc->char_id = -1;
		return NULL;
	}

	if(sqldbs_num_rows(&mysql_handle) > 0) {
		int i;
		char **sql_row;

		for(i = 0; (sql_row = sqldbs_fetch(&mysql_handle)) && i < MAX_STATUSCHANGE; i++) {
			if(sc->account_id == 0) {
				sc->account_id = atoi(sql_row[0]);
			}
			sc->data[i].type = (short)atoi(sql_row[1]);
			sc->data[i].val1 = atoi(sql_row[2]);
			sc->data[i].val2 = atoi(sql_row[3]);
			sc->data[i].val3 = atoi(sql_row[4]);
			sc->data[i].val4 = atoi(sql_row[5]);
			sc->data[i].tick = atoi(sql_row[6]);
		}
		sc->count = (i < MAX_STATUSCHANGE)? i: MAX_STATUSCHANGE;
	} else {
		// ������Ȃ��Ă�����
		sc = NULL;
	}
	sqldbs_free_result(&mysql_handle);

	return sc;
}

/*==========================================
 * �Z�[�u
 *------------------------------------------
 */
bool statusdb_sql_save(struct scdata *sc2)
{
	struct scdata *sc1;
	bool result = false;

	nullpo_retr(false, sc2);

	sc1 = statusdb_sql_load(sc2->char_id);

	if(sc1 && sc1->count <= 0 && sc2->count <= 0) {
		// �f�[�^������0�Ȃ̂ŉ������Ȃ�
		return true;
	}

	if( sqldbs_transaction_start(&mysql_handle) == false )
		return false;

	// try
	do {
		int i;

		if(sc1 == NULL || sc1->count > 0) {
			// �f�[�^�T�[�o���Ƀf�[�^������Ƃ������폜�N�G���𔭍s
			if( sqldbs_query(&mysql_handle, "DELETE FROM `" SCDATA_TABLE "` WHERE `char_id`='%d'", sc2->char_id) == false )
				break;
		}

		for(i = 0; i < sc2->count; i++) {
			if( sqldbs_query(&mysql_handle,
				"INSERT INTO `" SCDATA_TABLE "` (`char_id`, `account_id`, `type`, `val1`, `val2`, `val3`, `val4`, `tick`) "
				"VALUES ('%d','%d','%d','%d','%d','%d','%d','%d')",
				sc2->char_id, sc2->account_id, sc2->data[i].type,
				sc2->data[i].val1, sc2->data[i].val2, sc2->data[i].val3, sc2->data[i].val4, sc2->data[i].tick
			) == false )
				break;
		}
		if(i != sc2->count)
			break;

		// success
		result = true;

		if(sc1) {
			// cache copy
			memcpy(sc1, sc2, sizeof(struct scdata));
		}
	} while(0);

	sqldbs_transaction_end(&mysql_handle, result);

	return result;
}

/*==========================================
 * �I��
 *------------------------------------------
 */
static int statusdb_sql_final_sub(void *key, void *data, va_list ap)
{
	struct scdata *sc = (struct scdata *)data;

	aFree(sc);

	return 0;
}

void statusdb_sql_final(void)
{
	if(scdata_db)
		numdb_final(scdata_db, statusdb_sql_final_sub);
}

/*==========================================
 * ������
 *------------------------------------------
 */
bool statusdb_sql_init(void)
{
	scdata_db = numdb_init();
	return true;
}

#endif
