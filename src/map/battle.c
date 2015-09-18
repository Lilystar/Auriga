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

#ifdef MEMWATCH
#include "memwatch.h"
#endif

struct Battle_Config battle_config;

struct battle_delay_damage_ {
	struct block_list *src;
	int target;
	int damage;
	int flag;
};

/*==========================================
 * ダメージの遅延
 *------------------------------------------
 */
static int battle_delay_damage_sub(int tid,unsigned int tick,int id,int data)
{
	struct battle_delay_damage_ *dat = (struct battle_delay_damage_ *)data;

	if(dat) {
		struct block_list *target = map_id2bl(dat->target);

		if(map_id2bl(id) == dat->src && target && target->prev != NULL) {
			battle_damage(dat->src,target,dat->damage,dat->flag);
		}
		aFree(dat);
	}
	return 0;
}

int battle_delay_damage(unsigned int tick,struct block_list *src,struct block_list *target,int damage,int flag)
{
	struct battle_delay_damage_ *dat;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	dat = (struct battle_delay_damage_*)aCalloc(1,sizeof(struct battle_delay_damage_));
	dat->src    = src;
	dat->target = target->id;
	dat->damage = damage;
	dat->flag   = flag;
	add_timer2(tick,battle_delay_damage_sub,src->id,(int)dat,TIMER_FREE_DATA);
	return 0;
}

/*==========================================
 * 実際にHPを操作
 *------------------------------------------
 */
int battle_damage(struct block_list *bl,struct block_list *target,int damage,int flag)
{
	struct map_session_data *sd = NULL;
	int race, ele;

	nullpo_retr(0, target);	// blはNULLで呼ばれることがあるので他でチェック

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

	// 凍結・石化・睡眠を消去
	status_change_attacked_end(target);

	// 種族・属性取得
	race = status_get_race(target);
	ele  = status_get_elem_type(target);

	map_freeblock_lock();

	if(target->type == BL_MOB) {	// MOB
		struct mob_data *md = (struct mob_data *)target;

		unit_skillcastcancel(target,2);	// 詠唱妨害
		mob_damage(bl,md,damage,0);

		if(sd && md && md->bl.prev != NULL && !unit_isdead(&md->bl) && flag&(BF_WEAPON|BF_NORMAL) && status_get_class(target) != 1288)
		{
			int mode = status_get_mode(target);
			// カード効果のコーマ・即死
			if(atn_rand()%10000 < sd->weapon_coma_ele[ele] ||
			   atn_rand()%10000 < sd->weapon_coma_race[race] ||
			   (mode&0x20 && atn_rand()%10000 < sd->weapon_coma_race[RCT_BOSS]) ||
			   (!(mode&0x20) && atn_rand()%10000 < sd->weapon_coma_race[RCT_NONBOSS])) {
					mob_damage(bl,md,status_get_hp(target),0);
			}
			else if(atn_rand()%10000 < sd->weapon_coma_ele2[ele] ||
				atn_rand()%10000 < sd->weapon_coma_race2[race] ||
				(mode&0x20 && atn_rand()%10000 < sd->weapon_coma_race2[RCT_BOSS]) ||
				(!(mode&0x20) && atn_rand()%10000 < sd->weapon_coma_race2[RCT_BOSS])) {
					mob_damage(bl,md,status_get_hp(target)-1,0);
			}
		}
	} else if(target->type == BL_PC) {	// PC
		struct map_session_data *tsd = (struct map_session_data *)target;

		if(tsd && tsd->sc_data && tsd->sc_data[SC_DEVOTION].val1) {	// ディボーションをかけられている
			struct map_session_data *msd = map_id2sd(tsd->sc_data[SC_DEVOTION].val1);
			if(msd && skill_devotion3(msd,target->id)) {
				skill_devotion(msd);
			}
			else if(msd && bl) {
				int i;
				for(i=0; i<5; i++) {
					if(msd->dev.val1[i] != target->id)
						continue;
					clif_damage(&msd->bl, &msd->bl, gettick(), 0, 0, damage, 0, 9, 0);
					pc_damage(&msd->bl,msd,damage);

					if(sd && msd->bl.prev != NULL && !unit_isdead(&msd->bl) && flag&(BF_WEAPON|BF_NORMAL))
					{
						// カード効果のコーマ・即死
						if(atn_rand()%10000 < sd->weapon_coma_ele[ele] ||
						   atn_rand()%10000 < sd->weapon_coma_race[race] ||
						   atn_rand()%10000 < sd->weapon_coma_race[RCT_NONBOSS]) {
								pc_damage(&msd->bl,msd,status_get_hp(target));
						}
						else if(atn_rand()%10000 < sd->weapon_coma_ele2[ele] ||
						        atn_rand()%10000 < sd->weapon_coma_race2[race] ||
						        atn_rand()%10000 < sd->weapon_coma_race2[RCT_NONBOSS]) {
								pc_damage(&msd->bl,msd,status_get_hp(target)-1);
						}
					}
					map_freeblock_unlock();
					return 0;
				}
			}
		}
		unit_skillcastcancel(target,2);		// 詠唱妨害

		pc_damage(bl,tsd,damage);

		if(sd && tsd && tsd->bl.prev != NULL && !unit_isdead(&tsd->bl) && flag&(BF_WEAPON|BF_NORMAL))
		{
			// カード効果のコーマ・即死
			if(atn_rand()%10000 < sd->weapon_coma_ele[ele] ||
			   atn_rand()%10000 < sd->weapon_coma_race[race] ||
			   atn_rand()%10000 < sd->weapon_coma_race[RCT_NONBOSS]) {
					pc_damage(bl,tsd,status_get_hp(target));
			}
			else if(atn_rand()%10000 < sd->weapon_coma_ele2[ele] ||
			        atn_rand()%10000 < sd->weapon_coma_race2[race] ||
			        atn_rand()%10000 < sd->weapon_coma_race2[RCT_NONBOSS]) {
					pc_damage(bl,tsd,status_get_hp(target)-1);
			}
		}
	} else if(target->type == BL_HOM) {	// HOM
		struct homun_data *hd = (struct homun_data *)target;

		unit_skillcastcancel(target,2);		// 詠唱妨害
		homun_damage(bl,hd,damage);

		if(sd && hd && hd->bl.prev != NULL && !unit_isdead(&hd->bl) && flag&(BF_WEAPON|BF_NORMAL) && status_get_class(target) != 1288)
		{
			int mode = status_get_mode(target);
			// カード効果のコーマ・即死
			if(atn_rand()%10000 < sd->weapon_coma_ele[ele] ||
			   atn_rand()%10000 < sd->weapon_coma_race[race] ||
			   (mode&0x20 && atn_rand()%10000 < sd->weapon_coma_race[RCT_BOSS]) ||
			   (!(mode&0x20) && atn_rand()%10000 < sd->weapon_coma_race[RCT_NONBOSS])) {
					homun_damage(bl,hd,status_get_hp(target));
			}
			else if(atn_rand()%10000 < sd->weapon_coma_ele2[ele] ||
			        atn_rand()%10000 < sd->weapon_coma_race2[race] ||
			        (mode&0x20 && atn_rand()%10000 < sd->weapon_coma_race2[RCT_BOSS]) ||
			        (!(mode&0x20) && atn_rand()%10000 < sd->weapon_coma_race2[RCT_NONBOSS])) {
					homun_damage(bl,hd,status_get_hp(target)-1);
			}
		}
	} else if(target->type == BL_SKILL) {
		skill_unit_ondamaged((struct skill_unit *)target,bl,damage,gettick());
	}

	map_freeblock_unlock();
	return 0;
}

/*==========================================
 * HP/SP回復
 *------------------------------------------
 */
int battle_heal(struct block_list *bl,struct block_list *target,int hp,int sp,int flag)
{
	nullpo_retr(0, target);	// blはNULLで呼ばれることがあるので他でチェック

	if(target->type == BL_PET)
		return 0;
	if(unit_isdead(target))
		return 0;
	if(hp == 0 && sp == 0)
		return 0;

	if(hp < 0)
		return battle_damage(bl,target,-hp,flag);

	if(target->type == BL_MOB)
		return mob_heal((struct mob_data *)target,hp);
	else if(target->type == BL_PC)
		return pc_heal((struct map_session_data *)target,hp,sp);
	else if(target->type == BL_HOM)
		return homun_heal((struct homun_data *)target,hp,sp);
	return 0;
}

/*==========================================
 * ダメージの属性修正
 *------------------------------------------
 */
int battle_attr_fix(int damage,int atk_elem,int def_elem)
{
	int def_type, def_lv, rate;

	// 属性無し(!=無属性)
	if (atk_elem == ELE_NONE)
		return damage;

	def_type = def_elem%20;
	def_lv   = def_elem/20;

	if( atk_elem == ELE_MAX )
		atk_elem = atn_rand()%ELE_MAX;	// 武器属性ランダムで付加

	if( atk_elem < 0 || atk_elem >= ELE_MAX ||
	    def_type < 0 || def_type >= ELE_MAX ||
	    def_lv <= 0 || def_lv > MAX_ELE_LEVEL )
	{
		// 属性値がおかしいのでとりあえずそのまま返す
		if(battle_config.error_log)
			printf("battle_attr_fix: unknown attr type: atk=%d def_type=%d def_lv=%d\n",atk_elem,def_type,def_lv);
		return damage;
	}

	rate = attr_fix_table[def_lv-1][atk_elem][def_type];
	if(damage < 0 && rate < 0)	// 負×負の場合は結果を負にする
		rate = -rate;

	return damage * rate / 100;
}

/*==========================================
 * ダメージ最終計算
 *------------------------------------------
 */
static int battle_calc_damage(struct block_list *src,struct block_list *bl,int damage,int div_,int skill_num,int skill_lv,int flag)
{
	struct map_session_data *tsd = NULL;
	struct mob_data         *tmd = NULL;
	struct status_change *sc_data;
	short *sc_count;
	unsigned int tick = gettick();

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	tsd = BL_DOWNCAST( BL_PC,  bl );
	tmd = BL_DOWNCAST( BL_MOB, bl );

	sc_data  = status_get_sc_data(bl);
	sc_count = status_get_sc_count(bl);

	// スキルダメージ補正
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
	if(sc_count != NULL && *sc_count > 0 && sc_data) {
		if(sc_data[SC_ASSUMPTIO].timer != -1 && damage > 0 && skill_num != PA_PRESSURE && skill_num != HW_GRAVITATION) { // アスムプティオ
			if(map[bl->m].flag.pvp || map[bl->m].flag.gvg)
				damage = damage*2/3;
			else
				damage = damage/2;
		}

		// ゴスペルの特殊状態異常
		if(sc_data[SC_INCDAMAGE].timer != -1 && damage > 0 && skill_num != PA_PRESSURE && skill_num != HW_GRAVITATION)
			damage += damage * sc_data[SC_INCDAMAGE].val1/100;

		if(sc_data[SC_BASILICA].timer != -1 && damage > 0 && !(status_get_mode(src)&0x20))
			damage = 0;

		if(sc_data[SC_FOGWALL].timer != -1 && damage > 0 && flag&BF_WEAPON && flag&BF_LONG)
		{
			if(skill_num == 0) {	// 通常攻撃75%OFF
				damage = damage*25/100;
			} else {		// スキル25%OFF
				damage = damage*75/100;
			}
		}

		if(sc_data[SC_SAFETYWALL].timer != -1 && flag&BF_SHORT && skill_num != NPC_GUIDEDATTACK) {
			// セーフティウォール
			struct skill_unit *unit = map_id2su(sc_data[SC_SAFETYWALL].val2);
			if(unit && unit->group) {
				if((--unit->group->val2) <= 0)
					skill_delunit(unit);
				damage = 0;
			} else {
				status_change_end(bl,SC_SAFETYWALL,-1);
			}
		}

		// ニューマ
		if( (sc_data[SC_PNEUMA].timer != -1 || sc_data[SC_TATAMIGAESHI].timer != -1) && damage > 0 &&
		    flag&(BF_WEAPON|BF_MISC) && flag&BF_LONG && skill_num != NPC_GUIDEDATTACK )
		{
			damage = 0;
		}
		if(sc_data[SC_AETERNA].timer != -1 && damage > 0 && skill_num != PA_PRESSURE && skill_num != HW_GRAVITATION) {	// レックスエーテルナ
			damage <<= 1;
			status_change_end( bl,SC_AETERNA,-1);
		}

		// 属性場のダメージ増加
		if(sc_data[SC_VOLCANO].timer != -1 && damage > 0) {	// ボルケーノ
			if( flag&BF_SKILL && skill_get_pl(skill_num) == ELE_FIRE )
				damage += damage*sc_data[SC_VOLCANO].val4/100;
			else if( !(flag&BF_SKILL) && status_get_attack_element(bl) == ELE_FIRE )
				damage += damage*sc_data[SC_VOLCANO].val4/100;
		}

		if(sc_data[SC_VIOLENTGALE].timer != -1 && damage > 0) {	// バイオレントゲイル
			if( flag&BF_SKILL && skill_get_pl(skill_num) == ELE_WIND )
				damage += damage*sc_data[SC_VIOLENTGALE].val4/100;
			else if( !(flag&BF_SKILL) && status_get_attack_element(bl) == ELE_WIND )
				damage += damage*sc_data[SC_VIOLENTGALE].val4/100;
		}

		if(sc_data[SC_DELUGE].timer != -1 && damage > 0) {	// デリュージ
			if( flag&BF_SKILL && skill_get_pl(skill_num) == ELE_WATER )
				damage += damage*sc_data[SC_DELUGE].val4/100;
			else if( !(flag&BF_SKILL) && status_get_attack_element(bl) == ELE_WATER )
				damage += damage*sc_data[SC_DELUGE].val4/100;
		}

		if(sc_data[SC_ENERGYCOAT].timer != -1 && damage > 0 && flag&BF_WEAPON && skill_num != PA_PRESSURE) {	// エナジーコート プレッシャーは軽減しない
			if(tsd) {
				if(tsd->status.sp > 0) {
					int per = tsd->status.sp * 5 / (tsd->status.max_sp + 1);
					tsd->status.sp -= tsd->status.sp * (per * 5 + 10) / 1000;
					if(tsd->status.sp < 0)
						tsd->status.sp = 0;
					damage -= damage * ((per+1) * 6) / 100;
					clif_updatestatus(tsd,SP_SP);
				}
				if(tsd->status.sp <= 0)
					status_change_end( bl,SC_ENERGYCOAT,-1);
			} else {
				damage -= damage * (sc_data[SC_ENERGYCOAT].val1 * 6) / 100;
			}
		}

		if(sc_data[SC_KYRIE].timer != -1 && damage > 0) {	// キリエエレイソン
			struct status_change *sc = &sc_data[SC_KYRIE];
			sc->val2 -= damage;
			if(flag&BF_WEAPON) {
				if(sc->val2 >= 0)
					damage = 0;
				else
					damage = -sc->val2;
			}
			if(--sc->val3 <= 0 || sc->val2 <= 0 || skill_num == AL_HOLYLIGHT)
				status_change_end(bl, SC_KYRIE, -1);
		}
		// インデュア
		if(sc_data[SC_ENDURE].timer != -1 && damage > 0 && flag&BF_WEAPON && src->type != BL_PC) {
			if((--sc_data[SC_ENDURE].val2) <= 0)
				status_change_end(bl, SC_ENDURE, -1);
		}
		// オートガード
		if(sc_data[SC_AUTOGUARD].timer != -1 && damage > 0 && flag&BF_WEAPON && skill_num != WS_CARTTERMINATION) {
			if(atn_rand()%100 < sc_data[SC_AUTOGUARD].val2) {
				int delay;
				damage = 0;
				clif_skill_nodamage(bl,bl,CR_AUTOGUARD,sc_data[SC_AUTOGUARD].val1,1);
				if(sc_data[SC_AUTOGUARD].val1 <= 5)
					delay = 300;
				else if (sc_data[SC_AUTOGUARD].val1 > 5 && sc_data[SC_AUTOGUARD].val1 <= 9)
					delay = 200;
				else
					delay = 100;
				if(tsd) {
					tsd->ud.canmove_tick = tick + delay;
					if(sc_data[SC_SHRINK].timer != -1 && atn_rand()%100 < 5*sc_data[SC_AUTOGUARD].val1)
						skill_blown(bl,src,2);
				} else if(tmd) {
					tmd->ud.canmove_tick = tick + delay;
				}
			}
		}
		// パリイング
		if(sc_data[SC_PARRYING].timer != -1 && damage > 0 && flag&BF_WEAPON && skill_num != WS_CARTTERMINATION) {
			if(atn_rand()%100 < sc_data[SC_PARRYING].val2)
			{
				int dir = map_calc_dir(bl,src->x,src->y);
				struct unit_data *ud = unit_bl2ud(bl);
				damage = 0;
				clif_skill_nodamage(bl,bl,LK_PARRYING,sc_data[SC_PARRYING].val1,1);
				clif_changedir(bl,dir,dir);
				if(ud)
					ud->attackabletime = tick + 500;	// 値適当
			}
		}
		// リジェクトソード
		if(sc_data[SC_REJECTSWORD].timer != -1 && damage > 0 && flag&BF_WEAPON && atn_rand()%100 < 15*sc_data[SC_REJECTSWORD].val1) {
			short weapon = -1;
			if(src->type == BL_PC)
				weapon = ((struct map_session_data *)src)->status.weapon;
			if(src->type == BL_MOB || weapon == WT_DAGGER || weapon == WT_1HSWORD || weapon == WT_2HSWORD) {
				damage = damage*50/100;
				battle_damage(bl,src,damage,0);
				clif_skill_nodamage(bl,bl,ST_REJECTSWORD,sc_data[SC_REJECTSWORD].val1,1);
				if((--sc_data[SC_REJECTSWORD].val2) <= 0)
					status_change_end(bl, SC_REJECTSWORD, -1);
			}
		}
		if(sc_data[SC_SPIDERWEB].timer != -1 && damage > 0) {
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

		if(status_get_class(bl) == 1288) {	// 1288:エンペリウム
			if(flag&BF_SKILL && skill_num != HW_GRAVITATION)
				return 0;
			if(src->type == BL_PC) {
				struct guild *g = guild_search(((struct map_session_data *)src)->status.guild_id);

				if(g == NULL)
					return 0;		// ギルド未加入ならダメージ無し
				if(guild_checkskill(g,GD_APPROVAL) <= 0)
					return 0;		// 正規ギルド承認がないとダメージ無し
				if((gc = guild_mapname2gc(map[bl->m].name)) != NULL) {
					if(g->guild_id == gc->guild_id)
						return 0;	// 自占領ギルドのエンペならダメージ無し
					if(guild_check_alliance(gc->guild_id, g->guild_id, 0))
						return 0;	// 同盟ならダメージ無し
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
				if(gc == NULL && !noflag)	// エンペリウムの項で既に検索してNULLなら再度検索しない
					gc = guild_mapname2gc(map[bl->m].name);
				if(gc) {
					// defenseがあればダメージが減るらしい？
					damage -= damage*(gc->defense/100)*(battle_config.castle_defense_rate/100);
				}
			}
			if(flag&BF_WEAPON) {
				if(flag&BF_SHORT)
					damage = damage*battle_config.gvg_short_damage_rate/100;
				if(flag&BF_LONG)
					damage = damage*battle_config.gvg_long_damage_rate/100;
			}
			if(flag&BF_MAGIC)
				damage = damage*battle_config.gvg_magic_damage_rate/100;
			if(flag&BF_MISC)
				damage = damage*battle_config.gvg_misc_damage_rate/100;
			if(damage < 1)
				damage = (!battle_config.skill_min_damage && flag&BF_MAGIC && src->type == BL_PC)? 0: 1;
		}
		// PK
		if(map[bl->m].flag.pk && bl->type == BL_PC && skill_num != PA_PRESSURE && skill_num != HW_GRAVITATION) {
			if(flag&BF_WEAPON) {
				if(flag&BF_SHORT)
					damage = damage*battle_config.pk_short_damage_rate/100;
				if(flag&BF_LONG)
					damage = damage*battle_config.pk_long_damage_rate/100;
			}
			if(flag&BF_MAGIC)
				damage = damage*battle_config.pk_magic_damage_rate/100;
			if(flag&BF_MISC)
				damage = damage*battle_config.pk_misc_damage_rate/100;
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

	if(tmd && tmd->hp > 0 && damage > 0)	// 反撃などのMOBスキル判定
	{
		int mtg = tmd->target_id;
		if (battle_config.mob_changetarget_byskill != 0 || mtg == 0)
		{
			if(src->type == BL_PC || src->type == BL_HOM)
				tmd->target_id = src->id;
		}
		mobskill_event(tmd,flag);
		tmd->target_id = mtg;	// ターゲットを戻す
	}

	// PCの反撃オートスペル
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

	// PCの反撃
	if(tsd && src != &tsd->bl && !unit_isdead(src) && tsd->status.hp > 0 && damage > 0 && flag&BF_WEAPON)
	{
		// 反撃状態異常
		if(tsd->addreveff_flag) {
			int i, rate, luk;
			int sc_def_card = 100;
			int sc_def_mdef,sc_def_vit,sc_def_int,sc_def_luk;
			const int sc2[]={
				MG_STONECURSE,MG_FROSTDIVER,NPC_STUNATTACK,
				NPC_SLEEPATTACK,TF_POISON,NPC_CURSEATTACK,
				NPC_SILENCEATTACK,0,NPC_BLINDATTACK,LK_HEADCRUSH
			};
			// 対象の耐性
			luk = status_get_luk(src);
			sc_def_mdef = 50 - (3 + status_get_mdef(src) + luk/3);
			sc_def_vit  = 50 - (3 + status_get_vit(src) + luk/3);
			sc_def_int  = 50 - (3 + status_get_int(src) + luk/3);
			sc_def_luk  = 50 - (3 + luk);

/*			if(target->bl.type == BL_MOB) {
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

			for(i=SC_STONE; i<=SC_BLEED; i++) {
				// 対象に状態異常
				if(i == SC_STONE || i == SC_FREEZE)
					sc_def_card = sc_def_mdef;
				else if(i == SC_STAN || i == SC_POISON || i == SC_SILENCE || i == SC_BLEED)
					sc_def_card = sc_def_vit;
				else if(i == SC_SLEEP || i == SC_CONFUSION || i == SC_BLIND)
					sc_def_card = sc_def_int;
				else if(i == SC_CURSE)
					sc_def_card = sc_def_luk;

				if(battle_config.reveff_plus_addeff)
					rate = (tsd->addreveff[i-SC_STONE] + tsd->addeff[i-SC_STONE] + tsd->arrow_addeff[i-SC_STONE])*sc_def_card/100;
				else
					rate = (tsd->addreveff[i-SC_STONE])*sc_def_card/100;

				if(src->type == BL_PC || src->type == BL_MOB || src->type == BL_HOM)
				{
					if(atn_rand()%10000 < rate ) {
						if(battle_config.battle_log)
							printf("PC %d skill_addreveff: cardによる異常発動 %d %d\n",tsd->bl.id,i,tsd->addreveff[i-SC_STONE]);
						status_change_start(src,i,7,0,0,0,(i == SC_CONFUSION)? 10000+7000: skill_get_time2(sc2[i-SC_STONE],7),0);
					}
				}
			}
		}
	}

	return damage;
}

/*==========================================
 * HP/SPの％吸収計算
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
 * HP/SPの一定吸収計算
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
 * 攻撃によるHP/SP吸収
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

	if(flag & 1) {	// ％吸収
		if(!battle_config.left_cardfix_to_right) {
			// 二刀流左手カードの吸収系効果を右手に追加しない場合
			hp += battle_calc_drain_per(damage,  sd->hp_drain.p_rate,  sd->hp_drain.per );
			hp += battle_calc_drain_per(damage2, sd->hp_drain_.p_rate, sd->hp_drain_.per);
			sp += battle_calc_drain_per(damage,  sd->sp_drain.p_rate,  sd->sp_drain.per );
			sp += battle_calc_drain_per(damage2, sd->sp_drain_.p_rate, sd->sp_drain_.per);
		} else {
			// 二刀流左手カードの吸収系効果を右手に追加する場合
			int hp_rate = sd->hp_drain.p_rate + sd->hp_drain_.p_rate;
			int hp_per  = sd->hp_drain.per + sd->hp_drain_.per;
			int sp_rate = sd->sp_drain.p_rate + sd->sp_drain_.p_rate;
			int sp_per  = sd->sp_drain.per + sd->sp_drain_.per;
			hp += battle_calc_drain_per(damage, hp_rate, hp_per);
			sp += battle_calc_drain_per(damage, sp_rate, sp_per);
		}
	}
	if(flag & 2) {	// 一定吸収
		if(!battle_config.left_cardfix_to_right) {
			// 二刀流左手カードの吸収系効果を右手に追加しない場合
			hp += battle_calc_drain_value(damage,  sd->hp_drain.v_rate,  sd->hp_drain.value );
			hp += battle_calc_drain_value(damage2, sd->hp_drain_.v_rate, sd->hp_drain_.value);
			sp += battle_calc_drain_value(damage,  sd->sp_drain.v_rate,  sd->sp_drain.value );
			sp += battle_calc_drain_value(damage2, sd->sp_drain_.v_rate, sd->sp_drain_.value);
		} else {
			// 二刀流左手カードの吸収系効果を右手に追加する場合
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
 * 修練ダメージ
 *------------------------------------------
 */
static int battle_addmastery(struct map_session_data *sd,struct block_list *target,int dmg,int type)
{
	int damage = 0, race, skill, weapon;

	nullpo_retr(0, sd);
	nullpo_retr(0, target);

	race = status_get_race(target);

	// デーモンベイン vs 不死 or 悪魔 (死人は含めない？)
	if((skill = pc_checkskill(sd,AL_DEMONBANE)) > 0 && (battle_check_undead(race,status_get_elem_type(target)) || race == RCT_DEMON) ) {
		damage += (300 + 5 * sd->status.base_level) * skill / 100;
	}

	// ビーストベイン(+4 〜 +40) vs 動物 or 昆虫
	if((skill = pc_checkskill(sd,HT_BEASTBANE)) > 0 && (race == RCT_BRUTE || race == RCT_INSECT) )
	{
		damage += (skill * 4);

		if(sd->sc_data && sd->sc_data[SC_HUNTER].timer != -1)
			damage += status_get_str(&sd->bl);
	}
	weapon = (type == 0)? sd->weapontype1: sd->weapontype2;

	switch(weapon)
	{
		case WT_DAGGER:
		case WT_1HSWORD:
			// 剣修練(+4 〜 +40) 片手剣 短剣含む
			if((skill = pc_checkskill(sd,SM_SWORD)) > 0) {
				damage += (skill * 4);
			}
			break;
		case WT_2HSWORD:
			// 両手剣修練(+4 〜 +40) 両手剣
			if((skill = pc_checkskill(sd,SM_TWOHAND)) > 0) {
				damage += (skill * 4);
			}
			break;
		case WT_1HSPEAR:
			// 槍修練(+4 〜 +40,+5 〜 +50) 槍
			if((skill = pc_checkskill(sd,KN_SPEARMASTERY)) > 0) {
				if(!pc_isriding(sd))
					damage += (skill * 4);	// ペコに乗ってない
				else
					damage += (skill * 5);	// ペコに乗ってる
			}
			break;
		case WT_2HSPEAR:
			// 槍修練(+4 〜 +40,+5 〜 +50) 槍
			if((skill = pc_checkskill(sd,KN_SPEARMASTERY)) > 0) {
				if(!pc_isriding(sd))
					damage += (skill * 4);	// ペコに乗ってない
				else
					damage += (skill * 5);	// ペコに乗ってる
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
			// メイス修練(+3 〜 +30) メイス
			if((skill = pc_checkskill(sd,PR_MACEMASTERY)) > 0) {
				damage += (skill * 3);
			}
			break;
		case WT_2HMACE:
		case WT_STAFF:
		case WT_BOW:
			break;
		case WT_FIST:
			// タイリギ(+10 〜 +100) 素手
			if((skill = pc_checkskill(sd,TK_RUN)) > 0) {
				damage += (skill * 10);
			}
			// fall through
		case WT_KNUCKLE:
			// 鉄拳(+3 〜 +30) 素手,ナックル
			if((skill = pc_checkskill(sd,MO_IRONHAND)) > 0) {
				damage += (skill * 3);
			}
			break;
		case WT_MUSICAL:
			// 楽器の練習(+3 〜 +30) 楽器
			if((skill = pc_checkskill(sd,BA_MUSICALLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		case WT_WHIP:
			// ダンスの練習(+3 〜 +30) 鞭
			if((skill = pc_checkskill(sd,DC_DANCINGLESSON)) > 0) {
				damage += (skill * 3);
			}
			break;
		case WT_BOOK:
			// アドバンスドブック(+3 〜 +30) {
			if((skill = pc_checkskill(sd,SA_ADVANCEDBOOK)) > 0) {
				damage += (skill * 3);
			}
			break;
		case WT_KATAR:
			// カタール修練(+3 〜 +30) カタール
			if((skill = pc_checkskill(sd,AS_KATAR)) > 0) {
				// ソニックブロー時は別処理（1撃に付き1/8適応)
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
 * 基本武器ダメージ計算
 *------------------------------------------
 */
static int battle_calc_base_damage(struct block_list *src,struct block_list *target,int skill_num,int type,int lh)
{
	int damage = 0;
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

		if(skill_num == HW_MAGICCRASHER) {		// マジッククラッシャーはMATKで殴る
			damage = status_get_matk1(src);
		} else {
			damage = status_get_baseatk(src);
		}

		atkmin = dex;	// 最低ATKはDEXで初期化

		if(idx >= 0 && sd->inventory_data[idx])
			atkmin = atkmin * (80 + sd->inventory_data[idx]->wlv * 20) / 100;
		if(sd->state.arrow_atk)						// 武器が弓矢の場合
			atkmin = watk * ((atkmin < watk)? atkmin: watk) / 100;	// 弓用最低ATK計算

		/* サイズ修正 */
		if(skill_num == MO_EXTREMITYFIST) {
			// 阿修羅
			atkmax = watk;
		} else if(pc_isriding(sd) && (sd->status.weapon == WT_1HSPEAR || sd->status.weapon == WT_2HSPEAR) && t_size == 1) {
			// ペコ騎乗していて、槍で中型を攻撃した場合はサイズ修正を100にする
			atkmax = watk;
		} else {
			int rate = (lh == 0)? sd->atkmods[t_size]: sd->atkmods_[t_size];
			atkmax = (watk   * rate) / 100;
			atkmin = (atkmin * rate) / 100;
		}
		if(sc_data && sc_data[SC_WEAPONPERFECTION].timer != -1) {
			// ウェポンパーフェクション
			atkmax = watk;
		} else if(sd->special_state.no_sizefix) {
			// ドレイクカード
			atkmax = watk;
		}
		if(!sd->state.arrow_atk && atkmin > atkmax)
			atkmin = atkmax;	// 弓は最低が上回る場合あり
		if(lh && atkmin > atkmax)
			atkmin = atkmax;

		/* 太陽と月と星の怒り */
		if(target->type == BL_PC || target->type == BL_MOB || target->type == BL_HOM)
		{
			int atk_rate = 0;
			int str = status_get_str(src);
			int luk = status_get_luk(src);
			int tclass = status_get_class(target);

			if(sc_data && sc_data[SC_MIRACLE].timer != -1) {	// 太陽と月と星の奇跡
				// 全ての敵が星
				atk_rate = (sd->status.base_level + dex + luk + str)/(12-3*pc_checkskill(sd,SG_STAR_ANGER));
			} else {
				if(tclass == sd->hate_mob[0] && pc_checkskill(sd,SG_SUN_ANGER) > 0)		// 太陽の怒り
					atk_rate = (sd->status.base_level + dex + luk) / (12 - 3 * pc_checkskill(sd,SG_SUN_ANGER));
				else if(tclass == sd->hate_mob[1] && pc_checkskill(sd,SG_MOON_ANGER) > 0)	// 月の怒り
					atk_rate = (sd->status.base_level + dex + luk) / (12 - 3 * pc_checkskill(sd,SG_MOON_ANGER));
				else if(tclass == sd->hate_mob[2] && pc_checkskill(sd,SG_STAR_ANGER) > 0)	// 星の怒り
					atk_rate = (sd->status.base_level + dex + luk + str) / (12 - 3 * pc_checkskill(sd,SG_STAR_ANGER));
			}
			if(atk_rate > 0) {
				atkmin += atkmin * atk_rate / 100;
				atkmax += atkmax * atk_rate / 100;
			}
		}
		/* 過剰精錬ボーナス */
		if(!lh && sd->overrefine > 0)
			damage += (atn_rand() % sd->overrefine ) + 1;
		if(lh && sd->overrefine_ > 0)
			damage += (atn_rand() % sd->overrefine_) + 1;
	} else {
		if(battle_config.enemy_str)
			damage = status_get_baseatk(src);
		else
			damage = 0;
		if(skill_num == HW_MAGICCRASHER) {	// マジッククラッシャーはMATKで殴る
			atkmin = status_get_matk1(src);
			atkmax = status_get_matk2(src);
		} else {
			atkmin = status_get_atk(src);
			atkmax = status_get_atk2(src);
		}
		if(atkmin > atkmax)
			atkmin = atkmax;
	}

	if(sc_data && sc_data[SC_MAXIMIZEPOWER].timer != -1) {
		// マキシマイズパワー
		atkmin = atkmax;
	}

	if(type == 0x0a) {
		/* クリティカル攻撃 */
		damage += atkmax;
		if(sd && (sd->atk_rate != 100 || sd->weapon_atk_rate != 0)) {
			damage = (damage * (sd->atk_rate + sd->weapon_atk_rate[sd->status.weapon])) / 100;

			// クリティカル時ダメージ増加
			damage += damage *sd->critical_damage / 100;
		}
		if(sd && sd->state.arrow_atk)
			damage += sd->arrow_atk;
	} else {
		/* 通常攻撃・スキル攻撃 */
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

// 左手判定があるときだけdamage2を計算する
#define DMG_FIX( a,b ) { wd.damage = wd.damage*(a)/(b); if(calc_flag.lh) wd.damage2 = wd.damage2*(a)/(b); }
#define DMG_ADD( a )   { wd.damage += (a); if(calc_flag.lh) wd.damage2 += (a); }
#define DMG_SET( a )   { wd.damage = (a); if(calc_flag.lh) wd.damage2 = (a); }

/*==========================================
 * 武器ダメージ計算
 *------------------------------------------
 */
static struct Damage battle_calc_weapon_attack(struct block_list *src,struct block_list *target,int skill_num,int skill_lv,int wflag)
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
	int cardfix, skill, damage_sbr = 0;
	int i;
	struct {
		int rh;			// 右手
		int lh;			// 左手
		int hitrate;		// ヒット確率
		int autocounter;	// オートカウンターON
		int da;			// 連撃判定（0〜6）
		int idef;		// DEF無視
		int idef_;		// DEf無視（左手）
		int nocardfix;		// カード補正なし
		int dist;		// 遠距離スキルの計算フラグ
	} calc_flag;

	memset(&calc_flag, 0, sizeof(calc_flag));

	// return前の処理があるので情報出力部のみ変更
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

	// アタッカー
	s_ele    = status_get_attack_element(src);	// 属性
	s_ele_   = status_get_attack_element2(src);	// 左手属性
	s_str    = status_get_str(src);			// STR
	sc_data  = status_get_sc_data(src);		// ステータス異常

	// ターゲット
	t_vit     = status_get_vit(target);
	t_race    = status_get_race(target);		// 対象の種族
	t_ele     = status_get_elem_type(target);	// 対象の属性
	t_enemy   = status_get_enemy_type(target);	// 対象の敵タイプ
	t_size    = status_get_size(target);		// 対象のサイズ
	t_mode    = status_get_mode(target);		// 対象のMode
	t_sc_data = status_get_sc_data(target);		// 対象のステータス異常
	t_group   = status_get_group(target);
	t_class   = status_get_class(target);
	t_flee    = status_get_flee(target);
	t_def1    = status_get_def(target);
	t_def2    = status_get_def2(target);

	if(src_sd && skill_num != CR_GRANDCROSS && skill_num != NPC_DARKGRANDCROSS)
		src_sd->state.attack_type = BF_WEAPON;	// 攻撃タイプは武器攻撃

	/* １．オートカウンター処理 */
	if(skill_lv >= 0 && (skill_num == 0 || (target_sd && battle_config.pc_auto_counter_type&2) ||
		(target_md && battle_config.monster_auto_counter_type&2))
	) {
		if( skill_num != CR_GRANDCROSS &&
		    skill_num != NPC_DARKGRANDCROSS &&
		    t_sc_data &&
		    t_sc_data[SC_AUTOCOUNTER].timer != -1 )
		{
			// グランドクロスでなく、対象がオートカウンター状態の場合
			int dir   = map_calc_dir(src,target->x,target->y);
			int t_dir = status_get_dir(target);
			int dist  = unit_distance2(src,target);

			if(dist <= 0 || map_check_dir(dir,t_dir) ) {
				// 対象との距離が0以下、または対象の正面？
				t_sc_data[SC_AUTOCOUNTER].val3 = 0;
				t_sc_data[SC_AUTOCOUNTER].val4 = 1;
				if(sc_data && sc_data[SC_AUTOCOUNTER].timer == -1) {
					int range = status_get_range(target);
					// 自分がオートカウンター状態
					if( target_sd &&
						(target_sd->status.weapon != WT_BOW && !(target_sd->status.weapon >= WT_HANDGUN && target_sd->status.weapon <= WT_GRENADE))
						&& dist <= range+1)
						// 対象がPCで武器が弓矢でなく射程内
						t_sc_data[SC_AUTOCOUNTER].val3 = src->id;
					if( target_md && range <= 3 && dist <= range+1)
						// または対象がMobで射程が3以下で射程内
						t_sc_data[SC_AUTOCOUNTER].val3 = src->id;
				}
				return wd; // ダメージ構造体を返して終了
			}
			else calc_flag.autocounter = 1;
		}
	}

	/* ２．初期化補正 */
	// 属性無し(!=無属性)
	if( (src_sd && battle_config.pc_attack_attr_none) ||
	    (src_md && battle_config.mob_attack_attr_none) ||
	    (src_pd && battle_config.pet_attack_attr_none) ||
	     src_hd )
	{
		if (s_ele == ELE_NEUTRAL)
			s_ele  = ELE_NONE;
		if (s_ele_ == ELE_NEUTRAL)
			s_ele_ = ELE_NONE;
	}

	calc_flag.hitrate = status_get_hit(src) - t_flee + 80;	// 命中率計算
	// 霧のHIT補正
	if(skill_num == 0 && t_sc_data && t_sc_data[SC_FOGWALL].timer != -1)
		calc_flag.hitrate -= 50;

	/* ３．wd構造体の初期設定 */
	wd.type      = 0;
	wd.div_      = skill_get_num(skill_num,skill_lv);
	wd.blewcount = skill_get_blewcount(skill_num,skill_lv);
	wd.flag      = BF_SHORT | BF_WEAPON | BF_NORMAL;	// 攻撃の種類の設定

	if(skill_num == GS_DESPERADO)
		wd.div_ = 1;
	if(wd.div_ <= 0 || wd.div_ >= 251)			// 251〜254 = テコン蹴り、255 = 三段掌として予約済
		wd.div_ = 1;

	if(src_sd) {
		if(src_sd->status.weapon == WT_BOW || (src_sd->status.weapon >= WT_HANDGUN && src_sd->status.weapon <= WT_GRENADE)) {	// 武器が弓矢の場合
			wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;	// 遠距離攻撃フラグを有効
			if(src_sd->arrow_ele > 0)	// 属性矢なら属性を矢の属性に変更
				s_ele = src_sd->arrow_ele;
			src_sd->state.arrow_atk = 1;	// 有効化
		} else {
			src_sd->state.arrow_atk = 0;	// 初期化
		}
	} else if(src_md || src_pd) {
		if(status_get_range(src) > 3)
			wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;
	}

	/* ４．右手・左手判定 */
	calc_flag.rh = 1;	// 基本は右手のみ
	if(src_sd) {
		if(src_sd->weapontype1 == WT_FIST && src_sd->weapontype2 > WT_FIST) {	// 左手のみ武器装備
			calc_flag.rh = 0;
			calc_flag.lh = 1;
		} else if(skill_num == 0 && src_sd->status.weapon > WT_HUUMA) {		// 通常攻撃で二刀流（スキル攻撃に左手は乗らない）
			calc_flag.lh = 1;
		}
	}

	/* ５．連撃判定 */
	if(src_sd && skill_num == 0 && skill_lv >= 0) {
		do {
			// 三段掌
			if((skill = pc_checkskill(src_sd,MO_TRIPLEATTACK)) > 0 && src_sd->status.weapon <= WT_HUUMA)
			{
				int triple_rate = 0;
				if(sc_data && sc_data[SC_TRIPLEATTACK_RATE_UP].timer != -1) {
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
			// ダブルアタック
			if((skill = pc_checkskill(src_sd,TF_DOUBLE)) > 0 && src_sd->weapontype1 == WT_DAGGER && atn_rand()%100 < skill*5) {
				calc_flag.da = 1;
				calc_flag.hitrate = calc_flag.hitrate*(100+skill)/100;
				break;
			}
			// チェーンアクション
			if((skill = pc_checkskill(src_sd,GS_CHAINACTION)) > 0 && src_sd->weapontype1 == WT_HANDGUN && atn_rand()%100 < skill*5) {
				calc_flag.da = 1;
				break;
			}
			// フェオリチャギ
			if(sc_data && sc_data[SC_READYSTORM].timer != -1 && pc_checkskill(src_sd,TK_STORMKICK) > 0 && atn_rand()%100 < 15) {
				calc_flag.da = 3;
				break;
			}
			// ネリョチャギ
			if(sc_data && sc_data[SC_READYDOWN].timer != -1 && pc_checkskill(src_sd,TK_DOWNKICK) > 0 && atn_rand()%100 < 15) {
				calc_flag.da = 4;
				break;
			}
			// トルリョチャギ
			if(sc_data && sc_data[SC_READYTURN].timer != -1 &&  pc_checkskill(src_sd,TK_TURNKICK) > 0 && atn_rand()%100 < 15) {
				calc_flag.da = 5;
				break;
			}
			// アプチャオルリギ
			if(sc_data && sc_data[SC_READYCOUNTER].timer != -1 && pc_checkskill(src_sd,TK_COUNTER) > 0)
			{
				int counter_rate = 0;
				if(sc_data[SC_COUNTER_RATE_UP].timer != -1 && (skill = pc_checkskill(src_sd,SG_FRIEND)) > 0) {
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
			// サイドワインダー等
			if(src_sd->double_rate > 0 && atn_rand()%100 < src_sd->double_rate) {
				calc_flag.da = 1;
				break;
			}
		} while(0);
	}

	/* ６．クリティカル計算 */
	if( calc_flag.da == 0 && (skill_num == 0 || skill_num == KN_AUTOCOUNTER || skill_num == SN_SHARPSHOOTING || skill_num == NJ_KIRIKAGE) &&
	    (!src_md || battle_config.enemy_critical) && skill_lv >= 0 )
	{
		// 連撃が発動してなくて、通常攻撃・オートカウンター・シャープシューティング・影斬りならば
		int cri = status_get_critical(src);
		if(src_sd) {
			cri += src_sd->critical_race[t_race];
			if(src_sd->state.arrow_atk)
				cri += src_sd->arrow_cri;
			if(src_sd->status.weapon == WT_KATAR)
				cri <<= 1;	// カタールの場合、クリティカルを倍に
		}
		cri -= status_get_luk(target) * 3;
		if(src_md && battle_config.enemy_critical_rate != 100) {
			cri = cri*battle_config.enemy_critical_rate/100;
			if(cri < 1) cri = 1;
		}
		if(t_sc_data != NULL && t_sc_data[SC_SLEEP].timer != -1 )
			cri <<= 1;		// 睡眠中はクリティカルが倍に
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

		// 確率判定
		if(atn_rand() % 1000 < cri) {
			if(skill_num == SN_SHARPSHOOTING || skill_num == NJ_KIRIKAGE) {
				// DEF無視フラグ
				calc_flag.idef = calc_flag.idef_ = 1;
			} else {
				wd.type = 0x0a;	// クリティカル攻撃
			}
		}
	}

	/* ７．ヒット・属性・レンジ修正 */
	if(wd.type == 0) {
		// 矢があるならヒットを加算
		if(src_sd && src_sd->state.arrow_atk) {
			calc_flag.hitrate += src_sd->arrow_hit;
		}
		if(skill_num > 0) {
			wd.flag = (wd.flag&~BF_SKILLMASK)|BF_SKILL;
			if( (i = skill_get_pl(skill_num)) > 0 && (!src_sd || !src_sd->arrow_ele) )
				s_ele = s_ele_ = i;
		}
		switch( skill_num ) {
		case SM_BASH:			// バッシュ
		case KN_PIERCE:			// ピアース
			calc_flag.hitrate = calc_flag.hitrate*(100+5*skill_lv)/100;
			break;
		case SM_MAGNUM:			// マグナムブレイク
			calc_flag.hitrate = calc_flag.hitrate*(100+10*skill_lv)/100;
			break;
		case KN_AUTOCOUNTER:		// オートカウンター
			wd.flag = (wd.flag&~BF_SKILLMASK)|BF_NORMAL;
			if(battle_config.pc_auto_counter_type&1)
				calc_flag.hitrate += 20;
			else
				calc_flag.hitrate = 1000000;
			break;
		case AS_SONICBLOW:		// ソニックブロウ
			if(src_sd && pc_checkskill(src_sd,AS_SONICACCEL) > 0)
				calc_flag.hitrate = calc_flag.hitrate*150/100;
			break;
		case CR_SHIELDBOOMERANG:	// シールドブーメラン
			if(sc_data && sc_data[SC_CRUSADER].timer != -1)
				calc_flag.hitrate = 1000000;
			calc_flag.dist = 1;
			s_ele = s_ele_ = ELE_NEUTRAL;
			break;
		case AM_ACIDTERROR:		// アシッドテラー
			calc_flag.hitrate = 1000000;
			calc_flag.dist = 1;
			s_ele = s_ele_ = ELE_NEUTRAL;
			break;
		case NPC_CRITICALSLASH:		// 防御無視攻撃
		case NPC_GUIDEDATTACK:		// 必中攻撃
		case MO_INVESTIGATE:		// 発勁
		case MO_EXTREMITYFIST:		// 阿修羅覇鳳拳
		case CR_ACIDDEMONSTRATION:	// アシッドデモンストレーション
		case NJ_ISSEN:			// 一閃
			calc_flag.hitrate = 1000000;
			s_ele = s_ele_ = ELE_NEUTRAL;
			break;
		case HVAN_EXPLOSION:		// バイオエクスプロージョン
		case RG_BACKSTAP:		// バックスタブ
		case CR_GRANDCROSS:		// グランドクロス
		case NPC_DARKGRANDCROSS:	// グランドダークネス
		case AM_DEMONSTRATION:		// デモンストレーション
		case TK_COUNTER:		// アプチャオルリギ
		case AS_SPLASHER:		// ベナムスプラッシャー
			calc_flag.hitrate = 1000000;
			break;
		case GS_TRACKING:		// トラッキング
			calc_flag.hitrate = calc_flag.hitrate*4+5;
			calc_flag.dist = 1;
			break;
		case AC_DOUBLE:			// ダブルストレイフィング
		case HT_POWER:			// ビーストストレイフィング
		case AC_SHOWER:			// アローシャワー
		case AC_CHARGEARROW:		// チャージアロー
		case HT_PHANTASMIC:		// ファンタスミックアロー
		case KN_SPEARSTAB:		// スピアスタブ
		case KN_SPEARBOOMERANG:		// スピアブーメラン
		case AS_GRIMTOOTH:		// グリムトゥース
		case MO_FINGEROFFENSIVE:	// 指弾
		case LK_SPIRALPIERCE:		// スパイラルピアース
		case HW_MAGICCRASHER:		// マジッククラッシャー
		case ASC_BREAKER:		// ソウルブレイカー
		case SN_SHARPSHOOTING:		// シャープシューティング
		case ITM_TOMAHAWK:		// トマホーク投げ
		case GS_FLING:			// フライング
		case GS_TRIPLEACTION:		// トリプルアクション
		case GS_BULLSEYE:		// ブルズアイ
		case GS_MAGICALBULLET:		// マジカルバレット
		case GS_DISARM:			// ディスアーム
		case GS_PIERCINGSHOT:		// ピアーシングショット
		case GS_RAPIDSHOWER:		// ラピッドシャワー
		case GS_DUST:			// ダスト
		case GS_FULLBUSTER:		// フルバスター
		case GS_SPREADATTACK:		// スプレッドアタック
		case NJ_HUUMA:			// 風魔手裏剣投げ
		case NJ_TATAMIGAESHI:		// 畳返し
			calc_flag.dist = 1;
			break;
		case AS_VENOMKNIFE:		// ベナムナイフ
			calc_flag.dist = 1;
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				if(src_sd->arrow_ele > 0)	// 属性矢なら属性を矢の属性に変更
					s_ele = src_sd->arrow_ele;
			}
			break;
		case NPC_COMBOATTACK:		// 多段攻撃
		case NPC_RANDOMATTACK:		// ランダムATK攻撃
			s_ele = s_ele_ = ELE_NEUTRAL;
			break;
		case NPC_RANGEATTACK:		// 遠距離攻撃
		case NJ_ZENYNAGE:		// 銭投げ
			calc_flag.dist = 1;
			s_ele = s_ele_ = ELE_NEUTRAL;
			break;
		case PA_SHIELDCHAIN:		// シールドチェイン
			calc_flag.hitrate += 20;
			calc_flag.dist = 1;
			s_ele = s_ele_ = ELE_NEUTRAL;
			break;
		case NPC_PIERCINGATT:		// 突き刺し攻撃
		case CR_SHIELDCHARGE:		// シールドチャージ
			wd.flag = (wd.flag&~BF_RANGEMASK)|BF_SHORT;
			s_ele = s_ele_ = ELE_NEUTRAL;
			break;
		case BA_MUSICALSTRIKE:		// ミュージカルストライク
		case DC_THROWARROW:		// 矢撃ち
		case CG_ARROWVULCAN:		// アローバルカン
			calc_flag.dist = 1;
			if(src_sd)
				s_ele = src_sd->arrow_ele;
			break;
		case NJ_SYURIKEN:		// 手裏剣投げ
		case NJ_KUNAI:			// 苦無投げ
			calc_flag.dist = 1;
			if(src_sd && src_sd->arrow_ele > 0)	// 属性矢なら属性を矢の属性に変更
				s_ele = src_sd->arrow_ele;
			break;
		}

		// ここから距離による判定
		if(calc_flag.dist) {				// 距離によってレンジが変化するスキルか
			if(battle_config.calc_dist_flag&1 && src->type != BL_PC && target->type != BL_PC) {	// PC vs PCは強制無視
				int target_dist = unit_distance2(src,target)-1;	// 距離を取得
				if(target_dist >= battle_config.allow_sw_dist) {				// SWで防げる距離より多い＝遠距離からの攻撃
					if(battle_config.sw_def_type & 1 && src->type == BL_PC)		// 人間からのを判定するか　&1でする
						wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;		// 遠距離に設定
					if(battle_config.sw_def_type & 2 && src->type == BL_MOB)	// モンスターからのを判定するか　&2でする
						wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;		// 遠距離に設定
				}
			} else {	// 本来遠距離のスキルで使用者と許可フラグが全て一致しないから遠距離攻撃だ
				wd.flag = (wd.flag&~BF_RANGEMASK)|BF_LONG;	// 遠距離に設定
			}
		}
	}
	// サクリファイス
	if(sc_data && sc_data[SC_SACRIFICE].timer != -1 && !skill_num && t_class != 1288) {
		calc_flag.hitrate = 1000000;
		s_ele = s_ele_ = ELE_NEUTRAL;
	}
	// カード効果による必中ボーナス
	if(src_sd && src_sd->perfect_hit > 0) {
		if(atn_rand()%100 < src_sd->perfect_hit)
			calc_flag.hitrate = 1000000;
	}
	// 対象が状態異常中の場合の必中ボーナス
	if(calc_flag.hitrate < 1000000 && t_sc_data) {
		if( t_sc_data[SC_SLEEP].timer != -1 ||
		    t_sc_data[SC_STAN].timer != -1 ||
		    t_sc_data[SC_FREEZE].timer != -1 ||
		    (t_sc_data[SC_STONE].timer != -1 && t_sc_data[SC_STONE].val2 == 0) ) {
			calc_flag.hitrate = 1000000;
		}
	}
	if(calc_flag.hitrate < battle_config.min_hitrate)
		calc_flag.hitrate = battle_config.min_hitrate;

	/* ８．回避判定 */
	if(wd.type == 0 && atn_rand()%100 >= calc_flag.hitrate) {
		wd.dmg_lv = ATK_FLEE;
	}
	else if(wd.type == 0 && t_sc_data && t_sc_data[SC_KAUPE].timer != -1 && atn_rand()%100 < t_sc_data[SC_KAUPE].val2) {	// カウプ
		wd.dmg_lv = ATK_FLEE;
		status_change_end(target,SC_KAUPE,-1);	// カウプ終了処理
	}
	else if(wd.type == 0 && t_sc_data && t_sc_data[SC_UTSUSEMI].timer != -1) {	// 空蝉
		wd.dmg_lv = ATK_FLEE;
		clif_misceffect2(target,463);
		if(--t_sc_data[SC_UTSUSEMI].val3 == 0)
			status_change_end(target,SC_UTSUSEMI,-1);
		if(t_sc_data[SC_ANKLE].timer == -1) {
			int dir = 0, head_dir = 0;
			int count = skill_get_blewcount(NJ_UTSUSEMI,t_sc_data[SC_UTSUSEMI].val1);
			if(target_sd) {
				dir = target_sd->dir;
				head_dir = target_sd->head_dir;
			}
			unit_stop_walking(target,1);
			skill_blown(src,target,count|SAB_NODAMAGE|SAB_NOPATHSTOP);
			if(target_sd)
				pc_setdir(target_sd, dir, head_dir);
			if(t_sc_data[SC_CLOSECONFINE].timer != -1)
				status_change_end(target,SC_CLOSECONFINE,-1);
		}
	}
	else if(wd.type == 0 && t_sc_data && t_sc_data[SC_BUNSINJYUTSU].timer != -1) {	// 影分身
		wd.dmg_lv = ATK_FLEE;
		if(--t_sc_data[SC_BUNSINJYUTSU].val3 == 0)
			status_change_end(target,SC_BUNSINJYUTSU,-1);
	}
	else if(target_sd && t_sc_data && t_sc_data[SC_DODGE].timer != -1 && (wd.flag&BF_LONG || t_sc_data[SC_SPURT].timer != -1) && atn_rand()%100 < 20) {	// 落法
		int slv = pc_checkskill(target_sd,TK_DODGE);
		wd.dmg_lv = ATK_FLEE;
		clif_skill_nodamage(&target_sd->bl,&target_sd->bl,TK_DODGE,slv,1);
		status_change_start(&target_sd->bl,SC_DODGE_DELAY,slv,src->id,0,0,skill_get_time(TK_DODGE,slv),0);
	}
	else {
		int damage_ot = 0, damage_ot2 = 0;
		int tk_power_damage = 0, tk_power_damage2 = 0;

		// 回避できなかったときのみstep9〜18のダメージ計算を行う
		wd.dmg_lv = ATK_DEF;

		/* ９．基本ダメージの算出 */
		wd.damage = battle_calc_base_damage(src, target, skill_num, wd.type, 0);
		if(calc_flag.lh)
			wd.damage2 = battle_calc_base_damage(src, target, skill_num, wd.type, 1);

		if(wd.type == 0) {	// クリティカルでないとき矢のダメージを加算
			if(src_sd && src_sd->state.arrow_atk && src_sd->arrow_atk > 0)
				wd.damage += atn_rand()%(src_sd->arrow_atk+1);
		}

		/* 10．ファイティング計算 */
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

		/* 11．オーバートラスト系のスキル倍率計算前の攻撃力確保 */
		damage_ot += wd.damage;
		if(calc_flag.lh)
			damage_ot2 += wd.damage2;

		/* 12．スキル修正１（攻撃力倍加系）*/
		switch( skill_num ) {
		case SM_BASH:		// バッシュ
			DMG_FIX( 100+30*skill_lv, 100 );
			break;
		case SM_MAGNUM:		// マグナムブレイク
			if(!wflag) {	// 内周
				DMG_FIX( 100+20*skill_lv, 100 );
			} else {	// 外周
				DMG_FIX( 100+10*skill_lv, 100 );
			}
			break;
		case MC_MAMMONITE:	// メマーナイト
			DMG_FIX( 100+50*skill_lv, 100 );
			break;
		case AC_DOUBLE:		// ダブルストレイフィング
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 100+20*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case HT_POWER:		// ビーストストレイフィング
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 100+16*s_str, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case AC_SHOWER:		// アローシャワー
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 75+5*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			wd.blewcount = 0;
			break;
		case AC_CHARGEARROW:	// チャージアロー
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 150, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case HT_PHANTASMIC:	// ファンタスミックアロー
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 150, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case KN_CHARGEATK:	// チャージアタック
			{
				int dist = unit_distance2(src,target)-1;
				if(dist > 2)
					DMG_FIX( 100+100*(dist/3), 100 );
			}
			break;
		case AS_VENOMKNIFE:	// ベナムナイフ
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			calc_flag.nocardfix = 1;
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case SG_SUN_WARM:	// 太陽の温もり
		case SG_MOON_WARM:	// 月の温もり
		case SG_STAR_WARM:	// 星の温もり
			if(src_sd) {
				if(src_sd->status.sp < 2) {
					status_change_end(src,SkillStatusChangeTable[skill_num],-1);
					break;
				}
				// 殴ったのでSP消費
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
		case KN_PIERCE:		// ピアース
			wd.div_ = t_size+1;
			DMG_FIX( (100+10*skill_lv) * wd.div_, 100 );
			break;
		case KN_SPEARSTAB:	// スピアスタブ
			DMG_FIX( 100+15*skill_lv, 100 );
			wd.blewcount = 0;
			break;
		case KN_SPEARBOOMERANG:	// スピアブーメラン
			DMG_FIX( 100+50*skill_lv, 100 );
			break;
		case KN_BRANDISHSPEAR:	// ブランディッシュスピア
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
		case KN_BOWLINGBASH:	// ボウリングバッシュ
			DMG_FIX( 100+40*skill_lv, 100 );
			wd.blewcount = 0;
			break;
		case AS_SONICBLOW:	// ソニックブロウ
			{
				int rate = 300+50*skill_lv;
				if(src_sd && pc_checkskill(src_sd,AS_SONICACCEL) > 0)
					rate = rate*110/100;
				if(sc_data && sc_data[SC_ASSASIN].timer != -1)
				{
					if(map[src->m].flag.gvg)
						rate = rate*125/100;
					else
						rate *= 2;
				}
				DMG_FIX( rate, 100 );
			}
			break;
		case AS_GRIMTOOTH:	// グリムトゥース
			DMG_FIX( 100+20*skill_lv, 100 );
			break;
		case TF_SPRINKLESAND:	// 砂まき
			DMG_FIX( 130, 100 );
			break;
		case MC_CARTREVOLUTION:	// カートレボリューション
			if(src_sd && src_sd->cart_max_weight > 0 && src_sd->cart_weight > 0) {
				DMG_FIX( 150 + pc_checkskill(src_sd,BS_WEAPONRESEARCH) + src_sd->cart_weight*100/src_sd->cart_max_weight, 100 );
			} else {
				DMG_FIX( 150, 100 );
			}
			wd.blewcount = 0;
			break;
		case NPC_COMBOATTACK:	// 多段攻撃
			DMG_FIX( 50*wd.div_, 100 );
			break;
		case NPC_RANDOMATTACK:	// ランダムATK攻撃
			DMG_FIX( 50+atn_rand()%150, 100 );
			break;
		// 属性攻撃
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
		case RG_BACKSTAP:	// バックスタブ
			{
				int rate = 300+40*skill_lv;
				if(src_sd && src_sd->status.weapon == WT_BOW) {	// 弓なら半減
					rate /= 2;
				}
				DMG_FIX( rate, 100 );
			}
			break;
		case RG_RAID:		// サプライズアタック
			DMG_FIX( 100+40*skill_lv, 100 );
			break;
		case RG_INTIMIDATE:	// インティミデイト
			DMG_FIX( 100+30*skill_lv, 100 );
			break;
		case CR_SHIELDCHARGE:	// シールドチャージ
			DMG_FIX( 100+20*skill_lv, 100 );
			break;
		case CR_SHIELDBOOMERANG:	// シールドブーメラン
			{
				int rate = 100+30*skill_lv;
				if(sc_data && sc_data[SC_CRUSADER].timer != -1)
					rate *= 2;
				DMG_FIX( rate, 100 );
			}
			break;
		case CR_HOLYCROSS:	// ホーリークロス
		case NPC_DARKCROSS:	// ダーククロス
			DMG_FIX( 100+35*skill_lv, 100 );
			break;
		case CR_GRANDCROSS:		// グランドクロス
		case NPC_DARKGRANDCROSS:	// グランドダークネス
			if (!battle_config.gx_cardfix)
				calc_flag.nocardfix = 1;
			break;
		case AM_DEMONSTRATION:	// デモンストレーション
			DMG_FIX( 100+20*skill_lv, 100 );
			calc_flag.nocardfix = 1;
			break;
		case AM_ACIDTERROR:	// アシッドテラー
			DMG_FIX( 100+40*skill_lv, 100 );
			calc_flag.nocardfix = 1;
			break;
		case MO_FINGEROFFENSIVE:	// 指弾
			if(src_sd && battle_config.finger_offensive_type == 0) {
				wd.div_ = src_sd->spiritball_old;
				DMG_FIX( (100+50*skill_lv) * wd.div_, 100 );
			} else {
				wd.div_ = 1;
				DMG_FIX( 100+50*skill_lv, 100 );
			}
			break;
		case MO_INVESTIGATE:	// 発勁
			if(t_def1 < 1000000) {
				DMG_FIX( (100+75*skill_lv) * (t_def1 + t_def2), 100*50 );
			}
			break;
		case MO_BALKYOUNG:
			DMG_FIX( 300, 100 );
			break;
		case MO_EXTREMITYFIST:	// 阿修羅覇鳳拳
			if(src_sd) {
				DMG_FIX( 8+src_sd->status.sp/10, 1 );
				src_sd->status.sp = 0;
				clif_updatestatus(src_sd,SP_SP);
			} else {
				DMG_FIX( 8, 1 );
			}
			DMG_ADD( 250+150*skill_lv );
			break;
		case MO_CHAINCOMBO:	// 連打掌
			DMG_FIX( 150+50*skill_lv, 100 );
			break;
		case MO_COMBOFINISH:	// 猛龍拳
			DMG_FIX( 240+60*skill_lv, 100 );
			// PTには入っている
			// カウンターアタックの確率上昇
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
		case TK_STORMKICK:	// 旋風蹴り
			DMG_FIX( 160+20*skill_lv, 100 );
			break;
		case TK_DOWNKICK:	// 下段蹴り
			DMG_FIX( 160+20*skill_lv, 100 );
			break;
		case TK_TURNKICK:	// 回転蹴り
			DMG_FIX( 190+30*skill_lv, 100 );
			wd.blewcount = 0;
			break;
		case TK_COUNTER:	// カウンター蹴り
			DMG_FIX( 190+30*skill_lv, 100 );
			// PTには入っている
			// 三段掌の確率上昇
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
		case BA_MUSICALSTRIKE:	// ミュージカルストライク
		case DC_THROWARROW:	// 矢撃ち
			DMG_FIX( 60+40*skill_lv, 100 );
			break;
		case CH_TIGERFIST:	// 伏虎拳
			DMG_FIX( 40+100*skill_lv, 100 );
			break;
		case CH_CHAINCRUSH:	// 連柱崩撃
			DMG_FIX( 400+100*skill_lv, 100 );
			break;
		case CH_PALMSTRIKE:	// 猛虎硬派山
			DMG_FIX( 200+100*skill_lv, 100 );
			break;
		case LK_SPIRALPIERCE:	// スパイラルピアース
			DMG_FIX( 80+40*skill_lv, 100 );
			break;
		case LK_HEADCRUSH:	// ヘッドクラッシュ
			DMG_FIX( 100+40*skill_lv, 100 );
			break;
		case LK_JOINTBEAT:	// ジョイントビート
			DMG_FIX( 50+10*skill_lv, 100 );
			break;
		case ASC_METEORASSAULT:	// メテオアサルト
			DMG_FIX( 40+40*skill_lv, 100 );
			calc_flag.nocardfix = 1;
			break;
		case ASC_BREAKER:	// ソウルブレイカー
			DMG_FIX( skill_lv, 1 );
			calc_flag.nocardfix = 1;
			break;
		case SN_SHARPSHOOTING:	// シャープシューティング
			DMG_FIX( 200+50*skill_lv, 100 );
			break;
		case CG_ARROWVULCAN:	// アローバルカン
			DMG_FIX( 200+100*skill_lv, 100 );
			break;
		case AS_SPLASHER:	// ベナムスプラッシャー
			if(src_sd) {
				DMG_FIX( 500+50*skill_lv+20*pc_checkskill(src_sd,AS_POISONREACT), 100 );
			} else {
				DMG_FIX( 500+50*skill_lv, 100 );
			}
			calc_flag.nocardfix = 1;
			break;
		case AS_POISONREACT:	// ポイズンリアクト（攻撃で反撃）
			wd.damage = wd.damage*(100+30*skill_lv)/100;
			//wd.damage2 = wd.damage2	// 左手には乗らない
			break;
		case TK_JUMPKICK:	// 飛び蹴り
			if(sc_data && (sc_data[SC_RUN].timer != -1 || sc_data[SC_DODGE_DELAY].timer != -1)) {
				DMG_FIX( 30 + (10+status_get_lv(src)/10)*skill_lv, 100 );
				if(sc_data[SC_DODGE_DELAY].timer != -1)
					status_change_end(src,SC_DODGE_DELAY,-1);
			} else {
				DMG_FIX( 30+10*skill_lv, 100 );
			}
			if(src_sd && sc_data && sc_data[SC_RUN].timer != -1 && sc_data[SC_SPURT].timer != -1) {
				// タイリギ中で且つスパート状態なら威力さらにアップ
				// 計算式不明なので適当
				DMG_ADD( 10*pc_checkskill(src_sd,TK_RUN) );
			}
			// ティオアプチャギによる対象のステータス異常解除
			if(target_sd || target_md) {
				// ソウルリンカーは無視
				if(target_sd && target_sd->status.class_ == PC_CLASS_SL)
					break;
				// プリザーブ中は切れない
				if(t_sc_data && t_sc_data[SC_PRESERVE].timer != -1)
					break;
				status_change_release(target,0x10);
			}
			break;
		case PA_SHIELDCHAIN:	// シールドチェイン
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
		case WS_CARTTERMINATION:	// カートターミネーション
			{
				int rate = (skill_lv >= 16)? 1: 16-skill_lv;
				if(src_sd && src_sd->cart_max_weight > 0 && src_sd->cart_weight > 0) {
					DMG_FIX( src_sd->cart_max_weight, rate*100 );
					DMG_FIX( 8000, src_sd->cart_max_weight );
				} else if(!src_sd) {
					DMG_FIX( 80, rate );
				}
			}
			calc_flag.nocardfix = 1;
			break;
		case CR_ACIDDEMONSTRATION:	// アシッドデモンストレーション
			{
				int s_int = status_get_int(src);
				atn_bignumber dmg = (atn_bignumber)s_int * s_int * t_vit * skill_lv * 7 / 10 / (s_int + t_vit);
				if(target->type != BL_MOB)
					dmg /= 2;
				DMG_SET( (int)dmg );
			}
			calc_flag.nocardfix = 1;
			break;
		case GS_TRIPLEACTION:	// トリプルアクション
			DMG_FIX( 450, 100 );
			break;
		case GS_BULLSEYE:	// ブルズアイ
			DMG_FIX( 500, 100 );
			calc_flag.nocardfix = 1;
			break;
		case GS_MAGICALBULLET:	// マジカルバレット
			{
				int matk1 = status_get_matk1(src);
				int matk2 = status_get_matk2(src);
				if(matk1 > matk2)
					wd.damage += matk2+atn_rand()%(matk1-matk2+1);
				else
					wd.damage += matk2;
			}
			break;
		case GS_TRACKING:	// トラッキング
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 200+100*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_DISARM:		// ディスアーム
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_PIERCINGSHOT:	// ピアーシングショット
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 100+20*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_RAPIDSHOWER:	// ラピッドシャワー
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 500+50*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_DESPERADO:	// デスペラード
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 50+50*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_DUST:		// ダスト
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 100+50*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_FULLBUSTER:	// フルバスター
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 300+100*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case GS_SPREADATTACK:	// スプレッドアタック
			if(src_sd && !src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
				int arr = atn_rand()%(src_sd->arrow_atk+1);
				DMG_ADD( arr );
			}
			DMG_FIX( 80+20*skill_lv, 100 );
			if(src_sd)
				src_sd->state.arrow_atk = 1;
			break;
		case NJ_SYURIKEN:	// 手裏剣投げ
			if(src_sd) {
				if(!src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					DMG_ADD( arr );
				}
				src_sd->state.arrow_atk = 1;
			}
			DMG_ADD( skill_lv*3 );
			break;
		case NJ_KUNAI:		// 苦無投げ
			if(src_sd) {
				if(!src_sd->state.arrow_atk && src_sd->arrow_atk > 0) {
					int arr = atn_rand()%(src_sd->arrow_atk+1);
					DMG_ADD( arr );
				}
				src_sd->state.arrow_atk = 1;
			}
			DMG_FIX( 300, 100 );
			break;
		case NJ_HUUMA:		// 風魔手裏剣投げ
			{
				int rate = 150+150*skill_lv;
				if(wflag > 1)
					rate /= wflag;
				DMG_FIX( rate, 100 );
			}
			break;
		case NJ_ZENYNAGE:	// 銭投げ
			{
				int dmg = 0;
				if(src_sd) {
					dmg = src_sd->zenynage_damage;
					src_sd->zenynage_damage = 0;	// 撃ったらリセット
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
		case NJ_TATAMIGAESHI:	// 畳返し
			DMG_FIX( 100+10*skill_lv, 100 );
			break;
		case NJ_KASUMIKIRI:	// 霞斬り
			DMG_FIX( 100+10*skill_lv, 100 );
			break;
		case NJ_KIRIKAGE:	// 影斬り
			DMG_FIX( skill_lv, 1 );
			break;
		case NJ_ISSEN:		// 一閃
			{
				int hp = status_get_hp(src);
				DMG_SET( (s_str*40)+(skill_lv*(hp-1)*8)/100 );
				unit_heal(src,-hp+1,0);
				if(sc_data && sc_data[SC_NEN].timer != -1)
					status_change_end(src,SC_NEN,-1);
			}
			break;
		case HFLI_MOON:		// ムーンライト
			DMG_FIX( 110+110*skill_lv, 100 );
			break;
		case HFLI_SBR44:	// S.B.R.44
			if(src_hd) {
				DMG_SET( src_hd->intimate*skill_lv );
				src_hd->intimate = 200;
				if(battle_config.homun_skill_intimate_type)
					src_hd->status.intimate = 200;
				clif_send_homdata(src_hd->msd,0x100,src_hd->intimate/100);
			}
			break;
		}

		/* 13．ファイティングの追加ダメージ */
		wd.damage += tk_power_damage;
		if(calc_flag.lh)
			wd.damage2 += tk_power_damage2;

		if(calc_flag.da == 2) {	// 三段掌が発動している
			if(src_sd)
				wd.damage = wd.damage * (100 + 20 * pc_checkskill(src_sd, MO_TRIPLEATTACK)) / 100;
		}

		/* 14．防御無視判定および錐効果ダメージ計算 */
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
			if( wd.type != 0 )	// クリティカル時は無効
				break;
			if( skill_num == WS_CARTTERMINATION && !battle_config.def_ratio_atk_to_carttermination )
				break;
			if( skill_num == PA_SHIELDCHAIN && !battle_config.def_ratio_atk_to_shieldchain )
				break;
			if(src_sd && t_def1 < 1000000)
			{
				int mask = (1<<t_race) | ( (t_mode&0x20)? (1<<10): (1<<11) );

				// bIgnoreDef系
				if( !calc_flag.idef && (src_sd->ignore_def_ele & (1<<t_ele) || src_sd->ignore_def_race & mask || src_sd->ignore_def_enemy & (1<<t_enemy)) )
					calc_flag.idef = 1;
				if( calc_flag.lh ) {
					if( !calc_flag.idef_ && (src_sd->ignore_def_ele_ & (1<<t_ele) || src_sd->ignore_def_race_ & mask || src_sd->ignore_def_enemy_ & (1<<t_enemy)) ) {
						calc_flag.idef_ = 1;
						if(battle_config.left_cardfix_to_right)
							calc_flag.idef = 1;
					}
				}
				// bDefRatioATK系
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

		/* 15．対象の防御力によるダメージの減少 */
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
			if(wd.type != 0)	// クリティカル時は無効
				break;
			// 太陽と月と星の融合 DEF無視
			if(sc_data && sc_data[SC_FUSION].timer != -1)
				calc_flag.idef = 1;

			// DEF無視フラグがないとき
			if( ((calc_flag.rh && !calc_flag.idef) || (calc_flag.lh && !calc_flag.idef_)) && t_def1 < 1000000 )
			{
				int t_def, vitbonusmax;
				int target_count = 1;

				if(target->type != BL_HOM) {
					target_count = unit_counttargeted(target,battle_config.vit_penalty_count_lv);
				}
				if(battle_config.vit_penalty_type > 0 && (!t_sc_data || t_sc_data[SC_STEELBODY].timer == -1)) {
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

		/* 16．状態異常中のダメージ追加でクリティカルにも有効なスキル */
		if (sc_data) {
			// オーバートラスト
			if(sc_data[SC_OVERTHRUST].timer != -1) {
				wd.damage += damage_ot*(5*sc_data[SC_OVERTHRUST].val1)/100;
				if(calc_flag.lh)
					wd.damage2 += damage_ot2*(5*sc_data[SC_OVERTHRUST].val1)/100;
			}
			// オーバートラストマックス
			if(sc_data[SC_OVERTHRUSTMAX].timer != -1) {
				wd.damage += damage_ot*(20*sc_data[SC_OVERTHRUSTMAX].val1)/100;
				if(calc_flag.lh)
					wd.damage2 += damage_ot2*(20*sc_data[SC_OVERTHRUSTMAX].val1)/100;
			}
			// トゥルーサイト
			if(sc_data[SC_TRUESIGHT].timer != -1) {
				DMG_FIX( 100+2*sc_data[SC_TRUESIGHT].val1, 100 );
			}
			// バーサーク
			if(sc_data[SC_BERSERK].timer != -1) {
				DMG_FIX( 200, 100 );
			}
			// エンチャントデッドリーポイズン
			if(sc_data[SC_EDP].timer != -1 && !calc_flag.nocardfix) {
				// 右手のみに効果がのる。カード効果無効のスキルには乗らない
				if(map[src->m].flag.pk && target->type == BL_PC) {
					wd.damage += wd.damage * (150 + sc_data[SC_EDP].val1 * 50) * battle_config.pk_edp_down_rate / 10000;
				} else if(map[src->m].flag.gvg) {
					wd.damage += wd.damage * (150 + sc_data[SC_EDP].val1 * 50) * battle_config.gvg_edp_down_rate / 10000;
				} else if(map[src->m].flag.pvp) {
					wd.damage += wd.damage * (150 + sc_data[SC_EDP].val1 * 50) * battle_config.pvp_edp_down_rate / 10000;
				} else {
					wd.damage += wd.damage * (150 + sc_data[SC_EDP].val1 * 50) / 100;
				}
				// calc_flag.nocardfix = 1;
			}
			// サクリファイス
			if(sc_data[SC_SACRIFICE].timer != -1 && !skill_num && t_class != 1288) {
				int dmg = status_get_max_hp(src) * 9 / 100;
				battle_heal(NULL, src, -dmg, 0, 0);
				wd.damage = dmg * (90 + sc_data[SC_SACRIFICE].val1 * 10) / 100;
				wd.damage2 = 0;
				clif_misceffect2(src,366);
				if((--sc_data[SC_SACRIFICE].val2) <= 0)
					status_change_end(src, SC_SACRIFICE,-1);
			}
		}

		/* 17．精錬ダメージの追加 */
		if( src_sd ) {
			if(skill_num != MO_INVESTIGATE && skill_num != MO_EXTREMITYFIST && skill_num != PA_SHIELDCHAIN && skill_num != CR_ACIDDEMONSTRATION && skill_num != NJ_ZENYNAGE) {
				wd.damage += status_get_atk2(src);
				if(calc_flag.lh)
					wd.damage2 += status_get_atk_2(src);
			}
			switch (skill_num) {
			case CR_SHIELDBOOMERANG:	// シールドブーメラン
				if(src_sd->equip_index[8] >= 0) {
					int idx = src_sd->equip_index[8];
					if(src_sd->inventory_data[idx] && src_sd->inventory_data[idx]->type == 5) {
						wd.damage += src_sd->inventory_data[idx]->weight/10;
						wd.damage += src_sd->status.inventory[idx].refine * status_get_overrefine_bonus(0);
					}
				}
				break;
			case LK_SPIRALPIERCE:		// スパイラルピアース
				if(src_sd->equip_index[9] >= 0) {	// {((STR/10)^2 ＋ 武器重量×スキル倍率×0.8) × サイズ補正 ＋ 精錬}×カード倍率×属性倍率×5の模様
					int idx = src_sd->equip_index[9];
					if(src_sd->inventory_data[idx] && src_sd->inventory_data[idx]->type == 4) {
						wd.damage = ( ( (s_str/10)*(s_str/10) + src_sd->inventory_data[idx]->weight * (skill_lv * 4 + 8 ) / 100 )
									* (5 - t_size) / 4 + status_get_atk2(src) ) * 5;
					}
				}
				break;
			case PA_SHIELDCHAIN:		// シールドチェイン
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
			case NJ_SYURIKEN:		// 手裏剣投げ
				wd.damage += pc_checkskill(src_sd,NJ_TOBIDOUGU) * 3;
				break;
			}
		}

		// 0未満だった場合1に補正
		if(wd.damage  < 1) wd.damage  = 1;
		if(wd.damage2 < 1) wd.damage2 = 1;

		/* 18．スキル修正２（修練系）*/
		// 修練ダメージ(右手のみ) ソニックブロー時は別処理（1撃に付き1/8適応)
		if( src_sd &&
		    skill_num != MO_INVESTIGATE &&
		    skill_num != MO_EXTREMITYFIST &&
		    skill_num != CR_GRANDCROSS &&
		    skill_num != NPC_DARKGRANDCROSS &&
		    skill_num != LK_SPIRALPIERCE &&
		    skill_num != CR_ACIDDEMONSTRATION &&
		    skill_num != NJ_ZENYNAGE )
		{
			wd.damage = battle_addmastery(src_sd,target,wd.damage,0);
			if(calc_flag.lh)
				wd.damage2 = battle_addmastery(src_sd,target,wd.damage2,1);
		}
		if(sc_data) {
			if(sc_data[SC_AURABLADE].timer != -1)		// オーラブレード
				DMG_ADD( sc_data[SC_AURABLADE].val1*20 );
			if(sc_data[SC_GATLINGFEVER].timer != -1)	// ガトリングフィーバー
				DMG_ADD( 20+sc_data[SC_GATLINGFEVER].val1*10 );
		}
	}

	/* 19．スキル修正３（必中ダメージ）*/
	if( src_sd && (skill = pc_checkskill(src_sd,BS_WEAPONRESEARCH)) > 0) {
		DMG_ADD( skill*2 );
	}
	if( src_sd && (skill = pc_checkskill(src_sd,TK_RUN)) > 0) {	// タイリギパッシブで蹴りの威力加算
		if( (skill_num == TK_DOWNKICK || skill_num == TK_STORMKICK || skill_num == TK_TURNKICK || skill_num == TK_COUNTER) &&
		    src_sd->weapontype1 == WT_FIST && src_sd->weapontype2 == WT_FIST ) {
			DMG_ADD( skill*10 );
		}
	}

	/* 20．カードによるダメージ追加処理 */
	if( src_sd && wd.damage > 0 && calc_flag.rh && !calc_flag.nocardfix ) {
		cardfix = 100;
		if(!src_sd->state.arrow_atk) {	// 弓矢以外
			if(!battle_config.left_cardfix_to_right) {	// 左手カード補正設定無し
				cardfix = cardfix*(100+src_sd->addrace[t_race])/100;	// 種族によるダメージ修正
				cardfix = cardfix*(100+src_sd->addele[t_ele])/100;	// 属性によるダメージ修正
				cardfix = cardfix*(100+src_sd->addenemy[t_enemy])/100;	// 敵タイプによるダメージ修正
				cardfix = cardfix*(100+src_sd->addsize[t_size])/100;	// サイズによるダメージ修正
				cardfix = cardfix*(100+src_sd->addgroup[t_group])/100;	// グループによるダメージ修正
			} else {
				cardfix = cardfix*(100+src_sd->addrace[t_race]+src_sd->addrace_[t_race])/100;		// 種族によるダメージ修正(左手による追加あり)
				cardfix = cardfix*(100+src_sd->addele[t_ele]+src_sd->addele_[t_ele])/100;		// 属性によるダメージ修正(左手による追加あり)
				cardfix = cardfix*(100+src_sd->addenemy[t_enemy]+src_sd->addenemy_[t_enemy])/100;	// 敵タイプによるダメージ修正(左手による追加あり)
				cardfix = cardfix*(100+src_sd->addsize[t_size]+src_sd->addsize_[t_size])/100;		// サイズによるダメージ修正(左手による追加あり)
				cardfix = cardfix*(100+src_sd->addgroup[t_group]+src_sd->addgroup_[t_group])/100;	// グループによるダメージ修正(左手による追加あり)
			}
		} else { // 弓矢
			cardfix = cardfix*(100+src_sd->addrace[t_race]+src_sd->arrow_addrace[t_race])/100;	// 種族によるダメージ修正(弓矢による追加あり)
			cardfix = cardfix*(100+src_sd->addele[t_ele]+src_sd->arrow_addele[t_ele])/100;		// 属性によるダメージ修正(弓矢による追加あり)
			cardfix = cardfix*(100+src_sd->addenemy[t_enemy]+src_sd->arrow_addenemy[t_enemy])/100;	// 敵タイプによるダメージ修正(弓矢による追加あり)
			cardfix = cardfix*(100+src_sd->addsize[t_size]+src_sd->arrow_addsize[t_size])/100;	// サイズによるダメージ修正(弓矢による追加あり)
			cardfix = cardfix*(100+src_sd->addgroup[t_group]+src_sd->arrow_addgroup[t_group])/100;	// グループによるダメージ修正(弓矢による追加あり)
		}
		if(t_mode & 0x20) {	// ボス
			if(!src_sd->state.arrow_atk) {	// 弓矢攻撃以外なら
				if(!battle_config.left_cardfix_to_right) {
					// 左手カード補正設定無し
					cardfix = cardfix*(100+src_sd->addrace[RCT_BOSS])/100;					// ボスモンスターに追加ダメージ
				} else {
					// 左手カード補正設定あり
					cardfix = cardfix*(100+src_sd->addrace[RCT_BOSS]+src_sd->addrace_[RCT_BOSS])/100;	// ボスモンスターに追加ダメージ(左手による追加あり)
				}
			} else {	// 弓矢攻撃
				cardfix = cardfix*(100+src_sd->addrace[RCT_BOSS]+src_sd->arrow_addrace[RCT_BOSS])/100;		// ボスモンスターに追加ダメージ(弓矢による追加あり)
			}
		} else {		// ボスじゃない
			if(!src_sd->state.arrow_atk) {	// 弓矢攻撃以外
				if(!battle_config.left_cardfix_to_right) {
					// 左手カード補正設定無し
					cardfix = cardfix*(100+src_sd->addrace[RCT_NONBOSS])/100;				// ボス以外モンスターに追加ダメージ
				} else {
					// 左手カード補正設定あり
					cardfix = cardfix*(100+src_sd->addrace[RCT_NONBOSS]+src_sd->addrace_[RCT_NONBOSS])/100;	// ボス以外モンスターに追加ダメージ(左手による追加あり)
				}
			} else {
				cardfix = cardfix*(100+src_sd->addrace[RCT_NONBOSS]+src_sd->arrow_addrace[RCT_NONBOSS])/100;	// ボス以外モンスターに追加ダメージ(弓矢による追加あり)
			}
		}
		// カード効果による特定レンジ攻撃のダメージ増幅
		if(wd.flag&BF_SHORT) {
			cardfix = cardfix * (100+src_sd->short_weapon_damege_rate) / 100;
		}
		if(wd.flag&BF_LONG) {
			cardfix = cardfix * (100+src_sd->long_weapon_damege_rate) / 100;
		}
		// カード効果による特定スキルのダメージ増幅（武器スキル）
		if(src_sd->skill_dmgup.count > 0 && skill_num > 0 && wd.damage > 0) {
			for( i=0; i<src_sd->skill_dmgup.count; i++ ) {
				if( skill_num == src_sd->skill_dmgup.id[i] ) {
					cardfix = cardfix*(100+src_sd->skill_dmgup.rate[i])/100;
					break;
				}
			}
		}
		// 特定Class用補正処理(少女の日記→ボンゴン用？)
		for(i=0; i<src_sd->add_damage_class_count; i++) {
			if(src_sd->add_damage_classid[i] == t_class) {
				cardfix = cardfix*(100+src_sd->add_damage_classrate[i])/100;
				break;
			}
		}
		wd.damage = wd.damage*cardfix/100;	// カード補正によるダメージ増加
	}

	/* 21．カードによる左手ダメージ追加処理 */
	if( src_sd && wd.damage2 > 0 && calc_flag.lh && !calc_flag.nocardfix ) {
		cardfix = 100;
		if(!battle_config.left_cardfix_to_right) {	// 左手カード補正設定無し
			cardfix = cardfix*(100+src_sd->addrace_[t_race])/100;	// 種族によるダメージ修正左手
			cardfix = cardfix*(100+src_sd->addele_[t_ele])/100;	// 属性によるダメージ修正左手
			cardfix = cardfix*(100+src_sd->addenemy_[t_enemy])/100;	// 敵タイプによるダメージ修正左手
			cardfix = cardfix*(100+src_sd->addsize_[t_size])/100;	// サイズによるダメージ修正左手
			cardfix = cardfix*(100+src_sd->addgroup_[t_group])/100;	// グループによるダメージ修正左手
			if(t_mode & 0x20)	// ボス
				cardfix = cardfix*(100+src_sd->addrace_[RCT_BOSS])/100;		// ボスモンスターに追加ダメージ左手
			else
				cardfix = cardfix*(100+src_sd->addrace_[RCT_NONBOSS])/100;	// ボス以外モンスターに追加ダメージ左手
		}
		// 特定Class用補正処理左手(少女の日記→ボンゴン用？)
		for(i=0; i<src_sd->add_damage_class_count_; i++) {
			if(src_sd->add_damage_classid_[i] == t_class) {
				cardfix = cardfix*(100+src_sd->add_damage_classrate_[i])/100;
				break;
			}
		}
		wd.damage2 = wd.damage2*cardfix/100;	// カード補正による左手ダメージ増加
	}

	/* 22．ソウルブレイカーの魔法ダメージ計算 */
	if(skill_num == ASC_BREAKER)
		damage_sbr = status_get_int(src) * skill_lv * 5; 

	/* 23．カードによるダメージ減衰処理 */
	if( target_sd && (wd.damage > 0 || wd.damage2 > 0) ) {	// 対象がPCの場合
		int s_race  = status_get_race(src);
		int s_enemy = status_get_enemy_type(src);
		int s_size  = status_get_size(src);
		int s_group = status_get_group(src);
		cardfix = 100;
		cardfix = cardfix*(100-target_sd->subrace[s_race])/100;			// 種族によるダメージ耐性
		if (s_ele == ELE_NONE)
			cardfix = cardfix*(100-target_sd->subele[ELE_NEUTRAL])/100;	// 属性無しの耐性は無属性
		else
			cardfix = cardfix*(100-target_sd->subele[s_ele])/100;		// 属性によるダメージ耐性
		cardfix = cardfix*(100-target_sd->subenemy[s_enemy])/100;		// 敵タイプによるダメージ耐性
		cardfix = cardfix*(100-target_sd->subsize[s_size])/100;			// サイズによるダメージ耐性
		cardfix = cardfix*(100-target_sd->subgroup[s_group])/100;		// グループによるダメージ耐性

		if(status_get_mode(src) & 0x20)
			cardfix = cardfix*(100-target_sd->subrace[RCT_BOSS])/100;	// ボスからの攻撃はダメージ減少
		else
			cardfix = cardfix*(100-target_sd->subrace[RCT_NONBOSS])/100;	// ボス以外からの攻撃はダメージ減少

		// 特定Class用補正処理左手(少女の日記→ボンゴン用？)
		for(i=0; i<target_sd->add_def_class_count; i++) {
			if(target_sd->add_def_classid[i] == status_get_class(src)) {
				cardfix = cardfix*(100-target_sd->add_def_classrate[i])/100;
				break;
			}
		}
		if(wd.flag&BF_LONG)
			cardfix = cardfix*(100-target_sd->long_attack_def_rate)/100;	// 遠距離攻撃はダメージ減少(ホルンCとか)
		if(wd.flag&BF_SHORT)
			cardfix = cardfix*(100-target_sd->near_attack_def_rate)/100;	// 近距離攻撃はダメージ減少(該当無し？)
		DMG_FIX( cardfix, 100 );	// カード補正によるダメージ減少

		damage_sbr = damage_sbr * cardfix / 100;	// カード補正によるソウルブレイカーの魔法ダメージ減少
	}

	/* 24．アイテムボーナスのフラグ処理 */
	// 状態異常のレンジフラグ
	//   addeff_range_flag  0:指定無し 1:近距離 2:遠距離 3,4:それぞれのレンジで状態異常を発動させない
	//   flagがあり、攻撃タイプとflagが一致しないときは、flag+2する
	if(src_sd && wd.flag&BF_WEAPON) {
		for(i=SC_STONE; i<=SC_BLEED; i++) {
			if( (src_sd->addeff_range_flag[i-SC_STONE] == 1 && wd.flag&BF_LONG ) ||
			    (src_sd->addeff_range_flag[i-SC_STONE] == 2 && wd.flag&BF_SHORT) ) {
				src_sd->addeff_range_flag[i-SC_STONE] += 2;
			}
		}
	}

	/* 25．対象にステータス異常がある場合のダメージ減算処理 */
	if( t_sc_data && (wd.damage > 0 || wd.damage2 > 0) ) {
		cardfix = 100;
		if(t_sc_data[SC_DEFENDER].timer != -1 && wd.flag&BF_LONG && skill_num != CR_ACIDDEMONSTRATION)	// ディフェンダー状態で遠距離攻撃
			cardfix = cardfix*(100-t_sc_data[SC_DEFENDER].val2)/100;
		if(t_sc_data[SC_ADJUSTMENT].timer != -1 && wd.flag&BF_LONG)	// アジャストメント状態で遠距離攻撃
			cardfix -= 20;
		if(cardfix != 100) {
			DMG_FIX( cardfix, 100 );	// ステータス異常補正によるダメージ減少
		}
	}

	if(wd.damage  < 0) wd.damage  = 0;
	if(wd.damage2 < 0) wd.damage2 = 0;

	/* 26．属性の適用 */
	wd.damage = battle_attr_fix(wd.damage, s_ele, status_get_element(target));
	if(calc_flag.lh)
		wd.damage2 = battle_attr_fix(wd.damage2, s_ele_, status_get_element(target));

	/* 27．スキル修正４（追加ダメージ） */
	// マグナムブレイク状態
	if(sc_data && sc_data[SC_MAGNUM].timer != -1) {
		int bonus_damage = battle_attr_fix(wd.damage, ELE_FIRE, status_get_element(target)) * 20/100;	// 火属性攻撃ダメージの20%を追加
		if(bonus_damage > 0) {
			DMG_ADD( bonus_damage );
		}
	}
	// ソウルブレイカー
	if(skill_num == ASC_BREAKER) {
		wd.damage += damage_sbr;		// 魔法ダメージ
		wd.damage += 500 + (atn_rand() % 500);	// ランダムダメージ
		if(t_def1 < 1000000) {
			int vitbonusmax = (t_vit/20)*(t_vit/20)-1;
			wd.damage -= (t_def1 + t_def2 + ((vitbonusmax < 1)? 0: atn_rand()%(vitbonusmax+1)) + status_get_mdef(target) + status_get_mdef2(target))/2;
		}
	}

	/* 28．星のかけら、気球の適用 */
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
	/* 29．必中固定ダメージ */
	if(src_sd && src_sd->special_state.fix_damage)
		DMG_SET( src_sd->fix_damage );

	if(skill_num == PA_PRESSURE)	// プレッシャー必中
		DMG_SET( 500+300*skill_lv );

	/* 30．左手ダメージの補正 */
	if(calc_flag.rh == 0 && calc_flag.lh == 1) {	// 左手のみ武器装備
		wd.damage = wd.damage2;
		wd.damage2 = 0;
		// 一応左右を入れ替えておく
		calc_flag.rh = 1;
		calc_flag.lh = 0;
	} else if(src_sd && calc_flag.lh) {		// 左手があるなら右手・左手修練の適用
		int dmg = wd.damage, dmg2 = wd.damage2;
		// 右手修練(60% 〜 100%) 右手全般
		skill = pc_checkskill(src_sd,AS_RIGHT);
		wd.damage = wd.damage * (50 + (skill * 10))/100;
		if(dmg > 0 && wd.damage < 1) wd.damage = 1;
		// 左手修練(40% 〜 80%) 左手全般
		skill = pc_checkskill(src_sd,AS_LEFT);
		wd.damage2 = wd.damage2 * (30 + (skill * 10))/100;
		if(dmg2 > 0 && wd.damage2 < 1) wd.damage2 = 1;
	} else {
		wd.damage2 = 0;	// 念のため0を明示しておく
	}

	// 右手,短剣のみ
	if(calc_flag.da > 0) {
		wd.type = 0x08;
		switch (calc_flag.da) {
			case 1:		// ダブルアタック
				wd.div_ = 2;
				wd.damage += wd.damage;
				break;
			case 2:		// 三段掌
				wd.div_ = 255;
				break;
			case 3:		// フェオリチャギ
			case 4:		// ネリョチャギ
			case 5:		// トルリョチャギ
			case 6:		// アプチャオルリギ
				wd.div_ = 248+calc_flag.da;
				break;
		}
	}

	/* 31．スキル修正５（追加ダメージ２） */
	if(src_sd && src_sd->status.weapon == WT_KATAR) {
		// アドバンスドカタール研究
		if((skill = pc_checkskill(src_sd,ASC_KATAR)) > 0) {
			wd.damage += wd.damage*(10+(skill * 2))/100;
		}
		// カタール追撃ダメージ
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

	/* 32．完全回避の判定 */
	if(skill_num == 0 && skill_lv >= 0 && target_sd != NULL && wd.div_ < 255 && atn_rand()%1000 < status_get_flee2(target) ) {
		wd.damage  = 0;
		wd.damage2 = 0;
		wd.type    = 0x0b;
		wd.dmg_lv  = ATK_LUCKY;
	}

	// 対象が完全回避をする設定がONなら
	if(battle_config.enemy_perfect_flee) {
		if(skill_num == 0 && skill_lv >= 0 && target_md != NULL && wd.div_ < 255 && atn_rand()%1000 < status_get_flee2(target) ) {
			wd.damage  = 0;
			wd.damage2 = 0;
			wd.type    = 0x0b;
			wd.dmg_lv  = ATK_LUCKY;
		}
	}

	/* 33．固定ダメージ2 */
	if(t_mode&0x40) {	// MobのModeに頑強フラグが立っているときの処理
		if(wd.damage > 0)
			wd.damage = (wd.div_ < 255)? 1: 3;	// 三段掌のみ3ダメージ
		if(wd.damage2 > 0)
			wd.damage2 = 1;
	}

	// bNoWeaponDamageでグランドクロスじゃない場合はダメージが0
	if( target_sd && target_sd->special_state.no_weapon_damage && skill_num != CR_GRANDCROSS && skill_num != NPC_DARKGRANDCROSS)
		wd.damage = wd.damage2 = 0;

	/* 34．ダメージ最終計算 */
	if(skill_num != CR_GRANDCROSS && skill_num != NPC_DARKGRANDCROSS) {
		if(wd.damage2 < 1)		// ダメージ最終修正
			wd.damage  = battle_calc_damage(src,target,wd.damage,wd.div_,skill_num,skill_lv,wd.flag);
		else if(wd.damage < 1)	// 右手がミス？
			wd.damage2 = battle_calc_damage(src,target,wd.damage2,wd.div_,skill_num,skill_lv,wd.flag);
		else {	// 両手/カタールの場合はちょっと計算ややこしい
			int dmg = wd.damage+wd.damage2;
			wd.damage  = battle_calc_damage(src,target,dmg,wd.div_,skill_num,skill_lv,wd.flag);
			wd.damage2 = (wd.damage2*100/dmg)*wd.damage/100;
			if(wd.damage > 1 && wd.damage2 < 1) wd.damage2 = 1;
			wd.damage -= wd.damage2;
		}
	}

	/* 35．物理攻撃スキルによるオートスペル発動(item_bonus) */
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

	/* 36．太陽と月と星の融合 HP2%消費 */
	if(src_sd && sc_data && sc_data[SC_FUSION].timer != -1)
	{
		int hp;
		if(target->type == BL_PC) {
			hp = src_sd->status.max_hp * 8 / 100;
			if( src_sd->status.hp < (src_sd->status.max_hp * 20 / 100))	// 対象がプレイヤーでHPが20％未満である時、攻撃をすれば即死します。
				hp = src_sd->status.hp;
		} else {
			hp = src_sd->status.max_hp * 2 / 100;
		}
		pc_heal(src_sd,-hp,0);
	}

	/* 37．カアヒ */
	if(skill_num == 0 && wd.flag&BF_WEAPON && t_sc_data && t_sc_data[SC_KAAHI].timer != -1)
	{
		int kaahi_lv = t_sc_data[SC_KAAHI].val1;
		if(status_get_hp(target) < status_get_max_hp(target))
		{
			if(target->type == BL_MOB || status_get_sp(target) > 5*kaahi_lv)	// 対象がmob以外でSPが減少量以下のときは発生しない
			{
				unit_heal(target,200*kaahi_lv,-5*kaahi_lv);
				if(target->type == BL_PC)
					clif_misceffect3(target,7);				// 回復した本人にのみ回復エフェクト
			}
		}
	}

	/* 38．太陽と月と星の奇跡 */
	if(src_sd && wd.flag&BF_WEAPON && pc_checkskill(src_sd,SG_FEEL) > 2 && atn_rand()%10000 < battle_config.sg_miracle_rate)
		status_change_start(src,SC_MIRACLE,1,0,0,0,3600000,0);

	/* 39．計算結果の最終補正 */
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
 * 魔法ダメージ計算
 *------------------------------------------
 */
static struct Damage battle_calc_magic_attack(struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	struct Damage mgd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct map_session_data *sd = NULL, *tsd = NULL;
	struct mob_data         *tmd = NULL;
	struct homun_data       *thd = NULL;
	struct status_change    *sc_data = NULL, *t_sc_data = NULL;
	int matk1, matk2, ele, race;
	int mdef1, mdef2, t_ele, t_race, t_enemy, t_mode;
	int t_class, cardfix, i;
	int normalmagic_flag = 1;

	// return前の処理があるので情報出力部のみ変更
	if( bl == NULL || target == NULL || target->type == BL_PET ) {
		nullpo_info(NLP_MARK);
		return mgd;
	}

	sd  = BL_DOWNCAST( BL_PC,  bl );
	tsd = BL_DOWNCAST( BL_PC,  target );
	tmd = BL_DOWNCAST( BL_MOB, target );
	thd = BL_DOWNCAST( BL_HOM, target );

	// アタッカー
	matk1   = status_get_matk1(bl);
	matk2   = status_get_matk2(bl);
	ele     = skill_get_pl(skill_num);
	race    = status_get_race(bl);
	sc_data = status_get_sc_data(bl);

	// ターゲット
	mdef1     = status_get_mdef(target);
	mdef2     = status_get_mdef2(target);
	t_ele     = status_get_elem_type(target);
	t_race    = status_get_race(target);
	t_enemy   = status_get_enemy_type(target);
	t_mode    = status_get_mode(target);
	t_sc_data = status_get_sc_data(target);

	if(sd) {
		sd->state.attack_type = BF_MAGIC;
		if(sd->matk_rate != 100)
			MATK_FIX( sd->matk_rate, 100 );
		sd->state.arrow_atk = 0;
	}

	/* １．mgd構造体の初期設定 */
	mgd.div_      = skill_get_num(skill_num,skill_lv);
	mgd.blewcount = skill_get_blewcount(skill_num,skill_lv);
	mgd.flag      = BF_MAGIC|BF_LONG|BF_SKILL;

	if(battle_config.calc_dist_flag & 2) {	// 魔法の時計算するか？ &2で計算する
		int target_dist = unit_distance2(bl,target);	// 距離を取得
		if(target_dist < battle_config.allow_sw_dist) {				// SWで防げる距離より小さい＝近距離からの攻撃
			if(battle_config.sw_def_type & 1 && bl->type == BL_PC)		// 人間からのを判定するか　&1でする
				mgd.flag = (mgd.flag&~BF_RANGEMASK)|BF_SHORT;		// 近距離に設定
			if(battle_config.sw_def_type & 2 && bl->type == BL_MOB)		// モンスターからの魔法を判定するか　&2でする
				mgd.flag = (mgd.flag&~BF_RANGEMASK)|BF_SHORT;		// 近距離に設定
		}
	}

	/* ２．魔法力増幅によるMATK増加 */
	if (sc_data && sc_data[SC_MAGICPOWER].timer != -1) {
		matk1 += (matk1 * sc_data[SC_MAGICPOWER].val1 * 5)/100;
		matk2 += (matk2 * sc_data[SC_MAGICPOWER].val1 * 5)/100;
	}

	/* ３．基本ダメージ計算(スキルごとに処理) */
	switch(skill_num)
	{
		case AL_HEAL:	// ヒールor聖体
		case PR_BENEDICTIO:
			mgd.damage = skill_calc_heal(bl,skill_lv)/2;
			if(sd)	// メディタティオを乗せる
				mgd.damage += mgd.damage * pc_checkskill(sd,HP_MEDITATIO)*2/100;
			normalmagic_flag = 0;
			break;
		case PR_ASPERSIO:		// アスペルシオ
			mgd.damage = 40;	// 固定ダメージ
			normalmagic_flag = 0;
			break;
		case PR_SANCTUARY:	// サンクチュアリ
			ele = ELE_HOLY;
			mgd.damage = (skill_lv > 6)? 388: 50*skill_lv;
			normalmagic_flag = 0;
			mgd.blewcount |= 0x10000;
			break;
		case PA_GOSPEL:		// ゴスペル(ランダムダメージ判定の場合)
			mgd.damage = 1000+atn_rand()%9000;
			normalmagic_flag = 0;
			break;
		case ALL_RESURRECTION:
		case PR_TURNUNDEAD:	// 攻撃リザレクションとターンアンデッド
			if(battle_check_undead(t_race,t_ele)) {
				int hp, mhp, thres;
				hp = status_get_hp(target);
				mhp = status_get_max_hp(target);
				thres = skill_lv * 20 + status_get_luk(bl) + status_get_int(bl) + status_get_lv(bl) + 200 - (hp * 200 / mhp);
				if(thres > 700)
					thres = 700;
				if(atn_rand()%1000 < thres && !(t_mode&0x20))	// 成功
					mgd.damage = hp;
				else					// 失敗
					mgd.damage = status_get_lv(bl) + status_get_int(bl) + skill_lv * 10;
			}
			normalmagic_flag = 0;
			break;

		case HW_NAPALMVULCAN:	// ナパームバルカン
		case MG_NAPALMBEAT:	// ナパームビート（分散計算込み）
			MATK_FIX( 70+10*skill_lv, 100 );
			if(flag > 0) {
				MATK_FIX( 1, flag );
			} else {
				if(battle_config.error_log)
					printf("battle_calc_magic_attack: NAPALM enemy count=0 !\n");
			}
			break;
		case MG_SOULSTRIKE:			// ソウルストライク（対アンデッドダメージ補正）
			if(battle_check_undead(t_race,t_ele))
				MATK_FIX( 20+skill_lv, 20 );	// MATKに補正じゃ駄目ですかね？
			break;
		case MG_FIREBALL:	// ファイヤーボール
			if(flag > 2) {
				matk1 = matk2 = 0;
			} else {
				MATK_FIX( 70+10*skill_lv, 100 );
				if(flag == 2)
					MATK_FIX( 3, 4 );
			}
			break;
		case MG_FIREWALL:	// ファイヤーウォール
			if((t_ele == ELE_FIRE || battle_check_undead(t_race,t_ele)) && target->type != BL_PC)
				mgd.blewcount = 0;
			else
				mgd.blewcount |= 0x10000;
			MATK_FIX( 1, 2 );
			break;
		case MG_THUNDERSTORM:	// サンダーストーム
			MATK_FIX( 80, 100 );
			break;
		case MG_FROSTDIVER:	// フロストダイバ
			MATK_FIX( 100+10*skill_lv, 100 );
			break;
		case WZ_FROSTNOVA:	// フロストノヴァ
			MATK_FIX( 200+20*skill_lv, 300 );
			break;
		case WZ_FIREPILLAR:	// ファイヤーピラー
			if(mdef1 < 1000000)
				mdef1 = mdef2 = 0;	// MDEF無視
			if(bl->type != BL_MOB)
				MATK_FIX( 1, 5 );
			matk1 += 50;
			matk2 += 50;
			break;
		case WZ_SIGHTRASHER:
			MATK_FIX( 100+20*skill_lv, 100);
			break;
		case WZ_METEOR:
		case WZ_JUPITEL:	// ユピテルサンダー
		case NPC_DARKJUPITEL:	// 闇ユピテル
			break;
		case WZ_VERMILION:	// ロードオブバーミリオン
			MATK_FIX( 80+20*skill_lv, 100 );
			break;
		case WZ_WATERBALL:	// ウォーターボール
			MATK_FIX( 100+30*skill_lv, 100 );
			break;
		case WZ_STORMGUST:	// ストームガスト
			MATK_FIX( 100+40*skill_lv, 100 );
			//mgd.blewcount |= 0x10000;
			break;
		case AL_HOLYLIGHT:	// ホーリーライト
			MATK_FIX( 125, 100 );
			if(sc_data && sc_data[SC_PRIEST].timer != -1) {
				MATK_FIX( 500, 100 );
			}
			break;
		case AL_RUWACH:
			MATK_FIX( 145, 100 );
			break;
		case WZ_SIGHTBLASTER:
			MATK_FIX( 145, 100 );
			break;
		case SL_STIN:	// エスティン
			if(status_get_size(target) == 0) {
				MATK_FIX( 10*skill_lv, 100 );
			} else {
				MATK_FIX( skill_lv, 100 );
			}
			if(skill_lv >= 7)
				status_change_start(bl,SC_SMA,skill_lv,0,0,0,3000,0);
			break;
		case SL_STUN:	// エスタン
			MATK_FIX( 5*skill_lv, 100 );
			ele = status_get_attack_element(bl);
			if(skill_lv >= 7)
				status_change_start(bl,SC_SMA,skill_lv,0,0,0,3000,0);
			break;
		case SL_SMA:	// エスマ
			if(sd && skill_lv >= 10)
			{
				MATK_FIX( 40+sd->status.base_level, 100 );
			}
			ele = status_get_attack_element_nw(bl);
			if(sc_data && sc_data[SC_SMA].timer != -1)
				status_change_end(bl,SC_SMA,-1);
			break;
		case NJ_KOUENKA:	// 紅炎華
			MATK_FIX( 90, 100 );
			break;
		case NJ_KAENSIN:	// 火炎陣
			MATK_FIX( 50, 100 );
			break;
		case NJ_HUUJIN:		// 風刃
			break;
		case NJ_HYOUSENSOU:	// 氷閃槍
			if(t_sc_data && t_sc_data[SC_SUITON].timer != -1) {
				MATK_FIX( 70+2*t_sc_data[SC_SUITON].val1, 100 );
			} else {
				MATK_FIX( 70, 100 );
			}
			break;
		case NJ_BAKUENRYU:	// 龍炎陣
			MATK_FIX( 50+50*skill_lv, 100 );
			break;
		case NJ_HYOUSYOURAKU:	// 氷柱落し
			MATK_FIX( 100+50*skill_lv, 100 );
			break;
		case NJ_RAIGEKISAI:	// 雷撃砕
			MATK_FIX( 160+40*skill_lv, 100 );
			break;
		case NJ_KAMAITACHI:	// 朔風
			MATK_FIX( 100+100*skill_lv,100 );
			break;
	}

	/* ４．一般魔法ダメージ計算 */
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
		if(mgd.damage < 1)	// プレイヤーの魔法スキルは1ダメージ保証無し
			mgd.damage = (!battle_config.skill_min_damage && bl->type == BL_PC)? 0: 1;
	}

	/* ５．カードによるダメージ追加処理 */
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
		// カード効果による特定スキルのダメージ増幅（魔法スキル）
		if(bl->type == BL_PC && sd->skill_dmgup.count > 0 && skill_num > 0) {
			for(i=0; i<sd->skill_dmgup.count; i++) {
				if(skill_num == sd->skill_dmgup.id[i]) {
					cardfix = cardfix*(100+sd->skill_dmgup.rate[i])/100;
					break;
				}
			}
		}
		mgd.damage = mgd.damage*cardfix/100;
	}

	/* ６．カードによるダメージ減衰処理 */
	if(tsd && mgd.damage > 0) {
		int s_class = status_get_class(bl);
		cardfix = 100;
		cardfix = cardfix*(100-tsd->subele[ele])/100;				// 属性によるダメージ耐性
		cardfix = cardfix*(100-tsd->subrace[race])/100;				// 種族によるダメージ耐性
		cardfix = cardfix*(100-tsd->subenemy[status_get_enemy_type(bl)])/100;	// 敵タイプによるダメージ耐性
		cardfix = cardfix*(100-tsd->subsize[status_get_size(bl)])/100;		// サイズによるダメージ耐性
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

	/* ７．属性の適用 */
	mgd.damage = battle_attr_fix(mgd.damage, ele, status_get_element(target));

	/* ８．スキル修正１ */
	if(skill_num == CR_GRANDCROSS || skill_num == NPC_DARKGRANDCROSS) {	// グランドクロス
		static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		wd = battle_calc_weapon_attack(bl,target,skill_num,skill_lv,flag);
		mgd.damage = (mgd.damage + wd.damage) * (100 + 40*skill_lv)/100;
		if(battle_config.gx_dupele)
			mgd.damage = battle_attr_fix(mgd.damage, ele, status_get_element(target));	// 属性2回かかる
		if(bl == target) {
			if(bl->type == BL_MOB || bl->type == BL_HOM)
				mgd.damage = 0;		// MOB,HOMが使う場合は反動無し
		} else {
			 mgd.damage /= 2;	// 反動は半分
		}
	}

	if (skill_num == WZ_WATERBALL)
		mgd.div_ = 1;

	/* ９．対象にステータス異常がある場合 */
	if( tsd && tsd->special_state.no_magic_damage )
		mgd.damage = 0;	// 黄金蟲カード（魔法ダメージ０)

	if(t_sc_data && t_sc_data[SC_HERMODE].timer != -1 && t_sc_data[SC_HERMODE].val1 == 1)	// ヘルモードなら魔法ダメージなし
		mgd.damage = 0;

	/* 10．固定ダメージ */
	if(skill_num == HW_GRAVITATION)	// グラビテーションフィールド
		mgd.damage = 200+200*skill_lv;

	/* 11．ヒット回数によるダメージ倍加 */
	if(mgd.damage != 0) {
		if(t_mode&0x40) { // 草・きのこ等
			// ロードオブヴァーミリオンはノーダメージ。それ以外は連打数ダメージ
			if (!battle_config.skill_min_damage && skill_num == WZ_VERMILION)
				mgd.damage = 0;
			else
				mgd.damage = (mgd.div_ == 255)? 3: mgd.div_;
		}
		else if(mgd.div_ > 1 && skill_num != WZ_VERMILION)
			mgd.damage *= mgd.div_;
	}

	/* 12．ダメージ最終計算 */
	mgd.damage = battle_calc_damage(bl,target,mgd.damage,mgd.div_,skill_num,skill_lv,mgd.flag);

	/* 13．魔法でもオートスペル発動(item_bonus) */
	if(bl && bl->type == BL_PC && bl != target && mgd.damage > 0)
	{
		unsigned long asflag = EAS_ATTACK;
		if(battle_config.magic_attack_autospell)
			asflag += EAS_SHORT|EAS_LONG;
		else
			asflag += EAS_MAGIC;

		skill_bonus_autospell(bl,target,asflag,gettick(),0);
	}

	/* 14．魔法でもHP/SP回復(月光剣など) */
	if(battle_config.magic_attack_drain && bl != target)
		battle_attack_drain(bl,mgd.damage,0,battle_config.magic_attack_drain_enable_type);

	/* 15．計算結果の最終補正 */
	mgd.amotion = status_get_amotion(bl);
	mgd.dmotion = status_get_dmotion(target);
	mgd.damage2 = 0;
	mgd.type    = 0;

	return mgd;
}

/*==========================================
 * その他ダメージ計算
 *------------------------------------------
 */
static struct Damage battle_calc_misc_attack(struct block_list *bl,struct block_list *target,int skill_num,int skill_lv,int flag)
{
	struct Damage mcd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	struct map_session_data *sd = NULL, *tsd = NULL;
	struct skill_unit       *unit = NULL;
	int int_, dex, race, ele;
	int skill;
	int damagefix = 1;

	// return前の処理があるので情報出力部のみ変更
	if( bl == NULL || target == NULL || target->type == BL_PET ) {
		nullpo_info(NLP_MARK);
		return mcd;
	}

	// グラウンドドリフトのときはblを設置者に置換する
	if(bl->type == BL_SKILL) {
		unit = (struct skill_unit *)bl;
		if(unit && unit->group)
			bl = map_id2bl(unit->group->src_id);
	}

	sd  = BL_DOWNCAST( BL_PC, bl );
	tsd = BL_DOWNCAST( BL_PC, target );

	// アタッカー
	int_ = status_get_int(bl);
	dex  = status_get_dex(bl);
	race = status_get_race(bl);
	ele  = skill_get_pl(skill_num);

	if(sd) {
		sd->state.attack_type = BF_MISC;
		sd->state.arrow_atk = 0;
	}

	/* １．mcd構造体の初期設定 */
	mcd.div_      = skill_get_num(skill_num,skill_lv);
	mcd.blewcount = skill_get_blewcount(skill_num,skill_lv);
	mcd.flag      = BF_MISC|BF_SHORT|BF_SKILL;

	/* ２．基本ダメージ計算(スキルごとに処理) */
	switch(skill_num)
	{
	case HT_LANDMINE:	// ランドマイン
		mcd.damage = skill_lv*(dex+75)*(100+int_)/100;
		break;

	case HT_BLASTMINE:	// ブラストマイン
		mcd.damage = skill_lv*(dex/2+50)*(100+int_)/100;
		break;

	case HT_CLAYMORETRAP:	// クレイモアートラップ
		mcd.damage = skill_lv*(dex/2+75)*(100+int_)/100;
		break;

	case HT_BLITZBEAT:	// ブリッツビート
		if(sd == NULL || (skill = pc_checkskill(sd,HT_STEELCROW)) <= 0)
			skill = 0;
		mcd.damage = (dex/10 + int_/2 + skill*3 + 40)*2;
		if(flag > 1)
			mcd.damage /= flag;
		flag &= ~(BF_SKILLMASK|BF_RANGEMASK|BF_WEAPONMASK);
		mcd.flag = flag|(mcd.flag&~BF_RANGEMASK)|BF_LONG;
		break;

	case TF_THROWSTONE:	// 石投げ
		mcd.damage = 50;
		damagefix = 0;
		flag &= ~(BF_SKILLMASK|BF_RANGEMASK|BF_WEAPONMASK);
		mcd.flag = flag|(mcd.flag&~BF_RANGEMASK)|BF_LONG;
		break;

	case BA_DISSONANCE:	// 不協和音
		mcd.damage = (skill_lv)*20+pc_checkskill(sd,BA_MUSICALLESSON)*3;
		break;
	case NPC_SELFDESTRUCTION:	// 自爆
	case NPC_SELFDESTRUCTION2:	// 自爆2
		mcd.damage = status_get_hp(bl)-((bl == target)? 1: 0);
		damagefix = 0;
		break;

	case NPC_SMOKING:	// タバコを吸う
		mcd.damage = 3;
		damagefix = 0;
		break;

	case NPC_DARKBREATH:
		{
			struct status_change *sc_data = status_get_sc_data(target);
			int hitrate = status_get_hit(bl) - status_get_flee(target) + 80;
			int t_hp = status_get_hp(target);
			hitrate = (hitrate > 95)? 95: (hitrate < 5)? 5: hitrate;
			if(sc_data && (sc_data[SC_SLEEP].timer != -1 || sc_data[SC_STAN].timer != -1 ||
				sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0) ) )
				hitrate = 1000000;
			if(atn_rand()%100 < hitrate)
				mcd.damage = t_hp*(skill_lv*6)/100;
		}
		break;
	case SN_FALCONASSAULT:		// ファルコンアサルト
		if(sd == NULL || (skill = pc_checkskill(sd,HT_STEELCROW)) <= 0)
			skill = 0;
		mcd.damage = ((dex/10+int_/2+skill*3+40)*2*(150+skill_lv*70)/100)*5;
		if(sd && battle_config.allow_falconassault_elemet)
			ele = sd->atk_ele;
		flag &= ~(BF_WEAPONMASK|BF_RANGEMASK|BF_WEAPONMASK);
		mcd.flag = flag|(mcd.flag&~BF_RANGEMASK)|BF_LONG;
		break;
	case GS_GROUNDDRIFT:		// グラウンドドリフト
		if(unit && unit->group)
		{
			const int ele_type[5] = { ELE_WIND, ELE_DARK, ELE_POISON, ELE_WATER, ELE_FIRE };
			ele = ele_type[unit->group->unit_id - UNT_GROUNDDRIFT_WIND];
			mcd.damage = status_get_baseatk(bl);
		}
		break;
	case HVAN_EXPLOSION:		// バイオエクスプロージョン
		mcd.damage = status_get_hp(bl)*(50+50*skill_lv)/100;
		break;
	default:
		mcd.damage = status_get_baseatk(bl);
		break;
	}

	if(damagefix) {
		if(mcd.damage < 1 && skill_num != NPC_DARKBREATH)
			mcd.damage = 1;

		/* ３．カードによるダメージ減衰処理 */
		if(tsd && mcd.damage > 0) {
			int cardfix = 100;
			cardfix = cardfix*(100-tsd->subele[ele])/100;	// 属性によるダメージ耐性
			cardfix = cardfix*(100-tsd->subrace[race])/100;	// 種族によるダメージ耐性
			cardfix = cardfix*(100-tsd->subenemy[status_get_enemy_type(bl)])/100;	// 敵タイプによるダメージ耐性
			cardfix = cardfix*(100-tsd->subsize[status_get_size(bl)])/100;	// サイズによるダメージ耐性
			cardfix = cardfix*(100-tsd->misc_def_rate)/100;
			mcd.damage = mcd.damage*cardfix/100;
		}
		if(mcd.damage < 0)
			mcd.damage = 0;

		/* ４．属性の適用 */
		mcd.damage = battle_attr_fix(mcd.damage, ele, status_get_element(target));

		/* ５．スキル修正 */
		if(skill_num == GS_GROUNDDRIFT) {	// 固定ダメージを加算してさらに無属性として属性計算する
			mcd.damage += skill_lv*50;
			mcd.damage = battle_attr_fix(mcd.damage, ELE_NEUTRAL, status_get_element(target));
		}

	}

	/* ６．ヒット回数によるダメージ倍加 */
	if(mcd.div_ > 1)
		mcd.damage *= mcd.div_;
	if( mcd.damage > 0 && (mcd.damage < mcd.div_ || (status_get_def(target) >= 1000000 && status_get_mdef(target) >= 1000000)) ) {
		mcd.damage = mcd.div_;
	}

	/* ７．固定ダメージ */
	if(status_get_mode(target)&0x40 && mcd.damage > 0)	// 草・きのこ等
		mcd.damage = 1;

	/* ８．カードによるダメージ追加処理 */
	if(sd && sd->skill_dmgup.count > 0 && skill_num > 0 && mcd.damage > 0) {	// カード効果による特定スキルのダメージ増幅
		int i;
		for(i=0; i<sd->skill_dmgup.count; i++) {
			if(skill_num == sd->skill_dmgup.id[i]) {
				mcd.damage += mcd.damage * sd->skill_dmgup.rate[i] / 100;
				break;
			}
		}
	}

	/* ９．ダメージ最終計算 */
	mcd.damage = battle_calc_damage(bl,target,mcd.damage,mcd.div_,skill_num,skill_lv,mcd.flag);

	/* 10．miscでもオートスペル発動(bonus) */
	if(bl->type == BL_PC && bl != target && mcd.damage > 0)
	{
		unsigned long asflag = EAS_ATTACK;
		if(battle_config.misc_attack_autospell)
			asflag += EAS_SHORT|EAS_LONG;
		else
			asflag += EAS_MISC;

		skill_bonus_autospell(bl,target,asflag,gettick(),0);
	}

	/* 11．miscでもHP/SP回復(月光剣など) */
	if(battle_config.misc_attack_drain && bl != target)
		battle_attack_drain(bl,mcd.damage,0,battle_config.misc_attack_drain_enable_type);

	/* 12．計算結果の最終補正 */
	mcd.amotion = status_get_amotion(bl);
	mcd.dmotion = status_get_dmotion(target);
	mcd.damage2 = 0;
	mcd.type    = 0;

	return mcd;
}

/*==========================================
 * ダメージ計算一括処理用
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
 * 通常攻撃処理まとめ
 *------------------------------------------
 */
int battle_weapon_attack( struct block_list *src,struct block_list *target,unsigned int tick,int flag)
{
	struct map_session_data *sd = NULL, *tsd = NULL;
	struct status_change *sc_data, *t_sc_data;
	unsigned short *opt1;
	int damage,rdamage = 0;
	static struct Damage wd = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	if(src->prev == NULL || target->prev == NULL)
		return 0;
	if(unit_isdead(src) || unit_isdead(target))
		return 0;

	sd  = BL_DOWNCAST(BL_PC, src);
	tsd = BL_DOWNCAST(BL_PC, target);

	opt1 = status_get_opt1(src);
	if(opt1 && *opt1 > 0) {
		unit_stopattack(src);
		return 0;
	}

	sc_data   = status_get_sc_data(src);
	t_sc_data = status_get_sc_data(target);

	// 自分が白羽・強制移動・魅惑のウィンク中はダメ
	if(sc_data && (sc_data[SC_BLADESTOP].timer != -1 || sc_data[SC_FORCEWALKING].timer != -1 || sc_data[SC_WINKCHARM].timer != -1)) {
		unit_stopattack(src);
		return 0;
	}
	// 相手が強制移動中
	if(t_sc_data && t_sc_data[SC_FORCEWALKING].timer != -1) {
		unit_stopattack(src);
		return 0;
	}

	if(battle_check_target(src,target,BCT_ENEMY) <= 0 && !battle_check_range(src,target,0))
		return 0;	// 攻撃対象外

	// ターゲットがMOB GMハイド中で、コンフィグでハイド中攻撃不可 GMレベルが指定より大きい場合
	if(target->type == BL_MOB && sd && sd->status.option&0x40 && battle_config.hide_attack == 0 && pc_isGM(sd) < battle_config.gm_hide_attack_lv)
		return 0;	// 隠れて攻撃するなんて卑怯なGMﾃﾞｽﾈ

	if(sd) {
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

	if(wd.div_ == 255 && sd) {	// 三段掌
		int delay = 0;
		int skilllv;
		if(wd.damage+wd.damage2 < status_get_hp(target)) {
			if((skilllv = pc_checkskill(sd, MO_CHAINCOMBO)) > 0) {
				delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
				delay += 300 * battle_config.combo_delay_rate /100;
				// コンボ入力時間の最低保障追加
				if( delay < battle_config.combo_delay_lower_limits )
					delay = battle_config.combo_delay_lower_limits;
			}
			status_change_start(src,SC_COMBO,MO_TRIPLEATTACK,skilllv,0,0,delay,0);
		}
		sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
		clif_combo_delay(src,delay);
		clif_skill_damage(src , target , tick , wd.amotion , wd.dmotion ,
			wd.damage , 3 , MO_TRIPLEATTACK, pc_checkskill(sd,MO_TRIPLEATTACK) , -1 );

		// クローンスキル
		if(wd.damage> 0 && tsd && pc_checkskill(tsd,RG_PLAGIARISM) && sc_data && sc_data[SC_PRESERVE].timer == -1) {
			skill_clone(tsd,MO_TRIPLEATTACK,pc_checkskill(sd, MO_TRIPLEATTACK));
		}
	} else if(wd.div_ >= 251 && wd.div_ <= 254 && sd) {	// テコン蹴り系統
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
		clif_skill_nodamage(&sd->bl,&sd->bl,skillid-1,pc_checkskill(sd,skillid-1),1);
	} else {
		clif_damage(src,target,tick, wd.amotion, wd.dmotion,
			wd.damage, wd.div_ , wd.type, wd.damage2);
		// 二刀流左手とカタール追撃のミス表示(無理やり〜)
		if(sd && (sd->status.weapon > WT_HUUMA || sd->status.weapon == WT_KATAR) && wd.damage2 == 0)
			clif_damage(src,target,tick+10, wd.amotion, wd.dmotion,0, 1, 0, 0);
	}
	if(sd && sd->splash_range > 0 && (wd.damage > 0 || wd.damage2 > 0))
		skill_castend_damage_id(src,target,0,-1,tick,0);

	map_freeblock_lock();
	battle_delay_damage(tick+wd.amotion,src,target,(wd.damage+wd.damage2),wd.flag);

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
	if(sc_data && sc_data[SC_AUTOSPELL].timer != -1 && atn_rand()%100 < sc_data[SC_AUTOSPELL].val4) {
		int spellid = sc_data[SC_AUTOSPELL].val2;
		int spelllv = sc_data[SC_AUTOSPELL].val3;
		int r = atn_rand()%100;
		int sp = 0, fail = 0;

		if(r >= 50)
			spelllv -= 2;
		else if(r >= 15)
			spelllv--;
		if(spelllv < 1)
			spelllv = 1;

		if(sd) {
			if(sd->sc_data[SC_SAGE].timer != -1)	// セージの魂
				spelllv = pc_checkskill(sd,spellid);
			sp = skill_get_sp(spellid,spelllv)*2/3;
			if(sd->status.sp < sp)
				fail = 1;
		}
		if(!fail) {
			if(skill_get_inf(spellid) & 0x22) {
				fail = skill_castend_pos2(src,target->x,target->y,spellid,spelllv,tick,flag);
			} else {
				switch(skill_get_nk(spellid) & 3) {
				case 0:
				case 2:	/* 攻撃系 */
					fail = skill_castend_damage_id(src,target,spellid,spelllv,tick,flag);
					break;
				case 1:	/* 支援系 */
					if( (spellid == AL_HEAL || (spellid == ALL_RESURRECTION && target->type != BL_PC)) &&
					    battle_check_undead(status_get_race(target),status_get_elem_type(target)) ) {
						fail = skill_castend_damage_id(src,target,spellid,spelllv,tick,flag);
					} else {
						fail = skill_castend_nodamage_id(src,target,spellid,spelllv,tick,flag);
					}
					break;
				}
			}
			if(sd && !fail)
				pc_heal(sd,0,-sp);
		}
	}

	// カードによるオートスペル
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
		// SP消失
		if(tsd && atn_rand()%100 < sd->sp_vanish_rate)
		{
			int sp = status_get_sp(target) * sd->sp_vanish_per/100;
			if(sp > 0)
				pc_heal(tsd, 0, -sp);
		}
	}

	if(sd && src != target && wd.flag&BF_WEAPON && (wd.damage > 0 || wd.damage2 > 0)) {
		// ％吸収、一定吸収ともに
		battle_attack_drain(src, wd.damage, wd.damage2, 3);
	}

	if(rdamage > 0) {
		battle_delay_damage(tick+wd.amotion,target,src,rdamage,0);

		// 反射ダメージのオートスペル
		if(battle_config.weapon_reflect_autospell && target->type == BL_PC)
			skill_bonus_autospell(target,src,EAS_ATTACK,gettick(),0);

		if(battle_config.weapon_reflect_drain && src != target)
			battle_attack_drain(target,rdamage,0,battle_config.weapon_reflect_drain_enable_type);
	}

	if(t_sc_data && t_sc_data[SC_AUTOCOUNTER].timer != -1 && t_sc_data[SC_AUTOCOUNTER].val4 > 0) {
		if(t_sc_data[SC_AUTOCOUNTER].val3 == src->id)
			battle_weapon_attack(target,src,tick,0x8000|t_sc_data[SC_AUTOCOUNTER].val1);
		status_change_end(target,SC_AUTOCOUNTER,-1);
	}
	if(t_sc_data && t_sc_data[SC_BLADESTOP_WAIT].timer != -1 && !(status_get_mode(src)&0x20)) {	// ボスには無効
		int lv = t_sc_data[SC_BLADESTOP_WAIT].val1;
		status_change_end(target,SC_BLADESTOP_WAIT,-1);
		status_change_start(src,SC_BLADESTOP,lv,1,src->id,target->id,skill_get_time2(MO_BLADESTOP,lv),0);
		status_change_start(target,SC_BLADESTOP,lv,2,target->id,src->id,skill_get_time2(MO_BLADESTOP,lv),0);
	}
	if(t_sc_data && t_sc_data[SC_POISONREACT].timer != -1) {
		if( (src->type == BL_MOB && status_get_elem_type(src) == ELE_POISON) || status_get_attack_element(src) == ELE_POISON ) {
			// 毒属性mobまたは毒属性による攻撃ならば反撃
			if( battle_check_range(target,src,status_get_range(target)+1) ) {
				t_sc_data[SC_POISONREACT].val2 = 0;
				battle_skill_attack(BF_WEAPON,target,target,src,AS_POISONREACT,t_sc_data[SC_POISONREACT].val1,tick,0);
			}
		} else {
			// それ以外の通常攻撃に対するインベ反撃（射線チェックなし）
			--t_sc_data[SC_POISONREACT].val2;
			if(atn_rand()&1) {
				if( tsd == NULL || pc_checkskill(tsd,TF_POISON) >= 5 )
					battle_skill_attack(BF_WEAPON,target,target,src,TF_POISON,5,tick,flag);
			}
		}
		if (t_sc_data[SC_POISONREACT].val2 <= 0)
			status_change_end(target,SC_POISONREACT,-1);
	}
	map_freeblock_unlock();
	return wd.dmg_lv;
}

/*=========================================================================
 * スキル攻撃効果処理まとめ
 * flagの説明。16進図
 * 	0XYRTTff
 *  ff = battle_calc_attackで各種計算に利用
 *  TT = パケットのtype部分（0でデフォルト）
 *   R = 予約（skill_area_subで使用されたBCT_*）
 *   Y = パケットのスキルLv（fのときは-1に変換）
 *   X = エフェクトのみでダメージなしフラグ
 *-------------------------------------------------------------------------
 */
int battle_skill_attack(int attack_type,struct block_list* src,struct block_list *dsrc,
	struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag)
{
	struct Damage dmg;
	struct map_session_data *sd = NULL, *tsd = NULL;
	struct status_change *sc_data;
	struct status_change *ssc_data;
	int type, lv, damage, rdamage = 0;

	nullpo_retr(0, src);
	nullpo_retr(0, dsrc);
	nullpo_retr(0, bl);

	sc_data = status_get_sc_data(bl);
	ssc_data = status_get_sc_data(src);

	sd  = BL_DOWNCAST( BL_PC, src );
	tsd = BL_DOWNCAST( BL_PC, bl );

	if(dsrc->m != bl->m)	// 対象が同じマップにいなければ何もしない
		return 0;
	if(src->prev == NULL || dsrc->prev == NULL || bl->prev == NULL)
		return 0;
	if(unit_isdead(src) || unit_isdead(dsrc) || unit_isdead(bl))	// すでに死んでいたら何もしない
		return 0;

	if(ssc_data) {		// 自分が強制移動もしくは魅惑のウィンク中なら何もしない
		if(ssc_data[SC_FORCEWALKING].timer != -1 || ssc_data[SC_WINKCHARM].timer != -1)
			return 0;
	}
	if(sc_data) {
		if(sc_data[SC_HIDING].timer != -1 && skill_get_pl(skillid) != ELE_EARTH)	// ハイディング状態でスキルの属性が地属性でないなら何もしない
			return 0;
		if(sc_data[SC_CHASEWALK].timer != -1 && skillid == AL_RUWACH)	// チェイスウォーク状態でルアフ無効
			return 0;
		if(sc_data[SC_TRICKDEAD].timer != -1) 				// 死んだふり中は何もしない
			return 0;
		if(sc_data[SC_FORCEWALKING].timer != -1) 			// 強制移動中は何もしない
			return 0;
		// 凍結状態でストームガスト、フロストノヴァ、氷衝落は無効
		if(sc_data[SC_FREEZE].timer != -1 && (skillid == WZ_STORMGUST || skillid == WZ_FROSTNOVA || skillid == NJ_HYOUSYOURAKU))
			return 0;
	}
	if(skillid == WZ_FROSTNOVA && dsrc->x == bl->x && dsrc->y == bl->y)	// 使用スキルがフロストノヴァで、dsrcとblが同じ場所なら何もしない
		return 0;
	if(sd && sd->chatID)	// 発動元がPCでチャット中なら何もしない
		return 0;
	if(sd && mob_gvmobcheck(sd,bl) == 0)
		return 0;

	type = (flag >> 8) & 0xff;
	if(skillid == 0)
		type = 5;

	/* 矢の消費 */
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

	/* フラグ値チェック */
	lv = (flag >> 20) & 0x0f;
	if(lv == 0)
		lv = skilllv;
	else if(lv == 0x0f)
		lv = -1;

	if(flag & 0x01000000) {	// エフェクトだけ出してダメージなしで終了
		clif_skill_damage(dsrc, bl, tick, status_get_amotion(src), 0, -1, 1, skillid, lv, type);
		return -1;
	}

	/* ダメージ計算 */
	if(skillid == GS_GROUNDDRIFT)	// グラウンドドリフトはdsrcを引数として渡す
		dmg = battle_calc_attack(attack_type,dsrc,bl,skillid,skilllv,flag&0xff);
	else
		dmg = battle_calc_attack(attack_type,src,bl,skillid,skilllv,flag&0xff);

	/* マジックロッド */
	if(attack_type&BF_MAGIC && sc_data && sc_data[SC_MAGICROD].timer != -1 && src == dsrc) {
		dmg.damage = dmg.damage2 = 0;
		if(tsd) {	// 対象がPCの場合
			int sp = skill_get_sp(skillid,skilllv);		// 使用されたスキルのSPを吸収
			sp = sp * sc_data[SC_MAGICROD].val2 / 100;
			if(skillid == WZ_WATERBALL && skilllv > 1)
				sp = sp/((skilllv|1)*(skilllv|1));	// ウォーターボールはさらに計算？
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
			tsd->ud.canact_tick = tick + skill_delayfix(&tsd->bl, skill_get_delay(SA_MAGICROD,sc_data[SC_MAGICROD].val1), skill_get_cast(SA_MAGICROD,sc_data[SC_MAGICROD].val1));
		}
		clif_skill_nodamage(bl,bl,SA_MAGICROD,sc_data[SC_MAGICROD].val1,1);	// マジックロッドエフェクトを表示
	}

	damage = dmg.damage + dmg.damage2;

	if(damage <= 0 || damage < dmg.div_)	// 吹き飛ばし判定
		dmg.blewcount = 0;

	if(skillid == CR_GRANDCROSS || skillid == NPC_DARKGRANDCROSS) {	// グランドクロス
		if(battle_config.gx_disptype)
			dsrc = src;	// 敵ダメージ白文字表示
		if(src == bl)
			type = 4;	// 反動はダメージモーションなし
	}

	/* コンボ */
	if(sd) {
		// 連打掌ここから
		if(skillid == MO_CHAINCOMBO) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src); // 基本ディレイの計算
			if(damage < status_get_hp(bl)) { // ダメージが対象のHPより小さい場合
				if(pc_checkskill(sd, MO_COMBOFINISH) > 0 && sd->spiritball > 0) { // 猛龍拳取得＆気球保持時は+300ms
					delay += 300 * battle_config.combo_delay_rate /100; // 追加ディレイをconfにより調整
					// コンボ入力時間の最低保障追加
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,MO_CHAINCOMBO,skilllv,0,0,delay,0); // コンボ状態に
			}
			sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
			clif_combo_delay(src,delay); // コンボディレイパケットの送信
		}
		// 猛龍拳ここから
		else if(skillid == MO_COMBOFINISH) {
			int delay = 700 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				// 阿修羅覇凰拳取得＆気球4個保持＆爆裂波動状態時は+300ms
				// 伏虎拳取得時も+300ms
				if((pc_checkskill(sd, MO_EXTREMITYFIST) > 0 && sd->spiritball >= 4 && sd->sc_data[SC_EXPLOSIONSPIRITS].timer != -1) ||
				(pc_checkskill(sd, CH_TIGERFIST) > 0 && sd->spiritball > 0) ||
				(pc_checkskill(sd, CH_CHAINCRUSH) > 0 && sd->spiritball > 1))
				{
					delay += 300 * battle_config.combo_delay_rate /100; // 追加ディレイをconfにより調整
					// コンボ入力時間最低保障追加
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,MO_COMBOFINISH,skilllv,0,0,delay,0); // コンボ状態に
			}
			sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
			clif_combo_delay(src,delay); // コンボディレイパケットの送信
		}
		// 伏虎拳ここから
		else if(skillid == CH_TIGERFIST) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				if(pc_checkskill(sd, CH_CHAINCRUSH) > 0) { // 連柱崩撃取得時は+300ms
					delay += 300 * battle_config.combo_delay_rate /100; // 追加ディレイをconfにより調整
					// コンボ入力時間最低保障追加
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}

				status_change_start(src,SC_COMBO,CH_TIGERFIST,skilllv,0,0,delay,0); // コンボ状態に
			}
			sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
			clif_combo_delay(src,delay); // コンボディレイパケットの送信
		}
		// 連柱崩撃ここから
		else if(skillid == CH_CHAINCRUSH) {
			int delay = 1000 - 4 * status_get_agi(src) - 2 *  status_get_dex(src);
			if(damage < status_get_hp(bl)) {
				// 伏虎拳習得または阿修羅習得＆気球1個保持＆爆裂波動時ディレイ
				if(pc_checkskill(sd, CH_TIGERFIST) > 0 || (pc_checkskill(sd, MO_EXTREMITYFIST) > 0 && sd->spiritball >= 1 && sd->sc_data[SC_EXPLOSIONSPIRITS].timer != -1))
				{
					delay += (600+(skilllv/5)*200) * battle_config.combo_delay_rate /100; // 追加ディレイをconfにより調整
					// コンボ入力時間最低保障追加
					if(delay < battle_config.combo_delay_lower_limits)
						delay = battle_config.combo_delay_lower_limits;
				}
				status_change_start(src,SC_COMBO,CH_CHAINCRUSH,skilllv,0,0,delay,0); // コンボ状態に
			}
			sd->ud.attackabletime = sd->ud.canmove_tick = tick + delay;
			clif_combo_delay(src,delay); // コンボディレイパケットの送信
		}
		// TKコンボ
		if(skillid == TK_STORMKICK || skillid == TK_DOWNKICK || skillid == TK_TURNKICK || skillid == TK_COUNTER) {
			if(ranking_get_pc_rank(sd,RK_TAEKWON) > 0) {	// テコンランカーはコンボ続行
				int delay = status_get_adelay(src);
				if(damage < status_get_hp(bl)) {
					//delay += 500 * battle_config.combo_delay_rate /100;
					delay += 2000 - 4*status_get_agi(src) - 2*status_get_dex(src);	// eA方式
					status_change_start(src,SC_TKCOMBO,skillid,0,0,TK_MISSION,delay,0);
				}
				sd->ud.attackabletime = tick + delay;
			} else {
				status_change_end(src,SC_TKCOMBO,-1);
			}
		}
	}

	/* ダメージ反射 */
	if(attack_type&BF_WEAPON && damage > 0 && src != bl && src == dsrc) {	// 武器スキル＆ダメージあり＆使用者と対象者が違う＆src=dsrc
		if(dmg.flag&BF_SHORT) {	// 近距離攻撃時
			if(tsd) {	// 対象がPCの時
				if(tsd->short_weapon_damage_return > 0) {	// 近距離攻撃跳ね返し
					rdamage += damage * tsd->short_weapon_damage_return / 100;
					if(rdamage < 1) rdamage = 1;
				}
			}
			if(sc_data && sc_data[SC_REFLECTSHIELD].timer != -1 && skillid != WS_CARTTERMINATION) {	// リフレクトシールド時
				rdamage += damage * sc_data[SC_REFLECTSHIELD].val2 / 100;	// 跳ね返し計算
				if(rdamage < 1) rdamage = 1;
			}
		} else if(dmg.flag&BF_LONG) {	// 遠距離攻撃時
			if(tsd) {		// 対象がPCの時
				if(tsd->long_weapon_damage_return > 0) { // 遠距離攻撃跳ね返し
					rdamage += damage * tsd->long_weapon_damage_return / 100;
					if(rdamage < 1) rdamage = 1;
				}
			}
		}
		if(rdamage > 0)
			clif_damage(src,src,tick, dmg.amotion,0,rdamage,1,4,0);
	}
	if(attack_type&BF_MAGIC && damage > 0 && src != bl && src == dsrc) {	// 魔法スキル＆ダメージあり＆使用者と対象者が違う
		if(tsd) {	// 対象がPCの時
			if(tsd->magic_damage_return > 0 && atn_rand()%100 < tsd->magic_damage_return) {	// 魔法攻撃跳ね返し？※
				rdamage = damage;
				damage  = -1;	// ダメージ0だがmissを出さない
			}
		}
		// カイト
		if(damage > 0 && sc_data && sc_data[SC_KAITE].timer != -1)
		{
			if(src->type == BL_PC || (status_get_lv(src) < 80 && !(status_get_mode(src)&0x20)))
			{
				int idx;
				clif_misceffect2(bl,438);
				clif_skill_nodamage(bl,src,skillid,skilllv,1);
				if(--sc_data[SC_KAITE].val2 == 0)
					status_change_end(bl,SC_KAITE,-1);

				if( sd && ssc_data && ssc_data[SC_WIZARD].timer != -1 &&
				    (idx = pc_search_inventory(sd,7321)) >= 0 ) {
					pc_delitem(sd,idx,1,0);
				} else {
					rdamage += damage;
				}
				damage = -1;	// ダメージ0だがmissを出さない
			}
		}
		if(rdamage > 0) {
			//clif_damage(src,src,tick, dmg.amotion,0,rdamage,1,4,0);
			clif_skill_damage(src, src, tick, dmg.amotion, dmg.dmotion, rdamage, dmg.div_, skillid, lv, type);
			if(dmg.blewcount > 0 && !map[src->m].flag.gvg) {
				int dir = map_calc_dir(src,bl->x,bl->y);
				if(dir == 0)
					dir = 8;
				skill_blown(src,src,dmg.blewcount|(dir<<20));	// 対象に対する向きと逆方向に飛ばす
			}
			memset(&dmg,0,sizeof(dmg));
		}
	}

	/* ダメージパケット送信 */
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
	} else {	// ダメージ消失時はパケット送信しない
		damage = 0;
	}

	/* 吹き飛ばし処理とそのパケット */
	if(dmg.blewcount > 0 && bl->type != BL_SKILL && !map[src->m].flag.gvg) {
		skill_blown(dsrc,bl,dmg.blewcount);
	}
	/* 吹き飛ばし処理とそのパケット カード効果 */
	if(dsrc->type == BL_PC && bl->type != BL_SKILL && !map[src->m].flag.gvg) {
		skill_add_blown(dsrc,bl,skillid,SAB_REVERSEBLOW);
	}

	map_freeblock_lock();

	/* クローンスキル */
	if(damage > 0 && dmg.flag&BF_SKILL && tsd && pc_checkskill(tsd,RG_PLAGIARISM) && sc_data && sc_data[SC_PRESERVE].timer == -1) {
		skill_clone(tsd,skillid,skilllv);
	}

	/* 実際にダメージ処理を行う */
	if(skillid || flag) {
		if(attack_type&BF_WEAPON) {
			battle_delay_damage(tick+dmg.amotion,src,bl,damage,dmg.flag);
		} else {
			battle_damage(src,bl,damage,dmg.flag);

			/* ソウルドレイン */
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

	/* インティミデイト */
	if(skillid == RG_INTIMIDATE && damage > 0 && !(status_get_mode(bl)&0x20) && !map[src->m].flag.gvg) {
		int s_lv = status_get_lv(src),t_lv = status_get_lv(bl);
		int rate = 50 + skilllv * 5;
		rate = rate + s_lv - t_lv;
		if(atn_rand()%100 < rate)
			skill_addtimerskill(src,tick + 800,bl->id,0,0,skillid,skilllv,0,flag);
	}

	/* ダメージがあるなら追加効果判定 */
	if(bl->prev != NULL && !unit_isdead(bl)) {
		if(damage > 0 || skillid == TF_POISON) {
			// グラウンドドリフトはdsrcを引数として渡す
			if(skillid == GS_GROUNDDRIFT)
				skill_additional_effect(dsrc,bl,skillid,skilllv,attack_type,tick);
			else
				skill_additional_effect(src,bl,skillid,skilllv,attack_type,tick);
		}

		if(bl->type == BL_MOB && src != bl)	// スキル使用条件のMOBスキル
		{
			struct mob_data *md = (struct mob_data *)bl;
			if(md) {
				int target = md->target_id;
				if(battle_config.mob_changetarget_byskill == 1 || target == 0)
				{
					if(src->type == BL_PC || src->type == BL_HOM)
						md->target_id = src->id;
				}
				mobskill_use(md,tick,MSC_SKILLUSED|(skillid<<16));
				md->target_id = target;
			}
		}
	}

	/* HP,SP吸収 */
	if(sd && dmg.flag&BF_WEAPON && src != bl && src == dsrc && damage > 0) {
		// ％吸収のみ
		battle_attack_drain(src, dmg.damage, dmg.damage2, 1);
	}

	/* 反射ダメージの実際の処理 */
	if (sd && (skillid || flag) && rdamage > 0) {
		unsigned long asflag = EAS_WEAPON | EAS_ATTACK | EAS_NORMAL;

		if (attack_type&BF_WEAPON) {
			battle_delay_damage(tick+dmg.amotion,bl,src,rdamage,0);
			// 反射ダメージのオートスペル
			if(battle_config.weapon_reflect_autospell)
			{
				skill_bonus_autospell(bl,src,asflag,gettick(),0);
			}
			if(battle_config.weapon_reflect_drain && src != bl)
				battle_attack_drain(bl,rdamage,0,battle_config.weapon_reflect_drain_enable_type);
		} else {
			battle_damage(bl,src,rdamage,0);
			// 反射ダメージのオートスペル
			if(battle_config.magic_reflect_autospell)
			{
				skill_bonus_autospell(bl,src,asflag,gettick(),0);
			}
			if(battle_config.magic_reflect_drain && src != bl)
				battle_attack_drain(bl,rdamage,0,battle_config.magic_reflect_drain_enable_type);
		}
	}

	/* オートカウンター */
	if(attack_type&BF_WEAPON && sc_data && sc_data[SC_AUTOCOUNTER].timer != -1 && sc_data[SC_AUTOCOUNTER].val4 > 0) {
		if(sc_data[SC_AUTOCOUNTER].val3 == dsrc->id)
			battle_weapon_attack(bl,dsrc,tick,0x8000|sc_data[SC_AUTOCOUNTER].val1);
		status_change_end(bl,SC_AUTOCOUNTER,-1);
	}
	/* ダブルキャスティング */
	if ((skillid == MG_COLDBOLT || skillid == MG_FIREBOLT || skillid == MG_LIGHTNINGBOLT) &&
		ssc_data && ssc_data[SC_DOUBLECASTING].timer != -1 &&
		atn_rand() % 100 < 30+10*ssc_data[SC_DOUBLECASTING].val1) {
		if (!(flag & 1)) {
			//skill_castend_delay (src, bl, skillid, skilllv, tick + dmg.div_*dmg.amotion, flag|1);
			skill_castend_delay (src, bl, skillid, skilllv, tick + 100, flag|1);
		}
	}
	/* ブラッドラスト */
	if(src->type == BL_HOM && ssc_data && ssc_data[SC_BLOODLUST].timer != -1 && dmg.flag&BF_WEAPON && src != bl && src == dsrc && damage > 0)
	{
		struct homun_data *hd = (struct homun_data *)src;
		if(hd && atn_rand()%100 < ssc_data[SC_BLOODLUST].val1*9)
		{
			homun_heal(hd,damage/5,0);
		}
	}

	map_freeblock_unlock();

	return dmg.damage+dmg.damage2;	/* 与ダメを返す */
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
 * 不死判定
 *------------------------------------------
 */
int battle_check_undead(int race,int element)
{
	// element に属性値＋lv(status_get_element の戻り値)が渡されるミスに
	// 対応する為、elementから属性タイプだけを抜き出す。
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
 * 敵味方判定(1=肯定,0=否定,-1=エラー)
 * flag&0xf0000 = 0x00000:敵じゃないか判定（ret:1＝敵ではない）
 *              = 0x10000:パーティー判定（ret:1=パーティーメンバ)
 *              = 0x20000:全て(ret:1=敵味方両方)
 *              = 0x40000:敵か判定(ret:1=敵)
 *              = 0x50000:パーティーじゃないか判定(ret:1=パーティでない)
 *------------------------------------------
 */
int battle_check_target( struct block_list *src, struct block_list *target,int flag)
{
	int s_p,s_g,t_p,t_g;
	struct block_list *ss = src;

	nullpo_retr(-1, src);
	nullpo_retr(-1, target);

	if( flag&0x40000 ) {	// 反転フラグ
		int ret = battle_check_target(src,target,flag&0x30000);
		if(ret != -1)
			return !ret;
		return -1;
	}

	if( flag&0x20000 ) {
		if( target->type == BL_MOB || target->type == BL_PC )
			return 1;
		if( target->type == BL_HOM && src->type != BL_SKILL )	// ホムはスキルユニットの影響を受けない
			return 1;
		else
			return -1;
	}

	if(src->type == BL_SKILL && target->type == BL_SKILL)	// 対象がスキルユニットなら無条件肯定
		return -1;

	if(target->type == BL_PC && ((struct map_session_data *)target)->invincible_timer != -1)
		return -1;

	if(target->type == BL_SKILL) {
		switch(((struct skill_unit *)target)->group->unit_id) {
			case 0x8d:
			case 0x8f:
			case 0x98:
				return 0;
		}
	}

	if(target->type == BL_PET)
		return -1;

	// スキルユニットの場合、親を求める
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

	// Mobでmaster_idがあってspecial_mob_aiなら、召喚主を求める
	if( src->type == BL_MOB ) {
		struct mob_data *md = (struct mob_data *)src;
		if(md && md->master_id > 0) {
			if(md->master_id == target->id)	// 主なら肯定
				return 1;
			if(md->state.special_mob_ai) {
				if(target->type == BL_MOB) {	// special_mob_aiで対象がMob
					struct mob_data *tmd = (struct mob_data *)target;
					if(tmd) {
						if(tmd->master_id != md->master_id)	// 召喚主が一緒でなければ否定
							return 0;
						else if(md->state.special_mob_ai > 2)	// 召喚主が一緒なので肯定したいけど自爆は否定
							return 0;
						else
							return 1;
					}
				} else if(target->type == BL_HOM) {	// special_mob_aiで対象がHomならエラーで返す
					return -1;
				}
			}
			if((ss = map_id2bl(md->master_id)) == NULL)
				return -1;
		}
	}

	if( src == target || ss == target )	// 同じなら肯定
		return 1;

	if(target->type == BL_PC && pc_isinvisible((struct map_session_data *)target))
		return -1;

	if( src->prev == NULL || unit_isdead(src) ) // 死んでるならエラー
		return -1;

	if( (ss->type == BL_PC && target->type == BL_MOB) ||
		(ss->type == BL_MOB && target->type == BL_PC) )
		return 0;	// PCvsMOBなら敵

	if(ss->type == BL_PET && target->type == BL_MOB) {
		struct pet_data *pd = (struct pet_data*)ss;
		struct mob_data *md = (struct mob_data*)target;
		int mode = mob_db[pd->class_].mode;
		int race = mob_db[pd->class_].race;
		if(mob_db[pd->class_].mexp <= 0 && !(mode&0x20) && (md->option & 0x06 && race != RCT_INSECT && race != RCT_DEMON) ) {
			return 1; // 失敗
		} else {
			return 0; // 成功
		}
	}

	s_p = status_get_party_id(ss);
	s_g = status_get_guild_id(ss);

	t_p = status_get_party_id(target);
	t_g = status_get_guild_id(target);

	if(flag&0x10000) {
		if(s_p && t_p && s_p == t_p)
			return 1;	// 同じパーティなら肯定（味方）
		else
			return 0;	// パーティ検索なら同じパーティじゃない時点で否定
	}

	if(ss->type == BL_MOB && s_g > 0 && t_g > 0 && s_g == t_g )	// 同じギルド/mobクラスなら肯定（味方）
		return 1;

	if( ss->type == BL_PC && target->type == BL_PC) {	// 両方PVPモードなら否定（敵）
		struct skill_unit *su = NULL;
		if(src->type == BL_SKILL)
			su = (struct skill_unit *)src;
		// PK
		if(map[ss->m].flag.pk) {
			struct guild *g = NULL;
			struct map_session_data* ssd = (struct map_session_data*)ss;
			struct map_session_data* tsd = (struct map_session_data*)target;

			// battle_config.no_pk_level以下　1次は味方　転生は駄目
			if((ssd->sc_data && ssd->sc_data[SC_PK_PENALTY].timer != -1) ||
			   (ssd->status.base_level <= battle_config.no_pk_level && (ssd->s_class.job <= 6 || ssd->s_class.job == 24) && ssd->s_class.upper != 1))
				return 1;
			if(tsd->status.base_level <= battle_config.no_pk_level && (tsd->s_class.job <= 6 || tsd->s_class.job == 24) && tsd->s_class.upper != 1)
				return 1;
			if(su && su->group->target_flag == BCT_NOENEMY)
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
							return 0;	// 敵対ギルドなら無条件に敵
						else
							return 1;	// 同盟ギルドなら無条件に味方
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
			if( s_g > 0 && s_g == t_g)
				return 1;
			if(map[src->m].flag.gvg_noparty && s_p > 0 && t_p > 0 && s_p == t_p)
				return 1;
			if((g = guild_search(s_g)) != NULL) {
				int i;
				for(i=0; i<MAX_GUILDALLIANCE; i++) {
					if(g->alliance[i].guild_id > 0 && g->alliance[i].guild_id == t_g) {
						if(g->alliance[i].opposition)
							return 0;	// 敵対ギルドなら無条件に敵
						else
							return 1;	// 同盟ギルドなら無条件に味方
					}
				}
			}
			return 0;
		}
	}

	if( (ss->type == BL_HOM && target->type == BL_MOB) ||
	    (ss->type == BL_MOB && target->type == BL_HOM) )
		return 0;	// HOMvsMOBなら敵

	if(!(map[ss->m].flag.pvp || map[ss->m].flag.gvg) &&
	   ((ss->type == BL_PC && target->type == BL_HOM) ||
	   ( ss->type == BL_HOM && target->type == BL_PC)))
		return 1;	// PvでもGvでもないなら、PCvsHOMは味方

	// 同PTとか同盟Guildとかは後回し（＝＝
	if(ss->type == BL_HOM) {
		struct homun_data *hd = (struct homun_data *)ss;
		if(map[ss->m].flag.pvp) {	// PVP
			if(target->type == BL_HOM)
				return 0;
			if(target->type == BL_PC) {
				struct map_session_data *tsd = (struct map_session_data*)target;
				if(tsd != hd->msd)
					return 0;
			}
		}
		if(map[ss->m].flag.gvg) {	// GVG
			if(target->type == BL_HOM)
				return 0;
		}
	}
	if(ss->type == BL_PC && target->type == BL_HOM) {
		struct homun_data *hd = (struct homun_data *)target;
		if(map[ss->m].flag.pvp) {	// PVP
			if(ss != &hd->msd->bl) {
				return 0;
			}
		}
		if(map[ss->m].flag.gvg) {	// GVG
			return 0;
		}
	}
	return 1;	// 該当しないので無関係人物（まあ敵じゃないので味方）
}

/*==========================================
 * 射程判定
 *------------------------------------------
 */
int battle_check_range(struct block_list *src,struct block_list *bl,int range)
{
	int arange;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	arange = unit_distance(src->x,src->y,bl->x,bl->y);

	if(src->m != bl->m)	// 違うマップ
		return 0;

	if(range > 0 && range < arange)	// 遠すぎる
		return 0;

	if(arange < 2)	// 同じマスか隣接
		return 1;

	// 障害物判定
	return path_search_long(NULL,src->m,src->x,src->y,bl->x,bl->y);
}

/*==========================================
 * 矢の消費
 *------------------------------------------
 */
int battle_delarrow(struct map_session_data* sd,int num,int skillid)
{
	int mask = 0, idx = -1;

	nullpo_retr(0, sd);

	if(skillid == 0) {	// 通常攻撃
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
	} else {		// スキル攻撃
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
 * ダメージなしで共闘に参加
 *------------------------------------------
 */
void battle_join_struggle(struct mob_data *md,struct block_list *src)
{
	nullpo_retv(md);
	nullpo_retv(src);

	if(src->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)src;
		if(sd) {
			// ダメージ-1で戦闘参加者入り(0にするとリスト未登録のNULLとかぶって困る)
			if(linkdb_search( &md->dmglog, (void*)sd->status.char_id ) == NULL) {
				linkdb_insert( &md->dmglog, (void*)sd->status.char_id, (void*)-1 );
			}
		}
	}
	return;
}

/*==========================================
 * 設定ファイル読み込み用（フラグ）
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
 * 設定ファイルを読み込む
 *------------------------------------------
 */
int battle_config_read(const char *cfgName)
{
	int i;
	char line[1024],w1[1024],w2[1024];
	FILE *fp;
	static int count = 0;

	if( (count++) == 0 ) {
		battle_config.warp_point_debug = 0;
		battle_config.enemy_critical = 0;
		battle_config.enemy_critical_rate = 100;
		battle_config.enemy_str = 1;
		battle_config.enemy_perfect_flee = 0;
		battle_config.cast_rate = 100;
		battle_config.no_cast_dex = 150;
		battle_config.delay_rate = 100;
		battle_config.delay_dependon_dex = 0;
		battle_config.no_delay_dex = 150;
		battle_config.sdelay_attack_enable = 0;
		battle_config.left_cardfix_to_right = 0;
		battle_config.pc_skill_add_range = 0;
		battle_config.skill_out_range_consume = 1;
		battle_config.mob_skill_add_range = 0;
		battle_config.pc_damage_delay = 1;
		battle_config.pc_damage_delay_rate = 100;
		battle_config.defnotenemy = 1;
		battle_config.random_monster_checklv = 1;
		battle_config.attr_recover = 1;
		battle_config.flooritem_lifetime = 60*1000;
		battle_config.item_auto_get = 0;
		battle_config.item_first_get_time = 3000;
		battle_config.item_second_get_time = 1000;
		battle_config.item_third_get_time = 1000;
		battle_config.mvp_item_first_get_time = 10000;
		battle_config.mvp_item_second_get_time = 10000;
		battle_config.mvp_item_third_get_time = 2000;
		battle_config.item_rate = 100;
		battle_config.drop_rate0item = 0;
		battle_config.base_exp_rate = 100;
		battle_config.job_exp_rate = 100;
		battle_config.death_penalty_type = 0;
		battle_config.death_penalty_base = 0;
		battle_config.death_penalty_job = 0;
		battle_config.zeny_penalty = 0;
		battle_config.zeny_penalty_percent = 0;
		battle_config.zeny_penalty_by_lvl = 0;
		battle_config.restart_hp_rate = 0;
		battle_config.restart_sp_rate = 0;
		battle_config.mvp_item_rate = 100;
		battle_config.mvp_exp_rate = 100;
		battle_config.mvp_hp_rate = 100;
		battle_config.monster_hp_rate = 100;
		battle_config.monster_max_aspd = 199;
		battle_config.atc_gmonly = 0;
		battle_config.gm_allskill = 0;
		battle_config.gm_allskill_addabra = 0;
		battle_config.gm_allequip = 0;
		battle_config.gm_skilluncond = 0;
		battle_config.skillfree = 0;
		battle_config.skillup_limit = 0;
		battle_config.wp_rate = 100;
		battle_config.pp_rate = 100;
		battle_config.cdp_rate = 100;
		battle_config.monster_active_enable = 1;
		battle_config.monster_damage_delay_rate = 100;
		battle_config.monster_loot_type = 0;
		battle_config.mob_skill_use = 1;
		battle_config.mob_count_rate = 100;
		battle_config.mob_delay_rate = 100;
		battle_config.mob_middle_boss_delay_rate = 100;
		battle_config.mob_mvp_boss_delay_rate = 100;
		battle_config.quest_skill_learn = 0;
		battle_config.quest_skill_reset = 1;
		battle_config.basic_skill_check = 1;
		battle_config.guild_emperium_check = 1;
		battle_config.guild_exp_limit = 50;
		battle_config.guild_emblem_colors = 0;
		battle_config.pc_invincible_time  =  5000;
		battle_config.pet_catch_rate = 100;
		battle_config.pet_rename = 0;
		battle_config.pet_friendly_rate = 100;
		battle_config.pet_hungry_delay_rate = 100;
		battle_config.pet_hungry_friendly_decrease = 5;
		battle_config.pet_str = 1;
		battle_config.pet_status_support = 0;
		battle_config.pet_attack_support = 0;
		battle_config.pet_damage_support = 0;
		battle_config.pet_support_rate = 100;
		battle_config.pet_attack_exp_to_master = 0;
		battle_config.pet_attack_exp_rate = 100;
		battle_config.skill_min_damage = 0;
		battle_config.finger_offensive_type = 0;
		battle_config.heal_exp = 0;
		battle_config.resurrection_exp = 0;
		battle_config.shop_exp = 0;
		battle_config.combo_delay_rate = 100;
		battle_config.item_check = 1;
		battle_config.wedding_modifydisplay = 0;
		battle_config.natural_healhp_interval = 6000;
		battle_config.natural_healsp_interval = 8000;
		battle_config.natural_heal_skill_interval = 10000;
		battle_config.natural_heal_weight_rate = 50;
		battle_config.natural_heal_weight_rate_icon = 0;
		battle_config.item_name_override_grffile = 1;
		battle_config.arrow_decrement = 1;
		battle_config.allow_any_weapon_autoblitz = 0;
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
		battle_config.agi_penalty_type = 0;
		battle_config.agi_penalty_count = 3;
		battle_config.agi_penalty_num = 0;
		battle_config.agi_penalty_count_lv = ATK_FLEE;
		battle_config.vit_penalty_type = 0;
		battle_config.vit_penalty_count = 3;
		battle_config.vit_penalty_num = 0;
		battle_config.vit_penalty_count_lv = ATK_DEF;
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
		battle_config.party_skill_penalty = 1;
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
		battle_config.pet_loot_type = 1;
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
		battle_config.gvg_flee_penalty	= 0;
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
		battle_config.magic_attack_drain_enable_type = 2;
		battle_config.misc_attack_drain_enable_type = 2;
		battle_config.hallucianation_off = 0;
		battle_config.weapon_reflect_autospell = 0;
		battle_config.magic_reflect_autospell = 0;
		battle_config.weapon_reflect_drain = 0;
		battle_config.weapon_reflect_drain_enable_type = 2;
		battle_config.magic_reflect_drain = 0;
		battle_config.magic_reflect_drain_enable_type = 2;
		battle_config.max_parameter_str	= 999;
		battle_config.max_parameter_agi	= 999;
		battle_config.max_parameter_vit	= 999;
		battle_config.max_parameter_int	= 999;
		battle_config.max_parameter_dex	= 999;
		battle_config.max_parameter_luk	= 999;
		battle_config.cannibalize_nocost = 0;
		battle_config.spheremine_nocost	= 0;
		battle_config.demonstration_nocost = 0;
		battle_config.acidterror_nocost = 0;
		battle_config.aciddemonstration_nocost = 0;
		battle_config.chemical_nocost = 0;
		battle_config.slimpitcher_nocost = 0;
		battle_config.mes_send_type = 0;
		battle_config.allow_assumptop_in_gvg = 1;
		battle_config.allow_falconassault_elemet = 0;
		battle_config.allow_guild_invite_in_gvg = 1;
		battle_config.allow_guild_leave_in_gvg  = 1;
		battle_config.guild_skill_available = 1;
		battle_config.guild_hunting_skill_available = 1;
		battle_config.guild_skill_check_range = 0;
		battle_config.allow_guild_skill_in_gvg_only = 1;
		battle_config.allow_me_guild_skill = 0;
		battle_config.emergencycall_point_type = 1;
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
		battle_config.marionette_type = 0;
		battle_config.max_marionette_str = 99;
		battle_config.max_marionette_agi = 99;
		battle_config.max_marionette_vit = 99;
		battle_config.max_marionette_int = 99;
		battle_config.max_marionette_dex = 99;
		battle_config.max_marionette_luk = 99;
		battle_config.max_marionette_pk_str = 99;
		battle_config.max_marionette_pk_agi = 99;
		battle_config.max_marionette_pk_vit = 99;
		battle_config.max_marionette_pk_int = 99;
		battle_config.max_marionette_pk_dex = 99;
		battle_config.max_marionette_pk_luk = 99;
		battle_config.max_marionette_pvp_str = 99;
		battle_config.max_marionette_pvp_agi = 99;
		battle_config.max_marionette_pvp_vit = 99;
		battle_config.max_marionette_pvp_int = 99;
		battle_config.max_marionette_pvp_dex = 99;
		battle_config.max_marionette_pvp_luk = 99;
		battle_config.max_marionette_gvg_str = 99;
		battle_config.max_marionette_gvg_agi = 99;
		battle_config.max_marionette_gvg_vit = 99;
		battle_config.max_marionette_gvg_int = 99;
		battle_config.max_marionette_gvg_dex = 99;
		battle_config.max_marionette_gvg_luk = 99;
		battle_config.baby_status_max = 80;
		battle_config.baby_hp_rate = 70;
		battle_config.baby_sp_rate = 70;
		battle_config.upper_hp_rate = 125;
		battle_config.upper_sp_rate = 125;
		battle_config.normal_hp_rate = 100;
		battle_config.normal_sp_rate = 100;
		battle_config.baby_weight_rate = 100;
		battle_config.no_emergency_call = 1;
		battle_config.save_am_pharmacy_success = 0;
		battle_config.save_all_ranking_point_when_logout = 0;
		battle_config.soul_linker_battle_mode = 0;
		battle_config.soul_linker_battle_mode_ka = 0;
		battle_config.skillup_type = 1;
		battle_config.allow_me_dance_effect = 0;
		battle_config.allow_me_concert_effect = 0;
		battle_config.allow_me_rokisweil = 0;
		battle_config.pharmacy_get_point_type = 0;
		battle_config.soulskill_can_be_used_for_myself = 1;
		battle_config.hermode_wp_check_range = 3;
		battle_config.hermode_wp_check = 1;
		battle_config.hermode_no_walking = 0;
		battle_config.hermode_gvg_only = 1;
		battle_config.atcommand_go_significant_values = 21;
		battle_config.redemptio_penalty_type = 1;
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
		battle_config.no_pk_level = 60;
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
		battle_config.save_pckiller_type = 0;
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
		battle_config.save_player_when_storage_closed = 0;
		battle_config.allow_homun_status_change = 1;
		battle_config.save_homun_temporal_intimate = 1;
		battle_config.homun_intimate_rate = 100;
		battle_config.homun_temporal_intimate_resilience = 50;
		battle_config.hvan_explosion_intimate = 45000;
		battle_config.homun_speed_is_same_as_pc = 1;
		battle_config.homun_skill_intimate_type = 0;
		battle_config.master_get_homun_base_exp = 100;
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
		battle_config.personal_storage_sort = 1;
		battle_config.guild_storage_sort = 1;
		battle_config.allow_es_magic_all = 0;
		battle_config.trap_is_invisible = 0;
		battle_config.gm_perfect_hide = 0;
		battle_config.pcview_mob_clear_type = 1;
		battle_config.party_item_share_type = 1;
		battle_config.party_item_share_show = 0;
		battle_config.pk_murderer_point = 100;
		battle_config.sg_miracle_rate = 1;
		battle_config.baby_copy_skilltree = 1;
	}

	fp = fopen(cfgName,"r");
	if(fp == NULL) {
		printf("file not found: %s\n",cfgName);
		return 1;
	}
	while(fgets(line,1020,fp)) {
		static const struct {
			const char *str;
			int *val;
		} data[] = {
			{ "warp_point_debug",                   &battle_config.warp_point_debug                   },
			{ "enemy_critical",                     &battle_config.enemy_critical                     },
			{ "enemy_critical_rate",                &battle_config.enemy_critical_rate                },
			{ "enemy_str",                          &battle_config.enemy_str                          },
			{ "enemy_perfect_flee",                 &battle_config.enemy_perfect_flee                 },
			{ "casting_rate",                       &battle_config.cast_rate                          },
			{ "no_casting_dex",                     &battle_config.no_cast_dex                        },
			{ "delay_rate",                         &battle_config.delay_rate                         },
			{ "delay_dependon_dex",                 &battle_config.delay_dependon_dex                 },
			{ "no_delay_dex",                       &battle_config.no_delay_dex                       },
			{ "skill_delay_attack_enable",          &battle_config.sdelay_attack_enable               },
			{ "left_cardfix_to_right",              &battle_config.left_cardfix_to_right              },
			{ "player_skill_add_range",             &battle_config.pc_skill_add_range                 },
			{ "skill_out_range_consume",            &battle_config.skill_out_range_consume            },
			{ "monster_skill_add_range",            &battle_config.mob_skill_add_range                },
			{ "player_damage_delay",                &battle_config.pc_damage_delay                    },
			{ "player_damage_delay_rate",           &battle_config.pc_damage_delay_rate               },
			{ "defunit_not_enemy",                  &battle_config.defnotenemy                        },
			{ "random_monster_checklv",             &battle_config.random_monster_checklv             },
			{ "attribute_recover",                  &battle_config.attr_recover                       },
			{ "flooritem_lifetime",                 &battle_config.flooritem_lifetime                 },
			{ "item_auto_get",                      &battle_config.item_auto_get                      },
			{ "item_first_get_time",                &battle_config.item_first_get_time                },
			{ "item_second_get_time",               &battle_config.item_second_get_time               },
			{ "item_third_get_time",                &battle_config.item_third_get_time                },
			{ "mvp_item_first_get_time",            &battle_config.mvp_item_first_get_time            },
			{ "mvp_item_second_get_time",           &battle_config.mvp_item_second_get_time           },
			{ "mvp_item_third_get_time",            &battle_config.mvp_item_third_get_time            },
			{ "item_rate",                          &battle_config.item_rate                          },
			{ "drop_rate0item",                     &battle_config.drop_rate0item                     },
			{ "base_exp_rate",                      &battle_config.base_exp_rate                      },
			{ "job_exp_rate",                       &battle_config.job_exp_rate                       },
			{ "death_penalty_type",                 &battle_config.death_penalty_type                 },
			{ "death_penalty_base",                 &battle_config.death_penalty_base                 },
			{ "death_penalty_job",                  &battle_config.death_penalty_job                  },
			{ "zeny_penalty",                       &battle_config.zeny_penalty                       },
			{ "zeny_penalty_percent",               &battle_config.zeny_penalty_percent               },
			{ "zeny_penalty_by_lvl",                &battle_config.zeny_penalty_by_lvl                },
			{ "restart_hp_rate",                    &battle_config.restart_hp_rate                    },
			{ "restart_sp_rate",                    &battle_config.restart_sp_rate                    },
			{ "mvp_hp_rate",                        &battle_config.mvp_hp_rate                        },
			{ "mvp_item_rate",                      &battle_config.mvp_item_rate                      },
			{ "mvp_exp_rate",                       &battle_config.mvp_exp_rate                       },
			{ "monster_hp_rate",                    &battle_config.monster_hp_rate                    },
			{ "monster_max_aspd",                   &battle_config.monster_max_aspd                   },
			{ "atcommand_gm_only",                  &battle_config.atc_gmonly                         },
			{ "gm_all_skill",                       &battle_config.gm_allskill                        },
			{ "gm_all_skill_add_abra",              &battle_config.gm_allskill_addabra                },
			{ "gm_all_equipment",                   &battle_config.gm_allequip                        },
			{ "gm_skill_unconditional",             &battle_config.gm_skilluncond                     },
			{ "player_skillfree",                   &battle_config.skillfree                          },
			{ "player_skillup_limit",               &battle_config.skillup_limit                      },
			{ "weapon_produce_rate",                &battle_config.wp_rate                            },
			{ "potion_produce_rate",                &battle_config.pp_rate                            },
			{ "deadly_potion_produce_rate",         &battle_config.cdp_rate                           },
			{ "monster_active_enable",              &battle_config.monster_active_enable              },
			{ "monster_damage_delay_rate",          &battle_config.monster_damage_delay_rate          },
			{ "monster_loot_type",                  &battle_config.monster_loot_type                  },
			{ "mob_skill_use",                      &battle_config.mob_skill_use                      },
			{ "mob_count_rate",                     &battle_config.mob_count_rate                     },
			{ "mob_delay_rate",                     &battle_config.mob_delay_rate                     },
			{ "mob_middle_boss_delay_rate",         &battle_config.mob_middle_boss_delay_rate         },
			{ "mob_mvp_boss_delay_rate",            &battle_config.mob_mvp_boss_delay_rate            },
			{ "quest_skill_learn",                  &battle_config.quest_skill_learn                  },
			{ "quest_skill_reset",                  &battle_config.quest_skill_reset                  },
			{ "basic_skill_check",                  &battle_config.basic_skill_check                  },
			{ "guild_emperium_check",               &battle_config.guild_emperium_check               },
			{ "guild_emblem_colors",                &battle_config.guild_emblem_colors                },
			{ "guild_exp_limit",                    &battle_config.guild_exp_limit                    },
			{ "player_invincible_time" ,            &battle_config.pc_invincible_time                 },
			{ "pet_catch_rate",                     &battle_config.pet_catch_rate                     },
			{ "pet_rename",                         &battle_config.pet_rename                         },
			{ "pet_friendly_rate",                  &battle_config.pet_friendly_rate                  },
			{ "pet_hungry_delay_rate",              &battle_config.pet_hungry_delay_rate              },
			{ "pet_hungry_friendly_decrease",       &battle_config.pet_hungry_friendly_decrease       },
			{ "pet_str",                            &battle_config.pet_str                            },
			{ "pet_status_support",                 &battle_config.pet_status_support                 },
			{ "pet_attack_support",                 &battle_config.pet_attack_support                 },
			{ "pet_damage_support",                 &battle_config.pet_damage_support                 },
			{ "pet_support_rate",                   &battle_config.pet_support_rate                   },
			{ "pet_attack_exp_to_master",           &battle_config.pet_attack_exp_to_master           },
			{ "pet_attack_exp_rate",                &battle_config.pet_attack_exp_rate                },
			{ "skill_min_damage",                   &battle_config.skill_min_damage                   },
			{ "finger_offensive_type",              &battle_config.finger_offensive_type              },
			{ "heal_exp",                           &battle_config.heal_exp                           },
			{ "resurrection_exp",                   &battle_config.resurrection_exp                   },
			{ "shop_exp",                           &battle_config.shop_exp                           },
			{ "combo_delay_rate",                   &battle_config.combo_delay_rate                   },
			{ "item_check",                         &battle_config.item_check                         },
			{ "wedding_modifydisplay",              &battle_config.wedding_modifydisplay              },
			{ "natural_healhp_interval",            &battle_config.natural_healhp_interval            },
			{ "natural_healsp_interval",            &battle_config.natural_healsp_interval            },
			{ "natural_heal_skill_interval",        &battle_config.natural_heal_skill_interval        },
			{ "natural_heal_weight_rate",           &battle_config.natural_heal_weight_rate           },
			{ "natural_heal_weight_rate_icon",      &battle_config.natural_heal_weight_rate_icon      },
			{ "item_name_override_grffile",         &battle_config.item_name_override_grffile         },
			{ "arrow_decrement",                    &battle_config.arrow_decrement                    },
			{ "allow_any_weapon_autoblitz",         &battle_config.allow_any_weapon_autoblitz         },
			{ "max_aspd",                           &battle_config.max_aspd                           },
			{ "pk_max_aspd",                        &battle_config.pk_max_aspd                        },
			{ "gvg_max_aspd",                       &battle_config.gvg_max_aspd                       },
			{ "pvp_max_aspd",                       &battle_config.pvp_max_aspd                       },
			{ "max_hp",                             &battle_config.max_hp                             },
			{ "max_sp",                             &battle_config.max_sp                             },
			{ "max_parameter",                      &battle_config.max_parameter                      },
			{ "max_cart_weight",                    &battle_config.max_cart_weight                    },
			{ "player_skill_log",                   &battle_config.pc_skill_log                       },
			{ "monster_skill_log",                  &battle_config.mob_skill_log                      },
			{ "battle_log",                         &battle_config.battle_log                         },
			{ "save_log",                           &battle_config.save_log                           },
			{ "error_log",                          &battle_config.error_log                          },
			{ "etc_log",                            &battle_config.etc_log                            },
			{ "save_clothcolor",                    &battle_config.save_clothcolor                    },
			{ "undead_detect_type",                 &battle_config.undead_detect_type                 },
			{ "player_auto_counter_type",           &battle_config.pc_auto_counter_type               },
			{ "monster_auto_counter_type",          &battle_config.monster_auto_counter_type          },
			{ "min_hitrate",                        &battle_config.min_hitrate                        },
			{ "agi_penalty_type",                   &battle_config.agi_penalty_type                   },
			{ "agi_penalty_count",                  &battle_config.agi_penalty_count                  },
			{ "agi_penalty_num",                    &battle_config.agi_penalty_num                    },
			{ "agi_penalty_count_lv",               &battle_config.agi_penalty_count_lv               },
			{ "vit_penalty_type",                   &battle_config.vit_penalty_type                   },
			{ "vit_penalty_count",                  &battle_config.vit_penalty_count                  },
			{ "vit_penalty_num",                    &battle_config.vit_penalty_num                    },
			{ "vit_penalty_count_lv",               &battle_config.vit_penalty_count_lv               },
			{ "player_defense_type",                &battle_config.player_defense_type                },
			{ "monster_defense_type",               &battle_config.monster_defense_type               },
			{ "pet_defense_type",                   &battle_config.pet_defense_type                   },
			{ "magic_defense_type",                 &battle_config.magic_defense_type                 },
			{ "player_skill_reiteration",           &battle_config.pc_skill_reiteration               },
			{ "monster_skill_reiteration",          &battle_config.monster_skill_reiteration          },
			{ "player_skill_nofootset",             &battle_config.pc_skill_nofootset                 },
			{ "monster_skill_nofootset",            &battle_config.monster_skill_nofootset            },
			{ "player_cloak_check_type",            &battle_config.pc_cloak_check_type                },
			{ "monster_cloak_check_type",           &battle_config.monster_cloak_check_type           },
			{ "gvg_short_attack_damage_rate",       &battle_config.gvg_short_damage_rate              },
			{ "gvg_long_attack_damage_rate",        &battle_config.gvg_long_damage_rate               },
			{ "gvg_magic_attack_damage_rate",       &battle_config.gvg_magic_damage_rate              },
			{ "gvg_misc_attack_damage_rate",        &battle_config.gvg_misc_damage_rate               },
			{ "gvg_eliminate_time",                 &battle_config.gvg_eliminate_time                 },
			{ "mob_changetarget_byskill",           &battle_config.mob_changetarget_byskill           },
			{ "gvg_edp_down_rate",                  &battle_config.gvg_edp_down_rate                  },
			{ "pvp_edp_down_rate",                  &battle_config.pvp_edp_down_rate                  },
			{ "pk_edp_down_rate",                   &battle_config.pk_edp_down_rate                   },
			{ "player_attack_direction_change",     &battle_config.pc_attack_direction_change         },
			{ "monster_attack_direction_change",    &battle_config.monster_attack_direction_change    },
			{ "player_land_skill_limit",            &battle_config.pc_land_skill_limit                },
			{ "monster_land_skill_limit",           &battle_config.monster_land_skill_limit           },
			{ "party_skill_penalty",                &battle_config.party_skill_penalty                },
			{ "monster_class_change_full_recover",  &battle_config.monster_class_change_full_recover  },
			{ "produce_item_name_input",            &battle_config.produce_item_name_input            },
			{ "produce_potion_name_input",          &battle_config.produce_potion_name_input          },
			{ "making_arrow_name_input",            &battle_config.making_arrow_name_input            },
			{ "holywater_name_input",               &battle_config.holywater_name_input               },
			{ "display_delay_skill_fail",           &battle_config.display_delay_skill_fail           },
			{ "display_snatcher_skill_fail",        &battle_config.display_snatcher_skill_fail        },
			{ "chat_warpportal",                    &battle_config.chat_warpportal                    },
			{ "mob_warpportal",                     &battle_config.mob_warpportal                     },
			{ "dead_branch_active",                 &battle_config.dead_branch_active                 },
			{ "vending_max_value",                  &battle_config.vending_max_value                  },
			{ "pet_lootitem",                       &battle_config.pet_lootitem                       },
			{ "pet_weight",                         &battle_config.pet_weight                         },
			{ "show_steal_in_same_party",           &battle_config.show_steal_in_same_party           },
			{ "enable_upper_class",                 &battle_config.enable_upper_class                 },
			{ "pet_attack_attr_none",               &battle_config.pet_attack_attr_none               },
			{ "mob_attack_attr_none",               &battle_config.mob_attack_attr_none               },
			{ "pc_attack_attr_none",                &battle_config.pc_attack_attr_none                },
			{ "gx_allhit",                          &battle_config.gx_allhit                          },
			{ "gx_cardfix",                         &battle_config.gx_cardfix                         },
			{ "gx_dupele",                          &battle_config.gx_dupele                          },
			{ "gx_disptype",                        &battle_config.gx_disptype                        },
			{ "devotion_level_difference",          &battle_config.devotion_level_difference          },
			{ "player_skill_partner_check",         &battle_config.player_skill_partner_check         },
			{ "sole_concert_type",                  &battle_config.sole_concert_type                  },
			{ "hide_GM_session",                    &battle_config.hide_GM_session                    },
			{ "invite_request_check",               &battle_config.invite_request_check               },
			{ "gvg_trade_request_refused",          &battle_config.gvg_trade_request_refused          },
			{ "pvp_trade_request_refused",          &battle_config.pvp_trade_request_refused          },
			{ "skill_removetrap_type",              &battle_config.skill_removetrap_type              },
			{ "disp_experience",                    &battle_config.disp_experience                    },
			{ "castle_defense_rate",                &battle_config.castle_defense_rate                },
			{ "riding_weight",                      &battle_config.riding_weight                      },
			{ "hp_rate",                            &battle_config.hp_rate                            },
			{ "sp_rate",                            &battle_config.sp_rate                            },
			{ "gm_can_drop_lv",                     &battle_config.gm_can_drop_lv                     },
			{ "disp_hpmeter",                       &battle_config.disp_hpmeter                       },
			{ "bone_drop",                          &battle_config.bone_drop                          },
			{ "bone_drop_itemid",                   &battle_config.bone_drop_itemid                   },
			{ "item_rate_details",                  &battle_config.item_rate_details                  },
			{ "item_rate_1",                        &battle_config.item_rate_1                        },
			{ "item_rate_10",                       &battle_config.item_rate_10                       },
			{ "item_rate_100",                      &battle_config.item_rate_100                      },
			{ "item_rate_1000",                     &battle_config.item_rate_1000                     },
			{ "item_rate_1_min",                    &battle_config.item_rate_1_min                    },
			{ "item_rate_1_max",                    &battle_config.item_rate_1_max                    },
			{ "item_rate_10_min",                   &battle_config.item_rate_10_min                   },
			{ "item_rate_10_max",                   &battle_config.item_rate_10_max                   },
			{ "item_rate_100_min",                  &battle_config.item_rate_100_min                  },
			{ "item_rate_100_max",                  &battle_config.item_rate_100_max                  },
			{ "item_rate_1000_min",                 &battle_config.item_rate_1000_min                 },
			{ "item_rate_1000_max",                 &battle_config.item_rate_1000_max                 },
			{ "monster_damage_delay",               &battle_config.monster_damage_delay               },
			{ "card_drop_rate",                     &battle_config.card_drop_rate                     },
			{ "equip_drop_rate",                    &battle_config.equip_drop_rate                    },
			{ "consume_drop_rate",                  &battle_config.consume_drop_rate                  },
			{ "refine_drop_rate",                   &battle_config.refine_drop_rate                   },
			{ "etc_drop_rate",                      &battle_config.etc_drop_rate                      },
			{ "potion_drop_rate",                   &battle_config.potion_drop_rate                   },
			{ "arrow_drop_rate",                    &battle_config.arrow_drop_rate                    },
			{ "petequip_drop_rate",                 &battle_config.petequip_drop_rate                 },
			{ "weapon_drop_rate",                   &battle_config.weapon_drop_rate                   },
			{ "other_drop_rate",                    &battle_config.other_drop_rate                    },
			{ "Item_res",                           &battle_config.Item_res                           },
			{ "next_exp_limit",                     &battle_config.next_exp_limit                     },
			{ "heal_counterstop",                   &battle_config.heal_counterstop                   },
			{ "finding_ore_drop_rate",              &battle_config.finding_ore_drop_rate              },
			{ "joint_struggle_exp_bonus",           &battle_config.joint_struggle_exp_bonus           },
			{ "joint_struggle_limit",               &battle_config.joint_struggle_limit               },
			{ "pt_bonus_b",                         &battle_config.pt_bonus_b                         },
			{ "pt_bonus_j",                         &battle_config.pt_bonus_j                         },
			{ "mvp_announce",                       &battle_config.mvp_announce                       },
			{ "petowneditem",                       &battle_config.petowneditem                       },
			{ "pet_loot_type",                      &battle_config.pet_loot_type                      },
			{ "buyer_name",                         &battle_config.buyer_name                         },
			{ "noportal_flag",                      &battle_config.noportal_flag                      },
			{ "once_autospell",                     &battle_config.once_autospell                     },
			{ "allow_same_autospell",               &battle_config.allow_same_autospell               },
			{ "combo_delay_lower_limits",           &battle_config.combo_delay_lower_limits           },
			{ "new_marrige_skill",                  &battle_config.new_marrige_skill                  },
			{ "reveff_plus_addeff",                 &battle_config.reveff_plus_addeff                 },
			{ "summonslave_no_drop",                &battle_config.summonslave_no_drop                },
			{ "summonslave_no_exp",                 &battle_config.summonslave_no_exp                 },
			{ "summonslave_no_mvp",                 &battle_config.summonslave_no_mvp                 },
			{ "summonmonster_no_drop",              &battle_config.summonmonster_no_drop              },
			{ "summonmonster_no_exp",               &battle_config.summonmonster_no_exp               },
			{ "summonmonster_no_mvp",               &battle_config.summonmonster_no_mvp               },
			{ "cannibalize_no_drop",                &battle_config.cannibalize_no_drop                },
			{ "cannibalize_no_exp",                 &battle_config.cannibalize_no_exp                 },
			{ "cannibalize_no_mvp",                 &battle_config.cannibalize_no_mvp                 },
			{ "spheremine_no_drop",                 &battle_config.spheremine_no_drop                 },
			{ "spheremine_no_exp",                  &battle_config.spheremine_no_exp                  },
			{ "spheremine_no_mvp",                  &battle_config.spheremine_no_mvp                  },
			{ "branch_mob_no_drop",                 &battle_config.branch_mob_no_drop                 },
			{ "branch_mob_no_exp",                  &battle_config.branch_mob_no_exp                  },
			{ "branch_mob_no_mvp",                  &battle_config.branch_mob_no_mvp                  },
			{ "branch_boss_no_drop",                &battle_config.branch_boss_no_drop                },
			{ "branch_boss_no_exp",                 &battle_config.branch_boss_no_exp                 },
			{ "branch_boss_no_mvp",                 &battle_config.branch_boss_no_mvp                 },
			{ "pc_hit_stop_type",                   &battle_config.pc_hit_stop_type                   },
			{ "nomanner_mode",                      &battle_config.nomanner_mode                      },
			{ "death_by_unrig_penalty",             &battle_config.death_by_unrig_penalty             },
			{ "dance_and_play_duration",            &battle_config.dance_and_play_duration            },
			{ "soulcollect_max_fail",               &battle_config.soulcollect_max_fail               },
			{ "gvg_flee_rate",                      &battle_config.gvg_flee_rate                      },
			{ "gvg_flee_penalty",                   &battle_config.gvg_flee_penalty                   },
			{ "equip_sex",                          &battle_config.equip_sex                          },
			{ "noexp_hiding",                       &battle_config.noexp_hiding                       },
			{ "noexp_trickdead",                    &battle_config.noexp_trickdead                    },
			{ "hide_attack",                        &battle_config.hide_attack                        },
			{ "gm_hide_attack_lv",                  &battle_config.gm_hide_attack_lv                  },
			{ "weapon_attack_autospell",            &battle_config.weapon_attack_autospell            },
			{ "magic_attack_autospell",             &battle_config.magic_attack_autospell             },
			{ "misc_attack_autospell",              &battle_config.misc_attack_autospell              },
			{ "magic_attack_drain",                 &battle_config.magic_attack_drain                 },
			{ "misc_attack_drain",                  &battle_config.misc_attack_drain                  },
			{ "magic_attack_drain_enable_type",     &battle_config.magic_attack_drain_enable_type     },
			{ "misc_attack_drain_enable_type",      &battle_config.misc_attack_drain_enable_type      },
			{ "hallucianation_off",                 &battle_config.hallucianation_off                 },
			{ "weapon_reflect_autospell",           &battle_config.weapon_reflect_autospell           },
			{ "magic_reflect_autospell",            &battle_config.magic_reflect_autospell            },
			{ "weapon_reflect_drain",               &battle_config.weapon_reflect_drain               },
			{ "weapon_reflect_drain_enable_type",   &battle_config.weapon_reflect_drain_enable_type   },
			{ "magic_reflect_drain",                &battle_config.magic_reflect_drain                },
			{ "magic_reflect_drain_enable_type",    &battle_config.magic_reflect_drain_enable_type    },
			{ "max_parameter_str",                  &battle_config.max_parameter_str                  },
			{ "max_parameter_agi",                  &battle_config.max_parameter_agi                  },
			{ "max_parameter_vit",                  &battle_config.max_parameter_vit                  },
			{ "max_parameter_int",                  &battle_config.max_parameter_int                  },
			{ "max_parameter_dex",                  &battle_config.max_parameter_dex                  },
			{ "max_parameter_luk",                  &battle_config.max_parameter_luk                  },
			{ "cannibalize_nocost",                 &battle_config.cannibalize_nocost                 },
			{ "spheremine_nocost",                  &battle_config.spheremine_nocost                  },
			{ "demonstration_nocost",               &battle_config.demonstration_nocost               },
			{ "acidterror_nocost",                  &battle_config.acidterror_nocost                  },
			{ "aciddemonstration_nocost",           &battle_config.aciddemonstration_nocost           },
			{ "chemical_nocost",                    &battle_config.chemical_nocost                    },
			{ "slimpitcher_nocost",                 &battle_config.slimpitcher_nocost                 },
			{ "mes_send_type",                      &battle_config.mes_send_type                      },
			{ "allow_assumptop_in_gvg",             &battle_config.allow_assumptop_in_gvg             },
			{ "allow_falconassault_elemet",         &battle_config.allow_falconassault_elemet         },
			{ "allow_guild_invite_in_gvg",          &battle_config.allow_guild_invite_in_gvg          },
			{ "allow_guild_leave_in_gvg",           &battle_config.allow_guild_leave_in_gvg           },
			{ "guild_skill_available",              &battle_config.guild_skill_available              },
			{ "guild_hunting_skill_available",      &battle_config.guild_hunting_skill_available      },
			{ "guild_skill_check_range",            &battle_config.guild_skill_check_range            },
			{ "allow_guild_skill_in_gvg_only",      &battle_config.allow_guild_skill_in_gvg_only      },
			{ "allow_me_guild_skill",               &battle_config.allow_me_guild_skill               },
			{ "emergencycall_point_type",           &battle_config.emergencycall_point_type           },
			{ "emergencycall_call_limit",           &battle_config.emergencycall_call_limit           },
			{ "allow_guild_skill_in_gvgtime_only",  &battle_config.allow_guild_skill_in_gvgtime_only  },
			{ "guild_skill_in_pvp_limit",           &battle_config.guild_skill_in_pvp_limit           },
			{ "guild_exp_rate",                     &battle_config.guild_exp_rate                     },
			{ "guild_skill_effective_range",        &battle_config.guild_skill_effective_range        },
			{ "tarotcard_display_position",         &battle_config.tarotcard_display_position         },
			{ "serverside_friendlist",              &battle_config.serverside_friendlist              },
			{ "pet0078_hair_id",                    &battle_config.pet0078_hair_id                    },
			{ "job_soul_check",                     &battle_config.job_soul_check                     },
			{ "repeal_die_counter_rate",            &battle_config.repeal_die_counter_rate            },
			{ "disp_job_soul_state_change",         &battle_config.disp_job_soul_state_change         },
			{ "check_knowlege_map",                 &battle_config.check_knowlege_map                 },
			{ "tripleattack_rate_up_keeptime",      &battle_config.tripleattack_rate_up_keeptime      },
			{ "tk_counter_rate_up_keeptime",        &battle_config.tk_counter_rate_up_keeptime        },
			{ "allow_skill_without_day",            &battle_config.allow_skill_without_day            },
			{ "save_feel_map",                      &battle_config.save_feel_map                      },
			{ "save_hate_mob",                      &battle_config.save_hate_mob                      },
			{ "twilight_party_check",               &battle_config.twilight_party_check               },
			{ "alchemist_point_type",               &battle_config.alchemist_point_type               },
			{ "marionette_type",                    &battle_config.marionette_type                    },
			{ "max_marionette_str",                 &battle_config.max_marionette_str                 },
			{ "max_marionette_agi",                 &battle_config.max_marionette_agi                 },
			{ "max_marionette_vit",                 &battle_config.max_marionette_vit                 },
			{ "max_marionette_int",                 &battle_config.max_marionette_int                 },
			{ "max_marionette_dex",                 &battle_config.max_marionette_dex                 },
			{ "max_marionette_luk",                 &battle_config.max_marionette_luk                 },
			{ "max_marionette_pk_str",              &battle_config.max_marionette_pk_str              },
			{ "max_marionette_pk_agi",              &battle_config.max_marionette_pk_agi              },
			{ "max_marionette_pk_vit",              &battle_config.max_marionette_pk_vit              },
			{ "max_marionette_pk_int",              &battle_config.max_marionette_pk_int              },
			{ "max_marionette_pk_dex",              &battle_config.max_marionette_pk_dex              },
			{ "max_marionette_pk_luk",              &battle_config.max_marionette_pk_luk              },
			{ "max_marionette_pvp_str",             &battle_config.max_marionette_pvp_str             },
			{ "max_marionette_pvp_agi",             &battle_config.max_marionette_pvp_agi             },
			{ "max_marionette_pvp_vit",             &battle_config.max_marionette_pvp_vit             },
			{ "max_marionette_pvp_int",             &battle_config.max_marionette_pvp_int             },
			{ "max_marionette_pvp_dex",             &battle_config.max_marionette_pvp_dex             },
			{ "max_marionette_pvp_luk",             &battle_config.max_marionette_pvp_luk             },
			{ "max_marionette_gvg_str",             &battle_config.max_marionette_gvg_str             },
			{ "max_marionette_gvg_agi",             &battle_config.max_marionette_gvg_agi             },
			{ "max_marionette_gvg_vit",             &battle_config.max_marionette_gvg_vit             },
			{ "max_marionette_gvg_int",             &battle_config.max_marionette_gvg_int             },
			{ "max_marionette_gvg_dex",             &battle_config.max_marionette_gvg_dex             },
			{ "max_marionette_gvg_luk",             &battle_config.max_marionette_gvg_luk             },
			{ "baby_status_max",                    &battle_config.baby_status_max                    },
			{ "baby_hp_rate",                       &battle_config.baby_hp_rate                       },
			{ "baby_sp_rate",                       &battle_config.baby_sp_rate                       },
			{ "upper_hp_rate",                      &battle_config.upper_hp_rate                      },
			{ "upper_sp_rate",                      &battle_config.upper_sp_rate                      },
			{ "normal_hp_rate",                     &battle_config.normal_hp_rate                     },
			{ "normal_sp_rate",                     &battle_config.normal_sp_rate                     },
			{ "baby_weight_rate",                   &battle_config.baby_weight_rate                   },
			{ "no_emergency_call",                  &battle_config.no_emergency_call                  },
			{ "save_am_pharmacy_success",           &battle_config.save_am_pharmacy_success           },
			{ "save_all_ranking_point_when_logout", &battle_config.save_all_ranking_point_when_logout },
			{ "soul_linker_battle_mode",            &battle_config.soul_linker_battle_mode            },
			{ "soul_linker_battle_mode_ka",         &battle_config.soul_linker_battle_mode_ka         },
			{ "skillup_type",                       &battle_config.skillup_type                       },
			{ "allow_me_dance_effect",              &battle_config.allow_me_dance_effect              },
			{ "allow_me_concert_effect",            &battle_config.allow_me_concert_effect            },
			{ "allow_me_rokisweil",                 &battle_config.allow_me_rokisweil                 },
			{ "pharmacy_get_point_type",            &battle_config.pharmacy_get_point_type            },
			{ "soulskill_can_be_used_for_myself",   &battle_config.soulskill_can_be_used_for_myself   },
			{ "hermode_wp_check_range",             &battle_config.hermode_wp_check_range             },
			{ "hermode_wp_check",                   &battle_config.hermode_wp_check                   },
			{ "hermode_no_walking",                 &battle_config.hermode_no_walking                 },
			{ "hermode_gvg_only",                   &battle_config.hermode_gvg_only                   },
			{ "atcommand_go_significant_values",    &battle_config.atcommand_go_significant_values    },
			{ "redemptio_penalty_type",             &battle_config.redemptio_penalty_type             },
			{ "allow_weaponrearch_to_weaponrefine", &battle_config.allow_weaponrearch_to_weaponrefine },
			{ "boss_no_knockbacking",               &battle_config.boss_no_knockbacking               },
			{ "boss_no_element_change",             &battle_config.boss_no_element_change             },
			{ "scroll_produce_rate",                &battle_config.scroll_produce_rate                },
			{ "scroll_item_name_input",             &battle_config.scroll_item_name_input             },
			{ "pet_leave",                          &battle_config.pet_leave                          },
			{ "pk_short_attack_damage_rate",        &battle_config.pk_short_damage_rate               },
			{ "pk_long_attack_damage_rate",         &battle_config.pk_long_damage_rate                },
			{ "pk_magic_attack_damage_rate",        &battle_config.pk_magic_damage_rate               },
			{ "pk_misc_attack_damage_rate",         &battle_config.pk_misc_damage_rate                },
			{ "cooking_rate",                       &battle_config.cooking_rate                       },
			{ "making_rate",                        &battle_config.making_rate                        },
			{ "extended_abracadabra",               &battle_config.extended_abracadabra               },
			{ "no_pk_level",                        &battle_config.no_pk_level                        },
			{ "allow_cloneskill_at_autospell",      &battle_config.allow_cloneskill_at_autospell      },
			{ "pk_noshift",                         &battle_config.pk_noshift                         },
			{ "pk_penalty_time",                    &battle_config.pk_penalty_time                    },
			{ "dropitem_itemrate_fix",              &battle_config.dropitem_itemrate_fix              },
			{ "gm_nomanner_lv",                     &battle_config.gm_nomanner_lv                     },
			{ "clif_fixpos_type",                   &battle_config.clif_fixpos_type                   },
			{ "romailuse",                          &battle_config.romail                             },
			{ "pc_die_script",                      &battle_config.pc_die_script                      },
			{ "pc_kill_script",                     &battle_config.pc_kill_script                     },
			{ "pc_movemap_script",                  &battle_config.pc_movemap_script                  },
			{ "pc_login_script",                    &battle_config.pc_login_script                    },
			{ "pc_logout_script",                   &battle_config.pc_logout_script                   },
			{ "save_pckiller_type",                 &battle_config.save_pckiller_type                 },
			{ "def_ratio_atk_to_shieldchain",       &battle_config.def_ratio_atk_to_shieldchain       },
			{ "def_ratio_atk_to_carttermination",   &battle_config.def_ratio_atk_to_carttermination   },
			{ "player_gravitation_type",            &battle_config.player_gravitation_type            },
			{ "enemy_gravitation_type",             &battle_config.enemy_gravitation_type             },
			{ "mob_attack_fixwalkpos",              &battle_config.mob_attack_fixwalkpos              },
			{ "mob_ai_limiter",                     &battle_config.mob_ai_limiter                     },
			{ "mob_ai_cpu_usage",                   &battle_config.mob_ai_cpu_usage                   },
			{ "itemidentify",                       &battle_config.itemidentify                       },
			{ "casting_penalty_type",               &battle_config.casting_penalty_type               },
			{ "casting_penalty_weapon",             &battle_config.casting_penalty_weapon             },
			{ "casting_penalty_shield",             &battle_config.casting_penalty_shield             },
			{ "casting_penalty_armor",              &battle_config.casting_penalty_armor              },
			{ "casting_penalty_helm",               &battle_config.casting_penalty_helm               },
			{ "casting_penalty_robe",               &battle_config.casting_penalty_robe               },
			{ "casting_penalty_shoes",              &battle_config.casting_penalty_shoes              },
			{ "casting_penalty_acce",               &battle_config.casting_penalty_acce               },
			{ "casting_penalty_arrow",              &battle_config.casting_penalty_arrow              },
			{ "show_always_party_name",             &battle_config.show_always_party_name             },
			{ "check_player_name_global_msg",       &battle_config.check_player_name_global_msg       },
			{ "check_player_name_party_msg",        &battle_config.check_player_name_party_msg        },
			{ "check_player_name_guild_msg",        &battle_config.check_player_name_guild_msg        },
			{ "save_player_when_drop_item",         &battle_config.save_player_when_drop_item         },
			{ "save_player_when_storage_closed",    &battle_config.save_player_when_storage_closed    },
			{ "allow_homun_status_change",          &battle_config.allow_homun_status_change          },
			{ "save_homun_temporal_intimate",       &battle_config.save_homun_temporal_intimate       },
			{ "homun_intimate_rate",                &battle_config.homun_intimate_rate                },
			{ "homun_temporal_intimate_resilience", &battle_config.homun_temporal_intimate_resilience },
			{ "hvan_explosion_intimate",            &battle_config.hvan_explosion_intimate            },
			{ "homun_speed_is_same_as_pc",          &battle_config.homun_speed_is_same_as_pc          },
			{ "homun_skill_intimate_type",          &battle_config.homun_skill_intimate_type          },
			{ "master_get_homun_base_exp",          &battle_config.master_get_homun_base_exp          },
			{ "master_get_homun_job_exp",           &battle_config.master_get_homun_job_exp           },
			{ "extra_system_flag",                  &battle_config.extra_system_flag                  },
			{ "mob_take_over_sp",                   &battle_config.mob_take_over_sp                   },
			{ "party_join_limit",                   &battle_config.party_join_limit                   },
			{ "check_skillpos_range",               &battle_config.check_skillpos_range               },
			{ "pet_speed_is_same_as_pc",            &battle_config.pet_speed_is_same_as_pc            },
			{ "tax_rate",                           &battle_config.tax_rate                           },
			{ "steal_rate",                         &battle_config.steal_rate                         },
			{ "sw_def_type",                        &battle_config.sw_def_type                        },
			{ "calc_dist_flag",                     &battle_config.calc_dist_flag                     },
			{ "allow_sw_dist",                      &battle_config.allow_sw_dist                      },
			{ "personal_storage_sort",              &battle_config.personal_storage_sort              },
			{ "guild_storage_sort",                 &battle_config.guild_storage_sort                 },
			{ "allow_es_magic_all",                 &battle_config.allow_es_magic_all                 },
			{ "trap_is_invisible",                  &battle_config.trap_is_invisible                  },
			{ "gm_perfect_hide",                    &battle_config.gm_perfect_hide                    },
			{ "pcview_mob_clear_type",              &battle_config.pcview_mob_clear_type              },
			{ "party_item_share_type",              &battle_config.party_item_share_type              },
			{ "party_item_share_show",              &battle_config.party_item_share_show              },
			{ "pk_murderer_point",                  &battle_config.pk_murderer_point                  },
			{ "sg_miracle_rate",                    &battle_config.sg_miracle_rate                    },
			{ "baby_copy_skilltree",                &battle_config.baby_copy_skilltree                },
		};
		int max = sizeof(data)/sizeof(data[0]);

		if(line[0] == '/' && line[1] == '/')
			continue;
		i = sscanf(line,"%[^:]:%s",w1,w2);
		if(i != 2)
			continue;

		if( strcmpi(w1,"import") == 0 ) {
			battle_config_read(w2);
			continue;
		}
		for(i=0; i<max; i++) {
			if(strcmpi(w1,data[i].str) == 0) {
				*data[i].val = battle_config_switch(w2);
				break;
			}
		}
		if(i >= max)
			printf("unknown battle config : %s\a\n",w1);
	}
	fclose(fp);

	// フラグ調整
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
 * 初期化
 *------------------------------------------
 */
int do_init_battle(void)
{
	add_timer_func_list(battle_delay_damage_sub, "battle_delay_damage_sub");

	return 0;
}

/*==========================================
 * 終了
 *------------------------------------------
 */
int do_final_battle(void)
{
	// nothing to do
	return 0;
}
