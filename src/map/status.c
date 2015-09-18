// �X�e�[�^�X�v�Z�A��Ԉُ폈��

#include <time.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>

#include "db.h"
#include "malloc.h"
#include "nullpo.h"
#include "timer.h"
#include "utils.h"

#include "pc.h"
#include "map.h"
#include "pet.h"
#include "homun.h"
#include "mob.h"
#include "clif.h"
#include "skill.h"
#include "itemdb.h"
#include "battle.h"
#include "status.h"
#include "script.h"
#include "guild.h"
#include "unit.h"
#include "ranking.h"

static struct job_db {
	int max_weight_base;
	int hp_coefficient;
	int hp_coefficient2;
	int sp_coefficient;
	int hp_sigma[MAX_LEVEL];
	int bonus[3][MAX_LEVEL];
	int aspd_base[WT_MAX];
} job_db[MAX_PC_CLASS];

static int atkmods[MAX_SIZE_FIX][WT_MAX];	// ����ATK�T�C�Y�C��(size_fix.txt)

static struct refine_db {
	int safety_bonus;
	int over_bonus;
	int limit;
	int per[MAX_REFINE];
} refine_db[MAX_WEAPON_LEVEL+1];

int current_equip_item_index;//�X�e�[�^�X�v�Z�p
int current_equip_card_id;
static char race_name[11][5] = {{"���`"},{"�s��"},{"����"},{"�A��"},{"����"},{""},{"���L"},{"����"},{"�l��"},{"�V�g"},{"��"}};
struct status_change dummy_sc_data[MAX_STATUSCHANGE];
static struct scdata_db scdata_db[MAX_STATUSCHANGE];	// �X�e�[�^�X�ُ�f�[�^�x�[�X

static int StatusIconChangeTable[] = {
/* 0- */
	SI_PROVOKE,SI_ENDURE,SI_TWOHANDQUICKEN,SI_CONCENTRATE,SI_BLANK,SI_CLOAKING,SI_ENCPOISON,SI_POISONREACT,SI_QUAGMIRE,SI_ANGELUS,
/* 10- */
	SI_BLESSING,SI_SIGNUMCRUCIS,SI_INCREASEAGI,SI_DECREASEAGI,SI_SLOWPOISON,SI_IMPOSITIO,SI_SUFFRAGIUM,SI_ASPERSIO,SI_BENEDICTIO,SI_KYRIE,
/* 20- */
	SI_MAGNIFICAT,SI_GLORIA,SI_AETERNA,SI_ADRENALINE,SI_WEAPONPERFECTION,SI_OVERTHRUST,SI_MAXIMIZEPOWER,SI_RIDING,SI_FALCON,SI_TRICKDEAD,
/* 30- */
	SI_LOUD,SI_ENERGYCOAT,SI_BLANK,SI_BLANK,SI_HALLUCINATION,SI_WEIGHT50,SI_WEIGHT90,SI_SPEEDPOTION0,SI_SPEEDPOTION1,SI_SPEEDPOTION2,
/* 40- */
	SI_SPEEDPOTION3,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 50- */
	SI_STRIPWEAPON,SI_STRIPSHIELD,SI_STRIPARMOR,SI_STRIPHELM,SI_CP_WEAPON,SI_CP_SHIELD,SI_CP_ARMOR,SI_CP_HELM,SI_AUTOGUARD,SI_REFLECTSHIELD,
/* 60- */
	SI_DEVOTION,SI_PROVIDENCE,SI_DEFENDER,SI_BLANK,SI_BLANK,SI_AUTOSPELL,SI_BLANK,SI_BLANK,SI_SPEARSQUICKEN,SI_BLANK,
/* 70- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 80- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_EXPLOSIONSPIRITS,SI_STEELBODY,SI_BLANK,SI_COMBO,
/* 90- */
	SI_FLAMELAUNCHER,SI_FROSTWEAPON,SI_LIGHTNINGLOADER,SI_SEISMICWEAPON,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 100- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_AURABLADE,SI_PARRYING,SI_CONCENTRATION,SI_TENSIONRELAX,SI_BERSERK,SI_BLANK,SI_BLANK,
/* 110- */
	SI_ASSUMPTIO,SI_BLANK,SI_BLANK,SI_MAGICPOWER,SI_EDP,SI_TRUESIGHT,SI_WINDWALK,SI_MELTDOWN,SI_CARTBOOST,SI_CHASEWALK,
/* 120- */
	SI_REJECTSWORD,SI_MARIONETTE,SI_MARIONETTE2,SI_BLANK,SI_HEADCRUSH,SI_JOINTBEAT,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 130- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_HEADCRUSH,SI_BLANK,SI_BLANK,
/* 140- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 150- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 160- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 170- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 180- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 190- */
	SI_BLANK,SI_MAGNUM,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 200- */
	SI_BLANK,SI_PRESERVE,SI_OVERTHRUSTMAX,SI_CHASEWALK_STR,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 210- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 220- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_MEAL_INCSTR,SI_MEAL_INCAGI,SI_MEAL_INCVIT,SI_MEAL_INCINT,SI_MEAL_INCDEX,SI_MEAL_INCLUK,
/* 230- */
	SI_RUN,SI_SPURT,SI_BLANK,SI_DODGE,SI_BLANK,SI_BLANK,SI_BLANK,SI_WARM,SI_BLANK,SI_BLANK,
/* 240- */
	SI_SUN_COMFORT,SI_MOON_COMFORT,SI_STAR_COMFORT,SI_BLANK,SI_SOULLINK,SI_SOULLINK,SI_SOULLINK,SI_SOULLINK,SI_SOULLINK,SI_SOULLINK,
/* 250- */
	SI_SOULLINK,SI_SOULLINK,SI_SOULLINK,SI_SOULLINK,SI_SOULLINK,SI_SOULLINK,SI_SOULLINK,SI_SOULLINK,SI_SOULLINK,SI_SOULLINK,
/* 260- */
	SI_SOULLINK,SI_SOULLINK,SI_SOULLINK,SI_SOULLINK,SI_KAIZEL,SI_KAAHI,SI_KAUPE,SI_KAITE,SI_SMA,SI_BLANK,
/* 270- */
	SI_BLANK,SI_BLANK,SI_ONEHAND,SI_READYSTORM,SI_READYDOWN,SI_READYTURN,SI_READYCOUNTER,SI_BLANK,SI_AUTOBERSERK,SI_DEVIL,
/* 280- */
	SI_DOUBLECASTING,SI_ELEMENTFIELD,SI_DARKELEMENT,SI_ATTENELEMENT,SI_MIRACLE,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BABY,
/* 290- */
	SI_BLANK,SI_SHRINK,SI_CLOSECONFINE,SI_SIGHTBLASTER,SI_BLANK,SI_MEAL_INCHIT,SI_MEAL_INCFLEE,SI_BLANK,SI_MEAL_INCCRITICAL,SI_BLANK,
/* 300- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 310- */
	SI_BLANK,SI_BLANK,SI_UNDEAD,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 320- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,
/* 330- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_TIGEREYE,SI_BLANK,SI_BLANK,
/* 340- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_TAROTCARD,SI_HOLDWEB,SI_BLANK,SI_BLANK,SI_BLANK,SI_MADNESSCANCEL,SI_ADJUSTMENT,
/* 350- */
	SI_INCREASING,SI_BLANK,SI_GATLINGFEVER,SI_BLANK,SI_BLANK,SI_UTSUSEMI,SI_BUNSINJYUTSU,SI_BLANK,SI_NEN,SI_BLANK,
/* 360- */
	SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_BLANK,SI_ADRENALINE2,SI_BLANK,SI_BLANK,SI_COMBATHAN,SI_LIFEINSURANCE,
/* 370- */
	SI_BLANK,SI_BLANK,SI_MEAL_INCSTR2,SI_MEAL_INCAGI2,SI_MEAL_INCVIT2,SI_MEAL_INCDEX2,SI_MEAL_INCINT2,SI_MEAL_INCLUK2,SI_BLANK,SI_BLANK,
};

/*==========================================
 * �ߏ萸�B�{�[�i�X
 *------------------------------------------
 */
int status_get_overrefine_bonus(int lv)
{
	if(lv >= 0 && lv <= MAX_WEAPON_LEVEL)
		return refine_db[lv].over_bonus;
	return 0;
}

/*==========================================
 * ���B������
 *------------------------------------------
 */
int status_percentrefinery(struct map_session_data *sd,struct item *item)
{
	int percent;

	nullpo_retr(0, item);

	if(item->refine < 0 || item->refine >= MAX_REFINE)	// �l���G���[�������͊��ɍő�l�Ȃ�0%
		return 0;

	percent = refine_db[itemdb_wlv(item->nameid)].per[(int)item->refine];

	percent += pc_checkskill(sd,BS_WEAPONRESEARCH);	// ���팤���X�L������

	// �m���̗L���͈̓`�F�b�N
	if( percent > 100 ){
		percent = 100;
	}
	if( percent < 0 ){
		percent = 0;
	}

	return percent;
}

/*==========================================
 * ���B������ ������
 *------------------------------------------
 */
int status_percentrefinery_weaponrefine(struct map_session_data *sd,struct item *item)
{
	int percent;
	int joblv;

	nullpo_retr(0, sd);
	nullpo_retr(0, item);

	if(item->refine < 0 || item->refine >= MAX_REFINE)	// �l���G���[�������͊��ɍő�l�Ȃ�0%
		return 0;

	joblv = sd->status.job_level > 70? 70 : sd->status.job_level;
	percent = refine_db[itemdb_wlv(item->nameid)].per[(int)item->refine]*100 + (joblv - 50)*50;

	if(battle_config.allow_weaponrearch_to_weaponrefine)
		percent += pc_checkskill(sd,BS_WEAPONRESEARCH)*100;	// ���팤���X�L������

	// �m���̗L���͈̓`�F�b�N
	if( percent > 10000 ){
		percent = 10000;
	}
	if( percent < 0 ){
		percent = 0;
	}

	return percent;
}

/*==========================================
 * �Z�[�u�\�ȃX�e�[�^�X�ُ킩�ǂ���
 *------------------------------------------
 */
int status_can_save(int type)
{
	if(type >= 0 && type < MAX_STATUSCHANGE) {
		if(scdata_db[type].save > 0)
			return 1;
	}
	return 0;
}

/*==========================================
 * diable�ȃX�e�[�^�X�ُ킩�ǂ���
 *------------------------------------------
 */
int status_is_disable(int type,int mask)
{
	if(type >= 0 && type < MAX_STATUSCHANGE) {
		if(scdata_db[type].disable & mask)
			return 1;
	}
	return 0;
}

/*==========================================
 * �p�����[�^�v�Z
 * first==0�̎��A�v�Z�Ώۂ̃p�����[�^���Ăяo���O����
 * �ω������ꍇ������send���邪�A
 * �\���I�ɕω��������p�����[�^�͎��O��send����悤��
 *------------------------------------------
 */
int status_calc_pc(struct map_session_data* sd,int first)
{
	// ���ӁF�����ł͕ϐ��̐錾�݂̂ɂƂǂ߁A��������L_RECALC�̌�ɂ�邱�ƁB
	int b_speed,b_max_hp,b_max_sp,b_hp,b_sp,b_weight,b_max_weight,b_paramb[6],b_parame[6],b_hit,b_flee;
	int b_aspd,b_watk,b_def,b_watk2,b_def2,b_flee2,b_critical,b_attackrange,b_matk1,b_matk2,b_mdef,b_mdef2,b_class;
	int b_base_atk;
	short b_tigereye;
	struct skill b_skill[MAX_SKILL];
	int i,blv,calc_val,index;
	int skill,aspd_rate,wele,wele_,def_ele,refinedef;
	int pele,pdef_ele;
	int str,dstr,dex;
	struct pc_base_job s_class;
	int    calclimit = 2; // �����use script���݂Ŏ��s

	nullpo_retr(0, sd);

	if(sd->stop_status_calc_pc)
	{
		sd->call_status_calc_pc_while_stopping++;
		return 0;
	}

	sd->call_status_calc_pc_while_stopping = 0;

	// status_calc_pc �̏�������status_calc_pc���Ăяo�����ƍŏ��Ɍv�Z���Ă���
	// �l�������\��������B�܂��A���̊֐����Ă΂��Ƃ������Ƃ́A�L�����̏�Ԃ�
	// �ω����Ă��邱�Ƃ��Î����Ă���̂ŁA�v�Z���ʂ��̂ĂčČv�Z���Ȃ���΂����Ȃ��B
	// �֐��̏I�����_�ŌĂяo��������΁AL_RECALC�ɔ�΂��悤�ɂ���B
	if( sd->status_calc_pc_process++ ) return 0;

	// �ȑO�̏�Ԃ̕ۑ�
	b_speed = sd->speed;
	b_max_hp = sd->status.max_hp;
	b_max_sp = sd->status.max_sp;
	b_hp = sd->status.hp;
	b_sp = sd->status.sp;
	b_weight = sd->weight;
	b_max_weight = sd->max_weight;
	memcpy(b_paramb,&sd->paramb,sizeof(b_paramb));
	memcpy(b_parame,&sd->paramc,sizeof(b_parame));
	memcpy(b_skill,&sd->status.skill,sizeof(b_skill));
	b_hit = sd->hit;
	b_flee = sd->flee;
	b_aspd = sd->aspd;
	b_watk = sd->watk;
	b_def = sd->def;
	b_watk2 = sd->watk2;
	b_def2 = sd->def2;
	b_flee2 = sd->flee2;
	b_critical = sd->critical;
	b_attackrange = sd->attackrange;
	b_matk1 = sd->matk1;
	b_matk2 = sd->matk2;
	b_mdef = sd->mdef;
	b_mdef2 = sd->mdef2;
	b_class = sd->view_class;
	b_base_atk = sd->base_atk;
	b_tigereye = sd->infinite_tigereye;

L_RECALC:
	// �{���̌v�Z�J�n(���̃p�����[�^���X�V���Ȃ��̂́A�v�Z���Ɍv�Z�������Ă΂ꂽ�Ƃ���
	// ���f�����V���ɑ��M���邽��)�B

	pele=0;
	pdef_ele=0;
	refinedef=0;
	sd->view_class = sd->status.class_;
	if(sd->view_class == PC_CLASS_GS || sd->view_class==PC_CLASS_NJ)
		sd->view_class = sd->view_class-4;

	//�]����{�q�̏ꍇ�̌��̐E�Ƃ��Z�o����
	s_class = pc_calc_base_job(sd->status.class_);

	sd->race = RCT_HUMAN;
	sd->ranker_weapon_bonus  = 0;
	sd->ranker_weapon_bonus_ = 0;
	sd->infinite_tigereye    = 0;

	pc_calc_skilltree(sd);	// �X�L���c���[�̌v�Z

	sd->max_weight = job_db[s_class.job].max_weight_base+sd->status.str*300;

	if(battle_config.baby_weight_rate !=100 && pc_isbaby(sd))
		sd->max_weight = sd->max_weight*battle_config.baby_weight_rate/100;


	//�y�R�R�掞������悤�ړ�
	if( (skill=pc_checkskill(sd,MC_INCCARRY))>0)	// �����ʑ���
		sd->max_weight += skill*2000;

	if( (skill=pc_checkskill(sd,SG_KNOWLEDGE))>0)// ���z�ƌ��Ɛ��̒m��
	{
	 	if(battle_config.check_knowlege_map)//�ꏊ�`�F�b�N���s�Ȃ�
	 	{
			if(sd->bl.m == sd->feel_index[0] || sd->bl.m == sd->feel_index[1] || sd->bl.m == sd->feel_index[2])
				sd->max_weight += sd->max_weight*skill/10;
		}else
			sd->max_weight += sd->max_weight*skill/10;
	}

	if(first&1) {
		sd->weight=0;
		for(i=0;i<MAX_INVENTORY;i++){
			if(sd->status.inventory[i].nameid==0 || sd->inventory_data[i] == NULL)
				continue;
			sd->weight += sd->inventory_data[i]->weight*sd->status.inventory[i].amount;
		}
		sd->cart_max_weight=battle_config.max_cart_weight;
		sd->cart_weight=0;
		sd->cart_max_num=MAX_CART;
		sd->cart_num=0;
		for(i=0;i<MAX_CART;i++){
			if(sd->status.cart[i].nameid==0)
				continue;
			sd->cart_weight+=itemdb_weight(sd->status.cart[i].nameid)*sd->status.cart[i].amount;
			sd->cart_num++;
		}
	}

	memset(sd->paramb,0,sizeof(sd->paramb));
	memset(sd->parame,0,sizeof(sd->parame));
	sd->hit = 0;
	sd->flee = 0;
	sd->flee2 = 0;
	sd->critical = 0;
	sd->aspd = 0;
	sd->watk = 0;
	sd->def = 0;
	sd->mdef = 0;
	sd->watk2 = 0;
	sd->def2 = 0;
	sd->mdef2 = 0;
	sd->status.max_hp = 0;
	sd->status.max_sp = 0;
	sd->attackrange = 0;
	sd->attackrange_ = 0;
	sd->atk_ele = 0;
	sd->def_ele = 0;
	sd->star =0;
	sd->overrefine =0;
	sd->matk1 =0;
	sd->matk2 =0;
	sd->speed = DEFAULT_WALK_SPEED;
	sd->hprate=battle_config.hp_rate;
	sd->sprate=battle_config.sp_rate;
	sd->castrate=100;
	sd->dsprate=100;
	sd->base_atk=0;
	sd->arrow_atk=0;
	sd->arrow_ele=0;
	sd->arrow_hit=0;
	sd->arrow_range=0;
	sd->nhealhp=sd->nhealsp=sd->nshealhp=sd->nshealsp=sd->nsshealhp=sd->nsshealsp=0;
	memset(sd->addele,0,sizeof(sd->addele));
	memset(sd->addrace,0,sizeof(sd->addrace));
	memset(sd->addenemy,0,sizeof(sd->addenemy));
	memset(sd->addsize,0,sizeof(sd->addsize));
	memset(sd->addele_,0,sizeof(sd->addele_));
	memset(sd->addrace_,0,sizeof(sd->addrace_));
	memset(sd->addenemy_,0,sizeof(sd->addenemy_));
	memset(sd->addsize_,0,sizeof(sd->addsize_));
	memset(sd->subele,0,sizeof(sd->subele));
	memset(sd->subrace,0,sizeof(sd->subrace));
	memset(sd->subenemy,0,sizeof(sd->subenemy));
	memset(sd->addeff,0,sizeof(sd->addeff));
	memset(sd->addeff2,0,sizeof(sd->addeff2));
	memset(sd->reseff,0,sizeof(sd->reseff));
	memset(sd->addeff_range_flag,0,sizeof(sd->addeff));
	memset(&sd->special_state,0,sizeof(sd->special_state));
	memset(sd->weapon_coma_ele,0,sizeof(sd->weapon_coma_ele));
	memset(sd->weapon_coma_race,0,sizeof(sd->weapon_coma_race));
	memset(sd->weapon_coma_ele2,0,sizeof(sd->weapon_coma_ele2));
	memset(sd->weapon_coma_race2,0,sizeof(sd->weapon_coma_race2));
	memset(sd->weapon_atk,0,sizeof(sd->weapon_atk));
	memset(sd->weapon_atk_rate,0,sizeof(sd->weapon_atk_rate));
	memset(sd->auto_status_calc_pc,0,sizeof(sd->auto_status_calc_pc));
	memset(sd->eternal_status_change,0,sizeof(sd->eternal_status_change));

	sd->watk_ = 0;			//�񓁗��p(��)
	sd->watk_2 = 0;
	sd->atk_ele_ = 0;
	sd->star_ = 0;
	sd->overrefine_ = 0;

	sd->aspd_rate = 100;
	sd->speed_rate = 100;
	sd->hprecov_rate = 100;
	sd->sprecov_rate = 100;
	sd->critical_def = 0;
	sd->double_rate = 0;
	sd->near_attack_def_rate = sd->long_attack_def_rate = 0;
	sd->atk_rate = sd->matk_rate = 100;
	sd->ignore_def_ele = sd->ignore_def_race = sd->ignore_def_enemy = 0;
	sd->ignore_def_ele_ = sd->ignore_def_race_ = sd->ignore_def_enemy_ = 0;
	sd->ignore_mdef_ele = sd->ignore_mdef_race = sd->ignore_mdef_enemy = 0;
	sd->arrow_cri = 0;
	sd->magic_def_rate = sd->misc_def_rate = 0;
	memset(sd->arrow_addele,0,sizeof(sd->arrow_addele));
	memset(sd->arrow_addrace,0,sizeof(sd->arrow_addrace));
	memset(sd->arrow_addenemy,0,sizeof(sd->arrow_addenemy));
	memset(sd->arrow_addsize,0,sizeof(sd->arrow_addsize));
	memset(sd->arrow_addeff,0,sizeof(sd->arrow_addeff));
	memset(sd->arrow_addeff2,0,sizeof(sd->arrow_addeff2));
	memset(sd->magic_addele,0,sizeof(sd->magic_addele));
	memset(sd->magic_addrace,0,sizeof(sd->magic_addrace));
	memset(sd->magic_addenemy,0,sizeof(sd->magic_addenemy));
	memset(sd->magic_subrace,0,sizeof(sd->magic_subrace));
	sd->perfect_hit = 0;
	sd->critical_rate = sd->hit_rate = sd->flee_rate = sd->flee2_rate = 100;
	sd->def_rate = sd->def2_rate = sd->mdef_rate = sd->mdef2_rate = 100;
	sd->def_ratio_atk_ele = sd->def_ratio_atk_race = sd->def_ratio_atk_enemy = 0;
	sd->def_ratio_atk_ele_ = sd->def_ratio_atk_race_ = sd->def_ratio_atk_enemy_ = 0;
	sd->get_zeny_num = sd->get_zeny_num2 = 0;
	sd->add_damage_class_count = sd->add_damage_class_count_ = sd->add_magic_damage_class_count = 0;
	sd->add_def_class_count = sd->add_mdef_class_count = 0;
	sd->monster_drop_item_count = 0;
	memset(sd->add_damage_classrate,0,sizeof(sd->add_damage_classrate));
	memset(sd->add_damage_classrate_,0,sizeof(sd->add_damage_classrate_));
	memset(sd->add_magic_damage_classrate,0,sizeof(sd->add_magic_damage_classrate));
	memset(sd->add_def_classrate,0,sizeof(sd->add_def_classrate));
	memset(sd->add_mdef_classrate,0,sizeof(sd->add_mdef_classrate));
	memset(sd->monster_drop_race,0,sizeof(sd->monster_drop_race));
	memset(sd->monster_drop_itemrate,0,sizeof(sd->monster_drop_itemrate));
	sd->sp_gain_value = 0;
	sd->hp_gain_value = 0;
	sd->speed_add_rate = sd->aspd_add_rate = 100;
	sd->double_add_rate = sd->perfect_hit_add = sd->get_zeny_add_num = sd->get_zeny_add_num2 = 0;
	sd->splash_range = sd->splash_add_range = 0;
	sd->autospell_id = sd->autospell_lv = sd->autospell_rate = 0;
	sd->autospell_flag = 0;
	sd->hp_drain_rate = sd->hp_drain_per = sd->sp_drain_rate = sd->sp_drain_per = 0;
	sd->hp_drain_rate_ = sd->hp_drain_per_ = sd->sp_drain_rate_ = sd->sp_drain_per_ = 0;
	sd->hp_drain_value = sd->hp_drain_value_ = sd->sp_drain_value = sd->sp_drain_value_ = 0;
	sd->short_weapon_damage_return = sd->long_weapon_damage_return = sd->magic_damage_return = 0;
	sd->break_weapon_rate = sd->break_armor_rate = 0;
	sd->add_steal_rate = 0;
	sd->unbreakable_equip = 0;
	//�V�J�[�h�p������
	//sd->revautospell_id = sd->revautospell_lv=sd->revautospell_rate = sd->revautospell_flag = 0;
	sd->critical_damage=0;
	sd->hp_recov_stop = sd->sp_recov_stop = 0;
	memset(sd->critical_race,0,sizeof(sd->critical_race));
	memset(sd->critical_race_rate,0,sizeof(sd->critical_race_rate));
	memset(sd->subsize,0,sizeof(sd->subsize));
	memset(sd->magic_subsize,0,sizeof(sd->magic_subsize));
	memset(sd->exp_rate,0,sizeof(sd->exp_rate));
	memset(sd->job_rate,0,sizeof(sd->job_rate));
	memset(sd->hp_drain_rate_race,0,sizeof(sd->hp_drain_rate_race));
	memset(sd->sp_drain_rate_race,0,sizeof(sd->sp_drain_rate_race));
	memset(sd->hp_drain_value_race,0,sizeof(sd->hp_drain_value_race));
	memset(sd->sp_drain_value_race,0,sizeof(sd->sp_drain_value_race));
	memset(sd->addreveff,0,sizeof(sd->addreveff));
	sd->addreveff_flag = 0;
	memset(sd->addgroup,0,sizeof(sd->addgroup));
	memset(sd->addgroup_,0,sizeof(sd->addgroup_));
	memset(sd->arrow_addgroup,0,sizeof(sd->arrow_addgroup));
	memset(sd->subgroup,0,sizeof(sd->subgroup));
	sd->hp_penalty_time 	= 0;
	sd->hp_penalty_value 	= 0;
 	sd->sp_penalty_time 	= 0;
	sd->sp_penalty_value 	= 0;
	memset(sd->hp_penalty_unrig_value,0,sizeof(sd->hp_penalty_unrig_value));
	memset(sd->sp_penalty_unrig_value,0,sizeof(sd->sp_penalty_unrig_value));
	memset(sd->hp_rate_penalty_unrig,0,sizeof(sd->hp_rate_penalty_unrig));
	memset(sd->sp_rate_penalty_unrig,0,sizeof(sd->sp_rate_penalty_unrig));
	sd->mob_class_change_rate = 0;
	memset(&sd->skill_dmgup,0,sizeof(sd->skill_dmgup));
	memset(&sd->skill_blow,0,sizeof(sd->skill_blow));
	memset(&sd->autospell,0,sizeof(sd->autospell));
	memset(&sd->itemheal_rate,0,sizeof(sd->itemheal_rate));
	memset(&sd->autoraise,0,sizeof(sd->autoraise));
	sd->hp_vanish_rate = 0;
	sd->hp_vanish_per  = 0;
	sd->sp_vanish_rate = 0;
	sd->sp_vanish_per  = 0;
	sd->bonus_damage = 0;
	sd->curse_by_muramasa = 0;
	memset(sd->loss_equip_rate_when_die,0,sizeof(sd->loss_equip_rate_when_die));
	memset(sd->loss_equip_rate_when_attack,0,sizeof(sd->loss_equip_rate_when_attack));
	memset(sd->loss_equip_rate_when_hit,0,sizeof(sd->loss_equip_rate_when_hit));
	memset(sd->break_myequip_rate_when_attack,0,sizeof(sd->break_myequip_rate_when_attack));
	memset(sd->break_myequip_rate_when_hit,0,sizeof(sd->break_myequip_rate_when_hit));
	sd->loss_equip_flag = 0;
	sd->short_weapon_damege_rate = sd->long_weapon_damege_rate = 0;
	sd->add_attackrange = 0;
	sd->add_attackrange_rate = 100;
	sd->special_state.item_no_use = 0;
	sd->skill_delay_rate = 0;

	for(i=0;i<10;i++) {
		index = sd->equip_index[i];
		current_equip_item_index = i;//(���ʃ`�F�b�N�p)
		if(index < 0)
			continue;
		if(i == 9 && sd->equip_index[8] == index)
			continue;
		if(i == 5 && sd->equip_index[4] == index)
			continue;
		if(i == 6 && (sd->equip_index[5] == index || sd->equip_index[4] == index))
			continue;

		if(sd->inventory_data[index]) {
			if(sd->inventory_data[index]->type == 4) {
				if( !itemdb_isspecial(sd->status.inventory[index].card[0]) ) {
					int j;
					for(j=0;j<sd->inventory_data[index]->slot;j++){	// �J�[�h
						int c=sd->status.inventory[index].card[j];
						current_equip_card_id = c;		//�I�[�g�X�y��(�d���`�F�b�N�p)
						if(c>0){
							if(i == 8 && sd->status.inventory[index].equip == 0x20)
								sd->state.lr_flag = 1;
							if(calclimit == 2)
								run_script(itemdb_usescript(c),0,sd->bl.id,0);
							run_script(itemdb_equipscript(c),0,sd->bl.id,0);
							sd->state.lr_flag = 0;
						}
					}
				}
			}
			else if(sd->inventory_data[index]->type == 5) { // �h��
				if( !itemdb_isspecial(sd->status.inventory[index].card[0]) ) {
					int j;
					for(j=0;j<sd->inventory_data[index]->slot;j++){	// �J�[�h
						int c=sd->status.inventory[index].card[j];
						current_equip_card_id = c;		//�I�[�g�X�y��(�d���`�F�b�N�p)
						if(c>0) {
							if(calclimit == 2)
								run_script(itemdb_usescript(c),0,sd->bl.id,0);
							run_script(itemdb_equipscript(c),0,sd->bl.id,0);
						}
					}
				}
			}
		}
	}

	wele = sd->atk_ele;
	wele_ = sd->atk_ele_;
	def_ele = sd->def_ele;
	if(battle_config.pet_status_support) {
		if(sd->status.pet_id > 0 && sd->petDB && sd->pet.intimate > 0)
			run_script(sd->petDB->script,0,sd->bl.id,0);
		pele = sd->atk_ele;
		pdef_ele = sd->def_ele;
		sd->atk_ele = sd->def_ele = 0;
	}
	memcpy(sd->paramcard,sd->parame,sizeof(sd->paramcard));

	// �����i�ɂ��X�e�[�^�X�ω��͂����Ŏ��s
	for(i=0;i<10;i++) {
		index = sd->equip_index[i];
		current_equip_item_index = i;//index;	//���ʃ`�F�b�N�p
		current_equip_card_id = index;		//�I�[�g�X�y��(�d���`�F�b�N�p)
		if(index < 0)
			continue;
		if(i == 9 && sd->equip_index[8] == index)
			continue;
		if(i == 5 && sd->equip_index[4] == index)
			continue;
		if(i == 6 && (sd->equip_index[5] == index || sd->equip_index[4] == index))
			continue;
		if(sd->inventory_data[index]) {
			sd->def += sd->inventory_data[index]->def;
			if(sd->inventory_data[index]->type == 4) {
				int r,wlv = sd->inventory_data[index]->wlv;
				if(i == 8 && sd->status.inventory[index].equip == 0x20) {
					//�񓁗��p�f�[�^����
					sd->watk_ += sd->inventory_data[index]->atk;
					sd->watk_2 = (r=sd->status.inventory[index].refine)*	// ���B�U����
						refine_db[wlv].safety_bonus;
					if( (r-=refine_db[wlv].limit)>0 )	// �ߏ萸�B�{�[�i�X
						sd->overrefine_ = r*refine_db[wlv].over_bonus;

					if(sd->status.inventory[index].card[0]==0x00ff){	// ��������
						sd->star_ = (sd->status.inventory[index].card[1]>>8);	// ���̂�����
						if(sd->star_ == 15) sd->star_ = 40;
						wele_= (sd->status.inventory[index].card[1]&0x0f);	// �� ��
						//�����L���O�{�[�i�X
						if(ranking_get_id2rank( *((unsigned long *)(&sd->status.inventory[index].card[2])) ,RK_BLACKSMITH))
							sd->ranker_weapon_bonus_ = 10;
					}
					sd->attackrange_ += sd->inventory_data[index]->range;
					sd->state.lr_flag = 1;
					if(calclimit == 2)
						run_script(sd->inventory_data[index]->use_script,0,sd->bl.id,0);
					run_script(sd->inventory_data[index]->equip_script,0,sd->bl.id,0);
					sd->state.lr_flag = 0;
				}
				else {	//�񓁗�����ȊO
					sd->watk += sd->inventory_data[index]->atk;
					sd->watk2 += (r=sd->status.inventory[index].refine)*	// ���B�U����
						refine_db[wlv].safety_bonus;
					if( (r-=refine_db[wlv].limit)>0 )	// �ߏ萸�B�{�[�i�X
						sd->overrefine += r*refine_db[wlv].over_bonus;

					if(sd->status.inventory[index].card[0]==0x00ff){	// ��������
						sd->star += (sd->status.inventory[index].card[1]>>8);	// ���̂�����
						if(sd->star == 15) sd->star = 40;
						wele = (sd->status.inventory[index].card[1]&0x0f);	// �� ��
						//�����L���O�{�[�i�X
						if(ranking_get_id2rank(*((unsigned long *)(&sd->status.inventory[index].card[2])),RK_BLACKSMITH))
							sd->ranker_weapon_bonus = 10;
					}
					sd->attackrange += sd->inventory_data[index]->range;
					if(calclimit == 2)
						run_script(sd->inventory_data[index]->use_script,0,sd->bl.id,0);
					run_script(sd->inventory_data[index]->equip_script,0,sd->bl.id,0);
				}
			}
			else if(sd->inventory_data[index]->type == 5) {
				sd->watk += sd->inventory_data[index]->atk;
				refinedef += sd->status.inventory[index].refine*refine_db[0].safety_bonus;
				if(calclimit == 2)
					run_script(sd->inventory_data[index]->use_script,0,sd->bl.id,0);
				run_script(sd->inventory_data[index]->equip_script,0,sd->bl.id,0);
			}
		}
	}

	if(sd->equip_index[10] >= 0){ // ��
		index = sd->equip_index[10];
		if(sd->inventory_data[index]){		//�܂������������Ă��Ȃ�
			sd->state.lr_flag = 2;
			if(calclimit == 2)
				run_script(sd->inventory_data[index]->use_script,0,sd->bl.id,0);
			run_script(sd->inventory_data[index]->equip_script,0,sd->bl.id,0);
			sd->state.lr_flag = 0;
			sd->arrow_atk += sd->inventory_data[index]->atk;
		}
	}

	sd->def += (refinedef+50)/100;

	if(sd->attackrange < 1) sd->attackrange = 1;
	if(sd->attackrange_ < 1) sd->attackrange_ = 1;
	if(sd->attackrange < sd->attackrange_)
		sd->attackrange = sd->attackrange_;
	if(sd->status.weapon == WT_BOW)
		sd->attackrange += sd->arrow_range;
	if(wele > 0)
		sd->atk_ele = wele;
	if(wele_ > 0)
		sd->atk_ele_ = wele_;
	if(def_ele > 0)
		sd->def_ele = def_ele;
	if(battle_config.pet_status_support) {
		if(pele > 0 && !sd->atk_ele)
			sd->atk_ele = pele;
		if(pdef_ele > 0 && !sd->def_ele)
			sd->def_ele = pdef_ele;
	}
	sd->double_rate += sd->double_add_rate;
	sd->perfect_hit += sd->perfect_hit_add;
	sd->get_zeny_num = (sd->get_zeny_num + sd->get_zeny_add_num > 100) ? 100 : (sd->get_zeny_num + sd->get_zeny_add_num);
	sd->get_zeny_num2 = (sd->get_zeny_num2 + sd->get_zeny_add_num2 > 100) ? 100 : (sd->get_zeny_num2 + sd->get_zeny_add_num2);
	sd->splash_range += sd->splash_add_range;
	if(sd->speed_add_rate != 100)
		sd->speed_rate += sd->speed_add_rate - 100;
	if(sd->aspd_add_rate != 100)
		sd->aspd_rate += sd->aspd_add_rate - 100;

	// ����ATK�T�C�Y�␳
	for(i=0; i<MAX_SIZE_FIX; i++) {
		sd->atkmods[i]  = atkmods[i][sd->weapontype1];	// �E��
		sd->atkmods_[i] = atkmods[i][sd->weapontype2];	// ����
	}

	// job�{�[�i�X��
	for(i=0;i<sd->status.job_level && i<MAX_LEVEL;i++){
		if(job_db[s_class.job].bonus[s_class.upper][i])
			sd->paramb[job_db[s_class.job].bonus[s_class.upper][i]-1]++;
	}

	if( (skill=pc_checkskill(sd,AC_OWL))>0 )	// �ӂ��낤�̖�
		sd->paramb[4] += skill;

	if( pc_checkskill(sd,BS_HILTBINDING) ) {	// �q���g�o�C���f�B���O
		sd->paramb[0] += 1;
		sd->watk += 4;
	}

	if( (skill=pc_checkskill(sd,SA_DRAGONOLOGY))>0 ){// �h���S�m���W�[
		sd->paramb[3] += (skill+1)>>1;
	}

	//�}�[�_���[�{�[�i�X
	if(battle_config.pk_murderer_point > 0) {
		int point = ranking_get_point(sd,RK_PK);
		if(point >= battle_config.pk_murderer_point * 4)
		{
			sd->paramb[0]+= 5;
			sd->paramb[1]+= 5;
			sd->paramb[2]+= 5;
			sd->paramb[3]+= 5;
			sd->paramb[4]+= 5;
			sd->paramb[5]+= 5;

			sd->atk_rate += 10;
			sd->matk_rate += 10;
		}else if(point >= battle_config.pk_murderer_point)
		{
			sd->paramb[0]+= 3;
			sd->paramb[1]+= 3;
			sd->paramb[2]+= 3;
			sd->paramb[3]+= 3;
			sd->paramb[4]+= 3;
			sd->paramb[5]+= 3;

			sd->atk_rate += 10;
			sd->matk_rate += 10;
		}
	}
	//1�x������łȂ�Job70�X�p�m�r��+10
	if(s_class.job == 23 && sd->status.die_counter == 0 && sd->status.job_level >= 70){
		sd->paramb[0]+= 10;
		sd->paramb[1]+= 10;
		sd->paramb[2]+= 10;
		sd->paramb[3]+= 10;
		sd->paramb[4]+= 10;
		sd->paramb[5]+= 10;
	}

	//�M���h�X�L��
	if(battle_config.guild_hunting_skill_available)
	{
		struct guild *g = guild_search(sd->status.guild_id);	//�M���h�擾
		struct map_session_data *gmsd = NULL;

		if(g)
			gmsd = guild_get_guildmaster_sd(g);

		//�M���h�L && �}�X�^�[�ڑ� && ����!=�}�X�^�[ && �����}�b�v
		if(g && gmsd && (battle_config.allow_me_guild_skill == 1 || gmsd != sd) && sd->bl.m == gmsd->bl.m)
		{
			int dx,dy,range;
			//����������s��
			if(battle_config.guild_skill_check_range){
				dx = abs(sd->bl.x - gmsd->bl.x);
				dy = abs(sd->bl.y - gmsd->bl.y);
				if(battle_config.guild_skill_effective_range > 0)//���ꋗ���Ōv�Z
				{
					range = battle_config.guild_skill_effective_range;
					if(dx <=range &&  dy <= range){
						sd->paramb[0] += guild_checkskill(g,GD_LEADERSHIP);//str
						sd->paramb[1] += guild_checkskill(g,GD_SOULCOLD);//agi
						sd->paramb[2] += guild_checkskill(g,GD_GLORYWOUNDS);//vit
						sd->paramb[4] += guild_checkskill(g,GD_HAWKEYES);//dex
						sd->under_the_influence_of_the_guild_skill = range+1;//(0>�ŉe����,�d�Ȃ�ꍇ������̂�+1)
					}else
						sd->under_the_influence_of_the_guild_skill = 0;
				}else{//�ʋ���
					int min_range=999;
					range = skill_get_range(GD_LEADERSHIP,guild_skill_get_lv(g,GD_LEADERSHIP));
					if(dx <=range &&  dy <= range){
						sd->paramb[0] += guild_checkskill(g,GD_LEADERSHIP);//str
						if(min_range>range) min_range = range;
					}
					range = skill_get_range(GD_SOULCOLD,guild_skill_get_lv(g,GD_SOULCOLD));
					if(dx <=range &&  dy <= range){
						sd->paramb[1] += guild_checkskill(g,GD_SOULCOLD);//agi
						if(min_range>range) min_range = range;
					}
					range = skill_get_range(GD_GLORYWOUNDS,guild_skill_get_lv(g,GD_GLORYWOUNDS));
					if(dx <=range &&  dy <= range){
						sd->paramb[2] += guild_checkskill(g,GD_GLORYWOUNDS);//vit
						if(min_range>range) min_range = range;
					}

					range = skill_get_range(GD_HAWKEYES,guild_skill_get_lv(g,GD_HAWKEYES));
					if(dx <=range &&  dy <= range){
						sd->paramb[4] += guild_checkskill(g,GD_HAWKEYES);//dex
						if(min_range>range) min_range = range;
					}
					if(min_range == 999) sd->under_the_influence_of_the_guild_skill = 0;
					else sd->under_the_influence_of_the_guild_skill = min_range+1;
				}

			}else{//�}�b�v�S��
				sd->paramb[0] += guild_checkskill(g,GD_LEADERSHIP);//str
				sd->paramb[1] += guild_checkskill(g,GD_SOULCOLD);//agi
				sd->paramb[2] += guild_checkskill(g,GD_GLORYWOUNDS);//vit
				sd->paramb[4] += guild_checkskill(g,GD_HAWKEYES);//dex
				sd->under_the_influence_of_the_guild_skill = battle_config.guild_skill_effective_range+1;
			}
		} else {
			sd->under_the_influence_of_the_guild_skill = 0;
		}
	}else{//�}�b�v���������c������������
		sd->under_the_influence_of_the_guild_skill = 0;
	}

	// �X�e�[�^�X�ω��ɂ���{�p�����[�^�␳
	if(sd->sc_count){

		// �W���͌���
		if(sd->sc_data[SC_CONCENTRATE].timer!=-1 && sd->sc_data[SC_QUAGMIRE].timer == -1){
			sd->paramb[1]+= (sd->status.agi+sd->paramb[1]+sd->parame[1]-sd->paramcard[1])*(2+sd->sc_data[SC_CONCENTRATE].val1)/100;
			sd->paramb[4]+= (sd->status.dex+sd->paramb[4]+sd->parame[4]-sd->paramcard[4])*(2+sd->sc_data[SC_CONCENTRATE].val1)/100;
		}
		//�S�X�y��ALL+20
		if(sd->sc_data[SC_INCALLSTATUS].timer!=-1){
			sd->paramb[0]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[1]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[2]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[3]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[4]+= sd->sc_data[SC_INCALLSTATUS].val1;
			sd->paramb[5]+= sd->sc_data[SC_INCALLSTATUS].val1;
		}

		//��ʈꎟ�E�̍�
		//��Ȃ̂�LV/10����
		if(sd->sc_data[SC_HIGH].timer!=-1)
		{
			sd->paramb[0]+= sd->status.base_level/10;
			sd->paramb[1]+= sd->status.base_level/10;
			sd->paramb[2]+= sd->status.base_level/10;
			sd->paramb[3]+= sd->status.base_level/10;
			sd->paramb[4]+= sd->status.base_level/10;
			sd->paramb[5]+= sd->status.base_level/10;
		}

		//�H���p
		if(sd->sc_data[SC_MEAL_INCSTR2].timer!=-1)
			sd->paramb[0]+= sd->sc_data[SC_MEAL_INCSTR2].val1;
		else if(sd->sc_data[SC_MEAL_INCSTR].timer!=-1)
			sd->paramb[0]+= sd->sc_data[SC_MEAL_INCSTR].val1;
		if(sd->sc_data[SC_MEAL_INCAGI2].timer!=-1)
			sd->paramb[1]+= sd->sc_data[SC_MEAL_INCAGI2].val1;
		else if(sd->sc_data[SC_MEAL_INCAGI].timer!=-1)
			sd->paramb[1]+= sd->sc_data[SC_MEAL_INCAGI].val1;
		if(sd->sc_data[SC_MEAL_INCVIT2].timer!=-1)
			sd->paramb[2]+= sd->sc_data[SC_MEAL_INCVIT2].val1;
		else if(sd->sc_data[SC_MEAL_INCVIT].timer!=-1)
			sd->paramb[2]+= sd->sc_data[SC_MEAL_INCVIT].val1;
		if(sd->sc_data[SC_MEAL_INCINT2].timer!=-1)
			sd->paramb[3]+= sd->sc_data[SC_MEAL_INCINT2].val1;
		else if(sd->sc_data[SC_MEAL_INCINT].timer!=-1)
			sd->paramb[3]+= sd->sc_data[SC_MEAL_INCINT].val1;
		if(sd->sc_data[SC_MEAL_INCDEX2].timer!=-1)
			sd->paramb[4]+= sd->sc_data[SC_MEAL_INCDEX2].val1;
		else if(sd->sc_data[SC_MEAL_INCDEX].timer!=-1)
			sd->paramb[4]+= sd->sc_data[SC_MEAL_INCDEX].val1;
		if(sd->sc_data[SC_MEAL_INCLUK2].timer!=-1)
			sd->paramb[5]+= sd->sc_data[SC_MEAL_INCLUK2].val1;
		else if(sd->sc_data[SC_MEAL_INCLUK].timer!=-1)
			sd->paramb[5]+= sd->sc_data[SC_MEAL_INCLUK].val1;

		//�삯���̃X�p�[�g��� STR+10
		if(sd->sc_data[SC_SPURT].timer!=-1)
			sd->paramb[0] += 10;

		//�M���h�X�L�� �Ր�Ԑ�
		if(sd->sc_data[SC_BATTLEORDER].timer!=-1){
			sd->paramb[0]+= 5*sd->sc_data[SC_BATTLEORDER].val1;
			sd->paramb[3]+= 5*sd->sc_data[SC_BATTLEORDER].val1;
			sd->paramb[4]+= 5*sd->sc_data[SC_BATTLEORDER].val1;
		}

		if(sd->sc_data[SC_CHASEWALK_STR].timer!=-1)
			sd->paramb[0] += sd->sc_data[SC_CHASEWALK_STR].val1;

		if(sd->sc_data[SC_INCREASEAGI].timer!=-1)	// ���x����
			sd->paramb[1]+= 2+sd->sc_data[SC_INCREASEAGI].val1;

		if(sd->sc_data[SC_DECREASEAGI].timer!=-1)	// ���x����(agi��battle.c��)
			sd->paramb[1]-= 2+sd->sc_data[SC_DECREASEAGI].val1;

		if(sd->sc_data[SC_BLESSING].timer!=-1){	// �u���b�V���O
			sd->paramb[0]+= sd->sc_data[SC_BLESSING].val1;
			sd->paramb[3]+= sd->sc_data[SC_BLESSING].val1;
			sd->paramb[4]+= sd->sc_data[SC_BLESSING].val1;
		}
		if(sd->sc_data[SC_NEN].timer!=-1){	// �O
			sd->paramb[0]+= sd->sc_data[SC_NEN].val1;
			sd->paramb[3]+= sd->sc_data[SC_NEN].val1;
		}
		if(sd->sc_data[SC_SUITON].timer!=-1){	// ����
			if(sd->sc_data[SC_SUITON].val3)
				sd->paramb[1]+=sd->sc_data[SC_SUITON].val3;
			if(sd->sc_data[SC_SUITON].val4)
				sd->speed = sd->speed*2;
		}

		if(sd->sc_data[SC_GLORIA].timer!=-1)	// �O�����A
			sd->paramb[5]+= 30;

		if(sd->sc_data[SC_LOUD].timer!=-1 && sd->sc_data[SC_QUAGMIRE].timer == -1)	// ���E�h�{�C�X
			sd->paramb[0]+= 4;

		if(sd->sc_data[SC_TRUESIGHT].timer!=-1){	// �g�D���[�T�C�g
			sd->paramb[0]+= 5;
			sd->paramb[1]+= 5;
			sd->paramb[2]+= 5;
			sd->paramb[3]+= 5;
			sd->paramb[4]+= 5;
			sd->paramb[5]+= 5;
		}

		if(sd->sc_data[SC_INCREASING].timer!=-1) // �C���N���[�V���O�A�L���A���V�[
		{
			sd->paramb[1]+= 4;
			sd->paramb[4]+= 4;
		}
		
		//�f�B�t�F���X
		if(sd->sc_data[SC_DEFENCE].timer!=-1)
			sd->paramb[2]+= sd->sc_data[SC_DEFENCE].val1*2;
		
		if(sd->sc_data[SC_QUAGMIRE].timer!=-1){	// �N�@�O�}�C�A
			short subagi = 0;
			short subdex = 0;
			subagi = (sd->status.agi/2 < sd->sc_data[SC_QUAGMIRE].val1*10) ? sd->status.agi/2 : sd->sc_data[SC_QUAGMIRE].val1*10;
			subdex = (sd->status.dex/2 < sd->sc_data[SC_QUAGMIRE].val1*10) ? sd->status.dex/2 : sd->sc_data[SC_QUAGMIRE].val1*10;
			if(map[sd->bl.m].flag.pvp || map[sd->bl.m].flag.gvg){
				subagi/= 2;
				subdex/= 2;
			}
			sd->speed = sd->speed*4/3;
			sd->paramb[1]-= subagi;
			sd->paramb[4]-= subdex;
		}

		//
		if(sd->sc_data[SC_MARIONETTE].timer!=-1){
			sd->paramb[0]-= sd->status.str/2;
			sd->paramb[1]-= sd->status.agi/2;
			sd->paramb[2]-= sd->status.vit/2;
			sd->paramb[3]-= sd->status.int_/2;
			sd->paramb[4]-= sd->status.dex/2;
			sd->paramb[5]-= sd->status.luk/2;
		}

		//
		if(sd->sc_data[SC_MARIONETTE2].timer!=-1)
		{
			struct map_session_data* ssd = map_id2sd(sd->sc_data[SC_MARIONETTE2].val2);
			if(ssd){
				if(battle_config.marionette_type){
					sd->paramb[0]+= ssd->status.str/2;
					sd->paramb[1]+= ssd->status.agi/2;
					sd->paramb[2]+= ssd->status.vit/2;
					sd->paramb[3]+= ssd->status.int_/2;
					sd->paramb[4]+= ssd->status.dex/2;
					sd->paramb[5]+= ssd->status.luk/2;
			//�o�j�}�b�v��MC����
				}else if(map[sd->bl.m].flag.pk){
					//str
					if(sd->paramb[0]+sd->parame[0]+sd->status.str < battle_config.max_marionette_pk_str)
					{
						sd->paramb[0]+= ssd->status.str/2;
						if(sd->paramb[0]+sd->parame[0]+sd->status.str > battle_config.max_marionette_pk_str)
							sd->paramb[0] = battle_config.max_marionette_pk_str - sd->status.str;
					}
					//agi
					if(sd->paramb[1]+sd->parame[1]+sd->status.agi < battle_config.max_marionette_pk_agi)
					{
						sd->paramb[1]+= ssd->status.agi/2;
						if(sd->paramb[1]+sd->parame[1]+sd->status.agi > battle_config.max_marionette_pk_agi)
							sd->paramb[1] = battle_config.max_marionette_pk_agi - sd->status.agi;
					}
					//vit
					if(sd->paramb[2]+sd->parame[2]+sd->status.vit < battle_config.max_marionette_pk_vit)
					{
						sd->paramb[2]+= ssd->status.vit/2;
						if(sd->paramb[2]+sd->parame[2]+sd->status.vit > battle_config.max_marionette_pk_vit)
							sd->paramb[2] = battle_config.max_marionette_pk_vit - sd->status.vit;
					}
					//int
					if(sd->paramb[3]+sd->parame[3]+sd->status.int_ < battle_config.max_marionette_pk_int)
					{
						sd->paramb[3]+= ssd->status.int_/2;
						if(sd->paramb[3]+sd->parame[3]+sd->status.int_ > battle_config.max_marionette_pk_int)
							sd->paramb[3] = battle_config.max_marionette_pk_int - sd->status.int_;
					}
					//dex
					if(sd->paramb[4]+sd->parame[4]+sd->status.dex < battle_config.max_marionette_pk_dex)
					{
					sd->paramb[4]+= ssd->status.dex/2;
						if(sd->paramb[4]+sd->parame[4]+sd->status.dex > battle_config.max_marionette_pk_dex)
							sd->paramb[4] = battle_config.max_marionette_pk_dex - sd->status.dex;
					}
					//luk
					if(sd->paramb[5]+sd->parame[5]+sd->status.luk < battle_config.max_marionette_pk_luk)
					{
						sd->paramb[5]+= ssd->status.luk/2;
						if(sd->paramb[5]+sd->parame[5]+sd->status.luk > battle_config.max_marionette_pk_luk)
							sd->paramb[5] = battle_config.max_marionette_pk_luk - sd->status.luk;
					}
			//�o�u�o�}�b�v��MC����
				}else if(map[sd->bl.m].flag.pvp){
					//str
					if(sd->paramb[0]+sd->parame[0]+sd->status.str < battle_config.max_marionette_pvp_str)
					{
						sd->paramb[0]+= ssd->status.str/2;
						if(sd->paramb[0]+sd->parame[0]+sd->status.str > battle_config.max_marionette_pvp_str)
							sd->paramb[0] = battle_config.max_marionette_pvp_str - sd->status.str;
					}
					//agi
					if(sd->paramb[1]+sd->parame[1]+sd->status.agi < battle_config.max_marionette_pvp_agi)
					{
						sd->paramb[1]+= ssd->status.agi/2;
						if(sd->paramb[1]+sd->parame[1]+sd->status.agi > battle_config.max_marionette_pvp_agi)
							sd->paramb[1] = battle_config.max_marionette_pvp_agi - sd->status.agi;
					}
					//vit
					if(sd->paramb[2]+sd->parame[2]+sd->status.vit < battle_config.max_marionette_pvp_vit)
					{
						sd->paramb[2]+= ssd->status.vit/2;
						if(sd->paramb[2]+sd->parame[2]+sd->status.vit > battle_config.max_marionette_pvp_vit)
							sd->paramb[2] = battle_config.max_marionette_pvp_vit - sd->status.vit;
					}
					//int
					if(sd->paramb[3]+sd->parame[3]+sd->status.int_ < battle_config.max_marionette_pvp_int)
					{
						sd->paramb[3]+= ssd->status.int_/2;
						if(sd->paramb[3]+sd->parame[3]+sd->status.int_ > battle_config.max_marionette_pvp_int)
							sd->paramb[3] = battle_config.max_marionette_pvp_int - sd->status.int_;
					}
					//dex
					if(sd->paramb[4]+sd->parame[4]+sd->status.dex < battle_config.max_marionette_pvp_dex)
					{
					sd->paramb[4]+= ssd->status.dex/2;
						if(sd->paramb[4]+sd->parame[4]+sd->status.dex > battle_config.max_marionette_pvp_dex)
							sd->paramb[4] = battle_config.max_marionette_pvp_dex - sd->status.dex;
					}
					//luk
					if(sd->paramb[5]+sd->parame[5]+sd->status.luk < battle_config.max_marionette_pvp_luk)
					{
						sd->paramb[5]+= ssd->status.luk/2;
						if(sd->paramb[5]+sd->parame[5]+sd->status.luk > battle_config.max_marionette_pvp_luk)
							sd->paramb[5] = battle_config.max_marionette_pvp_luk - sd->status.luk;
					}
			//�f�u�f�}�b�v��MC����
				}else if(map[sd->bl.m].flag.gvg){
					//str
					if(sd->paramb[0]+sd->parame[0]+sd->status.str < battle_config.max_marionette_gvg_str)
					{
						sd->paramb[0]+= ssd->status.str/2;
						if(sd->paramb[0]+sd->parame[0]+sd->status.str > battle_config.max_marionette_gvg_str)
							sd->paramb[0] = battle_config.max_marionette_gvg_str - sd->status.str;
					}
					//agi
					if(sd->paramb[1]+sd->parame[1]+sd->status.agi < battle_config.max_marionette_gvg_agi)
					{
						sd->paramb[1]+= ssd->status.agi/2;
						if(sd->paramb[1]+sd->parame[1]+sd->status.agi > battle_config.max_marionette_gvg_agi)
							sd->paramb[1] = battle_config.max_marionette_gvg_agi - sd->status.agi;
					}
					//vit
					if(sd->paramb[2]+sd->parame[2]+sd->status.vit < battle_config.max_marionette_gvg_vit)
					{
						sd->paramb[2]+= ssd->status.vit/2;
						if(sd->paramb[2]+sd->parame[2]+sd->status.vit > battle_config.max_marionette_gvg_vit)
							sd->paramb[2] = battle_config.max_marionette_gvg_vit - sd->status.vit;
					}
					//int
					if(sd->paramb[3]+sd->parame[3]+sd->status.int_ < battle_config.max_marionette_gvg_int)
					{
						sd->paramb[3]+= ssd->status.int_/2;
						if(sd->paramb[3]+sd->parame[3]+sd->status.int_ > battle_config.max_marionette_gvg_int)
							sd->paramb[3] = battle_config.max_marionette_gvg_int - sd->status.int_;
					}
					//dex
					if(sd->paramb[4]+sd->parame[4]+sd->status.dex < battle_config.max_marionette_gvg_dex)
					{
					sd->paramb[4]+= ssd->status.dex/2;
						if(sd->paramb[4]+sd->parame[4]+sd->status.dex > battle_config.max_marionette_gvg_dex)
							sd->paramb[4] = battle_config.max_marionette_gvg_dex - sd->status.dex;
					}
					//luk
					if(sd->paramb[5]+sd->parame[5]+sd->status.luk < battle_config.max_marionette_gvg_luk)
					{
						sd->paramb[5]+= ssd->status.luk/2;
						if(sd->paramb[5]+sd->parame[5]+sd->status.luk > battle_config.max_marionette_gvg_luk)
							sd->paramb[5] = battle_config.max_marionette_gvg_luk - sd->status.luk;
					}
			//�ʏ��MC����
				}else{
					//str
					if(sd->paramb[0]+sd->parame[0]+sd->status.str < battle_config.max_marionette_str)
					{
						sd->paramb[0]+= ssd->status.str/2;
						if(sd->paramb[0]+sd->parame[0]+sd->status.str > battle_config.max_marionette_str)
							sd->paramb[0] = battle_config.max_marionette_str - sd->status.str;
					}
					//agi
					if(sd->paramb[1]+sd->parame[1]+sd->status.agi < battle_config.max_marionette_agi)
					{
						sd->paramb[1]+= ssd->status.agi/2;
						if(sd->paramb[1]+sd->parame[1]+sd->status.agi > battle_config.max_marionette_agi)
							sd->paramb[1] = battle_config.max_marionette_agi - sd->status.agi;
					}
					//vit
					if(sd->paramb[2]+sd->parame[2]+sd->status.vit < battle_config.max_marionette_vit)
					{
						sd->paramb[2]+= ssd->status.vit/2;
						if(sd->paramb[2]+sd->parame[2]+sd->status.vit > battle_config.max_marionette_vit)
							sd->paramb[2] = battle_config.max_marionette_vit - sd->status.vit;
					}
					//int
					if(sd->paramb[3]+sd->parame[3]+sd->status.int_ < battle_config.max_marionette_int)
					{
						sd->paramb[3]+= ssd->status.int_/2;
						if(sd->paramb[3]+sd->parame[3]+sd->status.int_ > battle_config.max_marionette_int)
							sd->paramb[3] = battle_config.max_marionette_int - sd->status.int_;
					}
					//dex
					if(sd->paramb[4]+sd->parame[4]+sd->status.dex < battle_config.max_marionette_dex)
					{
					sd->paramb[4]+= ssd->status.dex/2;
						if(sd->paramb[4]+sd->parame[4]+sd->status.dex > battle_config.max_marionette_dex)
							sd->paramb[4] = battle_config.max_marionette_dex - sd->status.dex;
					}
					//luk
					if(sd->paramb[5]+sd->parame[5]+sd->status.luk < battle_config.max_marionette_luk)
					{
						sd->paramb[5]+= ssd->status.luk/2;
						if(sd->paramb[5]+sd->parame[5]+sd->status.luk > battle_config.max_marionette_luk)
							sd->paramb[5] = battle_config.max_marionette_luk - sd->status.luk;
					}
				}
			}
		}
	}

	sd->paramc[0]=sd->status.str+sd->paramb[0]+sd->parame[0];
	sd->paramc[1]=sd->status.agi+sd->paramb[1]+sd->parame[1];
	sd->paramc[2]=sd->status.vit+sd->paramb[2]+sd->parame[2];
	sd->paramc[3]=sd->status.int_+sd->paramb[3]+sd->parame[3];
	sd->paramc[4]=sd->status.dex+sd->paramb[4]+sd->parame[4];
	sd->paramc[5]=sd->status.luk+sd->paramb[5]+sd->parame[5];

	for(i=0;i<6;i++)
		if(sd->paramc[i] < 0) sd->paramc[i] = 0;

	//BASEATK�v�Z
	if(sd->status.weapon == WT_BOW || sd->status.weapon == WT_MUSICAL || sd->status.weapon == WT_WHIP
			|| (sd->status.weapon >= WT_HANDGUN && sd->status.weapon <= WT_GRENADE)) {
		str = sd->paramc[4];
		dex = sd->paramc[0];
	}
	else {
		str = sd->paramc[0];
		dex = sd->paramc[4];
	}
	dstr = str/10;

	sd->base_atk += str + dstr*dstr + dex/5 + sd->paramc[5]/5;
	sd->matk1 += sd->paramc[3]+(sd->paramc[3]/5)*(sd->paramc[3]/5);
	sd->matk2 += sd->paramc[3]+(sd->paramc[3]/7)*(sd->paramc[3]/7);

	//�A�C�e���␳
	if(sd->sc_data){
		if(sd->sc_data[SC_MEAL_INCATK].timer!=-1)
			sd->base_atk += sd->sc_data[SC_MEAL_INCATK].val1;

		if(sd->sc_data[SC_MEAL_INCMATK].timer!=-1){
			sd->matk1 += sd->sc_data[SC_MEAL_INCMATK].val1;
			sd->matk2 += sd->sc_data[SC_MEAL_INCMATK].val1;
		}
	}

	if(sd->matk1 < sd->matk2) {
		int temp = sd->matk2;
		sd->matk2 = sd->matk1;
		sd->matk1 = temp;
	}

	sd->hit += sd->paramc[4] + sd->status.base_level;
	sd->flee += sd->paramc[1] + sd->status.base_level;
	sd->def2 += sd->paramc[2];
	sd->mdef2 += sd->paramc[3];
	sd->flee2 += sd->paramc[5]+10;
	sd->critical += (sd->paramc[5]*3)+10;
	//�A�C�e���␳
	if(sd->sc_data){
		if(sd->sc_data[SC_MEAL_INCHIT].timer!=-1)
			sd->hit += sd->sc_data[SC_MEAL_INCHIT].val1;
		if(sd->sc_data[SC_MEAL_INCFLEE].timer!=-1)
			sd->flee += sd->sc_data[SC_MEAL_INCFLEE].val1;
		if(sd->sc_data[SC_MEAL_INCFLEE2].timer!=-1)
			sd->flee2 += sd->sc_data[SC_MEAL_INCFLEE2].val1;
		if(sd->sc_data[SC_MEAL_INCCRITICAL].timer!=-1)
			sd->critical += sd->sc_data[SC_MEAL_INCCRITICAL].val1*10;
		if(sd->sc_data[SC_MEAL_INCDEF].timer!=-1)
			sd->def+=sd->sc_data[SC_MEAL_INCDEF].val1;
		if(sd->sc_data[SC_MEAL_INCMDEF].timer!=-1)
			sd->mdef+=sd->sc_data[SC_MEAL_INCMDEF].val1;
	}

	if(sd->base_atk < 1)
		sd->base_atk = 1;
	if(sd->critical_rate != 100)
		sd->critical = (sd->critical*sd->critical_rate)/100;
	if(sd->critical < 10)
		sd->critical = 10;
	if(sd->hit_rate != 100)
		sd->hit = (sd->hit*sd->hit_rate)/100;
	if(sd->hit < 1) sd->hit = 1;
	if(sd->flee_rate != 100)
		sd->flee = (sd->flee*sd->flee_rate)/100;
	if(sd->flee < 1) sd->flee = 1;
	if(sd->flee2_rate != 100)
		sd->flee2 = (sd->flee2*sd->flee2_rate)/100;
	if(sd->flee2 < 10) sd->flee2 = 10;
	if(sd->def_rate != 100)
		sd->def = (sd->def*sd->def_rate)/100;
	if(sd->def < 0) sd->def = 0;
	if(sd->def2_rate != 100)
		sd->def2 = (sd->def2*sd->def2_rate)/100;
	if(sd->def2 < 1) sd->def2 = 1;
	if(sd->mdef_rate != 100)
		sd->mdef = (sd->mdef*sd->mdef_rate)/100;
	if(sd->mdef < 0) sd->mdef = 0;
	if(sd->mdef2_rate != 100)
		sd->mdef2 = (sd->mdef2*sd->mdef2_rate)/100;
	if(sd->mdef2 < 1) sd->mdef2 = 1;

	// �񓁗� ASPD �C��
	if (sd->status.weapon <= WT_HUUMA)
		sd->aspd += job_db[s_class.job].aspd_base[sd->status.weapon]-(sd->paramc[1]*4+sd->paramc[4])*job_db[s_class.job].aspd_base[sd->status.weapon]/1000;
	else
		sd->aspd += (
			(job_db[s_class.job].aspd_base[sd->weapontype1]-(sd->paramc[1]*4+sd->paramc[4])*job_db[s_class.job].aspd_base[sd->weapontype1]/1000) +
			(job_db[s_class.job].aspd_base[sd->weapontype2]-(sd->paramc[1]*4+sd->paramc[4])*job_db[s_class.job].aspd_base[sd->weapontype2]/1000)
			) * 140 / 200;

	aspd_rate = sd->aspd_rate;

	//�U�����x����

	//�A�h�o���X�h�u�b�N
	if(sd->weapontype1 == WT_BOOK && (skill = pc_checkskill(sd,SA_ADVANCEDBOOK)) > 0)
	{
		aspd_rate -= skill/2;
	}
	//�V���O���A�N�V����
	if(sd->status.weapon >= WT_HANDGUN && sd->status.weapon <= WT_GRENADE && (skill = pc_checkskill(sd,GS_SINGLEACTION)) > 0)
	{
		aspd_rate -= skill/2;
		sd->hit += skill*2;
	}
	//���z�ƌ��Ɛ��̈���
	if((skill = pc_checkskill(sd,SG_DEVIL)) > 0 && sd->status.job_level>=50)
	{
		aspd_rate -= skill*3;
		if(sd->sc_data[SC_DEVIL].timer!=-1 || sd->sc_data[SC_DEVIL].val1<skill)
			status_change_start(&sd->bl,SC_DEVIL,skill,0,0,0,5000,0);
		clif_status_change(&sd->bl,SI_DEVIL,1);
	}

	//���z�ƌ��Ɛ��̗Z��
	if(sd && sd->sc_data[SC_FUSION].timer!=-1)
	{
		aspd_rate -= 20;
		sd->perfect_hit += 100;
	//	if(sd->view_class==PC_CLASS_SG)
	//		sd->view_class = sd->view_class+1;
	}
	if(sd && sd->sc_data[SC_SANTA].timer!=-1)
	{
		sd->view_class = 26;
	}

	if( (skill=pc_checkskill(sd,AC_VULTURE))>0){	// ���V�̖�
		sd->hit += skill;
		if(sd->status.weapon == WT_BOW)
			sd->attackrange += skill;
	}
	if( (skill=pc_checkskill(sd,GS_SNAKEEYE))>0){	// �X�l�[�N�A�C
		if(sd->status.weapon >= WT_HANDGUN && sd->status.weapon <= WT_GRENADE)
		{
			sd->attackrange += skill;
			sd->hit += skill;
		}
	}
	if( (skill=pc_checkskill(sd,BS_WEAPONRESEARCH))>0)	// ���팤���̖���������
		sd->hit += skill*2;

	if(sd->status.option&2 && (skill = pc_checkskill(sd,RG_TUNNELDRIVE))>0 )	// �g���l���h���C�u
		sd->speed += (12*DEFAULT_WALK_SPEED - skill*90) / 10;

	if(s_class.job == 12 && (skill=pc_checkskill(sd,TF_MISS))>0)	// �A�T�V���n�̉�𗦏㏸�ɂ��ړ����x����
		sd->speed -= sd->speed * skill / 100;

	if (pc_iscarton(sd) && (skill=pc_checkskill(sd,MC_PUSHCART))>0)	// �J�[�g�ɂ�鑬�x�ቺ
		sd->speed += (10-skill) * DEFAULT_WALK_SPEED / 10;
	else if (pc_isriding(sd)){					// �y�R�y�R���ɂ�鑬�x����
		sd->max_weight += battle_config.riding_weight;		// Weight+��(�����ݒ��0)10000�Ŗ{�I;
		if(sd->sc_data[SC_DEFENDER].timer == -1)		// �f�B�t�F���_�[���͑��x�������Ȃ�
			sd->speed -= DEFAULT_WALK_SPEED / 4;
	}

	if(sd->sc_count && sd->sc_data){
		int sc_speed_rate=100;
		if(sd->sc_data[SC_AVOID].timer!=-1)				// �ً}���
			sc_speed_rate -= sd->sc_data[SC_AVOID].val1*10;
		if(sd->sc_data[SC_INCREASEAGI].timer!=-1 && sc_speed_rate > 75)	// ���x�����ɂ��ړ����x����
			sc_speed_rate = 75;
		if(sd->sc_data[SC_RUN].timer!=-1 && sc_speed_rate > 50)		// �삯���ɂ��ړ����x����
			sc_speed_rate = 50;
		if(sd->sc_data[SC_BERSERK].timer!=-1 && sc_speed_rate > 75)	// �o�[�T�[�N�ɂ��ړ����x����
			sc_speed_rate = 75;
		if(sd->sc_data[SC_CARTBOOST].timer!=-1 && sc_speed_rate > 80)	// �J�[�g�u�[�X�g�ɂ��ړ����x����
			sc_speed_rate = 80;
		if(sd->sc_data[SC_FUSION].timer!=-1 && sc_speed_rate > 75)	// ���z�ƌ��Ɛ��̗Z���ɂ��ړ����x����
			sc_speed_rate = 75;
		if(sd->sc_data[SC_WINDWALK].timer!=-1 && sc_speed_rate > 100-(sd->sc_data[SC_WINDWALK].val1*2))	// �E�B���h�E�H�[�N�ɂ��ړ����x����
			sc_speed_rate = 100-(sd->sc_data[SC_WINDWALK].val1*2);

		sd->speed = sd->speed*sc_speed_rate/100;

		if(sd->sc_data[SC_CLOAKING].timer!=-1)	// �N���[�L���O�ɂ�鑬�x�ω�
		{
			int check=1;
			for(i=0;i<8;i++){
				if(map_getcell(sd->bl.m,sd->bl.x+dirx[i],sd->bl.y+diry[i],CELL_CHKNOPASS)){
					check=0;
					break;
				}
			}
			if(check){
				// ���n�ړ����x
				sd->speed += sd->speed * (30-sd->sc_data[SC_CLOAKING].val1*3) / 100;
			}else{
				// �ǉ����ړ����x
				int rate = (sd->sc_data[SC_CLOAKING].val1 -1)*3;
				if(rate > 25)
					rate = 25;
				sd->speed -= sd->speed * rate / 100;
			}
		}

		if(sd->sc_data[SC_CHASEWALK].timer!=-1)/*�`�F�C�X�E�H�[�N�ɂ�鑬�x�ω�*/
		{
			if(sd->sc_data[SC_ROGUE].timer==-1)
				sd->speed += sd->speed*(35 - (5*sd->sc_data[SC_CHASEWALK].val1))/100;
		}

		if(sd->sc_data[SC_DECREASEAGI].timer!=-1){		// ���x����(agi��battle.c��)
			if(sd->sc_data[SC_DEFENDER].timer == -1) {	// �f�B�t�F���_�[���͑��x�ቺ���Ȃ�
				sd->speed = sd->speed *((sd->sc_data[SC_DECREASEAGI].val1>5)?150:133)/100;
			}
		}

		if(sd->sc_data[SC_WEDDING].timer!=-1)	//�������͕����̂��x��
			sd->speed = 2*DEFAULT_WALK_SPEED;
	}

	if(s_class.job == 23 && sd->status.base_level >= 99)
	{
		if(pc_isupper(sd))
			 sd->status.max_hp +=  2000*(100 + sd->paramc[2])/100 * battle_config.upper_hp_rate/100;
		else if(pc_isbaby(sd))//�{�q�̏ꍇ�ő�HP70%
			 sd->status.max_hp +=  2000*(100 + sd->paramc[2])/100 * battle_config.baby_hp_rate/100;
		else sd->status.max_hp +=  2000*(100 + sd->paramc[2])/100 * battle_config.normal_hp_rate/100;
	}

	if((skill=pc_checkskill(sd,CR_TRUST))>0) { // �t�F�C�X
		sd->status.max_hp    += skill*200;
		sd->subele[ELE_HOLY] += skill*5;
	}

	if((skill=pc_checkskill(sd,BS_SKINTEMPER))>0){ // �X�L���e���p�����O
		sd->subele[ELE_FIRE]    += skill*5;
		sd->subele[ELE_NEUTRAL] += skill*1;
	}

	//bAtkRange2,bAtkRangeRate2�̎˒��v�Z
	sd->attackrange += sd->add_attackrange;
	sd->attackrange_ += sd->add_attackrange;
	sd->attackrange = sd->attackrange * sd->add_attackrange_rate /100;
	sd->attackrange_ = sd->attackrange_ * sd->add_attackrange_rate /100;
	if(sd->attackrange < 1) sd->attackrange = 1;
	if(sd->attackrange_ < 1) sd->attackrange_ = 1;
	if(sd->attackrange < sd->attackrange_)
		sd->attackrange = sd->attackrange_;

	blv = sd->status.base_level;

	//�ő�HP�v�Z
	//�]���E�̏ꍇ�ő�HP25%UP
	calc_val = (3500 + blv * job_db[s_class.job].hp_coefficient2 + job_db[s_class.job].hp_sigma[(blv > 0)? blv-1: 0])/100 *
			(100 + sd->paramc[2])/100 + (sd->parame[2] - sd->paramcard[2]);
	if(pc_isupper(sd))
		sd->status.max_hp += calc_val * battle_config.upper_hp_rate/100;
	else if(pc_isbaby(sd))	//�{�q�̏ꍇ�ő�HP70%
		sd->status.max_hp += calc_val * battle_config.baby_hp_rate/100;
	else
		sd->status.max_hp += calc_val * battle_config.normal_hp_rate/100;

	if(sd->hprate!=100)
		sd->status.max_hp = sd->status.max_hp*sd->hprate/100;

	if(sd->sc_data && sd->sc_data[SC_BERSERK].timer!=-1){	// �o�[�T�[�N
		sd->status.max_hp = sd->status.max_hp * 3;
	}
	if(sd->sc_data && sd->sc_data[SC_INCMHP2].timer!=-1){
		sd->status.max_hp *= (100+sd->sc_data[SC_INCMHP2].val1)/100;
	}

	// �ő�SP�v�Z
	//�]���E�̏ꍇ�ő�SP125%
	calc_val = (1000 + blv * job_db[s_class.job].sp_coefficient)/100 *
			(100 + sd->paramc[3])/100 + (sd->parame[3] - sd->paramcard[3]);
	if(pc_isupper(sd))
		sd->status.max_sp += calc_val * battle_config.upper_sp_rate/100;
	else if(pc_isbaby(sd))	//�{�q�̏ꍇ�ő�SP70%
		sd->status.max_sp += calc_val * battle_config.baby_sp_rate/100;
	else
		sd->status.max_sp += calc_val * battle_config.normal_sp_rate/100;

	if(sd->sprate!=100)
		sd->status.max_sp = sd->status.max_sp*sd->sprate/100;

	if((skill=pc_checkskill(sd,HP_MEDITATIO))>0) // ���f�B�^�e�B�I
		sd->status.max_sp += sd->status.max_sp*skill/100;
	if((skill=pc_checkskill(sd,HW_SOULDRAIN))>0) /* �\�E���h���C�� */
		sd->status.max_sp += sd->status.max_sp*2*skill/100;
	if(sd->sc_data && sd->sc_data[SC_INCMSP2].timer!=-1) {
		sd->status.max_sp *= (100+sd->sc_data[SC_INCMSP2].val1)/100;
	}
	if((skill=pc_checkskill(sd,SL_KAINA))>0) /* �J�C�i */
		sd->status.max_sp += 30*skill;

	//���R��HP
	sd->nhealhp = 1 + (sd->paramc[2]/5) + (sd->status.max_hp/200);
	if((skill=pc_checkskill(sd,SM_RECOVERY)) > 0) {	/* HP�񕜗͌��� */
		sd->nshealhp = skill*5 + (sd->status.max_hp*skill/500);
		if(sd->nshealhp > 0x7fff) sd->nshealhp = 0x7fff;
	}
	if((skill=pc_checkskill(sd,TK_HPTIME)) > 0) {	// ���炩�ȋx��
		sd->tk_nhealhp = skill*30 + (sd->status.max_hp*skill/500);
		if(sd->tk_nhealhp > 0x7fff) sd->tk_nhealhp = 0x7fff;
	}
	if(sd->sc_data && sd->sc_data[SC_BERSERK].timer!=-1){
		sd->nhealhp = 0;
	}
	//���R��SP
	sd->nhealsp = 1 + (sd->paramc[3]/6) + (sd->status.max_sp/100);
	if(sd->paramc[3] >= 120)
		sd->nhealsp += ((sd->paramc[3]-120)>>1) + 4;
	if((skill=pc_checkskill(sd,MG_SRECOVERY)) > 0) { /* SP�񕜗͌��� */
		sd->nshealsp = skill*3 + (sd->status.max_sp*skill/500);
		if(sd->nshealsp > 0x7fff) sd->nshealsp = 0x7fff;
	}
	if((skill=pc_checkskill(sd,NJ_NINPOU)) > 0) { /* �E�@�C�� */
		sd->nshealsp = skill*3 + (sd->status.max_sp*skill/500);
		if(sd->nshealsp > 0x7fff) sd->nshealsp = 0x7fff;
	}

	if((skill = pc_checkskill(sd,MO_SPIRITSRECOVERY)) > 0) {
		sd->nsshealhp = skill*4 + (sd->status.max_hp*skill/500);
		sd->nsshealsp = skill*2 + (sd->status.max_sp*skill/500);
		if(sd->nsshealhp > 0x7fff) sd->nsshealhp = 0x7fff;
		if(sd->nsshealsp > 0x7fff) sd->nsshealsp = 0x7fff;
	}
	if((skill=pc_checkskill(sd,TK_SPTIME)) > 0) { // �y�����x��
		sd->tk_nhealsp = skill*3 + (sd->status.max_sp*skill/500);
		if(sd->tk_nhealsp > 0x7fff) sd->tk_nhealsp = 0x7fff;
	}
	if(sd->hprecov_rate != 100) {
		sd->nhealhp = sd->nhealhp*sd->hprecov_rate/100;
		if(sd->nhealhp < 1) sd->nhealhp = 1;
	}
	if(sd->sprecov_rate != 100) {
		sd->nhealsp = sd->nhealsp*sd->sprecov_rate/100;
		if(sd->nhealsp < 1) sd->nhealsp = 1;
	}
	if((skill=pc_checkskill(sd,HP_MEDITATIO)) > 0) {
		// ���f�B�^�e�B�I��SPR�ł͂Ȃ����R�񕜂ɂ�����
		sd->nhealsp += (sd->nhealsp)*3*skill/100;
		if(sd->nhealsp > 0x7fff) sd->nhealsp = 0x7fff;
	}

	// �푰�ϐ��i����ł����́H �f�B�o�C���v���e�N�V�����Ɠ������������邩���j
	if( (skill=pc_checkskill(sd,SA_DRAGONOLOGY))>0 ){	// �h���S�m���W�[
		skill = skill*4;
		sd->addrace[RCT_DRAGON]  += skill;
		sd->addrace_[RCT_DRAGON] += skill;
		sd->subrace[RCT_DRAGON]  += skill;
	}
	//Flee�㏸
	if( (skill=pc_checkskill(sd,TF_MISS))>0 ) {	// ��𗦑���
		if( s_class.job == 12 || s_class.job == 17 )
			sd->flee += skill*4;
		else
			sd->flee += skill*3;
	}
	if( (skill=pc_checkskill(sd,MO_DODGE))>0 )	// ���؂�
		sd->flee += (skill*3)>>1;
	if( sd->sc_data ) {
		if( sd->sc_data[SC_INCFLEE].timer!=-1 )
			sd->flee += sd->sc_data[SC_INCFLEE].val1;
		if( sd->sc_data[SC_INCFLEE2].timer!=-1 )
			sd->flee += sd->sc_data[SC_INCFLEE2].val1;
	}

	// �X�L����X�e�[�^�X�ُ�ɂ��c��̃p�����[�^�␳
	if(sd->sc_count){
		//���z�̈��y DEF����
		if(sd->sc_data[SC_SUN_COMFORT].timer!=-1 && sd->bl.m == sd->feel_index[0])
			sd->def2 += (sd->status.base_level + sd->status.dex + sd->status.luk)/2;
			//sd->def += (sd->status.base_level + sd->status.dex + sd->status.luk + sd->paramb[4] + sd->paramb[5])/10;

		//���̈��y
		if(sd->sc_data[SC_MOON_COMFORT].timer!=-1 && sd->bl.m == sd->feel_index[1])
			sd->flee += (sd->status.base_level + sd->status.dex + sd->status.luk)/10;
			//sd->flee += (sd->status.base_level + sd->status.dex + sd->status.luk + sd->paramb[4] + sd->paramb[5])/10;

		//���̈��y
		if(sd->sc_data[SC_STAR_COMFORT].timer!=-1 && sd->bl.m == sd->feel_index[2])
			aspd_rate -= (sd->status.base_level + sd->status.dex + sd->status.luk)/10;
			//aspd_rate += (sd->status.base_level + sd->status.dex + sd->status.luk + sd->paramb[0] + sd->paramb[4] + sd->paramb[5])/10;

		//�N���[�Y�R���t�@�C��
		if(sd->sc_data[SC_CLOSECONFINE].timer!=-1)
			sd->flee += 10;

		// ATK/DEF�ω��`
		if(sd->sc_data[SC_ANGELUS].timer!=-1)	// �G���W�F���X
			sd->def2 = sd->def2*(110+5*sd->sc_data[SC_ANGELUS].val1)/100;
		if(sd->sc_data[SC_IMPOSITIO].timer!=-1)	{// �C���|�V�e�B�I�}�k�X
			sd->watk += sd->sc_data[SC_IMPOSITIO].val1*5;
			index = sd->equip_index[8];
			// ����ɂ͓K�p���Ȃ�
			//if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
			//	sd->watk_ += sd->sc_data[SC_IMPOSITIO].val1*5;
		}
		if(sd->sc_data[SC_PROVOKE].timer!=-1){	// �v���{�b�N
			sd->def2 = sd->def2*(100-6*sd->sc_data[SC_PROVOKE].val1)/100;
			sd->base_atk = sd->base_atk*(100+2*sd->sc_data[SC_PROVOKE].val1)/100;
			sd->watk = sd->watk*(100+2*sd->sc_data[SC_PROVOKE].val1)/100;
			index = sd->equip_index[8];
			// ����ɂ͓K�p���Ȃ�
			//if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
			//	sd->watk_ = sd->watk_*(100+2*sd->sc_data[SC_PROVOKE].val1)/100;
		}
		if(sd->sc_data[SC_POISON].timer!=-1)	// �ŏ��
			sd->def2 = sd->def2*75/100;

		//�^���b�g�J�[�h
		if(sd->sc_data[SC_TAROTCARD].timer!=-1 && sd->sc_data[SC_TAROTCARD].val4 == SC_THE_MAGICIAN)
		{
			//THE MAGICIAN ATK����
			//ATK
			sd->base_atk = sd->base_atk * 50/100;
			sd->watk = sd->watk * 50/100;
			index = sd->equip_index[8];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
				sd->watk_ = sd->watk_ * 50/100;
		}
		else if(sd->sc_data[SC_TAROTCARD].timer!=-1 && sd->sc_data[SC_TAROTCARD].val4 == SC_STRENGTH)
		{	//STRENGTH MATK����
			//MATK
			sd->matk1 = sd->matk1*50/100;
			sd->matk2 = sd->matk2*50/100;
		}
		else if(sd->sc_data[SC_TAROTCARD].timer!=-1 && sd->sc_data[SC_TAROTCARD].val4 == SC_THE_DEVIL)
		{	//THE DEVIL ATK�����AMATK����
			//ATK
			sd->base_atk = sd->base_atk * 50/100;
			sd->watk = sd->watk * 50/100;
			index = sd->equip_index[8];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
				sd->watk_ = sd->watk_ * 50/100;

			//MATK
			sd->matk1 = sd->matk1*50/100;
			sd->matk2 = sd->matk2*50/100;
		}
		else if(sd->sc_data[SC_TAROTCARD].timer!=-1 && sd->sc_data[SC_TAROTCARD].val4 == SC_THE_SUN)
		{
			//THE SUN ATK�AMATK�A����A�����A�h��͂��S��20%���������� 536
			//ATK
			sd->base_atk = sd->base_atk * 80/100;
			sd->watk = sd->watk * 80/100;
			index = sd->equip_index[8];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
				sd->watk_ = sd->watk_ * 80/100;
			//MATK
			sd->matk1 = sd->matk1*80/100;
			sd->matk2 = sd->matk2*80/100;
			//
			sd->flee = sd->flee * 80/100;
			//
			sd->hit  = sd->hit * 80/100;
			//�h���
			sd->def = sd->def * 80/100;
			sd->def2 = sd->def2 * 80/100;
		}

		if(sd->sc_data[SC_DRUMBATTLE].timer!=-1){	// �푾�ۂ̋���
			sd->watk += sd->sc_data[SC_DRUMBATTLE].val2;
			sd->def  += sd->sc_data[SC_DRUMBATTLE].val3;
			index = sd->equip_index[8];
			// ����ɂ͓K�p���Ȃ�
			//if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
			//	sd->watk_ += sd->sc_data[SC_DRUMBATTLE].val2;
		}
		if(sd->sc_data[SC_NIBELUNGEN].timer!=-1) {	// �j�[�x�����O�̎w��
			index = sd->equip_index[9];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->wlv >= 4)
				sd->watk += sd->sc_data[SC_NIBELUNGEN].val2;
			index = sd->equip_index[8];
			// ����ɂ͓K�p���Ȃ�
			//if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->wlv >= 4)
			//	sd->watk_ += sd->sc_data[SC_NIBELUNGEN].val2;
		}

		if(sd->sc_data[SC_VOLCANO].timer!=-1 && sd->def_ele==ELE_FIRE){	// �{���P�[�m
			sd->watk += sd->sc_data[SC_VOLCANO].val3;
		}
		if(sd->sc_data[SC_INCATK2].timer!=-1) {
			sd->watk = sd->watk*(100+sd->sc_data[SC_INCATK2].val1)/100;
		}

		if(sd->sc_data[SC_SIGNUMCRUCIS].timer!=-1)
			sd->def = sd->def * (100 - sd->sc_data[SC_SIGNUMCRUCIS].val2)/100;
		if(sd->sc_data[SC_ETERNALCHAOS].timer!=-1)	// �G�^�[�i���J�I�X
			sd->def2=0;

		if(sd->sc_data[SC_CONCENTRATION].timer!=-1){ //�R���Z���g���[�V����
			sd->base_atk = sd->base_atk * (100 + 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
			sd->watk = sd->watk * (100 + 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
			index = sd->equip_index[8];
			if(index >= 0 && sd->inventory_data[index] && sd->inventory_data[index]->type == 4)
				sd->watk_ = sd->watk_ * (100 + 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
			sd->def = sd->def * (100 - 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
			sd->def2 = sd->def2 * (100 - 5*sd->sc_data[SC_CONCENTRATION].val1)/100;
		}

		if(sd->sc_data[SC_INCATK].timer!=-1)	//item 682�p
			sd->watk += sd->sc_data[SC_INCATK].val1;
		if(sd->sc_data[SC_INCMATK].timer!=-1){	//item 683�p
			sd->matk1 += sd->sc_data[SC_INCMATK].val1;
			sd->matk2 += sd->sc_data[SC_INCMATK].val1;
		}
		if (sd->sc_data[SC_MINDBREAKER].timer!=-1) {
			sd->matk1 += (sd->matk1*20*sd->sc_data[SC_MINDBREAKER].val1)/100;
			sd->matk2 += (sd->matk2*20*sd->sc_data[SC_MINDBREAKER].val1)/100;
			sd->mdef -= (sd->mdef*12*sd->sc_data[SC_MINDBREAKER].val1)/100;
		}
		if (sd->sc_data[SC_ENDURE].timer!=-1) {
			sd->mdef += sd->sc_data[SC_ENDURE].val1;
		}

		// ASPD/�ړ����x�ω��n
		if(sd->sc_data[SC_TWOHANDQUICKEN].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->sc_data[SC_DECREASEAGI].timer==-1 )	// 2HQ
			aspd_rate -= 30;

		if(sd->sc_data[SC_ONEHAND].timer != -1 && sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->sc_data[SC_DECREASEAGI].timer==-1)	// 1HQ
			aspd_rate -= 30;

		if(sd->sc_data[SC_ADRENALINE2].timer != -1 && sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_ONEHAND].timer == -1 &&
			sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->sc_data[SC_DECREASEAGI].timer==-1) {	// �A�h���i�������b�V��2
			if(sd->sc_data[SC_ADRENALINE2].val2 || !battle_config.party_skill_penalty)
				aspd_rate -= 30;
			else
				aspd_rate -= 25;
		}else if(sd->sc_data[SC_ADRENALINE].timer != -1 && sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_ONEHAND].timer == -1 &&
			sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->sc_data[SC_DECREASEAGI].timer==-1) {	// �A�h���i�������b�V��
			if(sd->sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
				aspd_rate -= 30;
			else
				aspd_rate -= 25;
		}
		if(sd->sc_data[SC_SPEARSQUICKEN].timer != -1 && sd->sc_data[SC_ADRENALINE].timer == -1 && sd->sc_data[SC_ADRENALINE2].timer == -1 &&
			sd->sc_data[SC_TWOHANDQUICKEN].timer == -1 && sd->sc_data[SC_ONEHAND].timer == -1 &&
			sd->sc_data[SC_QUAGMIRE].timer == -1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->sc_data[SC_DECREASEAGI].timer==-1)	// �X�s�A�N�B�b�P��
			aspd_rate -= sd->sc_data[SC_SPEARSQUICKEN].val2;

		if(sd->sc_data[SC_ASSNCROS].timer!=-1 && // �[�z�̃A�T�V���N���X
			sd->sc_data[SC_TWOHANDQUICKEN].timer==-1 && sd->sc_data[SC_ONEHAND].timer==-1 &&
			sd->sc_data[SC_ADRENALINE].timer==-1 && sd->sc_data[SC_ADRENALINE2].timer==-1 &&
			sd->sc_data[SC_SPEARSQUICKEN].timer==-1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->status.weapon != WT_BOW && !(sd->status.weapon >= WT_HANDGUN && sd->status.weapon <= WT_GRENADE))
				aspd_rate -= 5+sd->sc_data[SC_ASSNCROS].val1+sd->sc_data[SC_ASSNCROS].val2+sd->sc_data[SC_ASSNCROS].val3;
		else if(sd->sc_data[SC_ASSNCROS_].timer!=-1 && // �[�z�̃A�T�V���N���X
			sd->sc_data[SC_TWOHANDQUICKEN].timer==-1 && sd->sc_data[SC_ONEHAND].timer==-1 &&
			sd->sc_data[SC_ADRENALINE].timer==-1 && sd->sc_data[SC_ADRENALINE2].timer==-1 &&
			sd->sc_data[SC_SPEARSQUICKEN].timer==-1 && sd->sc_data[SC_DONTFORGETME].timer == -1 && sd->status.weapon != WT_BOW && !(sd->status.weapon >= WT_HANDGUN && sd->status.weapon <= WT_GRENADE))
				aspd_rate -= 5+sd->sc_data[SC_ASSNCROS_].val1+sd->sc_data[SC_ASSNCROS_].val2+sd->sc_data[SC_ASSNCROS_].val3;

		if(sd->sc_data[SC_DONTFORGETME].timer!=-1){		// ����Y��Ȃ���
			aspd_rate += sd->sc_data[SC_DONTFORGETME].val1*3 + sd->sc_data[SC_DONTFORGETME].val2 + (sd->sc_data[SC_DONTFORGETME].val3>>16);
			sd->speed= sd->speed*(100+sd->sc_data[SC_DONTFORGETME].val1*2 + sd->sc_data[SC_DONTFORGETME].val2 + (sd->sc_data[SC_DONTFORGETME].val3&0xffff))/100;
		}else if(sd->sc_data[SC_DONTFORGETME_].timer!=-1){		// ����Y��Ȃ���
			aspd_rate += sd->sc_data[SC_DONTFORGETME_].val1*3 + sd->sc_data[SC_DONTFORGETME_].val2 + (sd->sc_data[SC_DONTFORGETME_].val3>>16);
			sd->speed= sd->speed*(100+sd->sc_data[SC_DONTFORGETME_].val1*2 + sd->sc_data[SC_DONTFORGETME_].val2 + (sd->sc_data[SC_DONTFORGETME_].val3&0xffff))/100;
		}

		if(sd->sc_data[SC_GRAVITATION].timer!=-1)
		{
			aspd_rate += sd->sc_data[SC_GRAVITATION].val1*5;
			if(battle_config.player_gravitation_type)
				sd->speed = sd->speed*(100+sd->sc_data[SC_GRAVITATION].val1*5)/100;

		}
		if(sd->sc_data[SC_BERSERK].timer!=-1){
			aspd_rate -= 30;
		}
		if(sd->sc_data[SC_POISONPOTION].timer!=-1){
			aspd_rate -= 25;
		}
		if(	sd->sc_data[i=SC_SPEEDPOTION3].timer!=-1 ||
			sd->sc_data[i=SC_SPEEDPOTION2].timer!=-1 ||
			sd->sc_data[i=SC_SPEEDPOTION1].timer!=-1 ||
			sd->sc_data[i=SC_SPEEDPOTION0].timer!=-1)	// �� ���|�[�V����
			aspd_rate -= sd->sc_data[i].val2;

		// HIT/FLEE�ω��n
		if(sd->sc_data[SC_WHISTLE].timer!=-1){  // ���J
			sd->flee += sd->flee * (sd->sc_data[SC_WHISTLE].val1
					+sd->sc_data[SC_WHISTLE].val2+(sd->sc_data[SC_WHISTLE].val3>>16))/100;
			sd->flee2+= (sd->sc_data[SC_WHISTLE].val1+sd->sc_data[SC_WHISTLE].val2+(sd->sc_data[SC_WHISTLE].val3&0xffff)) * 10;
		}else if(sd->sc_data[SC_WHISTLE_].timer!=-1){  // ���J
			sd->flee += sd->flee * (sd->sc_data[SC_WHISTLE_].val1
					+sd->sc_data[SC_WHISTLE_].val2+(sd->sc_data[SC_WHISTLE_].val3>>16))/100;
			sd->flee2+= (sd->sc_data[SC_WHISTLE_].val1+sd->sc_data[SC_WHISTLE_].val2+(sd->sc_data[SC_WHISTLE_].val3&0xffff)) * 10;
		}

		if(sd->sc_data[SC_HUMMING].timer!=-1){  // �n�~���O
			sd->hit += (sd->sc_data[SC_HUMMING].val1*2+sd->sc_data[SC_HUMMING].val2
					+sd->sc_data[SC_HUMMING].val3) * sd->hit/100;
		}else if(sd->sc_data[SC_HUMMING_].timer!=-1){  // �n�~���O
			sd->hit += (sd->sc_data[SC_HUMMING_].val1*2+sd->sc_data[SC_HUMMING_].val2
					+sd->sc_data[SC_HUMMING_].val3) * sd->hit/100;
		}

		if(sd->sc_data[SC_VIOLENTGALE].timer!=-1 && sd->def_ele==ELE_WIND){	// �o�C�I�����g�Q�C��
			sd->flee += sd->flee*sd->sc_data[SC_VIOLENTGALE].val3/100;
		}
		if(sd->sc_data[SC_BLIND].timer!=-1){	// �Í�
			sd->hit -= sd->hit*25/100;
			sd->flee -= sd->flee*25/100;
		}
		if(sd->sc_data[SC_WINDWALK].timer!=-1) // �E�B���h�E�H�[�N
			sd->flee += sd->flee*(sd->sc_data[SC_WINDWALK].val2)/100;
		if(sd->sc_data[SC_SPIDERWEB].timer!=-1) //�X�p�C�_�[�E�F�u
			sd->flee -= sd->flee*50/100;
		if(sd->sc_data[SC_TRUESIGHT].timer!=-1) //�g�D���[�T�C�g
			sd->hit += 3*(sd->sc_data[SC_TRUESIGHT].val1);
		if(sd->sc_data[SC_CONCENTRATION].timer!=-1) //�R���Z���g���[�V����
			sd->hit += 10*(sd->sc_data[SC_CONCENTRATION].val1);
		if(sd->sc_data[SC_INCHIT].timer!=-1)
			sd->hit += sd->sc_data[SC_INCHIT].val1;
		if(sd->sc_data[SC_INCHIT2].timer!=-1)
			sd->hit *= (100+sd->sc_data[SC_INCHIT2].val1)/100;
		if(sd->sc_data[SC_BERSERK].timer!=-1)
			sd->flee -= sd->flee*50/100;
		if(sd->sc_data[SC_INCFLEE].timer!=-1) // ���x����
			sd->flee += sd->flee*(sd->sc_data[SC_INCFLEE].val2)/100;

		//�K���X�����K�[�X�L��
		if(sd->sc_data[SC_FLING].timer!=-1)			// �t���C���O
			sd->def = sd->def * (100 - 5*sd->sc_data[SC_FLING].val1)/100;
		if(sd->sc_data[SC_MADNESSCANCEL].timer!=-1){ // �}�b�h�l�X�L�����Z���[
			sd->base_atk += 100;
			aspd_rate -= 20;
		}
		if(sd->sc_data[SC_ADJUSTMENT].timer!=-1){ // �A�W���X�g�����g
			sd->hit -= 30;
			sd->flee += 30;
		}
		if(sd->sc_data[SC_INCREASING].timer!=-1) // �C���N���[�V���O�A�L���A���V�[
			sd->hit += 20;
		if(sd->sc_data[SC_GATLINGFEVER].timer!=-1){ // �K�g�����O�t�B�[�o�[
			aspd_rate -= 20;
			sd->flee -= sd->flee*(sd->sc_data[SC_GATLINGFEVER].val1*5)/100;
			sd->speed += sd->flee*(sd->sc_data[SC_GATLINGFEVER].val1*5)/100;
		}

		// �ϐ�
		if(sd->sc_data[SC_RESISTWATER].timer!=-1)
			sd->subele[ELE_WATER]  += sd->sc_data[SC_RESISTWATER].val1;
		if(sd->sc_data[SC_RESISTGROUND].timer!=-1)
			sd->subele[ELE_EARTH]  += sd->sc_data[SC_RESISTGROUND].val1;
		if(sd->sc_data[SC_RESISTFIRE].timer!=-1)
			sd->subele[ELE_FIRE]   += sd->sc_data[SC_RESISTFIRE].val1;
		if(sd->sc_data[SC_RESISTWIND].timer!=-1)
			sd->subele[ELE_WIND]   += sd->sc_data[SC_RESISTWIND].val1;
		if(sd->sc_data[SC_RESISTPOISON].timer!=-1)
			sd->subele[ELE_POISON] += sd->sc_data[SC_RESISTPOISON].val1;
		if(sd->sc_data[SC_RESISTHOLY].timer!=-1)
			sd->subele[ELE_HOLY]   += sd->sc_data[SC_RESISTHOLY].val1;
		if(sd->sc_data[SC_RESISTDARK].timer!=-1)
			sd->subele[ELE_DARK]   += sd->sc_data[SC_RESISTDARK].val1;
		if(sd->sc_data[SC_RESISTTELEKINESIS].timer!=-1)
			sd->subele[ELE_GHOST]  += sd->sc_data[SC_RESISTTELEKINESIS].val1;
		if(sd->sc_data[SC_RESISTUNDEAD].timer!=-1)
			sd->subele[ELE_UNDEAD] += sd->sc_data[SC_RESISTUNDEAD].val1;

		// �ϐ�
		if(sd->sc_data[SC_RESISTALL].timer!=-1) {
			for(i=1; i<ELE_MAX; i++)
				sd->subele[i] += sd->sc_data[SC_RESISTALL].val1;	// �S�Ăɑϐ�����
		}
		// �s���g�̃W�[�N�t���[�h
		if(sd->sc_data[SC_SIEGFRIED].timer!=-1) {
			for(i=1; i<ELE_MAX; i++)
				sd->subele[i] += sd->sc_data[SC_SIEGFRIED].val2;	// �S�Ăɑϐ�����
		}
		// �v�����B�f���X
		if(sd->sc_data[SC_PROVIDENCE].timer!=-1) {
			sd->subele[ELE_HOLY]   += sd->sc_data[SC_PROVIDENCE].val2;	// �ΐ�����
			sd->subrace[RCT_DEMON] += sd->sc_data[SC_PROVIDENCE].val2;	// �Έ���
		}

		// ���̑�
		if(sd->sc_data[SC_BERSERK].timer!=-1){	// �o�[�T�[�N
			sd->def = 0;
			sd->def2 = 0;
			sd->mdef = 0;
			sd->mdef2 = 0;
		}
		if(sd->sc_data[SC_JOINTBEAT].timer != -1) {	// �W���C���g�r�[�g
			switch (sd->sc_data[SC_JOINTBEAT].val4) {
				case 0:		// ����
					sd->speed += (sd->speed * 50)/100;
					break;
				case 1:		// ���
					sd->aspd -= (sd->aspd * 25)/100;
					break;
				case 2:		// �G
					sd->speed += (sd->speed * 30)/100;
					sd->aspd  -= (sd->aspd * 10)/100;
					break;
				case 3:		// ��
					sd->def2 -= (sd->def2 * 50)/100;
					break;
				case 4:		// ��
					sd->def2     -= (sd->def2 * 25)/100;
					sd->base_atk -= (sd->base_atk * 25)/100;
					break;
				case 5:		// ��
					sd->critical_def -= (sd->critical_def * 50)/100;
					break;
			}
		}
		if(sd->sc_data[SC_APPLEIDUN].timer!=-1){	// �C�h�D���̗ь�
			sd->status.max_hp += ((5+sd->sc_data[SC_APPLEIDUN].val1*2+((sd->sc_data[SC_APPLEIDUN].val2+1)>>1)
						+sd->sc_data[SC_APPLEIDUN].val3/10) * sd->status.max_hp)/100;

		}else if(sd->sc_data[SC_APPLEIDUN_].timer!=-1){	// �C�h�D���̗ь�
			sd->status.max_hp += ((5+sd->sc_data[SC_APPLEIDUN_].val1*2+((sd->sc_data[SC_APPLEIDUN_].val2+1)>>1)
						+sd->sc_data[SC_APPLEIDUN_].val3/10) * sd->status.max_hp)/100;
		}

		if(sd->sc_data[SC_DELUGE].timer!=-1 && sd->def_ele==ELE_WATER){	// �f�����[�W
			sd->status.max_hp += sd->status.max_hp*sd->sc_data[SC_DELUGE].val3/100;
		}
		if(sd->sc_data[SC_SERVICE4U].timer!=-1) {	// �T�[�r�X�t�H�[���[
			sd->status.max_sp += sd->status.max_sp*(10+sd->sc_data[SC_SERVICE4U].val1+sd->sc_data[SC_SERVICE4U].val2
						+sd->sc_data[SC_SERVICE4U].val3)/100;

			sd->dsprate-=(10+sd->sc_data[SC_SERVICE4U].val1*3+sd->sc_data[SC_SERVICE4U].val2
					+sd->sc_data[SC_SERVICE4U].val3);
			if(sd->dsprate<0)sd->dsprate=0;
		}else if(sd->sc_data[SC_SERVICE4U_].timer!=-1) {	// �T�[�r�X�t�H�[���[
			sd->status.max_sp += sd->status.max_sp*(10+sd->sc_data[SC_SERVICE4U_].val1+sd->sc_data[SC_SERVICE4U_].val2
						+sd->sc_data[SC_SERVICE4U_].val3)/100;

			sd->dsprate-=(10+sd->sc_data[SC_SERVICE4U_].val1*3+sd->sc_data[SC_SERVICE4U_].val2
					+sd->sc_data[SC_SERVICE4U_].val3);
			if(sd->dsprate<0)sd->dsprate=0;
		}

		if(sd->sc_data[SC_FORTUNE].timer!=-1){	// �K�^�̃L�X
			sd->critical += (10+sd->sc_data[SC_FORTUNE].val1+sd->sc_data[SC_FORTUNE].val2
						+sd->sc_data[SC_FORTUNE].val3)*10;
		}else if(sd->sc_data[SC_FORTUNE_].timer!=-1){	// �K�^�̃L�X
			sd->critical += (10+sd->sc_data[SC_FORTUNE_].val1+sd->sc_data[SC_FORTUNE_].val2
						+sd->sc_data[SC_FORTUNE_].val3)*10;
		}

		if(sd->sc_data[SC_EXPLOSIONSPIRITS].timer!=-1){	// �����g��
			if(s_class.job==23)
				sd->critical += sd->sc_data[SC_EXPLOSIONSPIRITS].val1*100;
			else
				sd->critical += sd->sc_data[SC_EXPLOSIONSPIRITS].val2;
		}

		if(sd->sc_data[SC_STEELBODY].timer!=-1){	// ����
			sd->def = 90;
			sd->mdef = 90;
			aspd_rate += 25;
			sd->speed = (sd->speed * 135) / 100;
		}
		if(sd->sc_data[SC_DEFENDER].timer != -1) {	// �f�B�t�F���_�[
			sd->aspd += (25 - sd->sc_data[SC_DEFENDER].val1*5);
			sd->speed = (sd->speed * (155 - sd->sc_data[SC_DEFENDER].val1*5)) / 100;
		}
		if(sd->sc_data[SC_ENCPOISON].timer != -1)
			sd->addeff[4] += sd->sc_data[SC_ENCPOISON].val2;

		if (sd->sc_data[SC_DANCING].timer != -1 && sd->sc_data[SC_BARDDANCER].timer == -1) // �x��/���t
		{
			if (sd->sc_data[SC_LONGINGFREEDOM].timer != -1)
			{
				if (sd->sc_data[SC_LONGINGFREEDOM].val1 < 5)
				{
					sd->speed = sd->speed * (200-20*sd->sc_data[SC_LONGINGFREEDOM].val1)/100;
					sd->aspd  = sd->aspd * (200-20*sd->sc_data[SC_LONGINGFREEDOM].val1)/100;
				}
			}
			else
			{
				int lesson = pc_checkskill(sd,BA_MUSICALLESSON) > pc_checkskill(sd,DC_DANCINGLESSON) ?
					pc_checkskill(sd,BA_MUSICALLESSON) : pc_checkskill(sd,DC_DANCINGLESSON);
				sd->speed = sd->speed * (400-20*lesson)/100;
			}
		}

		if(sd->sc_data[SC_CURSE].timer!=-1){
			if(sd->sc_data[SC_DEFENDER].timer != -1)//�f�B�t�F���_�[���͎􂢂ő��x�ቺ���Ȃ�
				sd->speed += 0;
			else
				sd->speed += 450;
		}
		if(sd->sc_data[SC_TRUESIGHT].timer!=-1) //�g�D���[�T�C�g
			sd->critical += 10*(sd->sc_data[SC_TRUESIGHT].val1);

/*		if(sd->sc_data[SC_VOLCANO].timer!=-1)	// �G���`�����g�|�C�Y��(������battle.c��)
			sd->addeff[2]+=sd->sc_data[SC_VOLCANO].val2;//% of granting
		if(sd->sc_data[SC_DELUGE].timer!=-1)	// �G���`�����g�|�C�Y��(������battle.c��)
			sd->addeff[0]+=sd->sc_data[SC_DELUGE].val2;//% of granting
		*/
	}
	//�e�R�������J�[�{�[�i�X
	if(sd->status.class_ == PC_CLASS_TK && sd->status.base_level >= 90 && ranking_get_pc_rank(sd,RK_TAEKWON) > 0)
	{
		sd->status.max_hp*=3;
		sd->status.max_sp*=3;
	}

	if(sd->speed_rate != 100)
		sd->speed = sd->speed*sd->speed_rate/100;
	if(sd->speed < 1) sd->speed = 1;
	if(aspd_rate != 100)
		sd->aspd = sd->aspd*aspd_rate/100;
	if(pc_isriding(sd))							// �R���C��
		sd->aspd = sd->aspd*(100 + 10*(5 - pc_checkskill(sd,KN_CAVALIERMASTERY)))/ 100;
	if(sd->aspd < battle_config.max_aspd) sd->aspd = battle_config.max_aspd;
	if(map[sd->bl.m].flag.pk){
		if(sd->aspd < battle_config.pk_max_aspd) sd->aspd = battle_config.pk_max_aspd;
	}
	if(map[sd->bl.m].flag.gvg){
		if(sd->aspd < battle_config.gvg_max_aspd) sd->aspd = battle_config.gvg_max_aspd;
	}
	if(map[sd->bl.m].flag.pvp){
		if(sd->aspd < battle_config.pvp_max_aspd) sd->aspd = battle_config.pvp_max_aspd;
	}
	sd->amotion = sd->aspd;
	sd->dmotion = 800-sd->paramc[1]*4;
	if(sd->dmotion<400)
		sd->dmotion = 400;
	if(sd->ud.skilltimer != -1 && (skill = pc_checkskill(sd,SA_FREECAST)) > 0) {
		sd->prev_speed = sd->speed;
		sd->speed = sd->speed*(175 - skill*5)/100;
	}

	if(sd->matk_rate!=100){
		sd->matk1 = sd->matk1*sd->matk_rate/100;
		sd->matk2 = sd->matk2*sd->matk_rate/100;
	}
	if(sd->matk1<1)
	 	sd->matk1 = 1;
	if(sd->matk2<1)
		sd->matk2 = 1;

	if(sd->status.max_hp>battle_config.max_hp)
		sd->status.max_hp=battle_config.max_hp;
	if(sd->status.max_sp>battle_config.max_sp)
		sd->status.max_sp=battle_config.max_sp;

	if(sd->status.max_hp<=0)
		sd->status.max_hp=1;
	if(sd->status.max_sp<=0)
		sd->status.max_sp=1;

	if(sd->status.hp>sd->status.max_hp)
		sd->status.hp=sd->status.max_hp;
	if(sd->status.sp>sd->status.max_sp)
		sd->status.sp=sd->status.max_sp;

	// bTigereye���Ȃ��Ȃ��Ă�����p�P�b�g�����Č��ɖ߂�
	if(b_tigereye == 1 && sd->infinite_tigereye == 0 && sd->sc_data[SC_TIGEREYE].timer == -1)
		clif_status_change(&sd->bl, SI_TIGEREYE, 0);

	// �v�Z���������܂�
	if( sd->status_calc_pc_process > 1 ) {
		// ���̊֐����ċA�I�ɌĂ΂ꂽ�̂ŁA�Čv�Z����
		if( --calclimit ) {
			sd->status_calc_pc_process = 1;
			goto L_RECALC;
		} else {
			// �������[�v�ɂȂ����̂Ōv�Z�ł��؂�
			printf("status_calc_pc: infinity loop!\n");
		}
	}
	sd->status_calc_pc_process = 0;

	if(first&4)
		return 0;
	if(first&3) {
		clif_updatestatus(sd,SP_SPEED);
		clif_updatestatus(sd,SP_MAXHP);
		clif_updatestatus(sd,SP_MAXSP);
		if(first&1) {
			clif_updatestatus(sd,SP_HP);
			clif_updatestatus(sd,SP_SP);
		}
		return 0;
	}

	if(b_class != sd->view_class) {
		clif_changelook(&sd->bl,LOOK_BASE,sd->view_class);
#if PACKETVER < 4
		clif_changelook(&sd->bl,LOOK_WEAPON,sd->status.weapon);
		clif_changelook(&sd->bl,LOOK_SHIELD,sd->status.shield);
#else
		clif_changelook(&sd->bl,LOOK_WEAPON,0);
#endif
	}

	if( memcmp(b_skill,sd->status.skill,sizeof(sd->status.skill)) || b_attackrange != sd->attackrange) {
		int type;
		for(i=0; i<MAX_SKILL; i++) {
			// �J�[�h�X�L�������X�g�����Ƃ����������^�Ȃ��Ԉُ������
			if(b_skill[i].flag == 1 && b_skill[i].lv > 0 && sd->status.skill[i].lv <= 0 && skill_get_inf(i) == 4) {
				type = GetSkillStatusChangeTable(i);
				if(type >= 0 && sd->sc_data && sd->sc_data[type].timer != -1)
					status_change_end(&sd->bl, type, -1);
			}
		}
		clif_skillinfoblock(sd);	// �X�L�����M
	}

	if(b_speed != sd->speed)
		clif_updatestatus(sd,SP_SPEED);
	if(b_weight != sd->weight)
		clif_updatestatus(sd,SP_WEIGHT);
	if(b_max_weight != sd->max_weight) {
		clif_updatestatus(sd,SP_MAXWEIGHT);
		pc_checkweighticon(sd);
	}
	for(i=0;i<6;i++)
		if(b_paramb[i] + b_parame[i] != sd->paramb[i] + sd->parame[i])
			clif_updatestatus(sd,SP_STR+i);
	if(b_hit != sd->hit)
		clif_updatestatus(sd,SP_HIT);
	if(b_flee != sd->flee)
		clif_updatestatus(sd,SP_FLEE1);
	if(b_aspd != sd->aspd)
		clif_updatestatus(sd,SP_ASPD);
	if(b_watk != sd->watk || b_base_atk != sd->base_atk)
		clif_updatestatus(sd,SP_ATK1);
	if(b_def != sd->def)
		clif_updatestatus(sd,SP_DEF1);
	if(b_watk2 != sd->watk2)
		clif_updatestatus(sd,SP_ATK2);
	if(b_def2 != sd->def2)
		clif_updatestatus(sd,SP_DEF2);
	if(b_flee2 != sd->flee2)
		clif_updatestatus(sd,SP_FLEE2);
	if(b_critical != sd->critical)
		clif_updatestatus(sd,SP_CRITICAL);
	if(b_matk1 != sd->matk1)
		clif_updatestatus(sd,SP_MATK1);
	if(b_matk2 != sd->matk2)
		clif_updatestatus(sd,SP_MATK2);
	if(b_mdef != sd->mdef)
		clif_updatestatus(sd,SP_MDEF1);
	if(b_mdef2 != sd->mdef2)
		clif_updatestatus(sd,SP_MDEF2);
	if(b_attackrange != sd->attackrange)
		clif_updatestatus(sd,SP_ATTACKRANGE);
	if(b_max_hp != sd->status.max_hp)
		clif_updatestatus(sd,SP_MAXHP);
	if(b_max_sp != sd->status.max_sp)
		clif_updatestatus(sd,SP_MAXSP);
	if(b_hp != sd->status.hp)
		clif_updatestatus(sd,SP_HP);
	if(b_sp != sd->status.sp)
		clif_updatestatus(sd,SP_SP);

/*	if(before.cart_num != before.cart_num || before.cart_max_num != before.cart_max_num ||
		before.cart_weight != before.cart_weight || before.cart_max_weight != before.cart_max_weight )
		clif_updatestatus(sd,SP_CARTINFO);*/

	if(sd->status.hp<sd->status.max_hp>>2 && pc_checkskill(sd,SM_AUTOBERSERK)>0 && sd->sc_data[SC_AUTOBERSERK].timer!=-1 &&
		(sd->sc_data[SC_PROVOKE].timer==-1 || sd->sc_data[SC_PROVOKE].val2==0 ) && !unit_isdead(&sd->bl))
		// �I�[�g�o�[�T�[�N����
		status_change_start(&sd->bl,SC_PROVOKE,10,1,0,0,0,0);

	return 0;
}

/*==========================================
 * �Ώۂ�group��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_group(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class_].group_id;
	//PC PET��0�i���ݒ�)

	return 0;
}
/*==========================================
 * �Ώۂ�Class��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_class(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->class_;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.class_;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return ((struct pet_data *)bl)->class_;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data *)bl)->status.class_;
	else
		return 0;
}
/*==========================================
 * �Ώۂ̕�����Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_dir(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->dir;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->dir;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return ((struct pet_data *)bl)->dir;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data *)bl)->dir;
	else
		return 0;
}
/*==========================================
 * �Ώۂ̃��x����Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_lv(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class_].lv;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.base_level;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return ((struct pet_data *)bl)->msd->pet.level;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data *)bl)->status.base_level;
	else
		return 0;
}

/*==========================================
 * �Ώۂ̎˒���Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_range(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class_].range;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->attackrange;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class_].range;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return 2;//((struct homun_data *)bl)->attackrange;
	else
		return 0;
}
/*==========================================
 * �Ώۂ�HP��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_hp(struct block_list *bl)
{
	nullpo_retr(1, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->hp;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.hp;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data *)bl)->status.hp;
	else
		return 1;
}
/*==========================================
 * �Ώۂ�SP��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_sp(struct block_list *bl)
{
	nullpo_retr(1, bl);

	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.sp;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data *)bl)->status.sp;
	else
		return 0;
}
/*==========================================
 * �Ώۂ�MHP��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_max_hp(struct block_list *bl)
{
	nullpo_retr(1, bl);
	if(bl->type==BL_PC && ((struct map_session_data *)bl))
		return ((struct map_session_data *)bl)->status.max_hp;
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		return ((struct homun_data *)bl)->max_hp;
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int max_hp=1;
		if(bl->type==BL_MOB && ((struct mob_data*)bl)) {
			max_hp = mob_db[((struct mob_data*)bl)->class_].max_hp;
			if(mob_db[((struct mob_data*)bl)->class_].mexp > 0) {
				if(battle_config.mvp_hp_rate != 100) {
					atn_bignumber hp = (atn_bignumber)max_hp * battle_config.mvp_hp_rate / 100;
					max_hp = (hp > 0x7FFFFFFF ? 0x7FFFFFFF : (int)hp);
				}
			}
			else {
				if(battle_config.monster_hp_rate != 100)
					max_hp = (max_hp * battle_config.monster_hp_rate)/100;
			}
		}
		else if(bl->type==BL_PET && ((struct pet_data*)bl)) {
			max_hp = mob_db[((struct pet_data*)bl)->class_].max_hp;
			if(mob_db[((struct pet_data*)bl)->class_].mexp > 0) {
				if(battle_config.mvp_hp_rate != 100) {
					atn_bignumber hp = (atn_bignumber)max_hp * battle_config.mvp_hp_rate / 100;
					max_hp = (hp > 0x7FFFFFFF ? 0x7FFFFFFF : (int)hp);
				}
			}
			else {
				if(battle_config.monster_hp_rate != 100)
					max_hp = (max_hp * battle_config.monster_hp_rate)/100;
			}
		}
		if(sc_data) {
			if(sc_data[SC_APPLEIDUN].timer!=-1)
				max_hp += ((5+sc_data[SC_APPLEIDUN].val1*2+((sc_data[SC_APPLEIDUN].val2+1)>>1)
						+sc_data[SC_APPLEIDUN].val3/10) * max_hp)/100;
		}
		if(max_hp < 1) max_hp = 1;
		return max_hp;
	}

	return 1;
}
/*==========================================
 * �Ώۂ�Str��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_str(struct block_list *bl)
{
	int str=0;
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_MOB && ((struct mob_data *)bl))
		str = mob_db[((struct mob_data *)bl)->class_].str;
	else if(bl->type==BL_PC && ((struct map_session_data *)bl))
		return ((struct map_session_data *)bl)->paramc[0];
	else if(bl->type==BL_PET && ((struct pet_data *)bl))
		str = mob_db[((struct pet_data *)bl)->class_].str;
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		str = ((struct homun_data *)bl)->status.str;

	if(sc_data && bl->type!=BL_HOM) {
		if(sc_data[SC_LOUD].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1 && bl->type != BL_PC)
			str += 4;
		if( sc_data[SC_BLESSING].timer != -1 && bl->type != BL_PC){	// �u���b�V���O
			int race=status_get_race(bl);
			if(battle_check_undead(race,status_get_elem_type(bl)) || race == RCT_DEMON)
				str >>= 1;				// ����/�s��
			else
				str += sc_data[SC_BLESSING].val1;	// ���̑�
		}
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// �g�D���[�T�C�g
			str += 5;
		if(sc_data[SC_CHASEWALK_STR].timer!=-1)
			str += sc_data[SC_CHASEWALK_STR].val1;
	}
	if(str < 0) str = 0;
	return str;
}
/*==========================================
 * �Ώۂ�Agi��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */

int status_get_agi(struct block_list *bl)
{
	int agi=0;
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		agi=mob_db[((struct mob_data *)bl)->class_].agi;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		agi=((struct map_session_data *)bl)->paramc[1];
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		agi=mob_db[((struct pet_data *)bl)->class_].agi;
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		agi = ((struct homun_data *)bl)->agi;

	if(sc_data && bl->type!=BL_HOM) {
		if(sc_data[SC_INCFLEE].timer!=-1 && bl->type != BL_PC) //���x����
			agi *= 3;
		if( sc_data[SC_INCREASEAGI].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1 &&
			bl->type != BL_PC)	// ���x����(PC��pc.c��)
			agi += 2+sc_data[SC_INCREASEAGI].val1;

		if(sc_data[SC_SUITON].timer!=-1 && sc_data[SC_SUITON].val3!=0 && bl->type != BL_PC)//����
			agi += sc_data[SC_SUITON].val3;

		if(sc_data[SC_CONCENTRATE].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1 && bl->type != BL_PC)
			agi += agi*(2+sc_data[SC_CONCENTRATE].val1)/100;

		if(sc_data[SC_DECREASEAGI].timer!=-1)	// ���x����
			agi -= 2+sc_data[SC_DECREASEAGI].val1;

		if(sc_data[SC_QUAGMIRE].timer!=-1 )	// �N�@�O�}�C�A
			agi >>= 1;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// �g�D���[�T�C�g
			agi += 5;
	}
	if(agi < 0) agi = 0;
	return agi;
}
/*==========================================
 * �Ώۂ�Vit��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_vit(struct block_list *bl)
{
	int vit=0;
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		vit=mob_db[((struct mob_data *)bl)->class_].vit;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		vit=((struct map_session_data *)bl)->paramc[2];
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		vit=mob_db[((struct pet_data *)bl)->class_].vit;
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		vit = ((struct homun_data *)bl)->vit;
	if(sc_data && bl->type!=BL_HOM) {
		if(sc_data[SC_STRIPARMOR].timer != -1 && bl->type!=BL_PC)
			vit = vit*60/100;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// �g�D���[�T�C�g
			vit += 5;
	}

	if(vit < 0) vit = 0;
	return vit;
}
/*==========================================
 * �Ώۂ�Int��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_int(struct block_list *bl)
{
	int int_=0;
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		int_=mob_db[((struct mob_data *)bl)->class_].int_;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		int_=((struct map_session_data *)bl)->paramc[3];
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		int_=mob_db[((struct pet_data *)bl)->class_].int_;
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		int_= ((struct homun_data *)bl)->int_;

	if(sc_data && bl->type!=BL_HOM) {
		if( sc_data[SC_BLESSING].timer != -1 && bl->type != BL_PC){	// �u���b�V���O
			int race=status_get_race(bl);
			if(battle_check_undead(race,status_get_elem_type(bl)) || race == RCT_DEMON)
				int_ >>= 1;	// ����/�s��
			else
				int_ += sc_data[SC_BLESSING].val1;	// ���̑�
		}
		if( sc_data[SC_STRIPHELM].timer != -1 && bl->type != BL_PC)
			int_ = int_*90/100;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// �g�D���[�T�C�g
			int_ += 5;
	}
	if(int_ < 0) int_ = 0;
	return int_;
}
/*==========================================
 * �Ώۂ�Dex��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_dex(struct block_list *bl)
{
	int dex=0;
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		dex=mob_db[((struct mob_data *)bl)->class_].dex;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		dex=((struct map_session_data *)bl)->paramc[4];
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		dex=mob_db[((struct pet_data *)bl)->class_].dex;
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		dex = ((struct homun_data *)bl)->dex;

	if(sc_data && bl->type!=BL_HOM) {
		if(sc_data[SC_EXPLOSIONSPIRITS].timer!=-1 && bl->type != BL_PC)
			dex *= 3; //�m�o�b�����g��
		if(sc_data[SC_CONCENTRATE].timer!=-1 && sc_data[SC_QUAGMIRE].timer == -1 && bl->type != BL_PC)
			dex += dex*(2+sc_data[SC_CONCENTRATE].val1)/100;

		if( sc_data[SC_BLESSING].timer != -1 && bl->type != BL_PC){	// �u���b�V���O
			int race=status_get_race(bl);
			if(battle_check_undead(race,status_get_elem_type(bl)) || race == RCT_DEMON)
				dex >>= 1;	// ����/�s��
			else
				dex += sc_data[SC_BLESSING].val1;	// ���̑�
		}

		if(sc_data[SC_QUAGMIRE].timer!=-1 )	// �N�@�O�}�C�A
			dex >>= 1;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// �g�D���[�T�C�g
			dex += 5;

	}
	if(dex < 0) dex = 0;
	return dex;
}
/*==========================================
 * �Ώۂ�Luk��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_luk(struct block_list *bl)
{
	int luk=0;
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		luk=mob_db[((struct mob_data *)bl)->class_].luk;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		luk=((struct map_session_data *)bl)->paramc[5];
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		luk=mob_db[((struct pet_data *)bl)->class_].luk;
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		luk = ((struct homun_data *)bl)->luk;

	if(sc_data && bl->type!=BL_HOM) {
		if(sc_data[SC_GLORIA].timer!=-1 && bl->type != BL_PC)	// �O�����A(PC��pc.c��)
			luk += 30;
		if(sc_data[SC_CURSE].timer!=-1 )		// ��
			luk=0;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC)	// �g�D���[�T�C�g
			luk += 5;
	}
	if(luk < 0) luk = 0;
	return luk;
}

/*==========================================
 * �Ώۂ�Flee��Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_flee(struct block_list *bl)
{
	int flee=1, target_count=1;
	struct status_change *sc_data;

	nullpo_retr(1, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		flee=((struct map_session_data *)bl)->flee;
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		flee = ((struct homun_data *)bl)->flee;
	else
		flee=status_get_agi(bl) + status_get_lv(bl);

	if(sc_data && bl->type!=BL_HOM) {
		if(sc_data[SC_WHISTLE].timer!=-1 && bl->type != BL_PC)
			flee += flee*(sc_data[SC_WHISTLE].val1+sc_data[SC_WHISTLE].val2
					+(sc_data[SC_WHISTLE].val3>>16))/100;
		if(sc_data[SC_BLIND].timer!=-1 && bl->type != BL_PC)
			flee -= flee*25/100;
		if(sc_data[SC_WINDWALK].timer!=-1 && bl->type != BL_PC) // �E�B���h�E�H�[�N
			flee += flee*(sc_data[SC_WINDWALK].val2)/100;
		if(sc_data[SC_SPIDERWEB].timer!=-1 && bl->type != BL_PC) //�X�p�C�_�[�E�F�u
			flee -= flee*50/100;
			//THE SUN
		if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN && bl->type != BL_PC)
			flee = flee*80/100;
		if(sc_data[SC_GATLINGFEVER].timer!=-1 && bl->type != BL_PC)	//�K�g�����O�t�B�[�o�[
			flee = flee*(100-sc_data[SC_GATLINGFEVER].val1*5)/100;
		if(sc_data[SC_ADJUSTMENT].timer!=-1 && bl->type != BL_PC)	//�A�W���X�g�����g
			flee += 30;

	}

	// ��𗦕␳
	if(bl->type!=BL_HOM)
		target_count = unit_counttargeted(bl,battle_config.agi_penalty_count_lv);

	if(battle_config.agi_penalty_type > 0 && target_count >= battle_config.agi_penalty_count) {
		//�y�i���e�B�ݒ���Ώۂ�����
		if(battle_config.agi_penalty_type == 1) {
			//��𗦂�agi_penalty_num%������
			int flee_rate = (target_count-(battle_config.agi_penalty_count-1)) * battle_config.agi_penalty_num;
			flee = flee * (100 - flee_rate) / 100;
		} else if(battle_config.agi_penalty_type == 2) {
			//��𗦂�agi_penalty_num������
			flee -= (target_count - (battle_config.agi_penalty_count - 1))*battle_config.agi_penalty_num;
		}
	}
	// �ΐlMAP�ł̌�������
	if(battle_config.gvg_flee_penalty & 1 && map[bl->m].flag.gvg) {
		flee = flee*(200 - battle_config.gvg_flee_rate)/100;	//���ۂ�GvG��Flee������
	}
	if(battle_config.gvg_flee_penalty & 2 && map[bl->m].flag.pvp) {
		flee = flee*(200 - battle_config.gvg_flee_rate)/100;	//���ۂ�PvP��Flee������
	}
	if(flee < 1) flee = 1;

	return flee;
}
/*==========================================
 * �Ώۂ�Hit��Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_hit(struct block_list *bl)
{
	int hit=1;
	struct status_change *sc_data;

	nullpo_retr(1, bl);
	if (bl->type==BL_PC)
		return ((struct map_session_data *)bl)->hit;
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		return ((struct homun_data *)bl)->hit;
	else
		hit=status_get_dex(bl) + status_get_lv(bl);

	sc_data = status_get_sc_data(bl);
	if (sc_data) {
		if(sc_data[SC_HUMMING].timer!=-1)	//
			hit += hit*(sc_data[SC_HUMMING].val1*2+sc_data[SC_HUMMING].val2
					+sc_data[SC_HUMMING].val3)/100;
		if(sc_data[SC_TRUESIGHT].timer!=-1)		// �g�D���[�T�C�g
			hit += 3*(sc_data[SC_TRUESIGHT].val1);
		if(sc_data[SC_CONCENTRATION].timer!=-1) //�R���Z���g���[�V����
			hit += 10*(sc_data[SC_CONCENTRATION].val1);
			//THE SUN
		if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN && bl->type != BL_PC)
			hit = hit*80/100;
		if(sc_data[SC_ADJUSTMENT].timer!=-1 && bl->type != BL_PC) // �A�W���X�g�����g
			hit -= 30;
		if(sc_data[SC_INCREASING].timer!=-1 && bl->type != BL_PC) // �C���N���[�V���O�A�L���A���V�[
			hit += 20;
	}
	if(hit < 1) hit = 1;
	return hit;
}
/*==========================================
 * �Ώۂ̊��S�����Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_flee2(struct block_list *bl)
{
	int flee2=1;
	struct status_change *sc_data;

	nullpo_retr(1, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl){
		flee2 = status_get_luk(bl) + 10;
		flee2 += ((struct map_session_data *)bl)->flee2 - (((struct map_session_data *)bl)->paramc[5] + 10);
	}
	else
		flee2=status_get_luk(bl)+1;

	if(sc_data) {
		if(sc_data[SC_WHISTLE].timer!=-1 && bl->type != BL_PC)
			flee2 += (sc_data[SC_WHISTLE].val1+sc_data[SC_WHISTLE].val2
					+(sc_data[SC_WHISTLE].val3&0xffff))*10;
	}
	if(flee2 < 1) flee2 = 1;
	return flee2;
}
/*==========================================
 * �Ώۂ̃N���e�B�J����Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_critical(struct block_list *bl)
{
	int critical=1;
	struct status_change *sc_data;

	nullpo_retr(1, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl){
		critical = status_get_luk(bl)*3 + 10;
		critical += ((struct map_session_data *)bl)->critical - ((((struct map_session_data *)bl)->paramc[5]*3) + 10);
	}
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		critical = ((struct homun_data *)bl)->critical;
	else
		critical=status_get_luk(bl)*3 + 1;

	if(sc_data) {
		if(sc_data[SC_FORTUNE].timer!=-1 && bl->type != BL_PC)
			critical += (10+sc_data[SC_FORTUNE].val1+sc_data[SC_FORTUNE].val2
					+sc_data[SC_FORTUNE].val3)*10;
		if(sc_data[SC_EXPLOSIONSPIRITS].timer!=-1 && bl->type != BL_PC)
			critical += sc_data[SC_EXPLOSIONSPIRITS].val2;
		if(sc_data[SC_TRUESIGHT].timer!=-1 && bl->type != BL_PC) //�g�D���[�T�C�g
			critical += 10*sc_data[SC_TRUESIGHT].val1;
	}
	if(critical < 1) critical = 1;
	return critical;
}
/*==========================================
 * base_atk�̎擾
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_baseatk(struct block_list *bl)
{
	struct status_change *sc_data;
	int batk=1;

	nullpo_retr(1, bl);
	sc_data=status_get_sc_data(bl);

	if(bl->type==BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if(sd) {
			batk = sd->base_atk; //�ݒ肳��Ă���base_atk
			batk += sd->weapon_atk[sd->status.weapon];
		}
	}else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		batk = 1;
	else { //����ȊO�Ȃ�
		int str,dstr;
		str = status_get_str(bl); //STR
		dstr = str/10;
		batk = dstr*dstr + str; //base_atk���v�Z����
	}
	if(sc_data) { //��Ԉُ킠��
		if(sc_data[SC_PROVOKE].timer!=-1 && bl->type != BL_PC) //PC�Ńv���{�b�N(SM_PROVOKE)���
			batk = batk*(100+2*sc_data[SC_PROVOKE].val1)/100; //base_atk����
		if(sc_data[SC_CURSE].timer!=-1 ) //����Ă�����
			batk -= batk*25/100; //base_atk��25%����
		if(sc_data[SC_CONCENTRATION].timer!=-1 && bl->type != BL_PC) //�R���Z���g���[�V����
			batk += batk*(5*sc_data[SC_CONCENTRATION].val1)/100;
		if(sc_data[SC_JOINTBEAT].timer != -1 && sc_data[SC_JOINTBEAT].val4 == 4)	//�W���C���g�r�[�g�ō�
			batk -= batk*25/100;
		if(sc_data[SC_MADNESSCANCEL].timer!=-1 && bl->type != BL_PC) //�}�b�h�l�X�L�����Z���[
			batk += 100;
		//�^���b�g���ʂ͂P�����H
		if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_MAGICIAN)
				batk = batk*50/100;
		else if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_DEVIL)
				batk = batk*50/100;
		else if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN)
				batk = batk*80/100;
		//�G�X�N
		if(sc_data[SC_SKE].timer!=-1 && bl->type == BL_MOB)
			batk *= 4;
	}
	if(batk < 1) batk = 1; //base_atk�͍Œ�ł�1
	return batk;
}
/*==========================================
 * �Ώۂ�Atk��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_atk(struct block_list *bl)
{
	struct status_change *sc_data;
	int atk=0;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		atk = ((struct map_session_data*)bl)->watk;
	else if(bl->type==BL_HOM && ((struct homun_data *)bl))
		atk = ((struct homun_data *)bl)->atk-((struct homun_data *)bl)->atk/10;
	else if(bl->type==BL_MOB)
	{
		struct mob_data *md = (struct mob_data *)bl;
		if(md) {
			int guardup_lv = md->guardup_lv;
			atk = mob_db[md->class_].atk1;
			if(guardup_lv>0)
				atk += 1000*guardup_lv;
		}
	}else if(bl->type==BL_PET && (struct pet_data *)bl)
		atk = mob_db[((struct pet_data*)bl)->class_].atk1;

	if(sc_data) {
		if(sc_data[SC_PROVOKE].timer!=-1 && bl->type != BL_PC)
			atk = atk*(100+2*sc_data[SC_PROVOKE].val1)/100;
		if(sc_data[SC_CURSE].timer!=-1 )
			atk -= atk*25/100;
		if(sc_data[SC_CONCENTRATION].timer!=-1 && bl->type != BL_PC) //�R���Z���g���[�V����
			atk += atk*(5*sc_data[SC_CONCENTRATION].val1)/100;
		if(sc_data[SC_EXPLOSIONSPIRITS].timer!=-1 && bl->type != BL_PC)
			atk *= 3; //�m�o�b�����g��
		if(sc_data[SC_STRIPWEAPON].timer!=-1 && bl->type != BL_PC)
			atk -= atk*25/100;
		if(sc_data[SC_DISARM].timer!=-1 && bl->type != BL_PC)		//�f�B�X�A�[��
			atk -= atk*25/100;
		if(sc_data[SC_MADNESSCANCEL].timer!=-1 && bl->type != BL_PC)	//�}�b�h�l�X�L�����Z���[
			atk += 100;

		if(bl->type != BL_PC && sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_MAGICIAN)
				atk = atk*50/100;
		else if(bl->type != BL_PC && sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_DEVIL)
				atk = atk*50/100;
		else if(bl->type != BL_PC && sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN)
				atk = atk*80/100;

		//�G�X�N
		if(sc_data[SC_SKE].timer!=-1 && bl->type == BL_MOB)
			atk *=4;
	}
	if(atk < 0) atk = 0;
	return atk;
}
/*==========================================
 * �Ώۂ̍���Atk��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_atk_(struct block_list *bl)
{
	struct map_session_data *sd = NULL;

	nullpo_retr(0, bl);

	if(bl->type==BL_PC && (sd = (struct map_session_data *)bl)) {
		int atk = sd->watk_;

		if(sd->sc_data[SC_CURSE].timer != -1)
			atk -= atk*25/100;
		return atk;
	}
	return 0;
}
/*==========================================
 * �Ώۂ�Atk2��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_atk2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data*)bl)->watk2;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data *)bl)->atk;
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int atk2=0;
		if(bl->type==BL_MOB){
			struct mob_data *md = (struct mob_data *)bl;
			if(md) {
				int guardup_lv = md->guardup_lv;
				atk2 = mob_db[md->class_].atk2;
				if(guardup_lv>0)
					atk2 += 1000*guardup_lv;
			}
		}
		else if(bl->type==BL_PET && (struct pet_data *)bl)
			atk2 = mob_db[((struct pet_data*)bl)->class_].atk2;
		if(sc_data) {
			if( sc_data[SC_IMPOSITIO].timer!=-1)
				atk2 += sc_data[SC_IMPOSITIO].val1*5;
			if( sc_data[SC_PROVOKE].timer!=-1 )
				atk2 = atk2*(100+2*sc_data[SC_PROVOKE].val1)/100;
			if( sc_data[SC_CURSE].timer!=-1 )
				atk2 -= atk2*25/100;
			if(sc_data[SC_DRUMBATTLE].timer!=-1)
				atk2 += sc_data[SC_DRUMBATTLE].val2;
			if(sc_data[SC_NIBELUNGEN].timer!=-1 && (status_get_element(bl)/10) >= 8 )
				atk2 += sc_data[SC_NIBELUNGEN].val2;
			if(sc_data[SC_STRIPWEAPON].timer!=-1)
				atk2 -= atk2*10/100;
			if(sc_data[SC_CONCENTRATION].timer!=-1) //�R���Z���g���[�V����
				atk2 += atk2*(5*sc_data[SC_CONCENTRATION].val1)/100;
			if(sc_data[SC_EXPLOSIONSPIRITS].timer!=-1 && bl->type != BL_PC)
				atk2 *= 3; //�m�o�b�����g��
			if(sc_data[SC_DISARM].timer!=-1 && bl->type != BL_PC)		//�f�B�X�A�[��
				atk2 -= atk2*25/100;
			if(sc_data[SC_MADNESSCANCEL].timer!=-1 && bl->type != BL_PC)	//�}�b�h�l�X�L�����Z���[
				atk2 += 100;

			//�^���b�g����
			if(bl->type != BL_PC && sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_MAGICIAN)
					atk2 = atk2*50/100;
			else if(bl->type != BL_PC && sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_DEVIL)
					atk2 = atk2*50/100;
			else if(bl->type != BL_PC && sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN)
					atk2 = atk2*80/100;
			//�G�X�N
			if(sc_data[SC_SKE].timer!=-1 && bl->type == BL_MOB)
				atk2 *= 4;

		}
		if(atk2 < 0) atk2 = 0;
		return atk2;
	}
	return 0;
}
/*==========================================
 * �Ώۂ̍���Atk2��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_atk_2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data*)bl)->watk_2;
	else
		return 0;
}
/*==========================================
 * �Ώۂ�MAtk1��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_matk1(struct block_list *bl)
{
	struct status_change *sc_data;
	int matk1,int_;
	nullpo_retr(0, bl);

	if (bl->type==BL_PC)
		return ((struct map_session_data *)bl)->matk1;
	else if (bl->type==BL_HOM)
		return ((struct homun_data *)bl)->matk-((struct homun_data *)bl)->matk/10;
	else if (bl->type!=BL_PET && bl->type!=BL_MOB)
		return 0;

	int_=status_get_int(bl);
	matk1 = int_+(int_/5)*(int_/5);
	
	//MOB��max_sp�l��MATK�␳�l�ŏ����鎞
	if(battle_config.mob_take_over_sp == 1){	//battleconf
		if(bl->type == BL_MOB){					//mobonly
			int b_class = status_get_class(bl);		//����maxsp�擾�����̈׃�������
			if(mob_db[b_class].max_sp > 0){		//1�ȏ�̎��̂�
				matk1 = matk1 * (mob_db[b_class].max_sp/100);
			}
		}
	}
	
	sc_data = status_get_sc_data(bl);
	if (sc_data) {
		if (sc_data[SC_MINDBREAKER].timer!=-1)
			matk1 += (matk1*20*sc_data[SC_MINDBREAKER].val1)/100;

		if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_STRENGTH)
			matk1 = matk1*50/100;
		else if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_DEVIL)
			matk1 = matk1*50/100;
		else if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN)
			matk1 = matk1*80/100;
	}
	return matk1;
}
/*==========================================
 * �Ώۂ�MAtk2��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_matk2(struct block_list *bl)
{
	struct status_change *sc_data;
	int matk2,int_;
	nullpo_retr(0, bl);

	if (bl->type==BL_PC)
		return ((struct map_session_data *)bl)->matk2;
	else if (bl->type==BL_HOM)
		return ((struct homun_data *)bl)->matk;
	else if (bl->type!=BL_PET && bl->type!=BL_MOB)
		return 0;

	int_=status_get_int(bl);
	matk2 = int_+(int_/7)*(int_/7);

	//MOB��max_sp�l��MATK�␳�l�ŏ����鎞
	if(battle_config.mob_take_over_sp == 1){	//battleconf
		if(bl->type == BL_MOB){					//mobonly
			int b_class = status_get_class(bl);		//����maxsp�擾�����̈׃�������
			if(mob_db[b_class].max_sp > 0){		//1�ȏ�̎��̂�
				matk2 = matk2 * (mob_db[b_class].max_sp/100);
			}
		}
	}

	sc_data = status_get_sc_data(bl);
	if (sc_data) {
		if (sc_data[SC_MINDBREAKER].timer!=-1)
			matk2 += (matk2*20*sc_data[SC_MINDBREAKER].val1)/100;

		if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_STRENGTH)
			matk2 = matk2*50/100;
		else if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_DEVIL)
			matk2 = matk2*50/100;
		else if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN)
			matk2 = matk2*80/100;
	}
	return matk2;
}
/*==========================================
 * �Ώۂ�Def��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_def(struct block_list *bl)
{
	struct unit_data *ud= NULL;
	struct status_change *sc_data;
	int def;

	nullpo_retr(0, bl);

	ud = unit_bl2ud(bl);
	sc_data=status_get_sc_data(bl);

	if(bl->type==BL_PC && (struct map_session_data *)bl) {
		def = ((struct map_session_data *)bl)->def;
	}
	else if(bl->type==BL_MOB && (struct mob_data *)bl) {
		def = mob_db[((struct mob_data *)bl)->class_].def;
	}
	else if(bl->type==BL_PET && (struct pet_data *)bl) {
		def = mob_db[((struct pet_data *)bl)->class_].def;
	}
	else {
		def = 0;
	}

	if(def < 1000000) {
		if(sc_data) {
			//�L�[�s���O����DEF100
			if( sc_data[SC_KEEPING].timer!=-1)
				def *= 2;
			//�v���{�b�N���͌��Z
			if( sc_data[SC_PROVOKE].timer!=-1 && bl->type != BL_PC)
				def = (def*(100 - 6*sc_data[SC_PROVOKE].val1)+50)/100;
			//�푾�ۂ̋������͉��Z
			if( sc_data[SC_DRUMBATTLE].timer!=-1 && bl->type != BL_PC)
				def += sc_data[SC_DRUMBATTLE].val3;
			//�łɂ������Ă��鎞�͌��Z
			if(sc_data[SC_POISON].timer!=-1 && bl->type != BL_PC)
				def = def*75/100;
			//�X�g���b�v�V�[���h���͌��Z
			if(sc_data[SC_STRIPSHIELD].timer!=-1 && bl->type != BL_PC)
				def = def*85/100;
			//�V�O�i���N���V�X���͌��Z
			if(sc_data[SC_SIGNUMCRUCIS].timer!=-1 && bl->type != BL_PC)
				def = def * (100 - sc_data[SC_SIGNUMCRUCIS].val2)/100;
			//�i���̍��׎���PC�ȊODEF��0�ɂȂ�
			if(sc_data[SC_ETERNALCHAOS].timer!=-1 && bl->type != BL_PC)
				def = 0;
			//�����A�Ή����͉E�V�t�g
			if(sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))
				def >>= 1;
			//�R���Z���g���[�V�������͌��Z
			if( sc_data[SC_CONCENTRATION].timer!=-1 && bl->type != BL_PC)
				def = (def*(100 - 5*sc_data[SC_CONCENTRATION].val1))/100;
			//NPC�f�B�t�F���_�[
			if(sc_data[SC_NPC_DEFENDER].timer!=-1 && bl->type != BL_PC)
				def += 100;
			//THE SUN
			if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN && bl->type != BL_PC)
				def = def*80/100;
			if(sc_data[SC_FLING].timer!=-1 && bl->type != BL_PC)			// �t���C���O
				def = def * (100 - 5*sc_data[SC_FLING].val1)/100;
			//�G�X�N
			if(sc_data[SC_SKE].timer!=-1 && bl->type == BL_MOB)
				def = def/2;
		}
		//�r�����͉r�������Z���Ɋ�Â��Č��Z
		if(ud && ud->skilltimer != -1) {
			int def_rate = skill_get_castdef(ud->skillid);
			if(def_rate != 0)
				def = (def * (100 - def_rate))/100;
		}
	}
	if(def < 0) def = 0;
	return def;
}
/*==========================================
 * �Ώۂ�MDef��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_mdef(struct block_list *bl)
{
	struct status_change *sc_data;
	int mdef=0;

	nullpo_retr(0, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		mdef = ((struct map_session_data *)bl)->mdef;
	else if(bl->type==BL_MOB && (struct mob_data *)bl)
		mdef = mob_db[((struct mob_data *)bl)->class_].mdef;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		mdef = ((struct homun_data *)bl)->mdef;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		mdef = mob_db[((struct pet_data *)bl)->class_].mdef;

	if(mdef < 1000000) {
		if(sc_data) {
			//�o���A�[��Ԏ���MDEF100
			if(sc_data[SC_BARRIER].timer != -1)
				mdef += 100;
			//�����A�Ή�����1.25�{
			if(sc_data[SC_FREEZE].timer != -1 || (sc_data[SC_STONE].timer != -1 && sc_data[SC_STONE].val2 == 0))
				mdef = mdef*125/100;
			if (bl->type!=BL_PC && sc_data[SC_MINDBREAKER].timer!=-1)
				mdef -= (mdef*12*sc_data[SC_MINDBREAKER].val1)/100;
		}
	}
	if(mdef < 0) mdef = 0;
	return mdef;
}
/*==========================================
 * �Ώۂ�Def2��Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 *------------------------------------------
 */
int status_get_def2(struct block_list *bl)
{
	struct status_change *sc_data;
	int def2=1;

	nullpo_retr(1, bl);
	sc_data=status_get_sc_data(bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		def2 = ((struct map_session_data *)bl)->def2;
	else if(bl->type==BL_MOB && (struct mob_data *)bl)
		def2 = mob_db[((struct mob_data *)bl)->class_].vit;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		def2 = mob_db[((struct pet_data *)bl)->class_].vit;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		def2 = ((struct homun_data *)bl)->def;

	if(sc_data) {
		if( sc_data[SC_ANGELUS].timer!=-1 && bl->type != BL_PC)
			def2 = def2*(110+5*sc_data[SC_ANGELUS].val1)/100;
		if( sc_data[SC_PROVOKE].timer!=-1 && bl->type != BL_PC)
			def2 = (def2*(100 - 6*sc_data[SC_PROVOKE].val1)+50)/100;
		if(sc_data[SC_POISON].timer!=-1 && bl->type != BL_PC)
			def2 = def2*75/100;
		//�R���Z���g���[�V�������͌��Z
		if( sc_data[SC_CONCENTRATION].timer!=-1 && bl->type != BL_PC)
			def2 = def2*(100 - 5*sc_data[SC_CONCENTRATION].val1)/100;
		//�W���C���g�r�[�g���Ȃ猸�Z
		if(sc_data[SC_JOINTBEAT].timer != -1) {
			if(sc_data[SC_JOINTBEAT].val4 == 3)	//��
				def2 -= def2*50/100;
			if(sc_data[SC_JOINTBEAT].val4 == 4)	//��
				def2 -= def2*25/100;
		}
		//�i���̍��׎���DEF2��0�ɂȂ�
		if(sc_data[SC_ETERNALCHAOS].timer!=-1)
			def2 = 0;
		//THE SUN
		if(sc_data[SC_TAROTCARD].timer!=-1 && sc_data[SC_TAROTCARD].val4 == SC_THE_SUN && bl->type != BL_PC)
			def2 = def2*80/100;
		//�G�X�J
		if(sc_data[SC_SKA].timer!=-1 && bl->type == BL_MOB)
			def2 += 90;
	}
	if(def2 < 1) def2 = 1;
	return def2;
}
/*==========================================
 * �Ώۂ�MDef2��Ԃ�(�ėp)
 * �߂�͐�����0�ȏ�
 *------------------------------------------
 */
int status_get_mdef2(struct block_list *bl)
{
	struct status_change *sc_data;
	int mdef2=0;

	nullpo_retr(0, bl);

	sc_data=status_get_sc_data(bl);

	if(bl->type==BL_MOB && (struct mob_data *)bl)
		mdef2 = mob_db[((struct mob_data *)bl)->class_].int_ + (mob_db[((struct mob_data *)bl)->class_].vit>>1);
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		mdef2 = ((struct map_session_data *)bl)->mdef2 + (((struct map_session_data *)bl)->paramc[2]>>1);
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		mdef2 = mob_db[((struct pet_data *)bl)->class_].int_ + (mob_db[((struct pet_data *)bl)->class_].vit>>1);
	else if (bl->type==BL_HOM && (struct homun_data *)bl)
		mdef2 = ((struct homun_data *)bl)->mdef;

	if(sc_data) {
		//�G�X�J
		if(sc_data[SC_SKA].timer!=-1 && bl->type == BL_MOB)
			mdef2 = 90;
	}
	return mdef2;
}
/*==========================================
 * �Ώۂ�Speed(�ړ����x)��Ԃ�(�ėp)
 * �߂�͐�����1�ȏ�
 * Speed�͏������ق����ړ����x������
 *------------------------------------------
 */
int status_get_speed(struct block_list *bl)
{
	nullpo_retr(1000, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->speed;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
	{
		if(battle_config.homun_speed_is_same_as_pc)
			return ((struct homun_data *)bl)->msd->speed;
		else
			return ((struct homun_data *)bl)->speed;
	}else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int speed = 1000;
		if(bl->type==BL_MOB && (struct mob_data *)bl)
//			speed = mob_db[((struct mob_data *)bl)->class_].speed;
			speed = ((struct mob_data *)bl)->speed;
		else if(bl->type==BL_PET && (struct pet_data *)bl)
			speed = ((struct pet_data *)bl)->speed;

		if(sc_data) {
			//��������35%���Z
			if(sc_data[SC_STEELBODY].timer!=-1)
				speed = speed*135/100;
			//���x��������25%���Z(�������͖���)
			else if(sc_data[SC_INCREASEAGI].timer!=-1)
				speed -= speed*25/100;
			//�E�B���h�E�H�[�N����Lv*2%���Z(���x�������͖���)
			else if(sc_data[SC_WINDWALK].timer!=-1)
				speed -= (speed*(sc_data[SC_WINDWALK].val1*2))/100;
			//���x��������50%���Z
			if(sc_data[SC_INCFLEE].timer!=-1 && bl->type != BL_PC)
				speed -= speed*50/100;
			//���x��������33�`50%���Z
			if(sc_data[SC_DECREASEAGI].timer!=-1 && sc_data[SC_DEFENDER].timer == -1)//�i�f�B�t�F���_�[���͉��Z�����j
				speed = speed*((sc_data[SC_DECREASEAGI].val1>5)?150:133)/100;
			//�N�@�O�}�C�A����1/3���Z
			if(sc_data[SC_QUAGMIRE].timer!=-1)
				speed = speed*4/3;
			//����Y��Ȃ��Łc���͉��Z
			if(sc_data[SC_DONTFORGETME].timer!=-1)
				speed = speed*(100+sc_data[SC_DONTFORGETME].val1*2 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3&0xffff))/100;
			//�f�B�t�F���_�[���͉��Z
			if(sc_data[SC_DEFENDER].timer!=-1)
				speed = (speed * (155 - sc_data[SC_DEFENDER].val1*5)) / 100;
			// �x�蒆��4�{�x��
			if (sc_data[SC_DANCING].timer != -1)
				speed *= 4;
			// �W���C���g�r�[�g���Ȃ���Z
			if(sc_data[SC_JOINTBEAT].timer != -1) {
				if(sc_data[SC_JOINTBEAT].val4 == 0)	//����
					speed += speed*50/100;
				if(sc_data[SC_JOINTBEAT].val4 == 2)	//�G
					speed += speed*30/100;
			}
			//�G�X�E��4�{�x��
			if(sc_data[SC_SWOO].timer!=-1 )
				speed*=4;
			//�O���r�e�[�V�����t�B�[���h
			if(sc_data[SC_GRAVITATION].timer!=-1&&battle_config.enemy_gravitation_type)
				speed = speed*(100+sc_data[SC_GRAVITATION].val1*5)/100;
			//�􂢎���450���Z�i�f�B�t�F���_�[���͑��x�ቺ�����j
			if(sc_data[SC_CURSE].timer!=-1){
				if(sc_data[SC_DEFENDER].timer != -1)
					speed += 0;
				else
					speed += 450;
			}
			if(sc_data[SC_SUITON].timer!=-1 && sc_data[SC_SUITON].val4==1)//����
				speed *= 2;
			//�K�g�����O�t�B�[�o�[
			if(sc_data[SC_GATLINGFEVER].timer!=-1 )
				speed = speed*(100+sc_data[SC_GATLINGFEVER].val1*5)/100;
//			if(sc_data[SC_CURSE].timer!=-1)
//				speed = speed + 450;
		}
		if(speed < 1) speed = 1;
		return speed;
	}

	return 1000;
}
/*==========================================
 * �Ώۂ�aDelay(�U�����f�B���C)��Ԃ�(�ėp)
 * aDelay�͏������ق����U�����x������
 *------------------------------------------
 */
int status_get_adelay(struct block_list *bl)
{
	nullpo_retr(4000, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return (((struct map_session_data *)bl)->aspd<<1);
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int adelay=4000,aspd_rate = 100,i;

		if(bl->type==BL_MOB && (struct mob_data *)bl)
		{
			int guardup_lv = ((struct mob_data*)bl)->guardup_lv;
			adelay = mob_db[((struct mob_data *)bl)->class_].adelay;

			if(guardup_lv>0)
				aspd_rate -= 5 + 5*guardup_lv;
		}
		else if(bl->type==BL_PET && (struct pet_data *)bl)
			adelay = mob_db[((struct pet_data *)bl)->class_].adelay;
		else if(bl->type==BL_HOM && (struct homun_data *)bl)
			adelay = (((struct homun_data *)bl)->aspd<<1);

		if(sc_data) {
			//�c�[�n���h�N�C�b�P���g�p���ŃN�@�O�}�C�A�ł�����Y��Ȃ��Łc�ł��Ȃ�����3�����Z
			if(sc_data[SC_TWOHANDQUICKEN].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 2HQ
				aspd_rate -= 30;
			//�����n���h�N�C�b�P���g�p���ŃN�@�O�}�C�A�ł�����Y��Ȃ��Łc�ł��Ȃ�����3�����Z
			if(sc_data[SC_ONEHAND].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 1HQ
				aspd_rate -= 30;
			//�A�h���i�������b�V���g�p���Ńc�[�n���h�N�C�b�P���ł��N�@�O�}�C�A�ł�����Y��Ȃ��Łc�ł��Ȃ�����
			if(sc_data[SC_ADRENALINE2].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// �A�h���i�������b�V��
				//�g�p�҂ƃp�[�e�B�����o�[�Ŋi�����o��ݒ�łȂ����3�����Z
				if(sc_data[SC_ADRENALINE2].val2 || !battle_config.party_skill_penalty)
					aspd_rate -= 30;
				//�����łȂ����2.5�����Z
				else
					aspd_rate -= 25;
			}else if(sc_data[SC_ADRENALINE].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// �A�h���i�������b�V��
				//�g�p�҂ƃp�[�e�B�����o�[�Ŋi�����o��ݒ�łȂ����3�����Z
				if(sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
					aspd_rate -= 30;
				//�����łȂ����2.5�����Z
				else
					aspd_rate -= 25;
			}
			//�X�s�A�N�B�b�P�����͌��Z
			if(sc_data[SC_SPEARSQUICKEN].timer != -1 && sc_data[SC_ADRENALINE].timer == -1 && sc_data[SC_ADRENALINE2].timer == -1 &&
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer == -1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// �X�s�A�N�B�b�P��
				aspd_rate -= sc_data[SC_SPEARSQUICKEN].val2;
			//�[���̃A�T�V���N���X���͌��Z
			if(sc_data[SC_ASSNCROS].timer!=-1 && // �[�z�̃A�T�V���N���X
				sc_data[SC_TWOHANDQUICKEN].timer==-1 && sc_data[SC_ONEHAND].timer == -1 &&
				sc_data[SC_ADRENALINE].timer==-1 && sc_data[SC_ADRENALINE2].timer==-1 && sc_data[SC_SPEARSQUICKEN].timer==-1 &&
				sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sc_data[SC_ASSNCROS].val1+sc_data[SC_ASSNCROS].val2+sc_data[SC_ASSNCROS].val3;
			//����Y��Ȃ��Łc���͉��Z
			if(sc_data[SC_DONTFORGETME].timer!=-1)		// ����Y��Ȃ���
				aspd_rate += sc_data[SC_DONTFORGETME].val1*3 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3>>16);
			//������25%���Z
			if(sc_data[SC_STEELBODY].timer!=-1)	// ����
				aspd_rate += 25;
			//�Ŗ�̕r�g�p���͌��Z
			if(	sc_data[SC_POISONPOTION].timer!=-1)
				aspd_rate -= 25;
			//�����|�[�V�����g�p���͌��Z
			if(	sc_data[i=SC_SPEEDPOTION2].timer!=-1 || sc_data[i=SC_SPEEDPOTION1].timer!=-1 || sc_data[i=SC_SPEEDPOTION0].timer!=-1)
				aspd_rate -= sc_data[i].val2;
			//�f�B�t�F���_�[���͉��Zaspd_rate�ɕύX&�}�X�^�[�����Z0��
			if(sc_data[SC_DEFENDER].timer != -1)
				aspd_rate += (25 - sc_data[SC_DEFENDER].val1*5);
			//�W���C���g�r�[�g���Ȃ���Z
			if(sc_data[SC_JOINTBEAT].timer != 1) {
				if(sc_data[SC_JOINTBEAT].val4 == 1)	//���
					aspd_rate += aspd_rate*25/100;
				if(sc_data[SC_JOINTBEAT].val4 == 2)	//�G
					aspd_rate += aspd_rate*10/100;
			}
			//�O���r�e�[�V����
			if(sc_data[SC_GRAVITATION].timer!=-1)
				aspd_rate += sc_data[SC_GRAVITATION].val1*5;
			//�K�g�����O�t�B�[�o�[
			if(sc_data[SC_GATLINGFEVER].timer!=-1)
				aspd_rate -= sc_data[SC_GATLINGFEVER].val1*2;
		}

		if(aspd_rate != 100)
			adelay = adelay*aspd_rate/100;
		if(adelay < battle_config.monster_max_aspd<<1) adelay = battle_config.monster_max_aspd<<1;
		return adelay;
	}
	return 4000;
}
/*==========================================
 * �Ώۂ�amotion��Ԃ�(�ėp)
 *------------------------------------------
 */
int status_get_amotion(struct block_list *bl)
{
	nullpo_retr(2000, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->amotion;
	else {
		struct status_change *sc_data=status_get_sc_data(bl);
		int amotion=2000,aspd_rate = 100,i;

		if(bl->type==BL_MOB && (struct mob_data *)bl)
		{
			int guardup_lv = ((struct mob_data*)bl)->guardup_lv;
			amotion = mob_db[((struct mob_data *)bl)->class_].amotion;
			if(guardup_lv>0)
				aspd_rate -= 5 + 5*guardup_lv;
		}else if(bl->type==BL_PET && (struct pet_data *)bl)
			amotion = mob_db[((struct pet_data *)bl)->class_].amotion;
		else if(bl->type==BL_HOM && (struct homun_data *)bl && ((struct homun_data *)bl)->msd)
			amotion = ((struct homun_data *)bl)->aspd;

		if(sc_data) {
			if(sc_data[SC_TWOHANDQUICKEN].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 2HQ
				aspd_rate -= 30;
			if(sc_data[SC_ONEHAND].timer != -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// 1HQ
				aspd_rate -= 30;

			if(sc_data[SC_ADRENALINE2].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 &&
				sc_data[SC_ONEHAND].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// �t���A�h���i�������b�V
				if(sc_data[SC_ADRENALINE2].val2 || !battle_config.party_skill_penalty)
					aspd_rate -= 30;
				else
					aspd_rate -= 25;
			}else if(sc_data[SC_ADRENALINE].timer != -1 && sc_data[SC_TWOHANDQUICKEN].timer == -1 &&
				sc_data[SC_ONEHAND].timer == -1 && sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1) {	// �A�h���i�������b�V��
				if(sc_data[SC_ADRENALINE].val2 || !battle_config.party_skill_penalty)
					aspd_rate -= 30;
				else
					aspd_rate -= 25;
			}
			if(sc_data[SC_SPEARSQUICKEN].timer != -1 && sc_data[SC_ADRENALINE].timer == -1 && sc_data[SC_ADRENALINE2].timer == -1 &&
				sc_data[SC_TWOHANDQUICKEN].timer == -1 && sc_data[SC_ONEHAND].timer==-1 &&
				sc_data[SC_QUAGMIRE].timer == -1 && sc_data[SC_DONTFORGETME].timer == -1)	// �X�s�A�N�B�b�P��
				aspd_rate -= sc_data[SC_SPEARSQUICKEN].val2;
			if(sc_data[SC_ASSNCROS].timer!=-1 && // �[�z�̃A�T�V���N���X
				sc_data[SC_TWOHANDQUICKEN].timer==-1 && sc_data[SC_ONEHAND].timer==-1 &&
				sc_data[SC_ADRENALINE].timer==-1 && sc_data[SC_ADRENALINE2].timer==-1 && sc_data[SC_SPEARSQUICKEN].timer==-1 &&
				sc_data[SC_DONTFORGETME].timer == -1)
				aspd_rate -= 5+sc_data[SC_ASSNCROS].val1+sc_data[SC_ASSNCROS].val2+sc_data[SC_ASSNCROS].val3;
			if(sc_data[SC_DONTFORGETME].timer!=-1)		// ����Y��Ȃ���
				aspd_rate += sc_data[SC_DONTFORGETME].val1*3 + sc_data[SC_DONTFORGETME].val2 + (sc_data[SC_DONTFORGETME].val3>>16);
			if(	sc_data[SC_POISONPOTION].timer!=-1)
				aspd_rate -= 25;
			if(sc_data[SC_STEELBODY].timer!=-1)	// ����
				aspd_rate += 25;
			if(	sc_data[i=SC_SPEEDPOTION2].timer!=-1 || sc_data[i=SC_SPEEDPOTION1].timer!=-1 || sc_data[i=SC_SPEEDPOTION0].timer!=-1)
				aspd_rate -= sc_data[i].val2;
			//�f�B�t�F���_�[���͉��ZASPD�ɕύX&�}�X�^�[�����Z0��
			if(sc_data[SC_DEFENDER].timer != -1)
				aspd_rate += (25 - sc_data[SC_DEFENDER].val1*5);
			//�W���C���g�r�[�g���Ȃ���Z
			if(sc_data[SC_JOINTBEAT].timer != 1) {
				if(sc_data[SC_JOINTBEAT].val4 == 1)	//���
					aspd_rate += aspd_rate*25/100;
				if(sc_data[SC_JOINTBEAT].val4 == 2)	//�G
					aspd_rate += aspd_rate*10/100;
			}
			if(sc_data[SC_GRAVITATION].timer!=-1)
				aspd_rate += sc_data[SC_GRAVITATION].val1*5;
		}
		if(aspd_rate != 100)
			amotion = amotion*aspd_rate/100;
		if(amotion < battle_config.monster_max_aspd) amotion = battle_config.monster_max_aspd;
		return amotion;
	}
	return 2000;
}
/*==========================================
 * �Ώۂ�dmotion��Ԃ�(�ėp)
 *------------------------------------------
 */
int status_get_dmotion(struct block_list *bl)
{
	int ret = 2000;

	nullpo_retr(0, bl);

	if(bl->type==BL_MOB && (struct mob_data *)bl){
		ret=mob_db[((struct mob_data *)bl)->class_].dmotion;
		if(battle_config.monster_damage_delay_rate != 100)
			ret = ret*battle_config.monster_damage_delay_rate/100;
	}
	else if(bl->type==BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if(sd) {
			if(sd->sc_data[SC_ENDURE].timer != -1 && (!map[sd->bl.m].flag.gvg || sd->special_state.infinite_endure)) {
				ret = 0;
			} else {
				ret = sd->dmotion;
				if(battle_config.pc_damage_delay_rate != 100)
					ret = ret*battle_config.pc_damage_delay_rate/100;
			}
		}
	}
	else if(bl->type==BL_HOM && (struct homun_data *)bl && ((struct homun_data *)bl)->msd){
		ret = 800 - ((struct homun_data *)bl)->status.agi*4;
		if( ret < 400 )
			ret = 400;
	}
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		ret=mob_db[((struct pet_data *)bl)->class_].dmotion;

	return ret;
}
/*==========================================
 * �Ώۂ̑�����Ԃ�(�ėp)
 *------------------------------------------
 */
int status_get_element(struct block_list *bl)
{
	int ret = 20;
	struct status_change *sc_data;

	nullpo_retr(ret, bl);

	sc_data = status_get_sc_data(bl);
	if(sc_data) {
		if( sc_data[SC_BENEDICTIO].timer!=-1 )	// ���̍~��
			ret=20 + ELE_HOLY;
		if( sc_data[SC_ELEMENTWATER].timer!=-1 )	// ��
			ret=20*sc_data[SC_ELEMENTWATER].val1 + ELE_WATER;
		if( sc_data[SC_ELEMENTGROUND].timer!=-1 )	// �y
			ret=20*sc_data[SC_ELEMENTGROUND].val1 + ELE_EARTH;
		if( sc_data[SC_ELEMENTFIRE].timer!=-1 )		// ��
			ret=20*sc_data[SC_ELEMENTFIRE].val1 + ELE_FIRE;
		if( sc_data[SC_ELEMENTWIND].timer!=-1 )		// ��
			ret=20*sc_data[SC_ELEMENTWIND].val1 + ELE_WIND;
		if( sc_data[SC_ELEMENTPOISON].timer!=-1 )	// ��
			ret=20*sc_data[SC_ELEMENTPOISON].val1 + ELE_POISON;
		if( sc_data[SC_ELEMENTHOLY].timer!=-1 )	// ��
			ret=20*sc_data[SC_ELEMENTHOLY].val1 + ELE_HOLY;
		if( sc_data[SC_ELEMENTDARK].timer!=-1 )		// ��
			ret=20*sc_data[SC_ELEMENTDARK].val1 + ELE_DARK;
		if( sc_data[SC_ELEMENTELEKINESIS].timer!=-1 )	// �O
			ret=20*sc_data[SC_ELEMENTELEKINESIS].val1 + ELE_GHOST;
		if( sc_data[SC_ELEMENTUNDEAD].timer!=-1 )	// �s��
			ret=20*sc_data[SC_ELEMENTUNDEAD].val1 + ELE_UNDEAD;
		if( sc_data[SC_FREEZE].timer!=-1 )	// ����
			ret=20 + ELE_WATER;
		if( sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
			ret=20 + ELE_EARTH;

		if(ret != 20)
			return ret;
	}

	if(bl->type==BL_MOB && (struct mob_data *)bl)	// 10�̈ʁ�Lv*2�A�P�̈ʁ�����
		ret=((struct mob_data *)bl)->def_ele;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		ret=20+((struct map_session_data *)bl)->def_ele;	// �h�䑮��Lv1
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		ret = mob_db[((struct pet_data *)bl)->class_].element;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		ret = homun_db[((struct homun_data *)bl)->status.class_-HOM_ID].element;

	return ret;
}
/*==========================================
 * �Ώۂ̍U��������Ԃ�(�ėp)
 *------------------------------------------
 */
int status_get_attack_element(struct block_list *bl)
{
	int ret = 0;
	struct status_change *sc_data = NULL;

	nullpo_retr(0, bl);

	sc_data = status_get_sc_data(bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		ret = ELE_NEUTRAL;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		ret=((struct map_session_data *)bl)->atk_ele;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		ret = ELE_NEUTRAL;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		ret = ELE_NONE;// ������

	if(sc_data) {
		if( sc_data[SC_FROSTWEAPON].timer!=-1)		// �t���X�g�E�F�|��
			ret = ELE_WATER;
		if( sc_data[SC_SEISMICWEAPON].timer!=-1)	// �T�C�Y�~�b�N�E�F�|��
			ret = ELE_EARTH;
		if( sc_data[SC_FLAMELAUNCHER].timer!=-1)	// �t���[�������`���[
			ret = ELE_FIRE;
		if( sc_data[SC_LIGHTNINGLOADER].timer!=-1)	// ���C�g�j���O���[�_�[
			ret = ELE_WIND;
		if( sc_data[SC_ENCPOISON].timer!=-1)		// �G���`�����g�|�C�Y��
			ret = ELE_POISON;
		if( sc_data[SC_ASPERSIO].timer!=-1)			// �A�X�y���V�I
			ret = ELE_HOLY;
		if( sc_data[SC_DARKELEMENT].timer!=-1)		// �ő���
			ret = ELE_DARK;
		if( sc_data[SC_ATTENELEMENT].timer!=-1)		// �O����
			ret = ELE_GHOST;
		if( sc_data[SC_UNDEADELEMENT].timer!=-1)	// �s������
			ret = ELE_UNDEAD;
	}
	return ret;
}
/*==========================================
 * �Ώۂ̍U�������i����j��Ԃ�(�ėp)
 *------------------------------------------
 */
int status_get_attack_element2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl) {
		int ret = ((struct map_session_data *)bl)->atk_ele_;
		struct status_change *sc_data = status_get_sc_data(bl);

		if(sc_data) {

			if( sc_data[SC_FROSTWEAPON].timer!=-1)		// �t���X�g�E�F�|��
				ret = ELE_WATER;
			if( sc_data[SC_SEISMICWEAPON].timer!=-1)	// �T�C�Y�~�b�N�E�F�|��
				ret = ELE_EARTH;
			if( sc_data[SC_FLAMELAUNCHER].timer!=-1)	// �t���[�������`���[
				ret = ELE_FIRE;
			if( sc_data[SC_LIGHTNINGLOADER].timer!=-1)	// ���C�g�j���O���[�_�[
				ret = ELE_WIND;
			if( sc_data[SC_ENCPOISON].timer!=-1)		// �G���`�����g�|�C�Y��
				ret = ELE_POISON;
			if( sc_data[SC_ASPERSIO].timer!=-1)			// �A�X�y���V�I
				ret = ELE_HOLY;
			if( sc_data[SC_DARKELEMENT].timer!=-1)		// �ő���
				ret = ELE_DARK;
			if( sc_data[SC_ATTENELEMENT].timer!=-1)		// �O����
				ret = ELE_GHOST;
			if( sc_data[SC_UNDEADELEMENT].timer!=-1)	// �s������
				ret = ELE_UNDEAD;

		}
		return ret;
	}
	return 0;
}
/*==========================================
 * �Ώۂ̃p�[�e�BID��Ԃ�(�ėp)
 *------------------------------------------
 */
int status_get_party_id(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.party_id;
	else if(bl->type==BL_MOB && (struct mob_data *)bl){
		struct mob_data *md=(struct mob_data *)bl;
		if( md->master_id>0 )
				return -md->master_id;
			return -md->bl.id;
	}
	else if(bl->type==BL_HOM && (struct homun_data *)bl){
		//struct homun_data *hd = (struct homun_data *)bl;
		//return status_get_party_id(&hd->msd->bl);
		return 0;
	}
	else if(bl->type==BL_SKILL && (struct skill_unit *)bl)
		return ((struct skill_unit *)bl)->group->party_id;
	else
		return 0;
}
/*==========================================
 * �Ώۂ̃M���hID��Ԃ�(�ėp)
 *------------------------------------------
 */
int status_get_guild_id(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data *)bl)->status.guild_id;
	else if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data *)bl)->class_;
	else if(bl->type==BL_HOM && (struct homun_data *)bl){
		//struct homun_data *hd = (struct homun_data *)bl;
		//return status_get_guild_id(&hd->msd->bl);
		return 0;
	}
	else if(bl->type==BL_SKILL && (struct skill_unit *)bl)
		return ((struct skill_unit *)bl)->group->guild_id;
	else
		return 0;
}
/*==========================================
 * �Ώۂ̎푰��Ԃ�(�ėp)
 *------------------------------------------
 */
int status_get_race(struct block_list *bl)
{
	int race;
	struct status_change *sc_data;

	nullpo_retr(RCT_FORMLESS, bl);

	if(bl->type==BL_MOB && (struct mob_data *)bl)
		race = mob_db[((struct mob_data *)bl)->class_].race;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		race = ((struct map_session_data *)bl)->race;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class_].race;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return homun_db[((struct homun_data *)bl)->status.class_-HOM_ID].race;
	else
		return RCT_FORMLESS;

	sc_data = status_get_sc_data(bl);

	if(sc_data) {
		if( sc_data[SC_RACEUNKNOWN].timer!=-1)	//���`
			race = RCT_FORMLESS;
		if( sc_data[SC_RACEUNDEAD].timer!=-1)	//�s��
			race = RCT_UNDEAD;
		if( sc_data[SC_RACEBEAST].timer!=-1)	//����
			race = RCT_BRUTE;
		if( sc_data[SC_RACEPLANT].timer!=-1)	//�A��
			race = RCT_PLANT;
		if( sc_data[SC_RACEINSECT].timer!=-1)	//����
			race = RCT_INSECT;
		if( sc_data[SC_RACEFISH].timer!=-1)		//���L
			race = RCT_FISH;
		if( sc_data[SC_RACEDEVIL].timer!=-1)	//����
			race = RCT_DEMON;
		if( sc_data[SC_RACEHUMAN].timer!=-1)	//�l��
			race = RCT_HUMAN;
		if( sc_data[SC_RACEANGEL].timer!=-1)	//�V�g
			race = RCT_ANGEL;
		if( sc_data[SC_RACEDRAGON].timer!=-1)	//��
			race = RCT_DRAGON;
	}

	return race;
}
/*==========================================
 * �Ώۂ̃T�C�Y��Ԃ�(�ėp)
 *------------------------------------------
 */
int status_get_size(struct block_list *bl)
{
	nullpo_retr(1, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class_].size;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
	{
		if(pc_isbaby((struct map_session_data *)bl))
			return 0;
		else
			return 1;
	}else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class_].size;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return homun_db[((struct homun_data *)bl)->status.class_-HOM_ID].size;
	else
		return 1;
}
/*==========================================
 * �Ώۂ̃��[�h��Ԃ�(�ėp)
 *------------------------------------------
 */
int status_get_mode(struct block_list *bl)
{
	nullpo_retr(0x01, bl);
	if(bl->type==BL_MOB) {
		struct mob_data* md = (struct mob_data*)bl;
		return (md->mode ? md->mode : mob_db[md->class_].mode);
	}
	else if(bl->type==BL_PET)
		return mob_db[((struct pet_data *)bl)->class_].mode;
	else
		return 0x01;	// �Ƃ肠���������Ƃ������Ƃ�1
}
/*==========================================
 * �Ώۂ�MVPExp��Ԃ�(�ėp)
 *------------------------------------------
 */
int status_get_mexp(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return mob_db[((struct mob_data *)bl)->class_].mexp;
	else if(bl->type==BL_PET && (struct pet_data *)bl)
		return mob_db[((struct pet_data *)bl)->class_].mexp;
	else
		return 0;
}
/*==========================================
 * �Ώۂ̓G�^�C�v��Ԃ�(�ėp)
 *------------------------------------------
 */
int status_get_enemy_type(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if( bl->type == BL_PC )
		return 1;
	else if( bl->type == BL_MOB && !(status_get_mode(bl)&0x20) )
		return 2;
	else if( bl->type == BL_HOM )
		return 3;
	else
		return 0;
}
/*==========================================
 * �Ώۂ̕��F��Ԃ�(�ėp)
 *------------------------------------------
 */
short status_get_clothes_color(struct block_list *bl)
{
	short color = -1;

	nullpo_retr(-1, bl);

	if(bl->type == BL_PC)
		color = ((struct map_session_data *)bl)->status.clothes_color;
	else if(bl->type == BL_MOB || bl->type == BL_PET)
	{
		int id = status_get_class(bl);
		if(id >= 0 && mob_get_viewclass(id) < MAX_VALID_PC_CLASS)
			color = mob_get_clothes_color(id);
	}
	if(color >= 0 && color < MAX_CLOTH_COLOR)
		return color;

	return -1;
}

/*==========================================
 * StatusChange�n�̎擾
 *------------------------------------------
 */
struct status_change *status_get_sc_data(struct block_list *bl)
{
	nullpo_retr(NULL, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return ((struct mob_data*)bl)->sc_data;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return ((struct map_session_data*)bl)->sc_data;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return ((struct homun_data*)bl)->sc_data;
	return NULL;
}
short *status_get_sc_count(struct block_list *bl)
{
	nullpo_retr(NULL, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->sc_count;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->sc_count;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return &((struct homun_data*)bl)->sc_count;
	return NULL;
}
unsigned short *status_get_opt1(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->opt1;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->opt1;
	else if(bl->type==BL_NPC && (struct npc_data *)bl)
		return &((struct npc_data*)bl)->opt1;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return &((struct homun_data*)bl)->opt1;
	return 0;
}
unsigned short *status_get_opt2(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->opt2;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->opt2;
	else if(bl->type==BL_NPC && (struct npc_data *)bl)
		return &((struct npc_data*)bl)->opt2;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return &((struct homun_data*)bl)->opt2;
	return 0;
}
unsigned int *status_get_opt3(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->opt3;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->opt3;
	else if(bl->type==BL_NPC && (struct npc_data *)bl)
		return &((struct npc_data*)bl)->opt3;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return &((struct homun_data*)bl)->opt3;
	return 0;
}
unsigned int *status_get_option(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_MOB && (struct mob_data *)bl)
		return &((struct mob_data*)bl)->option;
	else if(bl->type==BL_PC && (struct map_session_data *)bl)
		return &((struct map_session_data*)bl)->status.option;
	else if(bl->type==BL_NPC && (struct npc_data *)bl)
		return &((struct npc_data*)bl)->option;
	else if(bl->type==BL_HOM && (struct homun_data *)bl)
		return &((struct homun_data*)bl)->status.option;
	return 0;
}

int status_check_no_magic_damage(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC)
	{
		if(((struct map_session_data*)bl)->special_state.no_magic_damage)
			return 1;
	}
	return 0;
}
#ifdef DYNAMIC_SC_DATA
int status_calloc_sc_data(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(status_check_dummy_sc_data(bl) == 0)
		return 0;
	if(bl->type == BL_MOB)
	{
		struct mob_data* md = (struct mob_data*)bl;
		int i;

		md->sc_data = (struct status_change *)aCalloc(MAX_STATUSCHANGE,sizeof(struct status_change));
		for(i=0;i<MAX_STATUSCHANGE;i++) {
			md->sc_data[i].timer=-1;
			md->sc_data[i].val1 = md->sc_data[i].val2 = md->sc_data[i].val3 = md->sc_data[i].val4 =0;
		}
		md->sc_count = 0;
		return 1;	// calloced
	}
	return 0;
}
int status_free_sc_data(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type == BL_MOB && status_check_dummy_sc_data(bl)==0)
	{
		struct mob_data *md = (struct mob_data *)bl;
		map_freeblock(md->sc_data);
		md->sc_data = dummy_sc_data;
		md->sc_count = 0;
	}
	return 0;
}
int status_check_dummy_sc_data(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type == BL_MOB)
	{
		if(((struct mob_data *)bl)->sc_data == dummy_sc_data)
			return 1;
	}
	return 0;
}
#endif

/*==========================================
 * �X�e�[�^�X�ُ�J�n
 *------------------------------------------
 */
int status_change_start(struct block_list *bl,int type,int val1,int val2,int val3,int val4,int tick,int flag)
{
	struct map_session_data *sd = NULL;
	struct mob_data *md = NULL;
	struct homun_data *hd = NULL;
	struct status_change* sc_data;
	short *sc_count;
	unsigned short *opt1, *opt2;
	unsigned int *opt3, *option;
	int opt_flag = 0, calc_flag = 0, race, mode, elem;
	int scdef=0;

	nullpo_retr(0, bl);
	if(type == -1) return 0;
	if(bl->type == BL_SKILL)
		return 0;
	if(bl->type == BL_HOM && !battle_config.allow_homun_status_change)
	{
		if(type<SC_AVOID || SC_SPEED<type)
			return 0;
	}
#ifdef DYNAMIC_SC_DATA
	status_calloc_sc_data(bl);
#endif
	nullpo_retr(0, sc_data=status_get_sc_data(bl));
	nullpo_retr(0, sc_count=status_get_sc_count(bl));
	nullpo_retr(0, option=status_get_option(bl));
	nullpo_retr(0, opt1=status_get_opt1(bl));
	nullpo_retr(0, opt2=status_get_opt2(bl));
	nullpo_retr(0, opt3=status_get_opt3(bl));

	race=status_get_race(bl);
	mode=status_get_mode(bl);
	elem=status_get_elem_type(bl);

	if(sc_data[SC_STATUS_UNCHANGE].timer != -1) {
		if(status_is_disable(type,0x10))	// �S�X�y���̑S��Ԉُ�ϐ����Ȃ疳��
			return 0;
	}

	if(type == SC_AETERNA && (sc_data[SC_STONE].timer != -1 || sc_data[SC_FREEZE].timer != -1) )
		return 0;

	//����n
	if(type >= MAX_STATUSCHANGE)
	{
		switch(type)
		{
			case SC_SOUL:
				status_change_soulstart(bl,val1,val2,val3,val4,tick,flag);
				break;
			default:
				if(battle_config.error_log)
					printf("UnknownStatusChange [%d]\n", type);
				break;
		}
		return 0;
	}

	//ON/OFF
	switch(type)
	{
		case SC_AUTOBERSERK:
		case SC_READYSTORM:
		case SC_READYDOWN:
		case SC_READYTURN:
		case SC_READYCOUNTER:
		case SC_DODGE:
		case SC_FUSION:
			if(sc_data[type].timer!=-1)
			{
				status_change_end(bl,type,-1);
				return 0;
			}
			break;
	}

	switch(type){
		case SC_STONE:
		case SC_FREEZE:
			scdef=3+status_get_mdef(bl)+status_get_luk(bl)/3;
			break;
		case SC_STAN:
		case SC_SILENCE:
		case SC_POISON:
		case SC_DPOISON:
		case SC_BLEED:	// �ڍוs���Ȃ̂łƂ肠����������
			scdef=3+status_get_vit(bl)+status_get_luk(bl)/3;
			break;
		case SC_SLEEP:
		case SC_BLIND:
			scdef=3+status_get_int(bl)+status_get_luk(bl)/3;
			break;
		case SC_CURSE:
			scdef=3+status_get_luk(bl);
			break;

		case SC_CONFUSION:
		default:
			scdef=0;
	}
	if(scdef) {
		if(sc_data[SC_STATUS_UNCHANGE].timer != -1) {
			if(status_is_disable(type,0x10))	// �S�X�y���̑S��Ԉُ�ϐ����Ȃ疳��
				return 0;
		}
		if(sc_data[SC_SIEGFRIED].timer != -1)		// �W�[�N�t���[�h�̏�Ԉُ�ϐ�
			scdef += 50;
	}

	if(!(flag&8) && scdef>=100)	//flag��+8�Ȃ犮�S�ϐ��v�Z���Ȃ�
		return 0;

	if( bl->type != BL_PC && bl->type != BL_MOB && bl->type != BL_HOM ) {
		if(battle_config.error_log)
			printf("status_change_start: neither MOB nor PC !\n");
		return 0;
	}

	sd = BL_DOWNCAST( BL_PC,  bl );
	md = BL_DOWNCAST( BL_MOB, bl );
	hd = BL_DOWNCAST( BL_HOM, bl );

	if( sd ) {
		// �A�h���i�������b�V���̕��픻��
		if( type == SC_ADRENALINE && !(skill_get_weapontype(BS_ADRENALINE)&(1<<sd->status.weapon)) )
			return 0;
		// �t���A�h���i�������b�V���̕��픻��
		if( type == SC_ADRENALINE2 && !(skill_get_weapontype(BS_ADRENALINE2)&(1<<sd->status.weapon)) )
			return 0;
		if( SC_STONE <= type && type <= SC_BLIND ) {	/* �J�[�h�ɂ��ϐ� */
			if( !(flag&8) && sd->reseff[type-SC_STONE] > 0 && atn_rand()%10000 < sd->reseff[type-SC_STONE] ) {
				if(battle_config.battle_log)
					printf("PC %d skill_sc_start: card�ɂ��ُ�ϐ�����\n",sd->bl.id);
				return 0;
			}
		}
	}

	//�A���f�b�h�͓����E�Ή��E�o������
	if((race == RCT_UNDEAD || elem == ELE_UNDEAD) && !(flag&1) && (type == SC_STONE || type == SC_FREEZE || type == SC_BLEED))
		return 0;

	if((type == SC_ADRENALINE || type == SC_ADRENALINE2 || type == SC_WEAPONPERFECTION || type == SC_OVERTHRUST) &&
		sc_data[type].timer != -1 && sc_data[type].val2 && !val2)
		return 0;

	// �{�X�����ɂ͖���(�������J�[�h�ɂ����ʂ͓K�p�����)
	if( mode&0x20 && !(flag&1) && status_is_disable(type,0x01) ) {
		if(type == SC_BLESSING && !battle_check_undead(race,elem) && race != RCT_DEMON) {
			// �u���X�͕s��/�����łȂ��Ȃ����
			;
		} else {
			return 0;
		}
	}

	//if(type==SC_FREEZE || type==SC_STAN || type==SC_SLEEP)
	if(type==SC_STAN || type==SC_SLEEP)
		unit_stop_walking(bl,1);

	if (type==SC_BLESSING && (bl->type==BL_PC || (!battle_check_undead(race,elem) && race != RCT_DEMON))) {
		if (sc_data[SC_CURSE].timer!=-1)
			status_change_end(bl,SC_CURSE,-1);
		if (sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
			status_change_end(bl,SC_STONE,-1);
	}

	if(sc_data[type].timer != -1){	/* ���łɓ����ُ�ɂȂ��Ă���ꍇ�^�C�}���� */
		if(sc_data[type].val1 > val1 && type != SC_COMBO && type != SC_DANCING && type != SC_DEVOTION &&
			type != SC_SPEEDPOTION0 && type != SC_SPEEDPOTION1 && type != SC_SPEEDPOTION2 && type != SC_SPEEDPOTION3 &&
			type!= SC_DEVIL && type!=SC_DOUBLE && type != SC_TKCOMBO && type!=SC_DODGE && type!=SC_SPURT)
			return 0;
		if ((type >=SC_STAN && type <= SC_BLIND) || type == SC_DPOISON || type == SC_FOGWALLPENALTY || type == SC_FORCEWALKING)
			return 0;/* �p���������ł��Ȃ���Ԉُ�ł��鎞�͏�Ԉُ���s��Ȃ� */
		if(type == SC_GRAFFITI){	//�ُ풆�ɂ�����x��Ԉُ�ɂȂ������ɉ������Ă���ēx������
			status_change_end(bl,type,-1);
		}else{
			(*sc_count)--;
			delete_timer(sc_data[type].timer, status_change_timer);
			sc_data[type].timer = -1;
		}
	}
	// �N�@�O�}�C�A���͖���
	if(sc_data[SC_QUAGMIRE].timer != -1 && status_is_disable(type,0x02))
		return 0;
	// ����Y��Ȃ��Œ��͖���
	if(sc_data[SC_DONTFORGETME].timer != -1 && status_is_disable(type,0x04))
		return 0;
	// ���x�������͖���
	if(sc_data[SC_DECREASEAGI].timer != -1 && status_is_disable(type,0x08))
		return 0;

	switch(type){	/* �ُ�̎�ނ��Ƃ̏��� */
		case SC_ACTION_DELAY:
		case SC_ITEM_DELAY:
		case SC_DOUBLE:				/* �_�u���X�g���C�t�B���O */
		case SC_SUFFRAGIUM:			/* �T�t���M�� */
		case SC_MAGNIFICAT:			/* �}�O�j�t�B�J�[�g */
		case SC_AETERNA:			/* �G�[�e���i */
		case SC_BASILICA:			/* �o�W���J */
		case SC_TRICKDEAD:			/* ���񂾂ӂ� */
		case SC_STRIPWEAPON:
		case SC_STRIPSHIELD:
		case SC_STRIPARMOR:
		case SC_STRIPHELM:
		case SC_CP_WEAPON:
		case SC_CP_SHIELD:
		case SC_CP_ARMOR:
		case SC_CP_HELM:
		case SC_COMBO:
		case SC_EXTREMITYFIST:			/* ���C���e���� */
		case SC_RICHMANKIM:
		case SC_ROKISWEIL:			/* ���L�̋��� */
		case SC_INTOABYSS:			/* �[���̒��� */
		case SC_POEMBRAGI:			/* �u���M�̎� */
		case SC_ANKLE:				/* �A���N�� */
		case SC_MAGNUM:				/* �}�O�i���u���C�N */
		case SC_TIGERFIST:			/* ���Ռ� */
		case SC_ENERGYCOAT:			/* �G�i�W�[�R�[�g */
			break;

		case SC_CONCENTRATE:			/* �W���͌��� */
		case SC_BLESSING:			/* �u���b�V���O */
		case SC_ANGELUS:			/* �A���[���X */
		case SC_RESISTWATER:
		case SC_RESISTGROUND:
		case SC_RESISTFIRE:
		case SC_RESISTWIND:
		case SC_RESISTPOISON:
		case SC_RESISTHOLY:
		case SC_RESISTDARK:
		case SC_RESISTTELEKINESIS:
		case SC_RESISTUNDEAD:
		case SC_RESISTALL:
		case SC_TAROTCARD:
		case SC_IMPOSITIO:			/* �C���|�V�e�B�I�}�k�X */
		case SC_DEVOTION:			/* �f�B�{�[�V���� */
		case SC_GLORIA:				/* �O�����A */
		case SC_LOUD:				/* ���E�h�{�C�X */
		case SC_MINDBREAKER:			/* �}�C���h�u���[�J�[ */
		case SC_ETERNALCHAOS:			/* �G�^�[�i���J�I�X */
		case SC_WHISTLE:			/* ���J */
		case SC_ASSNCROS:			/* �[�z�̃A�T�V���N���X */
		case SC_APPLEIDUN:			/* �C�h�D���̗ь� */
		case SC_SANTA:
		case SC_TRUESIGHT:			/* �g�D���[�T�C�g */
		case SC_SPIDERWEB:			/* �X�p�C�_�[�E�F�b�u */
		case SC_TWOHANDQUICKEN:			/* 2HQ */
		case SC_STEELBODY:			/* ���� */
		case SC_CONCENTRATION:			/* �R���Z���g���[�V���� */
		case SC_MARIONETTE:			/* �}���I�l�b�g�R���g���[�� */
		case SC_MARIONETTE2:			/* �}���I�l�b�g�R���g���[�� */
		case SC_WEDDING:			/* �����p(�����ߏւɂȂ��ĕ����̂��x���Ƃ�) */
			calc_flag = 1;
			break;

		//�ړ��H
		case SC_POEMBRAGI_:			/* �u���M�̎� */
		case SC_FOGWALLPENALTY:
		case SC_FOGWALL:
		case SC_REVERSEORCISH:
		case SC_REDEMPTIO:
		case SC_GRAVITATION_USER:
			break;

		case SC_HUMMING:			/* �n�~���O */
		case SC_FORTUNE:			/* �K�^�̃L�X */
		case SC_SERVICE4U:			/* �T�[�r�X�t�H�[���[ */
		case SC_WHISTLE_:			/* ���J */
		case SC_ASSNCROS_:			/* �[�z�̃A�T�V���N���X */
		case SC_APPLEIDUN_:			/* �C�h�D���̗ь� */
		case SC_HUMMING_:			/* �n�~���O */
		case SC_DONTFORGETME_:			/* ����Y��Ȃ��� */
		case SC_FORTUNE_:			/* �K�^�̃L�X */
		case SC_SERVICE4U_:			/* �T�[�r�X�t�H�[���[ */
		case SC_GRAVITATION:			/* �O���r�e�[�V�����t�B�[���h */
			calc_flag = 1;
			break;

		case SC_PROVOKE:			/* �v���{�b�N */
			calc_flag = 1;
			if(tick <= 0) tick = 1000;	/* (�I�[�g�o�[�T�[�N) */
			break;
		case SC_ENDURE:				/* �C���f���A */
			if(tick <= 0) tick = 1000 * 60;
			calc_flag = 1;
			val2 = 7;	// 7��U�����ꂽ�����
			break;
		case SC_INCREASEAGI:		/* ���x���� */
			calc_flag = 1;
			if(sc_data[SC_DECREASEAGI].timer!=-1 )
				status_change_end(bl,SC_DECREASEAGI,-1);
			break;
		case SC_DECREASEAGI:		/* ���x���� */
			calc_flag = 1;
			if(sc_data[SC_INCREASEAGI].timer!=-1 )
				status_change_end(bl,SC_INCREASEAGI,-1);
			if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
				status_change_end(bl,SC_TWOHANDQUICKEN,-1);
			if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
				status_change_end(bl,SC_SPEARSQUICKEN,-1);
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			break;
		case SC_SIGNUMCRUCIS:		/* �V�O�i���N���V�X */
			calc_flag = 1;
			val2 = 10 + val1*4;
			tick = 600*1000;
			clif_emotion(bl,4);
			break;
		case SC_SLOWPOISON:
			if (sc_data[SC_POISON].timer == -1 && sc_data[SC_DPOISON].timer == -1)
				return 0;
			break;
		case SC_ONEHAND:			/* 1HQ */
			if(sc_data[SC_SPEEDPOTION0].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION0,-1);
			if(sc_data[SC_SPEEDPOTION1].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION1,-1);
			if(sc_data[SC_SPEEDPOTION2].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION2,-1);
			calc_flag = 1;
			break;
		case SC_ADRENALINE:			/* �A�h���i�������b�V�� */
			calc_flag = 1;
			if(sc_data[SC_ADRENALINE2].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE2,-1);
			if(!(flag&2) && sd && pc_checkskill(sd,BS_HILTBINDING)>0)
				tick += tick / 10;
			break;
		case SC_ADRENALINE2:			/* �t���A�h���i�������b�V�� */
			calc_flag = 1;
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			if(!(flag&2) && sd && pc_checkskill(sd,BS_HILTBINDING)>0)
				tick += tick / 10;
			break;
		case SC_WEAPONPERFECTION:		/* �E�F�|���p�[�t�F�N�V���� */
			if(!(flag&2) && sd && pc_checkskill(sd,BS_HILTBINDING)>0)
				tick += tick / 10;
			break;
		case SC_OVERTHRUST:			/* �I�[�o�[�g���X�g */
			if(sc_data[SC_OVERTHRUSTMAX].timer != -1)
				return 0;
			if(!(flag&2) && sd && pc_checkskill(sd,BS_HILTBINDING)>0)
				tick += tick / 10;
			break;
		case SC_MAXIMIZEPOWER:		/* �}�L�V�}�C�Y�p���[(SP��1���鎞��,val2�ɂ�) */
			if(bl->type == BL_PC)
				val2 = tick;
			else
				tick = 5000*val1;
			break;
		case SC_ENCPOISON:			/* �G���`�����g�|�C�Y�� */
			calc_flag = 1;
			val2=(((val1 - 1) / 2) + 3)*100;	/* �ŕt�^�m�� */
			status_encchant_eremental_end(bl,SC_ENCPOISON);
			break;
		case SC_EDP:			/* �G���`�����g�f�b�h���[�|�C�Y�� */
			val2 = val1 + 2;			/* �ғŕt�^�m��(%) */
			break;
		case SC_POISONREACT:	/* �|�C�Y�����A�N�g */
			val2 = val1/2 + 1/(val1)?val1:1;
			break;
		case SC_ASPERSIO:			/* �A�X�y���V�I */
			status_encchant_eremental_end(bl,SC_ASPERSIO);
			break;
		case SC_BENEDICTIO:			/* ���� */
			status_enchant_armor_eremental_end(bl,SC_BENEDICTIO);
			break;
		case SC_ELEMENTWATER:		// ��
			status_enchant_armor_eremental_end(bl,SC_ELEMENTWATER);
			if(sd){
				clif_displaymessage(sd->fd,"�h��ɐ��������t�^����܂����B");
			}
			break;
		case SC_ELEMENTGROUND:		// �y
			status_enchant_armor_eremental_end(bl,SC_ELEMENTGROUND);
			if(sd){
				clif_displaymessage(sd->fd,"�h��ɓy�������t�^����܂����B");
			}
			break;
		case SC_ELEMENTFIRE:		// ��
			status_enchant_armor_eremental_end(bl,SC_ELEMENTFIRE);
			if(sd){
				clif_displaymessage(sd->fd,"�h��ɉΑ������t�^����܂����B");
			}
			break;
		case SC_ELEMENTWIND:		// ��
			status_enchant_armor_eremental_end(bl,SC_ELEMENTWIND);
			if(sd){
				clif_displaymessage(sd->fd,"�h��ɕ��������t�^����܂����B");
			}
			break;
		case SC_ELEMENTHOLY:		// ��
			status_enchant_armor_eremental_end(bl,SC_ELEMENTHOLY);
			if(sd){
				clif_displaymessage(sd->fd,"�h��ɐ��������t�^����܂����B");
			}
			break;
		case SC_ELEMENTDARK:		// ��
			status_enchant_armor_eremental_end(bl,SC_ELEMENTDARK);
			if(sd){
				clif_displaymessage(sd->fd,"�h��Ɉő������t�^����܂����B");
			}
			break;
		case SC_ELEMENTELEKINESIS:		// �O
			status_enchant_armor_eremental_end(bl,SC_ELEMENTELEKINESIS);
			if(sd){
				clif_displaymessage(sd->fd,"�h��ɔO�������t�^����܂����B");
			}
			break;
		case SC_ELEMENTPOISON:		// ��
			status_enchant_armor_eremental_end(bl,SC_ELEMENTPOISON);
			if(sd){
				clif_displaymessage(sd->fd,"�h��ɓő������t�^����܂����B");
			}
			break;
		case SC_ELEMENTUNDEAD:		// �s��
			status_enchant_armor_eremental_end(bl,SC_ELEMENTUNDEAD);
			//if(sd){
			//	clif_displaymessage(sd->fd,"�h��ɕs���������t�^����܂����B");
			//}
			break;
		case SC_RACEUNKNOWN:
		case SC_RACEUNDEAD:
		case SC_RACEBEAST:
		case SC_RACEPLANT:
		case SC_RACEINSECT:
		case SC_RACEFISH:
		case SC_RACEDEVIL:
		case SC_RACEHUMAN:
		case SC_RACEANGEL:
		case SC_RACEDRAGON:
			status_change_race_end(bl,type);
			if(sd){
				char message[64];
				sprintf(message,"�푰��%s�ɂȂ�܂���",race_name[type-SC_RACEUNKNOWN]);
				clif_displaymessage(sd->fd,message);
			}
			break;
		case SC_MAGICROD:
			val2 = val1*20;
			break;
		case SC_KYRIE:				/* �L���G�G���C�\�� */
			/* �A�X�����|�����Ă������������ */
			if(sc_data[SC_ASSUMPTIO].timer!=-1)
				status_change_end(bl,SC_ASSUMPTIO,-1);
			/* �L���G���|���� */
			val2 = (int)((atn_bignumber)status_get_max_hp(bl) * (val1 * 2 + 10) / 100);/* �ϋv�x */
			val3 = (val1 / 2 + 5);	/* �� */
			break;
		case SC_QUAGMIRE:			/* �N�@�O�}�C�A */
			calc_flag = 1;
			if(sc_data[SC_CONCENTRATE].timer!=-1 )	/* �W���͌������ */
				status_change_end(bl,SC_CONCENTRATE,-1);
			if(sc_data[SC_INCREASEAGI].timer!=-1 )	/* ���x�㏸���� */
				status_change_end(bl,SC_INCREASEAGI,-1);
			if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
				status_change_end(bl,SC_TWOHANDQUICKEN,-1);
			if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
				status_change_end(bl,SC_SPEARSQUICKEN,-1);
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			if(sc_data[SC_ADRENALINE2].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE2,-1);
			if(sc_data[SC_LOUD].timer!=-1 )
				status_change_end(bl,SC_LOUD,-1);
			if(sc_data[SC_WINDWALK].timer!=-1 )	/* �E�C���h�E�H�[�N */
				status_change_end(bl,SC_WINDWALK,-1);
			if(sc_data[SC_CARTBOOST].timer!=-1 )	/* �J�[�g�u�[�X�g */
				status_change_end(bl,SC_CARTBOOST,-1);
			if(sc_data[SC_ONEHAND].timer!=-1 )
				status_change_end(bl,SC_ONEHAND,-1);
			break;
		case SC_MAGICPOWER:			/* ���@�͑��� */
			val2 = 1;				// ��x��������
			break;
		case SC_SACRIFICE:			/* �T�N���t�@�C�X */
			val2 = 5;				// 5��̍U���ŗL��
			break;
		case SC_FLAMELAUNCHER:		/* �t���[�������`���[ */
		case SC_FROSTWEAPON:		/* �t���X�g�E�F�|�� */
		case SC_LIGHTNINGLOADER:	/* ���C�g�j���O���[�_�[ */
		case SC_SEISMICWEAPON:		/* �T�C�Y�~�b�N�E�F�|�� */
		case SC_DARKELEMENT:		/* �ő��� */
		case SC_ATTENELEMENT:		/* �O���� */
		case SC_UNDEADELEMENT:		/* �s������ */
			status_encchant_eremental_end(bl,type);
			break;
		case SC_PROVIDENCE:			/* �v�����B�f���X */
			calc_flag = 1;
			val2=val1*5;
			break;
		case SC_REFLECTSHIELD:
			val2=10+val1*3;
			break;
		case SC_AUTOSPELL:			/* �I�[�g�X�y�� */
			val4 = 5 + val1*2;
			break;
		case SC_VOLCANO:
			calc_flag = 1;
			val3 = val1*10;
			val4 = val1*(11-val1)/2 + 5;
			break;
		case SC_DELUGE:
			calc_flag = 1;
			val3 = val1*(11-val1)/2;
			val4 = val1*(11-val1)/2 + 5;
			break;
		case SC_VIOLENTGALE:
			calc_flag = 1;
			val3 = val1*3;
			val4 = val1*(11-val1)/2 + 5;
			break;
		case SC_SPEARSQUICKEN:		/* �X�s�A�N�C�b�P�� */
			calc_flag = 1;
			val2 = 20+val1;
			break;
		case SC_BLADESTOP_WAIT:		/* ���n���(�҂�) */
			break;
		case SC_BLADESTOP:		/* ���n��� */
			if(val2==2)
				clif_bladestop(map_id2bl(val3),val4,1);
			break;
		case SC_LULLABY:			/* �q��S */
			val2 = 11;
			break;
		case SC_DRUMBATTLE:			/* �푾�ۂ̋��� */
			calc_flag = 1;
			val2 = (val1+1)*25;
			val3 = (val1+1)*2;
			break;
		case SC_NIBELUNGEN:			/* �j�[�x�����O�̎w�� */
			calc_flag = 1;
			val2 = (val1+2)*25;
			break;
		case SC_SIEGFRIED:			/* �s���g�̃W�[�N�t���[�h */
			calc_flag = 1;
			val2 = 5 + val1*15;
			break;
		case SC_DISSONANCE:			/* �s���a�� */
			val2 = 10;
			break;
		case SC_UGLYDANCE:			/* ��������ȃ_���X */
			val2 = 10;
			break;
		case SC_DONTFORGETME:		/* ����Y��Ȃ��� */
			calc_flag = 1;
			if(sc_data[SC_INCREASEAGI].timer!=-1 )	/* ���x�㏸���� */
				status_change_end(bl,SC_INCREASEAGI,-1);
			if(sc_data[SC_TWOHANDQUICKEN].timer!=-1 )
				status_change_end(bl,SC_TWOHANDQUICKEN,-1);
			if(sc_data[SC_SPEARSQUICKEN].timer!=-1 )
				status_change_end(bl,SC_SPEARSQUICKEN,-1);
			if(sc_data[SC_ADRENALINE].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE,-1);
			if(sc_data[SC_ADRENALINE2].timer!=-1 )
				status_change_end(bl,SC_ADRENALINE2,-1);
			if(sc_data[SC_ASSNCROS].timer!=-1 )
				status_change_end(bl,SC_ASSNCROS,-1);
			if(sc_data[SC_WINDWALK].timer!=-1 )	/* �E�C���h�E�H�[�N */
				status_change_end(bl,SC_WINDWALK,-1);
			if(sc_data[SC_CARTBOOST].timer!=-1 )	/* �J�[�g�u�[�X�g */
				status_change_end(bl,SC_CARTBOOST,-1);
			if(sc_data[SC_ONEHAND].timer!=-1 )
				status_change_end(bl,SC_ONEHAND,-1);
			break;
		case SC_LONGINGFREEDOM:		//�����S�����Ȃ���
			calc_flag = 1;
			val3 = 1;
			tick = 1000;
			break;
		case SC_DANCING:			/* �_���X/���t�� */
			calc_flag = 1;
			val3= tick / 1000;
			tick = 1000;
			break;
		case SC_EXPLOSIONSPIRITS:	// �����g��
			calc_flag = 1;
			val2 = 75 + 25*val1;
			break;
		case SC_AUTOCOUNTER:
			val3 = val4 = 0;
			break;
		case SC_POISONPOTION:		/* �Ŗ�̕r */
			calc_flag = 1;
			val2 = 25;
			break;
		case SC_SPEEDPOTION0:		/* �����|�[�V���� */
		case SC_SPEEDPOTION1:
		case SC_SPEEDPOTION2:
			if(type!=SC_SPEEDPOTION0 && sc_data[SC_SPEEDPOTION0].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION0,-1);
			if(type!=SC_SPEEDPOTION1 && sc_data[SC_SPEEDPOTION1].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION1,-1);
			if(type!=SC_SPEEDPOTION2 && sc_data[SC_SPEEDPOTION2].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION2,-1);
			if(sc_data[SC_SPEEDPOTION3].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION3,-1);
			calc_flag = 1;
			val2 = 5*(2+type-SC_SPEEDPOTION0);
			break;
		case SC_SPEEDPOTION3: //�o�[�T�[�N�s�b�`���[
			if(sc_data[SC_SPEEDPOTION0].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION0,-1);
			if(sc_data[SC_SPEEDPOTION1].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION1,-1);
			if(sc_data[SC_SPEEDPOTION2].timer!=-1)
				status_change_end(bl,SC_SPEEDPOTION2,-1);
			calc_flag = 1;
			val2 = 20;
			break;

		case SC_INCATK:		//item 682�p
		case SC_INCMATK:	//item 683�p
			calc_flag = 1;
			break;
		case SC_NOCHAT:	//�`���b�g�֎~���
			{
				time_t timer;
				tick = 60000;
				if(!val2)
					val2 = (int)time(&timer);
				if(sd)
					clif_updatestatus(sd,SP_MANNER);	// �X�e�[�^�X���N���C�A���g�ɑ���
			}
			break;
		case SC_SELFDESTRUCTION: //����
			tick = 100;
			break;

		/* option1 */
		case SC_STONE:				/* �Ή� */
			if(!(flag&2)) {
				int sc_def = status_get_mdef(bl)*200;
				tick = tick - sc_def;
			}
			val3 = tick/1000;
			if(val3 < 1) val3 = 1;
			tick = 5000;
			val2 = 1;
			break;
		case SC_SLEEP:				/* ���� */
			if(!(flag&2)) {
//				int sc_def = 100 - (status_get_int(bl) + status_get_luk(bl)/3);
//				tick = tick * sc_def / 100;
//				if(tick < 1000) tick = 1000;
				tick = 30000;//�����̓X�e�[�^�X�ϐ��Ɋւ�炸30�b
			}
			break;
		case SC_FREEZE:				/* ���� */
			if(!(flag&2)) {
				int sc_def = 100 - status_get_mdef(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_STAN:				/* �X�^���ival2�Ƀ~���b�Z�b�g�j */
			if(!(flag&2)) {
				int sc_def = 100 - (status_get_vit(bl) + status_get_luk(bl)/3);
				tick = tick * sc_def / 100;
			}
			break;
		/* option2 */
		case SC_DPOISON:			/* �ғ� */
		{
			int mhp = status_get_max_hp(bl);
			int hp = status_get_hp(bl);
			// MHP��1/4�ȉ��ɂ͂Ȃ�Ȃ�
			if (hp > mhp>>2) {
				int diff = 0;
				if(sd)
					diff = mhp*10/100;
				else if(md)
					diff = mhp*15/100;
				if (hp - diff < mhp>>2)
					diff = hp - (mhp>>2);
				unit_heal(bl, -diff, 0);
			}
		}	// fall through
		case SC_POISON:				/* �� */
			calc_flag = 1;
			if(!(flag&2)) {
				int sc_def = 100 - (status_get_vit(bl) + status_get_luk(bl)/5);
				tick = tick * sc_def / 100;
			}
			val3 = tick/1000;
			if(val3 < 1) val3 = 1;
			tick = 1000;
			break;
		case SC_SILENCE:			/* ���فi���b�N�X�f�r�[�i�j */
			if (sc_data[SC_GOSPEL].timer!=-1) {
				status_change_end(bl,SC_GOSPEL,-1);
				break;
			}
			if(!(flag&2)) {
				int sc_def = 100 - status_get_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_BLIND:				/* �Í� */
			calc_flag = 1;
			if(!(flag&2)) {
				int sc_def = status_get_lv(bl)/10 + status_get_int(bl)/15;
				tick = 30000 - sc_def;
			}
			break;
		case SC_CURSE:				/* �� */
			calc_flag = 1;
			if(!(flag&2)) {
				int sc_def = 100 - status_get_vit(bl);
				tick = tick * sc_def / 100;
			}
			break;
		case SC_CONFUSION:			/* ���� */
			break;
		case SC_BLEED:				/* �o�� */
			if(!(flag&2)) {
				// ���ʎ��Ԃ̏ڍוs���Ȃ̂łƂ肠�����ł̂��̂��g��
				int sc_def = 100 - (status_get_vit(bl) + status_get_luk(bl)/5);
				tick = tick * sc_def / 100;
			}
			val3 = (tick < 10000)? 1: tick/10000;
			tick = 10000;	// �_���[�W������10sec��
			break;

		/* option */
		case SC_HIDING:		/* �n�C�f�B���O */
			calc_flag = 1;
			if(bl->type == BL_PC) {
				val2 = tick / 1000;		/* �������� */
				tick = 1000;
			}
			break;
		case SC_CHASEWALK:		/*�`�F�C�X�E�H�[�N*/
		case SC_CLOAKING:		/* �N���[�L���O */
		case SC_INVISIBLE:		/* �C���r�W�u�� */
			if(bl->type == BL_PC)
			{
				calc_flag = 1;
				val2 = tick;
			}
			else
				tick = 5000*val1;
			break;
		case SC_SIGHTBLASTER:	//�T�C�g�u���X�^�[
		case SC_SIGHT:			/* �T�C�g/���A�t */
		case SC_RUWACH:
		case SC_DETECTING:
			val2 = tick/250;
			tick = 10;
			break;

		/* �Z�[�t�e�B�E�H�[���A�j���[�} */
		case SC_SAFETYWALL:
		case SC_PNEUMA:
			break;

		/* �X�L������Ȃ�/���ԂɊ֌W���Ȃ� */
		case SC_RIDING:
			calc_flag = 1;
			tick = 600*1000;
			break;
		case SC_FALCON:
		case SC_WEIGHT50:
		case SC_WEIGHT90:
			tick=600*1000;
			break;

		case SC_AUTOGUARD:
			{
				int i,t;
				for(i=val2=0;i<val1;i++) {
					t = 5-(i>>1);
					val2 += (t < 0)? 1:t;
				}
			}
			break;

		case SC_DEFENDER:
			calc_flag = 1;
			val2 = 5 + val1*15;
			break;

		case SC_KEEPING:
		case SC_BARRIER:
		case SC_HALLUCINATION:
			break;
		case SC_TENSIONRELAX:	/* �e���V���������b�N�X */
			if(bl->type == BL_PC) {
				tick = 10000;
			} else
				return 0;
			break;
		case SC_AURABLADE:		/* �I�[���u���[�h */
		case SC_HEADCRUSH:		/* �w�b�h�N���b�V�� */
		case SC_MELTDOWN:		/* �����g�_�E�� */
			//�Ƃ肠�����蔲��
			break;
		case SC_PARRYING:		/* �p���C���O */
			val2 = 20 + val1*3;
			break;
		case SC_JOINTBEAT:		/* �W���C���g�r�[�g */
			calc_flag = 1;
			val4 = atn_rand()%6;
			if(val4 == 5) {
				// ��͋����I�ɏo���t��
				status_change_start(bl,SC_BLEED,val1,0,0,0,skill_get_time2(LK_JOINTBEAT,val1),10);
			}
			if(!(flag&2)) {
				tick = tick - (status_get_agi(bl)/10 + status_get_luk(bl)/4)*1000;
			}
			break;
		case SC_WINDWALK:		/* �E�C���h�E�H�[�N */
			calc_flag = 1;
			val2 = (val1 / 2); //Flee�㏸��
			break;
		case SC_BERSERK:		/* �o�[�T�[�N */
			if(sd){
				sd->status.sp = 0;
				clif_updatestatus(sd,SP_SP);
				clif_status_change(bl,SC_INCREASEAGI,1);	/* �A�C�R���\�� */
			}
			tick = 1000;
			calc_flag = 1;
			break;
		case SC_ASSUMPTIO:		/* �A�X���v�e�B�I */
			/* �L���G���|�����Ă�������� */
			if(sc_data[SC_KYRIE].timer!=-1)
				status_change_end(bl,SC_KYRIE,-1);
			/* �J�C�g���|�����Ă�������� */
			if(sc_data[SC_KAITE].timer!=-1)
				status_change_end(bl,SC_KAITE,-1);
			break;
		case SC_CARTBOOST:		/* �J�[�g�u�[�X�g */
			calc_flag = 1;
			if(sc_data[SC_DECREASEAGI].timer!=-1 )
				status_change_end(bl,SC_DECREASEAGI,-1);
			break;
		case SC_REJECTSWORD:	/* ���W�F�N�g�\�[�h */
			val2 = 3; //3��U���𒵂˕Ԃ�
			break;
		case SC_MEMORIZE:		/* �������C�Y */
			val2 = 5; //5��r����1/2�ɂ���
			break;
		case SC_GRAFFITI:		/* �O���t�B�e�B */
			{
				struct skill_unit_group *sg = skill_unitsetting(bl,RG_GRAFFITI,val1,val2,val3,0);
				if(sg)
					val4 = (int)sg;
			}
			break;
		case SC_SPLASHER:		/* �x�i���X�v���b�V���[ */
			break;
		case SC_GOSPEL:			/* �S�X�y�� */
			break;
		case SC_INCHIT:			/* HIT�㏸ */
		case SC_INCFLEE:		/* FLEE�㏸ */
		case SC_INCMHP2:		/* MHP%�㏸ */
		case SC_INCMSP2:		/* MSP%�㏸ */
		case SC_INCATK2:		/* ATK%�㏸ */
		case SC_INCHIT2:		/* HIT%�㏸ */
		case SC_INCFLEE2:		/* FLEE%�㏸ */
		case SC_INCALLSTATUS:	/* �S�X�e�[�^�X�{20 */
			calc_flag = 1;
			break;
		case SC_STATUS_UNCHANGE:	/* �S��Ԉُ�ϐ� */
		case SC_INCDAMAGE:		/* ��_���[�W%�㏸ */
			break;
		case SC_PRESERVE:		/* �v���U�[�u */
			break;
		case SC_OVERTHRUSTMAX:		/* �I�[�o�[�g���X�g�}�b�N�X */
			if(sc_data[SC_OVERTHRUST].timer!=-1)
				status_change_end(bl,SC_OVERTHRUST,-1);
			calc_flag = 1;
			break;

		case SC_CHASEWALK_STR:			/* STR�㏸ */
		case SC_BATTLEORDER://�Ր�Ԑ�
			calc_flag = 1;
			break;
		case SC_REGENERATION://����
		case SC_BATTLEORDER_DELAY:
		case SC_REGENERATION_DELAY:
		case SC_RESTORE_DELAY:
		case SC_EMERGENCYCALL_DELAY:
			break;
		case SC_THE_MAGICIAN:
		case SC_STRENGTH:
		case SC_THE_DEVIL:
		case SC_THE_SUN:
		case SC_SPURT://�삯���pSTR
		case SC_SUN_COMFORT://#���z�̈��y#
		case SC_MOON_COMFORT://#���̈��y#
		case SC_STAR_COMFORT://#���̈��y#
		case SC_FUSION://#���z�ƌ��Ɛ��̗Z��#
		case SC_MEAL_INCHIT:	// �H���p
		case SC_MEAL_INCFLEE:
		case SC_MEAL_INCFLEE2:
		case SC_MEAL_INCCRITICAL:
		case SC_MEAL_INCDEF:
		case SC_MEAL_INCMDEF:
		case SC_MEAL_INCATK:
		case SC_MEAL_INCMATK:
			calc_flag = 1;
			break;
		case SC_MEAL_INCSTR:	// �H���p
		case SC_MEAL_INCAGI:
		case SC_MEAL_INCVIT:
		case SC_MEAL_INCINT:
		case SC_MEAL_INCDEX:
		case SC_MEAL_INCLUK:
			// �ۋ������g�p���͌��ʂȂ�
			if(sc_data[type - SC_MEAL_INCSTR + SC_MEAL_INCSTR2].timer != -1)
				return 0;
			calc_flag = 1;
			break;
		case SC_MEAL_INCSTR2:	// �ۋ������p
		case SC_MEAL_INCAGI2:
		case SC_MEAL_INCVIT2:
		case SC_MEAL_INCINT2:
		case SC_MEAL_INCDEX2:
		case SC_MEAL_INCLUK2:
			// �ʏ�̐H���Ƃ͏d�����Ȃ�
			if(sc_data[type - SC_MEAL_INCSTR2 + SC_MEAL_INCSTR].timer != -1)
				status_change_end(bl, type - SC_MEAL_INCSTR2 + SC_MEAL_INCSTR, -1);
			calc_flag = 1;
			break;
		case SC_MEAL_INCEXP:
		case SC_MEAL_INCJOB:
		case SC_COMBATHAN:	// �퓬����
		case SC_LIFEINSURANCE:	// �����ی���
		case SC_FORCEWALKING:
		case SC_TKCOMBO://�e�R���n�p�R���{
		case SC_TRIPLEATTACK_RATE_UP:
		case SC_COUNTER_RATE_UP:
		case SC_WARM://#������#
		case SC_KAIZEL://#�J�C�[��#
		case SC_KAAHI://#�J�A�q#
		case SC_SMA://#�G�X�}#
		case SC_MIRACLE://���z�ƌ��Ɛ��̊��
		case SC_ANGEL://���z�ƌ��Ɛ��̓V�g
		case SC_BABY://�p�p�A�}�}�A��D��
			break;
		case SC_ELEMENTFIELD: //������
			val2 = tick;
			break;
		case SC_RUN://�삯��
			val4 = 0;
			calc_flag = 1;
			break;
		case SC_KAUPE://#�J�E�v#
			val2 = val1*33;
			if(val1>=3)
				val2 = 100;
			break;
		case SC_KAITE://#�J�C�g#
			/* �A�X�����|�����Ă������������ */
			if(sc_data[SC_ASSUMPTIO].timer!=-1)
				status_change_end(bl,SC_ASSUMPTIO,-1);
			//���ˉ�
			if(val1 >= 5) val2 = 2;
			else val2 = 1;
			break;
		case SC_SWOO://#�G�X�E#
			calc_flag = 1;
			if(status_get_mode(bl)&0x20)
				tick /= 5;
			break;
		case SC_SKE://#�G�X�N#
		case SC_SKA://#�G�X�J#
			calc_flag = 1;
			break;
		case SC_MONK://#�����N�̍�#
		case SC_STAR://#�P���Z�C�̍�#
		case SC_SAGE://#�Z�[�W�̍�#
		case SC_CRUSADER://#�N���Z�C�_�[�̍�#
		case SC_WIZARD://#�E�B�U�[�h�̍�#
		case SC_PRIEST://#�v���[�X�g�̍�#
		case SC_ROGUE://#���[�O�̍�#
		case SC_ASSASIN://#�A�T�V���̍�#
		case SC_SOULLINKER://#�\�E�������J�[�̍�#
		case SC_GUNNER:		//�K���X�����K�[�̍�
		case SC_NINJA:			//�E�҂̍�
		case SC_DEATHKINGHT:	//�f�X�i�C�g�̍�
		case SC_COLLECTOR:		//�R���N�^�[�̍�
			if(sd && battle_config.disp_job_soul_state_change) {
				char output[] = "����ԂɂȂ�܂���";
				clif_disp_onlyself(sd,output,strlen(output));
			}
			break;
		case SC_KNIGHT://#�i�C�g�̍�#
		case SC_ALCHEMIST://#�A���P�~�X�g�̍�#
		case SC_BARDDANCER://#�o�[�h�ƃ_���T�[�̍�#
		case SC_BLACKSMITH://#�u���b�N�X�~�X�̍�#
		case SC_HUNTER://#�n���^�[�̍�#
		case SC_HIGH://#�ꎟ��ʐE�Ƃ̍�#
			if(sd && battle_config.disp_job_soul_state_change) {
				char output[] = "����ԂɂȂ�܂���";
				clif_disp_onlyself(sd,output,strlen(output));
			}
			calc_flag = 1;
			break;
		case SC_SUPERNOVICE://#�X�[�p�[�m�[�r�X�̍�#
			if(sd) {
				//1%�Ŏ��S�t���O����
				if(sd->status.base_level >=90 && atn_rand()%10000 < battle_config.repeal_die_counter_rate)
					sd->status.die_counter = 0; //���ɃJ�E���^�[���Z�b�g
				if(battle_config.disp_job_soul_state_change) {
					char output[] = "����ԂɂȂ�܂���";
					clif_disp_onlyself(sd,output,strlen(output));
				}
			}
			calc_flag = 1;
			break;
		case SC_AUTOBERSERK:
		case SC_READYSTORM:
		case SC_READYDOWN:
		case SC_READYTURN:
		case SC_READYCOUNTER:
			tick = 600*1000;
			break;
		case SC_DODGE:
		case SC_DODGE_DELAY:
		case SC_DEVIL:
		case SC_DOUBLECASTING://�_�u���L���X�e�B���O
		case SC_SHRINK://�V�������N
		case SC_WINKCHARM://���f�̃E�B���N
		case SC_TIGEREYE:
		case SC_PK_PENALTY:
		case SC_HERMODE:
			break;
		case SC_CLOSECONFINE://�N���[�Y�R���t�@�C��
		case SC_HOLDWEB://�z�[���h�E�F�u
		case SC_DISARM:				//�f�B�X�A�[��
		case SC_GATLINGFEVER:		//�K�g�����O�t�B�[�o�[
		case SC_FLING:				//�t���C���O
		case SC_MADNESSCANCEL:		// �}�b�h�l�X�L�����Z���[
		case SC_ADJUSTMENT:			// �A�W���X�g�����g
		case SC_INCREASING:			// �C���N���[�W���O�A�L���A���V�[
		case SC_FULLBUSTER:			// �t���o�X�^�[
		case SC_NEN:	// NJ_NEN
		case SC_SUITON:	// NJ_SUITON
			calc_flag = 1;
			break;
		case SC_UTSUSEMI:// NJ_UTSUSEMI
			val3 = (val1+1)/2;
			break;
		case SC_BUNSINJYUTSU:// NJ_BUNSINJYUTSU
			val3 = (val1+1)/2;
			if(sd) {
				val4 = sd->status.clothes_color;
				pc_changelook(sd, LOOK_CLOTHES_COLOR, 0);
			}
			break;
		case SC_TATAMIGAESHI://���Ԃ�
		case SC_NPC_DEFENDER:
			break;
		case SC_AVOID://#�ً}���#
		case SC_CHANGE://#�����^���`�F���W#
		case SC_DEFENCE://#�f�B�t�F���X#
		case SC_BLOODLUST://#�u���b�h���X�g#
		case SC_FLEET://#�t���[�g���[�u#
		case SC_SPEED://#�I�[�o�[�h�X�s�[�h#
			calc_flag = 1;
			break;

		default:
			if(battle_config.error_log)
				printf("UnknownStatusChange [%d]\n", type);
			return 0;
	}

	if(tick <= 0)
		return 0;

	if(bl->type==BL_PC && StatusIconChangeTable[type] != SI_BLANK)
		clif_status_change(bl,StatusIconChangeTable[type],1);	// �A�C�R���\��

	/* option�̕ύX */
	opt_flag = 1;
	switch(type){
		// opt1
		case SC_STONE:
		case SC_FREEZE:
		case SC_STAN:
		case SC_SLEEP:
			unit_stopattack(bl);	/* �U����~ */
			skill_stop_dancing(bl,0);	/* ���t/�_���X�̒��f */
			{	/* �����Ɋ|����Ȃ��X�e�[�^�X�ُ������ */
				int i;
				for(i = SC_STONE; i <= SC_SLEEP; i++){
					if(sc_data[i].timer != -1){
						(*sc_count)--;
						delete_timer(sc_data[i].timer, status_change_timer);
						sc_data[i].timer = -1;
					}
				}
			}
			if(type == SC_STONE)
				*opt1 = 6;
			else
				*opt1 = type - SC_STONE + 1;
			break;
		// opt2
		case SC_POISON:
		case SC_CURSE:
		case SC_SILENCE:
		case SC_CONFUSION:
			*opt2 |= 1<<(type-SC_POISON);
			break;
		case SC_FOGWALLPENALTY:
		case SC_BLIND:
			if(sc_data[SC_FOGWALLPENALTY].timer==-1){
				*opt2 |= 16;
				if(md && !(flag&2))
					md->target_id = 0;
			} else {
				opt_flag = 0;
			}
			break;
		case SC_DPOISON:
			*opt2 |= 128;
			break;
		case SC_SIGNUMCRUCIS:
			*opt2 |= 64;
			break;
		// opt3
		case SC_ONEHAND:		/* 1HQ */
		case SC_TWOHANDQUICKEN:		/* 2HQ */
		case SC_SPEARSQUICKEN:		/* �X�s�A�N�C�b�P�� */
		case SC_CONCENTRATION:		/* �R���Z���g���[�V���� */
			*opt3 |= 1;
			break;
		case SC_OVERTHRUST:		/* �I�[�o�[�g���X�g */
		case SC_SWOO:			/* �G�X�E */
			*opt3 |= 2;
			break;
		case SC_ENERGYCOAT:		/* �G�i�W�[�R�[�g */
		case SC_SKE:			/* �G�X�N */
			*opt3 |= 4;
			break;
		case SC_EXPLOSIONSPIRITS:	/* �����g�� */
			*opt3 |= 8;
			break;
		case SC_STEELBODY:		/* ���� */
		case SC_SKA:			/* �G�X�J */
			*opt3 |= 16;
			break;
		case SC_BLADESTOP:		/* ���n��� */
			*opt3 |= 32;
			break;
		case SC_BERSERK:		/* �o�[�T�[�N */
			*opt3 |= 128;
			break;
		case SC_MARIONETTE:		/* �}���I�l�b�g�R���g���[�� */
		case SC_MARIONETTE2:		/* �}���I�l�b�g�R���g���[�� */
			*opt3 |= 1024;
			break;
		case SC_ASSUMPTIO:		/* �A�X���v�e�B�I */
			*opt3 |= 2048;
			clif_misceffect2(bl,375);
			break;
		case SC_WARM:			/* ������ */
			*opt3 |= 4096;
			break;
		case SC_KAITE:
			*opt3 |= 8192;
			break;
		case SC_MONK:			//#�����N�̍�#
		case SC_STAR:			//#�P���Z�C�̍�#
		case SC_SAGE:			//#�Z�[�W�̍�#
		case SC_CRUSADER:		//#�N���Z�C�_�[�̍�#
		case SC_WIZARD:			//#�E�B�U�[�h�̍�#
		case SC_PRIEST:			//#�v���[�X�g�̍�#
		case SC_ROGUE:			//#���[�O�̍�#
		case SC_ASSASIN:		//#�A�T�V���̍�#
		case SC_SOULLINKER:		//#�\�E�������J�[�̍�#
		case SC_KNIGHT:			//#�i�C�g�̍�#
		case SC_ALCHEMIST:		//#�A���P�~�X�g�̍�#
		case SC_BARDDANCER:		//#�o�[�h�ƃ_���T�[�̍�#
		case SC_BLACKSMITH:		//#�u���b�N�X�~�X�̍�#
		case SC_HUNTER:			//#�n���^�[�̍�#
		case SC_HIGH:			//#�ꎟ��ʐE�Ƃ̍�#
		case SC_SUPERNOVICE:		//#�X�[�p�[�m�[�r�X�̍�#
		case SC_GUNNER:		//�K���X�����K�[�̍�
		case SC_NINJA:			//�E�҂̍�
		case SC_DEATHKINGHT:	//�f�X�i�C�g�̍�
		case SC_COLLECTOR:		//�R���N�^�[�̍�
			*opt3 |= 32768;
			clif_misceffect2(bl,424);
			break;
		// option
		case SC_SIGHT:
			*option |= 1;
			break;
		case SC_HIDING:
			if(bl->type == BL_PC && val3 == 0)	// ���a��łȂ��ʏ�̃n�C�h�Ȃ�A�C�R���\��
				clif_status_change(bl,SI_HIDING,1);
			unit_stopattack(bl);
			*option |= 2;
			break;
		case SC_CLOAKING:
			unit_stopattack(bl);
			*option |= 4;
			break;
		case SC_INVISIBLE:
			unit_stopattack(bl);
			*option |= 64;
			break;
		case SC_REVERSEORCISH:
			*option |= 2048;
			break;
		case SC_WEDDING:
			*option |= 4096;
			break;
		case SC_RUWACH:
			*option |= 8192;
			break;
		case SC_CHASEWALK:
			unit_stopattack(bl);
			*option |= 0x4004;
			break;
		case SC_FUSION:
			*option |= 32768;
			break;
		default:
			opt_flag = 0;
			break;
	}

	if(opt_flag) {	/* option�̕ύX */
		clif_changeoption(bl);
		clif_send_clothcolor(bl);
	}
	(*sc_count)++;	/* �X�e�[�^�X�ُ�̐� */

	sc_data[type].val1 = val1;
	sc_data[type].val2 = val2;
	sc_data[type].val3 = val3;
	sc_data[type].val4 = val4;
	/* �^�C�}�[�ݒ� */
	sc_data[type].timer = add_timer(gettick() + tick, status_change_timer, bl->id, type);

	if(sd && calc_flag && !(flag&4))
		status_calc_pc(sd,0);	/* �X�e�[�^�X�Čv�Z */

	if(hd && calc_flag)
	{
		homun_calc_status(hd);
		clif_send_homstatus(hd->msd,0);
	}
	//�v�Z��ɑ��点��
	switch(type){
		case SC_RUN://�삯��
			// clif_skill_nodamage() �͕K�� clif_status_change() �� clif_walkok() �̊ԂɌĂяo��
			clif_skill_nodamage(bl,bl,TK_RUN,val1,1);
			if(sd) {
				pc_runtodir(sd);
			}
			break;
		case SC_FORCEWALKING:
			unit_forcewalktodir(bl,val4);
			break;
	}

	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�I��
 *------------------------------------------
 */
int status_change_end( struct block_list* bl , int type,int tid)
{
	struct map_session_data *sd = NULL;
	struct status_change* sc_data;
	int opt_flag=0, calc_flag = 0;
	short *sc_count;
	unsigned short *opt1, *opt2;
	unsigned int *opt3, *option;

	nullpo_retr(0, bl);

	if(type < 0)
		return 0;
	if(bl->type!=BL_PC && bl->type!=BL_MOB && bl->type!=BL_HOM) {
		if(battle_config.error_log)
			printf("status_change_end: neither MOB nor PC !\n");
		return 0;
	}

	nullpo_retr(0, sc_data=status_get_sc_data(bl));
	nullpo_retr(0, sc_count=status_get_sc_count(bl));
	nullpo_retr(0, option=status_get_option(bl));
	nullpo_retr(0, opt1=status_get_opt1(bl));
	nullpo_retr(0, opt2=status_get_opt2(bl));
	nullpo_retr(0, opt3=status_get_opt3(bl));

	if(type >= MAX_STATUSCHANGE)
	{
		switch(type)
		{
			case SC_RACECLEAR:
				status_change_race_end(bl,-1);
				break;
			case SC_RESISTCLEAR:
				status_change_resistclear(bl);
				break;
			case SC_SOUL:
			case SC_SOULCLEAR:
				status_change_soulclear(bl);
				break;
			default:
				if(battle_config.error_log)
					printf("UnknownStatusChangeEnd [%d]\n", type);
				break;
		}
		return 0;
	}

	if(bl->type == BL_PC)
		sd = (struct map_session_data *)bl;

	if((*sc_count)>0 && sc_data[type].timer!=-1 &&
		(sc_data[type].timer==tid || tid==-1) ){

		if(tid==-1)	/* �^�C�}����Ă΂�Ă��Ȃ��Ȃ�^�C�}�폜������ */
			delete_timer(sc_data[type].timer,status_change_timer);

		/* �Y���ُ̈�𐳏�ɖ߂� */
		sc_data[type].timer=-1;
		(*sc_count)--;

		switch(type){	/* �ُ�̎�ނ��Ƃ̏��� */
			case SC_PROVOKE:			/* �v���{�b�N */
			case SC_ENDURE:				/* �C���f���A */
			case SC_CONCENTRATE:		/* �W���͌��� */
			case SC_BLESSING:			/* �u���b�V���O */
			case SC_ANGELUS:			/* �A���[���X */
			case SC_INCREASEAGI:		/* ���x�㏸ */
			case SC_DECREASEAGI:		/* ���x���� */
			case SC_SIGNUMCRUCIS:		/* �V�O�i���N���V�X */
			case SC_HIDING:
			case SC_CLOAKING:
			case SC_TWOHANDQUICKEN:		/* 2HQ */
			case SC_ONEHAND:			/* 1HQ */
			case SC_ADRENALINE:			/* �A�h���i�������b�V�� */
			case SC_ENCPOISON:			/* �G���`�����g�|�C�Y�� */
			case SC_IMPOSITIO:			/* �C���|�V�e�B�I�}�k�X */
			case SC_GLORIA:				/* �O�����A */
			case SC_LOUD:				/* ���E�h�{�C�X */
			case SC_MINDBREAKER:		/* �}�C���h�u���[�J�[ */
			case SC_QUAGMIRE:			/* �N�@�O�}�C�A */
			case SC_PROVIDENCE:			/* �v�����B�f���X */
			case SC_SPEARSQUICKEN:		/* �X�s�A�N�C�b�P�� */
			case SC_VOLCANO:
			case SC_DELUGE:
			case SC_VIOLENTGALE:
			case SC_ETERNALCHAOS:		/* �G�^�[�i���J�I�X */
			case SC_DRUMBATTLE:			/* �푾�ۂ̋��� */
			case SC_NIBELUNGEN:			/* �j�[�x�����O�̎w�� */
			case SC_SIEGFRIED:			/* �s���g�̃W�[�N�t���[�h */
			case SC_EXPLOSIONSPIRITS:	// �����g��
			case SC_STEELBODY:			// ����
			case SC_DEFENDER:
			case SC_POISONPOTION:		/* �Ŗ�̕r */
			case SC_SPEEDPOTION0:		/* �����|�[�V���� */
			case SC_SPEEDPOTION1:
			case SC_SPEEDPOTION2:
			case SC_SPEEDPOTION3:
			case SC_RIDING:
			case SC_BLADESTOP_WAIT:
			case SC_CONCENTRATION:		/* �R���Z���g���[�V���� */
			case SC_WINDWALK:		/* �E�C���h�E�H�[�N */
			case SC_TRUESIGHT:		/* �g�D���[�T�C�g */
			case SC_SPIDERWEB:		/* �X�p�C�_�[�E�F�b�u */
			case SC_CARTBOOST:		/* �J�[�g�u�[�X�g */
			case SC_INCATK:		//item 682�p
			case SC_INCMATK:	//item 683�p
			case SC_WEDDING:	//�����p(�����ߏւɂȂ��ĕ����̂��x���Ƃ�)
			case SC_SANTA:
			case SC_INCALLSTATUS:
			case SC_INCHIT:
			case SC_INCFLEE:
			case SC_INCMHP2:
			case SC_INCMSP2:
			case SC_INCATK2:
			case SC_INCHIT2:
			case SC_INCFLEE2:
			//case SC_PRESERVE:
			case SC_OVERTHRUSTMAX:
			case SC_CHASEWALK:	/*�`�F�C�X�E�H�[�N*/
			case SC_CHASEWALK_STR:
			case SC_BATTLEORDER:
			//case SC_THE_MAGICIAN:
			//case SC_STRENGTH:
			//case SC_THE_DEVIL:
			//case SC_THE_SUN:
			case SC_MEAL_INCSTR:	// �H���p
			case SC_MEAL_INCAGI:
			case SC_MEAL_INCVIT:
			case SC_MEAL_INCINT:
			case SC_MEAL_INCDEX:
			case SC_MEAL_INCLUK:
			case SC_MEAL_INCHIT:
			case SC_MEAL_INCFLEE:
			case SC_MEAL_INCFLEE2:
			case SC_MEAL_INCCRITICAL:
			case SC_MEAL_INCDEF:
			case SC_MEAL_INCMDEF:
			case SC_MEAL_INCATK:
			case SC_MEAL_INCMATK:
			case SC_MEAL_INCSTR2:	// �ۋ������p
			case SC_MEAL_INCAGI2:
			case SC_MEAL_INCVIT2:
			case SC_MEAL_INCINT2:
			case SC_MEAL_INCDEX2:
			case SC_MEAL_INCLUK2:
			case SC_SPURT:
			case SC_SUN_COMFORT://#���z�̈��y#
			case SC_MOON_COMFORT://#���̈��y#
			case SC_STAR_COMFORT://#���̈��y#
			case SC_FUSION://#���z�ƌ��Ɛ��̗Z��#
			case SC_ADRENALINE2://#�t���A�h���i�������b�V��#
			case SC_RESISTWATER:
			case SC_RESISTGROUND:
			case SC_RESISTFIRE:
			case SC_RESISTWIND:
			case SC_RESISTPOISON:
			case SC_RESISTHOLY:
			case SC_RESISTDARK:
			case SC_RESISTTELEKINESIS:
			case SC_RESISTUNDEAD:
			case SC_RESISTALL:
			case SC_INVISIBLE:
			case SC_TIGEREYE:
			case SC_TAROTCARD:
			case SC_DISARM:				/* �f�B�X�A�[�� */
			case SC_GATLINGFEVER:		/* �K�g�����O�t�B�[�o�[ */
			case SC_FLING:				/* �t���C���O */
			case SC_MADNESSCANCEL:		/* �}�b�h�l�X�L�����Z���[ */
			case SC_ADJUSTMENT:			/* �A�W���X�g�����g */
			case SC_INCREASING:			/* �C���N���[�W���O�A�L���A���V�[ */
			case SC_FULLBUSTER:			/* �t���o�X�^�[ */
				calc_flag = 1;
				break;
			case SC_ELEMENTWATER:		// ��
			case SC_ELEMENTGROUND:		// �y
			case SC_ELEMENTFIRE:		// ��
			case SC_ELEMENTWIND:		// ��
			case SC_ELEMENTHOLY:		// ��
			case SC_ELEMENTDARK:		// ��
			case SC_ELEMENTELEKINESIS:	// �O
			case SC_ELEMENTPOISON:		// ��
			//case SC_ELEMENTUNDEAD:		// �s��
				if(sd){
					clif_displaymessage(sd->fd,"�h��̑��������ɖ߂�܂���");
				}
				break;
			case SC_RACEUNKNOWN:
			case SC_RACEUNDEAD:
			case SC_RACEBEAST:
			case SC_RACEPLANT:
			case SC_RACEINSECT:
			case SC_RACEFISH:
			case SC_RACEDEVIL:
			case SC_RACEHUMAN:
			case SC_RACEANGEL:
			case SC_RACEDRAGON:
				if(sd)
					clif_displaymessage(sd->fd,"�푰�����ɖ߂�܂���");
				break;
			case SC_RUN://�삯��
				unit_stop_walking(bl,0);
				calc_flag = 1;
				break;
			case SC_MONK://#�����N�̍�#
			case SC_STAR://#�P���Z�C�̍�#
			case SC_SAGE://#�Z�[�W�̍�#
			case SC_CRUSADER://#�N���Z�C�_�[�̍�#
			case SC_WIZARD://#�E�B�U�[�h�̍�#
			case SC_PRIEST://#�v���[�X�g�̍�#
			case SC_ROGUE://#���[�O�̍�#
			case SC_ASSASIN://#�A�T�V���̍�#
			case SC_SOULLINKER://#�\�E�������J�[�̍�#
			case SC_GUNNER:		//�K���X�����K�[�̍�
			case SC_NINJA:			//�E�҂̍�
			case SC_DEATHKINGHT:	//�f�X�i�C�g�̍�
			case SC_COLLECTOR:		//�R���N�^�[�̍�
				if(sd && battle_config.disp_job_soul_state_change) {
					char output[] = "����Ԃ��I�����܂���";
					clif_disp_onlyself(sd,output,strlen(output));
				}
				break;
			case SC_KNIGHT://#�i�C�g�̍�#
			case SC_ALCHEMIST://#�A���P�~�X�g�̍�#
			case SC_BARDDANCER://#�o�[�h�ƃ_���T�[�̍�#
			case SC_BLACKSMITH://#�u���b�N�X�~�X�̍�#
			case SC_HUNTER://#�n���^�[�̍�#
			case SC_HIGH://#�ꎟ��ʐE�Ƃ̍�#
				if(sd && battle_config.disp_job_soul_state_change) {
					char output[] = "����Ԃ��I�����܂���";
					clif_disp_onlyself(sd,output,strlen(output));
				}
				calc_flag = 1;
				break;
			case SC_SUPERNOVICE://#�X�[�p�[�m�[�r�X�̍�#
				if(sd) {
					if(battle_config.disp_job_soul_state_change) {
						char output[] = "����Ԃ��I�����܂���";
						clif_disp_onlyself(sd,output,strlen(output));
					}
				}
				break;
			case SC_POEMBRAGI:			/* �u���M */
			case SC_WHISTLE:			/* ���J */
			case SC_ASSNCROS:			/* �[�z�̃A�T�V���N���X */
			case SC_APPLEIDUN:			/* �C�h�D���̗ь� */
			case SC_HUMMING:			/* �n�~���O */
			case SC_DONTFORGETME:		/* ����Y��Ȃ��� */
			case SC_FORTUNE:			/* �K�^�̃L�X */
			case SC_SERVICE4U:			/* �T�[�r�X�t�H�[���[ */
				calc_flag = 1;
				//�x�艉�t�����Z�b�g
				if(sc_data[type + SC_WHISTLE_ - SC_WHISTLE].timer==-1)
					status_change_start(bl,type + SC_WHISTLE_ - SC_WHISTLE,sc_data[type].val1,
						sc_data[type].val2,sc_data[type].val3,sc_data[type].val4,battle_config.dance_and_play_duration,0);
				break;
			case SC_WHISTLE_:			/* ���J */
			case SC_ASSNCROS_:			/* �[�z�̃A�T�V���N���X */
			case SC_APPLEIDUN_:			/* �C�h�D���̗ь� */
			case SC_HUMMING_:			/* �n�~���O */
			case SC_DONTFORGETME_:		/* ����Y��Ȃ��� */
			case SC_FORTUNE_:			/* �K�^�̃L�X */
			case SC_SERVICE4U_:			/* �T�[�r�X�t�H�[���[ */
			case SC_GRAVITATION:
				calc_flag = 1;
				break;
			case SC_MARIONETTE:			/* �}���I�l�b�g�R���g���[�� */
			case SC_MARIONETTE2:			/* �}���I�l�b�g�R���g���[�� */
				{
					struct block_list *tbl = map_id2bl(sc_data[type].val2);
					if(tbl) {
						struct status_change *t_sc_data = status_get_sc_data(tbl);
						int tmp = (type==SC_MARIONETTE)? SC_MARIONETTE2: SC_MARIONETTE;
						//�������}���I�l�b�g��ԂȂ炢������ɉ���
						if(t_sc_data && t_sc_data[tmp].timer!=-1)
							status_change_end(tbl, tmp, -1);
					}
				}
				calc_flag = 1;
				break;
			case SC_BERSERK:			/* �o�[�T�[�N */
				calc_flag = 1;
				clif_status_change(bl,SC_INCREASEAGI,0);	/* �A�C�R������ */
				break;
			case SC_DEVOTION:		/* �f�B�{�[�V���� */
				{
					struct map_session_data *dsd = map_id2sd(sc_data[type].val1);
					sc_data[type].val1=sc_data[type].val2=0;
					if(dsd)
						skill_devotion(dsd);
					calc_flag = 1;
				}
				break;
			case SC_BLADESTOP:
				{
					struct block_list *tbl = map_id2bl(sc_data[type].val4);
					if(tbl) {
						struct status_change *t_sc_data = status_get_sc_data(tbl);
						//�Е����؂ꂽ�̂ő���̔��n��Ԃ��؂�ĂȂ��̂Ȃ����
						if(t_sc_data && t_sc_data[SC_BLADESTOP].timer!=-1)
							status_change_end(tbl,SC_BLADESTOP,-1);
						if(sc_data[type].val2==2)
							clif_bladestop(map_id2bl(sc_data[type].val3),tbl->id,0);
					}
				}
				break;
			case SC_CLOSECONFINE://���H���p
				{
					struct block_list *tbl = map_id2bl(sc_data[type].val4);
					if(tbl) {
						struct status_change *t_sc_data = status_get_sc_data(tbl);
						//�Е����؂ꂽ�̂ő����CLOSECONFINE��Ԃ��؂�ĂȂ��̂Ȃ����
						if(t_sc_data && t_sc_data[SC_CLOSECONFINE].timer!=-1)
							status_change_end(tbl,SC_CLOSECONFINE,-1);
					}
					if(sc_data[type].val2==2)
					{
						//��������ȏ�������H
					}
					calc_flag = 1;
				}
				break;
			case SC_HOLDWEB:
				{
					struct block_list *tbl = map_id2bl(sc_data[type].val4);
					if(tbl) {
						struct status_change *t_sc_data = status_get_sc_data(tbl);
						//�Е����؂ꂽ�̂ő����SC_HOLDWEB��Ԃ��؂�ĂȂ��̂Ȃ����
						if(t_sc_data && t_sc_data[SC_HOLDWEB].timer!=-1)
							status_change_end(tbl,SC_HOLDWEB,-1);
					}
					calc_flag = 1;
				}
				break;
			case SC_DANCING:
				{
					struct map_session_data *dsd;
					//�����肾�������ŃA�C�R������
					if(sc_data[type].val1==CG_MOONLIT)
						clif_status_change(bl,SI_MOONLIT,0);	// �A�C�R������

					if(sc_data[type].val4 && (dsd=map_id2sd(sc_data[type].val4))){
						//���t�ő��肪����ꍇ�����val4��0�ɂ���
						if(dsd->sc_data && dsd->sc_data[type].timer!=-1)
							dsd->sc_data[type].val4=0;
					}
					if(sc_data[SC_LONGINGFREEDOM].timer!=-1)
						status_change_end(bl,SC_LONGINGFREEDOM,-1);
				}
				calc_flag = 1;
				break;
			case SC_GOSPEL:
			case SC_GRAFFITI:
			case SC_WARM:
				{
					struct skill_unit_group *sg = (struct skill_unit_group *)sc_data[type].val4;	// val4��group_id
					sc_data[type].val4 = 0;
					if(sg)
						skill_delunitgroup(sg);
				}
				break;
			case SC_NOCHAT:	//�`���b�g�֎~���
				if(sd)
					clif_updatestatus(sd,SP_MANNER);
				break;
			case SC_SPLASHER:		/* �x�i���X�v���b�V���[ */
				{
					struct block_list *src=map_id2bl(sc_data[type].val3);
					if(src && tid!=-1){
						//�����Ƀ_���[�W������3*3�Ƀ_���[�W
						skill_castend_damage_id(src, bl,sc_data[type].val2,sc_data[type].val1,gettick(),0 );
					}
				}
				break;
			case SC_ANKLE:
				{
					struct skill_unit_group *sg = (struct skill_unit_group *)sc_data[SC_ANKLE].val2;
					// skill_delunitgroup����status_change_end ���Ă΂�Ȃ��ׂɁA
					// ��[�������Ă��Ȃ����ɂ��Ă���O���[�v�폜����B
					if(sg) {
						sg->val2 = 0;
						skill_delunitgroup(sg);
					}
				}
				break;
			case SC_SELFDESTRUCTION:	/* ���� */
				unit_stop_walking(bl,5);
				if(bl->type == BL_MOB) {
					struct mob_data *md = (struct mob_data *)bl;
					if(md) {
						md->mode &= ~0x1;
						md->state.special_mob_ai = 2;
					}
				}
				break;
			case SC_TATAMIGAESHI://���Ԃ�
			case SC_UTSUSEMI://#NJ_UTSUSEMI#
				break;
			case SC_BUNSINJYUTSU://#NJ_BUNSINJYUTSU#
				if(sd) {
					int color = sc_data[SC_BUNSINJYUTSU].val4;
					if(color > 0)
						pc_changelook(sd, LOOK_CLOTHES_COLOR, color);
				}
				break;
			case SC_SUITON://#NJ_SUITON#
			case SC_NEN://#NJ_NEN#
			case SC_AVOID://#�ً}���#
			case SC_CHANGE://#�����^���`�F���W#
			case SC_DEFENCE://#�f�B�t�F���X#
			case SC_BLOODLUST://#�u���b�h���X�g#
			case SC_FLEET://#�t���[�g���[�u#
			case SC_SPEED://#�I�[�o�[�h�X�s�[�h#
				calc_flag = 1;
				break;
			/* option1 */
			case SC_FREEZE:
				sc_data[type].val3 = 0;
				break;

			/* option2 */
			case SC_POISON:				/* �� */
			case SC_BLIND:				/* �Í� */
			case SC_CURSE:
				calc_flag = 1;
				break;
		}

		if(bl->type==BL_PC && StatusIconChangeTable[type] != SI_BLANK)
			clif_status_change(bl,StatusIconChangeTable[type],0);	// �A�C�R������

		opt_flag = 1;
		switch(type){	/* ����ɖ߂�Ƃ��Ȃɂ��������K�v */
		// opt1
		case SC_STONE:
		case SC_FREEZE:
		case SC_STAN:
		case SC_SLEEP:
			*opt1 = 0;
			break;
		// opt2
		case SC_POISON:
			*opt2 &= ~1;
			break;
		case SC_CURSE:
		case SC_SILENCE:
		case SC_CONFUSION:
			*opt2 &= ~(1<<(type-SC_POISON));
			break;
		case SC_FOGWALLPENALTY:
			if(sc_data[SC_BLIND].timer==-1)
				*opt2 &= ~16;
			else
				opt_flag = 0;
			break;
		case SC_BLIND:
			if(sc_data[SC_FOGWALLPENALTY].timer==-1)
				*opt2 &= ~16;
			else
				opt_flag = 0;
			break;
		case SC_DPOISON:
			*opt2 &= ~128;
			break;
		case SC_SIGNUMCRUCIS:
			*opt2 &= ~64;
			break;
		// opt3
		case SC_ONEHAND:		/* 1HQ */
		case SC_TWOHANDQUICKEN:		/* 2HQ */
		case SC_SPEARSQUICKEN:		/* �X�s�A�N�C�b�P�� */
		case SC_CONCENTRATION:		/* �R���Z���g���[�V���� */
			*opt3 &= ~1;
			break;
		case SC_OVERTHRUST:		/* �I�[�o�[�g���X�g */
		case SC_SWOO:			/* �G�X�E */
			*opt3 &= ~2;
			break;
		case SC_ENERGYCOAT:		/* �G�i�W�[�R�[�g */
		case SC_SKE:			/* �G�X�N */
			*opt3 &= ~4;
			break;
		case SC_EXPLOSIONSPIRITS:	/* �����g�� */
			*opt3 &= ~8;
			break;
		case SC_STEELBODY:		/* ���� */
		case SC_SKA:			/* �G�X�J */
			*opt3 &= ~16;
			break;
		case SC_BLADESTOP:		/* ���n��� */
			*opt3 &= ~32;
			break;
		case SC_BERSERK:		/* �o�[�T�[�N */
			*opt3 &= ~128;
			break;
		case SC_MARIONETTE:		/* �}���I�l�b�g�R���g���[�� */
		case SC_MARIONETTE2:		/* �}���I�l�b�g�R���g���[�� */
			*opt3 &= ~1024;
			break;
		case SC_ASSUMPTIO:		/* �A�X���v�e�B�I */
			*opt3 &= ~2048;
			break;
		case SC_WARM:			/* ������ */
			*opt3 &= ~4096;
			break;
		case SC_KAITE:
			*opt3 &= ~8192;
			break;
		case SC_MONK:			//#�����N�̍�#
		case SC_STAR:			//#�P���Z�C�̍�#
		case SC_SAGE:			//#�Z�[�W�̍�#
		case SC_CRUSADER:		//#�N���Z�C�_�[�̍�#
		case SC_WIZARD:			//#�E�B�U�[�h�̍�#
		case SC_PRIEST:			//#�v���[�X�g�̍�#
		case SC_ROGUE:			//#���[�O�̍�#
		case SC_ASSASIN:		//#�A�T�V���̍�#
		case SC_SOULLINKER:		//#�\�E�������J�[�̍�#
		case SC_KNIGHT:			//#�i�C�g�̍�#
		case SC_ALCHEMIST:		//#�A���P�~�X�g�̍�#
		case SC_BARDDANCER:		//#�o�[�h�ƃ_���T�[�̍�#
		case SC_BLACKSMITH:		//#�u���b�N�X�~�X�̍�#
		case SC_HUNTER:			//#�n���^�[�̍�#
		case SC_HIGH:			//#�ꎟ��ʐE�Ƃ̍�#
		case SC_SUPERNOVICE:		//#�X�[�p�[�m�[�r�X�̍�#
		case SC_GUNNER:		//�K���X�����K�[�̍�
		case SC_NINJA:			//�E�҂̍�
		case SC_DEATHKINGHT:	//�f�X�i�C�g�̍�
		case SC_COLLECTOR:		//�R���N�^�[�̍�
			*opt3 &= ~32768;
			break;
		// option
		case SC_SIGHT:
			*option &= ~1;
			break;
		case SC_HIDING:
			if(bl->type == BL_PC && sc_data[type].val3 == 0)	// ���a��łȂ��ʏ�̃n�C�h�Ȃ�A�C�R������
				clif_status_change(bl,SI_HIDING,0);
			*option &= ~2;
			break;
		case SC_CLOAKING:
			*option &= ~4;
			break;
		case SC_INVISIBLE:
			*option &= ~64;
			break;
		case SC_REVERSEORCISH:
			*option &= ~2048;
			break;
		case SC_WEDDING:	//�����p(�����ߏւɂȂ��ĕ����̂��x���Ƃ�)
			*option &= ~4096;
			break;
		case SC_RUWACH:
			*option &= ~8192;
			break;
		case SC_CHASEWALK:	/*�`�F�C�X�E�H�[�N*/
			*option &= ~0x4004;
			break;
		case SC_FUSION:
			*option &= ~32768;
			break;
		default:
			opt_flag = 0;
			break;
		}

		if(opt_flag) {	/* option�̕ύX */
			clif_changeoption(bl);
			clif_send_clothcolor(bl);
		}

		if(sd // && !(flag&4) //�����ǉ�����Ɨ��_��������Ǝv������
		 	&& (calc_flag || sd->auto_status_calc_pc[type]==1))
		{
			status_calc_pc(sd,0);	/* �X�e�[�^�X�Čv�Z */
		}
		if(bl->type==BL_HOM && calc_flag)
		{
			homun_calc_status((struct homun_data*)bl);
			clif_send_homstatus(((struct homun_data*)bl)->msd,0);
		}

	}

	return 0;
}

/*==========================================
 * �X�e�[�^�X�Čv�Z���ꎞ��~����
 *------------------------------------------
 */
int status_calc_pc_stop_begin(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC)
		((struct map_session_data *)bl)->stop_status_calc_pc++;
	return 0;
}

/*==========================================
 * �X�e�[�^�X�Čv�Z���ĊJ����
 *------------------------------------------
 */
int status_calc_pc_stop_end(struct block_list *bl)
{
	nullpo_retr(0, bl);
	if(bl->type==BL_PC){
		struct map_session_data *sd = (struct map_session_data *)bl;
		sd->stop_status_calc_pc--;
		if(sd->stop_status_calc_pc==0 && sd->call_status_calc_pc_while_stopping>0)
			status_calc_pc(sd,0);
		if(sd->stop_status_calc_pc<0){
			puts("status_calc_pc_stop_end���s���ɌĂяo����Ă��܂�");
		}
	}
	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�J�n�^�C�}�[
 *------------------------------------------
 */
static int status_pretimer(int tid, unsigned int tick, int id, int data)
{
	struct block_list *bl = map_id2bl(id);
	struct status_pretimer *stpt = NULL;
	struct unit_data *ud;

	if(bl == NULL)
		return 0;	//�Y��ID�����łɏ��ł��Ă���

	if(bl->prev == NULL) {
		aFree(stpt);
		return 0;
	}

	nullpo_retr(0, ud = unit_bl2ud(bl));

	stpt = (struct status_pretimer*)data;
	linkdb_erase(&ud->statuspretimer, stpt);

	stpt->timer = -1;

	if(stpt->target_id){
		struct block_list *target = map_id2bl(stpt->target_id);
		if( target == NULL || bl->m != target->m || unit_isdead(bl) || unit_isdead(target) ) {
			aFree(stpt);
			return 0;
		}
	}else{
		if(bl->m != stpt->map){
			aFree(stpt);
			return 0;
		}
	}

	status_change_start(bl, stpt->type, stpt->val1, stpt->val2, stpt->val3, stpt->val4, stpt->tick, stpt->flag);
	aFree(stpt);

	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�J�n�^�C�}�[�̍폜
 *------------------------------------------
 */
int status_clearpretimer(struct block_list *bl)
{
	struct unit_data *ud;
	struct linkdb_node *node1, *node2;

	nullpo_retr(0, bl);
	nullpo_retr(0, ud = unit_bl2ud(bl));

	node1 = ud->statuspretimer;
	while(node1){
		struct status_pretimer *stpt = (struct status_pretimer *)node1->data;
		if(stpt->timer != -1){
			delete_timer(stpt->timer, status_pretimer);
		}
		node2 = node1->next;
		aFree(stpt);
		node1 = node2;
	}
	linkdb_final(&ud->statuspretimer);

	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�J�n�^�C�}�[�̃Z�b�g
 *------------------------------------------
 */
int status_change_pretimer(struct block_list *bl,int type,int val1,int val2,int val3,int val4,int tick,int flag,int pre_tick)
{
	struct unit_data *ud;
	struct status_pretimer *stpt;
	nullpo_retr(1, bl);
	nullpo_retr(1, ud = unit_bl2ud(bl));

	stpt = (struct status_pretimer *)aCalloc(1, sizeof(struct status_pretimer));
	stpt->timer = add_timer(pre_tick, status_pretimer, bl->id, (int)stpt);
	stpt->target_id = bl->id;
	stpt->map = bl->m;
	stpt->type = type;
	stpt->val1 = val1;
	stpt->val2 = val2;
	stpt->val3 = val3;
	stpt->val4 = val4;
	stpt->tick = tick;
	stpt->flag = flag;

	linkdb_insert(&ud->statuspretimer, stpt, stpt);

	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�I���^�C�}�[
 * 
 * �\�[�X���C��������ւ̒���
 * 
 * �E��Ԉُ�p�����ɂ́Aadd_timer() ���������return ���邱��
 * �E��Ԉُ�I�����ɂ́A�֐��̍Ō�ɂ���status_change_end() ��
 * �@�Ăяo���O�� return ���Ȃ�����
 * 
 * ���̂Q�_������Ă��Ȃ��ƁA���l�̏�Ԉُ킪����ɉ������ꂽ��A
 * delete_timer error���o�Ă���Ȃǂ̃o�O���������܂��B
 *------------------------------------------
 */
int status_change_timer(int tid, unsigned int tick, int id, int data)
{
	int type=data;
	struct block_list *bl;
	struct map_session_data *sd=NULL;
	struct status_change *sc_data;
	//short *sc_count; //�g���ĂȂ��H

	if(type == -1) return 0;

	if( (bl=map_id2bl(id)) == NULL )
		return 0; //�Y��ID�����łɏ��ł��Ă���Ƃ����̂͂����ɂ����肻���Ȃ̂ŃX���[���Ă݂�
	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if(bl->type==BL_PC)
		sd=(struct map_session_data *)bl;

	//sc_count=status_get_sc_count(bl); //�g���ĂȂ��H

	if(sc_data[type].timer != tid) {
		if(battle_config.error_log)
			printf("status_change_timer %d != %d (type = %d)\n",tid,sc_data[type].timer,type);
		return 0;
	}

	switch(type){	/* ����ȏ����ɂȂ�ꍇ */
	case SC_MAXIMIZEPOWER:	/* �}�L�V�}�C�Y�p���[ */
		if(sd){
			if( sd->status.sp > 0 ){	/* SP�؂��܂Ŏ��� */
				sd->status.sp--;
				clif_updatestatus(sd,SP_SP);
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
					sc_data[type].val2+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;
	case SC_CLOAKING:		/* �N���[�L���O */
	case SC_INVISIBLE:		/* �C���r�W�u�� */
		if(sd){
			if( sd->status.sp > 0 ){	/* SP�؂��܂Ŏ��� */
				sd->status.sp--;
				clif_updatestatus(sd,SP_SP);
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
					sc_data[type].val2+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;

	case SC_CHASEWALK:	/*�`�F�C�X�E�H�[�N*/
		if(sd){
			int sp = 10+sc_data[SC_CHASEWALK].val1*2;
			if (map[sd->bl.m].flag.gvg) sp *= 5;
			if (sd->status.sp > sp){
				sd->status.sp -= sp; // update sp cost [Celest]
				clif_updatestatus(sd,SP_SP);
				if ((++sc_data[SC_CHASEWALK].val4) == 1) {

					//���[�O�̍�
					if(sd->sc_data[SC_ROGUE].timer!=-1)
						status_change_start(bl, SC_CHASEWALK_STR, 1<<(sc_data[SC_CHASEWALK].val1-1), 0, 0, 0, 300000, 0);
					else status_change_start(bl, SC_CHASEWALK_STR, 1<<(sc_data[SC_CHASEWALK].val1-1), 0, 0, 0, 30000, 0);
					status_calc_pc(sd, 0);
				}
				sc_data[type].timer = add_timer( /* �^�C�}?�Đݒ� */
					sc_data[type].val2+tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;

	case SC_HIDING:		/* �n�C�f�B���O */
		if(sd){		/* SP�������āA���Ԑ����̊Ԃ͎��� */
			if( sd->status.sp > 0 && (--sc_data[type].val2)>0 ){
				if(sc_data[type].val2 % (sc_data[type].val1+3) ==0 ){
					sd->status.sp--;
					clif_updatestatus(sd,SP_SP);
				}
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
					1000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;
	case SC_SIGHTBLASTER:
	case SC_SIGHT:	/* �T�C�g */
	case SC_RUWACH:	/* ���A�t */
	case SC_DETECTING:
		{
			//const int range=10; �C���O�͗���10x10�������̂ł�
			// �X�L���C��: �T�C�g�͈̔�=7x7 ���A�t�͈̔�=5x5
			int range = 2;
			if ( type == SC_SIGHT || type == SC_SIGHTBLASTER) range = 3;

			map_foreachinarea( status_change_timer_sub,
				bl->m, bl->x-range, bl->y-range, bl->x+range,bl->y+range,0,
				bl,type,tick);

			if( (--sc_data[type].val2)>0 ){
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
					250+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;

	case SC_SIGNUMCRUCIS:		/* �V�O�i���N���V�X */
		{
			int race = status_get_race(bl);
			if(race == RCT_DEMON || battle_check_undead(race,status_get_elem_type(bl))) {
				sc_data[type].timer=add_timer(1000*600+tick,status_change_timer, bl->id, data );
				return 0;
			}
		}
		break;

	case SC_PROVOKE:	/* �v���{�b�N/�I�[�g�o�[�T�[�N */
		if(sc_data[type].val2!=0){	/* �I�[�g�o�[�T�[�N�i�P�b���Ƃ�HP�`�F�b�N�j */
			if(sd && sd->status.hp>sd->status.max_hp>>2)	/* ��~ */
				break;
			sc_data[type].timer=add_timer( 1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		break;

	case SC_DISSONANCE:	/* �s���a�� */
		if( (--sc_data[type].val2)>0){
			struct skill_unit *unit = map_id2su(sc_data[type].val4);
			struct block_list *src;
			if(!unit || !unit->group)
				break;
			src=map_id2bl(unit->group->src_id);
			if(!src)
				break;
			battle_skill_attack(BF_MISC,src,&unit->bl,bl,unit->group->skill_id,sc_data[type].val1,tick,0);
			sc_data[type].timer=add_timer(skill_get_time2(unit->group->skill_id,unit->group->skill_lv)+tick,
				status_change_timer, bl->id, data );
			return 0;
		}
		break;
	case SC_UGLYDANCE:	/* ��������ȃ_���X */
		if( (--sc_data[type].val2)>0){
			struct skill_unit *unit = map_id2su(sc_data[type].val4);
			struct block_list *src;
			if(!unit || !unit->group)
				break;
			src=map_id2bl(unit->group->src_id);
			if(!src)
				break;
			skill_additional_effect(src,bl,unit->group->skill_id,sc_data[type].val1,0,tick);
			sc_data[type].timer=add_timer(skill_get_time2(unit->group->skill_id,unit->group->skill_lv)+tick,
				status_change_timer, bl->id, data );
			return 0;
		}
		break;

	case SC_LULLABY:	/* �q��S */
		if( (--sc_data[type].val2)>0){
			struct skill_unit *unit = map_id2su(sc_data[type].val4);
			if(!unit || !unit->group || unit->group->src_id==bl->id)
				break;
			skill_additional_effect(bl,bl,unit->group->skill_id,sc_data[type].val1,BF_LONG|BF_SKILL|BF_MISC,tick);
			sc_data[type].timer=add_timer(skill_get_time(unit->group->skill_id,unit->group->skill_lv)/10+tick,
				status_change_timer, bl->id, data );
			return 0;
		}
		break;

	case SC_STONE:
		if(sc_data[type].val2 != 0) {
			unsigned short *opt1 = status_get_opt1(bl);
			sc_data[type].val2 = 0;
			sc_data[type].val4 = 0;
			unit_stop_walking(bl,1);
			if(opt1) {
				*opt1 = 1;
				clif_changeoption(bl);
				clif_send_clothcolor(bl);
			}
			sc_data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		else if( (--sc_data[type].val3) > 0) {
			int hp = status_get_max_hp(bl);
			if((++sc_data[type].val4)%5 == 0 && status_get_hp(bl) > hp>>2) {
				hp = (hp < 100)? 1: hp/100;
				if(sd)
					pc_heal(sd,-hp,0);
				else if(bl->type == BL_MOB){
					struct mob_data *md = (struct mob_data *)bl;
					if(md)
						md->hp -= hp;
				}
				else if(bl->type == BL_HOM){
					struct homun_data *hd = (struct homun_data *)bl;
					if(hd)
						homun_heal(hd,-hp,0);
				}
			}
			sc_data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		break;
	case SC_POISON:
		if (sc_data[SC_SLOWPOISON].timer == -1 && (--sc_data[type].val3) > 0) {
			int hp = status_get_hp(bl), p_dmg = status_get_max_hp(bl);
			if (hp > p_dmg>>2) {		// �ő�HP��25%�ȏ�
				p_dmg = 3 + p_dmg*3/200;
				if(p_dmg >= hp) 
					p_dmg = hp-1;	// �łł͎��ȂȂ�
				unit_heal(bl, -p_dmg, 0);
			}
		}
		if (sc_data[type].val3 > 0) {
			sc_data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		break;
	case SC_DPOISON:
		if (sc_data[SC_SLOWPOISON].timer == -1 && (--sc_data[type].val3) > 0) {
			int hp = status_get_max_hp(bl);
			if (status_get_hp(bl) > hp>>2) {
				hp = 3 + hp/50;
				unit_heal(bl, -hp, 0);
			}
		}
		if (sc_data[type].val3 > 0) {
			sc_data[type].timer=add_timer(1000+tick,status_change_timer, bl->id, data );
			return 0;
		}
		break;
	case SC_BLEED:
		if (--sc_data[type].val3 > 0) {
			int dmg = atn_rand()%600 + 200;
			if(bl->type == BL_MOB) {
				struct mob_data *md = (struct mob_data *)bl;
				if(md)
					md->hp = (md->hp - dmg < 50)? 50: md->hp - dmg;	// mob��HP50�ȉ��ɂȂ�Ȃ�
			} else {
				unit_heal(bl, -dmg, 0);
			}
			if(!unit_isdead(bl)) {
				sc_data[type].timer = add_timer(10000+tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;
	case SC_TENSIONRELAX:	/* �e���V���������b�N�X */
		if(sd){		/* SP�������āAHP�����^���łȂ���Όp�� */
			if( sd->status.sp > 12 && sd->status.max_hp > sd->status.hp ){
				if(sc_data[type].val2 % (sc_data[type].val1+3) ==0 ){
					sd->status.sp -= 12;
					clif_updatestatus(sd,SP_SP);
				}
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
					10000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
			if(sd->status.max_hp <= sd->status.hp)
				status_change_end(&sd->bl,SC_TENSIONRELAX,-1);
		}
		break;

	/* ���Ԑ؂ꖳ���H�H */
	case SC_AETERNA:
	case SC_TRICKDEAD:
	case SC_RIDING:
	case SC_FALCON:
	case SC_WEIGHT50:
	case SC_WEIGHT90:
	case SC_MAGICPOWER:		/* ���@�͑��� */
	case SC_REJECTSWORD:	/* ���W�F�N�g�\�[�h */
	case SC_MEMORIZE:		/* �������C�Y */
	case SC_SACRIFICE:		/* �T�N���t�@�C�X */
	case SC_READYSTORM:
	case SC_READYDOWN:
	case SC_READYTURN:
	case SC_READYCOUNTER:
	case SC_DODGE:
	case SC_AUTOBERSERK:
	case SC_RUN:
	case SC_MARIONETTE:
	case SC_MARIONETTE2:
		sc_data[type].timer=add_timer( 1000*600+tick,status_change_timer, bl->id, data );
		return 0;
	case SC_DEVIL:
		clif_status_change(bl,SI_DEVIL,1);
		sc_data[type].timer=add_timer( 1000*5+tick,status_change_timer, bl->id, data );
		return 0;
	case SC_LONGINGFREEDOM:
		if(sd && sd->status.sp >= 3) {
			if (--sc_data[type].val3 <= 0)
			{
				sd->status.sp -= 3;
				clif_updatestatus(sd, SP_SP);
				sc_data[type].val3 = 3;
			}
			sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
				1000 + tick, status_change_timer, bl->id, data);
			return 0;
		}
		break;
	case SC_DANCING: //�_���X�X�L���̎���SP����
		if(sd) {
			int s=0, cost=1;
			if(sc_data[type].val1 == CG_HERMODE)
				cost = 5;
			if(sd->status.sp < cost)
				skill_stop_dancing(&sd->bl,0);
			else if(--sc_data[type].val3 > 0){
				switch(sc_data[type].val1){
				case BD_RICHMANKIM:				/* �j�����h�̉� 3�b��SP1 */
				case BD_DRUMBATTLEFIELD:		/* �푾�ۂ̋��� 3�b��SP1 */
				case BD_RINGNIBELUNGEN:			/* �j�[�x�����O�̎w�� 3�b��SP1 */
				case BD_SIEGFRIED:				/* �s���g�̃W�[�N�t���[�h 3�b��SP1 */
				case BA_DISSONANCE:				/* �s���a�� 3�b��SP1 */
				case BA_ASSASSINCROSS:			/* �[�z�̃A�T�V���N���X 3�b��SP1 */
				case DC_UGLYDANCE:				/* ��������ȃ_���X 3�b��SP1 */
					s=3;
					break;
				case BD_LULLABY:				/* �q��� 4�b��SP1 */
				case BD_ETERNALCHAOS:			/* �i���̍��� 4�b��SP1 */
				case BD_ROKISWEIL:				/* ���L�̋��� 4�b��SP1 */
				case DC_FORTUNEKISS:			/* �K�^�̃L�X 4�b��SP1 */
					s=4;
					break;
				case BD_INTOABYSS:				/* �[���̒��� 5�b��SP1 */
				case BA_WHISTLE:				/* ���J 5�b��SP1 */
				case DC_HUMMING:				/* �n�~���O 5�b��SP1 */
				case BA_POEMBRAGI:				/* �u���M�̎� 5�b��SP1 */
				case DC_SERVICEFORYOU:			/* �T�[�r�X�t�H�[���[ 5�b��SP1 */
				case CG_HERMODE:				//�w�����[�h�̏�
					s=5;
					break;
				case BA_APPLEIDUN:				/* �C�h�D���̗ь� 6�b��SP1 */
					s=6;
					break;
				case DC_DONTFORGETME:			/* ����Y��Ȃ��Łc 10�b��SP1 */
				case CG_MOONLIT:				/* ������̉��� 10�b��SP1�H */
					s=10;
				case CG_LONGINGFREEDOM:
					s=3;
					break;
				}
				if(s && ((sc_data[type].val3 % s) == 0)){
					sd->status.sp-=cost;
					clif_updatestatus(sd,SP_SP);
				}
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
					1000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;
	case SC_BERSERK:		/* �o�[�T�[�N */
		if(sd){		/* HP��100�ȏ�Ȃ�p�� */
			if( (sd->status.hp - (sd->status.max_hp*5)/100) > 100 ){
				sd->status.hp -= (sd->status.max_hp*5)/100;
				clif_updatestatus(sd,SP_HP);
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
					10000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
			else
				sd->status.hp = 100;
		}
		break;
	case SC_NOCHAT:	//�`���b�g�֎~���
		if(sd){
			time_t timer;
			if((++sd->status.manner) && time(&timer) < ((sc_data[type].val2) + 60*(0-sd->status.manner))){	//�J�n����status.manner���o���ĂȂ��̂Ōp��
				clif_updatestatus(sd,SP_MANNER);
				sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ�(60�b) */
					60000+tick, status_change_timer,
					bl->id, data);
				return 0;
			}
		}
		break;
	case SC_SELFDESTRUCTION:		/* ���� */
		if(bl->type == BL_MOB) {
			struct mob_data *md = (struct mob_data *)bl;
			if(md && unit_iscasting(&md->bl) && md->state.special_mob_ai > 2 && md->mode&0x1 && md->speed > 0) {
				md->speed -= 5;
				if(md->speed <= 0)
					md->speed = 1;
				md->dir = sc_data[type].val4;
				unit_walktodir(&md->bl,1);	// ���x���ς��̂Ŗ���Ăяo��

				/* �^�C�}�[�Đݒ� */
				sc_data[type].timer = add_timer(100+tick, status_change_timer, bl->id, data);
				return 0;
			}
		}
		break;
	}

	if(sd && sd->state.dead_sit!=1 && sd->eternal_status_change[type]>0)
	{
		sc_data[type].timer=add_timer(	/* �^�C�}�[�Đݒ� */
			sd->eternal_status_change[type]+tick, status_change_timer,
			bl->id, data);
		return 0;
	}

	return status_change_end( bl,type,tid );
}

/*==========================================
 * �X�e�[�^�X�ُ�^�C�}�[�͈͏���
 *------------------------------------------
 */
int status_change_timer_sub(struct block_list *bl, va_list ap )
{
	struct block_list *src;
	int type;
	unsigned int tick;
	unsigned int *opt;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, src=va_arg(ap,struct block_list*));
	type=va_arg(ap,int);
	tick=va_arg(ap,unsigned int);

	if(type == -1) return 0;
	if(bl->type!=BL_PC && bl->type!=BL_MOB)
		return 0;

	opt = status_get_option(bl);

	switch( type ){
	case SC_SIGHT:	/* �T�C�g */
	case SC_CONCENTRATE:
		if( *opt&0x06 ){
			status_change_end( bl, SC_HIDING, -1);
			status_change_end( bl, SC_CLOAKING, -1);
		}
		break;
	case SC_RUWACH:	/* ���A�t */
		if( *opt&0x06 ){
			status_change_end( bl, SC_HIDING, -1);
			status_change_end( bl, SC_CLOAKING, -1);
			if(battle_check_target( src,bl, BCT_ENEMY ) > 0) {
				struct status_change *sc_data = status_get_sc_data(src);
				if(sc_data)
					battle_skill_attack(BF_MAGIC,src,src,bl,AL_RUWACH,sc_data[type].val1,tick,0);
			}
		}
		break;
	case SC_SIGHTBLASTER:
		if( *opt&0x06 ){
			status_change_end( bl, SC_HIDING, -1);
			status_change_end( bl, SC_CLOAKING, -1);
		}
		if(battle_check_target( src,bl, BCT_ENEMY ) > 0 && !unit_isdead(bl)) {
			struct status_change *sc_data = status_get_sc_data(src);
			if(sc_data) {
				battle_skill_attack(BF_MAGIC,src,src,bl,WZ_SIGHTBLASTER,sc_data[type].val1,tick,0);
				sc_data[type].val2 = 0;
			}
			status_change_end(src,SC_SIGHTBLASTER,-1);
		}
		break;
	case SC_DETECTING:
		if( *opt&0x46 ){
			status_change_end( bl, SC_HIDING, -1);
			status_change_end( bl, SC_CLOAKING, -1);
			status_change_end( bl, SC_INVISIBLE, -1);
		}
		break;
	}
	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�S����
 *------------------------------------------
 */
int status_change_clear(struct block_list *bl,int type)
{
	struct status_change* sc_data;
	short *sc_count;
	unsigned short *opt1, *opt2;
	unsigned int *opt3, *option;
	int i;

	nullpo_retr(0, bl);
#ifdef DYNAMIC_SC_DATA
	if(status_check_dummy_sc_data(bl))
		return 0;
#endif
	nullpo_retr(0, sc_data=status_get_sc_data(bl));
	nullpo_retr(0, sc_count=status_get_sc_count(bl));
	nullpo_retr(0, option=status_get_option(bl));
	nullpo_retr(0, opt1=status_get_opt1(bl));
	nullpo_retr(0, opt2=status_get_opt2(bl));
	nullpo_retr(0, opt3=status_get_opt3(bl));

	if(*sc_count == 0)
		return 0;
	status_calc_pc_stop_begin(bl);
	for(i = 0; i < MAX_STATUSCHANGE; i++) {
		if(i==SC_BABY || i==SC_REDEMPTIO)
		{
			if(type == 0 && unit_isdead(bl))
				continue;
		}
		if(sc_data[i].timer != -1)	/* �ُ킪����Ȃ�^�C�}�[���폜���� */
			status_change_end(bl,i,-1);
	}
	status_calc_pc_stop_end(bl);

	*opt1 = 0;
	*opt2 = 0;
	*opt3 = 0;
	*option &= OPTION_MASK;

	if(type != 1) {
		clif_changeoption(bl);
		clif_send_clothcolor(bl);
	}

	return 0;
}

/*==========================================
 * ����������ɂ�����X�e�[�^�X�ُ����
 *------------------------------------------
 */
int status_change_release(struct block_list *bl,int mask)
{
	struct status_change* sc_data;
	short *sc_count;
	int i;

	nullpo_retr(0, bl);
#ifdef DYNAMIC_SC_DATA
	if(status_check_dummy_sc_data(bl))
		return 0;
#endif
	nullpo_retr(0, sc_data=status_get_sc_data(bl));
	nullpo_retr(0, sc_count=status_get_sc_count(bl));

	if(*sc_count <= 0)
		return 0;

	status_calc_pc_stop_begin(bl);
	for(i = 0; i < MAX_STATUSCHANGE; i++) {
		// �ُ킪�����Ċ��t���O������Ȃ�^�C�}�[���폜
		if( (scdata_db[i].releasable & mask) && sc_data[i].timer != -1 ) {
			if(i == SC_DANCING) {
				skill_stop_dancing(bl,0);
			} else {
				if(i == SC_BASILICA)
					skill_basilica_cancel(bl);
				status_change_end(bl,i,-1);
			}
		}
	}
	status_calc_pc_stop_end(bl);

	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�(����̑���)�I��
 *------------------------------------------
 */
int status_encchant_eremental_end(struct block_list *bl,int type)
{
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if( type!=SC_ENCPOISON && sc_data[SC_ENCPOISON].timer!=-1 )	/* �G���`�����g�|�C�Y������ */
		status_change_end(bl,SC_ENCPOISON,-1);
	if( type!=SC_ASPERSIO && sc_data[SC_ASPERSIO].timer!=-1 )	/* �A�X�y���V�I���� */
		status_change_end(bl,SC_ASPERSIO,-1);
	if( type!=SC_FLAMELAUNCHER && sc_data[SC_FLAMELAUNCHER].timer!=-1 )	/* �t���C�������`������ */
		status_change_end(bl,SC_FLAMELAUNCHER,-1);
	if( type!=SC_FROSTWEAPON && sc_data[SC_FROSTWEAPON].timer!=-1 )	/* �t���X�g�E�F�|������ */
		status_change_end(bl,SC_FROSTWEAPON,-1);
	if( type!=SC_LIGHTNINGLOADER && sc_data[SC_LIGHTNINGLOADER].timer!=-1 )	/* ���C�g�j���O���[�_�[���� */
		status_change_end(bl,SC_LIGHTNINGLOADER,-1);
	if( type!=SC_SEISMICWEAPON && sc_data[SC_SEISMICWEAPON].timer!=-1 )	/* �T�C�X�~�b�N�E�F�|������ */
		status_change_end(bl,SC_SEISMICWEAPON,-1);
	if( type!=SC_DARKELEMENT && sc_data[SC_DARKELEMENT].timer!=-1 )		//��
		status_change_end(bl,SC_DARKELEMENT,-1);
	if( type!=SC_ATTENELEMENT && sc_data[SC_ATTENELEMENT].timer!=-1 )	//�O
		status_change_end(bl,SC_ATTENELEMENT,-1);
	if( type!=SC_UNDEADELEMENT && sc_data[SC_UNDEADELEMENT].timer!=-1 )	//�s��
		status_change_end(bl,SC_UNDEADELEMENT,-1);

	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�(�̂̑���)�I��
 *------------------------------------------
 */
int status_enchant_armor_eremental_end(struct block_list *bl,int type)
{
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if( type!=SC_BENEDICTIO && sc_data[SC_BENEDICTIO].timer!=-1 )//����
		status_change_end(bl,SC_BENEDICTIO,-1);
	if( type!=SC_ELEMENTWATER && sc_data[SC_ELEMENTWATER].timer!=-1 )	//��
		status_change_end(bl,SC_ELEMENTWATER,-1);
	if( type!=SC_ELEMENTGROUND && sc_data[SC_ELEMENTGROUND].timer!=-1 )	//�n
		status_change_end(bl,SC_ELEMENTGROUND,-1);
	if( type!=SC_ELEMENTWIND && sc_data[SC_ELEMENTWIND].timer!=-1 )		//��
		status_change_end(bl,SC_ELEMENTWIND,-1);
	if( type!=SC_ELEMENTFIRE && sc_data[SC_ELEMENTFIRE].timer!=-1 )		//��
		status_change_end(bl,SC_ELEMENTFIRE,-1);
	if( type!=SC_ELEMENTHOLY && sc_data[SC_ELEMENTHOLY].timer!=-1 )	//��
		status_change_end(bl,SC_ELEMENTHOLY,-1);
	if( type!=SC_ELEMENTDARK && sc_data[SC_ELEMENTDARK].timer!=-1 )		//��
		status_change_end(bl,SC_ELEMENTDARK,-1);
	if( type!=SC_ELEMENTELEKINESIS && sc_data[SC_ELEMENTELEKINESIS].timer!=-1 )	//�O
		status_change_end(bl,SC_ELEMENTELEKINESIS,-1);
	if( type!=SC_ELEMENTPOISON && sc_data[SC_ELEMENTPOISON].timer!=-1 )	//��
		status_change_end(bl,SC_ELEMENTPOISON,-1);
	if( type!=SC_ELEMENTUNDEAD && sc_data[SC_ELEMENTUNDEAD].timer!=-1 )	//�s��
		status_change_end(bl,SC_ELEMENTUNDEAD,-1);

	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�(�푰�ύX)�I��
 *------------------------------------------
 */
int status_change_race_end(struct block_list *bl,int type)
{
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if( type!=SC_RACEUNDEAD && sc_data[SC_RACEUNDEAD].timer!=-1 )
		status_change_end(bl,SC_RACEUNDEAD,-1);
	if( type!=SC_RACEBEAST && sc_data[SC_RACEBEAST].timer!=-1 )
		status_change_end(bl,SC_RACEBEAST,-1);
	if( type!=SC_RACEPLANT && sc_data[SC_RACEPLANT].timer!=-1 )
		status_change_end(bl,SC_RACEPLANT,-1);
	if( type!=SC_RACEINSECT && sc_data[SC_RACEINSECT].timer!=-1 )
		status_change_end(bl,SC_RACEINSECT,-1);
	if( type!=SC_RACEFISH && sc_data[SC_RACEFISH].timer!=-1 )
		status_change_end(bl,SC_RACEFISH,-1);
	if( type!=SC_RACEDEVIL && sc_data[SC_RACEDEVIL].timer!=-1 )
		status_change_end(bl,SC_RACEDEVIL,-1);
	if( type!=SC_RACEHUMAN && sc_data[SC_RACEHUMAN].timer!=-1 )
		status_change_end(bl,SC_RACEHUMAN,-1);
	if( type!=SC_RACEANGEL && sc_data[SC_RACEANGEL].timer!=-1 )
		status_change_end(bl,SC_RACEANGEL,-1);
	if( type!=SC_RACEDRAGON && sc_data[SC_RACEDRAGON].timer!=-1 )
		status_change_end(bl,SC_RACEDRAGON,-1);

	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�(�푰�ύX)�I��
 *------------------------------------------
 */
int status_change_resistclear(struct block_list *bl)
{
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	status_calc_pc_stop_begin(bl);
	// �ϐ�
	if(sc_data[SC_RESISTWATER].timer!=-1)
		status_change_end(bl,SC_RESISTWATER,-1);
	if(sc_data[SC_RESISTGROUND].timer!=-1)
		status_change_end(bl,SC_RESISTGROUND,-1);
	if(sc_data[SC_RESISTFIRE].timer!=-1)
		status_change_end(bl,SC_RESISTFIRE,-1);
	if(sc_data[SC_RESISTWIND].timer!=-1)
		status_change_end(bl,SC_RESISTWIND,-1);
	if(sc_data[SC_RESISTPOISON].timer!=-1)
		status_change_end(bl,SC_RESISTPOISON,-1);
	if(sc_data[SC_RESISTHOLY].timer!=-1)
		status_change_end(bl,SC_RESISTHOLY,-1);
	if(sc_data[SC_RESISTDARK].timer!=-1)
		status_change_end(bl,SC_RESISTDARK,-1);
	if(sc_data[SC_RESISTTELEKINESIS].timer!=-1)
		status_change_end(bl,SC_RESISTTELEKINESIS,-1);
	if(sc_data[SC_RESISTUNDEAD].timer!=-1)
		status_change_end(bl,SC_RESISTUNDEAD,-1);
	status_calc_pc_stop_end(bl);

	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�(��)�J�n
 *------------------------------------------
 */
int status_change_soulstart(struct block_list *bl,int val1,int val2,int val3,int val4,int tick,int flag)
{
	int type = -1;
	struct pc_base_job s_class;
	nullpo_retr(0, bl);
	if(bl->type!=BL_PC)
		return 0;
	s_class = pc_calc_base_job(status_get_class(bl));

	switch(s_class.job){
		case 15:
			type = SC_MONK;
			break;
		case 25:
			type = SC_STAR;
			break;
		case 16:
			type = SC_SAGE;
		 	break;
		case 14:
			type = SC_CRUSADER;
			break;
		case 9:
			type = SC_WIZARD;
			break;
		case 8:
			type = SC_PRIEST;
			break;
		case 17:
			type = SC_ROGUE;
			break;
		case 12:
			type = SC_ASSASIN;
			break;
		case 27:
			type = SC_SOULLINKER;
			break;
		case 7:
			type = SC_KNIGHT;
			break;
		case 18:
			type = SC_ALCHEMIST;
			break;
		case 19:
		case 20:
			type = SC_BARDDANCER;
			break;
		case 10:
			type = SC_BLACKSMITH;
			break;
		case 11:
			type = SC_HUNTER;
			break;
		case 23:
			type = SC_SUPERNOVICE;
			break;
		case 28:
			type = SC_GUNNER;
			break;
		case 29:
			type = SC_NINJA;
			break;
		case 30:
			type = SC_DEATHKINGHT;
			break;
		case 31:
			type = SC_COLLECTOR;
			break;
		default:
			if(s_class.upper==1 && s_class.job>=1 && s_class.job<=6)
				type = SC_HIGH;
			break;
	}
	if(type >= 0)
		status_change_start(bl,type,val1,val2,val3,val4,tick,flag);
	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�(��)�I��
 *------------------------------------------
 */
int status_change_soulclear(struct block_list *bl)
{
	struct status_change *sc_data;

	nullpo_retr(0, bl);
	nullpo_retr(0, sc_data=status_get_sc_data(bl));

	if(sc_data[SC_MONK].timer!=-1)
		status_change_end(bl,SC_MONK,-1);
	if(sc_data[SC_STAR].timer!=-1)
		status_change_end(bl,SC_STAR,-1);
	if(sc_data[SC_SAGE].timer!=-1)
		status_change_end(bl,SC_SAGE,-1);
	if(sc_data[SC_CRUSADER].timer!=-1)
		status_change_end(bl,SC_CRUSADER,-1);
	if(sc_data[SC_WIZARD].timer!=-1)
		status_change_end(bl,SC_WIZARD,-1);
	if(sc_data[SC_PRIEST].timer!=-1)
		status_change_end(bl,SC_PRIEST,-1);
	if(sc_data[SC_ROGUE].timer!=-1)
		status_change_end(bl,SC_ROGUE,-1);
	if(sc_data[SC_ASSASIN].timer!=-1)
		status_change_end(bl,SC_ASSASIN,-1);
	if(sc_data[SC_SOULLINKER].timer!=-1)
		status_change_end(bl,SC_SOULLINKER,-1);
	if(sc_data[SC_KNIGHT].timer!=-1)
		status_change_end(bl,SC_KNIGHT,-1);
	if(sc_data[SC_ALCHEMIST].timer!=-1)
		status_change_end(bl,SC_ALCHEMIST,-1);
	if(sc_data[SC_BARDDANCER].timer!=-1)
		status_change_end(bl,SC_BARDDANCER,-1);
	if(sc_data[SC_BLACKSMITH].timer!=-1)
		status_change_end(bl,SC_BLACKSMITH,-1);
	if(sc_data[SC_HUNTER].timer!=-1)
		status_change_end(bl,SC_HUNTER,-1);
	if(sc_data[SC_HIGH].timer!=-1)
		status_change_end(bl,SC_HIGH,-1);
	if(sc_data[SC_SUPERNOVICE].timer!=-1)
		status_change_end(bl,SC_SUPERNOVICE,-1);
	if(sc_data[SC_GUNNER].timer!=-1)
		status_change_end(bl,SC_GUNNER,-1);
	if(sc_data[SC_NINJA].timer!=-1)
		status_change_end(bl,SC_NINJA,-1);
	if(sc_data[SC_DEATHKINGHT].timer!=-1)
		status_change_end(bl,SC_DEATHKINGHT,-1);
	if(sc_data[SC_COLLECTOR].timer!=-1)
		status_change_end(bl,SC_COLLECTOR,-1);
	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�(�����E�Ή��E����)�I��
 *------------------------------------------
 */
int status_change_attacked_end(struct block_list *bl)
{
	struct status_change *sc_data;

	nullpo_retr(0, bl);

	sc_data = status_get_sc_data(bl);
	if(sc_data) {
		if(sc_data[SC_FREEZE].timer!=-1)
			status_change_end(bl,SC_FREEZE,-1);
		if(sc_data[SC_STONE].timer!=-1 && sc_data[SC_STONE].val2==0)
			status_change_end(bl,SC_STONE,-1);
		if(sc_data[SC_SLEEP].timer!=-1)
			status_change_end(bl,SC_SLEEP,-1);
	}
	return 0;
}

/*==========================================
 * �X�e�[�^�X�ُ�(�n�C�h)�I��
 *------------------------------------------
 */
int status_change_hidden_end(struct block_list *bl)
{
	unsigned int *option;

	nullpo_retr(0, bl);

	option = status_get_option(bl);

	if (option && *option > 0) {
		if ((*option) & 0x02)
			status_change_end(bl,SC_HIDING,-1);
		if (((*option) & 0x4004) == 4)
			status_change_end(bl,SC_CLOAKING,-1);
		if (((*option) & 0x4004) == 0x4004)
			status_change_end(bl,SC_CHASEWALK,-1);
	}
	return 0;
}


/*==========================================
 * �f�[�^�x�[�X�ǂݍ���
 *------------------------------------------
 */
static int status_calc_sigma(void)
{
	int i,j,k;

	for(i=0;i<MAX_PC_CLASS;i++) {
		memset(job_db[i].hp_sigma,0,sizeof(job_db[i].hp_sigma));
		for(k=0,j=2;j<=MAX_LEVEL;j++) {
			k += job_db[i].hp_coefficient*j + 50;
			k -= k%100;
			job_db[i].hp_sigma[j-1] = k;
		}
	}
	return 0;
}

int status_readdb(void) {
	int i,j,k;
	FILE *fp;
	char line[1024],*p;

	memset(&job_db, 0, sizeof(job_db));

	// JOB�␳���l�P
	fp=fopen("db/job_db1.txt","r");
	if(fp==NULL){
		printf("can't read db/job_db1.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		char *split[50];
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<27 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(j<27)
			continue;
		job_db[i].max_weight_base = atoi(split[0]);
		job_db[i].hp_coefficient  = atoi(split[1]);
		job_db[i].hp_coefficient2 = atoi(split[2]);
		job_db[i].sp_coefficient  = atoi(split[3]);
		for(j=0;j<23 && j<WT_MAX;j++)
			job_db[i].aspd_base[j] = atoi(split[j+4]);
		i++;
		if(i >= MAX_VALID_PC_CLASS)
			break;
	}
	fclose(fp);
	status_calc_sigma();
	printf("read db/job_db1.txt done\n");

	// JOB�{�[�i�X
	fp=fopen("db/job_db2.txt","r");
	if(fp==NULL){
		printf("can't read db/job_db2.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<MAX_LEVEL && p;j++){
			if(sscanf(p,"%d",&k)==0)
				break;
			job_db[i].bonus[0][j] = k;
			job_db[i].bonus[2][j] = k; //�{�q�E�̃{�[�i�X�͕�����Ȃ��̂ŉ�
			p=strchr(p,',');
			if(p) p++;
		}
		i++;
		if(i >= MAX_VALID_PC_CLASS)
			break;
	}
	fclose(fp);
	printf("read db/job_db2.txt done\n");

	// JOB�{�[�i�X2 �]���E�p
	fp=fopen("db/job_db2-2.txt","r");
	if(fp==NULL){
		printf("can't read db/job_db2-2.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		if(line[0]=='/' && line[1]=='/')
			continue;
		for(j=0,p=line;j<MAX_LEVEL && p;j++){
			if(sscanf(p,"%d",&k)==0)
				break;
			job_db[i].bonus[1][j]=k;
			p=strchr(p,',');
			if(p) p++;
		}
		i++;
		if(i >= MAX_VALID_PC_CLASS)
			break;
	}
	fclose(fp);
	printf("read db/job_db2-2.txt done\n");

	// ���B�f�[�^�e�[�u��
	for(i=0;i<5;i++){
		refine_db[i].safety_bonus = 0;
		refine_db[i].over_bonus   = 0;
		refine_db[i].limit        = MAX_REFINE;
		for(j=0;j<MAX_REFINE;j++)
			refine_db[i].per[j] = 0;
	}
	fp=fopen("db/refine_db.txt","r");
	if(fp==NULL){
		printf("can't read db/refine_db.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		char *split[MAX_REFINE+3];
		if(line[0]=='/' && line[1]=='/')
			continue;
		if(atoi(line)<=0)
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<MAX_REFINE+3 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		refine_db[i].safety_bonus = atoi(split[0]);	// ���B�{�[�i�X
		refine_db[i].over_bonus   = atoi(split[1]);	// �ߏ萸�B�{�[�i�X
		refine_db[i].limit        = atoi(split[2]);	// ���S���B���E
		for(j=0;j<MAX_REFINE && split[j+3];j++)
			refine_db[i].per[j] = atoi(split[j+3]);
		if(++i > MAX_WEAPON_LEVEL)
			break;
	}
	fclose(fp);
		printf("read db/refine_db.txt done\n");

	// �T�C�Y�␳�e�[�u��
	for(i=0;i<MAX_SIZE_FIX;i++)
		for(j=0;j<WT_MAX;j++)
			atkmods[i][j]=100;
	fp=fopen("db/size_fix.txt","r");
	if(fp==NULL){
		printf("can't read db/size_fix.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		char *split[WT_MAX];
		if(line[0]=='/' && line[1]=='/')
			continue;
		if(atoi(line)<=0)
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<WT_MAX && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		for(j=0;j<WT_MAX && split[j];j++)
			atkmods[i][j]=atoi(split[j]);
		if(++i > MAX_SIZE_FIX)
			break;
	}
	fclose(fp);
	printf("read db/size_fix.txt done\n");

	// �X�e�[�^�X�ُ�e�[�u��
	memset(&scdata_db, 0, sizeof(scdata_db));
	fp=fopen("db/scdata_db.txt","r");
	if(fp==NULL){
		printf("can't read db/scdata_db.txt\n");
		return 1;
	}
	i=0;
	while(fgets(line,1020,fp)){
		char *split[5];
		if(line[0] == '/' && line[1] == '/')
			continue;
		if(line[0] == '\r' || line[0] == '\n')
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<5 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		j = atoi(split[0]);
		if(j < 0 || j >= MAX_STATUSCHANGE)
			continue;
		scdata_db[j].save       = (short)atoi(split[2]);
		scdata_db[j].releasable = atoi(split[3]);
		scdata_db[j].disable    = atoi(split[4]);
		i++;
	}
	fclose(fp);
	printf("read db/scdata_db.txt done (count=%d)\n",i);

	for(i=0;i<MAX_STATUSCHANGE;i++) {
		dummy_sc_data[i].timer=-1;
		dummy_sc_data[i].val1 = dummy_sc_data[i].val2 = dummy_sc_data[i].val3 = dummy_sc_data[i].val4 = 0;
	}

#ifdef DYNAMIC_SC_DATA
	puts("status_readdb: enable dynamic sc_data.");
#endif
	return 0;
}

/*==========================================
 * �X�L���֌W����������
 *------------------------------------------
 */
int do_init_status(void)
{
	add_timer_func_list(status_change_timer,"status_change_timer");
	add_timer_func_list(status_pretimer,"status_pretimer");
	status_readdb();
	return 0;
}
