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

#ifndef _GUILDDB_H_
#define _GUILDDB_H_

#include "db.h"
#include "mmo.h"
#include "utils.h"

#include "int_guild.h"

#ifdef TXT_ONLY
	// �v���g�^�C�v�錾
	bool guilddb_txt_new(struct guild *g2);
	bool guilddb_txt_init(void);
	bool guilddb_txt_save(struct guild* g2);
	int guilddb_txt_sync(void);
	void guilddb_txt_final(void);
	const struct guild* guilddb_txt_load_num(int guild_id);
	const struct guild* guilddb_txt_load_str(char *str);
	void guilddb_txt_delete(int guild_id);
	void guilddb_txt_config_read_sub(const char* w1,const char *w2);
	// �G�C���A�X
	#define guilddb_new      guilddb_txt_new
	#define guilddb_init     guilddb_txt_init
	#define guilddb_save     guilddb_txt_save
	#define guilddb_sync     guilddb_txt_sync
	#define guilddb_final    guilddb_txt_final
	#define guilddb_load_num guilddb_txt_load_num
	#define guilddb_load_str guilddb_txt_load_str
	#define guilddb_delete   guilddb_txt_delete
	#define guilddb_config_read_sub guilddb_txt_config_read_sub
#else
	// �v���g�^�C�v�錾
	bool guilddb_sql_new(struct guild *g);
	bool guilddb_sql_init(void);
	bool guilddb_sql_save(struct guild* g2);
	int guilddb_sql_sync(void);
	void guilddb_sql_final(void);
	const struct guild *guilddb_sql_load_num(int guild_id);
	const struct guild* guilddb_sql_load_str(char *str);
	void guilddb_sql_delete(int guild_id);
	void guilddb_sql_config_read_sub(const char* w1,const char *w2);
	// �G�C���A�X
	#define guilddb_new      guilddb_sql_new
	#define guilddb_init     guilddb_sql_init
	#define guilddb_save     guilddb_sql_save
	#define guilddb_sync     guilddb_sql_sync
	#define guilddb_final    guilddb_sql_final
	#define guilddb_load_num guilddb_sql_load_num
	#define guilddb_load_str guilddb_sql_load_str
	#define guilddb_delete   guilddb_sql_delete
	#define guilddb_config_read_sub guilddb_sql_config_read_sub
#endif

#ifdef TXT_JOURNAL
extern int guildcastle_journal_enable;
extern struct journal guildcastle_journal;
#endif

extern int guild_exp[MAX_GUILDLEVEL];
extern struct dbt *guild_db;
extern struct guild_castle castle_db[MAX_GUILDCASTLE];

#endif
