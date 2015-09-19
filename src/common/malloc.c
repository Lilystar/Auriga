#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>

#ifdef MEMWATCH
#include "memwatch.h"
#endif

#define _MALLOC_C_
#include "malloc.h"

// �Ǝ��������}�l�[�W�����g�p���Ȃ��ꍇ�A�����R�����g�A�E�g���Ă�������
#define USE_MEMMGR

// �f�o�b�O���[�h�i�G���[�`�F�b�N�̋����p�A�ʏ�^�c�ł͂����߂ł��Ȃ��j
//#define DEBUG_MEMMGR


#ifdef MEMWATCH

void* aMalloc_( size_t size, const char *file, int line, const char *func )
{
	void *ret = mwMalloc(size,file,line);

	if(ret == NULL) {
		exit(1);
	}
	return ret;
}

void* aCalloc_( size_t num, size_t size, const char *file, int line, const char *func )
{
	void *ret = mwCalloc(num,size,file,line);

	if(ret == NULL) {
		exit(1);
	}
	return ret;
}

void* aRealloc_( void *p, size_t size, const char *file, int line, const char *func )
{
	void *ret = mwRealloc(p,size,file,line);

	if(ret == NULL) {
		exit(1);
	}
	return ret;
}

void* aStrdup_( const void *p, const char *file, int line, const char *func )
{
	void *ret = mwStrdup(p,file,line);

	if(ret == NULL) {
		exit(1);
	}
	return ret;
}

void aFree_( void *p, const char *file, int line, const char *func )
{
	mwFree(p,file,line);
}

int do_init_memmgr(const char* file)
{
	mwInit();
	atexit(mwAbort);

	return 0;
}

double memmgr_usage(void)
{
	return 0;
}

#elif !defined(USE_MEMMGR)

void* aMalloc_( size_t size, const char *file, int line, const char *func )
{
	void *ret = malloc(size);

	if(ret == NULL) {
		printf("%s:%d: in func %s: malloc error out of memory!\n",file,line,func);
		exit(1);
	}
	return ret;
}

void* aCalloc_( size_t num, size_t size, const char *file, int line, const char *func )
{
	void *ret = calloc(num,size);

	if(ret == NULL) {
		printf("%s:%d: in func %s: calloc error out of memory!\n",file,line,func);
		exit(1);
	}
	return ret;
}

void* aRealloc_( void *p, size_t size, const char *file, int line, const char *func )
{
	void *ret = realloc(p,size);

	if(ret == NULL) {
		printf("%s:%d: in func %s: realloc error out of memory!\n",file,line,func);
		exit(1);
	}
	return ret;
}

void* aStrdup_( const void *p, const char *file, int line, const char *func )
{
	void *ret = strdup(p);

	if(ret == NULL) {
		printf("%s:%d: in func %s: strdup error out of memory!\n",file,line,func);
		exit(1);
	}
	return ret;
}

void aFree_( void *p, const char *file, int line, const char *func )
{
	free(p);
}

int do_init_memmgr(const char* file)
{
	return 0;
}

double memmgr_usage(void)
{
	return 0;
}

#else /* USE_MEMMGR */

/*
 * �������}�l�[�W��
 *     malloc , free �̏����������I�ɏo����悤�ɂ������́B
 *     ���G�ȏ������s���Ă���̂ŁA�኱�d���Ȃ邩������܂���B
 *
 * �f�[�^�\���Ȃǁi��������ł����܂���^^; �j
 *     �E�������𕡐��́u�u���b�N�v�ɕ����āA����Ƀu���b�N�𕡐��́u���j�b�g�v
 *       �ɕ����Ă��܂��B���j�b�g�̃T�C�Y�́A�P�u���b�N�̗e�ʂ𕡐��ɋϓ��z��
 *       �������̂ł��B���Ƃ��΁A�P���j�b�g32KB�̏ꍇ�A�u���b�N�P��32Byte�̃�
 *       �j�b�g���A1024�W�܂��ďo���Ă�����A64Byte�̃��j�b�g�� 512�W�܂���
 *       �o���Ă����肵�܂��B�ipadding,unit_head �������j
 *
 *     �E�u���b�N���m�̓����N���X�g(block_prev,block_next) �łȂ���A�����T�C
 *       �Y�����u���b�N���m�������N���X�g(hash_prev,hash_nect) �ł�
 *       �����Ă��܂��B����ɂ��A�s�v�ƂȂ����������̍ė��p�������I�ɍs���܂��B
 */

/* �u���b�N�̃A���C�����g */
#define BLOCK_ALIGNMENT1	16
#define BLOCK_ALIGNMENT2	64

/* �u���b�N�ɓ���f�[�^�� */
#define BLOCK_DATA_COUNT1	128
#define BLOCK_DATA_COUNT2	608

/* �u���b�N�̑傫��: 16*128 + 64*608 = 40KB */
#define BLOCK_DATA_SIZE1	( BLOCK_ALIGNMENT1 * BLOCK_DATA_COUNT1 )
#define BLOCK_DATA_SIZE2	( BLOCK_ALIGNMENT2 * BLOCK_DATA_COUNT2 )
#define BLOCK_DATA_SIZE		( BLOCK_DATA_SIZE1 + BLOCK_DATA_SIZE2 )

/* ��x�Ɋm�ۂ���u���b�N�̐� */
#define BLOCK_ALLOC		104

/* �u���b�N */
struct block {
	struct block* block_next;		/* ���Ɋm�ۂ����̈� */
	struct block* unfill_prev;		/* ���̖��܂��Ă��Ȃ��̈� */
	struct block* unfill_next;		/* ���̖��܂��Ă��Ȃ��̈� */
	unsigned short unit_size;		/* ���j�b�g�̑傫�� */
	unsigned short unit_hash;		/* ���j�b�g�̃n�b�V�� */
	unsigned short unit_count;		/* ���j�b�g�̌� */
	unsigned short unit_used;		/* �g�p���j�b�g�� */
	unsigned short unit_unfill;		/* ���g�p���j�b�g�̏ꏊ */
	unsigned short unit_maxused;		/* �g�p���j�b�g�̍ő�l */
	char data[ BLOCK_DATA_SIZE ];
};

struct unit_head {
	struct block   *block;
	const  char*   file;
	unsigned short line;
	unsigned short size;
#ifdef DEBUG_MEMMGR
	time_t time_stamp;
#endif
	long checksum;
};

static struct block* hash_unfill[BLOCK_DATA_COUNT1 + BLOCK_DATA_COUNT2 + 1];
static struct block* block_first, *block_last, block_head;

/* ���������g���񂹂Ȃ��̈�p�̃f�[�^ */
struct unit_head_large {
	size_t                  size;
	struct unit_head_large* prev;
	struct unit_head_large* next;
	struct unit_head        unit_head;
};

static struct unit_head_large *unit_head_large_first = NULL;

static struct block* block_malloc(unsigned short hash);
static void          block_free(struct block* p);
static void          memmgr_warning(const char* format,...);
static size_t        memmgr_usage_bytes;

#define block2unit(p, n) ((struct unit_head*)(&(p)->data[ p->unit_size * (n) ]))

#ifdef DEBUG_MEMMGR
#define memmgr_assert(v) do { if(!(v)) { memmgr_warning("memmgr_assert: " #v "\n"); } } while(0)
#else
#define memmgr_assert(v)
#endif

static unsigned short size2hash( size_t size )
{
	if( size <= BLOCK_DATA_SIZE1 )
		return (unsigned short)(size + BLOCK_ALIGNMENT1 - 1) / BLOCK_ALIGNMENT1;

	if( size <= BLOCK_DATA_SIZE )
		return (unsigned short)(size - BLOCK_DATA_SIZE1 + BLOCK_ALIGNMENT2 - 1) / BLOCK_ALIGNMENT2 + BLOCK_DATA_COUNT1;

	return 0xffff;	// �u���b�N���𒴂���ꍇ�� hash �ɂ��Ȃ�
}

static size_t hash2size( unsigned short hash )
{
	if( hash <= BLOCK_DATA_COUNT1)
		return hash * BLOCK_ALIGNMENT1;

	return (hash - BLOCK_DATA_COUNT1) * BLOCK_ALIGNMENT2 + BLOCK_DATA_SIZE1;
}

void* aMalloc_(size_t size, const char *file, int line, const char *func)
{
	struct block *block;
	unsigned short size_hash = size2hash( size );
	struct unit_head *head;

	if(size == 0) {
		return NULL;
	}
	memmgr_usage_bytes += size;

	/* �u���b�N���𒴂���̈�̊m�ۂɂ́Amalloc() ��p���� */
	/* ���̍ہAunit_head.block �� NULL �������ċ�ʂ��� */
	if(hash2size(size_hash) > BLOCK_DATA_SIZE - sizeof(struct unit_head)) {
		struct unit_head_large* p = (struct unit_head_large *)malloc(sizeof(struct unit_head_large) + size);

		if(p == NULL) {
			printf("MEMMGR::memmgr_alloc failed.\n");
			exit(1);
		}
#ifdef DEBUG_MEMMGR
		// �^�C���X�^���v�̋L�^
		p->unit_head.time_stamp = time(NULL);
#endif
		p->size            = size;
		p->unit_head.block = NULL;
		p->unit_head.size  = 0;
		p->unit_head.file  = file;
		p->unit_head.line  = line;
		if(unit_head_large_first == NULL) {
			unit_head_large_first = p;
			p->next = NULL;
			p->prev = NULL;
		} else {
			unit_head_large_first->prev = p;
			p->prev = NULL;
			p->next = unit_head_large_first;
			unit_head_large_first = p;
		}
		*(long*)((char*)p + sizeof(struct unit_head_large) - sizeof(long) + size) = 0xdeadbeaf;

		return (char *)p + sizeof(struct unit_head_large) - sizeof(long);
	}

	/* ����T�C�Y�̃u���b�N���m�ۂ���Ă��Ȃ����A�V���Ɋm�ۂ��� */
	if(hash_unfill[size_hash]) {
		block = hash_unfill[size_hash];
	} else {
		block = block_malloc(size_hash);
	}

	if( block->unit_unfill == 0xFFFF ) {
		// free�ςݗ̈悪�c���Ă��Ȃ�
		memmgr_assert(block->unit_used <  block->unit_count);
		memmgr_assert(block->unit_used == block->unit_maxused);
		head = block2unit(block, block->unit_maxused);
		block->unit_maxused++;
	} else {
		head = block2unit(block, block->unit_unfill);
		block->unit_unfill = head->size;
	}
	block->unit_used++;

	if( block->unit_unfill == 0xFFFF && block->unit_maxused >= block->unit_count) {
		// ���j�b�g���g���ʂ������̂ŁAunfill���X�g����폜
		if( block->unfill_prev == &block_head ) {
			hash_unfill[ size_hash ] = block->unfill_next;
		} else {
			block->unfill_prev->unfill_next = block->unfill_next;
		}
		if( block->unfill_next ) {
			block->unfill_next->unfill_prev = block->unfill_prev;
		}
		block->unfill_prev = NULL;
	}

#ifdef DEBUG_MEMMGR
	{
		size_t i, sz = hash2size( size_hash );
		for(i = 0; i < sz; i++)
		{
			if( ((unsigned char*)head)[ sizeof(struct unit_head) - sizeof(long) + i] != 0xfd )
			{
				if( head->line != 0xfdfd )
					memmgr_warning("memmgr: freed-data is changed. (freed in %s line %d)\n", head->file, head->line);
				else
					memmgr_warning("memmgr: not-allocated-data is changed.\n");
				break;
			}
		}
		memset( (char *)head + sizeof(struct unit_head) - sizeof(long), 0xcd, sz );
		// �^�C���X�^���v�̋L�^
		head->time_stamp = time(NULL);
	}
#endif

	head->block = block;
	head->file  = file;
	head->line  = line;
	head->size  = (unsigned short)size;
	*(long*)((char*)head + sizeof(struct unit_head) - sizeof(long) + size) = 0xdeadbeaf;

	return (char *)head + sizeof(struct unit_head) - sizeof(long);
};

void* aCalloc_(size_t num, size_t size, const char *file, int line, const char *func)
{
	void *p = aMalloc_(num * size,file,line,func);

	memset(p,0,num * size);
	return p;
}

void* aRealloc_(void *memblock, size_t size, const char *file, int line, const char *func)
{
	size_t old_size;
	void *p;

	if(memblock == NULL)
		return aMalloc_(size,file,line,func);

	if(size == 0) {
		aFree_(memblock,file,line,func);
		return NULL;
	}

	old_size = ((struct unit_head *)((char *)memblock - sizeof(struct unit_head) + sizeof(long)))->size;
	if( old_size == 0 ) {
		old_size = ((struct unit_head_large *)((char *)memblock - sizeof(struct unit_head_large) + sizeof(long)))->size;
	}

	p = aMalloc_(size,file,line,func);
	if(p != NULL) {
		if(old_size > size)
			memcpy(p, memblock, size);	// �T�C�Y�k��
		else
			memcpy(p, memblock, old_size);	// �T�C�Y�g��
	}
	aFree_(memblock,file,line,func);
	return p;
}

void* aStrdup_(const void* string, const char *file, int line, const char *func)
{
	if(string == NULL) {
		return NULL;
	} else {
		size_t len = strlen((const char *)string);
		char   *p  = (char *)aMalloc_(len + 1,file,line,func);
		memcpy(p,string,len+1);
		return p;
	}
}

void aFree_(void *ptr, const char *file, int line, const char *func)
{
	struct unit_head *head;

	if(ptr == NULL)
		return;

	head = (struct unit_head *)((char *)ptr - sizeof(struct unit_head) + sizeof(long));
	if(head->size == 0) {
		/* malloc() �Œ��Ɋm�ۂ��ꂽ�̈� */
		struct unit_head_large *head_large = (struct unit_head_large *)((char *)ptr - sizeof(struct unit_head_large) + sizeof(long));

		if(*(long*)((char*)head_large + sizeof(struct unit_head_large) - sizeof(long) + head_large->size) != 0xdeadbeaf)
		{
			memmgr_warning("memmgr: args of aFree is overflowed pointer %s line %d\n",file,line);
		} else {
			head->size = 0xffff;
			if(head_large->prev) {
				head_large->prev->next = head_large->next;
			} else {
				unit_head_large_first  = head_large->next;
			}
			if(head_large->next) {
				head_large->next->prev = head_large->prev;
			}
			memmgr_usage_bytes -= head_large->size;
			free(head_large);
		}
	} else {
		/* ���j�b�g��� */
		struct block *block = head->block;

		if( (char*)head - (char*)block > sizeof(struct block) ) {
			memmgr_warning("memmgr: args of aFree is invalid pointer %s line %d\n",file,line);
		} else if(head->block == NULL) {
			memmgr_warning("memmgr: args of aFree is freed pointer %s line %d\n",file,line);
		} else if(*(long*)((char*)head + sizeof(struct unit_head) - sizeof(long) + head->size) != 0xdeadbeaf) {
			memmgr_warning("memmgr: args of aFree is overflowed pointer %s line %d\n",file,line);
		} else {
			memmgr_usage_bytes -= head->size;
			head->block         = NULL;
#ifdef DEBUG_MEMMGR
			memset( ptr, 0xfd, block->unit_size - sizeof(struct unit_head) + sizeof(long) );
			head->file = file;
			head->line = line;
#endif
			memmgr_assert( block->unit_used > 0 );
			if(--block->unit_used == 0) {
				/* �u���b�N�̉�� */
				block_free(block);
			} else {
				if( block->unfill_prev == NULL ) {
					// unfill ���X�g�ɒǉ�
					if( hash_unfill[ block->unit_hash ] ) {
						hash_unfill[ block->unit_hash ]->unfill_prev = block;
					}
					block->unfill_prev = &block_head;
					block->unfill_next = hash_unfill[ block->unit_hash ];
					hash_unfill[ block->unit_hash ] = block;
				}
				head->size = block->unit_unfill;
				block->unit_unfill = (unsigned short)(((unsigned long)head - (unsigned long)block->data) / block->unit_size);
			}
		}
	}
}

/* �u���b�N���m�ۂ��� */
static struct block* block_malloc(unsigned short hash)
{
	int i;
	struct block *p;

	if(hash_unfill[0] != NULL) {
		/* �u���b�N�p�̗̈�͊m�ۍς� */
		p = hash_unfill[0];
		hash_unfill[0] = hash_unfill[0]->unfill_next;
	} else {
		/* �u���b�N�p�̗̈��V���Ɋm�ۂ��� */
		p = (struct block *)malloc(sizeof(struct block) * BLOCK_ALLOC);

		if(p == NULL) {
			printf("MEMMGR::block_alloc failed.\n");
			exit(1);
		}
#ifdef _WIN64
		if( ((__int64)p + sizeof(struct block) * BLOCK_ALLOC) >> 32 ) {
			printf("memmgr: 64bit version does not compatible with this machine\n");
			exit(1);
		}
#endif
		if(block_first == NULL) {
			/* ����m�� */
			block_first = p;
		} else {
			block_last->block_next = p;
		}
		block_last = &p[BLOCK_ALLOC - 1];
		block_last->block_next = NULL;
		/* �u���b�N��A�������� */
		for(i = 0; i < BLOCK_ALLOC; i++) {
			if(i != 0) {
				// p[0] �͂��ꂩ��g���̂Ń����N�ɂ͉����Ȃ�
				p[i].unfill_next = hash_unfill[0];
				hash_unfill[0]   = &p[i];
				p[i].unfill_prev = NULL;
			}
			if(i != BLOCK_ALLOC -1) {
				p[i].block_next = &p[i+1];
			}
			p[i].unit_used = 0;
		}
	}

	// unfill �ɒǉ�
	memmgr_assert(hash_unfill[ hash ] == NULL);
	hash_unfill[ hash ] = p;
	p->unfill_prev  = &block_head;
	p->unfill_next  = NULL;
	p->unit_size    = (unsigned short)(hash2size( hash ) + sizeof(struct unit_head));
	p->unit_hash    = hash;
	p->unit_count   = BLOCK_DATA_SIZE / p->unit_size;
	p->unit_used    = 0;
	p->unit_unfill  = 0xFFFF;
	p->unit_maxused = 0;
#ifdef DEBUG_MEMMGR
	memset( p->data, 0xfd, sizeof(p->data) );
#endif
	return p;
}

static void block_free(struct block* p)
{
	if( p->unfill_prev ) {
		if( p->unfill_prev == &block_head ) {
			hash_unfill[ p->unit_hash ] = p->unfill_next;
		} else {
			p->unfill_prev->unfill_next = p->unfill_next;
		}
		if( p->unfill_next ) {
			p->unfill_next->unfill_prev = p->unfill_prev;
		}
		p->unfill_prev = NULL;
	}

	p->unfill_next = hash_unfill[0];
	hash_unfill[0] = p;
}

static char memmer_logfile[128];

static void memmgr_warning(const char* format,...)
{
	FILE *fp = fopen(memmer_logfile,"a");
	va_list ap;

	va_start(ap,format);

	if(fp) {
		vfprintf(fp,format,ap);
		fclose(fp);
	}
	vprintf(format,ap);
	va_end(ap);
}

static FILE* memmgr_log(void)
{
	FILE *fp = fopen(memmer_logfile,"a");

	if(!fp) {
		fp = stdout;
	}
	fprintf(fp,"memmgr: memory leaks found" RETCODE);

	return fp;
}

static void memmer_exit(void)
{
	FILE *fp = NULL;
	int count = 0;
	struct block *block = block_first;
	struct unit_head_large *large = unit_head_large_first;

	while(block) {
		if(block->unit_used > 0) {
			int i;
			if(!fp) {
				fp = memmgr_log();
			}
			for(i = 0; i < block->unit_maxused; i++) {
				struct unit_head *head = block2unit(block, i);
				if(head->block != NULL) {
#ifdef DEBUG_MEMMGR
					{
						char buf[24];
						strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", localtime(&head->time_stamp));
						fprintf(
							fp,"%04d [%s] : %s line %d size %d" RETCODE,++count,buf,
							head->file,head->line,head->size
						);
					}
#else
					fprintf(
						fp,"%04d : %s line %d size %d" RETCODE,++count,
						head->file,head->line,head->size
					);
#endif
				}
			}
		}
		block = block->block_next;
	}
	while(large) {
		if(!fp) {
			fp = memmgr_log();
		}

#ifdef DEBUG_MEMMGR
		{
			char buf[24];
			strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", localtime(&large->unit_head.time_stamp));
			fprintf(
				fp,"%04d [%s] : %s line %d size %d" RETCODE,++count,buf,
				large->unit_head.file,
				large->unit_head.line,large->unit_head.size
			);
		}
#else
		fprintf(
			fp,"%04d : %s line %d size %d" RETCODE,++count,
			large->unit_head.file,
			large->unit_head.line,large->unit_head.size
		);
#endif
		large = large->next;
	}
	if(!fp) {
		printf("memmgr: no memory leaks found.\n");
	} else {
		printf("memmgr: memory leaks found.\n");
		fclose(fp);
	}
}

int do_init_memmgr(const char* file)
{
	sprintf(memmer_logfile,"%s.log",file);
	atexit(memmer_exit);
	printf("memmgr: initialised: %s\n",memmer_logfile);
	return 0;
}

double memmgr_usage(void)
{
	return memmgr_usage_bytes / 1024.0;
}

#endif
