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

#ifndef _SCRIPT_H_
#define _SCRIPT_H_

#include "sqldbs.h"

struct script_data {
	int type;
	union {
		int num;
		char *str;
		void *ptr;
	} u;
	struct linkdb_node** ref; // ���t�@�����X
};

struct script_code {
	struct linkdb_node* script_vars;
	int script_size;
	unsigned char script_buf[1];
};

struct script_stack {
	int sp,sp_max,defsp;
	struct script_data *stack_data;
	struct linkdb_node **var_function;	// �֐��ˑ��ϐ�
};

struct script_sleep {
	int tick;
	int timer;
};

struct script_state {
	struct script_stack *stack;
	int start,end;
	int pos,state;
	int rid,oid;
	struct script_code *script, *scriptroot;
	struct script_sleep sleep;
};

char* parse_script_line_curly(unsigned char *p,int *curly_count,int line);
char* parse_script_line_end(unsigned char *src, const char *file, int line);
struct script_code* parse_script(unsigned char *,const char*,int);
void run_script(struct script_code*,int,int,int);
void script_error(const char *src, const char *file, int start_line, const char *msg, const char *pos);

void script_free_stack(struct script_stack *stack);
void script_free_code(struct script_code* code);

struct dbt* script_get_userfunc_db(void);

int script_config_read(const char *cfgName);
int do_init_script(void);
int do_final_script(void);

extern struct script_code error_code;
extern struct linkdb_node *scriptlabel_db;

// @readvars, @writevars
int script_check_variable(const char *name,int array_flag,int read_only);
void* script_read_vars(struct map_session_data *sd,const char *var,int elem,struct linkdb_node **ref);
void script_write_vars(struct map_session_data *sd,const char *var,int elem,void *v,struct linkdb_node **ref);

#ifdef TXT_ONLY
extern char mapreg_txt[256];
#else
extern MYSQL mysql_handle_script;
#endif /* TXT_ONLY */

#endif
