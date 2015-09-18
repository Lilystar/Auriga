
#ifdef TXT_JOURNAL

#include <string.h>
#include <stdlib.h>

#include "journal.h"
#include "malloc.h"
#include "timer.h"
#include "grfio.h"

#ifndef _WIN32
#	include "unistd.h"
#endif

#define UNUSEDCHUNK_DEFAULT_QUEUESIZE	256		// �L���[�̍ŏ��T�C�Y ( 2 �̗ݏ�ł��邱�� )
#define CHUNKUNITSIZE 					4096	// �`�����N�̍ŏ��T�C�Y�i�����f�B�X�N�̃N���X�^�T�C�Y�ɂ���Ƃ����Ǝv����j

//#define JOURNAL_DEBUG		// ��`����ƁA����I�����Ƀt�@�C�����폜�����ɕʖ��ɂ��Ďc��
							// ".debug0" �͏I���̉ߒ��ŏo��S�~�A".debug1" �͏I�����̃W���[�i��


#define JOURNAL_IDENTIFIER	"AURIGA_JOURNAL02"	// ���ʎq�i�t�@�C���\����ς�����A�Ō�̐��l��ς���ׂ��j
//                           0123456789abcdef

struct journal_header
{
	unsigned int crc32, tick;
	time_t timestamp;
	int key, flag;
};

int journal_flush_timer( int tid, unsigned int tick, int id, int data );

// ==========================================
// �W���[�i���̏�����( load �Ƌ��ʕ���)
// ------------------------------------------
static void journal_init_( struct journal* j, size_t datasize, const char* filename )
{
	static int first = 1;
	if( first ) {
		grfio_load_zlib();
		first = 0;
	}
	memset( j, 0, sizeof( struct journal ) );
	j->db = numdb_init();
	j->datasize = datasize;
	j->chunksize = ( (datasize + sizeof(struct journal_header) + CHUNKUNITSIZE-1 ) / CHUNKUNITSIZE ) * CHUNKUNITSIZE;
	j->nextchunk = 1;
	j->unusedchunk_read = j->unusedchunk_write = 0;
	j->unusedchunk_size = 0;
	j->unusedchunk_queue = NULL;
	j->cache_timer = -1;
	j->mode = 0;
	strcpy( j->filename, filename );
}

// ==========================================
// �W���[�i���̏�����
// ------------------------------------------
void journal_create( struct journal* j, size_t datasize, int cache_interval, const char* filename )
{
	static int first = 1;
	
	// �ŏ��Ȃ�֐����o�^
	if( first )
	{
		first = 0;
		add_timer_func_list( journal_flush_timer, "journal_flush_timer");
	}
	
	journal_init_( j, datasize, filename );
	
	// �t�@�C���w�b�_�̐ݒ�
	memcpy( j->fhd.identifier, JOURNAL_IDENTIFIER, sizeof(j->fhd.identifier) );
	j->fhd.datasize = datasize;
	
	// �t�@�C��������ăw�b�_��������
	if( ( j->fp = fopen( filename, "w+b" ) ) == NULL )
	{
		printf("journal: fopen [%s]: failed\n", filename );
		exit(-1);
	}
	fwrite( &j->fhd, sizeof(j->fhd), 1, j->fp );
	
	// �L���b�V������Ȃ�^�C�}�[�ݒ�
	if( cache_interval > 0)
	{
		j->cache_timer = add_timer_interval( gettick()+ rand()%cache_interval + cache_interval,
											 journal_flush_timer, 0, (int)j, cache_interval);
	}
}

// ==========================================
// �W���[�i���̖��g�p�G���A��ǉ�
// ------------------------------------------

static void journal_push_free( struct journal *j, int pos ) {
	// �L���[�̃��������Ȃ��Ȃ�m�ۂ���
	if( j->unusedchunk_size==0 )
	{
		j->unusedchunk_size = UNUSEDCHUNK_DEFAULT_QUEUESIZE;
		j->unusedchunk_queue = (int *)aCalloc( sizeof(int), UNUSEDCHUNK_DEFAULT_QUEUESIZE );
	}
	
	// �L���[�ɓo�^
	j->unusedchunk_queue[ j->unusedchunk_write ++ ] = pos;
	j->unusedchunk_write %= j->unusedchunk_size;
	
	if( j->unusedchunk_read == j->unusedchunk_write )
	{
		// �L���[�������ς��ɂȂ����̂Ŋg������ ( �L���[�����̏��Ԃ͓���ւ���Ă��悢 )
		int* p = (int *)aCalloc( sizeof(int), j->unusedchunk_size*2 );
		memcpy( p, j->unusedchunk_queue, sizeof(int)*j->unusedchunk_size );
		aFree( j->unusedchunk_queue );
		j->unusedchunk_read  = 0;
		j->unusedchunk_write = j->unusedchunk_size;
		j->unusedchunk_size *= 2;
		j->unusedchunk_queue = p;
		
		printf("journal: unused-chunk-queue size expanded (%d, [%s]).\n", j->unusedchunk_size, j->filename );
	}
}

// ==========================================
// �W���[�i���̃f�[�^�̃f�X�g���N�^
// ------------------------------------------
static void journal_final_dtor( struct journal_data* dat )
{
	if( dat->buf ){ aFree(dat->buf); dat->buf = NULL; }
	aFree( dat );
}

// ==========================================
// �W���[�i���̃f�[�^�̏I������(db_foreach)
// ------------------------------------------
static int journal_final_sub( void* key, void* data, va_list ap )
{
	journal_final_dtor( (struct journal_data*) data );
	return 0;
}

// ==========================================
// �W���[�i���̏I������
// ------------------------------------------
void journal_final( struct journal* j )
{
	// �L���b�V���t���b�V���p�̃^�C�}�[�폜
	if( j->cache_timer != -1 )
	{
		delete_timer( j->cache_timer, journal_flush_timer );
		j->cache_timer = -1;
	}

	// �쐬���[�h�Ȃ�t���b�V��
	if( j->mode==0 && j->db && j->fp )
	{
		journal_flush( j );
	}

	// �t�@�C�������
	if( j->fp )
	{
		fclose( j->fp );
		j->fp = NULL;
	}

	// �f�[�^�j��
	if( j->db ){
		numdb_final( j->db, journal_final_sub );
		j->db=NULL;
	}
	
	// �󂫃L���[�̍폜
	if( j->unusedchunk_queue )
	{
		aFree( j->unusedchunk_queue );
		j->unusedchunk_queue = NULL;
		j->unusedchunk_size = 0;
	}
	
	// �t�@�C���̍폜
	if( j->mode==0 && j->filename[0] )
#ifdef JOURNAL_DEBUG
	{
		char newname[1040], newname2[1040];
		sprintf( newname,"%s.debug1", j->filename );
		sprintf( newname2,"%s.debug0", j->filename );
		//unlink( newname );
		remove( newname );
		rename( newname2, newname );
		rename( j->filename, newname2 );
	}
#else
		//unlink( j->filename );
		remove( j->filename );
#endif
}

// ==========================================
// �W���[�i���֏�������(�W���[�i���̃L���b�V���֏�������)
// ------------------------------------------
int journal_write( struct journal* j, int key, const void* data )
{
	struct journal_data* dat;
	
	if( !j->db )
	{
		printf("journal_write: error: journal not ready\n");
		return 0;
	}
	 
	dat = (struct journal_data*) numdb_search( j->db, key );
	
	// �W���[�i���f�[�^�̓o�^
	if( !dat )
	{
		dat = (struct journal_data *)aCalloc( 1,sizeof(struct journal_data ) );
		numdb_insert( j->db, key, dat );
		dat->idx = -1;
	}
	
	// �L���b�V���p�̃������m��
	if( !dat->buf )
	{
		dat->buf = aCalloc( 1, j->datasize );
	}

	// �f�[�^�̏����������폜��
	if( data )
	{
		memcpy( dat->buf, data, j->datasize );
		dat->flag = JOURNAL_FLAG_WRITE;
	}
	else
	{
		memset( dat->buf, 0, j->datasize );
		dat->flag = JOURNAL_FLAG_DELETE;
	}
	
	// �L���b�V�����Ȃ��Ȃ炷���Ƀt�@�C���ɏ�������
	if( j->cache_timer==-1 )
	{
		journal_flush( j );
	}
	
	return 1;
}

// ==========================================
// �W���[�i���֏�������(�W���[�i���̃L���b�V������t�@�C���ւ̏�������)
// ------------------------------------------
static int journal_flush_sub( void* key, void* data, va_list ap )
{
	struct journal* j = va_arg( ap, struct journal * );
	unsigned int tick = va_arg( ap, unsigned int );
	time_t timestamp  = va_arg( ap, time_t );
	struct journal_data* dat = (struct journal_data*) data;
	struct journal_header jhd;	
	int old_idx = dat->idx;
	
	// �L���b�V���f�[�^�͂Ȃ��̂Ŕ�΂�
	if( !dat->buf )
	{
		return 0;
	}
	
	// �t�@�C�����̈ʒu�������I�ɒu��������̂́A���̃f�[�^�̏������ݒ���
	// ��肪�N���������ɁA�ȑO�̏������݃f�[�^�𐶂�����悤�ɂ���ׂł��B
	if( j->unusedchunk_read != j->unusedchunk_write )
	{
		// �󂫃`�����N������o��
		dat->idx = j->unusedchunk_queue[ j->unusedchunk_read ++ ];
		j->unusedchunk_read %= j->unusedchunk_size;
	}
	else
	{
		// �󂫂��Ȃ��̂ŐV�������
		dat->idx = ( j->nextchunk ++ );
	}
	
	// �W���[�i���������ݗp�̃w�b�_�ݒ�
	jhd.key = (int)key;
	jhd.timestamp = timestamp;
	jhd.tick = tick;
	jhd.flag = dat->flag;
	jhd.crc32 = grfio_crc32( (const char *)dat->buf, j->datasize );
	
	// �f�[�^��������
	fseek( j->fp, dat->idx * j->chunksize, SEEK_SET );
	if( fwrite( &jhd, sizeof(jhd), 1, j->fp )==0 ||
		fwrite( dat->buf, j->datasize, 1, j->fp )==0 )
	{
		printf("journal: file write error! key=%d\n", (int)key );
		return 0;
	}
	
	// �������񂾂̂ŃL���b�V���f�[�^�͂�������Ȃ�
	aFree( dat->buf );
	dat->buf = NULL;
	
	if( old_idx != -1 )
	{
		journal_push_free( j, old_idx );
	}
	
	return 1;
}

// ==========================================
// �W���[�i���̑S�L���b�V�����t�@�C���֏�������
// ------------------------------------------
int journal_flush( struct journal* j )
{
	if( j->db && j->fp )
		numdb_foreach( j->db, journal_flush_sub, j, gettick(), time(NULL) );
	
	if( j->fp )
		fflush( j->fp );
	return 0;
}

// ==========================================
// �W���[�i���̑S�L���b�V�����t�@�C���֏�������(�^�C�}�[)
// ------------------------------------------
int journal_flush_timer( int tid, unsigned int tick, int id, int data )
{
	journal_flush( (struct journal* )data );
	return 0;
}

// ==========================================
// �W���[�i������f�[�^���폜
// ------------------------------------------
int journal_delete( struct journal* j, int key )
{
	struct journal_data* dat = (struct journal_data*) numdb_search( j->db, key );
	if( dat )
	{
		journal_push_free( j, dat->idx );
		numdb_erase( j->db, key );
		journal_final_dtor( dat );
		return 1;
	}
	return 0;
}


// ==========================================
// �W���[�i������ŐV�f�[�^�̎擾( commit �Ɏg�p)
// ------------------------------------------
const char* journal_get( struct journal* j, int key, int* flag )
{
	struct journal_data* dat = (struct journal_data*) numdb_search( j->db, key );
	if( dat )
	{
		if( flag ) *flag = dat->flag;
		return (const char *)dat->buf;
	}
	return NULL;
}

// ==========================================
// �W���[�i���̑S�L���b�V�����t�@�C������ǂݍ���
// ------------------------------------------
int journal_load( struct journal* j, size_t datasize, const char* filename )
{
	struct journal_header jhd;
	int c,i ;

	journal_init_( j, datasize, filename );

	// �t�@�C����ǂݍ��ݗp�ɊJ�� 
	if( ( j->fp = fopen( filename, "r+b" ) ) == NULL )
	{
		return 0;
	}

	j->mode = 1;	// �ǂݎ�胂�[�h�t���O

	// �t�@�C�������������`�F�b�N
	fread( &j->fhd, 1, sizeof(j->fhd), j->fp );
	if( memcmp( j->fhd.identifier, JOURNAL_IDENTIFIER, sizeof(j->fhd.identifier) ) !=0 ||
		j->fhd.datasize != datasize )
	{
		printf("journal: file version or datasize mismatch ! [%s]\n", filename );
		abort();
	}
	
	// �f�[�^�̓ǂݍ��݃��[�v
	c = 0;
	for( i=1; fseek( j->fp, i*j->chunksize, SEEK_SET ), fread( &jhd, sizeof(jhd), 1, j->fp ) > 0; i++ )
	{
		struct journal_data *dat;
		int x;
		char* buf = (char *)aCalloc( 1, datasize );
		
		// �f�[�^�{�̂̓ǂݍ��݂� crc32 �`�F�b�N
		if( (x=fread( buf, datasize, 1, j->fp )) == 0 || grfio_crc32( buf, datasize ) != jhd.crc32 )
		{
			printf("journal: file broken [%s], but continue...\n", filename );
			aFree( buf );
			continue;	// ���̃f�[�^�����ĂĂ����̃f�[�^�͐����Ă�Ǝv����
		}
		
		// �o�^����
		dat = (struct journal_data*) numdb_search( j->db, jhd.key );
		if( dat )
		{
			// ���łɂ���̂Œu���������K�v�����ׂ�
			if( jhd.timestamp > dat->timestamp  &&  DIFF_TICK( jhd.tick, dat->tick ) > 0 )
			{
				// �u������
				journal_push_free( j, dat->idx );
				aFree( dat->buf );
				dat->buf = buf;
				dat->timestamp = jhd.timestamp;
				dat->tick = jhd.tick;
				dat->flag = jhd.flag;
				dat->idx  = i;
			}
			else
			{
				// �u�������Ȃ�
				aFree( buf );
			}
		}
		else
		{
			// �V�����o�^
			dat = (struct journal_data*) aCalloc( 1, sizeof(struct journal_data) );
			dat->buf = buf;
			dat->timestamp = jhd.timestamp;
			dat->tick = jhd.tick;
			dat->flag = jhd.flag;
			dat->idx  = i;
			numdb_insert( j->db, jhd.key, dat );
			c++;
		}
		
	}
	return c;
}

// ==========================================
// �W���[�i���̃��[���t�H���[�h(foreach)
// ------------------------------------------
typedef int(* JOURNAL_ROLLFORWARD_CALLBACK )( int, void*, int );
static int journal_rollforward_sub( void* key, void* data, va_list ap )
{
	JOURNAL_ROLLFORWARD_CALLBACK func = (JOURNAL_ROLLFORWARD_CALLBACK) va_arg( ap, JOURNAL_ROLLFORWARD_CALLBACK );
	struct journal_data* dat = (struct journal_data*) data;
	int* c = va_arg( ap, int* );
	
	*c += func( (int)key, dat->buf, dat->flag );
	
	return 0;
}

// ==========================================
// �W���[�i���̃��[���t�H���[�h
// ------------------------------------------
int journal_rollforward( struct journal* j, int(*func)( int key, void* buf, int flag ) )
{
	int c = 0;
	numdb_foreach( j->db, journal_rollforward_sub, func, &c );
	return c;
}

#endif
