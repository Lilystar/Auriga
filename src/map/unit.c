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

#include "db.h"
#include "timer.h"
#include "nullpo.h"
#include "malloc.h"
#include "utils.h"

#include "map.h"
#include "pc.h"
#include "mob.h"
#include "pet.h"
#include "homun.h"
#include "skill.h"
#include "unit.h"
#include "battle.h"
#include "status.h"
#include "storage.h"
#include "clif.h"
#include "party.h"
#include "npc.h"
#include "chat.h"
#include "trade.h"
#include "guild.h"
#include "friend.h"
#include "mob.h"
#include "vending.h"
#include "intif.h"
#include "mail.h"
#include "merc.h"

static int unit_walktoxy_timer(int tid,unsigned int tick,int id,void *data);
static int unit_attack_timer(int tid,unsigned int tick,int id,void *data);

/*==========================================
 * ��_�Ԃ̋�����Ԃ�
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int unit_distance(int x0,int y0,int x1,int y1)
{
	int dx,dy;

	dx = abs(x0 - x1);
	dy = abs(y0 - y1);

	return (dx > dy) ? dx : dy;
}

int unit_distance2( struct block_list *bl, struct block_list *bl2)
{
	nullpo_retr(0, bl);
	nullpo_retr(0, bl2);

	return unit_distance(bl->x,bl->y,bl2->x,bl2->y);
}

/*==========================================
 *
 *------------------------------------------
 */
struct unit_data* unit_bl2ud(struct block_list *bl)
{
	if( bl == NULL ) return NULL;
	if( bl->type == BL_PC   ) return &((struct map_session_data*)bl)->ud;
	if( bl->type == BL_MOB  ) return &((struct mob_data*)bl)->ud;
	if( bl->type == BL_PET  ) return &((struct pet_data*)bl)->ud;
	if( bl->type == BL_HOM  ) return &((struct homun_data*)bl)->ud;
	if( bl->type == BL_MERC ) return &((struct merc_data*)bl)->ud;
	return NULL;
}

/*==========================================
 *
 *------------------------------------------
 */
static int unit_walktoxy_sub(struct block_list *bl)
{
	int i;
	struct walkpath_data wpd;
	struct map_session_data *sd = NULL;
	struct unit_data        *ud = NULL;
	struct status_change    *sc = NULL;

	nullpo_retr(0, bl);

	ud = unit_bl2ud(bl);
	if(ud == NULL)
		return 0;

	if(bl->type == BL_PC)
		sd = (struct map_session_data *)bl;

	if(sd && pc_iscloaking(sd))	// �N���[�L���O���Čv�Z
		status_calc_pc(sd,0);

	sc = status_get_sc(bl);
	if(sc && sc->data[SC_FORCEWALKING].timer != -1) {
		if(path_search2(&wpd,bl->m,bl->x,bl->y,ud->to_x,ud->to_y,0))
			return 0;
	} else {
		if(path_search(&wpd,bl->m,bl->x,bl->y,ud->to_x,ud->to_y,0))
			return 0;
	}

	if(bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)bl;
		if(md) {
			int x = md->bl.x+dirx[wpd.path[0]];
			int y = md->bl.y+diry[wpd.path[0]];
			if (map_getcell(bl->m,x,y,CELL_CHKBASILICA) && !(status_get_mode(bl)&0x20)) {
				ud->state.change_walk_target=0;
				return 0;
			}
		}
	}

	memcpy(&ud->walkpath,&wpd,sizeof(wpd));

	if(sd) {
		clif_walkok(sd);
	}
	clif_move(bl);

	ud->state.change_walk_target = 0;

	if(ud->walkpath.path_pos>=ud->walkpath.path_len)
		i = -1;
	else if(ud->walkpath.path[ud->walkpath.path_pos]&1)
		i = status_get_speed(bl)*14/10;
	else
		i = status_get_speed(bl);
	if(i > 0) {
		ud->walktimer = add_timer(gettick()+i,unit_walktoxy_timer,bl->id,(void*)0);
	}

	return 1;
}

/*==========================================
 *
 *------------------------------------------
 */
static int unit_walktoxy_timer(int tid,unsigned int tick,int id,void *data)
{
	int i;
	int moveblock;
	int x,y,dx,dy,dir;
	struct block_list       *bl  = NULL;
	struct map_session_data *sd  = NULL;
	struct pet_data         *pd  = NULL;
	struct mob_data         *md  = NULL;
	struct homun_data       *hd  = NULL;
	struct merc_data        *mcd = NULL;
	struct unit_data        *ud  = NULL;
	struct status_change    *sc  = NULL;

	bl=map_id2bl(id);
	if(bl == NULL)
		return 0;
	if( (sd = BL_DOWNCAST( BL_PC,  bl ) ) ) {
		ud = &sd->ud;
	} else if( (md = BL_DOWNCAST( BL_MOB, bl ) ) ) {
		ud = &md->ud;
	} else if( (pd = BL_DOWNCAST( BL_PET, bl ) ) ) {
		ud = &pd->ud;
	} else if( (hd = BL_DOWNCAST( BL_HOM, bl ) ) ) {
		ud = &hd->ud;
	} else if( (mcd = BL_DOWNCAST( BL_MERC, bl ) ) ) {
		ud = &mcd->ud;
	}
	if(ud == NULL) return 0;

	if(ud->walktimer != tid) {
		if(battle_config.error_log)
			printf("unit_walk_timer %d != %d\n",ud->walktimer,tid);
		return 0;
	}
	ud->walktimer = -1;
	if( bl->prev == NULL ) return 0; // block_list ���甲���Ă���̂ňړ���~����

	if(ud->walkpath.path_pos >= ud->walkpath.path_len || ud->walkpath.path_pos != (int)data)
		return 0;

	// �������̂ő����̃^�C�}�[��������
	if(sd) {
		sd->inchealspirithptick = 0;
		sd->inchealspiritsptick = 0;
		sd->state.warp_waiting  = 0;
	}

	sc = status_get_sc(bl);

	x = bl->x;
	y = bl->y;

	dir = ud->walkpath.path[ud->walkpath.path_pos];
	if(sd) pc_setdir(sd, dir, dir);
	else if(md)  md->dir  = dir;
	else if(pd)  pd->dir  = dir;
	else if(hd)  hd->dir  = dir;
	else if(mcd) mcd->dir = dir;

	dx = dirx[(int)dir];
	dy = diry[(int)dir];

	moveblock = map_block_is_differ(bl,bl->m,x+dx,y+dy);

	ud->walktimer = 1;
	if(sd) {
		map_foreachinmovearea(clif_pcoutsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,BL_ALL,sd);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,x-AREA_SIZE*2,y-AREA_SIZE*2,x+AREA_SIZE*2,y+AREA_SIZE*2,dx,dy,BL_MOB,sd,0);
	} else if(md) {
		map_foreachinmovearea(clif_moboutsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,BL_PC,md);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,x-AREA_SIZE*2,y-AREA_SIZE*2,x+AREA_SIZE*2,y+AREA_SIZE*2,dx,dy,BL_PC|BL_HOM|BL_MERC,md,0);
	} else if(pd) {
		map_foreachinmovearea(clif_petoutsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,BL_PC,pd);
	} else if(hd) {
		map_foreachinmovearea(clif_homoutsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,BL_PC,hd);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,x-AREA_SIZE*2,y-AREA_SIZE*2,x+AREA_SIZE*2,y+AREA_SIZE*2,dx,dy,BL_MOB,hd,0);
	} else if(mcd) {
		map_foreachinmovearea(clif_mercoutsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,dx,dy,BL_PC,mcd);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,x-AREA_SIZE*2,y-AREA_SIZE*2,x+AREA_SIZE*2,y+AREA_SIZE*2,dx,dy,BL_MOB,mcd,0);
	}
	ud->walktimer = -1;

	x += dx;
	y += dy;

	if(md && md->min_chase>13)
		md->min_chase--;

	if(!pd) skill_unit_move(bl,tick,0);
	if(moveblock) map_delblock(bl);
	bl->x = x;
	bl->y = y;
	if(moveblock) map_addblock(bl);
	if(!pd) skill_unit_move(bl,tick,1);

	if(sd) {
		if(sd->sc.data[SC_DANCING].timer != -1 && sd->sc.data[SC_LONGINGFREEDOM].timer == -1)	// Not �S�����Ȃ���
		{
			skill_unit_move_unit_group(map_id2sg(sd->sc.data[SC_DANCING].val2),sd->bl.m,dx,dy);
			sd->dance.x += dx;
			sd->dance.y += dy;
		}
		if(sd->sc.data[SC_WARM].timer != -1)
			skill_unit_move_unit_group(map_id2sg(sd->sc.data[SC_WARM].val4),sd->bl.m,dx,dy);
	}

	ud->walktimer = 1;
	if(sd) {
		map_foreachinmovearea(clif_pcinsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_ALL,sd);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,x-AREA_SIZE*2,y-AREA_SIZE*2,x+AREA_SIZE*2,y+AREA_SIZE*2,-dx,-dy,BL_MOB,sd,1);
	} else if(md) {
		map_foreachinmovearea(clif_mobinsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,md);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,x-AREA_SIZE*2,y-AREA_SIZE*2,x+AREA_SIZE*2,y+AREA_SIZE*2,-dx,-dy,BL_PC|BL_HOM|BL_MERC,md,1);
	} else if(pd) {
		map_foreachinmovearea(clif_petinsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,pd);
	} else if(hd) {
		map_foreachinmovearea(clif_hominsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,hd);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,x-AREA_SIZE*2,y-AREA_SIZE*2,x+AREA_SIZE*2,y+AREA_SIZE*2,-dx,-dy,BL_MOB,hd,1);
	} else if(mcd) {
		map_foreachinmovearea(clif_mercinsight,bl->m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,mcd);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,x-AREA_SIZE*2,y-AREA_SIZE*2,x+AREA_SIZE*2,y+AREA_SIZE*2,-dx,-dy,BL_MOB,mcd,1);
	}
	ud->walktimer = -1;

	if(sd) {
		if(sd->status.party_id > 0 && party_search(sd->status.party_id) != NULL) {	// �p�[�e�B�̂g�o���ʒm����
			if(map_foreachinmovearea(party_send_hp_check,sd->bl.m,x-AREA_SIZE,y-AREA_SIZE,x+AREA_SIZE,y+AREA_SIZE,-dx,-dy,BL_PC,sd->status.party_id)) {
				sd->party_hp = -1;
			}
		}

		/* �f�B�{�[�V�������� */
		for(i=0; i<5; i++) {
			if(sd->dev.val1[i]) {
				skill_devotion3(sd,sd->dev.val1[i]);
			}
		}

		if(sd->sc.count > 0)
		{
			/* ��f�B�{�[�V�������� */
			if(sd->sc.data[SC_DEVOTION].timer != -1) {
				skill_devotion2(&sd->bl,sd->sc.data[SC_DEVOTION].val1);
			}
			/* �}���I�l�b�g���� */
			if(sd->sc.data[SC_MARIONETTE].timer != -1) {
				skill_marionette(sd,sd->sc.data[SC_MARIONETTE].val2);
			}
			/* ��}���I�l�b�g���� */
			if(sd->sc.data[SC_MARIONETTE2].timer != -1) {
				skill_marionette2(sd,sd->sc.data[SC_MARIONETTE2].val2);
			}
			/* �_���X�`�F�b�N */
			if(sd->sc.data[SC_LONGINGFREEDOM].timer != -1) {
				// �͈͊O�ɏo����~�߂�
				if(unit_distance(sd->bl.x,sd->bl.y,sd->dance.x,sd->dance.y) > 4)
					skill_stop_dancing(&sd->bl,0);
			}
			/* �w�����[�h�`�F�b�N */
			if(battle_config.hermode_wp_check &&
			   sd->sc.data[SC_DANCING].timer != -1 &&
			   sd->sc.data[SC_DANCING].val1 == CG_HERMODE) {
				if(skill_hermode_wp_check(&sd->bl) == 0)
					skill_stop_dancing(&sd->bl,0);
			}
			/* �N���[�L���O�̏��Ō��� */
			if(pc_iscloaking(sd) && sd->sc.data[SC_CLOAKING].timer != -1) {
				if(sd->sc.data[SC_CLOAKING].val1 < 3)
					skill_check_cloaking(&sd->bl);
			}
			/* �^�C���M�����J�E���g */
			if(sd->sc.data[SC_RUN].timer != -1) {
				sd->sc.data[SC_RUN].val4++;
			}
			/* ��]�J�E���g���Z�b�g */
			if(sd->sc.data[SC_ROLLINGCUTTER].timer != -1) {
				status_change_end(&sd->bl,SC_ROLLINGCUTTER,-1);
			}
		}
		// �M���h�X�L���L��
		pc_check_guild_skill_effective_range(sd);

		if(map_getcell(sd->bl.m,x,y,CELL_CHKNPC))
			npc_touch_areanpc(sd,sd->bl.m,x,y);
		else
			sd->areanpc_id = 0;
	}
	else if(md) {
		if(md->sc.option&4)
			skill_check_cloaking(&md->bl);

		if(map_getcell(md->bl.m,x,y,CELL_CHKNPC))
			npc_touch_areanpc2(md,md->bl.m,x,y);
		else
			md->areanpc_id = 0;
	}

	ud->walkpath.path_pos++;
	if(ud->state.change_walk_target) {
		unit_walktoxy_sub(bl);
		return 0;
	}

	if(ud->walkpath.path_pos >= ud->walkpath.path_len)
		i = -1;
	else if(ud->walkpath.path[ud->walkpath.path_pos] & 1)
		i = status_get_speed(bl)*14/10;
	else
		i = status_get_speed(bl);

	if(i > 0) {
		// �ړ��r��
		dir = ud->walkpath.path[ud->walkpath.path_pos];
		dx  = dirx[(int)dir];
		dy  = diry[(int)dir];

		if(sd && sd->sc.data[SC_RUN].timer != -1) {
			// �^�C���M�̏�Q���ɓ�������
			if(map_getcell(sd->bl.m,x+dx,y+dy,CELL_CHKNOPASS) ||
			   map_getcell(sd->bl.m,x   ,y+dy,CELL_CHKNOPASS) ||
			   map_getcell(sd->bl.m,x+dx,y   ,CELL_CHKNOPASS) ||
			   map_count_oncell(sd->bl.m,x+dx,y+dy,BL_PC|BL_MOB|BL_NPC) > 0) {
				skill_blown(&sd->bl,&sd->bl,skill_get_blewcount(TK_RUN,sd->sc.data[SC_RUN].val1)|SAB_NODAMAGE);
				status_change_end(&sd->bl,SC_RUN,-1);
				clif_status_change(&sd->bl,SI_RUN_STOP,1,0,0);
				pc_setdir(sd, dir, dir);
				return 0;
			}
		} else if(map_getcell(bl->m,x+dx,y+dy,CELL_CHKNOPASS)) {	// ��Q���ɓ�������
			if(!sc || sc->data[SC_FORCEWALKING].timer == -1) {
				clif_fixwalkpos(bl);
				return 0;
			}
		}
		ud->walktimer = add_timer(tick+i,unit_walktoxy_timer,id,(void*)((int)ud->walkpath.path_pos));
	} else {
		// �ړI�n�ɒ�����
		if(sd && sd->sc.data[SC_RUN].timer != -1) {
			// �p������
			pc_runtodir(sd);
		}
		else if(md && md->sc.data[SC_SELFDESTRUCTION].timer != -1) {
			md->dir = md->sc.data[SC_SELFDESTRUCTION].val4;
			unit_walktodir(bl,1);
		}
		else if(sc && sc->data[SC_FORCEWALKING].timer != -1) {
			if(sc->data[SC_FORCEWALKING].val4 == 0) {
				sc->data[SC_FORCEWALKING].val4++;
				unit_walktodir(bl,1);
			}
			else if(sc->data[SC_FORCEWALKING].val4 == 1) {
				sc->data[SC_FORCEWALKING].val4++;
			}
		}
		else if(md && md->target_id > 0) {
			// Mob��AI�����s
			md->last_thinktime = tick + MIN_MOBTHINKTIME;
			mob_ai_sub_hard(md, tick);
		}

		// �Ƃ܂����Ƃ��̈ʒu�̍đ��M�͕s�v�i�J�N�J�N���邽�߁j
		// clif_fixwalkpos(bl);
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int unit_walktoxy(struct block_list *bl, int x, int y)
{
	struct unit_data        *ud  = NULL;
	struct map_session_data *sd  = NULL;
	struct pet_data         *pd  = NULL;
	struct mob_data         *md  = NULL;
	struct homun_data       *hd  = NULL;
	struct merc_data        *mcd = NULL;
	struct status_change    *sc  = NULL;

	nullpo_retr(0, bl);

	if( (sd = BL_DOWNCAST( BL_PC,  bl ) ) ) {
		ud = &sd->ud;
	} else if( (md = BL_DOWNCAST( BL_MOB, bl ) ) ) {
		ud = &md->ud;
	} else if( (pd = BL_DOWNCAST( BL_PET, bl ) ) ) {
		ud = &pd->ud;
	} else if( (hd = BL_DOWNCAST( BL_HOM, bl ) ) ) {
		ud = &hd->ud;
	} else if( (mcd = BL_DOWNCAST( BL_MERC, bl ) ) ) {
		ud = &mcd->ud;
	}
	if( ud == NULL ) return 0;

	// �ړ��o���Ȃ����j�b�g�͒e��(�y�b�g�͏���)
	if( !pd && !(status_get_mode(bl)&0x01) )
		return 0;

	if( !unit_can_move(bl) )
		return 0;

	sc = status_get_sc(bl);

	// �����ړ����͍������珜�O
	if(sc && sc->data[SC_CONFUSION].timer != -1 && sc->data[SC_FORCEWALKING].timer == -1) {
		ud->to_x = bl->x + atn_rand()%7 - 3;
		ud->to_y = bl->y + atn_rand()%7 - 3;
	} else {
		ud->to_x = x;
		ud->to_y = y;
	}

	if(ud->walktimer != -1) {
		// ���ݕ����Ă���Œ��̖ړI�n�ύX�Ȃ̂Ń}�X�ڂ̒��S�ɗ�������
		// timer�֐�����unit_walktoxy_sub���ĂԂ悤�ɂ���
		ud->state.change_walk_target = 1;
		return 1;
	}
	return unit_walktoxy_sub(bl);
}

/*==========================================
 * �����Ă��������step������
 *------------------------------------------
 */
int unit_walktodir(struct block_list *bl,int step)
{
	int i,to_x,to_y,dir_x,dir_y,d;

	nullpo_retr(0, bl);

	d = status_get_dir(bl);

	to_x = bl->x;
	to_y = bl->y;
	dir_x = dirx[d];
	dir_y = diry[d];

	for(i=0; i<step; i++) {
		if(!map_getcell(bl->m,to_x+dir_x,to_y+dir_y,CELL_CHKPASS))
			break;

		// ���̃Z���ւP���ňړ��\�łȂ��Ȃ�
		if(map_getcell(bl->m,to_x+dir_x,to_y+dir_y,CELL_CHKNOPASS) ||
		   map_getcell(bl->m,to_x      ,to_y+dir_y,CELL_CHKNOPASS) ||
		   map_getcell(bl->m,to_x+dir_x,to_y      ,CELL_CHKNOPASS))
			break;

		to_x += dir_x;
		to_y += dir_y;
	}
	unit_walktoxy(bl, to_x, to_y);

	return 1;
}

/*==========================================
 * �i���s�Z���𖳎����ĕ����v��
 *------------------------------------------
 */
int unit_forcewalktodir(struct block_list *bl,int distance)
{
	struct status_change *sc = NULL;
	int i,to_x,to_y,dir_x,dir_y,d;

	nullpo_retr(0, bl);

	sc = status_get_sc(bl);
	d  = status_get_dir(bl);

	to_x = bl->x;
	to_y = bl->y;
	dir_x = dirx[d];
	dir_y = diry[d];

	for(i=distance; i>1; i--) {
		if(map_getcell(bl->m,bl->x+dir_x*i,bl->y+dir_y*i,CELL_CHKPASS)) {
			to_x = bl->x+dir_x*(i-1);
			to_y = bl->y+dir_y*(i-1);
			break;
		}
	}

	if(sc) {
		sc->data[SC_FORCEWALKING].val4 = 0;
	}
	unit_walktoxy(bl, to_x, to_y);

	return 1;
}

/*==========================================
 * �ʒu�ړ�
 *   flag -> 0xXY
 *	X : �o�H�������@�A0,1,2
 *	Y : �p�P�b�g�̎�ށA0,1
 *------------------------------------------
 */
int unit_movepos(struct block_list *bl,int dst_x,int dst_y,int flag)
{
	int moveblock;
	int dx,dy,dir,x[4],y[4];
	unsigned int tick = gettick();
	struct map_session_data *sd  = NULL;
	struct pet_data         *pd  = NULL;
	struct mob_data         *md  = NULL;
	struct homun_data       *hd  = NULL;
	struct merc_data        *mcd = NULL;
	struct unit_data        *ud  = NULL;

	nullpo_retr(1, bl);

	if( bl->prev == NULL ) return 1;

	if( (sd = BL_DOWNCAST( BL_PC,  bl ) ) ) {
		ud = &sd->ud;
	} else if( (md = BL_DOWNCAST( BL_MOB, bl ) ) ) {
		ud = &md->ud;
	} else if( (pd = BL_DOWNCAST( BL_PET, bl ) ) ) {
		ud = &pd->ud;
	} else if( (hd = BL_DOWNCAST( BL_HOM, bl ) ) ) {
		ud = &hd->ud;
	} else if( (mcd = BL_DOWNCAST( BL_MERC, bl ) ) ) {
		ud = &mcd->ud;
	}
	if( ud == NULL ) return 1;

	unit_stop_walking(bl,1);

	if(ud->attacktimer != -1) {
		delete_timer( ud->attacktimer, unit_attack_timer );
		ud->attacktimer = -1;
	}

	switch ( (flag>>4)&0x0f ) {
		case 0:
			if(path_search(NULL,bl->m,bl->x,bl->y,dst_x,dst_y,0))
				return 1;
			break;
		case 1:
			if(path_search2(NULL,bl->m,bl->x,bl->y,dst_x,dst_y,0))
				return 1;
			break;
		case 2:
			if(path_search3(NULL,bl->m,bl->x,bl->y,dst_x,dst_y,0))
				return 1;
			break;
	}

	dir = map_calc_dir(bl, dst_x, dst_y);
	if(sd)
		pc_setdir(sd, dir, dir);

	// �ʒu�ύX�O�A�ύX�����ʓ��ɂ���N���C�A���g�̍��W���L�^
	x[0] = bl->x-AREA_SIZE;
	x[1] = bl->x+AREA_SIZE;
	x[2] = dst_x-AREA_SIZE;
	x[3] = dst_x+AREA_SIZE;
	y[0] = bl->y-AREA_SIZE;
	y[1] = bl->y+AREA_SIZE;
	y[2] = dst_y-AREA_SIZE;
	y[3] = dst_y+AREA_SIZE;

	dx = dst_x - bl->x;
	dy = dst_y - bl->y;

	moveblock = ( bl->x/BLOCK_SIZE != dst_x/BLOCK_SIZE || bl->y/BLOCK_SIZE != dst_y/BLOCK_SIZE);

	if(sd) {	/* ��ʊO�ɏo���̂ŏ��� */
		map_foreachinmovearea(clif_pcoutsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,dx,dy,BL_ALL,sd);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,bl->x-AREA_SIZE*2,bl->y-AREA_SIZE*2,bl->x+AREA_SIZE*2,bl->y+AREA_SIZE*2,dx,dy,BL_MOB,sd,0);
	} else if(md) {
		map_foreachinmovearea(clif_moboutsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,dx,dy,BL_PC,md);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,bl->x-AREA_SIZE*2,bl->y-AREA_SIZE*2,bl->x+AREA_SIZE*2,bl->y+AREA_SIZE*2,dx,dy,BL_PC|BL_HOM|BL_MERC,md,0);
	} else if(pd) {
		map_foreachinmovearea(clif_petoutsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,dx,dy,BL_PC,pd);
	} else if(hd) {
		map_foreachinmovearea(clif_homoutsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,dx,dy,BL_PC,hd);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,bl->x-AREA_SIZE*2,bl->y-AREA_SIZE*2,bl->x+AREA_SIZE*2,bl->y+AREA_SIZE*2,dx,dy,BL_MOB,hd,0);
	} else if(mcd) {
		map_foreachinmovearea(clif_mercoutsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,dx,dy,BL_PC,mcd);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,bl->x-AREA_SIZE*2,bl->y-AREA_SIZE*2,bl->x+AREA_SIZE*2,bl->y+AREA_SIZE*2,dx,dy,BL_MOB,mcd,0);
	}

	if(!pd) skill_unit_move(bl,tick,0);
	if(moveblock) map_delblock(bl);
	bl->x = dst_x;
	bl->y = dst_y;
	if(moveblock) map_addblock(bl);
	if(!pd) skill_unit_move(bl,tick,1);

	if(sd) {	/* ��ʓ��ɓ����Ă����̂ŕ\�� */
		map_foreachinmovearea(clif_pcinsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,-dx,-dy,BL_ALL,sd);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,bl->x-AREA_SIZE*2,bl->y-AREA_SIZE*2,bl->x+AREA_SIZE*2,bl->y+AREA_SIZE*2,-dx,-dy,BL_MOB,sd,1);
	} else if(md) {
		map_foreachinmovearea(clif_mobinsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,-dx,-dy,BL_PC,md);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,bl->x-AREA_SIZE*2,bl->y-AREA_SIZE*2,bl->x+AREA_SIZE*2,bl->y+AREA_SIZE*2,-dx,-dy,BL_PC|BL_HOM|BL_MERC,md,1);
	} else if(pd) {
		map_foreachinmovearea(clif_petinsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,-dx,-dy,BL_PC,pd);
	} else if(hd) {
		map_foreachinmovearea(clif_hominsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,-dx,-dy,BL_PC,hd);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,bl->x-AREA_SIZE*2,bl->y-AREA_SIZE*2,bl->x+AREA_SIZE*2,bl->y+AREA_SIZE*2,-dx,-dy,BL_MOB,hd,1);
	} else if(mcd) {
		map_foreachinmovearea(clif_mercinsight,bl->m,bl->x-AREA_SIZE,bl->y-AREA_SIZE,bl->x+AREA_SIZE,bl->y+AREA_SIZE,-dx,-dy,BL_PC,mcd);
		map_foreachinmovearea(mob_ai_hard_spawn_sub,bl->m,bl->x-AREA_SIZE*2,bl->y-AREA_SIZE*2,bl->x+AREA_SIZE*2,bl->y+AREA_SIZE*2,-dx,-dy,BL_MOB,mcd,1);
	}

	if( flag&1 )		// ������΂��p�p�P�b�g���M
		clif_blown(bl,dst_x,dst_y);
	else			// �ʒu�ύX��񑗐M
		clif_fixpos2(bl,x,y);

	if(sd) {
		if(sd->status.party_id > 0 && party_search(sd->status.party_id) != NULL) {	// �p�[�e�B�̂g�o���ʒm����
			if(map_foreachinmovearea(party_send_hp_check,sd->bl.m,sd->bl.x-AREA_SIZE,sd->bl.y-AREA_SIZE,sd->bl.x+AREA_SIZE,sd->bl.y+AREA_SIZE,-dx,-dy,BL_PC,sd->status.party_id)) {
				sd->party_hp = -1;
			}
		}

		// �N���[�L���O�̏��Ō���
		if(pc_iscloaking(sd) && sd->sc.data[SC_CLOAKING].timer != -1 && sd->sc.data[SC_CLOAKING].val1 < 3) {
			skill_check_cloaking(&sd->bl);
		}

		// ������̈ʒu�ύX
		if(sd->sc.data[SC_WARM].timer != -1) {
			skill_unit_move_unit_group(map_id2sg(sd->sc.data[SC_WARM].val4),sd->bl.m,dx,dy);
		}

		if(map_getcell(bl->m,bl->x,bl->y,CELL_CHKNPC))
			npc_touch_areanpc(sd,sd->bl.m,sd->bl.x,sd->bl.y);
		else
			sd->areanpc_id=0;
	}

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int unit_setdir(struct block_list *bl,int dir)
{
	nullpo_retr( 0, bl );

	if(bl->type == BL_PC)
		pc_setdir((struct map_session_data *)bl, dir, dir);
	else if(bl->type == BL_MOB)
		((struct mob_data *)bl)->dir = dir;
	else if(bl->type == BL_PET)
		((struct pet_data *)bl)->dir = dir;
	else if(bl->type == BL_HOM)
		((struct homun_data *)bl)->dir = dir;
	else if(bl->type == BL_MERC)
		((struct merc_data *)bl)->dir = dir;

	clif_changedir( bl, dir, dir );
	return 0;
}

/*==========================================
 * ���s��~
 *------------------------------------------
 */
int unit_stop_walking(struct block_list *bl,int type)
{
	struct map_session_data *sd  = NULL;
	struct pet_data         *pd  = NULL;
	struct mob_data         *md  = NULL;
	struct homun_data       *hd  = NULL;
	struct merc_data        *mcd = NULL;
	struct unit_data        *ud  = NULL;

	nullpo_retr(0, bl);

	if( (sd = BL_DOWNCAST( BL_PC,  bl ) ) ) {
		ud = &sd->ud;
	} else if( (md = BL_DOWNCAST( BL_MOB, bl ) ) ) {
		ud = &md->ud;
	} else if( (pd = BL_DOWNCAST( BL_PET, bl ) ) ) {
		ud = &pd->ud;
	} else if( (hd = BL_DOWNCAST( BL_HOM, bl ) ) ) {
		ud = &hd->ud;
	} else if( (mcd = BL_DOWNCAST( BL_MERC, bl ) ) ) {
		ud = &mcd->ud;
	}
	if( ud == NULL ) return 0;

	ud->to_x = bl->x;
	ud->to_y = bl->y;

	if(type&0x08 && md) {
		if(ud->walktimer == -1)
			return 0;
	} else {
		ud->walkpath.path_len = 0;
		ud->walkpath.path_pos = 0;

		if(ud->walktimer == -1)
			return 0;

		delete_timer(ud->walktimer, unit_walktoxy_timer);
		ud->walktimer = -1;
	}

	if(type&0x01) { // �ʒu�␳���M���K�v
		clif_fixwalkpos(bl);
	}
	if(type&0x02) { // �_���[�W�H�炤
		unsigned int tick = gettick();
		int delay = status_get_dmotion(bl);
		if( (sd && battle_config.pc_damage_delay) || (md && battle_config.monster_damage_delay) ) {
			ud->canmove_tick = tick + delay;
		}
		else if( (sd && !battle_config.pc_damage_delay) || (md && !battle_config.monster_damage_delay) ) {
			ud->canmove_tick = (tick)/2 + (delay)/5;
		}
	}
	if(type&0x04 && md) {
		int dx = ud->to_x - md->bl.x;
		int dy = ud->to_y - md->bl.y;

		if(dx < 0) dx = -1; else if(dx > 0) dx = 1;
		if(dy < 0) dy = -1; else if(dy > 0) dy = 1;

		if(dx || dy) {
			unit_walktoxy( bl, md->bl.x+dx, md->bl.y+dy );
		}
	}

	return 0;
}

/*==========================================
 * �X�L���g�p�iID�w��j
 *------------------------------------------
 */
int unit_skilluse_id(struct block_list *src, int target_id, int skill_num, int skill_lv)
{
	int id = skill_get_skilldb_id(skill_num);

	if( id == 0 )
		return 0;
	if( skill_lv <= 0 || skill_lv > MAX_SKILL_LEVEL )
		return 0;

	return unit_skilluse_id2(
		src, target_id, skill_num, skill_lv,
		skill_castfix(src, skill_num, skill_db[id].cast[skill_lv-1], skill_db[id].fixedcast[skill_lv-1]),
		skill_db[id].castcancel
	);
}

int unit_skilluse_id2(struct block_list *src, int target_id, int skill_num, int skill_lv, int casttime, int castcancel)
{
	struct map_session_data *src_sd  = NULL;
	struct pet_data         *src_pd  = NULL;
	struct mob_data         *src_md  = NULL;
	struct homun_data       *src_hd  = NULL;
	struct merc_data        *src_mcd = NULL;
	struct unit_data        *src_ud  = NULL;
	unsigned int tick;
	int range;
	struct block_list       *target;
	struct map_session_data *target_sd  = NULL;
	struct mob_data         *target_md  = NULL;
	struct homun_data       *target_hd  = NULL;
	struct merc_data        *target_mcd = NULL;
	int forcecast = 0, zone = 0, nomemorize = 0;
	struct status_change *sc;

	nullpo_retr(0, src);

	if( (src_sd = BL_DOWNCAST( BL_PC,  src ) ) ) {
		src_ud = &src_sd->ud;
	} else if( (src_md = BL_DOWNCAST( BL_MOB, src ) ) ) {
		src_ud = &src_md->ud;
	} else if( (src_pd = BL_DOWNCAST( BL_PET, src ) ) ) {
		src_ud = &src_pd->ud;
	} else if( (src_hd = BL_DOWNCAST( BL_HOM, src ) ) ) {
		src_ud = &src_hd->ud;
	} else if( (src_mcd = BL_DOWNCAST( BL_MERC, src ) ) ) {
		src_ud = &src_mcd->ud;
	}
	if( src_ud == NULL ) return 0;

	if( unit_isdead(src) )		 return 0;	// ����ł��Ȃ���
	if( src_sd && src_sd->sc.opt1 > 0 && src_sd->sc.opt1 != 7 ) return 0;	// ���ق�ُ�i�������A�O�����Ȃǂ̔��������j

	// �X�L������
	zone = skill_get_zone(skill_num);
	if(zone) {
		int m = src->m;
		int ban = 0;

		if(map[m].flag.turbo && zone&16)
			ban = 1;
		else if(map[m].flag.normal && zone&1)
			ban = 1;
		else if(map[m].flag.pvp && zone&2)
			ban = 1;
		else if(map[m].flag.gvg && zone&4)
			ban = 1;
		else if(map[m].flag.pk && zone&8)
			ban = 1;
		if(ban) {
			if(src_sd) {
				if(skill_num == AL_TELEPORT || skill_num == AL_WARP)
					clif_skill_teleportmessage(src_sd,0);
				else
					clif_skill_fail(src_sd,skill_num,0,0);
			}
			return 0;
		}
	}

	sc = status_get_sc(src);

	// �^�[�Q�b�g�̎����I��
	switch(skill_num) {
	case TK_STORMKICK:		/* �����R�� */
	case TK_DOWNKICK:		/* ���i�R�� */
	case TK_TURNKICK:		/* ��]�R�� */
	case TK_COUNTER:		/* �J�E���^�[�R�� */
	case MO_COMBOFINISH:		/* �җ��� */
	case CH_TIGERFIST:		/* ���Ռ� */
	case CH_CHAINCRUSH:		/* �A������ */
		target_id = src_ud->attacktarget;
		break;
	case MO_CHAINCOMBO:		/* �A�ŏ� */
		target_id = src_ud->attacktarget;
		if(sc && sc->data[SC_BLADESTOP].timer != -1)
			target_id = sc->data[SC_BLADESTOP].val4;
		break;
	case TK_JUMPKICK:		/* ��яR��i�e�B�I�A�v�`���M�j*/
		if(sc && sc->data[SC_DODGE_DELAY].timer != -1 && src->id == target_id)
			target_id = sc->data[SC_DODGE_DELAY].val2;
		break;
	case MO_EXTREMITYFIST:		/* ���C���e�P�� */
		if(sc && sc->data[SC_COMBO].timer != -1 && (sc->data[SC_COMBO].val1 == MO_COMBOFINISH || sc->data[SC_COMBO].val1 == CH_CHAINCRUSH) )
			target_id = src_ud->attacktarget;
		break;
	case WE_MALE:
	case WE_FEMALE:
		{
			struct map_session_data *p_sd = NULL;
			if(src_sd) {
				p_sd = pc_get_partner(src_sd);
			}
			target_id = (p_sd)? p_sd->bl.id: 0;
		}
		break;
	case GC_WEAPONCRUSH:	/* �E�F�|���N���b�V�� */
		if(sc && sc->data[SC_WEAPONBLOCKING2].timer != -1)
			target_id = sc->data[SC_WEAPONBLOCKING2].val2;
		break;
	}

	if( (target = map_id2bl(target_id)) == NULL ) {
		if(src_sd)
			clif_skill_fail(src_sd,skill_num,0,0);
		return 0;
	}

	if(skill_get_inf2(skill_num)&0x200 && src->id == target_id)
		return 0;

	if(src->m != target->m)         return 0; // �����}�b�v���ǂ���
	if(!src->prev || !target->prev) return 0; // map ��ɑ��݂��邩

	target_sd  = BL_DOWNCAST( BL_PC,   target );
	target_md  = BL_DOWNCAST( BL_MOB,  target );
	target_hd  = BL_DOWNCAST( BL_HOM,  target );
	target_mcd = BL_DOWNCAST( BL_MERC, target );

	// ���O�̃X�L���󋵂̋L�^
	if(src_sd) {
		switch(skill_num) {
		case SA_CASTCANCEL:
			if(src_ud->skillid != skill_num) { // �L���X�g�L�����Z�����̂͊o���Ȃ�
				src_sd->skillid_old = src_ud->skillid;
				src_sd->skilllv_old = src_ud->skilllv;
				break;
			}
		case BD_ENCORE:					/* �A���R�[�� */
			 // �O��g�p�����x�肪�Ȃ��Ƃ���
			if(!src_sd->skillid_dance || pc_checkskill(src_sd,src_sd->skillid_dance) <= 0) {
				clif_skill_fail(src_sd,skill_num,0,0);
				return 0;
			}
			src_sd->skillid_old = skill_num;
			break;
		}
	}

	// �R���f�B�V�����m�F
	{
		struct skill_condition cnd;

		cnd.id     = skill_num;
		cnd.lv     = skill_lv;
		cnd.x      = 0;
		cnd.y      = 0;
		cnd.target = target_id;

		if(!skill_check_condition2(src, &cnd, 0)) return 0;

		skill_num = cnd.id;
		skill_lv  = cnd.lv;
		target_id = cnd.target;
	}

	/* �˒��Ə�Q���`�F�b�N */
	range = skill_get_fixed_range(src,skill_num,skill_lv);
	if(!battle_check_range(src,target,range + 1))
		return 0;

	switch(skill_num) {
		case MO_CHAINCOMBO:
		case MO_COMBOFINISH:
		case CH_TIGERFIST:
		case CH_CHAINCRUSH:
		case TK_STORMKICK:
		case TK_DOWNKICK:
		case TK_TURNKICK:
		case TK_COUNTER:
		case GC_WEAPONCRUSH:
			break;
		case MO_EXTREMITYFIST:
		case TK_JUMPKICK:
			if(src_sd && !src_sd->state.skill_flag)
				break;
			// fall through
		default:
			unit_stopattack(src);
			break;
	}

	src_ud->state.skillcastcancel = castcancel;

	/* ��������ȏ������K�v */
	// ���s�����skill_check_condition() �ɏ�������
	switch(skill_num)
	{
	case ALL_RESURRECTION:	/* ���U���N�V���� */
		if( !target_sd && battle_check_undead(status_get_race(target),status_get_elem_type(target)) ) {	/* �G���A���f�b�h�Ȃ� */
			forcecast = 1;	/* �^�[���A���f�b�g�Ɠ����r������ */
			casttime  = skill_castfix(src, skill_num, skill_get_cast(PR_TURNUNDEAD,skill_lv), skill_get_fixedcast(PR_TURNUNDEAD,skill_lv));
		}
		break;
	case MO_FINGEROFFENSIVE:	/* �w�e */
		if(src_sd)
			casttime += casttime * ((skill_lv > src_sd->spiritball)? src_sd->spiritball: skill_lv);
		break;
	case MO_EXTREMITYFIST:	/* ���C���e�P�� */
		if(sc && sc->data[SC_COMBO].timer != -1 && (sc->data[SC_COMBO].val1 == MO_COMBOFINISH || sc->data[SC_COMBO].val1 == CH_CHAINCRUSH)) {
			casttime = 0;
		}
		forcecast = 1;
		break;
	case SA_MAGICROD:
	case SA_SPELLBREAKER:
		forcecast = 1;
		break;
	case HP_BASILICA:
		if(sc && sc->data[SC_BASILICA].timer != -1)
			casttime = 0;
		break;
	case PF_MEMORIZE:
	case GD_REGENERATION:	/* ���� */
	case GD_RESTORE:	/* ���� */
		nomemorize = 1;
		break;
	case KN_CHARGEATK:	/* �`���[�W�A�^�b�N */
		{
			int dist = unit_distance(src->x,src->y,target->x,target->y);
			if(dist >= 4 && dist <= 6)
				casttime = casttime * 2;
			else if(dist > 6)
				casttime = casttime * 3;
		}
		break;
	case TK_RUN:		/* �삯���i�^�C���M�j*/
		if(sc && sc->data[SC_RUN].timer != -1)
			casttime = 0;
		break;
	case GD_EMERGENCYCALL:	/* �ً}���W */
		if(src_sd && pc_checkskill(src_sd,TK_HIGHJUMP) > 0) {
			casttime <<= 1;
		}
		nomemorize = 1;
		break;
	case ST_CHASEWALK:	/* �`�F�C�X�E�H�[�N */
		if(sc && sc->data[SC_CHASEWALK].timer != -1)
			casttime = 0;
		break;
	}

	// �������C�Y��ԂȂ�L���X�g�^�C����1/2
	if(sc && sc->data[SC_MEMORIZE].timer != -1 && casttime > 0 && !nomemorize) {
		casttime = casttime/2;
		if((--sc->data[SC_MEMORIZE].val2) <= 0)
			status_change_end(src, SC_MEMORIZE, -1);
	}

	if(battle_config.pc_skill_log)
		printf("PC %d skill use target_id=%d skill=%d lv=%d cast=%d\n",src->id,target_id,skill_num,skill_lv,casttime);

	tick = gettick();
	if( casttime > 0 || forcecast ) { /* �r�����K�v */
		clif_skillcasting(src, src->id, target_id, 0, 0, skill_num,casttime);

		/* �r�����������X�^�[ */
		if(src_sd && target_md && status_get_mode(&target_md->bl)&0x10 && target_md->ud.attacktimer == -1 && src_sd->invincible_timer == -1) {
			target_md->target_id = src->id;
			target_md->min_chase = 13;
		}
		/* �����Ȃǂ�MOB�X�L������ */
		if(target_md) {
			int id = target_md->target_id;
			if(battle_config.mob_changetarget_byskill || id == 0)
			{
				if(src->type == BL_PC || src->type == BL_HOM || src->type == BL_MERC)
					target_md->target_id = src->id;
			}
			mobskill_use(target_md,tick,MSC_CASTTARGETED);
			target_md->target_id = id;
		}
	}

	if( casttime <= 0 )	/* �r���̖������̂̓L�����Z������Ȃ� */
		src_ud->state.skillcastcancel = 0;

	src_ud->canact_tick  = tick + casttime + skill_delayfix(src, skill_num, skill_lv);
	src_ud->canmove_tick = tick;
	src_ud->skilltarget  = target_id;
	src_ud->skillx       = 0;
	src_ud->skilly       = 0;
	src_ud->skillid      = skill_num;
	src_ud->skilllv      = skill_lv;

	if( (src_sd && !(battle_config.pc_cloak_check_type&2)) ||
	    (src_md && !(battle_config.monster_cloak_check_type&2)) )
	{
	 	if(sc && sc->data[SC_CLOAKING].timer != -1 && skill_num != AS_CLOAKING)
			status_change_end(src,SC_CLOAKING,-1);
	}

 	if(sc && sc->data[SC_CLOAKINGEXCEED].timer != -1 && skill_num != GC_CLOAKINGEXCEED) {
		status_change_end(src,SC_CLOAKINGEXCEED,-1);
	}

	if(casttime > 0) {
		int skill;
		src_ud->skilltimer = add_timer(tick+casttime, skill_castend_id, src->id, NULL);
		if(src_sd && (skill = pc_checkskill(src_sd,SA_FREECAST)) > 0) {
			src_sd->prev_speed = src_sd->speed;
			src_sd->speed = src_sd->speed*(175 - skill*5)/100;
			clif_updatestatus(src_sd,SP_SPEED);
		} else {
			unit_stop_walking(src,1);
		}
	} else {
		if(skill_num != SA_CASTCANCEL)
			src_ud->skilltimer = -1;
		skill_castend_id(src_ud->skilltimer,tick,src->id,NULL);
	}
	return 1;
}

/*==========================================
 * �X�L���g�p�i�ꏊ�w��j
 *------------------------------------------
 */
int unit_skilluse_pos(struct block_list *src, int skill_x, int skill_y, int skill_num, int skill_lv)
{
	int id = skill_get_skilldb_id(skill_num);

	if( id == 0 )
		return 0;
	if( skill_lv <= 0 || skill_lv > MAX_SKILL_LEVEL )
		return 0;

	return unit_skilluse_pos2(
		src, skill_x, skill_y, skill_num, skill_lv,
		skill_castfix(src, skill_num, skill_db[id].cast[skill_lv-1], skill_db[id].fixedcast[skill_lv-1]),
		skill_db[id].castcancel
	);
}

int unit_skilluse_pos2( struct block_list *src, int skill_x, int skill_y, int skill_num, int skill_lv, int casttime, int castcancel)
{
	struct map_session_data *src_sd  = NULL;
	struct pet_data         *src_pd  = NULL;
	struct mob_data         *src_md  = NULL;
	struct homun_data       *src_hd  = NULL;
	struct merc_data        *src_mcd = NULL;
	struct unit_data        *src_ud  = NULL;
	int zone;
	unsigned int tick = gettick();
	int range;
	struct status_change *sc;

	nullpo_retr(0, src);

	if(!src->prev) return 0; // map ��ɑ��݂��邩

	if( (src_sd = BL_DOWNCAST( BL_PC,  src ) ) ) {
		src_ud = &src_sd->ud;
	} else if( (src_md = BL_DOWNCAST( BL_MOB, src ) ) ) {
		src_ud = &src_md->ud;
	} else if( (src_pd = BL_DOWNCAST( BL_PET, src ) ) ) {
		src_ud = &src_pd->ud;
	} else if( (src_hd = BL_DOWNCAST( BL_HOM, src ) ) ) {
		src_ud = &src_hd->ud;
	} else if( (src_mcd = BL_DOWNCAST( BL_MERC, src ) ) ) {
		src_ud = &src_mcd->ud;
	}
	if( src_ud == NULL ) return 0;

	if(unit_isdead(src)) return 0;

	sc = status_get_sc(src);

	// �X�L������
	zone = skill_get_zone(skill_num);
	if(zone) {
		int m = src->m;
		int ban = 0;
		if(map[m].flag.turbo && zone&16)
			ban = 1;
		else if(map[m].flag.normal && zone&1)
			ban = 1;
		else if(map[m].flag.pvp && zone&2)
			ban = 1;
		else if(map[m].flag.gvg && zone&4)
			ban = 1;
		else if(map[m].flag.pk && zone&8)
			ban = 1;
		if(ban) {
			if(src_sd) {
				if(skill_num == AL_TELEPORT)
					clif_skill_teleportmessage(src_sd,0);
				else
					clif_skill_fail(src_sd,skill_num,0,0);
			}
			return 0;
		}
	}

	// �`�F�C�X�E�H�[�N���Ɛݒu�n���s
	if(src_sd && pc_ischasewalk(src_sd))
	 	return 0;

	// �R���f�B�V�����m�F
	{
		struct skill_condition cnd;

		cnd.id     = skill_num;
		cnd.lv     = skill_lv;
		cnd.x      = skill_x;
		cnd.y      = skill_y;
		cnd.target = 0;

		if(!skill_check_condition2(src, &cnd ,0)) return 0;

		skill_num = cnd.id;
		skill_lv  = cnd.lv;
		skill_x   = cnd.x;
		skill_y   = cnd.y;
	}

	/* �˒��Ə�Q���`�F�b�N */
	{
		struct block_list bl;

		memset(&bl, 0, sizeof(bl));
		bl.type = BL_NUL;
		bl.m = src->m;
		bl.x = skill_x;
		bl.y = skill_y;

		range = skill_get_fixed_range(src,skill_num,skill_lv);
		if(!battle_check_range(src,&bl,range+1))
			return 0;
	}

	if(!map_getcell(src->m,skill_x,skill_y,CELL_CHKPASS))
		return 0;

	unit_stopattack(src);

	src_ud->state.skillcastcancel = castcancel;

	if(battle_config.pc_skill_log)
		printf("PC %d skill use target_pos=(%d,%d) skill=%d lv=%d cast=%d\n",src->id,skill_x,skill_y,skill_num,skill_lv,casttime);

	// �������C�Y��ԂȂ�L���X�g�^�C����1/2
	if(sc && sc->data[SC_MEMORIZE].timer != -1 && casttime > 0) {
		casttime = casttime/2;
		if((--sc->data[SC_MEMORIZE].val2) <= 0)
			status_change_end(src, SC_MEMORIZE, -1);
	}

	if( casttime > 0 ) {
		/* �r�����K�v */
		unit_stop_walking( src, 1 );		// ���s��~
		clif_skillcasting( src, src->id, 0, skill_x,skill_y, skill_num,casttime );
	}

	if( casttime <= 0 )	/* �r���̖������̂̓L�����Z������Ȃ� */
		src_ud->state.skillcastcancel = 0;

	tick = gettick();
	src_ud->canact_tick  = tick + casttime + skill_delayfix(src, skill_num, skill_lv);
	src_ud->canmove_tick = tick;
	src_ud->skillid      = skill_num;
	src_ud->skilllv      = skill_lv;
	src_ud->skillx       = skill_x;
	src_ud->skilly       = skill_y;
	src_ud->skilltarget  = 0;

	if( (src_sd && !(battle_config.pc_cloak_check_type&2)) ||
	    (src_md && !(battle_config.monster_cloak_check_type&2)) )
	{
	 	if(sc && sc->data[SC_CLOAKING].timer != -1)
			status_change_end(src,SC_CLOAKING,-1);
	}

 	if(sc && sc->data[SC_CLOAKINGEXCEED].timer != -1)
		status_change_end(src,SC_CLOAKINGEXCEED,-1);

	if(casttime > 0) {
		int skill;
		src_ud->skilltimer = add_timer(tick+casttime, skill_castend_pos, src->id, NULL);
		if(src_sd && (skill = pc_checkskill(src_sd,SA_FREECAST)) > 0) {
			src_sd->prev_speed = src_sd->speed;
			src_sd->speed = src_sd->speed*(175 - skill*5)/100;
			clif_updatestatus(src_sd,SP_SPEED);
		} else {
			unit_stop_walking(src,1);
		}
	} else {
		src_ud->skilltimer = -1;
		skill_castend_pos(src_ud->skilltimer,tick,src->id,NULL);
	}
	return 1;
}

/*==========================================
 * �U����~
 *------------------------------------------
 */
void unit_stopattack(struct block_list *bl)
{
	struct unit_data *ud = NULL;

	nullpo_retv(bl);

	ud = unit_bl2ud(bl);
	if(!ud || ud->attacktimer == -1) {
		return;
	}
	delete_timer(ud->attacktimer, unit_attack_timer);
	ud->attacktimer = -1;
	if(bl->type == BL_MOB) {
		mob_unlocktarget( (struct mob_data*)bl, gettick());
	}

	return;
}

/*==========================================
 *
 *------------------------------------------
 */
static int unit_unattackable(struct block_list *bl)
{
	if(bl && bl->type == BL_MOB) {
		mob_unlocktarget((struct mob_data*)bl, gettick());
	}
	return 0;
}

/*==========================================
 * �U���v��
 * type��1�Ȃ�p���U��
 *------------------------------------------
 */
int unit_attack(struct block_list *src,int target_id,int type)
{
	struct block_list *target;
	struct unit_data  *src_ud;
	int d;

	nullpo_retr(0, src);
	nullpo_retr(0, src_ud = unit_bl2ud(src));

	target = map_id2bl(target_id);
	if(target == NULL || battle_check_target(src,target,BCT_ENEMY) <= 0) {
		unit_unattackable(src);
		return 1;
	}
	unit_stopattack(src);
	src_ud->attacktarget          = target_id;
	src_ud->state.attack_continue = type;

	d = DIFF_TICK(src_ud->attackabletime,gettick());
	if(d > 0) {
		// �U��delay��
		src_ud->attacktimer = add_timer(src_ud->attackabletime,unit_attack_timer,src->id,NULL);
	} else {
		// �{��timer�֐��Ȃ̂ň��������킹��
		unit_attack_timer(-1,gettick(),src->id,NULL);
	}

	return 0;
}

/*==========================================
 * �w��ʒu�ɓ��B�\���ǂ���
 *------------------------------------------
 */
int unit_can_reach(struct block_list *bl,int x,int y)
{
	nullpo_retr(0, bl);

	if( bl->x == x && bl->y == y )	// �����}�X
		return 1;

	// ��Q������
	return (path_search(NULL,bl->m,bl->x,bl->y,x,y,0) != -1);
}

/*==========================================
 * �ړ��\�ȏ�Ԃ��ǂ���
 *------------------------------------------
 */
int unit_can_move(struct block_list *bl)
{
	struct map_session_data *sd = NULL;
	struct unit_data *ud = NULL;
	struct status_change *sc = NULL;

	nullpo_retr(0, bl);

	ud = unit_bl2ud(bl);
	if( ud == NULL )
		return 0;

	if( DIFF_TICK(ud->canmove_tick, gettick()) > 0 )
		return 0;

	sc = status_get_sc(bl);

	if( sc && sc->opt1 > 0 && sc->opt1 != 6 && sc->opt1 != 7 )
		return 0;

	if( bl->type == BL_PC )
		sd = (struct map_session_data *)bl;
	if( sc && sc->option&2 && (!sd || pc_checkskill(sd,RG_TUNNELDRIVE) <= 0) )
		return 0;

	if( ud->skilltimer != -1 && (!sc || sc->data[SC_SELFDESTRUCTION].timer == -1) && (!sd || pc_checkskill(sd,SA_FREECAST) <= 0) )
		return 0;

	if(sc && sc->count > 0)
	{
		if( sc->data[SC_ANKLE].timer != -1 ||		// �A���N���X�l�A
		    sc->data[SC_AUTOCOUNTER].timer != -1 ||	// �I�[�g�J�E���^�[
		    sc->data[SC_DEATHBOUND].timer != -1 ||	// �f�X�o�E���h
		    sc->data[SC_TRICKDEAD].timer != -1 ||	// ���񂾂ӂ�
		    sc->data[SC_BLADESTOP_WAIT].timer != -1 ||		// ���n���
		    sc->data[SC_BLADESTOP].timer != -1 ||	// ���n���
		    sc->data[SC_SPIDERWEB].timer != -1 ||	// �X�p�C�_�[�E�F�b�u
		    sc->data[SC_TIGERFIST].timer != -1 ||	// ���Ռ�
		    sc->data[SC_STOP].timer != -1 ||		// �z�[���h�E�F�u
		    sc->data[SC_MADNESSCANCEL].timer != -1 ||	// �}�b�h�l�X�L�����Z���[
		    sc->data[SC_CLOSECONFINE].timer != -1 ||	// �N���[�Y�R���t�@�C��
		    (sc->data[SC_GRAVITATION_USER].timer != -1 && battle_config.player_gravitation_type < 2) ||	//�O���r�e�[�V�����t�B�[���h�g�p��
		    (battle_config.hermode_no_walking && sc->data[SC_DANCING].timer != -1 && sc->data[SC_DANCING].val1 == CG_HERMODE) ||
		    (sc->data[SC_FEAR].timer != -1 && sc->data[SC_FEAR].val3 > 0) ||	// ���|��ԁi2�b�ԁj
		    sc->data[SC_WEAPONBLOCKING2].timer != -1 ||	// �E�F�|���u���b�L���O�i�u���b�N���j
		    sc->data[SC_WHITEIMPRISON].timer != -1	// �z���C�g�C���v���Y��
		)
			return 0;

		if(sc->data[SC_LONGINGFREEDOM].timer == -1 && sc->data[SC_DANCING].timer != -1)
		{
			struct skill_unit_group *sg = NULL;

			// ���t�X�L�����t��
			if(sc->data[SC_DANCING].val4)
				return 0;
			// �P�ƍ��t���ɓ����Ȃ��ݒ�
			sg = map_id2sg(sc->data[SC_DANCING].val2);
			if(sg && skill_get_unit_flag(sg->skill_id, sg->skill_lv) & UF_ENSEMBLE) {
				if(!sd || (!battle_config.player_skill_partner_check && !(battle_config.sole_concert_type & 1)))
					return 0;
			}
		}

		if((sc->data[SC_BASILICA].timer != -1 && sc->data[SC_BASILICA].val2 == bl->id) ||
		   (sc->data[SC_GOSPEL].timer != -1 && sc->data[SC_GOSPEL].val2 == bl->id))
			return 0;		// �o�W���J�������̓S�X�y���𒣂��Ă���l�͓����Ȃ�
	}
	return 1;
}

/*==========================================
 * �����I�Ȉړ������ǂ���
 *------------------------------------------
 */
int unit_isrunning(struct block_list *bl)
{
	struct status_change *sc = NULL;

	nullpo_retr(0, bl);

	sc = status_get_sc(bl);
	if(sc) {
		if( sc->data[SC_RUN].timer != -1 ||		// �삯��
		    sc->data[SC_FORCEWALKING].timer != -1 ||	// �����ړ�
		    sc->data[SC_SELFDESTRUCTION].timer != -1 )	// ����2
			return 1;
	}
	return 0;
}

/*==========================================
 * �U�� (timer�֐�)
 *------------------------------------------
 */
static int unit_attack_timer_sub(int tid,unsigned int tick,int id,void *data)
{
	struct block_list *src, *target;
	struct status_change *sc, *tsc;
	int dist,skill,range;
	struct unit_data *src_ud;
	struct map_session_data *src_sd  = NULL, *target_sd  = NULL;
	struct pet_data         *src_pd  = NULL;
	struct mob_data         *src_md  = NULL, *target_md  = NULL;
	struct homun_data       *src_hd  = NULL, *target_hd  = NULL;
	struct merc_data        *src_mcd = NULL, *target_mcd = NULL;

	if((src = map_id2bl(id)) == NULL)
		return 0;
	if((src_ud = unit_bl2ud(src)) == NULL)
		return 0;

	if(src_ud->attacktimer != tid) {
		if(battle_config.error_log)
			printf("unit_attack_timer %d != %d\n",src_ud->attacktimer,tid);
		return 0;
	}
	src_ud->attacktimer = -1;
	target = map_id2bl(src_ud->attacktarget);

	if(src->prev == NULL)
		return 0;
	if(target == NULL || target->prev == NULL)
		return 0;
	if(src->m != target->m || unit_isdead(src) || unit_isdead(target))	// �^�[�Q�b�g�Ɠ���MAP�łȂ�������ł�Ȃ�U�����Ȃ�
		return 0;

	sc  = status_get_sc( src    );
	tsc = status_get_sc( target );

	if( sc ) {
		if(sc->data[SC_AUTOCOUNTER].timer != -1 ||
		   sc->data[SC_DEATHBOUND].timer != -1 ||
		   sc->data[SC_TRICKDEAD].timer != -1 ||
		   sc->data[SC_BLADESTOP].timer != -1 ||
		   sc->data[SC_FULLBUSTER].timer != -1 ||
		   sc->data[SC_KEEPING].timer != -1 ||
		   sc->data[SC_WHITEIMPRISON].timer != -1)
			return 0;
	}
	if( tsc ) {
		if(tsc->data[SC_TRICKDEAD].timer != -1)
			return 0;
	}

	src_sd  = BL_DOWNCAST( BL_PC ,  src );
	src_pd  = BL_DOWNCAST( BL_PET,  src );
	src_md  = BL_DOWNCAST( BL_MOB,  src );
	src_hd  = BL_DOWNCAST( BL_HOM,  src );
	src_mcd = BL_DOWNCAST( BL_MERC, src );

	target_sd  = BL_DOWNCAST( BL_PC ,  target );
	target_md  = BL_DOWNCAST( BL_MOB,  target );
	target_hd  = BL_DOWNCAST( BL_HOM,  target );
	target_mcd = BL_DOWNCAST( BL_MERC, target );

	if( src_sd ) {
		// �ُ�ȂǂōU���ł��Ȃ�
		if( (src_sd->sc.opt1 > 0 && src_sd->sc.opt1 != 7) || src_sd->sc.option&2 || pc_ischasewalk(src_sd) )
			return 0;
		// �����E������ԂłȂ��Ȃ�n�C�h���̓G�ɍU���ł��Ȃ�
		if( tsc && tsc->option&0x46 && src_sd->race != RCT_INSECT && src_sd->race != RCT_DEMON )
			return 0;
	}

	if( src_md ) {
		int mode, race;
		if((src_md->sc.opt1 > 0 && src_md->sc.opt1 != 7) || src_md->sc.option&2)
			return 0;
		if(src_md->sc.data[SC_WINKCHARM].timer != -1)
			return 0;

		mode = status_get_mode(src);
		race = status_get_race(src);

		if( !(mode&0x80) )
			return 0;
		if( !(mode&0x20) ) {
			if( tsc && (tsc->data[SC_FORCEWALKING].timer != -1 || tsc->data[SC_WINKCHARM].timer != -1) )
				return 0;
			if( target_sd ) {
				if( pc_ishiding(target_sd) && race != RCT_INSECT && race != RCT_DEMON )
					return 0;
				if( target_sd->state.gangsterparadise )
					return 0;
			}
		}
	}

	if( target_sd ) {
		if( target_sd->invincible_timer != -1 )
			return 0;
		if( pc_isinvisible(target_sd) )
			return 0;
	}

	if(src_ud->skilltimer != -1 && (!src_sd || pc_checkskill(src_sd,SA_FREECAST) <= 0))
		return 0;

	if(!battle_config.sdelay_attack_enable && (!src_sd || pc_checkskill(src_sd,SA_FREECAST) <= 0)) {
		if(DIFF_TICK(tick , src_ud->canact_tick) < 0) {
			if(src_sd)
				clif_skill_fail(src_sd,1,4,0);
			return 0;
		}
	}

	dist  = unit_distance(src->x,src->y,target->x,target->y);
	range = status_get_range(src);
	if( src_md && status_get_mode(src) & 1 )
		range++;
	if(src_sd && (src_sd->status.weapon != WT_BOW && !(src_sd->status.weapon >= WT_HANDGUN && src_sd->status.weapon <= WT_GRENADE)))
		range++;

	if( dist > range ) {	// �͂��Ȃ��̂ňړ�
		if(!unit_can_reach(src,target->x,target->y))
			return 0;
		if(src_sd)
			clif_movetoattack(src_sd,target);
		return 1;
	}
	if(dist <= range && !battle_check_range(src,target,range)) {
		if(unit_can_reach(src,target->x,target->y))
			unit_walktoxy(src,target->x,target->y);
		src_ud->attackabletime = tick + status_get_adelay(src);
	} else {
		// �����ݒ�
		int dir = map_calc_dir(src, target->x,target->y);
		if(src_sd && battle_config.pc_attack_direction_change)
			pc_setdir(src_sd, dir, dir);
		else if(src_pd && battle_config.monster_attack_direction_change)
			src_pd->dir = dir;
		else if(src_md && battle_config.monster_attack_direction_change)
			src_md->dir = dir;
		else if(src_hd && battle_config.monster_attack_direction_change)
			src_hd->dir = dir;
		else if(src_mcd && battle_config.monster_attack_direction_change)
			src_mcd->dir = dir;
		else if(src_ud->walktimer != -1)
			unit_stop_walking(src,1);

		if( src_md && mobskill_use(src_md,tick,-2) ) {	// �X�L���g�p
			return 1;
		}
		if(!sc || (sc->data[SC_COMBO].timer == -1 && sc->data[SC_TKCOMBO].timer == -1)) {
			map_freeblock_lock();
			unit_stop_walking(src,1);

			src_ud->attacktarget_lv = battle_weapon_attack(src,target,tick,0);

			if(src_md && !(battle_config.monster_cloak_check_type&2) && sc && sc->data[SC_CLOAKING].timer != -1)
				status_change_end(src,SC_CLOAKING,-1);
			if(src_sd && !(battle_config.pc_cloak_check_type&2) && sc && sc->data[SC_CLOAKING].timer != -1)
				status_change_end(src,SC_CLOAKING,-1);
			if(sc && sc->data[SC_CLOAKINGEXCEED].timer != -1)
				status_change_end(src,SC_CLOAKINGEXCEED,-1);
			if(src_sd && src_sd->status.pet_id > 0 && src_sd->pd && src_sd->petDB)
				pet_target_check(src_sd,target,0);
			map_freeblock_unlock();
			if(src_ud->skilltimer != -1 && src_sd && (skill = pc_checkskill(src_sd,SA_FREECAST)) > 0)	// �t���[�L���X�g
				src_ud->attackabletime = tick + (status_get_adelay(src)*(150 - skill*5)/100);
			else
				src_ud->attackabletime = tick + status_get_adelay(src);
		}
		else if(src_ud->attackabletime <= tick) {
			if(src_ud->skilltimer != -1 && src_sd && (skill = pc_checkskill(src_sd,SA_FREECAST)) > 0)	// �t���[�L���X�g
				src_ud->attackabletime = tick + (status_get_adelay(src)*(150 - skill*5)/100);
			else
				src_ud->attackabletime = tick + status_get_adelay(src);
		}
		if(src_ud->attackabletime <= tick)
			src_ud->attackabletime = tick + (battle_config.max_aspd<<1);
	}

	if(src_ud->state.attack_continue) {
		src_ud->attacktimer = add_timer(src_ud->attackabletime,unit_attack_timer,src->id,NULL);
	}
	return 1;
}

static int unit_attack_timer(int tid,unsigned int tick,int id,void *data)
{
	if(unit_attack_timer_sub(tid, tick, id, data) == 0) {
		unit_unattackable( map_id2bl(id) );
	}
	return 0;
}

/*==========================================
 * �X�L���r���L�����Z��
 *   type= 0: �r���̋������~
 *   type=+1: �L���X�g�L�����Z���p
 *   type=+2: �r���̖W�Q
 *------------------------------------------
 */
int unit_skillcastcancel(struct block_list *bl,int type)
{
	int skillid;
	int ret=0;
	struct map_session_data *sd = NULL;
	struct mob_data         *md = NULL;
	struct unit_data        *ud = NULL;
	struct status_change    *sc = NULL;
	unsigned long tick = gettick();

	nullpo_retr(0, bl);

	if( (ud = unit_bl2ud(bl)) == NULL || ud->skilltimer == -1 )
		return 0;

	sd = BL_DOWNCAST( BL_PC,  bl );
	md = BL_DOWNCAST( BL_MOB, bl );

	if(type&2) {	// �L�����Z���\�ȏ�Ԃ�����
		if(!ud->state.skillcastcancel)
			return 0;
		if(sd) {
			if(sd->special_state.no_castcancel && !map[bl->m].flag.gvg)
				return 0;
			if(sd->special_state.no_castcancel2)
				return 0;
		}
	}

	ud->canact_tick  = tick;
	ud->canmove_tick = tick;

	if(sd && pc_checkskill(sd,SA_FREECAST) > 0) {
		sd->speed = sd->prev_speed;
		clif_updatestatus(sd,SP_SPEED);
	}

	skillid = (type&1 && sd)? sd->skillid_old: ud->skillid;

	if(skill_get_inf(skillid) & 0x02)
		ret = delete_timer(ud->skilltimer, skill_castend_pos);
	else
		ret = delete_timer(ud->skilltimer, skill_castend_id);
	if(ret < 0)
		printf("delete timer error : skillid : %d\n", skillid);

	if(md) {
		md->skillidx = -1;
	}

	ud->skilltimer = -1;
	clif_skillcastcancel(bl);

	sc = status_get_sc(bl);
	if(sc && sc->data[SC_SELFDESTRUCTION].timer != -1)
		status_change_end(bl,SC_SELFDESTRUCTION,-1);

	return 1;
}

/*==========================================
 * unit_data �̏���������
 *------------------------------------------
 */
int unit_dataset(struct block_list *bl)
{
	struct unit_data *ud;
	unsigned int tick = gettick();

	nullpo_retr(0, ud = unit_bl2ud(bl));

	memset(ud, 0, sizeof(struct unit_data));
	ud->bl             = bl;
	ud->walktimer      = -1;
	ud->skilltimer     = -1;
	ud->attacktimer    = -1;
	ud->attackabletime = tick;
	ud->canact_tick    = tick;
	ud->canmove_tick   = tick;

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int unit_heal(struct block_list *bl,int hp,int sp)
{
	nullpo_retr(0, bl);

	if(bl->type == BL_PC)
		pc_heal((struct map_session_data*)bl,hp,sp);
	else if(bl->type == BL_MOB)
		mob_heal((struct mob_data*)bl,hp);
	else if(bl->type == BL_HOM)
		homun_heal((struct homun_data*)bl,hp,sp);
	else if(bl->type == BL_MERC)
		merc_heal((struct merc_data*)bl,hp,sp);

	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int unit_fixdamage(struct block_list *src,struct block_list *target,unsigned int tick,int sdelay,int ddelay,int damage,int div_,int type,int damage2)
{
	nullpo_retr(0, target);

	if(damage + damage2 <= 0)
		return 0;
	// �^�Q
	if(target->type == BL_MOB) {
		mob_attacktarget((struct mob_data*)target,src,0);
	}
	clif_damage(target,target,tick,sdelay,ddelay,damage,div_,type,damage2);
	battle_damage(src,target,damage + damage2,0,0,0);
	return 0;
}

/*==========================================
 * ���������b�N���Ă��郆�j�b�g�̐��𐔂���(foreachclient)
 *------------------------------------------
 */
static int unit_counttargeted_sub(struct block_list *bl, va_list ap)
{
	int id, target_lv;

	nullpo_retr(0, bl);

	id        = va_arg(ap,int);
	target_lv = va_arg(ap,int);

	if(bl->id == id) {
		// ����
		return 0;
	}

	if(bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if( sd && sd->ud.attacktarget == id && sd->ud.attacktimer != -1 && sd->ud.attacktarget_lv >= target_lv ) {
			return 1;
		}
	} else if(bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)bl;
		if( md && md->target_id == id && md->ud.attacktimer != -1 && md->ud.attacktarget_lv >= target_lv ) {
			return 1;
		}
	} else if(bl->type == BL_PET) {
		struct pet_data *pd = (struct pet_data *)bl;
		if( pd && pd->target_id == id && pd->ud.attacktimer != -1 && pd->ud.attacktarget_lv >= target_lv ) {
			return 1;
		}
	} else if(bl->type == BL_HOM) {
		struct homun_data *hd = (struct homun_data *)bl;
		if( hd && hd->target_id == id && hd->ud.attacktimer != -1 && hd->ud.attacktarget_lv >= target_lv ) {
			return 1;
		}
	} else if(bl->type == BL_MERC) {
		struct merc_data *mcd = (struct merc_data *)bl;
		if( mcd && mcd->target_id == id && mcd->ud.attacktimer != -1 && mcd->ud.attacktarget_lv >= target_lv ) {
			return 1;
		}
	}
	return 0;
}

/*==========================================
 * ���������b�N���Ă���Ώۂ̐���Ԃ�
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int unit_counttargeted(struct block_list *bl,int target_lv)
{
	nullpo_retr(0, bl);

	return map_foreachinarea(unit_counttargeted_sub, bl->m,
		bl->x-AREA_SIZE,bl->y-AREA_SIZE,
		bl->x+AREA_SIZE,bl->y+AREA_SIZE,(BL_CHAR|BL_PET),bl->id,target_lv
	);
}

/*==========================================
 * ���S���Ă��邩�ǂ���
 *------------------------------------------
 */
int unit_isdead(struct block_list *bl)
{
	nullpo_retr(1, bl);

	if(bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data*)bl;
		return (sd->state.dead_sit == 1);
	} else if(bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)bl;
		return md->hp <= 0;
	} else if(bl->type == BL_PET) {
		return 0;
	} else if(bl->type == BL_HOM) {
		struct homun_data *hd = (struct homun_data *)bl;
		return hd->status.hp <= 0;
	} else if(bl->type == BL_MERC) {
		struct merc_data *mcd = (struct merc_data *)bl;
		return mcd->status.hp <= 0;
	}
	return 0;
}

/*==========================================
 * id���U�����Ă���PC�̍U�����~
 * clif_foreachclient��callback�֐�
 *------------------------------------------
 */
int unit_mobstopattacked(struct map_session_data *sd,va_list ap)
{
	int id;

	nullpo_retr(0, sd);
	nullpo_retr(0, ap);

	id = va_arg(ap,int);

	if(sd->ud.attacktarget == id)
		unit_stopattack(&sd->bl);
	return 0;
}

/*==========================================
 * �����ڂ̃T�C�Y��ύX����
 *------------------------------------------
 */
int unit_changeviewsize(struct block_list *bl,int size)
{
	nullpo_retr(0, bl);

	size = (size < 0)? -1: (size > 0)? 1: 0;

	if(bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data*)bl;
		sd->view_size = size;
	} else if(bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data *)bl;
		md->view_size = size;
	} else if(bl->type == BL_PET) {
		struct pet_data *pd = (struct pet_data *)bl;
		pd->view_size = size;
	} else if(bl->type == BL_HOM) {
		struct homun_data *hd = (struct homun_data *)bl;
		hd->view_size = size;
	} else if(bl->type == BL_MERC) {
		struct merc_data *mcd = (struct merc_data *)bl;
		mcd->view_size = size;
	} else if(bl->type == BL_NPC) {
		struct npc_data *nd = (struct npc_data *)bl;
		nd->view_size = size;
	} else {
		return 0;
	}
	if(size != 0)
		clif_misceffect2(bl,421+size);
	return 0;
}

/*==========================================
 * �X�L���r�������ǂ�����Ԃ�
 *------------------------------------------
 */
int unit_iscasting(struct block_list *bl)
{
	struct unit_data *ud = unit_bl2ud(bl);

	if( ud == NULL )
		return 0;

	return (ud->skilltimer != -1);
}

/*==========================================
 * ���s�����ǂ�����Ԃ�
 *------------------------------------------
 */
int unit_iswalking(struct block_list *bl)
{
	struct unit_data *ud = unit_bl2ud(bl);

	if( ud == NULL )
		return 0;

	return (ud->walktimer != -1);
}

/*==========================================
 * �}�b�v���痣�E����
 *------------------------------------------
 */
int unit_remove_map(struct block_list *bl, int clrtype, int flag)
{
	struct unit_data *ud;

	nullpo_retr(0, bl);
	nullpo_retr(0, ud = unit_bl2ud(bl));

	if(bl->prev == NULL)
		return 1;

	map_freeblock_lock();

	unit_stop_walking(bl,1);			// ���s���f
	unit_stopattack(bl);				// �U�����f
	unit_skillcastcancel(bl,0);			// �r�����f
	skill_stop_dancing(bl,1);			// �_���X���f
	if(!flag)
		skill_clear_unitgroup(bl);		// �X�L�����j�b�g�O���[�v�̍폜
	if(!unit_isdead(bl))
		skill_unit_move(bl,gettick(),0);	// �X�L�����j�b�g���痣�E

	// tickset �폜
	linkdb_final( &ud->skilltickset );
	status_clearpretimer( bl );
	skill_cleartimerskill( bl );			// �^�C�}�[�X�L���N���A

	// MAP�𗣂��Ƃ��̏�Ԉُ����
	status_change_removemap_end(bl);

	if(bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data*)bl;
		// �`���b�g����o��
		if(sd->chatID)
			chat_leavechat(sd,0);

		// ����𒆒f����
		if(sd->trade_partner)
			trade_tradecancel(sd);

		// �I�V�����
		if(sd->vender_id)
			vending_closevending(sd);

		// �q�ɂ��J���Ă�Ȃ���ĕۑ�����
		if(sd->state.storage_flag == 2)
			storage_guild_storageclose(sd);
		else if(sd->state.storage_flag == 1)
			storage_storageclose(sd);

		// �F�B���X�g���U�����ۂ���
		if(sd->friend_invite > 0)
			friend_add_reply(sd,sd->friend_invite,sd->friend_invite_char,0);

		// �p�[�e�B���U�����ۂ���
		if(sd->party_invite > 0)
			party_reply_invite(sd,sd->party_invite_account,0);

		// �M���h���U�����ۂ���
		if(sd->guild_invite > 0)
			guild_reply_invite(sd,sd->guild_invite,0);

		// �M���h�������U�����ۂ���
		if(sd->guild_alliance > 0)
			guild_reply_reqalliance(sd,sd->guild_alliance_account,0);

		// �{�q�v�������ۂ���
		if(sd->adopt_invite > 0)
			pc_adopt_reply(sd,0,0,0);

		// ���[���Y�t����j��
		mail_removeitem(sd,0);

		pc_delinvincibletimer(sd);		// ���G�^�C�}�[�폜

		// PVP �^�C�}�[�폜
		if(sd->pvp_timer != -1) {
			delete_timer(sd->pvp_timer,pc_calc_pvprank_timer);
			sd->pvp_timer = -1;
		}

		skill_sit(sd,0);			// �M�����O�X�^�[�p���_�C�X����уe�R���x���폜

		clif_clearchar_area(&sd->bl,clrtype&0xffff);
		mob_ai_hard_spawn( &sd->bl, 0 );
		map_delblock(&sd->bl);
	} else if(bl->type == BL_MOB) {
		struct mob_data *md = (struct mob_data*)bl;

		linkdb_final( &md->dmglog );
		//mobskill_deltimer(md);
		md->state.skillstate = MSS_DEAD;
		// ���񂾂̂ł���mob�ւ̍U���ґS���̍U�����~�߂�
		clif_foreachclient(unit_mobstopattacked,md->bl.id);
		status_change_clear(&md->bl,2);	// �X�e�[�^�X�ُ����������
		if(md->deletetimer != -1) {
			delete_timer(md->deletetimer,mob_timer_delete);
			md->deletetimer = -1;
		}
		md->hp               = 0;
		md->target_id        = 0;
		md->attacked_id      = 0;
		md->attacked_players = 0;

		clif_clearchar_area(&md->bl,clrtype);
		if(mob_is_pcview(md->class_)) {
			if(battle_config.pcview_mob_clear_type == 2)
				clif_clearchar(&md->bl,0);
			else
				clif_clearchar_delay(gettick()+3000,&md->bl);
		}
		mob_ai_hard_spawn( &md->bl, 0 );
		if(!battle_config.monster_damage_delay || battle_config.monster_damage_delay_rate == 0)
			mob_deleteslave(md);

		if( md->ai_pc_count != 0 || md->ai_prev != NULL || md->ai_next != NULL ) {
			printf("unit_remove_map: ai error\n");
			mob_ai_hard_del( md );
			md->ai_pc_count = 0;
		}
		map_delblock(&md->bl);
#ifdef DYNAMIC_SC_DATA
		if(md->sc.data != NULL)
			status_free_sc_data(&md->sc);
#endif
		if(md->lootitem) {
			int i;
			for(i=0; i<md->lootitem_count; i++) {
				if(md->lootitem[i].card[0] == (short)0xff00)
					intif_delete_petdata(*((long *)(&md->lootitem[i].card[1])));
			}
			md->lootitem_count = 0;
		}

		// �������Ȃ�MOB�̏���
		if(md->spawndelay1 == -1 && md->spawndelay2 == -1 && md->n == 0) {
			map_deliddb(&md->bl);
			if(md->lootitem) {
				aFree(md->lootitem);
				md->lootitem = NULL;
			}
			map_freeblock(md);	// free�̂����
		} else {
			unsigned int spawntime1,spawntime2,spawntime3;
			unsigned int tick = gettick();
			spawntime1 = md->last_spawntime + md->spawndelay1;
			spawntime2 = tick + md->spawndelay2;
			spawntime3 = tick + 1000;
			if(DIFF_TICK(spawntime1,spawntime2) > 0) {
				md->last_spawntime = spawntime1;
			} else {
				md->last_spawntime = spawntime2;
			}
			if(DIFF_TICK(spawntime3,md->last_spawntime) > 0) {
				md->last_spawntime = spawntime3;
			}
			add_timer(md->last_spawntime,mob_delayspawn,bl->id,NULL);
		}
	} else if(bl->type == BL_PET) {
		struct pet_data *pd = (struct pet_data*)bl;

		clif_clearchar_area(&pd->bl,0);
		map_delblock(&pd->bl);
	} else if(bl->type == BL_HOM) {
		struct homun_data *hd = (struct homun_data*)bl;

		if(battle_config.homun_skilldelay_reset) {
			unsigned int tick = gettick();
			int i;
			for(i = 0; i < MAX_HOMSKILL; i++) {
				hd->skillstatictimer[i] = tick;
			}
		}
		clif_clearchar_area(&hd->bl,clrtype);
		mob_ai_hard_spawn( &hd->bl, 0 );
		map_delblock(&hd->bl);
	} else if(bl->type == BL_MERC) {
		struct merc_data *mcd = (struct merc_data*)bl;

		clif_clearchar_area(&mcd->bl,clrtype);
		mob_ai_hard_spawn( &mcd->bl, 0 );
		map_delblock(&mcd->bl);
	}
	map_freeblock_unlock();
	return 0;
}

/*==========================================
 * �}�b�v���痣�E��A�̈���������
 *------------------------------------------
 */
int unit_free(struct block_list *bl, int clrtype)
{
	struct unit_data *ud = unit_bl2ud( bl );

	nullpo_retr(0, ud);

	map_freeblock_lock();
	if( bl->prev )
		unit_remove_map(bl, clrtype, 0);

	if( bl->type == BL_PC ) {
		struct map_session_data *sd = (struct map_session_data*)bl;

		if(unit_isdead(&sd->bl))
			pc_setrestartvalue(sd,2);

		if(sd->sc.data[SC_BERSERK].timer != -1) // �o�[�T�[�N���̏I����HP��100��
			sd->status.hp = 100;

		// OnPCLogout�C�x���g
		if(battle_config.pc_logout_script)
			npc_event_doall_id("OnPCLogout",sd->bl.id,sd->bl.m);

		friend_send_online( sd, 1 );			// �F�B���X�g�̃��O�A�E�g���b�Z�[�W���M
		party_send_logout(sd);					// �p�[�e�B�̃��O�A�E�g���b�Z�[�W���M
		guild_send_memberinfoshort(sd,0);		// �M���h�̃��O�A�E�g���b�Z�[�W���M
		intif_save_scdata(sd);				// �X�e�[�^�X�ُ�f�[�^�̕ۑ�
		status_change_clear(&sd->bl,1);			// �X�e�[�^�X�ُ����������
		pc_cleareventtimer(sd);					// �C�x���g�^�C�}��j������
		pc_delspiritball(sd,sd->spiritball,1);	// �C���폜
		pc_delcoin(sd,sd->coin,1);				// �R�C���폜
		//storage_storage_save(sd);
		storage_delete(sd->status.account_id);
		pc_clearitemlimit(sd);
		pc_makesavestatus(sd);
	} else if( bl->type == BL_PET ) {
		struct pet_data *pd = (struct pet_data*)bl;
		struct map_session_data *sd = pd->msd;
		if(sd && sd->status.pet_id > 0 && sd->pd) {
			if(sd->pet.intimate <= 0) {
				intif_delete_petdata(sd->status.pet_id);
				sd->status.pet_id = 0;
				sd->pd = NULL;
				sd->petDB = NULL;
				if(battle_config.pet_status_support)
					status_calc_pc(sd,2);
			} else {
				intif_save_petdata(sd->status.account_id,&sd->pet);
				sd->pd = NULL;
			}
		}
		if(pd->a_skill) {
			aFree(pd->a_skill);
			pd->a_skill = NULL;
		}
		if(pd->s_skill) {
			if(pd->s_skill->timer != -1)
				delete_timer(pd->s_skill->timer, pet_skill_support_timer);
			aFree(pd->s_skill);
			pd->s_skill = NULL;
		}
		pet_hungry_timer_delete(pd);
		map_deliddb(&pd->bl);
		pet_lootitem_free(pd);
		pd->lootitem = NULL;
		map_freeblock(pd);
	} else if( bl->type == BL_HOM ) {
		struct homun_data *hd = (struct homun_data*)bl;
		struct map_session_data *sd = hd->msd;

		status_change_clear(&hd->bl,1);			// �X�e�[�^�X�ُ����������
		if(sd && sd->hd) {
			homun_hungry_timer_delete(sd->hd);
			//sd->hd->status.incubate = 0;
			homun_save_data(sd);
			if(sd->hd->natural_heal_hp != -1 || sd->hd->natural_heal_sp != -1)
				homun_natural_heal_timer_delete(sd->hd);
			sd->hd = NULL;
		}
		map_deliddb(&hd->bl);
#ifdef DYNAMIC_SC_DATA
		status_free_sc_data(&hd->sc);
#endif
		map_freeblock(hd);
	} else if( bl->type == BL_MERC ) {
		struct merc_data *mcd = (struct merc_data*)bl;
		struct map_session_data *sd = mcd->msd;

		if(mcd->sc.data[SC_BERSERK].timer != -1) // �o�[�T�[�N���̏I����HP��100��
			mcd->status.hp = 100;

		status_change_clear(&mcd->bl,1);			// �X�e�[�^�X�ُ����������
		if(sd && sd->mcd) {
			merc_employ_timer_delete(sd->mcd);
			merc_save_data(sd);
			if(sd->mcd->natural_heal_hp != -1 || sd->mcd->natural_heal_sp != -1)
				merc_natural_heal_timer_delete(sd->mcd);
			sd->mcd = NULL;
		}
		map_deliddb(&mcd->bl);
#ifdef DYNAMIC_SC_DATA
		status_free_sc_data(&mcd->sc);
#endif
		map_freeblock(mcd);
	}
	map_freeblock_unlock();

	return 0;
}

/*==========================================
 * ������
 *------------------------------------------
 */
int do_init_unit(void)
{
	add_timer_func_list(unit_attack_timer);
	add_timer_func_list(unit_walktoxy_timer);
	return 0;
}

/*==========================================
 * �I��
 *------------------------------------------
 */
int do_final_unit(void)
{
	// nothing to do
	return 0;
}
