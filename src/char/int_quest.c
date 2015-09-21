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

#define _INT_QUEST_C_

#include <stdio.h>

#include "mmo.h"
#include "socket.h"
#include "utils.h"

#include "questdb.h"
#include "int_quest.h"

// ----------------------------------------------------------
// �N�G�X�g�f�[�^���M
// ----------------------------------------------------------
int mapif_load_quest(int fd,int account_id,int char_id)
{
	const struct quest *q = questdb_load(char_id);
	int i,p = 8;

	WFIFOW(fd,0)=0x3860;
	WFIFOL(fd,4)=account_id;

	if(q) {
		for(i=0; i<q->count; i++,p+=27) {
			WFIFOL(fd,p)    = q->data[i].nameid;
			WFIFOB(fd,p+4)  = q->data[i].state;
			WFIFOL(fd,p+5)  = (unsigned int)q->data[i].limit;
			WFIFOW(fd,p+9)  = q->data[i].mob[0].id;
			WFIFOW(fd,p+11) = q->data[i].mob[0].max;
			WFIFOW(fd,p+13) = q->data[i].mob[0].cnt;
			WFIFOW(fd,p+15) = q->data[i].mob[1].id;
			WFIFOW(fd,p+17) = q->data[i].mob[1].max;
			WFIFOW(fd,p+19) = q->data[i].mob[1].cnt;
			WFIFOW(fd,p+21) = q->data[i].mob[2].id;
			WFIFOW(fd,p+23) = q->data[i].mob[2].max;
			WFIFOW(fd,p+25) = q->data[i].mob[2].cnt;
		}
	}

	WFIFOW(fd,2)=p;
	WFIFOSET(fd,p);

	return 0;
}

// ----------------------------------------------------------
// �N�G�X�g�f�[�^�ۑ��������M
// ----------------------------------------------------------
static int mapif_save_quest_ack(int fd,int char_id,int flag)
{
	WFIFOW(fd,0)=0x3861;
	WFIFOL(fd,2)=char_id;
	WFIFOB(fd,6)=flag;
	WFIFOSET(fd,7);

	return 0;
}


// ----------------------------------------------------------
// �N�G�X�g�f�[�^�v����M
// ----------------------------------------------------------
int mapif_parse_LoadQuestList(int fd)
{
	mapif_load_quest(fd,RFIFOL(fd,2),RFIFOL(fd,6));
	return 0;
}

// ----------------------------------------------------------
// �N�G�X�g�f�[�^�ۑ��v����M
// ----------------------------------------------------------
int mapif_parse_SaveQuestList(int fd)
{
	struct quest q;
	int i,p;
	short len = RFIFOW(fd,2);

	q.account_id = RFIFOL(fd,4);
	q.char_id    = RFIFOL(fd,8);

	for(i=0,p=12; p<len && i<MAX_QUESTLIST; i++,p+=27) {
		q.data[i].nameid     = RFIFOL(fd,p);
		q.data[i].state      = RFIFOB(fd,p+4);
		q.data[i].limit      = (unsigned int)RFIFOL(fd,p+5);
		q.data[i].mob[0].id  = RFIFOW(fd,p+9);
		q.data[i].mob[0].max = RFIFOW(fd,p+11);
		q.data[i].mob[0].cnt = RFIFOW(fd,p+13);
		q.data[i].mob[1].id  = RFIFOW(fd,p+15);
		q.data[i].mob[1].max = RFIFOW(fd,p+17);
		q.data[i].mob[1].cnt = RFIFOW(fd,p+19);
		q.data[i].mob[2].id  = RFIFOW(fd,p+21);
		q.data[i].mob[2].max = RFIFOW(fd,p+23);
		q.data[i].mob[2].cnt = RFIFOW(fd,p+25);
	}
	q.count = i;

	questdb_save(&q);
	mapif_save_quest_ack(fd,q.char_id,0);

	return 0;
}

// ----------------------------------------------------------
// �}�b�v�T�[�o�[����̒ʐM
// �E�P�p�P�b�g�̂݉�͂��邱��
// �E�p�P�b�g���f�[�^��inter.c�ɃZ�b�g���Ă�������
// �E�p�P�b�g���`�F�b�N��ARFIFOSKIP�͌Ăяo�����ōs����̂ōs���Ă͂Ȃ�Ȃ�
// �E�G���[�Ȃ�0(false)�A�����łȂ��Ȃ�1(true)���������Ȃ���΂Ȃ�Ȃ�
// ----------------------------------------------------------
int inter_quest_parse_frommap(int fd)
{
	switch(RFIFOW(fd,0)){
	case 0x3060: mapif_parse_LoadQuestList(fd); break;
	case 0x3061: mapif_parse_SaveQuestList(fd); break;
	default:
		return 0;
	}
	return 1;
}
