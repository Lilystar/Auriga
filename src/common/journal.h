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

#ifndef _JOURNAL_H_
#define _JOURNAL_H_

#include <stdio.h>
#include <time.h>

#include "db.h"

struct journal_fileheader
{
	char identifier[16];
	unsigned int datasize;
};

enum JOURNAL_FLAG
{
	JOURNAL_FLAG_WRITE=0,
	JOURNAL_FLAG_DELETE=1,
};

struct journal_header {
	unsigned long crc32;
	unsigned int tick;
	time_t timestamp;
	int key, flag;
};

struct journal_data
{
	int idx, flag;
	unsigned int tick;
	time_t timestamp;
	void *buf;
};

struct journal
{
	FILE* fp;
	struct journal_fileheader fhd;
	int mode;
	int cache_timer;
	size_t datasize;
	size_t chunksize;
	size_t nextchunk;
	int unusedchunk_read;
	int unusedchunk_write;
	int* unusedchunk_queue;
	int unusedchunk_size;
	char filename[1024];
	struct dbt *db;
};


// �W���[�i�������
//  journal_create, journal_load ������K���ĂԕK�v������
void journal_final( struct journal* j );

// �W���[�i����V�K�쐬����(�������݃��[�h�ŊJ��)
//   ���̃f�[�^�ɑ΂��鑀��� journal_write, journal_flush, journal_final �̂݋������
void journal_create( struct journal* j, size_t datasize, int cache_interval, const char* filename );

// �����̃W���[�i�������[���t�H���[�h��p�̓ǂݍ��݃��[�h�ŊJ��
//   ���̃f�[�^�ɑ΂��鑀��� journal_rollforward, journal_final �̂݋������
//   ���[���t�H���[�h���I������� final ���ĊJ������K�v������B
int journal_load_with_convert( struct journal* j, size_t datasize, const char* filename, void(*func)( struct journal_header *jhd, void *buf ) );
#define journal_load(j, datasize, filename) journal_load_with_convert(j, datasize, filename, NULL)


// �f�[�^�̏������� ( journal_create ��̂� )
//   key: ���ʗp�̃L�[�i���j�[�N�ł��邱�Ɓj
//   data: �ۑ�������f�[�^, NULL �Ńf�[�^�̍폜
int journal_write( struct journal* j, int key, const void* data );

// �L���b�V���̃t���b�V�� ( journal_create ��̂� )
int journal_flush( struct journal* j );


// ���[���t�H���[�h�p�̊֐� ( journal_load ��̂� )
//   �W���[�i���̑S�f�[�^�ɑ΂��� func �Őݒ肵���R�[���o�b�N�֐����Ăяo��
//   ���[���t�H���[�h�p�̃R�[���o�b�N�֐��̃p�����[�^�͈ȉ��̂Ƃ���
//   key: journal_write �Őݒ肵�� key (���j�[�N)
//   buf: journal_write �Őݒ肵�� data
//   flag: journal_write �� data �� NULL ���w�肳��Ă���� JOURNAL_FLAG_DELETE,
//         ����ȊO�ł� JOURNAL_FLAG_WRITE ���ݒ肳��Ă���B
int journal_rollforward( struct journal* j, int(*func)( int key, void* buf, int flag ) );


//const char* journal_get( struct journal* j, int key, int* flag );
//int journal_delete( struct journal* j, int key );

#endif
