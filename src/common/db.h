#ifndef _DB_H_
#define _DB_H_

#include <stdarg.h>

#define HASH_SIZE (256+27)

#define RED 0
#define BLACK 1

struct dbn {
	struct dbn *parent,*left,*right;
	int color;
	void *key;
	void *data;
	int  deleted;	// �폜�ς݃t���O(db_foreach)
};
struct dbt {
	int (*cmp)(struct dbt*,void*,void*);
	unsigned int (*hash)(struct dbt*,void*);
	int maxlen;
	struct dbn *ht[HASH_SIZE];
	int item_count; // �v�f�̐�
	const char* alloc_file; // DB�����������ꂽ�t�@�C��
	int         alloc_line; // DB�����������ꂽ�s

	// db_foreach ������db_erase �����΍�Ƃ��āA
	// db_foreach ���I���܂Ń��b�N���邱�Ƃɂ���
	struct db_free {
		struct dbn *z;
		struct dbn **root;
	} *free_list;
	int free_count;
	int free_max;
	int free_lock;
};

#define strdb_search(t,k)   db_search((t),(void*)(k))
#define strdb_insert(t,k,d) db_insert((t),(void*)(k),(void*)(d))
#define strdb_erase(t,k)    db_erase ((t),(void*)(k))
#define strdb_foreach       db_foreach
#define strdb_final         db_final
#define numdb_search(t,k)   db_search((t),(void*)(k))
#define numdb_insert(t,k,d) db_insert((t),(void*)(k),(void*)(d))
#define numdb_erase(t,k)    db_erase ((t),(void*)(k))
#define numdb_foreach       db_foreach
#define numdb_final         db_final

#define strdb_init(a)       strdb_init_(a,__FILE__,__LINE__)
#define numdb_init()        numdb_init_(__FILE__,__LINE__)

struct dbt* strdb_init_(int maxlen,const char *file,int line);
struct dbt* numdb_init_(const char *file,int line);
void* db_search(struct dbt *table,void* key);
struct dbn* db_insert(struct dbt *table,void* key,void* data);
void* db_erase(struct dbt *table,void* key);
void db_foreach(struct dbt*,int(*)(void*,void*,va_list),...);
void db_foreach_sub(struct dbt*,int(*)(void*,void*,va_list), va_list ap);
void db_final(struct dbt*,int(*)(void*,void*,va_list),...);
void exit_dbn(void);

// �����N���X�gDB -- treedb �����K�͂�������������̃f�[�^�x�[�X
// �@�E�L�[�̏d���`�F�b�N��replace �̂�
// �@�E���ב΍�̂��߁A���O�̏�������head��NULL�������邾��
// �@�Elinkdb_node �͊֐����Ŋm�ۂ��邽�߁A���p���̓|�C���^�P��錾���邾��

struct linkdb_node {
	struct linkdb_node *next;
	struct linkdb_node *prev;
	void               *key;
	void               *data;
};

void  linkdb_insert ( struct linkdb_node** head, void *key, void* data); // �d�����l�����Ȃ�
void  linkdb_replace( struct linkdb_node** head, void *key, void* data); // �d�����l������
void* linkdb_search ( struct linkdb_node** head, void *key);
void* linkdb_erase  ( struct linkdb_node** head, void *key);
void  linkdb_final  ( struct linkdb_node** head );

#ifndef NO_CSVDB
// csvdb -- csv �t�@�C���̓ǂݍ��݊֐�

// �ő��
#define MAX_CSVCOL 128

struct csvdb_line {
	int   num;
	char *buf;
	char *data_p[MAX_CSVCOL];
	int   data_v[MAX_CSVCOL];
};

struct csvdb_data {
	struct csvdb_line *data;
	char *file;
	int  *index;
	int row_max;
	int row_count;
	int row_notempty;
	int dirty;
};

struct csvdb_data* csvdb_open(const char* file, int skip_comment);
int         csvdb_get_rows(struct csvdb_data *csv);
int         csvdb_get_columns(struct csvdb_data *csv, int row);
int         csvdb_get_num(struct csvdb_data *csv, int row, int col);
const char* csvdb_get_str(struct csvdb_data *csv, int row, int col);
int         csvdb_set_num(struct csvdb_data *csv, int row, int col, int num);
int         csvdb_set_str(struct csvdb_data *csv, int row, int col, const char* str);
int         csvdb_find_num(struct csvdb_data *csv, int col, int value);
int         csvdb_find_str(struct csvdb_data *csv, int col, const char* value);
int         csvdb_clear_row(struct csvdb_data *csv, int row);
int         csvdb_sort(struct csvdb_data *csv, int key, int order);
int         csvdb_delete_row(struct csvdb_data *csv, int row);
int         csvdb_insert_row(struct csvdb_data *csv, int row);
int         csvdb_flush(struct csvdb_data *csv);
void        csvdb_close(struct csvdb_data *csv);
void        csvdb_dump(struct csvdb_data* csv);
#endif

#endif
