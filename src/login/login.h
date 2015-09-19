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

#ifndef _LOGIN_H_
#define _LOGIN_H_

#include "mmo.h"

#define MAX_CHAR_SERVERS 5

#define PASSWORDENC		3	// �Í����p�X���[�h�ɑΉ�������Ƃ���`����
							// passwordencrypt�̂Ƃ���1�A
							// passwordencrypt2�̂Ƃ���2�ɂ���B
							// 3�ɂ���Ɨ����ɑΉ�

struct login_session_data {
	int account_id;
	int login_id1;
	int login_id2;
	int char_id;
	int sex;
	int passwdenc;
	int md5keylen;
	char md5key[64];
	char userid[24],pass[24],lastlogin[24];
	char lastip[16];
};

struct mmo_char_server {
	char name[20];
	unsigned long ip;
	unsigned short port;
	int users;
	int maintenance;
	int new_;
};

struct mmo_account {
	int account_id,sex;
	char userid[24],pass[24],lastlogin[24],mail[40];
	int logincount;
	int state;
	int account_reg2_num;
	struct global_reg account_reg2[ACCOUNT_REG2_NUM];
	char lastip[16];
};

extern char login_conf_filename[];

#endif /* _LOGIN_H_ */
