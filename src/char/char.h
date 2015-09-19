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

#ifndef _CHAR_H_
#define _CHAR_H_

#include "mmo.h"

#define MAX_MAP_SERVERS 8

#define DEFAULT_AUTOSAVE_INTERVAL_CS 300*1000

struct mmo_map_server {
	unsigned long ip;
	unsigned short port;
	int users;
	char *map;
	short map_num;
	short active;
};

struct mmo_chardata {
	struct mmo_charstatus st;
	struct registry reg;
};

#define MAX_CHAR_SLOT 12

struct char_session_data {
	int state;
	int account_id,login_id1,login_id2,sex;
	const struct mmo_chardata *found_char[MAX_CHAR_SLOT];
};

struct cram_session_data {
	int md5keylen;
	char md5key[128];
};

int mapif_sendall(unsigned char *buf,unsigned int len);
int mapif_sendallwos(int fd,unsigned char *buf,unsigned int len);
int mapif_send(int fd,unsigned char *buf,unsigned int len);
void mapif_parse_CharConnectLimit(int fd);

extern int autosave_interval;

struct char_online {
	int account_id;
	int char_id;
	unsigned long ip;
	unsigned short port;
	char name[24];
};

extern char char_conf_filename[];
extern char inter_conf_filename[];


#ifdef TXT_ONLY

int char_txt_save(struct mmo_charstatus *st);
const struct mmo_chardata* char_txt_load(int char_id);
const struct mmo_chardata* char_txt_nick2chardata(const char *char_name);

#ifndef _CHAR_C_
#define char_save          char_txt_save
#define char_load          char_txt_load
#define char_nick2chardata char_txt_nick2chardata
#endif /* _CHAR_C_ */

#else

int char_sql_save(struct mmo_charstatus *st);
const struct mmo_chardata* char_sql_load(int char_id);
const struct mmo_chardata* char_sql_nick2chardata(const char *char_name);

#ifndef _CHAR_C_
#define char_save          char_sql_save
#define char_load          char_sql_load
#define char_nick2chardata char_sql_nick2chardata
#endif /* _CHAR_C_ */

// for sql
enum {
	TABLE_NUM_INVENTORY,
	TABLE_NUM_CART,
	TABLE_NUM_STORAGE,
	TABLE_NUM_GUILD_STORAGE,
};
int char_sql_saveitem(struct item *item, int max, int id, int tableswitch);
int char_sql_loaditem(struct item *item, int max, int id, int tableswitch);

#endif /* TXT_ONLY */

#endif /* _CHAR_H_ */
