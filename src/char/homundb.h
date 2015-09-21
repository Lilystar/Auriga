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

#ifndef _HOMUNDB_H_
#define _HOMUMDB_H_

#include "utils.h"

#ifdef TXT_ONLY
	// �v���g�^�C�v�錾
	bool homundb_txt_init(void);
	int homundb_txt_sync(void);
	bool homundb_txt_delete(int homun_id);
	const struct mmo_homunstatus* homundb_txt_load(int homun_id);
	bool homundb_txt_save(struct mmo_homunstatus* p2);
	bool homundb_txt_new(struct mmo_homunstatus *p);
	void homundb_txt_final(void);
	void homundb_txt_config_read_sub(const char* w1,const char *w2);
	// �G�C���A�X
	#define homundb_init   homundb_txt_init
	#define homundb_sync   homundb_txt_sync
	#define homundb_delete homundb_txt_delete
	#define homundb_load   homundb_txt_load
	#define homundb_save   homundb_txt_save
	#define homundb_new    homundb_txt_new
	#define homundb_final  homundb_txt_final
	#define homundb_config_read_sub homundb_txt_config_read_sub
#else
	// �v���g�^�C�v�錾
	bool homundb_sql_init(void);
	int homundb_sql_sync(void);
	bool homundb_sql_delete(int homun_id);
	const struct mmo_homunstatus* homundb_sql_load(int homun_id);
	bool homundb_sql_save(struct mmo_homunstatus* p2);
	bool homundb_sql_new(struct mmo_homunstatus *p2);
	void homundb_sql_final(void);
	void homundb_sql_config_read_sub(const char* w1,const char *w2);
	// �G�C���A�X
	#define homundb_init   homundb_sql_init
	#define homundb_sync   homundb_sql_sync
	#define homundb_delete homundb_sql_delete
	#define homundb_load   homundb_sql_load
	#define homundb_save   homundb_sql_save
	#define homundb_new    homundb_sql_new
	#define homundb_final  homundb_sql_final
	#define homundb_config_read_sub homundb_sql_config_read_sub
#endif

int inter_hom_parse_frommap(int fd);

#endif
