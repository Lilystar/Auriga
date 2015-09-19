#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "db.h"
#include "timer.h"
#include "nullpo.h"
#include "malloc.h"
#include "utils.h"

#include "battle.h"
#include "map.h"
#include "pc.h"
#include "skill.h"
#include "mob.h"
#include "homun.h"
#include "itemdb.h"
#include "clif.h"
#include "pet.h"
#include "guild.h"
#include "status.h"
#include "party.h"
#include "unit.h"
#include "ranking.h"
#include "merc.h"

struct Battle_Config battle_config;

struct battle_delay_damage_ {
	struct block_list *src;
	int target;
	int damage;
	short skillid;
	short skilllv;
	int flag;
};

/*==========================================
 * �_���[�W�̒x��
 *------------------------------------------
 */
static int battle_delay_damage_sub(int tid,unsigned int tick,int id,int data)
{
	struct battle_delay_damage_ *dat = (struct battle_delay_damage_ *)data;

	if(dat) {
		struct block_list *target = map_id2bl(dat->target);

		if(map_id2bl(id) == dat->src && target && target->prev != NULL) {
			battle_damage(dat->src,target,dat->damage,dat->skillid,dat->skilllv,dat->flag);
		}
		aFree(dat);
	}
	return 0;
}

int battle_delay_damage(unsigned int tick,struct block_list *src,struct block_list *target,int damage,int skillid,int skilllv,int flag)
{
	struct battle_delay_damage_ *dat;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	dat = (struct battle_delay_damage_*)aCalloc(1,sizeof(struct battle_delay_damage_));
	dat->src     = src;
	dat->target  = target->id;
	dat->damage  = damage;
	dat->skillid = skillid;
	dat->skilllv = skilllv;
	dat->flag    = flag;
	add_timer2(tick,battle_delay_damage_sub,src->id,(int)dat,TIMER_FREE_DATA);

	return 0;
}

/*==========================================
 * ���ۂ�HP�𑀍�
 *------------------------------------------
 */
int battle_damage(struct block_list *bl,struct block_list *target,int damage,int skillid,int skilllv,int flag)
{
	struct map_session_data *sd = NULL, *tsd = NULL;
	struct mob_data   *tmd  = NULL;
	struct homun_data *thd  = NULL;
	struct merc_data  *tmcd = NULL;

	nullpo_retr(0, target);	// bl��NULL�ŌĂ΂�邱�Ƃ�����̂ő��Ń`�F�b�N

	if(damage == 0 || target->type == BL_PET)
		return 0;

	if(target->prev == NULL)
		return 0;

	if(bl) {
		if(bl->prev == NULL)
			return 0;
		if(bl->type == BL_PC)
			sd = (struct map_session_data *)bl;
	}

	if(damage < 0)
		return battle_heal(bl,target,-damage,0,flag);

	map_freeblock_lock();

	if(target->type == BL_SKILL) {
		skill_unit_ondamaged((struct skill_unit *)target,bl,damage,gettick());
		map_freeblock_unlock();
		return 0;
	}

	tsd  = BL_DOWNCAST( BL_PC,   target );
	tmd  = BL_DOWNCAST( BL_MOB,  target );
	thd  = BL_DOWNCAST( BL_HOM,  target );
	tmcd = BL_DOWNCAST( BL_MERC, target );

	if(tsd) {
		// �f�B�{�[�V�������������Ă���
		if( tsd &&
		    tsd->sc.data[SC_DEVOTION].timer != -1 &&
		    tsd->sc.data[SC_DEVOTION].val1 &&
		    skillid != PA_PRESSURE &&
		    skillid != SA_COMA &&
		    skillid != NPC_DARKBLESSING &&
		    (skillid != CR_GRANDCROSS || bl == NULL || bl != target) )
		{
			struct map_session_data *msd = map_id2sd(tsd->sc.data[SC_DEVOTION].val1);

			if(msd && skill_devotion3(msd,tsd->bl.id)) {
				skill_devotion(msd);
			} else if(msd && bl) {
				int i;
				for(i=0; i<5; i++) {
					if(msd->dev.val1[i] != target->id)
						continue;
					// �_���[�W���[�V�����t���Ń_���[�W�\��
					clif_damage(&msd->bl,&msd->bl,gettick(),0,status_get_dmotion(&msd->bl),damage,0,0,0);
					battle_damage(bl,&msd->bl,damage,skillid,skilllv,flag);
					map_freeblock_unlock();
					return 0;
				}
			}
		}
	}

	status_change_attacked_end(target);	// �����E�Ή��E����������
	unit_skillcastcancel(target,2);		// �r���W�Q

	if(tsd)       pc_damage(bl,tsd,damage);
	else if(tmd)  mob_damage(bl,tmd,damage,0);
	else if(thd)  homun_damage(bl,thd,damage);
	else if(tmcd) merc_damage(bl,tmcd,damage);

	// �J�[�h���ʂ̃R�[�}�E����
	if(sd && target && target->prev && !unit_isdead(target) && flag&(BF_WEAPON|BF_NORMAL) && status_get_class(target) != 1288)
	{
		int race = status_get_race(target);
		int ele  = status_get_elem_type(target);
		int mode = status_get_mode(target);

		if(atn_rand()%10000 < sd->weapon_coma_ele[ele] ||
		   atn_rand()%10000 < sd->weapon_coma_race[race] ||
		   (mode&0x20 && atn_rand()%10000 < sd->weapon_coma_race[RCT_BOSS]) ||
		   (!(mode&0x20) && atn_rand()%10000 < sd->weapon_coma_race[RCT_NONBOSS]))
		{
			int hp = status_get_hp(target);
			if(tsd)       pc_damage(bl,tsd,hp);
			else if(tmd)  mob_damage(bl,tmd,hp,0);
			else if(thd)  homun_damage(bl,thd,hp);
			else if(tmcd) merc_damage(bl,tmcd,hp);
		}
		else if(atn_rand()%10000 < sd->weapon_coma_ele2[ele] ||
			atn_rand()%10000 < sd->weapon_coma_race2[race] ||
			(mode&0x20 && atn_rand()%10000 < sd->weapon_coma_race2[RCT_BOSS]) ||
			(!(mode&0x20) && atn_rand()%10000 < sd->weapon_coma_race2[RCT_BOSS]))
		{
			int hp = status_get_hp(target) - 1;
			if(tsd)       pc_damage(bl,tsd,hp);
			else if(tmd)  mob_damage(bl,tmd,hp,0);
			else if(thd)  homun_damage(bl,thd,hp);
			else if(tmcd) merc_damage(bl,tmcd,hp);
		}
	}
	map_freeblock_unlock();

	return 0;
}

/*==========================================
 * HP/SP��
 *------------------------------------------
 */
int battle_heal(struct block_list *bl,struct block_list *target,int hp,int sp,int flag)
{
	nullpo_retr(0, target);	// bl��NULL�ŌĂ΂�邱�Ƃ�����̂ő��Ń`�F�b�N

	if(target->type == BL_PET)
		return 0;
	if(unit_isdead(target))
		return 0;
	if(hp == 0 && sp == 0)
		return 0;

	if(hp < 0)
		return battle_damage(bl,target,-hp,0,0,flag);

	if(target->type == BL_MOB)
		return mob_heal((struct mob_data *)target,hp);
	else if(target->type == BL_PC)
		return pc_heal((struct map_session_data *)target,hp,sp);
	else if(target->type == BL_HOM)
		return homun_heal((struct homun_data *)target,hp,sp);
	else if(target->type == BL_MERC)
		return merc_heal((struct merc_data *)target,hp,sp);

	return 0;
}

/*==========================================
 * �_���[�W�̑����C��
 *------------------------------------------
 */
int battle_attr_fix(int damage,int atk_elem,int def_elem)
{
	int def_type, def_lv, rate;

	// ��������(!=������)
	if (atk_elem == ELE_NONE)
		return damage;

	def_type = def_elem%20;
	def_lv   = def_elem/20;

	if( atk_elem == ELE_MAX )
		atk_elem = atn_rand()%ELE_MAX;	// ���푮�������_���ŕt��

	if( atk_elem < 0 || atk_elem >= ELE_MAX ||
	    def_type < 0 || def_type >= ELE_MAX ||
	    def_lv <= 0 || def_lv > MAX_ELE_LEVEL )
	{
		// �����l�����������̂łƂ肠�������̂܂ܕԂ�
		if(battle_config.error_log)
			printf("battle_attr_fix: unknown attr type: atk=%d def_type=%d def_lv=%d\n",atk_elem,def_type,def_lv);
		return damage;
	}

	rate = attr_fix_table[def_lv-1][atk_elem][def_type];
	if(damage < 0 && rate < 0)	// ���~���̏ꍇ�͌��ʂ𕉂ɂ���
		rate = -rate;

	return damage * rate / 100;
}

/*==========================================
 * �_���[�W�ŏI�v�Z
 *------------------------------------------
 */
static int battle_calc_damage(struct block_list *src,struct block_list *bl,int damage,int div_,int skill_num,int skill_lv,int flag)
{
	struct map_session_data *tsd = NULL;
	struct mob_data         *tmd = NULL;
	struct unit_data *ud;
	struct status_change *sc;
	unsigned int tick = gettick();

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	tsd = BL_DOWNCAST( BL_PC,  bl );
	tmd = BL_DOWNCAST( BL_MOB, bl );

	ud = unit_bl2ud(bl);
	sc = status_get_sc(bl);

	// �X�L���_���[�W�␳
	if(damage > 0 && skill_num > 0) {
		int damage_rate = 100;
		if(map[bl->m].flag.normal)
			damage_rate = skill_get_damage_rate(skill_num,0);
		else if(map[bl->m].flag.gvg)
			damage_rate = skill_get_damage_rate(skill_num,2);
		else if(map[bl->m].flag.pvp)
			damage_rate = skill_get_damage_rate(skill_num,1);
		else if(map[bl->m].flag.pk)
			damage_rate = skill_get_damage_rate(skill_num,3);
		if(damage_rate != 100)
			damage = damage*damage_rate/100;
	}
	if(sc && sc->count > 0 && skill_num != PA_PRESSURE && skill_num != HW_GRAVITATION) {
		// �A�X���v�e�B�I
		if(sc->data[SC_ASSUMPTIO].timer != -1 && damage > 0) {
			if(map[bl->m].flag.pvp || map[bl->m].flag.gvg)
				damage = damage * 2 / 3;
			else
				damage = damage / 2;
		}

		// �S�X�y���̓����Ԉُ�
		if(sc->data[SC_INCDAMAGE].timer != -1 && damage > 0)
			damage += damage * sc->data[SC_INCDAMAGE].val1/100;

		// �o�W���J
		if(sc->data[SC_BASILICA].timer != -1 && damage > 0 && !(status_get_mode(src)&0x20))
			damage = 0;

		// �E�H�[���I�u�t�H�O
		if(sc->data[SC_FOGWALL].timer != -1 && damage > 0 && flag&BF_WEAPON && flag&BF_LONG)
		{
			if(skill_num == 0) {	// �ʏ�U��75%OFF
				damage = damage * 25 / 100;
			} else {		// �X�L��25%OFF
				damage = damage * 75 / 100;
			}
		}

		// �Z�C�t�e�B�E�H�[��
		if(sc->data[SC_SAFETYWALL].timer != -1 && flag&BF_SHORT && skill_num != NPC_GUIDEDATTACK) {
			struct skill_unit *unit = map_id2su(sc->data[SC_SAFETYWALL].val2);
			if(unit && unit->group) {
				if((--unit->group->val2) <= 0)
					skill_delunit(unit);
				damage = 0;
			} else {
				status_change_end(bl,SC_SAFETYWALL,-1);
			}
		}

		// �J�E�v
		if(sc->data[SC_KAUPE].timer != -1 && atn_rand()%100 < sc->data[SC_KAUPE].val2) {
			status_change_end(bl,SC_KAUPE,-1);
			damage = 0;
		}

		// �j���[�}
		if( (sc->data[SC_PNEUMA].timer != -1 || sc->data[SC_TATAMIGAESHI].timer != -1) && damage > 0 &&
		    flag&(BF_WEAPON|BF_MISC) && flag&BF_LONG && skill_num != NPC_GUIDEDATTACK )
		{
			damage = 0;
		}

		// ���b�N�X�G�[�e���i
		if(sc->data[SC_AETERNA].timer != -1 && damage > 0) {
			damage <<= 1;
			status_change_end(bl,SC_AETERNA,-1);
		}

		// ������̃_���[�W����
		if(sc->data[SC_VOLCANO].timer != -1 && damage > 0) {	// �{���P�[�m
			if( flag&BF_SKILL && skill_get_pl(skill_num) == ELE_FIRE )
				damage += damage * sc->data[SC_VOLCANO].val4 / 100;
			else if( !(flag&BF_SKILL) && status_get_attack_element(bl) == ELE_FIRE )
				damage += damage * sc->data[SC_VOLCANO].val4 / 100;
		}
		if(sc->data[SC_VIOLENTGALE].timer != -1 && damage > 0) {	// �o�C�I�����g�Q�C��
			if( flag&BF_SKILL && skill_get_pl(skill_num) == ELE_WIND )
				damage += damage * sc->data[SC_VIOLENTGALE].val4 / 100;
			else if( !(flag&BF_SKILL) && status_get_attack_element(bl) == ELE_WIND )
				damage += damage * sc->data[SC_VIOLENTGALE].val4 / 100;
		}
		if(sc->data[SC_DELUGE].timer != -1 && damage > 0) {	// �f�����[�W
			if( flag&BF_SKILL && skill_get_pl(skill_num) == ELE_WATER )
				damage += damage * sc->data[SC_DELUGE].val4 / 100;
			else if( !(flag&BF_SKILL) && status_get_attack_element(bl) == ELE_WATER )
				damage += damage * sc->data[SC_DELUGE].val4 / 100;
		}

		// �G�i�W�[�R�[�g
		if(sc->data[SC_ENERGYCOAT].timer != -1 && damage > 0 && flag&BF_WEAPON && skill_num != CR_ACIDDEMONSTRATION) {
			if(tsd) {
				if(tsd->status.sp > 0) {
					int per = tsd->status.sp * 5 / (tsd->status.max_sp + 1);
					tsd->status.sp -= tsd->status.sp * (per * 5 + 10) / 1000;
					if(tsd->status.sp < 0)
						tsd->status.sp = 0;
					damage -= damage * ((per + 1) * 6) / 100;
					clif_updatestatus(tsd,SP_SP);
				}
				if(tsd->status.sp <= 0)
					status_change_end(bl,SC_ENERGYCOAT,-1);
			} else {
				damage -= damage * (sc->data[SC_ENERGYCOAT].val1 * 6) / 100;
			}
		}

		// �L���G�G���C�\��
		if(sc->data[SC_KYRIE].timer != -1 && damage > 0) {
			struct status_change_data *scd = &sc->data[SC_KYRIE];
			scd->val2 -= damage;
			if(flag&BF_WEAPON) {
				if(scd->val2 >= 0)
					damage = 0;
				else
					damage = -scd->val2;
			}
			if(--scd->val3 <= 0 || scd->val2 <= 0 || skill_num == AL_HOLYLIGHT)
				status_change_end(bl, SC_KYRIE, -1);
		}

		// �C���f���A
		if(sc->data[SC_ENDURE].timer != -1 && damage > 0 && flag&BF_WEAPON && src->type != BL_PC) {
			if((--sc->data[SC_ENDURE].val2) <= 0)
				status_change_end(bl, SC_ENDURE, -1);
		}

		// �I�[�g�K�[�h
		if(sc->data[SC_AUTOGUARD].timer != -1 && damage > 0 && flag&BF_WEAPON && skill_num != WS_CARTTERMINATION) {
			if(atn_rand()%100 < sc->data[SC_AUTOGUARD].val2) {
				int delay;
				damage = 0;
				clif_skill_nodamage(bl,bl,CR_AUTOGUARD,sc->data[SC_AUTOGUARD].val1,1);
				if(sc->data[SC_AUTOGUARD].val1 <= 5)
					delay = 300;
				else if (sc->data[SC_AUTOGUARD].val1 > 5 && sc->data[SC_AUTOGUARD].val1 <= 9)
					delay = 200;
				else
					delay = 100;
				if(ud) {
					ud->canmove_tick = tick + delay;
					if(sc->data[SC_SHRINK].timer != -1 && atn_rand()%100 < 5*sc->data[SC_AUTOGUARD].val1)
						skill_blown(bl,src,2);
				}
			}
		}

		// �p���C���O
		if(sc->data[SC_PARRYING].timer != -1 && damage > 0 && flag&BF_WEAPON && skill_num != WS_CARTTERMINATION) {
			if(atn_rand()%100 < sc->data[SC_PARRYING].val2)
			{
				int dir = map_calc_dir(bl,src->x,src->y);
				damage = 0;
				clif_skill_nodamage(bl,bl,sc->data[SC_PARRYING].val4,sc->data[SC_PARRYING].val1,1);	// val4�̓X�L��ID
				clif_changedir(bl,dir,dir);
				if(ud)
					ud->attackabletime = tick + 500;	// �l�K��
			}
		}

		// ���W�F�N�g�\�[�h
		if(sc->data[SC_REJECTSWORD].timer != -1 && damage > 0 && flag&BF_WEAPON && atn_rand()%100 < 15*sc->data[SC_REJECTSWORD].val1) {
			short weapon = -1;
			if(src->type == BL_PC)
				weapon = ((struct map_session_data *)src)->status.weapon;
			if(src->type == BL_MOB || weapon == WT_DAGGER || weapon == WT_1HSWORD || weapon == WT_2HSWORD) {
				damage = damage*50/100;
				battle_damage(bl,src,damage,ST_REJECTSWORD,sc->data[SC_REJECTSWORD].val1,0);
				clif_skill_nodamage(bl,bl,ST_REJECTSWORD,sc->data[SC_REJECTSWORD].val1,1);
				if((--sc->data[SC_REJECTSWORD].val2) <= 0)
					status_change_end(bl, SC_REJECTSWORD, -1);
			}
		}

		// �X�p�[�_�[�E�F�u
		if(sc->data[SC_SPIDERWEB].timer != -1 && damage > 0) {
			if( (flag&BF_SKILL && skill_get_pl(skill_num) == ELE_FIRE) ||
			    (!(flag&BF_SKILL) && status_get_attack_element(src) == ELE_FIRE) )
			{
				damage <<= 1;
				status_change_end(bl, SC_SPIDERWEB, -1);
			}
		}
	}

	if(damage > 0) {
		struct guild_castle *gc = NULL;
		int noflag = 0;

		// �G���y���E��
		if(status_get_class(bl) == 1288) {
			if(flag&BF_SKILL && skill_num != HW_GRAVITATION)
				return 0;
			if(src->type == BL_PC) {
				struct guild *g = guild_search(((struct map_session_data *)src)->status.guild_id);

				if(g == NULL)
					return 0;		// �M���h�������Ȃ�_���[�W����
				if(guild_checkskill(g,GD_APPROVAL) <= 0)
					return 0;		// ���K�M���h���F���Ȃ��ƃ_���[�W����
				if((gc = guild_mapname2gc(map[bl->m].name)) != NULL) {
					if(g->guild_id == gc->guild_id)
						return 0;	// ����̃M���h�̃G���y�Ȃ�_���[�W����
					if(guild_check_alliance(gc->guild_id, g->guild_id, 0))
						return 0;	// �����Ȃ�_���[�W����
				} else {
					noflag = 1;
				}
			} else {
				return 0;
			}
		}

		// GvG
		if(map[bl->m].flag.gvg && skill_num != PA_PRESSURE && skill_num != HW_GRAVITATION) {
			if(bl->type == BL_MOB) {
				if(gc == NULL && !noflag)	// �G���y���E���̍��Ŋ��Ɍ�������NULL�Ȃ�ēx�������Ȃ�
					gc = guild_mapname2gc(map[bl->m].name);
				if(gc) {
					// defense������΃_���[�W������炵���H
					damage -= damage * gc->defense / 100 * battle_config.castle_defense_rate / 100;
				}
			}
			if(flag&BF_WEAPON) {
				if(flag&BF_SHORT)
					damage = damage * battle_config.gvg_short_damage_rate / 100;
				if(flag&BF_LONG)
					damage = damage * battle_config.gvg_long_damage_rate / 100;
			}
			if(flag&BF_MAGIC)
				damage = damage * battle_config.gvg_magic_damage_rate / 100;
			if(flag&BF_MISC)
				damage = damage * battle_config.gvg_misc_damage_rate / 100;
			if(damage < 1)
				damage = (!battle_config.skill_min_damage && flag&BF_MAGIC && src->type == BL_PC)? 0: 1;
		}

		// PK
		if(map[bl->m].flag.pk && bl->type == BL_PC && skill_num != PA_PRESSURE && skill_num != HW_GRAVITATION) {
			if(flag&BF_WEAPON) {
				if(flag&BF_SHORT)
					damage = damage * battle_config.pk_short_damage_rate / 100;
				if(flag&BF_LONG)
					damage = damage * battle_config.pk_long_damage_rate / 100;
			}
			if(flag&BF_MAGIC)
				damage = damage * battle_config.pk_magic_damage_rate / 100;
			if(flag&BF_MISC)
				damage = damage * battle_config.pk_misc_damage_rate / 100;
			if(damage < 1)
				damage = (!battle_config.skill_min_damage && flag&BF_MAGIC && src->type == BL_PC)? 0: 1;
		}
	}

	if((battle_config.skill_min_damage || flag&BF_MISC) && damage > 0) {
		if(div_ == 255) {
			if(damage < 3)
				damage = 3;
		} else {
			if(damage < div_)
				damage = div_;
		}
	}

	if(tmd && tmd->hp > 0 && damage > 0)	// �����Ȃǂ�MOB�X�L������
	{
		int mtg = tmd->target_id;
		if (battle_config.mob_changetarget_byskill != 0 || mtg == 0)
		{
			if(src->type == BL_PC || src->type == BL_HOM || src->type == BL_MERC)
				tmd->target_id = src->id;
		}
		mobskill_event(tmd,flag);
		tmd->target_id = mtg;	// �^�[�Q�b�g��߂�
	}

	// PC�̔����I�[�g�X�y��
	if(tsd && src != &tsd->bl && !unit_isdead(src) && tsd->status.hp > 0 && damage > 0)
	{
		unsigned long asflag = EAS_REVENGE;

		if(skill_num == AM_DEMONSTRATION)
			flag = (flag&~BF_WEAPONMASK)|BF_MISC;

		if(flag&BF_WEAPON) {
			if(flag&BF_SKILL) {
				if(battle_config.weapon_attack_autospell)
					asflag += EAS_NORMAL;
				else
					asflag += EAS_SKILL;
			} else {
				asflag += EAS_NORMAL;
			}
			if(flag&BF_SHORT)
				asflag += EAS_SHORT;
			if(flag&BF_LONG)
				asflag += EAS_LONG;
		}
		if(flag&BF_MAGIC) {
			if(battle_config.magic_attack_autospell)
				asflag += EAS_SHORT|EAS_LONG;
			else
				asflag += EAS_MAGIC;
		}
		if(flag&BF_MISC) {
			if(battle_config.misc_attack_autospell)
				asflag += EAS_SHORT|EAS_LONG;
			else
				asflag += EAS_MISC;
		}
		skill_bonus_autospell(&tsd->bl,src,asflag,tick,0);
	}

	// PC�̏�Ԉُ픽��
	if(tsd && src != &tsd->bl && tsd->addreveff_flag && !unit_isdead(src) && tsd->status.hp > 0 && damage > 0 && flag&BF_WEAPON)
	{
		int i, rate, luk;
		int sc_def_card = 100;
		int sc_def_mdef, sc_def_vit, sc_def_int, sc_def_luk;
		const int sc2[] = {
			MG_STONECURSE,MG_FROSTDIVER,NPC_STUNATTACK,
			NPC_SLEEPATTACK,TF_POISON,NPC_CURSEATTACK,
			NPC_SILENCEATTACK,0,NPC_BLINDATTACK,LK_HEADCRUSH
		};

		// �Ώۂ̑ϐ�
		luk = status_get_luk(src);
		sc_def_mdef = 50 - (3 + status_get_mdef(src) + luk/3);
		sc_def_vit  = 50 - (3 + status_get_vit(src) + luk/3);
		sc_def_int  = 50 - (3 + status_get_int(src) + luk/3);
		sc_def_luk  = 50 - (3 + luk);

		/*
		if(target->bl.type == BL_MOB) {
			if(sc_def_mdef < 50)
				sc_def_mdef = 50;
			if(sc_def_vit < 50)
				sc_def_vit = 50;
			if(sc_def_int < 50)
				sc_def_int = 50;
			if(sc_def_luk < 50)
				sc_def_luk = 50;
		}
		*/

		if(sc_def_mdef < 0)
			sc_def_mdef = 0;
		if(sc_def_vit < 0)
			sc_def_vit = 0;
		if(sc_def_int < 0)
			sc_def_int = 0;

		for(i = SC_STONE; i <= SC_BLEED; i++) {
			// �Ώۂɏ�Ԉُ�
			if(i == SC_STONE || i == SC_FREEZE)
				sc_def_card = sc_def_mdef;
			else if(i == SC_STUN || i == SC_POISON || i == SC_SILENCE || i == SC_BLEED)
				sc_def_card = sc_def_vit;
			else if(i == SC_SLEEP || i == SC_CONFUSION || i == SC_BLIND)
				sc_def_card = sc_def_int;
			else if(i == SC_CURSE)
				sc_def_card = sc_def_luk;

			if(battle_config.reveff_plus_addeff)
				rate = (tsd->addreveff[i-SC_STONE] + tsd->addeff[i-SC_STONE] + tsd->arrow_addeff[i-SC_STONE])*sc_def_card/100;
			else
				rate = (tsd->addreveff[i-SC_STONE])*sc_def_card/100;

			if(src->type == BL_PC || src->type == BL_MOB || src->type == BL_HOM || src->type == BL_MERC)
			{
				if(atn_rand()%10000 < rate) {
					if(battle_config.battle_log)
						printf("PC %d skill_addreveff: card�ɂ��ُ픭�� %d %d\n",tsd->bl.id,i,tsd->addreveff[i-SC_STONE]);
					status_change_start(src,i,7,0,0,0,(i == SC_CONFUSION)? 10000+7000: skill_get_time2(sc2[i-SC_STONE],7),0);
				}
			}
		}
	}

	return damage;
}

/*==========================================
 * HP/SP�́��z���v�Z
 *------------------------------------------
 */
static int battle_calc_drain_per(int damage, int rate, int per)
{
	int diff = 0;

	if (damage <= 0 || rate <= 0)
		return 0;

	if (per && atn_rand()%100 < rate) {
		diff = damage * per / 100;
		if (diff == 0)
			diff = (per > 0)? 1: -1;
	}

	return diff;
}

/*==========================================
 * HP/SP�̈��z���v�Z
 *------------------------------------------
 */
static int battle_calc_drain_value(int damage, int rate, int value)
{
	int diff = 0;

	if (damage <= 0 || rate <= 0)
		return 0;

	if (value && atn_rand()%100 < rate) {
		diff = value;
	}

	return diff;
}

/*==========================================
 * �U���ɂ��HP/SP�z��
 *------------------------------------------
 */
static int battle_attack_drain(struct block_list *bl,int damage,int damage2,int flag)
{
	int hp = 0,sp = 0;
	struct map_session_data* sd = NULL;

	nullpo_retr(0, bl);

	if(damage <= 0 && damage2 <= 0)
		return 0;

	if(bl->type != BL_PC || (sd = (struct map_session_data *)bl) == NULL)
		return 0;

	if(flag & 1) {	// ���z��
		if(!battle_config.left_cardfix_to_right) {
			// �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ����Ȃ��ꍇ
			hp += battle_calc_drain_per(damage,  sd->hp_drain.p_rate,  sd->hp_drain.per );
			hp += battle_calc_drain_per(damage2, sd->hp_drain_.p_rate, sd->hp_drain_.per);
			sp += battle_calc_drain_per(damage,  sd->sp_drain.p_rate,  sd->sp_drain.per );
			sp += battle_calc_drain_per(damage2, sd->sp_drain_.p_rate, sd->sp_drain_.per);
		} else {
			// �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ�����ꍇ
			int hp_rate = sd->hp_drain.p_rate + sd->hp_drain_.p_rate;
			int hp_per  = sd->hp_drain.per + sd->hp_drain_.per;
			int sp_rate = sd->sp_drain.p_rate + sd->sp_drain_.p_rate;
			int sp_per  = sd->sp_drain.per + sd->sp_drain_.per;
			hp += battle_calc_drain_per(damage, hp_rate, hp_per);
			sp += battle_calc_drain_per(damage, sp_rate, sp_per);
		}
	}
	if(flag & 2) {	// ���z��
		if(!battle_config.left_cardfix_to_right) {
			// �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ����Ȃ��ꍇ
			hp += battle_calc_drain_value(damage,  sd->hp_drain.v_rate,  sd->hp_drain.value );
			hp += battle_calc_drain_value(damage2, sd->hp_drain_.v_rate, sd->hp_drain_.value);
			sp += battle_calc_drain_value(damage,  sd->sp_drain.v_rate,  sd->sp_drain.value );
			sp += battle_calc_drain_value(damage2, sd->sp_drain_.v_rate, sd->sp_drain_.value);
		} else {
			// �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ�����ꍇ
			int hp_rate  = sd->hp_drain.v_rate + sd->hp_drain_.v_rate;
			int hp_value = sd->hp_drain.value + sd->hp_drain_.value;
			int sp_rate  = sd->sp_drain.v_rate + sd->sp_drain_.v_rate;
			int sp_value = sd->sp_drain.value + sd->sp_drain_.value;
			hp += battle_calc_drain_value(damage, hp_rate, hp_value);
			sp += battle_calc_drain_value(damage, sp_rate, sp_value);
		}
	}
	if(hp || sp)
		pc_heal(sd, hp, sp);

	return 1;
}

/*==========================================
 * �C���_���[�W
 *------------------------------------------
 */
static int battle_addmastery(struct map_session_data *sd,struct block_list *target,int dmg,int type)
{
	int damage = 0, race, skill, weapon;

	nullpo_retr(0, sd);
	nullpo_retr(0, target);

	race = status_get_race(target);

	// �f�[�����x�C�� vs �s�� or ���� (���l�͊܂߂Ȃ��H)
	if((skill = pc_checkskill(sd,AL_DEMONBANE)) > 0 && (battle_check_undead(race,status_get_elem_type(target)) || race == RCT_DEMON) ) {
		damage += (300 + 5 * sd->status.base_level) * skill / 100;
	}

	// �r�[�X�g�x�C��(+4 �` +40) vs ���� or ����
	if((skill = pc_checkskill(sd,HT_BEASTBANE)) > 0 && (race == RCT_BRUTE || race == RCT_INSECT) )
	{
		damage += (skill * 4);

		if(sd->sc.data[SC_HUNTER].timer != -1)
			damage += status_get_str(&sd->bl);
	}
	weapon = (type == 0)? sd->weapontype1: sd->weapontype2;

	switch(weapon)
	{
		case WT_DAGGER:
		case WT_1HSWORD:
			// ���C��(+4 �` +40) �Ў茕 �Z���܂�
			if((skill = pc_checkskill(sd,SM_SWORD)) > 0) {
				damage += (skill * 4);
			}
			break;
		case WT_2HSWORD:
			// ���茕�C��(+4 �` +40) ���茕
			if((skill = pc_checkskill(sd,SM_TWOHAND)) > 0) {
				damage += (skill * 4);
			}
			break;
		case WT_1HSPEAR:
			// ���C��(+4 �` +40,+5 �` +50) ��
			if((skill = pc_checkskill(sd,KN_SPEARMASTERY)) > 0) {
				if(!pc_isriding(sd))
					damage += (skill * 4);	// �y�R�ɏ���ĂȂ�
				else
					damage += (skill * 5);	// �y�R�ɏ���Ă�
			}
			break;
		case WT_2HSPEAR:
			// ���C��(+4 �` +40,+5 �` +50) ��
			if((skill = pc_checkskill(sd,KN_SPEARMASTERY)) > 0) {
				if(!pc_isriding(sd))
					damage += (skill * 4);	// �y�R�ɏ���ĂȂ�
				else
					damage += (skill * 5);	// �y�R�ɏ���Ă�
			}
			break;
		case WT_1HAXE:
			if((skill = pc_checkskill(sd,AM_AXEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		case WT_2HAXE:
			if((skill = pc_checkskill(sd,AM_AXEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		case WT_MACE:
			// ���C�X�C��(+3 �` +30) ���C�X
			if((skill = pc_checkskill(sd,PR_MACEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		case WT_2HMACE:
		case WT_STAFF:
		case WT_BOW:
			break;
		case WT_FIST:
			// �^�C���M(+10 �` +100) �f��
			if((skill = pc_checkskill(sd,TK_RUN)) > 0) {
				damage += (skill * 10);
			}
			// fall through
		case WT_KNUCKLE:
			// �S��(+3 �` +30) �f��,�i�b�N��
			if((skill = pc_checkskill(sd,MO_IRONHAND)) > 0) {
				damage += (skill * 3);
			}
			break;
		case WT_MUSICAL:
			// �y��̗��K(+3 �` +30) �y��
			if((skill = pc_checkskill(sd,BA_MUSICALLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		case WT_WHIP:
			// �_���X�̗��K(+3 �` +30) ��
			if((skill = pc_checkskill(sd,DC_DANCINGLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		case WT_BOOK:
			// �A�h�o���X�h�u�b�N(+3 �` +30) {
			if((skill = pc_checkskill(sd,SA_ADVANCEDBOOK)) > 0) {
				damage += (skill * 3);
			}
			break;
		case WT_KATAR:
			// �J�^�[���C��(+3 �` +30) �J�^�[��
			if((skill = pc_checkskill(sd,AS_KATAR)) > 0) {
				// �\�j�b�N�u���[���͕ʏ����i1���ɕt��1/8�K��)
				damage += (skill * 3);
			}
			break;
		case WT_HANDGUN:
		case WT_RIFLE:
		case WT_SHOTGUN:
		case WT_GATLING:
		case WT_GRENADE:
		case WT_HUUMA:
			break;
	}
	return dmg+damage;
}

/*==========================================
 * ��{����_���[�W�v�Z
 *------------------------------------------
 */
static int battle_calc_base_damage(struct block_list *src,struct block_list *target,int skill_num,int type,int lh)
{
	int damage = 0;
	int atkmin, atkmax;
	struct map_session_data *sd   = NULL;
	struct status_change *sc = NULL;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	sc = status_get_sc(src);
	if(src->type == BL_PC)
		sd = (struct map_session_data *)src;

	if(sd) {
		int watk   = (lh == 0)? status_get_atk(src): status_get_atk_(src);
		int dex    = status_get_dex(src);
		int t_size = status_get_size(target);
		int idx    = (lh == 0)? sd->equip_index[9]: sd->equip_index[8];

		if(skill_num == HW_MAGICCRASHER || (skill_num == 0 && sc && sc->data[SC_CHANGE].timer != -1)) {
			// �}�W�b�N�N���b�V���[�܂��̓����^���`�F���W���̒ʏ�U���Ȃ�MATK�ŉ���
			damage = status_get_matk1(src);
		} else {
			damage = status_get_baseatk(src);
		}

		atkmin = dex;	// �Œ�ATK��DEX�ŏ�����

		if(idx >= 0 && sd->inventory_data[idx])
			atkmin = atkmin * (80 + sd->inventory_data[idx]->wlv * 20) / 100;
		if(sd->state.arrow_atk)						// ���킪�|��̏ꍇ
			atkmin = watk * ((atkmin < watk)? atkmin: watk) / 100;	// �|�p�Œ�ATK�v�Z

		/* �T�C�Y�C�� */
		if(skill_num == MO_EXTREMITYFIST) {
			// ���C��
			atkmax = watk;
		} else if(pc_isriding(sd) && (sd->status.weapon == WT_1HSPEAR || sd->status.weapon == WT_2HSPEAR) && t_size == 1) {
			// �y�R�R�悵�Ă��āA���Œ��^���U�������ꍇ�̓T�C�Y�C����100�ɂ���
			atkmax = watk;
		} else {
			int rate = (lh == 0)? sd->atkmods[t_size]: sd->atkmods_[t_size];
			atkmax = (watk   * rate) / 100;
			atkmin = (atkmin * rate) / 100;
		}
		if(sc && sc->data[SC_WEAPONPERFECTION].timer != -1) {
			// �E�F�|���p�[�t�F�N�V����
			atkmax = watk;
		} else if(sd->special_state.no_sizefix) {
			// �h���C�N�J�[�h
			atkmax = watk;
		}
		if(!sd->state.arrow_atk && atkmin > atkmax)
			atkmin = atkmax;	// �|�͍ŒႪ����ꍇ����
		if(lh && atkmin > atkmax)
			atkmin = atkmax;

		/* ���z�ƌ��Ɛ��̓{�� */
		if(target->type == BL_PC || target->type == BL_MOB || target->type == BL_HOM || target->type == BL_MERC)
		{
			int atk_rate = 0;
			int str = status_get_str(src);
			int luk = status_get_luk(src);
			int tclass = status_get_class(target);

			if(sc && sc->data[SC_MIRACLE].timer != -1) {	// ���z�ƌ��Ɛ��̊��
				// �S�Ă̓G����
				atk_rate = (sd->status.base_level + dex + luk + str)/(12-3*pc_checkskill(sd,SG_STAR_ANGER));
			} else {
				if(tclass == sd->hate_mob[0] && pc_checkskill(sd,SG_SUN_ANGER) > 0)		// ���z�̓{��
					atk_rate = (sd->status.base_level + dex + luk) / (12 - 3 * pc_checkskill(sd,SG_SUN_ANGER));
				else if(tclass == sd->hate_mob[1] && pc_checkskill(sd,SG_MOON_ANGER) > 0)	// ���̓{��
					atk_rate = (sd->status.base_level + dex + luk) / (12 - 3 * pc_checkskill(sd,SG_MOON_ANGER));
				else if(tclass == sd->hate_mob[2] && pc_checkskill(sd,SG_STAR_ANGER) > 0)	// ���̓{��
					atk_rate = (sd->status.base_level + dex + luk + str) / (12 - 3 * pc_checkskill(sd,SG_STAR_ANGER));
			}
			if(atk_rate > 0) {
				atkmin += atkmin * atk_rate / 100;
				atkmax += atkmax * atk_rate / 100;
			}
		}
		/* �ߏ萸�B�{�[�i�X */
		if(!lh && sd->overrefine > 0)
			damage += (atn_rand() % sd->overrefine ) + 1;
		if(lh && sd->overrefine_ > 0)
			damage += (atn_rand() % sd->overrefine_) + 1;
	} else {
		if(battle_config.enemy_str)
			damage = status_get_baseatk(src);
		else
			damage = 0;
		if(skill_num == HW_MAGICCRASHER || (skill_num == 0 && sc && sc->data[SC_CHANGE].timer != -1)) {
			// �}�W�b�N�N���b�V���[�܂��̓����^���`�F���W���̒ʏ�U���Ȃ�MATK�ŉ���
			atkmin = status_get_matk1(src);
			atkmax = status_get_matk2(src);
		} else {
			atkmin = status_get_atk(src);
			atkmax = status_get_atk2(src);
		}
		if(atkmin > atkmax)
			atkmin = atkmax;
	}

	if(sc && sc->data[SC_MAXIMIZEPOWER].timer != -1) {
		// �}�L�V�}�C�Y�p���[
		atkmin = atkmax;
	}

	if(type == 0x0a) {
		/* �N���e�B�J���U�� */
		damage += atkmax;
		if(sd && (sd->atk_rate != 100 || sd->weapon_atk_rate != 0)) {
			damage = (damage * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon])) / 100;

			// �N���e�B�J�����_���[�W����
			damage += damage *sd->critical_damage / 100;
		}
		if(sd && sd->state.arrow_atk)
			damage += sd->arrow_atk;
	} else {
		/* �ʏ�U���E�X�L���U�� */
		if(atkmax > atkmin)
			damage += atkmin + atn_rand() % (atkmax - atkmin + 1);
		else
			damage += atkmin;
		if(sd && (sd->atk_rate != 100 || sd->weapon_atk_rate != 0)) {
			damage  = (damage * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon])) / 100;
		}
	}

	return damage;
}

// ���蔻�肪����Ƃ�����damage2���v�Z����
#define DMG_FIX( a,b ) { wd.damage = wd.damage*(a)/(b); if(calc_flag.lh) wd.damage2 = wd.damage2*(a)/(b); }
#define DMG_ADD( a )   { wd.damage += (a); if(calc_flag.lh) wd.damage2 += (a); }
#define DMG_SET( a )   { wd.damage = (a); if(calc_flag.lh) wd.damage2 = (a); }

/*==========================================
 * ����_���[�W�v�Z
 *------------------------------------------
 */
static struct Damage battle_calc_weapon_attack(struct block_list *src,struct block_list *target,int skill_num,int skill_lv,int wflag)
{
	struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct map_session_data *src_sd  = NULL, *target_sd  = NULL;
	struct mob_data         *src_md  = NULL, *target_md  = NULL;
	struct pet_data         *src_pd  = NULL;
	struct homun_data       *src_hd  = NULL, *target_hd  = NULL;
	struct merc_data        *src_mcd = NULL, *target_mcd = NULL;
	struct status_change    *sc      = NULL, *t_sc       = NULL;
	int s_ele, s_ele_, s_str;
	int t_vit, t_race, t_ele, t_enemy, t_size, t_mode, t_group, t_class;
	int t_flee, t_def1, t_def2;
	int cardfix, skill, damage_sbr = 0;
	int i;
	struct {
		int rh;			// �E��
		int lh;			// ����
		int hitrate;		// �q�b�g�m��
		int autocounter;	// �I�[�g�J�E���^�[ON
		int da;			// �A������i0�`6�j
		int idef;		// DEF����
		int idef_;		// DEf�����i����j
		int nocardfix;		// �J�[�h�␳�Ȃ�
		int dist;		// �������X�L���̌v�Z�t���O
	} calc_flag;

	memset(&calc_flag, 0, sizeof(calc_flag));

	// return�O�̏���������̂ŏ��o�͕��̂ݕύX
	if(src == NULL || target == NULL) {
		nullpo_info(NLP_MARK);
		return wd;
	}

	src_sd  = BL_DOWNCAST( BL_PC,   src );
	src_md  = BL_DOWNCAST( BL_MOB,  src );
	src_pd  = BL_DOWNCAST( BL_PET,  src );
	src_hd  = BL_DOWNCAST( BL_HOM,  src );
	src_mcd = BL_DOWNCAST( BL_MERC, src );

	target_sd  = BL_DOWNCAST( BL_PC,   target );
	target_md  = BL_DOWNCAST( BL_MOB,  target );
	target_hd  = BL_DOWNCAST( BL_HOM,  target );
	target_mcd = BL_DOWNCAST( BL_MERC, target );

	// �A�^�b�J�[
	s_ele  = status_get_attack_element(src);	// ����
	s_ele_ = status_get_attack_element2(src);	// ���葮��
	s_str  = status_get_str(src);			// STR
	sc     = status_get_sc(src);		// �X�e�[�^�X�ُ�

	// �^�[�Q�b�g
	t_vit   = status_get_vit(target);
	t_race  = status_get_race(target);		// �Ώۂ̎푰
	t_ele   = status_get_elem_type(target);	// �Ώۂ̑���
	t_enemy = status_get_enemy_type(target);	// �Ώۂ̓G�^�C�v
	t_size  = status_get_size(target);		// �Ώۂ̃T�C�Y
	t_mode  = status_get_mode(target);		// �Ώۂ�Mode
	t_group = status_get_group(target);
	t_class = status_get_class(target);
	t_flee  = status_get_flee(target);
	t_def1  = status_get_def(target);
	t_def2  = status_get_def2(target);
	t_sc    = status_get_sc(target);		// �Ώۂ̃X�e�[�^�X�ُ�

	if(src_sd && skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS)
		src_sd->state.attack_type = BF_WEAPON;	// �U���^�C�v�͕���U��

	/* �P�D�I�[�g�J�E���^�[���� */
	if(skill_lv >= 0 && (skill_num == 0 || (target_sd && battle_config.pc_auto_counter_type&2) ||
		(target_md && battle_config.monster_auto_counter_type&2))
	) {
		if( skill_num != CR_GRANDCROSS &&
		    skill_num != NPC_GRANDDARKNESS &&
		    t_sc &&
		    t_sc->data[SC_AUTOCOUNTER].timer != -1 )
		{
			// �O�����h�N���X�łȂ��A�Ώۂ��I�[�g�J�E���^�[��Ԃ̏ꍇ
			int dir   = map_calc_dir(src,target->x,target->y);
			int t_dir = status_get_dir(target);
			int dist  = unit_distance2(src,target);

			if(dist <= 0 || map_check_dir(dir,t_dir) ) {
				// �ΏۂƂ̋�����0�ȉ��A�܂��͑Ώۂ̐��ʁH
				t_sc->data[SC_AUTOCOUNTER].val3 = 0;
				t_sc->data[SC_AUTOCOUNTER].val4 = 1;
				if(sc && sc->data[SC_AUTOCOUNTER].timer == -1) {
					int range = status_get_range(target);
					// �������I�[�g�J�E���^�[���
					if( target_sd &&
						(target_sd->status.weapon != WT_BOW && !(target_sd->status.weapon >= WT_HANDGUN && target_sd->status.weapon <= WT_GRENADE))
						&& dist <= range+1)
						// �Ώۂ�PC�ŕ��킪�|��łȂ��˒���
						t_sc->data[SC_AUTOCOUNTER].val3 = src->id;
					if( target_md && range <= 3 && dist <= range+1)
						// �܂��͑Ώۂ�Mob�Ŏ˒���3�ȉ��Ŏ˒���
						t_sc->data[SC_AUTOCOUNTER].val3 = src->id;
				}
				return wd; // �_���[�W�\���̂�Ԃ��ďI��
			}
			calc_flag.autocounter = 1;
		}
	}

	/* �Q�D�������␳ */
	// ��������(!=������)
	if( (src_sd && battle_config.pc_attack_attr_none) ||
	    (src_md && battle_config.mob_attack_attr_none) ||
	    (src_pd && battle_config.pet_attack_attr_none) ||
	     src_hd ||
	     src_mcd )
	{
		if (s_ele == ELE_NEUTRAL)
			s_ele  = ELE_NONE;
		if (s_ele_ == ELE_NEUTRAL)
			s_ele_ = ELE_NONE;
	}

	calc_flag.hitrate = status_get_hit(src) - t_flee + 80;	// �������v�Z
	// ����HIT�␳
	if(skill_num == 0 && t_sc && t_sc->data[SC_FOGWALL].timer != -1)
		calc_flag.hitrate -= 50;

	/* �R�Dwd�\���̂̏����ݒ� */
	wd.type      = 0;
	wd.div_      = skill_get_num(skill_num,skill_lv);
	wd.blewcount = skill_get_blewcount(skill_num,skill_lv);
	wd.flag      = BF_SHORT | BF_WEAPON | BF_NORMAL;	// �U���̎�ނ̐ݒ�

	if(skill_num == GS_DESPERADO)
		wd.div_ = 1;
	if(wd.div_ <= 0 || wd.div_ >= 251)			// 251�`254 = �e�R���R��A255 = �O�i���Ƃ��ė\���
		wd.div_ = 1;

	if(src_sd) {
		if(src_sd->status.weapon == WT_BOW || (src_sd->status.weapon >= WT_HANDGUN && src_sd->status.weapon <= WT_GRENADE)) {	// ���킪�|��̏ꍇ
			wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;	// �������U���t���O��L��
			if(src_sd->arrow_ele > 0)	// ������Ȃ瑮�����̑����ɕύX
				s_ele = src_sd->arrow_ele;
			src_sd->state.arrow_atk = 1;	// �L����
		} else {
			src_sd->state.arrow_atk = 0;	// ������
		}
	} else if(src_md || src_pd || src_mcd) {
		if(status_get_range(src) > 3)
			wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;
	}

	/* �S�D�E��E���蔻�� */
	calc_flag.rh = 1;		// ��{�͉E��̂�
	if(src_sd && skill_num == 0) {	// �X�L���U���͏�ɉE����Q��
		if(src_sd->weapontype1 == WT_FIST && src_sd->weapontype2 > WT_FIST) {	// ����̂ݕ��푕��
			calc_flag.rh = 0;
			calc_flag.lh = 1;
		} else if(src_sd->status.weapon > WT_HUUMA || src_sd->status.weapon == WT_KATAR) {	// �񓁗�
			calc_flag.lh = 1;
		}
	}

	/* �T�D�A������ */
	if(src_sd && skill_num == 0 && skill_lv >= 0) {
		do {
			// �O�i��
			if((skill = pc_checkskill(src_sd,MO_TRIPLEATTACK)) > 0 && src_sd->status.weapon <= WT_HUUMA)
			{
				int triple_rate = 0;
				if(sc && sc->data[SC_TRIPLEATTACK_RATE_UP].timer != -1) {
					triple_rate = (30 - skill)*(150+50*sc->data[SC_TRIPLEATTACK_RATE_UP].val1)/100;
					status_change_end(src,SC_TRIPLEATTACK_RATE_UP,-1);
				} else {
					triple_rate = 30 - skill;
				}
				if(atn_rand()%100 < triple_rate) {
					calc_flag.da = 2;
					break;
				}
			}
			// �_�u���A�^�b�N
			if((skill = pc_checkskill(src_sd,TF_DOUBLE)) > 0 && src_sd->weapontype1 == WT_DAGGER && atn_rand()%100 < skill*5) {
				calc_flag.da = 1;
				calc_flag.hitrate = calc_flag.hitrate*(100+skill)/100;
				break;
			}
			// �`�F�[���A�N�V����
			if((skill = pc_checkskill(src_sd,GS_CHAINACTION)) > 0 && src_sd->weapontype1 == WT_HANDGUN && atn_rand()%100 < skill*5) {
				calc_flag.da = 1;
				break;
			}
			// �t�F�I���`���M
			if(sc && sc->data[SC_READYSTORM].timer != -1 && pc_checkskill(src_sd,TK_STORMKICK) > 0 && atn_rand()%100 < 15) {
				calc_flag.da = 3;
				break;
			}
			// �l�����`���M
			if(sc && sc->data[SC_READYDOWN].timer != -1 && pc_checkskill(src_sd,TK_DOWNKICK) > 0 && atn_rand()%100 < 15) {
				calc_flag.da = 4;
				break;
			}
			// �g�������`���M
			if(sc && sc->data[SC_READYTURN].timer != -1 &&  pc_checkskill(src_sd,TK_TURNKICK) > 0 && atn_rand()%100 < 15) {
				calc_flag.da = 5;
				break;
			}
			// �A�v�`���I�����M
			if(sc && sc->data[SC_READYCOUNTER].timer != -1 && pc_checkskill(src_sd,TK_COUNTER) > 0)
			{
				int counter_rate = 0;
				if(sc->data[SC_COUNTER_RATE_UP].timer != -1 && (skill = pc_checkskill(src_sd,SG_FRIEND)) > 0) {
					counter_rate = 30+10*skill;
					status_change_end(src,SC_COUNTER_RATE_UP,-1);
				} else {
					counter_rate = 20;
				}
				if(atn_rand()%100 < counter_rate) {
					calc_flag.da = 6;
					break;
				}
			}
			// �T�C�h���C���_�[��
			if(src_sd->double_rate > 0 && atn_rand()%100 < src_sd->double_rate) {
				calc_flag.da = 1;
				break;
			}
		} while(0);
	}

	/* �U�D�N���e�B�J���v�Z */
	if( calc_flag.da == 0 &&
	    (skill_num == 0 || skill_num == KN_AUTOCOUNTER || skill_num == SN_SHARPSHOOTING || skill_num == NJ_KIRIKAGE || skill_num == MA_SHARPSHOOTING) &&
	    (!src_md || battle_config.enemy_critical) &&
	    skill_lv >= 0 )
	{
		// �A�����������ĂȂ��āA�ʏ�U���E�I�[�g�J�E���^�[�E�V���[�v�V���[�e�B���O�E�e�a��Ȃ��
		int cri = status_get_critical(src);
		if(src_sd) {
			cri += src_sd->critical_race[t_race];
			if(src_sd->state.arrow_atk)
				cri += src_sd->arrow_cri;
			if(src_sd->status.weapon == WT_KATAR)
				cri <<= 1;	// �J�^�[���̏ꍇ�A�N���e�B�J����{��
		}
		cri -= status_get_luk(target) * 3;
		if(src_md && battle_config.enemy_critical_rate != 100) {
			cri = cri * battle_config.enemy_critical_rate / 100;
			if(cri < 1) cri = 1;
		}
		if(t_sc && t_sc->data[SC_SLEEP].timer != -1)
			cri <<= 1;		// �������̓N���e�B�J�����{��
		if(calc_flag.autocounter)
			cri = 1000;

		if(skill_num == KN_AUTOCOUNTER) {
			if(!(battle_config.pc_auto_counter_type&1))
				cri = 1000;
			else
				cri <<= 1;
		} else if(skill_num == SN_SHARPSHOOTING || skill_num == MA_SHARPSHOOTING) {
			cri += 200;
		} else if(skill_num == NJ_KIRIKAGE) {
			cri += (250+skill_lv*50);
		}

		if(target_sd && target_sd->critical_def) {
			if(target_sd->critical_def > 100)
				cri = 0;
			else
				cri = cri * (100 - target_sd->critical_def) / 100;
		}

		// �m������
		if(atn_rand() % 1000 < cri) {
			if(skill_num == SN_SHARPSHOOTING || skill_num == NJ_KIRIKAGE || skill_num == MA_SHARPSHOOTING) {
				// DEF�����t���O
				calc_flag.idef = calc_flag.idef_ = 1;
			} else {
				wd.type = 0x0a;	// �N���e�B�J���U��
			}
		}
	}

	/* �V�D�q�b�g�E�����E�����W�C�� */
	if(wd.type == 0) {
		// �����Ȃ�q�b�g�����Z
		if(src_sd && src_sd->state.arrow_atk) {
			calc_flag.hitrate += src_sd->arrow_hit;
		}
		if(skill_num > 0) {
			wd.flag = (wd.flag&~BF_SKILLMASK)|BF_SKILL;
			if( (i = skill_get_pl(skill_num)) > 0 && (!src_sd || !src_sd->arrow_ele) )
				s_ele = s_ele_ = i;
		}
		switch( skill_num ) {
		case SM_BASH:			// �o�b�V��
		case MS_BASH:
		case KN_PIERCE:			// �s�A�[�X
		case ML_PIERCE:
			calc_flag.hitrate = calc_flag.hitrate*(100+5*skill_lv)/100;
			break;
		case SM_MAGNUM:			// �}�O�i���u���C�N
		case MS_MAGNUM:
			calc_flag.hitrate = calc_flag.hitrate*(100+10*skill_lv)/100;
			break;
		case KN_AUTOCOUNTER:		// �I�[�g�J�E���^�[
			wd.flag = (wd.flag&~BF_SKILLMASK)|BF_NORMAL;
			if(battle_config.pc_auto_counter_type&1)
				calc_flag.hitrate += 20;
			else
				calc_flag.hitrate = 1000000;
			break;
		case AS_SONICBLOW:		// �\�j�b�N�u���E
			if(src_sd && pc_checkskill(src_sd,AS_SONICACCEL) > 0)
				calc_flag.hitrate = calc_flag.hitrate*150/100;
			break;
		case CR_SHIELDBOOMERANG:	// �V�[���h�u�[������
			if(sc && sc->data[SC_CRUSADER].timer != -1)
				calc_flag.hitrate = 1000000;
			calc_flag.dist = 1;
			s_ele = s_ele_ = ELE_NEUTRAL;
			break;
		case AM_ACIDTERROR:		// �A�V�b�h�e���[
			calc_flag.hitrate = 1000000;
			calc_flag.dist = 1;
			s_ele = s_ele_ = ELE_NEUTRAL;
			break;
		case NPC_CRITICALSLASH:		// �h�䖳���U��
		case NPC_GUIDEDATTACK:		// �K���U��
		case MO_INVESTIGATE:		// ����
		case MO_EXTREMITYFIST:		// ���C���e�P��
		case CR_ACIDDEMONSTRATION:	// �A�V�b�h�f�����X�g���[�V����
		case NJ_ISSEN:			// ��M
			calc_flag.hitrate = 1000000;
			s_ele = s_ele_ = ELE_NEUTRAL;
			break;
		case HVAN_EXPLOSION:		// �o�C�I�G�N�X�v���[�W����
		case RG_BACKSTAP:		// �o�b�N�X�^�u
		case CR_GRANDCROSS:		// �O�����h�N���X
		case NPC_GRANDDARKNESS:		// �O�����h�_�[�N�l�X
		case AM_DEMONSTRATION:		// �f�����X�g���[�V����
		case TK_COUNTER:		// �A�v�`���I�����M
		case AS_SPLASHER:		// �x�i���X�v���b�V���[
		case NPC_EARTHQUAKE:		// �A�[�X�N�G�C�N
			calc_flag.hitrate = 1000000;
			break;
		case NPC_EXPULSION:		// �G�N�X�p���V�I��
			calc_flag.hitrate = 1000000;
			calc_flag.dist = 1;
			break;
		case GS_TRACKING:		// �g���b�L���O
			calc_flag.hitrate = calc_flag.hitrate*4+5;
			calc_flag.dist = 1;
			break;
		case AC_DOUBLE:			// �_�u���X�g���C�t�B���O
		case HT_POWER:			// �r�[�X�g�X�g���C�t�B���O
		case AC_SHOWER:			// �A���[�V�����[
		case AC_CHARGEARROW:		// �`���[�W�A���[
		case HT_PHANTASMIC:		// �t�@���^�X�~�b�N�A���[
		case KN_SPEARSTAB:		// �X�s�A�X�^�u
		case KN_SPEARBOOMERANG:		// �X�s�A�u�[������
		case AS_GRIMTOOTH:		// �O�����g�D�[�X
		case MO_FINGEROFFENSIVE:	// �w�e
		case LK_SPIRALPIERCE:		// �X�p�C�����s�A�[�X
		case HW_MAGICCRASHER:		// �}�W�b�N�N���b�V���[
		case ASC_BREAKER:		// �\�E���u���C�J�[
		case SN_SHARPSHOOTING:		// �V���[�v�V���[�e�B���O
		case ITM_TOMAHAWK:		// �g�}�z�[�N����
		case GS_FLING:			// �t���C���O
		case GS_TRIPLEACTION:		// �g���v���A�N�V����
		case GS_BULLSEYE:		// �u���Y�A�C
		case GS_MAGICALBULLET:		// �}�W�J���o���b�g
		case GS_DISARM:			// �f�B�X�A�[��
		case GS_PIERCINGSHOT:		// �s�A�[�V���O�V���b�g
		case GS_RAPIDSHOWER:		// ���s�b�h�V�����[
		case GS_DUST:			// �_�X�g
		case GS_FULLBUSTER:		// �t���o�X�^�[
		case GS_SPREADATTACK:		// �X�v���b�h�A�^�b�N
		case NJ_HUUMA:			// �����藠������
		case NJ_TATAMIGAESHI:		// ���Ԃ�
		case MA_DOUBLE:
		case MA_SHOWER:
		case MA_CHARGEARROW:
		case MA_SHARPSHOOTING:
		case ML_SPIRALPIERCE:
			calc_flag.dist = 1;
			break;
		case AS_VENOMKNIFE:		// �x�i���i�C�t
			calc_flag.dist = 1;
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				if(src_sd->arrow_ele > 0)	// ������Ȃ瑮�����̑����ɕύX
					s_ele = src_sd->arrow_ele;
			}
			break;
		case NPC_COMBOATTACK:		// ���i�U��
		case NPC_RANDOMATTACK:		// �����_��ATK�U��
			s_ele = s_ele_ = ELE_NEUTRAL;
			break;
		case NPC_RANGEATTACK:		// �������U��
		case NJ_ZENYNAGE:		// �K����
		case NPC_CRITICALWOUND:		// �v�����U��
			calc_flag.dist = 1;
			s_ele = s_ele_ = ELE_NEUTRAL;
			break;
		case PA_SHIELDCHAIN:		// �V�[���h�`�F�C��
			calc_flag.hitrate += 20;
			calc_flag.dist = 1;
			s_ele = s_ele_ = ELE_NEUTRAL;
			break;
		case NPC_PIERCINGATT:		// �˂��h���U��
		case CR_SHIELDCHARGE:		// �V�[���h�`���[�W
			wd.flag = (wd.flag&~BF_RANGEMASK)|BF_SHORT;
			s_ele = s_ele_ = ELE_NEUTRAL;
			break;
		case BA_MUSICALSTRIKE:		// �~���[�W�J���X�g���C�N
		case DC_THROWARROW:		// ���
		case CG_ARROWVULCAN:		// �A���[�o���J��
			calc_flag.dist = 1;
			if(src_sd)
				s_ele = src_sd->arrow_ele;
			break;
		case NJ_SYURIKEN:		// �藠������
		case NJ_KUNAI:			// �ꖳ����
			calc_flag.dist = 1;
			if(src_sd && src_sd->arrow_ele > 0)	// ������Ȃ瑮�����̑����ɕύX
				s_ele = src_sd->arrow_ele;
			break;
		}

		// �������狗���ɂ�锻��
		if(calc_flag.dist) {				// �����ɂ���ă����W���ω�����X�L����
			if(battle_config.calc_dist_flag&1 && src->type != BL_PC && target->type != BL_PC) {	// PC vs PC�͋�������
				int target_dist = unit_distance2(src,target)-1;	// �������擾
				if(target_dist >= battle_config.allow_sw_dist) {				// SW�Ŗh���鋗����葽��������������̍U��
					if(src->type == BL_PC && battle_config.sw_def_type & 1)		// �l�Ԃ���̂𔻒肷�邩�@&1�ł���
						wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;		// �������ɐݒ�
					else if(src->type == BL_MOB && battle_config.sw_def_type & 2)	// �����X�^�[����̂𔻒肷�邩�@&2�ł���
						wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;		// �������ɐݒ�
				}
			} else {	// �{���������̃X�L���Ŏg�p�҂Ƌ��t���O���S�Ĉ�v���Ȃ����牓�����U����
				wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;	// �������ɐݒ�
			}
		}
	}
	// �T�N���t�@�C�X
	if(sc && sc->data[SC_SACRIFICE].timer != -1 && !skill_num && t_class != 1288) {
		calc_flag.hitrate = 1000000;
		s_ele = s_ele_ = ELE_NEUTRAL;
	}
	// �J�[�h���ʂɂ��K���{�[�i�X
	if(src_sd && src_sd->perfect_hit > 0) {
		if(atn_rand()%100 < src_sd->perfect_hit)
			calc_flag.hitrate = 1000000;
	}
	// �Ώۂ���Ԉُ풆�̏ꍇ�̕K���{�[�i�X
	if(calc_flag.hitrate < 1000000 && t_sc) {
		if( t_sc->data[SC_SLEEP].timer != -1 ||
		    t_sc->data[SC_STUN].timer != -1 ||
		    t_sc->data[SC_FREEZE].timer != -1 ||
		    (t_sc->data[SC_STONE].timer != -1 && t_sc->data[SC_STONE].val2 == 0) ) {
			calc_flag.hitrate = 1000000;
		}
	}
	if(calc_flag.hitrate < battle_config.min_hitrate)
		calc_flag.hitrate = battle_config.min_hitrate;

	/* �W�D��𔻒� */
	if(wd.type == 0 && atn_rand()%100 >= calc_flag.hitrate) {
		wd.dmg_lv = ATK_FLEE;
	}
	else if(wd.type == 0 && t_sc && t_sc->data[SC_UTSUSEMI].timer != -1) {	// ���
		wd.dmg_lv = ATK_FLEE;
		clif_misceffect2(target,463);
		if(--t_sc->data[SC_UTSUSEMI].val3 == 0)
			status_change_end(target,SC_UTSUSEMI,-1);
		if(t_sc->data[SC_ANKLE].timer == -1) {
			int dir = 0, head_dir = 0;
			int count = skill_get_blewcount(NJ_UTSUSEMI,t_sc->data[SC_UTSUSEMI].val1);
			if(target_sd) {
				dir = target_sd->dir;
				head_dir = target_sd->head_dir;
			}
			unit_stop_walking(target,1);
			skill_blown(src,target,count|SAB_NODAMAGE|SAB_NOPATHSTOP);
			if(target_sd)
				pc_setdir(target_sd, dir, head_dir);
			if(t_sc->data[SC_CLOSECONFINE].timer != -1)
				status_change_end(target,SC_CLOSECONFINE,-1);
		}
	}
	else if(wd.type == 0 && t_sc && t_sc->data[SC_BUNSINJYUTSU].timer != -1) {	// �e���g
		wd.dmg_lv = ATK_FLEE;
		if(--t_sc->data[SC_BUNSINJYUTSU].val3 == 0)
			status_change_end(target,SC_BUNSINJYUTSU,-1);
	}
	else if(target_sd && t_sc && t_sc->data[SC_DODGE].timer != -1 && (wd.flag&BF_LONG || t_sc->data[SC_SPURT].timer != -1) && atn_rand()%100 < 20) {	// ���@
		int slv = pc_checkskill(target_sd,TK_DODGE);
		wd.dmg_lv = ATK_FLEE;
		clif_skill_nodamage(&target_sd->bl,&target_sd->bl,TK_DODGE,slv,1);
		status_change_start(&target_sd->bl,SC_DODGE_DELAY,slv,src->id,0,0,skill_get_time(TK_DODGE,slv),0);
	}
	else {
		int damage_ot = 0, damage_ot2 = 0;
		int tk_power_damage = 0, tk_power_damage2 = 0;

		// ����ł��Ȃ������Ƃ��̂�step9�`18�̃_���[�W�v�Z���s��
		wd.dmg_lv = ATK_DEF;

		/* �X�D��{�_���[�W�̎Z�o */
		wd.damage = battle_calc_base_damage(src, target, skill_num, wd.type, 0);
		if(calc_flag.lh)
			wd.damage2 = battle_calc_base_damage(src, target, skill_num, wd.type, 1);

		if(wd.type == 0) {	// �N���e�B�J���łȂ��Ƃ���̃_���[�W�����Z
			if(src_sd && src_sd->state.arrow_atk && src_sd->arrow_atk > 0)
				wd.damage += atn_rand()%(src_sd->arrow_atk+1);
		}

		/* 10�D�t�@�C�e�B���O�v�Z */
		if(src_sd && (skill = pc_checkskill(src_sd,TK_POWER) > 0) && src_sd->status.party_id > 0)
		{
			int member_num = party_check_same_map_member_count(src_sd);
			if(member_num > 0)
			{
				tk_power_damage = wd.damage*member_num * skill/50;
				if(calc_flag.lh)
					tk_power_damage2 = wd.damage2*member_num * skill/50;
			}
		}

		/* 11�D�I�[�o�[�g���X�g�n�̃X�L���{���v�Z�O�̍U���͊m�� */
		damage_ot += wd.damage;
		if(calc_flag.lh)
			damage_ot2 += wd.damage2;

		/* 12�D�X�L���C���P�i�U���͔{���n�j*/
		switch( skill_num ) {
		case SM_BASH:		// �o�b�V��
		case MS_BASH:
			DMG_FIX( 100+30*skill_lv, 100 );
			break;
		case SM_MAGNUM:		// �}�O�i���u���C�N
		case MS_MAGNUM:
			if(!wflag) {	// ����
				DMG_FIX( 100+20*skill_lv, 100 );
			} else {	// �O��
				DMG_FIX( 100+10*skill_lv, 100 );
			}
			break;
		case MC_MAMMONITE:	// ���}�[�i�C�g
			DMG_FIX( 100+50*skill_lv, 100 );
			break;
		case AC_DOUBLE:		// �_�u���X�g���C�t�B���O
		case MA_DOUBLE:
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 100+20*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case HT_POWER:		// �r�[�X�g�X�g���C�t�B���O
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 100+16*s_str, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case AC_SHOWER:		// �A���[�V�����[
		case MA_SHOWER:
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 75+5*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			wd.blewcount = 0;
			break;
		case AC_CHARGEARROW:	// �`���[�W�A���[
		case MA_CHARGEARROW:
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 150, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case HT_PHANTASMIC:	// �t�@���^�X�~�b�N�A���[
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 150, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case KN_CHARGEATK:	// �`���[�W�A�^�b�N
			{
				int dist = unit_distance2(src,target)-1;
				if(dist > 2)
					DMG_FIX( 100+100*(dist/3), 100 );
			}
			break;
		case AS_VENOMKNIFE:	// �x�i���i�C�t
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			calc_flag.nocardfix = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case SG_SUN_WARM:	// ���z�̉�����
		case SG_MOON_WARM:	// ���̉�����
		case SG_STAR_WARM:	// ���̉�����
			if(src_sd) {
				if(src_sd->status.sp < 2) {
					status_change_end(src,SkillStatusChangeTable[skill_num],-1);
					break;
				}
				// �������̂�SP����
				src_sd->status.sp -= 2;
				clif_updatestatus(src_sd,SP_SP);
			}
			if(target_sd) {
				target_sd->status.sp -= 15;
				if(target_sd->status.sp < 0)
					target_sd->status.sp = 0;
				clif_updatestatus(target_sd,SP_SP);
			}
			wd.blewcount = 0;
			break;
		case KN_PIERCE:		// �s�A�[�X
		case ML_PIERCE:
			wd.div_ = t_size+1;
			DMG_FIX( (100+10*skill_lv) * wd.div_, 100 );
			break;
		case KN_SPEARSTAB:	// �X�s�A�X�^�u
			DMG_FIX( 100+15*skill_lv, 100 );
			wd.blewcount = 0;
			break;
		case KN_SPEARBOOMERANG:	// �X�s�A�u�[������
			DMG_FIX( 100+50*skill_lv, 100 );
			break;
		case KN_BRANDISHSPEAR:	// �u�����f�B�b�V���X�s�A
		case ML_BRANDISH:
			{
				int rate = 100+20*skill_lv;
				if(wflag == 1) {
					if(skill_lv > 3)
						rate += rate/2;
					if(skill_lv > 6)
						rate += rate/4;
					if(skill_lv > 9)
						rate += rate/8;
				} else if(wflag == 2) {
					if(skill_lv > 6)
						rate += rate/2;
					if(skill_lv > 9)
						rate += rate/4;
				} else if(wflag == 3) {
					if(skill_lv > 3)
						rate += rate/2;
				}
				DMG_FIX( rate, 100 );
			}
			break;
		case KN_BOWLINGBASH:	// �{�E�����O�o�b�V��
		case MS_BOWLINGBASH:
			DMG_FIX( 100+40*skill_lv, 100 );
			wd.blewcount = 0;
			break;
		case AS_SONICBLOW:	// �\�j�b�N�u���E
			{
				int rate = 300+50*skill_lv;
				if(src_sd && pc_checkskill(src_sd,AS_SONICACCEL) > 0)
					rate = rate*110/100;
				if(sc && sc->data[SC_ASSASIN].timer != -1)
				{
					if(map[src->m].flag.gvg)
						rate = rate*125/100;
					else
						rate *= 2;
				}
				DMG_FIX( rate, 100 );
			}
			break;
		case AS_GRIMTOOTH:	// �O�����g�D�[�X
			DMG_FIX( 100+20*skill_lv, 100 );
			break;
		case TF_SPRINKLESAND:	// ���܂�
			DMG_FIX( 130, 100 );
			break;
		case MC_CARTREVOLUTION:	// �J�[�g���{�����[�V����
			if(src_sd && src_sd->cart_max_weight > 0 && src_sd->cart_weight > 0) {
				DMG_FIX( 150 + pc_checkskill(src_sd,BS_WEAPONRESEARCH) + src_sd->cart_weight*100/src_sd->cart_max_weight, 100 );
			} else {
				DMG_FIX( 150, 100 );
			}
			wd.blewcount = 0;
			break;
		case NPC_COMBOATTACK:	// ���i�U��
			DMG_FIX( 50*wd.div_, 100 );
			break;
		case NPC_RANDOMATTACK:	// �����_��ATK�U��
			DMG_FIX( 50+atn_rand()%150, 100 );
			break;
		case NPC_WATERATTACK:
		case NPC_GROUNDATTACK:
		case NPC_FIREATTACK:
		case NPC_WINDATTACK:
		case NPC_POISONATTACK:
		case NPC_HOLYATTACK:
		case NPC_DARKNESSATTACK:
		case NPC_TELEKINESISATTACK:
		case NPC_UNDEADATTACK:
			DMG_FIX( 25+75*skill_lv, 100 );
			break;
		case NPC_PIERCINGATT:
			DMG_FIX( 75, 100 );
			break;
		case RG_BACKSTAP:	// �o�b�N�X�^�u
			{
				int rate = 300+40*skill_lv;
				if(src_sd && src_sd->status.weapon == WT_BOW) {	// �|�Ȃ甼��
					rate /= 2;
				}
				DMG_FIX( rate, 100 );
			}
			break;
		case RG_RAID:		// �T�v���C�Y�A�^�b�N
			DMG_FIX( 100+40*skill_lv, 100 );
			break;
		case RG_INTIMIDATE:	// �C���e�B�~�f�C�g
			DMG_FIX( 100+30*skill_lv, 100 );
			break;
		case CR_SHIELDCHARGE:	// �V�[���h�`���[�W
			DMG_FIX( 100+20*skill_lv, 100 );
			break;
		case CR_SHIELDBOOMERANG:	// �V�[���h�u�[������
			{
				int rate = 100+30*skill_lv;
				if(sc && sc->data[SC_CRUSADER].timer != -1)
					rate *= 2;
				DMG_FIX( rate, 100 );
			}
			break;
		case CR_HOLYCROSS:	// �z�[���[�N���X
		case NPC_DARKCROSS:	// �_�[�N�N���X
			DMG_FIX( 100+35*skill_lv, 100 );
			break;
		case CR_GRANDCROSS:	// �O�����h�N���X
		case NPC_GRANDDARKNESS:	// �O�����h�_�[�N�l�X
			if (!battle_config.gx_cardfix)
				calc_flag.nocardfix = 1;
			break;
		case AM_DEMONSTRATION:	// �f�����X�g���[�V����
			DMG_FIX( 100+20*skill_lv, 100 );
			calc_flag.nocardfix = 1;
			break;
		case AM_ACIDTERROR:	// �A�V�b�h�e���[
			DMG_FIX( 100+40*skill_lv, 100 );
			calc_flag.nocardfix = 1;
			break;
		case MO_FINGEROFFENSIVE:	// �w�e
			if(src_sd && battle_config.finger_offensive_type == 0) {
				wd.div_ = src_sd->spiritball_old;
				DMG_FIX( (100+50*skill_lv) * wd.div_, 100 );
			} else {
				wd.div_ = 1;
				DMG_FIX( 100+50*skill_lv, 100 );
			}
			break;
		case MO_INVESTIGATE:	// ����
			if(t_def1 < 1000000) {
				DMG_FIX( (100+75*skill_lv) * (t_def1 + t_def2), 100*50 );
			}
			break;
		case MO_BALKYOUNG:
			DMG_FIX( 300, 100 );
			break;
		case MO_EXTREMITYFIST:	// ���C���e�P��
			if(src_sd) {
				DMG_FIX( 8+src_sd->status.sp/10, 1 );
				src_sd->status.sp = 0;
				clif_updatestatus(src_sd,SP_SP);
			} else {
				DMG_FIX( 8, 1 );
			}
			DMG_ADD( 250+150*skill_lv );
			break;
		case MO_CHAINCOMBO:	// �A�ŏ�
			DMG_FIX( 150+50*skill_lv, 100 );
			break;
		case MO_COMBOFINISH:	// �җ���
			DMG_FIX( 240+60*skill_lv, 100 );
			// PT�ɂ͓����Ă���
			// �J�E���^�[�A�^�b�N�̊m���㏸
			if(src_sd && src_sd->status.party_id > 0) {
				struct party *pt = party_search(src_sd->status.party_id);
				if(pt != NULL)
				{
					struct map_session_data* psrc_sd = NULL;
					for(i=0; i<MAX_PARTY; i++)
					{
						psrc_sd = pt->member[i].sd;
						if(!psrc_sd || src_sd == psrc_sd)
							continue;
						if(src_sd->bl.m == psrc_sd->bl.m && pc_checkskill(psrc_sd,TK_COUNTER) > 0)
						{
							status_change_start(&psrc_sd->bl,SC_COUNTER_RATE_UP,1,0,0,0,battle_config.tk_counter_rate_up_keeptime,0);
						}
					}
				}
			}
			break;
		case TK_STORMKICK:	// �����R��
			DMG_FIX( 160+20*skill_lv, 100 );
			break;
		case TK_DOWNKICK:	// ���i�R��
			DMG_FIX( 160+20*skill_lv, 100 );
			break;
		case TK_TURNKICK:	// ��]�R��
			DMG_FIX( 190+30*skill_lv, 100 );
			wd.blewcount = 0;
			break;
		case TK_COUNTER:	// �J�E���^�[�R��
			DMG_FIX( 190+30*skill_lv, 100 );
			// PT�ɂ͓����Ă���
			// �O�i���̊m���㏸
			if(src_sd && src_sd->status.party_id > 0 && (skill = pc_checkskill(src_sd,SG_FRIEND)) > 0) {
				struct party *pt = party_search(src_sd->status.party_id);
				if(pt != NULL)
				{
					struct map_session_data* psrc_sd = NULL;
					for(i=0; i<MAX_PARTY; i++)
					{
						psrc_sd = pt->member[i].sd;
						if(!psrc_sd || src_sd == psrc_sd)
							continue;
						if(src_sd->bl.m == psrc_sd->bl.m && pc_checkskill(psrc_sd,MO_TRIPLEATTACK) > 0)
						{
							status_change_start(&psrc_sd->bl,SC_TRIPLEATTACK_RATE_UP,skill,0,0,0,battle_config.tripleattack_rate_up_keeptime,0);
						}
					}
				}
			}
			break;
		case BA_MUSICALSTRIKE:	// �~���[�W�J���X�g���C�N
		case DC_THROWARROW:	// ���
			DMG_FIX( 60+40*skill_lv, 100 );
			break;
		case CH_TIGERFIST:	// ���Ռ�
			DMG_FIX( 40+100*skill_lv, 100 );
			break;
		case CH_CHAINCRUSH:	// �A������
			DMG_FIX( 400+100*skill_lv, 100 );
			break;
		case CH_PALMSTRIKE:	// �ҌՍd�h�R
			DMG_FIX( 200+100*skill_lv, 100 );
			break;
		case LK_SPIRALPIERCE:	// �X�p�C�����s�A�[�X
		case ML_SPIRALPIERCE:
			DMG_FIX( 80+40*skill_lv, 100 );
			break;
		case LK_HEADCRUSH:	// �w�b�h�N���b�V��
			DMG_FIX( 100+40*skill_lv, 100 );
			break;
		case LK_JOINTBEAT:	// �W���C���g�r�[�g
			DMG_FIX( 50+10*skill_lv, 100 );
			break;
		case ASC_METEORASSAULT:	// ���e�I�A�T���g
			DMG_FIX( 40+40*skill_lv, 100 );
			calc_flag.nocardfix = 1;
			break;
		case ASC_BREAKER:	// �\�E���u���C�J�[
			DMG_FIX( skill_lv, 1 );
			calc_flag.nocardfix = 1;
			break;
		case SN_SHARPSHOOTING:	// �V���[�v�V���[�e�B���O
		case MA_SHARPSHOOTING:
			DMG_FIX( 200+50*skill_lv, 100 );
			break;
		case CG_ARROWVULCAN:	// �A���[�o���J��
			DMG_FIX( 200+100*skill_lv, 100 );
			break;
		case AS_SPLASHER:	// �x�i���X�v���b�V���[
			if(src_sd) {
				DMG_FIX( 500+50*skill_lv+20*pc_checkskill(src_sd,AS_POISONREACT), 100 );
			} else {
				DMG_FIX( 500+50*skill_lv, 100 );
			}
			calc_flag.nocardfix = 1;
			break;
		case AS_POISONREACT:	// �|�C�Y�����A�N�g�i�U���Ŕ����j
			wd.damage = wd.damage*(100+30*skill_lv)/100;
			//wd.damage2 = wd.damage2	// ����ɂ͏��Ȃ�
			break;
		case TK_JUMPKICK:	// ��яR��
			if(sc && (sc->data[SC_RUN].timer != -1 || sc->data[SC_DODGE_DELAY].timer != -1)) {
				DMG_FIX( 30 + (10+status_get_lv(src)/10)*skill_lv, 100 );
				if(sc->data[SC_DODGE_DELAY].timer != -1)
					status_change_end(src,SC_DODGE_DELAY,-1);
			} else {
				DMG_FIX( 30+10*skill_lv, 100 );
			}
			if(src_sd && sc && sc->data[SC_RUN].timer != -1 && sc->data[SC_SPURT].timer != -1) {
				// �^�C���M���Ŋ��X�p�[�g��ԂȂ�З͂���ɃA�b�v
				// �v�Z���s���Ȃ̂œK��
				DMG_ADD( 10*pc_checkskill(src_sd,TK_RUN) );
			}
			// �e�B�I�A�v�`���M�ɂ��Ώۂ̃X�e�[�^�X�ُ����
			if(target_sd && target_sd->status.class_ == PC_CLASS_SL)	// �\�E�������J�[�͖���
				break;
			if(t_sc && t_sc->data[SC_PRESERVE].timer != -1)		// �v���U�[�u���͐؂�Ȃ�
				break;
			status_change_release(target,0x10);
			break;
		case PA_SHIELDCHAIN:	// �V�[���h�`�F�C��
			if(src_sd) {
				int s_dex = status_get_dex(src);
				int s_luk = status_get_luk(src);
				int idx   = src_sd->equip_index[8];
				DMG_SET( s_str+(s_str/10)*(s_str/10)+(s_dex/5)+(s_luk/5) );
				if(idx >= 0 && src_sd->inventory_data[idx] && src_sd->inventory_data[idx]->type == 5)
					DMG_ADD( src_sd->status.inventory[idx].refine*4 + src_sd->inventory_data[idx]->weight/10 );
			} else {
				DMG_FIX( 100+30*skill_lv, 100 );
			}
			break;
		case WS_CARTTERMINATION:	// �J�[�g�^�[�~�l�[�V����
			{
				int rate = (skill_lv >= 16)? 1: 16-skill_lv;
				if(src_sd && src_sd->cart_max_weight > 0 && src_sd->cart_weight > 0) {
					DMG_FIX( src_sd->cart_weight, rate*100 );
					DMG_FIX( 8000, src_sd->cart_max_weight );
				} else if(!src_sd) {
					DMG_FIX( 80, rate );
				}
			}
			calc_flag.nocardfix = 1;
			break;
		case CR_ACIDDEMONSTRATION:	// �A�V�b�h�f�����X�g���[�V����
			{
				int s_int = status_get_int(src);
				atn_bignumber dmg = (atn_bignumber)s_int * s_int * t_vit * skill_lv * 7 / 10 / (s_int + t_vit);
				if(target->type != BL_MOB)
					dmg /= 2;
				DMG_SET( (int)dmg );
			}
			calc_flag.nocardfix = 1;
			break;
		case GS_TRIPLEACTION:	// �g���v���A�N�V����
			DMG_FIX( 450, 100 );
			break;
		case GS_BULLSEYE:	// �u���Y�A�C
			DMG_FIX( 500, 100 );
			calc_flag.nocardfix = 1;
			break;
		case GS_MAGICALBULLET:	// �}�W�J���o���b�g
			{
				int matk1 = status_get_matk1(src);
				int matk2 = status_get_matk2(src);
				if(matk1 > matk2)
					wd.damage += matk2+atn_rand()%(matk1-matk2+1);
				else
					wd.damage += matk2;
			}
			break;
		case GS_TRACKING:	// �g���b�L���O
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 200+100*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_DISARM:		// �f�B�X�A�[��
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_PIERCINGSHOT:	// �s�A�[�V���O�V���b�g
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 100+20*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_RAPIDSHOWER:	// ���s�b�h�V�����[
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 500+50*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_DESPERADO:	// �f�X�y���[�h
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 50+50*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_DUST:		// �_�X�g
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 100+50*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_FULLBUSTER:	// �t���o�X�^�[
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 300+100*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_SPREADATTACK:	// �X�v���b�h�A�^�b�N
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 80+20*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case NJ_SYURIKEN:	// �藠������
			if(src_sd) {
				if(!src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					DMG_ADD( arr );
				}
				src_sd->state.arrow_atk = 1;
			}
			DMG_ADD( skill_lv*3 );
			break;
		case NJ_KUNAI:		// �ꖳ����
			if(src_sd) {
				if(!src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					DMG_ADD( arr );
				}
				src_sd->state.arrow_atk = 1;
			}
			DMG_FIX( 300, 100 );
			break;
		case NJ_HUUMA:		// �����藠������
			{
				int rate = 150+150*skill_lv;
				if(wflag > 1)
					rate /= wflag;
				DMG_FIX( rate, 100 );
			}
			break;
		case NJ_ZENYNAGE:	// �K����
			{
				int dmg = 0;
				if(src_sd) {
					dmg = src_sd->zenynage_damage;
					src_sd->zenynage_damage = 0;	// �������烊�Z�b�g
				} else {
					dmg = skill_get_zeny(NJ_ZENYNAGE,skill_lv)/2;
					if(dmg > 0)
						dmg += atn_rand()%dmg;
					else
						dmg = 500*skill_lv + atn_rand()%(500*skill_lv);
				}
				if(target->type == BL_PC || t_mode & 0x20) {
					dmg /= 2;
				}
				DMG_SET( dmg );
				calc_flag.nocardfix = 1;
			}
			break;
		case NJ_TATAMIGAESHI:	// ���Ԃ�
			DMG_FIX( 100+10*skill_lv, 100 );
			break;
		case NJ_KASUMIKIRI:	// ���a��
			DMG_FIX( 100+10*skill_lv, 100 );
			break;
		case NJ_KIRIKAGE:	// �e�a��
			DMG_FIX( skill_lv, 1 );
			break;
		case NJ_ISSEN:		// ��M
			{
				int hp = status_get_hp(src);
				DMG_SET( (s_str*40)+(skill_lv*(hp-1)*8)/100 );
				unit_heal(src,-hp+1,0);
				if(sc && sc->data[SC_NEN].timer != -1)
					status_change_end(src,SC_NEN,-1);
			}
			break;
		case NPC_EARTHQUAKE:		// �A�[�X�N�G�C�N
			DMG_FIX( 500+500*skill_lv, 100 );
			if(wflag > 1) {
				DMG_FIX( 1, wflag );
			}
			break;
		case NPC_FIREBREATH:		// �t�@�C�A�u���X
		case NPC_ICEBREATH:		// �A�C�X�u���X
		case NPC_THUNDERBREATH:		// �T���_�[�u���X
		case NPC_ACIDBREATH:		// �A�V�b�h�u���X
		case NPC_DARKNESSBREATH:	// �_�[�N�l�X�u���X
		case NPC_HELLJUDGEMENT:		// �w���W���b�W�����g
			DMG_FIX( 100*skill_lv, 100 );
			break;
		case NPC_PULSESTRIKE:		// �p���X�X�g���C�N
			DMG_FIX( 100*skill_lv, 100 );
			wd.blewcount = 0;
			break;
		case HFLI_MOON:		// ���[�����C�g
			DMG_FIX( 110+110*skill_lv, 100 );
			break;
		case HFLI_SBR44:	// S.B.R.44
			if(src_hd) {
				DMG_SET( src_hd->intimate*skill_lv );
				src_hd->intimate = 200;
				if(battle_config.homun_skill_intimate_type)
					src_hd->status.intimate = 200;
				clif_send_homdata(src_hd->msd,1,src_hd->intimate/100);
			}
			break;
		case MER_CRASH:		// �N���b�V��
			DMG_FIX( 100+10*skill_lv, 100 );
			break;
		}

		/* 13�D�t�@�C�e�B���O�̒ǉ��_���[�W */
		wd.damage += tk_power_damage;
		if(calc_flag.lh)
			wd.damage2 += tk_power_damage2;

		if(calc_flag.da == 2) {	// �O�i�����������Ă���
			if(src_sd)
				wd.damage = wd.damage * (100 + 20 * pc_checkskill(src_sd, MO_TRIPLEATTACK)) / 100;
		}

		/* 14�D�h�䖳�����肨��ѐ����ʃ_���[�W�v�Z */
		switch (skill_num) {
		case KN_AUTOCOUNTER:
		case CR_GRANDCROSS:
		case MO_INVESTIGATE:
		case MO_EXTREMITYFIST:
		case AM_ACIDTERROR:
		case CR_ACIDDEMONSTRATION:
		case NJ_ZENYNAGE:
			break;
		default:
			if( wd.type != 0 )	// �N���e�B�J�����͖���
				break;
			if( skill_num == WS_CARTTERMINATION && !battle_config.def_ratio_atk_to_carttermination )
				break;
			if( skill_num == PA_SHIELDCHAIN && !battle_config.def_ratio_atk_to_shieldchain )
				break;
			if(src_sd && t_def1 < 1000000)
			{
				int mask = (1<<t_race) | ( (t_mode&0x20)? (1<<10): (1<<11) );

				// bIgnoreDef�n
				if( !calc_flag.idef && (src_sd->ignore_def_ele & (1<<t_ele) || src_sd->ignore_def_race & mask || src_sd->ignore_def_enemy & (1<<t_enemy)) )
					calc_flag.idef = 1;
				if( calc_flag.lh ) {
					if( !calc_flag.idef_ && (src_sd->ignore_def_ele_ & (1<<t_ele) || src_sd->ignore_def_race_ & mask || src_sd->ignore_def_enemy_ & (1<<t_enemy)) ) {
						calc_flag.idef_ = 1;
						if(battle_config.left_cardfix_to_right)
							calc_flag.idef = 1;
					}
				}
				// bDefRatioATK�n
				if( !calc_flag.idef && (src_sd->def_ratio_atk_ele & (1<<t_ele) || src_sd->def_ratio_atk_race & mask || src_sd->def_ratio_atk_enemy & (1<<t_enemy)) ) {
					wd.damage = (wd.damage * (t_def1 + t_def2))/100;
					calc_flag.idef = 1;
				}
				if( calc_flag.lh ) {
					if( !calc_flag.idef_ && (src_sd->def_ratio_atk_ele_ & (1<<t_ele) || src_sd->def_ratio_atk_race_ & mask || src_sd->def_ratio_atk_enemy_ & (1<<t_enemy)) ) {
						wd.damage2 = (wd.damage2 * (t_def1 + t_def2))/100;
						calc_flag.idef_ = 1;
						if(!calc_flag.idef && battle_config.left_cardfix_to_right) {
							wd.damage = (wd.damage * (t_def1 + t_def2))/100;
							calc_flag.idef = 1;
						}
					}
				}
			}
			break;
		}

		/* 15�D�Ώۂ̖h��͂ɂ��_���[�W�̌��� */
		switch (skill_num) {
		case KN_AUTOCOUNTER:
		case MO_INVESTIGATE:
		case MO_EXTREMITYFIST:
		case CR_ACIDDEMONSTRATION:
		case NJ_ZENYNAGE:
		case NPC_CRITICALSLASH:
		case GS_PIERCINGSHOT:
			break;
		default:
			if(wd.type != 0)	// �N���e�B�J�����͖���
				break;
			// ���z�ƌ��Ɛ��̗Z�� DEF����
			if(sc && sc->data[SC_FUSION].timer != -1)
				calc_flag.idef = 1;

			// DEF�����t���O���Ȃ��Ƃ�
			if( ((calc_flag.rh && !calc_flag.idef) || (calc_flag.lh && !calc_flag.idef_)) && t_def1 < 1000000 )
			{
				int t_def, vitbonusmax;
				int target_count = 1;

				if(target->type != BL_HOM) {
					target_count = unit_counttargeted(target,battle_config.vit_penalty_count_lv);
				}
				if(battle_config.vit_penalty_type > 0 && (!t_sc || t_sc->data[SC_STEELBODY].timer == -1)) {
					if(target_count >= battle_config.vit_penalty_count) {
						if(battle_config.vit_penalty_type == 1) {
							t_def1 = (t_def1 * (100 - (target_count - (battle_config.vit_penalty_count - 1))*battle_config.vit_penalty_num))/100;
							t_def2 = (t_def2 * (100 - (target_count - (battle_config.vit_penalty_count - 1))*battle_config.vit_penalty_num))/100;
							t_vit  = (t_vit  * (100 - (target_count - (battle_config.vit_penalty_count - 1))*battle_config.vit_penalty_num))/100;
						}
						else if(battle_config.vit_penalty_type == 2) {
							t_def1 -= (target_count - (battle_config.vit_penalty_count - 1))*battle_config.vit_penalty_num;
							t_def2 -= (target_count - (battle_config.vit_penalty_count - 1))*battle_config.vit_penalty_num;
							t_vit  -= (target_count - (battle_config.vit_penalty_count - 1))*battle_config.vit_penalty_num;
						}
						if(t_def1 < 0) t_def1 = 0;
						if(t_def2 < 1) t_def2 = 1;
						if(t_vit < 1) t_vit = 1;
					}
				}
				t_def = t_def2*8/10;

				// �f�B�o�C���v���e�N�V����
				if(target_sd && (skill = pc_checkskill(target_sd,AL_DP)) > 0) {
					int s_race = status_get_race(src);
					if(battle_check_undead(s_race,status_get_elem_type(src)) || s_race == RCT_DEMON)
						t_def += ((300 + 4 * target_sd->status.base_level) * skill + 50) / 100;
				}
				vitbonusmax = (t_vit/20)*(t_vit/20)-1;

				if(calc_flag.rh && !calc_flag.idef) {
					if(battle_config.player_defense_type) {
						wd.damage = wd.damage - (t_def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)? 0: atn_rand()%(vitbonusmax+1) );
						damage_ot = damage_ot - (t_def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)? 0: atn_rand()%(vitbonusmax+1) );
					} else {
						wd.damage = wd.damage * (100 - t_def1) /100 - t_def - ((vitbonusmax < 1)? 0: atn_rand()%(vitbonusmax+1) );
						damage_ot = damage_ot * (100 - t_def1) /100;
					}
				}
				if(calc_flag.lh && !calc_flag.idef_) {
					if(battle_config.player_defense_type) {
						wd.damage2 = wd.damage2 - (t_def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)? 0: atn_rand()%(vitbonusmax+1) );
						damage_ot2 = damage_ot2 - (t_def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)? 0: atn_rand()%(vitbonusmax+1) );
					} else {
						wd.damage2 = wd.damage2 * (100 - t_def1) /100 - t_def - ((vitbonusmax < 1)? 0: atn_rand()%(vitbonusmax+1) );
						damage_ot2 = damage_ot2 * (100 - t_def1) /100;
					}
				}
			}
			break;
		}

		/* 16�D��Ԉُ풆�̃_���[�W�ǉ��ŃN���e�B�J���ɂ��L���ȃX�L�� */
		if (sc) {
			// �I�[�o�[�g���X�g
			if(sc->data[SC_OVERTHRUST].timer != -1) {
				wd.damage += damage_ot*(5*sc->data[SC_OVERTHRUST].val1)/100;
				if(calc_flag.lh)
					wd.damage2 += damage_ot2*(5*sc->data[SC_OVERTHRUST].val1)/100;
			}
			// �I�[�o�[�g���X�g�}�b�N�X
			if(sc->data[SC_OVERTHRUSTMAX].timer != -1) {
				wd.damage += damage_ot*(20*sc->data[SC_OVERTHRUSTMAX].val1)/100;
				if(calc_flag.lh)
					wd.damage2 += damage_ot2*(20*sc->data[SC_OVERTHRUSTMAX].val1)/100;
			}
			// �g�D���[�T�C�g
			if(sc->data[SC_TRUESIGHT].timer != -1) {
				DMG_FIX( 100+2*sc->data[SC_TRUESIGHT].val1, 100 );
			}
			// �o�[�T�[�N
			if(sc->data[SC_BERSERK].timer != -1) {
				DMG_FIX( 200, 100 );
			}
			// �G���`�����g�f�b�h���[�|�C�Y��
			if(sc->data[SC_EDP].timer != -1 && !calc_flag.nocardfix) {
				// �E��݂̂Ɍ��ʂ��̂�B�J�[�h���ʖ����̃X�L���ɂ͏��Ȃ�
				if(map[src->m].flag.pk && target->type == BL_PC) {
					wd.damage += wd.damage * (150 + sc->data[SC_EDP].val1 * 50) * battle_config.pk_edp_down_rate / 10000;
				} else if(map[src->m].flag.gvg) {
					wd.damage += wd.damage * (150 + sc->data[SC_EDP].val1 * 50) * battle_config.gvg_edp_down_rate / 10000;
				} else if(map[src->m].flag.pvp) {
					wd.damage += wd.damage * (150 + sc->data[SC_EDP].val1 * 50) * battle_config.pvp_edp_down_rate / 10000;
				} else {
					wd.damage += wd.damage * (150 + sc->data[SC_EDP].val1 * 50) / 100;
				}
				// calc_flag.nocardfix = 1;
			}
			// �T�N���t�@�C�X
			if(sc->data[SC_SACRIFICE].timer != -1 && !skill_num) {
				if(t_class != 1288) {
					int dmg = status_get_max_hp(src) * 9 / 100;
					battle_heal(NULL, src, -dmg, 0, 0);
					wd.damage  = dmg * (90 + sc->data[SC_SACRIFICE].val1 * 10) / 100;
					wd.damage2 = 0;
					clif_misceffect2(src,366);
				}
				if((--sc->data[SC_SACRIFICE].val2) <= 0)
					status_change_end(src, SC_SACRIFICE,-1);
			}
		}

		/* 17�D���B�_���[�W�̒ǉ� */
		if( src_sd ) {
			if(skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST && skill_num != PA_SHIELDCHAIN && skill_num != CR_ACIDDEMONSTRATION && skill_num != NJ_ZENYNAGE) {
				wd.damage += status_get_atk2(src);
				if(calc_flag.lh)
					wd.damage2 += status_get_atk_2(src);
			}
			switch (skill_num) {
			case CR_SHIELDBOOMERANG:	// �V�[���h�u�[������
				if(src_sd->equip_index[8] >= 0) {
					int idx = src_sd->equip_index[8];
					if(src_sd->inventory_data[idx] && src_sd->inventory_data[idx]->type == 5) {
						wd.damage += src_sd->inventory_data[idx]->weight/10;
						wd.damage += src_sd->status.inventory[idx].refine * status_get_overrefine_bonus(0);
					}
				}
				break;
			case LK_SPIRALPIERCE:		// �X�p�C�����s�A�[�X
				if(src_sd->equip_index[9] >= 0) {	// {((STR/10)^2 �{ ����d�ʁ~�X�L���{���~0.8) �~ �T�C�Y�␳ �{ ���B}�~�J�[�h�{���~�����{���~5�̖͗l
					int idx = src_sd->equip_index[9];
					if(src_sd->inventory_data[idx] && src_sd->inventory_data[idx]->type == 4) {
						wd.damage = ( ( (s_str/10)*(s_str/10) + src_sd->inventory_data[idx]->weight * (skill_lv * 4 + 8 ) / 100 )
									* (5 - t_size) / 4 + status_get_atk2(src) ) * 5;
					}
				}
				break;
			case PA_SHIELDCHAIN:		// �V�[���h�`�F�C��
				if(src_sd->equip_index[8] >= 0) {
					int idx = src_sd->equip_index[8];
					if(src_sd->inventory_data[idx] && src_sd->inventory_data[idx]->type == 5) {
						int refinedamage = 2*(src_sd->status.inventory[idx].refine-4) + src_sd->status.inventory[idx].refine * src_sd->status.inventory[idx].refine;
						wd.damage *= (100+30*skill_lv)/100;
						if(refinedamage > 0)
							wd.damage += atn_rand() % refinedamage;
						wd.damage = (wd.damage+100) * 5;
					}
				}
				break;
			case NJ_SYURIKEN:		// �藠������
				wd.damage += pc_checkskill(src_sd,NJ_TOBIDOUGU) * 3;
				break;
			}
		}

		// 0�����������ꍇ1�ɕ␳
		if(wd.damage  < 1) wd.damage  = 1;
		if(wd.damage2 < 1) wd.damage2 = 1;

		/* 18�D�X�L���C���Q�i�C���n�j*/
		// �C���_���[�W(�E��̂�) �\�j�b�N�u���[���͕ʏ����i1���ɕt��1/8�K��)
		if( src_sd &&
		    skill_num != MO_INVESTIGATE &&
		    skill_num != MO_EXTREMITYFIST &&
		    skill_num != CR_GRANDCROSS &&
		    skill_num != NPC_GRANDDARKNESS &&
		    skill_num != LK_SPIRALPIERCE &&
		    skill_num != CR_ACIDDEMONSTRATION &&
		    skill_num != NJ_ZENYNAGE )
		{
			wd.damage = battle_addmastery(src_sd,target,wd.damage,0);
			if(calc_flag.lh)
				wd.damage2 = battle_addmastery(src_sd,target,wd.damage2,1);
		}
		if(sc) {
			if(sc->data[SC_AURABLADE].timer != -1)		// �I�[���u���[�h
				DMG_ADD( sc->data[SC_AURABLADE].val1*20 );
			if(sc->data[SC_GATLINGFEVER].timer != -1)	// �K�g�����O�t�B�[�o�[
				DMG_ADD( 20+sc->data[SC_GATLINGFEVER].val1*10 );
		}
	}

	/* 19�D�X�L���C���R�i�K���_���[�W�j*/
	if( src_sd && (skill = pc_checkskill(src_sd,BS_WEAPONRESEARCH)) > 0) {
		DMG_ADD( skill*2 );
	}
	if( src_sd && (skill = pc_checkskill(src_sd,TK_RUN)) > 0) {	// �^�C���M�p�b�V�u�ŏR��̈З͉��Z
		if( (skill_num == TK_DOWNKICK || skill_num == TK_STORMKICK || skill_num == TK_TURNKICK || skill_num == TK_COUNTER) &&
		    src_sd->weapontype1 == WT_FIST && src_sd->weapontype2 == WT_FIST ) {
			DMG_ADD( skill*10 );
		}
	}

	/* 20�D�J�[�h�ɂ��_���[�W�ǉ����� */
	if( src_sd && wd.damage > 0 && calc_flag.rh && !calc_flag.nocardfix ) {
		cardfix = 100;
		if(!src_sd->state.arrow_atk) {	// �|��ȊO
			if(!battle_config.left_cardfix_to_right) {	// ����J�[�h�␳�ݒ薳��
				cardfix = cardfix*(100+src_sd->addrace[t_race])/100;	// �푰�ɂ��_���[�W�C��
				cardfix = cardfix*(100+src_sd->addele[t_ele])/100;	// �����ɂ��_���[�W�C��
				cardfix = cardfix*(100+src_sd->addenemy[t_enemy])/100;	// �G�^�C�v�ɂ��_���[�W�C��
				cardfix = cardfix*(100+src_sd->addsize[t_size])/100;	// �T�C�Y�ɂ��_���[�W�C��
				cardfix = cardfix*(100+src_sd->addgroup[t_group])/100;	// �O���[�v�ɂ��_���[�W�C��
			} else {
				cardfix = cardfix*(100+src_sd->addrace[t_race]+src_sd->addrace_[t_race])/100;		// �푰�ɂ��_���[�W�C��(����ɂ��ǉ�����)
				cardfix = cardfix*(100+src_sd->addele[t_ele]+src_sd->addele_[t_ele])/100;		// �����ɂ��_���[�W�C��(����ɂ��ǉ�����)
				cardfix = cardfix*(100+src_sd->addenemy[t_enemy]+src_sd->addenemy_[t_enemy])/100;	// �G�^�C�v�ɂ��_���[�W�C��(����ɂ��ǉ�����)
				cardfix = cardfix*(100+src_sd->addsize[t_size]+src_sd->addsize_[t_size])/100;		// �T�C�Y�ɂ��_���[�W�C��(����ɂ��ǉ�����)
				cardfix = cardfix*(100+src_sd->addgroup[t_group]+src_sd->addgroup_[t_group])/100;	// �O���[�v�ɂ��_���[�W�C��(����ɂ��ǉ�����)
			}
		} else { // �|��
			cardfix = cardfix*(100+src_sd->addrace[t_race]+src_sd->arrow_addrace[t_race])/100;	// �푰�ɂ��_���[�W�C��(�|��ɂ��ǉ�����)
			cardfix = cardfix*(100+src_sd->addele[t_ele]+src_sd->arrow_addele[t_ele])/100;		// �����ɂ��_���[�W�C��(�|��ɂ��ǉ�����)
			cardfix = cardfix*(100+src_sd->addenemy[t_enemy]+src_sd->arrow_addenemy[t_enemy])/100;	// �G�^�C�v�ɂ��_���[�W�C��(�|��ɂ��ǉ�����)
			cardfix = cardfix*(100+src_sd->addsize[t_size]+src_sd->arrow_addsize[t_size])/100;	// �T�C�Y�ɂ��_���[�W�C��(�|��ɂ��ǉ�����)
			cardfix = cardfix*(100+src_sd->addgroup[t_group]+src_sd->arrow_addgroup[t_group])/100;	// �O���[�v�ɂ��_���[�W�C��(�|��ɂ��ǉ�����)
		}
		if(t_mode & 0x20) {	// �{�X
			if(!src_sd->state.arrow_atk) {	// �|��U���ȊO�Ȃ�
				if(!battle_config.left_cardfix_to_right) {
					// ����J�[�h�␳�ݒ薳��
					cardfix = cardfix*(100+src_sd->addrace[RCT_BOSS])/100;					// �{�X�����X�^�[�ɒǉ��_���[�W
				} else {
					// ����J�[�h�␳�ݒ肠��
					cardfix = cardfix*(100+src_sd->addrace[RCT_BOSS]+src_sd->addrace_[RCT_BOSS])/100;	// �{�X�����X�^�[�ɒǉ��_���[�W(����ɂ��ǉ�����)
				}
			} else {	// �|��U��
				cardfix = cardfix*(100+src_sd->addrace[RCT_BOSS]+src_sd->arrow_addrace[RCT_BOSS])/100;		// �{�X�����X�^�[�ɒǉ��_���[�W(�|��ɂ��ǉ�����)
			}
		} else {		// �{�X����Ȃ�
			if(!src_sd->state.arrow_atk) {	// �|��U���ȊO
				if(!battle_config.left_cardfix_to_right) {
					// ����J�[�h�␳�ݒ薳��
					cardfix = cardfix*(100+src_sd->addrace[RCT_NONBOSS])/100;				// �{�X�ȊO�����X�^�[�ɒǉ��_���[�W
				} else {
					// ����J�[�h�␳�ݒ肠��
					cardfix = cardfix*(100+src_sd->addrace[RCT_NONBOSS]+src_sd->addrace_[RCT_NONBOSS])/100;	// �{�X�ȊO�����X�^�[�ɒǉ��_���[�W(����ɂ��ǉ�����)
				}
			} else {
				cardfix = cardfix*(100+src_sd->addrace[RCT_NONBOSS]+src_sd->arrow_addrace[RCT_NONBOSS])/100;	// �{�X�ȊO�����X�^�[�ɒǉ��_���[�W(�|��ɂ��ǉ�����)
			}
		}
		// �J�[�h���ʂɂ����背���W�U���̃_���[�W����
		if(wd.flag&BF_SHORT) {
			cardfix = cardfix * (100+src_sd->short_weapon_damege_rate) / 100;
		}
		if(wd.flag&BF_LONG) {
			cardfix = cardfix * (100+src_sd->long_weapon_damege_rate) / 100;
		}
		// �J�[�h���ʂɂ�����X�L���̃_���[�W�����i����X�L���j
		if(src_sd->skill_dmgup.count > 0 && skill_num > 0 && wd.damage > 0) {
			for( i=0; i<src_sd->skill_dmgup.count; i++ ) {
				if( skill_num == src_sd->skill_dmgup.id[i] ) {
					cardfix = cardfix*(100+src_sd->skill_dmgup.rate[i])/100;
					break;
				}
			}
		}
		// ����Class�p�␳����(�����̓��L���{���S���p�H)
		for(i=0; i<src_sd->add_damage_class_count; i++) {
			if(src_sd->add_damage_classid[i] == t_class) {
				cardfix = cardfix*(100+src_sd->add_damage_classrate[i])/100;
				break;
			}
		}
		wd.damage = wd.damage*cardfix/100;	// �J�[�h�␳�ɂ��_���[�W����
	}

	/* 21�D�J�[�h�ɂ�鍶��_���[�W�ǉ����� */
	if( src_sd && wd.damage2 > 0 && calc_flag.lh && !calc_flag.nocardfix ) {
		cardfix = 100;
		if(!battle_config.left_cardfix_to_right) {	// ����J�[�h�␳�ݒ薳��
			cardfix = cardfix*(100+src_sd->addrace_[t_race])/100;	// �푰�ɂ��_���[�W�C������
			cardfix = cardfix*(100+src_sd->addele_[t_ele])/100;	// �����ɂ��_���[�W�C������
			cardfix = cardfix*(100+src_sd->addenemy_[t_enemy])/100;	// �G�^�C�v�ɂ��_���[�W�C������
			cardfix = cardfix*(100+src_sd->addsize_[t_size])/100;	// �T�C�Y�ɂ��_���[�W�C������
			cardfix = cardfix*(100+src_sd->addgroup_[t_group])/100;	// �O���[�v�ɂ��_���[�W�C������
			if(t_mode & 0x20)	// �{�X
				cardfix = cardfix*(100+src_sd->addrace_[RCT_BOSS])/100;		// �{�X�����X�^�[�ɒǉ��_���[�W����
			else
				cardfix = cardfix*(100+src_sd->addrace_[RCT_NONBOSS])/100;	// �{�X�ȊO�����X�^�[�ɒǉ��_���[�W����
		}
		// ����Class�p�␳��������(�����̓��L���{���S���p�H)
		for(i=0; i<src_sd->add_damage_class_count_; i++) {
			if(src_sd->add_damage_classid_[i] == t_class) {
				cardfix = cardfix*(100+src_sd->add_damage_classrate_[i])/100;
				break;
			}
		}
		wd.damage2 = wd.damage2*cardfix/100;	// �J�[�h�␳�ɂ�鍶��_���[�W����
	}

	/* 22�D�\�E���u���C�J�[�̖��@�_���[�W�v�Z */
	if(skill_num == ASC_BREAKER)
		damage_sbr = status_get_int(src) * skill_lv * 5; 

	/* 23�D�J�[�h�ɂ��_���[�W�������� */
	if( target_sd && (wd.damage > 0 || wd.damage2 > 0) ) {	// �Ώۂ�PC�̏ꍇ
		int s_race  = status_get_race(src);
		int s_enemy = status_get_enemy_type(src);
		int s_size  = status_get_size(src);
		int s_group = status_get_group(src);
		cardfix = 100;
		cardfix = cardfix*(100-target_sd->subrace[s_race])/100;			// �푰�ɂ��_���[�W�ϐ�
		if (s_ele == ELE_NONE)
			cardfix = cardfix*(100-target_sd->subele[ELE_NEUTRAL])/100;	// ���������̑ϐ��͖�����
		else
			cardfix = cardfix*(100-target_sd->subele[s_ele])/100;		// �����ɂ��_���[�W�ϐ�
		cardfix = cardfix*(100-target_sd->subenemy[s_enemy])/100;		// �G�^�C�v�ɂ��_���[�W�ϐ�
		cardfix = cardfix*(100-target_sd->subsize[s_size])/100;			// �T�C�Y�ɂ��_���[�W�ϐ�
		cardfix = cardfix*(100-target_sd->subgroup[s_group])/100;		// �O���[�v�ɂ��_���[�W�ϐ�

		if(status_get_mode(src) & 0x20)
			cardfix = cardfix*(100-target_sd->subrace[RCT_BOSS])/100;	// �{�X����̍U���̓_���[�W����
		else
			cardfix = cardfix*(100-target_sd->subrace[RCT_NONBOSS])/100;	// �{�X�ȊO����̍U���̓_���[�W����

		// ����Class�p�␳��������(�����̓��L���{���S���p�H)
		for(i=0; i<target_sd->add_def_class_count; i++) {
			if(target_sd->add_def_classid[i] == status_get_class(src)) {
				cardfix = cardfix*(100-target_sd->add_def_classrate[i])/100;
				break;
			}
		}
		if(wd.flag&BF_LONG)
			cardfix = cardfix*(100-target_sd->long_attack_def_rate)/100;	// �������U���̓_���[�W����(�z����C�Ƃ�)
		if(wd.flag&BF_SHORT)
			cardfix = cardfix*(100-target_sd->near_attack_def_rate)/100;	// �ߋ����U���̓_���[�W����(�Y�������H)
		DMG_FIX( cardfix, 100 );	// �J�[�h�␳�ɂ��_���[�W����

		damage_sbr = damage_sbr * cardfix / 100;	// �J�[�h�␳�ɂ��\�E���u���C�J�[�̖��@�_���[�W����
	}

	/* 24�D�A�C�e���{�[�i�X�̃t���O���� */
	// ��Ԉُ�̃����W�t���O
	//   addeff_range_flag  0:�w�薳�� 1:�ߋ��� 2:������ 3,4:���ꂼ��̃����W�ŏ�Ԉُ�𔭓������Ȃ�
	//   flag������A�U���^�C�v��flag����v���Ȃ��Ƃ��́Aflag+2����
	if(src_sd && wd.flag&BF_WEAPON) {
		for(i=SC_STONE; i<=SC_BLEED; i++) {
			if( (src_sd->addeff_range_flag[i-SC_STONE] == 1 && wd.flag&BF_LONG ) ||
			    (src_sd->addeff_range_flag[i-SC_STONE] == 2 && wd.flag&BF_SHORT) ) {
				src_sd->addeff_range_flag[i-SC_STONE] += 2;
			}
		}
	}

	/* 25�D�ΏۂɃX�e�[�^�X�ُ킪����ꍇ�̃_���[�W���Z���� */
	if( t_sc && (wd.damage > 0 || wd.damage2 > 0) ) {
		cardfix = 100;
		if(t_sc->data[SC_DEFENDER].timer != -1 && wd.flag&BF_LONG && skill_num != CR_ACIDDEMONSTRATION)	// �f�B�t�F���_�[��Ԃŉ������U��
			cardfix = cardfix*(100-t_sc->data[SC_DEFENDER].val2)/100;
		if(t_sc->data[SC_ADJUSTMENT].timer != -1 && wd.flag&BF_LONG)	// �A�W���X�g�����g��Ԃŉ������U��
			cardfix -= 20;
		if(cardfix != 100) {
			DMG_FIX( cardfix, 100 );	// �X�e�[�^�X�ُ�␳�ɂ��_���[�W����
		}
	}

	if(wd.damage  < 0) wd.damage  = 0;
	if(wd.damage2 < 0) wd.damage2 = 0;

	/* 26�D�����̓K�p */
	wd.damage = battle_attr_fix(wd.damage, s_ele, status_get_element(target));
	if(calc_flag.lh)
		wd.damage2 = battle_attr_fix(wd.damage2, s_ele_, status_get_element(target));

	/* 27�D�X�L���C���S�i�ǉ��_���[�W�j */
	// �}�O�i���u���C�N���
	if(sc && sc->data[SC_MAGNUM].timer != -1) {
		int bonus_damage = battle_attr_fix(wd.damage, ELE_FIRE, status_get_element(target)) * 20/100;	// �Α����U���_���[�W��20%��ǉ�
		if(bonus_damage > 0) {
			DMG_ADD( bonus_damage );
		}
	}
	// �\�E���u���C�J�[
	if(skill_num == ASC_BREAKER) {
		wd.damage += damage_sbr;		// ���@�_���[�W
		wd.damage += 500 + (atn_rand() % 500);	// �����_���_���[�W
		if(t_def1 < 1000000) {
			int vitbonusmax = (t_vit/20)*(t_vit/20)-1;
			wd.damage -= (t_def1 + t_def2 + ((vitbonusmax < 1)? 0: atn_rand()%(vitbonusmax+1)) + status_get_mdef(target) + status_get_mdef2(target))/2;
		}
	}

	/* 28�D���̂�����A�C���̓K�p */
	if(src_sd) {
		DMG_ADD( src_sd->spiritball*3 );
		DMG_ADD( src_sd->coin*3 );
		DMG_ADD( src_sd->bonus_damage );

		wd.damage += src_sd->star;
		wd.damage += src_sd->ranker_weapon_bonus;
		if(calc_flag.lh) {
			wd.damage2 += src_sd->star_;
			wd.damage2 += src_sd->ranker_weapon_bonus_;
		}
	}
	/* 29�D�K���Œ�_���[�W */
	if(src_sd && src_sd->special_state.fix_damage)
		DMG_SET( src_sd->fix_damage );

	/* 30�D����_���[�W�̕␳ */
	if(calc_flag.rh == 0 && calc_flag.lh == 1) {	// ����̂ݕ��푕��
		wd.damage = wd.damage2;
		wd.damage2 = 0;
		// �ꉞ���E�����ւ��Ă���
		calc_flag.rh = 1;
		calc_flag.lh = 0;
	} else if(src_sd && calc_flag.lh) {		// ���肪����Ȃ�E��E����C���̓K�p
		int dmg = wd.damage, dmg2 = wd.damage2;
		// �E��C��(60% �` 100%) �E��S��
		skill = pc_checkskill(src_sd,AS_RIGHT);
		wd.damage = wd.damage * (50 + (skill * 10))/100;
		if(dmg > 0 && wd.damage < 1) wd.damage = 1;
		// ����C��(40% �` 80%) ����S��
		skill = pc_checkskill(src_sd,AS_LEFT);
		wd.damage2 = wd.damage2 * (30 + (skill * 10))/100;
		if(dmg2 > 0 && wd.damage2 < 1) wd.damage2 = 1;
	} else {
		wd.damage2 = 0;	// �O�̂���0�𖾎����Ă���
	}

	// �E��,�Z���̂�
	if(calc_flag.da > 0) {
		wd.type = 0x08;
		switch (calc_flag.da) {
			case 1:		// �_�u���A�^�b�N
				wd.div_ = 2;
				wd.damage += wd.damage;
				break;
			case 2:		// �O�i��
				wd.div_ = 255;
				break;
			case 3:		// �t�F�I���`���M
			case 4:		// �l�����`���M
			case 5:		// �g�������`���M
			case 6:		// �A�v�`���I�����M
				wd.div_ = 248+calc_flag.da;
				break;
		}
	}

	/* 31�D�X�L���C���T�i�ǉ��_���[�W�Q�j */
	if(src_sd && src_sd->status.weapon == WT_KATAR && skill_num != ASC_BREAKER) {
		// �J�^�[������
		if((skill = pc_checkskill(src_sd,ASC_KATAR)) > 0) {
			wd.damage += wd.damage*(10+(skill * 2))/100;
		}
		// �J�^�[���ǌ��_���[�W
		skill = pc_checkskill(src_sd,TF_DOUBLE);
		wd.damage2 = wd.damage * (1 + (skill * 2))/100;
		if(wd.damage > 0 && wd.damage2 < 1)
			wd.damage2 = 1;
	}
	if(skill_num == TF_POISON) {
		wd.damage = battle_attr_fix(wd.damage + 15*skill_lv, s_ele, status_get_element(target) );
	}
	if(skill_num == MC_CARTREVOLUTION) {
		wd.damage = battle_attr_fix(wd.damage, ELE_NEUTRAL, status_get_element(target) );
	}

	/* 32�D���S����̔��� */
	if(skill_num == 0 && skill_lv >= 0 && target_sd != NULL && wd.div_ < 255 && atn_rand()%1000 < status_get_flee2(target) ) {
		wd.damage  = 0;
		wd.damage2 = 0;
		wd.type    = 0x0b;
		wd.dmg_lv  = ATK_LUCKY;
	}

	// �Ώۂ����S���������ݒ肪ON�Ȃ�
	if(battle_config.enemy_perfect_flee) {
		if(skill_num == 0 && skill_lv >= 0 && target_md != NULL && wd.div_ < 255 && atn_rand()%1000 < status_get_flee2(target) ) {
			wd.damage  = 0;
			wd.damage2 = 0;
			wd.type    = 0x0b;
			wd.dmg_lv  = ATK_LUCKY;
		}
	}

	/* 33�D�Œ�_���[�W2 */
	if(t_mode&0x40) {	// Mob��Mode�Ɋ拭�t���O�������Ă���Ƃ��̏���
		if(wd.damage > 0)
			wd.damage = (wd.div_ < 255)? 1: 3;	// �O�i���̂�3�_���[�W
		if(wd.damage2 > 0)
			wd.damage2 = 1;
	}

	// bNoWeaponDamage�ŃO�����h�N���X����Ȃ��ꍇ�̓_���[�W��0
	if( target_sd && target_sd->special_state.no_weapon_damage && skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS)
		wd.damage = wd.damage2 = 0;

	/* 34�D�_���[�W�ŏI�v�Z */
	if(skill_num != CR_GRANDCROSS && skill_num != NPC_GRANDDARKNESS) {
		if(wd.damage2 < 1) {		// �_���[�W�ŏI�C��
			wd.damage  = battle_calc_damage(src,target,wd.damage,wd.div_,skill_num,skill_lv,wd.flag);
		} else if(wd.damage < 1) {	// �E�肪�~�X�H
			wd.damage2 = battle_calc_damage(src,target,wd.damage2,wd.div_,skill_num,skill_lv,wd.flag);
		} else {			// ����A�J�^�[���̏ꍇ�͂�����ƌv�Z��₱����
			int dmg = wd.damage+wd.damage2;
			wd.damage  = battle_calc_damage(src,target,dmg,wd.div_,skill_num,skill_lv,wd.flag);
			wd.damage2 = (wd.damage2*100/dmg)*wd.damage/100;
			if(wd.damage > 1 && wd.damage2 < 1) wd.damage2 = 1;
			wd.damage -= wd.damage2;
		}
	}

	/* 35�D�����U���X�L���ɂ��I�[�g�X�y������(item_bonus) */
	if(wd.flag&BF_SKILL && src && src->type == BL_PC && src != target && (wd.damage+wd.damage2) > 0)
	{
		unsigned long asflag = EAS_ATTACK;
		if(skill_num == AM_DEMONSTRATION) {
			asflag += EAS_MISC;
		} else {
			if(wd.flag&BF_LONG)
				asflag += EAS_LONG;
			else
				asflag += EAS_SHORT;
		}
		if(battle_config.weapon_attack_autospell)
			asflag += EAS_NORMAL;
		else
			asflag += EAS_SKILL;

		skill_bonus_autospell(src,target,asflag,gettick(),0);
	}

	/* 36�D���z�ƌ��Ɛ��̗Z�� HP2%���� */
	if(src_sd && sc && sc->data[SC_FUSION].timer != -1)
	{
		int hp;
		if(target->type == BL_PC) {
			hp = src_sd->status.max_hp * 8 / 100;
			if( src_sd->status.hp < (src_sd->status.max_hp * 20 / 100))	// �Ώۂ��v���C���[��HP��20�������ł��鎞�A�U��������Α������܂��B
				hp = src_sd->status.hp;
		} else {
			hp = src_sd->status.max_hp * 2 / 100;
		}
		pc_heal(src_sd,-hp,0);
	}

	/* 37�D�J�A�q */
	if(skill_num == 0 && wd.flag&BF_WEAPON && t_sc && t_sc->data[SC_KAAHI].timer != -1)
	{
		int kaahi_lv = t_sc->data[SC_KAAHI].val1;
		if(status_get_hp(target) < status_get_max_hp(target))
		{
			if(target->type == BL_MOB || status_get_sp(target) > 5*kaahi_lv)	// �Ώۂ�mob�ȊO��SP�������ʈȉ��̂Ƃ��͔������Ȃ�
			{
				int heal = skill_fix_heal(target, SL_KAAHI, 200 * kaahi_lv);
				unit_heal(target,heal,-5*kaahi_lv);
				if(target_sd)
					clif_misceffect3(target_sd->fd, target_sd->bl.id, 7);	// �񕜂����{�l�ɂ̂݉񕜃G�t�F�N�g
			}
		}
	}

	/* 38�D���z�ƌ��Ɛ��̊�� */
	if(src_sd && wd.flag&BF_WEAPON && pc_checkskill(src_sd,SG_FEEL) > 2 && atn_rand()%10000 < battle_config.sg_miracle_rate)
		status_change_start(src,SC_MIRACLE,1,0,0,0,3600000,0);

	/* 39�D�v�Z���ʂ̍ŏI�␳ */
	if(!calc_flag.lh)
		wd.damage2 = 0;
	wd.amotion = status_get_amotion(src);
	if(skill_num == KN_AUTOCOUNTER)
		wd.amotion >>= 1;
	wd.dmotion = status_get_dmotion(target);

	return wd;
}

#define MATK_FIX( a,b ) { matk1 = matk1*(a)/(b); matk2 = matk2*(a)/(b); }

/*==========================================
 * ���@�_���[�W�v�Z
 *------------------------------------------
 */
static struct Damage battle_calc_magic_attack(struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	struct Damage mgd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct map_session_data *sd   = NULL, *tsd = NULL;
	struct mob_data         *tmd  = NULL;
	struct homun_data       *thd  = NULL;
	struct merc_data        *tmcd = NULL;
	struct status_change    *sc   = NULL, *t_sc = NULL;
	int matk1, matk2, ele, race;
	int mdef1, mdef2, t_ele, t_race, t_enemy, t_mode;
	int t_class, cardfix, i;
	int normalmagic_flag = 1;

	// return�O�̏���������̂ŏ��o�͕��̂ݕύX
	if( bl == NULL || target == NULL || target->type == BL_PET ) {
		nullpo_info(NLP_MARK);
		return mgd;
	}

	sd = BL_DOWNCAST( BL_PC, bl );

	tsd  = BL_DOWNCAST( BL_PC,   target );
	tmd  = BL_DOWNCAST( BL_MOB,  target );
	thd  = BL_DOWNCAST( BL_HOM,  target );
	tmcd = BL_DOWNCAST( BL_MERC, target );

	// �A�^�b�J�[
	matk1 = status_get_matk1(bl);
	matk2 = status_get_matk2(bl);
	ele   = skill_get_pl(skill_num);
	race  = status_get_race(bl);
	sc    = status_get_sc(bl);

	// �^�[�Q�b�g
	mdef1   = status_get_mdef(target);
	mdef2   = status_get_mdef2(target);
	t_ele   = status_get_elem_type(target);
	t_race  = status_get_race(target);
	t_enemy = status_get_enemy_type(target);
	t_mode  = status_get_mode(target);
	t_sc    = status_get_sc(target);

	if(sd) {
		sd->state.attack_type = BF_MAGIC;
		if(sd->matk_rate != 100)
			MATK_FIX( sd->matk_rate, 100 );
		sd->state.arrow_atk = 0;
	}

	/* �P�Dmgd�\���̂̏����ݒ� */
	mgd.div_      = skill_get_num(skill_num,skill_lv);
	mgd.blewcount = skill_get_blewcount(skill_num,skill_lv);
	mgd.flag      = BF_MAGIC|BF_LONG|BF_SKILL;

	if(battle_config.calc_dist_flag & 2) {	// ���@�̎��v�Z���邩�H &2�Ōv�Z����
		int target_dist = unit_distance2(bl,target);	// �������擾
		if(target_dist < battle_config.allow_sw_dist) {				// SW�Ŗh���鋗����菬�������ߋ�������̍U��
			if(bl->type == BL_PC && battle_config.sw_def_type & 1)		// �l�Ԃ���̂𔻒肷�邩�@&1�ł���
				mgd.flag = (mgd.flag&~BF_RANGEMASK)|BF_SHORT;		// �ߋ����ɐݒ�
			else if(bl->type == BL_MOB && battle_config.sw_def_type & 2)	// �����X�^�[����̖��@�𔻒肷�邩�@&2�ł���
				mgd.flag = (mgd.flag&~BF_RANGEMASK)|BF_SHORT;		// �ߋ����ɐݒ�
		}
	}

	/* �Q�D���@�͑����ɂ��MATK���� */
	if (sc && sc->data[SC_MAGICPOWER].timer != -1) {
		matk1 += (matk1 * sc->data[SC_MAGICPOWER].val1 * 5)/100;
		matk2 += (matk2 * sc->data[SC_MAGICPOWER].val1 * 5)/100;
	}

	/* �R�D��{�_���[�W�v�Z(�X�L�����Ƃɏ���) */
	switch(skill_num)
	{
		case AL_HEAL:	// �q�[��or����
		case PR_BENEDICTIO:
			mgd.damage = skill_calc_heal(bl,skill_lv)/2;
			if(sd)	// ���f�B�^�e�B�I���悹��
				mgd.damage += mgd.damage * pc_checkskill(sd,HP_MEDITATIO)*2/100;
			normalmagic_flag = 0;
			break;
		case PR_ASPERSIO:		// �A�X�y���V�I
			mgd.damage = 40;	// �Œ�_���[�W
			normalmagic_flag = 0;
			break;
		case PR_SANCTUARY:	// �T���N�`���A��
			ele = ELE_HOLY;
			mgd.damage = (skill_lv > 6)? 388: 50*skill_lv;
			normalmagic_flag = 0;
			mgd.blewcount |= 0x10000;
			break;
		case PA_GOSPEL:		// �S�X�y��(�����_���_���[�W����̏ꍇ)
			mgd.damage = 1000+atn_rand()%9000;
			normalmagic_flag = 0;
			break;
		case ALL_RESURRECTION:
		case PR_TURNUNDEAD:	// �U�����U���N�V�����ƃ^�[���A���f�b�h
			if(battle_check_undead(t_race,t_ele)) {
				int hp, mhp, thres;
				hp = status_get_hp(target);
				mhp = status_get_max_hp(target);
				thres = skill_lv * 20 + status_get_luk(bl) + status_get_int(bl) + status_get_lv(bl) + 200 - (hp * 200 / mhp);
				if(thres > 700)
					thres = 700;
				if(atn_rand()%1000 < thres && !(t_mode&0x20))	// ����
					mgd.damage = hp;
				else					// ���s
					mgd.damage = status_get_lv(bl) + status_get_int(bl) + skill_lv * 10;
			}
			normalmagic_flag = 0;
			break;

		case HW_NAPALMVULCAN:	// �i�p�[���o���J��
		case MG_NAPALMBEAT:	// �i�p�[���r�[�g�i���U�v�Z���݁j
			MATK_FIX( 70+10*skill_lv, 100 );
			if(flag > 0) {
				MATK_FIX( 1, flag );
			} else {
				if(battle_config.error_log)
					printf("battle_calc_magic_attack: NAPALM enemy count=0 !\n");
			}
			break;
		case MG_SOULSTRIKE:			// �\�E���X�g���C�N�i�΃A���f�b�h�_���[�W�␳�j
			if(battle_check_undead(t_race,t_ele))
				MATK_FIX( 20+skill_lv, 20 );	// MATK�ɕ␳����ʖڂł����ˁH
			break;
		case MG_FIREBALL:	// �t�@�C���[�{�[��
			if(flag > 2) {
				matk1 = matk2 = 0;
			} else {
				MATK_FIX( 70+10*skill_lv, 100 );
				if(flag == 2)
					MATK_FIX( 3, 4 );
			}
			break;
		case MG_FIREWALL:	// �t�@�C���[�E�H�[��
			if((t_ele == ELE_FIRE || battle_check_undead(t_race,t_ele)) && target->type != BL_PC)
				mgd.blewcount = 0;
			else
				mgd.blewcount |= 0x10000;
			MATK_FIX( 1, 2 );
			break;
		case MG_THUNDERSTORM:	// �T���_�[�X�g�[��
			MATK_FIX( 80, 100 );
			break;
		case MG_FROSTDIVER:	// �t���X�g�_�C�o
			MATK_FIX( 100+10*skill_lv, 100 );
			break;
		case WZ_FROSTNOVA:	// �t���X�g�m���@
			MATK_FIX( 200+20*skill_lv, 300 );
			break;
		case WZ_FIREPILLAR:	// �t�@�C���[�s���[
			if(mdef1 < 1000000)
				mdef1 = mdef2 = 0;	// MDEF����
			if(bl->type != BL_MOB)
				MATK_FIX( 1, 5 );
			matk1 += 50;
			matk2 += 50;
			break;
		case WZ_SIGHTRASHER:
			MATK_FIX( 100+20*skill_lv, 100);
			break;
		case WZ_METEOR:
		case WZ_JUPITEL:	// ���s�e���T���_�[
		case NPC_DARKTHUNDER:	// �_�[�N�T���_�[
			break;
		case WZ_VERMILION:	// ���[�h�I�u�o�[�~���I��
			MATK_FIX( 80+20*skill_lv, 100 );
			break;
		case WZ_WATERBALL:	// �E�H�[�^�[�{�[��
			MATK_FIX( 100+30*skill_lv, 100 );
			break;
		case WZ_STORMGUST:	// �X�g�[���K�X�g
			MATK_FIX( 100+40*skill_lv, 100 );
			//mgd.blewcount |= 0x10000;
			break;
		case AL_HOLYLIGHT:	// �z�[���[���C�g
			MATK_FIX( 125, 100 );
			if(sc && sc->data[SC_PRIEST].timer != -1) {
				MATK_FIX( 500, 100 );
			}
			break;
		case AL_RUWACH:
			MATK_FIX( 145, 100 );
			break;
		case WZ_SIGHTBLASTER:
			MATK_FIX( 145, 100 );
			break;
		case SL_STIN:	// �G�X�e�B��
			if(status_get_size(target) == 0) {
				MATK_FIX( 10*skill_lv, 100 );
			} else {
				MATK_FIX( skill_lv, 100 );
			}
			if(skill_lv >= 7)
				status_change_start(bl,SC_SMA,skill_lv,0,0,0,3000,0);
			break;
		case SL_STUN:	// �G�X�^��
			MATK_FIX( 5*skill_lv, 100 );
			ele = status_get_attack_element(bl);
			if(skill_lv >= 7)
				status_change_start(bl,SC_SMA,skill_lv,0,0,0,3000,0);
			break;
		case SL_SMA:	// �G�X�}
			MATK_FIX( 40+sd->status.base_level, 100 );
			ele = status_get_attack_element_nw(bl);
			if(sc && sc->data[SC_SMA].timer != -1)
				status_change_end(bl,SC_SMA,-1);
			break;
		case NJ_KOUENKA:	// �g����
			MATK_FIX( 90, 100 );
			break;
		case NJ_KAENSIN:	// �Ή��w
			MATK_FIX( 50, 100 );
			break;
		case NJ_HUUJIN:		// ���n
			break;
		case NJ_HYOUSENSOU:	// �X�M��
			if(t_sc && t_sc->data[SC_SUITON].timer != -1) {
				MATK_FIX( 70+2*t_sc->data[SC_SUITON].val1, 100 );
			} else {
				MATK_FIX( 70, 100 );
			}
			break;
		case NJ_BAKUENRYU:	// �����w
			MATK_FIX( 50+50*skill_lv, 100 );
			break;
		case NJ_HYOUSYOURAKU:	// �X������
			MATK_FIX( 100+50*skill_lv, 100 );
			break;
		case NJ_RAIGEKISAI:	// ������
			MATK_FIX( 160+40*skill_lv, 100 );
			break;
		case NJ_KAMAITACHI:	// ��
			MATK_FIX( 100+100*skill_lv,100 );
			break;
		case NPC_EVILLAND:	// �C�r�������h
			mgd.damage = (skill_lv > 6)? 666: skill_lv*100;
			normalmagic_flag = 0;
			break;
	}

	/* �S�D��ʖ��@�_���[�W�v�Z */
	if(normalmagic_flag) {
		int imdef_flag = 0;
		if(matk1 > matk2)
			mgd.damage = matk2+atn_rand()%(matk1-matk2+1);
		else
			mgd.damage = matk2;
		if(sd) {
			int mask = (1<<t_race) | ( (t_mode&0x20)? (1<<RCT_BOSS): (1<<RCT_NONBOSS) );
			if(sd->ignore_mdef_ele & (1<<t_ele) || sd->ignore_mdef_race & mask || sd->ignore_mdef_enemy & (1<<t_enemy))
				imdef_flag = 1;
		}
		if(!imdef_flag) {
			if(battle_config.magic_defense_type) {
				mgd.damage = mgd.damage - (mdef1 * battle_config.magic_defense_type) - mdef2;
			} else {
				mgd.damage = (mgd.damage*(100-mdef1))/100 - mdef2;
			}
		}
		if(mgd.damage < 1)	// �v���C���[�̖��@�X�L����1�_���[�W�ۏؖ���
			mgd.damage = (!battle_config.skill_min_damage && bl->type == BL_PC)? 0: 1;
	}

	/* �T�D�J�[�h�ɂ��_���[�W�ǉ����� */
	if(sd && mgd.damage > 0) {
		cardfix = 100;
		cardfix = cardfix*(100+sd->magic_addrace[t_race])/100;
		cardfix = cardfix*(100+sd->magic_addele[t_ele])/100;
		cardfix = cardfix*(100+sd->magic_addenemy[t_enemy])/100;
		if(t_mode & 0x20)
			cardfix = cardfix*(100+sd->magic_addrace[RCT_BOSS])/100;
		else
			cardfix = cardfix*(100+sd->magic_addrace[RCT_NONBOSS])/100;
		t_class = status_get_class(target);
		for(i=0; i<sd->add_magic_damage_class_count; i++) {
			if(sd->add_magic_damage_classid[i] == t_class) {
				cardfix = cardfix*(100+sd->add_magic_damage_classrate[i])/100;
				break;
			}
		}
		// �J�[�h���ʂɂ�����X�L���̃_���[�W�����i���@�X�L���j
		if(sd->skill_dmgup.count > 0 && skill_num > 0) {
			for(i=0; i<sd->skill_dmgup.count; i++) {
				if(skill_num == sd->skill_dmgup.id[i]) {
					cardfix = cardfix*(100+sd->skill_dmgup.rate[i])/100;
					break;
				}
			}
		}
		mgd.damage = mgd.damage*cardfix/100;
	}

	/* �U�D�J�[�h�ɂ��_���[�W�������� */
	if(tsd && mgd.damage > 0) {
		int s_class = status_get_class(bl);
		cardfix = 100;
		cardfix = cardfix*(100-tsd->subele[ele])/100;				// �����ɂ��_���[�W�ϐ�
		cardfix = cardfix*(100-tsd->subrace[race])/100;				// �푰�ɂ��_���[�W�ϐ�
		cardfix = cardfix*(100-tsd->subenemy[status_get_enemy_type(bl)])/100;	// �G�^�C�v�ɂ��_���[�W�ϐ�
		cardfix = cardfix*(100-tsd->subsize[status_get_size(bl)])/100;		// �T�C�Y�ɂ��_���[�W�ϐ�
		cardfix = cardfix*(100-tsd->magic_subrace[race])/100;
		if(status_get_mode(bl) & 0x20)
			cardfix = cardfix*(100-tsd->magic_subrace[RCT_BOSS])/100;
		else
			cardfix = cardfix*(100-tsd->magic_subrace[RCT_NONBOSS])/100;
		for(i=0; i<tsd->add_mdef_class_count; i++) {
			if(tsd->add_mdef_classid[i] == s_class) {
				cardfix = cardfix*(100-tsd->add_mdef_classrate[i])/100;
				break;
			}
		}
		cardfix = cardfix*(100-tsd->magic_def_rate)/100;
		mgd.damage = mgd.damage*cardfix/100;
	}

	if(mgd.damage < 0)
		mgd.damage = 0;

	/* �V�D�����̓K�p */
	mgd.damage = battle_attr_fix(mgd.damage, ele, status_get_element(target));

	/* �W�D�X�L���C���P */
	if(skill_num == CR_GRANDCROSS || skill_num == NPC_GRANDDARKNESS) {	// �O�����h�N���X
		static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		wd = battle_calc_weapon_attack(bl,target,skill_num,skill_lv,flag);
		mgd.damage = (mgd.damage + wd.damage) * (100 + 40*skill_lv)/100;
		if(battle_config.gx_dupele)
			mgd.damage = battle_attr_fix(mgd.damage, ele, status_get_element(target));	// ����2�񂩂���
		if(bl == target) {
			if(bl->type == BL_MOB || bl->type == BL_HOM || bl->type == BL_MERC)
				mgd.damage = 0;		// MOB,HOM,MERC���g���ꍇ�͔�������
		} else {
			 mgd.damage /= 2;	// �����͔���
		}
	}

	if(skill_num == WZ_WATERBALL)
		mgd.div_ = 1;

	/* �X�D�ΏۂɃX�e�[�^�X�ُ킪����ꍇ */
	if(tsd && tsd->special_state.no_magic_damage)
		mgd.damage = 0;	// ����峃J�[�h�i���@�_���[�W�O)

	if(t_sc && t_sc->data[SC_HERMODE].timer != -1 && t_sc->data[SC_HERMODE].val1 == 1)	// �w�����[�h�Ȃ疂�@�_���[�W�Ȃ�
		mgd.damage = 0;

	/* 10�D�Œ�_���[�W */
	if(skill_num == HW_GRAVITATION)	// �O���r�e�[�V�����t�B�[���h
		mgd.damage = 200+200*skill_lv;

	/* 11�D�q�b�g�񐔂ɂ��_���[�W�{�� */
	if(mgd.damage != 0) {
		if(t_mode&0x40) { // ���E���̂���
			// ���[�h�I�u���@�[�~���I���̓m�[�_���[�W�B����ȊO�͘A�Ő��_���[�W
			if (!battle_config.skill_min_damage && skill_num == WZ_VERMILION)
				mgd.damage = 0;
			else
				mgd.damage = (mgd.div_ == 255)? 3: mgd.div_;
		}
		else if(mgd.div_ > 1 && skill_num != WZ_VERMILION) {
			mgd.damage *= mgd.div_;
		}
	}

	/* 12�D�_���[�W�ŏI�v�Z */
	mgd.damage = battle_calc_damage(bl,target,mgd.damage,mgd.div_,skill_num,skill_lv,mgd.flag);

	/* 13�D���@�ł��I�[�g�X�y������(item_bonus) */
	if(bl && bl->type == BL_PC && bl != target && mgd.damage > 0)
	{
		unsigned long asflag = EAS_ATTACK;
		if(battle_config.magic_attack_autospell)
			asflag += EAS_SHORT|EAS_LONG;
		else
			asflag += EAS_MAGIC;

		skill_bonus_autospell(bl,target,asflag,gettick(),0);
	}

	/* 14�D���@�ł�HP/SP��(�������Ȃ�) */
	if(battle_config.magic_attack_drain && bl != target)
		battle_attack_drain(bl,mgd.damage,0,battle_config.magic_attack_drain_enable_type);

	/* 15�D�v�Z���ʂ̍ŏI�␳ */
	mgd.amotion = status_get_amotion(bl);
	mgd.dmotion = status_get_dmotion(target);
	mgd.damage2 = 0;
	mgd.type    = 0;

	return mgd;
}

/*==========================================
 * ���̑��_���[�W�v�Z
 *------------------------------------------
 */
static struct Damage battle_calc_misc_attack(struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	struct Damage mid = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct map_session_data *sd = NULL, *tsd = NULL;
	struct skill_unit       *unit = NULL;
	int int_, dex, race, ele;
	int skill;
	int damagefix = 1;

	// return�O�̏���������̂ŏ��o�͕��̂ݕύX
	if( bl == NULL || target == NULL || target->type == BL_PET ) {
		nullpo_info(NLP_MARK);
		return mid;
	}

	// �O���E���h�h���t�g�̂Ƃ���bl��ݒu�҂ɒu������
	if(bl->type == BL_SKILL) {
		unit = (struct skill_unit *)bl;
		if(unit && unit->group)
			bl = map_id2bl(unit->group->src_id);
	}

	sd  = BL_DOWNCAST( BL_PC, bl );
	tsd = BL_DOWNCAST( BL_PC, target );

	// �A�^�b�J�[
	int_ = status_get_int(bl);
	dex  = status_get_dex(bl);
	race = status_get_race(bl);
	ele  = skill_get_pl(skill_num);

	if(sd) {
		sd->state.attack_type = BF_MISC;
		sd->state.arrow_atk = 0;
	}

	/* �P�Dmid�\���̂̏����ݒ� */
	mid.div_      = skill_get_num(skill_num,skill_lv);
	mid.blewcount = skill_get_blewcount(skill_num,skill_lv);
	mid.flag      = BF_MISC|BF_SHORT|BF_SKILL;

	/* �Q�D��{�_���[�W�v�Z(�X�L�����Ƃɏ���) */
	switch(skill_num)
	{
	case HT_LANDMINE:	// �����h�}�C��
	case MA_LANDMINE:
		mid.damage = skill_lv*(dex+75)*(100+int_)/100;
		break;

	case HT_BLASTMINE:	// �u���X�g�}�C��
		mid.damage = skill_lv*(dex/2+50)*(100+int_)/100;
		break;

	case HT_CLAYMORETRAP:	// �N���C���A�[�g���b�v
		mid.damage = skill_lv*(dex/2+75)*(100+int_)/100;
		break;

	case HT_BLITZBEAT:	// �u���b�c�r�[�g
		if(sd == NULL || (skill = pc_checkskill(sd,HT_STEELCROW)) <= 0)
			skill = 0;
		mid.damage = (dex/10 + int_/2 + skill*3 + 40)*2;
		if(flag > 1)
			mid.damage /= flag;
		flag &= ~(BF_SKILLMASK|BF_RANGEMASK|BF_WEAPONMASK);
		mid.flag = flag|(mid.flag&~BF_RANGEMASK)|BF_LONG;
		break;

	case TF_THROWSTONE:	// �Γ���
		mid.damage = 50;
		damagefix = 0;
		flag &= ~(BF_SKILLMASK|BF_RANGEMASK|BF_WEAPONMASK);
		mid.flag = flag|(mid.flag&~BF_RANGEMASK)|BF_LONG;
		break;

	case BA_DISSONANCE:	// �s���a��
		mid.damage = (skill_lv)*20+pc_checkskill(sd,BA_MUSICALLESSON)*3;
		break;
	case NPC_SELFDESTRUCTION:	// ����
	case NPC_SELFDESTRUCTION2:	// ����2
		mid.damage = status_get_hp(bl)-((bl == target)? 1: 0);
		damagefix = 0;
		break;

	case NPC_DARKBREATH:
		{
			struct status_change *t_sc = status_get_sc(target);
			int hitrate = status_get_hit(bl) - status_get_flee(target) + 80;
			int t_hp = status_get_hp(target);
			hitrate = (hitrate > 95)? 95: (hitrate < 5)? 5: hitrate;
			if(t_sc && (t_sc->data[SC_SLEEP].timer != -1 || t_sc->data[SC_STUN].timer != -1 ||
				t_sc->data[SC_FREEZE].timer != -1 || (t_sc->data[SC_STONE].timer != -1 && t_sc->data[SC_STONE].val2 == 0) ) )
				hitrate = 1000000;
			if(atn_rand()%100 < hitrate)
				mid.damage = t_hp*(skill_lv*6)/100;
		}
		break;
	case PA_PRESSURE:		// �v���b�V���[
		mid.damage = 500 + 300 * skill_lv;
		damagefix = 0;
		break;
	case SN_FALCONASSAULT:		// �t�@���R���A�T���g
		if(sd == NULL || (skill = pc_checkskill(sd,HT_STEELCROW)) <= 0)
			skill = 0;
		mid.damage = ((dex/10+int_/2+skill*3+40)*2*(150+skill_lv*70)/100)*5;
		if(sd && battle_config.allow_falconassault_elemet)
			ele = sd->atk_ele;
		flag &= ~(BF_WEAPONMASK|BF_RANGEMASK|BF_WEAPONMASK);
		mid.flag = flag|(mid.flag&~BF_RANGEMASK)|BF_LONG;
		break;
	case GS_GROUNDDRIFT:		// �O���E���h�h���t�g
		if(unit && unit->group)
		{
			const int ele_type[5] = { ELE_WIND, ELE_DARK, ELE_POISON, ELE_WATER, ELE_FIRE };
			ele = ele_type[unit->group->unit_id - UNT_GROUNDDRIFT_WIND];
			mid.damage = status_get_baseatk(bl);
		}
		break;
	case HVAN_EXPLOSION:		// �o�C�I�G�N�X�v���[�W����
		mid.damage = status_get_hp(bl)*(50+50*skill_lv)/100;
		break;
	default:
		mid.damage = status_get_baseatk(bl);
		break;
	}

	if(damagefix) {
		if(mid.damage < 1 && skill_num != NPC_DARKBREATH)
			mid.damage = 1;

		/* �R�D�J�[�h�ɂ��_���[�W�������� */
		if(tsd && mid.damage > 0) {
			int cardfix = 100;
			cardfix = cardfix*(100-tsd->subele[ele])/100;	// �����ɂ��_���[�W�ϐ�
			cardfix = cardfix*(100-tsd->subrace[race])/100;	// �푰�ɂ��_���[�W�ϐ�
			cardfix = cardfix*(100-tsd->subenemy[status_get_enemy_type(bl)])/100;	// �G�^�C�v�ɂ��_���[�W�ϐ�
			cardfix = cardfix*(100-tsd->subsize[status_get_size(bl)])/100;	// �T�C�Y�ɂ��_���[�W�ϐ�
			cardfix = cardfix*(100-tsd->misc_def_rate)/100;
			mid.damage = mid.damage*cardfix/100;
		}
		if(mid.damage < 0)
			mid.damage = 0;

		/* �S�D�����̓K�p */
		mid.damage = battle_attr_fix(mid.damage, ele, status_get_element(target));

		/* �T�D�X�L���C�� */
		if(skill_num == GS_GROUNDDRIFT) {	// �Œ�_���[�W�����Z���Ă���ɖ������Ƃ��đ����v�Z����
			mid.damage += skill_lv*50;
			mid.damage = battle_attr_fix(mid.damage, ELE_NEUTRAL, status_get_element(target));
		}

	}

	/* �U�D�q�b�g�񐔂ɂ��_���[�W�{�� */
	if(mid.div_ > 1)
		mid.damage *= mid.div_;
	if( mid.damage > 0 && (mid.damage < mid.div_ || (status_get_def(target) >= 1000000 && status_get_mdef(target) >= 1000000)) ) {
		mid.damage = mid.div_;
	}

	/* �V�D�Œ�_���[�W */
	if(status_get_mode(target)&0x40 && mid.damage > 0)	// ���E���̂���
		mid.damage = 1;

	/* �W�D�J�[�h�ɂ��_���[�W�ǉ����� */
	if(sd && sd->skill_dmgup.count > 0 && skill_num > 0 && mid.damage > 0) {	// �J�[�h���ʂɂ�����X�L���̃_���[�W����
		int i;
		for(i=0; i<sd->skill_dmgup.count; i++) {
			if(skill_num == sd->skill_dmgup.id[i]) {
				mid.damage += mid.damage * sd->skill_dmgup.rate[i] / 100;
				break;
			}
		}
	}

	/* �X�D�_���[�W�ŏI�v�Z */
	mid.damage = battle_calc_damage(bl,target,mid.damage,mid.div_,skill_num,skill_lv,mid.flag);

	/* 10�Dmisc�ł��I�[�g�X�y������(bonus) */
	if(bl->type == BL_PC && bl != target && mid.damage > 0)
	{
		unsigned long asflag = EAS_ATTACK;
		if(battle_config.misc_attack_autospell)
			asflag += EAS_SHORT|EAS_LONG;
		else
			asflag += EAS_MISC;

		skill_bonus_autospell(bl,target,asflag,gettick(),0);
	}

	/* 11�Dmisc�ł�HP/SP��(�������Ȃ�) */
	if(battle_config.misc_attack_drain && bl != target)
		battle_attack_drain(bl,mid.damage,0,battle_config.misc_attack_drain_enable_type);

	/* 12�D�v�Z���ʂ̍ŏI�␳ */
	mid.amotion = status_get_amotion(bl);
	mid.dmotion = status_get_dmotion(target);
	mid.damage2 = 0;
	mid.type    = 0;

	return mid;
}

/*==========================================
 * �_���[�W�v�Z�ꊇ�����p
 *------------------------------------------
 */
static struct Damage battle_calc_attack(int attack_type,struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	switch(attack_type) {
		case BF_WEAPON:
			return battle_calc_weapon_attack(bl,target,skill_num,skill_lv,flag);
		case BF_MAGIC:
			return battle_calc_magic_attack(bl,target,skill_num,skill_lv,flag);
		case BF_MISC:
			return battle_calc_misc_attack(bl,target,skill_num,skill_lv,flag);
		default:
			if(battle_config.error_log)
				printf("battle_calc_attack: unknwon attack type ! %d\n",attack_type);
			break;
	}
	return wd;
}

/*==========================================
 * �ʏ�U�������܂Ƃ�
 *------------------------------------------
 */
int battle_weapon_attack( struct block_list *src,struct block_list *target,unsigned int tick,int flag)
{
	struct map_session_data *sd = NULL, *tsd = NULL;
	struct status_change *sc, *t_sc;
	int damage,rdamage = 0;
	static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	if(src->prev == NULL || target->prev == NULL)
		return 0;
	if(unit_isdead(src) || unit_isdead(target))
		return 0;

	sd  = BL_DOWNCAST( BL_PC, src );
	tsd = BL_DOWNCAST( BL_PC, target );

	sc   = status_get_sc(src);
	t_sc = status_get_sc(target);

	if(sc && sc->opt1 > 0) {
		unit_stopattack(src);
		return 0;
	}

	// ���������H�E�����ړ��E���f�̃E�B���N���̓_��
	if(sc && (sc->data[SC_BLADESTOP].timer != -1 || sc->data[SC_FORCEWALKING].timer != -1 || sc->data[SC_WINKCHARM].timer != -1)) {
		unit_stopattack(src);
		return 0;
	}
	// ���肪�����ړ���
	if(t_sc && t_sc->data[SC_FORCEWALKING].timer != -1) {
		unit_stopattack(src);
		return 0;
	}

	if(battle_check_target(src,target,BCT_ENEMY) <= 0 && !battle_check_range(src,target,0))
		return 0;	// �U���ΏۊO

	// �^�[�Q�b�g��MOB GM�n�C�h���ŁA�R���t�B�O�Ńn�C�h���U���s�� GM���x�����w����傫���ꍇ
	if(battle_config.hide_attack == 0 && target->type == BL_MOB && sd && sd->sc.option&0x40 && pc_isGM(sd) < battle_config.gm_hide_attack_lv)
		return 0;

	if(sd) {
		if(!battle_delarrow(sd,1,0))
			return 0;
	}

	if(flag&0x8000) {
		if(sd && battle_config.pc_attack_direction_change)
			sd->dir = sd->head_dir = map_calc_dir(src, target->x,target->y);
		else if(src->type == BL_MOB && battle_config.monster_attack_direction_change)
			((struct mob_data *)src)->dir = map_calc_dir(src, target->x,target->y);
		else if(src->type == BL_HOM && battle_config.monster_attack_direction_change)	// homun_attack_direction_change
			((struct homun_data *)src)->dir = map_calc_dir(src, target->x,target->y);
		else if(src->type == BL_MERC && battle_config.monster_attack_direction_change)	// merc_attack_direction_change
			((struct merc_data *)src)->dir = map_calc_dir(src, target->x,target->y);
		wd = battle_calc_weapon_attack(src,target,KN_AUTOCOUNTER,flag&0xff,0);
	} else {
		wd = battle_calc_weapon_attack(src,target,0,0,0);
	}

	if((damage = wd.damage + wd.damage2) > 0 && src != target) {
		if(wd.flag&BF_SHORT) {
			if(tsd && tsd->short_weapon_damage_return > 0) {
				rdamage += damage * tsd->short_weapon_damage_return / 100;
				if(rdamage < 1) rdamage = 1;
			}
			if(t_sc &&
			   t_sc->data[SC_REFLECTSHIELD].timer != -1 &&
			   (sd || t_sc->data[SC_DEVOTION].timer == -1))	// ��f�B�{�[�V�����҂Ȃ�PC����ȊO�͔������Ȃ�
			{
				rdamage += damage * t_sc->data[SC_REFLECTSHIELD].val2 / 100;
				if(rdamage < 1) rdamage = 1;
			}
		} else if(wd.flag&BF_LONG) {
			if(tsd && tsd->long_weapon_damage_return > 0) {
				rdamage += damage * tsd->long_weapon_damage_return / 100;
				if(rdamage < 1) rdamage = 1;
			}
		}
		if(rdamage > 0)
			clif_damage(src,src,tick,wd.amotion,wd.dmotion,rdamage,1,4,0);
	}

	if(wd.div_ == 255 && sd) {	// �O�i��
		int delay = 0;
		int skilllv;
		if(wd.damage+wd.damage2 < status_get_hp(target)) {
			if((skilllv = pc_checkskill(sd, MO_CHAINCOMBO)) > 0) {
				delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
				delay += 300 * battle_config.combo_delay_rate /100;
				// �R���{���͎��Ԃ̍Œ�ۏ�ǉ�
				if( delay < battle_config.combo_delay_lower_limits )
					delay = battle_config.combo_delay_lower_limits;
			}
			status_change_start(src,SC_COMBO,MO_TRIPLEATTACK,skilllv,0,0,delay,0);
		}
		sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
		clif_combo_delay(src,delay);
		clif_skill_damage(src , target , tick , wd.amotion , wd.dmotion ,
			wd.damage , 3 , MO_TRIPLEATTACK, pc_checkskill(sd,MO_TRIPLEATTACK) , -1 );

		// �N���[���X�L��
		if(wd.damage> 0 && tsd && pc_checkskill(tsd,RG_PLAGIARISM) && sc && sc->data[SC_PRESERVE].timer == -1) {
			skill_clone(tsd,MO_TRIPLEATTACK,pc_checkskill(sd, MO_TRIPLEATTACK));
		}
	} else if(wd.div_ >= 251 && wd.div_ <= 254 && sd) {	// �e�R���R��n��
		int delay = 0;
		int skillid = TK_STORMKICK + 2*(wd.div_-251);
		int skilllv;
		delay = status_get_adelay(src);
		if(wd.damage+wd.damage2 < status_get_hp(target)) {
			if((skilllv = pc_checkskill(sd, skillid)) > 0) {
				delay += 2000 - 4*status_get_agi(src) - 2*status_get_dex(src);
				// TK�R���{���͎��Ԃ̍Œ�ۏ�ǉ�
				if( delay < battle_config.tkcombo_delay_lower_limits )
					delay = battle_config.tkcombo_delay_lower_limits;
			}
			status_change_start(src,SC_TKCOMBO,skillid,skilllv,0,0,delay,0);
		}
		sd->ud.attackabletime = tick + delay;
		clif_skill_nodamage(&sd->bl,&sd->bl,skillid-1,pc_checkskill(sd,skillid-1),1);
	} else {
		clif_damage(src, target, tick, wd.amotion, wd.dmotion, wd.damage, wd.div_, wd.type, wd.damage2);

		// �񓁗�����ƃJ�^�[���ǌ��̃~�X�\��(�������`)
		if(sd && (sd->status.weapon > WT_HUUMA || sd->status.weapon == WT_KATAR) && wd.damage2 == 0)
			clif_damage(src, target, tick+10, wd.amotion, wd.dmotion, 0, 1, 0, 0);
	}
	if(sd && sd->splash_range > 0 && (wd.damage > 0 || wd.damage2 > 0))
		skill_castend_damage_id(src,target,0,-1,tick,0);

	map_freeblock_lock();
	battle_delay_damage(tick+wd.amotion,src,target,(wd.damage+wd.damage2),0,0,wd.flag);

	if(target->prev != NULL && !unit_isdead(target)) {
		if(wd.damage > 0 || wd.damage2 > 0) {
			skill_additional_effect(src,target,0,0,BF_WEAPON,tick);
			if(sd && tsd) {
				if(sd->break_weapon_rate > 0 && atn_rand()%10000 < sd->break_weapon_rate)
					pc_break_equip(tsd, EQP_WEAPON);
				if(sd->break_armor_rate > 0 && atn_rand()%10000 < sd->break_armor_rate)
					pc_break_equip(tsd, EQP_ARMOR);
			}
		}
	}
	if(sc && sc->data[SC_AUTOSPELL].timer != -1 && atn_rand()%100 < sc->data[SC_AUTOSPELL].val4) {
		int spellid = sc->data[SC_AUTOSPELL].val2;
		int spelllv = sc->data[SC_AUTOSPELL].val3;
		int r = atn_rand()%100;
		int sp = 0, fail = 0;

		if(r >= 50)
			spelllv -= 2;
		else if(r >= 15)
			spelllv--;
		if(spelllv < 1)
			spelllv = 1;

		if(sd) {
			if(sc->data[SC_SAGE].timer != -1)	// �Z�[�W�̍�
				spelllv = pc_checkskill(sd,spellid);
			sp = skill_get_sp(spellid,spelllv)*2/3;
			if(sd->status.sp < sp)
				fail = 1;
		}
		if(!fail) {
			if(battle_config.skill_autospell_delay_enable) {
				struct unit_data *ud = unit_bl2ud(src);
				if(ud) {
					int delay = skill_delayfix(src, skill_get_delay(spellid,spelllv), skill_get_cast(spellid,spelllv));
					ud->canact_tick = tick + delay;
				}
			}
			if(skill_get_inf(spellid) & 0x02) {
				fail = skill_castend_pos2(src,target->x,target->y,spellid,spelllv,tick,flag);
			} else {
				switch(skill_get_nk(spellid) & 3) {
				case 0:
				case 2:	/* �U���n */
					fail = skill_castend_damage_id(src,target,spellid,spelllv,tick,flag);
					break;
				case 1:	/* �x���n */
					if( (spellid == AL_HEAL || (spellid == ALL_RESURRECTION && target->type != BL_PC)) &&
					    battle_check_undead(status_get_race(target),status_get_elem_type(target)) )
						fail = skill_castend_damage_id(src,target,spellid,spelllv,tick,flag);
					else
						fail = skill_castend_nodamage_id(src,target,spellid,spelllv,tick,flag);
					break;
				}
			}
			if(sd && !fail)
				pc_heal(sd,0,-sp);
		}
	}

	// �J�[�h�ɂ��I�[�g�X�y��
	if(sd && src != target && (wd.damage > 0 || wd.damage2 > 0))
	{
		unsigned long asflag = EAS_ATTACK | EAS_NORMAL;
		if(wd.flag&BF_LONG)
			asflag += EAS_LONG;
		else
			asflag += EAS_SHORT;

		skill_bonus_autospell(src,target,asflag,gettick(),0);
	}

	if(sd && src != target && wd.flag&BF_WEAPON && (wd.damage > 0 || wd.damage2 > 0))
	{
		// SP����
		if(tsd && atn_rand()%100 < sd->sp_vanish_rate)
		{
			int sp = status_get_sp(target) * sd->sp_vanish_per/100;
			if(sp > 0)
				pc_heal(tsd, 0, -sp);
		}
	}

	if(sd && src != target && wd.flag&BF_WEAPON && (wd.damage > 0 || wd.damage2 > 0)) {
		// ���z���A���z���Ƃ���
		battle_attack_drain(src, wd.damage, wd.damage2, 3);
	}

	if(rdamage > 0) {
		battle_delay_damage(tick+wd.amotion,target,src,rdamage,0,0,0);

		// ���˃_���[�W�̃I�[�g�X�y��
		if(battle_config.weapon_reflect_autospell && target->type == BL_PC)
			skill_bonus_autospell(target,src,EAS_ATTACK,gettick(),0);

		if(battle_config.weapon_reflect_drain && src != target)
			battle_attack_drain(target,rdamage,0,battle_config.weapon_reflect_drain_enable_type);
	}

	// �ΏۂɃX�e�[�^�X�ُ킪����ꍇ
	if(t_sc && t_sc->count > 0)
	{
		if(t_sc->data[SC_AUTOCOUNTER].timer != -1 && t_sc->data[SC_AUTOCOUNTER].val4 > 0) {
			if(t_sc->data[SC_AUTOCOUNTER].val3 == src->id)
				battle_weapon_attack(target,src,tick,0x8000|t_sc->data[SC_AUTOCOUNTER].val1);
			status_change_end(target,SC_AUTOCOUNTER,-1);
		}
		if(t_sc->data[SC_BLADESTOP_WAIT].timer != -1 &&
		   !(status_get_mode(src)&0x20) &&
		   (map[target->m].flag.pvp || unit_distance2(src,target) <= 2)) {	// PvP�ȊO�ł̗L���˒���2�Z��
			int lv  = t_sc->data[SC_BLADESTOP_WAIT].val1;
			int sec = skill_get_time2(MO_BLADESTOP,lv);
			status_change_end(target,SC_BLADESTOP_WAIT,-1);
			status_change_start(src,SC_BLADESTOP,lv,1,src->id,target->id,sec,0);
			status_change_start(target,SC_BLADESTOP,lv,2,target->id,src->id,sec,0);
		}
		if(t_sc->data[SC_POISONREACT].timer != -1) {
			if( (src->type == BL_MOB && status_get_elem_type(src) == ELE_POISON) || status_get_attack_element(src) == ELE_POISON ) {
				// �ő���mob�܂��͓ő����ɂ��U���Ȃ�Δ���
				if( battle_check_range(target,src,status_get_range(target)+1) ) {
					t_sc->data[SC_POISONREACT].val2 = 0;
					battle_skill_attack(BF_WEAPON,target,target,src,AS_POISONREACT,t_sc->data[SC_POISONREACT].val1,tick,0);
				}
			} else {
				// ����ȊO�̒ʏ�U���ɑ΂���C���x�����i�ː��`�F�b�N�Ȃ��j
				--t_sc->data[SC_POISONREACT].val2;
				if(atn_rand()&1) {
					if( tsd == NULL || pc_checkskill(tsd,TF_POISON) >= 5 )
						battle_skill_attack(BF_WEAPON,target,target,src,TF_POISON,5,tick,flag);
				}
			}
			if(t_sc->data[SC_POISONREACT].val2 <= 0)
				status_change_end(target,SC_POISONREACT,-1);
		}
	}

	map_freeblock_unlock();
	return wd.dmg_lv;
}

/*=========================================================================
 * �X�L���U�����ʏ����܂Ƃ�
 * flag�̐����B16�i�}
 * 	0XYRTTff
 *  ff = battle_calc_attack�Ŋe��v�Z�ɗ��p
 *  TT = �p�P�b�g��type�����i0�Ńf�t�H���g�j
 *   R = �\��iskill_area_sub�Ŏg�p���ꂽBCT_*�j
 *   Y = �p�P�b�g�̃X�L��Lv�if�̂Ƃ���-1�ɕϊ��j
 *   X = �G�t�F�N�g�݂̂Ń_���[�W�Ȃ��t���O
 *-------------------------------------------------------------------------
 */
int battle_skill_attack(int attack_type,struct block_list* src,struct block_list *dsrc,
	struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag)
{
	struct Damage dmg;
	struct map_session_data *sd = NULL, *tsd = NULL;
	struct status_change *sc;
	struct status_change *ssc;
	int type, lv, damage, rdamage = 0;

	nullpo_retr(0, src);
	nullpo_retr(0, dsrc);
	nullpo_retr(0, bl);

	sc  = status_get_sc(bl);
	ssc = status_get_sc(src);

	sd  = BL_DOWNCAST( BL_PC, src );
	tsd = BL_DOWNCAST( BL_PC, bl );

	if(dsrc->m != bl->m)	// �Ώۂ������}�b�v�ɂ��Ȃ���Ή������Ȃ�
		return 0;
	if(src->prev == NULL || dsrc->prev == NULL || bl->prev == NULL)
		return 0;
	if(unit_isdead(src) || unit_isdead(dsrc) || unit_isdead(bl))	// ���łɎ���ł����牽�����Ȃ�
		return 0;

	if(ssc) {		// �����������ړ��������͖��f�̃E�B���N���Ȃ牽�����Ȃ�
		if(ssc->data[SC_FORCEWALKING].timer != -1 || ssc->data[SC_WINKCHARM].timer != -1)
			return 0;
	}
	if(sc) {
		if(sc->data[SC_HIDING].timer != -1 && skill_get_pl(skillid) != ELE_EARTH)	// �n�C�f�B���O��ԂŃX�L���̑������n�����łȂ��Ȃ牽�����Ȃ�
			return 0;
		if(sc->data[SC_CHASEWALK].timer != -1 && skillid == AL_RUWACH)	// �`�F�C�X�E�H�[�N��ԂŃ��A�t����
			return 0;
		if(sc->data[SC_TRICKDEAD].timer != -1) 				// ���񂾂ӂ蒆�͉������Ȃ�
			return 0;
		if(sc->data[SC_FORCEWALKING].timer != -1) 			// �����ړ����͉������Ȃ�
			return 0;
		// ������ԂŃX�g�[���K�X�g�A�t���X�g�m���@�A�X�՗��͖���
		if(sc->data[SC_FREEZE].timer != -1 && (skillid == WZ_STORMGUST || skillid == WZ_FROSTNOVA || skillid == NJ_HYOUSYOURAKU))
			return 0;
	}
	if(skillid == WZ_FROSTNOVA && dsrc->x == bl->x && dsrc->y == bl->y)	// �g�p�X�L�����t���X�g�m���@�ŁAdsrc��bl�������ꏊ�Ȃ牽�����Ȃ�
		return 0;
	if(sd && sd->chatID)	// ��������PC�Ń`���b�g���Ȃ牽�����Ȃ�
		return 0;
	if(sd && mob_gvmobcheck(sd,bl) == 0)
		return 0;

	type = (flag >> 8) & 0xff;
	if(skillid == 0)
		type = 5;

	/* ��̏��� */
	if(sd) {
		int cost = skill_get_arrow_cost(skillid,skilllv);
		if(cost > 0) {
			switch(skillid)
			{
			case AC_DOUBLE:
			case AC_CHARGEARROW:
			case BA_MUSICALSTRIKE:
			case DC_THROWARROW:
			case CG_ARROWVULCAN:
			case AS_VENOMKNIFE:
			case NJ_SYURIKEN:
			case NJ_KUNAI:
			case GS_TRACKING:
			case GS_DISARM:
			case GS_PIERCINGSHOT:
			case GS_DUST:
			case GS_RAPIDSHOWER:
			case GS_FULLBUSTER:
				if( !battle_delarrow(sd, cost, skillid) )
					return 0;
				break;
			case SN_SHARPSHOOTING:
			case GS_SPREADATTACK:
				if( type == 0 && !battle_delarrow(sd, cost, skillid) )
					return 0;
				break;
			}
		}
	}

	/* �t���O�l�`�F�b�N */
	lv = (flag >> 20) & 0x0f;
	if(lv == 0)
		lv = skilllv;
	else if(lv == 0x0f)
		lv = -1;

	if(flag & 0x01000000) {	// �G�t�F�N�g�����o���ă_���[�W�Ȃ��ŏI��
		clif_skill_damage(dsrc, bl, tick, status_get_amotion(src), 0, -1, 1, skillid, lv, type);
		return -1;
	}

	/* �_���[�W�v�Z */
	if(skillid == GS_GROUNDDRIFT)	// �O���E���h�h���t�g��dsrc�������Ƃ��ēn��
		dmg = battle_calc_attack(attack_type,dsrc,bl,skillid,skilllv,flag&0xff);
	else
		dmg = battle_calc_attack(attack_type,src,bl,skillid,skilllv,flag&0xff);

	/* �}�W�b�N���b�h */
	if(attack_type&BF_MAGIC && sc && sc->data[SC_MAGICROD].timer != -1 && src == dsrc) {
		dmg.damage = dmg.damage2 = 0;
		if(tsd) {	// �Ώۂ�PC�̏ꍇ
			int sp = skill_get_sp(skillid,skilllv);		// �g�p���ꂽ�X�L����SP���z��
			sp = sp * sc->data[SC_MAGICROD].val2 / 100;
			if(skillid == WZ_WATERBALL && skilllv > 1)
				sp = sp/((skilllv|1)*(skilllv|1));	// �E�H�[�^�[�{�[���͂���Ɍv�Z�H
			if(sp > 0x7fff)
				sp = 0x7fff;
			else if(sp < 1)
				sp = 1;
			if(tsd->status.sp + sp > tsd->status.max_sp) {
				sp = tsd->status.max_sp - tsd->status.sp;
				tsd->status.sp = tsd->status.max_sp;
			} else {
				tsd->status.sp += sp;
			}
			clif_heal(tsd->fd,SP_SP,sp);
			tsd->ud.canact_tick = tick + skill_delayfix(&tsd->bl, skill_get_delay(SA_MAGICROD,sc->data[SC_MAGICROD].val1), skill_get_cast(SA_MAGICROD,sc->data[SC_MAGICROD].val1));
		}
		clif_skill_nodamage(bl,bl,SA_MAGICROD,sc->data[SC_MAGICROD].val1,1);	// �}�W�b�N���b�h�G�t�F�N�g��\��
	}

	damage = dmg.damage + dmg.damage2;

	if(damage <= 0 || damage < dmg.div_)	// ������΂�����
		dmg.blewcount = 0;

	if(skillid == CR_GRANDCROSS || skillid == NPC_GRANDDARKNESS) {	// �O�����h�N���X
		if(battle_config.gx_disptype)
			dsrc = src;	// �G�_���[�W�������\��
		if(src == bl)
			type = 4;	// �����̓_���[�W���[�V�����Ȃ�
	}

	/* �R���{ */
	if(sd) {
		int delay;

		switch(skillid) {
		case MO_CHAINCOMBO:	// �A�ŏ�
			delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				if(pc_checkskill(sd, MO_COMBOFINISH) > 0 && sd->spiritball > 0) { // �җ����擾���C���ێ�����+300ms
					delay += 300 * battle_config.combo_delay_rate /100;
					// �R���{���͎��Ԃ̍Œ�ۏ�ǉ�
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,MO_CHAINCOMBO,skilllv,0,0,delay,0);
			}
			if(delay > 0) {
				sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
				clif_combo_delay(src,delay);
			}
			break;
		case MO_COMBOFINISH:	// �җ���
			delay = 700 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				// ���C���e�����擾���C��4�ێ��������g����Ԏ���+300ms
				// ���Ռ��擾����+300ms
				if((pc_checkskill(sd, MO_EXTREMITYFIST) > 0 && sd->spiritball >= 4 && sd->sc.data[SC_EXPLOSIONSPIRITS].timer != -1) ||
				   (pc_checkskill(sd, CH_TIGERFIST) > 0 && sd->spiritball > 0) ||
				   (pc_checkskill(sd, CH_CHAINCRUSH) > 0 && sd->spiritball > 1))
				{
					delay += 300 * battle_config.combo_delay_rate /100;
					// �R���{���͎��ԍŒ�ۏ�ǉ�
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,MO_COMBOFINISH,skilllv,0,0,delay,0);
			}
			if(delay > 0) {
				sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
				clif_combo_delay(src,delay);
			}
			break;
		case CH_TIGERFIST:	// ���Ռ�
			delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				if(pc_checkskill(sd, CH_CHAINCRUSH) > 0) { // �A�������擾����+300ms
					delay += 300 * battle_config.combo_delay_rate /100;
					// �R���{���͎��ԍŒ�ۏ�ǉ�
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,CH_TIGERFIST,skilllv,0,0,delay,0);
			}
			if(delay > 0) {
				sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
				clif_combo_delay(src,delay);
			}
			break;
		case CH_CHAINCRUSH:	// �A������
			delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				// ���Ռ��K���܂��͈��C���K�����C��1�ێ��������g�����f�B���C
				if(pc_checkskill(sd, CH_TIGERFIST) > 0 || (pc_checkskill(sd, MO_EXTREMITYFIST) > 0 && sd->spiritball >= 1 && sd->sc.data[SC_EXPLOSIONSPIRITS].timer != -1))
				{
					delay += (600+(skilllv/5)*200) * battle_config.combo_delay_rate /100;
					// �R���{���͎��ԍŒ�ۏ�ǉ�
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,CH_CHAINCRUSH,skilllv,0,0,delay,0);
			}
			if(delay > 0) {
				sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
				clif_combo_delay(src,delay);
			}
			break;
		case TK_STORMKICK:
		case TK_DOWNKICK:
		case TK_TURNKICK:
		case TK_COUNTER:
			if(ranking_get_pc_rank(sd,RK_TAEKWON) > 0) {	// �e�R�������J�[�̓R���{���s
				delay = status_get_adelay(src);
				if(damage < status_get_hp(bl)) {
					delay += 2000 - 4 * status_get_agi(src) - 2 * status_get_dex(src);	// eA����
					// TK�R���{���͎��ԍŒ�ۏ�ǉ�
					if(delay < battle_config.tkcombo_delay_lower_limits)
						delay = battle_config.tkcombo_delay_lower_limits;
					status_change_start(src,SC_TKCOMBO,skillid,0,0,TK_MISSION,delay,0);
				}
				if(delay > 0)
					sd->ud.attackabletime = tick + delay;
			} else {
				status_change_end(src,SC_TKCOMBO,-1);
			}
			break;
		}
	}

	/* �_���[�W���� */
	if(attack_type&BF_WEAPON && damage > 0 && src != bl && src == dsrc) {	// ����X�L�����_���[�W���聕�g�p�҂ƑΏێ҂��Ⴄ��src=dsrc
		if(dmg.flag&BF_SHORT) {	// �ߋ����U����
			if(tsd) {	// �Ώۂ�PC�̎�
				if(tsd->short_weapon_damage_return > 0) {	// �ߋ����U�����˕Ԃ�
					rdamage += damage * tsd->short_weapon_damage_return / 100;
					if(rdamage < 1) rdamage = 1;
				}
			}

			// ���t���N�g�V�[���h��
			if(sc &&
			   sc->data[SC_REFLECTSHIELD].timer != -1 &&
			   (sd || sc->data[SC_DEVOTION].timer == -1) &&	// ��f�B�{�[�V�����҂Ȃ�PC����ȊO�͔������Ȃ�
			   skillid != WS_CARTTERMINATION)
			{
				rdamage += damage * sc->data[SC_REFLECTSHIELD].val2 / 100;	// ���˕Ԃ��v�Z
				if(rdamage < 1) rdamage = 1;
			}
		} else if(dmg.flag&BF_LONG) {	// �������U����
			if(tsd) {		// �Ώۂ�PC�̎�
				if(tsd->long_weapon_damage_return > 0) { // �������U�����˕Ԃ�
					rdamage += damage * tsd->long_weapon_damage_return / 100;
					if(rdamage < 1) rdamage = 1;
				}
			}
		}
		if(rdamage > 0)
			clif_damage(src,src,tick, dmg.amotion,0,rdamage,1,4,0);
	}
	if(attack_type&BF_MAGIC && damage > 0 && src != bl) {	// ���@�X�L�����_���[�W���聕�g�p�҂ƑΏێ҂��Ⴄ
		if(tsd && src == dsrc) {	// �Ώۂ�PC�̎�
			if(tsd->magic_damage_return > 0 && atn_rand()%100 < tsd->magic_damage_return) {	// ���@�U�����˕Ԃ��H��
				rdamage = damage;
				damage  = -1;	// �_���[�W0����miss���o���Ȃ�
			}
		}
		// �J�C�g
		if(damage > 0 && sc && sc->data[SC_KAITE].timer != -1 && skillid != HW_GRAVITATION)
		{
			if(src->type == BL_PC || (status_get_lv(src) < 80 && !(status_get_mode(src)&0x20)))
			{
				int idx;
				clif_misceffect2(bl,438);
				clif_skill_nodamage(bl,src,skillid,skilllv,1);
				if(--sc->data[SC_KAITE].val2 == 0)
					status_change_end(bl,SC_KAITE,-1);

				if( sd && ssc && ssc->data[SC_WIZARD].timer != -1 &&
				    (idx = pc_search_inventory(sd,7321)) >= 0 ) {
					pc_delitem(sd,idx,1,0);
				} else {
					rdamage += damage;
				}
				damage = -1;	// �_���[�W0����miss���o���Ȃ�
			}
		}
		// �}�W�b�N�~���[
		if(damage > 0 && sc && sc->data[SC_MAGICMIRROR].timer != -1) {
			if(atn_rand()%100 < sc->data[SC_MAGICMIRROR].val1 * 20) {
				clif_misceffect2(bl,675);
				clif_skill_nodamage(bl,src,skillid,skilllv,1);
				rdamage += damage;
				damage = -1;	// �_���[�W0����miss���o���Ȃ�
			}
		}
		if(rdamage > 0) {
			clif_skill_damage(src, src, tick, dmg.amotion, dmg.dmotion, rdamage, dmg.div_, skillid, ((src == dsrc)? lv: -1), type);
			if(dmg.blewcount > 0 && !map[src->m].flag.gvg) {
				int dir = map_calc_dir(src,bl->x,bl->y);
				if(dir == 0)
					dir = 8;
				skill_blown(src,src,dmg.blewcount|(dir<<20));	// �Ώۂɑ΂�������Ƌt�����ɔ�΂�
			}
			memset(&dmg,0,sizeof(dmg));
		}
	}

	/* �_���[�W�p�P�b�g���M */
	if(damage != -1) {
		switch(skillid) {
		case NPC_SELFDESTRUCTION:
		case NPC_SELFDESTRUCTION2:
			dmg.blewcount |= SAB_NODAMAGE;
			break;
		default:
			clif_skill_damage(dsrc, bl, tick, dmg.amotion, dmg.dmotion, damage, dmg.div_, skillid, lv, type);
			break;
		}
	} else {	// �_���[�W�������̓p�P�b�g���M���Ȃ�
		damage = 0;
	}

	/* ������΂������Ƃ��̃p�P�b�g */
	if(dmg.blewcount > 0 && bl->type != BL_SKILL && !map[src->m].flag.gvg) {
		skill_blown(dsrc,bl,dmg.blewcount);
	}
	/* ������΂������Ƃ��̃p�P�b�g �J�[�h���� */
	if(dsrc->type == BL_PC && bl->type != BL_SKILL && !map[src->m].flag.gvg) {
		skill_add_blown(dsrc,bl,skillid,SAB_REVERSEBLOW);
	}

	map_freeblock_lock();

	/* �N���[���X�L�� */
	if(damage > 0 && dmg.flag&BF_SKILL && tsd && pc_checkskill(tsd,RG_PLAGIARISM) && sc && sc->data[SC_PRESERVE].timer == -1) {
		skill_clone(tsd,skillid,skilllv);
	}

	/* ���ۂɃ_���[�W�������s�� */
	if(skillid || flag) {
		if(attack_type&BF_WEAPON) {
			battle_delay_damage(tick+dmg.amotion,src,bl,damage,skillid,skilllv,dmg.flag);
		} else {
			battle_damage(src,bl,damage,skillid,skilllv,dmg.flag);

			/* �\�E���h���C�� */
			if(sd && bl->type == BL_MOB && unit_isdead(bl) && attack_type&BF_MAGIC)
			{
				int level = pc_checkskill(sd,HW_SOULDRAIN);
				if(level > 0 && skill_get_inf(skillid) & 0x01 && sd && sd->ud.skilltarget == bl->id) {
					int sp = 0;
					clif_skill_nodamage(src,bl,HW_SOULDRAIN,level,1);
					sp = (status_get_lv(bl))*(95+15*level)/100;
					if(sd->status.sp + sp > sd->status.max_sp)
						sp = sd->status.max_sp - sd->status.sp;
					sd->status.sp += sp;
					clif_heal(sd->fd,SP_SP,sp);
				}
			}
		}
	}

	if(skillid == RG_INTIMIDATE) {
		/* �C���e�B�~�f�C�g */
		if(damage > 0 && !(status_get_mode(bl)&0x20) && !map[src->m].flag.gvg) {
			int rate = 50 + skilllv * 5 + status_get_lv(src) - status_get_lv(bl);
			if(atn_rand()%100 < rate)
				skill_addtimerskill(src,tick + 800,bl->id,0,0,skillid,skilllv,0,flag);
		}
	} else if(skillid == NPC_EXPULSION) {
		/* �G�N�X�p���V�I�� */
		if(damage > 0 && !map[src->m].flag.gvg)
			skill_addtimerskill(src,tick + 800,bl->id,0,0,skillid,skilllv,0,flag);
	}

	/* �_���[�W������Ȃ�ǉ����ʔ��� */
	if(bl->prev != NULL && !unit_isdead(bl)) {
		if(damage > 0 || skillid == TF_POISON) {
			// �O���E���h�h���t�g��dsrc�������Ƃ��ēn��
			if(skillid == GS_GROUNDDRIFT)
				skill_additional_effect(dsrc,bl,skillid,skilllv,attack_type,tick);
			else
				skill_additional_effect(src,bl,skillid,skilllv,attack_type,tick);
		}

		if(bl->type == BL_MOB && src != bl)	// �X�L���g�p������MOB�X�L��
		{
			struct mob_data *md = (struct mob_data *)bl;
			if(md) {
				int target = md->target_id;
				if(battle_config.mob_changetarget_byskill == 1 || target == 0)
				{
					if(src->type == BL_PC || src->type == BL_HOM || src->type == BL_MERC)
						md->target_id = src->id;
				}
				mobskill_use(md,tick,MSC_SKILLUSED|(skillid<<16));
				md->target_id = target;
			}
		}
	}

	/* HP,SP�z�� */
	if(sd && dmg.flag&BF_WEAPON && src != bl && src == dsrc && damage > 0) {
		// ���z���̂�
		battle_attack_drain(src, dmg.damage, dmg.damage2, 1);
	}

	/* ���˃_���[�W�̎��ۂ̏��� */
	if((skillid || flag) && rdamage > 0) {
		unsigned long asflag = EAS_WEAPON | EAS_ATTACK | EAS_NORMAL;

		if(attack_type&BF_WEAPON) {
			battle_delay_damage(tick+dmg.amotion,bl,src,rdamage,skillid,skilllv,0);
			if(sd) {
				// ���˃_���[�W�̃I�[�g�X�y��
				if(battle_config.weapon_reflect_autospell) {
					skill_bonus_autospell(bl,src,asflag,gettick(),0);
				}
				if(battle_config.weapon_reflect_drain && src != bl)
					battle_attack_drain(bl,rdamage,0,battle_config.weapon_reflect_drain_enable_type);
			}
		} else {
			battle_damage(bl,src,rdamage,skillid,skilllv,0);
			if(sd) {
				// ���˃_���[�W�̃I�[�g�X�y��
				if(battle_config.magic_reflect_autospell) {
					skill_bonus_autospell(bl,src,asflag,gettick(),0);
				}
				if(battle_config.magic_reflect_drain && src != bl)
					battle_attack_drain(bl,rdamage,0,battle_config.magic_reflect_drain_enable_type);
			}
		}
	}

	/* �I�[�g�J�E���^�[ */
	if(attack_type&BF_WEAPON && sc && sc->data[SC_AUTOCOUNTER].timer != -1 && sc->data[SC_AUTOCOUNTER].val4 > 0) {
		if(sc->data[SC_AUTOCOUNTER].val3 == dsrc->id)
			battle_weapon_attack(bl,dsrc,tick,0x8000|sc->data[SC_AUTOCOUNTER].val1);
		status_change_end(bl,SC_AUTOCOUNTER,-1);
	}
	/* �_�u���L���X�e�B���O */
	if ((skillid == MG_COLDBOLT || skillid == MG_FIREBOLT || skillid == MG_LIGHTNINGBOLT) &&
		ssc && ssc->data[SC_DOUBLECASTING].timer != -1 &&
		atn_rand() % 100 < 30+10*ssc->data[SC_DOUBLECASTING].val1) {
		if (!(flag & 1)) {
			//skill_castend_delay (src, bl, skillid, skilllv, tick + dmg.div_*dmg.amotion, flag|1);
			skill_castend_delay (src, bl, skillid, skilllv, tick + 100, flag|1);
		}
	}
	/* �u���b�h���X�g */
	if(src->type == BL_HOM && ssc && ssc->data[SC_BLOODLUST].timer != -1 && dmg.flag&BF_WEAPON && src != bl && src == dsrc && damage > 0)
	{
		struct homun_data *hd = (struct homun_data *)src;
		if(hd && atn_rand()%100 < ssc->data[SC_BLOODLUST].val1*9)
		{
			homun_heal(hd,damage/5,0);
		}
	}

	map_freeblock_unlock();

	return dmg.damage+dmg.damage2;	/* �^�_����Ԃ� */
}

/*==========================================
 *
 *------------------------------------------
 */
int battle_skill_attack_area(struct block_list *bl,va_list ap)
{
	struct block_list *src,*dsrc;
	int atk_type,skillid,skilllv,flag,type;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	atk_type = va_arg(ap,int);
	if((src = va_arg(ap,struct block_list*)) == NULL)
		return 0;
	if((dsrc = va_arg(ap,struct block_list*)) == NULL)
		return 0;

	skillid = va_arg(ap,int);
	skilllv = va_arg(ap,int);
	tick    = va_arg(ap,unsigned int);
	flag    = va_arg(ap,int);
	type    = va_arg(ap,int);

	if(battle_check_target(dsrc,bl,type) > 0)
		battle_skill_attack(atk_type,src,dsrc,bl,skillid,skilllv,tick,flag);

	return 0;
}

/*==========================================
 * �s������
 *------------------------------------------
 */
int battle_check_undead(int race,int element)
{
	// element �ɑ����l�{lv(status_get_element �̖߂�l)���n�����~�X��
	// �Ή�����ׁAelement���瑮���^�C�v�����𔲂��o���B
	element %= 20;

	if(battle_config.undead_detect_type == 0) {
		if(element == ELE_UNDEAD)
			return 1;
	}
	else if(battle_config.undead_detect_type == 1) {
		if(race == RCT_UNDEAD)
			return 1;
	}
	else {
		if(element == ELE_UNDEAD || race == RCT_UNDEAD)
			return 1;
	}
	return 0;
}

/*==========================================
 * �G��������(1=�m��,0=�ے�,-1=�G���[)
 * flag&0xf0000 = 0x00000:�G����Ȃ�������iret:1���G�ł͂Ȃ��j
 *              = 0x10000:�p�[�e�B�[����iret:1=�p�[�e�B�[�����o)
 *              = 0x20000:�S��(ret:1=�G��������)
 *              = 0x40000:�G������(ret:1=�G)
 *              = 0x50000:�p�[�e�B�[����Ȃ�������(ret:1=�p�[�e�B�łȂ�)
 *------------------------------------------
 */
int battle_check_target( struct block_list *src, struct block_list *target,int flag)
{
	int s_p,s_g,t_p,t_g;
	struct block_list *ss = src;

	nullpo_retr(-1, src);
	nullpo_retr(-1, target);

	if( flag&0x40000 ) {	// ���]�t���O
		int ret = battle_check_target(src,target,flag&0x30000);
		if(ret != -1)
			return !ret;
		return -1;
	}

	if( flag&0x20000 ) {
		if( target->type == BL_MOB || target->type == BL_PC )
			return 1;
		if( target->type == BL_HOM && src->type != BL_SKILL )	// �z���̓X�L�����j�b�g�̉e�����󂯂Ȃ�
			return 1;
		else
			return -1;
	}

	if(src->type == BL_SKILL && target->type == BL_SKILL)	// �Ώۂ��X�L�����j�b�g�Ȃ疳�����m��
		return -1;

	if(target->type == BL_PC && ((struct map_session_data *)target)->invincible_timer != -1)
		return -1;

	if(target->type == BL_SKILL) {
		switch(((struct skill_unit *)target)->group->unit_id) {
			case UNT_ICEWALL:
			case UNT_BLASTMINE:
			case UNT_CLAYMORETRAP:
				return 0;
		}
	}

	if(target->type == BL_PET)
		return -1;

	// �X�L�����j�b�g�̏ꍇ�A�e�����߂�
	if( src->type == BL_SKILL) {
		struct skill_unit_group *sg = ((struct skill_unit *)src)->group;
		int inf2;
		if( sg == NULL )
			return -1;
		inf2 = skill_get_inf2(sg->skill_id);
		if( (ss = map_id2bl(sg->src_id)) == NULL )
			return -1;
		if(ss->prev == NULL)
			return -1;
		if( target->type != BL_PC || !pc_isinvisible((struct map_session_data *)target) ) {
			if(inf2&0x2000 && map[src->m].flag.pk)
				return 0;
			if(inf2&0x1000 && map[src->m].flag.gvg)
				return 0;
			if(inf2&0x80   && map[src->m].flag.pvp)
				return 0;
		}
		if(ss == target) {
			if(inf2&0x100)
				return 0;
			if(inf2&0x200)
				return -1;
		}
	}

	// Mob��master_id��������special_mob_ai�Ȃ�A����������߂�
	if( src->type == BL_MOB ) {
		struct mob_data *md = (struct mob_data *)src;
		if(md && md->master_id > 0) {
			if(md->master_id == target->id)	// ��Ȃ�m��
				return 1;
			if(md->state.special_mob_ai) {
				if(target->type == BL_MOB) {	// special_mob_ai�őΏۂ�Mob
					struct mob_data *tmd = (struct mob_data *)target;
					if(tmd) {
						if(tmd->master_id != md->master_id)	// �����傪�ꏏ�łȂ���Δے�
							return 0;
						else if(md->state.special_mob_ai > 2)	// �����傪�ꏏ�Ȃ̂ōm�肵�������ǎ����͔ے�
							return 0;
						else
							return 1;
					}
				} else if(target->type == BL_HOM || target->type == BL_MERC) {
					// special_mob_ai�őΏۂ��z�����b���Ȃ�G���[�ŕԂ�
					return -1;
				}
			}
			if((ss = map_id2bl(md->master_id)) == NULL)
				return -1;
		}
	}

	if( src == target || ss == target )	// �����Ȃ�m��
		return 1;

	if(target->type == BL_PC && pc_isinvisible((struct map_session_data *)target))
		return -1;

	if( src->prev == NULL || unit_isdead(src) ) // ����ł�Ȃ�G���[
		return -1;

	if( (ss->type == BL_PC && target->type == BL_MOB) ||
	    (ss->type == BL_MOB && target->type == BL_PC) )
		return 0;	// PCvsMOB�Ȃ�G

	if(ss->type == BL_PET && target->type == BL_MOB) {
		struct pet_data *pd = (struct pet_data*)ss;
		struct mob_data *md = (struct mob_data*)target;
		int mode = mob_db[pd->class_].mode;
		int race = mob_db[pd->class_].race;
		if(mob_db[pd->class_].mexp <= 0 && !(mode&0x20) && (md->sc.option & 0x06 && race != RCT_INSECT && race != RCT_DEMON) ) {
			return 1; // ���s
		} else {
			return 0; // ����
		}
	}

	s_p = status_get_party_id(ss);
	s_g = status_get_guild_id(ss);

	t_p = status_get_party_id(target);
	t_g = status_get_guild_id(target);

	if(flag&0x10000) {
		if(s_p && t_p && s_p == t_p)
			return 1;	// �����p�[�e�B�Ȃ�m��i�����j
		else
			return 0;	// �p�[�e�B�����Ȃ瓯���p�[�e�B����Ȃ����_�Ŕے�
	}

	if(ss->type == BL_MOB && s_g > 0 && t_g > 0 && s_g == t_g )	// �����M���h/mob�N���X�Ȃ�m��i�����j
		return 1;

	if(ss->type == BL_PC && target->type == BL_PC) {	// ����PVP���[�h�Ȃ�ے�i�G�j
		struct skill_unit *su = NULL;
		if(src->type == BL_SKILL)
			su = (struct skill_unit *)src;
		// PK
		if(map[ss->m].flag.pk) {
			struct guild *g = NULL;
			struct map_session_data* ssd = (struct map_session_data*)ss;
			struct map_session_data* tsd = (struct map_session_data*)target;

			// battle_config.no_pk_level�ȉ��@1���͖����@�]���͑ʖ�
			if(ssd->sc.data[SC_PK_PENALTY].timer != -1)
				return 1;
			if(ssd->status.base_level <= battle_config.no_pk_level && (ssd->s_class.job <= 6 || ssd->s_class.job == 24) && ssd->s_class.upper != 1)
				return 1;
			if(tsd->status.base_level <= battle_config.no_pk_level && (tsd->s_class.job <= 6 || tsd->s_class.job == 24) && tsd->s_class.upper != 1)
				return 1;
			if(su && su->group && su->group->target_flag == BCT_NOENEMY)
				return 1;
			if(s_p > 0 && t_p > 0 && s_p == t_p)
				return 1;
			else if(s_g > 0 && t_g > 0 && s_g == t_g)
				return 1;
			if((g = guild_search(s_g)) != NULL) {
				int i;
				for(i=0; i<MAX_GUILDALLIANCE; i++) {
					if(g->alliance[i].guild_id > 0 && g->alliance[i].guild_id == t_g) {
						if(g->alliance[i].opposition)
							return 0;	// �G�΃M���h�Ȃ疳�����ɓG
						else
							return 1;	// �����M���h�Ȃ疳�����ɖ���
					}
				}
			}
			return 0;
		}
		// PVP
		if(map[ss->m].flag.pvp) {
			if(su && su->group->target_flag == BCT_NOENEMY)
				return 1;
			if(map[ss->m].flag.pvp_noparty && s_p > 0 && t_p > 0 && s_p == t_p)
				return 1;
			else if(map[ss->m].flag.pvp_noguild && s_g > 0 && t_g > 0 && s_g == t_g)
				return 1;
			return 0;
		}
		// GVG
		if(map[src->m].flag.gvg) {
			struct guild *g = NULL;
			if(su && su->group->target_flag == BCT_NOENEMY)
				return 1;
			if(s_g > 0 && s_g == t_g)
				return 1;
			if(map[src->m].flag.gvg_noparty && s_p > 0 && t_p > 0 && s_p == t_p)
				return 1;
			if((g = guild_search(s_g)) != NULL) {
				int i;
				for(i=0; i<MAX_GUILDALLIANCE; i++) {
					if(g->alliance[i].guild_id > 0 && g->alliance[i].guild_id == t_g) {
						if(g->alliance[i].opposition)
							return 0;	// �G�΃M���h�Ȃ疳�����ɓG
						else
							return 1;	// �����M���h�Ȃ疳�����ɖ���
					}
				}
			}
			return 0;
		}
	}

	if( (ss->type == BL_HOM && target->type == BL_MOB) ||
	    (ss->type == BL_MOB && target->type == BL_HOM) )
		return 0;	// HOMvsMOB�Ȃ�G

	if( (ss->type == BL_MERC && target->type == BL_MOB) ||
	    (ss->type == BL_MOB && target->type == BL_MERC) )
		return 0;	// MERCvsMOB�Ȃ�G

	if(!(map[ss->m].flag.pvp || map[ss->m].flag.gvg)) {
		if( (ss->type == BL_PC && target->type == BL_HOM) ||
		    (ss->type == BL_HOM && target->type == BL_PC) )
			return 1;	// Pv�ł�Gv�ł��Ȃ��Ȃ�APCvsHOM�͖���

		if( (ss->type == BL_PC && target->type == BL_MERC) ||
		    (ss->type == BL_MERC && target->type == BL_PC) )
			return 1;	// Pv�ł�Gv�ł��Ȃ��Ȃ�APCvsMERC�͖���
	}

	// ��PT�Ƃ�����Guild�Ƃ��͌�񂵁i����
	if(ss->type == BL_HOM || ss->type == BL_MERC) {
		if(map[ss->m].flag.pvp) {	// PVP
			if(target->type == BL_HOM || target->type == BL_MERC)
				return 0;
			if(target->type == BL_PC) {
				struct map_session_data *msd = NULL;
				if(ss->type == BL_HOM)
					msd = ((struct homun_data*)ss)->msd;
				else if(ss->type == BL_MERC)
					msd = ((struct merc_data*)ss)->msd;
				if(msd == NULL || target != &msd->bl)
					return 0;
			}
		}
		if(map[ss->m].flag.gvg) {	// GVG
			if(target->type == BL_HOM || target->type == BL_MERC)
				return 0;
		}
	}
	if(ss->type == BL_PC && (target->type == BL_HOM || target->type == BL_MERC)) {
		if(map[ss->m].flag.pvp) {	// PVP
			struct map_session_data *msd = NULL;
			if(target->type == BL_HOM)
				msd = ((struct homun_data*)target)->msd;
			else if(target->type == BL_MERC)
				msd = ((struct merc_data*)target)->msd;
			if(msd == NULL || ss != &msd->bl)
				return 0;
		}
		if(map[ss->m].flag.gvg) {	// GVG
			return 0;
		}
	}
	return 1;	// �Y�����Ȃ��̂Ŗ��֌W�l���i�܂��G����Ȃ��̂Ŗ����j
}

/*==========================================
 * �˒�����
 *------------------------------------------
 */
int battle_check_range(struct block_list *src,struct block_list *bl,int range)
{
	int arange;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	arange = unit_distance(src->x,src->y,bl->x,bl->y);

	if(src->m != bl->m)	// �Ⴄ�}�b�v
		return 0;

	if(range > 0 && range < arange)	// ��������
		return 0;

	if(arange < 2)	// �����}�X���א�
		return 1;

	// ��Q������
	return path_search_long(NULL,src->m,src->x,src->y,bl->x,bl->y);
}

/*==========================================
 * ��̏���
 *------------------------------------------
 */
int battle_delarrow(struct map_session_data* sd,int num,int skillid)
{
	int mask = 0, idx = -1;

	nullpo_retr(0, sd);

	if(skillid == 0) {	// �ʏ�U��
		switch(sd->status.weapon) {
			case WT_BOW:
				mask = 1;
				break;
			case WT_HANDGUN:
				mask = 4;
				break;
			case WT_RIFLE:
				mask = 8;
				break;
			case WT_SHOTGUN:
				mask = 16;
				break;
			case WT_GATLING:
				mask = 32;
				break;
			case WT_GRENADE:
				mask = 64;
				break;
			default:
				return 1;
		}
	} else {		// �X�L���U��
		mask = skill_get_arrow_type(skillid);
	}

	idx = sd->equip_index[10];
	if(idx >= 0 && sd->status.inventory[idx].amount >= num && sd->inventory_data[idx]->arrow_type & mask) {
		if(battle_config.arrow_decrement)
			pc_delitem(sd,idx,num,0);
	} else {
		clif_arrow_fail(sd,0);
		return 0;
	}
	return 1;
}

/*==========================================
 * �_���[�W�Ȃ��ŋ����ɎQ��
 *------------------------------------------
 */
void battle_join_struggle(struct mob_data *md,struct block_list *src)
{
	nullpo_retv(md);
	nullpo_retv(src);

	if(src->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)src;
		if(sd) {
			if(linkdb_search( &md->dmglog, (void*)sd->status.char_id ) == NULL) {
				// �_���[�W-1�Ő퓬�Q���ғ���(0�ɂ���ƃ��X�g���o�^��NULL�Ƃ��Ԃ��č���)
				linkdb_insert( &md->dmglog, (void*)sd->status.char_id, (void*)-1 );
			}
		}
	}
	return;
}

/*==========================================
 * �ݒ�t�@�C���ǂݍ��ݗp�i�t���O�j
 *------------------------------------------
 */
static int battle_config_switch(const char *str)
{
	if(strcmpi(str,"on") == 0 || strcmpi(str,"yes") == 0)
		return 1;
	if(strcmpi(str,"off") == 0 || strcmpi(str,"no") == 0)
		return 0;
	return atoi(str);
}

/*==========================================
 * �ݒ�t�@�C����ǂݍ���
 *------------------------------------------
 */
int battle_config_read(const char *cfgName)
{
	int i;
	char line[1024],w1[1024],w2[1024];
	FILE *fp;
	static int count = 0;
	static const struct {
		const char *str;
		int *val;
		int defval;
	} data[] = {
		{ "warp_point_debug",                   &battle_config.warp_point_debug,                   0        },
		{ "enemy_critical",                     &battle_config.enemy_critical,                     0        },
		{ "enemy_critical_rate",                &battle_config.enemy_critical_rate,                100      },
		{ "enemy_str",                          &battle_config.enemy_str,                          1        },
		{ "enemy_perfect_flee",                 &battle_config.enemy_perfect_flee,                 0        },
		{ "casting_rate",                       &battle_config.cast_rate,                          100      },
		{ "no_casting_dex",                     &battle_config.no_cast_dex,                        150      },
		{ "delay_rate",                         &battle_config.delay_rate,                         100      },
		{ "delay_dependon_dex",                 &battle_config.delay_dependon_dex,                 0        },
		{ "no_delay_dex",                       &battle_config.no_delay_dex,                       150      },
		{ "skill_delay_attack_enable",          &battle_config.sdelay_attack_enable,               1        },
		{ "left_cardfix_to_right",              &battle_config.left_cardfix_to_right,              1        },
		{ "player_skill_add_range",             &battle_config.pc_skill_add_range,                 0        },
		{ "skill_out_range_consume",            &battle_config.skill_out_range_consume,            1        },
		{ "monster_skill_add_range",            &battle_config.mob_skill_add_range,                0        },
		{ "player_damage_delay",                &battle_config.pc_damage_delay,                    1        },
		{ "player_damage_delay_rate",           &battle_config.pc_damage_delay_rate,               10       },
		{ "defunit_not_enemy",                  &battle_config.defnotenemy,                        0        },
		{ "random_monster_checklv",             &battle_config.random_monster_checklv,             1        },
		{ "attribute_recover",                  &battle_config.attr_recover,                       1        },
		{ "flooritem_lifetime",                 &battle_config.flooritem_lifetime,                 60000    },
		{ "item_auto_get",                      &battle_config.item_auto_get,                      0        },
		{ "item_first_get_time",                &battle_config.item_first_get_time,                3000     },
		{ "item_second_get_time",               &battle_config.item_second_get_time,               1000     },
		{ "item_third_get_time",                &battle_config.item_third_get_time,                1000     },
		{ "mvp_item_first_get_time",            &battle_config.mvp_item_first_get_time,            10000    },
		{ "mvp_item_second_get_time",           &battle_config.mvp_item_second_get_time,           10000    },
		{ "mvp_item_third_get_time",            &battle_config.mvp_item_third_get_time,            2000     },
		{ "item_rate",                          &battle_config.item_rate,                          100      },
		{ "drop_rate0item",                     &battle_config.drop_rate0item,                     0        },
		{ "base_exp_rate",                      &battle_config.base_exp_rate,                      100      },
		{ "job_exp_rate",                       &battle_config.job_exp_rate,                       100      },
		{ "death_penalty_type",                 &battle_config.death_penalty_type,                 3        },
		{ "death_penalty_base",                 &battle_config.death_penalty_base,                 100      },
		{ "death_penalty_job",                  &battle_config.death_penalty_job,                  100      },
		{ "zeny_penalty",                       &battle_config.zeny_penalty,                       0        },
		{ "zeny_penalty_percent",               &battle_config.zeny_penalty_percent,               0        },
		{ "zeny_penalty_by_lvl",                &battle_config.zeny_penalty_by_lvl,                0        },
		{ "restart_hp_rate",                    &battle_config.restart_hp_rate,                    0        },
		{ "restart_sp_rate",                    &battle_config.restart_sp_rate,                    0        },
		{ "mvp_hp_rate",                        &battle_config.mvp_hp_rate,                        100      },
		{ "mvp_item_rate",                      &battle_config.mvp_item_rate,                      100      },
		{ "mvp_exp_rate",                       &battle_config.mvp_exp_rate,                       100      },
		{ "monster_hp_rate",                    &battle_config.monster_hp_rate,                    100      },
		{ "monster_max_aspd",                   &battle_config.monster_max_aspd,                   199      },
		{ "atcommand_gm_only",                  &battle_config.atc_gmonly,                         1        },
		{ "gm_all_skill",                       &battle_config.gm_allskill,                        1        },
		{ "gm_all_skill_add_abra",              &battle_config.gm_allskill_addabra,                0        },
		{ "gm_all_equipment",                   &battle_config.gm_allequip,                        0        },
		{ "gm_skill_unconditional",             &battle_config.gm_skilluncond,                     0        },
		{ "player_skillfree",                   &battle_config.skillfree,                          0        },
		{ "player_skillup_limit",               &battle_config.skillup_limit,                      0        },
		{ "weapon_produce_rate",                &battle_config.wp_rate,                            100      },
		{ "potion_produce_rate",                &battle_config.pp_rate,                            100      },
		{ "deadly_potion_produce_rate",         &battle_config.cdp_rate,                           100      },
		{ "monster_active_enable",              &battle_config.monster_active_enable,              1        },
		{ "monster_damage_delay_rate",          &battle_config.monster_damage_delay_rate,          100      },
		{ "monster_loot_type",                  &battle_config.monster_loot_type,                  0        },
		{ "mob_skill_use",                      &battle_config.mob_skill_use,                      1        },
		{ "mob_count_rate",                     &battle_config.mob_count_rate,                     100      },
		{ "mob_delay_rate",                     &battle_config.mob_delay_rate,                     100      },
		{ "mob_middle_boss_delay_rate",         &battle_config.mob_middle_boss_delay_rate,         100      },
		{ "mob_mvp_boss_delay_rate",            &battle_config.mob_mvp_boss_delay_rate,            100      },
		{ "quest_skill_learn",                  &battle_config.quest_skill_learn,                  0        },
		{ "quest_skill_reset",                  &battle_config.quest_skill_reset,                  1        },
		{ "basic_skill_check",                  &battle_config.basic_skill_check,                  1        },
		{ "guild_emperium_check",               &battle_config.guild_emperium_check,               1        },
		{ "guild_emblem_colors",                &battle_config.guild_emblem_colors,                3        },
		{ "guild_exp_limit",                    &battle_config.guild_exp_limit,                    50       },
		{ "player_invincible_time" ,            &battle_config.pc_invincible_time,                 5000     },
		{ "pet_catch_rate",                     &battle_config.pet_catch_rate,                     100      },
		{ "pet_rename",                         &battle_config.pet_rename,                         0        },
		{ "pet_friendly_rate",                  &battle_config.pet_friendly_rate,                  100      },
		{ "pet_hungry_delay_rate",              &battle_config.pet_hungry_delay_rate,              100      },
		{ "pet_hungry_friendly_decrease",       &battle_config.pet_hungry_friendly_decrease,       5        },
		{ "pet_str",                            &battle_config.pet_str,                            1        },
		{ "pet_status_support",                 &battle_config.pet_status_support,                 0        },
		{ "pet_attack_support",                 &battle_config.pet_attack_support,                 0        },
		{ "pet_damage_support",                 &battle_config.pet_damage_support,                 0        },
		{ "pet_support_rate",                   &battle_config.pet_support_rate,                   100      },
		{ "pet_attack_exp_to_master",           &battle_config.pet_attack_exp_to_master,           0        },
		{ "pet_attack_exp_rate",                &battle_config.pet_attack_exp_rate,                100      },
		{ "skill_min_damage",                   &battle_config.skill_min_damage,                   0        },
		{ "finger_offensive_type",              &battle_config.finger_offensive_type,              0        },
		{ "heal_exp",                           &battle_config.heal_exp,                           0        },
		{ "resurrection_exp",                   &battle_config.resurrection_exp,                   0        },
		{ "shop_exp",                           &battle_config.shop_exp,                           0        },
		{ "combo_delay_rate",                   &battle_config.combo_delay_rate,                   100      },
		{ "item_check",                         &battle_config.item_check,                         0        },
		{ "wedding_modifydisplay",              &battle_config.wedding_modifydisplay,              1        },
		{ "natural_healhp_interval",            &battle_config.natural_healhp_interval,            6000     },
		{ "natural_healsp_interval",            &battle_config.natural_healsp_interval,            8000     },
		{ "natural_heal_skill_interval",        &battle_config.natural_heal_skill_interval,        10000    },
		{ "natural_heal_weight_rate",           &battle_config.natural_heal_weight_rate,           50       },
		{ "natural_heal_weight_rate_icon",      &battle_config.natural_heal_weight_rate_icon,      0        },
		{ "item_name_override_grffile",         &battle_config.item_name_override_grffile,         0        },
		{ "arrow_decrement",                    &battle_config.arrow_decrement,                    1        },
		{ "allow_any_weapon_autoblitz",         &battle_config.allow_any_weapon_autoblitz,         0        },
		{ "max_aspd",                           &battle_config.max_aspd,                           199      },
		{ "pk_max_aspd",                        &battle_config.pk_max_aspd,                        199      },
		{ "gvg_max_aspd",                       &battle_config.gvg_max_aspd,                       199      },
		{ "pvp_max_aspd",                       &battle_config.pvp_max_aspd,                       199      },
		{ "max_hp",                             &battle_config.max_hp,                             999999   },
		{ "max_sp",                             &battle_config.max_sp,                             999999   },
		{ "max_parameter",                      &battle_config.max_parameter,                      99       },
		{ "max_cart_weight",                    &battle_config.max_cart_weight,                    8000     },
		{ "player_skill_log",                   &battle_config.pc_skill_log,                       0        },
		{ "monster_skill_log",                  &battle_config.mob_skill_log,                      0        },
		{ "battle_log",                         &battle_config.battle_log,                         0        },
		{ "save_log",                           &battle_config.save_log,                           0        },
		{ "error_log",                          &battle_config.error_log,                          1        },
		{ "etc_log",                            &battle_config.etc_log,                            1        },
		{ "save_clothcolor",                    &battle_config.save_clothcolor,                    0        },
		{ "undead_detect_type",                 &battle_config.undead_detect_type,                 0        },
		{ "player_auto_counter_type",           &battle_config.pc_auto_counter_type,               1        },
		{ "monster_auto_counter_type",          &battle_config.monster_auto_counter_type,          1        },
		{ "min_hitrate",                        &battle_config.min_hitrate,                        5        },
		{ "agi_penalty_type",                   &battle_config.agi_penalty_type,                   1        },
		{ "agi_penalty_count",                  &battle_config.agi_penalty_count,                  3        },
		{ "agi_penalty_num",                    &battle_config.agi_penalty_num,                    10       },
		{ "agi_penalty_count_lv",               &battle_config.agi_penalty_count_lv,               ATK_FLEE },
		{ "vit_penalty_type",                   &battle_config.vit_penalty_type,                   1        },
		{ "vit_penalty_count",                  &battle_config.vit_penalty_count,                  3        },
		{ "vit_penalty_num",                    &battle_config.vit_penalty_num,                    5        },
		{ "vit_penalty_count_lv",               &battle_config.vit_penalty_count_lv,               ATK_DEF  },
		{ "player_defense_type",                &battle_config.player_defense_type,                0        },
		{ "monster_defense_type",               &battle_config.monster_defense_type,               0        },
		{ "pet_defense_type",                   &battle_config.pet_defense_type,                   0        },
		{ "magic_defense_type",                 &battle_config.magic_defense_type,                 0        },
		{ "player_skill_reiteration",           &battle_config.pc_skill_reiteration,               0        },
		{ "monster_skill_reiteration",          &battle_config.monster_skill_reiteration,          0        },
		{ "player_skill_nofootset",             &battle_config.pc_skill_nofootset,                 0        },
		{ "monster_skill_nofootset",            &battle_config.monster_skill_nofootset,            0        },
		{ "player_cloak_check_type",            &battle_config.pc_cloak_check_type,                0        },
		{ "monster_cloak_check_type",           &battle_config.monster_cloak_check_type,           1        },
		{ "gvg_short_attack_damage_rate",       &battle_config.gvg_short_damage_rate,              60       },
		{ "gvg_long_attack_damage_rate",        &battle_config.gvg_long_damage_rate,               60       },
		{ "gvg_magic_attack_damage_rate",       &battle_config.gvg_magic_damage_rate,              60       },
		{ "gvg_misc_attack_damage_rate",        &battle_config.gvg_misc_damage_rate,               60       },
		{ "gvg_eliminate_time",                 &battle_config.gvg_eliminate_time,                 7000     },
		{ "mob_changetarget_byskill",           &battle_config.mob_changetarget_byskill,           0        },
		{ "gvg_edp_down_rate",                  &battle_config.gvg_edp_down_rate,                  100      },
		{ "pvp_edp_down_rate",                  &battle_config.pvp_edp_down_rate,                  100      },
		{ "pk_edp_down_rate",                   &battle_config.pk_edp_down_rate,                   100      },
		{ "player_attack_direction_change",     &battle_config.pc_attack_direction_change,         1        },
		{ "monster_attack_direction_change",    &battle_config.monster_attack_direction_change,    0        },
		{ "player_land_skill_limit",            &battle_config.pc_land_skill_limit,                1        },
		{ "monster_land_skill_limit",           &battle_config.monster_land_skill_limit,           1        },
		{ "party_skill_penalty",                &battle_config.party_skill_penalty,                1        },
		{ "monster_class_change_full_recover",  &battle_config.monster_class_change_full_recover,  1        },
		{ "produce_item_name_input",            &battle_config.produce_item_name_input,            0        },
		{ "produce_potion_name_input",          &battle_config.produce_potion_name_input,          1        },
		{ "making_arrow_name_input",            &battle_config.making_arrow_name_input,            0        },
		{ "holywater_name_input",               &battle_config.holywater_name_input,               0        },
		{ "display_delay_skill_fail",           &battle_config.display_delay_skill_fail,           0        },
		{ "display_snatcher_skill_fail",        &battle_config.display_snatcher_skill_fail,        1        },
		{ "chat_warpportal",                    &battle_config.chat_warpportal,                    0        },
		{ "mob_warpportal",                     &battle_config.mob_warpportal,                     0        },
		{ "dead_branch_active",                 &battle_config.dead_branch_active,                 1        },
		{ "vending_max_value",                  &battle_config.vending_max_value,                  99990000 },
		{ "pet_lootitem",                       &battle_config.pet_lootitem,                       0        },
		{ "pet_weight",                         &battle_config.pet_weight,                         1000     },
		{ "show_steal_in_same_party",           &battle_config.show_steal_in_same_party,           0        },
		{ "enable_upper_class",                 &battle_config.enable_upper_class,                 1        },
		{ "pet_attack_attr_none",               &battle_config.pet_attack_attr_none,               0        },
		{ "mob_attack_attr_none",               &battle_config.mob_attack_attr_none,               1        },
		{ "pc_attack_attr_none",                &battle_config.pc_attack_attr_none,                0        },
		{ "gx_allhit",                          &battle_config.gx_allhit,                          0        },
		{ "gx_cardfix",                         &battle_config.gx_cardfix,                         0        },
		{ "gx_dupele",                          &battle_config.gx_dupele,                          1        },
		{ "gx_disptype",                        &battle_config.gx_disptype,                        1        },
		{ "devotion_level_difference",          &battle_config.devotion_level_difference,          10       },
		{ "player_skill_partner_check",         &battle_config.player_skill_partner_check,         1        },
		{ "sole_concert_type",                  &battle_config.sole_concert_type,                  2        },
		{ "hide_GM_session",                    &battle_config.hide_GM_session,                    0        },
		{ "invite_request_check",               &battle_config.invite_request_check,               1        },
		{ "gvg_trade_request_refused",          &battle_config.gvg_trade_request_refused,          1        },
		{ "pvp_trade_request_refused",          &battle_config.pvp_trade_request_refused,          1        },
		{ "skill_removetrap_type",              &battle_config.skill_removetrap_type,              0        },
		{ "disp_experience",                    &battle_config.disp_experience,                    0        },
		{ "castle_defense_rate",                &battle_config.castle_defense_rate,                100      },
		{ "riding_weight",                      &battle_config.riding_weight,                      10000    },
		{ "hp_rate",                            &battle_config.hp_rate,                            100      },
		{ "sp_rate",                            &battle_config.sp_rate,                            100      },
		{ "gm_can_drop_lv",                     &battle_config.gm_can_drop_lv,                     0        },
		{ "disp_hpmeter",                       &battle_config.disp_hpmeter,                       0        },
		{ "bone_drop",                          &battle_config.bone_drop,                          0        },
		{ "bone_drop_itemid",                   &battle_config.bone_drop_itemid,                   7005     },
		{ "item_rate_details",                  &battle_config.item_rate_details,                  0        },
		{ "item_rate_1",                        &battle_config.item_rate_1,                        100      },
		{ "item_rate_10",                       &battle_config.item_rate_10,                       100      },
		{ "item_rate_100",                      &battle_config.item_rate_100,                      100      },
		{ "item_rate_1000",                     &battle_config.item_rate_1000,                     100      },
		{ "item_rate_1_min",                    &battle_config.item_rate_1_min,                    1        },
		{ "item_rate_1_max",                    &battle_config.item_rate_1_max,                    9        },
		{ "item_rate_10_min",                   &battle_config.item_rate_10_min,                   10       },
		{ "item_rate_10_max",                   &battle_config.item_rate_10_max,                   99       },
		{ "item_rate_100_min",                  &battle_config.item_rate_100_min,                  100      },
		{ "item_rate_100_max",                  &battle_config.item_rate_100_max,                  999      },
		{ "item_rate_1000_min",                 &battle_config.item_rate_1000_min,                 1000     },
		{ "item_rate_1000_max",                 &battle_config.item_rate_1000_max,                 10000    },
		{ "monster_damage_delay",               &battle_config.monster_damage_delay,               0        },
		{ "card_drop_rate",                     &battle_config.card_drop_rate,                     100      },
		{ "equip_drop_rate",                    &battle_config.equip_drop_rate,                    100      },
		{ "consume_drop_rate",                  &battle_config.consume_drop_rate,                  100      },
		{ "refine_drop_rate",                   &battle_config.refine_drop_rate,                   100      },
		{ "etc_drop_rate",                      &battle_config.etc_drop_rate,                      100      },
		{ "potion_drop_rate",                   &battle_config.potion_drop_rate,                   100      },
		{ "arrow_drop_rate",                    &battle_config.arrow_drop_rate,                    100      },
		{ "petequip_drop_rate",                 &battle_config.petequip_drop_rate,                 100      },
		{ "weapon_drop_rate",                   &battle_config.weapon_drop_rate,                   100      },
		{ "other_drop_rate",                    &battle_config.other_drop_rate,                    100      },
		{ "item_res",                           &battle_config.item_res,                           1        },
		{ "next_exp_limit",                     &battle_config.next_exp_limit,                     150      },
		{ "heal_counterstop",                   &battle_config.heal_counterstop,                   11       },
		{ "finding_ore_drop_rate",              &battle_config.finding_ore_drop_rate,              100      },
		{ "joint_struggle_exp_bonus",           &battle_config.joint_struggle_exp_bonus,           25       },
		{ "joint_struggle_limit",               &battle_config.joint_struggle_limit,               600      },
		{ "pt_bonus_b",                         &battle_config.pt_bonus_b,                         0        },
		{ "pt_bonus_j",                         &battle_config.pt_bonus_j,                         0        },
		{ "mvp_announce",                       &battle_config.mvp_announce,                       0        },
		{ "petowneditem",                       &battle_config.petowneditem,                       0        },
		{ "pet_loot_type",                      &battle_config.pet_loot_type,                      1        },
		{ "buyer_name",                         &battle_config.buyer_name,                         0        },
		{ "noportal_flag",                      &battle_config.noportal_flag,                      0        },
		{ "once_autospell",                     &battle_config.once_autospell,                     1        },
		{ "allow_same_autospell",               &battle_config.allow_same_autospell,               1        },
		{ "combo_delay_lower_limits",           &battle_config.combo_delay_lower_limits,           0        },
		{ "tkcombo_delay_lower_limits",         &battle_config.tkcombo_delay_lower_limits,         0        },
		{ "new_marrige_skill",                  &battle_config.new_marrige_skill,                  1        },
		{ "reveff_plus_addeff",                 &battle_config.reveff_plus_addeff,                 0        },
		{ "summonslave_no_drop",                &battle_config.summonslave_no_drop,                0        },
		{ "summonslave_no_exp",                 &battle_config.summonslave_no_exp,                 0        },
		{ "summonslave_no_mvp",                 &battle_config.summonslave_no_mvp,                 0        },
		{ "summonmonster_no_drop",              &battle_config.summonmonster_no_drop,              0        },
		{ "summonmonster_no_exp",               &battle_config.summonmonster_no_exp,               0        },
		{ "summonmonster_no_mvp",               &battle_config.summonmonster_no_mvp,               0        },
		{ "cannibalize_no_drop",                &battle_config.cannibalize_no_drop,                1        },
		{ "cannibalize_no_exp",                 &battle_config.cannibalize_no_exp,                 1        },
		{ "cannibalize_no_mvp",                 &battle_config.cannibalize_no_mvp,                 1        },
		{ "spheremine_no_drop",                 &battle_config.spheremine_no_drop,                 1        },
		{ "spheremine_no_exp",                  &battle_config.spheremine_no_exp,                  1        },
		{ "spheremine_no_mvp",                  &battle_config.spheremine_no_mvp,                  1        },
		{ "branch_mob_no_drop",                 &battle_config.branch_mob_no_drop,                 0        },
		{ "branch_mob_no_exp",                  &battle_config.branch_mob_no_exp,                  0        },
		{ "branch_mob_no_mvp",                  &battle_config.branch_mob_no_mvp,                  0        },
		{ "branch_boss_no_drop",                &battle_config.branch_boss_no_drop,                0        },
		{ "branch_boss_no_exp",                 &battle_config.branch_boss_no_exp,                 0        },
		{ "branch_boss_no_mvp",                 &battle_config.branch_boss_no_mvp,                 0        },
		{ "pc_hit_stop_type",                   &battle_config.pc_hit_stop_type,                   0        },
		{ "nomanner_mode",                      &battle_config.nomanner_mode,                      1        },
		{ "death_by_unrig_penalty",             &battle_config.death_by_unrig_penalty,             0        },
		{ "dance_and_play_duration",            &battle_config.dance_and_play_duration,            20000    },
		{ "soulcollect_max_fail",               &battle_config.soulcollect_max_fail,               0        },
		{ "gvg_flee_rate",                      &battle_config.gvg_flee_rate,                      100      },
		{ "gvg_flee_penalty",                   &battle_config.gvg_flee_penalty,                   0        },
		{ "equip_sex",                          &battle_config.equip_sex,                          0        },
		{ "noexp_hiding",                       &battle_config.noexp_hiding,                       0        },
		{ "noexp_trickdead",                    &battle_config.noexp_trickdead,                    1        },
		{ "hide_attack",                        &battle_config.hide_attack,                        1        },
		{ "gm_hide_attack_lv",                  &battle_config.gm_hide_attack_lv,                  1        },
		{ "weapon_attack_autospell",            &battle_config.weapon_attack_autospell,            0        },
		{ "magic_attack_autospell",             &battle_config.magic_attack_autospell,             0        },
		{ "misc_attack_autospell",              &battle_config.misc_attack_autospell,              0        },
		{ "magic_attack_drain",                 &battle_config.magic_attack_drain,                 0        },
		{ "misc_attack_drain",                  &battle_config.misc_attack_drain,                  0        },
		{ "magic_attack_drain_enable_type",     &battle_config.magic_attack_drain_enable_type,     2        },
		{ "misc_attack_drain_enable_type",      &battle_config.misc_attack_drain_enable_type,      2        },
		{ "hallucianation_off",                 &battle_config.hallucianation_off,                 0        },
		{ "weapon_reflect_autospell",           &battle_config.weapon_reflect_autospell,           0        },
		{ "magic_reflect_autospell",            &battle_config.magic_reflect_autospell,            0        },
		{ "weapon_reflect_drain",               &battle_config.weapon_reflect_drain,               0        },
		{ "weapon_reflect_drain_enable_type",   &battle_config.weapon_reflect_drain_enable_type,   2        },
		{ "magic_reflect_drain",                &battle_config.magic_reflect_drain,                0        },
		{ "magic_reflect_drain_enable_type",    &battle_config.magic_reflect_drain_enable_type,    2        },
		{ "max_parameter_str",                  &battle_config.max_parameter_str,                  999      },
		{ "max_parameter_agi",                  &battle_config.max_parameter_agi,                  999      },
		{ "max_parameter_vit",                  &battle_config.max_parameter_vit,                  999      },
		{ "max_parameter_int",                  &battle_config.max_parameter_int,                  999      },
		{ "max_parameter_dex",                  &battle_config.max_parameter_dex,                  999      },
		{ "max_parameter_luk",                  &battle_config.max_parameter_luk,                  999      },
		{ "cannibalize_nocost",                 &battle_config.cannibalize_nocost,                 0        },
		{ "spheremine_nocost",                  &battle_config.spheremine_nocost,                  0        },
		{ "demonstration_nocost",               &battle_config.demonstration_nocost,               0        },
		{ "acidterror_nocost",                  &battle_config.acidterror_nocost,                  0        },
		{ "aciddemonstration_nocost",           &battle_config.aciddemonstration_nocost,           0        },
		{ "chemical_nocost",                    &battle_config.chemical_nocost,                    0        },
		{ "slimpitcher_nocost",                 &battle_config.slimpitcher_nocost,                 0        },
		{ "mes_send_type",                      &battle_config.mes_send_type,                      0        },
		{ "allow_assumptop_in_gvg",             &battle_config.allow_assumptop_in_gvg,             1        },
		{ "allow_falconassault_elemet",         &battle_config.allow_falconassault_elemet,         0        },
		{ "allow_guild_invite_in_gvg",          &battle_config.allow_guild_invite_in_gvg,          0        },
		{ "allow_guild_leave_in_gvg",           &battle_config.allow_guild_leave_in_gvg,           0        },
		{ "allow_guild_alliance_in_agit",       &battle_config.allow_guild_alliance_in_agit,       0        },
		{ "guild_skill_available",              &battle_config.guild_skill_available,              1        },
		{ "guild_hunting_skill_available",      &battle_config.guild_hunting_skill_available,      1        },
		{ "guild_skill_check_range",            &battle_config.guild_skill_check_range,            0        },
		{ "allow_guild_skill_in_gvg_only",      &battle_config.allow_guild_skill_in_gvg_only,      1        },
		{ "allow_me_guild_skill",               &battle_config.allow_me_guild_skill,               0        },
		{ "emergencycall_point_type",           &battle_config.emergencycall_point_type,           1        },
		{ "emergencycall_call_limit",           &battle_config.emergencycall_call_limit,           0        },
		{ "allow_guild_skill_in_gvgtime_only",  &battle_config.allow_guild_skill_in_gvgtime_only,  0        },
		{ "guild_skill_in_pvp_limit",           &battle_config.guild_skill_in_pvp_limit,           1        },
		{ "guild_exp_rate",                     &battle_config.guild_exp_rate,                     100      },
		{ "guild_skill_effective_range",        &battle_config.guild_skill_effective_range,        2        },
		{ "tarotcard_display_position",         &battle_config.tarotcard_display_position,         2        },
		{ "serverside_friendlist",              &battle_config.serverside_friendlist,              1        },
		{ "pet0078_hair_id",                    &battle_config.pet0078_hair_id,                    100      },
		{ "job_soul_check",                     &battle_config.job_soul_check,                     1        },
		{ "repeal_die_counter_rate",            &battle_config.repeal_die_counter_rate,            100      },
		{ "disp_job_soul_state_change",         &battle_config.disp_job_soul_state_change,         0        },
		{ "check_knowlege_map",                 &battle_config.check_knowlege_map,                 1        },
		{ "tripleattack_rate_up_keeptime",      &battle_config.tripleattack_rate_up_keeptime,      2000     },
		{ "tk_counter_rate_up_keeptime",        &battle_config.tk_counter_rate_up_keeptime,        2000     },
		{ "allow_skill_without_day",            &battle_config.allow_skill_without_day,            0        },
		{ "save_feel_map",                      &battle_config.save_feel_map,                      1        },
		{ "save_hate_mob",                      &battle_config.save_hate_mob,                      1        },
		{ "twilight_party_check",               &battle_config.twilight_party_check,               1        },
		{ "alchemist_point_type",               &battle_config.alchemist_point_type,               0        },
		{ "marionette_type",                    &battle_config.marionette_type,                    0        },
		{ "max_marionette_str",                 &battle_config.max_marionette_str,                 99       },
		{ "max_marionette_agi",                 &battle_config.max_marionette_agi,                 99       },
		{ "max_marionette_vit",                 &battle_config.max_marionette_vit,                 99       },
		{ "max_marionette_int",                 &battle_config.max_marionette_int,                 99       },
		{ "max_marionette_dex",                 &battle_config.max_marionette_dex,                 99       },
		{ "max_marionette_luk",                 &battle_config.max_marionette_luk,                 99       },
		{ "max_marionette_pk_str",              &battle_config.max_marionette_pk_str,              99       },
		{ "max_marionette_pk_agi",              &battle_config.max_marionette_pk_agi,              99       },
		{ "max_marionette_pk_vit",              &battle_config.max_marionette_pk_vit,              99       },
		{ "max_marionette_pk_int",              &battle_config.max_marionette_pk_int,              99       },
		{ "max_marionette_pk_dex",              &battle_config.max_marionette_pk_dex,              99       },
		{ "max_marionette_pk_luk",              &battle_config.max_marionette_pk_luk,              99       },
		{ "max_marionette_pvp_str",             &battle_config.max_marionette_pvp_str,             99       },
		{ "max_marionette_pvp_agi",             &battle_config.max_marionette_pvp_agi,             99       },
		{ "max_marionette_pvp_vit",             &battle_config.max_marionette_pvp_vit,             99       },
		{ "max_marionette_pvp_int",             &battle_config.max_marionette_pvp_int,             99       },
		{ "max_marionette_pvp_dex",             &battle_config.max_marionette_pvp_dex,             99       },
		{ "max_marionette_pvp_luk",             &battle_config.max_marionette_pvp_luk,             99       },
		{ "max_marionette_gvg_str",             &battle_config.max_marionette_gvg_str,             99       },
		{ "max_marionette_gvg_agi",             &battle_config.max_marionette_gvg_agi,             99       },
		{ "max_marionette_gvg_vit",             &battle_config.max_marionette_gvg_vit,             99       },
		{ "max_marionette_gvg_int",             &battle_config.max_marionette_gvg_int,             99       },
		{ "max_marionette_gvg_dex",             &battle_config.max_marionette_gvg_dex,             99       },
		{ "max_marionette_gvg_luk",             &battle_config.max_marionette_gvg_luk,             99       },
		{ "baby_status_max",                    &battle_config.baby_status_max,                    80       },
		{ "baby_hp_rate",                       &battle_config.baby_hp_rate,                       70       },
		{ "baby_sp_rate",                       &battle_config.baby_sp_rate,                       70       },
		{ "upper_hp_rate",                      &battle_config.upper_hp_rate,                      125      },
		{ "upper_sp_rate",                      &battle_config.upper_sp_rate,                      125      },
		{ "normal_hp_rate",                     &battle_config.normal_hp_rate,                     100      },
		{ "normal_sp_rate",                     &battle_config.normal_sp_rate,                     100      },
		{ "baby_weight_rate",                   &battle_config.baby_weight_rate,                   100      },
		{ "no_emergency_call",                  &battle_config.no_emergency_call,                  0        },
		{ "save_am_pharmacy_success",           &battle_config.save_am_pharmacy_success,           1        },
		{ "save_all_ranking_point_when_logout", &battle_config.save_all_ranking_point_when_logout, 0        },
		{ "soul_linker_battle_mode",            &battle_config.soul_linker_battle_mode,            0        },
		{ "soul_linker_battle_mode_ka",         &battle_config.soul_linker_battle_mode_ka,         0        },
		{ "skillup_type",                       &battle_config.skillup_type,                       1        },
		{ "allow_me_dance_effect",              &battle_config.allow_me_dance_effect,              0        },
		{ "allow_me_concert_effect",            &battle_config.allow_me_concert_effect,            0        },
		{ "allow_me_rokisweil",                 &battle_config.allow_me_rokisweil,                 0        },
		{ "pharmacy_get_point_type",            &battle_config.pharmacy_get_point_type,            0        },
		{ "soulskill_can_be_used_for_myself",   &battle_config.soulskill_can_be_used_for_myself,   0        },
		{ "hermode_wp_check_range",             &battle_config.hermode_wp_check_range,             3        },
		{ "hermode_wp_check",                   &battle_config.hermode_wp_check,                   1        },
		{ "hermode_no_walking",                 &battle_config.hermode_no_walking,                 0        },
		{ "hermode_gvg_only",                   &battle_config.hermode_gvg_only,                   1        },
		{ "atcommand_go_significant_values",    &battle_config.atcommand_go_significant_values,    22       },
		{ "redemptio_penalty_type",             &battle_config.redemptio_penalty_type,             1        },
		{ "allow_weaponrearch_to_weaponrefine", &battle_config.allow_weaponrearch_to_weaponrefine, 0        },
		{ "boss_no_knockbacking",               &battle_config.boss_no_knockbacking,               1        },
		{ "boss_no_element_change",             &battle_config.boss_no_element_change,             0        },
		{ "boss_teleport_on_landprotector",     &battle_config.boss_teleport_on_landprotector,     1        },
		{ "scroll_produce_rate",                &battle_config.scroll_produce_rate,                100      },
		{ "scroll_item_name_input",             &battle_config.scroll_item_name_input,             0        },
		{ "pet_leave",                          &battle_config.pet_leave,                          1        },
		{ "pk_short_attack_damage_rate",        &battle_config.pk_short_damage_rate,               80       },
		{ "pk_long_attack_damage_rate",         &battle_config.pk_long_damage_rate,                70       },
		{ "pk_magic_attack_damage_rate",        &battle_config.pk_magic_damage_rate,               60       },
		{ "pk_misc_attack_damage_rate",         &battle_config.pk_misc_damage_rate,                100      },
		{ "cooking_rate",                       &battle_config.cooking_rate,                       100      },
		{ "making_rate",                        &battle_config.making_rate,                        100      },
		{ "extended_abracadabra",               &battle_config.extended_abracadabra,               0        },
		{ "no_pk_level",                        &battle_config.no_pk_level,                        60       },
		{ "allow_cloneskill_at_autospell",      &battle_config.allow_cloneskill_at_autospell,      0        },
		{ "pk_noshift",                         &battle_config.pk_noshift,                         0        },
		{ "pk_penalty_time",                    &battle_config.pk_penalty_time,                    60000    },
		{ "dropitem_itemrate_fix",              &battle_config.dropitem_itemrate_fix,              1        },
		{ "gm_nomanner_lv",                     &battle_config.gm_nomanner_lv,                     50       },
		{ "clif_fixpos_type",                   &battle_config.clif_fixpos_type,                   1        },
		{ "romailuse",                          &battle_config.romail,                             0        },
		{ "pc_die_script",                      &battle_config.pc_die_script,                      0        },
		{ "pc_kill_script",                     &battle_config.pc_kill_script,                     0        },
		{ "pc_movemap_script",                  &battle_config.pc_movemap_script,                  0        },
		{ "pc_login_script",                    &battle_config.pc_login_script,                    0        },
		{ "pc_logout_script",                   &battle_config.pc_logout_script,                   0        },
		{ "save_pckiller_type",                 &battle_config.save_pckiller_type,                 0        },
		{ "def_ratio_atk_to_shieldchain",       &battle_config.def_ratio_atk_to_shieldchain,       0        },
		{ "def_ratio_atk_to_carttermination",   &battle_config.def_ratio_atk_to_carttermination,   0        },
		{ "player_gravitation_type",            &battle_config.player_gravitation_type,            0        },
		{ "enemy_gravitation_type",             &battle_config.enemy_gravitation_type,             0        },
		{ "mob_attack_fixwalkpos",              &battle_config.mob_attack_fixwalkpos,              0        },
		{ "mob_ai_limiter",                     &battle_config.mob_ai_limiter,                     0        },
		{ "mob_ai_cpu_usage",                   &battle_config.mob_ai_cpu_usage,                   80       },
		{ "itemidentify",                       &battle_config.itemidentify,                       0        },
		{ "casting_penalty_type",               &battle_config.casting_penalty_type,               0        },
		{ "casting_penalty_weapon",             &battle_config.casting_penalty_weapon,             0        },
		{ "casting_penalty_shield",             &battle_config.casting_penalty_shield,             0        },
		{ "casting_penalty_armor",              &battle_config.casting_penalty_armor,              0        },
		{ "casting_penalty_helm",               &battle_config.casting_penalty_helm,               0        },
		{ "casting_penalty_robe",               &battle_config.casting_penalty_robe,               0        },
		{ "casting_penalty_shoes",              &battle_config.casting_penalty_shoes,              0        },
		{ "casting_penalty_acce",               &battle_config.casting_penalty_acce,               0        },
		{ "casting_penalty_arrow",              &battle_config.casting_penalty_arrow,              0        },
		{ "show_always_party_name",             &battle_config.show_always_party_name,             0        },
		{ "check_player_name_global_msg",       &battle_config.check_player_name_global_msg,       0        },
		{ "check_player_name_party_msg",        &battle_config.check_player_name_party_msg,        0        },
		{ "check_player_name_guild_msg",        &battle_config.check_player_name_guild_msg,        0        },
		{ "save_player_when_drop_item",         &battle_config.save_player_when_drop_item,         0        },
		{ "save_player_when_storage_closed",    &battle_config.save_player_when_storage_closed,    0        },
		{ "allow_homun_status_change",          &battle_config.allow_homun_status_change,          0        },
		{ "save_homun_temporal_intimate",       &battle_config.save_homun_temporal_intimate,       1        },
		{ "homun_intimate_rate",                &battle_config.homun_intimate_rate,                100      },
		{ "homun_temporal_intimate_resilience", &battle_config.homun_temporal_intimate_resilience, 50       },
		{ "hvan_explosion_intimate",            &battle_config.hvan_explosion_intimate,            45000    },
		{ "homun_speed_is_same_as_pc",          &battle_config.homun_speed_is_same_as_pc,          1        },
		{ "homun_skill_intimate_type",          &battle_config.homun_skill_intimate_type,          0        },
		{ "master_get_homun_base_exp",          &battle_config.master_get_homun_base_exp,          100      },
		{ "master_get_homun_job_exp",           &battle_config.master_get_homun_job_exp,           0        },
		{ "extra_system_flag",                  &battle_config.extra_system_flag,                  0        },
		{ "mob_take_over_sp",                   &battle_config.mob_take_over_sp,                   0        },
		{ "party_join_limit",                   &battle_config.party_join_limit,                   1        },
		{ "check_skillpos_range",               &battle_config.check_skillpos_range,               0        },
		{ "pet_speed_is_same_as_pc",            &battle_config.pet_speed_is_same_as_pc,            1        },
		{ "tax_rate",                           &battle_config.tax_rate,                           0        },
		{ "steal_rate",                         &battle_config.steal_rate,                         100      },
		{ "sw_def_type",                        &battle_config.sw_def_type,                        3        },
		{ "calc_dist_flag",                     &battle_config.calc_dist_flag,                     3        },
		{ "allow_sw_dist",                      &battle_config.allow_sw_dist,                      4        },
		{ "personal_storage_sort",              &battle_config.personal_storage_sort,              1        },
		{ "guild_storage_sort",                 &battle_config.guild_storage_sort,                 1        },
		{ "allow_es_magic_all",                 &battle_config.allow_es_magic_all,                 0        },
		{ "trap_is_invisible",                  &battle_config.trap_is_invisible,                  0        },
		{ "gm_perfect_hide",                    &battle_config.gm_perfect_hide,                    0        },
		{ "pcview_mob_clear_type",              &battle_config.pcview_mob_clear_type,              1        },
		{ "party_item_share_type",              &battle_config.party_item_share_type,              1        },
		{ "party_item_share_show",              &battle_config.party_item_share_show,              0        },
		{ "pk_murderer_point",                  &battle_config.pk_murderer_point,                  100      },
		{ "sg_miracle_rate",                    &battle_config.sg_miracle_rate,                    1        },
		{ "baby_copy_skilltree",                &battle_config.baby_copy_skilltree,                1        },
		{ "skill_autospell_delay_enable",       &battle_config.skill_autospell_delay_enable,       1        },
		{ "bonus_autospell_delay_enable",       &battle_config.bonus_autospell_delay_enable,       1        },
		{ "merc_speed_is_same_as_pc",           &battle_config.merc_speed_is_same_as_pc,           1        },
		{ "master_get_merc_base_exp",           &battle_config.master_get_merc_base_exp,           100      },
		{ "master_get_merc_job_exp",            &battle_config.master_get_merc_job_exp,            0        },
		{ "party_invite_range_check",           &battle_config.party_invite_range_check,           0        },
		{ "homun_skilldelay_reset",             &battle_config.homun_skilldelay_reset,             1        },
		{ "homun_statuschange_reset",           &battle_config.homun_statuschange_reset,           1        },
		{ "free_sc_data_dynamically",           &battle_config.free_sc_data_dynamically,           0        },
		{ "snovice_maxexp_border",              &battle_config.snovice_maxexp_border,              99999999 },
		{ "homun_rename",                       &battle_config.homun_rename,                       0        },
		{ NULL,                                 NULL,                                              0        },

	};

	if(count++ == 0) {
		for(i=0; data[i].val; i++) {
			*data[i].val = data[i].defval;
		}
	}

	fp = fopen(cfgName,"r");
	if(fp == NULL) {
		printf("file not found: %s\n",cfgName);
		return 1;
	}
	while(fgets(line,1020,fp)) {
		if(line[0] == '/' && line[1] == '/')
			continue;
		if(sscanf(line,"%[^:]:%s",w1,w2) != 2)
			continue;

		if(strcmpi(w1,"import") == 0) {
			battle_config_read(w2);
		} else {
			for(i=0; data[i].val; i++) {
				if(strcmpi(w1,data[i].str) == 0) {
					*data[i].val = battle_config_switch(w2);
					break;
				}
			}
			if(data[i].val == NULL)
				printf("unknown battle config : %s\a\n",w1);
		}
	}
	fclose(fp);

	// �t���O����
	if(battle_config.allow_guild_skill_in_gvgtime_only)
		battle_config.guild_skill_available = 0;

	if(--count == 0) {
		if(battle_config.flooritem_lifetime < 1000)
			battle_config.flooritem_lifetime = 60*1000;
		if(battle_config.restart_hp_rate < 0)
			battle_config.restart_hp_rate = 0;
		else if(battle_config.restart_hp_rate > 100)
			battle_config.restart_hp_rate = 100;
		if(battle_config.restart_sp_rate < 0)
			battle_config.restart_sp_rate = 0;
		else if(battle_config.restart_sp_rate > 100)
			battle_config.restart_sp_rate = 100;
		if(battle_config.natural_healhp_interval < NATURAL_HEAL_INTERVAL)
			battle_config.natural_healhp_interval = NATURAL_HEAL_INTERVAL;
		if(battle_config.natural_healsp_interval < NATURAL_HEAL_INTERVAL)
			battle_config.natural_healsp_interval = NATURAL_HEAL_INTERVAL;
		if(battle_config.natural_heal_skill_interval < NATURAL_HEAL_INTERVAL)
			battle_config.natural_heal_skill_interval = NATURAL_HEAL_INTERVAL;
		if(battle_config.natural_heal_weight_rate < 50)
			battle_config.natural_heal_weight_rate = 50;
		else if(battle_config.natural_heal_weight_rate > 101)
			battle_config.natural_heal_weight_rate = 101;
		battle_config.monster_max_aspd = 2000 - battle_config.monster_max_aspd*10;
		if(battle_config.monster_max_aspd < 10)
			battle_config.monster_max_aspd = 10;
		else if(battle_config.monster_max_aspd > 1000)
			battle_config.monster_max_aspd = 1000;
		battle_config.max_aspd = 2000 - battle_config.max_aspd*10;
		if(battle_config.max_aspd < 10)
			battle_config.max_aspd = 10;
		else if(battle_config.max_aspd > 1000)
			battle_config.max_aspd = 1000;
		battle_config.pk_max_aspd = 2000 - battle_config.pk_max_aspd*10;
		if(battle_config.pk_max_aspd < 10)
			battle_config.pk_max_aspd = 10;
		else if(battle_config.pk_max_aspd > 1000)
			battle_config.pk_max_aspd = 1000;
		battle_config.gvg_max_aspd = 2000 - battle_config.gvg_max_aspd*10;
		if(battle_config.gvg_max_aspd < 10)
			battle_config.gvg_max_aspd = 10;
		else if(battle_config.gvg_max_aspd > 1000)
			battle_config.gvg_max_aspd = 1000;
		battle_config.pvp_max_aspd = 2000 - battle_config.pvp_max_aspd*10;
		if(battle_config.pvp_max_aspd < 10)
			battle_config.pvp_max_aspd = 10;
		else if(battle_config.pvp_max_aspd > 1000)
			battle_config.pvp_max_aspd = 1000;
		if(battle_config.hp_rate < 0)
			battle_config.hp_rate = 1;
		if(battle_config.sp_rate < 0)
			battle_config.sp_rate = 1;
		if(battle_config.max_hp > 1000000)
			battle_config.max_hp = 1000000;
		else if(battle_config.max_hp < 100)
			battle_config.max_hp = 100;
		if(battle_config.max_sp > 1000000)
			battle_config.max_sp = 1000000;
		else if(battle_config.max_sp < 100)
			battle_config.max_sp = 100;

		if(battle_config.max_parameter < 10)
			battle_config.max_parameter = 10;
		else if(battle_config.max_parameter > 10000)
			battle_config.max_parameter = 10000;
		if(battle_config.max_parameter_str < 1)
			battle_config.max_parameter_str = 1;
		else if(battle_config.max_parameter_str > battle_config.max_parameter)
			battle_config.max_parameter_str = battle_config.max_parameter;
		if(battle_config.max_parameter_agi < 1)
			battle_config.max_parameter_agi = 1;
		else if(battle_config.max_parameter_agi > battle_config.max_parameter)
			battle_config.max_parameter_agi = battle_config.max_parameter;
		if(battle_config.max_parameter_vit < 1)
			battle_config.max_parameter_vit = 1;
		else if(battle_config.max_parameter_vit > battle_config.max_parameter)
			battle_config.max_parameter_vit = battle_config.max_parameter;
		if(battle_config.max_parameter_int < 1)
			battle_config.max_parameter_int = 1;
		else if(battle_config.max_parameter_int > battle_config.max_parameter)
			battle_config.max_parameter_int = battle_config.max_parameter;
		if(battle_config.max_parameter_dex < 1)
			battle_config.max_parameter_dex = 1;
		else if(battle_config.max_parameter_dex > battle_config.max_parameter)
			battle_config.max_parameter_dex = battle_config.max_parameter;
		if(battle_config.max_parameter_luk < 1)
			battle_config.max_parameter_luk = 1;
		else if(battle_config.max_parameter_luk > battle_config.max_parameter)
			battle_config.max_parameter_luk = battle_config.max_parameter;

		if(battle_config.max_cart_weight > 1000000)
			battle_config.max_cart_weight = 1000000;
		else if(battle_config.max_cart_weight < 100)
			battle_config.max_cart_weight = 100;
		battle_config.max_cart_weight *= 10;

		if(battle_config.agi_penalty_count < 2)
			battle_config.agi_penalty_count = 2;
		if(battle_config.vit_penalty_count < 2)
			battle_config.vit_penalty_count = 2;

		if(battle_config.guild_exp_limit > 99)
			battle_config.guild_exp_limit = 99;
		if(battle_config.guild_exp_limit < 0)
			battle_config.guild_exp_limit = 0;
		if(battle_config.pet_weight < 0)
			battle_config.pet_weight = 0;

		if(battle_config.castle_defense_rate < 0)
			battle_config.castle_defense_rate = 0;
		if(battle_config.castle_defense_rate > 100)
			battle_config.castle_defense_rate = 100;

		if(battle_config.next_exp_limit < 0)
			battle_config.next_exp_limit = 150;

		if(battle_config.card_drop_rate < 0)
			battle_config.card_drop_rate = 0;
		if(battle_config.equip_drop_rate < 0)
			battle_config.equip_drop_rate = 0;
		if(battle_config.consume_drop_rate < 0)
			battle_config.consume_drop_rate = 0;
		if(battle_config.refine_drop_rate < 0)
			battle_config.refine_drop_rate = 0;
		if(battle_config.etc_drop_rate < 0)
			battle_config.etc_drop_rate = 0;

		if(battle_config.potion_drop_rate < 0)
			battle_config.potion_drop_rate = 0;
		if(battle_config.arrow_drop_rate < 0)
			battle_config.arrow_drop_rate = 0;
		if(battle_config.petequip_drop_rate < 0)
			battle_config.petequip_drop_rate = 0;
		if(battle_config.weapon_drop_rate < 0)
			battle_config.weapon_drop_rate = 0;
		if(battle_config.other_drop_rate < 0)
			battle_config.other_drop_rate = 0;

		if(battle_config.heal_counterstop < 0)
			battle_config.heal_counterstop = 0;
		if (battle_config.finding_ore_drop_rate < 0)
			battle_config.finding_ore_drop_rate = 0;
		else if (battle_config.finding_ore_drop_rate > 10000)
			battle_config.finding_ore_drop_rate = 10000;

		if(battle_config.max_marionette_str < 1)
			battle_config.max_marionette_str = battle_config.max_parameter;
		if(battle_config.max_marionette_agi < 1)
			battle_config.max_marionette_agi = battle_config.max_parameter;
		if(battle_config.max_marionette_vit < 1)
			battle_config.max_marionette_vit = battle_config.max_parameter;
		if(battle_config.max_marionette_int < 1)
			battle_config.max_marionette_int = battle_config.max_parameter;
		if(battle_config.max_marionette_dex < 1)
			battle_config.max_marionette_dex = battle_config.max_parameter;
		if(battle_config.max_marionette_luk < 1)
			battle_config.max_marionette_luk = battle_config.max_parameter;

		if(battle_config.max_marionette_pk_str < 1)
			battle_config.max_marionette_pk_str = battle_config.max_parameter;
		if(battle_config.max_marionette_pk_agi < 1)
			battle_config.max_marionette_pk_agi = battle_config.max_parameter;
		if(battle_config.max_marionette_pk_vit < 1)
			battle_config.max_marionette_pk_vit = battle_config.max_parameter;
		if(battle_config.max_marionette_pk_int < 1)
			battle_config.max_marionette_pk_int = battle_config.max_parameter;
		if(battle_config.max_marionette_pk_dex < 1)
			battle_config.max_marionette_pk_dex = battle_config.max_parameter;
		if(battle_config.max_marionette_pk_luk < 1)
			battle_config.max_marionette_pk_luk = battle_config.max_parameter;

		if(battle_config.max_marionette_pvp_str < 1)
			battle_config.max_marionette_pvp_str = battle_config.max_parameter;
		if(battle_config.max_marionette_pvp_agi < 1)
			battle_config.max_marionette_pvp_agi = battle_config.max_parameter;
		if(battle_config.max_marionette_pvp_vit < 1)
			battle_config.max_marionette_pvp_vit = battle_config.max_parameter;
		if(battle_config.max_marionette_pvp_int < 1)
			battle_config.max_marionette_pvp_int = battle_config.max_parameter;
		if(battle_config.max_marionette_pvp_dex < 1)
			battle_config.max_marionette_pvp_dex = battle_config.max_parameter;
		if(battle_config.max_marionette_pvp_luk < 1)
			battle_config.max_marionette_pvp_luk = battle_config.max_parameter;

		if(battle_config.max_marionette_gvg_str < 1)
			battle_config.max_marionette_gvg_str = battle_config.max_parameter;
		if(battle_config.max_marionette_gvg_agi < 1)
			battle_config.max_marionette_gvg_agi = battle_config.max_parameter;
		if(battle_config.max_marionette_gvg_vit < 1)
			battle_config.max_marionette_gvg_vit = battle_config.max_parameter;
		if(battle_config.max_marionette_gvg_int < 1)
			battle_config.max_marionette_gvg_int = battle_config.max_parameter;
		if(battle_config.max_marionette_gvg_dex < 1)
			battle_config.max_marionette_gvg_dex = battle_config.max_parameter;
		if(battle_config.max_marionette_gvg_luk < 1)
			battle_config.max_marionette_gvg_luk = battle_config.max_parameter;
		if(battle_config.tax_rate < 0)
			battle_config.tax_rate = 0;
		if(battle_config.tax_rate > 100)
			battle_config.tax_rate = 100;
		if(battle_config.steal_rate < 0)
			battle_config.steal_rate = 0;
		if(battle_config.pk_murderer_point < 0)
			battle_config.pk_murderer_point = 0;
	}

	return 0;
}

/*==========================================
 * ������
 *------------------------------------------
 */
int do_init_battle(void)
{
	add_timer_func_list(battle_delay_damage_sub, "battle_delay_damage_sub");

	return 0;
}

/*==========================================
 * �I��
 *------------------------------------------
 */
int do_final_battle(void)
{
	// nothing to do
	return 0;
}
