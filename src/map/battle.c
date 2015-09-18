#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "battle.h"

#include "timer.h"
#include "nullpo.h"
#include "malloc.h"

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

#ifdef MEMWATCH
#include "memwatch.h"
#endif

int attr_fix_table[4][10][10];

struct Battle_Config battle_config;

//-------------------------------------------------------------------

// �_���[�W�̒x��
struct battle_delay_damage_ {
	struct block_list *src;
	int target;
	int damage;
	int flag;
};
int battle_delay_damage_sub(int tid,unsigned int tick,int id,int data)
{
	struct battle_delay_damage_ *dat=(struct battle_delay_damage_ *)data;
	struct block_list *target=map_id2bl(dat->target);
	if( dat && map_id2bl(id)==dat->src && target && target->prev!=NULL)
		battle_damage(dat->src,target,dat->damage,dat->flag);
	free(dat);
	return 0;
}
int battle_delay_damage(unsigned int tick,struct block_list *src,struct block_list *target,int damage,int flag)
{
	struct battle_delay_damage_ *dat = (struct battle_delay_damage_*)aCalloc(1,sizeof(struct battle_delay_damage_));

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	dat->src=src;
	dat->target=target->id;
	dat->damage=damage;
	dat->flag=flag;
	add_timer2(tick,battle_delay_damage_sub,src->id,(int)dat,TIMER_FREE_DATA);
	return 0;
}

// ���ۂ�HP�𑀍�
int battle_damage(struct block_list *bl,struct block_list *target,int damage,int flag)
{
	struct map_session_data *sd=NULL;
	struct status_change *sc_data;
	short *sc_count;
	int race = 7, ele = 0;

	nullpo_retr(0, target); //bl��NULL�ŌĂ΂�邱�Ƃ�����̂ő��Ń`�F�b�N

	if(damage==0 || target->type == BL_PET)
		return 0;

	if(target->prev == NULL)
		return 0;

	if(bl) {
		if(bl->prev==NULL)
			return 0;
		if(bl->type==BL_PC)
			sd=(struct map_session_data *)bl;
	}

	if(damage<0)
		return battle_heal(bl,target,-damage,0,flag);

	sc_data = status_get_sc_data(target);

	if((sc_count=status_get_sc_count(target))!=NULL && *sc_count>0){
		// �����A�Ή��A����������
		if(sc_data[SC_FREEZE].timer!=-1)
			status_change_end(target,SC_FREEZE,-1);
		if(sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
			status_change_end(target,SC_STONE,-1);
		if(sc_data[SC_SLEEP].timer!=-1)
			status_change_end(target,SC_SLEEP,-1);
	}

	// �푰�E�����擾
	race = status_get_race(target);
	ele  = status_get_elem_type(target);

	map_freeblock_lock();

	if(target->type==BL_MOB){	// MOB
		struct mob_data *md=(struct mob_data *)target;
		if(md && md->ud.skilltimer!=-1 && md->ud.state.skillcastcancel)	// �r���W�Q
			unit_skillcastcancel(target,0);
		mob_damage(bl,md,damage,0);

		if(sd && md && md->bl.prev != NULL && !unit_isdead(&md->bl) && flag&(BF_WEAPON|BF_NORMAL) && status_get_class(target) != 1288)
		{
			int mode = status_get_mode(target);
			// �J�[�h���ʂ̃R�[�}�E����
			if(atn_rand()%10000 < sd->weapon_coma_ele[ele] ||
				atn_rand()%10000 < sd->weapon_coma_race[race] ||
				(mode&0x20 && atn_rand()%10000 < sd->weapon_coma_race[10]) ||
				(!(mode&0x20) && atn_rand()%10000 < sd->weapon_coma_race[11]))
					mob_damage(bl,md,status_get_hp(target),0);

			else if(atn_rand()%10000 < sd->weapon_coma_ele2[ele] ||
				atn_rand()%10000 < sd->weapon_coma_race2[race] ||
				(mode&0x20 && atn_rand()%10000 < sd->weapon_coma_race2[10]) ||
				(!(mode&0x20) && atn_rand()%10000 < sd->weapon_coma_race2[11]))
					mob_damage(bl,md,status_get_hp(target)-1,0);
		}
	}
	else if(target->type==BL_PC){	// PC
		struct map_session_data *tsd=(struct map_session_data *)target;

		if(tsd && tsd->sc_data && tsd->sc_data[SC_DEVOTION].val1){	// �f�B�{�[�V�������������Ă���
			struct map_session_data *md = map_id2sd(tsd->sc_data[SC_DEVOTION].val1);
			if(md && skill_devotion3(md,target->id)){
				skill_devotion(md,target->id);
			}
			else if(md && bl) {
				int i;
				for(i=0;i<5;i++) {
					if(md->dev.val1[i] == target->id) {
						clif_damage(&md->bl, &md->bl, gettick(), 0, 0, damage, 0, 9, 0);
						pc_damage(&md->bl,md,damage);

						if(sd && md && md->bl.prev != NULL && !unit_isdead(&md->bl) && flag&(BF_WEAPON|BF_NORMAL))
						{
							// �J�[�h���ʂ̃R�[�}�E����
							if(atn_rand()%10000 < sd->weapon_coma_ele[ele] ||
								atn_rand()%10000 < sd->weapon_coma_race[race] ||
								atn_rand()%10000 < sd->weapon_coma_race[11])
								pc_damage(&md->bl,md,status_get_hp(target));

							else if(atn_rand()%10000 < sd->weapon_coma_ele2[ele] ||
								atn_rand()%10000 < sd->weapon_coma_race2[race] ||
								atn_rand()%10000 < sd->weapon_coma_race2[11])
								pc_damage(&md->bl,md,status_get_hp(target)-1);
						}
						map_freeblock_unlock();
						return 0;
					}
				}
			}
		}

		if(tsd && tsd->ud.skilltimer!=-1){	// �r���W�Q
				// �t�F���J�[�h��W�Q����Ȃ��X�L�����̌���
			if( (!tsd->special_state.no_castcancel || map[bl->m].flag.gvg) && tsd->ud.state.skillcastcancel &&
				!tsd->special_state.no_castcancel2)
			{
				unit_skillcastcancel(target,0);
			}
		}

		pc_damage(bl,tsd,damage);

		if(sd && tsd && tsd->bl.prev != NULL && !unit_isdead(&tsd->bl) && flag&(BF_WEAPON|BF_NORMAL))
		{
			// �J�[�h���ʂ̃R�[�}�E����
			if(atn_rand()%10000 < sd->weapon_coma_ele[ele] ||
			   atn_rand()%10000 < sd->weapon_coma_race[race] ||
			   atn_rand()%10000 < sd->weapon_coma_race[11])
				pc_damage(bl,tsd,status_get_hp(target));

			else if(atn_rand()%10000 < sd->weapon_coma_ele2[ele] ||
			   atn_rand()%10000 < sd->weapon_coma_race2[race] ||
			   atn_rand()%10000 < sd->weapon_coma_race2[11])
				pc_damage(bl,tsd,status_get_hp(target)-1);
		}
	}
	else if(target->type==BL_HOM){	// HOM
		struct homun_data *hd=(struct homun_data *)target;
		if(hd && hd->ud.skilltimer!=-1 && hd->ud.state.skillcastcancel)	// �r���W�Q
			unit_skillcastcancel(target,0);
		homun_damage(bl,hd,damage);

		if(sd && hd && hd->bl.prev != NULL && !unit_isdead(&hd->bl) && flag&(BF_WEAPON|BF_NORMAL) && status_get_class(target) != 1288)
		{
			int mode = status_get_mode(target);
			// �J�[�h���ʂ̃R�[�}�E����
			if(atn_rand()%10000 < sd->weapon_coma_ele[ele] ||
				atn_rand()%10000 < sd->weapon_coma_race[race] ||
				(mode&0x20 && atn_rand()%10000 < sd->weapon_coma_race[10]) ||
				(!(mode&0x20) && atn_rand()%10000 < sd->weapon_coma_race[11]))
					homun_damage(bl,hd,status_get_hp(target));

			else if(atn_rand()%10000 < sd->weapon_coma_ele2[ele] ||
				atn_rand()%10000 < sd->weapon_coma_race2[race] ||
				(mode&0x20 && atn_rand()%10000 < sd->weapon_coma_race2[10]) ||
				(!(mode&0x20) && atn_rand()%10000 < sd->weapon_coma_race2[11]))
					homun_damage(bl,hd,status_get_hp(target)-1);
		}
	}
	else if(target->type==BL_SKILL)
		skill_unit_ondamaged((struct skill_unit *)target,bl,damage,gettick());

	map_freeblock_unlock();
	return 0;
}

int battle_heal(struct block_list *bl,struct block_list *target,int hp,int sp,int flag)
{
	nullpo_retr(0, target); //bl��NULL�ŌĂ΂�邱�Ƃ�����̂ő��Ń`�F�b�N

	if(target->type == BL_PET)
		return 0;
	if( unit_isdead(target) )
		return 0;
	if(hp==0 && sp==0)
		return 0;

	if(hp<0)
		return battle_damage(bl,target,-hp,flag);

	if(target->type==BL_MOB)
		return mob_heal((struct mob_data *)target,hp);
	else if(target->type==BL_PC)
		return pc_heal((struct map_session_data *)target,hp,sp);
	else if(target->type==BL_HOM)
		return homun_heal((struct homun_data *)target,hp,sp);
	return 0;
}

/*==========================================
 * �_���[�W�̑����C��
 *------------------------------------------
 */
int battle_attr_fix(int damage,int atk_elem,int def_elem)
{
	int def_type= def_elem%10, def_lv=def_elem/10/2;

	if( atk_elem == 10 )
		atk_elem = atn_rand()%9;	//���푮�������_���ŕt��

	// ��������(!=������)
	if (atk_elem == -1)
		return damage;

	if(	atk_elem<0 || atk_elem>9 || def_type<0 || def_type>9 ||
		def_lv<1 || def_lv>4){	// �����l�����������̂łƂ肠�������̂܂ܕԂ�
		if(battle_config.error_log)
			printf("battle_attr_fix: unknown attr type: atk=%d def_type=%d def_lv=%d\n",atk_elem,def_type,def_lv);
		return damage;
	}

	return ((damage > 0)? damage*attr_fix_table[def_lv-1][atk_elem][def_type]/100: 0);
}

/*==========================================
 * �_���[�W�ŏI�v�Z
 *------------------------------------------
 */
int battle_calc_damage(struct block_list *src,struct block_list *bl,int damage,int div_,int skill_num,int skill_lv,int flag)
{
	struct map_session_data *tsd=NULL;
	struct mob_data *tmd=NULL;
	//struct homun_data *thd=NULL;
	struct status_change *sc_data,*sc;
	short *sc_count;
	int class;
	unsigned int tick = gettick();

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	tsd = BL_DOWNCAST( BL_PC,  bl );
	tmd = BL_DOWNCAST( BL_MOB, bl );
	//thd = BL_DOWNCAST( BL_HOM, bl );

	sc_data = status_get_sc_data(bl);
	sc_count= status_get_sc_count(bl);
	class = status_get_class(bl);

	//�X�L���_���[�W�␳
	if(damage > 0 && skill_num > 0){
		int damage_rate=100;
		if(map[bl->m].flag.normal)
			damage_rate = skill_get_damage_rate(skill_num,0);
		else if(map[bl->m].flag.gvg)
			damage_rate = skill_get_damage_rate(skill_num,2);
		else if(map[bl->m].flag.pvp)
			damage_rate = skill_get_damage_rate(skill_num,1);
		else if(map[bl->m].flag.pk)
			damage_rate = skill_get_damage_rate(skill_num,3);
		if(damage_rate!=100)
			damage = damage*damage_rate/100;
	}
	if(sc_count!=NULL && *sc_count>0 && sc_data){
		if(sc_data[SC_ASSUMPTIO].timer != -1 && damage > 0 && skill_num!=PA_PRESSURE && skill_num!=HW_GRAVITATION) { //�A�X���v�e�B�I
			if(map[bl->m].flag.pvp || map[bl->m].flag.gvg)
				damage=damage*2/3;
			else
				damage=damage/2;
		}

		// �S�X�y���̓����Ԉُ�
		if(sc_data[SC_INCDAMAGE].timer != -1 && damage > 0 && skill_num!=PA_PRESSURE && skill_num!=HW_GRAVITATION)
			damage += damage * sc_data[SC_INCDAMAGE].val1/100;

		if (sc_data[SC_BASILICA].timer!=-1 && damage > 0 && !(status_get_mode(src)&0x20))
			damage = 0;

		if (sc_data[SC_FOGWALL].timer!=-1 && damage>0 && flag&BF_WEAPON && flag&BF_LONG)
		{
			if(skill_num == 0)//�ʏ�U��75%OFF
			{
				damage = damage*25/100;
			}else{	//�X�L��25%OFF
				damage = damage*75/100;
			}
		}

		if (sc_data[SC_SAFETYWALL].timer!=-1 &&	flag&BF_SHORT && skill_num != NPC_GUIDEDATTACK) {
			// �Z�[�t�e�B�E�H�[���@SW�̎d�l�g���ɔ����ĕ�������Ȃ���_���t���O�͍폜���܂���
			struct skill_unit *unit = map_id2su(sc_data[SC_SAFETYWALL].val2);
			if (unit && unit->group ) {
				if ((--unit->group->val2)<=0)
					skill_delunit(unit);
				damage=0;
			} else {
				status_change_end(bl,SC_SAFETYWALL,-1);
			}
		}

		// �j���[�}
		if((sc_data[SC_PNEUMA].timer!=-1 || sc_data[SC_TATAMIGAESHI].timer != -1) && damage>0 && flag&(BF_WEAPON|BF_MISC) && flag&BF_LONG && skill_num != NPC_GUIDEDATTACK){
			damage=0;
		}
		if(sc_data[SC_AETERNA].timer!=-1 && damage>0 && skill_num!=PA_PRESSURE && skill_num!=HW_GRAVITATION){	// ���b�N�X�G�[�e���i
			damage<<=1;
			status_change_end( bl,SC_AETERNA,-1 );
		}

		//������̃_���[�W����
		if(sc_data[SC_VOLCANO].timer!=-1 && damage>0){	// �{���P�[�m
			if(flag&BF_SKILL && skill_get_pl(skill_num)==3)
				damage += damage*sc_data[SC_VOLCANO].val4/100;
			else if(!flag&BF_SKILL && status_get_attack_element(bl)==3)
				damage += damage*sc_data[SC_VOLCANO].val4/100;
		}

		if(sc_data[SC_VIOLENTGALE].timer!=-1 && damage>0){	// �o�C�I�����g�Q�C��
			if(flag&BF_SKILL && skill_get_pl(skill_num)==4)
				damage += damage*sc_data[SC_VIOLENTGALE].val4/100;
			else if(!flag&BF_SKILL && status_get_attack_element(bl)==4)
				damage += damage*sc_data[SC_VIOLENTGALE].val4/100;
		}

		if(sc_data[SC_DELUGE].timer!=-1 && damage>0){	// �f�����[�W
			if(flag&BF_SKILL && skill_get_pl(skill_num)==1)
				damage += damage*sc_data[SC_DELUGE].val4/100;
			else if(!flag&BF_SKILL && status_get_attack_element(bl)==1)
				damage += damage*sc_data[SC_DELUGE].val4/100;
		}

		if(sc_data[SC_ENERGYCOAT].timer!=-1 && damage>0  && flag&BF_WEAPON && skill_num != PA_PRESSURE){	// �G�i�W�[�R�[�g �v���b�V���[�͌y�����Ȃ�
			if(tsd){
				if(tsd->status.sp>0){
					int per = tsd->status.sp * 5 / (tsd->status.max_sp + 1);
					tsd->status.sp -= tsd->status.sp * (per * 5 + 10) / 1000;
					if( tsd->status.sp < 0 ) tsd->status.sp = 0;
					damage -= damage * ((per+1) * 6) / 100;
					clif_updatestatus(tsd,SP_SP);
				}
				if(tsd->status.sp<=0)
					status_change_end( bl,SC_ENERGYCOAT,-1 );
			}
			else
				damage -= damage * (sc_data[SC_ENERGYCOAT].val1 * 6) / 100;
		}

		if(sc_data[SC_KYRIE].timer!=-1 && damage > 0){	// �L���G�G���C�\��
			sc=&sc_data[SC_KYRIE];
			sc->val2-=damage;
			if(flag&BF_WEAPON){
				if(sc->val2>=0)	damage=0;
				else damage=-sc->val2;
			}
			if((--sc->val3)<=0 || (sc->val2<=0) || skill_num == AL_HOLYLIGHT)
				status_change_end(bl, SC_KYRIE, -1);
		}
		/* �C���f���A */
		if(sc_data[SC_ENDURE].timer != -1 && damage > 0 && flag&BF_WEAPON && src->type != BL_PC){
			if((--sc_data[SC_ENDURE].val2)<=0)
				status_change_end(bl, SC_ENDURE, -1);
		}
		/* �I�[�g�K�[�h */
		if(sc_data[SC_AUTOGUARD].timer != -1 && damage > 0 && flag&BF_WEAPON) {
			if(atn_rand()%100 < sc_data[SC_AUTOGUARD].val2) {
				int delay;
				damage = 0;
				clif_skill_nodamage(bl,bl,CR_AUTOGUARD,sc_data[SC_AUTOGUARD].val1,1);
				if (sc_data[SC_AUTOGUARD].val1 <= 5)
					delay = 300;
				else if (sc_data[SC_AUTOGUARD].val1 > 5 && sc_data[SC_AUTOGUARD].val1 <= 9)
					delay = 200;
				else
					delay = 100;
				if(tsd){
					tsd->ud.canmove_tick = tick + delay;
					if(sc_data[SC_SHRINK].timer != -1 && atn_rand()%100<5*sc_data[SC_AUTOGUARD].val1)
					{
						skill_blown(bl,src,2);
					}

				}else if(tmd)
					tmd->ud.canmove_tick = tick + delay;
			}
		}
		/* �p���C���O */
		if(sc_data[SC_PARRYING].timer != -1 && damage > 0 && flag&BF_WEAPON) {
			if(atn_rand()%100 < sc_data[SC_PARRYING].val2) {
				int dir = map_calc_dir(bl,src->x,src->y);
				struct unit_data *ud = unit_bl2ud(bl);
				damage = 0;
				clif_skill_nodamage(bl,bl,LK_PARRYING,sc_data[SC_PARRYING].val1,1);
				clif_changedir(bl,dir,dir);
				if(ud)
					ud->attackabletime = tick + 500;	// �l�K��
			}
		}
		// ���W�F�N�g�\�[�h
		if(sc_data[SC_REJECTSWORD].timer!=-1 && damage > 0 && flag&BF_WEAPON && atn_rand()%100 < 15*sc_data[SC_REJECTSWORD].val1) {
			short weapon = -1;
			if(src->type == BL_PC)
				weapon = ((struct map_session_data *)src)->status.weapon;
			if(weapon == WT_DAGGER || weapon == WT_1HSWORD || weapon == WT_2HSWORD || src->type == BL_MOB) {
				damage = damage*50/100;
				battle_damage(bl,src,damage,0);
				clif_skill_nodamage(bl,bl,ST_REJECTSWORD,sc_data[SC_REJECTSWORD].val1,1);
				if((--sc_data[SC_REJECTSWORD].val2)<=0)
					status_change_end(bl, SC_REJECTSWORD, -1);
			}
		}
		if(sc_data[SC_SPIDERWEB].timer!=-1 && damage > 0) {	// [Celest]
			if( (flag&BF_SKILL && skill_get_pl(skill_num)==3) ||
			  (!(flag&BF_SKILL) && status_get_attack_element(src)==3) ) {
				damage<<=1;
				status_change_end(bl, SC_SPIDERWEB, -1);
			}
		}
	}

	if(damage > 0) { // GvG PK
		struct guild_castle *gc = NULL;
		int noflag = 0;

		if(class == 1288) {	// 1288:�G���y���E��
			if(flag&BF_SKILL && skill_num!=PA_PRESSURE && skill_num!=HW_GRAVITATION)	// �v���b�V���[
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
		if(map[bl->m].flag.gvg && skill_num!=PA_PRESSURE && skill_num!=HW_GRAVITATION){
			if(bl->type == BL_MOB) {
				if(gc == NULL && !noflag)	// �G���y���E���̍��Ŋ��Ɍ�������NULL�Ȃ�ēx�������Ȃ�
					gc = guild_mapname2gc(map[bl->m].name);
				if(gc) {
					// defense������΃_���[�W������炵���H
					damage -= damage*(gc->defense/100)*(battle_config.castle_defense_rate/100);
				}
			}
			if(flag&BF_WEAPON) {
				if(flag&BF_SHORT)
					damage=damage*battle_config.gvg_short_damage_rate/100;
				if(flag&BF_LONG)
					damage=damage*battle_config.gvg_long_damage_rate/100;
			}
			if(flag&BF_MAGIC)
				damage = damage*battle_config.gvg_magic_damage_rate/100;
			if(flag&BF_MISC)
				damage=damage*battle_config.gvg_misc_damage_rate/100;
			if(damage < 1) damage = (!battle_config.skill_min_damage && flag&BF_MAGIC && src->type==BL_PC)? 0: 1;
		}
		// PK
		if(map[bl->m].flag.pk && bl->type == BL_PC && skill_num!=PA_PRESSURE && skill_num!=HW_GRAVITATION){
			if(flag&BF_WEAPON) {
				if(flag&BF_SHORT)
					damage=damage*battle_config.pk_short_damage_rate/100;
				if(flag&BF_LONG)
					damage=damage*battle_config.pk_long_damage_rate/100;
			}
			if(flag&BF_MAGIC)
				damage = damage*battle_config.pk_magic_damage_rate/100;
			if(flag&BF_MISC)
				damage=damage*battle_config.pk_misc_damage_rate/100;
			if(damage < 1) damage = (!battle_config.skill_min_damage && flag&BF_MAGIC && src->type==BL_PC)? 0: 1;
		}
	}

	if((battle_config.skill_min_damage || flag&BF_MISC) && damage > 0) {
		if(div_==255) {
			if(damage<3)
				damage = 3;
		}
		else {
			if(damage<div_)
				damage = div_;
		}
	}

	if( tmd!=NULL && tmd->hp>0 && damage > 0 )	// �����Ȃǂ�MOB�X�L������
	{
		int mtg = tmd->target_id;
		if (battle_config.mob_changetarget_byskill != 0 || mtg == 0)
		{
			if(src->type == BL_PC || src->type == BL_HOM)
				tmd->target_id = src->id;
		}
		mobskill_event(tmd,flag);
		tmd->target_id = mtg;	// �^�[�Q�b�g��߂�
	}

	//PC�̔����I�[�g�X�y��
	if(tsd && tsd->bl.type == BL_PC && src && src!=bl && !unit_isdead(src) && tsd->status.hp > 0 && damage > 0)
	{
		long asflag = EAS_REVENGE;

		if(skill_num==AM_DEMONSTRATION)
			flag=(flag&~BF_WEAPONMASK)|BF_MISC;

		if(flag&BF_WEAPON) {
			if(flag&BF_SKILL){
				if(battle_config.weapon_attack_autospell)
					asflag += EAS_NORMAL;
				else
					asflag += EAS_SKILL;
			}else
				asflag += EAS_NORMAL;
			if(flag&BF_SHORT)
				asflag += EAS_SHORT;
			if(flag&BF_LONG)
				asflag += EAS_LONG;
		}
		if(flag&BF_MAGIC){
			if(battle_config.magic_attack_autospell)
				asflag += EAS_SHORT|EAS_LONG;
			else
				asflag += EAS_MAGIC;
		}
		if(flag&BF_MISC){
			if(battle_config.misc_attack_autospell)
				asflag += EAS_SHORT|EAS_LONG;
			else
				asflag += EAS_MISC;
		}

		skill_bonus_autospell(&tsd->bl,src,asflag,tick,0);
	}

	//PC�̔���
	if(tsd && tsd->bl.type == BL_PC && src && src!=bl && !unit_isdead(src) && tsd->status.hp > 0 && damage > 0 && flag&BF_WEAPON)
	{
		//������Ԉُ�
		if(tsd->addreveff_flag){
			int i;
			int rate;
			int luk;
			int sc_def_card=100;
			int sc_def_mdef,sc_def_vit,sc_def_int,sc_def_luk;
			//int race = status_get_race(target), ele = status_get_element(target);
			const int sc2[]={
				MG_STONECURSE,MG_FROSTDIVER,NPC_STUNATTACK,
				NPC_SLEEPATTACK,TF_POISON,NPC_CURSEATTACK,
				NPC_SILENCEATTACK,0,NPC_BLINDATTACK,LK_HEADCRUSH
			};
			//�Ώۂ̑ϐ�
			luk = status_get_luk(src);
			sc_def_mdef=50 - (3 + status_get_mdef(src) + luk/3);
			sc_def_vit=50 - (3 + status_get_vit(src) + luk/3);
			sc_def_int=50 - (3 + status_get_int(src) + luk/3);
			sc_def_luk=50 - (3 + luk);

/*			if(target->bl.type==BL_MOB){
				if(sc_def_mdef<50)
					sc_def_mdef=50;
				if(sc_def_vit<50)
					sc_def_vit=50;
				if(sc_def_int<50)
					sc_def_int=50;
				if(sc_def_luk<50)
					sc_def_luk=50;
			}
*/
			if(sc_def_mdef<0)
				sc_def_mdef=0;
			if(sc_def_vit<0)
				sc_def_vit=0;
			if(sc_def_int<0)
				sc_def_int=0;

			for(i=SC_STONE;i<=SC_BLEED;i++){
				//�Ώۂɏ�Ԉُ�
				if(i==SC_STONE || i==SC_FREEZE)
					sc_def_card=sc_def_mdef;
				else if(i==SC_STAN || i==SC_POISON || i==SC_SILENCE || i==SC_BLEED)
					sc_def_card=sc_def_vit;
				else if(i==SC_SLEEP || i==SC_CONFUSION || i==SC_BLIND)
					sc_def_card=sc_def_int;
				else if(i==SC_CURSE)
					sc_def_card=sc_def_luk;

				if(battle_config.reveff_plus_addeff)
					rate = (tsd->addreveff[i-SC_STONE] + tsd->addeff[i-SC_STONE] + tsd->arrow_addeff[i-SC_STONE])*sc_def_card/100;
				else
					rate = (tsd->addreveff[i-SC_STONE])*sc_def_card/100;

				if(src->type == BL_PC || src->type == BL_MOB || src->type == BL_HOM)
				{
					if(atn_rand()%10000 < rate ){
						if(battle_config.battle_log)
							printf("PC %d skill_addreveff: card�ɂ��ُ픭�� %d %d\n",tsd->bl.id,i,tsd->addreveff[i-SC_STONE]);
						status_change_start(src,i,7,0,0,0,(i==SC_CONFUSION)? 10000+7000:skill_get_time2(sc2[i-SC_STONE],7),0);
					}
				}
			}
		}
	}

	return damage;
}

/*==========================================
 * HP/SP�z���̌v�Z
 *------------------------------------------
 */
int battle_calc_drain(int damage, int rate, int per, int val)
{
	int diff = 0;

	if (damage <= 0 || rate <= 0)
		return 0;

	if (per && atn_rand()%100 < rate) {
		diff = (damage * per) / 100;
		if (diff == 0)
			diff = (per > 0)? 1: -1;
	}

	if (val && atn_rand()%100 < rate) {
		diff += val;
	}
	return diff;
}

/*==========================================
 * �C���_���[�W
 *------------------------------------------
 */
int battle_addmastery(struct map_session_data *sd,struct block_list *target,int dmg,int type)
{
	int damage = 0, race, skill, weapon;

	nullpo_retr(0, sd);
	nullpo_retr(0, target);

	race = status_get_race(target);

	// �f�[�����x�C�� vs �s�� or ���� (���l�͊܂߂Ȃ��H)
	// DB�C���O: SkillLv * 3
	// DB�C����: floor( ( 3 + 0.05 * BaseLv ) * SkillLv )
	if((skill = pc_checkskill(sd,AL_DEMONBANE)) > 0 && (battle_check_undead(race,status_get_elem_type(target)) || race==6) ) {
		//damage += (skill * 3);
		damage += (int)(floor( ( 3 + 0.05 * sd->status.base_level ) * skill )); // sd�̓��e�͕ۏ؂���Ă���
	}

	// �r�[�X�g�x�C��(+4 �` +40) vs ���� or ����
	if((skill = pc_checkskill(sd,HT_BEASTBANE)) > 0 && (race==2 || race==4) )
	{
		damage += (skill * 4);

		if(sd->sc_data && sd->sc_data[SC_HUNTER].timer!=-1)
			damage += status_get_str(&sd->bl);
	}

	if(type == 0)
		weapon = sd->weapontype1;
	else
		weapon = sd->weapontype2;
	switch(weapon)
	{
		case WT_DAGGER:
		case WT_1HSWORD:
		{
			// ���C��(+4 �` +40) �Ў茕 �Z���܂�
			if((skill = pc_checkskill(sd,SM_SWORD)) > 0) {
				damage += (skill * 4);
			}
			break;
		}
		case WT_2HSWORD:
		{
			// ���茕�C��(+4 �` +40) ���茕
			if((skill = pc_checkskill(sd,SM_TWOHAND)) > 0) {
				damage += (skill * 4);
			}
			break;
		}
		case WT_1HSPEAR:
		{
			// ���C��(+4 �` +40,+5 �` +50) ��
			if((skill = pc_checkskill(sd,KN_SPEARMASTERY)) > 0) {
				if(!pc_isriding(sd))
					damage += (skill * 4);	// �y�R�ɏ���ĂȂ�
				else
					damage += (skill * 5);	// �y�R�ɏ���Ă�
			}
			break;
		}
		case WT_2HSPEAR:
		{
			// ���C��(+4 �` +40,+5 �` +50) ��
			if((skill = pc_checkskill(sd,KN_SPEARMASTERY)) > 0) {
				if(!pc_isriding(sd))
					damage += (skill * 4);	// �y�R�ɏ���ĂȂ�
				else
					damage += (skill * 5);	// �y�R�ɏ���Ă�
			}
			break;
		}
		case WT_1HAXE:
		{
			if((skill = pc_checkskill(sd,AM_AXEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case WT_2HAXE:
		{
			if((skill = pc_checkskill(sd,AM_AXEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case WT_MACE:
		{
			// ���C�X�C��(+3 �` +30) ���C�X
			if((skill = pc_checkskill(sd,PR_MACEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case WT_2HMACE:
		case WT_STAFF:
		case WT_BOW:
			break;
		case WT_FIST:
		{
			// �^�C���M(+10 �` +100) �f��
			if((skill = pc_checkskill(sd,TK_RUN)) > 0) {
				damage += (skill * 10);
			}
			// fall through
		}
		case WT_KNUCKLE:
		{
			// �S��(+3 �` +30) �f��,�i�b�N��
			if((skill = pc_checkskill(sd,MO_IRONHAND)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case WT_MUSICAL:
		{
			// �y��̗��K(+3 �` +30) �y��
			if((skill = pc_checkskill(sd,BA_MUSICALLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case WT_WHIP:
		{
			// �_���X�̗��K(+3 �` +30) ��
			if((skill = pc_checkskill(sd,DC_DANCINGLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case WT_BOOK:
		{
			// �A�h�o���X�h�u�b�N(+3 �` +30) {
			if((skill = pc_checkskill(sd,SA_ADVANCEDBOOK)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
		case WT_KATAR:
		{
			// �J�^�[���C��(+3 �` +30) �J�^�[��
			if((skill = pc_checkskill(sd,AS_KATAR)) > 0) {
				//�\�j�b�N�u���[���͕ʏ����i1���ɕt��1/8�K��)
				damage += (skill * 3);
			}
			break;
		}
		case WT_HANDGUN:
		case WT_RIFLE:
		case WT_SHOTGUN:
		case WT_GATLING:
		case WT_GRENADE:
			break;
		case WT_HUUMA:
		{
			//�򓁏C��
			if((skill = pc_checkskill(sd,NJ_TOBIDOUGU)) > 0) {
				damage += (skill * 3);
			}
			break;
		}
	}
	return dmg+damage;
}

/*==========================================
 * ��{����_���[�W�v�Z
 *------------------------------------------
 */
static int battle_calc_base_damage(struct block_list *src,struct block_list *target,int skill_num,int type,int lh)
{
	int damage=0;
	int atkmin, atkmax;
	struct map_session_data *sd   = NULL;
	struct status_change *sc_data = NULL;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	sc_data = status_get_sc_data(src);
	if(src->type == BL_PC)
		sd = (struct map_session_data *)src;

	if(sd) {
		int watk   = (lh == 0)? status_get_atk(src): status_get_atk_(src);
		int dex    = status_get_dex(src);
		int t_size = status_get_size(target);
		int idx    = (lh == 0)? sd->equip_index[9]: sd->equip_index[8];

		if(skill_num == HW_MAGICCRASHER) {		// �}�W�b�N�N���b�V���[��MATK�ŉ���
			damage = status_get_matk1(src);
		} else {
			damage = status_get_baseatk(src);
		}

		atkmin = dex;	// �Œ�ATK��DEX�ŏ������H

		if(idx >= 0 && sd->inventory_data[idx])
			atkmin = atkmin * (80 + sd->inventory_data[idx]->wlv*20)/100;
		if(sd->state.arrow_atk)						// ���킪�|��̏ꍇ
			atkmin = watk * ((atkmin < watk)? atkmin: watk)/100;	// �|�p�Œ�ATK�v�Z

		/* �T�C�Y�C�� */
		if(skill_num == MO_EXTREMITYFIST) {
			// ���C��
			atkmax = watk;
		} else if(pc_isriding(sd) && (sd->status.weapon == WT_1HSPEAR || sd->status.weapon == WT_2HSPEAR) && t_size==1) {
			// �y�R�R�悵�Ă��āA���Œ��^���U�������ꍇ�̓T�C�Y�C����100�ɂ���
			atkmax = watk;
		} else {
			int rate = (lh == 0)? sd->atkmods[t_size]: sd->atkmods_[t_size];
			atkmax = (watk   * rate) / 100;
			atkmin = (atkmin * rate) / 100;
		}
		if(sc_data && sc_data[SC_WEAPONPERFECTION].timer != -1) {
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
		if(target->type == BL_PC || target->type == BL_MOB || target->type== BL_HOM)
		{
			int atk_rate = 0;
			int str = status_get_str(src);
			int luk = status_get_luk(src);
			int tclass = status_get_class(target);

			if(sc_data && sc_data[SC_MIRACLE].timer!=-1)	// ���z�ƌ��Ɛ��̊��
			{
				// �S�Ă̓G����
				atk_rate = (sd->status.base_level + dex + luk + str)/(12-3*pc_checkskill(sd,SG_STAR_ANGER));
			}else{
				if(tclass == sd->hate_mob[0] && pc_checkskill(sd,SG_SUN_ANGER)>0)	//���z�̓{��
					atk_rate = (sd->status.base_level + dex + luk)/(12-3*pc_checkskill(sd,SG_SUN_ANGER));
				else if(tclass == sd->hate_mob[1] && pc_checkskill(sd,SG_MOON_ANGER)>0)	//���̓{��
					atk_rate = (sd->status.base_level + dex + luk)/(12-3*pc_checkskill(sd,SG_MOON_ANGER));
				else if(tclass == sd->hate_mob[2] && pc_checkskill(sd,SG_STAR_ANGER)>0)	//���̓{��
					atk_rate = (sd->status.base_level + dex + luk + str)/(12-3*pc_checkskill(sd,SG_STAR_ANGER));
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
		if(skill_num == HW_MAGICCRASHER) {		// �}�W�b�N�N���b�V���[��MATK�ŉ���
			atkmin = status_get_matk1(src);
			atkmax = status_get_matk2(src);
		} else {
			atkmin = status_get_atk(src);
			atkmax = status_get_atk2(src);
		}
		if(atkmin > atkmax)
			atkmin = atkmax;
	}

	if(sc_data && sc_data[SC_MAXIMIZEPOWER].timer!=-1 ){
		// �}�L�V�}�C�Y�p���[
		atkmin = atkmax;
	}

	if(type == 0x0a) {
		/* �N���e�B�J���U�� */
		damage += atkmax;
		if(sd && (sd->atk_rate != 100 || sd->weapon_atk_rate != 0)) {
			damage = (damage * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon]))/100;

			// �N���e�B�J�����_���[�W����
			damage += damage *sd->critical_damage/100;
		}
		if(sd && sd->state.arrow_atk)
			damage += sd->arrow_atk;
	} else {
		/* �ʏ�U���E�X�L���U�� */
		if(atkmax > atkmin)
			damage += atkmin + atn_rand() % (atkmax-atkmin + 1);
		else
			damage += atkmin;
		if(sd && (sd->atk_rate != 100 || sd->weapon_atk_rate != 0)) {
			damage  = (damage * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon]))/100;
		}
	}

	return damage;
}

// ���蔻�肪����Ƃ�����damage2���v�Z����
#define ATK_FIX( a,b ) { wd.damage = wd.damage*(a)/(b); if(calc_flag.lh) wd.damage2 = wd.damage2*(a)/(b); }
#define ATK_ADD( a ) { wd.damage += (a); if(calc_flag.lh) wd.damage2 += (a); }

/*==========================================
 * ����_���[�W�v�Z
 *------------------------------------------
 */
struct Damage battle_calc_weapon_attack(struct block_list *src,struct block_list *target,int skill_num,int skill_lv,int wflag)
{
	struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct map_session_data *src_sd = NULL, *target_sd = NULL;
	struct mob_data         *src_md = NULL, *target_md = NULL;
	struct pet_data         *src_pd = NULL;
	struct homun_data       *src_hd = NULL, *target_hd = NULL;
	struct status_change    *sc_data = NULL, *t_sc_data = NULL;
	int s_ele, s_ele_, s_str;
	int t_vit, t_race, t_ele, t_enemy, t_size, t_mode, t_group, t_class;
	int t_flee, t_def1, t_def2;
	int damage_ot=0, damage_ot2=0;
	int tk_power_damage=0, tk_power_damage2=0;	//TK_POWER�p
	int cardfix, skill;
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

	//memset(&wd,0,sizeof(wd));
	memset(&calc_flag, 0, sizeof(calc_flag));

	// return�O�̏���������̂ŏ��o�͕��̂ݕύX
	if(src == NULL || target == NULL) {
		nullpo_info(NLP_MARK);
		return wd;
	}

	src_sd = BL_DOWNCAST( BL_PC,  src );
	src_md = BL_DOWNCAST( BL_MOB, src );
	src_pd = BL_DOWNCAST( BL_PET, src );
	src_hd = BL_DOWNCAST( BL_HOM, src );
	target_sd = BL_DOWNCAST( BL_PC,  target );
	target_md = BL_DOWNCAST( BL_MOB, target );
	target_hd = BL_DOWNCAST( BL_HOM, target );

	// �A�^�b�J�[
	s_ele    = status_get_attack_element(src);	// ����
	s_ele_   = status_get_attack_element2(src);	// ���葮��
	s_str    = status_get_str(src);			// STR
	sc_data  = status_get_sc_data(src);		// �X�e�[�^�X�ُ�

	// �^�[�Q�b�g
	t_vit     = status_get_vit(target);
	t_race    = status_get_race(target);		// �Ώۂ̎푰
	t_ele     = status_get_elem_type(target);	// �Ώۂ̑���
	t_enemy   = status_get_enemy_type(target);	// �Ώۂ̓G�^�C�v
	t_size    = status_get_size(target);		// �Ώۂ̃T�C�Y
	t_mode    = status_get_mode(target);		// �Ώۂ�Mode
	t_sc_data = status_get_sc_data(target);		// �Ώۂ̃X�e�[�^�X�ُ�
	t_group   = status_get_group(target);
	t_class   = status_get_class(target);
	t_flee    = status_get_flee(target);
	t_def1    = status_get_def(target);
	t_def2    = status_get_def2(target);

	if(src_sd && skill_num != CR_GRANDCROSS && skill_num != NPC_DARKGRANDCROSS)
		src_sd->state.attack_type = BF_WEAPON;	// �U���^�C�v�͕���U��

	/* �P�D�I�[�g�J�E���^�[���� */
	if(skill_lv >= 0 && (skill_num == 0 || (target_sd && battle_config.pc_auto_counter_type&2) ||
		(target_md && battle_config.monster_auto_counter_type&2))
	) {
		if(
			skill_num != CR_GRANDCROSS && skill_num != NPC_DARKGRANDCROSS &&
			t_sc_data && t_sc_data[SC_AUTOCOUNTER].timer != -1
		) { //�O�����h�N���X�łȂ��A�Ώۂ��I�[�g�J�E���^�[��Ԃ̏ꍇ
			int dir   = map_calc_dir(src,target->x,target->y);
			int t_dir = status_get_dir(target);
			int dist  = unit_distance2(src,target);
			if(dist <= 0 || map_check_dir(dir,t_dir) ) {
				//�ΏۂƂ̋�����0�ȉ��A�܂��͑Ώۂ̐��ʁH
				t_sc_data[SC_AUTOCOUNTER].val3 = 0;
				t_sc_data[SC_AUTOCOUNTER].val4 = 1;
				if(sc_data && sc_data[SC_AUTOCOUNTER].timer == -1) {
					int range = status_get_range(target);
					// �������I�[�g�J�E���^�[���
					if( target_sd &&
						(target_sd->status.weapon != WT_BOW && !(target_sd->status.weapon >= WT_HANDGUN && target_sd->status.weapon <= WT_GRENADE))
						&& dist <= range+1)
						//�Ώۂ�PC�ŕ��킪�|��łȂ��˒���
						t_sc_data[SC_AUTOCOUNTER].val3 = src->id;
					if( target_md && range <= 3 && dist <= range+1)
						//�܂��͑Ώۂ�Mob�Ŏ˒���3�ȉ��Ŏ˒���
						t_sc_data[SC_AUTOCOUNTER].val3 = src->id;
				}
				return wd; //�_���[�W�\���̂�Ԃ��ďI��
			}
			else calc_flag.autocounter = 1;
		}
	}

	/* �Q�D�������␳ */
	// ��������(!=������)
	if ((src_sd && battle_config.pc_attack_attr_none) ||
	    (src_md && battle_config.mob_attack_attr_none) ||
	    (src_pd && battle_config.pet_attack_attr_none) ||
	     src_hd)
	{
		if (s_ele == 0)
			s_ele  = -1;
		if (s_ele_ == 0)
			s_ele_ = -1;
	}

	calc_flag.hitrate = status_get_hit(src) - t_flee + 80;	// �������v�Z
	// ����HIT�␳
	if(skill_num == 0 && t_sc_data && t_sc_data[SC_FOGWALL].timer!=-1)
		calc_flag.hitrate -= 50;

	/* �R�Dwd�\���̂̏����ݒ� */
	wd.type      = 0;
	wd.div_      = skill_get_num(skill_num,skill_lv);
	wd.blewcount = skill_get_blewcount(skill_num,skill_lv);
	wd.flag      = BF_SHORT | BF_WEAPON | BF_NORMAL;	// �U���̎�ނ̐ݒ�

	if(wd.div_ <= 0 || wd.div_ >= 251)			// 251�`254 = �e�R���R��A255 = �O�i���Ƃ��ė\���
		wd.div_ = 1;

	if(src_sd) {
		if(src_sd->status.weapon == WT_BOW || (src_sd->status.weapon >= WT_HANDGUN && src_sd->status.weapon <= WT_GRENADE)) {	//���킪�|��̏ꍇ
			wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;	// �������U���t���O��L��
			if(src_sd->arrow_ele > 0)	// ������Ȃ瑮�����̑����ɕύX
				s_ele = src_sd->arrow_ele;
			src_sd->state.arrow_atk = 1;	// �L����
		} else {
			src_sd->state.arrow_atk = 0;	// ������
		}
	} else if(src_md || src_pd) {
		if(status_get_range(src) > 3)
			wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;
	}

	/* �S�D�E��E���蔻�� */
	calc_flag.rh = 1;	// ��{�͉E��̂�
	if(src_sd) {
		if(src_sd->weapontype1 == WT_FIST && src_sd->weapontype2 > WT_FIST) {	// ����̂ݕ��푕��
			calc_flag.rh = 0;
			calc_flag.lh = 1;
		} else if(skill_num == 0 && src_sd->status.weapon > WT_HUUMA) {		// �ʏ�U���œ񓁗��i�X�L���U���ɍ���͏��Ȃ��j
			calc_flag.lh = 1;
		}
	}

	/* �T�D�A������ */
	if(src_sd && skill_num == 0 && skill_lv >= 0) {
		do {
			// �_�u���A�^�b�N
			if((skill = pc_checkskill(src_sd,TF_DOUBLE)) > 0 && src_sd->weapontype1 == WT_DAGGER && atn_rand()%100 < skill*5) {
				calc_flag.da = 1;
				calc_flag.hitrate = calc_flag.hitrate*(100+skill)/100;
				break;
			}
			// �`�F�C���A�N�V����
			if((skill = pc_checkskill(src_sd,GS_CHAINACTION)) > 0 && src_sd->weapontype1 == WT_HANDGUN && atn_rand()%100 < skill*5) {
				calc_flag.da = 1;
				break;
			}
			// �O�i��
			if((skill = pc_checkskill(src_sd,MO_TRIPLEATTACK)) > 0 && src_sd->status.weapon <= WT_HUUMA && !src_sd->state.arrow_atk)
			{
				int triple_rate = 0;
				if(sc_data && sc_data[SC_TRIPLEATTACK_RATE_UP].timer!=-1) {
					triple_rate = (30 - skill)*(150+50*sc_data[SC_TRIPLEATTACK_RATE_UP].val1)/100;
					status_change_end(src,SC_TRIPLEATTACK_RATE_UP,-1);
				} else {
					triple_rate = 30 - skill;
				}
				if(atn_rand()%100 < triple_rate) {
					calc_flag.da = 2;
					break;
				}
			}
			// �t�F�I���`���M
			if(sc_data && sc_data[SC_READYSTORM].timer!=-1 && pc_checkskill(src_sd,TK_STORMKICK) > 0 && atn_rand()%100 < 15) {
				calc_flag.da = 3;
				break;
			}
			// �l�����`���M
			if(sc_data && sc_data[SC_READYDOWN].timer!=-1 && pc_checkskill(src_sd,TK_DOWNKICK) > 0 && atn_rand()%100 < 15) {
				calc_flag.da = 4;
				break;
			}
			// �g�������`���M
			if(sc_data && sc_data[SC_READYTURN].timer!=-1 &&  pc_checkskill(src_sd,TK_TURNKICK) > 0 && atn_rand()%100 < 15) {
				calc_flag.da = 5;
				break;
			}
			// �A�v�`���I�����M
			if(sc_data && sc_data[SC_READYCOUNTER].timer!=-1 && pc_checkskill(src_sd,TK_COUNTER) > 0)
			{
				int counter_rate = 0;
				if(sc_data[SC_COUNTER_RATE_UP].timer!=-1 && (skill = pc_checkskill(src_sd,SG_FRIEND)) > 0) {
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
	if( calc_flag.da == 0 && (skill_num == 0 || skill_num == KN_AUTOCOUNTER || skill_num == SN_SHARPSHOOTING || skill_num == NJ_KIRIKAGE) &&
	    (!src_md || battle_config.enemy_critical) && skill_lv >= 0 )
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
			cri = cri*battle_config.enemy_critical_rate/100;
			if(cri < 1) cri = 1;
		}
		if(t_sc_data != NULL && t_sc_data[SC_SLEEP].timer!=-1 )
			cri <<= 1;		// �������̓N���e�B�J�����{��
		if(calc_flag.autocounter)
			cri = 1000;

		if(skill_num == KN_AUTOCOUNTER) {
			if(!(battle_config.pc_auto_counter_type&1))
				cri = 1000;
			else
				cri <<= 1;
		}
		if(skill_num == SN_SHARPSHOOTING)
			cri += 200;
		if(skill_num == NJ_KIRIKAGE)
			cri += (250+skill_lv*50);

		if(target_sd && target_sd->critical_def) {
			if(target_sd->critical_def > 100)
				cri = 0;
			else
				cri = cri * (100-target_sd->critical_def) / 100;
		}

		// �m������
		if(atn_rand() % 1000 < cri) {
			if(skill_num == SN_SHARPSHOOTING || skill_num == NJ_KIRIKAGE) {
				// DEF�����t���O
				calc_flag.idef = calc_flag.idef_ = 1;
			} else {
				wd.type = 0x0a;	// �N���e�B�J���U��
			}
		}
	}

	/* �V�D��{�_���[�W�̎Z�o */
	// �񓁗��̏ꍇ�͕ʁX�Ɍv�Z����
	wd.damage = battle_calc_base_damage(src, target, skill_num, wd.type, 0);
	if(calc_flag.lh)
		wd.damage2 = battle_calc_base_damage(src, target, skill_num, wd.type, 1);

	if(wd.type == 0) {		// �N���e�B�J���łȂ��Ƃ�
		if(src_sd && src_sd->state.arrow_atk) {
			if(src_sd->arrow_atk > 0)
				wd.damage += atn_rand()%(src_sd->arrow_atk+1);
			calc_flag.hitrate += src_sd->arrow_hit;
		}
	}

	/* �W�D�t�@�C�e�B���O�v�Z */
	if(src_sd && (skill = pc_checkskill(src_sd,TK_POWER)>0) && src_sd->status.party_id >0)
	{
		int member_num = party_check_same_map_member_count(src_sd);
		if(member_num > 0)
		{
			tk_power_damage = wd.damage*member_num * skill/50;
			if(calc_flag.lh)
				tk_power_damage2 = wd.damage2*member_num * skill/50;
		}
	}

	/* �X�D�I�[�o�[�g���X�g�n�̃X�L���{���v�Z�O�̍U���͊m�� */
	damage_ot += wd.damage;
	if(calc_flag.lh)
		damage_ot2 += wd.damage2;

	/* 10�D�N���e�B�J���ȊO�̍U���͔{�� */
	if(wd.type == 0) {
		// �X�L���C���P�i�U���͔{���n�j
		if(skill_num > 0) {
			int i;
			if( (i=skill_get_pl(skill_num))>0 && (!src_sd || !src_sd->arrow_ele ) )
				s_ele=s_ele_=i;

			wd.flag = (wd.flag&~BF_SKILLMASK)|BF_SKILL;
		}
		switch( skill_num ){
		case SM_BASH:		// �o�b�V��
			ATK_FIX( 100+30*skill_lv, 100 );
			calc_flag.hitrate = (calc_flag.hitrate*(100+5*skill_lv))/100;
			break;
		case SM_MAGNUM:		// �}�O�i���u���C�N
			if(!wflag) {	// ����
				ATK_FIX( 100+20*skill_lv, 100 );
			} else {	// �O��
				ATK_FIX( 100+10*skill_lv, 100 );
			}
			calc_flag.hitrate = calc_flag.hitrate*(10*skill_lv + 100)/100;
			break;
		case HVAN_EXPLOSION:
			wd.damage = status_get_hp(src)*(50+50*skill_lv)/100;
			if(calc_flag.lh)
				wd.damage2 = wd.damage;
			calc_flag.hitrate = 1000000;
			break;
		case MC_MAMMONITE:	// ���}�[�i�C�g
			ATK_FIX( 100+50*skill_lv, 100 );
			break;
		case AC_DOUBLE:	// �_�u���X�g���C�t�B���O
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				ATK_ADD( arr );
			}
			ATK_FIX( 100+20*skill_lv, 100 );
			calc_flag.dist = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case HT_POWER:	// �r�[�X�g�X�g���C�t�B���O
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				ATK_ADD( arr );
			}
			ATK_FIX( 100+160*(s_str/10), 100 );
			calc_flag.dist = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case AC_SHOWER:	// �A���[�V�����[
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				ATK_ADD( arr );
			}
			ATK_FIX( 75+5*skill_lv, 100 );
			calc_flag.dist = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			wd.blewcount=0;
			break;
		case AC_CHARGEARROW:	// �`���[�W�A���[
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				ATK_ADD( arr );
			}
			ATK_FIX( 150, 100 );
			calc_flag.dist = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case HT_PHANTASMIC:	// �t�@���^�X�~�b�N�A���[
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				ATK_ADD( arr );
			}
			ATK_FIX( 150, 100 );
			calc_flag.dist = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case KN_CHARGEATK:			//�`���[�W�A�^�b�N
			{
				//distance
				int dist = unit_distance2(src,target)-1;
				if(dist > 2)
					ATK_FIX( 100+100*(dist/3), 100 );
			}
			break;
		case AS_VENOMKNIFE:			//�x�i���i�C�t
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				ATK_ADD( arr );
				if(src_sd->arrow_ele > 0) //������Ȃ瑮�����̑����ɕύX
					s_ele = src_sd->arrow_ele;
			}
			calc_flag.dist = 1;
			calc_flag.nocardfix = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case SG_SUN_WARM://���z�̉�����
		case SG_MOON_WARM://���̉�����
		case SG_STAR_WARM://���̉�����
			if(src_sd) {
				if(src_sd->status.sp < 2)
				{
					status_change_end(src,SkillStatusChangeTable[skill_num],-1);
					break;
				}
				if(target_sd)
				{
					target_sd->status.sp -= 5;
					if(target_sd->status.sp<0)
						target_sd->status.sp = 0;
					clif_updatestatus(target_sd,SP_SP);
				}
				//�������̂�SP����
				src_sd->status.sp -= 2;
				clif_updatestatus(src_sd,SP_SP);
			} else if(target_sd)
			{
				target_sd->status.sp -= 5;
				clif_updatestatus(target_sd,SP_SP);
			}
			break;
		case KN_PIERCE:	// �s�A�[�X
			wd.div_ = t_size+1;
			ATK_FIX( (100+10*skill_lv) * wd.div_, 100 );
			calc_flag.hitrate = calc_flag.hitrate*(100+5*skill_lv)/100;
			break;
		case KN_SPEARSTAB:	// �X�s�A�X�^�u
			ATK_FIX( 100+15*skill_lv, 100 );
			wd.blewcount = 0;
			calc_flag.dist = 1;
			break;
		case KN_SPEARBOOMERANG:	// �X�s�A�u�[������
			ATK_FIX( 100+50*skill_lv, 100 );
			calc_flag.dist = 1;
			break;
		case KN_BRANDISHSPEAR: // �u�����f�B�b�V���X�s�A
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
				ATK_FIX( rate, 100 );
			}
			break;
		case KN_BOWLINGBASH:	// �{�E�����O�o�b�V��
			ATK_FIX( 100+40*skill_lv, 100 );
			wd.blewcount=0;
			break;
		case KN_AUTOCOUNTER:
			if(battle_config.pc_auto_counter_type&1)
				calc_flag.hitrate += 20;
			else
				calc_flag.hitrate = 1000000;
			wd.flag = (wd.flag&~BF_SKILLMASK)|BF_NORMAL;
			break;
		case AS_SONICBLOW:	// �\�j�b�N�u���E
			{
				int rate = 300+50*skill_lv;
				if(src_sd && pc_checkskill(src_sd,AS_SONICACCEL)>0)
				{
					rate = rate*110/100;
					calc_flag.hitrate = calc_flag.hitrate*150/100;
				}
				if(sc_data && sc_data[SC_ASSASIN].timer!=-1)
				{
					if(map[src->m].flag.gvg)
						rate = rate*125/100;
					else
						rate *= 2;
				}
				ATK_FIX( rate, 100 );
			}
			break;
		case AS_GRIMTOOTH:	// �O�����g�D�[�X
			ATK_FIX( 100+20*skill_lv, 100 );
			calc_flag.dist = 1;
			break;
		case TF_SPRINKLESAND:	// ���܂�
			ATK_FIX( 130, 100 );
			break;
		case MC_CARTREVOLUTION:	// �J�[�g���{�����[�V����
			if(src_sd && src_sd->cart_max_weight > 0 && src_sd->cart_weight > 0) {
				ATK_FIX( 150 + pc_checkskill(src_sd,BS_WEAPONRESEARCH) + src_sd->cart_weight*100/src_sd->cart_max_weight, 100 );
			} else {
				ATK_FIX( 150, 100 );
			}
			wd.blewcount=0;
			break;
		// �ȉ�MOB
		case NPC_COMBOATTACK:	// ���i�U��
			ATK_FIX( 50*wd.div_, 100 );
			s_ele  = 0;
			s_ele_ = 0;
			break;
		case NPC_RANDOMATTACK:	// �����_��ATK�U��
			ATK_FIX( 50+atn_rand()%150, 100 );
			s_ele  = 0;
			s_ele_ = 0;
			break;
		// �����U��
		case NPC_WATERATTACK:
		case NPC_GROUNDATTACK:
		case NPC_FIREATTACK:
		case NPC_WINDATTACK:
		case NPC_POISONATTACK:
		case NPC_HOLYATTACK:
		case NPC_DARKNESSATTACK:
		case NPC_TELEKINESISATTACK:
		case NPC_UNDEADATTACK:
			ATK_FIX( 25+75*skill_lv, 100 );
			break;
		case NPC_CRITICALSLASH:
		case NPC_GUIDEDATTACK:
			calc_flag.hitrate = 1000000;
			s_ele  = 0;
			s_ele_ = 0;
			break;
		case NPC_RANGEATTACK:
			calc_flag.dist = 1;
			s_ele  = 0;
			s_ele_ = 0;
			break;
		case NPC_PIERCINGATT:
			ATK_FIX( 75, 100 );
			wd.flag = (wd.flag&~BF_RANGEMASK)|BF_SHORT;
			s_ele  = 0;
			s_ele_ = 0;
			break;
		case RG_BACKSTAP:	// �o�b�N�X�^�u
			{
				int rate = 300+40*skill_lv;
				if(src_sd && src_sd->status.weapon == WT_BOW) {	// �|�Ȃ甼��
					rate /= 2;
				}
				ATK_FIX( rate, 100 );
			}
			calc_flag.hitrate = 1000000;
			break;
		case RG_RAID:	// �T�v���C�Y�A�^�b�N
			ATK_FIX( 100+40*skill_lv, 100 );
			break;
		case RG_INTIMIDATE:	// �C���e�B�~�f�C�g
			ATK_FIX( 100+30*skill_lv, 100 );
			break;
		case CR_SHIELDCHARGE:	// �V�[���h�`���[�W
			ATK_FIX( 100+20*skill_lv, 100 );
			wd.flag = (wd.flag&~BF_RANGEMASK)|BF_SHORT;
			s_ele = 0;
			break;
		case CR_SHIELDBOOMERANG:	// �V�[���h�u�[������
			{
				int rate = 100+30*skill_lv;
				if(sc_data && sc_data[SC_CRUSADER].timer!=-1)
				{
					rate *= 2;
					calc_flag.hitrate= 1000000;
				}
				ATK_FIX( rate, 100 );
				calc_flag.dist = 1;
				s_ele = 0;
			}
			break;
		case CR_HOLYCROSS:	// �z�[���[�N���X
		case NPC_DARKCROSS:	// �_�[�N�N���X
			ATK_FIX( 100+35*skill_lv, 100 );
			break;
		case CR_GRANDCROSS:
		case NPC_DARKGRANDCROSS:
			calc_flag.hitrate= 1000000;
			if (!battle_config.gx_cardfix)
				calc_flag.nocardfix = 1;
			break;
		case AM_DEMONSTRATION:	// �f�����X�g���[�V����
			calc_flag.hitrate= 1000000;
			ATK_FIX( 100+20*skill_lv, 100 );
			calc_flag.nocardfix = 1;
			break;
		case AM_ACIDTERROR:	// �A�V�b�h�e���[
			calc_flag.hitrate= 1000000;
			ATK_FIX( 100+40*skill_lv, 100 );
			s_ele  = 0;
			s_ele_ = 0;
			calc_flag.nocardfix = 1;
			calc_flag.dist = 1;
			break;
		case MO_FINGEROFFENSIVE:	//�w�e
			if(src_sd && battle_config.finger_offensive_type == 0) {
				wd.div_ = src_sd->spiritball_old;
				ATK_FIX( (100+50*skill_lv) * wd.div_, 100 );
			}
			else {
				wd.div_ = 1;
				ATK_FIX( 100+50*skill_lv, 100 );
			}
			calc_flag.dist = 1;
			break;
		case MO_INVESTIGATE:	// ����
			if(t_def1 < 1000000) {
				ATK_FIX( (100+75*skill_lv) * (t_def1 + t_def2), 100*50 );
			}
			calc_flag.hitrate = 1000000;
			s_ele  = 0;
			s_ele_ = 0;
			break;
		case MO_BALKYOUNG:
			ATK_FIX( 300, 100 );
			break;
		case MO_EXTREMITYFIST:	// ���C���e�P��
			if(src_sd) {
				ATK_FIX( 8+src_sd->status.sp/10, 1 );
				src_sd->status.sp = 0;
				clif_updatestatus(src_sd,SP_SP);
			} else {
				ATK_FIX( 8, 1 );
			}
			ATK_ADD( 250+150*skill_lv );
			calc_flag.hitrate = 1000000;
			s_ele  = 0;
			s_ele_ = 0;
			break;
		case MO_CHAINCOMBO:	// �A�ŏ�
			ATK_FIX( 150+50*skill_lv, 100 );
			break;
		case MO_COMBOFINISH:	// �җ���
			ATK_FIX( 240+60*skill_lv, 100 );
			//PT�ɂ͓����Ă���
			//�J�E���^�[�A�^�b�N�̊m���㏸
			if(src_sd && src_sd->status.party_id>0){
				struct party *pt = party_search(src_sd->status.party_id);
				if(pt!=NULL)
				{
					struct map_session_data* psrc_sd = NULL;
					for(i=0;i<MAX_PARTY;i++)
					{
						psrc_sd = pt->member[i].sd;
						if(!psrc_sd || src_sd == psrc_sd)
							continue;
						if(src_sd->bl.m == psrc_sd->bl.m && pc_checkskill(psrc_sd,TK_COUNTER)>0)
						{
							status_change_start(&psrc_sd->bl,SC_COUNTER_RATE_UP,1,0,0,0,battle_config.tk_counter_rate_up_keeptime,0);
						}
					}
				}
			}
			break;
		case TK_STORMKICK:	//�����R��
			ATK_FIX( 160+20*skill_lv, 100 );
			break;
		case TK_DOWNKICK:	//���i�R��
			ATK_FIX( 160+20*skill_lv, 100 );
			break;
		case TK_TURNKICK:	//��]�R��
			ATK_FIX( 190+30*skill_lv, 100 );
			break;
		case TK_COUNTER:	//�J�E���^�[�R��
			ATK_FIX( 190+30*skill_lv, 100 );
			calc_flag.hitrate = 1000000;
			//PT�ɂ͓����Ă���
			//�O�i���̊m���㏸
			if(src_sd && src_sd->status.party_id>0 && (skill = pc_checkskill(src_sd,SG_FRIEND))>0){
				struct party *pt = party_search(src_sd->status.party_id);
				if(pt!=NULL)
				{
					struct map_session_data* psrc_sd = NULL;
					for(i=0;i<MAX_PARTY;i++)
					{
						psrc_sd = pt->member[i].sd;
						if(!psrc_sd || src_sd==psrc_sd)
							continue;

						if(src_sd->bl.m == psrc_sd->bl.m && pc_checkskill(psrc_sd,MO_TRIPLEATTACK)>0)
						{
							status_change_start(&psrc_sd->bl,SC_TRIPLEATTACK_RATE_UP,skill,0,0,0,battle_config.tripleattack_rate_up_keeptime,0);
						}
					}
				}
			}
			break;

		case BA_MUSICALSTRIKE:	// �~���[�W�J���X�g���C�N
		case DC_THROWARROW:	    // ���
			ATK_FIX( 60+40*skill_lv, 100 );
			calc_flag.dist = 1;
			if (src_sd)
				s_ele = src_sd->arrow_ele;
			break;

		case CH_TIGERFIST:	// ���Ռ�
			ATK_FIX( 40+100*skill_lv, 100 );
			break;
		case CH_CHAINCRUSH:	// �A������
			ATK_FIX( 400+100*skill_lv, 100 );
			break;
		case CH_PALMSTRIKE:	// �ҌՍd�h�R
			ATK_FIX( 200+100*skill_lv, 100 );
			break;
		case LK_SPIRALPIERCE:			/* �X�p�C�����s�A�[�X */
			ATK_FIX( 80+40*skill_lv, 100 );
			calc_flag.dist = 1;
		case LK_HEADCRUSH:				/* �w�b�h�N���b�V�� */
			ATK_FIX( 100+40*skill_lv, 100 );
			break;
		case LK_JOINTBEAT:				/* �W���C���g�r�[�g */
			ATK_FIX( 50+10*skill_lv, 100 );
			break;
		case HW_MAGICCRASHER:				/* �}�W�b�N�N���b�V���[ */
			calc_flag.dist = 1;
			break;
		case ASC_METEORASSAULT:			/* ���e�I�A�T���g */
			ATK_FIX( 40+40*skill_lv, 100 );
			calc_flag.nocardfix = 1;
			break;
		case ASC_BREAKER:				/* �\�E���u���C�J�[ */
			ATK_FIX( skill_lv, 1 );
			calc_flag.dist = 1;
			calc_flag.nocardfix = 1;
			break;
		case SN_SHARPSHOOTING:			/* �V���[�v�V���[�e�B���O */
			ATK_FIX( 200+50*skill_lv, 100 );
			calc_flag.dist = 1;
			break;
		case CG_ARROWVULCAN:			/* �A���[�o���J�� */
			ATK_FIX( 200+100*skill_lv, 100 );
			calc_flag.dist = 1;
			if (src_sd)
				 s_ele = src_sd->arrow_ele;
			break;
		case AS_SPLASHER:		/* �x�i���X�v���b�V���[ */
			if(src_sd) {
				ATK_FIX( 500+50*skill_lv+20*pc_checkskill(src_sd,AS_POISONREACT), 100 );
			} else {
				ATK_FIX( 500+50*skill_lv, 100 );
			}
			calc_flag.nocardfix = 1;
			calc_flag.hitrate = 1000000;
			break;
		case AS_POISONREACT:		/* �|�C�Y�����A�N�g�i�U���Ŕ����j */
			wd.damage = wd.damage*(100+30*skill_lv)/100;
			//wd.damage2 = wd.damage2	//����ɂ͏��Ȃ�
			break;
		case TK_JUMPKICK: //��яR��
			if(sc_data && (sc_data[SC_RUN].timer != -1 || sc_data[SC_DODGE_DELAY].timer != -1))
			{
				ATK_FIX( 30 + (10+status_get_lv(src)/10)*skill_lv, 100 );
				if(sc_data[SC_DODGE_DELAY].timer!=-1)
					status_change_end(src,SC_DODGE_DELAY,-1);
			}else{
				ATK_FIX( 30+10*skill_lv, 100 );
			}
			if(src_sd && sc_data && sc_data[SC_RUN].timer != -1 && sc_data[SC_SPURT].timer != -1) {
				// �^�C���M���Ŋ��X�p�[�g��ԂȂ�З͂���ɃA�b�v
				// �v�Z���s���Ȃ̂œK��
				ATK_ADD( 10*pc_checkskill(src_sd,TK_RUN) );
			}
			status_change_end_by_jumpkick(target);
			break;
		case PA_SHIELDCHAIN:	/* �V�[���h�`�F�C�� */
			if(src_sd)
			{
				int s_dex = status_get_dex(src);
				int s_luk = status_get_luk(src);
				int idx = src_sd->equip_index[8];
				wd.damage = s_str+(s_str/10)*(s_str/10)+(s_dex/5)+(s_luk/5);
				if(idx >= 0 && src_sd->inventory_data[idx] && src_sd->inventory_data[idx]->type == 5)
					wd.damage += src_sd->status.inventory[idx].refine*4 + src_sd->inventory_data[idx]->weight/10;
			}else{
				wd.damage = wd.damage*(100+30*skill_lv)/100;
			}
			if(calc_flag.lh)
				wd.damage2 = wd.damage;
			calc_flag.hitrate = (calc_flag.hitrate*(100+5*skill_lv))/100;
			calc_flag.dist = 1;
			s_ele  = 0;
			s_ele_ = 0;
			break;
		case WS_CARTTERMINATION:	/* �J�[�g�^�[�~�l�[�V���� */
			{
				int rate = (skill_lv >= 16)? 1: 16-skill_lv;
				if(src_sd && src_sd->cart_max_weight > 0 && src_sd->cart_weight > 0) {
					double weight=(8000.*src_sd->cart_weight)/src_sd->cart_max_weight;
					wd.damage = (int)(wd.damage*(weight/rate/100));
					if(calc_flag.lh)
						wd.damage2 = (int)(wd.damage2*(weight/rate/100));
				}
				else if(!src_sd) {
					ATK_FIX( 80, rate );
				}
			}
			calc_flag.nocardfix = 1;
			break;
		case CR_ACIDDEMONSTRATION:	/* �A�V�b�h�f�����X�g���[�V���� */
			calc_flag.hitrate = 1000000;
			{
				int s_int = status_get_int(src);
				double val = s_int*s_int/10./(s_int+t_vit);
				val = val*t_vit*skill_lv*7.;
				if(target->type != BL_MOB)
					val /= 2;
				wd.damage = (int)val;
				if(calc_flag.lh)
					wd.damage2 = wd.damage;
			}
			s_ele  = 0;
			s_ele_ = 0;
			calc_flag.nocardfix = 1;
			calc_flag.dist = 1;
			break;
		case ITM_TOMAHAWK:		/* �g�}�z�[�N���� */
			calc_flag.dist = 1;
			break;
		case GS_FLING:			/* �t���C���O */
			calc_flag.dist = 1;
			break;
		case GS_TRIPLEACTION:	/* �g���v���A�N�V���� */
			ATK_FIX( 300, 100 );
			calc_flag.dist = 1;
			break;
		case GS_BULLSEYE:		/* �u���Y�A�C */
			ATK_FIX( 500, 100 );
			calc_flag.dist = 1;
			break;
		case GS_MAGICALBULLET:	/* �}�W�J���o���b�g */
			{
				int matk1=status_get_matk1(src),matk2=status_get_matk2(src);
				if(matk1>matk2)
					wd.damage += matk2+atn_rand()%(matk1-matk2+1);
				else
					wd.damage += matk2;
				calc_flag.dist = 1;
			}
			break;
		case GS_TRACKING:		/* �g���b�L���O */
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				ATK_ADD( arr );
			}
			ATK_FIX( 120*skill_lv, 100 );
			calc_flag.dist = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_DISARM:			/* �f�B�X�A�[�� */
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				ATK_ADD( arr );
			}
			calc_flag.dist = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_PIERCINGSHOT:	/* �s�A�[�V���O�V���b�g */
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				ATK_ADD( arr );
			}
			ATK_FIX( 100+20*skill_lv, 100 );
			calc_flag.dist = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_RAPIDSHOWER:	/* ���s�b�h�V�����[ */
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				ATK_ADD( arr );
			}
			ATK_FIX( 500+50*skill_lv, 100 );
			calc_flag.dist = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_DESPERADO:		/* �f�X�y���[�h */
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				ATK_ADD( arr );
			}
			ATK_FIX( 50+50*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_DUST:			/* �_�X�g */
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				ATK_ADD( arr );
			}
			ATK_FIX( 100+50*skill_lv, 100 );
			calc_flag.dist = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_FULLBUSTER:		/* �t���o�X�^�[ */
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				ATK_ADD( arr );
			}
			ATK_FIX( 300+100*skill_lv, 100 );
			calc_flag.dist = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_SPREADATTACK:	/* �X�v���b�h�A�^�b�N */
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				ATK_ADD( arr );
			}
			ATK_FIX( 80+20*skill_lv, 100 );
			calc_flag.dist = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_GROUNDDRIFT:	/* �O���E���h�h���t�g */
			wd.damage += skill_lv*50;
			break;
		case NJ_SYURIKEN:		/* �藠������ */
			if(src_sd){
				if(!src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					ATK_ADD( arr );
				}
				if(src_sd->arrow_ele > 0) //������Ȃ瑮�����̑����ɕύX
					s_ele = src_sd->arrow_ele;
				src_sd->state.arrow_atk = 1;
			}
			ATK_ADD( skill_lv*4 );
			calc_flag.dist = 1;
			break;
		case NJ_KUNAI:			/* �ꖳ���� */
			if(src_sd){
				if(!src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					ATK_ADD( arr );
				}
				if(src_sd->arrow_ele > 0) //������Ȃ瑮�����̑����ɕύX
					s_ele = src_sd->arrow_ele;
				src_sd->state.arrow_atk = 1;
			}
			ATK_FIX( 300, 100 );
			calc_flag.dist = 1;
			break;
		case NJ_HUUMA:			/* �����藠������ */
			ATK_FIX( 150+150*skill_lv, 100 );
			calc_flag.dist = 1;
			break;
		case NJ_ZENYNAGE:	/* �K���� */
			if(src_sd){
				wd.damage = src_sd->zenynage_damage;
				src_sd->zenynage_damage = 0;//�������烊�Z�b�g
			}else{
				wd.damage = skill_get_zeny(NJ_ZENYNAGE,skill_lv)/2;
				if(wd.damage > 0)
					wd.damage += atn_rand()%wd.damage;
				else
					wd.damage = 500*skill_lv + atn_rand()%(500*skill_lv);
			}
			if(target->type==BL_PC || t_mode & 0x20){
				wd.damage /= 2;
			}
			if(calc_flag.lh)
				wd.damage2 = wd.damage;
			calc_flag.dist = 1;
			s_ele = 0;
			s_ele_ = 0;
			calc_flag.nocardfix = 1;
			break;
		case NJ_TATAMIGAESHI:	/* ���Ԃ� */
			ATK_FIX( 100+10*skill_lv, 100 );
			calc_flag.dist = 1;
			break;
		case NJ_KASUMIKIRI:		/* ���a�� */
			ATK_FIX( 100+10*skill_lv, 100 );
			break;
		case NJ_KIRIKAGE:		/* �e�a�� */
			ATK_FIX( skill_lv, 1 );
			break;
		case NJ_ISSEN://��M
			{
				int hp = status_get_hp(src);
				wd.damage = (s_str*40)+(skill_lv*(hp-1)*8)/100;
				if(calc_flag.lh)
					wd.damage2 = wd.damage;
				unit_heal(src,-hp+1,0);
				s_ele  = 0;
				s_ele_ = 0;
				if(sc_data && sc_data[SC_NEN].timer!=-1)
					status_change_end(src,SC_NEN,-1);
			}
			break;
		case HFLI_MOON:
			ATK_FIX( 110+110*skill_lv, 100 );
			break;
		case HFLI_SBR44:
			if(src_hd){
				wd.damage = src_hd->intimate*skill_lv;
				if(calc_flag.lh)
					wd.damage2 = wd.damage;
				src_hd->intimate = 200;
				if(battle_config.homun_skill_intimate_type)
					src_hd->status.intimate = 200;
				clif_send_homdata(src_hd->msd,0x100,src_hd->intimate/100);
			}
			break;
		}

		//�������狗���ɂ�锻��A���̎���wd.flag�͈ȉ�4�̂����ꂩ
		//�@�ʏ�U���@�FBF_SHORT|BF_WEAPON|BF_NORMAL or BF_LONG|BF_WEAPON|BF_NORMAL (����������)
		//�@�X�L���U���FBF_SHORT|BF_WEAPON|BF_SKILL  or BF_LONG|BF_WEAPON|BF_SKILL  (����������)

		if(calc_flag.dist){				//�����ɂ���ă����W���ω�����X�L����
			if(battle_config.calc_dist_flag&1) {	//�����̎��v�Z���邩�H &1�Ōv�Z
				int target_dist = unit_distance2(src,target)-1;	//�������擾
				if(target_dist >= battle_config.allow_sw_dist){				//SW�Ŗh���鋗����葽��������������̍U��
					if(battle_config.sw_def_type & 1 && src->type==BL_PC)		//�l�Ԃ���̂𔻒肷�邩�@&1�ł���
						wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;			//�������ɐݒ�
					if(battle_config.sw_def_type & 2 && src->type==BL_MOB)		//�����X�^�[����̂𔻒肷�邩�@&2�ł���
						wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;			//�������ɐݒ�
				}
			} else {	//�{���������̃X�L���Ŏg�p�҂Ƌ��t���O���S�Ĉ�v���Ȃ����牓�����U����
				//wd.flag=(wd.flag&~BF_RANGEMASK)|BF_LONG;//�������ɐݒ�
			}
		}
		//�����ɂ��t���O�����ɂЂ����������ꍇ��BF_LONG�ŏ����I��

		//�t�@�C�e�B���O�̒ǉ��_���[�W
		wd.damage += tk_power_damage;
		if(calc_flag.lh)
			wd.damage2 += tk_power_damage2;

		if(calc_flag.da == 2) {	// �O�i�����������Ă���
			if(src_sd)
				wd.damage = wd.damage * (100 + 20 * pc_checkskill(src_sd, MO_TRIPLEATTACK)) / 100;
		}

		// �h�䖳�����肨��ѐ����ʃ_���[�W�v�Z
		switch (skill_num) {
		case KN_AUTOCOUNTER:
		case CR_GRANDCROSS:
		case MO_INVESTIGATE:
		case MO_EXTREMITYFIST:
		case AM_ACIDTERROR:
		case CR_ACIDDEMONSTRATION:
		case NJ_ZENYNAGE:
			break;
		case WS_CARTTERMINATION:
		case PA_SHIELDCHAIN:
			if( skill_num==WS_CARTTERMINATION && !battle_config.def_ratio_atk_to_carttermination )
				break;
			if( skill_num==PA_SHIELDCHAIN && !battle_config.def_ratio_atk_to_shieldchain )
				break;
		default:
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
						if(!calc_flag.idef && battle_config.left_cardfix_to_right){
							wd.damage = (wd.damage * (t_def1 + t_def2))/100;
							calc_flag.idef = 1;
						}
					}
				}
			}
			break;
		}

		// �Ώۂ̖h��͂ɂ��_���[�W�̌���
		switch (skill_num) {
		case KN_AUTOCOUNTER:
		case CR_GRANDCROSS:
		case MO_INVESTIGATE:
		case MO_EXTREMITYFIST:
		case CR_ACIDDEMONSTRATION:
		case NJ_ZENYNAGE:
		case NPC_CRITICALSLASH:
			break;
		default:
			// ���z�ƌ��Ɛ��̗Z�� DEF����
			if(sc_data && sc_data[SC_FUSION].timer != -1)
				calc_flag.idef = 1;

			// DEF�����t���O���Ȃ��Ƃ�
			if( ((calc_flag.rh && !calc_flag.idef) || (calc_flag.lh && !calc_flag.idef_)) && t_def1 < 1000000 )
			{
				int t_def, vitbonusmax;
				int target_count=1;

				if(target->type!=BL_HOM) {
					target_count = unit_counttargeted(target,battle_config.vit_penaly_count_lv);
				}
				if(battle_config.vit_penaly_type > 0 && (!t_sc_data || t_sc_data[SC_STEELBODY].timer==-1)) {
					if(target_count >= battle_config.vit_penaly_count) {
						if(battle_config.vit_penaly_type == 1) {
							t_def1 = (t_def1 * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
							t_def2 = (t_def2 * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
							t_vit = (t_vit * (100 - (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num))/100;
						}
						else if(battle_config.vit_penaly_type == 2) {
							t_def1 -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
							t_def2 -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
							t_vit -= (target_count - (battle_config.vit_penaly_count - 1))*battle_config.vit_penaly_num;
						}
						if(t_def1 < 0) t_def1 = 0;
						if(t_def2 < 1) t_def2 = 1;
						if(t_vit < 1) t_vit = 1;
					}
				}
				t_def = t_def2*8/10;
				vitbonusmax = (t_vit/20)*(t_vit/20)-1;

				if(calc_flag.rh && !calc_flag.idef) {
					if(battle_config.player_defense_type) {
						wd.damage = wd.damage - (t_def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
						damage_ot = damage_ot - (t_def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
					} else {
						wd.damage = wd.damage * (100 - t_def1) /100 - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
						damage_ot = damage_ot * (100 - t_def1) /100;
					}
				}
				if(calc_flag.lh && !calc_flag.idef_) {
					if(battle_config.player_defense_type) {
						wd.damage2 = wd.damage2 - (t_def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
						damage_ot2 = damage_ot2 - (t_def1 * battle_config.player_defense_type) - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
					} else {
						wd.damage2 = wd.damage2 * (100 - t_def1) /100 - t_def - ((vitbonusmax < 1)?0: atn_rand()%(vitbonusmax+1) );
						damage_ot2 = damage_ot2 * (100 - t_def1) /100;
					}
				}
			}
			break;
		}
	}

	/* 11�D��Ԉُ풆�̃_���[�W�ǉ��ŃN���e�B�J���ɂ��L���ȃX�L�� */
	if (sc_data) {
		//�I�[�o�[�g���X�g
		if(sc_data[SC_OVERTHRUST].timer!=-1){	// �I�[�o�[�g���X�g
			wd.damage += damage_ot*(5*sc_data[SC_OVERTHRUST].val1)/100;
			if(calc_flag.lh)
				wd.damage2 += damage_ot2*(5*sc_data[SC_OVERTHRUST].val1)/100;
		}
		//�I�[�o�[�g���X�g�}�b�N�X
		if(sc_data[SC_OVERTHRUSTMAX].timer!=-1){	// �I�[�o�[�g���X�g�}�b�N�X
			wd.damage += damage_ot*(20*sc_data[SC_OVERTHRUSTMAX].val1)/100;
			if(calc_flag.lh)
				wd.damage2 += damage_ot2*(20*sc_data[SC_OVERTHRUSTMAX].val1)/100;
		}
		// �g�D���[�T�C�g
		if(sc_data[SC_TRUESIGHT].timer!=-1){
			ATK_FIX( 100+2*sc_data[SC_TRUESIGHT].val1, 100 );
		}
		// �o�[�T�[�N
		if(sc_data[SC_BERSERK].timer!=-1){
			ATK_FIX( 200, 100 );
		}
		// �G���`�����g�f�b�h���[�|�C�Y��
		if (sc_data[SC_EDP].timer != -1 && !calc_flag.nocardfix) {
			// �E��݂̂Ɍ��ʂ��̂�B�J�[�h���ʖ����̃X�L���ɂ͏��Ȃ�
			if(map[src->m].flag.pk && target->type==BL_PC){
				wd.damage += wd.damage * (150 + sc_data[SC_EDP].val1 * 50) * battle_config.pk_edp_down_rate / 10000;
			}else if(map[src->m].flag.gvg){
				wd.damage += wd.damage * (150 + sc_data[SC_EDP].val1 * 50) * battle_config.gvg_edp_down_rate / 10000;
			}else if(map[src->m].flag.pvp){
				wd.damage += wd.damage * (150 + sc_data[SC_EDP].val1 * 50) * battle_config.pvp_edp_down_rate / 10000;
			}else{
				wd.damage += wd.damage * (150 + sc_data[SC_EDP].val1 * 50) / 100;
			}
			// calc_flag.nocardfix = 1;
		}
		// �T�N���t�@�C�X
		if (sc_data[SC_SACRIFICE].timer != -1 && !skill_num && t_class != 1288) {
			int dmg = status_get_max_hp(src) * 9 / 100;
			battle_heal(NULL, src, -dmg, 0, 0);
			wd.damage = dmg * (90 + sc_data[SC_SACRIFICE].val1 * 10) / 100;
			wd.damage2 = 0;
			calc_flag.hitrate = 1000000;
			s_ele  = 0;
			s_ele_ = 0;
			clif_misceffect2(src,366);
			if ((--sc_data[SC_SACRIFICE].val2) <= 0)
				status_change_end(src, SC_SACRIFICE,-1);
		}
	}

	/* 12�D���B�_���[�W�̒ǉ� */
	if( src_sd ) {
		if(skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST && skill_num != PA_SHIELDCHAIN && skill_num != CR_ACIDDEMONSTRATION && skill_num != NJ_ZENYNAGE) {
			wd.damage += status_get_atk2(src);
			if(calc_flag.lh)
				wd.damage2 += status_get_atk_2(src);
		}
		switch (skill_num) {
		case CR_SHIELDBOOMERANG:
			if(src_sd->equip_index[8] >= 0) {
				int idx = src_sd->equip_index[8];
				if(src_sd->inventory_data[idx] && src_sd->inventory_data[idx]->type == 5) {
					wd.damage += src_sd->inventory_data[idx]->weight/10;
					wd.damage += src_sd->status.inventory[idx].refine * status_getrefinebonus(0,1);
				}
			}
			break;
		case LK_SPIRALPIERCE:		/* �X�p�C�����s�A�[�X */
			if(src_sd->equip_index[9] >= 0) {	//{((STR/10)^2 �{ ����d�ʁ~�X�L���{���~0.8) �~ �T�C�Y�␳ �{ ���B}�~�J�[�h�{���~�����{���~5�̖͗l
				int idx = src_sd->equip_index[9];
				if(src_sd->inventory_data[idx] && src_sd->inventory_data[idx]->type == 4) {
					wd.damage = ( ( (s_str/10)*(s_str/10) + src_sd->inventory_data[idx]->weight * (skill_lv * 4 + 8 ) / 100 )
								* (5 - t_size) / 4 + status_get_atk2(src) ) * 5;
				}
			}
			break;
		case PA_SHIELDCHAIN:		/* �V�[���h�`�F�C��*/
			if(src_sd->equip_index[8] >= 0) {
				int idx = src_sd->equip_index[8];
				if(src_sd->inventory_data[idx] && src_sd->inventory_data[idx]->type == 5) {
					int refinedamage = 2*(src_sd->status.inventory[idx].refine-4) + src_sd->status.inventory[idx].refine * src_sd->status.inventory[idx].refine;
					wd.damage *= (100+30*skill_lv)/100;
					if(refinedamage>0)
						wd.damage += atn_rand() % refinedamage;
					wd.damage = (wd.damage+100) * 5;
				}
			}
			break;
		case NJ_SYURIKEN:		/* �藠������ */
		case NJ_KUNAI:			/* �N�i�C���� */
			wd.damage += pc_checkskill(src_sd,NJ_TOBIDOUGU) * 3;
			break;
		}
	}

	// 0�����������ꍇ1�ɕ␳
	if(wd.damage<1)  wd.damage  = 1;
	if(wd.damage2<1) wd.damage2 = 1;

	/* 13�D�X�L���C���Q�i�C���n�j*/
	// �C���_���[�W(�E��̂�) �\�j�b�N�u���[���͕ʏ����i1���ɕt��1/8�K��)
	if( src_sd && skill_num != MO_INVESTIGATE &&
	              skill_num != MO_EXTREMITYFIST &&
	              skill_num != CR_GRANDCROSS &&
	              skill_num != NPC_DARKGRANDCROSS &&
	              skill_num != LK_SPIRALPIERCE &&
	              skill_num != CR_ACIDDEMONSTRATION &&
	              skill_num != NJ_ZENYNAGE ) {			//�C���_���[�W����
		wd.damage = battle_addmastery(src_sd,target,wd.damage,0);
		if(calc_flag.lh)
			wd.damage2 = battle_addmastery(src_sd,target,wd.damage2,1);
	}
	if(sc_data && sc_data[SC_AURABLADE].timer!=-1) {	//�I�[���u���[�h������
		ATK_ADD( sc_data[SC_AURABLADE].val1*20 );
	}
	if(sc_data && sc_data[SC_GATLINGFEVER].timer!=-1) {	//�K�g�����O�t�B�[�o�[
		ATK_ADD( 20+sc_data[SC_GATLINGFEVER].val1*10 );
	}
	if(src_sd && src_sd->perfect_hit > 0) {
		if(atn_rand()%100 < src_sd->perfect_hit)
			calc_flag.hitrate = 1000000;
	}

	/* 14�D����C�� */
	calc_flag.hitrate = (calc_flag.hitrate < battle_config.min_hitrate)? battle_config.min_hitrate: calc_flag.hitrate;
	if(	calc_flag.hitrate < 1000000 && // �K���U��
		(t_sc_data != NULL && (t_sc_data[SC_SLEEP].timer!=-1 ||	// �����͕K��
		t_sc_data[SC_STAN].timer!=-1 ||		// �X�^���͕K��
		t_sc_data[SC_FREEZE].timer!=-1 || (t_sc_data[SC_STONE].timer!=-1 && t_sc_data[SC_STONE].val2==0) ) ) )	// �����͕K��
		calc_flag.hitrate = 1000000;

	if(wd.type == 0 && atn_rand()%100 >= calc_flag.hitrate) {
		wd.damage = wd.damage2 = 0;
		wd.dmg_lv = ATK_FLEE;
	}else if(wd.type == 0 && t_sc_data && t_sc_data[SC_KAUPE].timer !=-1 && atn_rand()%100 < (t_sc_data[SC_KAUPE].val2))//�J�E�v
	{
		wd.damage = wd.damage2 = 0;
		wd.dmg_lv = ATK_FLEE;
		//�J�E�v�I������
		status_change_end(target,SC_KAUPE,-1);
	}else if(wd.type == 0 && t_sc_data && t_sc_data[SC_UTSUSEMI].timer !=-1)//���
	{
		wd.damage = wd.damage2 = 0;
		wd.dmg_lv = ATK_FLEE;
		clif_misceffect2(target,463);
		if((--t_sc_data[SC_UTSUSEMI].val3)==0)
			status_change_end(target,SC_UTSUSEMI,-1);
		if(t_sc_data[SC_ANKLE].timer==-1) {
			int dir = 0, head_dir = 0;
			int count = skill_get_blewcount(NJ_UTSUSEMI,t_sc_data[SC_UTSUSEMI].val1);
			if(target_sd) {
				dir = target_sd->dir;
				head_dir = target_sd->head_dir;
			}
			unit_stop_walking(target,1);
			skill_blown(src,target,count|SAB_REVERSEBLOW|SAB_NODAMAGE|SAB_NOPATHSTOP);
			if(target_sd)
				pc_setdir(target_sd, dir, head_dir);
			if(t_sc_data && t_sc_data[SC_CLOSECONFINE].timer != -1)
				status_change_end(target,SC_CLOSECONFINE,-1);
		}
	}else if(wd.type == 0 && t_sc_data && t_sc_data[SC_BUNSINJYUTSU].timer !=-1)//���g
	{
		wd.damage = wd.damage2 = 0;
		wd.dmg_lv = ATK_FLEE;
		if((--t_sc_data[SC_BUNSINJYUTSU].val3)==0)
			status_change_end(target,SC_BUNSINJYUTSU,-1);
	}else if(target_sd && t_sc_data && (wd.flag&BF_LONG || t_sc_data[SC_SPURT].timer!=-1) && t_sc_data[SC_DODGE].timer!=-1 && atn_rand()%100 < 20)//���@
	{
		int slv = pc_checkskill(target_sd,TK_DODGE);
		wd.damage = wd.damage2 = 0;
		wd.dmg_lv = ATK_FLEE;
		clif_skill_nodamage(&target_sd->bl,&target_sd->bl,TK_DODGE,slv,1);
		status_change_start(&target_sd->bl,SC_DODGE_DELAY,slv,src->id,0,0,skill_get_time(TK_DODGE,slv),0);
	}
	else{
		wd.dmg_lv = ATK_DEF;
	}

	/* 15�D�X�L���C���R�i�K���_���[�W�j*/
	if( src_sd && (skill = pc_checkskill(src_sd,BS_WEAPONRESEARCH)) > 0) {
		ATK_ADD( skill*2 );
	}
	if( src_sd && (skill = pc_checkskill(src_sd,TK_RUN)) > 0) {	// �^�C���M�p�b�V�u�ŏR��̈З͉��Z
		if( (skill_num == TK_DOWNKICK || skill_num == TK_STORMKICK || skill_num == TK_TURNKICK || skill_num == TK_COUNTER) &&
		    src_sd->weapontype1 == WT_FIST && src_sd->weapontype2 == WT_FIST ) {
			ATK_ADD( skill*10 );
		}
	}

	/* 16�D�J�[�h�ɂ��_���[�W�ǉ����� */
	if( src_sd && !calc_flag.nocardfix && calc_flag.rh ) {
		cardfix = 100;
		if(!src_sd->state.arrow_atk) {	// �|��ȊO
			if(!battle_config.left_cardfix_to_right) {	// ����J�[�h�␳�ݒ薳��
				cardfix=cardfix*(100+src_sd->addrace[t_race])/100;	// �푰�ɂ��_���[�W�C��
				cardfix=cardfix*(100+src_sd->addele[t_ele])/100;	// �����ɂ��_���[�W�C��
				cardfix=cardfix*(100+src_sd->addenemy[t_enemy])/100;	// �G�^�C�v�ɂ��_���[�W�C��
				cardfix=cardfix*(100+src_sd->addsize[t_size])/100;	// �T�C�Y�ɂ��_���[�W�C��
				cardfix=cardfix*(100+src_sd->addgroup[t_group])/100;	// �O���[�v�ɂ��_���[�W�C��
			}
			else {
				cardfix=cardfix*(100+src_sd->addrace[t_race]+src_sd->addrace_[t_race])/100;	// �푰�ɂ��_���[�W�C��(����ɂ��ǉ�����)
				cardfix=cardfix*(100+src_sd->addele[t_ele]+src_sd->addele_[t_ele])/100;		// �����ɂ��_���[�W�C��(����ɂ��ǉ�����)
				cardfix=cardfix*(100+src_sd->addenemy[t_enemy]+src_sd->addenemy_[t_enemy])/100;	// �G�^�C�v�ɂ��_���[�W�C��(����ɂ��ǉ�����)
				cardfix=cardfix*(100+src_sd->addsize[t_size]+src_sd->addsize_[t_size])/100;	// �T�C�Y�ɂ��_���[�W�C��(����ɂ��ǉ�����)
				cardfix=cardfix*(100+src_sd->addgroup[t_group]+src_sd->addgroup_[t_group])/100;	// �O���[�v�ɂ��_���[�W�C��(����ɂ��ǉ�����)
			}
		}
		else { //�|��
			cardfix=cardfix*(100+src_sd->addrace[t_race]+src_sd->arrow_addrace[t_race])/100;	// �푰�ɂ��_���[�W�C��(�|��ɂ��ǉ�����)
			cardfix=cardfix*(100+src_sd->addele[t_ele]+src_sd->arrow_addele[t_ele])/100;		// �����ɂ��_���[�W�C��(�|��ɂ��ǉ�����)
			cardfix=cardfix*(100+src_sd->addenemy[t_enemy]+src_sd->arrow_addenemy[t_enemy])/100;	// �G�^�C�v�ɂ��_���[�W�C��(�|��ɂ��ǉ�����)
			cardfix=cardfix*(100+src_sd->addsize[t_size]+src_sd->arrow_addsize[t_size])/100;	// �T�C�Y�ɂ��_���[�W�C��(�|��ɂ��ǉ�����)
			cardfix=cardfix*(100+src_sd->addgroup[t_group]+src_sd->arrow_addgroup[t_group])/100;	// �O���[�v�ɂ��_���[�W�C��(�|��ɂ��ǉ�����)
		}
		if(t_mode & 0x20) {	//�{�X
			if(!src_sd->state.arrow_atk) {	// �|��U���ȊO�Ȃ�
				if(!battle_config.left_cardfix_to_right)	// ����J�[�h�␳�ݒ薳��
					cardfix=cardfix*(100+src_sd->addrace[10])/100;				// �{�X�����X�^�[�ɒǉ��_���[�W
				else	// ����J�[�h�␳�ݒ肠��
					cardfix=cardfix*(100+src_sd->addrace[10]+src_sd->addrace_[10])/100;	//�{�X�����X�^�[�ɒǉ��_���[�W(����ɂ��ǉ�����)
			}
			else	// �|��U��
				cardfix=cardfix*(100+src_sd->addrace[10]+src_sd->arrow_addrace[10])/100;	//�{�X�����X�^�[�ɒǉ��_���[�W(�|��ɂ��ǉ�����)
		}
		else {	// �{�X����Ȃ�
			if(!src_sd->state.arrow_atk) {	// �|��U���ȊO
				if(!battle_config.left_cardfix_to_right)	// ����J�[�h�␳�ݒ薳��
					cardfix=cardfix*(100+src_sd->addrace[11])/100;				// �{�X�ȊO�����X�^�[�ɒǉ��_���[�W
				else	// ����J�[�h�␳�ݒ肠��
					cardfix=cardfix*(100+src_sd->addrace[11]+src_sd->addrace_[11])/100;	// �{�X�ȊO�����X�^�[�ɒǉ��_���[�W(����ɂ��ǉ�����)
			}
			else
				cardfix=cardfix*(100+src_sd->addrace[11]+src_sd->arrow_addrace[11])/100;	//�{�X�ȊO�����X�^�[�ɒǉ��_���[�W(�|��ɂ��ǉ�����)
		}
		// �J�[�h���ʂɂ����背���W�U���̃_���[�W����
		if(wd.damage > 0){
			if(wd.flag&BF_SHORT){
				cardfix = cardfix * (100+src_sd->short_weapon_damege_rate) / 100;
			}
			if(wd.flag&BF_LONG){
				cardfix = cardfix * (100+src_sd->long_weapon_damege_rate) / 100;
			}
		}
		// �J�[�h���ʂɂ�����X�L���̃_���[�W�����i����X�L���j
		if(src_sd->skill_dmgup.count > 0 && (skill_num > 0) && (wd.damage > 0)){
			for( i=0 ; i<src_sd->skill_dmgup.count ; i++ ){
				if( skill_num == src_sd->skill_dmgup.id[i] ){
					cardfix = cardfix*(100+src_sd->skill_dmgup.rate[i])/100;
					break;
				}
			}
		}
		// ����Class�p�␳����(�����̓��L���{���S���p�H)
		for(i=0;i<src_sd->add_damage_class_count;i++) {
			if(src_sd->add_damage_classid[i] == t_class) {
				cardfix=cardfix*(100+src_sd->add_damage_classrate[i])/100;
				break;
			}
		}
		wd.damage = wd.damage*cardfix/100;	// �J�[�h�␳�ɂ��_���[�W����
	}

	/* 17�D�J�[�h�ɂ�鍶��_���[�W�ǉ����� */
	if( src_sd && !calc_flag.nocardfix && calc_flag.lh ) {
		cardfix = 100;
		if(!battle_config.left_cardfix_to_right) {	// ����J�[�h�␳�ݒ薳��
			cardfix=cardfix*(100+src_sd->addrace_[t_race])/100;	// �푰�ɂ��_���[�W�C������
			cardfix=cardfix*(100+src_sd->addele_[t_ele])/100;	// �����ɂ��_���[�W�C������
			cardfix=cardfix*(100+src_sd->addenemy_[t_enemy])/100;	// �G�^�C�v�ɂ��_���[�W�C������
			cardfix=cardfix*(100+src_sd->addsize_[t_size])/100;	// �T�C�Y�ɂ��_���[�W�C������
			cardfix=cardfix*(100+src_sd->addgroup_[t_group])/100;	// �O���[�v�ɂ��_���[�W�C������
			if(t_mode & 0x20) //�{�X
				cardfix=cardfix*(100+src_sd->addrace_[10])/100;	// �{�X�����X�^�[�ɒǉ��_���[�W����
			else
				cardfix=cardfix*(100+src_sd->addrace_[11])/100;	// �{�X�ȊO�����X�^�[�ɒǉ��_���[�W����
		}
		// ����Class�p�␳��������(�����̓��L���{���S���p�H)
		for(i=0;i<src_sd->add_damage_class_count_;i++) {
			if(src_sd->add_damage_classid_[i] == t_class) {
				cardfix=cardfix*(100+src_sd->add_damage_classrate_[i])/100;
				break;
			}
		}
		wd.damage2 = wd.damage2*cardfix/100;	// �J�[�h�␳�ɂ�鍶��_���[�W����
	}

	/* 18�D�J�[�h�ɂ��_���[�W�������� */
	if(target_sd){	// �Ώۂ�PC�̏ꍇ
		int s_race  = status_get_race(src);
		int s_enemy = status_get_enemy_type(src);
		int s_size  = status_get_size(src);
		int s_group = status_get_group(src);
		cardfix = 100;
		cardfix=cardfix*(100-target_sd->subrace[s_race])/100;		// �푰�ɂ��_���[�W�ϐ�
		if (s_ele >= 0)
			cardfix=cardfix*(100-target_sd->subele[s_ele])/100;	// �����ɂ��_���[�W�ϐ�
		if (s_ele == -1)
			cardfix=cardfix*(100-target_sd->subele[0])/100;		// ���������̑ϐ��͖�����
		cardfix=cardfix*(100-target_sd->subenemy[s_enemy])/100;		// �G�^�C�v�ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-target_sd->subsize[s_size])/100;		// �T�C�Y�ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-target_sd->subgroup[s_group])/100;		// �O���[�v�ɂ��_���[�W�ϐ�

		if(status_get_mode(src) & 0x20)
			cardfix=cardfix*(100-target_sd->subrace[10])/100;	// �{�X����̍U���̓_���[�W����
		else
			cardfix=cardfix*(100-target_sd->subrace[11])/100;	// �{�X�ȊO����̍U���̓_���[�W����

		// ����Class�p�␳��������(�����̓��L���{���S���p�H)
		for(i=0;i<target_sd->add_def_class_count;i++) {
			if(target_sd->add_def_classid[i] == status_get_class(src)) {
				cardfix=cardfix*(100-target_sd->add_def_classrate[i])/100;
				break;
			}
		}
		if(wd.flag&BF_LONG)
			cardfix=cardfix*(100-target_sd->long_attack_def_rate)/100;	// �������U���̓_���[�W����(�z����C�Ƃ�)
		if(wd.flag&BF_SHORT)
			cardfix=cardfix*(100-target_sd->near_attack_def_rate)/100; //�ߋ����U���̓_���[�W����(�Y�������H)
		ATK_FIX( cardfix, 100 );	// �J�[�h�␳�ɂ��_���[�W����
	}

	/* 19�D�A�C�e���{�[�i�X�̃t���O���� */
	// ��Ԉُ�̃����W�t���O
	//   addeff_range_flag  0:�w�薳�� 1:�ߋ��� 2:������ 3,4:���ꂼ��̃����W�ŏ�Ԉُ�𔭓������Ȃ�
	//   flag������A�U���^�C�v��flag����v���Ȃ��Ƃ��́Aflag+2����
	if(src_sd && wd.flag&BF_WEAPON){
		for(i=SC_STONE;i<=SC_BLEED;i++){
			if( (src_sd->addeff_range_flag[i-SC_STONE]==1 && wd.flag&BF_LONG ) ||
			    (src_sd->addeff_range_flag[i-SC_STONE]==2 && wd.flag&BF_SHORT) ){
				src_sd->addeff_range_flag[i-SC_STONE]+=2;
			}
		}
	}

	/* 20�D�ΏۂɃX�e�[�^�X�ُ킪����ꍇ�̃_���[�W���Z���� */
	if(t_sc_data) {
		cardfix=100;
		if(t_sc_data[SC_DEFENDER].timer != -1 && wd.flag&BF_LONG)	// �f�B�t�F���_�[��Ԃŉ������U��
			cardfix=cardfix*(100-t_sc_data[SC_DEFENDER].val2)/100;
		if(t_sc_data[SC_ADJUSTMENT].timer != -1 && wd.flag&BF_LONG)	// �A�W���X�g�����g��Ԃŉ������U��
			cardfix-=20;
		if(cardfix != 100) {
			ATK_FIX( cardfix, 100 );	// �X�e�[�^�X�ُ�␳�ɂ��_���[�W����
		}
	}

	if(wd.damage  < 0) wd.damage  = 0;
	if(wd.damage2 < 0) wd.damage2 = 0;

	/* 21�D�����̓K�p */
	wd.damage = battle_attr_fix(wd.damage, s_ele, status_get_element(target));
	if(calc_flag.lh)
		wd.damage2 = battle_attr_fix(wd.damage2, s_ele_, status_get_element(target));

	/* 22�D�X�L���␳�P */
	// �}�O�i���u���C�N���
	if(sc_data && sc_data[SC_MAGNUM].timer != -1) {
		int bonus_damage = battle_attr_fix(wd.damage, 3, status_get_element(target)) * 20/100;	// �Α����U���_���[�W��20%��ǉ�
		if(bonus_damage > 0) {
			ATK_ADD( bonus_damage );
		}
	}
	// �\�E���u���C�J�[
	if(skill_num == ASC_BREAKER) {
		// int�ɂ��ǉ��_���[�W
		wd.damage += status_get_int(src) * skill_lv * 5;
		if (target_sd) {
			if (s_ele >= 0)
				wd.damage = wd.damage * (100-target_sd->subele[s_ele])/100;
			if (s_ele == -1)
				wd.damage = wd.damage * (100-target_sd->subele[0])/100;
		}
		// �����_���_���[�W
		wd.damage += 500 + (atn_rand() % 500);
		if(t_def1 < 1000000) {
			int vitbonusmax = (t_vit/20)*(t_vit/20)-1;
			wd.damage -= (t_def1 + t_def2 + ((vitbonusmax < 1)? 0: atn_rand()%(vitbonusmax+1)) + status_get_mdef(target) + status_get_mdef2(target))/2;
		}
	}

	/* 23�D���̂�����A�C���̓K�p */
	if(src_sd) {
		wd.damage += src_sd->star;
		if(calc_flag.lh)
			wd.damage2 += src_sd->star_;
		ATK_ADD( src_sd->spiritball*3 );
		ATK_ADD( src_sd->coin*3 );
		ATK_ADD( src_sd->bonus_damage );
		wd.damage += src_sd->ranker_weapon_bonus;
		if(calc_flag.lh)
			wd.damage2 += src_sd->ranker_weapon_bonus_;
	}
	/* 24�D�K���Œ�_���[�W */
	if(src_sd && src_sd->special_state.fix_damage){
		wd.damage = src_sd->fix_damage;
		if(calc_flag.lh)
			wd.damage2 = src_sd->fix_damage;
	}
	if(skill_num == PA_PRESSURE){	// �v���b�V���[�K��
		wd.damage = 500+300*skill_lv;
		if(calc_flag.lh)
			wd.damage2 = wd.damage;
	}

	/* 25�D����_���[�W�̕␳ */
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

	/* 26�D�X�L���␳�Q */
	if(src_sd && src_sd->status.weapon == WT_KATAR) {
		// �A�h�o���X�h�J�^�[������
		if((skill = pc_checkskill(src_sd,ASC_KATAR)) > 0) {
			wd.damage += wd.damage*(10+(skill * 2))/100;
		}
		// �J�^�[���ǌ��_���[�W
		skill = pc_checkskill(src_sd,TF_DOUBLE);
		wd.damage2 = wd.damage * (1 + (skill * 2))/100;
		if(wd.damage > 0 && wd.damage2 < 1)
			wd.damage2 = 1;
	}
	if(skill_num == TF_POISON){
		wd.damage = battle_attr_fix(wd.damage + 15*skill_lv, s_ele, status_get_element(target) );
	}
	if(skill_num == MC_CARTREVOLUTION){
		wd.damage = battle_attr_fix(wd.damage, 0, status_get_element(target) );
	}

	/* 27�D���S����̔��� */
	if(skill_num == 0 && skill_lv >= 0 && target_sd!=NULL && wd.div_ < 255 && atn_rand()%1000 < status_get_flee2(target) ){
		wd.damage=wd.damage2=0;
		wd.type=0x0b;
		wd.dmg_lv = ATK_LUCKY;
	}

	// �Ώۂ����S���������ݒ肪ON�Ȃ�
	if(battle_config.enemy_perfect_flee) {
		if(skill_num == 0 && skill_lv >= 0 && target_md!=NULL && wd.div_ < 255 && atn_rand()%1000 < status_get_flee2(target) ) {
			wd.damage=wd.damage2=0;
			wd.type=0x0b;
			wd.dmg_lv = ATK_LUCKY;
		}
	}

	/* 28�D�Œ�_���[�W2 */
	// Mob��Mode�Ɋ拭�t���O�������Ă���Ƃ��̏���
	if(t_mode&0x40 && skill_num != PA_PRESSURE){
		if(wd.damage > 0)
			wd.damage = (wd.div_<255)? 1: 3;	// �O�i���̂�3�_���[�W
		if(wd.damage2 > 0)
			wd.damage2 = 1;
	}

	// bNoWeaponDamage�ŃO�����h�N���X����Ȃ��ꍇ�̓_���[�W��0
	if( target_sd && target_sd->special_state.no_weapon_damage && skill_num != CR_GRANDCROSS && skill_num != NPC_DARKGRANDCROSS)
		wd.damage = wd.damage2 = 0;

	/* 29�D�_���[�W�ŏI�v�Z */
	if(skill_num != CR_GRANDCROSS && skill_num != NPC_DARKGRANDCROSS) {
		if(wd.damage2 < 1)		// �_���[�W�ŏI�C��
			wd.damage  = battle_calc_damage(src,target,wd.damage,wd.div_,skill_num,skill_lv,wd.flag);
		else if(wd.damage < 1)	// �E�肪�~�X�H
			wd.damage2 = battle_calc_damage(src,target,wd.damage2,wd.div_,skill_num,skill_lv,wd.flag);
		else {	// ����/�J�^�[���̏ꍇ�͂�����ƌv�Z��₱����
			int dmg = wd.damage+wd.damage2;
			wd.damage  = battle_calc_damage(src,target,dmg,wd.div_,skill_num,skill_lv,wd.flag);
			wd.damage2 = (wd.damage2*100/dmg)*wd.damage/100;
			if(wd.damage > 1 && wd.damage2 < 1) wd.damage2 = 1;
			wd.damage -= wd.damage2;
		}
	}

	/* 30�D�����U���X�L���ɂ��I�[�g�X�y������(item_bonus) */
	if(wd.flag&BF_SKILL && src && src->type == BL_PC && src != target && (wd.damage+wd.damage2) > 0)
	{
		long asflag = EAS_ATTACK;
		if(skill_num==AM_DEMONSTRATION)
			asflag += EAS_MISC;
		else{
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

	/* 31�D���z�ƌ��Ɛ��̗Z�� HP2%���� */
	if(src_sd && sc_data && sc_data[SC_FUSION].timer != -1)
	{
		int hp;
		if(target->type == BL_PC)
		{
			hp = src_sd->status.max_hp * 8 / 100;
			if( src_sd->status.hp < (src_sd->status.max_hp * 20 / 100))	// �Ώۂ��v���C���[��HP��20�������ł��鎞�A�U��������Α������܂��B
				hp = src_sd->status.hp;
		}else
			hp = src_sd->status.max_hp * 2 / 100;
		pc_heal(src_sd,-hp,0);
	}

	/* 32�D�J�A�q */
	if(skill_num==0 && wd.flag&BF_WEAPON && t_sc_data && t_sc_data[SC_KAAHI].timer != -1)
	{
		int kaahi_lv = t_sc_data[SC_KAAHI].val1;
		if(status_get_hp(target) < status_get_max_hp(target))
		{
			if(target->type == BL_MOB || status_get_sp(target) > 5*kaahi_lv)	// �Ώۂ�mob�ȊO��SP�������ʈȉ��̂Ƃ��͔������Ȃ�
			{
				unit_heal(target,200*kaahi_lv,-5*kaahi_lv);
				if(target->type == BL_PC)
					clif_misceffect3(target,7);				// �񕜂����{�l�ɂ̂݉񕜃G�t�F�N�g
			}
		}
	}

	/* 33�D�v�Z���ʂ̍ŏI�␳ */
	wd.damage2 = (skill_num == 0) ? wd.damage2 : 0;
	wd.amotion = status_get_amotion(src);
	if(skill_num == KN_AUTOCOUNTER)
		wd.amotion >>= 1;
	wd.dmotion = status_get_dmotion(target);

	return wd;
}

/*==========================================
 * ���@�_���[�W�v�Z
 *------------------------------------------
 */
struct Damage battle_calc_magic_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	int mdef1,mdef2;
	int matk1,matk2,damage=0,div_=1;
	int blewcount=skill_get_blewcount(skill_num,skill_lv);
	struct Damage md;
	int aflag;
	int normalmagic_flag=1;
	int ele=0,race=7,t_ele=0,t_race=7,t_enemy=0,t_mode = 0,cardfix,t_class,i;
	struct map_session_data *sd=NULL,*tsd=NULL;
	struct mob_data *tmd = NULL;
	struct homun_data *thd = NULL;
	struct status_change *sc_data;
	struct status_change *t_sc_data;
	long asflag = EAS_ATTACK;

	//return�O�̏���������̂ŏ��o�͕��̂ݕύX
	if( bl == NULL || target == NULL ){
		nullpo_info(NLP_MARK);
		memset(&md,0,sizeof(md));
		return md;
	}

	if(target->type == BL_PET) {
		memset(&md,0,sizeof(md));
		return md;
	}

	matk1 = status_get_matk1(bl);
	matk2 = status_get_matk2(bl);
	ele = skill_get_pl(skill_num);
	race = status_get_race(bl);

	mdef1 = status_get_mdef(target);
	mdef2 = status_get_mdef2(target);
	t_ele = status_get_elem_type(target);
	t_race = status_get_race(target);
	t_enemy = status_get_enemy_type(target);
	t_mode = status_get_mode(target);

#define MATK_FIX( a,b ) { matk1=matk1*(a)/(b); matk2=matk2*(a)/(b); }

	sd  = BL_DOWNCAST( BL_PC,  bl );
	tsd = BL_DOWNCAST( BL_PC,  target );
	tmd = BL_DOWNCAST( BL_MOB, target );
	thd = BL_DOWNCAST( BL_HOM, target );

	if(sd) {
		sd->state.attack_type = BF_MAGIC;
		if(sd->matk_rate != 100)
			MATK_FIX(sd->matk_rate,100);
		sd->state.arrow_atk = 0;
	}

	aflag=BF_MAGIC|BF_LONG|BF_SKILL;

	if(battle_config.calc_dist_flag & 2){	//���@�̎��v�Z���邩�H�@&2�Ōv�Z����
		int target_dist = unit_distance2(bl,target);	//�������擾
		if(target_dist < battle_config.allow_sw_dist){			//SW�Ŗh���鋗����菬�������ߋ�������̍U��
			if(battle_config.sw_def_type & 1 && bl->type==BL_PC)	//�l�Ԃ���̂𔻒肷�邩�@&1�ł���
				aflag=(aflag&~BF_RANGEMASK)|BF_SHORT;		//�ߋ����ɐݒ�
			if(battle_config.sw_def_type & 2 && bl->type==BL_MOB)	//�����X�^�[����̖��@�𔻒肷�邩�@&2�ł���
				aflag=(aflag&~BF_RANGEMASK)|BF_SHORT;		//�ߋ����ɐݒ�
		}
	}
	sc_data = status_get_sc_data(bl);
	t_sc_data = status_get_sc_data(target);

	// ���@�͑����ɂ��MATK����
	if (sc_data && sc_data[SC_MAGICPOWER].timer != -1) {
		matk1 += (matk1 * sc_data[SC_MAGICPOWER].val1 * 5)/100;
		matk2 += (matk2 * sc_data[SC_MAGICPOWER].val1 * 5)/100;
	}

	// ��{�_���[�W�v�Z(�X�L�����Ƃɏ���)
	switch(skill_num)
	{
		case AL_HEAL:	// �q�[��or����
		case PR_BENEDICTIO:
			damage = skill_calc_heal(bl,skill_lv)/2;
			if(sd)	//���f�B�^�e�B�I���悹��
				damage += damage * pc_checkskill(sd,HP_MEDITATIO)*2/100;
			normalmagic_flag=0;
			break;
		case PR_ASPERSIO:		/* �A�X�y���V�I */
			damage = 40; //�Œ�_���[�W
			normalmagic_flag=0;
			break;
		case PR_SANCTUARY:	// �T���N�`���A��
			ele = 6;
			damage = (skill_lv>6)?388:skill_lv*50;
			normalmagic_flag=0;
			blewcount|=0x10000;
			break;
		case PA_GOSPEL:		// �S�X�y��(�����_���_���[�W����̏ꍇ)
			damage = 1000+atn_rand()%9000;
			normalmagic_flag=0;
			break;
		case ALL_RESURRECTION:
		case PR_TURNUNDEAD:	// �U�����U���N�V�����ƃ^�[���A���f�b�h
			if(battle_check_undead(t_race,t_ele)){
				int hp, mhp, thres;
				hp = status_get_hp(target);
				mhp = status_get_max_hp(target);
				thres = (skill_lv * 20) + status_get_luk(bl)+
						status_get_int(bl) + status_get_lv(bl)+
						((200 - hp * 200 / mhp));
				if(thres > 700) thres = 700;
				if(atn_rand()%1000 < thres && !(t_mode&0x20))	// ����
					damage = hp;
				else					// ���s
					damage = status_get_lv(bl) + status_get_int(bl) + skill_lv * 10;
			}
			normalmagic_flag=0;
			break;

		case HW_NAPALMVULCAN:	// �i�p�[���o���J��
		case MG_NAPALMBEAT:	// �i�p�[���r�[�g�i���U�v�Z���݁j
			MATK_FIX(70+ skill_lv*10,100);
			if(flag>0){
				MATK_FIX(1,flag);
			}else {
				if(battle_config.error_log)
					printf("battle_calc_magic_attack(): napam enemy count=0 !\n");
			}
			break;
		case MG_SOULSTRIKE:			/* �\�E���X�g���C�N �i�΃A���f�b�h�_���[�W�␳�j*/
			if(battle_check_undead(t_race,t_ele))
				MATK_FIX( 20+skill_lv,20 );//MATK�ɕ␳����ʖڂł����ˁH
			break;
		case MG_FIREBALL:	// �t�@�C���[�{�[��
			if(flag>2)
				matk1=matk2=0;
			else{
				MATK_FIX((70+skill_lv*10),100);
				if(flag==2)
					MATK_FIX(3,4);
			}
			break;
		case MG_FIREWALL:	// �t�@�C���[�E�H�[��
			if((t_ele==3 || battle_check_undead(t_race,t_ele)) && target->type!=BL_PC)
				blewcount = 0;
			else
				blewcount |= 0x10000;
			MATK_FIX( 1,2 );
			break;
		case MG_THUNDERSTORM:	// �T���_�[�X�g�[��
			MATK_FIX( 80,100 );
			break;
		case MG_FROSTDIVER:	// �t���X�g�_�C�o
			MATK_FIX( 100+skill_lv*10, 100);
			break;
		case WZ_FROSTNOVA:	// �t���X�g�m���@
			MATK_FIX((100+skill_lv*10)*2/3, 100);
			break;
		case WZ_FIREPILLAR:	// �t�@�C���[�s���[
			if(mdef1 < 1000000)
				mdef1=mdef2=0;	// MDEF����
			if(bl->type!=BL_MOB)
				MATK_FIX( 1,5 );
			matk1+=50;
			matk2+=50;
			break;
		case WZ_SIGHTRASHER:
			MATK_FIX( 100+skill_lv*20, 100);
			break;
		case WZ_METEOR:
		case WZ_JUPITEL:	// ���s�e���T���_�[
		case NPC_DARKJUPITEL:	//�Ń��s�e��
			break;
		case WZ_VERMILION:	// ���[�h�I�u�o�[�~���I��
			MATK_FIX( skill_lv*20+80, 100 );
			break;
		case WZ_WATERBALL:	// �E�H�[�^�[�{�[��
			MATK_FIX( 100+skill_lv*30, 100 );
			break;
		case WZ_STORMGUST:	// �X�g�[���K�X�g
			MATK_FIX( skill_lv*40+100 ,100 );
//			blewcount|=0x10000;
			break;
		case AL_HOLYLIGHT:	// �z�[���[���C�g
			MATK_FIX( 125,100 );
			if(sc_data && sc_data[SC_PRIEST].timer!=-1)
			{
				MATK_FIX( 500,100 );
			//	matk1 *= 5;
			//	matk2 *= 5;
			}
			break;
		case AL_RUWACH:
			MATK_FIX( 145,100 );
			break;
		case WZ_SIGHTBLASTER:
			MATK_FIX( 145,100 );
			break;
		case SL_STIN://�G�X�e�B��
			if(status_get_size(target) == 0)
			{
				MATK_FIX(skill_lv*10,100);
			}
			else
			{
				MATK_FIX(skill_lv*1,100);
			}
			//ele = status_get_attack_element(bl);
			if(skill_lv>=7)
				status_change_start(bl,SC_SMA,skill_lv,0,0,0,3000,0);
			break;
		case SL_STUN://�G�X�^��
			MATK_FIX(skill_lv*5,100);
			ele = status_get_attack_element(bl);
			if(skill_lv>=7)
				status_change_start(bl,SC_SMA,skill_lv,0,0,0,3000,0);
			break;
		case SL_SMA://�G�X�}
			if(sd && skill_lv==10)
			{
				MATK_FIX(40+sd->status.base_level,100);
			}
			ele = status_get_attack_element(bl);
			if(sc_data && sc_data[SC_SMA].timer!=-1)
				status_change_end(bl,SC_SMA,-1);
			break;
		case NJ_KOUENKA:	// �g����
			MATK_FIX( 90,100 );
			break;
		case NJ_KAENSIN:	// �Ή��w
		case NJ_HUUJIN:		// ���n
			break;
		case NJ_HYOUSENSOU:	// �X�M��
			if(t_sc_data && t_sc_data[SC_SUITON].timer!=-1)
			{
				MATK_FIX(100+t_sc_data[SC_SUITON].val1*2,100 );
			}
			break;
		case NJ_BAKUENRYU:	// ������
			MATK_FIX(250+ skill_lv*150,300);
			break;
		case NJ_HYOUSYOURAKU:	// �X����
			MATK_FIX(200+ skill_lv*50,100);
			break;
		case NJ_RAIGEKISAI:		// ������
			MATK_FIX(160+ skill_lv*40,100);
			break;
		case NJ_KAMAITACHI:		// �J�}�C�^�`
			MATK_FIX(300+ skill_lv*100,100);
			break;
	}

	if(normalmagic_flag){	// ��ʖ��@�_���[�W�v�Z
		int imdef_flag=0;
		if(matk1>matk2)
			damage= matk2+atn_rand()%(matk1-matk2+1);
		else
			damage= matk2;
		if(sd) {
			int mask = (1<<t_race) | ( (t_mode&0x20)? (1<<10): (1<<11) );
			if(sd->ignore_mdef_ele & (1<<t_ele) || sd->ignore_mdef_race & mask || sd->ignore_mdef_enemy & (1<<t_enemy))
				imdef_flag = 1;
		}
		if(!imdef_flag){
			if(battle_config.magic_defense_type) {
				damage = damage - (mdef1 * battle_config.magic_defense_type) - mdef2;
			}
			else{
				damage = (damage*(100-mdef1))/100 - mdef2;
			}
		}
		if(damage<1) // �v���C���[�̖��@�X�L����1�_���[�W�ۏؖ���
			damage=(!battle_config.skill_min_damage && bl->type == BL_PC)?0:1;
	}

	if(sd) {
		cardfix=100;
		cardfix=cardfix*(100+sd->magic_addrace[t_race])/100;
		cardfix=cardfix*(100+sd->magic_addele[t_ele])/100;
		cardfix=cardfix*(100+sd->magic_addenemy[t_enemy])/100;
		if(t_mode & 0x20)
			cardfix=cardfix*(100+sd->magic_addrace[10])/100;
		else
			cardfix=cardfix*(100+sd->magic_addrace[11])/100;
		t_class = status_get_class(target);
		for(i=0;i<sd->add_magic_damage_class_count;i++) {
			if(sd->add_magic_damage_classid[i] == t_class) {
				cardfix=cardfix*(100+sd->add_magic_damage_classrate[i])/100;
				break;
			}
		}
		// �J�[�h���ʂɂ�����X�L���̃_���[�W�����i���@�X�L���j
		if((bl->type == BL_PC) && (sd->skill_dmgup.count > 0) && (skill_num > 0) && (damage > 0)){
			for( i=0 ; i<sd->skill_dmgup.count ; i++ ){
				if( skill_num == sd->skill_dmgup.id[i] ){
					cardfix = cardfix*(100+sd->skill_dmgup.rate[i])/100;
					break;
				}
			}
		}
		damage=damage*cardfix/100;
	}

	if( tsd ){
		int s_class = status_get_class(bl);
		cardfix=100;
		cardfix=cardfix*(100-tsd->subele[ele])/100;	// �����ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-tsd->subrace[race])/100;	// �푰�ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-tsd->subenemy[status_get_enemy_type(bl)])/100;	// �G�^�C�v�ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-tsd->subsize[status_get_size( bl )])/100;	// �T�C�Y�ɂ��_���[�W�ϐ�
		cardfix=cardfix*(100-tsd->magic_subrace[race])/100;
		if(status_get_mode(bl) & 0x20)
			cardfix=cardfix*(100-tsd->magic_subrace[10])/100;
		else
			cardfix=cardfix*(100-tsd->magic_subrace[11])/100;
		for(i=0;i<tsd->add_mdef_class_count;i++) {
			if(tsd->add_mdef_classid[i] == s_class) {
				cardfix=cardfix*(100-tsd->add_mdef_classrate[i])/100;
				break;
			}
		}
		cardfix=cardfix*(100-tsd->magic_def_rate)/100;
		damage=damage*cardfix/100;
	}
	if(damage < 0) damage = 0;

	damage=battle_attr_fix(damage, ele, status_get_element(target) );		// �����C��

	if(skill_num == CR_GRANDCROSS || skill_num ==NPC_DARKGRANDCROSS) {	// �O�����h�N���X
		static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		wd=battle_calc_weapon_attack(bl,target,skill_num,skill_lv,flag);
		damage = (damage + wd.damage) * (100 + 40*skill_lv)/100;
		if(battle_config.gx_dupele) damage=battle_attr_fix(damage, ele, status_get_element(target) );	//����2�񂩂���
		if(bl==target){
			if(bl->type == BL_MOB || bl->type == BL_HOM)
				damage = 0;		//MOB,HOM���g���ꍇ�͔�������
		else
			 damage=damage/2;	//�����͔���
		}
	}

	if (skill_num==WZ_WATERBALL)
		div_ = 1;
	else
		div_=skill_get_num( skill_num,skill_lv );

	if( tsd && tsd->special_state.no_magic_damage )
		damage=0;	// ����峃J�[�h�i���@�_���[�W�O)

	//�w�����[�h�Ȃ疂�@�_���[�W�Ȃ�
	if(t_sc_data && t_sc_data[SC_HERMODE].timer!=-1 && t_sc_data[SC_HERMODE].val1 == 1)
		damage = 0;

	if(skill_num==HW_GRAVITATION)	// �O���r�e�[�V�����t�B�[���h
		damage = 200+skill_lv*200;

	if(damage != 0) {
		if(t_mode&0x40) { // ���E���̂���
			// ���[�h�I�u���@�[�~���I���̓m�[�_���[�W�B����ȊO�͘A�Ő��_���[�W
			if (!battle_config.skill_min_damage && skill_num == WZ_VERMILION)
				damage = 0;
			else
				damage = (div_==255)? 3: div_;
		}
		else if(div_>1 && skill_num != WZ_VERMILION)
			damage*=div_;
	}

	damage=battle_calc_damage(bl,target,damage,div_,skill_num,skill_lv,aflag);	// �ŏI�C��

	//���@�ł��I�[�g�X�y������(item_bonus)
	if(bl && bl->type == BL_PC && bl != target && damage > 0)
	{
		if(battle_config.magic_attack_autospell)
			asflag += EAS_SHORT|EAS_LONG;
		else
			asflag += EAS_MAGIC;

		skill_bonus_autospell(bl,target,asflag,gettick(),0);
	}

	//���@�ł�HP/SP��(�������Ȃ�)
	if(battle_config.magic_attack_drain && sd)
		battle_attack_drain(bl,target,damage,0,battle_config.magic_attack_drain_per_enable);

	md.damage=damage;
	md.div_=div_;
	md.amotion=status_get_amotion(bl);
	md.dmotion=status_get_dmotion(target);
	md.damage2=0;
	md.type=0;
	md.blewcount=blewcount;
	md.flag=aflag;

	return md;
}

/*==========================================
 * ���̑��_���[�W�v�Z
 *------------------------------------------
 */
struct Damage  battle_calc_misc_attack(
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	int int_, dex;
	int skill,ele,race,cardfix;
	struct map_session_data *sd=NULL,*tsd=NULL;
	int damage=0,div_=1;
	int blewcount=skill_get_blewcount(skill_num,skill_lv);
	struct Damage md;
	int damagefix=1;
	long asflag = EAS_ATTACK;

	int aflag=BF_MISC|BF_SHORT|BF_SKILL;

	//return�O�̏���������̂ŏ��o�͕��̂ݕύX
	if( bl == NULL || target == NULL ){
		nullpo_info(NLP_MARK);
		memset(&md,0,sizeof(md));
		return md;
	}

	if(target->type == BL_PET) {
		memset(&md,0,sizeof(md));
		return md;
	}

	sd  = BL_DOWNCAST( BL_PC, bl );
	tsd = BL_DOWNCAST( BL_PC, target );

	if( sd ) {
		sd->state.attack_type = BF_MISC;
		sd->state.arrow_atk = 0;
	}

	int_ = status_get_int(bl);
	dex  = status_get_dex(bl);
	race = status_get_race(bl);
	ele  = skill_get_pl(skill_num);

	switch(skill_num){

	case HT_LANDMINE:	// �����h�}�C��
		damage=skill_lv*(dex+75)*(100+int_)/100;
		break;

	case HT_BLASTMINE:	// �u���X�g�}�C��
		damage=skill_lv*(dex/2+50)*(100+int_)/100;
		break;

	case HT_CLAYMORETRAP:	// �N���C���A�[�g���b�v
		damage=skill_lv*(dex/2+75)*(100+int_)/100;
		break;

	case HT_BLITZBEAT:	// �u���b�c�r�[�g
		if( sd==NULL || (skill = pc_checkskill(sd,HT_STEELCROW)) <= 0)
			skill=0;
		damage=(dex/10 + int_/2 + skill*3 + 40)*2;
		if(flag > 1)
			damage /= flag;
		flag &= ~(BF_SKILLMASK|BF_RANGEMASK|BF_WEAPONMASK);
		aflag = flag|(aflag&~BF_RANGEMASK)|BF_LONG;
		break;

	case TF_THROWSTONE:	// �Γ���
		damage=50;
		damagefix=0;
		flag &= ~(BF_SKILLMASK|BF_RANGEMASK|BF_WEAPONMASK);
		aflag = flag|(aflag&~BF_RANGEMASK)|BF_LONG;
		break;

	case BA_DISSONANCE:	// �s���a��
		damage=(skill_lv)*20+pc_checkskill(sd,BA_MUSICALLESSON)*3;
		break;
	case NPC_SELFDESTRUCTION:	// ����
	case NPC_SELFDESTRUCTION2:	// ����2
		damage=status_get_hp(bl)-(bl==target?1:0);
		damagefix=0;
		break;

	case NPC_SMOKING:	// �^�o�R���z��
		damage=3;
		damagefix=0;
		break;

	case NPC_DARKBREATH:
		{
			struct status_change *sc_data = status_get_sc_data(target);
			int hitrate = status_get_hit(bl) - status_get_flee(target) + 80;
			int t_hp=status_get_hp(target);
			hitrate = (hitrate > 95)? 95: (hitrate < 5)? 5: hitrate;
			if(sc_data && (sc_data[SC_SLEEP].timer!=-1 || sc_data[SC_STAN].timer!=-1 ||
				sc_data[SC_FREEZE].timer!=-1 || (sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0) ) )
				hitrate = 1000000;
			if(atn_rand()%100 < hitrate)
				damage = t_hp*(skill_lv*6)/100;
		}
		break;
	case SN_FALCONASSAULT:			/* �t�@���R���A�T���g */
		if( sd==NULL || (skill = pc_checkskill(sd,HT_STEELCROW)) <= 0)
			skill=0;
		damage=((dex/10+int_/2+skill*3+40)*2*(150+skill_lv*70)/100)*5;
		if(sd && battle_config.allow_falconassault_elemet)
			ele = sd->atk_ele;
		flag &= ~(BF_WEAPONMASK|BF_RANGEMASK|BF_WEAPONMASK);
		aflag = flag|(aflag&~BF_RANGEMASK)|BF_LONG;
		break;
	default:
		damage = status_get_baseatk(bl);
		break;
	}

	if(damagefix){
		if(damage<1 && skill_num != NPC_DARKBREATH)
			damage=1;

		if( tsd ){
			cardfix=100;
			cardfix=cardfix*(100-tsd->subele[ele])/100;	// �����ɂ��_���[�W�ϐ�
			cardfix=cardfix*(100-tsd->subrace[race])/100;	// �푰�ɂ��_���[�W�ϐ�
			cardfix=cardfix*(100-tsd->subenemy[status_get_enemy_type(bl)])/100;	// �G�^�C�v�ɂ��_���[�W�ϐ�
			cardfix=cardfix*(100-tsd->subsize[status_get_size( bl )])/100;	// �T�C�Y�ɂ��_���[�W�ϐ�
			cardfix=cardfix*(100-tsd->misc_def_rate)/100;
			damage=damage*cardfix/100;
		}
		if(damage < 0) damage = 0;
		damage=battle_attr_fix(damage, ele, status_get_element(target) );		// �����C��
	}

	div_=skill_get_num( skill_num,skill_lv );
	if(div_>1)
		damage*=div_;

	if(damage > 0 && (damage < div_ || (status_get_def(target) >= 1000000 && status_get_mdef(target) >= 1000000) ) ) {
		damage = div_;
	}

	if(status_get_mode(target)&0x40 && damage>0) // ���E���̂���
		damage = 1;

	// �J�[�h���ʂɂ�����X�L���̃_���[�W�����i���̑��̃X�L���j
	if(sd && sd->skill_dmgup.count > 0 && skill_num > 0 && damage > 0){
		int i;
		for( i=0 ; i<sd->skill_dmgup.count ; i++ ){
			if( skill_num == sd->skill_dmgup.id[i] ){
				damage += damage * sd->skill_dmgup.rate[i] / 100;
				break;
			}
		}
	}

	damage=battle_calc_damage(bl,target,damage,div_,skill_num,skill_lv,aflag);	// �ŏI�C��

	//misc�ł��I�[�g�X�y������(bonus)
	if(bl->type == BL_PC && bl != target && damage > 0)
	{
		if(battle_config.misc_attack_autospell)
			asflag += EAS_SHORT|EAS_LONG;
		else
			asflag += EAS_MISC;

		skill_bonus_autospell(bl,target,asflag,gettick(),0);
	}

	//misc�ł�HP/SP��(�������Ȃ�)
	if(battle_config.misc_attack_drain)
		battle_attack_drain(bl,target,damage,0,battle_config.misc_attack_drain_per_enable);

	md.damage=damage;
	md.div_=div_;
	md.amotion=status_get_amotion(bl);
	md.dmotion=status_get_dmotion(target);
	md.damage2=0;
	md.type=0;
	md.blewcount=blewcount;
	md.flag=aflag;

	return md;

}
/*==========================================
 * �U���ɂ��HP/SP�z��
 *------------------------------------------
 */
int battle_attack_drain(struct block_list *bl,struct block_list *target,int damage,int damage2,int calc_per_drain_flag)
{
	int hp = 0,sp = 0;
	struct map_session_data* sd = NULL;

	nullpo_retr(0, bl);
	nullpo_retr(0, target);

	if( bl->type != BL_PC || (sd=(struct map_session_data *)bl) == NULL )
		return 0;

	if(bl == target)
		return 0;

	if(!(damage > 0 || damage2 >0))
		return 0;

	if(calc_per_drain_flag)//���z�����̂���
	{
		if (!battle_config.left_cardfix_to_right) { // �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ����Ȃ��ꍇ
			hp += battle_calc_drain(damage, sd->hp_drain_rate, sd->hp_drain_per, sd->hp_drain_value);
			hp += battle_calc_drain(damage2, sd->hp_drain_rate_, sd->hp_drain_per_, sd->hp_drain_value_);
			sp += battle_calc_drain(damage, sd->sp_drain_rate, sd->sp_drain_per, sd->sp_drain_value);
			sp += battle_calc_drain(damage2, sd->sp_drain_rate_, sd->sp_drain_per_, sd->sp_drain_value_);
		} else { // �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ�����ꍇ
			int hp_drain_rate = sd->hp_drain_rate + sd->hp_drain_rate_;
			int hp_drain_per = sd->hp_drain_per + sd->hp_drain_per_;
			int hp_drain_value = sd->hp_drain_value + sd->hp_drain_value_;
			int sp_drain_rate = sd->sp_drain_rate + sd->sp_drain_rate_;
			int sp_drain_per = sd->sp_drain_per + sd->sp_drain_per_;
			int sp_drain_value = sd->sp_drain_value + sd->sp_drain_value_;
			hp += battle_calc_drain(damage, hp_drain_rate, hp_drain_per, hp_drain_value);
			sp += battle_calc_drain(damage, sp_drain_rate, sp_drain_per, sp_drain_value);
		}
	}else{//���z���͏悹�Ȃ�
		if (!battle_config.left_cardfix_to_right) { // �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ����Ȃ��ꍇ
			hp += battle_calc_drain(damage, sd->hp_drain_rate, 0, sd->hp_drain_value);
			hp += battle_calc_drain(damage2, sd->hp_drain_rate_, 0, sd->hp_drain_value_);
			sp += battle_calc_drain(damage, sd->sp_drain_rate, 0, sd->sp_drain_value);
			sp += battle_calc_drain(damage2, sd->sp_drain_rate_, 0, sd->sp_drain_value_);
		} else { // �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ�����ꍇ
			int hp_drain_rate = sd->hp_drain_rate + sd->hp_drain_rate_;
			int hp_drain_value = sd->hp_drain_value + sd->hp_drain_value_;
			int sp_drain_rate = sd->sp_drain_rate + sd->sp_drain_rate_;
			int sp_drain_value = sd->sp_drain_value + sd->sp_drain_value_;
			hp += battle_calc_drain(damage,hp_drain_rate, 0, hp_drain_value);
			sp += battle_calc_drain(damage,sp_drain_rate, 0, sp_drain_value);
		}
	}
	if (hp || sp) pc_heal(sd, hp, sp);
	return 1;
}
/*==========================================
 * �_���[�W�v�Z�ꊇ�����p
 *------------------------------------------
 */
struct Damage battle_calc_attack( int attack_type,
	struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	switch(attack_type){
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
	struct map_session_data *sd=NULL,*tsd=NULL;
	struct status_change *sc_data, *t_sc_data;
	short *opt1;
	int race = 7, ele = 0;
	int damage,rdamage = 0;
	static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	long asflag = EAS_ATTACK;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	if(src->prev == NULL || target->prev == NULL)
		return 0;
	if(unit_isdead(src) || unit_isdead(target))
		return 0;

	sd  = BL_DOWNCAST(BL_PC, src);
	tsd = BL_DOWNCAST(BL_PC, target);

	opt1=status_get_opt1(src);
	if(opt1 && *opt1 > 0) {
		unit_stopattack(src);
		return 0;
	}

	sc_data = status_get_sc_data(src);
	t_sc_data = status_get_sc_data(target);

	//���������H�E�����ړ��E���f�̃E�B���N���̓_��
	if(sc_data && (sc_data[SC_BLADESTOP].timer!=-1 || sc_data[SC_FORCEWALKING].timer!=-1 || sc_data[SC_WINKCHARM].timer!=-1)) {
		unit_stopattack(src);
		return 0;
	}
	//���肪�����ړ���
	if(t_sc_data && t_sc_data[SC_FORCEWALKING].timer!=-1){
		unit_stopattack(src);
		return 0;
	}

	if(battle_check_target(src,target,BCT_ENEMY) <= 0 && !battle_check_range(src,target,0))
		return 0;	// �U���ΏۊO

	//�^�[�Q�b�g��MOB GM�n�C�h���ŁA�R���t�B�O�Ńn�C�h���U���s�� GM���x�����w����傫���ꍇ
	if(target->type==BL_MOB && sd && sd->status.option&0x40 && battle_config.hide_attack == 0 && pc_isGM(sd)<battle_config.gm_hide_attack_lv)
		return 0;	// �B��čU������Ȃ�Ĕڋ���GM�޽�

	race = status_get_race(target);
	ele = status_get_elem_type(target);

	if(sd){
		if(!battle_delarrow(sd,1,0))
			return 0;
	}

	if(flag&0x8000) {
		if(sd && battle_config.pc_attack_direction_change)
			sd->dir = sd->head_dir = map_calc_dir(src, target->x,target->y );
		else if(src->type == BL_MOB && battle_config.monster_attack_direction_change)
			((struct mob_data *)src)->dir = map_calc_dir(src, target->x,target->y );
		else if(src->type == BL_HOM && battle_config.monster_attack_direction_change)	// homun_attack_direction_change
			((struct homun_data *)src)->dir = map_calc_dir(src, target->x,target->y );
		wd=battle_calc_weapon_attack(src,target,KN_AUTOCOUNTER,flag&0xff,0);
	} else
		wd=battle_calc_weapon_attack(src,target,0,0,0);

	if((damage = wd.damage + wd.damage2) > 0 && src != target) {
		if(wd.flag&BF_SHORT) {
			if(tsd && tsd->short_weapon_damage_return > 0) {
				rdamage += damage * tsd->short_weapon_damage_return / 100;
				if(rdamage < 1) rdamage = 1;
			}
			if(t_sc_data && t_sc_data[SC_REFLECTSHIELD].timer != -1) {
				rdamage += damage * t_sc_data[SC_REFLECTSHIELD].val2 / 100;
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

	if (wd.div_ == 255 && sd) { //�O�i��
		int delay = 0;
		int skilllv;
		if(wd.damage+wd.damage2 < status_get_hp(target)) {
			if((skilllv = pc_checkskill(sd, MO_CHAINCOMBO)) > 0) {
				delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
				delay += 300 * battle_config.combo_delay_rate /100;
				//�R���{���͎��Ԃ̍Œ�ۏ�ǉ�
				if( delay < battle_config.combo_delay_lower_limits )
					delay = battle_config.combo_delay_lower_limits;
			}
			status_change_start(src,SC_COMBO,MO_TRIPLEATTACK,skilllv,0,0,delay,0);
		}
		sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
		clif_combo_delay(src,delay);
		clif_skill_damage(src , target , tick , wd.amotion , wd.dmotion ,
			wd.damage , 3 , MO_TRIPLEATTACK, pc_checkskill(sd,MO_TRIPLEATTACK) , -1 );

		//�N���[���X�L��
		if(wd.damage> 0 && tsd && pc_checkskill(tsd,RG_PLAGIARISM) && sc_data && sc_data[SC_PRESERVE].timer == -1){
			skill_clone(tsd,MO_TRIPLEATTACK,pc_checkskill(sd, MO_TRIPLEATTACK));
		}
	}else if (wd.div_ >= 251 && wd.div_<=254 && sd)	{ //�e�R���R��n��
		int delay = 0;
		int skillid = TK_STORMKICK + 2*(wd.div_-251);
		int skilllv;
		delay = status_get_adelay(src);
		if(wd.damage+wd.damage2 < status_get_hp(target)) {
			if((skilllv = pc_checkskill(sd, skillid)) > 0) {
				//delay += 500 * battle_config.combo_delay_rate /100;
				delay += 2000 - 4*status_get_agi(src) - 2*status_get_dex(src);
			}
			status_change_start(src,SC_TKCOMBO,skillid,skilllv,0,0,delay,0);
		}
		sd->ud.attackabletime = tick + delay;
		//clif_combo_delay(src,delay);
		clif_skill_nodamage(&sd->bl,&sd->bl,skillid-1,pc_checkskill(sd,skillid-1),1);
	}
	else {
		clif_damage(src,target,tick, wd.amotion, wd.dmotion,
			wd.damage, wd.div_ , wd.type, wd.damage2);
	//�񓁗�����ƃJ�^�[���ǌ��̃~�X�\��(�������`)
		if(sd && (sd->status.weapon > WT_HUUMA || sd->status.weapon == WT_KATAR) && wd.damage2 == 0)
			clif_damage(src,target,tick+10, wd.amotion, wd.dmotion,0, 1, 0, 0);
	}
	if(sd && sd->splash_range > 0 && (wd.damage > 0 || wd.damage2 > 0))
		skill_castend_damage_id(src,target,0,-1,tick,0);

	map_freeblock_lock();
	battle_delay_damage(tick+wd.amotion,src,target,(wd.damage+wd.damage2),wd.flag);
	if(target->prev != NULL &&
		(target->type != BL_PC || (target->type == BL_PC && !unit_isdead(target)))) {
		if(wd.damage > 0 || wd.damage2 > 0){
			skill_additional_effect(src,target,0,0,BF_WEAPON,tick);
			if(sd) {
				if(sd->break_weapon_rate > 0 && atn_rand()%10000 < sd->break_weapon_rate && tsd)
					pc_break_equip(tsd, EQP_WEAPON);
				if(sd->break_armor_rate > 0 && atn_rand()%10000 < sd->break_armor_rate && tsd)
					pc_break_equip(tsd, EQP_ARMOR);
			}
		}
	}
	if(sc_data && sc_data[SC_AUTOSPELL].timer != -1 && atn_rand()%100 < sc_data[SC_AUTOSPELL].val4) {
		int skilllv = sc_data[SC_AUTOSPELL].val3;
		int i = atn_rand()%100, f = 0;

		if(i >= 50) skilllv -= 2;
		else if(i >= 15) skilllv--;
		if(skilllv < 1) skilllv = 1;
		//PC���Z�[�W�̍�
		if(sd && sd->sc_data[SC_SAGE].timer!=-1)
			skilllv = pc_checkskill(sd,sc_data[SC_AUTOSPELL].val2);

		if(sd) {
			int sp = skill_get_sp(sc_data[SC_AUTOSPELL].val2,skilllv)*2/3;
			if(sd->status.sp >= sp) {
				if((skill_get_inf(sd->autospell_id) == 2) || (skill_get_inf(sd->autospell_id) == 32))
					f = skill_castend_pos2(src,target->x,target->y,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
				else {
					switch( skill_get_nk(sc_data[SC_AUTOSPELL].val2) ) {
						case 0:
						case 2:
						case 4:
							f = skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
							break;
						case 1:/* �x���n */
							if((sc_data[SC_AUTOSPELL].val2==AL_HEAL || (sc_data[SC_AUTOSPELL].val2==ALL_RESURRECTION && target->type != BL_PC)) && battle_check_undead(race,ele))
								f = skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
							else
								f = skill_castend_nodamage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
						default:
							break;
					}
				}
				if(!f) pc_heal(sd,0,-sp);
			}
		} else {
			if((skill_get_inf(sc_data[SC_AUTOSPELL].val2) == 2) || (skill_get_inf(sc_data[SC_AUTOSPELL].val2) == 32))
				skill_castend_pos2(src,target->x,target->y,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
			else {
				switch (skill_get_nk(sc_data[SC_AUTOSPELL].val2)) {
					case 0:
					case 2:
						skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
						break;
					case 1:/* �x���n */
						if((sc_data[SC_AUTOSPELL].val2==AL_HEAL || (sc_data[SC_AUTOSPELL].val2==ALL_RESURRECTION && target->type != BL_PC)) && battle_check_undead(race,ele))
							skill_castend_damage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
						else
							skill_castend_nodamage_id(src,target,sc_data[SC_AUTOSPELL].val2,skilllv,tick,flag);
					default:
						break;
				}
			}
		}
	}

	//�J�[�h�ɂ��I�[�g�X�y��
	if(sd && sd->bl.type == BL_PC && src != target 	&& (wd.damage > 0 || wd.damage2 > 0))
	{
		asflag += EAS_NORMAL;
		if(wd.flag&BF_LONG)
			asflag += EAS_LONG;
		else
			asflag += EAS_SHORT;

		skill_bonus_autospell(src,target,asflag,gettick(),0);
	}

	if(sd && sd->bl.type == BL_PC && src != target && wd.flag&BF_WEAPON && (wd.damage > 0 || wd.damage2 > 0))
	{
		//SP����
		if(tsd && atn_rand()%100 < sd->sp_vanish_rate)
		{
			int sp = 0;
			sp = status_get_sp(target)* sd->sp_vanish_per/100;
			if (sp > 0)
				pc_heal(tsd, 0, -sp);
		}
	}

	if(sd){
		if (wd.flag&BF_WEAPON && src != target && (wd.damage > 0 || wd.damage2 > 0)) {
			int hp = 0,sp = 0;
			if (!battle_config.left_cardfix_to_right) { // �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ����Ȃ��ꍇ
				hp += battle_calc_drain(wd.damage, sd->hp_drain_rate, sd->hp_drain_per, sd->hp_drain_value);
				hp += battle_calc_drain(wd.damage2, sd->hp_drain_rate_, sd->hp_drain_per_, sd->hp_drain_value_);
				sp += battle_calc_drain(wd.damage, sd->sp_drain_rate, sd->sp_drain_per, sd->sp_drain_value);
				sp += battle_calc_drain(wd.damage2, sd->sp_drain_rate_, sd->sp_drain_per_, sd->sp_drain_value_);
			} else { // �񓁗�����J�[�h�̋z���n���ʂ��E��ɒǉ�����ꍇ
				int hp_drain_rate = sd->hp_drain_rate + sd->hp_drain_rate_;
				int hp_drain_per = sd->hp_drain_per + sd->hp_drain_per_;
				int hp_drain_value = sd->hp_drain_value + sd->hp_drain_value_;
				int sp_drain_rate = sd->sp_drain_rate + sd->sp_drain_rate_;
				int sp_drain_per = sd->sp_drain_per + sd->sp_drain_per_;
				int sp_drain_value = sd->sp_drain_value + sd->sp_drain_value_;
				hp += battle_calc_drain(wd.damage, hp_drain_rate, hp_drain_per, hp_drain_value);
				sp += battle_calc_drain(wd.damage, sp_drain_rate, sp_drain_per, sp_drain_value);
			}
			if (hp || sp) pc_heal(sd, hp, sp);
		}
	}

	if(rdamage > 0){
		battle_delay_damage(tick+wd.amotion,target,src,rdamage,0);

		//���˃_���[�W�̃I�[�g�X�y��
		if(battle_config.weapon_reflect_autospell && target->type == BL_PC)
			skill_bonus_autospell(target,src,EAS_ATTACK,gettick(),0);

		if(battle_config.weapon_reflect_drain)
			battle_attack_drain(target,src,rdamage,0,battle_config.weapon_reflect_drain_per_enable);
	}
	if(t_sc_data && t_sc_data[SC_AUTOCOUNTER].timer != -1 && t_sc_data[SC_AUTOCOUNTER].val4 > 0) {
		if(t_sc_data[SC_AUTOCOUNTER].val3 == src->id)
			battle_weapon_attack(target,src,tick,0x8000|t_sc_data[SC_AUTOCOUNTER].val1);
		status_change_end(target,SC_AUTOCOUNTER,-1);
	}

	if (t_sc_data && t_sc_data[SC_BLADESTOP_WAIT].timer != -1 &&
			!(status_get_mode(src)&0x20)) { // �{�X�ɂ͖���
		int lv = t_sc_data[SC_BLADESTOP_WAIT].val1;
		status_change_end(target,SC_BLADESTOP_WAIT,-1);
		status_change_start(src,SC_BLADESTOP,lv,1,src->id,target->id,skill_get_time2(MO_BLADESTOP,lv),0);
		status_change_start(target,SC_BLADESTOP,lv,2,target->id,src->id,skill_get_time2(MO_BLADESTOP,lv),0);
	}

	if (t_sc_data && t_sc_data[SC_POISONREACT].timer != -1) {
		// �ő���mob�܂��͓ő����ɂ��U���Ȃ�Δ���
		if( (src->type==BL_MOB && status_get_elem_type(src) == 5) || status_get_attack_element(src) == 5 ) {
			if( battle_check_range(target,src,status_get_range(target)+1) ) {
				t_sc_data[SC_POISONREACT].val2 = 0;
				battle_skill_attack(BF_WEAPON,target,target,src,AS_POISONREACT,t_sc_data[SC_POISONREACT].val1,tick,0);
			}
		}
		// ����ȊO�̒ʏ�U���ɑ΂���C���x�����i�ː��`�F�b�N�Ȃ��j
		else {
			--t_sc_data[SC_POISONREACT].val2;
			if(atn_rand()&1) {
				if( tsd==NULL || pc_checkskill(tsd,TF_POISON)>=5 )
					battle_skill_attack(BF_WEAPON,target,target,src,TF_POISON,5,tick,flag);
			}
		}
		if (t_sc_data[SC_POISONREACT].val2 <= 0)
			status_change_end(target,SC_POISONREACT,-1);
	}
	map_freeblock_unlock();
	return wd.dmg_lv;
}

/*
 * =========================================================================
 * �X�L���U�����ʏ����܂Ƃ�
 * flag�̐����B16�i�}
 * 	00XRTTff
 *  ff	= magic�Ōv�Z�ɓn�����
 *	TT	= �p�P�b�g��type����(0�Ńf�t�H���g)
 *  X   = �p�P�b�g�̃X�L��Lv
 *  R	= �\��iskill_area_sub�Ŏg�p����)
 *-------------------------------------------------------------------------
 */
int battle_skill_attack(int attack_type,struct block_list* src,struct block_list *dsrc,
	 struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag)
{
	struct Damage dmg;
	struct map_session_data *sd = NULL, *tsd = NULL;
	struct status_change *sc_data;
	struct status_change *ssc_data;
	int type,lv,damage, rdamage = 0;

	nullpo_retr(0, src);
	nullpo_retr(0, dsrc);
	nullpo_retr(0, bl);

	sc_data = status_get_sc_data(bl);
	ssc_data = status_get_sc_data(src);

	sd  = BL_DOWNCAST( BL_PC, src );
	tsd = BL_DOWNCAST( BL_PC, bl );

	if(dsrc->m != bl->m)	//�Ώۂ������}�b�v�ɂ��Ȃ���Ή������Ȃ�
		return 0;
	if(src->prev == NULL || dsrc->prev == NULL || bl->prev == NULL)
		return 0;
	if(unit_isdead(src) || unit_isdead(dsrc) || unit_isdead(bl))	//���łɎ���ł����牽�����Ȃ�
		return 0;

	if(ssc_data) {		// �����������ړ��������͖��f�̃E�B���N���Ȃ牽�����Ȃ�
		if(ssc_data[SC_FORCEWALKING].timer != -1 || ssc_data[SC_WINKCHARM].timer != -1)
			return 0;
	}
	if(sc_data) {
		if(sc_data[SC_HIDING].timer != -1 && skill_get_pl(skillid) != 2)	//�n�C�f�B���O��ԂŃX�L���̑������n�����łȂ��Ȃ牽�����Ȃ�
			return 0;
		if(sc_data[SC_CHASEWALK].timer != -1 && skillid == AL_RUWACH)	//�`�F�C�X�E�H�[�N��ԂŃ��A�t����
			return 0;
		if(sc_data[SC_TRICKDEAD].timer != -1) 				//���񂾂ӂ蒆�͉������Ȃ�
			return 0;
		if(sc_data[SC_FORCEWALKING].timer != -1) 			//�����ړ����͉������Ȃ�
			return 0;
		//������ԂŃX�g�[���K�X�g�A�t���X�g�m���@�A�X�՗��͖���
		if(sc_data[SC_FREEZE].timer != -1 && (skillid == WZ_STORMGUST || skillid == WZ_FROSTNOVA || skillid == NJ_HYOUSYOURAKU))
			return 0;
	}
	if(skillid == WZ_FROSTNOVA && dsrc->x == bl->x && dsrc->y == bl->y)	//�g�p�X�L�����t���X�g�m���@�ŁAdsrc��bl�������ꏊ�Ȃ牽�����Ȃ�
		return 0;
	if(sd && sd->chatID)	//��������PC�Ń`���b�g���Ȃ牽�����Ȃ�
		return 0;
	if(sd && mob_gvmobcheck(sd,bl)==0)
		return 0;

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
			case AS_VENOMKNIFE:	//�x�i���i�C�t����
			case NJ_SYURIKEN:	//�藠������
			case NJ_KUNAI:		//�N�i�C����
			case GS_TRACKING:	//�e�e����
			case GS_DISARM:
			case GS_PIERCINGSHOT:
			case GS_DUST:
			case GS_RAPIDSHOWER:	//��������
			case GS_FULLBUSTER:
			case GS_GROUNDDRIFT:	//�O���l�[�h�e����
				if( !battle_delarrow(sd, cost, skillid) )
					return 0;
				break;
			case AC_SHOWER:
			case SN_SHARPSHOOTING:
			case GS_SPREADATTACK:
			case GS_DESPERADO:
				if( flag == 0 && !battle_delarrow(sd, cost, skillid) )
					return 0;
				break;
			}
		}
	}

	/* �_���[�W�v�Z */
	type=-1;
	lv=(flag>>20)&0xf;
	dmg=battle_calc_attack(attack_type,src,bl,skillid,skilllv,flag&0xff );

	/* �}�W�b�N���b�h */
	if(attack_type&BF_MAGIC && sc_data && sc_data[SC_MAGICROD].timer != -1 && src == dsrc) {
		dmg.damage = dmg.damage2 = 0;
		if(tsd) {	//�Ώۂ�PC�̏ꍇ
			int sp = skill_get_sp(skillid,skilllv);		//�g�p���ꂽ�X�L����SP���z��
			sp = sp * sc_data[SC_MAGICROD].val2 / 100;
			if(skillid == WZ_WATERBALL && skilllv > 1)
				sp = sp/((skilllv|1)*(skilllv|1));	//�E�H�[�^�[�{�[���͂���Ɍv�Z�H
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
			tsd->ud.canact_tick = tick + skill_delayfix(&tsd->bl, skill_get_delay(SA_MAGICROD,sc_data[SC_MAGICROD].val1), skill_get_cast(SA_MAGICROD,sc_data[SC_MAGICROD].val1)); //
		}
		clif_skill_nodamage(bl,bl,SA_MAGICROD,sc_data[SC_MAGICROD].val1,1);	//�}�W�b�N���b�h�G�t�F�N�g��\��
	}

	damage = dmg.damage + dmg.damage2;

	if(lv==15)
		lv=-1;

	if( flag&0xff00 )
		type=(flag&0xff00)>>8;

	if(damage <= 0 || damage < dmg.div_)	//������΂�����H��
		dmg.blewcount = 0;

	if(skillid == CR_GRANDCROSS || skillid == NPC_DARKGRANDCROSS) {	//�O�����h�N���X
		if(battle_config.gx_disptype) dsrc = src;		// �G�_���[�W�������\��
		if( src == bl) type = 4;	// �����̓_���[�W���[�V�����Ȃ�
	}

	/* �R���{ */
	if(sd) {
		//�A�ŏ�(MO_CHAINCOMBO)��������
		if(skillid == MO_CHAINCOMBO) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src); //��{�f�B���C�̌v�Z
			if(damage < status_get_hp(bl)) { //�_���[�W���Ώۂ�HP��菬�����ꍇ
				if(pc_checkskill(sd, MO_COMBOFINISH) > 0 && sd->spiritball > 0){ //�җ���(MO_COMBOFINISH)�擾���C���ێ�����+300ms
					delay += 300 * battle_config.combo_delay_rate /100; //�ǉ��f�B���C��conf�ɂ�蒲��
					//�R���{���͎��Ԃ̍Œ�ۏ�ǉ�
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,MO_CHAINCOMBO,skilllv,0,0,delay,0); //�R���{��Ԃ�
			}
			sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //�R���{�f�B���C�p�P�b�g�̑��M
		}
		//�җ���(MO_COMBOFINISH)��������
		else if(skillid == MO_COMBOFINISH) {
			int delay = 700 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				//���C���e����(MO_EXTREMITYFIST)�擾���C��4�ێ��������g��(MO_EXPLOSIONSPIRITS)��Ԏ���+300ms
				//���Ռ�(CH_TIGERFIST)�擾����+300ms
				if((pc_checkskill(sd, MO_EXTREMITYFIST) > 0 && sd->spiritball >= 4 && sd->sc_data[SC_EXPLOSIONSPIRITS].timer != -1) ||
				(pc_checkskill(sd, CH_TIGERFIST) > 0 && sd->spiritball > 0) ||
				(pc_checkskill(sd, CH_CHAINCRUSH) > 0 && sd->spiritball > 1))
				{
					delay += 300 * battle_config.combo_delay_rate /100; //�ǉ��f�B���C��conf�ɂ�蒲��
					//�R���{���͎��ԍŒ�ۏ�ǉ�
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,MO_COMBOFINISH,skilllv,0,0,delay,0); //�R���{��Ԃ�
			}
			sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //�R���{�f�B���C�p�P�b�g�̑��M
		}
		//���Ռ�(CH_TIGERFIST)��������
		else if(skillid == CH_TIGERFIST) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				if(pc_checkskill(sd, CH_CHAINCRUSH) > 0){ //�A������(CH_CHAINCRUSH)�擾����+300ms
					delay += 300 * battle_config.combo_delay_rate /100; //�ǉ��f�B���C��conf�ɂ�蒲��
					//�R���{���͎��ԍŒ�ۏ�ǉ�
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}

				status_change_start(src,SC_COMBO,CH_TIGERFIST,skilllv,0,0,delay,0); //�R���{��Ԃ�
			}
			sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //�R���{�f�B���C�p�P�b�g�̑��M
		}
		//�A������(CH_CHAINCRUSH)��������
		else if(skillid == CH_CHAINCRUSH) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				//���Ռ��K���܂��͈��C���K�����C��1�ێ��������g�����f�B���C
				if(pc_checkskill(sd, CH_TIGERFIST) > 0 || (pc_checkskill(sd, MO_EXTREMITYFIST) > 0 && sd->spiritball >= 1 && sd->sc_data[SC_EXPLOSIONSPIRITS].timer != -1))
				{
					delay += (600+(skilllv/5)*200) * battle_config.combo_delay_rate /100; //�ǉ��f�B���C��conf�ɂ�蒲��
					//�R���{���͎��ԍŒ�ۏ�ǉ�
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,CH_CHAINCRUSH,skilllv,0,0,delay,0); //�R���{��Ԃ�
			}
			sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
			clif_combo_delay(src,delay); //�R���{�f�B���C�p�P�b�g�̑��M
		}
		//TK�R���{
		if(skillid == TK_STORMKICK || skillid == TK_DOWNKICK || skillid == TK_TURNKICK || skillid == TK_COUNTER) {
			if(ranking_get_pc_rank(sd,RK_TAEKWON) > 0) {	// �e�R�������J�[�̓R���{���s
				int delay = status_get_adelay(src);
				if(damage < status_get_hp(bl)) {
					//delay += 500 * battle_config.combo_delay_rate /100;
					delay += 2000 - 4*status_get_agi(src) - 2*status_get_dex(src);	// eA����
					status_change_start(src,SC_TKCOMBO,skillid,0,0,TK_MISSION,delay,0);
				}
				sd->ud.attackabletime = tick + delay;
			} else {
				status_change_end(src,SC_TKCOMBO,-1);
			}
		}
	}

	/* �_���[�W���� */
	if(attack_type&BF_WEAPON && damage > 0 && src != bl && src == dsrc) { //����X�L�����_���[�W���聕�g�p�҂ƑΏێ҂��Ⴄ��src=dsrc
		if(dmg.flag&BF_SHORT) { //�ߋ����U�����H��
			if(tsd) { //�Ώۂ�PC�̎�
				if(tsd->short_weapon_damage_return > 0) { //�ߋ����U�����˕Ԃ��H��
					rdamage += damage * tsd->short_weapon_damage_return / 100;
					if(rdamage < 1) rdamage = 1;
				}
			}
			if(sc_data && sc_data[SC_REFLECTSHIELD].timer != -1) { //���t���N�g�V�[���h��
				rdamage += damage * sc_data[SC_REFLECTSHIELD].val2 / 100; //���˕Ԃ��v�Z
				if(rdamage < 1) rdamage = 1;
			}
		}
		else if(dmg.flag&BF_LONG) { //�������U�����H��
			if(tsd) { //�Ώۂ�PC�̎�
				if(tsd->long_weapon_damage_return > 0) { //�������U�����˕Ԃ��H��
					rdamage += damage * tsd->long_weapon_damage_return / 100;
					if(rdamage < 1) rdamage = 1;
				}
			}
		}
		if(rdamage > 0)
			clif_damage(src,src,tick, dmg.amotion,0,rdamage,1,4,0);
	}
	if(attack_type&BF_MAGIC && damage > 0 && src != bl && src == dsrc) { //���@�X�L�����_���[�W���聕�g�p�҂ƑΏێ҂��Ⴄ
		if(tsd) { //�Ώۂ�PC�̎�
			if(tsd->magic_damage_return > 0 && atn_rand()%100 < tsd->magic_damage_return) { //���@�U�����˕Ԃ��H��
				rdamage = damage;
				damage  = -1;	// �_���[�W0����miss���o���Ȃ�
			}
		}
		//�J�C�g
		if(damage > 0 && sc_data && sc_data[SC_KAITE].timer!=-1)
		{
			if(src->type == BL_PC || (status_get_lv(src) < 80 && !(status_get_mode(src)&0x20)))
			{
				int idx;
				clif_misceffect2(bl,438);
				clif_skill_nodamage(bl,src,skillid,skilllv,1);
				if(--sc_data[SC_KAITE].val2==0)
					status_change_end(bl,SC_KAITE,-1);

				if( sd && ssc_data && ssc_data[SC_WIZARD].timer!=-1 &&
				    (idx = pc_search_inventory(sd,7321)) >= 0 && sd->status.inventory[idx].amount > 0 ) {
					pc_delitem(sd,idx,1,0);
				} else {
					rdamage += damage;
				}
				damage = -1;	// �_���[�W0����miss���o���Ȃ�
			}
		}
		if(rdamage > 0){
			//clif_damage(src,src,tick, dmg.amotion,0,rdamage,1,4,0);
			clif_skill_damage(src,src,tick,dmg.amotion,dmg.dmotion, rdamage, dmg.div_, skillid, (lv!=0)?lv:skilllv, (skillid==0)? 5:type );
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
		switch(skillid){
		case AS_SPLASHER:
			clif_skill_damage(dsrc,bl,tick,dmg.amotion,dmg.dmotion, damage, dmg.div_, skillid, -1, 5);
			break;
		case NPC_SELFDESTRUCTION:
		case NPC_SELFDESTRUCTION2:
			dmg.blewcount |= SAB_NODAMAGE;
			break;
		default:
			clif_skill_damage(dsrc,bl,tick,dmg.amotion,dmg.dmotion, damage, dmg.div_, skillid, (lv!=0)?lv:skilllv, (skillid==0)? 5:type );
		}
	} else {	// �_���[�W�������̓p�P�b�g���M���Ȃ�
		damage = 0;
	}

	/* ������΂������Ƃ��̃p�P�b�g */
	if (dmg.blewcount>0 && bl->type!=BL_SKILL && !map[src->m].flag.gvg)
	{
		skill_blown(dsrc,bl,dmg.blewcount);
	}
	/* ������΂������Ƃ��̃p�P�b�g �J�[�h���� */
	if (dsrc->type == BL_PC && bl->type!=BL_SKILL && !map[src->m].flag.gvg)
	{
		skill_add_blown(dsrc,bl,skillid,SAB_REVERSEBLOW);
	}

	map_freeblock_lock();

	/* �N���[���X�L�� */
	if(damage > 0 && dmg.flag&BF_SKILL && tsd && pc_checkskill(tsd,RG_PLAGIARISM) && sc_data && sc_data[SC_PRESERVE].timer == -1){
		skill_clone(tsd,skillid,skilllv);
	}

	/* ���ۂɃ_���[�W�������s�� */
	if (skillid || flag) {
		if (attack_type&BF_WEAPON)
		{
			battle_delay_damage(tick+dmg.amotion,src,bl,damage,dmg.flag);
		}else{
			battle_damage(src,bl,damage,dmg.flag);

			/* �\�E���h���C�� */
			if(sd && bl->type == BL_MOB && unit_isdead(bl) && attack_type&BF_MAGIC)
			{
				int level = pc_checkskill(sd,HW_SOULDRAIN);
				if(level > 0 && skill_get_inf(skillid)==1 && sd && sd->ud.skilltarget == bl->id) {
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

	/* �C���e�B�~�f�C�g */
	if(skillid == RG_INTIMIDATE && damage > 0 && !(status_get_mode(bl)&0x20) && !map[src->m].flag.gvg ) {
		int s_lv = status_get_lv(src),t_lv = status_get_lv(bl);
		int rate = 50 + skilllv * 5;
		rate = rate + (s_lv - t_lv);
		if(atn_rand()%100 < rate)
			skill_addtimerskill(src,tick + 800,bl->id,0,0,skillid,skilllv,0,flag);
	}

	/* �_���[�W������Ȃ�ǉ����ʔ��� */
	if(bl->prev != NULL && !unit_isdead(bl)) {
		if(damage > 0 || skillid==TF_POISON)
			skill_additional_effect(src,bl,skillid,skilllv,attack_type,tick);
		if(bl->type==BL_MOB && src!=bl)	// �X�L���g�p������MOB�X�L��
		{
			struct mob_data *md=(struct mob_data *)bl;
			if(md) {
				int target = md->target_id;
				if(battle_config.mob_changetarget_byskill == 1 || target == 0)
				{
					if(src->type == BL_PC || src->type == BL_HOM)
						md->target_id=src->id;
				}
				mobskill_use(md,tick,MSC_SKILLUSED|(skillid<<16));
				md->target_id=target;
			}
		}
	}

	/* HP,SP�z�� */
	if(sd && dmg.flag&BF_WEAPON && src != bl && src == dsrc && damage > 0) {
		int hp = 0,sp = 0;
		if(sd->hp_drain_rate && sd->hp_drain_per > 0 && dmg.damage > 0 && atn_rand()%100 < sd->hp_drain_rate) {
			hp += (dmg.damage * sd->hp_drain_per)/100;
			if(sd->hp_drain_rate > 0 && hp < 1) hp = 1;
			else if(sd->hp_drain_rate < 0 && hp > -1) hp = -1;
		}
		if(sd->hp_drain_rate_ && sd->hp_drain_per_ > 0 && dmg.damage2 > 0 && atn_rand()%100 < sd->hp_drain_rate_) {
			hp += (dmg.damage2 * sd->hp_drain_per_)/100;
			if(sd->hp_drain_rate_ > 0 && hp < 1) hp = 1;
			else if(sd->hp_drain_rate_ < 0 && hp > -1) hp = -1;
		}
		if(sd->sp_drain_rate > 0 && sd->sp_drain_per > 0 && dmg.damage > 0 && atn_rand()%100 < sd->sp_drain_rate) {
			sp += (dmg.damage * sd->sp_drain_per)/100;
			if(sd->sp_drain_rate > 0 && sp < 1) sp = 1;
			else if(sd->sp_drain_rate < 0 && sp > -1) sp = -1;
		}
		if(sd->sp_drain_rate_ > 0 && sd->sp_drain_per_ > 0 && dmg.damage2 > 0 && atn_rand()%100 < sd->sp_drain_rate_) {
			sp += (dmg.damage2 * sd->sp_drain_per_)/100;
			if(sd->sp_drain_rate_ > 0 && sp < 1) sp = 1;
			else if(sd->sp_drain_rate_ < 0 && sp > -1) sp = -1;
		}
		if(hp || sp) pc_heal(sd,hp,sp);
	}

	/* ���˃_���[�W�̎��ۂ̏��� */
	if (sd && (skillid || flag) && rdamage>0) {
		const long mode = EAS_WEAPON | EAS_ATTACK | EAS_NORMAL;
		if (attack_type&BF_WEAPON)
		{
			battle_delay_damage(tick+dmg.amotion,bl,src,rdamage,0);
			//���˃_���[�W�̃I�[�g�X�y��
			if(battle_config.weapon_reflect_autospell)
			{
				skill_bonus_autospell(bl,src,mode,gettick(),0);
			}

			if(battle_config.weapon_reflect_drain)
				battle_attack_drain(bl,src,rdamage,0,battle_config.weapon_reflect_drain_per_enable);
		}
		else
		{
			battle_damage(bl,src,rdamage,0);
			//���˃_���[�W�̃I�[�g�X�y��
			if(battle_config.magic_reflect_autospell)
			{
				skill_bonus_autospell(bl,src,mode,gettick(),0);
			}
			if(battle_config.magic_reflect_drain)
				battle_attack_drain(bl,src,rdamage,0,battle_config.magic_reflect_drain_per_enable);
		}
	}

	/* �I�[�g�J�E���^�[ */
	if(attack_type&BF_WEAPON && sc_data && sc_data[SC_AUTOCOUNTER].timer != -1 && sc_data[SC_AUTOCOUNTER].val4 > 0) {
		if(sc_data[SC_AUTOCOUNTER].val3 == dsrc->id)
			battle_weapon_attack(bl,dsrc,tick,0x8000|sc_data[SC_AUTOCOUNTER].val1);
		status_change_end(bl,SC_AUTOCOUNTER,-1);
	}
	/* �_�u���L���X�e�B���O */
	if ((skillid == MG_COLDBOLT || skillid == MG_FIREBOLT || skillid == MG_LIGHTNINGBOLT) &&
		ssc_data && ssc_data[SC_DOUBLECASTING].timer != -1 &&
		atn_rand() % 100 < 30+10*ssc_data[SC_DOUBLECASTING].val1) {
		if (!(flag & 1)) {
			//skill_castend_delay (src, bl, skillid, skilllv, tick + dmg.div_*dmg.amotion, flag|1);
			skill_castend_delay (src, bl, skillid, skilllv, tick + 100, flag|1);
		}
	}
	/* �u���b�h���X�g */
	if(src->type == BL_HOM && ssc_data && ssc_data[SC_BLOODLUST].timer!=-1 && dmg.flag&BF_WEAPON && src != bl && src == dsrc && damage > 0)
	{
		struct homun_data *hd = (struct homun_data *)src;
		if(hd && atn_rand()%100 < ssc_data[SC_BLOODLUST].val1*9)
		{
			homun_heal(hd,damage/5,0);
		}
	}

	map_freeblock_unlock();

	return (dmg.damage+dmg.damage2);	/* �^�_����Ԃ� */
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
	if((src=va_arg(ap,struct block_list*)) == NULL)
		return 0;
	if((dsrc=va_arg(ap,struct block_list*)) == NULL)
		return 0;
	skillid=va_arg(ap,int);
	skilllv=va_arg(ap,int);
	tick=va_arg(ap,unsigned int);
	flag=va_arg(ap,int);
	type=va_arg(ap,int);

	if(battle_check_target(dsrc,bl,type) > 0)
		battle_skill_attack(atk_type,src,dsrc,bl,skillid,skilllv,tick,flag);

	return 0;
}

int battle_check_undead(int race,int element)
{
	// element �ɑ����l�{lv(status_get_element �̖߂�l)���n�����~�X��
	// �Ή�����ׁAelement���瑮���^�C�v�����𔲂��o���B
	element %= 10;

	if(battle_config.undead_detect_type == 0) {
		if(element == 9)
			return 1;
	}
	else if(battle_config.undead_detect_type == 1) {
		if(race == 1)
			return 1;
	}
	else {
		if(element == 9 || race == 1)
			return 1;
	}
	return 0;
}

/*==========================================
 * �G��������(1=�m��,0=�ے�,-1=�G���[)
 * flag&0xf0000 = 0x00000:�G����Ȃ�������iret:1���G�ł͂Ȃ��j
 *				= 0x10000:�p�[�e�B�[����iret:1=�p�[�e�B�[�����o)
 *				= 0x20000:�S��(ret:1=�G��������)
 *				= 0x40000:�G������(ret:1=�G)
 *				= 0x50000:�p�[�e�B�[����Ȃ�������(ret:1=�p�[�e�B�łȂ�)
 *------------------------------------------
 */
int battle_check_target( struct block_list *src, struct block_list *target,int flag)
{
	int s_p,s_g,t_p,t_g;
	struct block_list *ss=src;

	nullpo_retr(-1, src);
	nullpo_retr(-1, target);

	if( flag&0x40000 ){	// ���]�t���O
		int ret=battle_check_target(src,target,flag&0x30000);
		if(ret!=-1)
			return !ret;
		return -1;
	}

	if( flag&0x20000 ){
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
		switch(((struct skill_unit *)target)->group->unit_id){
		case 0x8d:
		case 0x8f:
		case 0x98:
			return 0;
			break;
		}
	}

	if(target->type == BL_PET)
		return -1;

	// �X�L�����j�b�g�̏ꍇ�A�e�����߂�
	if( src->type==BL_SKILL) {
		struct skill_unit_group *sg = ((struct skill_unit *)src)->group;
		int inf2;
		if( sg == NULL ) return -1;
		inf2 = skill_get_inf2(sg->skill_id);
		if( (ss=map_id2bl(sg->src_id))==NULL )
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
	if( src->type==BL_MOB ){
		struct mob_data *md=(struct mob_data *)src;
		if(md && md->master_id>0){
			if(md->master_id==target->id)	// ��Ȃ�m��
				return 1;
			if(md->state.special_mob_ai) {
				if(target->type==BL_MOB) {	//special_mob_ai�őΏۂ�Mob
					struct mob_data *tmd=(struct mob_data *)target;
					if(tmd){
						if(tmd->master_id != md->master_id)	//�����傪�ꏏ�łȂ���Δے�
							return 0;
						else if(md->state.special_mob_ai>2)	//�����傪�ꏏ�Ȃ̂ōm�肵�������ǎ����͔ے�
							return 0;
						else
							return 1;
					}
				} else if(target->type==BL_HOM) {	//special_mob_ai�őΏۂ�Hom�Ȃ�G���[�ŕԂ�
					return -1;
				}
			}
			if((ss=map_id2bl(md->master_id))==NULL)
				return -1;
		}
	}

	if( src==target || ss==target )	// �����Ȃ�m��
		return 1;

	if(target->type == BL_PC && pc_isinvisible((struct map_session_data *)target))
		return -1;

	if( src->prev==NULL || unit_isdead(src) ) // ����ł�Ȃ�G���[
		return -1;

	if( (ss->type == BL_PC && target->type==BL_MOB) ||
		(ss->type == BL_MOB && target->type==BL_PC) )
		return 0;	// PCvsMOB�Ȃ�G

	if(ss->type == BL_PET && target->type==BL_MOB) {
		struct pet_data *pd = (struct pet_data*)ss;
		struct mob_data *md = (struct mob_data*)target;
		int mode=mob_db[pd->class].mode;
		int race=mob_db[pd->class].race;
		if(mob_db[pd->class].mexp <= 0 && !(mode&0x20) && (md->option & 0x06 && race != 4 && race != 6) ) {
			return 1; // ���s
		} else {
			return 0; // ����
		}
	}

	s_p=status_get_party_id(ss);
	s_g=status_get_guild_id(ss);

	t_p=status_get_party_id(target);
	t_g=status_get_guild_id(target);

	if(flag&0x10000) {
		if(s_p && t_p && s_p == t_p)	// �����p�[�e�B�Ȃ�m��i�����j
			return 1;
		else		// �p�[�e�B�����Ȃ瓯���p�[�e�B����Ȃ����_�Ŕے�
			return 0;
	}

	if(ss->type == BL_MOB && s_g > 0 && t_g > 0 && s_g == t_g )	// �����M���h/mob�N���X�Ȃ�m��i�����j
		return 1;

	if( ss->type==BL_PC && target->type==BL_PC) { // ����PVP���[�h�Ȃ�ے�i�G�j
		struct skill_unit *su=NULL;
		if(src->type==BL_SKILL)
			su=(struct skill_unit *)src;
		//PK
		if(map[ss->m].flag.pk) {
			struct guild *g=NULL;
			struct map_session_data* ssd = (struct map_session_data*)ss;
			struct map_session_data* tsd = (struct map_session_data*)target;
			struct pc_base_job s_class,t_class;
			s_class = pc_calc_base_job(ssd->status.class);
			t_class = pc_calc_base_job(tsd->status.class);
			//battle_config.no_pk_level�ȉ��@1���͖����@�]���͑ʖ�
			if((ssd->sc_data && ssd->sc_data[SC_PK_PENALTY].timer!=-1) ||
				(ssd->status.base_level <= battle_config.no_pk_level && (s_class.job <=6 || s_class.job==24) && s_class.upper!=1))
				return 1;
			if(tsd->status.base_level <= battle_config.no_pk_level && (t_class.job <=6 || t_class.job==24) && t_class.upper!=1)
				return 1;
			if(su && su->group->target_flag==BCT_NOENEMY)
				return 1;
			if(s_p > 0 && t_p > 0 && s_p == t_p)
				return 1;
			else if(s_g > 0 && t_g > 0 && s_g == t_g)
				return 1;
			if((g = guild_search(s_g)) !=NULL) {
				int i;
				for(i=0;i<MAX_GUILDALLIANCE;i++){
					if(g->alliance[i].guild_id > 0 && g->alliance[i].guild_id == t_g) {
						if(g->alliance[i].opposition)
							return 0;//�G�΃M���h�Ȃ疳�����ɓG
						else
							return 1;//�����M���h�Ȃ疳�����ɖ���
					}
				}
			}
			return 0;
		}
		//PVP
		if(map[ss->m].flag.pvp) {//PVP
			if(su && su->group->target_flag==BCT_NOENEMY)
				return 1;
			if(map[ss->m].flag.pvp_noparty && s_p > 0 && t_p > 0 && s_p == t_p)
				return 1;
			else if(map[ss->m].flag.pvp_noguild && s_g > 0 && t_g > 0 && s_g == t_g)
				return 1;
			return 0;
		}

		if(map[src->m].flag.gvg) {
			struct guild *g=NULL;
			if(su && su->group->target_flag==BCT_NOENEMY)
				return 1;
			if( s_g > 0 && s_g == t_g)
				return 1;
			if(map[src->m].flag.gvg_noparty && s_p > 0 && t_p > 0 && s_p == t_p)
				return 1;
			if((g = guild_search(s_g)) !=NULL) {
				int i;
				for(i=0;i<MAX_GUILDALLIANCE;i++){
					if(g->alliance[i].guild_id > 0 && g->alliance[i].guild_id == t_g) {
						if(g->alliance[i].opposition)
							return 0;//�G�΃M���h�Ȃ疳�����ɓG
						else
							return 1;//�����M���h�Ȃ疳�����ɖ���
					}
				}
			}
			return 0;
		}
	}

	if( (ss->type == BL_HOM && target->type==BL_MOB) ||
		(ss->type == BL_MOB && target->type==BL_HOM) )
		return 0;	// HOMvsMOB�Ȃ�G

	if(!(map[ss->m].flag.pvp || map[ss->m].flag.gvg) &&
		((ss->type == BL_PC && target->type==BL_HOM) ||
		( ss->type == BL_HOM && target->type==BL_PC)))
		return 1;	// Pv�ł�Gv�ł��Ȃ��Ȃ�APCvsHOM�͖���

	// ��PT�Ƃ�����Guild�Ƃ��͌�񂵁i����
	if(ss->type == BL_HOM){
		struct homun_data *hd = (struct homun_data *)ss;
		if(map[ss->m].flag.pvp) {//PVP
			if(target->type==BL_HOM)
				return 0;
			if(target->type==BL_PC){
				struct map_session_data *tsd=(struct map_session_data*)target;
				if(tsd != hd->msd)
					return 0;
			}
		}
		if(map[ss->m].flag.gvg) {//GVG
			if(target->type==BL_HOM)
				return 0;
		}
	}
	if(ss->type == BL_PC && target->type == BL_HOM){
		struct homun_data *hd = (struct homun_data *)target;
		if(map[ss->m].flag.pvp) {//PVP
			if(ss != &hd->msd->bl){
				return 0;
			}
		}
		if(map[ss->m].flag.gvg) {//GVG
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
	int dx,dy;
	int arange;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	dx=abs(bl->x-src->x);
	dy=abs(bl->y-src->y);
	arange=((dx>dy)?dx:dy);

	if(src->m != bl->m)	// �Ⴄ�}�b�v
		return 0;

	if( range>0 && range < arange )	// ��������
		return 0;

	if( arange<2 )	// �����}�X���א�
		return 1;

//	if(bl->type == BL_SKILL && ((struct skill_unit *)bl)->group->unit_id == 0x8d)
//		return 1;

	// ��Q������
	return path_search_long(NULL,src->m,src->x,src->y,bl->x,bl->y);
}

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
 * �ݒ�t�@�C���ǂݍ��ݗp�i�t���O�j
 *------------------------------------------
 */
int battle_config_switch(const char *str)
{
	if(strcmpi(str,"on")==0 || strcmpi(str,"yes")==0)
		return 1;
	if(strcmpi(str,"off")==0 || strcmpi(str,"no")==0)
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
	static int count=0;

	if( (count++)==0 ){
		battle_config.warp_point_debug=0;
		battle_config.enemy_critical=0;
		battle_config.enemy_critical_rate=100;
		battle_config.enemy_str=1;
		battle_config.enemy_perfect_flee=0;
		battle_config.cast_rate=100;
		battle_config.no_cast_dex=150;
		battle_config.delay_rate=100;
		battle_config.delay_dependon_dex=0;
		battle_config.no_delay_dex=150;
		battle_config.sdelay_attack_enable=0;
		battle_config.left_cardfix_to_right=0;
		battle_config.pc_skill_add_range=0;
		battle_config.skill_out_range_consume=1;
		battle_config.mob_skill_add_range=0;
		battle_config.pc_damage_delay=1;
		battle_config.pc_damage_delay_rate=100;
		battle_config.defnotenemy=1;
		battle_config.random_monster_checklv=1;
		battle_config.attr_recover=1;
		battle_config.flooritem_lifetime=LIFETIME_FLOORITEM*1000;
		battle_config.item_auto_get=0;
		battle_config.item_first_get_time=3000;
		battle_config.item_second_get_time=1000;
		battle_config.item_third_get_time=1000;
		battle_config.mvp_item_first_get_time=10000;
		battle_config.mvp_item_second_get_time=10000;
		battle_config.mvp_item_third_get_time=2000;
		battle_config.item_rate=100;
		battle_config.drop_rate0item=0;
		battle_config.base_exp_rate=100;
		battle_config.job_exp_rate=100;
		battle_config.death_penalty_type=0;
		battle_config.death_penalty_base=0;
		battle_config.death_penalty_job=0;
		battle_config.zeny_penalty=0;
		battle_config.zeny_penalty_percent=0;
		battle_config.zeny_penalty_by_lvl=0;
		battle_config.restart_hp_rate=0;
		battle_config.restart_sp_rate=0;
		battle_config.mvp_item_rate=100;
		battle_config.mvp_exp_rate=100;
		battle_config.mvp_hp_rate=100;
		battle_config.monster_hp_rate=100;
		battle_config.monster_max_aspd=199;
		battle_config.atc_gmonly=0;
		battle_config.gm_allskill=0;
		battle_config.gm_allskill_addabra=0;
		battle_config.gm_allequip=0;
		battle_config.gm_skilluncond=0;
		battle_config.skillfree = 0;
		battle_config.skillup_limit = 0;
		battle_config.wp_rate=100;
		battle_config.pp_rate=100;
		battle_config.cdp_rate=100;
		battle_config.monster_active_enable=1;
		battle_config.monster_damage_delay_rate=100;
		battle_config.monster_loot_type=0;
		battle_config.mob_skill_use=1;
		battle_config.mob_count_rate=100;
		battle_config.mob_delay_rate=100;
		battle_config.quest_skill_learn=0;
		battle_config.quest_skill_reset=1;
		battle_config.basic_skill_check=1;
		battle_config.guild_emperium_check=1;
		battle_config.guild_exp_limit=50;
		battle_config.guild_emblem_colors=0;
		battle_config.pc_invincible_time = 5000;
		battle_config.pet_catch_rate=100;
		battle_config.pet_rename=0;
		battle_config.pet_friendly_rate=100;
		battle_config.pet_hungry_delay_rate=100;
		battle_config.pet_hungry_friendly_decrease=5;
		battle_config.pet_str=1;
		battle_config.pet_status_support=0;
		battle_config.pet_attack_support=0;
		battle_config.pet_damage_support=0;
		battle_config.pet_support_rate=100;
		battle_config.pet_attack_exp_to_master=0;
		battle_config.pet_attack_exp_rate=100;
		battle_config.skill_min_damage=0;
		battle_config.finger_offensive_type=0;
		battle_config.heal_exp=0;
		battle_config.resurrection_exp=0;
		battle_config.shop_exp=0;
		battle_config.combo_delay_rate=100;
		battle_config.item_check=1;
		battle_config.wedding_modifydisplay=0;
		battle_config.natural_healhp_interval=6000;
		battle_config.natural_healsp_interval=8000;
		battle_config.natural_heal_skill_interval=10000;
		battle_config.natural_heal_weight_rate=50;
		battle_config.natural_heal_weight_rate_icon=0;
		battle_config.item_name_override_grffile=1;
		battle_config.arrow_decrement=1;
		battle_config.allow_any_weapon_autoblitz=0;
		battle_config.max_aspd = 199;
		battle_config.pk_max_aspd = 199;
		battle_config.gvg_max_aspd = 199;
		battle_config.pvp_max_aspd = 199;
		battle_config.max_hp = 32500;
		battle_config.max_sp = 32500;
		battle_config.max_parameter = 99;
		battle_config.max_cart_weight = 8000;
		battle_config.pc_skill_log = 0;
		battle_config.mob_skill_log = 0;
		battle_config.battle_log = 0;
		battle_config.save_log = 0;
		battle_config.error_log = 1;
		battle_config.etc_log = 1;
		battle_config.save_clothcolor = 0;
		battle_config.undead_detect_type = 0;
		battle_config.pc_auto_counter_type = 1;
		battle_config.monster_auto_counter_type = 1;
		battle_config.min_hitrate = 5;
		battle_config.agi_penaly_type = 0;
		battle_config.agi_penaly_count = 3;
		battle_config.agi_penaly_num = 0;
		battle_config.agi_penaly_count_lv = ATK_FLEE;
		battle_config.vit_penaly_type = 0;
		battle_config.vit_penaly_count = 3;
		battle_config.vit_penaly_num = 0;
		battle_config.vit_penaly_count_lv = ATK_DEF;
		battle_config.player_defense_type = 0;
		battle_config.monster_defense_type = 0;
		battle_config.pet_defense_type = 0;
		battle_config.magic_defense_type = 0;
		battle_config.pc_skill_reiteration = 0;
		battle_config.monster_skill_reiteration = 0;
		battle_config.pc_skill_nofootset = 0;
		battle_config.monster_skill_nofootset = 0;
		battle_config.pc_cloak_check_type = 0;
		battle_config.monster_cloak_check_type = 0;
		battle_config.gvg_short_damage_rate = 100;
		battle_config.gvg_long_damage_rate = 100;
		battle_config.gvg_magic_damage_rate = 100;
		battle_config.gvg_misc_damage_rate = 100;
		battle_config.gvg_eliminate_time = 7000;
		battle_config.mob_changetarget_byskill = 0;
		battle_config.gvg_edp_down_rate = 100;
		battle_config.pvp_edp_down_rate = 100;
		battle_config.pk_edp_down_rate = 100;
		battle_config.pc_attack_direction_change = 1;
		battle_config.monster_attack_direction_change = 1;
		battle_config.pc_land_skill_limit = 1;
		battle_config.monster_land_skill_limit = 1;
		battle_config.party_skill_penaly = 1;
		battle_config.monster_class_change_full_recover = 0;
		battle_config.produce_item_name_input = 1;
		battle_config.produce_potion_name_input = 1;
		battle_config.making_arrow_name_input = 1;
		battle_config.holywater_name_input = 1;
		battle_config.display_delay_skill_fail = 1;
		battle_config.display_snatcher_skill_fail = 1;
		battle_config.chat_warpportal = 0;
		battle_config.mob_warpportal = 0;
		battle_config.dead_branch_active = 0;
		battle_config.vending_max_value = 10000000;
		battle_config.pet_lootitem = 0;
		battle_config.pet_weight = 1000;
		battle_config.show_steal_in_same_party = 0;
		battle_config.enable_upper_class = 0;
		battle_config.pet_attack_attr_none = 0;
		battle_config.pc_attack_attr_none = 0;
		battle_config.mob_attack_attr_none = 1;
		battle_config.gx_allhit = 0;
		battle_config.gx_cardfix = 0;
		battle_config.gx_dupele = 1;
		battle_config.gx_disptype = 1;
		battle_config.devotion_level_difference = 10;
		battle_config.player_skill_partner_check = 1;
		battle_config.sole_concert_type = 3;
		battle_config.hide_GM_session = 0;
		battle_config.unit_movement_type = 0;
		battle_config.invite_request_check = 1;
		battle_config.gvg_trade_request_refused = 1;
		battle_config.pvp_trade_request_refused = 1;
		battle_config.skill_removetrap_type = 0;
		battle_config.disp_experience = 0;
		battle_config.castle_defense_rate = 100;
		battle_config.riding_weight = 0;
		battle_config.hp_rate = 100;
		battle_config.sp_rate = 100;
		battle_config.gm_can_drop_lv = 0;
		battle_config.disp_hpmeter = 0;
		battle_config.bone_drop = 0;
		battle_config.bone_drop_itemid = 7005;
		battle_config.item_rate_details = 0;
		battle_config.item_rate_1 = 100;
		battle_config.item_rate_10 = 100;
		battle_config.item_rate_100 = 100;
		battle_config.item_rate_1000 = 100;
		battle_config.item_rate_1_min = 1;
		battle_config.item_rate_1_max = 9;
		battle_config.item_rate_10_min = 10;
		battle_config.item_rate_10_max = 99;
		battle_config.item_rate_100_min = 100;
		battle_config.item_rate_100_max = 999;
		battle_config.item_rate_1000_min = 1000;
		battle_config.item_rate_1000_max = 10000;
		battle_config.monster_damage_delay = 1;
		battle_config.card_drop_rate = 100;
		battle_config.equip_drop_rate = 100;
		battle_config.consume_drop_rate = 100;
		battle_config.refine_drop_rate = 100;
		battle_config.etc_drop_rate = 100;
		battle_config.potion_drop_rate = 100;
		battle_config.arrow_drop_rate = 100;
		battle_config.petequip_drop_rate = 100;
		battle_config.weapon_drop_rate = 100;
		battle_config.other_drop_rate = 100;
		battle_config.Item_res = 1;
		battle_config.next_exp_limit = 150;
		battle_config.heal_counterstop = 11;
		battle_config.finding_ore_drop_rate = 100;
		battle_config.joint_struggle_exp_bonus = 25;
		battle_config.joint_struggle_limit = 600;
		battle_config.pt_bonus_b = 0;
		battle_config.pt_bonus_j = 0;
		battle_config.mvp_announce = 0;
		battle_config.petowneditem = 0;
		battle_config.buyer_name = 0;
		battle_config.once_autospell = 1;
		battle_config.allow_same_autospell = 0;
		battle_config.combo_delay_lower_limits = 0;
		battle_config.new_marrige_skill = 0;
		battle_config.reveff_plus_addeff = 0;
		battle_config.summonslave_no_drop = 0;
		battle_config.summonslave_no_exp = 0;
		battle_config.summonslave_no_mvp = 0;
		battle_config.summonmonster_no_drop = 0;
		battle_config.summonmonster_no_exp = 0;
		battle_config.summonmonster_no_mvp = 0;
		battle_config.cannibalize_no_drop = 0;
		battle_config.cannibalize_no_exp = 0;
		battle_config.cannibalize_no_mvp = 0;
		battle_config.spheremine_no_drop = 0;
		battle_config.spheremine_no_exp = 0;
		battle_config.spheremine_no_mvp = 0;
		battle_config.branch_mob_no_drop = 0;
		battle_config.branch_mob_no_exp = 0;
		battle_config.branch_mob_no_mvp = 0;
		battle_config.branch_boss_no_drop = 0;
		battle_config.branch_boss_no_exp = 0;
		battle_config.branch_boss_no_mvp = 0;
		battle_config.pc_hit_stop_type = 3;
		battle_config.nomanner_mode = 0;
		battle_config.death_by_unrig_penalty = 0;
		battle_config.dance_and_play_duration = 20000;
		battle_config.soulcollect_max_fail = 0;
		battle_config.gvg_flee_rate	= 100;
		battle_config.gvg_flee_penaly	= 0;
		battle_config.equip_sex = 0;
		battle_config.noportal_flag = 0;
		battle_config.noexp_hiding = 0;
		battle_config.noexp_trickdead = 0;
		battle_config.gm_hide_attack_lv = 1;
		battle_config.hide_attack = 0;
		battle_config.weapon_attack_autospell = 0;
		battle_config.magic_attack_autospell = 0;
		battle_config.misc_attack_autospell = 0;
		battle_config.magic_attack_drain = 0;
		battle_config.misc_attack_drain = 0;
		battle_config.magic_attack_drain_per_enable = 0;
		battle_config.misc_attack_drain_per_enable = 0;
		battle_config.hallucianation_off = 0;
		battle_config.weapon_reflect_autospell = 0;
		battle_config.magic_reflect_autospell = 0;
		battle_config.weapon_reflect_drain = 0;
		battle_config.weapon_reflect_drain_per_enable = 0;
		battle_config.magic_reflect_drain = 0;
		battle_config.magic_reflect_drain_per_enable = 0;
		battle_config.max_parameter_str	= 999;
		battle_config.max_parameter_agi	= 999;
		battle_config.max_parameter_vit	= 999;
		battle_config.max_parameter_int	= 999;
		battle_config.max_parameter_dex	= 999;
		battle_config.max_parameter_luk	= 999;
		battle_config.cannibalize_nocost	= 0;
		battle_config.spheremine_nocost	= 0;
		battle_config.demonstration_nocost	= 0;
		battle_config.acidterror_nocost	= 0;
		battle_config.aciddemonstration_nocost	= 0;
		battle_config.chemical_nocost	= 0;
		battle_config.slimpitcher_nocost	= 0;
		battle_config.mes_send_type = 0;
		battle_config.allow_assumptop_in_gvg = 1;
		battle_config.allow_falconassault_elemet = 0;
		battle_config.allow_guild_invite_in_gvg = 1;
		battle_config.allow_guild_leave_in_gvg  = 1;
		battle_config.guild_skill_available   = 1;
		battle_config.guild_hunting_skill_available = 1;
		battle_config.guild_skill_check_range = 0;
		battle_config.allow_guild_skill_in_gvg_only = 1;
		battle_config.allow_me_guild_skill = 0;
		battle_config.emergencycall_point_randam = 0;
		battle_config.emergencycall_call_limit = 0;
		battle_config.allow_guild_skill_in_gvgtime_only = 0;
		battle_config.guild_skill_in_pvp_limit = 1;
		battle_config.guild_exp_rate = 100;
		battle_config.guild_skill_effective_range = 2;
		battle_config.tarotcard_display_position = 2;
		battle_config.serverside_friendlist = 1;
		battle_config.pet0078_hair_id = 24;
		battle_config.job_soul_check = 1;
		battle_config.repeal_die_counter_rate = 100;
		battle_config.disp_job_soul_state_change = 1;
		battle_config.check_knowlege_map = 0;
		battle_config.tripleattack_rate_up_keeptime = 2000;
		battle_config.tk_counter_rate_up_keeptime = 2000;
		battle_config.allow_skill_without_day = 1;
		battle_config.save_feel_map = 1;
		battle_config.save_hate_mob = 1;
		battle_config.twilight_party_check = 0;
		battle_config.alchemist_point_type = 1;
		battle_config.marionette_type	   = 0;
		battle_config.max_marionette_str	= 99;
		battle_config.max_marionette_agi	= 99;
		battle_config.max_marionette_vit	= 99;
		battle_config.max_marionette_int	= 99;
		battle_config.max_marionette_dex	= 99;
		battle_config.max_marionette_luk	= 99;
		battle_config.max_marionette_pk_str	= 99;
		battle_config.max_marionette_pk_agi	= 99;
		battle_config.max_marionette_pk_vit	= 99;
		battle_config.max_marionette_pk_int	= 99;
		battle_config.max_marionette_pk_dex	= 99;
		battle_config.max_marionette_pk_luk	= 99;
		battle_config.max_marionette_pvp_str	= 99;
		battle_config.max_marionette_pvp_agi	= 99;
		battle_config.max_marionette_pvp_vit	= 99;
		battle_config.max_marionette_pvp_int	= 99;
		battle_config.max_marionette_pvp_dex	= 99;
		battle_config.max_marionette_pvp_luk	= 99;
		battle_config.max_marionette_gvg_str	= 99;
		battle_config.max_marionette_gvg_agi	= 99;
		battle_config.max_marionette_gvg_vit	= 99;
		battle_config.max_marionette_gvg_int	= 99;
		battle_config.max_marionette_gvg_dex	= 99;
		battle_config.max_marionette_gvg_luk	= 99;
		battle_config.baby_status_max  = 80;
		battle_config.baby_hp_rate	   = 70;
		battle_config.baby_sp_rate	   = 70;
		battle_config.upper_hp_rate    =125;
		battle_config.upper_sp_rate    =125;
		battle_config.normal_hp_rate   =100;
		battle_config.normal_sp_rate   =100;
		battle_config.baby_weight_rate = 100;
		battle_config.no_emergency_call = 1;
		battle_config.save_am_pharmacy_success = 0;
		battle_config.save_all_ranking_point_when_logout = 0;
		battle_config.soul_linker_battle_mode 		= 0;
		battle_config.soul_linker_battle_mode_ka 	= 0;
		battle_config.skillup_type 					= 1;
		battle_config.allow_me_dance_effect			= 0;
		battle_config.allow_me_concert_effect		= 0;
		battle_config.allow_me_rokisweil			= 0;
		battle_config.pharmacy_get_point_type		= 0;
		battle_config.soulskill_can_be_used_for_myself = 1;
		battle_config.hermode_wp_check_range = 3;
		battle_config.hermode_wp_check = 1;
		battle_config.hermode_no_walking = 0;
		battle_config.hermode_gvg_only = 1;
		battle_config.atcommand_go_significant_values	= 21;
		battle_config.redemptio_penalty_type	=	1;
		battle_config.allow_weaponrearch_to_weaponrefine = 0;
		battle_config.boss_no_knockbacking = 0;
		battle_config.boss_no_element_change = 1;
		battle_config.scroll_produce_rate = 100;
		battle_config.scroll_item_name_input = 0;
		battle_config.pet_leave = 0;
		battle_config.pk_short_damage_rate = 100;
		battle_config.pk_long_damage_rate = 100;
		battle_config.pk_magic_damage_rate = 100;
		battle_config.pk_misc_damage_rate = 100;
		battle_config.cooking_rate = 100;
		battle_config.making_rate = 100;
		battle_config.extended_abracadabra = 0;
		battle_config.no_pk_level		   = 60;
		battle_config.allow_cloneskill_at_autospell = 0;
		battle_config.pk_noshift = 0;
		battle_config.pk_penalty_time = 60000;
		battle_config.dropitem_itemrate_fix = 0;
		battle_config.gm_nomanner_lv = 50;
		battle_config.clif_fixpos_type = 1;
		battle_config.romail = 0;
		battle_config.pc_die_script = 0;
		battle_config.pc_kill_script = 0;
		battle_config.pc_movemap_script = 0;
		battle_config.pc_login_script = 0;
		battle_config.pc_logout_script = 0;
		battle_config.set_pckillerid = 0;
		battle_config.def_ratio_atk_to_shieldchain = 0;
		battle_config.def_ratio_atk_to_carttermination = 0;
		battle_config.player_gravitation_type = 0;
		battle_config.enemy_gravitation_type = 0;
		battle_config.mob_attack_fixwalkpos = 0;
		battle_config.mob_ai_limiter = 0;
		battle_config.mob_ai_cpu_usage = 80;
		battle_config.itemidentify = 0;
		battle_config.casting_penalty_type = 0;
		battle_config.casting_penalty_weapon = 0;
		battle_config.casting_penalty_shield = 0;
		battle_config.casting_penalty_armor = 0;
		battle_config.casting_penalty_helm = 0;
		battle_config.casting_penalty_robe = 0;
		battle_config.casting_penalty_shoes = 0;
		battle_config.casting_penalty_acce = 0;
		battle_config.casting_penalty_arrow = 0;
		battle_config.show_always_party_name = 0;
		battle_config.check_player_name_global_msg = 0;
		battle_config.check_player_name_party_msg = 0;
		battle_config.check_player_name_guild_msg = 0;
		battle_config.save_player_when_drop_item = 0;
		battle_config.allow_homun_status_change = 1;
		battle_config.save_homun_temporal_intimate = 1;
		battle_config.homun_intimate_rate = 100;
		battle_config.homun_temporal_intimate_resilience = 50;
		battle_config.hvan_explosion_intimate   = 45000;
		battle_config.homun_speed_is_same_as_pc = 1;
		battle_config.homun_skill_intimate_type = 0;
		battle_config.master_get_homun_base_exp = 0;
		battle_config.master_get_homun_job_exp = 0;
		battle_config.extra_system_flag = 1;
		battle_config.mob_take_over_sp = 0;
		battle_config.party_join_limit = 1;
		battle_config.check_skillpos_range = 0;
		battle_config.pet_speed_is_same_as_pc = 1;
		battle_config.tax_rate = 0;
		battle_config.steal_rate = 100;
		battle_config.sw_def_type = 0;
		battle_config.calc_dist_flag = 0;
		battle_config.allow_sw_dist = 4;
		battle_config.storagesort_by_itemid = 3;
		battle_config.cancel_race = 1;
		battle_config.allow_es_magic_all = 0;
	}

	fp=fopen(cfgName,"r");
	if(fp==NULL){
		printf("file not found: %s\n",cfgName);
		return 1;
	}
	while(fgets(line,1020,fp)){
		static const struct {
			char *str;
			int  *val;
		} data[] ={
			{ "warp_point_debug",					&battle_config.warp_point_debug						},
			{ "enemy_critical",						&battle_config.enemy_critical						},
			{ "enemy_critical_rate",				&battle_config.enemy_critical_rate					},
			{ "enemy_str",							&battle_config.enemy_str							},
			{ "enemy_perfect_flee",					&battle_config.enemy_perfect_flee					},
			{ "casting_rate",						&battle_config.cast_rate							},
			{ "no_casting_dex",						&battle_config.no_cast_dex							},
			{ "delay_rate",							&battle_config.delay_rate							},
			{ "delay_dependon_dex",					&battle_config.delay_dependon_dex					},
			{ "no_delay_dex",						&battle_config.no_delay_dex							},
			{ "skill_delay_attack_enable",			&battle_config.sdelay_attack_enable 				},
			{ "left_cardfix_to_right",				&battle_config.left_cardfix_to_right				},
			{ "player_skill_add_range",				&battle_config.pc_skill_add_range					},
			{ "skill_out_range_consume",			&battle_config.skill_out_range_consume				},
			{ "monster_skill_add_range",			&battle_config.mob_skill_add_range					},
			{ "player_damage_delay",				&battle_config.pc_damage_delay						},
			{ "player_damage_delay_rate",			&battle_config.pc_damage_delay_rate					},
			{ "defunit_not_enemy",					&battle_config.defnotenemy							},
			{ "random_monster_checklv",				&battle_config.random_monster_checklv				},
			{ "attribute_recover",					&battle_config.attr_recover							},
			{ "flooritem_lifetime",					&battle_config.flooritem_lifetime					},
			{ "item_auto_get",						&battle_config.item_auto_get						},
			{ "item_first_get_time",				&battle_config.item_first_get_time					},
			{ "item_second_get_time",				&battle_config.item_second_get_time					},
			{ "item_third_get_time",				&battle_config.item_third_get_time					},
			{ "mvp_item_first_get_time",			&battle_config.mvp_item_first_get_time				},
			{ "mvp_item_second_get_time",			&battle_config.mvp_item_second_get_time				},
			{ "mvp_item_third_get_time",			&battle_config.mvp_item_third_get_time				},
			{ "item_rate",							&battle_config.item_rate							},
			{ "drop_rate0item",						&battle_config.drop_rate0item						},
			{ "base_exp_rate",						&battle_config.base_exp_rate						},
			{ "job_exp_rate",						&battle_config.job_exp_rate							},
			{ "death_penalty_type",					&battle_config.death_penalty_type					},
			{ "death_penalty_base",					&battle_config.death_penalty_base					},
			{ "death_penalty_job",					&battle_config.death_penalty_job					},
			{ "zeny_penalty",						&battle_config.zeny_penalty							},
			{ "zeny_penalty_percent",				&battle_config.zeny_penalty_percent					},
			{ "zeny_penalty_by_lvl",				&battle_config.zeny_penalty_by_lvl					},
			{ "restart_hp_rate",					&battle_config.restart_hp_rate						},
			{ "restart_sp_rate",					&battle_config.restart_sp_rate						},
			{ "mvp_hp_rate",						&battle_config.mvp_hp_rate							},
			{ "mvp_item_rate",						&battle_config.mvp_item_rate						},
			{ "mvp_exp_rate",						&battle_config.mvp_exp_rate							},
			{ "monster_hp_rate",					&battle_config.monster_hp_rate						},
			{ "monster_max_aspd",					&battle_config.monster_max_aspd						},
			{ "atcommand_gm_only",					&battle_config.atc_gmonly							},
			{ "gm_all_skill",						&battle_config.gm_allskill							},
			{ "gm_all_skill_add_abra",				&battle_config.gm_allskill_addabra					},
			{ "gm_all_equipment",					&battle_config.gm_allequip							},
			{ "gm_skill_unconditional",				&battle_config.gm_skilluncond						},
			{ "player_skillfree",					&battle_config.skillfree							},
			{ "player_skillup_limit",				&battle_config.skillup_limit						},
			{ "weapon_produce_rate",				&battle_config.wp_rate								},
			{ "potion_produce_rate",				&battle_config.pp_rate								},
			{ "deadly_potion_produce_rate",			&battle_config.cdp_rate								},
			{ "monster_active_enable",				&battle_config.monster_active_enable				},
			{ "monster_damage_delay_rate",			&battle_config.monster_damage_delay_rate			},
			{ "monster_loot_type",					&battle_config.monster_loot_type					},
			{ "mob_skill_use",						&battle_config.mob_skill_use						},
			{ "mob_count_rate",						&battle_config.mob_count_rate						},
			{ "mob_delay_rate",						&battle_config.mob_delay_rate						},
			{ "quest_skill_learn",					&battle_config.quest_skill_learn					},
			{ "quest_skill_reset",					&battle_config.quest_skill_reset					},
			{ "basic_skill_check",					&battle_config.basic_skill_check					},
			{ "guild_emperium_check",				&battle_config.guild_emperium_check					},
			{ "guild_emblem_colors",				&battle_config.guild_emblem_colors					},
			{ "guild_exp_limit",					&battle_config.guild_exp_limit						},
			{ "player_invincible_time" ,			&battle_config.pc_invincible_time					},
			{ "pet_catch_rate",						&battle_config.pet_catch_rate						},
			{ "pet_rename",							&battle_config.pet_rename							},
			{ "pet_friendly_rate",					&battle_config.pet_friendly_rate					},
			{ "pet_hungry_delay_rate",				&battle_config.pet_hungry_delay_rate				},
			{ "pet_hungry_friendly_decrease",		&battle_config.pet_hungry_friendly_decrease			},
			{ "pet_str",							&battle_config.pet_str								},
			{ "pet_status_support",					&battle_config.pet_status_support					},
			{ "pet_attack_support",					&battle_config.pet_attack_support					},
			{ "pet_damage_support",					&battle_config.pet_damage_support					},
			{ "pet_support_rate",					&battle_config.pet_support_rate						},
			{ "pet_attack_exp_to_master",			&battle_config.pet_attack_exp_to_master				},
			{ "pet_attack_exp_rate",				&battle_config.pet_attack_exp_rate					},
			{ "skill_min_damage",					&battle_config.skill_min_damage						},
			{ "finger_offensive_type",				&battle_config.finger_offensive_type				},
			{ "heal_exp",							&battle_config.heal_exp								},
			{ "resurrection_exp",					&battle_config.resurrection_exp						},
			{ "shop_exp",							&battle_config.shop_exp								},
			{ "combo_delay_rate",					&battle_config.combo_delay_rate						},
			{ "item_check",							&battle_config.item_check							},
			{ "wedding_modifydisplay",				&battle_config.wedding_modifydisplay				},
			{ "natural_healhp_interval",			&battle_config.natural_healhp_interval				},
			{ "natural_healsp_interval",			&battle_config.natural_healsp_interval				},
			{ "natural_heal_skill_interval",		&battle_config.natural_heal_skill_interval			},
			{ "natural_heal_weight_rate",			&battle_config.natural_heal_weight_rate				},
			{ "natural_heal_weight_rate_icon",		&battle_config.natural_heal_weight_rate_icon		},
			{ "item_name_override_grffile",			&battle_config.item_name_override_grffile			},
			{ "arrow_decrement",					&battle_config.arrow_decrement						},
			{ "allow_any_weapon_autoblitz",			&battle_config.allow_any_weapon_autoblitz			},
			{ "max_aspd",							&battle_config.max_aspd								},
			{ "pk_max_aspd",						&battle_config.pk_max_aspd							},
			{ "gvg_max_aspd",						&battle_config.gvg_max_aspd							},
			{ "pvp_max_aspd",						&battle_config.pvp_max_aspd							},
			{ "max_hp",								&battle_config.max_hp								},
			{ "max_sp",								&battle_config.max_sp								},
			{ "max_parameter", 						&battle_config.max_parameter						},
			{ "max_cart_weight",					&battle_config.max_cart_weight						},
			{ "player_skill_log",					&battle_config.pc_skill_log							},
			{ "monster_skill_log",					&battle_config.mob_skill_log						},
			{ "battle_log",							&battle_config.battle_log							},
			{ "save_log",							&battle_config.save_log								},
			{ "error_log",							&battle_config.error_log							},
			{ "etc_log",							&battle_config.etc_log								},
			{ "save_clothcolor",					&battle_config.save_clothcolor						},
			{ "undead_detect_type",					&battle_config.undead_detect_type					},
			{ "player_auto_counter_type",			&battle_config.pc_auto_counter_type					},
			{ "monster_auto_counter_type",			&battle_config.monster_auto_counter_type			},
			{ "min_hitrate",						&battle_config.min_hitrate							},
			{ "agi_penaly_type",					&battle_config.agi_penaly_type						},
			{ "agi_penaly_count",					&battle_config.agi_penaly_count						},
			{ "agi_penaly_num",						&battle_config.agi_penaly_num						},
			{ "agi_penaly_count_lv",				&battle_config.agi_penaly_count_lv					},
			{ "vit_penaly_type",					&battle_config.vit_penaly_type						},
			{ "vit_penaly_count",					&battle_config.vit_penaly_count						},
			{ "vit_penaly_num",						&battle_config.vit_penaly_num						},
			{ "vit_penaly_count_lv",				&battle_config.vit_penaly_count_lv					},
			{ "player_defense_type",				&battle_config.player_defense_type					},
			{ "monster_defense_type",				&battle_config.monster_defense_type					},
			{ "pet_defense_type",					&battle_config.pet_defense_type						},
			{ "magic_defense_type",					&battle_config.magic_defense_type					},
			{ "player_skill_reiteration",			&battle_config.pc_skill_reiteration					},
			{ "monster_skill_reiteration",			&battle_config.monster_skill_reiteration			},
			{ "player_skill_nofootset",				&battle_config.pc_skill_nofootset					},
			{ "monster_skill_nofootset",			&battle_config.monster_skill_nofootset				},
			{ "player_cloak_check_type",			&battle_config.pc_cloak_check_type					},
			{ "monster_cloak_check_type",			&battle_config.monster_cloak_check_type				},
			{ "gvg_short_attack_damage_rate",		&battle_config.gvg_short_damage_rate				},
			{ "gvg_long_attack_damage_rate",		&battle_config.gvg_long_damage_rate					},
			{ "gvg_magic_attack_damage_rate",		&battle_config.gvg_magic_damage_rate				},
			{ "gvg_misc_attack_damage_rate",		&battle_config.gvg_misc_damage_rate					},
			{ "gvg_eliminate_time",					&battle_config.gvg_eliminate_time					},
			{ "mob_changetarget_byskill",			&battle_config.mob_changetarget_byskill				},
			{ "gvg_edp_down_rate",					&battle_config.gvg_edp_down_rate					},
			{ "pvp_edp_down_rate",					&battle_config.pvp_edp_down_rate					},
			{ "pk_edp_down_rate",					&battle_config.pk_edp_down_rate						},
			{ "player_attack_direction_change",		&battle_config.pc_attack_direction_change			},
			{ "monster_attack_direction_change",	&battle_config.monster_attack_direction_change		},
			{ "player_land_skill_limit",			&battle_config.pc_land_skill_limit					},
			{ "monster_land_skill_limit",			&battle_config.monster_land_skill_limit				},
			{ "party_skill_penaly",					&battle_config.party_skill_penaly					},
			{ "monster_class_change_full_recover",	&battle_config.monster_class_change_full_recover	},
			{ "produce_item_name_input",			&battle_config.produce_item_name_input				},
			{ "produce_potion_name_input",			&battle_config.produce_potion_name_input			},
			{ "making_arrow_name_input",			&battle_config.making_arrow_name_input				},
			{ "holywater_name_input",				&battle_config.holywater_name_input					},
			{ "display_delay_skill_fail",			&battle_config.display_delay_skill_fail				},
			{ "display_snatcher_skill_fail",		&battle_config.display_snatcher_skill_fail			},
			{ "chat_warpportal",					&battle_config.chat_warpportal						},
			{ "mob_warpportal",						&battle_config.mob_warpportal						},
			{ "dead_branch_active",					&battle_config.dead_branch_active					},
			{ "vending_max_value",					&battle_config.vending_max_value					},
			{ "pet_lootitem",						&battle_config.pet_lootitem							},
			{ "pet_weight",							&battle_config.pet_weight							},
			{ "show_steal_in_same_party",			&battle_config.show_steal_in_same_party				},
			{ "enable_upper_class", 				&battle_config.enable_upper_class					},
			{ "pet_attack_attr_none", 				&battle_config.pet_attack_attr_none					},
			{ "mob_attack_attr_none", 				&battle_config.mob_attack_attr_none					},
			{ "pc_attack_attr_none", 				&battle_config.pc_attack_attr_none					},
			{ "gx_allhit", 							&battle_config.gx_allhit							},
			{ "gx_cardfix",							&battle_config.gx_cardfix							},
			{ "gx_dupele", 							&battle_config.gx_dupele							},
			{ "gx_disptype", 						&battle_config.gx_disptype							},
			{ "devotion_level_difference",			&battle_config.devotion_level_difference			},
			{ "player_skill_partner_check",			&battle_config.player_skill_partner_check			},
			{ "sole_concert_type",					&battle_config.sole_concert_type					},
			{ "hide_GM_session",					&battle_config.hide_GM_session						},
			{ "unit_movement_type",					&battle_config.unit_movement_type					},
			{ "invite_request_check",				&battle_config.invite_request_check					},
			{ "gvg_trade_request_refused",			&battle_config.gvg_trade_request_refused			},
			{ "pvp_trade_request_refused",			&battle_config.pvp_trade_request_refused			},
			{ "skill_removetrap_type",				&battle_config.skill_removetrap_type				},
			{ "disp_experience",					&battle_config.disp_experience						},
			{ "castle_defense_rate",				&battle_config.castle_defense_rate					},
			{ "riding_weight",						&battle_config.riding_weight						},
			{ "hp_rate",							&battle_config.hp_rate								},
			{ "sp_rate",							&battle_config.sp_rate								},
			{ "gm_can_drop_lv",						&battle_config.gm_can_drop_lv						},
			{ "disp_hpmeter",						&battle_config.disp_hpmeter							},
			{ "bone_drop",							&battle_config.bone_drop							},
			{ "bone_drop_itemid",					&battle_config.bone_drop_itemid						},
			{ "item_rate_details",					&battle_config.item_rate_details					},
			{ "item_rate_1",						&battle_config.item_rate_1							},
			{ "item_rate_10",						&battle_config.item_rate_10							},
			{ "item_rate_100",						&battle_config.item_rate_100						},
			{ "item_rate_1000",						&battle_config.item_rate_1000						},
			{ "item_rate_1_min",					&battle_config.item_rate_1_min						},
			{ "item_rate_1_max",					&battle_config.item_rate_1_max						},
			{ "item_rate_10_min",					&battle_config.item_rate_10_min						},
			{ "item_rate_10_max",					&battle_config.item_rate_10_max						},
			{ "item_rate_100_min",					&battle_config.item_rate_100_min					},
			{ "item_rate_100_max",					&battle_config.item_rate_100_max					},
			{ "item_rate_1000_min",					&battle_config.item_rate_1000_min					},
			{ "item_rate_1000_max",					&battle_config.item_rate_1000_max					},
			{ "monster_damage_delay",				&battle_config.monster_damage_delay					},
			{ "card_drop_rate",						&battle_config.card_drop_rate						},
			{ "equip_drop_rate",					&battle_config.equip_drop_rate						},
			{ "consume_drop_rate",					&battle_config.consume_drop_rate					},
			{ "refine_drop_rate",					&battle_config.refine_drop_rate						},
			{ "etc_drop_rate",						&battle_config.etc_drop_rate						},
			{ "potion_drop_rate",					&battle_config.potion_drop_rate						},
			{ "arrow_drop_rate",					&battle_config.arrow_drop_rate						},
			{ "petequip_drop_rate",					&battle_config.petequip_drop_rate					},
			{ "weapon_drop_rate",					&battle_config.weapon_drop_rate						},
			{ "other_drop_rate",					&battle_config.other_drop_rate						},
			{ "Item_res",							&battle_config.Item_res								},
			{ "next_exp_limit",						&battle_config.next_exp_limit						},
			{ "heal_counterstop",					&battle_config.heal_counterstop						},
			{ "finding_ore_drop_rate",				&battle_config.finding_ore_drop_rate				},
			{ "joint_struggle_exp_bonus",			&battle_config.joint_struggle_exp_bonus				},
			{ "joint_struggle_limit",				&battle_config.joint_struggle_limit					},
			{ "pt_bonus_b",							&battle_config.pt_bonus_b							},
			{ "pt_bonus_j",							&battle_config.pt_bonus_j							},
			{ "mvp_announce",						&battle_config.mvp_announce							},
			{ "petowneditem",						&battle_config.petowneditem							},
			{ "buyer_name",							&battle_config.buyer_name							},
			{ "noportal_flag",						&battle_config.noportal_flag						},
			{ "once_autospell",						&battle_config.once_autospell						},
			{ "allow_same_autospell",				&battle_config.allow_same_autospell					},
			{ "combo_delay_lower_limits",			&battle_config.combo_delay_lower_limits				},
			{ "new_marrige_skill",					&battle_config.new_marrige_skill					},
			{ "reveff_plus_addeff",					&battle_config.reveff_plus_addeff					},
			{ "summonslave_no_drop",				&battle_config.summonslave_no_drop					},
			{ "summonslave_no_exp",					&battle_config.summonslave_no_exp					},
			{ "summonslave_no_mvp",					&battle_config.summonslave_no_mvp					},
			{ "summonmonster_no_drop",				&battle_config.summonmonster_no_drop				},
			{ "summonmonster_no_exp",				&battle_config.summonmonster_no_exp					},
			{ "summonmonster_no_mvp",				&battle_config.summonmonster_no_mvp					},
			{ "cannibalize_no_drop",				&battle_config.cannibalize_no_drop					},
			{ "cannibalize_no_exp",					&battle_config.cannibalize_no_exp					},
			{ "cannibalize_no_mvp",					&battle_config.cannibalize_no_mvp					},
			{ "spheremine_no_drop",					&battle_config.spheremine_no_drop					},
			{ "spheremine_no_exp",					&battle_config.spheremine_no_exp					},
			{ "spheremine_no_mvp",					&battle_config.spheremine_no_mvp					},
			{ "branch_mob_no_drop",					&battle_config.branch_mob_no_drop					},
			{ "branch_mob_no_exp",					&battle_config.branch_mob_no_exp					},
			{ "branch_mob_no_mvp",					&battle_config.branch_mob_no_mvp					},
			{ "branch_boss_no_drop",				&battle_config.branch_boss_no_drop					},
			{ "branch_boss_no_exp",					&battle_config.branch_boss_no_exp					},
			{ "branch_boss_no_mvp",					&battle_config.branch_boss_no_mvp					},
			{ "pc_hit_stop_type",					&battle_config.pc_hit_stop_type						},
			{ "nomanner_mode",						&battle_config.nomanner_mode						},
			{ "death_by_unrig_penalty",				&battle_config.death_by_unrig_penalty				},
			{ "dance_and_play_duration",			&battle_config.dance_and_play_duration				},
			{ "soulcollect_max_fail",				&battle_config.soulcollect_max_fail					},
			{ "gvg_flee_rate",						&battle_config.gvg_flee_rate						},
			{ "gvg_flee_penaly",					&battle_config.gvg_flee_penaly						},
			{ "equip_sex",							&battle_config.equip_sex							},
			{ "noexp_hiding",						&battle_config.noexp_hiding							},
			{ "noexp_trickdead",					&battle_config.noexp_trickdead						},
			{ "hide_attack",						&battle_config.hide_attack							},
			{ "gm_hide_attack_lv",					&battle_config.gm_hide_attack_lv					},
			{ "weapon_attack_autospell",			&battle_config.weapon_attack_autospell				},
			{ "magic_attack_autospell",				&battle_config.magic_attack_autospell				},
			{ "misc_attack_autospell",				&battle_config.misc_attack_autospell				},
			{ "magic_attack_drain",					&battle_config.magic_attack_drain					},
			{ "misc_attack_drain",					&battle_config.misc_attack_drain					},
			{ "magic_attack_drain_per_enable",		&battle_config.magic_attack_drain_per_enable		},
			{ "misc_attack_drain_per_enable",		&battle_config.misc_attack_drain_per_enable			},
			{ "hallucianation_off",					&battle_config.hallucianation_off					},
			{ "weapon_reflect_autospell",			&battle_config.weapon_reflect_autospell				},
			{ "magic_reflect_autospell",			&battle_config.magic_reflect_autospell				},
			{ "weapon_reflect_drain",				&battle_config.weapon_reflect_drain					},
			{ "weapon_reflect_drain_per_enable",	&battle_config.weapon_reflect_drain_per_enable		},
			{ "magic_reflect_drain",				&battle_config.magic_reflect_drain					},
			{ "magic_reflect_drain_per_enable",		&battle_config.magic_reflect_drain_per_enable		},
			{ "max_parameter_str",					&battle_config.max_parameter_str					},
			{ "max_parameter_agi",					&battle_config.max_parameter_agi					},
			{ "max_parameter_vit",					&battle_config.max_parameter_vit					},
			{ "max_parameter_int",					&battle_config.max_parameter_int					},
			{ "max_parameter_dex",					&battle_config.max_parameter_dex					},
			{ "max_parameter_luk",					&battle_config.max_parameter_luk					},
			{ "cannibalize_nocost",					&battle_config.cannibalize_nocost					},
			{ "spheremine_nocost",					&battle_config.spheremine_nocost					},
			{ "demonstration_nocost",				&battle_config.demonstration_nocost					},
			{ "acidterror_nocost",					&battle_config.acidterror_nocost					},
			{ "aciddemonstration_nocost",			&battle_config.aciddemonstration_nocost				},
			{ "chemical_nocost",					&battle_config.chemical_nocost						},
			{ "slimpitcher_nocost",					&battle_config.slimpitcher_nocost					},
			{ "mes_send_type",						&battle_config.mes_send_type						},
			{ "allow_assumptop_in_gvg",				&battle_config.allow_assumptop_in_gvg				},
			{ "allow_falconassault_elemet",			&battle_config.allow_falconassault_elemet			},
			{ "allow_guild_invite_in_gvg",			&battle_config.allow_guild_invite_in_gvg			},
			{ "allow_guild_leave_in_gvg",			&battle_config.allow_guild_leave_in_gvg				},
			{ "guild_skill_available",				&battle_config.guild_skill_available				},
			{ "guild_hunting_skill_available",		&battle_config.guild_hunting_skill_available		},
			{ "guild_skill_check_range",			&battle_config.guild_skill_check_range				},
			{ "allow_guild_skill_in_gvg_only",		&battle_config.allow_guild_skill_in_gvg_only		},
			{ "allow_me_guild_skill",				&battle_config.allow_me_guild_skill					},
			{ "emergencycall_point_randam",			&battle_config.emergencycall_point_randam			},
			{ "emergencycall_call_limit",			&battle_config.emergencycall_call_limit				},
			{ "allow_guild_skill_in_gvgtime_only",	&battle_config.allow_guild_skill_in_gvgtime_only	},
			{ "guild_skill_in_pvp_limit",			&battle_config.guild_skill_in_pvp_limit				},
			{ "guild_exp_rate",						&battle_config.guild_exp_rate						},
			{ "guild_skill_effective_range",		&battle_config.guild_skill_effective_range			},
			{ "tarotcard_display_position",			&battle_config.tarotcard_display_position			},
			{ "serverside_friendlist",				&battle_config.serverside_friendlist				},
			{ "pet0078_hair_id",					&battle_config.pet0078_hair_id						},
			{ "job_soul_check",						&battle_config.job_soul_check						},
			{ "repeal_die_counter_rate",			&battle_config.repeal_die_counter_rate				},
			{ "disp_job_soul_state_change",			&battle_config.disp_job_soul_state_change			},
			{ "check_knowlege_map",					&battle_config.check_knowlege_map					},
			{ "tripleattack_rate_up_keeptime",		&battle_config.tripleattack_rate_up_keeptime		},
			{ "tk_counter_rate_up_keeptime",		&battle_config.tk_counter_rate_up_keeptime			},
			{ "allow_skill_without_day",			&battle_config.allow_skill_without_day				},
			{ "save_feel_map",						&battle_config.save_feel_map						},
			{ "save_hate_mob",						&battle_config.save_hate_mob						},
			{ "twilight_party_check",				&battle_config.twilight_party_check					},
			{ "alchemist_point_type",				&battle_config.alchemist_point_type					},
			{ "marionette_type",					&battle_config.marionette_type						},
			{ "max_marionette_str",					&battle_config.max_marionette_str					},
			{ "max_marionette_agi",					&battle_config.max_marionette_agi					},
			{ "max_marionette_vit",					&battle_config.max_marionette_vit					},
			{ "max_marionette_int",					&battle_config.max_marionette_int					},
			{ "max_marionette_dex",					&battle_config.max_marionette_dex					},
			{ "max_marionette_luk",					&battle_config.max_marionette_luk					},
			{ "max_marionette_pk_str",				&battle_config.max_marionette_pk_str				},
			{ "max_marionette_pk_agi",				&battle_config.max_marionette_pk_agi				},
			{ "max_marionette_pk_vit",				&battle_config.max_marionette_pk_vit				},
			{ "max_marionette_pk_int",				&battle_config.max_marionette_pk_int				},
			{ "max_marionette_pk_dex",				&battle_config.max_marionette_pk_dex				},
			{ "max_marionette_pk_luk",				&battle_config.max_marionette_pk_luk				},
			{ "max_marionette_pvp_str",				&battle_config.max_marionette_pvp_str				},
			{ "max_marionette_pvp_agi",				&battle_config.max_marionette_pvp_agi				},
			{ "max_marionette_pvp_vit",				&battle_config.max_marionette_pvp_vit				},
			{ "max_marionette_pvp_int",				&battle_config.max_marionette_pvp_int				},
			{ "max_marionette_pvp_dex",				&battle_config.max_marionette_pvp_dex				},
			{ "max_marionette_pvp_luk",				&battle_config.max_marionette_pvp_luk				},
			{ "max_marionette_gvg_str",				&battle_config.max_marionette_gvg_str				},
			{ "max_marionette_gvg_agi",				&battle_config.max_marionette_gvg_agi				},
			{ "max_marionette_gvg_vit",				&battle_config.max_marionette_gvg_vit				},
			{ "max_marionette_gvg_int",				&battle_config.max_marionette_gvg_int				},
			{ "max_marionette_gvg_dex",				&battle_config.max_marionette_gvg_dex				},
			{ "max_marionette_gvg_luk",				&battle_config.max_marionette_gvg_luk				},
			{ "baby_status_max",					&battle_config.baby_status_max						},
			{ "baby_hp_rate",						&battle_config.baby_hp_rate							},
			{ "baby_sp_rate",						&battle_config.baby_sp_rate							},
			{ "upper_hp_rate",						&battle_config.upper_hp_rate						},
			{ "upper_sp_rate",						&battle_config.upper_sp_rate						},
			{ "normal_hp_rate",						&battle_config.normal_hp_rate						},
			{ "normal_sp_rate",						&battle_config.normal_sp_rate						},
			{ "baby_weight_rate",					&battle_config.baby_weight_rate						},
			{ "no_emergency_call",					&battle_config.no_emergency_call					},
			{ "save_am_pharmacy_success",			&battle_config.save_am_pharmacy_success				},
			{ "save_all_ranking_point_when_logout",	&battle_config.save_all_ranking_point_when_logout	},
			{ "soul_linker_battle_mode",			&battle_config.soul_linker_battle_mode				},
			{ "soul_linker_battle_mode_ka",			&battle_config.soul_linker_battle_mode_ka			},
			{ "skillup_type",						&battle_config.skillup_type							},
			{ "allow_me_dance_effect",				&battle_config.allow_me_dance_effect				},
			{ "allow_me_concert_effect",			&battle_config.allow_me_concert_effect				},
			{ "allow_me_rokisweil",					&battle_config.allow_me_rokisweil					},
			{ "pharmacy_get_point_type",			&battle_config.pharmacy_get_point_type				},
			{ "soulskill_can_be_used_for_myself",	&battle_config.soulskill_can_be_used_for_myself		},
			{ "hermode_wp_check_range",				&battle_config.hermode_wp_check_range				},
			{ "hermode_wp_check",					&battle_config.hermode_wp_check						},
			{ "hermode_no_walking",					&battle_config.hermode_no_walking					},
			{ "hermode_gvg_only",					&battle_config.hermode_gvg_only						},
			{ "atcommand_go_significant_values",	&battle_config.atcommand_go_significant_values		},
			{ "redemptio_penalty_type",				&battle_config.redemptio_penalty_type				},
			{ "allow_weaponrearch_to_weaponrefine",	&battle_config.allow_weaponrearch_to_weaponrefine	},
			{ "boss_no_knockbacking",				&battle_config.boss_no_knockbacking					},
			{ "boss_no_element_change",				&battle_config.boss_no_element_change				},
			{ "scroll_produce_rate",				&battle_config.scroll_produce_rate					},
			{ "scroll_item_name_input",				&battle_config.scroll_item_name_input				},
			{ "pet_leave",							&battle_config.pet_leave							},
			{ "pk_short_attack_damage_rate",		&battle_config.pk_short_damage_rate					},
			{ "pk_long_attack_damage_rate",			&battle_config.pk_long_damage_rate					},
			{ "pk_magic_attack_damage_rate",		&battle_config.pk_magic_damage_rate					},
			{ "pk_misc_attack_damage_rate",			&battle_config.pk_misc_damage_rate					},
			{ "cooking_rate",						&battle_config.cooking_rate							},
			{ "making_rate",						&battle_config.making_rate							},
			{ "extended_abracadabra",				&battle_config.extended_abracadabra					},
			{ "no_pk_level",						&battle_config.no_pk_level							},
			{ "allow_cloneskill_at_autospell",		&battle_config.allow_cloneskill_at_autospell		},
			{ "pk_noshift",							&battle_config.pk_noshift							},
			{ "pk_penalty_time",					&battle_config.pk_penalty_time						},
			{ "dropitem_itemrate_fix",				&battle_config.dropitem_itemrate_fix				},
			{ "gm_nomanner_lv",						&battle_config.gm_nomanner_lv						},
			{ "clif_fixpos_type",					&battle_config.clif_fixpos_type						},
			{ "romailuse",							&battle_config.romail								},
			{ "pc_die_script",						&battle_config.pc_die_script						},
			{ "pc_kill_script",						&battle_config.pc_kill_script						},
			{ "pc_movemap_script",					&battle_config.pc_movemap_script					},
			{ "pc_login_script",					&battle_config.pc_login_script						},
			{ "pc_logout_script",					&battle_config.pc_logout_script						},
			{ "set_pckillerid",						&battle_config.set_pckillerid						},
			{ "def_ratio_atk_to_shieldchain",		&battle_config.def_ratio_atk_to_shieldchain			},
			{ "def_ratio_atk_to_carttermination",	&battle_config.def_ratio_atk_to_carttermination		},
			{ "player_gravitation_type",			&battle_config.player_gravitation_type				},
			{ "enemy_gravitation_type",				&battle_config.enemy_gravitation_type				},
			{ "mob_attack_fixwalkpos",				&battle_config.mob_attack_fixwalkpos				},
			{ "mob_ai_limiter",						&battle_config.mob_ai_limiter						},
			{ "mob_ai_cpu_usage",					&battle_config.mob_ai_cpu_usage						},
			{ "itemidentify",						&battle_config.itemidentify							},
			{ "casting_penalty_type",				&battle_config.casting_penalty_type					},
			{ "casting_penalty_weapon",				&battle_config.casting_penalty_weapon				},
			{ "casting_penalty_shield",				&battle_config.casting_penalty_shield				},
			{ "casting_penalty_armor",				&battle_config.casting_penalty_armor				},
			{ "casting_penalty_helm",				&battle_config.casting_penalty_helm					},
			{ "casting_penalty_robe",				&battle_config.casting_penalty_robe					},
			{ "casting_penalty_shoes",				&battle_config.casting_penalty_shoes				},
			{ "casting_penalty_acce",				&battle_config.casting_penalty_acce					},
			{ "casting_penalty_arrow",				&battle_config.casting_penalty_arrow				},
			{ "show_always_party_name",				&battle_config.show_always_party_name				},
			{ "check_player_name_global_msg",		&battle_config.check_player_name_global_msg			},
			{ "check_player_name_party_msg",		&battle_config.check_player_name_party_msg			},
			{ "check_player_name_guild_msg",		&battle_config.check_player_name_guild_msg			},
			{ "save_player_when_drop_item",			&battle_config.save_player_when_drop_item			},
			{ "allow_homun_status_change",			&battle_config.allow_homun_status_change			},
			{ "save_homun_temporal_intimate",		&battle_config.save_homun_temporal_intimate			},
			{ "homun_intimate_rate",				&battle_config.homun_intimate_rate					},
			{ "homun_temporal_intimate_resilience",	&battle_config.homun_temporal_intimate_resilience	},
			{ "hvan_explosion_intimate",			&battle_config.hvan_explosion_intimate				},
			{ "homun_speed_is_same_as_pc",			&battle_config.homun_speed_is_same_as_pc			},
			{ "homun_skill_intimate_type",			&battle_config.homun_skill_intimate_type			},
			{ "master_get_homun_base_exp",			&battle_config.master_get_homun_base_exp			},
			{ "master_get_homun_job_exp",			&battle_config.master_get_homun_job_exp				},
			{ "extra_system_flag",					&battle_config.extra_system_flag					},
			{ "mob_take_over_sp",					&battle_config.mob_take_over_sp						},
			{ "party_join_limit",					&battle_config.party_join_limit						},
			{ "check_skillpos_range",				&battle_config.check_skillpos_range					},
			{ "pet_speed_is_same_as_pc",			&battle_config.pet_speed_is_same_as_pc				},
			{ "tax_rate",							&battle_config.tax_rate								},
			{ "steal_rate",							&battle_config.steal_rate							},
			{ "sw_def_type",						&battle_config.sw_def_type							},
			{ "calc_dist_flag",						&battle_config.calc_dist_flag						},
			{ "allow_sw_dist",						&battle_config.allow_sw_dist						},
			{ "storagesort_by_itemid",				&battle_config.storagesort_by_itemid				},
			{ "cancel_race",						&battle_config.cancel_race							},
			{ "allow_es_magic_all",					&battle_config.allow_es_magic_all					},
		};
		const int max = sizeof(data)/sizeof(data[0]);

		if(line[0] == '/' && line[1] == '/')
			continue;
		i=sscanf(line,"%[^:]:%s",w1,w2);
		if(i!=2)
			continue;

		if( strcmpi(w1,"import")==0 ) {
			battle_config_read(w2);
			continue;
		}
		for(i=0; i<max; i++) {
			if(strcmpi(w1,data[i].str)==0) {
				*data[i].val=battle_config_switch(w2);
				break;
			}
		}
		if(i == max)
			printf("unknown battle config : %s\a\n",w1);
	}
	fclose(fp);

	//�t���O����
	if(battle_config.allow_guild_skill_in_gvgtime_only)
		battle_config.guild_skill_available = 0;

	if(--count==0){
		if(battle_config.flooritem_lifetime < 1000)
			battle_config.flooritem_lifetime = LIFETIME_FLOORITEM*1000;
		if(battle_config.restart_hp_rate < 0)
			battle_config.restart_hp_rate = 0;
		else if(battle_config.restart_hp_rate > 100)
			battle_config.restart_hp_rate = 100;
		if(battle_config.restart_sp_rate < 0)
			battle_config.restart_sp_rate = 0;
		else if(battle_config.restart_sp_rate > 100)
			battle_config.restart_sp_rate = 100;
		if(battle_config.natural_healhp_interval < NATURAL_HEAL_INTERVAL)
			battle_config.natural_healhp_interval=NATURAL_HEAL_INTERVAL;
		if(battle_config.natural_healsp_interval < NATURAL_HEAL_INTERVAL)
			battle_config.natural_healsp_interval=NATURAL_HEAL_INTERVAL;
		if(battle_config.natural_heal_skill_interval < NATURAL_HEAL_INTERVAL)
			battle_config.natural_heal_skill_interval=NATURAL_HEAL_INTERVAL;
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

		if(battle_config.agi_penaly_count < 2)
			battle_config.agi_penaly_count = 2;
		if(battle_config.vit_penaly_count < 2)
			battle_config.vit_penaly_count = 2;

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

		add_timer_func_list(battle_delay_damage_sub, "battle_delay_damage_sub");
	}

	return 0;
}

