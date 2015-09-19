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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "timer.h"
#include "socket.h"
#include "db.h"
#include "nullpo.h"
#include "malloc.h"
#include "utils.h"

#include "atcommand.h"
#include "map.h"
#include "clif.h"
#include "intif.h"
#include "pc.h"
#include "mob.h"
#include "homun.h"
#include "guild.h"
#include "itemdb.h"
#include "skill.h"
#include "battle.h"
#include "party.h"
#include "npc.h"
#include "status.h"
#include "date.h"
#include "unit.h"
#include "ranking.h"
#include "merc.h"

#define MIN_MOBTHINKTIME 100

#define MOB_LAZYMOVEPERC     50		// �蔲�����[�hMOB�̈ړ��m���i�番���j
#define MOB_LAZYWARPPERC     20		// �蔲�����[�hMOB�̃��[�v�m���i�番���j
#define MOB_LAZYSKILLUSEPERC  2		// �蔲�����[�hMOB�̃X�L���g�p�m���i�番���j

static struct mob_db mob_db_real[MOB_ID_MAX-MOB_ID_MIN];
struct mob_db *mob_db = &mob_db_real[-MOB_ID_MIN];

static int mob_dummy_class[MAX_RANDOMMONSTER];	// �����_�������X�^�[�I�����s���̃_�~�[ID

/*==========================================
 * ���[�J���v���g�^�C�v�錾 (�K�v�ȕ��̂�)
 *------------------------------------------
 */
static int mob_makedummymobdb(int);
static int mob_dead(struct block_list *src,struct mob_data *md,int type,unsigned int tick);
static int mob_skillid2skillidx(int class_,int skillid);
static int mobskill_use_id(struct mob_data *md,struct block_list *target,int skill_idx);

/*==========================================
 * mob�𖼑O�Ō���
 *------------------------------------------
 */
int mobdb_searchname(const char *str)
{
	int i;

	for(i=MOB_ID_MIN; i<MOB_ID_MAX; i++) {
		if( strcmpi(mob_db[i].name,str) == 0 || strcmp(mob_db[i].jname,str) == 0 )
			return i;
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mobdb_checkid(const int mob_id)
{
	if (mob_id >= MOB_ID_MIN && mob_id < MOB_ID_MAX) {
		if (mob_db[mob_id].name[0] != '\0' || mob_db[mob_id].jname[0] != '\0')
			return mob_id;
	}
	return 0;
}

/*==========================================
 * MOB�o���p�̍Œ���̃f�[�^�Z�b�g
 *------------------------------------------
 */
static int mob_spawn_dataset(struct mob_data *md,const char *mobname,int class_)
{
	nullpo_retr(0, md);

	md->bl.type = BL_MOB;
	md->bl.prev = NULL;
	md->bl.next = NULL;

	if(strcmp(mobname,"--en--") == 0) {
		memcpy(md->name,mob_db[class_].name,24);
	} else if(strcmp(mobname,"--ja--") == 0) {
		memcpy(md->name,mob_db[class_].jname,24);
	} else {
		strncpy(md->name,mobname,24);
		md->name[23] = '\0';	// force \0 terminal
	}

	md->n          = 0;
	md->base_class = class_;
	md->class_     = class_;
	md->bl.id      = npc_get_new_npc_id();

	memset(&md->state,0,sizeof(md->state));

	md->target_id   = 0;
	md->attacked_id = 0;
	md->guild_id    = 0;
	md->speed       = mob_db[class_].speed;

	unit_dataset( &md->bl );

	return 0;
}

/*==========================================
 * �ꔭMOB�o��(�X�N���v�g�p)
 *------------------------------------------
 */
int mob_once_spawn(struct map_session_data *sd,const char *mapname,
	int x,int y,const char *mobname,int class_,int amount,const char *event)
{
	struct mob_data *md = NULL;
	int m, count, lv;
	int r = class_;

	if(sd && strcmp(mapname,"this") == 0)
		m = sd->bl.m;
	else
		m = map_mapname2mapid(mapname);

	if( m < 0 || amount <= 0 || (class_ >= 0 && !mobdb_checkid(class_)) )	// �l���ُ�Ȃ珢�����~�߂�
		return 0;

	lv = (sd)? sd->status.base_level: MAX_LEVEL;

	if(class_ < 0) {	// �����_���ɏ���
		int k, i = 0;
		int j = -class_ - 1;

		if(j >= 0 && j < MAX_RANDOMMONSTER) {
			do {
				class_ = atn_rand()%(MOB_ID_MAX-MOB_ID_MIN) + MOB_ID_MIN;
				k = atn_rand()%1000000;
			} while((mob_db[class_].max_hp <= 0 || mob_db[class_].summonper[j] <= k ||
			        (lv < mob_db[class_].lv && battle_config.random_monster_checklv)) && (i++) < MOB_ID_MAX);

			if(i >= MOB_ID_MAX) {
				class_ = mob_dummy_class[j];
			}
		} else {
			return 0;
		}
	}
	if(sd) {
		if(x <= 0) x = sd->bl.x;
		if(y <= 0) y = sd->bl.y;
	} else if(x <= 0 || y <= 0) {
		printf("mob_once_spawn: Map %s, Class %d\n", map[m].name, class_);
	}

	for(count=0; count<amount; count++) {
		md = (struct mob_data *)aCalloc(1,sizeof(struct mob_data));

		if(mob_db[class_].mode & 0x02)
			md->lootitem = (struct item *)aCalloc(LOOTITEM_SIZE,sizeof(struct item));
		else
			md->lootitem = NULL;

		mob_spawn_dataset(md,mobname,class_);
		md->bl.m = m;
		md->bl.x = x;
		md->bl.y = y;
		if(r < 0)
			md->class_ = r;
		md->m  = m;
		md->x0 = x;
		md->y0 = y;
		md->xs = 0;
		md->ys = 0;
		md->spawndelay1  = -1;	// ��x�̂݃t���O
		md->spawndelay2  = -1;	// ��x�̂݃t���O
		md->state.nodrop = 0;
		md->state.noexp  = 0;
		md->state.nomvp  = 0;
		md->ai_pc_count  = 0;
		md->ai_prev = md->ai_next = NULL;

#ifdef DYNAMIC_SC_DATA
		// �_�~�[�}��
		md->sc.data = dummy_sc_data;
#endif

		strncpy(md->npc_event,event,sizeof(md->npc_event));
		md->npc_event[sizeof(md->npc_event)-1] = '\0';	// force \0 terminal

		map_addiddb(&md->bl);
		mob_spawn(md->bl.id);
	}
	return (amount > 0)? md->bl.id: 0;
}

/*==========================================
 * �ꔭMOB�o��(�X�N���v�g�p���G���A�w��)
 *------------------------------------------
 */
int mob_once_spawn_area(struct map_session_data *sd,const char *mapname,
	int x0,int y0,int x1,int y1,const char *mobname,int class_,int amount,const char *event)
{
	int m, x, y, i, max;
	int lx = -1, ly = -1, id = 0;

	m = map_mapname2mapid(mapname);

	max = (y1-y0+1)*(x1-x0+1)*3;
	if(x0 <= 0 && y0 <= 0)
		max = 50;	// mob_spawn�ɕ����50
	else if(max > 1000)
		max = 1000;

	if( m < 0 || amount <= 0 || (class_ >= 0 && !mobdb_checkid(class_)) )	// �l���ُ�Ȃ珢�����~�߂�
		return 0;

	for(i=0; i<amount; i++) {
		int j = 0;
		do {
			if(x0 <= 0 && y0 <= 0) {	// x0,y0��0�ȉ��̂Ƃ��ʒu�����_���N��
				x = atn_rand()%(map[m].xs-2)+1;
				y = atn_rand()%(map[m].ys-2)+1;
			} else {
				x = atn_rand()%(x1-x0+1)+x0;
				y = atn_rand()%(y1-y0+1)+y0;
			}
		} while(map_getcell(m,x,y,CELL_CHKNOPASS) && (++j) < max);

		if(j >= max) {
			if(lx < 0) {
				// �ŏ��ɕ����ꏊ�̌��������s�����̂ł�߂�
				return 0;
			}
			// �����Ɏ��s�����̂ňȑO�ɕ������ꏊ���g��
			x = lx;
			y = ly;
		}
		if(x == 0 || y == 0)
			printf("x or y=0, x=%d,y=%d,x0=%d,y0=%d\n",x,y,x0,y0);
		id = mob_once_spawn(sd,mapname,x,y,mobname,class_,1,event);
		lx = x;
		ly = y;
	}
	return id;
}

/*==========================================
 * delay�t��mob spawn (timer�֐�)
 *------------------------------------------
 */
int mob_delayspawn(int tid,unsigned int tick,int id,int data)
{
	mob_spawn(id);
	return 0;
}

/*==========================================
 * mob�o���B�F�X��������������
 *------------------------------------------
 */
int mob_spawn(int id)
{
	int x = 0, y = 0, i = 0;
	unsigned int c, tick = gettick();
	struct mob_data *md;

	md = map_id2md(id);
	if(md == NULL) {
		// ���ɍ폜���ꂽ�\��������
		return -1;
	}

	md->last_spawntime = tick;
	md->mode = mob_db[md->base_class].mode;

	if( md->bl.prev != NULL ) {
//		clif_clearchar_area(&md->bl,3);
//		skill_unit_move(&md->bl,tick,0);  // sc_data�͏����������וK�v�Ȃ�
		mob_ai_hard_spawn( &md->bl, 0 );
		map_delblock(&md->bl);
	} else {
		if(md->class_ < 0 && battle_config.dead_branch_active) {
			md->mode |= 0x01 + 0x04 + 0x80;				// �}mob�͈ړ����ăA�N�e�B�u�Ŕ���
		}
		else if(md->class_ >= 0 && md->class_ != md->base_class) {	// �N���X�`�F���W����Mob
			memcpy(md->name,mob_db[md->base_class].jname,24);
			md->speed = mob_db[md->base_class].speed;
		}
		md->class_ = md->base_class;
	}

	md->bl.m = md->m;
	do {
		if(md->x0 == 0 && md->y0 == 0) {
			x = atn_rand()%(map[md->bl.m].xs-2)+1;
			y = atn_rand()%(map[md->bl.m].ys-2)+1;
		} else {
			x = md->x0+atn_rand()%(md->xs+1)-md->xs/2;
			y = md->y0+atn_rand()%(md->ys+1)-md->ys/2;
		}
	} while(map_getcell(md->bl.m,x,y,CELL_CHKNOPASS) && (++i) < 50);

	if(i >= 50) {
		if(md->spawndelay1 == -1 && md->spawndelay2 == -1) {
			// ��x���������Ȃ��͕̂ǂłȂ�����X�^�b�N�ʒu�ł��o��
			if(!map_getcell(md->bl.m,x,y,CELL_CHKGROUND))
				return 1;
		} else {
			// 1�b��Ƀ��g���C����
			add_timer(tick+1000,mob_delayspawn,id,0);
			return 1;
		}
	}

	md->ud.to_x = md->bl.x = x;
	md->ud.to_y = md->bl.y = y;
	md->dir = 0;
	md->view_size = mob_db[md->class_].view_size;

	memset(&md->state,0,sizeof(md->state));

	md->attacked_id     = 0;
	md->target_id       = 0;
	md->move_fail_count = 0;

	if(!md->speed)
		md->speed = mob_db[md->class_].speed;
	md->def_ele     = mob_db[md->class_].element;
	md->master_id   = 0;
	md->master_dist = 0;

	md->state.skillstate = MSS_IDLE;
	md->last_thinktime   = tick;
	md->next_walktime    = tick + atn_rand()%50 + 5000;

	md->state.nodrop = 0;
	md->state.noexp  = 0;
	md->state.nomvp  = 0;

	if(md->class_ == 1288 || md->class_ == 1287 || md->class_ == 1286 || md->class_ == 1285) {
		struct guild_castle *gc = guild_mapname2gc(map[md->bl.m].name);
		if(gc)
			md->guild_id = gc->guild_id;
	}

	md->deletetimer = -1;

	c = tick - 1000 * 3600 * 10;
	for(i=0; i<MAX_MOBSKILL; i++) {
		md->skilldelay[i] = c;
	}
	if(md->lootitem)
		memset(md->lootitem, 0, sizeof(struct item) * LOOTITEM_SIZE);
	md->lootitem_count = 0;

#ifdef DYNAMIC_SC_DATA
	// �_�~�[�}��
	if(md->sc.data == NULL)
		md->sc.data = dummy_sc_data;
#else
	for(i=0; i<MAX_STATUSCHANGE; i++) {
		md->sc.data[i].timer = -1;
		md->sc.data[i].val1  = 0;
		md->sc.data[i].val2  = 0;
		md->sc.data[i].val3  = 0;
		md->sc.data[i].val4  = 0;
	}
#endif

	md->sc.count  = 0;
	md->sc.opt1   = 0;
	md->sc.opt2   = 0;
	md->sc.opt3   = 0;
	md->sc.option = 0;

	md->hp = status_get_max_hp(&md->bl);
	if(md->hp <= 0) {
		mob_makedummymobdb(md->class_);
		md->hp = status_get_max_hp(&md->bl);
	}
	unit_dataset( &md->bl );

	if(mob_is_pcview(md->class_))
		md->sc.option |= mob_db[md->class_].option;

	map_addblock(&md->bl);
	skill_unit_move(&md->bl,tick,1);	// sc_data��������̕K�v������

	clif_spawnmob(md);
	mob_ai_hard_spawn( &md->bl, 1 );
	mobskill_use(md, tick, MSC_SPAWN);

	return 0;
}

/*==========================================
 * �w��ID�̑��ݏꏊ�ւ̓��B�\��
 *------------------------------------------
 */
static int mob_can_reach(struct mob_data *md,struct block_list *bl,int range)
{
	struct walkpath_data wpd;

	nullpo_retr(0, md);
	nullpo_retr(0, bl);

	if(md->bl.m != bl-> m)	// �Ⴄ�}�b�v
		return 0;

	if(md->class_ == 1285 || md->class_ == 1286 || md->class_ == 1287) {
		// �K�[�f�B�A���̓M���h��ɑ�����PC�ɂ͍U�����Ȃ�
		if(bl->type == BL_PC) {
			struct map_session_data *sd = (struct map_session_data *)bl;
			struct guild_castle     *gc = guild_mapname2gc(map[bl->m].name);
			struct guild *g;

			if(!sd || !gc)
				return 0;
			if(!map[bl->m].flag.gvg)
				return 0;
			g = guild_search(sd->status.guild_id);
			if(g && (g->guild_id == gc->guild_id || guild_check_alliance(gc->guild_id, g->guild_id, 0)))
				return 0;
		}
	} else if(md->master_id > 0 && md->state.special_mob_ai && map[bl->m].flag.gvg) {
		// GvG���o�C�I�v�����gMob�͓���M���h�������M���h������PC�EMOB�ɂ͍U�����Ȃ�
		if(bl->type == BL_PC || bl->type == BL_MOB) {
			int gid = status_get_guild_id(bl);

			if(md->guild_id == gid || guild_check_alliance(md->guild_id, gid, 0))
				return 0;
		}
	}

	if( range > 0 && range < unit_distance(md->bl.x,md->bl.y,bl->x,bl->y) )	// ��������
		return 0;

	if( md->bl.x == bl->x && md->bl.y == bl->y )	// �����}�X
		return 1;

	if( mob_db[md->class_].range > 6 ) {
		// �U���\�ȏꍇ�͉������U���A����ȊO�͈ړ������݂�
		if( path_search_long(NULL,md->bl.m,md->bl.x,md->bl.y,bl->x,bl->y) )
			return 1;
	}

	// ��Q������
	wpd.path_len = 0;
	wpd.path_pos = 0;
	if( !path_search(&wpd,md->bl.m,md->bl.x,md->bl.y,bl->x,bl->y,0) && wpd.path_len <= AREA_SIZE )
		return 1;

	return 0;
}

/*==========================================
 * �^�[�Q�b�g�̃��b�N���\���ǂ���
 *------------------------------------------
 */
static int mob_can_lock(struct mob_data *md, struct block_list *bl)
{
	struct status_change *tsc;
	int mode, race;

	nullpo_retr(0, md);

	if(bl == NULL || unit_isdead(bl))
		return 0;

	mode = status_get_mode(&md->bl);
	race = status_get_race(&md->bl);
	tsc  = status_get_sc(bl);

	if( tsc && (tsc->data[SC_TRICKDEAD].timer != -1 || tsc->data[SC_FORCEWALKING].timer != -1) )
		return 0;
	if( md->sc.data[SC_WINKCHARM].timer != -1 )
		return 0;
	if( !(mode&0x20) && tsc && (tsc->option&0x4006) && race != RCT_INSECT && race != RCT_DEMON )
		return 0;

	if(bl->type == BL_PC) {
		struct map_session_data *tsd = (struct map_session_data *)bl;
		if(tsd) {
			if( tsd->invincible_timer != -1 )
				return 0;
			if( pc_isinvisible(tsd) )
				return 0;
			if( !(mode&0x20) && tsd->state.gangsterparadise )
				return 0;
		}
	}
	return 1;
}

/*==========================================
 * �����X�^�[�̍U���Ώی���
 *------------------------------------------
 */
int mob_target(struct mob_data *md,struct block_list *bl,int dist)
{
	int mode;

	nullpo_retr(0, md);
	nullpo_retr(0, bl);

	mode = status_get_mode(&md->bl);

	if(!(mode&0x80)) {
		md->target_id = 0;
		return 0;
	}
	// �A�C�e���ȊO�Ń^�Q��ς���C���Ȃ��Ȃ牽�����Ȃ�
	if( md->target_id > 0 ) {
		struct block_list *target = map_id2bl(md->target_id);
		if( target && target->type != BL_ITEM && (!(mode&0x04) || atn_rand()%100 > 25) )
			return 0;
	}

	if(mob_can_lock(md,bl)) {
		if(bl->type == BL_PC || bl->type == BL_MOB || bl->type == BL_HOM || bl->type == BL_MERC)
			md->target_id = bl->id;	// �W�Q���Ȃ������̂Ń��b�N
		md->min_chase = dist + 13;
		if(md->min_chase > 26)
			md->min_chase = 26;
	}
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_attacktarget(struct mob_data *md,struct block_list *target,int flag)
{
	nullpo_retr(0, md);
	nullpo_retr(0, target);

	if(flag == 0 && md->target_id)
		return 0;
	if( !unit_can_move(&md->bl) || unit_isrunning(&md->bl) )	// �����Ȃ���Ԃɂ���
		return 0;

	unit_stopattack(&md->bl);
	mob_target(md,target,unit_distance(md->bl.x,md->bl.y,target->x,target->y));
	//md->state.skillstate = MSS_CHASE;	// �ˌ����X�L��

	return 0;
}

/*==========================================
 * ���G���[�e�B��
 *------------------------------------------
 */
static int mob_ai_sub_hard_search(struct block_list *bl,va_list ap)
{
	struct mob_data *smd = NULL;
	int dist,flag;
	int *cnt;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, smd  = va_arg(ap,struct mob_data *));
	nullpo_retr(0, cnt  = va_arg(ap,int *));
	nullpo_retr(0, flag = va_arg(ap,int));

	cnt[3]++; // �͈͓��̃I�u�W�F�N�g��

	if( smd->bl.id == bl->id )
		return 0; // self

	dist = unit_distance(smd->bl.x,smd->bl.y,bl->x,bl->y);

	// �A�N�e�B�u
	if( (flag & 1) && (bl->type & BL_CHAR) ) {
		int range = (smd->sc.data[SC_BLIND].timer != -1 || smd->sc.data[SC_FOGWALLPENALTY].timer != -1)? 1: 10;

		// �^�[�Q�b�g�˒����ɂ���Ȃ�A���b�N����
		if(dist <= range && battle_check_target(&smd->bl,bl,BCT_ENEMY) >= 1 && mob_can_lock(smd,bl) ) {
			// �ː��`�F�b�N
			cell_t cell_flag = (mob_db[smd->class_].range > 6 ? CELL_CHKWALL : CELL_CHKNOPASS);
			if( path_search_long_real(NULL,smd->bl.m,smd->bl.x,smd->bl.y,bl->x,bl->y,cell_flag) &&
			    mob_can_reach(smd,bl,range) &&
			    atn_rand()%1000 < 1000/(++cnt[0]) )	// �͈͓�PC�œ��m���ɂ���
			{
				smd->target_id = bl->id;
				smd->min_chase = 13;
			}
		}
	}
	// ���[�g
	if( (flag & 2) && bl->type == BL_ITEM && smd->target_id < MAX_FLOORITEM && dist < 9 ) {
		if( mob_can_reach(smd,bl,12) && atn_rand()%1000 < 1000/(++cnt[1]) ) {	// �͈͓��A�C�e���œ��m���ɂ���
			smd->target_id = bl->id;
			smd->min_chase = 13;
		}
	}
	// �����N
	if( (flag & 4) && bl->type == BL_MOB ) {
		struct mob_data *tmd = (struct mob_data *)bl;
		if( tmd && tmd->class_ == smd->class_ && !tmd->target_id && dist < 13 ) {
			tmd->target_id = smd->target_id;
			tmd->min_chase = 13;
		}
	}

	return 0;
}

/*==========================================
 * ��芪�������X�^�[�̏���
 *------------------------------------------
 */
static int mob_ai_sub_hard_slavemob(struct mob_data *md,unsigned int tick)
{
	struct mob_data *mmd = NULL;
	struct block_list *bl;
	int old_dist;

	nullpo_retr(0, md);

	if((bl = map_id2bl(md->master_id)) == NULL || unit_isdead(bl)) {	// �傪���S���Ă��邩������Ȃ�
		if(md->state.special_mob_ai > 0)
			unit_remove_map(&md->bl,3,0);
		else
			unit_remove_map(&md->bl,1,0);
		return 0;
	}
	if(md->state.special_mob_ai > 0)	// �傪PC�̏ꍇ�́A�ȍ~�̏����͗v��Ȃ�
		return 0;

	if(bl->type == BL_MOB)
		mmd = (struct mob_data *)bl;	// ��̏��

	// ��ł͂Ȃ�
	if(!mmd || mmd->bl.id != md->master_id)
		return 0;
	// �Ăі߂�
	if(mmd->state.recall_flag == 1) {
		if(mmd->recallcount < mmd->recallmob_count + 2) {
			int dx = atn_rand()%5-2+mmd->bl.x;
			int dy = atn_rand()%5-2+mmd->bl.y;
			mob_warp(md,-1,dx,dy,3);
			mmd->recallcount += 1;
		} else {
			mmd->state.recall_flag = 0;
			mmd->recallcount = 0;
		}
		md->state.master_check = 1;
		return 0;
	}
	// �傪�Ⴄ�}�b�v�ɂ���̂Ńe���|�[�g���Ēǂ�������
	if(mmd->bl.m != md->bl.m) {
		mob_warp(md,mmd->bl.m,mmd->bl.x,mmd->bl.y,3);
		md->state.master_check = 1;
		return 0;
	}

	// ��Ƃ̋����𑪂�
	old_dist = md->master_dist;
	md->master_dist = unit_distance(md->bl.x,md->bl.y,mmd->bl.x,mmd->bl.y);

	// ���O�܂Ŏ傪�߂��ɂ����̂Ńe���|�[�g���Ēǂ�������
	if(old_dist < 10 && md->master_dist > 18) {
		mob_warp(md,-1,mmd->bl.x,mmd->bl.y,3);
		md->state.master_check = 1;
		return 0;
	}

	// �傪���邪�A���������̂ŋߊ��
	if(!md->target_id && unit_can_move(&md->bl) && !unit_isrunning(&md->bl) && md->ud.walktimer == -1 && md->master_dist < 15) {
		int i = 0, dx, dy, ret;
		if(md->master_dist > 2) {
			do {
				if(i <= 2) {
					dx = atn_rand()%5-2+mmd->bl.x - md->bl.x;
					dy = atn_rand()%5-2+mmd->bl.y - md->bl.y;
				} else {
					dx = mmd->bl.x - md->bl.x + atn_rand()%5 - 2;
					dy = mmd->bl.y - md->bl.y + atn_rand()%5 - 2;
				}
				ret = unit_walktoxy(&md->bl,md->bl.x+dx,md->bl.y+dy);
				i++;
			} while(ret == 0 && i < 5);
		} else {
			do {
				dx = atn_rand()%5 - 2;
				dy = atn_rand()%5 - 2;
				if(dx == 0 && dy == 0) {
					dx = (atn_rand()%2)? 1: -1;
					dy = (atn_rand()%2)? 1: -1;
				}
				dx += mmd->bl.x;
				dy += mmd->bl.y;
				ret = unit_walktoxy(&md->bl,mmd->bl.x+dx,mmd->bl.y+dy);
				i++;
			} while(ret == 0 && i < 5);
		}
		md->next_walktime = tick + 500;
		md->state.master_check = 1;
	}

	// �傪���āA�傪���b�N���Ă��Ď����̓��b�N���Ă��Ȃ�
	if(mmd->target_id > 0 && !md->target_id) {
		struct block_list *tbl = map_id2bl(mmd->target_id);
		if(tbl && (tbl->type != BL_ITEM || md->mode & 0x02) && mob_can_lock(md,tbl)) {
			md->target_id = tbl->id;
			md->min_chase = 5 + unit_distance(md->bl.x,md->bl.y,tbl->x,tbl->y);
			md->state.master_check = 1;
		}
	}

	return 0;
}

/*==========================================
 * ���b�N���~�߂đҋ@��ԂɈڂ�B
 *------------------------------------------
 */
int mob_unlocktarget(struct mob_data *md,unsigned int tick)
{
	nullpo_retr(0, md);

	md->target_id = 0;
	md->ud.attacktarget_lv = 0;
	md->state.skillstate = MSS_IDLE;
	md->next_walktime = tick + atn_rand()%3000 + 3000;
	return 0;
}

/*==========================================
 * �����_�����s
 *------------------------------------------
 */
static int mob_randomwalk(struct mob_data *md,unsigned int tick)
{
	const int retrycount = 20;

	nullpo_retr(0, md);

	if(DIFF_TICK(md->next_walktime,tick) < 0) {
		int i, x, y, c;
		int d = 12 - md->move_fail_count;
		int speed = status_get_speed(&md->bl);

		if(d < 4) d = 4;
		if(d > 6) d = 6;

		for(i=0; i<retrycount; i++) {	// �ړ��ł���ꏊ�̒T��
			int r = atn_rand();
			x = md->bl.x + r%(d*2+1) - d;
			y = md->bl.y + r/(d*2+1)%(d*2+1) - d;
			if(map_getcell(md->bl.m,x,y,CELL_CHKPASS) && path_search_long_real(NULL,md->bl.m,md->bl.x,md->bl.y,x,y,CELL_CHKNOPASS)) {
				if(unit_walktoxy(&md->bl,x,y)) {
					md->move_fail_count = 0;
					break;
				}
			}
			if(i+1 >= retrycount) {
				md->move_fail_count++;
				if(md->move_fail_count > 1000) {
					if(battle_config.error_log)
						printf("MOB cant move. hold position %d, class = %d\n",md->bl.id,md->class_);
					md->move_fail_count = 0;
					md->ud.canmove_tick = gettick() + 60000;
					return 0;
				}
			}
		}
		for(i=c=0; i<md->ud.walkpath.path_len; i++) {	// ���̕��s�J�n�������v�Z
			if(md->ud.walkpath.path[i] & 1)
				c += speed*14/10;
			else
				c += speed;
		}
		md->next_walktime    = tick + atn_rand()%3000 + 3000 + c;
		md->state.skillstate = MSS_WALK;
		return 1;
	}
	return 0;
}

/*==========================================
 * PC���߂��ɂ���MOB��AI
 *------------------------------------------
 */
int mob_ai_sub_hard(struct mob_data *md,unsigned int tick)
{
	struct block_list *tbl = NULL;
	int dist = 0;
	int attack_type = 0;
	int mode, search_flag = 0;

	nullpo_retr(0, md);

	if( md->bl.prev == NULL )
		return 0;

	if(DIFF_TICK(tick,md->last_thinktime) < MIN_MOBTHINKTIME)
		return 0;
	md->last_thinktime = tick;

	// �U�������X�L���r����
	if( md->ud.attacktimer != -1 || md->ud.skilltimer != -1 )
		return 0;
	// ���s���͂R�����Ƃ�AI�������s��
	if( md->ud.walktimer != -1 && md->ud.walkpath.path_pos <= 1 )
		return 0;
	// ���S��
	if( md->bl.prev == NULL ) {
		if(DIFF_TICK(tick,md->next_walktime) > MIN_MOBTHINKTIME)
			md->next_walktime = tick;
		return 0;
	}

	mode = status_get_mode( &md->bl );

	// �ُ�
	if( (md->sc.opt1 > 0 && md->sc.opt1 != 6) || md->sc.data[SC_BLADESTOP].timer != -1 )
		return 0;

	if( md->target_id > 0 && (!(mode&0x80) || md->sc.data[SC_BLIND].timer != -1 || md->sc.data[SC_FOGWALLPENALTY].timer != -1) )
		md->target_id = 0;

	if( md->target_id <= 0 )
		md->ud.attacktarget_lv = 0;
	else
		tbl = map_id2bl(md->target_id);

	// �܂��U�����ꂽ���m�F�i�A�N�e�B�u�Ȃ�25%�̊m���Ń^�[�Q�b�g�ύX�j
	if( mode > 0 && md->attacked_id > 0 && (tbl == NULL || tbl->type == BL_ITEM || (mode&0x04 && atn_rand()%100 < 25)) )
	{
		struct block_list *abl = map_id2bl(md->attacked_id);
		struct map_session_data *asd = NULL;

		md->attacked_players = 0;

		if( abl == NULL ||
		    abl->prev == NULL ||
		    md->bl.m != abl->m ||
		    (abl->type == BL_PC && (asd = (struct map_session_data *)abl) && (asd->invincible_timer != -1 || pc_isinvisible(asd))) ||
		    (dist = unit_distance(md->bl.x,md->bl.y,abl->x,abl->y)) >= 32 ||
		    battle_check_target(&md->bl,abl,BCT_ENEMY) <= 0 ) {
			md->attacked_id = 0;
		}
		else if(md->attacked_id > 0) {
			// �����������ꍇ�̓^�Q��ύX���Ȃ�
			if(!md->target_id || dist < 3) {
				md->target_id   = md->attacked_id; // set target
				attack_type     = 1;
				md->attacked_id = 0;
				md->min_chase   = dist + 13;
				if(md->min_chase > 26)
					md->min_chase = 26;
			}
		}
	}

	md->state.master_check = 0;
	// ��芪�������X�^�[�̏���
	if(md->master_id > 0) {
		mob_ai_sub_hard_slavemob(md,tick);
		if(md->bl.prev == NULL)
			return 0; // �e�Ɠ����Ɏ���
	}

	// �����N / �A�N�e�B�� / ���[�g�����X�^�[�̌���
	// �A�N�e�B������
	if( md->target_id < MAX_FLOORITEM && mode&0x04 && !md->state.master_check && battle_config.monster_active_enable ) {
		search_flag |= 1;
	}

	// ���[�g�����X�^�[�̃A�C�e���T�[�`
	if( !md->target_id && mode&0x02 && !md->state.master_check ) {
		if( md->lootitem && (battle_config.monster_loot_type == 0 || md->lootitem_count < LOOTITEM_SIZE) ) {
			search_flag |= 2;
		}
	}

	// �����N����
	if( md->target_id > 0 && mode&0x08 && md->ud.attacktarget_lv > 0 ) {
		struct map_session_data *asd = map_id2sd(md->target_id);
		if(asd && asd->invincible_timer == -1 && !pc_isinvisible(asd)) {
			search_flag |= 4;
		}
	}

	if( search_flag ) {
		int count[4] = { 0, 0, 0, 0 };
		// �˒����A�N�e�B�u(12), ���[�g(12), �����N(12) �Ȃ̂ŁA
		// ����AREA_SIZE�ȏ�͈̔͂�F������K�v�͖���
		map_foreachinarea(mob_ai_sub_hard_search,md->bl.m,
						  md->bl.x-AREA_SIZE,md->bl.y-AREA_SIZE,
						  md->bl.x+AREA_SIZE,md->bl.y+AREA_SIZE,
						  (BL_CHAR|BL_ITEM),md,count,search_flag);
		search_flag = count[3] + 20; // �͈͓��̃I�u�W�F�N�g��
	}

	if( md->target_id <= 0 || (tbl = map_id2bl(md->target_id)) == NULL ||
	    tbl->m != md->bl.m || tbl->prev == NULL ||
	    (dist = unit_distance(md->bl.x,md->bl.y,tbl->x,tbl->y)) >= md->min_chase )
	{
		// �Ώۂ����Ȃ� / �ǂ����ɏ����� / ���E�O
		if(md->target_id > 0) {
			mob_unlocktarget(md,tick);
			if(md->ud.walktimer != -1)
				unit_stop_walking(&md->bl,5);	// ���s���Ȃ��~
			return search_flag;
		}
	} else if(tbl->type == BL_PC || tbl->type == BL_MOB || tbl->type == BL_HOM || tbl->type == BL_MERC) {
		// �Ώۂ�PC�AMOB�AHOM��������MERC
		if(!mob_can_lock(md,tbl)) {
			// �X�L���Ȃǂɂ����G�W�Q����
			mob_unlocktarget(md,tick);
		} else if(!battle_check_range(&md->bl,tbl,mob_db[md->class_].range)) {
			// �U���͈͊O�Ȃ̂ňړ�
			if(!(mode&1)) {	// �ړ����Ȃ����[�h
				mob_unlocktarget(md,tick);
				mobskill_use(md,tick,-1);
				return search_flag;
			}
			if( !unit_can_move(&md->bl) || unit_isrunning(&md->bl) ) {	// �����Ȃ���Ԃɂ���
				if(DIFF_TICK(md->ud.canmove_tick,tick) <= 0)	// �_���[�W�f�B���C���łȂ��Ȃ�X�L���g�p
					mobskill_use(md,tick,-1);
				return search_flag;
			}
			md->state.skillstate = MSS_CHASE;	// �ˌ����X�L��
			mobskill_use(md,tick,-1);
			if(md->ud.walktimer != -1 && unit_distance(md->ud.to_x,md->ud.to_y,tbl->x,tbl->y) < 2)
				return search_flag; // ���Ɉړ���
			if( !mob_can_reach(md,tbl,(md->min_chase > 13)? md->min_chase: 13) ) {
				mob_unlocktarget(md,tick);	// �ړ��ł��Ȃ��̂Ń^�Q�����iIW�Ƃ��H�j
			} else {
				// �ǐ�
				int dx, dy, ret, i = 0;
				do {
					if(i == 0) {
						// �ŏ���AEGIS�Ɠ������@�Ō���
						dx = tbl->x - md->bl.x;
						dy = tbl->y - md->bl.y;
						if(dx < 0) dx++; else if(dx > 0) dx--;
						if(dy < 0) dy++; else if(dy > 0) dy--;
					} else {
						// ���߂Ȃ�Athena��(�����_��)
						dx = tbl->x - md->bl.x + atn_rand()%3 - 1;
						dy = tbl->y - md->bl.y + atn_rand()%3 - 1;
					}
					ret = unit_walktoxy(&md->bl,md->bl.x+dx,md->bl.y+dy);
					i++;
				} while(ret == 0 && i < 5);

				if(ret == 0) { // �ړ��s�\�ȏ�����̍U���Ȃ�2������
					if(dx < 0) dx = 2; else if(dx > 0) dx = -2;
					if(dy < 0) dy = 2; else if(dy > 0) dy = -2;
					unit_walktoxy(&md->bl,md->bl.x+dx,md->bl.y+dy);
				}
			}
		} else {
			// �U���˒��͈͓�
			if(md->ud.walktimer != -1)
				unit_stop_walking(&md->bl,1);	// ���s���Ȃ��~
			if(md->ud.attacktimer != -1 || md->ud.canact_tick > gettick())
				return search_flag; // ���ɍU����
			if(battle_config.mob_attack_fixwalkpos)	// �����ʒu�␳
				clif_fixwalkpos(&md->bl);
			unit_attack(&md->bl, md->target_id, attack_type);
			md->state.skillstate = MSS_ATTACK;
		}
		return search_flag;
	} else if(tbl->type == BL_ITEM && md->lootitem) {
		if(dist > 0) {
			// �A�C�e���܂ňړ�
			if(!(mode&1)) {	// �ړ��s�����X�^�[
				mob_unlocktarget(md,tick);
				return search_flag;
			}
			if( !unit_can_move(&md->bl) || unit_isrunning(&md->bl) )	// �����Ȃ���Ԃɂ���
				return search_flag;
			md->state.skillstate = MSS_LOOT;	// ���[�g���X�L���g�p
			mobskill_use(md,tick,-1);
			if(md->ud.walktimer != -1 && unit_distance(md->ud.to_x,md->ud.to_y,tbl->x,tbl->y) <= 0)
				return search_flag; // ���Ɉړ���
			if( !unit_walktoxy(&md->bl,tbl->x,tbl->y) )
				mob_unlocktarget(md,tick);// �ړ��ł��Ȃ��̂Ń^�Q�����iIW�Ƃ��H�j
		} else {
			// �A�C�e���܂ł��ǂ蒅����
			struct flooritem_data *fitem;

			if(md->ud.attacktimer != -1)
				return search_flag; // �U����
			if(md->ud.walktimer != -1)
				unit_stop_walking(&md->bl,1);	// ���s���Ȃ��~
			fitem = (struct flooritem_data *)tbl;
			if(md->lootitem_count < LOOTITEM_SIZE) {
				memcpy(&md->lootitem[md->lootitem_count++],&fitem->item_data,sizeof(md->lootitem[0]));
			} else if(battle_config.monster_loot_type == 1 && md->lootitem_count >= LOOTITEM_SIZE) {
				mob_unlocktarget(md,tick);
				return search_flag;
			} else {
				if(md->lootitem[0].card[0] == (short)0xff00)
					intif_delete_petdata(*((long *)(&md->lootitem[0].card[1])));
				memmove(&md->lootitem[0],&md->lootitem[1],sizeof(md->lootitem[0])*(LOOTITEM_SIZE-1));
				memcpy(&md->lootitem[LOOTITEM_SIZE-1],&fitem->item_data,sizeof(md->lootitem[0]));
			}
			if(mob_is_pcview(md->class_)) {
				int delay = tick + status_get_amotion(&md->bl);
				clif_takeitem(&md->bl,fitem->bl.id);
				md->ud.canact_tick  = delay;
				md->ud.canmove_tick = delay;
			}
			map_clearflooritem(tbl->id);
			mob_unlocktarget(md,tick);
		}
		return search_flag;
	} else {
		// �U���ΏۈȊO���^�[�Q�b�e�B���O�����i�o�O
		if(battle_config.error_log)
			printf("mob_ai_sub_hard target type error (type = 0x%03x, id = %d)\n",tbl->type,tbl->id);
		if(md->target_id > 0) {
			mob_unlocktarget(md,tick);
			if(md->ud.walktimer != -1)
				unit_stop_walking(&md->bl,5);	// ���s���Ȃ��~
			return search_flag;
		}
	}

	// ���s��/�ҋ@���X�L���g�p
	if( mobskill_use(md,tick,-1) )
		return search_flag;

	// ���s����
	if( mode&1 && unit_can_move(&md->bl) && !unit_isrunning(&md->bl) &&		// �ړ��\MOB&�������Ԃɂ���
	    (md->master_id == 0 || md->state.special_mob_ai || md->master_dist > 10) )	// ��芪��MOB����Ȃ�
	{
		if( DIFF_TICK(md->next_walktime,tick) > 7000 && md->ud.walktimer == -1 ) {
			md->next_walktime = tick + atn_rand()%2000 + 1000;
		}

		// �����_���ړ�
		if( mob_randomwalk(md,tick) )
			return search_flag;
	}

	// �����I����Ă�̂őҋ@
	if( md->ud.walktimer == -1 )
		md->state.skillstate = MSS_IDLE;

	return search_flag;
}

/*==========================================
 * ���G���[�e�B���p�T�u�֐��Q
 *     PC�̎��E���ɂ���MOB �̈ꗗ�������N���X�g�ɂ���ĕێ�����
 *------------------------------------------
 */
static struct mob_data*  mob_ai_hard_head;
static int               mob_ai_hard_count;
static struct mob_data** mob_ai_hard_buf;
static int               mob_ai_hard_max;
// ������ID�ꎞ�ۑ��p
static int *             mob_ai_hard_next_id;
static int               mob_ai_hard_next_max;
static int               mob_ai_hard_next_count;
// ���j�^�����O�p
static int               mob_ai_hard_graph1; // ������
static int               mob_ai_hard_graph2; // �^�C�}�[�Ăяo����

/*==========================================
 * PC���E����MOB �ړ�
 *------------------------------------------
 */
int mob_ai_hard_add(struct mob_data *md)
{
	nullpo_retr(0, md);

	if( md->bl.prev == NULL )
		return 0;
	if( mob_ai_hard_head == NULL ) {
		// �擪�ɒǉ�
		mob_ai_hard_head = md;
	} else {
		// ���X�g�A��
		md->ai_next = mob_ai_hard_head;
		mob_ai_hard_head->ai_prev = md;
		mob_ai_hard_head = md;
	}
	mob_ai_hard_count++;
	return 0;
}

/*==========================================
 * PC���E�O��MOB �ړ�
 *------------------------------------------
 */
int mob_ai_hard_del(struct mob_data *md)
{
	nullpo_retr(0, md);

	if( md->bl.prev == NULL )
		return 0;
	if( mob_ai_hard_head == md ) {
		// �擪����폜
		mob_ai_hard_head = md->ai_next;
		if( mob_ai_hard_head )
			mob_ai_hard_head->ai_prev = NULL;
	} else {
		// �r�����甲����
		if( md->ai_prev ) md->ai_prev->ai_next = md->ai_next;
		if( md->ai_next ) md->ai_next->ai_prev = md->ai_prev;
	}
	md->ai_prev = NULL;
	md->ai_next = NULL;
	mob_ai_hard_count--;
	if( mob_ai_hard_count < 0 ) {
		printf("mob_ai_hard_del: mob_ai_hard_count < 0\n");
		mob_ai_hard_count = 0;
	}
	return 0;
}

/*==========================================
 * PC���E���ւ�MOB �̏o����(map_foreach*** �n���̃R�[���o�b�N�֐�)
 *------------------------------------------
 */
int mob_ai_hard_spawn_sub(struct block_list *tbl, va_list ap)
{
	int flag;
	struct block_list *sbl;
	struct mob_data *md;

	sbl  = va_arg(ap, struct block_list*);
	flag = va_arg(ap, int);

	if( (sbl->type & (BL_PC | BL_HOM | BL_MERC)) && tbl->type == BL_MOB && (md = (struct mob_data *)tbl) ) {
		if( flag ) {
			if( md->ai_pc_count++ == 0 )
				mob_ai_hard_add( md );
		} else {
			if( --md->ai_pc_count == 0 )
				mob_ai_hard_del( md );
		}
	}
	if( sbl->type == BL_MOB && (tbl->type & (BL_PC | BL_HOM | BL_MERC)) && (md = (struct mob_data *)sbl) ) {
		if( flag ) {
			if( md->ai_pc_count++ == 0 )
				mob_ai_hard_add( md );
		} else {
			if( --md->ai_pc_count == 0 )
				mob_ai_hard_del( md );
		}
	}

	return 0;
}

/*==========================================
 * MOB ��PC�̏o���E���ŏ���( flag = 0: ���ŁA1: �o�� )
 *------------------------------------------
 */
int mob_ai_hard_spawn( struct block_list *bl, int flag )
{
	nullpo_retr(0, bl);

	if( bl->type == BL_PC || bl->type == BL_MOB || bl->type == BL_HOM || bl->type == BL_MERC ) {
		map_foreachinarea( mob_ai_hard_spawn_sub , bl->m,
			bl->x - AREA_SIZE * 2, bl->y - AREA_SIZE * 2,
			bl->x + AREA_SIZE * 2, bl->y + AREA_SIZE * 2,
			(bl->type == BL_MOB ? BL_PC|BL_HOM|BL_MERC : BL_MOB), bl, flag
		);
	}
	return 0;
}

/*==========================================
 * PC���E����mob�p�܂��ߏ��� (interval timer�֐�)
 *------------------------------------------
 */
static int mob_ai_hard_createlist(void)
{
	int i;
	struct mob_data *md = mob_ai_hard_head;

	if( mob_ai_hard_count == 0 )
		return 0;

	if( mob_ai_hard_count >= mob_ai_hard_max ) {
		mob_ai_hard_max = (mob_ai_hard_count + 255) & ~255;
		mob_ai_hard_buf = (struct mob_data **)aRealloc(mob_ai_hard_buf, mob_ai_hard_max * sizeof(struct mob_data*));
	}
	for(i = 0; md && i < mob_ai_hard_count; i++) {
		mob_ai_hard_buf[i] = md;
		md = md->ai_next;
	}
	return i;
}

static int mob_ai_hard(int tid,unsigned int tick,int id,int data)
{
	unsigned int limit = MIN_MOBTHINKTIME * battle_config.mob_ai_cpu_usage / 100;

	map_freeblock_lock();
	mob_ai_hard_graph2++; // �^�C�}�[�Ăяo����

	if( battle_config.mob_ai_limiter == 0 ) {
		// ���~�b�^�[����
		int i;
		int max = mob_ai_hard_createlist();
		for(i = 0; i < max; i++) {
			mob_ai_sub_hard(mob_ai_hard_buf[i], tick);
		}
		mob_ai_hard_graph1++; // �����ςݐ�
	} else if( gettick() - tick <= limit ) { // �������Ԃɗ]�T�����邩����
		// ���~�b�^�[�L��
		int i, j = 0;

		// �O��̂��c���̏���
		while( mob_ai_hard_next_count > 0 ) {
			struct mob_data *md = map_id2md( mob_ai_hard_next_id[--mob_ai_hard_next_count] );
			if(md == NULL || md->bl.prev == NULL)
				continue;
			j += mob_ai_sub_hard( md, tick ) + 5;
			if( j > 10000 ) {
				j -= 10000;
				if( gettick_nocache() - tick > limit ) break;
			}
		}
		// ���c���̏������I�����̂ŁA�����p���X�g���U����
		if( mob_ai_hard_next_count == 0 ) {
			int max = mob_ai_hard_createlist();
			mob_ai_hard_graph1++; // �����ςݐ�
			for(i = 0; i < max; i++) {
				j += mob_ai_sub_hard(mob_ai_hard_buf[i], tick) + 5;
				if( j > 10000 ) {
					j -= 10000;
					if( gettick_nocache() - tick > limit ) break;
				}
			}
			if( i != max ) {
				// ���c����ID��ۑ�����
				int k;
				mob_ai_hard_next_count = max - i;
				if( mob_ai_hard_next_count >= mob_ai_hard_next_max ) {
					mob_ai_hard_next_max = (mob_ai_hard_next_count + 255) & ~255;
					mob_ai_hard_next_id  = (int *)aRealloc(mob_ai_hard_next_id, mob_ai_hard_next_max * sizeof(int));
				}
				for(k = 0; i < max; i++, k++) {
					mob_ai_hard_next_id[k] = mob_ai_hard_buf[i]->bl.id;
				}
			}
		}
	}
	map_freeblock_unlock();
	return 0;
}

/*==========================================
 * mob �̃��j�^�����O
 *------------------------------------------
 */
double mob_ai_hard_sensor(void)
{
	double ret = 100.0f;

	if( mob_ai_hard_graph2 > 0 && mob_ai_hard_graph1 <= mob_ai_hard_graph2 ) {
		ret = (double)mob_ai_hard_graph1 * 100.0 / mob_ai_hard_graph2;
	}
	mob_ai_hard_graph1 = 0;
	mob_ai_hard_graph2 = 0;
	return ret;
}

/*==========================================
 * �蔲�����[�hMOB AI�i�߂���PC�����Ȃ��j
 *------------------------------------------
 */
static int mob_ai_sub_lazy(void * key,void * data,va_list ap)
{
	struct block_list *bl = (struct block_list *)data;
	struct mob_data *md = NULL;
	unsigned int tick = 0;

	nullpo_retr(0, bl);

	if(bl->type != BL_MOB)
		return 0;
	if((md = (struct mob_data *)bl) == NULL)
		return 0;
	if(md->ai_pc_count > 0)		// PC�̋߂��ɂ���̂Ŏ蔲�������͂��Ȃ�
		return 0;

	tick = va_arg(ap,unsigned int);

	if(DIFF_TICK(tick,md->last_thinktime) < MIN_MOBTHINKTIME * 20)
		return 0;

	md->last_thinktime = tick;
	if(md->target_id)
		mob_unlocktarget(md,tick);

	if(md->bl.prev == NULL || md->ud.skilltimer != -1) {
		if(DIFF_TICK(tick,md->next_walktime) > MIN_MOBTHINKTIME * 20)
			md->next_walktime = tick;
		return 0;
	}

	// ��芪�������X�^�[�̏����i�Ăі߂����ꂽ���j
	if( md->master_id > 0 && md->state.special_mob_ai == 0 ) {
		struct mob_data *mmd = map_id2md(md->master_id);

		if(mmd && mmd->state.recall_flag == 1) {
			mob_ai_sub_hard_slavemob(md,tick);
			return 0;
		}
	}

	if( DIFF_TICK(md->next_walktime,tick) < 0 && unit_can_move(&md->bl) && !unit_isrunning(&md->bl) )
	{
		int mode = mob_db[md->class_].mode;
		if( map[md->bl.m].users > 0 ) {
			// �����}�b�v��PC, HOM, MERC������̂ŁA�����܂��Ȏ蔲������������

			if( (mode&1) && atn_rand()%1000 < MOB_LAZYMOVEPERC ) {
				// ���X�ړ�����
				mob_randomwalk(md,tick);
			}
			else if( MOB_LAZYSKILLUSEPERC > 0 && atn_rand()%1000 < MOB_LAZYSKILLUSEPERC ) {
				// ���X�X�L�����g��
				mobskill_use(md,tick,-1);
			}
		} else {
			// �����}�b�v�ɂ���PC�����Ȃ��̂ŁA�Ƃ��Ă��K���ȏ���������
			// ����MOB�łȂ��ABOSS�ł��Ȃ�MOB�͏ꍇ�A���X���[�v����
			if( (mode&1) && !(mode & 0x20) && atn_rand()%1000 < MOB_LAZYWARPPERC && md->x0 <= 0 && !md->master_id && mob_db[md->class_].mexp <= 0 ) {
				mob_warp(md,-1,-1,-1,-1);
			}
		}
		md->next_walktime = tick + atn_rand()%10000 + 5000;
	}
	return 0;
}

/*==========================================
 * PC���E�O��mob�p�蔲������ (interval timer�֐�)
 *------------------------------------------
 */
static int mob_ai_lazy(int tid,unsigned int tick,int id,int data)
{
	map_foreachiddb(mob_ai_sub_lazy,tick);

	return 0;
}

/*==========================================
 * delay�t��item drop (timer�֐�)
 *------------------------------------------
 */
static int mob_delay_item_drop(int tid,unsigned int tick,int id,int data)
{
	struct delay_item_drop *ditem;
	struct item temp_item;

	nullpo_retr(0, ditem = (struct delay_item_drop *)id);

	memset(&temp_item, 0, sizeof(temp_item));
	temp_item.nameid   = ditem->nameid;
	temp_item.amount   = ditem->amount;
	temp_item.identify = !itemdb_isequip3(temp_item.nameid);
	if(battle_config.itemidentify)
		temp_item.identify = 1;

	if(ditem->first_id > 0) {
		struct map_session_data *sd = map_id2sd(ditem->first_id);
		if(sd && sd->bl.prev && sd->state.autoloot && !unit_isdead(&sd->bl) && sd->bl.m == ditem->m) {
			int flag;
			struct party *p = NULL;

			if(sd->status.party_id > 0) {
				p = party_search(sd->status.party_id);
			}
			if((flag = party_loot_share(p, sd, &temp_item, sd->bl.id)) != 0) {
				clif_additem(sd,0,0,flag);
			} else {
				// �擾����
				aFree(ditem);
				return 0;
			}
		}
	}

	map_addflooritem(
		&temp_item,temp_item.amount,ditem->m,ditem->x,ditem->y,
		ditem->first_id,ditem->second_id,ditem->third_id,0
	);
	aFree(ditem);

	return 0;
}

/*==========================================
 * delay�t��item drop (timer�֐�) - lootitem
 *------------------------------------------
 */
static int mob_delay_item_drop2(int tid,unsigned int tick,int id,int data)
{
	struct delay_item_drop2 *ditem;

	nullpo_retr(0, ditem = (struct delay_item_drop2 *)id);

	// �y�b�g�̗��Ȃ�h���b�v�f�B���C�L���[����pop����
	if(ditem->item_data.card[0] == (short)0xff00) {
		struct delay_item_drop2 *p = map_pop_delayitem_que();
		if(p != ditem)
			printf("mob_delay_item_drop2: que pop error!!\n");
	}

	if(ditem->first_id > 0) {
		struct map_session_data *sd = map_id2sd(ditem->first_id);
		if(sd && sd->bl.prev && sd->state.autoloot && !unit_isdead(&sd->bl) && sd->bl.m == ditem->m) {
			int flag;
			struct party *p = NULL;

			if(sd->status.party_id > 0) {
				p = party_search(sd->status.party_id);
			}
			if((flag = party_loot_share(p, sd, &ditem->item_data, sd->bl.id)) != 0) {
				clif_additem(sd,0,0,flag);
			} else {
				// �擾����
				aFree(ditem);
				return 0;
			}
		}
	}

	map_addflooritem(
		&ditem->item_data,ditem->item_data.amount,ditem->m,ditem->x,ditem->y,
		ditem->first_id,ditem->second_id,ditem->third_id,0
	);
	aFree(ditem);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int mob_timer_delete(int tid, unsigned int tick, int id, int data)
{
	struct block_list *bl = map_id2bl(id);

	nullpo_retr(0, bl);

	unit_remove_map(bl,3,0);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
static int mob_deleteslave_sub(struct block_list *bl,va_list ap)
{
	struct mob_data *md;
	int id;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md = (struct mob_data *)bl);

	id = va_arg(ap,int);

	if(md->master_id > 0 && md->master_id == id)
		unit_remove_map(&md->bl,1,0);
	return 0;
}

int mob_deleteslave(struct mob_data *md)
{
	nullpo_retr(0, md);

	map_foreachinarea(mob_deleteslave_sub, md->bl.m,
		0,0,map[md->bl.m].xs,map[md->bl.m].ys,
		BL_MOB,md->bl.id);
	return 0;
}

/*==========================================
 * md��damage�̃_���[�W
 *------------------------------------------
 */
int mob_damage(struct block_list *src,struct mob_data *md,int damage,int type)
{
	int max_hp;
	unsigned int tick = gettick();

	nullpo_retr(0, md);	// src��NULL�ŌĂ΂��ꍇ������̂ŁA���Ń`�F�b�N

	if(md->bl.prev == NULL) {
		if(battle_config.error_log)
			printf("mob_damage : BlockError!!\n");
		return 0;
	}

	if(md->hp <= 0) {
		if(md->bl.prev != NULL) {
			mobskill_use(md,tick,-1);	// ���S���X�L��
			unit_remove_map(&md->bl,1,0);
		}
		return 0;
	}

	max_hp = status_get_max_hp(&md->bl);

	if((md->sc.data[SC_ENDURE].timer == -1 || map[md->bl.m].flag.gvg) && md->sc.data[SC_BERSERK].timer == -1)
		unit_stop_walking(&md->bl,3);
	if(damage > max_hp>>2)
		skill_stop_dancing(&md->bl,0);

	if(md->hp > max_hp)
		md->hp = max_hp;

	// over kill���͊ۂ߂�
	if(damage > md->hp)
		damage = md->hp;

	if(!(type&2) && src)
	{
		int damage2 = 0;
		int id = 0;

		// �_���[�W��^�����l�ƌl�݌v�_���[�W��ۑ�(Exp�v�Z�p)
		if(src->type == BL_PC) {
			struct map_session_data *src_sd = (struct map_session_data *)src;
			if(src_sd)
			{
				damage2 = (int)linkdb_search( &md->dmglog, (void*)src_sd->status.char_id );
				damage2 += (damage2 == -1)? damage + 1: damage; // �搧���󂯂Ă����ꍇ-1�Ő퓬�Q���҂ɓo�^����Ă���
				if(damage2 <= 0)
					damage2 = -1;
				linkdb_replace( &md->dmglog, (void*)src_sd->status.char_id, (void*)damage2 );
				id = src_sd->bl.id;
			}
		} else if(src->type == BL_PET) {
			struct pet_data *src_pd = (struct pet_data *)src;
			if(src_pd && src_pd->msd && battle_config.pet_attack_exp_to_master)
			{
				damage2 = damage * battle_config.pet_attack_exp_rate/100;
				damage2 += (int)linkdb_search( &md->dmglog, (void*)src_pd->msd->status.char_id );
				linkdb_replace( &md->dmglog, (void*)src_pd->msd->status.char_id, (void*)damage2 );
				id = 0;
			}
		} else if(src->type == BL_MOB) {
			struct mob_data *src_md = (struct mob_data *)src;
			if(src_md && src_md->state.special_mob_ai)
			{
				struct map_session_data *msd = map_id2sd(src_md->master_id);
				// msd��NULL�̂Ƃ��̓_���[�W���O�ɋL�^���Ȃ�
				if(msd) {
					damage2 = damage + (int)linkdb_search( &md->dmglog, (void*)msd->status.char_id );
					linkdb_replace( &md->dmglog, (void*)msd->status.char_id, (void*)damage2 );
					id = src_md->master_id;
				}
			}
		} else if(src->type == BL_HOM || src->type == BL_MERC) {
			// �z���E�b���̏ꍇ��ID�𕉂ɔ��]����
			damage2 = damage;
			damage2 += (int)linkdb_search( &md->dmglog, (void*)-src->id );
			linkdb_replace( &md->dmglog, (void*)-src->id, (void*)damage2 );
			id = src->id;
		}

		// �^�[�Q�b�g�̕ύX
		if( md->attacked_id <= 0 && md->state.special_mob_ai == 0 && id > 0 && atn_rand()%1000 < 1000 / (++md->attacked_players) )
			md->attacked_id = id;
	}

	md->hp -= damage;

	// �n�C�h��Ԃ�����
	status_change_hidden_end(&md->bl);

	if(md->state.special_mob_ai == 2) {	// �X�t�B�A�[�}�C��
		int skillidx = mob_skillid2skillidx(md->class_,NPC_SELFDESTRUCTION2);
		// �����r���J�n
		if(skillidx >= 0 && mobskill_use_id(md,&md->bl,skillidx)) {
			// �������������ɂ���Ċ��ɏ��ł��Ă���\��������̂ŕK���`�F�b�N����
			if(md && md->hp > 0 && md->bl.prev != NULL) {
				md->dir = map_calc_dir(src,md->bl.x,md->bl.y);
				md->mode |= 0x1;
				md->state.special_mob_ai++;
				status_change_start(&md->bl,SC_SELFDESTRUCTION,0,0,0,md->dir,0,0);
			}
		}
	}

	if(md->hp <= 0) {
		// ���S����
		map_freeblock_lock();
		mob_dead(src, md, type, tick);
		map_freeblock_unlock();
	}
	return 0;
}

/*==========================================
 * md�̎��S����
 *------------------------------------------
 */
static int mob_dead(struct block_list *src,struct mob_data *md,int type,unsigned int tick)
{
	int i,count = 0;
	int mvp_damage = 0;
	int drop_rate;
	atn_bignumber tdmg = 0;
	struct map_session_data *sd = NULL;
	struct block_list **tmpbl = NULL;
	struct linkdb_node *node;

	struct party_exp {
		struct party *p;
		int id;
		atn_bignumber base_exp,job_exp;
	} *pt = NULL;
	struct {
		struct block_list *bl;
		int dmg;
	} mvp[3];

	nullpo_retr(0, md);	// src��NULL�ŌĂ΂��ꍇ������̂ŁA���Ń`�F�b�N

	mobskill_use(md,tick,-1);	// ���S���X�L��

	memset(mvp,0,sizeof(mvp));

	if(src) {
		if(src->type == BL_PC)
			sd = (struct map_session_data *)src;
		else if(src->type == BL_MOB)
			mob_unlocktarget((struct mob_data *)src,tick);
	}

	if(sd) {
		int hp = 0,sp = 0;
		int race_id = status_get_race(&md->bl);

		// �J�[�h�ɂ�鎀�S��HPSP�z������
		sp += sd->sp_gain_value;
		hp += sd->hp_gain_value;
		if(atn_rand()%100 < sd->hp_drain_rate_race[race_id])
			hp = sd->hp_drain_value_race[race_id];

		if(atn_rand()%100 < sd->sp_drain_rate_race[race_id])
			sp = sd->sp_drain_value_race[race_id];

		if(hp || sp)
			pc_heal(sd,hp,sp);

		// �e�R���~�b�V�����^�[�Q�b�g
		if(sd->status.class_ == PC_CLASS_TK && md->class_ == sd->tk_mission_target) {
			ranking_gain_point(sd,RK_TAEKWON,1);
			ranking_setglobalreg(sd,RK_TAEKWON);
			ranking_update(sd,RK_TAEKWON);
			if(ranking_get_point(sd,RK_TAEKWON)%100 == 0)
			{
				int c = 0;
				while(c++ < 1000) {
					sd->tk_mission_target = atn_rand()%(MOB_ID_MAX-MOB_ID_MIN) + MOB_ID_MIN;
					if(mob_db[sd->tk_mission_target].max_hp <= 0)
						continue;
					if(mob_db[sd->tk_mission_target].summonper[0] == 0)	// �}�ŌĂ΂�Ȃ��̂͏��O
						continue;
					if(mob_db[sd->tk_mission_target].mode & 0x20)	// �{�X�������O
						continue;
					break;
				}
				if(c >= 1000)
					sd->tk_mission_target = 0;
				pc_setglobalreg(sd,"PC_MISSION_TARGET",sd->tk_mission_target);
				clif_mission_mob(sd,sd->tk_mission_target,0);
			}
		}
	}

	// map�O�ɏ������l�͌v�Z���珜���̂�
	// overkill���͖�������sum��max_hp�Ƃ͈Ⴄ
	i    = 0;
	node = md->dmglog;
	while( node ) {
		node = node->next;
		i++;
	}
	if( i > 0 ) {
		pt    = (struct party_exp *)aCalloc(i, sizeof(*pt));
		tmpbl = (struct block_list **)aCalloc(i, sizeof(*tmpbl));
	}
	node = md->dmglog;

	for(i=0; node; node = node->next,i++) {
		int damage;
		int id = (int)node->key;
		if(id > 0) {
			struct map_session_data *tmpsd = map_charid2sd(id);
			if(tmpsd)
				tmpbl[i] = &tmpsd->bl;
		} else {
			tmpbl[i] = map_id2bl(-id);	// �z���E�b���̏ꍇ��ID�����ɔ��]����Ă���
		}
		if( !tmpbl[i] || (tmpbl[i]->type != BL_PC && tmpbl[i]->type != BL_HOM && tmpbl[i]->type != BL_MERC) ) {
			tmpbl[i] = NULL;
			continue;
		}
		count++;
		if(tmpbl[i]->m != md->bl.m || unit_isdead(tmpbl[i]))
			continue;

		damage = (int)node->data;
		if(damage > 0)
			tdmg += damage;	// �g�[�^���_���[�W
		if(mvp_damage < damage) {
			if( mvp[0].bl == NULL || damage > mvp[0].dmg ) {
				// ��ԑ傫���_���[�W
				mvp[2] = mvp[1];
				mvp[1] = mvp[0];
				mvp[0].bl  = tmpbl[i];
				mvp[0].dmg = damage;
				mvp_damage = (mvp[2].bl == NULL) ? 0 : mvp[2].dmg;
			} else if( mvp[1].bl == NULL || damage > mvp[1].dmg ) {
				// �Q�Ԗڂɑ傫���_���[�W
				mvp[2]     = mvp[1];
				mvp[1].bl  = tmpbl[i];
				mvp[1].dmg = damage;
				mvp_damage = (mvp[2].bl == NULL) ? 0 : mvp[2].dmg;
			} else {
				// �R�Ԗڂɑ傫���_���[�W
				mvp[2].bl  = tmpbl[i];
				mvp[2].dmg = damage;
				mvp_damage = damage;
			}
		}
	}

	// �o���l�̕��z
	if(!md->state.noexp && tdmg > 0)
	{
		int pnum = 0;
		int base_exp_rate, job_exp_rate, per;

		if(map[md->bl.m].flag.base_exp_rate)
			base_exp_rate = (map[md->bl.m].flag.base_exp_rate < 0)? 0: map[md->bl.m].flag.base_exp_rate;
		else
			base_exp_rate = battle_config.base_exp_rate;
		if(map[md->bl.m].flag.job_exp_rate)
			job_exp_rate  = (map[md->bl.m].flag.job_exp_rate < 0)? 0: map[md->bl.m].flag.job_exp_rate;
		else
			job_exp_rate  = battle_config.job_exp_rate;

		per = 100 + (count-1) * battle_config.joint_struggle_exp_bonus;
		if(battle_config.joint_struggle_limit && per > battle_config.joint_struggle_limit)
			per = battle_config.joint_struggle_limit;

		node = md->dmglog;
		for(i=0; node; node = node->next,i++) {
			int damage, rate, pid;
			atn_bignumber base_exp, job_exp;
			struct map_session_data *tmpsd = NULL;

			if(tmpbl[i] == NULL || tmpbl[i]->m != md->bl.m || unit_isdead(tmpbl[i]))
				continue;

			damage = (int)node->data;
			rate = (damage <= 0)? 0: per * damage / 100;

			if(base_exp_rate <= 0) {
				base_exp = 0;
			} else {
				base_exp = (rate <= 0)? 0: (atn_bignumber)mob_db[md->class_].base_exp * rate/tdmg * base_exp_rate/100;
				if(mob_db[md->class_].base_exp > 0 && base_exp < 1 && damage > 0)
					base_exp = 1;
				if(base_exp < 0)
					base_exp = 0;
			}
			if(job_exp_rate <= 0) {
				job_exp = 0;
			} else {
				job_exp = (rate <= 0)? 0: (atn_bignumber)mob_db[md->class_].job_exp * rate/tdmg * job_exp_rate/100;
				if(mob_db[md->class_].job_exp > 0 && job_exp < 1 && damage > 0)
					job_exp = 1;
				if(job_exp < 0)
					job_exp = 0;
			}

			if( tmpbl[i]->type == BL_HOM ) {
				struct homun_data *thd = (struct homun_data *)tmpbl[i];
				if(thd)
					homun_gainexp(thd, md, base_exp, job_exp);
			}
			else if( tmpbl[i]->type == BL_MERC ) {
				struct merc_data *tmcd = (struct merc_data *)tmpbl[i];
				if(tmcd)
					merc_gainexp(tmcd, md, base_exp, job_exp);
			}
			else if( tmpbl[i]->type == BL_PC ) {
				tmpsd = (struct map_session_data *)tmpbl[i];
			}
			if( !tmpsd )
				continue;

			if((pid = tmpsd->status.party_id) > 0) {	// �p�[�e�B�ɓ����Ă���
				int j;
				for(j = 0; j < pnum; j++) {	// �����p�[�e�B���X�g�ɂ��邩�ǂ���
					if(pt[j].id == pid)
						break;
				}
				if(j >= pnum) {	// ���Ȃ��Ƃ��͌������ǂ����m�F
					struct party *p = party_search(pid);
					if(p && p->exp != 0) {
						pt[pnum].id       = pid;
						pt[pnum].p        = p;
						pt[pnum].base_exp = base_exp;
						pt[pnum].job_exp  = job_exp;
						pnum++;
						continue;
					}
				} else {	// ����Ƃ��͌���
					pt[j].base_exp += base_exp;
					pt[j].job_exp  += job_exp;
					continue;
				}
			}
			if(base_exp > 0 || job_exp > 0)	// �e���擾
			{
				if( (tmpsd->sc.data[SC_TRICKDEAD].timer == -1 || !battle_config.noexp_trickdead) && 	// ���񂾂ӂ肵�Ă��Ȃ�
				    (tmpsd->sc.data[SC_HIDING].timer == -1    || !battle_config.noexp_hiding) )		// �n�C�h���Ă��Ȃ�
					pc_gainexp(tmpsd, md, base_exp, job_exp);
			}
		}
		// �������z
		for(i = 0; i < pnum; i++)
			party_exp_share(pt[i].p, md, pt[i].base_exp, pt[i].job_exp);
	}
	aFree( pt );
	aFree( tmpbl );

	// item drop
	if(!(type&1) && !map[md->bl.m].flag.nodrop) {
		if(!md->state.nodrop) {
			for(i=0; i<ITEM_DROP_COUNT; i++) {
				struct delay_item_drop *ditem;

				if(mob_db[md->class_].dropitem[i].nameid <= 0)
					continue;
				drop_rate = mob_droprate_fix( src, mob_db[md->class_].dropitem[i].nameid, mob_db[md->class_].dropitem[i].p );
				if(drop_rate <= 0 && battle_config.drop_rate0item)
					drop_rate = 1;
				if(drop_rate <= atn_rand()%10000)
					continue;

				ditem = (struct delay_item_drop *)aCalloc(1,sizeof(struct delay_item_drop));
				ditem->nameid    = mob_db[md->class_].dropitem[i].nameid;
				ditem->amount    = 1;
				ditem->m         = md->bl.m;
				ditem->x         = md->bl.x;
				ditem->y         = md->bl.y;
				ditem->first_id  = (mvp[0].bl)? mvp[0].bl->id: 0;
				ditem->second_id = (mvp[1].bl)? mvp[1].bl->id: 0;
				ditem->third_id  = (mvp[2].bl)? mvp[2].bl->id: 0;
				add_timer2(tick+500+i,mob_delay_item_drop,(int)ditem,0,TIMER_FREE_ID);
			}
		}
		if(sd) {
			for(i=0; i<sd->monster_drop_item_count; i++) {
				struct delay_item_drop *ditem;
				int race = status_get_race(&md->bl);
				int mode = status_get_mode(&md->bl);

				if(sd->monster_drop_itemrate[i] <= 0)
					continue;
				if(sd->monster_drop_race[i] & (1<<race) ||
				   (mode & 0x20 && sd->monster_drop_race[i] & 1<<RCT_BOSS) ||
				   (!(mode & 0x20) && sd->monster_drop_race[i] & 1<<RCT_NONBOSS) )
				{
					int itemid;
					if(sd->monster_drop_itemrate[i] <= atn_rand()%10000)
						continue;

					ditem = (struct delay_item_drop *)aCalloc(1,sizeof(struct delay_item_drop));
					if(sd->monster_drop_itemid[i] < 0) {
						itemid = itemdb_searchrandomid(-sd->monster_drop_itemid[i]);
						if(itemid <= 0)
							continue;
					} else {
						itemid = sd->monster_drop_itemid[i];
					}
					ditem->nameid    = itemid;
					ditem->amount    = 1;
					ditem->m         = md->bl.m;
					ditem->x         = md->bl.x;
					ditem->y         = md->bl.y;
					ditem->first_id  = (mvp[0].bl)? mvp[0].bl->id: 0;
					ditem->second_id = (mvp[1].bl)? mvp[1].bl->id: 0;
					ditem->third_id  = (mvp[2].bl)? mvp[2].bl->id: 0;
					add_timer2(tick+520+i,mob_delay_item_drop,(int)ditem,0,TIMER_FREE_ID);
				}
			}
			if(sd->get_zeny_num > 0)
				pc_getzeny(sd,mob_db[md->class_].lv*10 + atn_rand()%(sd->get_zeny_num+1));
			if(sd->get_zeny_num2 > 0 && atn_rand()%100 < sd->get_zeny_num2)
				pc_getzeny(sd,mob_db[md->class_].lv*10);
		}
		// �z�Δ�������
		if(sd && mvp[0].bl && (&sd->bl == mvp[0].bl) && pc_checkskill(sd, BS_FINDINGORE) > 0) {
			int rate = battle_config.finding_ore_drop_rate * battle_config.item_rate / 100;

			if (rate < 0)
				rate = 0;
			else if (rate > 10000)
				rate = 10000;
			if(rate > atn_rand() % 10000) {
				struct delay_item_drop *ditem;
				ditem = (struct delay_item_drop*)aCalloc(1, sizeof(struct delay_item_drop));
				ditem->nameid    = itemdb_searchrandomid(6);
				ditem->amount    = 1;
				ditem->m         = md->bl.m;
				ditem->x         = md->bl.x;
				ditem->y         = md->bl.y;
				ditem->first_id  = (mvp[0].bl)? mvp[0].bl->id: 0;
				ditem->second_id = (mvp[1].bl)? mvp[1].bl->id: 0;
				ditem->third_id  = (mvp[2].bl)? mvp[2].bl->id: 0;
				add_timer2(tick + 520 + i, mob_delay_item_drop, (int)ditem, 0, TIMER_FREE_ID);
			}
		}
		if(md->lootitem) {
			for(i=0; i<md->lootitem_count; i++) {
				struct delay_item_drop2 *ditem;

				ditem = (struct delay_item_drop2 *)aCalloc(1,sizeof(struct delay_item_drop2));
				memcpy(&ditem->item_data,&md->lootitem[i],sizeof(md->lootitem[0]));
				ditem->m         = md->bl.m;
				ditem->x         = md->bl.x;
				ditem->y         = md->bl.y;
				ditem->first_id  = (mvp[0].bl)? mvp[0].bl->id: 0;
				ditem->second_id = (mvp[1].bl)? mvp[1].bl->id: 0;
				ditem->third_id  = (mvp[2].bl)? mvp[2].bl->id: 0;
				ditem->next      = NULL;

				if(ditem->item_data.card[0] == (short)0xff00) {
					// �y�b�g�̗��̓h���b�v�f�B���C�L���[�ɕۑ�����
					map_push_delayitem_que(ditem);
					add_timer(tick+540,mob_delay_item_drop2,(int)ditem,0);
				} else {
					add_timer2(tick+540+i,mob_delay_item_drop2,(int)ditem,0,TIMER_FREE_ID);
				}
			}
			md->lootitem_count = 0;
		}
	}

	// mvp����
	if(mvp[0].bl && mob_db[md->class_].mexp > 0 && !md->state.nomvp) {
		struct map_session_data *mvpsd = NULL;

		if(mvp[0].bl->type == BL_PC) {
			mvpsd = (struct map_session_data *)mvp[0].bl;
		} else if(mvp[0].bl->type == BL_HOM) {
			mvpsd = ((struct homun_data *)mvp[0].bl)->msd;	// �z���������MVP�́A��l��
		} else if(mvp[0].bl->type == BL_MERC) {
			mvpsd = ((struct merc_data *)mvp[0].bl)->msd;	// �b���������MVP�́A��l��
		}
		if( mvpsd ) {
			int j,ret;
			struct item item;

			if( battle_config.mvp_announce == 1 ||
			   (battle_config.mvp_announce == 2 && !(md->spawndelay1 == -1 && md->spawndelay2 == -1)))
			{
				char output[256];
				// "�yMVP���z%s����%s��|���܂����I"
				snprintf(output, sizeof output, msg_txt(134), mvpsd->status.name, mob_db[md->class_].jname);
				clif_GMmessage(&mvpsd->bl,output,strlen(output)+1,0x10);
			}
			clif_mvp_effect(mvp[0].bl);	// �G�t�F�N�g

			if(mob_db[md->class_].mexpper > atn_rand()%10000) {
				atn_bignumber mexp = (atn_bignumber)mob_db[md->class_].mexp * battle_config.mvp_exp_rate * (9+count)/1000;

				// �z����b��������炤MVP�o���l�ɂ��{����K�p����
				if(mvp[0].bl->type == BL_HOM)
					mexp = mexp * battle_config.master_get_homun_base_exp / 100;
				else if(mvp[0].bl->type == BL_MERC)
					mexp = mexp * battle_config.master_get_merc_base_exp / 100;
				if(mexp < 1)
					mexp = 1;
				clif_mvp_exp(mvpsd, ((mexp > 0x7fffffff)? 0x7fffffff: (int)mexp));
				pc_gainexp(mvpsd,NULL,mexp,0);
			}
			for(j=0; j<3; j++) {
				i = atn_rand() % 3;
				if(mob_db[md->class_].mvpitem[i].nameid <= 0)
					continue;
				drop_rate = mob_db[md->class_].mvpitem[i].p;
				if(drop_rate <= 0 && battle_config.drop_rate0item)
					drop_rate = 1;
				if(drop_rate <= atn_rand()%10000)
					continue;
				memset(&item,0,sizeof(item));
				item.nameid   = mob_db[md->class_].mvpitem[i].nameid;
				item.identify = !itemdb_isequip3(item.nameid);
				if(battle_config.itemidentify)
					item.identify = 1;
				if(mvpsd->weight * 2 > mvpsd->max_weight) {
					clif_mvp_fail_item(mvpsd);
					map_addflooritem(&item,1,mvpsd->bl.m,mvpsd->bl.x,mvpsd->bl.y,
						(mvp[0].bl ? mvp[0].bl->id : 0),(mvp[1].bl ? mvp[1].bl->id : 0),(mvp[2].bl ? mvp[2].bl->id : 0),1);
				} else {
					clif_mvp_item(mvpsd,item.nameid);
					if((ret = pc_additem(mvpsd,&item,1))) {
						clif_additem(sd,0,0,ret);
						map_addflooritem(&item,1,mvpsd->bl.m,mvpsd->bl.x,mvpsd->bl.y,
							(mvp[0].bl ? mvp[0].bl->id : 0),(mvp[1].bl ? mvp[1].bl->id : 0),(mvp[2].bl ? mvp[2].bl->id : 0),1);
					}
				}
				break;
			}
		}
	}

	// <Agit> NPC Event [OnAgitBreak]
	if(md->npc_event[0]) {
		int len = strlen(md->npc_event) - 13;
		if(len >= 0 && strcmp(md->npc_event + len, "::OnAgitBreak") == 0) {
			printf("MOB.C: Run NPC_Event[OnAgitBreak].\n");
			if(agit_flag == 1)	// Call to Run NPC_Event[OnAgitBreak]
				guild_agit_break(md);
		}
	}

	// SCRIPT���s
	if(md->npc_event[0]) {
		struct map_session_data *ssd = NULL;
		if(sd)
			ssd = sd;
		else if(src && src->type == BL_PET)
			ssd = ((struct pet_data *)src)->msd;
		else if(src && src->type == BL_HOM)
			ssd = ((struct homun_data *)src)->msd;
		else if(src && src->type == BL_MERC)
			ssd = ((struct merc_data *)src)->msd;

		if(ssd == NULL) {
			if(mvp[0].bl != NULL && mvp[0].bl->type == BL_PC) {
				ssd = (struct map_session_data*)mvp[0].bl;
			} else {
				for(i=0; i<fd_max; i++) {
					if(session[i] && (ssd = (struct map_session_data *)session[i]->session_data) &&
					   ssd->state.auth && md->bl.m == ssd->bl.m)
						break;
				}
			}
		}
		if(ssd)
			npc_event(ssd,md->npc_event);
	}

	if(md->hp <= 0) {
		// �X�L�����j�b�g����̗��E���ɍς܂��Ă���
		md->hp = 1;
		skill_unit_move(&md->bl,gettick(),0);
		md->hp = 0;
		unit_remove_map(&md->bl, 1, 0);
	}

	return 0;
}

/*==========================================
 * �h���b�v���ɔ{����K�p
 *------------------------------------------
 */
int mob_droprate_fix(struct block_list *bl,int item,int drop)
{
	int drop_fix = drop;
	struct status_change *sc = NULL;

	if(bl)
		sc = status_get_sc(bl);

	if(drop < 1) return 0;
	if(drop > 10000) return 10000;

	if(battle_config.item_rate_details == 0) {
		drop_fix = drop * battle_config.item_rate / 100;
	}
	else if(battle_config.item_rate_details == 1) {
		if(drop < 10)
			drop_fix = drop * battle_config.item_rate_1 / 100;
		else if(drop < 100)
			drop_fix = drop * battle_config.item_rate_10 / 100;
		else if(drop < 1000)
			drop_fix = drop * battle_config.item_rate_100 / 100;
		else
			drop_fix = drop * battle_config.item_rate_1000 / 100;
	}
	else if(battle_config.item_rate_details == 2) {
		if(drop < 10) {
			drop_fix = drop * battle_config.item_rate_1 / 100;
			if(drop_fix < battle_config.item_rate_1_min)
				drop_fix = battle_config.item_rate_1_min;
			else if(drop_fix > battle_config.item_rate_1_max)
				drop_fix = battle_config.item_rate_1_max;
		} else if(drop < 100) {
			drop_fix = drop * battle_config.item_rate_10 / 100;
			if(drop_fix < battle_config.item_rate_10_min)
				drop_fix = battle_config.item_rate_10_min;
			else if(drop_fix > battle_config.item_rate_10_max)
				drop_fix = battle_config.item_rate_10_max;
		} else if(drop < 1000) {
			drop_fix = drop * battle_config.item_rate_100 / 100;
			if(drop_fix < battle_config.item_rate_100_min)
				drop_fix = battle_config.item_rate_100_min;
			else if(drop_fix > battle_config.item_rate_100_max)
				drop_fix = battle_config.item_rate_100_max;
		} else {
			drop_fix = drop * battle_config.item_rate_1000 / 100;
			if(drop_fix < battle_config.item_rate_1000_min)
				drop_fix = battle_config.item_rate_1000_min;
			else if(drop_fix > battle_config.item_rate_1000_max)
				drop_fix = battle_config.item_rate_1000_max;
		}
	}
	else if(battle_config.item_rate_details == 3) {
		switch(itemdb_type(item)) {
			case 0:
				drop_fix = drop * battle_config.potion_drop_rate / 100;
				break;
			case 2:
				drop_fix = drop * battle_config.consume_drop_rate / 100;
				break;
			case 3:
				if(item == 756 || item == 757 || item == 984 || item == 985 || item == 1010 || item == 1011)
					drop_fix = drop * battle_config.refine_drop_rate / 100;
				else
					drop_fix = drop * battle_config.etc_drop_rate / 100;
				break;
			case 4:
				drop_fix = drop * battle_config.weapon_drop_rate / 100;
				break;
			case 5:
				drop_fix = drop * battle_config.equip_drop_rate / 100;
				break;
			case 6:
				drop_fix = drop * battle_config.card_drop_rate / 100;
				break;
			case 8:
				drop_fix = drop * battle_config.petequip_drop_rate / 100;
				break;
			case 10:
				drop_fix = drop * battle_config.arrow_drop_rate / 100;
				break;
			default:
				drop_fix = drop * battle_config.other_drop_rate / 100;
				break;
		}
	}

	// �o�u���K��
	if(sc && sc->data[SC_ITEMDROPRATE].timer != -1)
		drop_fix = drop_fix * sc->data[SC_ITEMDROPRATE].val1 / 100;

	if(drop_fix > 10000)
		drop_fix = 10000;

	return drop_fix;
}

/*==========================================
 * �N���X�`�F���W
 *------------------------------------------
 */
static int mob_class_change_id(struct mob_data *md,int mob_id)
{
	unsigned int c, tick = gettick();
	int i, hp_rate, max_hp;

	nullpo_retr(0, md);

	if(md->bl.prev == NULL)
		return 0;
	if(!mobdb_checkid(mob_id))
		return 0;

	max_hp  = status_get_max_hp(&md->bl);	// max_hp>0�͕ۏ�
	hp_rate = md->hp * 100 / max_hp;
	clif_class_change(&md->bl,mob_get_viewclass(mob_id),1);

	md->class_ = mob_id;
	max_hp     = status_get_max_hp(&md->bl);
	if(battle_config.monster_class_change_full_recover) {
		md->hp = max_hp;
		linkdb_final( &md->dmglog );
	} else {
		md->hp = max_hp * hp_rate / 100;
	}
	if(md->hp > max_hp)
		md->hp = max_hp;
	else if(md->hp < 1)
		md->hp = 1;

	memcpy(md->name,mob_db[md->class_].jname,24);
	memset(&md->state,0,sizeof(md->state));

	md->attacked_id     = 0;
	md->target_id       = 0;
	md->move_fail_count = 0;

	md->speed   = mob_db[md->class_].speed;
	md->def_ele = mob_db[md->class_].element;
	md->mode    = mob_db[md->class_].mode;

	unit_skillcastcancel(&md->bl,0);
	md->state.skillstate = MSS_IDLE;
	md->last_thinktime   = tick;
	md->next_walktime    = tick + atn_rand()%50 + 5000;

	md->state.nodrop = 0;
	md->state.noexp  = 0;
	md->state.nomvp  = 0;

	c = tick - 1000 * 36000 * 10;
	for(i=0; i<MAX_MOBSKILL; i++) {
		md->skilldelay[i] = c;
	}
	md->ud.skillid = 0;
	md->ud.skilllv = 0;

	if(md->lootitem == NULL && mob_db[md->class_].mode&0x02) {
		md->lootitem = (struct item *)aCalloc(LOOTITEM_SIZE,sizeof(struct item));
		md->lootitem_count = 0;
	}

	skill_clear_unitgroup(&md->bl);
	skill_cleartimerskill(&md->bl);

	clif_clearchar_area(&md->bl,0);

	// ���̏ꍇ�폜->�ǉ��̏����͕K�v�Ȃ�
	// mob_ai_hard_spawn( &md->bl, 0 );
	// mob_ai_hard_spawn( &md->bl, 1 );

	clif_spawnmob(md);

	return 0;
}

/*==========================================
 * �N���X�`�F���W�i�����_���j
 *------------------------------------------
 */
int mob_class_change_randam(struct mob_data *md,int lv)
{
	int i = 0, k;
	int class_ = 1002;

	nullpo_retr(0, md);

	// mob_branch.txt���烉���_���ɏ���
	do {
		class_ = atn_rand()%(MOB_ID_MAX-MOB_ID_MIN) + MOB_ID_MIN;
		k = atn_rand()%1000000;
	} while((mob_db[class_].max_hp <= 0 || mob_db[class_].summonper[0] <= k ||
	        (lv < mob_db[class_].lv && battle_config.random_monster_checklv)) && (i++) < MOB_ID_MAX);

	if(i >= MOB_ID_MAX) {
		class_ = mob_dummy_class[0];
	}

	return mob_class_change_id(md,class_);
}

/*==========================================
 * �N���X�`�F���W�i���X�g�I���j
 *------------------------------------------
 */
int mob_class_change(struct mob_data *md,int *value,int value_count)
{
	int count = 0;

	nullpo_retr(0, md);
	nullpo_retr(0, value);

	while(count < value_count && mobdb_checkid(value[count])) {
		count++;
	}
	if(count <= 0)
		return 0;

	return mob_class_change_id(md,value[atn_rand()%count]);
}

/*==========================================
 * mob��
 *------------------------------------------
 */
int mob_heal(struct mob_data *md,int heal)
{
	int max_hp = status_get_max_hp(&md->bl);

	nullpo_retr(0, md);

	md->hp += heal;
	if( max_hp < md->hp )
		md->hp = max_hp;
	return 0;
}

/*==========================================
 * mob���[�v
 *------------------------------------------
 */
int mob_warp(struct mob_data *md,int m,int x,int y,int type)
{
	int moveblock;
	int i = 0, xs = 0, ys = 0, bx = x, by = y;
	unsigned int tick = gettick();

	nullpo_retr(0, md);

	if(md->bl.prev == NULL)
		return 0;

	if(m < 0)
		m = md->bl.m;

	if(type >= 0) {
		if(map[md->bl.m].flag.monster_noteleport)
			return 0;
		clif_clearchar_area(&md->bl,type);
	}

	if(bx > 0 && by > 0) {	// �ʒu�w��̏ꍇ���͂X�Z����T��
		xs = ys = 9;
	}

	while( (x < 0 || y < 0 || map_getcell(m,x,y,CELL_CHKNOPASS)) && (i++) < 1000 ) {
		if( xs > 0 && ys > 0 && i < 250 ) {	// �w��ʒu�t�߂̒T��
			x = bx+atn_rand()%xs-xs/2;
			y = by+atn_rand()%ys-ys/2;
		} else {				// ���S�����_���T��
			x = atn_rand()%(map[m].xs-2)+1;
			y = atn_rand()%(map[m].ys-2)+1;
		}
	}
	md->dir = 0;
	if(i >= 1000) {
		m = md->bl.m;
		x = md->bl.x;
		y = md->bl.y;
		if(battle_config.error_log)
			printf("MOB %d warp to (%d,%d) failed, class = %d\n",md->bl.id,bx,by,md->class_);
	}

	moveblock = map_block_is_differ(&md->bl,m,x,y);

	mob_ai_hard_spawn( &md->bl, 0 );
	skill_unit_move(&md->bl,tick,0);

	if(moveblock)
		map_delblock(&md->bl);
	md->bl.m = m;
	md->bl.x = md->ud.to_x = x;
	md->bl.y = md->ud.to_y = y;
	md->target_id   = 0;	// �^�Q����������
	md->attacked_id = 0;
	md->state.skillstate = MSS_IDLE;
	if(moveblock)
		map_addblock(&md->bl);

	skill_unit_move(&md->bl,tick,1);
	if(type >= 0) {
		clif_spawnmob(md);
	}
	mob_ai_hard_spawn( &md->bl, 1 );

	return 0;
}

/*==========================================
 * ��芪���̐��v�Z�p(foreachinarea)
 *------------------------------------------
 */
static int mob_countslave_sub(struct block_list *bl,va_list ap)
{
	int id, *c;
	struct mob_data *md;

	id = va_arg(ap,int);

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, c  = va_arg(ap,int *));
	nullpo_retr(0, md = (struct mob_data *)bl);

	if(md->master_id == id)
		(*c)++;
	return 0;
}

/*==========================================
 * ����芪���̐��v�Z
 *------------------------------------------
 */
int mob_countslave(struct mob_data *md)
{
	int c = 0;

	nullpo_retr(0, md);

	map_foreachinarea(mob_countslave_sub, md->bl.m,
		0,0,map[md->bl.m].xs-1,map[md->bl.m].ys-1,
		BL_MOB,md->bl.id,&c);
	return c;
}

/*==========================================
 * �w��͈͓��̎�芪���̐��v�Z
 *------------------------------------------
 */
static int mob_countslave_area(struct mob_data *md,int range)
{
	int c = 0;

	nullpo_retr(0, md);

	map_foreachinarea(mob_countslave_sub, md->bl.m,
		md->bl.x-range,md->bl.y-range,md->bl.x+range,md->bl.y+range,
		BL_MOB,md->bl.id,&c);
	return c;
}

/*==========================================
 * �艺MOB����
 *------------------------------------------
 */
int mob_summonslave(struct mob_data *md2,int *value,int size,int amount,int flag)
{
	struct mob_data *md;
	int bx, by, m, class_, k, count = 0;

	nullpo_retr(0, md2);
	nullpo_retr(0, value);

	bx = md2->bl.x;
	by = md2->bl.y;
	m  = md2->bl.m;

	while(count < size && mobdb_checkid(value[count])) {
		count++;
	}
	if(count <= 0)	// �l���ُ�Ȃ珢�����~�߂�
		return 0;

	for(k=0; k<amount; k++) {
		int x = 0, y = 0, i = 0;

		class_ = value[k%count];
		md = (struct mob_data *)aCalloc(1,sizeof(struct mob_data));
		if(mob_db[class_].mode&0x02)
			md->lootitem = (struct item *)aCalloc(LOOTITEM_SIZE,sizeof(struct item));
		else
			md->lootitem = NULL;

		while( (x <= 0 || y <= 0 || map_getcell(m,x,y,CELL_CHKNOPASS)) && (i++) < 100 ) {
			x = atn_rand()%5-2+bx;
			y = atn_rand()%5-2+by;
		}
		if(i >= 100) {
			x = bx;
			y = by;
		}

		mob_spawn_dataset(md,"--ja--",class_);
		md->bl.m = m;
		md->bl.x = x;
		md->bl.y = y;

		md->m           = m;
		md->x0          = x;
		md->y0          = y;
		md->xs          = 0;
		md->ys          = 0;
		md->spawndelay1 = -1;	// ��x�̂݃t���O
		md->spawndelay2 = -1;	// ��x�̂݃t���O
		md->ai_pc_count = 0;
		md->ai_prev = md->ai_next = NULL;

#ifdef DYNAMIC_SC_DATA
		// �_�~�[�}��
		md->sc.data = dummy_sc_data;
#endif

		memset(md->npc_event,0,sizeof(md->npc_event));
		map_addiddb(&md->bl);
		mob_spawn(md->bl.id);

		clif_skill_nodamage(&md->bl,&md->bl,(flag)? NPC_SUMMONSLAVE: NPC_SUMMONMONSTER,amount,1);

		if(flag) {
			md->master_id    = md2->bl.id;
			md->speed        = md2->speed;
			md->state.nodrop = battle_config.summonslave_no_drop;
			md->state.noexp  = battle_config.summonslave_no_exp;
			md->state.nomvp  = battle_config.summonslave_no_mvp;
		} else {
			md->state.nodrop = battle_config.summonmonster_no_drop;
			md->state.noexp  = battle_config.summonmonster_no_exp;
			md->state.nomvp  = battle_config.summonmonster_no_mvp;
		}
	}
	return 0;
}

/*==========================================
 * MOBskill����Y��skillid��skillidx��Ԃ�
 *------------------------------------------
 */
static int mob_skillid2skillidx(int class_,int skillid)
{
	int i;
	struct mob_skill *ms = mob_db[class_].skill;

	if(ms == NULL)
		return -1;

	for(i=0; i<mob_db[class_].maxskill; i++) {
		if(ms[i].skill_id == skillid)
			return i;
	}
	return -1;
}

/*==========================================
 * command�g�p
 *------------------------------------------
 */
static int mobskill_command_use_id_sub(struct block_list *bl, va_list ap )
{
	int skill_idx;
	int target_id;
	int target_type;
	int casttime;
	int commander_id;
	int *flag;
	struct mob_data* md = NULL;
	struct mob_skill *ms;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	commander_id = va_arg(ap,int);
	md           = va_arg(ap,struct mob_data *);
	target_type  = va_arg(ap,int);
	skill_idx    = va_arg(ap,int);
	flag         = va_arg(ap,int*);

	if(*flag)
		return 0;
	if(md == NULL)
		return 0;

	switch(target_type)
	{
		case MCT_TARGET:
			if(!(bl->type & BL_CHAR))
				return 0;
			if(md->bl.id == bl->id)
				return 0;
			if(md->target_id > 0)
				target_id = md->target_id;
			else
				target_id = bl->id;
			break;
		case MCT_FRIEND:
			if(bl->type != BL_MOB)
				return 0;
			if(md->bl.id == bl->id)
				return 0;
			target_id = bl->id;
			*flag = 1;
			break;
		case MCT_FRIENDS:
			if(bl->type != BL_MOB)
				return 0;
			if(md->bl.id == bl->id)
				return 0;
			target_id = bl->id;
			break;
		case MCT_SLAVE:
			if(bl->type != BL_MOB)
				return 0;
			if(md->bl.id == bl->id)
				return 0;
			if(md->bl.id != ((struct mob_data*)bl)->master_id)
				return 0;
			target_id = bl->id;
			*flag = 1;
			break;
		case MCT_SLAVES:
			if(bl->type != BL_MOB)
				return 0;
			if(md->bl.id == bl->id)
				return 0;
			if(md->bl.id != ((struct mob_data*)bl)->master_id)
				return 0;
			target_id = bl->id;
			break;
		default:
			printf("mobskill_command_use_id_sub: target_type error\n");
			return 0;
	}

	ms = &mob_db[md->class_].skill[skill_idx];
	casttime = skill_castfix(bl, ms->skill_id, ms->casttime, 0);
	md->skillidx = skill_idx;
	md->skilldelay[skill_idx] = gettick() + casttime;

	return unit_skilluse_id2(&md->bl,target_id, ms->skill_id,ms->skill_lv, casttime, ms->cancel);
}

static int mobskill_command(struct block_list *bl, va_list ap)
{
	int casttime;
	int commander_id,target_id;
	int skill_id,skill_idx;
	int command_target_type;
	int range;
	int target_type;
	int *flag;
	int once_flag = 0;
	struct mob_data* md = NULL;
	struct mob_skill *ms;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md = (struct mob_data *)bl);

	commander_id        = va_arg(ap,int);
	skill_id            = va_arg(ap,int);
	command_target_type = va_arg(ap,int);
	target_type         = va_arg(ap,int);
	range               = va_arg(ap,int);
	flag                = va_arg(ap,int*);

	skill_idx = mob_skillid2skillidx(md->class_,skill_id);
	if(skill_idx == -1)
		return 0;

	switch(command_target_type)
	{
		case MCT_SELF:
			if(bl->id != md->bl.id)
				return 0;
			if(*flag)
				return 0;
			break;
		case MCT_FRIEND:
			if(commander_id == md->master_id)
				return 0;
			if(*flag)
				return 0;
			break;
		case MCT_FRIENDS:
			if(commander_id == md->master_id)
				return 0;
			break;
		case MCT_SLAVE:
			if(commander_id != md->master_id)
				return 0;
			if(*flag)
				return 0;
			break;
		case MCT_SLAVES:
			if(commander_id != md->master_id)
				return 0;
			break;
	}

	// �^�[�Q�b�g�I��
	switch(target_type)
	{
		case MCT_MASTER:
			if(md->master_id <= 0)
				return 0;
			target_id = md->master_id;
			break;
		case MCT_COMMANDER:
			if(commander_id <= 0)
				return 0;
			target_id = commander_id;
			break;
		case MCT_SELF:
			target_id = bl->id;
			break;
		case MCT_TARGET:
			if(md->target_id) {
				target_id = md->target_id;
			} else {
				struct mob_data *cmd = map_id2md(commander_id);
				if(cmd && cmd->target_id)
					target_id = cmd->target_id;
				else
					map_foreachinarea(mobskill_command_use_id_sub,bl->m,bl->x-range,bl->y-range,bl->x+range,bl->y+range,
						BL_CHAR,commander_id,md,target_type,skill_idx,&once_flag);
			}
			*flag = 1;
			return 1;
		case MCT_FRIEND:
		case MCT_FRIENDS:
		case MCT_SLAVE:
		case MCT_SLAVES:
			map_foreachinarea( mobskill_command_use_id_sub,bl->m,bl->x-range,bl->y-range,bl->x+range,bl->y+range,
				BL_MOB,commander_id,md,target_type,skill_idx,&once_flag);
			*flag = 1;
			return 1;
		default:
			printf("mobskill_command_use_id_sub: target_type error\n");
			return 0;
	}
	*flag = 1;

	ms = &mob_db[md->class_].skill[skill_idx];
	casttime = skill_castfix(bl, ms->skill_id, ms->casttime, 0);
	md->skillidx = skill_idx;
	md->skilldelay[skill_idx] = gettick() + casttime;

	unit_skilluse_id2(&md->bl,target_id, ms->skill_id,ms->skill_lv, casttime, ms->cancel);

	return 1;
}

/*==========================================
 * modechange�g�p
 *------------------------------------------
 */
static int mobskill_modechange(struct block_list *bl, va_list ap )
{
	int commander_id, target_type, mode;
	int *flag;
	struct mob_data* md = NULL;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md = (struct mob_data *)bl);

	commander_id = va_arg(ap,int);
	target_type  = va_arg(ap,int);
	mode         = va_arg(ap,int);
	flag         = va_arg(ap,int*);

	switch(target_type)
	{
		case MCT_SELF:
			if(commander_id != bl->id)
				return 0;
			if(*flag)
				return 0;
			break;
		case MCT_FRIEND:
			if(commander_id == bl->id)
				return 0;
			if(*flag)
				return 0;
			break;
		case MCT_FRIENDS:
			if(commander_id == bl->id)
				return 0;
			break;
		case MCT_SLAVE:
			if(commander_id != md->master_id)
				return 0;
			if(*flag)
				return 0;
			break;
		case MCT_SLAVES:
			if(commander_id != md->master_id)
				return 0;
			break;
	}
	*flag = 1;
	md->mode = mode;

	return 1;
}

/*==========================================
 * ���݂Ƃ͕ʂ̃^�[�Q�b�g����������
 *------------------------------------------
 */
static int mobskill_anothertarget(struct block_list *bl, va_list ap)
{
	struct mob_data *md = NULL;
	struct block_list **target;
	int type;
	int *c;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	md     = va_arg(ap,struct mob_data *);
	type   = va_arg(ap,int);
	c      = va_arg(ap,int *);
	target = va_arg(ap,struct block_list **);

	if(!(bl->type & BL_CHAR))
		return 0;

	if(bl->id == md->bl.id || bl->id == md->target_id || unit_isdead(bl))
		return 0;
	if(!path_search_long(NULL,md->bl.m,md->bl.x,md->bl.y,bl->x,bl->y))
		return 0;

	switch (type) {
		case MST_FRIEND:
			if(bl->type != BL_MOB) {
				return 0;
			}
			break;
		case MST_SLAVE:
			if(bl->type != BL_MOB || ((struct mob_data *)bl)->master_id != md->bl.id) {
				return 0;
			}
			break;
		case MST_ANOTHERTARGET:
			if(battle_check_target(&md->bl,bl,BCT_ENEMY) <= 0) {
				return 0;
			}
			if(!mob_can_lock(md,bl)) {
				return 0;
			}
			break;
	}
	if( atn_rand()%1000 < 1000/(++(*c)) )	// �͈͓��œ��m���ɂ���
		*target = bl;
	return 1;
}

/*==========================================
 * �߂��̖�����HP�̏������������̂�T��
 *------------------------------------------
 */
static int mob_getfriendhpmaxrate_sub(struct block_list *bl,va_list ap)
{
	int cond, rate, diff;
	struct block_list **fr;
	struct mob_data *mmd = NULL;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, mmd = va_arg(ap,struct mob_data *));

	if(bl->type != BL_PC && bl->type != BL_MOB)
		return 0;
	if(mmd->bl.id == bl->id)
		return 0;
	if(battle_check_target(&mmd->bl,bl,BCT_ENEMY) > 0)
		return 0;

	cond = va_arg(ap,int);
	rate = va_arg(ap,int);
	fr   = va_arg(ap,struct block_list **);

	diff = status_get_hp(bl) - status_get_max_hp(bl)*rate/100;
	if( (diff < 0 && cond == MSC_FRIENDHPLTMAXRATE) ||
	    (diff > 0 && cond == MSC_FRIENDHPGTMAXRATE) )
	{
		int *c = va_arg(ap,int *);
		if( atn_rand()%1000 < 1000/(++(*c)) )	// �͈͓��œ��m���ɂ���
			(*fr) = bl;
	}
	return 0;
}

static struct block_list *mob_getfriendhpmaxrate(struct mob_data *md,int cond,int rate)
{
	struct block_list *fr = NULL;
	const int r = 8;
	int c = 0, type;

	nullpo_retr(NULL, md);

	if(md->state.special_mob_ai)	// PC����̏���MOB�Ȃ�PC������
		type = BL_PC;
	else
		type = BL_MOB;

	map_foreachinarea(mob_getfriendhpmaxrate_sub, md->bl.m,
		md->bl.x-r ,md->bl.y-r, md->bl.x+r, md->bl.y+r,
		type,md,cond,rate,&fr,&c);
	return fr;
}

/*==========================================
 * �߂��̖����ŃX�e�[�^�X��Ԃ��������̂�T��
 *------------------------------------------
 */
static int mob_getfriendstatus_sub(struct block_list *bl,va_list ap)
{
	int cond1, cond2;
	struct block_list **fr;
	struct mob_data *mmd = NULL;
	struct status_change *sc;
	int flag = 0;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, mmd = va_arg(ap,struct mob_data *));

	if(bl->type != BL_PC && bl->type != BL_MOB)
		return 0;
	if(mmd->bl.id == bl->id)
		return 0;
	if(battle_check_target(&mmd->bl,bl,BCT_ENEMY) > 0)
		return 0;

	cond1 = va_arg(ap,int);
	cond2 = va_arg(ap,int);
	fr    = va_arg(ap,struct block_list **);

	sc = status_get_sc(bl);
	if(sc) {
		if(cond2 == -1) {
			int j;
			for(j=SC_STONE; j<=SC_BLIND && !flag; j++) {
				flag = (sc->data[j].timer != -1);
			}
		} else {
			flag = (sc->data[cond2].timer != -1);
		}
		if( flag^(cond1 == MSC_FRIENDSTATUSOFF) ) {
			int *c = va_arg(ap,int *);
			if( atn_rand()%1000 < 1000/(++(*c)) )	// �͈͓��œ��m���ɂ���
				(*fr) = bl;
		}
	}
	return 0;
}

static struct block_list *mob_getfriendstatus(struct mob_data *md,int cond1,int cond2)
{
	struct block_list *fr = NULL;
	const int r = 8;
	int c = 0, type;

	nullpo_retr(0, md);

	if(md->state.special_mob_ai)	// PC����̏���MOB�Ȃ�PC������
		type = BL_PC;
	else
		type = BL_MOB;

	map_foreachinarea(mob_getfriendstatus_sub, md->bl.m,
		md->bl.x-r ,md->bl.y-r, md->bl.x+r, md->bl.y+r,
		type,md,cond1,cond2,&fr,&c);
	return fr;
}

/*==========================================
 * �����\�ȏ�Ԃ��ǂ���
 *------------------------------------------
 */
static int mob_can_counterattack(struct mob_data *md,struct block_list *target)
{
	struct block_list *tbl = NULL;
	int range, c = 0;

	nullpo_retr(0, md);

	range = mob_db[md->class_].range;

	if(target)
	{
		// �_���[�W�f�B���C���͓�����Ƃ݂Ȃ��̂ňꎞ�u��
		unsigned int tick = md->ud.canmove_tick;
		int flag;

		md->ud.canmove_tick = gettick();

		if( unit_can_move(&md->bl) && !unit_isrunning(&md->bl) ) {
			// ������Ƃ�
			flag = mob_can_reach(md,target,AREA_SIZE);
		} else {
			// �����Ȃ��Ƃ�
			flag = battle_check_range(&md->bl,target,range);
		}
		md->ud.canmove_tick = tick;
		if(flag)
			return 1;
	}

	// �^�[�Q�b�g�ɔ����ł��Ȃ���ԂȂ̂ŋ߂��ɍU���ł��鑊�肪���邩�T��
	map_foreachinarea(mobskill_anothertarget,
		md->bl.m,md->bl.x-range,md->bl.y-range,md->bl.x+range,md->bl.y+range,
		BL_CHAR,md,MST_ANOTHERTARGET,&c,&tbl);

	return (tbl != NULL);
}

/*==========================================
 * �X�L���g�p�i�r���J�n�AID�w��j
 *------------------------------------------
 */
static int mobskill_use_id(struct mob_data *md,struct block_list *target,int skill_idx)
{
	struct mob_skill *mds, *ms;
	int i, casttime;
	unsigned int tick = gettick();

	nullpo_retr(0, md);

	mds = mob_db[md->class_].skill;
	ms  = &mds[skill_idx];

	casttime     = skill_castfix(&md->bl, ms->skill_id, ms->casttime, 0);
	md->skillidx = skill_idx;

	for(i=0; i<mob_db[md->class_].maxskill; i++) {
		if(mds[i].skill_id == ms->skill_id)
			md->skilldelay[i] = tick + casttime;
	}

	return unit_skilluse_id2(
		&md->bl, (target ? target->id : md->target_id), ms->skill_id,
		ms->skill_lv, casttime, ms->cancel
	);
}

/*==========================================
 * �X�L���g�p�i�ꏊ�w��j
 *------------------------------------------
 */
static int mobskill_use_pos( struct mob_data *md, int skill_x, int skill_y, int skill_idx)
{
	struct mob_skill *mds, *ms;
	int i, casttime;
	unsigned int tick = gettick();

	nullpo_retr(0, md);

	mds = mob_db[md->class_].skill;
	ms  = &mds[skill_idx];

	casttime     = skill_castfix(&md->bl, ms->skill_id, ms->casttime, 0);
	md->skillidx = skill_idx;

	for(i=0; i<mob_db[md->class_].maxskill; i++) {
		if(mds[i].skill_id == ms->skill_id)
			md->skilldelay[i] = tick + casttime;
	}

	return unit_skilluse_pos2(&md->bl, skill_x, skill_y, ms->skill_id, ms->skill_lv, casttime, ms->cancel);
}

/*==========================================
 * �X�L���g�p����
 *------------------------------------------
 */
int mobskill_use(struct mob_data *md,unsigned int tick,int event)
{
	struct mob_skill *ms;
	struct block_list *target = NULL, *master = NULL;
	struct block_list *tbl = NULL;
	int i;

	nullpo_retr(0, md);
	nullpo_retr(0, ms = mob_db[md->class_].skill);

	if(battle_config.mob_skill_use == 0 || md->ud.skilltimer != -1)
		return 0;

	if(md->sc.data[SC_SELFDESTRUCTION].timer != -1)	// �������̓X�L�����g��Ȃ�
		return 0;

	if(md->state.special_mob_ai >= 2)		// �X�t�B�A�[�}�C���̓X�L�����g��Ȃ�
		return 0;

	if(md->hp <= 0) {
		md->state.skillstate = MSS_DEAD;
	}
	if(md->ud.attacktimer != -1) {
		md->state.skillstate = MSS_ATTACK;
	}
	if(md->ud.walktimer != -1 && md->state.skillstate != MSS_CHASE) {
		md->state.skillstate = MSS_WALK;
	}

	// �^�[�Q�b�g�ƃ}�X�^�[�̏��擾
	target = map_id2bl(md->target_id);
	if(target && target->type != BL_PC && target->type != BL_MOB && target->type != BL_HOM && target->type != BL_MERC)
		target = NULL;

	master = map_id2bl(md->master_id);
	if(master && master->type != BL_PC && master->type != BL_MOB)
		master = NULL;

	for(i=0; i<mob_db[md->class_].maxskill; i++)
	{
		int c2 = ms[i].cond2;
		int flag = 0;

		// event�g���K�[�̂Ƃ��ɏ���������Ȃ�
		if(event >= 0 && (event&0xffff) != ms[i].cond1)
			continue;

		// �f�B���C��
		if(DIFF_TICK(tick,md->skilldelay[i]) < ms[i].delay)
			continue;

		// �R�}���h��p
		if(ms[i].state == MSS_COMMANDONLY || ms[i].state == MSS_DISABLE)
			continue;

		// ��Ԕ���
		if(ms[i].state != MSS_ANY && ms[i].state != md->state.skillstate)
			continue;

		// �m������
		if(atn_rand()%10000 >= ms[i].permillage)
			continue;

		// ��������
		switch( ms[i].cond1 ) {
		case MSC_ALWAYS:
			flag = 1;
			break;
		case MSC_MYHPLTMAXRATE:		// HP < maxhp%
			flag = ( md->hp < status_get_max_hp(&md->bl)*c2/100 );
			break;
		case MSC_MYHPGTMAXRATE:		// HP > maxhp%
			flag = ( md->hp > status_get_max_hp(&md->bl)*c2/100 );
			break;
		case MSC_MYSTATUSON:		// status[num] on
		case MSC_MYSTATUSOFF:		// status[num] off
			if( c2 == -1 ) {
				int j;
				for(j=SC_STONE; j<=SC_BLIND && !flag; j++) {
					flag = ( md->sc.data[j].timer != -1 );
				}
			} else {
				flag = ( md->sc.data[c2].timer != -1 );
			}
			flag ^= ( ms[i].cond1 == MSC_MYSTATUSOFF );
			break;
		case MSC_FRIENDHPLTMAXRATE:	// friend HP < maxhp%
		case MSC_FRIENDHPGTMAXRATE:	// friend HP > maxhp%
			flag = ( (tbl = mob_getfriendhpmaxrate(md,ms[i].cond1,c2)) != NULL );
			break;
		case MSC_FRIENDSTATUSON:	// friend status[num] on
		case MSC_FRIENDSTATUSOFF:	// friend status[num] off
			flag = ( (tbl = mob_getfriendstatus(md,ms[i].cond1,c2)) != NULL );
			break;
		case MSC_SLAVELT:		// slave < num
			flag = ( mob_countslave(md) < c2 );
			break;
		case MSC_ATTACKPCGT:	// attack pc > num
			flag = ( unit_counttargeted(&md->bl,0) > c2 );
			break;
		case MSC_ATTACKPCGE:	// attack pc >= num
			flag = ( unit_counttargeted(&md->bl,0) >= c2 );
			break;
		case MSC_SLAVELE:		// slave <= num
			flag = ( mob_countslave(md) <= c2 );
			break;
		case MSC_AREASLAVEGT:		// slave > num
			flag = ( mob_countslave_area(md,(c2>>8)) > (c2&0xff) );
			break;
		case MSC_AREASLAVELE:		// slave <= num
			flag = ( mob_countslave_area(md,(c2>>8)) <= (c2&0xff) );
			break;
		case MSC_TARGETHPGTMAXRATE:	// target HP < maxhp%
			flag = ( target && status_get_hp(target) > status_get_max_hp(target)*c2/100 );
			break;
		case MSC_TARGETHPLTMAXRATE:	// target HP < maxhp%
			flag = ( target && status_get_hp(target) < status_get_max_hp(target)*c2/100 );
			break;
		case MSC_TARGETHPGT:	// target HP < maxhp%
			flag = ( target && status_get_hp(target) > c2 );
			break;
		case MSC_TARGETHPLT:	// target HP < maxhp%
			flag = ( target && status_get_hp(target) < c2 );
			break;
		case MSC_TARGETSTATUSON:	// target status[num] on
		case MSC_TARGETSTATUSOFF:	// target status[num] off
			if(target) {
				struct status_change *tsc = status_get_sc(target);
				if( !tsc ) {
					flag = 0;
				} else if( c2 == -1 ) {
					int j = 0;
					if(tsc->data[SC_STONE].timer != -1)
						flag = ( tsc->data[SC_STONE].val2 == 0 );
					for(j=SC_STONE+1; j<=SC_BLIND && !flag; j++) {
						flag = ( tsc->data[j].timer != -1 );
					}
				} else {
					flag = ( tsc->data[c2].timer != -1 );
				}
				flag ^= ( ms[i].cond1 == MSC_TARGETSTATUSOFF );
			}
			break;
		case MSC_MASTERHPGTMAXRATE:	// master HP < maxhp%
			flag = ( master && status_get_hp(master) > status_get_max_hp(master)*c2/100 );
			break;
		case MSC_MASTERHPLTMAXRATE:	// master HP < maxhp%
			flag = ( master && status_get_hp(master) < status_get_max_hp(master)*c2/100 );
			break;
		case MSC_MASTERSTATUSON:	// master status[num] on
		case MSC_MASTERSTATUSOFF:	// master status[num] off
			if(master) {
				struct status_change *msc = status_get_sc(master);
				if( !msc ) {
					flag = 0;
				} else if( c2 == -1 ) {
					int j = 0;
					if(msc->data[SC_STONE].timer != -1)
						flag = ( msc->data[SC_STONE].val2 == 0 );
					for(j=SC_STONE+1; j<=SC_BLIND && !flag; j++) {
						flag = ( msc->data[j].timer != -1 );
					}
				} else {
					flag = ( msc->data[c2].timer != -1 );
				}
				flag ^= ( ms[i].cond1 == MSC_MASTERSTATUSOFF );
			}
			break;
		/* �ȉ���event�g���K�[�ł̂݌Ăяo�������� */
		case MSC_CASTTARGETED:
		case MSC_CLOSEDATTACKED:
		case MSC_LONGRANGEATTACKED:
		case MSC_SPAWN:
			if(event >= 0)
				flag = 1;
			break;
		case MSC_SKILLUSED:
			flag = ( (event&0xffff) == MSC_SKILLUSED && ((event>>16) == c2 || c2 == 0) );
			break;
		case MSC_RUDEATTACKED:
			if(event >= 0)
				flag = !mob_can_counterattack(md,target);
			break;
		}
		if( !flag )
			continue;

		switch(ms[i].target)
		{
			case MST_TARGET:
			case MST_AROUND5:
			case MST_AROUND6:
			case MST_AROUND7:
			case MST_AROUND8:
				tbl = target;
				break;
			case MST_MASTER:
				tbl = master;
				break;
			case MST_FRIEND:
				if(tbl) {
					break;
				}
				// fall through
			case MST_SLAVE:
				{
					int c = 0, range = 8;

					tbl = NULL;
					map_foreachinarea( mobskill_anothertarget,
						md->bl.m,md->bl.x-range,md->bl.y-range,md->bl.x+range,md->bl.y+range,
						BL_MOB,md,ms[i].target,&c,&tbl);
				}
				break;
			case MST_ANOTHERTARGET:
				{
					int c = 0, range = skill_get_fixed_range(&md->bl,ms[i].skill_id,ms[i].skill_lv);

					tbl = target;
					map_foreachinarea( mobskill_anothertarget,
						md->bl.m,md->bl.x-range,md->bl.y-range,md->bl.x+range,md->bl.y+range,
						BL_CHAR,md,MST_ANOTHERTARGET,&c,&tbl);
				}
				break;
			default:
				tbl = &md->bl;
				break;
			// ����AI�n
			case MST_COMMAND:
			{
				int range = ms[i].val[1];
				int once_flag = 0;
				switch(ms[i].val[0])
				{
					case MCT_GROUP:
						{
							int casttime = skill_castfix(&md->bl, ms[i].skill_id, ms[i].casttime, 0);
							md->skilldelay[i] = gettick() + casttime;
							md->skillidx = i;
							unit_skilluse_id2(&md->bl,md->bl.id, ms[i].skill_id,
								ms[i].skill_lv,casttime, ms[i].cancel);

							map_foreachinarea( mobskill_command,
								md->bl.m,md->bl.x-range,md->bl.y-range,md->bl.x+range,md->bl.y+range,
								BL_MOB,md->bl.id,ms[i].skill_id,MCT_SLAVES,ms[i].val[2],ms[i].val[3],&once_flag);
						}
						break;
					case MCT_SELF:
					case MCT_SLAVE:
					case MCT_SLAVES:
					case MCT_FRIEND:
					case MCT_FRIENDS:
						map_foreachinarea( mobskill_command,
							md->bl.m,md->bl.x-range,md->bl.y-range,md->bl.x+range,md->bl.y+range,
							BL_MOB,md->bl.id,ms[i].skill_id,ms[i].val[0],ms[i].val[2],ms[i].val[3],&once_flag);
						break;
				}
				md->skilldelay[i] = gettick() + ms[i].delay;
				return 1;
			}
			case MST_MODECHANGE:
			{
				int range = ms[i].val[1];
				int once_flag = 0;
				switch(ms[i].val[0])
				{
					case MCT_SELF:
						md->mode = ms[i].val[2];
						break;
					case MCT_GROUP:
						md->mode = ms[i].val[2];
						map_foreachinarea( mobskill_modechange,
							md->bl.m,md->bl.x-range,md->bl.y-range,md->bl.x+range,md->bl.y+range,
							BL_MOB,md->bl.id,MCT_SLAVES,ms[i].val[2],&once_flag);
						break;
					case MCT_SLAVE:
					case MCT_SLAVES:
					case MCT_FRIEND:
					case MCT_FRIENDS:
						map_foreachinarea( mobskill_modechange,
							md->bl.m,md->bl.x-range,md->bl.y-range,md->bl.x+range,md->bl.y+range,
							BL_MOB,md->bl.id,ms[i].val[0],ms[i].val[2],&once_flag);
						break;
				}
				md->skilldelay[i] = gettick() + ms[i].delay;
				return 1;
			}
			case MST_TARGETCHANGE:
				// ����`�H
				md->skilldelay[i] = gettick() + ms[i].delay;
				return 1;
		}
		if(tbl == NULL)
			continue;

		if( skill_get_inf(ms[i].skill_id)&2 ) {
			// �ꏊ�w��
			int x = tbl->x;
			int y = tbl->y;

			if(ms[i].target >= MST_AROUND5) {
				int m = tbl->m, bx = x, by = y, j = 0, r;
				if(ms[i].target >= MST_AROUND1)
					r = ms[i].target-MST_AROUND1+1;
				else
					r = ms[i].target-MST_AROUND5+1;
				do {
					bx = x + atn_rand()%(r*2+1) - r;
					by = y + atn_rand()%(r*2+1) - r;
				} while( map_getcell(m,bx,by,CELL_CHKNOPASS) && (j++) < 1000 );

				if(j < 1000) {
					x = bx;
					y = by;
				}
			}
			return mobskill_use_pos(md,x,y,i);
		} else {
			// �Ώێw��
			return mobskill_use_id(md,tbl,i);
		}
	}
	return 0;
}

/*==========================================
 * �X�L���g�p�C�x���g����
 *------------------------------------------
 */
int mobskill_event(struct mob_data *md,int flag)
{
	unsigned int tick = gettick();

	nullpo_retr(0, md);

	// ���[�h�A�^�b�N���ŗD�悷��
	if( !mobskill_use(md, tick, MSC_RUDEATTACKED) ) {
		if( (flag&BF_SHORT) && mobskill_use(md, tick, MSC_CLOSEDATTACKED) )
			return 1;
		else if( (flag&BF_LONG) && mobskill_use(md, tick, MSC_LONGRANGEATTACKED) )
			return 1;
		else
			return 0;
	}
	return 1;
}

/*==========================================
 * Mob���G���y���E���Ȃǂ̏ꍇ�̔���
 *------------------------------------------
 */
int mob_gvmobcheck(struct map_session_data *sd, struct block_list *bl)
{
	struct mob_data *md = NULL;

	nullpo_retr(0, sd);
	nullpo_retr(0, bl);

	if(bl->type != BL_MOB || (md = (struct mob_data *)bl) == NULL)
		return 1;

	if(md->class_ == 1288 || md->class_ == 1287 || md->class_ == 1286 || md->class_ == 1285)
	{
		struct guild_castle *gc = guild_mapname2gc(map[sd->bl.m].name);
		struct guild *g = guild_search(sd->status.guild_id);

		if(g == NULL && md->class_ == 1288)
			return 0;	// �M���h�������Ȃ�_���[�W����
		if(gc != NULL && !map[sd->bl.m].flag.gvg)
			return 0;	// �ԓ���Gv����Ȃ��Ƃ��̓_���[�W�Ȃ�
		if(g && gc != NULL && g->guild_id == gc->guild_id)
			return 0;	// ����̃M���h�̃G���y�Ȃ�_���[�W����
		if(g && guild_checkskill(g,GD_APPROVAL) <= 0 && md->class_ == 1288)
			return 0;	// ���K�M���h���F���Ȃ��ƃ_���[�W����
		if(g && gc && guild_check_alliance(gc->guild_id, g->guild_id, 0) == 1)
			return 0;	// �����Ȃ�_���[�W����
	}

	return 1;
}

/*==========================================
 * �X�L���p�^�C�}�[�폜�i���g�p�j
 *------------------------------------------
 */
/*
int mobskill_deltimer(struct mob_data *md )
{
	nullpo_retr(0, md);

	if( md->ud.skilltimer != -1 ) {
		if( skill_get_inf(md->ud.skillid) & 2 )
			delete_timer(md->ud.skilltimer, skill_castend_pos);
		else
			delete_timer(md->ud.skilltimer, skill_castend_id);
		md->ud.skilltimer = -1;
	}
	return 0;
}
*/

//
// ������
//
/*==========================================
 * ���ݒ�mob���g��ꂽ�̂Ŏb�菉���l�ݒ�
 *------------------------------------------
 */
static int mob_makedummymobdb(int class_)
{
	int i;

	sprintf(mob_db[class_].name,"mob%d",class_);
	sprintf(mob_db[class_].jname,"mob%d",class_);
	mob_db[class_].lv       = 1;
	mob_db[class_].max_hp   = 1000;
	mob_db[class_].max_sp   = 1;
	mob_db[class_].base_exp = 2;
	mob_db[class_].job_exp  = 1;
	mob_db[class_].range    = 1;
	mob_db[class_].atk1     = 7;
	mob_db[class_].atk2     = 10;
	mob_db[class_].def      = 0;
	mob_db[class_].mdef     = 0;
	mob_db[class_].str      = 1;
	mob_db[class_].agi      = 1;
	mob_db[class_].vit      = 1;
	mob_db[class_].int_     = 1;
	mob_db[class_].dex      = 6;
	mob_db[class_].luk      = 2;
	mob_db[class_].range2   = 10;
	mob_db[class_].range3   = 10;
	mob_db[class_].size     = 0;
	mob_db[class_].race     = RCT_FORMLESS;
	mob_db[class_].element  = ELE_NEUTRAL;
	mob_db[class_].mode     = 0;
	mob_db[class_].speed    = 300;
	mob_db[class_].adelay   = 1000;
	mob_db[class_].amotion  = 500;
	mob_db[class_].dmotion  = 500;
	mob_db[class_].dropitem[0].nameid = 909;	// Jellopy
	mob_db[class_].dropitem[0].p      = 1000;

	for(i=1; i<ITEM_DROP_COUNT; i++) {
		mob_db[class_].dropitem[i].nameid = 0;
		mob_db[class_].dropitem[i].p      = 0;
	}
	mob_db[class_].mexp    = 0;
	mob_db[class_].mexpper = 0;

	for(i=0; i<3; i++) {
		mob_db[class_].mvpitem[i].nameid = 0;
		mob_db[class_].mvpitem[i].p      = 0;
	}
	for(i=0; i<MAX_RANDOMMONSTER; i++) {
		mob_db[class_].summonper[i] = 0;
	}
	return 0;
}

/*==========================================
 * db/mob_db.txt�ǂݍ���
 *------------------------------------------
 */
#define DB_ADD(a,b) a = ( (!cov || strlen(str[b]) > 0) ? atoi(str[b]) : a )

static int mob_readdb(void)
{
	FILE *fp;
	char line[1024];
	const char *filename[] = { "db/mob_db.txt","db/addon/mob_db_add.txt" };
	int n;

	memset(mob_db_real,0,sizeof(mob_db_real));

	for(n=0;n<2;n++){
		fp=fopen(filename[n],"r");
		if(fp==NULL){
			if(n>0)
				continue;
			return -1;
		}
		while(fgets(line,1020,fp)){
			int class_,i,cov=0,num=0;
			char *str[37+ITEM_DROP_COUNT*2];
			char *p,*np;

			if(line[0] == '/' && line[1] == '/')
				continue;

			for(i=0,p=line;i<(37+ITEM_DROP_COUNT*2);i++){
				if((np=strchr(p,','))!=NULL){
					str[i]=p;
					*np=0;
					p=np+1;
				} else
					str[i]=p;
			}
			class_ = atoi(str[num]);

			// ���O�ݒ�O�Ȃ̂�mobdb_checkid() �͎g���Ȃ�
			if(class_ < MOB_ID_MIN || class_ >= MOB_ID_MAX)
				continue;
			if(n == 1 && mob_db[class_].view_class == class_)
				cov = 1;	// mob_db_add�ɂ��A���łɓo�^�̂���ID�̏㏑�����ǂ���

			mob_db[class_].view_class = class_;

			// ���������́Amob_db_add�ł͋L�q�̂��镔���̂ݔ��f
			if(!cov || strlen(str[num+1]) > 0) {
				strncpy(mob_db[class_].name,str[num+1],24);
				mob_db[class_].name[23] = '\0';		// force \0 terminal
			}
			if(!cov || strlen(str[num+2]) > 0) {
				strncpy(mob_db[class_].jname,str[num+2],24);
				mob_db[class_].jname[23] = '\0';	// force \0 terminal
			}
			DB_ADD(mob_db[class_].lv,        num+3);
			DB_ADD(mob_db[class_].max_hp,    num+4);
			DB_ADD(mob_db[class_].max_sp,    num+5);
			DB_ADD(mob_db[class_].base_exp,  num+6);
			DB_ADD(mob_db[class_].job_exp,   num+7);
			DB_ADD(mob_db[class_].range,     num+8);
			DB_ADD(mob_db[class_].atk1,      num+9);
			DB_ADD(mob_db[class_].atk2,     (num=10));
			DB_ADD(mob_db[class_].def,       num+1);
			DB_ADD(mob_db[class_].mdef,      num+2);
			DB_ADD(mob_db[class_].str,       num+3);
			DB_ADD(mob_db[class_].agi,       num+4);
			DB_ADD(mob_db[class_].vit,       num+5);
			DB_ADD(mob_db[class_].int_,      num+6);
			DB_ADD(mob_db[class_].dex,       num+7);
			DB_ADD(mob_db[class_].luk,       num+8);
			DB_ADD(mob_db[class_].range2,    num+9);
			DB_ADD(mob_db[class_].range3,   (num=20));
			DB_ADD(mob_db[class_].size,      num+1);
			DB_ADD(mob_db[class_].race,      num+2);
			DB_ADD(mob_db[class_].element,   num+3);
			DB_ADD(mob_db[class_].mode,      num+4);
			DB_ADD(mob_db[class_].speed,     num+5);
			DB_ADD(mob_db[class_].adelay,    num+6);
			DB_ADD(mob_db[class_].amotion,   num+7);
			DB_ADD(mob_db[class_].dmotion,  (num=28));
			num++;

			// �A�C�e���h���b�v�̐ݒ�
			for(i=0; i<ITEM_DROP_COUNT; i++,num+=2) {
				int nameid;
				if(!cov || strlen(str[num]) > 0)
					nameid = atoi(str[num]);
				else
					nameid = mob_db[class_].dropitem[i].nameid;
				mob_db[class_].dropitem[i].nameid = (nameid == 0)? 512: nameid;	// id=0�́A�����S�ɒu������
				if(cov && strlen(str[num+1]) == 0)
					continue;
				mob_db[class_].dropitem[i].p = atoi(str[num+1]);
			}
			num = 29+ITEM_DROP_COUNT*2;

			DB_ADD(mob_db[class_].mexp,    num);
			DB_ADD(mob_db[class_].mexpper, num+1);

			num += 2;
			for(i=0; i<3; i++,num+=2) {
				DB_ADD(mob_db[class_].mvpitem[i].nameid, num);
				if(cov && strlen(str[num+1]) == 0)
					continue;
				mob_db[class_].mvpitem[i].p = atoi(str[num+1])*battle_config.mvp_item_rate/100;
			}

			for(i=0; i<MAX_RANDOMMONSTER; i++) {
				mob_db[class_].summonper[i] = 0;
			}
			mob_db[class_].maxskill      = 0;
			mob_db[class_].view_size     = 0;
			mob_db[class_].sex           = 0;
			mob_db[class_].hair          = 0;
			mob_db[class_].hair_color    = 0;
			mob_db[class_].clothes_color = 0;
			mob_db[class_].weapon        = 0;
			mob_db[class_].shield        = 0;
			mob_db[class_].head_top      = 0;
			mob_db[class_].head_mid      = 0;
			mob_db[class_].head_bottom   = 0;
		}
		fclose(fp);
		printf("read %s done\n",filename[n]);
	}

	// group_db
	fp=fopen("db/mob_group_db.txt","r");
	if(fp==NULL)
		return 0;	// �����Ă����� group_id = 0 �����ނ̂���

	while(fgets(line,1020,fp)){
		int class_,i;
		char *str[3],*p,*np;

		if(line[0] == '/' && line[1] == '/')
			continue;

		for(i=0,p=line;i<3;i++){
			if((np=strchr(p,','))!=NULL)
			{
				str[i]=p;
				*np=0;
				p=np+1;
			} else
				str[i]=p;
		}

		class_ = atoi(str[0]);
		if(!mobdb_checkid(class_))
			continue;

		mob_db[class_].group_id = atoi(str[2]);
	}
	fclose(fp);

	printf("read db/mob_group_db.txt done\n");

	return 0;
}

/*==========================================
 * MOB�\���O���t�B�b�N�ύX�f�[�^�ǂݍ���
 *------------------------------------------
 */
static int mob_readdb_mobavail(void)
{
	FILE *fp;
	char line[1024];
	int ln = 0;
	int class_,j,k;
	char *str[14],*p,*np;

	if( (fp=fopen("db/mob_avail.txt","r"))==NULL ){
		printf("can't read db/mob_avail.txt\n");
		return -1;
	}

	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(str,0,sizeof(str));

		for(j=0,p=line;j<14;j++){
			if((np=strchr(p,','))!=NULL){
				str[j]=p;
				*np=0;
				p=np+1;
			} else {
				str[j]=p;
			}
		}
		if(str[0] == NULL)
			continue;

		class_ = atoi(str[0]);

		if(!mobdb_checkid(class_))	// �l���ُ�Ȃ珈�����Ȃ��B
			continue;
		k = atoi(str[1]);
		if(k >= 0) {
			mob_db[class_].view_class = k;
			if(k < MAX_VALID_PC_CLASS)
				mob_db[class_].pcview_flag = 1;
			else
				mob_db[class_].pcview_flag = 0;
		}
		mob_db[class_].view_size = atoi(str[2]);

		if(mob_db[class_].pcview_flag) {
			mob_db[class_].sex           = atoi(str[3]);
			mob_db[class_].hair          = atoi(str[4]);
			mob_db[class_].hair_color    = atoi(str[5]);
			mob_db[class_].clothes_color = atoi(str[6]);
			mob_db[class_].weapon        = atoi(str[7]);
			mob_db[class_].shield        = atoi(str[8]);
			mob_db[class_].head_top      = atoi(str[9]);
			mob_db[class_].head_mid      = atoi(str[10]);
			mob_db[class_].head_bottom   = atoi(str[11]);
			mob_db[class_].option        = ((unsigned int)atoi(str[12])) & ~0x46UL;

			mob_db[class_].view_class = pc_calc_class_job(mob_db[class_].view_class, atoi(str[13]));
		}
		ln++;
	}
	fclose(fp);
	printf("read db/mob_avail.txt done (count=%d)\n",ln);
	return 0;
}

/*==========================================
 * �����_�������X�^�[�f�[�^�̓ǂݍ���
 *------------------------------------------
 */
static int mob_read_randommonster(void)
{
	FILE *fp;
	char line[1024];
	char *str[3],*p;
	int i,j;

	const char* mobfile[MAX_RANDOMMONSTER] = {
		"db/mob_branch.txt",
		"db/mob_poring.txt",
		"db/mob_boss.txt",
	};

	for(i=0; i<MAX_RANDOMMONSTER; i++) {
		mob_dummy_class[i] = 1002;	// �ݒ肵�Y�ꂽ�ꍇ�̓|�������o��悤�ɂ��Ă���
		fp=fopen(mobfile[i],"r");
		if(fp==NULL){
			printf("can't read %s\n",mobfile[i]);
			return -1;
		}
		while(fgets(line,1020,fp)){
			int class_,per;
			if(line[0] == '/' && line[1] == '/')
				continue;
			memset(str,0,sizeof(str));
			for(j=0,p=line;j<3 && p;j++){
				str[j]=p;
				p=strchr(p,',');
				if(p) *p++=0;
			}
			if(str[0] == NULL || str[2] == NULL)
				continue;

			class_ = atoi(str[0]);
			per    = atoi(str[2]);

			if(class_ == 0)
				mob_dummy_class[i] = per;
			else if(mobdb_checkid(class_))
				mob_db[class_].summonper[i] = per;
		}
		fclose(fp);
		printf("read %s done\n",mobfile[i]);
	}
	return 0;
}

/*==========================================
 * db/mob_skill_db.txt�ǂݍ���
 *------------------------------------------
 */
static int mob_readskilldb(void)
{
	FILE *fp;
	char line[1024];
	int i;

	const struct {
		char str[32];
		int id;
	} cond1[] = {
		{ "always",            MSC_ALWAYS            },
		{ "myhpltmaxrate",     MSC_MYHPLTMAXRATE     },
		{ "myhpgtmaxrate",     MSC_MYHPGTMAXRATE     },
		{ "friendhpltmaxrate", MSC_FRIENDHPLTMAXRATE },
		{ "friendhpgtmaxrate", MSC_FRIENDHPGTMAXRATE },
		{ "mystatuson",        MSC_MYSTATUSON        },
		{ "mystatusoff",       MSC_MYSTATUSOFF       },
		{ "friendstatuson",    MSC_FRIENDSTATUSON    },
		{ "friendstatusoff",   MSC_FRIENDSTATUSOFF   },
		{ "attackpcgt",        MSC_ATTACKPCGT        },
		{ "attackpcge",        MSC_ATTACKPCGE        },
		{ "slavelt",           MSC_SLAVELT           },
		{ "slavele",           MSC_SLAVELE           },
		{ "closedattacked",    MSC_CLOSEDATTACKED    },
		{ "longrangeattacked", MSC_LONGRANGEATTACKED },
		{ "skillused",         MSC_SKILLUSED         },
		{ "casttargeted",      MSC_CASTTARGETED      },
		{ "targethpgtmaxrate", MSC_TARGETHPGTMAXRATE },
		{ "targethpltmaxrate", MSC_TARGETHPLTMAXRATE },
		{ "targethpgt",        MSC_TARGETHPGT        },
		{ "targethplt",        MSC_TARGETHPLT        },
		{ "targetstatuson",    MSC_TARGETSTATUSON    },
		{ "targetstatusoff",   MSC_TARGETSTATUSOFF   },
		{ "masterhpgtmaxrate", MSC_MASTERHPGTMAXRATE },
		{ "masterhpltmaxrate", MSC_MASTERHPLTMAXRATE },
		{ "masterstatuson",    MSC_MASTERSTATUSON    },
		{ "masterstatusoff",   MSC_MASTERSTATUSOFF   },
		{ "areaslavegt",       MSC_AREASLAVEGT       },
		{ "areaslavele",       MSC_AREASLAVELE       },
		{ "rudeattacked",      MSC_RUDEATTACKED      },
		{ "onspawn",           MSC_SPAWN             },
	}, cond2[] = {
		{ "anybad",            -1                    },
		{ "stone",             SC_STONE              },
		{ "freeze",            SC_FREEZE             },
		{ "stun",              SC_STUN               },
		{ "sleep",             SC_SLEEP              },
		{ "poison",            SC_POISON             },
		{ "curse",             SC_CURSE              },
		{ "silence",           SC_SILENCE            },
		{ "confusion",         SC_CONFUSION          },
		{ "blind",             SC_BLIND              },
		{ "hiding",            SC_HIDING             },
		{ "sight",             SC_SIGHT              },
		{ "lexaeterna",        SC_AETERNA            },
	}, state[] = {
		{ "any",               MSS_ANY               },
		{ "idle",              MSS_IDLE              },
		{ "walk",              MSS_WALK              },
		{ "attack",            MSS_ATTACK            },
		{ "dead",              MSS_DEAD              },
		{ "loot",              MSS_LOOT              },
		{ "chase",             MSS_CHASE             },
		{ "command",           MSS_COMMANDONLY       },
	}, target[] = {
		{ "target",            MST_TARGET            },
		{ "self",              MST_SELF              },
		{ "friend",            MST_FRIEND            },
		{ "master",            MST_MASTER            },
		{ "slave",             MST_SLAVE             },
		{ "command",           MST_COMMAND           },
		{ "modechange",        MST_MODECHANGE        },
		{ "targetchange",      MST_TARGETCHANGE      },
		{ "anothertarget",     MST_ANOTHERTARGET     },
		{ "around5",           MST_AROUND5           },
		{ "around6",           MST_AROUND6           },
		{ "around7",           MST_AROUND7           },
		{ "around8",           MST_AROUND8           },
		{ "around1",           MST_AROUND1           },
		{ "around2",           MST_AROUND2           },
		{ "around3",           MST_AROUND3           },
		{ "around4",           MST_AROUND4           },
		{ "around",            MST_AROUND            },
	}, command_target[] = {
		{ "target",            MCT_TARGET            },
		{ "self",              MCT_SELF              },
		{ "commander",         MCT_COMMANDER         },
		{ "slave",             MCT_SLAVE             },
		{ "slaves",            MCT_SLAVES            },
		{ "group",             MCT_GROUP             },
		{ "friend",            MCT_FRIEND            },
		{ "friends",           MCT_FRIENDS           },
		{ "master",            MCT_MASTER            },
	};

	int x, lineno;
	const char *filename[] = { "db/mob_skill_db.txt","db/addon/mob_skill_db_add.txt" };

	for(x=0;x<2;x++){
		fp=fopen(filename[x],"r");
		if(fp==NULL){
			if(x==0)
				printf("can't read %s\n",filename[x]);
			continue;
		}
		lineno = 0;
		while(fgets(line,1020,fp)){
			char *sp[18],*p;
			int mob_id;
			struct mob_skill *ms = NULL;
			int j=0;

			lineno++;
			if(line[0] == '/' && line[1] == '/')
				continue;

			memset(sp,0,sizeof(sp));
			for(i=0,p=line;i<18 && p;i++){
				sp[i]=p;
				if((p=strchr(p,','))!=NULL) {
					*p++=0;
				} else {
					break;
				}
			}

			mob_id = atoi(sp[0]);
			if( !mobdb_checkid(mob_id) ) {
				printf("mob_skill: invalid mob id(%d) line %d\n", mob_id, lineno);
				continue;
			}

			if( strcmp(sp[1],"clear") == 0 ) {
				memset(mob_db[mob_id].skill,0,sizeof(mob_db[mob_id].skill));
				mob_db[mob_id].maxskill = 0;
				continue;
			}

			if( i != 17 ) {
				printf("mob_skill: invalid param count(%d) line %d\n", i, lineno);
				continue;
			}

			for(i=0; i<MAX_MOBSKILL; i++) {
				ms = &mob_db[mob_id].skill[i];
				if( ms->skill_id == 0 )
					break;
			}
			if(!ms || i >= MAX_MOBSKILL) {
				printf("mob_skill: readdb: too many skill ! [%s] in %d[%s]\n", sp[1], mob_id, mob_db[mob_id].jname);
				continue;
			}

			for(j=0; j<sizeof(state)/sizeof(state[0]); j++) {
				if( strcmp(sp[2],state[j].str) == 0) {
					ms->state = state[j].id;
					break;
				}
			}
			if( j == sizeof(state)/sizeof(state[0]) ) {
				ms->state = MSS_DISABLE;	// �����ɂ���
				printf("mob_skill: unknown state %s line %d\n", sp[2], lineno);
			}
			ms->skill_id   = atoi(sp[3]);
			ms->skill_lv   = atoi(sp[4]);
			ms->permillage = atoi(sp[5]);
			ms->casttime   = atoi(sp[6]);
			ms->delay      = atoi(sp[7]);
			ms->cancel     = atoi(sp[8]);
			if( strcmp(sp[8],"yes") == 0 )
				ms->cancel = 1;

			for(j=0; j<sizeof(target)/sizeof(target[0]); j++) {
				if( strcmp(sp[9],target[j].str) == 0) {
					ms->target = target[j].id;
					break;
				}
			}
			if( j == sizeof(target)/sizeof(target[0]) ) {
				ms->state = MSS_DISABLE;	// �����ɂ���
				printf("mob_skill: unknown target %s line %d\n", sp[9], lineno);
			}

			for(j=0; j<sizeof(cond1)/sizeof(cond1[0]); j++) {
				if( strcmp(sp[10],cond1[j].str) == 0) {
					ms->cond1 = cond1[j].id;
					break;
				}
			}
			if( j == sizeof(cond1)/sizeof(cond1[0]) ) {
				ms->state = MSS_DISABLE;	// �����ɂ���
				printf("mob_skill: unknown cond1 %s line %d\n", sp[10], lineno);
			}

			ms->cond2 = (short)strtol(sp[11],NULL,0);
			for(j=0; j<sizeof(cond2)/sizeof(cond2[0]); j++) {
				if( strcmp(sp[11],cond2[j].str) == 0) {
					ms->cond2 = cond2[j].id;
					break;
				}
			}
			switch(ms->cond1) {
			case MSC_MYSTATUSON:
			case MSC_MYSTATUSOFF:
			case MSC_FRIENDSTATUSON:
			case MSC_FRIENDSTATUSOFF:
			case MSC_TARGETSTATUSON:
			case MSC_TARGETSTATUSOFF:
			case MSC_MASTERSTATUSON:
			case MSC_MASTERSTATUSOFF:
				if( j == sizeof(cond2)/sizeof(cond2[0]) ) {
					ms->state = MSS_DISABLE;	// �����ɂ���
					printf("mob_skill: invalid combination (%s,%s) line %d\n", sp[10], sp[11], lineno);
				}
				break;
			default:
				if( j != sizeof(cond2)/sizeof(cond2[0]) ) {
					ms->state = MSS_DISABLE;	// �����ɂ���
					printf("mob_skill: invalid combination (%s,%s) line %d\n", sp[10], sp[11], lineno);
				}
				break;
			}

			ms->val[0] = atoi(sp[12]);
			for(j=0; j<sizeof(command_target)/sizeof(command_target[0]); j++) {
				if( strcmp(sp[12],command_target[j].str) == 0) {
					ms->val[0] = command_target[j].id;
					break;
				}
			}
			if( j == sizeof(command_target)/sizeof(command_target[0]) ) {
				if( isalpha((unsigned char)sp[12][0]) ) {
					ms->state = MSS_DISABLE;	// �����ɂ���
					printf("mob_skill: unknown val0 %s line %d\n", sp[12], lineno);
				}
			}

			ms->val[1] = atoi(sp[13]);
			for(j=0; j<sizeof(command_target)/sizeof(command_target[0]); j++) {
				if( strcmp(sp[13],command_target[j].str) == 0) {
					ms->val[1] = command_target[j].id;
					break;
				}
			}
			if( j == sizeof(command_target)/sizeof(command_target[0]) ) {
				if( isalpha((unsigned char)sp[13][0]) ) {
					ms->state = MSS_DISABLE;	// �����ɂ���
					printf("mob_skill: unknown val1 %s line %d\n", sp[13], lineno);
				}
			}

			ms->val[2] = atoi(sp[14]);
			for(j=0; j<sizeof(command_target)/sizeof(command_target[0]); j++) {
				if( strcmp(sp[14],command_target[j].str) == 0) {
					ms->val[2] = command_target[j].id;
					break;
				}
			}
			if( j == sizeof(command_target)/sizeof(command_target[0]) ) {
				if( isalpha((unsigned char)sp[14][0]) ) {
					ms->state = MSS_DISABLE;	// �����ɂ���
					printf("mob_skill: unknown val2 %s line %d\n", sp[14], lineno);
				}
			}

			ms->val[3] = atoi(sp[15]);
			ms->val[4] = atoi(sp[16]);
			if(strlen(sp[17]) > 2)
				ms->emotion = atoi(sp[17]);
			else
				ms->emotion = -1;
			mob_db[mob_id].maxskill = i+1;
		}
		fclose(fp);
		printf("read %s done\n",filename[x]);
	}
	return 0;
}

/*==========================================
 * �����[�h
 *------------------------------------------
 */
void mob_reload(void)
{
	mob_readdb();
	mob_readdb_mobavail();
	mob_read_randommonster();
	mob_readskilldb();
}

/*==========================================
 * mob���菉����
 *------------------------------------------
 */
int do_init_mob(void)
{
	mob_readdb();
	mob_readdb_mobavail();
	mob_read_randommonster();
	mob_readskilldb();

	add_timer_func_list(mob_delayspawn,"mob_delayspawn");
	add_timer_func_list(mob_delay_item_drop,"mob_delay_item_drop");
	add_timer_func_list(mob_delay_item_drop2,"mob_delay_item_drop2");
	add_timer_func_list(mob_ai_hard,"mob_ai_hard");
	add_timer_func_list(mob_ai_lazy,"mob_ai_lazy");
	add_timer_func_list(mob_timer_delete,"mob_timer_delete");
	add_timer_interval(gettick()+MIN_MOBTHINKTIME,mob_ai_hard,0,0,MIN_MOBTHINKTIME);
	add_timer_interval(gettick()+MIN_MOBTHINKTIME*20,mob_ai_lazy,0,0,MIN_MOBTHINKTIME*20);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int do_final_mob(void)
{
	aFree( mob_ai_hard_buf );
	aFree( mob_ai_hard_next_id );
	return 0;
}
