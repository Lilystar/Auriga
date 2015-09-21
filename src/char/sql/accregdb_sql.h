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

#ifndef _ACCREGDB_SQL_H_
#define _ACCREGDB_SQL_H_

#include "../inter.h"

// �v���g�^�C�v�錾
const struct accreg* accregdb_sql_load(int account_id);
void accregdb_sql_save(struct accreg *reg);
bool accregdb_sql_init(void);
int accregdb_sql_sync(void);
void accregdb_sql_final(void);
int accregdb_sql_config_read_sub(const char *w1,const char *w2);

// �G�C���A�X
#define accregdb_load  accregdb_sql_load
#define accregdb_save  accregdb_sql_save
#define accregdb_init  accregdb_sql_init
#define accregdb_sync  accregdb_sql_sync
#define accregdb_final accregdb_sql_final
#define accregdb_config_read_sub accregdb_sql_config_read_sub

#endif /* _ACCREGDB_SQL_H_ */
