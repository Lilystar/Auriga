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

#ifndef _PARTYDB_H_
#define _PARTYDB_H_

#include "utils.h"

#ifdef TXT_ONLY
	// プロトタイプ宣言
	bool partydb_txt_init(void);
	int partydb_txt_sync(void);
	const struct party* partydb_txt_load_str(char *str);
	const struct party* partydb_txt_load_num(int party_id);
	bool partydb_txt_save(struct party* p2);
	bool partydb_txt_delete(int party_id);
	bool partydb_txt_new(struct party *p);
	int partydb_txt_config_read_sub(const char *w1,const char *w2);
	void partydb_txt_final(void);

	// エイリアス
	#define partydb_init     partydb_txt_init
	#define partydb_sync     partydb_txt_sync
	#define partydb_save     partydb_txt_save
	#define partydb_final    partydb_txt_final
	#define partydb_delete   partydb_txt_delete
	#define partydb_load_str partydb_txt_load_str
	#define partydb_load_num partydb_txt_load_num
	#define partydb_new      partydb_txt_new
	#define partydb_config_read_sub partydb_txt_config_read_sub
#else
	// プロトタイプ宣言
	bool partydb_sql_init(void);
	int partydb_sql_sync(void);
	const struct party* partydb_sql_load_str(char *str);
	const struct party* partydb_sql_load_num(int party_id);
	bool partydb_sql_save(struct party* p2);
	bool partydb_sql_delete(int party_id);
	bool partydb_sql_new(struct party *p);
	int partydb_sql_config_read_sub(const char *w1,const char *w2);
	void partydb_sql_final(void);

	// エイリアス
	#define partydb_init     partydb_sql_init
	#define partydb_sync     partydb_sql_sync
	#define partydb_save     partydb_sql_save
	#define partydb_final    partydb_sql_final
	#define partydb_delete   partydb_sql_delete
	#define partydb_load_str partydb_sql_load_str
	#define partydb_load_num partydb_sql_load_num
	#define partydb_new      partydb_sql_new
	#define partydb_config_read_sub partydb_sql_config_read_sub
#endif

#endif
