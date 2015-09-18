// $Id: map.c,v 1.3 2004/09/15 00:20:52 running_pinata Exp $
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifndef _WIN32
#	include <netdb.h>
#	include <unistd.h>
#else
#	include <process.h>
#endif

#include "core.h"
#include "timer.h"
#include "db.h"
#include "grfio.h"
#include "nullpo.h"
#include "malloc.h"
#include "version.h"
#include "httpd.h"
#include "graph.h"
#include "socket.h"
#include "utils.h"

#include "map.h"
#include "chrif.h"
#include "clif.h"
#include "intif.h"
#include "npc.h"
#include "pc.h"
#include "mob.h"
#include "chat.h"
#include "itemdb.h"
#include "storage.h"
#include "skill.h"
#include "trade.h"
#include "party.h"
#include "battle.h"
#include "script.h"
#include "guild.h"
#include "pet.h"
#include "atcommand.h"
#include "status.h"
#include "friend.h"
#include "unit.h"
#include "homun.h"
#include "ranking.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

// �ɗ� static�Ń��[�J���Ɏ��߂�
static struct dbt * id_db;
static struct dbt * map_db;
static struct dbt * nick_db;
static struct dbt * charid_db;
//static struct dbt * address_db; // BL�֘A�̃A�h���X���i�[

static int users;
static struct block_list *object[MAX_FLOORITEM];
static int first_free_object_id,last_object_id;

#define block_free_max (128*1024)
static void *block_free[block_free_max];
static int block_free_count=0,block_free_lock=0;

#define BL_LIST_MAX (256*1024)
static struct block_list *bl_list[BL_LIST_MAX];
static int bl_list_count = 0;

struct map_data *map = NULL;
int map_num=0;

int autosave_interval=DEFAULT_AUTOSAVE_INTERVAL;
int autosave_gvg_rate=100;
int agit_flag=0;
int map_pk_server_flag = 0;
int map_pk_nightmaredrop_flag = 0;

extern int packet_parse_time;

static int read_grf_files_txt=1;

// �}�b�v�L���b�V�����p�t���O(map_athana.conf����read_map_from_cache�Ŏw��)
// 0:���p���Ȃ� 1:�񈳏k�ۑ� 2:���k�ۑ�
int map_read_flag=0;

char map_cache_file[256]="db/map.info"; // �}�b�v�L���b�V���t�@�C����
char motd_txt[256]="conf/motd.txt";
char help_txt[256]="conf/help.txt";
char extra_add_file_txt[1024] = "map_extra_add.txt"; // to add items from external software (use append to add a line)

// �����v�Z�p
const int dirx[8] = { 0,-1,-1,-1, 0, 1, 1, 1 };
const int diry[8] = { 1, 1, 0,-1,-1,-1, 0, 1 };

/*==========================================
 * �Smap�I���v�ł̐ڑ����ݒ�
 * (char�I���瑗���Ă���)
 *------------------------------------------
 */
void map_setusers(int n)
{
	users=n;
}

/*==========================================
 * �Smap�I���v�ł̐ڑ����擾 (/w�ւ̉����p)
 *------------------------------------------
 */
int map_getusers(void)
{
	return users;
}

//
// block�폜�̈��S���m�ۏ���
//

/*==========================================
 * block��free����Ƃ�free�̕ς��ɌĂ�
 * ���b�N����Ă���Ƃ��̓o�b�t�@�ɂ��߂�
 *------------------------------------------
 */
int map_freeblock( void *bl )
{
	if(block_free_lock==0){
		aFree(bl);
		bl = NULL;
	}
	else{
		if( block_free_count>=block_free_max ) {
			if(battle_config.error_log)
				printf("map_freeblock: *WARNING* too many free block! %d %d\n",
			block_free_count,block_free_lock);
		}
		else
			block_free[block_free_count++]=bl;
	}
	return block_free_lock;
}
/*==========================================
 * block��free���ꎞ�I�ɋ֎~����
 *------------------------------------------
 */
int map_freeblock_lock(void)
{
	return ++block_free_lock;
}
/*==========================================
 * block��free�̃��b�N����������
 * ���̂Ƃ��A���b�N�����S�ɂȂ��Ȃ��
 * �o�b�t�@�ɂ��܂��Ă���block��S���폜
 *------------------------------------------
 */
int map_freeblock_unlock(void)
{
	if( (--block_free_lock)==0 ){
		int i;
//		if(block_free_count>0) {
//			if(battle_config.error_log)
//				printf("map_freeblock_unlock: free %d object\n",block_free_count);
//		}
		for(i=0;i<block_free_count;i++){
			aFree(block_free[i]);
			block_free[i] = NULL;
		}
		block_free_count=0;
	}else if(block_free_lock<0){
		if(battle_config.error_log)
			printf("map_freeblock_unlock: lock count < 0 !\n");
		block_free_lock = 0; // ����ȍ~�̃��b�N�Ɏx�Ⴊ�o�Ă���̂Ń��Z�b�g
	}
	return block_free_lock;
}

// map_freeblock_lock() ���Ă�� map_freeblock_unlock() ���Ă΂Ȃ�
// �֐����������̂ŁA����I��block_free_lock�����Z�b�g����悤�ɂ���B
// ���̊֐��́Ado_timer() �̃g�b�v���x������Ă΂��̂ŁA
// block_free_lock �𒼐ڂ������Ă��x�ᖳ���͂��B

static int map_freeblock_timer(int tid,unsigned int tick,int id,int data) {
	if(block_free_lock > 0) {
		printf("map_freeblock_timer: block_free_lock(%d) is invalid.\n",block_free_lock);
		block_free_lock = 1;
		map_freeblock_unlock();
	}
	// else {
	// 	printf("map_freeblock_timer: check ok\n");
	// }
	return 0;
}

//
// block������
//
/*==========================================
 * map[]��block_list����q�����Ă���ꍇ��
 * bl->prev��bl_head�̃A�h���X�����Ă���
 *------------------------------------------
 */
static struct block_list bl_head;

/*==========================================
 * map[]��block_list�ɒǉ�
 * mob�͐��������̂ŕʃ��X�g
 *
 * ����link�ς݂��̊m�F�������B�댯����
 *------------------------------------------
 */
int map_addblock(struct block_list *bl)
{
	int m,x,y;
	int b;

	nullpo_retr(0, bl);

	if(bl->prev != NULL){
		if(battle_config.error_log)
			printf("map_addblock error : bl->prev!=NULL\n");
		return 0;
	}

	m=bl->m;
	x=bl->x;
	y=bl->y;
	if(m<0 || m>=map_num ||
	   x<0 || x>=map[m].xs ||
	   y<0 || y>=map[m].ys)
		return 1;

	b = x / BLOCK_SIZE + (y / BLOCK_SIZE) * map[m].bxs;

	if(bl->type==BL_MOB){
		bl->next = map[m].block_mob[b];
		bl->prev = &bl_head;
		if(bl->next) bl->next->prev = bl;
		map[m].block_mob[b] = bl;
	} else {
		bl->next = map[m].block[b];
		bl->prev = &bl_head;
		if(bl->next) bl->next->prev = bl;
		map[m].block[b] = bl;
		if(bl->type==BL_PC)
			map[m].users++;
	}

	return 0;
}

/*==========================================
 * map[]��block_list����O��
 * prev��NULL�̏ꍇlist�Ɍq�����ĂȂ�
 *------------------------------------------
 */
int map_delblock(struct block_list *bl)
{
	int b;
	nullpo_retr(0, bl);

	// ����blocklist���甲���Ă���
	if(bl->prev==NULL){
		if(bl->next!=NULL){
			// prev��NULL��next��NULL�łȂ��̂͗L���Ă͂Ȃ�Ȃ�
			if(battle_config.error_log)
				printf("map_delblock error : bl->next!=NULL\n");
		}
		return 0;
	}

	b = bl->x/BLOCK_SIZE+(bl->y/BLOCK_SIZE)*map[bl->m].bxs;

	if(bl->type==BL_PC)
		map[bl->m].users--;
	if(bl->next) bl->next->prev = bl->prev;
	if(bl->prev==&bl_head){
		// ���X�g�̓��Ȃ̂ŁAmap[]��block_list���X�V����
		if(bl->type==BL_MOB){
			map[bl->m].block_mob[b] = bl->next;
		} else {
			map[bl->m].block[b] = bl->next;
		}
	} else {
		bl->prev->next = bl->next;
	}
	bl->next = NULL;
	bl->prev = NULL;

	return 0;
}

/*==========================================
 * BL�̃A�h���X�����������̂��m�F����
 *------------------------------------------
 */
int map_is_valid_address(struct block_list *bl, const char *file, int line) {
/*
	if( ! numdb_search(address_db, bl) ) {
		printf("map_is_valid_address: invalid address %p %s line %d\n", bl, file, line);
		return 0;
	}
*/
	return 1;
}

/*==========================================
 * ���͂�PC�l���𐔂��� (���ݖ��g�p)
 *------------------------------------------
 */
/*
int map_countnearpc(int m,int x,int y)
{
	int bx,by,c=0;
	struct block_list *bl;

	if(map[m].users==0)
		return 0;
	for(by=y/BLOCK_SIZE-AREA_SIZE/BLOCK_SIZE-1;by<=y/BLOCK_SIZE+AREA_SIZE/BLOCK_SIZE+1;by++){
		if(by<0 || by>=map[m].bys)
			continue;
		for(bx=x/BLOCK_SIZE-AREA_SIZE/BLOCK_SIZE-1;bx<=x/BLOCK_SIZE+AREA_SIZE/BLOCK_SIZE+1;bx++){
			if(bx<0 || bx>=map[m].bxs)
				continue;
			bl = map[m].block[bx+by*map[m].bxs];
			for(;bl;bl=bl->next){
				if(bl->type==BL_PC)
					c++;
			}
		}
	}
	return c;
}*/

/*==========================================
 * �Z�����PC��MOB�̐��𐔂��� (�ėp)
 *------------------------------------------
 */
int map_count_oncell(int m,int x,int y,int type)
{
	int bx,by;
	struct block_list *bl;
	int count = 0;

	if (x < 0 || y < 0 || (x >= map[m].xs) || (y >= map[m].ys))
		return 1;
	bx = x/BLOCK_SIZE;
	by = y/BLOCK_SIZE;

	if (type == 0 || type&~BL_MOB) {
		bl = map[m].block[bx+by*map[m].bxs];
		for(;bl;bl=bl->next) {
			if(type && !(bl->type & type))
				continue;
			if(bl->x == x && bl->y == y) count++;
		}
	}
	if (type == 0 || type&BL_MOB) {
		bl = map[m].block_mob[bx+by*map[m].bxs];
		for(;bl;bl=bl->next){
			if(bl->x == x && bl->y == y) count++;
		}
	}
	return count;
}
/*
 * �Z����̍ŏ��Ɍ������X�L�����j�b�g��Ԃ�
 *   out_unit: �����ΏۊO�Ƃ��郆�j�b�g
 */
struct skill_unit *map_find_skill_unit_oncell(struct block_list *target,int x,int y,int skill_id,struct skill_unit *out_unit)
{
	int m,bx,by;
	struct block_list *bl;
	struct skill_unit *unit;

	nullpo_retr(0, target);
	m = target->m;

	if (x < 0 || y < 0 || (x >= map[m].xs) || (y >= map[m].ys))
		return NULL;
	bx = x/BLOCK_SIZE;
	by = y/BLOCK_SIZE;

	bl = map[m].block[bx+by*map[m].bxs];
	for(;bl;bl=bl->next){
		if (bl->x != x || bl->y != y || bl->type != BL_SKILL)
			continue;
		unit = (struct skill_unit *) bl;
		if (unit==out_unit || !unit->alive ||
				!unit->group || unit->group->skill_id!=skill_id)
			continue;
		if (battle_check_target(&unit->bl,target,unit->group->target_flag)>0)
			return unit;
	}
	return NULL;
}

/*============================================================
* For checking a path between two points (x0, y0) and (x1, y1)
*------------------------------------------------------------
 */
void map_foreachinpath(int (*func)(struct block_list*,va_list),int m,int x0,int y0,int x1,int y1,int type,...)
{

	va_list ap;
	int i, blockcount = bl_list_count;
	struct block_list *bl;

	//////////////////////////////////////////////////////////////
	// linear parametric equation
	// x=(x1-x0)*t+x0; y=(y1-y0)*t+y0; t=[0,1]
	//////////////////////////////////////////////////////////////
	// linear equation for finding a single line between (x0,y0)->(x1,y1)
	// independent of the given xy-values
	int dx = 0;
	int dy = 0;
	int bx=-1;	// initialize block coords to some impossible value
	int by=-1;

	int t;
	///////////////////////////////
	// find maximum runindex
	int tmax = abs(y1-y0);

	if(x0 != x1 && y0 != y1 && abs(x1-x0) != abs(y1-y0)) {
		printf("map_foreachinpath: not supported size\n");
		return;
	}
	if(tmax  < abs(x1-x0))
		tmax = abs(x1-x0);
	// pre-calculate delta values for x and y destination
	// should speed up cause you don't need to divide in the loop
	if(tmax>0)
	{
		dx = (x1-x0) / tmax;
		dy = (y1-y0) / tmax;
	}
	// go along the index
	for(t=0; t<=tmax; t++)
	{	// xy-values of the line including start and end point
		int x = dx * t + x0;
		int y = dy * t + y0;

		// check the block index of the calculated xy
		if( (bx!=x/BLOCK_SIZE) || (by!=y/BLOCK_SIZE) )
		{	// we have reached a new block
			// so we store the current block coordinates
			bx = x/BLOCK_SIZE;
			by = y/BLOCK_SIZE;
			if( bx < 0 || bx >= map[m].bxs ) continue;
			if( by < 0 || by >= map[m].bys ) continue;

			// and process the data

			if(type==0 || type&~BL_MOB) {
				bl = map[m].block[bx+by*map[m].bxs];		// a block with the elements
				for(;bl;bl=bl->next){		// go through all elements
					if( ( !type || bl->type&type ) && bl_list_count<BL_LIST_MAX )
					{
						// check if block xy is on the line
						if( (bl->x-x0)*(y1-y0) == (bl->y-y0)*(x1-x0) )
						// and if it is within start and end point
						if( (((x0<=x1)&&(x0<=bl->x)&&(bl->x<=x1)) || ((x0>=x1)&&(x0>=bl->x)&&(bl->x>=x1))) &&
							(((y0<=y1)&&(y0<=bl->y)&&(bl->y<=y1)) || ((y0>=y1)&&(y0>=bl->y)&&(bl->y>=y1))) )
							bl_list[bl_list_count++]=bl;
					}
				}//end for elements
			}

			if(type==0 || type&BL_MOB) {
				bl = map[m].block_mob[bx+by*map[m].bxs];	// and the mob block
				for(;bl;bl=bl->next){
					if(bl_list_count<BL_LIST_MAX) {
						// check if mob xy is on the line
						if( (bl->x-x0)*(y1-y0) == (bl->y-y0)*(x1-x0) )
						// and if it is within start and end point
						if( (((x0<=x1)&&(x0<=bl->x)&&(bl->x<=x1)) || ((x0>=x1)&&(x0>=bl->x)&&(bl->x>=x1))) &&
							(((y0<=y1)&&(y0<=bl->y)&&(bl->y<=y1)) || ((y0>=y1)&&(y0>=bl->y)&&(bl->y>=y1))) )
							bl_list[bl_list_count++]=bl;
					}
				}//end for mobs
			}
		}
	}//end for index

	if(bl_list_count>=BL_LIST_MAX) {
		if(battle_config.error_log)
			printf("map_foreachinpath: *WARNING* block count too many!\n");
	}

	va_start(ap,type);
	map_freeblock_lock();	// ����������̉�����֎~����

	for(i=blockcount;i<bl_list_count;i++)
		if(bl_list[i]->prev) {	// �L?���ǂ����`�F�b�N
			func(bl_list[i],ap);
		}

	map_freeblock_unlock();	// �����������
	va_end(ap);

	bl_list_count = blockcount;
}

/*==========================================
 * map m (x0,y0)-(x1,y1)���̑Sobj�ɑ΂���
 * func���Ă�
 * type!=0 �Ȃ炻�̎�ނ̂�
 *------------------------------------------
 */
void map_foreachinarea_sub(int (*func)(struct block_list*,va_list),int m,int x0,int y0,int x1,int y1,int type,va_list ap)
{
	int bx,by;
	struct block_list *bl;
	int blockcount=bl_list_count,i;

	if (x0 < 0) x0 = 0;
	if (y0 < 0) y0 = 0;
	if (x1 >= map[m].xs) x1 = map[m].xs-1;
	if (y1 >= map[m].ys) y1 = map[m].ys-1;

	if (type == 0 || type&~BL_MOB) {
		for (by = y0 / BLOCK_SIZE; by <= y1 / BLOCK_SIZE; by++) {
			for(bx=x0/BLOCK_SIZE;bx<=x1/BLOCK_SIZE;bx++){
				bl = map[m].block[bx+by*map[m].bxs];
				for(;bl;bl=bl->next){
					if(type && !(bl->type & type))
						continue;
					if(bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1 && bl_list_count<BL_LIST_MAX)
						bl_list[bl_list_count++]=bl;
				}
			}
		}
	}
	if (type == 0 || type&BL_MOB) {
		for(by=y0/BLOCK_SIZE;by<=y1/BLOCK_SIZE;by++){
			for(bx=x0/BLOCK_SIZE;bx<=x1/BLOCK_SIZE;bx++){
				bl = map[m].block_mob[bx+by*map[m].bxs];
				for(;bl;bl=bl->next){
					if(bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1 && bl_list_count<BL_LIST_MAX)
						bl_list[bl_list_count++]=bl;
				}
			}
		}
	}

	if(bl_list_count>=BL_LIST_MAX) {
		if(battle_config.error_log)
			printf("map_foreachinarea: *WARNING* block count too many!\n");
	}

	map_freeblock_lock();	// ����������̉�����֎~����

	for(i=blockcount;i<bl_list_count;i++)
		if(bl_list[i]->prev)	// �L�����ǂ����`�F�b�N
			func(bl_list[i],ap);

	map_freeblock_unlock();	// �����������

	bl_list_count = blockcount;

}

void map_foreachinarea(int (*func)(struct block_list*,va_list),int m,int x0,int y0,int x1,int y1,int type,...)
{
	va_list ap;

	va_start(ap,type);
	if (m < 0)
		printf("map_foreachinarea: invalid map index!! func = 0x%08x\n",(int)func);
	else
		map_foreachinarea_sub(func, m, x0, y0, x1, y1, type, ap);
	va_end(ap);
}

/*==========================================
 * ��`(x0,y0)-(x1,y1)��(dx,dy)�ړ���������
 * �̈�O�ɂȂ�̈�(��`��L���`)����obj��
 * �΂���func���Ă�
 *
 * dx,dy��-1,0,1�݂̂Ƃ���i�ǂ�Ȓl�ł��������ۂ��H�j
 *------------------------------------------
 */
void map_foreachinmovearea(int (*func)(struct block_list*,va_list),int m,int x0,int y0,int x1,int y1,int dx,int dy,int type,...)
{
	int bx,by,x2,y2,x3,y3;
	struct block_list *bl;
	va_list ap;
	int blockcount=bl_list_count,i;

	va_start(ap,type);
	if(dx==0 || dy==0){
		// ��`�̈�̏ꍇ
		if(dx==0){
			if(dy<0){
				y0=y1+dy+1;
			} else {
				y1=y0+dy-1;
			}
		} else if(dy==0){
			if(dx<0){
				x0=x1+dx+1;
			} else {
				x1=x0+dx-1;
			}
		}
		// if(x0<0) x0=0;
		// if(y0<0) y0=0;
		// if(x1>=map[m].xs) x1=map[m].xs-1;
		// if(y1>=map[m].ys) y1=map[m].ys-1;

		// �����ō��W�␳����ƁAmap ���̃I�u�W�F�N�g�ɑ΂��Ċ֐����������@�\���Ȃ��B
		// (-10, 0) - (10, 10) �� (10, 0) �ړ��������͈͊O�ɂȂ�̈�ƁA�␳���
		// (  0, 0) - (10, 10) �� (10, 0) �ړ��������͈͊O�ɂȂ�̈�͕ʕ��ɂȂ�B
		// �␳��� (0, 0) - (10, 10) �ɂ���I�u�W�F�N�g�ɑ΂��Ă��A���̊֐���
		// �Ăяo����邱�ƂɂȂ�B

		x2 = (x0<0 ? 0 : x0/BLOCK_SIZE);
		y2 = (y0<0 ? 0 : y0/BLOCK_SIZE);
		x3 = (x1>=map[m].xs ? map[m].bxs-1 : x1/BLOCK_SIZE);
		y3 = (y1>=map[m].ys ? map[m].bys-1 : y1/BLOCK_SIZE);
		for(by=y2;by<=y3;by++){
			for(bx=x2;bx<=x3;bx++){
				bl = map[m].block[bx+by*map[m].bxs];
				for(;bl;bl=bl->next){
					if(type && !(bl->type & type))
						continue;
					if(bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1 && bl_list_count<BL_LIST_MAX)
						bl_list[bl_list_count++]=bl;
				}
				bl = map[m].block_mob[bx+by*map[m].bxs];
				for(;bl;bl=bl->next){
					if(type && !(bl->type & type))
						continue;
					if(bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1 && bl_list_count<BL_LIST_MAX)
						bl_list[bl_list_count++]=bl;
				}
			}
		}
	}else{
		// L���̈�̏ꍇ

		// if(x0<0) x0=0;
		// if(y0<0) y0=0;
		// if(x1>=map[m].xs) x1=map[m].xs-1;
		// if(y1>=map[m].ys) y1=map[m].ys-1;

		// �����ō��W�␳����ƁAmap ���̃I�u�W�F�N�g�ɑ΂��Ċ֐����������@�\���Ȃ��B
		// (-10, -10) - (10, 10) �� (10, 10) �ړ��������͈͊O�ɂȂ�̈�ƁA�␳���
		// (  0,   0) - (10, 10) �� (10, 10) �ړ��������͈͊O�ɂȂ�̈�͕ʕ��ɂȂ�B
		// �␳��� (0, 0) - (10, 10) �ɂ���I�u�W�F�N�g�ɑ΂��Ă��A���̊֐���
		// �Ăяo����邱�ƂɂȂ�B

		x2 = (x0<0 ? 0 : x0/BLOCK_SIZE);
		y2 = (y0<0 ? 0 : y0/BLOCK_SIZE);
		x3 = (x1>=map[m].xs ? map[m].bxs-1 : x1/BLOCK_SIZE);
		y3 = (y1>=map[m].ys ? map[m].bys-1 : y1/BLOCK_SIZE);
		for(by=y2;by<=y3;by++){
			for(bx=x2;bx<=x3;bx++){
				bl = map[m].block[bx+by*map[m].bxs];
				for(;bl;bl=bl->next){
					if(type && !(bl->type & type))
						continue;
					if(!(bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1))
						continue;
					if(((dx>0 && bl->x<x0+dx) || (dx<0 && bl->x>x1+dx) ||
						(dy>0 && bl->y<y0+dy) || (dy<0 && bl->y>y1+dy)) &&
						bl_list_count<BL_LIST_MAX)
							bl_list[bl_list_count++]=bl;
				}
				bl = map[m].block_mob[bx+by*map[m].bxs];
				for(;bl;bl=bl->next){
					if(type && !(bl->type & type))
						continue;
					if(!(bl->x>=x0 && bl->x<=x1 && bl->y>=y0 && bl->y<=y1))
						continue;
					if(((dx>0 && bl->x<x0+dx) || (dx<0 && bl->x>x1+dx) ||
						(dy>0 && bl->y<y0+dy) || (dy<0 && bl->y>y1+dy)) &&
						bl_list_count<BL_LIST_MAX)
							bl_list[bl_list_count++]=bl;
				}
			}
		}

	}

	if(bl_list_count>=BL_LIST_MAX) {
		if(battle_config.error_log)
			printf("map_foreachinmovearea: *WARNING* block count too many!\n");
	}

	map_freeblock_lock();	// ����������̉�����֎~����

	for(i=blockcount;i<bl_list_count;i++)
		if(bl_list[i]->prev)	// �L�����ǂ����`�F�b�N
			func(bl_list[i],ap);

	map_freeblock_unlock();	// �����������

	va_end(ap);
	bl_list_count = blockcount;
}


/*==========================================
 * ��`(x[0],y[0])-(x[1],y[1])�Ƌ�`(x[2],y[2])-(x[3],y[3])��
 * ���ʕ����ɑ΂���func���ĂԁB�������Ax[0] < x[1], y[0] < y[1],
 * x[2] < x[3], y[2] < y[3]�����肵�Ă���
 *------------------------------------------
 */

// �����`�̒��ɓ_(x,y) ���܂܂�Ă��邩�𒲂ׂ�
#define map_square_in(p, _x, _y) (x[p+0] <= (_x) && x[p+1] >= (_x) && y[p+0] <= (_y) && y[p+1] >= (_y))

void map_foreachcommonarea(int (*func)(struct block_list*,va_list),int m,int x[4],int y[4],int type,...) {
	int flag = 0, i, j;
	int x0 = 9999, x1 = -9999;
	int y0 = 9999, y1 = -9999;
	va_list ap;
	va_start(ap,type);

	for(i = 0; i < 4; i++) {
		for(j = 0; j < 4; j++) {
			if( map_square_in( 0, x[i], y[j] ) && map_square_in( 2, x[i], y[j] ) ) {
				if( x0 > x[i]) { x0 = x[i]; flag = 1; }
				if( x1 < x[i]) { x1 = x[i]; flag = 1; }
				if( y0 > y[j]) { y0 = y[j]; flag = 1; }
				if( y1 < y[j]) { y1 = y[j]; flag = 1; }
			}
		}
	}
	if( flag ) {
		map_foreachinarea_sub(func, m, x0, y0, x1, y1, type, ap);
	}
	va_end(ap);
}

/*==========================================
 * ���A�C�e����G�t�F�N�g�p�̈ꎞobj���蓖��
 * object[]�ւ̕ۑ���id_db�o�^�܂�
 *
 * bl->id�����̒��Őݒ肵�Ė�薳��?
 *------------------------------------------
 */
int map_addobject(struct block_list *bl)
{
	int i;
	if(bl == NULL) {
		printf("map_addobject nullpo?\n");
		return 0;
	}
	if(first_free_object_id < MIN_FLOORITEM || first_free_object_id >= MAX_FLOORITEM)
		first_free_object_id = MIN_FLOORITEM;
	for(i = first_free_object_id; i < MAX_FLOORITEM; i++)
		if(object[i] == NULL)
			break;
	if(i >= MAX_FLOORITEM) {
		if(battle_config.error_log)
			printf("no free object id\n");
		return 0;
	}
	first_free_object_id = i;
	if(last_object_id < i)
		last_object_id = i;
	object[i] = bl;
	//numdb_insert(id_db,i,bl);
	//numdb_insert(address_db,bl,1);
	return i;
}

/*==========================================
 * �ꎞobject�̉��
 *	map_delobject��free���Ȃ��o�[�W����
 *------------------------------------------
 */
int map_delobjectnofree(int id)
{
	if(object[id] == NULL)
		return 0;

	map_delblock(object[id]);
	//numdb_erase(id_db,id);
	//numdb_erase(address_db,object[id]);
	object[id] = NULL;

	if(first_free_object_id > id)
		first_free_object_id = id;

	while(last_object_id > MIN_FLOORITEM && object[last_object_id] == NULL)
		last_object_id--;

	return 0;
}

/*==========================================
 * �ꎞobject�̉��
 * block_list����̍폜�Aid_db����̍폜
 * object data��free�Aobject[]�ւ�NULL���
 *
 * add�Ƃ̑Ώ̐��������̂��C�ɂȂ�
 *------------------------------------------
 */
int map_delobject(int id)
{
	struct block_list *obj = object[id];

	if(obj == NULL)
		return 0;

	map_delobjectnofree(id);
	map_freeblock(obj);

	return 0;
}

/*==========================================
 * �S�ꎞobj�����func���Ă�
 *------------------------------------------
 */
void map_foreachobject(int (*func)(struct block_list*,va_list),int type,...)
{
	int i;
	int blockcount = bl_list_count;
	va_list ap;

	va_start(ap,type);

	for(i = MIN_FLOORITEM; i <= last_object_id; i++){
		if(object[i]) {
			if(type && !(object[i]->type & type))
				continue;
			if(bl_list_count >= BL_LIST_MAX) {
				if(battle_config.error_log)
					printf("map_foreachobject: too many block !\n");
			} else {
				bl_list[bl_list_count++] = object[i];
			}
		}
	}

	map_freeblock_lock();

	for(i = blockcount; i < bl_list_count; i++)
		if( bl_list[i]->prev || bl_list[i]->next )
			func(bl_list[i],ap);

	map_freeblock_unlock();

	va_end(ap);
	bl_list_count = blockcount;
}

/*==========================================
 * ���A�C�e��������
 *
 * data==0�̎���timer�ŏ�������
 * data!=0�̎��͏E�����ŏ��������Ƃ��ē���
 *
 * ��҂́Amap_clearflooritem(id)��
 * map.h����#define���Ă���
 *------------------------------------------
 */
int map_clearflooritem_timer(int tid,unsigned int tick,int id,int data)
{
	struct block_list *bl = object[id];
	struct flooritem_data *fitem = NULL;

	if(bl && bl->type == BL_ITEM)
		fitem = (struct flooritem_data *)bl;

	if(fitem == NULL || (!data && fitem->cleartimer != tid)){
		if(battle_config.error_log)
			printf("map_clearflooritem_timer : error\n");
		return 1;
	}
	if(data)
		delete_timer(fitem->cleartimer,map_clearflooritem_timer);
	else if(fitem->item_data.card[0] == (short)0xff00)
		intif_delete_petdata(*((long *)(&fitem->item_data.card[1])));
	clif_clearflooritem(fitem,0);
	map_delobject(fitem->bl.id);

	return 0;
}

/*==========================================
 * (m,x,y)�̎���range�}�X���̋�(=�N���\)cell��
 * ������K���ȃ}�X�ڂ̍��W��x+(y<<16)�ŕԂ�
 *
 * ����range=1�ŃA�C�e���h���b�v�p�r�̂�
 *------------------------------------------
 */
int map_searchrandfreecell(int m,int x,int y,int range)
{
	int free_cell,i,j;

	for(free_cell=0,i=-range;i<=range;i++){
		if(i+y<0 || i+y>=map[m].ys)
			continue;
		for(j=-range;j<=range;j++){
			if(j+x<0 || j+x>=map[m].xs)
				continue;
			if(map_getcell(m,j+x,i+y,CELL_CHKNOPASS))
				continue;
			free_cell++;
		}
	}
	if(free_cell==0)
		return -1;
	free_cell=atn_rand()%free_cell;
	for(i=-range;i<=range;i++){
		if(i+y<0 || i+y>=map[m].ys)
			continue;
		for(j=-range;j<=range;j++){
			if(j+x<0 || j+x>=map[m].xs)
				continue;
			if(map_getcell(m,j+x,i+y,CELL_CHKNOPASS))
				continue;
			if(free_cell==0){
				x+=j;
				y+=i;
				i=range+1;
				break;
			}
			free_cell--;
		}
	}

	return x+(y<<16);
}

/*==========================================
 * (m,x,y)�𒆐S��3x3�ȓ��ɏ��A�C�e���ݒu
 *
 * item_data��amount�ȊO��copy����
 *------------------------------------------
 */
int map_addflooritem(struct item *item_data,int amount,int m,int x,int y,struct block_list *first_bl,
	struct block_list *second_bl,struct block_list *third_bl,int type)
{
	int xy,r;
	unsigned int tick;
	struct flooritem_data *fitem;

	nullpo_retr(0, item_data);

	if((xy=map_searchrandfreecell(m,x,y,1))<0)
		return 0;
	r=atn_rand();

	fitem = (struct flooritem_data *)aCalloc(1,sizeof(*fitem));
	fitem->bl.type=BL_ITEM;
	fitem->bl.prev = fitem->bl.next = NULL;
	fitem->bl.m=m;
	fitem->bl.x=xy&0xffff;
	fitem->bl.y=(xy>>16)&0xffff;
	fitem->first_get_id = 0;
	fitem->first_get_tick = 0;
	fitem->second_get_id = 0;
	fitem->second_get_tick = 0;
	fitem->third_get_id = 0;
	fitem->third_get_tick = 0;

	fitem->bl.id = map_addobject(&fitem->bl);
	if(fitem->bl.id==0){
		aFree(fitem);
		return 0;
	}

	tick = gettick();
	if(first_bl) {
		fitem->first_get_id = first_bl->id;
		if(type)
			fitem->first_get_tick = tick + battle_config.mvp_item_first_get_time;
		else
			fitem->first_get_tick = tick + battle_config.item_first_get_time;
	}
	if(second_bl) {
		fitem->second_get_id = second_bl->id;
		if(type)
			fitem->second_get_tick = tick + battle_config.mvp_item_first_get_time + battle_config.mvp_item_second_get_time;
		else
			fitem->second_get_tick = tick + battle_config.item_first_get_time + battle_config.item_second_get_time;
	}
	if(third_bl) {
		fitem->third_get_id = third_bl->id;
		if(type)
			fitem->third_get_tick = tick + battle_config.mvp_item_first_get_time + battle_config.mvp_item_second_get_time + battle_config.mvp_item_third_get_time;
		else
			fitem->third_get_tick = tick + battle_config.item_first_get_time + battle_config.item_second_get_time + battle_config.item_third_get_time;
	}

	memcpy(&fitem->item_data,item_data,sizeof(*item_data));
	fitem->item_data.amount=amount;
	fitem->subx=(r&3)*3+3;
	fitem->suby=((r>>2)&3)*3+3;
	fitem->cleartimer=add_timer(gettick()+battle_config.flooritem_lifetime,map_clearflooritem_timer,fitem->bl.id,0);

	map_addblock(&fitem->bl);
	clif_dropflooritem(fitem);

	return fitem->bl.id;
}

/*==========================================
 * charid_db�̃L�����f�[�^������
 *------------------------------------------
 */
struct charid2nick *char_search(int char_id)
{
	struct charid2nick *p;

	p = (struct charid2nick *)numdb_search(charid_db,char_id);
	if(p==NULL){	// �f�[�^�x�[�X�ɂȂ�
		chrif_searchcharid(char_id);
		return NULL;
	}

	return p;
}

/*==========================================
 * charid_db�֒ǉ�(�ԐM�҂�������ΕԐM)
 *------------------------------------------
 */
void map_addchariddb(int charid, char *name, int account_id, unsigned long ip, int port)
{
	struct charid2nick *p;
	int req = 0;

	if(account_id <= 0)
		return;

	p = (struct charid2nick *)numdb_search(charid_db,charid);
	if(p == NULL) {	// �f�[�^�x�[�X�ɂȂ�
		p = (struct charid2nick *)aCalloc(1,sizeof(struct charid2nick));
		p->req_id = 0;
	} else {
		numdb_erase(charid_db,charid);
	}

	req = p->req_id;
	memcpy(p->nick, name, 24);
	p->account_id = account_id;
	p->ip         = ip;
	p->port       = port;
	p->req_id     = 0;
	numdb_insert(charid_db,charid,p);

	if(req) {	// �ԐM�҂�������ΕԐM
		struct map_session_data *sd = map_id2sd(req);
		if(sd != NULL)
			clif_solved_charname(sd,charid);
	}
	//printf("map add chariddb:%s\n",p->nick);
	return;
}
/*==========================================
 * charid_db����폜
 *------------------------------------------
 */
void map_delchariddb(int charid)
{
	struct charid2nick *p;
	p = (struct charid2nick *)numdb_search(charid_db,charid);
	if(p){	// �f�[�^�x�[�X�ɂ�����
		p->ip=0;	//���ۂɍ폜����ƕ���̖��O�Ƃ����Ȃ��Ȃ�̂�map-server��IP��Port�����폜
		p->port=0;
//		printf("map delete chariddb:%s\n",p->nick);
	}//else
//		printf("map delete chariddb:notfound %d\n",charid);

	return;
}
/*==========================================
 * charid_db�֒ǉ��i�ԐM�v���̂݁j
 *------------------------------------------
 */
void map_reqchariddb(struct map_session_data * sd, int charid)
{
	struct charid2nick *p;

	nullpo_retv(sd);

	p = (struct charid2nick *)numdb_search(charid_db,charid);
	if(p!=NULL)	// �f�[�^�x�[�X�ɂ��łɂ���
		return;

	p = (struct charid2nick *)aCalloc(1,sizeof(struct charid2nick));
	p->req_id=sd->bl.id;
	numdb_insert(charid_db,charid,p);

	return;
}

/*==========================================
 * id_db��bl��ǉ�
 *------------------------------------------
 */
void map_addiddb(struct block_list *bl)
{
	nullpo_retv(bl);

	numdb_insert(id_db,bl->id,bl);
	//numdb_insert(address_db,bl,1);
}

/*==========================================
 * id_db����bl���폜
 *------------------------------------------
 */
void map_deliddb(struct block_list *bl)
{
	nullpo_retv(bl);

	numdb_erase(id_db,bl->id);
	//numdb_erase(address_db,bl);
}

/*==========================================
 * nick_db��sd��ǉ�
 *------------------------------------------
 */
void map_addnickdb(struct map_session_data *sd)
{
	nullpo_retv(sd);

	strdb_insert(nick_db,sd->status.name,sd);
}

/*==========================================
 * PC��quit���� map.c����
 *
 * quit�����̎�̂��Ⴄ�悤�ȋC�����Ă���
 *------------------------------------------
 */
int map_quit(struct map_session_data *sd)
{
	struct charid2nick *p;

	nullpo_retr(0, sd);

	if(!sd->state.waitingdisconnect && sd->new_fd != -1) {
		if( sd->pd ) {
			pet_lootitem_drop(sd->pd,sd);
			unit_free( &sd->pd->bl, 0);
		}
		if( sd->hd ) {
			unit_free( &sd->hd->bl, 0);
		}
		unit_free(&sd->bl, 0);
		chrif_save(sd);
	}

	if( sd->stack ) {
		script_free_stack( sd->stack );
	}

	// �Q�d���O�C�����A��Ƀ��O�C�������L������id_db�͍폜���Ȃ�
	if(sd->new_fd != -1)
		numdb_erase(id_db,sd->bl.id);

	strdb_erase(nick_db,sd->status.name);
	p = (struct charid2nick *)numdb_search(charid_db,sd->status.char_id);
	if(p) {
		p->ip   = 0;
		p->port = 0;
	}
	aFree(sd->reg);
	aFree(sd->regstr);
//printf("map quit:%s\n",sd->status.name);

	return 0;
}

/*==========================================
 * id�ԍ���PC��T���B���Ȃ����NULL
 *------------------------------------------
 */
struct map_session_data * map_id2sd(int id)
{
	struct block_list *bl;

	if(id > 0) {
		bl = (struct block_list *)numdb_search(id_db,id);
		if(bl && bl->type == BL_PC)
			return (struct map_session_data *)bl;
	}
	return NULL;
}

/*==========================================
 * id�ԍ���MOB��T���B���Ȃ����NULL
 *------------------------------------------
 */
struct mob_data * map_id2md(int id)
{
	struct block_list *bl;

	if(id > 0) {
		bl = (struct block_list *)numdb_search(id_db,id);
		if(bl && bl->type == BL_MOB)
			return (struct mob_data *)bl;
	}
	return NULL;
}

/*==========================================
 * id�ԍ���HOM��T���B���Ȃ����NULL
 *------------------------------------------
 */
struct homun_data * map_id2hd(int id)
{
	struct block_list *bl;

	if(id > 0) {
		bl = (struct block_list *)numdb_search(id_db,id);
		if(bl && bl->type == BL_HOM)
			return (struct homun_data *)bl;
	}
	return NULL;
}

/*==========================================
 * id�ԍ���NPC��T���B���Ȃ����NULL
 *------------------------------------------
 */
struct npc_data * map_id2nd(int id)
{
	struct block_list *bl;

	if(id > 0) {
		bl = (struct block_list *)numdb_search(id_db,id);
		if(bl && bl->type == BL_NPC)
			return (struct npc_data *)bl;
	}
	return NULL;
}

/*==========================================
 * id�ԍ���CHAT��T���B���Ȃ����NULL
 *------------------------------------------
 */
struct chat_data * map_id2cd(int id)
{
	struct block_list *bl;

	// �`���b�g�͈ꎞobject
	if(id > 0 && id < MAX_FLOORITEM) {
		bl = object[id];
		if(bl && bl->type == BL_CHAT) {
			if(id != bl->id)
				printf("map_id2cd : block id mismatch !!\n");
			else
				return (struct chat_data *)bl;
		}
	}
	return NULL;
}

/*==========================================
 * id�ԍ���SKILL��T���B���Ȃ����NULL
 *------------------------------------------
 */
struct skill_unit * map_id2su(int id)
{
	struct block_list *bl;

	// �X�L�����j�b�g�͈ꎞobject
	if(id > 0 && id < MAX_FLOORITEM) {
		bl = object[id];
		if(bl && bl->type == BL_SKILL) {
			if(id != bl->id)
				printf("map_id2su : block id mismatch !!\n");
			else
				return (struct skill_unit *)bl;
		}
	}
	return NULL;
}

/*==========================================
 * id�ԍ��̕���T��
 * �ꎞobject�̏ꍇ�͔z��������̂�
 *------------------------------------------
 */
struct block_list * map_id2bl(int id)
{
	if(id > 0) {
		if(id < MAX_FLOORITEM)
			return object[id];
		else
			return (struct block_list *)numdb_search(id_db,id);
	}
	return NULL;
}

/*==========================================
 * char_id�ԍ��̖��O��T��
 *------------------------------------------
 */
char * map_charid2nick(int id)
{
	struct charid2nick *p = (struct charid2nick *)numdb_search(charid_db,id);
	if(p==NULL)
		return NULL;
	if(p->req_id!=0)
		return NULL;
	return p->nick;
}

/*==========================================
 * ���O��nick��PC��T���B���Ȃ����NULL
 *------------------------------------------
 */
struct map_session_data * map_nick2sd(char *nick)
{
	if(nick == NULL)
		return NULL;
	return (struct map_session_data *)strdb_search(nick_db,nick);
}

/*==========================================
 * id_db���̑S�Ă�func�����s
 *------------------------------------------
 */
int map_foreachiddb(int (*func)(void*,void*,va_list),...)
{
	va_list ap;

	va_start(ap,func);
	db_foreach_sub(id_db,func,ap);
	va_end(ap);
	return 0;
}

/*==========================================
 * map.npc�֒ǉ� (warp���̗̈掝���̂�)
 *------------------------------------------
 */
int map_addnpc(int m,struct npc_data *nd)
{
	int i;
	if(m<0 || m>=map_num)
		return -1;
	for(i=0;i<map[m].npc_num && i<MAX_NPC_PER_MAP;i++)
		if(map[m].npc[i]==NULL)
			break;
	if(i==MAX_NPC_PER_MAP){
		if(battle_config.error_log)
			printf("too many NPCs in one map %s\n",map[m].name);
		return -1;
	}
	if(i==map[m].npc_num){
		map[m].npc_num++;
	}

	nullpo_retr(0, nd);

	map[m].npc[i]=nd;
	numdb_insert(id_db,nd->bl.id,nd);

	return i;
}

/*==========================================
 * map������map�ԍ��֕ϊ�
 *------------------------------------------
 */
int map_mapname2mapid(char *name)
{
	struct map_data *md;

	md = (struct map_data *)strdb_search(map_db,name);
	if(md==NULL || md->gat==NULL)
		return -1;
	return md->m;
}

/*==========================================
 * ���Imap������ip,port�ϊ�
 *------------------------------------------
 */
int map_mapname2ipport(char *name,int *ip,int *port)
{
	struct map_data_other_server *mdos;

	mdos = (struct map_data_other_server *)strdb_search(map_db,name);
	if(mdos==NULL || mdos->gat)
		return -1;
	*ip=mdos->ip;
	*port=mdos->port;
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int map_check_dir(int s_dir,int t_dir)
{
	if(s_dir == t_dir)
		return 0;
	switch(s_dir) {
		case 0:
			if(t_dir == 7 || t_dir == 1 || t_dir == 0)
				return 0;
			break;
		case 1:
			if(t_dir == 0 || t_dir == 2 || t_dir == 1)
				return 0;
			break;
		case 2:
			if(t_dir == 1 || t_dir == 3 || t_dir == 2)
				return 0;
			break;
		case 3:
			if(t_dir == 2 || t_dir == 4 || t_dir == 3)
				return 0;
			break;
		case 4:
			if(t_dir == 3 || t_dir == 5 || t_dir == 4)
				return 0;
			break;
		case 5:
			if(t_dir == 4 || t_dir == 6 || t_dir == 5)
				return 0;
			break;
		case 6:
			if(t_dir == 5 || t_dir == 7 || t_dir == 6)
				return 0;
			break;
		case 7:
			if(t_dir == 6 || t_dir == 0 || t_dir == 7)
				return 0;
			break;
	}
	return 1;
}

/*==========================================
 * �މ�̕������v�Z
 *------------------------------------------
 */
int map_calc_dir( struct block_list *src,int x,int y)
{
	int dir=0;
	int dx,dy;

	nullpo_retr(0, src);

	dx=x-src->x;
	dy=y-src->y;
	if( dx==0 && dy==0 ){	// �މ�̏ꏊ��v
		dir=0;	// ��
	}else if( dx>=0 && dy>=0 ){	// �����I�ɉE��
		dir=7;						// �E��
		if( dx*3-1<dy ) dir=0;		// ��
		if( dx>dy*3 ) dir=6;		// �E
	}else if( dx>=0 && dy<=0 ){	// �����I�ɉE��
		dir=5;						// �E��
		if( dx*3-1<-dy ) dir=4;		// ��
		if( dx>-dy*3 ) dir=6;		// �E
	}else if( dx<=0 && dy<=0 ){ // �����I�ɍ���
		dir=3;						// ����
		if( dx*3+1>dy ) dir=4;		// ��
		if( dx<dy*3 ) dir=2;		// ��
	}else{						// �����I�ɍ���
		dir=1;						// ����
		if( -dx*3-1<dy ) dir=0;		// ��
		if( -dx>dy*3 ) dir=2;		// ��
	}
	return dir;
}

// gat�n
/*==========================================
 * (m,x,y)�̏�Ԃ𒲂ׂ�
 *------------------------------------------
 */
int map_getcell(int m,int x,int y,cell_t cellchk)
{
	return (m < 0 || m >= map_num) ? 0 : map_getcellp(&map[m], x, y, cellchk);
}

int map_getcellp(struct map_data* m,int x,int y,cell_t cellchk)
{
	int type;
	nullpo_ret(m);

	if(x<0 || x>=m->xs-1 || y<0 || y>=m->ys-1)
	{
		if(cellchk==CELL_CHKNOPASS) return 1;
		return 0;
	}
	type = m->gat[x+y*m->xs];
	if (cellchk<0x10)
		type &= CELL_MASK;

	switch(cellchk)
	{
		case CELL_CHKPASS:
			return (type!=1 && type!=5);
		case CELL_CHKNOPASS:
			return (type==1 || type==5);
		case CELL_CHKWALL:
			return (type==1);
		case CELL_CHKWATER:
			return (type==3);
		case CELL_CHKGROUND:
			return (type==5);
		case CELL_GETTYPE:
			return type;
		case CELL_CHKNPC:
			return (type&CELL_NPC);
		case CELL_CHKBASILICA:
			return (type&CELL_BASILICA);
		default:
			return 0;
	}
}

/*==========================================
 * (m,x,y)�̏�Ԃ�ݒ肷��
 *------------------------------------------
 */
void map_setcell(int m,int x,int y,int cell)
{
	int j;
	if(x<0 || x>=map[m].xs || y<0 || y>=map[m].ys)
		return;
	j=x+y*map[m].xs;

	switch (cell) {
		case CELL_SETNPC:
			map[m].gat[j] |= CELL_NPC;
			break;
		case CELL_SETBASILICA:
			map[m].gat[j] |= CELL_BASILICA;
			break;
		case CELL_CLRBASILICA:
			map[m].gat[j] &= ~CELL_BASILICA;
			break;
		default:
			map[m].gat[j] = (map[m].gat[j]&~CELL_MASK) + cell;
			break;
	}
}

/*==========================================
 * ���I�Ǘ��̃}�b�v��db�ɒǉ�
 *------------------------------------------
 */
int map_setipport(char *name,unsigned long ip,int port)
{
	struct map_data *md;
	struct map_data_other_server *mdos;

	md = (struct map_data *)strdb_search(map_db,name);
	if(md==NULL){
		// ���݂��Ȃ��f�[�^
		mdos=(struct map_data_other_server *)aCalloc(1,sizeof(struct map_data_other_server));
		memcpy(mdos->name,name,24);
		mdos->gat  = NULL;
		mdos->ip   = ip;
		mdos->port = port;
		mdos->map  = NULL;
		strdb_insert(map_db,mdos->name,mdos);
	} else if(md->gat){
		if(ip!=clif_getip() || port!=clif_getport()){
			// �ǂݍ���ł������ǁA�S���O�ɂȂ����}�b�v
			mdos=(struct map_data_other_server *)aCalloc(1,sizeof(struct map_data_other_server));
			memcpy(mdos->name,name,24);
			mdos->gat  = NULL;
			mdos->ip   = ip;
			mdos->port = port;
			mdos->map  = md;
			strdb_insert(map_db,mdos->name,mdos);
			// printf("from char server : %s -> %08lx:%d\n",name,ip,port);
		} else {
			// �ǂݍ���ł��āA�S���ɂȂ����}�b�v�i�������Ȃ��j
			;
		}
	} else {
		mdos=(struct map_data_other_server *)md;
		if(ip == clif_getip() && port == clif_getport()) {
			// �����̒S���ɂȂ����}�b�v
			if(mdos->map == NULL) {
				// �ǂݍ���ł��Ȃ��̂ŏI������
				printf("map_setipport : %s is not loaded.\n",name);
				exit(1);
			} else {
				// �ǂݍ���ł���̂Œu��������
				md = mdos->map;
				strdb_insert(map_db,md->name,md);
				aFree(mdos);
			}
		} else {
			// ���̎I�̒S���}�b�v�Ȃ̂Œu�������邾��
			mdos->ip   = ip;
			mdos->port = port;
		}
	}
	return 0;
}

/*==========================================
 * ���I�Ǘ��̃}�b�v��S�č폜
 *------------------------------------------
 */
int map_eraseallipport_sub(void *key,void *data,va_list va) {
	struct map_data_other_server *mdos = (struct map_data_other_server*)data;
	if(mdos->gat == NULL && mdos->map == NULL) {
		strdb_erase(map_db,key);
		aFree(mdos);
	}
	return 0;
}

void map_eraseallipport(void) {
	strdb_foreach(map_db,map_eraseallipport_sub);
}

/*==========================================
 * ���I�Ǘ��̃}�b�v��db����폜
 *------------------------------------------
 */
int map_eraseipport(char *name,unsigned long ip,int port)
{
	struct map_data *md;
	struct map_data_other_server *mdos;
//	unsigned char *p=(unsigned char *)&ip;

	md = (struct map_data *)strdb_search(map_db,name);
	if(md){
		if(md->gat) // local -> check data
			return 0;
		else {
			mdos=(struct map_data_other_server *)md;
			if(mdos->ip==ip && mdos->port == port) {
				if(mdos->map) {
					// ���̃}�b�v�I�ł��ǂݍ���ł���̂ňړ��ł���
					return 1; // �Ăяo������ chrif_sendmap() ������
				} else {
					strdb_erase(map_db,name);
					aFree(mdos);
				}
//				if(battle_config.etc_log)
//					printf("erase map %s %d.%d.%d.%d:%d\n",name,p[0],p[1],p[2],p[3],port);
			}
		}
	}
	return 0;
}

// ����������
/*==========================================
 * ���ꍂ���ݒ�
 *------------------------------------------
 */
static struct waterlist {
	char mapname[16];
	int waterheight;
} *waterlist = NULL;
static int waterlist_num = 0;

#define NO_WATER 1000000

static int map_waterheight(const char *mapname)
{
	int i;

	for(i = 0; i < waterlist_num; i++)
		if (strncmp(waterlist[i].mapname, mapname, 16) == 0)
			return waterlist[i].waterheight;

	return NO_WATER;
}

static void map_readwater(const char *watertxt)
{
	char line[1024], w1[1024];
	FILE *fp;

	fp = fopen(watertxt, "r");
	if (fp == NULL) {
		printf("file not found: %s\n", watertxt);
		return;
	}

	while(fgets(line, sizeof(line), fp)) {
		int wh, count;
		if(line[0] == '/' && line[1] == '/')
			continue;
		if ((count = sscanf(line, "%s%d", w1, &wh)) < 1) {
			continue;
		}

		if (map_waterheight(w1) != NO_WATER) {
			printf("Error: Duplicated map [%s] in waterlist (file '%s').\n", w1, watertxt);
		} else {
			if (waterlist_num == 0) {
				waterlist = (struct waterlist *)aCalloc(1, sizeof(struct waterlist));
			} else {
				waterlist = (struct waterlist *)aRealloc(waterlist, sizeof(struct waterlist) * (waterlist_num + 1));
				//memset(waterlist + waterlist_num, 0, sizeof(struct waterlist));
			}
			memset(waterlist[waterlist_num].mapname, 0, sizeof(waterlist[waterlist_num].mapname));
			strncpy(waterlist[waterlist_num].mapname, w1, 16);
			if (count >= 2)
				waterlist[waterlist_num].waterheight = wh;
			else
				waterlist[waterlist_num].waterheight = 3;
			waterlist_num++;
		}
	}
	fclose(fp);

	printf("File '%s' readed ('%d' entrie%s).\n", watertxt, waterlist_num, (waterlist_num > 1) ? "s" : "");

	return;
}

/*==========================================
* �}�b�v�L���b�V���ɒǉ�����
*===========================================*/

// �}�b�v�L���b�V���̍ő�l
#define MAX_MAP_CACHE 768

//�e�}�b�v���Ƃ̍ŏ�������������́AREAD_FROM_BITMAP�p
struct map_cache_info {
	char fn[32];//�t�@�C����
	int xs,ys; //���ƍ���
	int water_height;
	int pos;  // �f�[�^������Ă���ꏊ
	int compressed;     // zilb�ʂ���悤�ɂ���ׂ̗\��
	int compressed_len; // zilb�ʂ���悤�ɂ���ׂ̗\��
}; // 56 byte

struct {
	struct map_cache_head {
		int sizeof_header;
		int sizeof_map;
		// ��̂Q���ϕs��
		int nmaps; // �}�b�v�̌�
		int filesize;
	} head;
	struct map_cache_info *map;
	FILE *fp;
	int dirty;
} map_cache;

static int map_cache_open(char *fn);
static void map_cache_close(void);
static int map_cache_read(struct map_data *m);
static int map_cache_write(struct map_data *m);

static int map_cache_open(char *fn)
{
	atexit(map_cache_close);
	if(map_cache.fp) {
		map_cache_close();
	}
	map_cache.fp = fopen(fn,"r+b");
	if(map_cache.fp) {
		fread(&map_cache.head,1,sizeof(struct map_cache_head),map_cache.fp);
		fseek(map_cache.fp,0,SEEK_END);
		if(
			map_cache.head.sizeof_header == sizeof(struct map_cache_head) &&
			map_cache.head.sizeof_map    == sizeof(struct map_cache_info) &&
			map_cache.head.filesize      == ftell(map_cache.fp)
		) {
			// �L���b�V���ǂݍ��ݐ���
			map_cache.map = (struct map_cache_info *)aMalloc(sizeof(struct map_cache_info) * map_cache.head.nmaps);
			fseek(map_cache.fp,sizeof(struct map_cache_head),SEEK_SET);
			fread(map_cache.map,sizeof(struct map_cache_info),map_cache.head.nmaps,map_cache.fp);
			return 1;
		}
		fclose(map_cache.fp);
	}
	// �ǂݍ��݂Ɏ��s�����̂ŐV�K�ɍ쐬����
	map_cache.fp = fopen(fn,"wb");
	if(map_cache.fp) {
		memset(&map_cache.head,0,sizeof(struct map_cache_head));
		map_cache.map = (struct map_cache_info *)aCalloc(sizeof(struct map_cache_info),MAX_MAP_CACHE);
		map_cache.head.nmaps         = MAX_MAP_CACHE;
		map_cache.head.sizeof_header = sizeof(struct map_cache_head);
		map_cache.head.sizeof_map    = sizeof(struct map_cache_info);

		map_cache.head.filesize  = sizeof(struct map_cache_head);
		map_cache.head.filesize += sizeof(struct map_cache_info) * map_cache.head.nmaps;

		map_cache.dirty = 1;
		return 1;
	}
	return 0;
}

static void map_cache_close(void)
{
	if(!map_cache.fp) { return; }
	if(map_cache.dirty) {
		fseek(map_cache.fp,0,SEEK_SET);
		fwrite(&map_cache.head,1,sizeof(struct map_cache_head),map_cache.fp);
		fwrite(map_cache.map,map_cache.head.nmaps,sizeof(struct map_cache_info),map_cache.fp);
	}
	fclose(map_cache.fp);
	aFree(map_cache.map);
	map_cache.fp = NULL;
	return;
}

int map_cache_read(struct map_data *m)
{
	int i;
	if(!map_cache.fp) { return 0; }
	for(i = 0;i < map_cache.head.nmaps ; i++) {
		if(!strcmp(m->name,map_cache.map[i].fn)) {
			if(map_cache.map[i].water_height != map_waterheight(m->name)) {
				// ����̍������Ⴄ�̂œǂݒ���
				return 0;
			} else if(map_cache.map[i].compressed == 0) {
				// �񈳏k�t�@�C��
				int size = map_cache.map[i].xs * map_cache.map[i].ys;
				m->xs = map_cache.map[i].xs;
				m->ys = map_cache.map[i].ys;
				m->gat = (unsigned char *)aCalloc(m->xs * m->ys,sizeof(unsigned char));
				fseek(map_cache.fp,map_cache.map[i].pos,SEEK_SET);
				if(fread(m->gat,1,size,map_cache.fp) == size) {
					// ����
					return 1;
				} else {
					// �Ȃ����t�@�C���㔼�������Ă�̂œǂݒ���
					m->xs = 0;
					m->ys = 0;
					aFree(m->gat);
					m->gat = NULL;
					return 0;
				}
			} else if(map_cache.map[i].compressed == 1) {
				// ���k�t���O=1 : zlib
				unsigned char *buf;
				unsigned long dest_len;
				int size_compress = map_cache.map[i].compressed_len;
				m->xs = map_cache.map[i].xs;
				m->ys = map_cache.map[i].ys;
				m->gat = (unsigned char *)aMalloc(m->xs * m->ys * sizeof(unsigned char));
				buf = (unsigned char*)aMalloc(size_compress);
				fseek(map_cache.fp,map_cache.map[i].pos,SEEK_SET);
				if(fread(buf,1,size_compress,map_cache.fp) != size_compress) {
					// �Ȃ����t�@�C���㔼�������Ă�̂œǂݒ���
					printf("fread error\n");
					m->xs = 0;
					m->ys = 0;
					aFree(m->gat);
					m->gat = NULL;
					aFree(buf);
					return 0;
				}
				dest_len = m->xs * m->ys;
				decode_zip(m->gat,&dest_len,buf,size_compress);
				if(dest_len != map_cache.map[i].xs * map_cache.map[i].ys) {
					// ����ɉ𓀂��o���ĂȂ�
					m->xs = 0;
					m->ys = 0;
					aFree(m->gat);
					m->gat = NULL;
					aFree(buf);
					return 0;
				}
				aFree(buf);
				return 1;
			}
		}
	}
	return 0;
}

static int map_cache_write(struct map_data *m)
{
	int i;
	unsigned long len_new , len_old;
	char *write_buf;
	if(!map_cache.fp) { return 0; }
	for(i = 0;i < map_cache.head.nmaps ; i++) {
		if(!strcmp(m->name,map_cache.map[i].fn)) {
			// �����G���g���[������Ώ㏑��
			if(map_cache.map[i].compressed == 0) {
				len_old = map_cache.map[i].xs * map_cache.map[i].ys;
			} else if(map_cache.map[i].compressed == 1) {
				len_old = map_cache.map[i].compressed_len;
			} else {
				// �T�|�[�g����ĂȂ��`���Ȃ̂Œ����O
				len_old = 0;
			}
			if(map_read_flag == 2) {
				// ���k�ۑ�
				// �������ɂQ�{�ɖc��鎖�͂Ȃ��Ƃ�������
				write_buf = (char *)aMalloc(m->xs * m->ys * 2);
				len_new = m->xs * m->ys * 2;
				encode_zip(write_buf,&len_new,m->gat,m->xs * m->ys);
				map_cache.map[i].compressed     = 1;
				map_cache.map[i].compressed_len = len_new;
			} else {
				len_new = m->xs * m->ys;
				write_buf = m->gat;
				map_cache.map[i].compressed     = 0;
				map_cache.map[i].compressed_len = 0;
			}
			if(len_new <= len_old) {
				// �T�C�Y���������������Ȃ����̂ŏꏊ�͕ς��Ȃ�
				fseek(map_cache.fp,map_cache.map[i].pos,SEEK_SET);
				fwrite(write_buf,1,len_new,map_cache.fp);
			} else {
				// �V�����ꏊ�ɓo�^
				fseek(map_cache.fp,map_cache.head.filesize,SEEK_SET);
				fwrite(write_buf,1,len_new,map_cache.fp);
				map_cache.map[i].pos = map_cache.head.filesize;
				map_cache.head.filesize += len_new;
			}
			map_cache.map[i].xs  = m->xs;
			map_cache.map[i].ys  = m->ys;
			map_cache.map[i].water_height = map_waterheight(m->name);
			map_cache.dirty = 1;
			if(map_read_flag == 2) {
				aFree(write_buf);
			}
			return 0;
		}
	}
	// �����G���g����������Ώ������߂�ꏊ��T��
	for(i = 0;i < map_cache.head.nmaps ; i++) {
		if(map_cache.map[i].fn[0] == 0) {
			// �V�����ꏊ�ɓo�^
			if(map_read_flag == 2) {
				write_buf = (char *)aMalloc(m->xs * m->ys * 2);
				len_new = m->xs * m->ys * 2;
				encode_zip(write_buf,&len_new,m->gat,m->xs * m->ys);
				map_cache.map[i].compressed     = 1;
				map_cache.map[i].compressed_len = len_new;
			} else {
				len_new = m->xs * m->ys;
				write_buf = m->gat;
				map_cache.map[i].compressed     = 0;
				map_cache.map[i].compressed_len = 0;
			}
			strncpy(map_cache.map[i].fn,m->name,sizeof(map_cache.map[0].fn));
			fseek(map_cache.fp,map_cache.head.filesize,SEEK_SET);
			fwrite(write_buf,1,len_new,map_cache.fp);
			map_cache.map[i].pos = map_cache.head.filesize;
			map_cache.map[i].xs  = m->xs;
			map_cache.map[i].ys  = m->ys;
			map_cache.map[i].water_height = map_waterheight(m->name);
			map_cache.head.filesize += len_new;
			map_cache.dirty = 1;
			if(map_read_flag == 2) {
				aFree(write_buf);
			}
			return 0;
		}
	}
	// �������߂Ȃ�����
	return 1;
}

/*==========================================
 * �ǂݍ���map��ǉ�����
 *------------------------------------------
 */
static void map_addmap(char *mapname)
{
	int i;

	if(strcmpi(mapname, "clear") == 0) {
		if(map != NULL) {
			aFree(map);
			map = NULL;
		}
		map_num = 0;
		return;
	}

	for(i=0;i<map_num;i++) {
		if(strcmp(map[i].name,mapname)==0)
			return;		// ���ɒǉ��ς݂�MAP
	}

	if(map_num == 0) {
		map = (struct map_data *)aCalloc(1, sizeof(struct map_data));
	} else {
		map = (struct map_data *)aRealloc(map, sizeof(struct map_data) * (map_num + 1));
		memset(map + map_num, 0, sizeof(struct map_data));
	}
	memcpy(map[map_num].name, mapname, 24);
	map_num++;

	return;
}

/*==========================================
 * �ǂݍ���map���폜����
 *------------------------------------------
 */
static void map_delmap(char *mapname)
{
	int i;
	if( strcmpi(mapname,"all")==0 ){
		map_num=0;
		if(map != NULL) {
			aFree(map);
			map = NULL;
		}
		return;
	}

	for(i=0;i<map_num;i++){
		if(strcmp(map[i].name,mapname)==0){
			memmove(map+i,map+i+1,sizeof(map[0])*(map_num-i-1));
			map_num--;
			map = (struct map_data *)aRealloc(map, sizeof(struct map_data) * map_num);
		}
	}

	return;
}

/*==========================================
 * �}�b�v1���ǂݍ���
 * ===================================================*/
static int map_readmap(int m,char *fn,int *map_cache)
{
	unsigned char *gat;
	size_t size;

	printf("map reading %s[%d/%d] %-20s  \r", (map_read_flag ? "with cache " : ""), m, map_num, fn);
	fflush(stdout);

	if(map_cache_read(&map[m])) {
		// �L���b�V������ǂݍ��߂�
		(*map_cache)++;
	} else {
		int s;
		int wh;
		int x,y,xs,ys;
		struct gat_1cell {float high[4]; int type;} *p;
		// read & convert fn
		gat = (unsigned char *)grfio_read(fn);
		if(gat==NULL) {
			// �������Ƀ}�b�v���ǂ߂Ȃ��̂͂܂����̂ŏI������
			printf("Map '%s' not found: removed from maplist.\n", fn);
			return -1;
		}

		xs = *(int*)(gat + 6);
		ys = *(int*)(gat + 10);
		if (xs == 0 || ys == 0) { // ?? but possible
			printf("Invalid size for map '%s' (xs,ys: %d,%d): removed from maplist.\n", fn, xs, ys);
			return -1;
		}
		map[m].xs = xs;
		map[m].ys = ys;
		map[m].gat = (unsigned char *)aCalloc(s = map[m].xs * map[m].ys,sizeof(unsigned char));
		wh=map_waterheight(map[m].name);
		for(y=0;y<ys;y++){
			p=(struct gat_1cell*)(gat+y*xs*20+14);
			for(x=0;x<xs;x++){
				if(wh!=NO_WATER && p->type==0){
					// ���ꔻ��
					map[m].gat[x+y*xs]=(p->high[0]>wh || p->high[1]>wh || p->high[2]>wh || p->high[3]>wh) ? 3 : 0;
				} else {
					map[m].gat[x+y*xs]=p->type;
				}
				p++;
			}
		}
		map_cache_write(&map[m]);
		aFree(gat);
	}
	map[m].m=m;
	map[m].npc_num=0;
	map[m].users=0;
	memset(&map[m].flag,0,sizeof(map[m].flag));
	map[m].bxs=(map[m].xs+BLOCK_SIZE-1)/BLOCK_SIZE;
	map[m].bys=(map[m].ys+BLOCK_SIZE-1)/BLOCK_SIZE;
	size = map[m].bxs * map[m].bys * sizeof(struct block_list*);
	map[m].block = (struct block_list **)aCalloc(1,size);
	map[m].block_mob = (struct block_list **)aCalloc(1,size);
	size = map[m].bxs*map[m].bys*sizeof(int);
	strdb_insert(map_db,map[m].name,&map[m]);

//	printf("%s read done\n", fn);

	return 0;
}

/*==========================================
 * �S�Ă�map�f�[�^��ǂݍ���
 *------------------------------------------
 */
static void map_readallmap(void)
{
	int i;
	char fn[256];
	FILE *fp=NULL;
	int map_cache = 0;
	int maps_removed = 0;

	// �}�b�v�L���b�V�����J��
	if(map_read_flag) {
		map_cache_open(map_cache_file);
	}

	/*
	// ��ɑS���̃}�b�v�̑��݂��m�F
	for(i=0;i<map_num;i++){
		if(strstr(map[i].name,".gat")==NULL)
			continue;
		sprintf(fn,"data\\%s",map[i].name);
		grfio_size(fn);
	}*/
	for(i=0;i<map_num;i++){
		if(strstr(map[i].name,".gat")==NULL)
			continue;
		sprintf(fn,"data\\%s",map[i].name);
		if (map_readmap(i, fn, &map_cache) == -1) {
			map_delmap(map[i].name);
			maps_removed++;
			i--;
		}
	}

	if (waterlist_num > 0) {
		aFree(waterlist);
		waterlist_num = 0;
	}
	if (maps_removed) {
		printf("%d maps in configuration file. %d map%s removed.\n", map_num + maps_removed, maps_removed, (maps_removed > 1) ? "s" : "");
	}
	printf("Map read done (%d map%s, %d map%s in cache). %24s\n", map_num, (map_num > 1) ? "s" : "", map_cache, (map_cache > 1) ? "s" : "", "");

	// �}�b�v�L���b�V�������
	map_cache_close();

	if(fp!=NULL)	fclose(fp);

	if (map_num <= 0) {
		printf("ERROR: no map found.\n");
		exit(1);
	}

	return;
}

/*==========================================
 * @who��DB��
 *------------------------------------------
 */
 int map_who_sub(void *key,void *data,va_list ap)
{
	struct charid2nick *p;
	int fd;

	nullpo_retr(-1, ap);
	nullpo_retr(-1, data);
	nullpo_retr(-1, p=(struct charid2nick *)data);

//printf("who: %s %d %d\n",p->nick,(int)p->ip,p->port);

	fd=va_arg(ap,int);

	if( p->ip != 0 &&
		p->port != 0 &&
		!(battle_config.hide_GM_session && pc_numisGM(p->account_id))
	)
		clif_displaymessage(fd, p->nick);

	return 0;
}
int map_who(int fd){
	numdb_foreach( charid_db, map_who_sub, fd );
	return 0;
}

//PK�T�[�o�[�Ɉꊇ�ύX
int map_pk_server(int flag)
{
	int i,count=0;
	if(!flag)
		return 0;
	printf("server setting:pk map ");
	for(i=0;i<map_num;i++)
	{
		if(map[i].flag.gvg || map[i].flag.pvp)
			continue;
		if(map[i].flag.nopenalty)
			continue;
		if(map[i].flag.gvg_noparty)
			continue;
		if(!map[i].flag.nomemo)
			continue;
		map[i].flag.pk = 1;
		count++;
	}
	printf("(count:%d/%d)\n",count,map_num);
	return 1;
}

//PK�t�B�[���h�̃A�C�e���h���b�v���ꊇ�ύX
int map_pk_nightmaredrop(int flag)
{
	int m,i,count=0;

	if(!flag)
		return 0;

	printf("server setting:pk nightmaredrop ");
	for(m=0; m<map_num; m++)
	{
		if(!map[m].flag.pk)
			continue;
		for(i=0; i<MAX_DROP_PER_MAP; i++) {
			if(map[m].drop_list[i].drop_id == 0) {
				map[m].drop_list[i].drop_id   = -1;			// random
				map[m].drop_list[i].drop_type = 2;			// equip
				map[m].drop_list[i].drop_per  = 1000;			// 10%
				map[m].drop_list[i].drop_flag = MF_PK_NIGHTMAREDROP;	// PK
				break;
			}
		}
		if(i >= MAX_DROP_PER_MAP) {	// �󂫂��Ȃ��ꍇ�̓}�b�v�t���O���Z�b�g���Ȃ�
			printf("\nmap_pk_nightmaredrop: drop list is full (%s, size = %d)\a\n", map[m].name, i);
			continue;
		}
		map[m].flag.pk_nightmaredrop = 1;
		count++;
	}
	printf("(count:%d/%d)\n",count,map_num);
	return 1;
}
/*==========================================
 * �ݒ�t�@�C����ǂݍ���
 *------------------------------------------
 */
static int map_config_read(char *cfgName)
{
	char line[1024], w1[1024], w2[1024];
	FILE *fp;
	struct hostent *h = NULL;

	fp = fopen(cfgName,"r");
	if (fp == NULL) {
		printf("file not found: %s\n", cfgName);
		return 1;
	}

	while(fgets(line, sizeof(line) - 1, fp)) {
		if (line[0] == '/' && line[1] == '/')
			continue;

		if(sscanf(line, "%[^:]: %[^\r\n]", w1, w2) != 2)
			continue;

		if (strcmpi(w1, "userid") == 0) {
			chrif_setuserid(w2);
		} else if (strcmpi(w1, "passwd") == 0) {
			chrif_setpasswd(w2);
		} else if (strcmpi(w1, "char_ip") == 0) {
			h = gethostbyname(w2);
			if (h != NULL) {
				printf("Character server IP address : %s -> %d.%d.%d.%d\n", w2, (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
				sprintf(w2, "%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
			}
			chrif_setip(w2);
		} else if (strcmpi(w1, "char_port") == 0) {
			if (atoi(w2) < 0 || atoi(w2) > 65535) {
				printf("map_config_read: Invalid char_port value: %d.\n", atoi(w2));
				continue;
			}
			chrif_setport(atoi(w2));
		} else if (strcmpi(w1, "map_ip") == 0) {
			h = gethostbyname(w2);
			if(h != NULL) {
				printf("Map server IP address : %s -> %d.%d.%d.%d\n", w2, (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
				sprintf(w2, "%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
			}
			clif_setip(w2);
		} else if (strcmpi(w1, "map_port") == 0) {
			if (atoi(w2) < 0 || atoi(w2) > 65535) {
				printf("map_config_read: Invalid map_port value: %d.\n", atoi(w2));
				continue;
			}
			clif_setport(atoi(w2));
		} else if (strcmpi(w1, "listen_ip") == 0) {
			unsigned long ip_result;
			h = gethostbyname(w2);
			if (h != NULL)
				sprintf(w2, "%d.%d.%d.%d", (unsigned char)h->h_addr[0], (unsigned char)h->h_addr[1], (unsigned char)h->h_addr[2], (unsigned char)h->h_addr[3]);
			if ((ip_result = inet_addr(w2)) == INADDR_NONE) // not always -1
				printf("map_config_read: Invalid listen_ip value: %s.\n", w2);
			else
				listen_ip = ip_result;
		} else if (strcmpi(w1, "water_height") == 0) {
			map_readwater(w2);
		} else if (strcmpi(w1, "gm_account_filename") == 0) {
			pc_set_gm_account_fname(w2);
		} else if (strcmpi(w1, "map") == 0) {
			map_addmap(w2);
		} else if (strcmpi(w1, "delmap") == 0) {
			map_delmap(w2);
		} else if (strcmpi(w1, "npc") == 0) {
			npc_addsrcfile(w2);
		} else if (strcmpi(w1, "delnpc") == 0) {
			npc_delsrcfile(w2);
		} else if (strcmpi(w1, "read_grf_files_txt") == 0) {
			read_grf_files_txt = atoi(w2);
		} else if (strcmpi(w1, "packet_parse_time") == 0) {
			packet_parse_time = atoi(w2);
			if (packet_parse_time < 0) {
				printf("map_config_read: Invalid packet_parse_time value: %d. Set to 0 (default).\n", packet_parse_time);
				packet_parse_time = 0;
			}
		} else if (strcmpi(w1, "autosave_time") == 0) {
			autosave_interval = atoi(w2) * 1000;
			if (autosave_interval <= 0) {
				printf("map_config_read: Invalid autosave_time value: %d. Set to %d (default).\n", autosave_interval, (int)(DEFAULT_AUTOSAVE_INTERVAL / 1000));
				autosave_interval = DEFAULT_AUTOSAVE_INTERVAL;
			}
		} else if (strcmpi(w1, "autosave_gvg_rate") == 0) {
			autosave_gvg_rate = atoi(w2);
			if (autosave_gvg_rate < 100) {
				printf("map_config_read: Invalid autosave_gvg_rate value: %d. Set to 100 (minimum).\n", autosave_gvg_rate);
				autosave_gvg_rate = 100;
			}
		} else if (strcmpi(w1, "motd_txt") == 0) {
			strncpy(motd_txt, w2, sizeof(motd_txt) - 1);
			motd_txt[sizeof(motd_txt) - 1] = '\0';
		} else if (strcmpi(w1, "help_txt") == 0) {
			strncpy(help_txt, w2, sizeof(help_txt) - 1);
			help_txt[sizeof(help_txt) - 1] = '\0';
		} else if (strcmpi(w1, "mapreg_txt") == 0) {
			strncpy(mapreg_txt, w2, sizeof(mapreg_txt) - 1);
			mapreg_txt[sizeof(mapreg_txt) - 1] = '\0';
		} else if (strcmpi(w1, "extra_add_file_txt") == 0) {
			strncpy(extra_add_file_txt, w2, sizeof(extra_add_file_txt) - 1);
			extra_add_file_txt[sizeof(extra_add_file_txt) - 1] = '\0';
		}else if (strcmpi(w1, "read_map_from_cache") == 0) {
			map_read_flag = atoi(w2);
		}else if (strcmpi(w1, "map_cache_file") == 0) {
			strncpy(map_cache_file, w2, sizeof(map_cache_file) - 1);
			map_cache_file[sizeof(map_cache_file) - 1] = '\0';
		}else if (strcmpi(w1, "httpd_enable") == 0) {
			socket_enable_httpd(atoi(w2));
		}else if (strcmpi(w1, "httpd_document_root") == 0) {
			httpd_set_document_root(w2);
		}else if (strcmpi(w1, "httpd_log_filename") == 0) {
			httpd_set_logfile(w2);
		}else if (strcmpi(w1, "httpd_config") == 0) {
			httpd_config_read(w2);
		}else if (strcmpi(w1, "import") == 0) {
			map_config_read(w2);
		}else if (strcmpi(w1, "map_pk_server") == 0) {
			map_pk_server_flag = atoi(w2);
		}else if (strcmpi(w1, "map_pk_nightmaredrop") == 0) {
			map_pk_nightmaredrop_flag = atoi(w2);
		}
	}
	fclose(fp);

	return 0;
}

/*==========================================
 * �}�b�v���ʏ�}�b�v�ł��邩��r���I�ɐݒ�
 *------------------------------------------
 */
int map_field_setting(void)
{
	int m;

	for(m=0;m<map_num;m++){
		if(map[m].flag.pk || map[m].flag.pvp || map[m].flag.gvg)
			map[m].flag.normal = 0;
		else
			map[m].flag.normal = 1;
	}

	return 0;
}

/*==========================================
 * socket �R���g���[���p�l������Ă΂��
 *------------------------------------------
 */
void map_socket_ctrl_panel_func(int fd,char* usage,char* user,char* status)
{
	struct socket_data *sd = session[fd];
	strcpy( usage,
		( sd->func_parse == clif_parse )? "map user" :
		( sd->func_parse == chrif_parse )? "char server" : "unknown" );

	if( sd->func_parse == clif_parse && sd->auth )
	{
		struct map_session_data *sd2 = (struct map_session_data *)sd->session_data;
		sprintf( user, "%d %d(%s)", sd2->bl.id, sd2->char_id, sd2->status.name );
	}
}

/*==========================================
 * map�I�I��������
 *------------------------------------------
 */
static int id_db_final(void *key,void *data,va_list ap)
{
	return 0;
}
static int map_db_final(void *key,void *data,va_list ap)
{
//	char *name;

//	nullpo_retr(0, name=data);

//	aFree(name);

	return 0;
}
static int nick_db_final(void *key,void *data,va_list ap)
{
	char *nick;

	nullpo_retr(0, nick = (char *)data);

	aFree(nick);

	return 0;
}
static int charid_db_final(void *key,void *data,va_list ap)
{
	struct charid2nick *p;

	nullpo_retr(0, p = (struct charid2nick *)data);

	aFree(p);

	return 0;
}
void do_final(void)
{
	int i;
	unsigned int tick = gettick();

	chrif_mapactive(0); //�}�b�v�T�[�o�[��~��

	for(i=0;i<MAX_FLOORITEM;i++) {
		if( object[i] == NULL ) continue;
		if( object[i]->type == BL_ITEM ) {
			map_clearflooritem_timer(-1, tick, i, 1);
		}
	}

	do_final_chrif(); // ���̓����ŃL������S�Đؒf����
	do_final_npc();
	do_final_script();
	do_final_itemdb();
	do_final_storage();
	do_final_guild();
	do_final_clif();
	do_final_pc();
	do_final_party();
	do_final_pet();
	do_final_homun();
	do_final_friend();
	do_final_unit();
	do_final_mob();
	do_final_atcommand();

	for(i=0;i<map_num;i++){
		if(map[i].gat) {
			aFree(map[i].gat);
			map[i].gat=NULL;
		}
		if(map[i].block)
			aFree(map[i].block);
		if(map[i].block_mob)
			aFree(map[i].block_mob);
	}

	if(map != NULL) {
		aFree(map);
		map = NULL;
	}
	map_num = 0;

	if(map_db)
		strdb_final(map_db,map_db_final);
	if(nick_db)
		strdb_final(nick_db,nick_db_final);
	if(charid_db)
		numdb_final(charid_db,charid_db_final);
	if(id_db)
		numdb_final(id_db,id_db_final);
	//if(address_db)
	//	numdb_final(address_db,NULL);
	exit_dbn();

	do_final_timer();
}


/*==========================================
 * map�I�������̑匳
 *------------------------------------------
 */
int do_init(int argc,char *argv[])
{
	printf("Athena Map Server [%s] v%d.%d.%d mod%d\n",
#ifdef TXT_ONLY
		"TXT",
#else
		"SQL",
#endif
		AURIGA_MAJOR_VERSION,AURIGA_MINOR_VERSION,AURIGA_REVISION,
		AURIGA_MOD_VERSION
	);

#ifdef _WIN32
	srand(gettick() ^ (GetCurrentProcessId() << 8) );
	atn_srand(gettick() ^ (GetCurrentProcessId() << 8) );
#else
	srand(gettick() ^ (getpid() << 8));
	atn_srand(gettick() ^ (getpid() << 8));
#endif

	if(map_config_read((argc<2)? MAP_CONF_NAME:argv[1]))
		exit(1);
	battle_config_read((argc>2)? argv[2]:BATTLE_CONF_FILENAME);
	atcommand_config_read((argc>3)? argv[3]:ATCOMMAND_CONF_FILENAME);
	script_config_read((argc>4)? argv[4]:SCRIPT_CONF_NAME);
	msg_config_read((argc>5)? argv[5]:MSG_CONF_NAME);

	socket_set_httpd_page_connection_func( map_socket_ctrl_panel_func );

	id_db = numdb_init();
	map_db = strdb_init(16);
	nick_db = strdb_init(24);
	charid_db = numdb_init();
	//address_db = numdb_init();

	grfio_init( (!read_grf_files_txt)? NULL : (argc>6)? argv[6] : GRF_PATH_FILENAME );
	map_readallmap();

	add_timer_func_list(map_freeblock_timer,"map_freeblock_timer");
	add_timer_func_list(map_clearflooritem_timer,"map_clearflooritem_timer");
	add_timer_interval(gettick()+1000,map_freeblock_timer,0,0,60*1000);

	do_init_chrif();
	do_init_clif();
	do_init_script(); // parse_script ���Ăяo���O�ɂ�����Ă�
	do_init_itemdb();
	do_init_mob();	// npc�̏�����������mob_spawn���āAmob_db���Q�Ƃ���̂�init_npc����
	do_init_npc();
	do_init_pc();
	do_init_party();
	do_init_guild();
	do_init_storage();
	do_init_skill();
	do_init_pet();
	do_init_homun();
	do_init_status();
	do_init_friend();
	do_init_ranking();
	do_init_unit();
	//
	map_pk_server(map_pk_server_flag);
	map_pk_nightmaredrop(map_pk_nightmaredrop_flag);
	map_field_setting();
	npc_event_do_oninit();	// npc��OnInit�C�x���g���s
	// for httpd support
	do_init_httpd();
	do_init_graph();
	graph_add_sensor("Uptime(days)",60*1000,uptime);
	graph_add_sensor("Memory Usage(KB)",60*1000,memmgr_usage);
	graph_add_sensor("Mob ai efficiency(%)",60*1000,mob_ai_hard_sensor);

	httpd_default_page(httpd_send_file);
	httpd_pages("/chat",clif_webchat);

	return 0;
}
