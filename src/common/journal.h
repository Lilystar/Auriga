#ifndef _JOURNAL_H_
#define _JOURNAL_H_

#include "db.h"
#include <stdio.h>
#include <time.h>

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
int journal_load( struct journal* j, size_t datasize, const char* filename );


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
