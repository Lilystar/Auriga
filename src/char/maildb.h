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

#ifndef _MAILDB_H_
#define _MAILDB_H_

#include "utils.h"

#ifdef TXT_ONLY
	// �v���g�^�C�v�錾
	bool maildb_txt_store_mail(int char_id,struct mail_data *md);
	bool maildb_txt_save_mail(int char_id,int i,int store,struct mail_data md[MAIL_STORE_MAX]);
	bool maildb_txt_read_mail(int char_id,const struct mail *m,struct mail_data md[MAIL_STORE_MAX]);
	bool maildb_txt_deletemail(int char_id,unsigned int mail_num,const struct mail *m);
	bool maildb_txt_init(void);
	int maildb_txt_sync(void);
	bool maildb_txt_delete(int char_id);
	const struct mail* maildb_txt_load(int char_id);
	bool maildb_txt_save(struct mail* m2);
	bool maildb_txt_new(int account_id,int char_id);
	void maildb_txt_final(void);
	void maildb_txt_config_read_sub(const char *w1,const char *w2);
	// �G�C���A�X
	#define maildb_store_mail      maildb_txt_store_mail
	#define maildb_save_mail       maildb_txt_save_mail
	#define maildb_read_mail       maildb_txt_read_mail
	#define maildb_deletemail      maildb_txt_deletemail
	#define maildb_init            maildb_txt_init
	#define maildb_sync            maildb_txt_sync
	#define maildb_delete          maildb_txt_delete
	#define maildb_load            maildb_txt_load
	#define maildb_save            maildb_txt_save
	#define maildb_new             maildb_txt_new
	#define maildb_final           maildb_txt_final
	#define maildb_config_read_sub maildb_txt_config_read_sub
#else
	// �v���g�^�C�v�錾
	bool maildb_sql_store_mail(int char_id,struct mail_data *md);
	bool maildb_sql_save_mail(int char_id,int i,int store,struct mail_data md[MAIL_STORE_MAX]);
	bool maildb_sql_read_mail(int char_id,const struct mail *m,struct mail_data md[MAIL_STORE_MAX]);
	bool maildb_sql_deletemail(int char_id,unsigned int mail_num,const struct mail *m);
	bool maildb_sql_init(void);
	int maildb_sql_sync(void);
	bool maildb_sql_delete(int char_id);
	const struct mail* maildb_sql_load(int char_id);
	bool maildb_sql_save(struct mail* m2);
	bool maildb_sql_new(int account_id,int char_id);
	void maildb_sql_final(void);
	void maildb_sql_config_read_sub(const char *w1,const char *w2);
	// �G�C���A�X
	#define maildb_store_mail      maildb_sql_store_mail
	#define maildb_save_mail       maildb_sql_save_mail
	#define maildb_read_mail       maildb_sql_read_mail
	#define maildb_deletemail      maildb_sql_deletemail
	#define maildb_init            maildb_sql_init
	#define maildb_sync            maildb_sql_sync
	#define maildb_delete          maildb_sql_delete
	#define maildb_load            maildb_sql_load
	#define maildb_save            maildb_sql_save
	#define maildb_new             maildb_sql_new
	#define maildb_final           maildb_sql_final
	#define maildb_config_read_sub maildb_sql_config_read_sub
#endif

#endif /* _MAILDB_H_ */
