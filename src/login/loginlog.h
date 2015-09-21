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

#ifndef _LOGINLOG_H_
#define _LOGINLOG_H_

#ifdef TXT_ONLY
	// �v���g�^�C�v�錾
	int loginlog_log_txt(const char *fmt, ...);
	void loginlog_config_read_txt(const char *str, const char *str2);
	// �G�C���A�X
	#define loginlog_log loginlog_log_txt
	#define loginlog_config_read loginlog_config_read_txt
#else
	// �v���g�^�C�v�錾
	int loginlog_log_sql(const char *fmt, ...);
	void loginlog_config_read_sql(const char *str, const char *str2);
	// �G�C���A�X
	#define loginlog_log loginlog_log_sql
	#define loginlog_config_read loginlog_config_read_sql
#endif

#endif /* _LOGINLOG_H_ */
