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
#include <string.h>
#include <time.h>

#include "db.h"
#include "timer.h"
#include "socket.h"
#include "nullpo.h"
#include "malloc.h"
#include "utils.h"
#include "mmo.h"

#include "atcommand.h"
#include "battle.h"
#include "clif.h"
#include "chrif.h"
#include "intif.h"
#include "map.h"
#include "merc.h"
#include "mob.h"
#include "npc.h"
#include "pc.h"
#include "skill.h"
#include "status.h"
#include "storage.h"
#include "unit.h"

struct merc_db merc_db[MAX_MERC_DB];

static struct merc_skill_tree_entry {
	int id;
	int max;
} merc_skill_tree[MAX_MERC_DB][MAX_MERCSKILL_TREE];

/*==========================================
 * �X�L���c���[���̌���
 *------------------------------------------
 */
static struct merc_skill_tree_entry* merc_search_skilltree(int class_, int skillid)
{
	int min = -1;
	int max = MAX_MERCSKILL_TREE;
	struct merc_skill_tree_entry *st;

	st = merc_skill_tree[class_ - MERC_ID];

	// binary search
	while(max - min > 1) {
		int mid = (min + max) / 2;
		if(st[mid].id && st[mid].id == skillid)
			return &st[mid];

		// 0�̂Ƃ��͑�Ƃ݂Ȃ�
		if(st[mid].id == 0 || st[mid].id > skillid)
			max = mid;
		else
			min = mid;
	}
	return NULL;
}

/*==========================================
 * �X�L����MaxLv��Ԃ�
 *------------------------------------------
 */
int merc_get_skilltree_max(int class_,int skillid)
{
	struct merc_skill_tree_entry *st;

	st = merc_search_skilltree(class_, skillid);
	if(st == NULL)
		return 0;

	return st->max;
}

/*==========================================
 *
 *------------------------------------------
 */
static int merc_calc_pos(struct merc_data *mcd,int tx,int ty,int dir)
{
	int x,y,dx,dy;
	int i,j=0,k;

	nullpo_retr(0, mcd);

	mcd->ud.to_x = tx;
	mcd->ud.to_y = ty;

	if(dir >= 0 && dir < 8) {
		dx = -dirx[dir]*2;
		dy = -diry[dir]*2;
		x = tx + dx;
		y = ty + dy;
		if(!(j=unit_can_reach(&mcd->bl,x,y))) {
			if(dx > 0) x--;
			else if(dx < 0) x++;
			if(dy > 0) y--;
			else if(dy < 0) y++;
			if(!(j=unit_can_reach(&mcd->bl,x,y))) {
				for(i=0;i<12;i++) {
					k = atn_rand()%8;
					dx = -dirx[k]*2;
					dy = -diry[k]*2;
					x = tx + dx;
					y = ty + dy;
					if((j=unit_can_reach(&mcd->bl,x,y)))
						break;
					else {
						if(dx > 0) x--;
						else if(dx < 0) x++;
						if(dy > 0) y--;
						else if(dy < 0) y++;
						if((j=unit_can_reach(&mcd->bl,x,y)))
							break;
					}
				}
				if(!j) {
					x = tx;
					y = ty;
					if(!unit_can_reach(&mcd->bl,x,y))
						return 1;
				}
			}
		}
	}
	else
		return 1;

	mcd->ud.to_x = x;
	mcd->ud.to_y = y;
	return 0;
}

/*==========================================
 * �ٗp�����^�C�}�[
 *------------------------------------------
 */
static int merc_employ_timer(int tid,unsigned int tick,int id,void *data)
{
	struct map_session_data *sd = map_id2sd(id);

	if(sd == NULL || sd->mcd == NULL)
		return 1;

	if(sd->mcd->limit_timer != tid) {
		if(battle_config.error_log)
			printf("merc_employ_timer %d != %d\n",sd->mcd->limit_timer,tid);
		return 0;
	}
	sd->mcd->limit_timer = -1;

	// �����l+1
	merc_set_fame(sd, sd->mcd->class_type, 1);

	//clif_disp_onlyself(sd->fd, msg_txt(188));	// �b���ٗp���Ԃ��������܂����B
	clif_msgstringtable(sd, 0x4f2);
	merc_delete_data(sd);

	return 0;
}

/*==========================================
 * �ٗp�����^�C�}�[�폜
 *------------------------------------------
 */
int merc_employ_timer_delete(struct merc_data *mcd)
{
	nullpo_retr(0, mcd);

	if(mcd->limit_timer != -1) {
		delete_timer(mcd->limit_timer,merc_employ_timer);
		mcd->limit_timer = -1;
	}

	return 0;
}

/*==========================================
 * �e�X�e�v�Z
 *------------------------------------------
 */
int merc_calc_status(struct merc_data *mcd)
{
	int class_;
	int aspd_rate=100,speed_rate=100,atk_rate=100,matk_rate=100,hp_rate=100,sp_rate=100;
	int flee_rate=100,def_rate=100,mdef_rate=100,critical_rate=100,hit_rate=100;

	nullpo_retr(1, mcd);

	class_ = mcd->status.class_ - MERC_ID;

	mcd->atk1     = merc_db[class_].atk1;
	mcd->atk2     = merc_db[class_].atk2;
	mcd->matk1    = 0;
	mcd->matk2    = 0;
	mcd->hit      = 0;
	mcd->flee     = 0;
	mcd->def      = merc_db[class_].def;
	mcd->mdef     = merc_db[class_].mdef;
	mcd->critical = 0;
	mcd->max_hp   = mcd->status.max_hp;
	mcd->max_sp   = mcd->status.max_sp;
	mcd->str      = mcd->status.str;
	mcd->agi      = mcd->status.agi;
	mcd->vit      = mcd->status.vit;
	mcd->dex      = mcd->status.dex;
	mcd->int_     = mcd->status.int_;
	mcd->luk      = mcd->status.luk;
	mcd->speed    = DEFAULT_WALK_SPEED;
	mcd->amotion  = merc_db[class_].amotion;
	mcd->nhealhp  = 0;
	mcd->nhealsp  = 0;
	mcd->hprecov_rate = 100;
	mcd->sprecov_rate = 100;

	// �X�e�[�^�X�ω��ɂ���{�p�����[�^�␳
	if(mcd->sc.count > 0)
	{
		int sc_speed_rate = 100;

		if(mcd->sc.data[SC_INCREASEAGI].timer != -1 && sc_speed_rate > 75)	// ���x�����ɂ��ړ����x����
			sc_speed_rate = 75;
		if(mcd->sc.data[SC_BERSERK].timer != -1 && sc_speed_rate > 75)	// �o�[�T�[�N�ɂ��ړ����x����
			sc_speed_rate = 75;

		mcd->speed = mcd->speed * sc_speed_rate / 100;

		if(mcd->sc.data[SC_DECREASEAGI].timer != -1) {		// ���x����(agi��battle.c��)
			if(mcd->sc.data[SC_DEFENDER].timer == -1) {	// �f�B�t�F���_�[���͑��x�ቺ���Ȃ�
				mcd->speed = mcd->speed *((mcd->sc.data[SC_DECREASEAGI].val1 > 5)? 150: 133)/100;
			}
		}

		// �S�X�y��ALL+20
		if(mcd->sc.data[SC_INCALLSTATUS].timer != -1) {
			mcd->str  += mcd->sc.data[SC_INCALLSTATUS].val1;
			mcd->agi  += mcd->sc.data[SC_INCALLSTATUS].val1;
			mcd->vit  += mcd->sc.data[SC_INCALLSTATUS].val1;
			mcd->int_ += mcd->sc.data[SC_INCALLSTATUS].val1;
			mcd->dex  += mcd->sc.data[SC_INCALLSTATUS].val1;
			mcd->luk  += mcd->sc.data[SC_INCALLSTATUS].val1;
		}

		if(mcd->sc.data[SC_INCREASEAGI].timer != -1)	// ���x����
			mcd->agi += 2+mcd->sc.data[SC_INCREASEAGI].val1;

		if(mcd->sc.data[SC_DECREASEAGI].timer != -1)	// ���x����(agi��battle.c��)
			mcd->agi -= 2+mcd->sc.data[SC_DECREASEAGI].val1;

		if(mcd->sc.data[SC_BLESSING].timer != -1) {	// �u���b�V���O
			mcd->str  += mcd->sc.data[SC_BLESSING].val1;
			mcd->dex  += mcd->sc.data[SC_BLESSING].val1;
			mcd->int_ += mcd->sc.data[SC_BLESSING].val1;
		}
		if(mcd->sc.data[SC_SUITON].timer != -1) {	// ����
			if(mcd->sc.data[SC_SUITON].val3)
				mcd->agi += mcd->sc.data[SC_SUITON].val3;
			if(mcd->sc.data[SC_SUITON].val4)
				mcd->speed = mcd->speed*2;
		}

		if(mcd->sc.data[SC_GLORIA].timer != -1)	// �O�����A
			mcd->luk += 30;

		if(mcd->sc.data[SC_QUAGMIRE].timer != -1) {	// �N�@�O�}�C�A
			short subagi = 0;
			short subdex = 0;
			subagi = (mcd->status.agi/2 < mcd->sc.data[SC_QUAGMIRE].val1*10) ? mcd->status.agi/2 : mcd->sc.data[SC_QUAGMIRE].val1*10;
			subdex = (mcd->status.dex/2 < mcd->sc.data[SC_QUAGMIRE].val1*10) ? mcd->status.dex/2 : mcd->sc.data[SC_QUAGMIRE].val1*10;
			if(map[mcd->bl.m].flag.pvp || map[mcd->bl.m].flag.gvg) {
				subagi /= 2;
				subdex /= 2;
			}
			mcd->speed = mcd->speed*4/3;
			mcd->agi -= subagi;
			mcd->dex -= subdex;
		}

		if(mcd->sc.data[SC_BERSERK].timer != -1) {	// �o�[�T�[�N
			def_rate  = 0;
			mdef_rate = 0;
			aspd_rate -= 30;
			mcd->flee -= mcd->flee * 50 / 100;
			mcd->max_hp = mcd->max_hp * 3;
		}

		if(mcd->sc.data[SC_DEFENDER].timer != -1) {	// �f�B�t�F���_�[
			mcd->amotion += (25 - mcd->sc.data[SC_DEFENDER].val1*5);
			mcd->speed = (mcd->speed * (155 - mcd->sc.data[SC_DEFENDER].val1*5)) / 100;
		}

		// �E�F�|���N�C�b�P��
		if(mcd->sc.data[SC_WEAPONQUICKEN].timer != -1 &&
		   mcd->sc.data[SC_QUAGMIRE].timer == -1 &&
		   mcd->sc.data[SC_DONTFORGETME].timer == -1 &&
		   mcd->sc.data[SC_DECREASEAGI].timer == -1)
			aspd_rate -= 30;
	}



	mcd->matk1    += mcd->int_+(mcd->int_/ 5) * (mcd->int_/ 5);
	mcd->matk2    += mcd->int_+(mcd->int_/ 7) * (mcd->int_/ 7);
	mcd->hit      += mcd->dex + mcd->status.base_level;
	mcd->flee     += mcd->agi + mcd->status.base_level;
	mcd->critical += mcd->luk * 3 + 10;

	// �␳
	if(atk_rate != 100) {
		mcd->atk1 = mcd->atk1*atk_rate/100;
		mcd->atk2 = mcd->atk2*atk_rate/100;
	}
	if(matk_rate != 100) {
		mcd->matk1 = mcd->matk1*matk_rate/100;
		mcd->matk2 = mcd->matk2*matk_rate/100;
	}
	if(hit_rate != 100)
		mcd->hit = mcd->hit*hit_rate/100;
	if(flee_rate != 100)
		mcd->flee = mcd->flee*flee_rate/100;
	if(def_rate != 100)
		mcd->def = mcd->def*def_rate/100;
	if(mdef_rate != 100)
		mcd->mdef = mcd->mdef*mdef_rate/100;
	if(critical_rate != 100)
		mcd->critical = mcd->critical*critical_rate/100;
	if(hp_rate != 100)
		mcd->max_hp = mcd->max_hp*hp_rate/100;
	if(sp_rate != 100)
		mcd->max_sp = mcd->max_sp*sp_rate/100;
	if(aspd_rate != 100)
		mcd->amotion = mcd->amotion*aspd_rate/100;
	if(speed_rate != 100)
		mcd->speed = mcd->speed*speed_rate/100;

	if(mcd->max_hp <= 0)
		mcd->max_hp = 1;
	if(mcd->max_sp <= 0)
		mcd->max_sp = 1;

	// ���R��
	mcd->nhealhp = (int)(((atn_bignumber)mcd->max_hp * mcd->vit / 10000 + 1) * 6);
	mcd->nhealsp = (int)(((atn_bignumber)mcd->max_sp * (mcd->int_ + 10) / 750) + 1);
	if(mcd->hprecov_rate != 100)
		mcd->nhealhp = mcd->nhealhp*mcd->hprecov_rate/100;
	if(mcd->sprecov_rate != 100)
		mcd->nhealsp = mcd->nhealsp*mcd->sprecov_rate/100;

	if( mcd->sc.data[SC_AUTOBERSERK].timer != -1 &&
	    mcd->status.hp < mcd->status.max_hp>>2 &&
	    (mcd->sc.data[SC_PROVOKE].timer == -1 || mcd->sc.data[SC_PROVOKE].val2 == 0) &&
	    !unit_isdead(&mcd->bl) )
	{
		// �I�[�g�o�[�T�[�N����
		status_change_start(&mcd->bl,SC_PROVOKE,10,1,0,0,0,0);
	}

	return 0;
}

/*==========================================
 * �b������
 *------------------------------------------
 */
int merc_callmerc(struct map_session_data *sd,int class_, unsigned int limit)
{
#ifdef TXT_ONLY
	struct mmo_mercstatus st;
	int i, id;

	nullpo_retr(0, sd);

	if(sd->status.merc_id > 0 || sd->mcd)	// ���ɏ�����
		return 0;
	if(sd->state.merc_creating)
		return 0;

	class_ -= MERC_ID;
	if(class_ < 0 || class_ >= MAX_MERC_DB)
		return 0;

	memset(&st, 0, sizeof(st));

	memcpy(st.name, merc_db[class_].jname , 24);
	st.class_     = class_ + MERC_ID;
	st.account_id = sd->status.account_id;
	st.char_id    = sd->status.char_id;
	st.base_level = merc_db[class_].lv;

	// �����X�e�[�^�X��DB���疄�ߍ���
	st.max_hp = merc_db[class_].max_hp;
	st.max_sp = merc_db[class_].max_sp;
	st.str    = merc_db[class_].str;
	st.agi    = merc_db[class_].agi;
	st.vit    = merc_db[class_].vit;
	st.int_   = merc_db[class_].int_;
	st.dex    = merc_db[class_].dex;
	st.luk    = merc_db[class_].luk;
	st.limit  = limit + (unsigned int)time(NULL);

	st.hp = st.max_hp;
	st.sp = st.max_sp;

	// �X�L���擾
	for(i = 0; (id = merc_skill_tree[class_][i].id) > 0; i++) {
		id -= MERC_SKILLID;
		st.skill[id].id = id + MERC_SKILLID;
		st.skill[id].lv = merc_skill_tree[class_][i].max;
	}

	// ������+1
	merc_set_call(sd, merc_db[class_].class_type, 1);

	sd->state.merc_creating = 1;
	intif_create_merc(sd->status.account_id,sd->status.char_id,&st);

	return 1;
#else
	// SQL���[�h�͔�Ή�
	return 0;
#endif
}

static int merc_natural_heal_hp(int tid,unsigned int tick,int id,void *data);
static int merc_natural_heal_sp(int tid,unsigned int tick,int id,void *data);

/*==========================================
 *
 *------------------------------------------
 */
static int merc_data_init(struct map_session_data *sd)
{
	struct merc_data *mcd;
	int i,class_;
	unsigned int tick = gettick();
	unsigned int now  = (unsigned int)time(NULL);
	unsigned int diff;

	nullpo_retr(1, sd);
	nullpo_retr(1, mcd = sd->mcd);

	class_ = sd->mcd->status.class_ - MERC_ID;

	mcd->bl.m    = sd->bl.m;
	mcd->bl.prev = mcd->bl.next = NULL;
	mcd->bl.x    = mcd->ud.to_x = sd->bl.x;
	mcd->bl.y    = mcd->ud.to_y = sd->bl.y;
	merc_calc_pos(mcd,sd->bl.x,sd->bl.y,sd->dir);
	mcd->bl.x        = mcd->ud.to_x;
	mcd->bl.y        = mcd->ud.to_y;
	mcd->bl.id       = npc_get_new_npc_id();
	mcd->dir         = sd->dir;
	mcd->speed       = status_get_speed(&sd->bl);	// ���s���x�́A�R�[�����̎�l��speed�ɂȂ�
	mcd->bl.type     = BL_MERC;
	mcd->target_id   = 0;
	mcd->msd         = sd;
	mcd->class_type  = merc_db[class_].class_type;
	mcd->view_class  = merc_db[class_].view_class;
	mcd->attackrange = merc_db[class_].range;

	for(i=0; i<MAX_MERCSKILL; i++)
		mcd->skillstatictimer[i] = tick;

#ifdef DYNAMIC_SC_DATA
	// �_�~�[�}��
	mcd->sc.data = dummy_sc_data;
#else
	// �X�e�[�^�X�ُ�̏�����
	for(i=0; i<MAX_STATUSCHANGE; i++) {
		mcd->sc.data[i].timer = -1;
		mcd->sc.data[i].val1  = 0;
		mcd->sc.data[i].val2  = 0;
		mcd->sc.data[i].val3  = 0;
		mcd->sc.data[i].val4  = 0;
	}
#endif

	mcd->sc.count = 0;
	mcd->sc.opt1  = 0;
	mcd->sc.opt2  = 0;
	mcd->sc.opt3  = 0;

	mcd->status.option &= OPTION_MASK;
	mcd->sc.option = mcd->status.option;	// option��sc�Ɉڂ��Ďg��

	merc_calc_status(mcd);			// �X�e�[�^�X�v�Z
	unit_dataset(&mcd->bl);
	map_addiddb(&mcd->bl);

	mcd->natural_heal_hp = add_timer(tick+MERC_NATURAL_HEAL_HP_INTERVAL,merc_natural_heal_hp,mcd->bl.id,NULL);
	mcd->natural_heal_sp = add_timer(tick+MERC_NATURAL_HEAL_SP_INTERVAL,merc_natural_heal_sp,mcd->bl.id,NULL);

	if(mcd->status.limit > now)
		diff = (mcd->status.limit - now) * 1000;
	else
		diff = 1;
	mcd->limit_timer = add_timer(tick+diff,merc_employ_timer,sd->bl.id,NULL);

	mcd->view_size = 0;

	return 0;
}

/*==========================================
 * inter����b���̃f�[�^��M
 *------------------------------------------
 */
int merc_recv_mercdata(int account_id,int char_id,struct mmo_mercstatus *p,int flag)
{
	struct map_session_data *sd;

	nullpo_retr(0, p);

	sd = map_id2sd(account_id);

	if(sd == NULL || sd->status.char_id != char_id || (sd->status.merc_id && sd->status.merc_id != p->merc_id))
	{
		if(flag) {
			// �V�K�쐬���Ȃ�b���f�[�^���폜����
			intif_delete_mercdata(account_id,char_id,p->merc_id);
		}
		if(sd)
			sd->state.merc_creating = 0;
		return 0;
	}

	if(sd->mcd == NULL) {
		sd->mcd = (struct merc_data *)aCalloc(1,sizeof(struct merc_data));
		memcpy(&sd->mcd->status, p, sizeof(struct mmo_mercstatus));

		if(sd->status.merc_id <= 0)
			sd->status.merc_id = p->merc_id;

		if(!merc_data_init(sd) && sd->bl.prev != NULL)
		{
			if(sd->mcd->status.hp <= 0) {	// ���S
				merc_delete_data(sd);
				sd->state.merc_creating = 0;
				return 0;
			}
			map_addblock(&sd->mcd->bl);
			mob_ai_hard_spawn( &sd->mcd->bl, 1 );
			clif_spawnmerc(sd->mcd);
			clif_send_mercstatus(sd);
			clif_mercskillinfoblock(sd);
			merc_save_data(sd);
			skill_unit_move(&sd->mcd->bl,gettick(),1);
		}
	}
	sd->state.merc_creating = 0;

	return 0;
}

/*==========================================
 * �b���폜
 *------------------------------------------
 */
int merc_delete_data(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	if(sd->status.merc_id > 0 && sd->mcd) {
		// �e���x�ۑ�
		//if(battle_config.save_homun_temporal_intimate)
		//	pc_setglobalreg(sd,"HOM_TEMP_INTIMATE",2000);	// �����l��
		unit_free(&sd->mcd->bl,0);
		intif_delete_mercdata(sd->status.account_id,sd->status.char_id,sd->status.merc_id);
		sd->status.merc_id = 0;
		chrif_save(sd,0);
		if(sd->state.storage_flag == 1)
			storage_storage_save(sd);
	}

	return 0;
}

/*==========================================
 * �b�����j���[�̉���
 *------------------------------------------
 */
int merc_menu(struct map_session_data *sd, int menunum)
{
	nullpo_retr(0, sd);

	if(!sd->mcd)
		return 0;

	switch(menunum) {
		case 1:
			clif_send_mercstatus(sd);
			clif_mercskillinfoblock(sd);
			break;
		case 2:
			//clif_disp_onlyself(sd->fd, msg_txt(190));	// �b�������ق��܂����B
			clif_msgstringtable(sd, 0x4f4);
			merc_delete_data(sd);
			break;
	}
	return 0;
}

/*==========================================
 * �ҋ@���߂ȂǂŁA��l�̉��ֈړ�
 *------------------------------------------
 */
int merc_return_master(struct map_session_data *sd)
{
	struct merc_data *mcd;

	nullpo_retr(0, sd);
	nullpo_retr(0, mcd = sd->mcd);

	merc_calc_pos(mcd,sd->bl.x,sd->bl.y,sd->dir);
	unit_walktoxy(&mcd->bl,mcd->ud.to_x,mcd->ud.to_y);
	return 0;
}

/*==========================================
 * �X�L���̌��� ���L���Ă����ꍇLv���Ԃ�
 *------------------------------------------
 */
int merc_checkskill(struct merc_data *mcd,int skill_id)
{
	if(mcd == NULL)
		return 0;
	if(skill_id >= MERC_SKILLID)
		skill_id -= MERC_SKILLID;

	if(skill_id >= MAX_MERCSKILL)
		return 0;
	if(mcd->status.skill[skill_id].id == skill_id + MERC_SKILLID)
		return mcd->status.skill[skill_id].lv;

	return 0;
}

/*==========================================
 * �����l�̑���
 *------------------------------------------
 */
int merc_set_fame(struct map_session_data *sd, short type, int val)
{
	struct merc_data *mcd;

	nullpo_retr(0, sd);
	nullpo_retr(0, mcd = sd->mcd);

	if(type < 0 || type >= MAX_MERC_TYPE)
		return 0;

	sd->status.merc_fame[type] += val;
	if(sd->status.merc_fame[type] < 0)
		sd->status.merc_fame[type] = 0;

	clif_mercupdatestatus(sd, SP_MERC_FAME);

	return 0;
}

/*==========================================
 * �����񐔂̑���
 *------------------------------------------
 */
int merc_set_call(struct map_session_data *sd, short type, int val)
{
	struct merc_data *mcd;;

	nullpo_retr(0, sd);
	nullpo_retr(0, mcd = sd->mcd);

	if(type < 0 || type >= MAX_MERC_TYPE)
		return 0;

	sd->status.merc_call[type] += val;
	if(sd->status.merc_call[type] < 0)
		sd->status.merc_call[type] = 0;

	return 0;
}

/*==========================================
 * �L���J�E���g�̑���
 *------------------------------------------
 */
int merc_killcount(struct merc_data *mcd, unsigned short lv)
{
	nullpo_retr(0, mcd);

	// ����̃��x���������̃��x����1/2�ȏォ�H
	if(lv >= mcd->status.base_level / 2) {
		mcd->status.kill_count++;

		// �L���J�E���g50���ɖ����l+1
		if(mcd->status.kill_count % 50 == 0) {
			merc_set_fame(mcd->msd, mcd->class_type, 1);
		}

		if(mcd->msd)
			clif_mercupdatestatus(mcd->msd, SP_MERC_KILLCOUNT);
	}

	return 0;
}

/*==========================================
 * �o���l�擾
 *------------------------------------------
 */
int merc_gainexp(struct merc_data *mcd,struct mob_data *md,atn_bignumber base_exp,atn_bignumber job_exp)
{
	nullpo_retr(0, mcd);

	if(mcd->bl.prev == NULL || unit_isdead(&mcd->bl))
		return 0;

	if(md && md->sc.data[SC_RICHMANKIM].timer != -1) {
		base_exp = base_exp * (125 + md->sc.data[SC_RICHMANKIM].val1*11)/100;
		job_exp  = job_exp  * (125 + md->sc.data[SC_RICHMANKIM].val1*11)/100;
	}

	if(mcd->msd) {	// ��l�֎w��{�̌o���l
		atn_bignumber mbexp = 0, mjexp = 0;
		if(battle_config.master_get_merc_base_exp)
			mbexp = base_exp * battle_config.master_get_merc_base_exp / 100;
		if(battle_config.master_get_merc_job_exp)
			mjexp = job_exp  * battle_config.master_get_merc_job_exp / 100;

		if(mbexp || mjexp)
			pc_gainexp(mcd->msd,md,mbexp,mjexp);
	}

	return 0;
}

/*==========================================
 * mcd��damage�̃_���[�W
 *------------------------------------------
 */
int merc_damage(struct block_list *src,struct merc_data *mcd,int damage)
{
	struct map_session_data *sd = NULL;

	nullpo_retr(0, mcd);
	nullpo_retr(0, sd = mcd->msd);

	// ���Ɏ���ł����疳��
	if(unit_isdead(&mcd->bl))
		return 0;

	// �����Ă����瑫���~�߂�
	if((mcd->sc.data[SC_ENDURE].timer == -1 || map[mcd->bl.m].flag.gvg) && mcd->sc.data[SC_BERSERK].timer == -1)
		unit_stop_walking(&mcd->bl,battle_config.pc_hit_stop_type);

	if(damage > 0 && mcd->sc.data[SC_GRAVITATION_USER].timer != -1)
		status_change_end(&mcd->bl, SC_GRAVITATION_USER, -1);

	if(mcd->bl.prev == NULL) {
		if(battle_config.error_log)
			printf("merc_damage : BlockError!!\n");
		return 0;
	}

	if(mcd->status.hp > mcd->max_hp)
		mcd->status.hp = mcd->max_hp;

	// over kill���͊ۂ߂�
	if(damage > mcd->status.hp)
		damage = mcd->status.hp;

	mcd->status.hp -= damage;

	// �n�C�h��Ԃ�����
	status_change_hidden_end(&mcd->bl);

	clif_mercupdatestatus(sd,SP_HP);

	// ���S���Ă���
	if(mcd->status.hp <= 0) {
		//if(battle_config.save_homun_temporal_intimate)
		//	pc_setglobalreg(sd,"HOM_TEMP_INTIMATE",hd->intimate);

		// �X�L�����j�b�g����̗��E
		mcd->status.hp = 1;
		skill_unit_move(&mcd->bl,gettick(),0);
		mcd->status.hp = 0;

		// �����l-1
		merc_set_fame(sd, mcd->class_type, -1);

		//clif_disp_onlyself(sd->fd, msg_txt(189));	// �b�������S���܂����B
		clif_msgstringtable(sd, 0x4f3);
		merc_delete_data(sd);
	} else {
		if( mcd->sc.data[SC_AUTOBERSERK].timer != -1 &&
		    mcd->status.hp < mcd->status.max_hp>>2 &&
		    (mcd->sc.data[SC_PROVOKE].timer == -1 || mcd->sc.data[SC_PROVOKE].val2 == 0) )
		{
			// �I�[�g�o�[�T�[�N����
			status_change_start(&mcd->bl,SC_PROVOKE,10,1,0,0,0,0);
		}
	}

	return 0;
}

/*==========================================
 * HP/SP��
 *------------------------------------------
 */
int merc_heal(struct merc_data *mcd,int hp,int sp)
{
	nullpo_retr(0, mcd);

	// �o�[�T�[�N���͉񕜂����Ȃ�
	if(mcd->sc.data[SC_BERSERK].timer != -1) {
		if(sp > 0)
			sp = 0;
		if(hp > 0)
			hp = 0;
	}

	if(hp + mcd->status.hp > mcd->max_hp)
		hp = mcd->max_hp - mcd->status.hp;
	if(sp + mcd->status.sp > mcd->max_sp)
		sp = mcd->max_sp - mcd->status.sp;
	mcd->status.hp += hp;
	if(mcd->status.hp <= 0) {
		mcd->status.hp = 0;
		merc_damage(NULL,mcd,1);
		hp = 0;
	}
	mcd->status.sp += sp;
	if(mcd->status.sp <= 0)
		mcd->status.sp = 0;
	if(mcd->msd) {
		if(hp)
			clif_mercupdatestatus(mcd->msd,SP_HP);
		if(sp)
			clif_mercupdatestatus(mcd->msd,SP_SP);
	}

	return hp + sp;
}

/*==========================================
 * ���R�񕜕�
 *------------------------------------------
 */
static int merc_natural_heal_hp(int tid,unsigned int tick,int id,void *data)
{
	struct merc_data *mcd = map_id2mcd(id);
	int bhp;

	nullpo_retr(0, mcd);

	if(mcd->natural_heal_hp != tid) {
		if(battle_config.error_log)
			printf("merc_natural_heal_hp %d != %d\n",mcd->natural_heal_hp,tid);
		return 0;
	}
	mcd->natural_heal_hp = -1;

	bhp = mcd->status.hp;

	if(mcd->ud.walktimer == -1) {
		mcd->status.hp += mcd->nhealhp;
		if(mcd->status.hp > mcd->max_hp)
			mcd->status.hp = mcd->max_hp;
		if(bhp != mcd->status.hp && mcd->msd)
			clif_mercupdatestatus(mcd->msd,SP_HP);
	}
	mcd->natural_heal_hp = add_timer(tick+MERC_NATURAL_HEAL_HP_INTERVAL,merc_natural_heal_hp,mcd->bl.id,NULL);

	return 0;
}

static int merc_natural_heal_sp(int tid,unsigned int tick,int id,void *data)
{
	struct merc_data *mcd = map_id2mcd(id);
	int bsp;

	nullpo_retr(0, mcd);

	if(mcd->natural_heal_sp != tid) {
		if(battle_config.error_log)
			printf("merc_natural_heal_sp %d != %d\n",mcd->natural_heal_sp,tid);
		return 0;
	}
	mcd->natural_heal_sp = -1;

	bsp = mcd->status.sp;

	if(mcd->ud.walktimer == -1) {
		mcd->status.sp += mcd->nhealsp;
		if(mcd->status.sp > mcd->max_sp)
			mcd->status.sp = mcd->max_sp;
		if(bsp != mcd->status.sp && mcd->msd)
			clif_mercupdatestatus(mcd->msd,SP_SP);
	}
	mcd->natural_heal_sp = add_timer(tick+MERC_NATURAL_HEAL_SP_INTERVAL,merc_natural_heal_sp,mcd->bl.id,NULL);

	return 0;
}

int merc_natural_heal_timer_delete(struct merc_data *mcd)
{
	nullpo_retr(0, mcd);

	if(mcd->natural_heal_hp != -1) {
		delete_timer(mcd->natural_heal_hp,merc_natural_heal_hp);
		mcd->natural_heal_hp = -1;
	}
	if(mcd->natural_heal_sp != -1) {
		delete_timer(mcd->natural_heal_sp,merc_natural_heal_sp);
		mcd->natural_heal_sp = -1;
	}

	return 0;
}

/*==========================================
 * �b���̃f�[�^���Z�[�u
 *------------------------------------------
 */
int merc_save_data(struct map_session_data *sd)
{
	struct merc_data *mcd;

	nullpo_retr(0, sd);
	nullpo_retr(0, mcd = sd->mcd);

	//if(battle_config.save_homun_temporal_intimate)
	//	pc_setglobalreg(sd,"HOM_TEMP_INTIMATE",mcd->intimate);

	// option�̃R�s�[
	mcd->status.option = mcd->sc.option;

	intif_save_mercdata(sd->status.account_id,&sd->mcd->status);

	return 0;
}

//
// ��������
//
/*==========================================
 * �b�������X�e�[�^�X�f�[�^�ǂݍ���
 *------------------------------------------
 */
static int read_merc_db(void)
{
	FILE *fp;
	char line[4096];
	int i, j;
	int lines, count = 0;
	struct script_code *script = NULL;
	const char *filename[] = { "db/merc_db.txt", "db/addon/merc_db_add.txt" };

	// DB���̏�����
	for(i=0; i<MAX_MERC_DB; i++) {
		if(merc_db[i].script)
			script_free_code(merc_db[i].script);
	}
	memset(merc_db,0,sizeof(merc_db));

	for(i=0;i<2;i++){
		fp=fopen(filename[i],"r");
		if(fp==NULL){
			if(i>0)
				continue;
			printf("can't read %s\n",filename[i]);
			return -1;
		}
		lines=count=0;
		while(fgets(line,sizeof(line),fp)){
			int nameid;
			short class_type;
			char *str[26],*p,*np;
			lines++;

			if(line[0] == '\0' || line[0] == '\r' || line[0] == '\n')
				continue;
			if(line[0] == '/' && line[1] == '/')
				continue;

			for(j=0,p=line;j<26;j++){
				if((np=strchr(p,','))!=NULL){
					str[j]=p;
					*np=0;
					p=np+1;
				} else {
					str[j]=p;
					p+=strlen(p);
				}
			}

			nameid = atoi(str[0]);
			j = nameid - MERC_ID;

			if(j < 0 || j >= MAX_MERC_DB)
				continue;

			class_type = atoi(str[4]);
			if(class_type < 0 || class_type >= MAX_MERC_TYPE)
				continue;

			merc_db[j].class_     = nameid;
			merc_db[j].class_type = class_type;
			merc_db[j].view_class = atoi(str[1]);
			strncpy(merc_db[j].name,str[2],24);
			strncpy(merc_db[j].jname,str[3],24);
			merc_db[j].lv      = atoi(str[5]);
			merc_db[j].max_hp  = atoi(str[6]);
			merc_db[j].max_sp  = atoi(str[7]);
			merc_db[j].range   = atoi(str[8]);
			merc_db[j].atk1    = atoi(str[9]);
			merc_db[j].atk2    = atoi(str[10]);
			merc_db[j].def     = atoi(str[11]);
			merc_db[j].mdef    = atoi(str[12]);
			merc_db[j].str     = atoi(str[13]);
			merc_db[j].agi     = atoi(str[14]);
			merc_db[j].vit     = atoi(str[15]);
			merc_db[j].int_    = atoi(str[16]);
			merc_db[j].dex     = atoi(str[17]);
			merc_db[j].luk     = atoi(str[18]);

			merc_db[j].size    = atoi(str[19]);
			merc_db[j].race    = atoi(str[20]);
			merc_db[j].element = atoi(str[21]);
			merc_db[j].speed   = atoi(str[22]);
			merc_db[j].adelay  = atoi(str[23]);
			merc_db[j].amotion = atoi(str[24]);
			merc_db[j].dmotion = atoi(str[25]);

			// force \0 terminal
			merc_db[j].name[23]  = '\0';
			merc_db[j].jname[23] = '\0';

			if((np = strchr(p,'{')) == NULL)
				continue;

			if(merc_db[j].script)
				script_free_code(merc_db[j].script);
			script = parse_script(np,filename[i],lines);

			merc_db[j].script = (script != &error_code)? script: NULL;
			count++;
		}
		fclose(fp);
		printf("read %s done (count=%d)\n",filename[i],count);
	}
	return 0;
}

/*==========================================
 * �b���X�L���f�[�^�ǂݍ���
 *------------------------------------------
 */
static int read_merc_skilldb(void)
{
	int i,j,class_=0;
	FILE *fp;
	char line[1024],*p;

	// �X�L���c���[
	memset(merc_skill_tree,0,sizeof(merc_skill_tree));
	fp=fopen("db/merc_skill_tree.txt","r");
	if(fp==NULL){
		printf("can't read db/merc_skill_tree.txt\n");
		return 1;
	}
	while(fgets(line,1020,fp)){
		int skillid;
		char *split[3];
		struct merc_skill_tree_entry *st;

		if(line[0] == '\0' || line[0] == '\r' || line[0] == '\n')
			continue;
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<3 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(j < 3)
			continue;
		class_ = atoi(split[0]);
		i = class_ - MERC_ID;
		if(i < 0 || i >= MAX_MERC_DB)
			continue;

		skillid = atoi(split[1]);
		if(skillid < MERC_SKILLID || skillid >= MAX_MERC_SKILLID)
			continue;

		st = merc_skill_tree[i];
		for(j=0; st[j].id && st[j].id != skillid; j++);

		if(j >= MAX_MERCSKILL_TREE - 1) {
			// �����̓A���J�[�Ƃ���0�ɂ��Ă����K�v������
			printf("read_merc_skilldb: skill (%d) is over max tree %d!!\n", skillid, MAX_MERCSKILL_TREE);
			continue;
		}
		if(j > 0 && skillid < st[j-1].id) {
			// �X�L��ID�̏����ɕ���łȂ��ꍇ
			int max = j;
			while(j > 0 && skillid < st[j-1].id) {
				j--;
			}
			memmove(&st[j+1], &st[j], (max-j)*sizeof(st[0]));
		}

		st[j].id  = skillid;
		st[j].max = atoi(split[2]);

		if(st[j].max > skill_get_max(skillid))
			st[j].max = skill_get_max(skillid);
	}
	fclose(fp);
	printf("read db/merc_skill_tree.txt done\n");

	return 0;
}

/*==========================================
 * �b��DB�̃����[�h
 *------------------------------------------
 */
void merc_reload(void)
{
	read_merc_db();
	read_merc_skilldb();
}

/*==========================================
 * ����������
 *------------------------------------------
 */
int do_init_merc(void)
{
	read_merc_db();
	read_merc_skilldb();

	add_timer_func_list(merc_natural_heal_hp);
	add_timer_func_list(merc_natural_heal_sp);
	add_timer_func_list(merc_employ_timer);

	return 0;
}

/*==========================================
 * �I��
 *------------------------------------------
 */
int do_final_merc(void)
{
	int i;

	for(i = 0; i < MAX_MERC_DB; i++) {
		if(merc_db[i].script) {
			script_free_code(merc_db[i].script);
		}
	}
	return 0;
}
