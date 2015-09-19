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
#include "nullpo.h"
#include "malloc.h"
#include "mmo.h"
#include "utils.h"

#include "guild.h"
#include "skill.h"
#include "map.h"
#include "clif.h"
#include "pc.h"
#include "pet.h"
#include "mob.h"
#include "battle.h"
#include "party.h"
#include "itemdb.h"
#include "script.h"
#include "intif.h"
#include "status.h"
#include "date.h"
#include "unit.h"
#include "homun.h"
#include "atcommand.h"
#include "ranking.h"
#include "npc.h"
#include "merc.h"

#define SKILLUNITTIMER_INVERVAL	100

/* �X�L���ԍ������X�e�[�^�X�ُ�ԍ��ϊ��e�[�u�� */
int SkillStatusChangeTable[MAX_SKILL] = {	/* status.h��enum��SC_***�Ƃ��킹�邱�� */
	/* 0- */
	-1,-1,-1,-1,-1,-1,SC_PROVOKE,SC_MAGNUM,SC_ENDURE,-1,
	/* 10- */
	SC_SIGHT,-1,SC_SAFETYWALL,-1,-1,SC_FREEZE,SC_STONE,-1,-1,-1,
	/* 20- */
	-1,-1,-1,-1,SC_RUWACH,SC_PNEUMA,-1,-1,-1,SC_INCREASEAGI,
	/* 30- */
	SC_DECREASEAGI,-1,SC_SIGNUMCRUCIS,SC_ANGELUS,SC_BLESSING,-1,-1,-1,-1,-1,
	/* 40- */
	-1,-1,-1,-1,-1,SC_CONCENTRATE,SC_DOUBLE,-1,-1,-1,
	/* 50- */
	-1,SC_HIDING,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 60- */
	SC_TWOHANDQUICKEN,SC_AUTOCOUNTER,-1,-1,-1,-1,SC_IMPOSITIO,SC_SUFFRAGIUM,SC_ASPERSIO,SC_BENEDICTIO,
	/* 70- */
	-1,SC_SLOWPOISON,-1,SC_KYRIE,SC_MAGNIFICAT,SC_GLORIA,SC_SILENCE,-1,SC_AETERNA,-1,
	/* 80- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 90- */
	-1,-1,SC_QUAGMIRE,-1,-1,-1,-1,-1,-1,-1,
	/* 100- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 110- */
	-1,SC_ADRENALINE,SC_WEAPONPERFECTION,SC_OVERTHRUST,SC_MAXIMIZEPOWER,-1,-1,-1,-1,-1,
	/* 120- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 130- */
	-1,-1,-1,-1,-1,SC_CLOAKING,SC_STUN,-1,SC_ENCPOISON,SC_POISONREACT,
	/* 140- */
	SC_POISON,SC_SPLASHER,-1,SC_TRICKDEAD,-1,-1,SC_AUTOBERSERK,-1,-1,-1,
	/* 150- */
	-1,-1,-1,-1,-1,SC_LOUD,-1,SC_ENERGYCOAT,-1,-1,
	/* 160- */
	-1,-1,SC_ELEMENTWATER,SC_ELEMENTGROUND,SC_ELEMENTFIRE,SC_ELEMENTWIND,SC_ELEMENTPOISON,SC_ELEMENTHOLY,SC_ELEMENTDARK,SC_ELEMENTELEKINESIS,
	/* 170- */
	-1,-1,-1,SC_SELFDESTRUCTION,-1,-1,-1,-1,-1,-1,
	/* 180- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 190- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 200- */
	-1,SC_KEEPING,-1,-1,SC_BARRIER,SC_NPC_DEFENDER,-1,SC_HALLUCINATION,-1,-1,
	/* 210- */
	-1,-1,-1,-1,-1,SC_STRIPWEAPON,SC_STRIPSHIELD,SC_STRIPARMOR,SC_STRIPHELM,-1,
	/* 220- */
	SC_GRAFFITI,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 230- */
	-1,-1,-1,-1,SC_CP_WEAPON,SC_CP_SHIELD,SC_CP_ARMOR,SC_CP_HELM,-1,-1,
	/* 240- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,SC_AUTOGUARD,
	/* 250- */
	-1,-1,SC_REFLECTSHIELD,-1,-1,SC_DEVOTION,SC_PROVIDENCE,SC_DEFENDER,SC_SPEARQUICKEN,-1,
	/* 260- */
	-1,-1,-1,-1,-1,-1,-1,-1,SC_STEELBODY,SC_BLADESTOP_WAIT,
	/* 270- */
	SC_EXPLOSIONSPIRITS,SC_EXTREMITYFIST,-1,-1,-1,-1,SC_MAGICROD,-1,-1,-1,
	/* 280- */
	SC_FLAMELAUNCHER,SC_FROSTWEAPON,SC_LIGHTNINGLOADER,SC_SEISMICWEAPON,-1,SC_VOLCANO,SC_DELUGE,SC_VIOLENTGALE,-1,-1,
	/* 290- */
	-1,-1,-1,-1,SC_REVERSEORCISH,-1,-1,-1,-1,-1,
	/* 300- */
	-1,-1,-1,-1,-1,-1,SC_LULLABY,SC_RICHMANKIM,SC_ETERNALCHAOS,SC_DRUMBATTLE,
	/* 310- */
	SC_NIBELUNGEN,SC_ROKISWEIL,SC_INTOABYSS,SC_SIEGFRIED,-1,-1,-1,SC_DISSONANCE,-1,SC_WHISTLE,
	/* 320- */
	SC_ASSNCROS,SC_POEMBRAGI,SC_APPLEIDUN,-1,-1,SC_UGLYDANCE,-1,SC_HUMMING,SC_DONTFORGETME,SC_FORTUNE,
	/* 330- */
	SC_SERVICE4U,SC_SELFDESTRUCTION,-1,-1,-1,SC_WE_FEMALE,-1,-1,-1,-1,
	/* 340- */
	-1,-1,SC_STOP,-1,-1,-1,-1,-1,SC_ELEMENTUNDEAD,SC_EXPLOSIONSPIRITS,
	/* 350- */
	SC_INCFLEE,-1,-1,SC_INVISIBLE,-1,SC_AURABLADE,SC_PARRYING,SC_CONCENTRATION,SC_TENSIONRELAX,SC_BERSERK,
	/* 360- */
	-1,SC_ASSUMPTIO,SC_BASILICA,-1,-1,-1,SC_MAGICPOWER,-1,SC_SACRIFICE,SC_GOSPEL,
	/* 370- */
	-1,SC_TIGERFIST,-1,-1,-1,-1,-1,-1,SC_EDP,-1,
	/* 380- */
	SC_TRUESIGHT,-1,-1,SC_WINDWALK,SC_MELTDOWN,-1,-1,SC_CARTBOOST,-1,SC_CHASEWALK,
	/* 390- */
	SC_REJECTSWORD,-1,-1,-1,-1,-1,SC_MARIONETTE,-1,SC_HEADCRUSH,SC_JOINTBEAT,
	/* 400- */
	-1,-1,SC_MINDBREAKER,SC_MEMORIZE,SC_FOGWALL,SC_SPIDERWEB,-1,-1,SC_BABY,-1,
	/* 410- */
	-1,SC_RUN,SC_READYSTORM,-1,SC_READYDOWN,-1,SC_READYTURN,-1,SC_READYCOUNTER,-1,
	/* 420- */
	SC_DODGE,-1,-1,-1,-1,SC_SEVENWIND,-1,-1,SC_WARM,SC_WARM,
	/* 430- */
	SC_WARM,SC_SUN_COMFORT,SC_MOON_COMFORT,SC_STAR_COMFORT,-1,-1,-1,-1,-1,-1,
	/* 440- */
	-1,-1,-1,-1,SC_FUSION,SC_ALCHEMIST,-1,SC_MONK,SC_STAR,SC_SAGE,
	/* 450- */
	SC_CRUSADER,SC_SUPERNOVICE,SC_KNIGHT,SC_WIZARD,SC_PRIEST,SC_BARDDANCER,SC_ROGUE,SC_ASSASIN,SC_BLACKSMITH,SC_ADRENALINE2,
	/* 460- */
	SC_HUNTER,SC_SOULLINKER,SC_KAIZEL,SC_KAAHI,SC_KAUPE,SC_KAITE,-1,-1,-1,SC_SMA,
	/* 470- */
	SC_SWOO,SC_SKE,SC_SKA,-1,SC_MODECHANGE,SC_PRESERVE,-1,-1,-1,-1,
	/* 480- */
	-1,-1,SC_DOUBLECASTING,-1,SC_GRAVITATION_USER,-1,SC_OVERTHRUSTMAX,SC_LONGINGFREEDOM,SC_HERMODE,-1,
	/* 490- */
	-1,-1,-1,-1,SC_HIGH,SC_ONEHAND,-1,-1,-1,-1,
	/* 500- */
	-1,SC_FLING,-1,-1,SC_MADNESSCANCEL,SC_ADJUSTMENT,SC_INCREASING,-1,-1,-1,
	/* 510- */
	-1,-1,-1,SC_DISARM,-1,-1,-1,SC_GATLINGFEVER,-1,SC_FULLBUSTER,
	/* 520- */
	-1,-1,-1,-1,-1,-1,-1,SC_TATAMIGAESHI,SC_HIDING,-1,
	/* 530- */
	-1,SC_UTSUSEMI,SC_BUNSINJYUTSU,-1,-1,-1,-1,-1,SC_SUITON,-1,
	/* 540- */
	-1,-1,-1,SC_NEN,-1,-1,-1,-1,-1,-1,
	/* 550- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 600- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 650- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 660- */
	-1,-1,-1,SC_SILENCE,SC_FREEZE,SC_BLEED,SC_STONE,SC_CONFUSION,SC_SLEEP,SC_SIGHT,
	/* 670- */
	-1,SC_MAGICMIRROR,SC_SLOWCAST,SC_CRITICALWOUND,-1,SC_STONESKIN,SC_ANTIMAGIC,SC_CURSE,SC_STUN,-1,
	/* 680- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 700- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 800- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 900- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 1000- */
	-1,-1,SC_SHRINK,-1,-1,SC_CLOSECONFINE,SC_SIGHTBLASTER,-1,SC_ELEMENTWATER,-1,
	/* 1010- */
	-1,SC_WINKCHARM,-1,-1,-1,-1,-1,SC_ELEMENTGROUND,SC_ELEMENTFIRE,SC_ELEMENTWIND,
};

/* (�X�L���ԍ� - HOM_SKILLID)�����X�e�[�^�X�ُ�ԍ��ϊ��e�[�u�� */
int HomSkillStatusChangeTable[MAX_HOMSKILL] = {	/* status.h��enum��SC_***�Ƃ��킹�邱�� */
	/* 0- */
	-1,SC_AVOID,-1,SC_CHANGE,-1,SC_DEFENCE,-1,SC_BLOODLUST,-1,SC_FLEET,
	/* 10- */
	SC_SPEED,-1,-1,-1,-1,-1,
};

/* (�X�L���ԍ� - MERC_SKILLID)�����X�e�[�^�X�ُ�ԍ��ϊ��e�[�u�� */
int MercSkillStatusChangeTable[MAX_MERCSKILL] = {	/* status.h��enum��SC_***�Ƃ��킹�邱�� */
	/* 0- */
	-1,SC_MAGNUM,-1,SC_PARRYING,SC_REFLECTSHIELD,SC_BERSERK,SC_DOUBLE,-1,-1,-1,
	/* 10- */
	-1,-1,-1,-1,-1,-1,-1,-1,SC_DEFENDER,SC_AUTOGUARD,
	/* 20- */
	SC_DEVOTION,SC_MAGNIFICAT,SC_WEAPONQUICKEN,SC_SIGHT,-1,-1,-1,-1,-1,-1,
	/* 30- */
	-1,SC_PROVOKE,SC_AUTOBERSERK,SC_DECREASEAGI,-1,SC_SILENCE,-1,
};

/* (�X�L���ԍ� - GUILD_SKILLID)�����X�e�[�^�X�ُ�ԍ��ϊ��e�[�u�� */
int GuildSkillStatusChangeTable[MAX_GUILDSKILL] = {	/* status.h��enum��SC_***�Ƃ��킹�邱�� */
	/* 0- */
	-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
	/* 10- */
	SC_BATTLEORDER,SC_REGENERATION,-1,-1,-1,-1,
};

/* �X�L���f�[�^�x�[�X */
struct skill_db skill_db[MAX_SKILL_DB];

/* �A�C�e���쐬�f�[�^�x�[�X */
struct skill_produce_db skill_produce_db[MAX_SKILL_PRODUCE_DB];

/* ��쐬�X�L���f�[�^�x�[�X */
struct skill_arrow_db skill_arrow_db[MAX_SKILL_ARROW_DB];

/* �A�u���J�_�u�������X�L���f�[�^�x�[�X */
struct skill_abra_db skill_abra_db[MAX_SKILL_ABRA_DB];

/* �v���g�^�C�v */
static struct skill_unit *skill_initunit(struct skill_unit_group *group,int idx,int x,int y);
static struct skill_unit_group *skill_initunitgroup(struct block_list *src,int count,int skillid,int skilllv,int unit_id,unsigned int tick);

static int skill_item_consume(struct block_list *bl, struct skill_condition *cnd, int type, int *itemid, int *amount);

static void skill_brandishspear_dir(struct square *tc,int dir,int are);
static void skill_brandishspear_first(struct square *tc,int dir,int x,int y);
static int skill_frostjoke_scream(struct block_list *bl,va_list ap);
static int skill_abra_dataset(struct map_session_data *sd, int skilllv);
static int skill_clear_element_field(struct block_list *bl);
static int skill_landprotector(struct block_list *bl, va_list ap );
static int skill_tarot_card_of_fate(struct block_list *src,struct block_list *target,int skillid,int skilllv,int wheel);
static int skill_trap_splash(struct block_list *bl, va_list ap );
static int skill_count_target(struct block_list *bl, va_list ap );
static int skill_unit_onplace(struct skill_unit *src,struct block_list *bl,unsigned int tick);
static int skill_unit_effect(struct block_list *bl,va_list ap);
static int skill_greed( struct block_list *bl,va_list ap );
static int skill_balkyoung( struct block_list *bl,va_list ap );
static int skill_castle_mob_changetarget(struct block_list *bl,va_list ap);
static int skill_delunit_by_ganbantein(struct block_list *bl, va_list ap );
static int skill_count_unitgroup(struct unit_data *ud,int skillid);
static int skill_am_twilight(struct map_session_data* sd, int skillid);

/* �X�L�����j�b�g�̔z�u����Ԃ� */
static struct skill_unit_layout skill_unit_layout[MAX_SKILL_UNIT_LAYOUT];
static int firewall_unit_pos;
static int icewall_unit_pos;

static struct skill_unit_layout *skill_get_unit_layout(int skillid,int skilllv,struct block_list *src,int x,int y)
{
	int pos = skill_get_unit_layout_type(skillid,skilllv);
	int dir;

	if(pos != -1)
		return &skill_unit_layout[pos];

	if(src->x == x && src->y == y)
		dir = 6;
	else
		dir = map_calc_dir(src,x,y);

	if(skillid == MG_FIREWALL)
		return &skill_unit_layout[firewall_unit_pos+dir];
	else if(skillid == WZ_ICEWALL)
		return &skill_unit_layout[icewall_unit_pos+dir];

	printf("unknown unit layout for skill %d, %d\n",skillid,skilllv);
	return &skill_unit_layout[0];
}

int GetSkillStatusChangeTable(int id)
{
	if(id < HOM_SKILLID)
		return SkillStatusChangeTable[id];

	if(id < MERC_SKILLID)
		return HomSkillStatusChangeTable[id - HOM_SKILLID];

	if(id < GUILD_SKILLID)
		return MercSkillStatusChangeTable[id - MERC_SKILLID];

	return GuildSkillStatusChangeTable[id - GUILD_SKILLID];
}
int skill_get_hit(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].hit;
}
int skill_get_inf(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].inf;
}
int skill_get_pl(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].pl;
}
int skill_get_nk(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].nk;
}
int skill_get_max(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].max;
}
int skill_get_range(int id,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].range[lv-1];
}
int skill_get_hp(int id,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].hp[lv-1];
}
int skill_get_sp(int id,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].sp[lv-1];
}
int skill_get_zeny(int id,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].zeny[lv-1];
}
int skill_get_num(int id,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].num[lv-1];
}
int skill_get_cast(int id,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].cast[lv-1];
}
int skill_get_fixedcast(int id ,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].fixedcast[lv-1];
}
int skill_get_delay(int id,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].delay[lv-1];
}
int skill_get_time(int id ,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].upkeep_time[lv-1];
}
int skill_get_time2(int id,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].upkeep_time2[lv-1];
}
int skill_get_castdef(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].cast_def_rate;
}
int skill_get_weapontype(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].weapon;
}
int skill_get_inf2(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].inf2;
}
int skill_get_maxcount(int id,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].maxcount[lv-1];
}
int skill_get_skill_type(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].skill_type;
}
int skill_get_blewcount(int id,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].blewcount[lv-1];
}
int skill_get_unit_id(int id,int flag)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].unit_id[flag];
}
int skill_get_unit_layout_type(int id,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].unit_layout_type[lv-1];
}
int skill_get_unit_interval(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].unit_interval;
}
int skill_get_unit_range(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].unit_range;
}
int skill_get_unit_target(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].unit_target;
}
int skill_get_unit_flag(int id,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].unit_flag[lv-1];
}
int skill_get_arrow_cost(int id,int lv)
{
	if(lv<=0) return 0;

	id = skill_get_skilldb_id(id);
	if(lv > MAX_SKILL_LEVEL) lv = MAX_SKILL_LEVEL;
	return skill_db[id].arrow_cost[lv-1];
}
int skill_get_arrow_type(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].arrow_type;
}
int skill_get_cloneable(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].cloneable;
}
int skill_get_misfire(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].misfire;
}
int skill_get_zone(int id)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].zone;
}
int skill_get_damage_rate(int id,int type)
{
	id = skill_get_skilldb_id(id);
	return skill_db[id].damage_rate[type];
}

/* �␳�ςݎ˒���Ԃ� */
int skill_get_fixed_range(struct block_list *bl,int id,int lv)
{
	int range;

	nullpo_retr(0, bl);

	range = skill_get_range(id,lv);
	if(range < 0)
		range = status_get_range(bl) - (range + 1);

	return range;
}

/*==========================================
 * �X�L���ǉ�����
 *------------------------------------------
 */
int skill_additional_effect( struct block_list* src, struct block_list *bl,int skillid,int skilllv,int attack_type,unsigned int tick)
{
	/* MOB�ǉ����ʃX�L���p */
	static const int sc[] = {
		SC_POISON, SC_BLIND, SC_SILENCE, SC_STUN,
		SC_STONE, SC_CURSE, SC_SLEEP
	};
	static const int sc2[] = {
		MG_STONECURSE,MG_FROSTDIVER,NPC_STUNATTACK,
		NPC_SLEEPATTACK,TF_POISON,NPC_CURSEATTACK,
		NPC_SILENCEATTACK,0,NPC_BLINDATTACK,LK_HEADCRUSH
	};

	struct map_session_data *sd = NULL, *dstsd = NULL;
	struct mob_data         *md = NULL, *dstmd = NULL;
	struct skill_unit       *unit = NULL;
	struct status_change    *tsc = NULL;
	int skill;
	int rate,luk;

	int sc_def_mdef,sc_def_vit,sc_def_int,sc_def_luk;
	int sc_def_mdef2,sc_def_vit2,sc_def_int2,sc_def_luk2;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	if(skilllv < 0) return 0;

	// PC,MOB,PET,MERC�ȊO�͒ǉ����ʂ̑ΏۊO
	if(bl->type != BL_PC && bl->type != BL_MOB && bl->type != BL_PET && bl->type != BL_MERC)
		return 0;

	// �O���E���h�h���t�g�̂Ƃ���src��ݒu�҂ɒu��
	if(src->type == BL_SKILL) {
		unit = (struct skill_unit *)src;
		if(unit && unit->group) {
			src = map_id2bl(unit->group->src_id);
			if(src == NULL)
				return 0;
		}
	}

	sd    = BL_DOWNCAST( BL_PC,  src );
	md    = BL_DOWNCAST( BL_MOB, src );
	dstsd = BL_DOWNCAST( BL_PC,  bl );
	dstmd = BL_DOWNCAST( BL_MOB, bl );

	tsc = status_get_sc(bl);

	// �Ώۂ̑ϐ�
	luk = status_get_luk(bl);
	sc_def_mdef = 100 - (3 + status_get_mdef(bl) + luk/3);
	sc_def_vit  = 100 - (3 + status_get_vit(bl) + luk/3);
	sc_def_int  = 100 - (3 + status_get_int(bl) + luk/3);
	sc_def_luk  = 100 - (3 + luk);
	// �����̑ϐ�
	luk = status_get_luk(src);
	sc_def_mdef2 = 100 - (3 + status_get_mdef(src) + luk/3);
	sc_def_vit2  = 100 - (3 + status_get_vit(src) + luk/3);
	sc_def_int2  = 100 - (3 + status_get_int(src) + luk/3);
	sc_def_luk2  = 100 - (3 + luk);

	if(dstmd) {
		if(sc_def_mdef < 50)
			sc_def_mdef = 50;
		if(sc_def_vit < 50)
			sc_def_vit = 50;
		if(sc_def_int < 50)
			sc_def_int = 50;
		if(sc_def_luk < 50)
			sc_def_luk = 50;
	} else {
		if(sc_def_mdef < 0)
			sc_def_mdef = 0;
		if(sc_def_vit < 0)
			sc_def_vit = 0;
		if(sc_def_int < 0)
			sc_def_int = 0;
	}

	/* �R���p�C���ˑ��̃o�O������܂��AGCC 3.3.0/3.3.1 �̐l�p */
	/* �R���p�C���Ōv�Z���O�^�O�̎��ɕϐ��̍ő�l����������o�O */
	/* ������悤�ł��A�������v�Z�ɂȂ�Ȃ��ł����ǁA�_���[�W�v�Z */
	/* ��X�e�[�^�X�ω����\�z�ȏ�ɓK�p����Ȃ��ꍇ�A�ȉ���L���ɂ��Ă������� */
	//	if(sc_def_mdef < 1)
	//		sc_def_mdef = 1;
	//	if(sc_def_vit < 1)
	//		sc_def_vit = 1;
	//	if(sc_def_int < 1)
	//		sc_def_int = 1;

	switch(skillid) {
	case 0:
		/* ������ */
		if( sd &&
		    pc_isfalcon(sd) &&
		    (skill = pc_checkskill(sd,HT_BLITZBEAT)) > 0 &&
		    (sd->status.weapon == WT_BOW || battle_config.allow_any_weapon_autoblitz) &&
		    atn_rand()%10000 < sd->paramc[5]*30+100 )
		{
			int lv = (sd->status.job_level+9)/10;
			skill_castend_damage_id(src,bl,HT_BLITZBEAT,(skill < lv)? skill: lv,tick,0xf00000);
		}
		/* �X�i�b�`���[ */
		if(sd && sd->status.weapon != WT_BOW && (skill = pc_checkskill(sd,RG_SNATCHER)) > 0) {
			int skill2;
			if((skill*15 + 55) + (skill2 = pc_checkskill(sd,TF_STEAL))*10 > atn_rand()%1000) {
				if(dstmd && pc_steal_item(sd,dstmd))
					clif_skill_nodamage(src,bl,TF_STEAL,skill2,1);
				else if(battle_config.display_snatcher_skill_fail)
					clif_skill_fail(sd,skillid,0,0);
			}
		}
		/* �e�R���R��\�� */
		if(sd && sd->sc.data[SC_TKCOMBO].timer == -1) {
			int id = 0, lv = 0;
			if(sd->sc.data[SC_READYSTORM].timer != -1 && (lv = pc_checkskill(sd,TK_STORMKICK)) > 0 && atn_rand()%100 < 15) {
				// �t�F�I���`���M
				id = TK_STORMKICK;
			}
			else if(sd->sc.data[SC_READYDOWN].timer != -1 && (lv = pc_checkskill(sd,TK_DOWNKICK)) > 0 && atn_rand()%100 < 15) {
				// �l�����`���M
				id = TK_DOWNKICK;
			}
			else if(sd->sc.data[SC_READYTURN].timer != -1 && (lv = pc_checkskill(sd,TK_TURNKICK)) > 0 && atn_rand()%100 < 15) {
				// �g�������`���M
				id = TK_TURNKICK;
			}
			else if(sd->sc.data[SC_READYCOUNTER].timer != -1 && (lv = pc_checkskill(sd,TK_COUNTER)) > 0) {
				// �A�v�`���I�����M
				int counter_rate = 20;
				if(sd->sc.data[SC_COUNTER_RATE_UP].timer != -1 && (skill = pc_checkskill(sd,SG_FRIEND)) > 0) {
					counter_rate += counter_rate * (50 + 50 * skill);
					status_change_end(&sd->bl,SC_COUNTER_RATE_UP,-1);
				}
				if(atn_rand()%100 < counter_rate)
					id = TK_COUNTER;
			}
			if(id > 0 && lv > 0) {
				int delay = status_get_adelay(src) + 2000 - 4 * status_get_agi(src) - 2 * status_get_dex(src);
				// TK�R���{���͎��Ԃ̍Œ�ۏ�ǉ�
				if(delay < battle_config.tkcombo_delay_lower_limits) {
					delay = battle_config.tkcombo_delay_lower_limits;
				}
				if(delay > 0) {
					status_change_start(&sd->bl,SC_TKCOMBO,id,lv,0,0,delay,0);
					sd->ud.attackabletime = tick + delay;
				}
				clif_skill_nodamage(&sd->bl,&sd->bl,id-1,pc_checkskill(sd,id-1),1);
			}
		}
		/* �G���`�����g�f�b�g���[�|�C�Y��(�ғŌ���) */
		if(sd && sd->sc.data[SC_EDP].timer != -1 && !(status_get_mode(bl)&0x20) && atn_rand() % 10000 < sd->sc.data[SC_EDP].val2 * sc_def_vit) {
			int lv = sd->sc.data[SC_EDP].val1;
			status_change_start(bl,SC_DPOISON,lv,0,0,0,skill_get_time2(ASC_EDP,lv),0);
		}
		/* �����g�_�E�� */
		if(sd && sd->sc.data[SC_MELTDOWN].timer != -1) {
			if(atn_rand() % 100 < sd->sc.data[SC_MELTDOWN].val1) {
				// ����j��
				if(dstsd) {
					pc_break_equip(dstsd, EQP_WEAPON);
				} else {
					status_change_start(bl,SC_STRIPWEAPON,1,0,0,0,skill_get_time2(WS_MELTDOWN,sd->sc.data[SC_MELTDOWN].val1),0);
				}
			}
			if(atn_rand() % 1000 < sd->sc.data[SC_MELTDOWN].val1*7) {
				// �Z�j��
				if(dstsd) {
					pc_break_equip(dstsd, EQP_ARMOR);
				} else {
					status_change_start(bl,SC_STRIPARMOR,1,0,0,0,skill_get_time2(WS_MELTDOWN,sd->sc.data[SC_MELTDOWN].val1),0);
				}
			}
		}
		break;
	case SM_BASH:			/* �o�b�V���i�}���U���j */
		if( sd && (skill = pc_checkskill(sd,SM_FATALBLOW)) > 0 ) {
			if( atn_rand()%100 < (5*(skilllv-5)+(sd->status.base_level/3))*sc_def_vit/100 && skilllv > 5 )
				status_change_start(bl,SC_STUN,skilllv,0,0,0,skill_get_time2(SM_FATALBLOW,skilllv),0);
		}
		break;

	case TF_POISON:			/* �C���x�i�� */
	case AS_SPLASHER:		/* �x�i���X�v���b�V���[ */
		if(atn_rand()%100 < (2*skilllv+10)*sc_def_vit/100 ) {
			status_change_start(bl,SC_POISON,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		} else {
			if(sd && skillid == TF_POISON)
				clif_skill_fail(sd,skillid,0,0);
		}
		break;

	case AS_VENOMKNIFE:		/* �x�i���i�C�t */
		if(atn_rand()%10000 < 6000*sc_def_vit/100 ) {
			status_change_start(bl,SC_POISON,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		} else {
			if(sd) clif_skill_fail(sd,TF_POISON,0,0);
		}
		break;
	case AS_SONICBLOW:		/* �\�j�b�N�u���[ */
		if( atn_rand()%100 < (2*skilllv+10)*sc_def_vit/100 )
			status_change_start(bl,SC_STUN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case HT_FREEZINGTRAP:		/* �t���[�W���O�g���b�v */
	case MA_FREEZINGTRAP:
		rate = skilllv*3+35;
		if(atn_rand()%100 < rate*sc_def_mdef/100)
			status_change_start(bl,SC_FREEZE,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case HT_FLASHER:		/* �t���b�V���[ */
		if( !(status_get_mode(bl)&0x20) && status_get_race(bl) != RCT_PLANT ) {	// �{�X�ƐA������
			if(atn_rand()%100 < (10*skilllv+30)*sc_def_int/100)
				status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		}
		break;

	case WZ_METEOR:			/* ���e�I�X�g�[�� */
		if(atn_rand()%100 < 3*skilllv)
			status_change_start(bl,SC_STUN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case WZ_VERMILION:		/* ���[�h�I�u���@�[�~���I�� */
		if(atn_rand()%100 < 4*skilllv)
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case WZ_FROSTNOVA:		/* �t���X�g�m���@ */
		rate = (skilllv*5+33)*sc_def_mdef/100-(status_get_int(bl)+status_get_luk(bl))/15;
		if(rate <= 5)
			rate = 5;
		if((!tsc || tsc->data[SC_FREEZE].timer == -1) && atn_rand()%100 < rate)
			status_change_start(bl,SC_FREEZE,skilllv,0,0,0,skill_get_time2(skillid,skilllv)*(1-sc_def_mdef/100),0);
		break;

	case WZ_STORMGUST:		/* �X�g�[���K�X�g */
#ifdef DYNAMIC_SC_DATA
		status_calloc_sc_data(tsc);
#endif
		if(tsc) {
			tsc->data[SC_FREEZE].val3++;
			if(tsc->data[SC_FREEZE].val3 >= 3)
				status_change_start(bl,SC_FREEZE,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		}
		break;

	case HT_LANDMINE:		/* �����h�}�C�� */
	case MA_LANDMINE:
		if( atn_rand()%100 < (5*skilllv+30)*sc_def_vit/100 )
			status_change_start(bl,SC_STUN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case HT_SHOCKWAVE:		/* �V���b�N�E�F�[�u�g���b�v */
		if(dstsd) {
			dstsd->status.sp -= dstsd->status.sp*(5+15*skilllv)/100;
			if(dstsd->status.sp <= 0)
				dstsd->status.sp = 0;
			clif_updatestatus(dstsd,SP_SP);
		}
		break;
	case HT_SANDMAN:		/* �T���h�}�� */
	case MA_SANDMAN:
		if( !(status_get_mode(bl)&0x20) && atn_rand()%100 < (5*skilllv+30)*sc_def_int/100 )
			status_change_start(bl,SC_SLEEP,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case TF_SPRINKLESAND:		/* ���܂� */
		if( atn_rand()%100 < 20*sc_def_int/100 )
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case TF_THROWSTONE:		/* �Γ��� */
		if( atn_rand()%100 < 5*sc_def_vit/100 )
			status_change_start(bl,SC_STUN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		if( atn_rand()%100 < 3*skilllv*sc_def_int/100 )
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case CR_HOLYCROSS:		/* �z�[���[�N���X */
		if( atn_rand()%100 < 3*skilllv*sc_def_int/100 )
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case CR_GRANDCROSS:		/* �O�����h�N���X */
	case NPC_GRANDDARKNESS:		/* �O�����h�_�[�N�l�X */
		{
			int race = status_get_race(bl);
			if( (battle_check_undead(race,status_get_elem_type(bl)) || race == RCT_DEMON) && atn_rand()%100 < 100000*sc_def_int/100)	// �����t�^�������S�ϐ��ɂ͖���
				status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		}
		break;

	case CR_SHIELDCHARGE:		/* �V�[���h�`���[�W */
		if( atn_rand()%100 < (15 + skilllv*5)*sc_def_vit/100 )
			status_change_start(bl,SC_STUN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case RG_RAID:			/* �T�v���C�Y�A�^�b�N */
		if( atn_rand()%100 < (10+3*skilllv)*sc_def_vit/100 )
			status_change_start(bl,SC_STUN,skilllv,0,0,0,3000,0);
		if( atn_rand()%100 < (10+3*skilllv)*sc_def_int/100 )
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case BA_FROSTJOKE:
		if(atn_rand()%100 < (15+5*skilllv)*sc_def_mdef/100)
			status_change_start(bl,SC_FREEZE,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case DC_SCREAM:
		if( atn_rand()%100 < (25+5*skilllv)*sc_def_vit/100 )
			status_change_start(bl,SC_STUN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case DC_UGLYDANCE:	/* ��������ȃ_���X */
		if(dstsd) {
			int sp = 5+skilllv*(5+pc_checkskill(dstsd,DC_DANCINGLESSON));
			pc_heal(dstsd,0,-sp);
		}
		break;

	case BD_LULLABY:	/* �q��S */
		if( atn_rand()%100 < 15*sc_def_int/100 )
			status_change_start(bl,SC_SLEEP,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case AM_ACIDTERROR:		/* �A�V�b�h�e���[ */
		if(bl->type == BL_PC && atn_rand()%100 < skill_get_time(skillid,skilllv)) {
			pc_break_equip((struct map_session_data *)bl, EQP_ARMOR);
			clif_emotion(bl,23);
		}
		if(atn_rand()%100 < 3*skilllv)
			status_change_start(bl,SC_BLEED,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	/* MOB�̒ǉ����ʕt���X�L�� */

	case NPC_PETRIFYATTACK:
		if(atn_rand()%100 < skilllv*20*sc_def_mdef/100)
			status_change_start(bl,sc[skillid-NPC_POISON],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_POISON:
	case NPC_SILENCEATTACK:
	case NPC_STUNATTACK:
		if(atn_rand()%100 < skilllv*20*sc_def_vit/100)
			status_change_start(bl,sc[skillid-NPC_POISON],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_CURSEATTACK:
		if(atn_rand()%100 < skilllv*20*sc_def_luk/100)
			status_change_start(bl,sc[skillid-NPC_POISON],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_SLEEPATTACK:
	case NPC_BLINDATTACK:
		if(atn_rand()%100 < skilllv*20*sc_def_int/100)
			status_change_start(bl,sc[skillid-NPC_POISON],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_MENTALBREAKER:
		if(dstsd) {
			int sp = dstsd->status.max_sp*(10+skilllv*5)/100;
			if(sp < 1) sp = 1;
			pc_heal(dstsd,0,-sp);
		}
		break;
	case NPC_WEAPONBRAKER:
		if(dstsd && atn_rand()%100 < skilllv*10)
			pc_break_equip(dstsd, EQP_WEAPON);
		break;
	case NPC_ARMORBRAKE:
		if(dstsd && atn_rand()%100 < skilllv*10)
			pc_break_equip(dstsd, EQP_ARMOR);
		break;
	case NPC_HELMBRAKE:
		if(dstsd && atn_rand()%100 < skilllv*10)
			pc_break_equip(dstsd, EQP_HELM);
		break;
	case NPC_SHIELDBRAKE:
		if(dstsd && atn_rand()%100 < skilllv*10)
			pc_break_equip(dstsd, EQP_SHIELD);
		break;

	case LK_HEADCRUSH:		/* �w�b�h�N���b�V�� */
		{
			int race = status_get_race(bl);
			if( !battle_check_undead(race,status_get_elem_type(bl)) && race != RCT_DEMON && atn_rand()%100 < sc_def_vit/2 )
				status_change_start(bl,SC_BLEED,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		}
		break;
	case LK_JOINTBEAT:		/* �W���C���g�r�[�g */
		if( atn_rand()%100 < skilllv*5+5-status_get_str(bl)*27/100 )
			status_change_start(bl,SC_JOINTBEAT,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case PF_SPIDERWEB:		/* �X�p�C�_�[�E�F�u */
		{
			int sec = skill_get_time2(skillid,skilllv);
			if( map[src->m].flag.pvp || map[src->m].flag.gvg ) // �ΐl�t�B�[���h�ł͍S�����Ԕ���
				sec = sec/2;
			unit_stop_walking(bl,1);
			status_change_start(bl,SC_SPIDERWEB,skilllv,0,0,0,sec,0);
		}
		break;
	case ASC_METEORASSAULT:		/* ���e�I�A�T���g */
		{
			int type = 0;
			switch(atn_rand()%3) {
				case 0:
					if( atn_rand()%100 < sc_def_vit*(5+skilllv*5)/100 )
						type = SC_STUN;
					break;
				case 1:
					if( atn_rand()%100 < sc_def_int*(5+skilllv*5)/100 )
						type = SC_BLIND;
					break;
				case 2:
					if( atn_rand()%100 < sc_def_vit*(5+skilllv*5)/100 )
						type = SC_BLEED;
					break;
			}
			if(type)
				status_change_start(bl,type,skilllv,0,0,0,skill_get_time2(sc2[type-SC_STONE],7),0);
		}
		break;
	case MO_EXTREMITYFIST:		/* ���C���e���� */
		// ���C�����g����5���Ԏ��R�񕜂��Ȃ��悤�ɂȂ�
		status_change_start(src,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0 );
		break;
	case HW_NAPALMVULCAN:			/* �i�p�[���o���J�� */
		// skilllv*5%�̊m���Ŏ�
		if(atn_rand()%10000 < 5*skilllv*sc_def_luk)
			status_change_start(bl,SC_CURSE,7,0,0,0,skill_get_time2(NPC_CURSEATTACK,7),0);
		break;
	case PA_PRESSURE:		/* �v���b�V���[ */
		if(dstsd) {
			// �Ώۂ�15% + skilllv*5%��SP�U��(�K��)
			int sp = dstsd->status.sp*(15+5*skilllv)/100;
			pc_heal(dstsd,0,-sp);
		}
		break;
	case WS_CARTTERMINATION:
		// skilllv*5%�̊m���ŃX�^��
		if( atn_rand()%100 < 5*skilllv*sc_def_vit/100 )
			status_change_start(bl,SC_STUN,7,0,0,0,skill_get_time2(NPC_STUNATTACK,7),0);
		break;
	case CR_ACIDDEMONSTRATION:	/* �A�V�b�h�f�����X�g���[�V���� */
		if(atn_rand()%100 <= skilllv) {
			if(dstsd)
				pc_break_equip(dstsd, EQP_WEAPON);
			else
				status_change_start(bl,SC_STRIPWEAPON,1,0,0,0,skill_get_time(RG_STRIPWEAPON,1),0);
		}
		if(atn_rand()%100 <= skilllv) {
			if(dstsd)
				pc_break_equip(dstsd, EQP_ARMOR);
			else
				status_change_start(bl,SC_STRIPARMOR,1,0,0,0,skill_get_time(RG_STRIPARMOR,1),0);
		}
		break;
	case TK_DOWNKICK:		/* �l�����`���M */
		status_change_start(bl,SC_STUN,7,0,0,0,3000,0);
		break;
	case TK_TURNKICK:		/* �g�������`���M */
		// �m���s���Ȃ̂łƂ肠����100%
		status_change_start(bl,SC_STUN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case CH_TIGERFIST:		/* ���Ռ� */
		if( atn_rand()%100 < 10 + skilllv*10 ) {
			int sec = skill_get_time2(skillid,skilllv) - status_get_agi(bl)*50;
			// �Œ�S�����ԕ⏞�i�Ƃ肠�����A���N����1/2�j
			if(sec < 1500 + 15*skilllv)
				sec = 1500 + 15*skilllv;
			status_change_start(bl,SC_TIGERFIST,skilllv,0,0,0,sec,0);
			unit_stop_walking(bl,1);
		}
		break;
	case SL_STUN:			/* �G�X�g�� */
		if(status_get_size(bl) == 1 && atn_rand()%100 < sc_def_vit)
			status_change_start(bl,SC_STUN,7,0,0,0,2000,0);
		break;
	case GS_FLING:			/* �t���C���O */
		if(sd) {
			int i, y = 0;
			for(i=0; i<MAX_INVENTORY; i++) {
				if(sd->status.inventory[i].nameid == 7517) {
					y = (sd->status.inventory[i].amount > 4)? 4: sd->status.inventory[i].amount;
					pc_delitem(sd,i,y,0);
				}
			}
			status_change_start(bl,SC_FLING,skilllv+y,0,0,0,skill_get_time2(skillid,skilllv),0);
		}
		break;
	case GS_BULLSEYE:		/* �u���Y�A�C */
		if(atn_rand()%10000 < 10) {
			if(dstsd) {
				dstsd->status.hp = 1;
				clif_updatestatus(dstsd,SP_HP);
			}
			if(dstmd && !(status_get_mode(bl)&0x20))
				dstmd->hp = 1;
		}
		break;
	case GS_DISARM:			/* �f�B�X�A�[�� */
		if(atn_rand()%100 < 10 + skilllv*10) {
			if(dstsd) {
				int i;
				for(i=0; i<MAX_INVENTORY; i++) {
					if(dstsd->status.inventory[i].equip && (dstsd->status.inventory[i].equip & EQP_WEAPON)) {
						pc_unequipitem(dstsd,i,0);
						break;
					}
				}
			}
			if(dstmd && !(status_get_mode(bl)&0x20)) {
				status_change_start(bl,SC_DISARM,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
			}
		}
		break;
	case GS_PIERCINGSHOT:		/* �s�A�[�V���O�V���b�g */
		{
			int race = status_get_race(bl);
			if( !(battle_check_undead(race,status_get_elem_type(bl)) || race == RCT_DEMON) && atn_rand()%100 < (2*skilllv+10)*sc_def_vit/100 )
				status_change_start(bl,SC_BLEED,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		}
		break;
	case GS_FULLBUSTER:		/* �t���o�X�^�[ */
		{
			//status_change_start(src,SC_FULLBUSTER,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
			if(atn_rand()%100 < (2*skilllv)*sc_def_int2/100)
				status_change_start(src,SC_BLIND,7,0,0,0,skill_get_time2(NPC_BLINDATTACK,7),0);
		}
		break;
	case GS_GROUNDDRIFT:		/* �O���E���h�h���t�g */
		if(unit && unit->group)
		{
			switch(unit->group->unit_id) {		// �m���͓K���A�b��Ŋ�{50%
			case UNT_GROUNDDRIFT_WIND:
				if(atn_rand()%100 < 50*sc_def_vit/100)
					status_change_start(bl,SC_STUN,7,0,0,0,skill_get_time2(NPC_STUNATTACK,7),0);
				break;
			case UNT_GROUNDDRIFT_DARK:
				if(atn_rand()%100 < 50*sc_def_int/100)
					status_change_start(bl,SC_BLIND,7,0,0,0,skill_get_time2(NPC_BLINDATTACK,7),0);
				break;
			case UNT_GROUNDDRIFT_POISON:
				if(atn_rand()%100 < 50*sc_def_vit/100)
					status_change_start(bl,SC_POISON,7,0,0,0,skill_get_time2(TF_POISON,7),0);
				break;
			case UNT_GROUNDDRIFT_WATER:
				if(atn_rand()%100 < 50*sc_def_mdef/100)
					status_change_start(bl,SC_FREEZE,7,0,0,0,skill_get_time2(MG_FROSTDIVER,7),0);
				break;
			case UNT_GROUNDDRIFT_FIRE:
				skill_blown(&unit->bl,bl,3|SAB_NODAMAGE);
				break;
			}
		}
		break;
	case NJ_KASUMIKIRI:		/* ���a�� */
		status_change_start(src,SC_HIDING,skilllv,0,1,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NJ_HYOUSYOURAKU:		/* �X������ */
		rate = (skilllv*5+10)*sc_def_mdef/100-(status_get_int(bl)+status_get_luk(bl))/15;
		if(rate < 5)
			rate = 5;
		if((!tsc || tsc->data[SC_FREEZE].timer == -1) && atn_rand()%100 < rate)
			status_change_start(bl,SC_FREEZE,skilllv,0,0,0,skill_get_time2(skillid,skilllv)*(1-sc_def_mdef/100),0);
		break;
	case NPC_ICEBREATH:		/* �A�C�X�u���X */
		rate = 70*sc_def_mdef/100-(status_get_int(bl)+status_get_luk(bl))/15;
		if(rate < 5)
			rate = 5;
		if((!tsc || tsc->data[SC_FREEZE].timer == -1) && atn_rand()%100 < rate)
			status_change_start(bl,SC_FREEZE,skilllv,0,0,0,skill_get_time2(skillid,skilllv)*(1-sc_def_mdef/100),0);
		break;
	case NPC_ACIDBREATH:		/* �A�V�b�h�u���X */
		if(atn_rand()%100 < 70*sc_def_vit/100)
			status_change_start(bl,SC_POISON,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_BLEEDING:		/* �o���U�� */
		if(atn_rand()%100 < skilllv*20)
			status_change_start(bl,SC_BLEED,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_HELLJUDGEMENT:		/* �w���W���b�W�����g */
		status_change_start(bl,SC_CURSE,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_EVILLAND:		/* �C�r�������h */
		if(atn_rand()%100 < skilllv*5)
			status_change_start(bl,SC_BLIND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case NPC_CRITICALWOUND:		/* �v�����U�� */
		status_change_start(bl,SC_CRITICALWOUND,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case MER_CRASH:			/* �N���b�V�� */
		if( atn_rand()%100 < 6 * skilllv * sc_def_vit / 100 )
			status_change_start(bl,SC_STUN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	}

	if(attack_type&BF_WEAPON)
	{
		// �����ʏ�U���Ȃ獬���I��
		if(tsc && tsc->data[SC_CONFUSION].timer != -1 && skillid == 0)
			status_change_end(bl,SC_CONFUSION,-1);

		// �J�[�h�ɂ��ǉ�����
		if(sd && skillid != WS_CARTTERMINATION && skillid != CR_ACIDDEMONSTRATION) {
			int i, sc_def_card;

			for(i = SC_STONE; i <= SC_BLEED; i++) {
				if(!dstmd || dstmd->class_ != 1288) {
					if(sd->addeff_range_flag[i-SC_STONE] > 2) {
						sd->addeff_range_flag[i-SC_STONE] -= 2;	// �����W�t���O������Ό��ɖ߂�
						continue;
					}

					// �Ώۂɏ�Ԉُ�t��
					if(i == SC_STONE || i == SC_FREEZE)
						sc_def_card = sc_def_mdef;
					else if(i == SC_STUN || i == SC_POISON || i == SC_SILENCE || i == SC_BLEED)
						sc_def_card = sc_def_vit;
					else if(i == SC_SLEEP || i == SC_CONFUSION || i == SC_BLIND)
						sc_def_card = sc_def_int;
					else if(i == SC_CURSE)
						sc_def_card = sc_def_luk;
					else
						sc_def_card = 100;

					rate = sd->addeff[i-SC_STONE];
					if(sd->state.arrow_atk)
						rate += sd->arrow_addeff[i-SC_STONE];

					if(atn_rand()%10000 < rate * sc_def_card / 100) {
						if(battle_config.battle_log)
							printf("PC %d skill_addeff: card�ɂ���Ԉُ픭�� %d %d\n",sd->bl.id,i,rate);
						status_change_pretimer(bl,i,7,0,0,0,((i == SC_CONFUSION)? 10000+7000: skill_get_time2(sc2[i-SC_STONE],7)),0,tick+status_get_adelay(src)*2/3);
					}
				}

				// �����ɏ�Ԉُ�t��
				if(i == SC_STONE || i == SC_FREEZE)
					sc_def_card = sc_def_mdef2;
				else if(i == SC_STUN || i == SC_POISON || i == SC_SILENCE || i == SC_BLEED)
					sc_def_card = sc_def_vit2;
				else if(i == SC_SLEEP || i == SC_CONFUSION || i == SC_BLIND)
					sc_def_card = sc_def_int2;
				else if(i == SC_CURSE)
					sc_def_card = sc_def_luk2;
				else
					sc_def_card = 100;

				rate = sd->addeff2[i-SC_STONE];
				if(sd->state.arrow_atk)
					rate += sd->arrow_addeff2[i-SC_STONE];

				if(atn_rand()%10000 < rate * sc_def_card / 100) {
					if(battle_config.battle_log)
						printf("PC %d skill_addeff2: card�ɂ���Ԉُ픭�� %d %d\n",sd->bl.id,i,rate);
					status_change_start(src,i,7,0,0,0,((i == SC_CONFUSION)? 10000+7000: skill_get_time2(sc2[i-SC_STONE],7)),0);
				}
			}
		}

		// �����ɂ���
		if(sd && sd->curse_by_muramasa > 0)
		{
			if(status_get_luk(src) < sd->status.base_level)
			{
				if(atn_rand()%10000 < sd->curse_by_muramasa*sc_def_luk2/100 )
					status_change_start(src,SC_CURSE,7,0,0,0,skill_get_time2(NPC_CURSEATTACK,7),0);
			}
		}

		// �����ăA�C�e������
		if(sd && sd->loss_equip_flag&0x0010)
		{
			int i;
			for(i = 0; i < 11; i++)
			{
				if(atn_rand()%10000 < sd->loss_equip_rate_when_attack[i])
				{
					pc_lossequipitem(sd,i,0);
				}
			}
		}

		// �����ăA�C�e���u���C�N
		if(sd && sd->loss_equip_flag&0x0100)
		{
			int i;
			for(i = 0; i < 11; i++)
			{
				if(atn_rand()%10000 < sd->break_myequip_rate_when_attack[i])
				{
					pc_break_equip2(sd,i);
				}
			}
		}

		// ������mob�ω�
		if(sd && dstmd && mob_db[dstmd->class_].race != RCT_HUMAN && !map[dstmd->bl.m].flag.nobranch &&
		   !(mob_db[dstmd->class_].mode&0x20) && dstmd->class_ != 1288 && dstmd->state.special_mob_ai != 1)
		{
			if(atn_rand()%10000 < sd->mob_class_change_rate)
			{
				//clif_skill_nodamage(src,bl,SA_CLASSCHANGE,1,1);
				mob_class_change_randam(dstmd,sd->status.base_level);
			}
		}
	}

	return 0;
}

/*=========================================================================
 * �X�L���U��������΂�����
 *  count -> 0x00XYZZZZ
 *	X: ������΂������w��(�t����)
 *	   ������0�i�^�k�w��A�^��ɔ�΂��j�Ȃ�8�Ƃ��đ��
 *	Y: �t���O
 *		SAB_NOMALBLOW   : src��target�̈ʒu�֌W�Ő���΂�����������
 *		SAB_REVERSEBLOW : target�̌����Ƌt�����ɐ����
 *		SAB_NODAMAGE    : �_���[�W�𔭐��������ɐ���΂�
 *		SAB_NOPATHSTOP  : ������ьo�H�ɕǂ��������炻���Ŏ~�܂�
 *	Z: ������΂��Z����
 *-------------------------------------------------------------------------
 */
int skill_blown( struct block_list *src, struct block_list *target,int count)
{
	int dx=0,dy=0,nx,ny;
	int dir,ret;
	struct status_change *sc = NULL;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	// �V�[�Y�Ȃ琁����΂����s
	if(map[target->m].flag.gvg)
		return 0;

	// ������΂��Z������0
	if((count&0xffff) == 0)
		return 0;

	sc = status_get_sc(target);

	// �A���N�����͖������Ő�����΂���Ȃ�
	if(sc && sc->data[SC_ANKLE].timer != -1)
		return 0;

	if(target->type == BL_PC) {
		if(((struct map_session_data *)target)->special_state.no_knockback)
			return 0;
		// �o�W���J���͐�����΂���Ȃ�
		if(sc && sc->data[SC_BASILICA].timer!=-1 && sc->data[SC_BASILICA].val2==target->id)
			return 0;
	} else if(target->type == BL_MOB) {
		struct mob_data *md=(struct mob_data *)target;
		if(battle_config.boss_no_knockbacking==1 && mob_db[md->class_].mode&0x20)
			return 0;
		if(battle_config.boss_no_knockbacking==2 && mob_db[md->class_].mexp > 0)
			return 0;
	} else if(target->type == BL_PET || target->type == BL_SKILL) {
		;	// �������Ȃ�
	} else {
		return 0;
	}

	if(count&0xf00000) {
		dir = (count>>20)&0xf;
		if(dir == 8)	// 0�ɒu������
			dir = 0;
	}
	else if(count&SAB_REVERSEBLOW || (target->x == src->x && target->y == src->y)) {
		dir = status_get_dir(target);
	}
	else {
		dir = map_calc_dir(target,src->x,src->y);
	}
	if(dir >= 0 && dir < 8) {
		dx = -dirx[dir];
		dy = -diry[dir];
	}

	ret = path_blownpos(target->m,target->x,target->y,dx,dy,count&0xffff,(count&SAB_NOPATHSTOP)? 1: 0);
	nx  = ret>>16;
	ny  = ret&0xffff;

	if(count&SAB_NODAMAGE)
		unit_stop_walking(target,0);	// �_���[�W�f�B���C����
	else
		unit_stop_walking(target,2);	// �_���[�W�f�B���C�L��

	if(target->type == BL_SKILL) {
		struct skill_unit *su = (struct skill_unit *)target;
		skill_unit_move_unit_group(su->group,target->m,nx-target->x,ny-target->y);
	} else {
		unit_movepos(target,nx,ny,(count&SAB_NODAMAGE)? 0: 1);
	}

	return 1;
}

/*=========================================================================
 * �X�L���U��������΂�����(�J�[�h�ǉ����ʗp)
 *-------------------------------------------------------------------------
 */
int skill_add_blown( struct block_list *src, struct block_list *target,int skillid,int flag)
{
	nullpo_retr(0, src);
	nullpo_retr(0, target);

	if(src->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)src;
		int i;
		for(i = 0; i<sd->skill_blow.count; i++)
		{
			if(sd->skill_blow.id[i] == skillid)
			{
				 skill_blown(src,target,sd->skill_blow.grid[i]|flag);
				 return 1;
			}
		}
	}
	return 0;
}

/*==========================================
 * �X�L���͈͍U���p(map_foreachinarea����Ă΂��)
 * flag�ɂ��āF16�i�}���m�F
 * MSB <- 0ffTffff -> LSB
 *  ffff = ���R�Ɏg�p�\
 *     T = �^�[�Q�b�g�I��p(BCT_*)
 *     0 = �\��B0�ɌŒ�
 *------------------------------------------
 */
static int skill_area_temp[8];	/* �ꎞ�ϐ��B�K�v�Ȃ�g���B */
typedef int (*SkillFunc)(struct block_list *,struct block_list *,int,int,unsigned int,int);

static int skill_area_sub( struct block_list *bl,va_list ap )
{
	struct block_list *src;
	int skill_id,skill_lv,flag;
	unsigned int tick;
	SkillFunc func;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	if(!(bl->type & (BL_CHAR | BL_SKILL)))
		return 0;

	src      = va_arg(ap,struct block_list *); // �����ł�src�̒l���Q�Ƃ��Ă��Ȃ��̂�NULL�`�F�b�N�͂��Ȃ�
	skill_id = va_arg(ap,int);
	skill_lv = va_arg(ap,int);
	tick     = va_arg(ap,unsigned int);
	flag     = va_arg(ap,int);
	func     = va_arg(ap,SkillFunc);

	if(battle_check_target(src,bl,flag) > 0)
		func(src,bl,skill_id,skill_lv,tick,flag);
	return 0;
}

static int skill_area_trap_sub( struct block_list *bl,va_list ap )
{
	struct block_list *src;
	struct skill_unit *unit;
	int skill_id, skill_lv, flag;
	unsigned int tick;
	SkillFunc func;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, unit = (struct skill_unit *)bl);

	if(!unit->alive || !unit->group)
		return 0;

	src      = va_arg(ap,struct block_list *);
	skill_id = va_arg(ap,int);
	skill_lv = va_arg(ap,int);
	tick     = va_arg(ap,unsigned int);
	flag     = va_arg(ap,int);
	func     = va_arg(ap,SkillFunc);

	// battle_check_target�ŊY�����Ȃ�㩂��U���Ώ�
	switch (unit->group->unit_id) {
		case UNT_SKIDTRAP:	/* �X�L�b�h�g���b�v */
		case UNT_LANDMINE:	/* �����h�}�C�� */
		case UNT_SHOCKWAVE:	/* �V���b�N�E�F�[�u�g���b�v */
		case UNT_SANDMAN:	/* �T���h�}�� */
		case UNT_FLASHER:	/* �t���b�V���[ */
		case UNT_FREEZINGTRAP:	/* �t���[�W���O�g���b�v */
		case UNT_TALKIEBOX:	/* �g�[�L�[�{�b�N�X */
			if(skill_id == AC_SHOWER || skill_id == MA_SHOWER)
				break;
			return 0;
		case UNT_ANKLESNARE:	/* �A���N���X�l�A */
			if(skill_id == AC_SHOWER || skill_id == MA_SHOWER || unit->group->val2 > 0)
				break;
			return 0;
		default:
			return 0;
	}

	func(src,bl,skill_id,skill_lv,tick,flag);
	return 0;
}

/*==========================================
 * �X�L�����j�b�g�̏d�˒u���`�F�b�N
 *------------------------------------------
 */
static int skill_check_unit_range_sub( struct block_list *bl,va_list ap )
{
	struct skill_unit *unit;
	int *c;
	int skillid,ug_id;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, c = va_arg(ap,int *));
	nullpo_retr(0, unit = (struct skill_unit *)bl);

	if (!unit->alive || !unit->group)
		return 0;

	skillid = va_arg(ap,int);
	ug_id = unit->group->skill_id;

	switch (skillid)
	{
		case MG_SAFETYWALL:
		case AL_PNEUMA:
			if(ug_id == MG_SAFETYWALL || ug_id == AL_PNEUMA) {
				(*c)++;
			}
			break;
		case AL_WARP:
		case HT_SKIDTRAP:
		case HT_LANDMINE:
		case HT_ANKLESNARE:
		case HT_SHOCKWAVE:
		case HT_SANDMAN:
		case HT_FLASHER:
		case HT_FREEZINGTRAP:
		case HT_BLASTMINE:
		case HT_CLAYMORETRAP:
		case HT_TALKIEBOX:
		case MA_SKIDTRAP:
		case MA_LANDMINE:
		case MA_SANDMAN:
		case MA_FREEZINGTRAP:
			if( (ug_id >= HT_SKIDTRAP && ug_id <= HT_CLAYMORETRAP) ||
			    (ug_id >= MA_SKIDTRAP && ug_id <= MA_FREEZINGTRAP) ||
			    ug_id == HT_TALKIEBOX )
			{
				(*c)++;
			}
			break;
		case HP_BASILICA:
			if( (ug_id >= HT_SKIDTRAP && ug_id <= HT_CLAYMORETRAP) ||
			    (ug_id >= MA_SKIDTRAP && ug_id <= MA_FREEZINGTRAP) ||
			    ug_id == HT_TALKIEBOX ||
			    ug_id == PR_SANCTUARY )
			{
				(*c)++;
			}
			break;
		default:	// �����X�L�����j�b�g�łȂ���΋���
			if(ug_id == skillid) {
				(*c)++;
			}
			break;
	}

	return 0;
}

static int skill_check_unit_range(int m,int x,int y,int skillid,int skilllv)
{
	int c = 0;
	int range = skill_get_unit_range(skillid);
	int layout_type = skill_get_unit_layout_type(skillid,skilllv);

	if(layout_type == -1 || layout_type > MAX_SQUARE_LAYOUT) {
		printf("skill_check_unit_range: unsupported layout type %d for skill %d\n",layout_type,skillid);
		return 0;
	}

	// �Ƃ肠���������`�̃��j�b�g���C�A�E�g�̂ݑΉ�
	range += layout_type;
	map_foreachinarea(skill_check_unit_range_sub,m,
			x-range,y-range,x+range,y+range,BL_SKILL,&c,skillid);

	return c;
}

/*==========================================
 * �X�L�����j�b�g�̑����u���`�F�b�N
 *------------------------------------------
 */
static int skill_check_unit_range2_sub( struct block_list *bl,va_list ap )
{
	int *c;
	int skillid;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, c = va_arg(ap,int *));

	if(!(bl->type & BL_CHAR))
		return 0;

	if(unit_isdead(bl))
		return 0;

	skillid = va_arg(ap,int);
	if(skillid == HP_BASILICA && bl->type == BL_PC)
		return 0;

	(*c)++;

	return 0;
}

static int skill_check_unit_range2(int m,int x,int y,int skillid, int skilllv)
{
	int c = 0;
	int range = skill_get_unit_range(skillid);
	int layout_type = skill_get_unit_layout_type(skillid,skilllv);

	if(layout_type == -1 || layout_type > MAX_SQUARE_LAYOUT) {
		printf("skill_check_unit_range2: unsupported layout type %d for skill %d\n",layout_type,skillid);
		return 0;
	}

	// �Ƃ肠���������`�̃��j�b�g���C�A�E�g�̂ݑΉ�
	range += layout_type;
	map_foreachinarea(skill_check_unit_range2_sub,m,
			x-range,y-range,x+range,y+range,BL_CHAR,&c,skillid);

	return c;
}

/*==========================================
 * �X�L�������̒x��
 *------------------------------------------
 */
struct castend_delay {
	struct block_list *src;
	int target;
	int id;
	int lv;
	int flag;
};

static int skill_castend_delay_sub(int tid, unsigned int tick, int id, int data)
{
	struct castend_delay *dat = (struct castend_delay *)data;
	struct block_list *target = map_id2bl(dat->target);

	if (target && dat && map_id2bl(id) == dat->src && target->prev != NULL)
		skill_castend_damage_id(dat->src, target, dat->id, dat->lv, tick, dat->flag);
	aFree(dat);
	return 0;
}

int skill_castend_delay(struct block_list* src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag)
{
	struct castend_delay *dat;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	dat = (struct castend_delay *)aCalloc(1, sizeof(struct castend_delay));
	dat->src    = src;
	dat->target = bl->id;
	dat->id     = skillid;
	dat->lv     = skilllv;
	dat->flag   = flag;
	add_timer2(tick, skill_castend_delay_sub, src->id, (int)dat, TIMER_FREE_DATA);

	return 0;
}

/*=========================================================================
 * �͈̓X�L���g�p������������������
 */
/* �Ώۂ̐����J�E���g����B�iskill_area_temp[0]�����������Ă������Ɓj */
static int skill_area_sub_count(struct block_list *src,struct block_list *target,int skillid,int skilllv,unsigned int tick,int flag)
{
	if(skill_area_temp[0] < 0xffff)
		skill_area_temp[0]++;
	return 0;
}

/*==========================================
 * ����̐��𐔂���
 *------------------------------------------
 */
static int skill_count_water(struct block_list *src,int range)
{
	int i,x,y,cnt = 0,size = range*2+1;
	struct skill_unit *unit;

	for (i=0;i<size*size;i++) {
		x = src->x+(i%size-range);
		y = src->y+(i/size-range);
		if (map_getcell(src->m,x,y,CELL_CHKWATER)) {
			cnt++;
			continue;
		}
		unit = map_find_skill_unit_oncell(src,x,y,SA_DELUGE,NULL);
		if (unit) {
			cnt++;
			skill_delunit(unit);
		}else{
			unit = map_find_skill_unit_oncell(src,x,y,NJ_SUITON,NULL);
			if (unit) {
				cnt++;
				skill_delunit(unit);
			}
		}
	}
	return cnt;
}

/*==========================================
 *
 *------------------------------------------
 */
static int skill_timerskill(int tid, unsigned int tick, int id,int data )
{
	struct block_list *src = map_id2bl(id);
	struct unit_data *ud;
	struct skill_timerskill *skl;
	int range;

	nullpo_retr(0, src);
	nullpo_retr(0, ud = unit_bl2ud(src));
	nullpo_retr(0, skl = (struct skill_timerskill*)data);

	linkdb_erase( &ud->skilltimerskill, skl);
	skl->timer = -1;

	do {
		if(src->prev == NULL)
			break;
		if(skl->target_id) {
			struct block_list *target = NULL;

			if(unit_isdead(src))
				break;
			target = map_id2bl(skl->target_id);

			// �C���e�B�~�f�C�g�ƃG�N�X�p���V�I����target�����݂��Ȃ��Ă��ǂ��̂ł����̔���͏��O
			if(skl->skill_id != RG_INTIMIDATE && skl->skill_id != NPC_EXPULSION) {
				if(target == NULL || src->m != target->m)
					break;
				if(target->prev == NULL || unit_isdead(target))
					break;
			}

			switch(skl->skill_id) {
			case TF_BACKSLIDING:
				clif_skill_nodamage(src,src,skl->skill_id,skl->skill_lv,1);
				break;
			case RG_INTIMIDATE:
				if(src->type == BL_PC && !map[src->m].flag.noteleport)
					pc_randomwarp((struct map_session_data *)src,3);
				else if(src->type == BL_MOB && !map[src->m].flag.monster_noteleport)
					mob_warp((struct mob_data *)src,-1,-1,-1,3);
				else
					break;
				if(target && target->prev != NULL && src->m == target->m) {
					struct cell_xy free_cell[3*3];
					int count, x, y;
					count = map_searchfreecell(free_cell, src->m, src->x-1, src->y-1, src->x+1, src->y+1);
					if(count > 0) {
						int n = atn_rand() % count;
						x = free_cell[n].x;
						y = free_cell[n].y;
					} else {
						x = src->x;
						y = src->y;
					}
					if(target->type == BL_PC && !unit_isdead(target))
						pc_setpos((struct map_session_data *)target,map[src->m].name,x,y,3);
					else if(target->type == BL_MOB)
						mob_warp((struct mob_data *)target,-1,x,y,3);
				}
				break;
			case NPC_EXPULSION:
				if(target && target->prev != NULL && src->m == target->m) {
					if(target->type == BL_PC && !map[target->m].flag.noteleport && !unit_isdead(target))
						pc_randomwarp((struct map_session_data *)target,3);
					else if(target->type == BL_MOB && !map[target->m].flag.monster_noteleport)
						mob_warp((struct mob_data *)target,-1,-1,-1,3);
				}
				break;
			case BA_FROSTJOKE:			/* �����W���[�N */
			case DC_SCREAM:				/* �X�N���[�� */
				range=AREA_SIZE;		// ���E�S��
				map_foreachinarea(skill_frostjoke_scream,src->m,src->x-range,src->y-range,
					src->x+range,src->y+range,BL_CHAR,src,skl->skill_id,skl->skill_lv,tick);
				break;
			case WZ_WATERBALL:
				if (skl->type>1) {
					skl->timer = 0;	// skill_addtimerskill�Ŏg�p����Ȃ��悤��
					skill_addtimerskill(src,tick+150,target->id,0,0,skl->skill_id,skl->skill_lv,skl->type-1,skl->flag);
					skl->timer = -1;
				}
				battle_skill_attack(BF_MAGIC,src,src,target,skl->skill_id,skl->skill_lv,tick,skl->flag);
				break;
			default:
				battle_skill_attack(skl->type,src,src,target,skl->skill_id,skl->skill_lv,tick,skl->flag);
				break;
			}
		} else {
			if(src->m != skl->m)
				break;

			switch(skl->skill_id) {
			case BS_HAMMERFALL:
				range=(skl->skill_lv>5)?AREA_SIZE:2;
				skill_area_temp[1] = skl->src_id;
				skill_area_temp[2] = skl->x;
				skill_area_temp[3] = skl->y;
				map_foreachinarea(skill_area_sub,skl->m,
					skl->x-range,skl->y-range,skl->x+range,skl->y+range,BL_CHAR,
					src,skl->skill_id,skl->skill_lv,tick,skl->flag|BCT_ENEMY|2,
					skill_castend_nodamage_id);
				break;
			case WZ_METEOR:
				if(skl->type >= 0) {
					int x = skl->type>>16, y = skl->type&0xffff;
					if(map_getcell(src->m,x,y,CELL_CHKPASS))
						skill_unitsetting(src,skl->skill_id,skl->skill_lv,x,y,0);
					if(map_getcell(src->m,skl->x,skl->y,CELL_CHKPASS))
						clif_skill_poseffect(src,skl->skill_id,skl->skill_lv,skl->x,skl->y,tick);
				} else {
					if(map_getcell(src->m,skl->x,skl->y,CELL_CHKPASS))
						skill_unitsetting(src,skl->skill_id,skl->skill_lv,skl->x,skl->y,0);
				}
				break;
			case GS_DESPERADO:
				if(map_getcell(src->m,skl->x,skl->y,CELL_CHKPASS))
					skill_unitsetting(src,skl->skill_id,skl->skill_lv,skl->x,skl->y,0);
				break;
			}
		}
	} while(0);

	aFree( skl );
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int skill_addtimerskill(struct block_list *src,unsigned int tick,int target,int x,int y,int skill_id,int skill_lv,int type,int flag)
{
	struct unit_data *ud;
	struct skill_timerskill *skl;

	nullpo_retr(1, src);
	nullpo_retr(1, ud = unit_bl2ud( src ) );

	skl            = (struct skill_timerskill *)aCalloc( 1, sizeof(struct skill_timerskill) );
	skl->timer     = add_timer(tick, skill_timerskill, src->id, (int)skl);
	skl->src_id    = src->id;
	skl->target_id = target;
	skl->skill_id  = skill_id;
	skl->skill_lv  = skill_lv;
	skl->m         = src->m;
	skl->x         = x;
	skl->y         = y;
	skl->type      = type;
	skl->flag      = flag;

	linkdb_insert( &ud->skilltimerskill, skl, skl);
	return 0;
}

/*==========================================
 *
 *------------------------------------------
 */
int skill_cleartimerskill(struct block_list *src)
{
	struct unit_data *ud;
	struct linkdb_node *node1, *node2;

	nullpo_retr(0, src);
	nullpo_retr(0, ud = unit_bl2ud( src ) );

	node1 = ud->skilltimerskill;
	while( node1 ) {
		struct skill_timerskill *skl = (struct skill_timerskill *)node1->data;
		if( skl->timer != -1 ) {
			delete_timer(skl->timer, skill_timerskill);
		}
		node2 = node1->next;
		aFree( skl );
		node1 = node2;
	}
	linkdb_final( &ud->skilltimerskill );

	return 0;
}

/* �͈̓X�L���g�p���������������܂�
 * -------------------------------------------------------------------------
 */

/*==========================================
 * �X�L���g�p�i�r�������AID�w��j
 *------------------------------------------
 */
int skill_castend_id( int tid, unsigned int tick, int id,int data )
{
	struct block_list *target, *src = map_id2bl(id);
	struct map_session_data *src_sd   = NULL;
	struct mob_data         *src_md   = NULL;
	struct homun_data       *src_hd   = NULL;
	struct merc_data        *src_mcd  = NULL;
	struct unit_data        *src_ud   = NULL;
	struct status_change    *tsc      = NULL;
	int inf2;

	nullpo_retr(0, src);
	nullpo_retr(0, src_ud = unit_bl2ud(src));

	if( src->prev == NULL ) // prev�������̂͂���Ȃ́H
		return 0;

	src_sd  = BL_DOWNCAST( BL_PC,   src );
	src_md  = BL_DOWNCAST( BL_MOB,  src );
	src_hd  = BL_DOWNCAST( BL_HOM,  src );
	src_mcd = BL_DOWNCAST( BL_MERC, src );

	if(src_ud->skillid != SA_CASTCANCEL) {
		if( src_ud->skilltimer != tid )	// �^�C�}ID�̊m�F
			return 0;
		if( src_sd && src_ud->skilltimer != -1 && pc_checkskill(src_sd,SA_FREECAST) > 0 ) {
			src_sd->speed = src_sd->prev_speed;
			clif_updatestatus(src_sd,SP_SPEED);
		}
		src_ud->skilltimer = -1;
	}

	target = map_id2bl(src_ud->skilltarget);
	if(target)
		tsc = status_get_sc(target);

	// �X�L�������m�F
	do {
		if(!target || target->prev == NULL)
			break;
		if(src->m != target->m || unit_isdead(src))
			break;

		// ���̒� �s������
		if(tsc && tsc->data[SC_FOGWALL].timer != -1 && skill_get_misfire(src_ud->skillid) && atn_rand()%100 < 75)
			break;

		if(src_ud->skillid == PR_LEXAETERNA) {
			if(tsc && (tsc->data[SC_FREEZE].timer != -1 || (tsc->data[SC_STONE].timer != -1 && tsc->data[SC_STONE].val2 == 0))) {
				break;
			}
		} else if(src_ud->skillid == RG_BACKSTAP) {
			int dir   = map_calc_dir(src,target->x,target->y);
			int t_dir = status_get_dir(target);
			int dist  = unit_distance2(src,target);
			if(target->type != BL_SKILL && (dist == 0 || map_check_dir(dir,t_dir)))
				break;
		}

		// ���ق��Ԉُ�Ȃ�
		if(src_md) {
			if(src_md->sc.data[SC_ROKISWEIL].timer != -1)
				break;
			if(!(mob_db[src_md->class_].mode & 0x20) && src_md->sc.data[SC_HERMODE].timer != -1)
				break;
			if(src_md->sc.opt1 > 0 || src_md->sc.data[SC_SILENCE].timer != -1 || src_md->sc.data[SC_STEELBODY].timer != -1)
				break;
			if(src_md->sc.data[SC_AUTOCOUNTER].timer != -1 && src_md->ud.skillid != KN_AUTOCOUNTER)
				break;
			if(src_md->sc.data[SC_BLADESTOP].timer != -1)
				break;
			if(src_md->sc.data[SC_BERSERK].timer != -1)
				break;

			if(src_md->ud.skillid != NPC_EMOTION)
				src_md->last_thinktime = tick + status_get_adelay(src);
			if(src_md->skillidx >= 0)
				src_md->skilldelay[src_md->skillidx] = tick;
		}

		inf2 = skill_get_inf2(src_ud->skillid);
		if(inf2 & 0x04 || skill_get_inf(src_ud->skillid) & 0x01) {
			int fail_flag = 1;
			switch(src_ud->skillid) {	// �G�ȊO���^�[�Q�b�g�ɂ��Ă��ǂ��X�L��
				case AS_GRIMTOOTH:
				case KN_BRANDISHSPEAR:
				case SN_SHARPSHOOTING:
				case GS_SPREADATTACK:
				case NJ_HUUMA:
				case NJ_BAKUENRYU:
				case NJ_KAMAITACHI:
				case MA_SHARPSHOOTING:
				case ML_BRANDISH:
				case PR_LEXDIVINA:
				case MER_LEXDIVINA:
					fail_flag = 0;
					break;
				case SA_SPELLBREAKER:
					if(map[src->m].flag.nopenalty)	// �X���̂�PC�ɗL��
						fail_flag = 0;
					break;
			}
			if(fail_flag) {
				if(battle_check_target(src,target,BCT_ENEMY) <= 0)	// �މ�G�Ί֌W�`�F�b�N
					break;
			}
		}
		if(inf2 & 0xC00 && src->id != target->id) {
			int fail_flag = 1;
			if(inf2 & 0x400 && battle_check_target(src,target,BCT_PARTY) > 0)
				fail_flag = 0;
			else if(src_sd && inf2 & 0x800 && src_sd->status.guild_id > 0 && src_sd->status.guild_id == status_get_guild_id(target))
				fail_flag = 0;
			if(fail_flag) {
				break;
			}
		}

		if(skill_get_nk(src_ud->skillid)&4) {
			// �ː��`�F�b�N
			if(!path_search_long(NULL,src->m,src->x,src->y,target->x,target->y)) {
				if(src_sd && battle_config.skill_out_range_consume)
					skill_check_condition(&src_sd->bl,1);	// �A�C�e������
				break;
			}
		}

		// PC,HOM,MERC�͎g�p�����`�F�b�N
		if(src_sd || src_hd || src_mcd) {
			if(!skill_check_condition(src,1))
				break;
		}
		if(src_sd) {
			src_sd->skillitem      = -1;
			src_sd->skillitemlv    = -1;
			src_sd->skillitem_flag = 0;
		}

		if(battle_config.pc_skill_log)
			printf("PC %d skill castend skill=%d\n",src->id,src_ud->skillid);
		unit_stop_walking(src,0);

		switch( skill_get_nk(src_ud->skillid)&3 )
		{
		case 0:	/* �U���n */
		case 2:	/* ������΂��n */
			skill_castend_damage_id(src,target,src_ud->skillid,src_ud->skilllv,tick,0);
			break;
		case 1:	/* �x���n */
			if( (src_ud->skillid == AL_HEAL ||
			     src_ud->skillid == PR_SANCTUARY ||
			     src_ud->skillid == ALL_RESURRECTION ||
			     src_ud->skillid == PR_ASPERSIO) &&
			    battle_check_undead(status_get_race(target),status_get_elem_type(target)) &&
			    !(src_md && target->type == BL_MOB) )	// MOB��MOB�Ȃ�A���f�b�h�ł���
			{
				if( target->type != BL_PC ||
				    (src_md && src_md->skillidx >= 0 && !mob_db[src_md->class_].skill[src_md->skillidx].val[0]) ) {
					skill_castend_damage_id(src,target,src_ud->skillid,src_ud->skilllv,tick,0);
				} else if( map[src->m].flag.pvp || map[src->m].flag.gvg ) {
					if(src_ud->skillid == AL_HEAL && battle_check_target(src,target,BCT_PARTY))
						break;
					skill_castend_damage_id(src,target,src_ud->skillid,src_ud->skilllv,tick,0);
				} else {
					break;
				}
			} else {
				skill_castend_nodamage_id(src,target,src_ud->skillid,src_ud->skilllv,tick,0);
			}
			break;
		}
		if( src_md )
			src_md->skillidx = -1;
		return 0;
	} while(0);

	// �X�L���g�p���s
	src_ud->canact_tick  = tick;
	src_ud->canmove_tick = tick;
	if(src_sd) {
		src_sd->skillitem      = -1;
		src_sd->skillitemlv    = -1;
		src_sd->skillitem_flag = 0;
	} else if(src_md) {
		src_md->skillidx = -1;
	}

	return 0;
}

/*==========================================
 * �X�L���g�p�i�r�������AID�w��U���n�j
 *------------------------------------------
 */
int skill_castend_damage_id( struct block_list* src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag )
{
	struct map_session_data *sd  = NULL;
	struct mob_data         *md  = NULL;
	struct homun_data       *hd  = NULL;
	struct merc_data        *mcd = NULL;
	struct status_change    *sc  = NULL;
	int is_enemy = 1;

	nullpo_retr(1, src);
	nullpo_retr(1, bl);

	if(bl->prev == NULL)
		return 1;
	if(unit_isdead(bl))
		return 1;

	sd  = BL_DOWNCAST( BL_PC,   src );
	md  = BL_DOWNCAST( BL_MOB,  src );
	hd  = BL_DOWNCAST( BL_HOM,  src );
	mcd = BL_DOWNCAST( BL_MERC, src );

	switch(skillid) {
		case CR_GRANDCROSS:
		case NPC_GRANDDARKNESS:
			if(src != bl)
				bl = src;
			break;
		case AS_GRIMTOOTH:
		case KN_BRANDISHSPEAR:
		case SN_SHARPSHOOTING:
		case GS_SPREADATTACK:
		case NJ_HUUMA:
		case NJ_BAKUENRYU:
		case NJ_KAMAITACHI:
		case MA_SHARPSHOOTING:
		case ML_BRANDISH:
			// skill_castend_id�ŋ������X�L���͂����œG�`�F�b�N
			if(skill_get_inf2(skillid) & 0x04 || skill_get_inf(skillid) & 0x01) {
				if(battle_check_target(src,bl,BCT_ENEMY) <= 0)
					is_enemy = 0;
			}
			break;
	}

	// �G��
	if(md && md->skillidx != -1)
	{
		short emotion = mob_db[md->class_].skill[md->skillidx].emotion;
		if(emotion >= 0)
			clif_emotion(&md->bl,emotion);
	}

	map_freeblock_lock();

	switch(skillid)
	{
	/* ����U���n�X�L�� */
	case SM_BASH:			/* �o�b�V�� */
	case MC_MAMMONITE:		/* ���}�[�i�C�g */
	case KN_PIERCE:			/* �s�A�[�X */
	case KN_SPEARBOOMERANG:	/* �X�s�A�u�[������ */
	case TF_POISON:			/* �C���x�i�� */
	case TF_SPRINKLESAND:	/* ���܂� */
	case AC_CHARGEARROW:	/* �`���[�W�A���[ */
	case RG_RAID:			/* �T�v���C�Y�A�^�b�N */
	case ASC_METEORASSAULT:	/* ���e�I�A�T���g */
	case RG_INTIMIDATE:		/* �C���e�B�~�f�C�g */
	case AM_ACIDTERROR:		/* �A�V�b�h�e���[ */
	case BA_MUSICALSTRIKE:	/* �~���[�W�J���X�g���C�N */
	case DC_THROWARROW:		/* ��� */
	case BA_DISSONANCE:		/* �s���a�� */
	case CR_HOLYCROSS:		/* �z�[���[�N���X */
	case CR_SHIELDCHARGE:
	case CR_SHIELDBOOMERANG:
	case NPC_PIERCINGATT:
	case NPC_MENTALBREAKER:
	case NPC_RANGEATTACK:
	case NPC_CRITICALSLASH:
	case NPC_COMBOATTACK:
	case NPC_GUIDEDATTACK:
	case NPC_POISON:
	case NPC_BLINDATTACK:
	case NPC_SILENCEATTACK:
	case NPC_STUNATTACK:
	case NPC_PETRIFYATTACK:
	case NPC_CURSEATTACK:
	case NPC_SLEEPATTACK:
	case NPC_RANDOMATTACK:
	case NPC_WATERATTACK:
	case NPC_GROUNDATTACK:
	case NPC_FIREATTACK:
	case NPC_WINDATTACK:
	case NPC_POISONATTACK:
	case NPC_HOLYATTACK:
	case NPC_DARKNESSATTACK:
	case NPC_TELEKINESISATTACK:
	case NPC_UNDEADATTACK:
	case NPC_WEAPONBRAKER:
	case NPC_ARMORBRAKE:
	case NPC_HELMBRAKE:
	case NPC_SHIELDBRAKE:
	case NPC_DARKCROSS:
	case LK_SPIRALPIERCE:		/* �X�p�C�����s�A�[�X */
	case LK_HEADCRUSH:			/* �w�b�h�N���b�V�� */
	case LK_JOINTBEAT:			/* �W���C���g�r�[�g */
	case ASC_BREAKER:			/* �\�E���u���[�J�[ */
	case HW_MAGICCRASHER:		/* �}�W�b�N�N���b�V���[ */
	case KN_BRANDISHSPEAR:		/* �u�����f�B�b�V���X�s�A */
	case PA_SHIELDCHAIN:		/* �V�[���h�`�F�C�� */
	case WS_CARTTERMINATION:	/* �J�[�g�^�[�~�l�[�V���� */
	case CR_ACIDDEMONSTRATION:	/* �A�V�b�h�f�����X�g���[�V���� */
	case ITM_TOMAHAWK:			/* �g�}�z�[�N���� */
	case AS_VENOMKNIFE:			/* �x�i���i�C�t */
	case HT_PHANTASMIC:			/* �t�@���^�X�~�b�N�A���[ */
	case CH_TIGERFIST:		/* ���Ռ� */
	case CH_CHAINCRUSH:		/* �A������ */
	case TK_DOWNKICK:	/* �l�����`���M */
	case TK_COUNTER:	/* �A�v�`���I�����M */
	case GS_FLING:			/* �t���C���O */
	case GS_TRIPLEACTION:	/* �g���v���A�N�V���� */
	case GS_MAGICALBULLET:	/* �}�W�J���o���b�g */
	case GS_TRACKING:		/* �g���b�L���O */
	case GS_RAPIDSHOWER:	/* ���s�b�h�V�����[ */
	case GS_DUST:			/* �_�X�g */
	case GS_PIERCINGSHOT:	/* �s�A�[�V���O�V���b�g */
	case GS_FULLBUSTER:		/* �t���o�X�^�[ */
	case NJ_SYURIKEN:		/* �藠������ */
	case NJ_KUNAI:			/* �ꖳ���� */
	case NJ_ZENYNAGE:		/* �K���� */
	case NJ_KASUMIKIRI:		/* ���a�� */
	case HFLI_MOON:
	case HFLI_SBR44:
	case NPC_BLEEDING:		/* �o���U�� */
	case NPC_CRITICALWOUND:		/* �v�����U�� */
	case NPC_EXPULSION:		/* �G�N�X�p���V�I�� */
	case MS_BASH:
	case MA_CHARGEARROW:
	case ML_PIERCE:
	case ML_BRANDISH:
	case ML_SPIRALPIERCE:
	case MER_CRASH:			/* �N���b�V�� */
		battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		break;
	case AC_DOUBLE:			/* �_�u���X�g���C�t�B���O */
	case MA_DOUBLE:
		status_change_start(src,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		break;
	case GS_DISARM:			/* �f�B�X�A�[�� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		break;
	case AS_SONICBLOW:		/* �\�j�b�N�u���[ */
	case CG_ARROWVULCAN:		/* �A���[�o���J�� */
		{
			struct unit_data *ud = unit_bl2ud(src);
			battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
			if(ud) {
				// �X�L�����[�V�����f�B���C�͍ő��3�b���炢�H
				int delay = status_get_adelay(src);
				ud->canmove_tick = tick + ( (delay>2000)? 3000: 6000*1000/(4000-delay) );
			}
		}
		break;
	case HT_POWER:			/* �s�[�X�g�X�g���C�t�B���O*/
		status_change_end(src,SC_DOUBLE,-1);
		battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		break;
	case MO_INVESTIGATE:	/* ���� */
		battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		sc = status_get_sc(src);
		if(sc && sc->data[SC_BLADESTOP].timer != -1)
			status_change_end(src,SC_BLADESTOP,-1);
		break;
	case RG_BACKSTAP:		/* �o�b�N�X�^�u */
		{
			int dir   = map_calc_dir(src,bl->x,bl->y);
			int t_dir = status_get_dir(bl);
			int dist  = unit_distance2(src,bl);
			if((dist > 0 && !map_check_dir(dir,t_dir)) || bl->type == BL_SKILL) {
				sc = status_get_sc(src);
				if(sc && sc->data[SC_HIDING].timer != -1)
					status_change_end(src, SC_HIDING, -1);	// �n�C�f�B���O����
				if(battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag)>0) { // �U�����󂯂��ڕW�͐U�����
					unit_setdir(bl, map_calc_dir(bl,src->x,src->y));
				}
			} else if(sd) {
				clif_skill_fail(sd,skillid,0,0);
			}
		}
		break;
	case MO_FINGEROFFENSIVE:	/* �w�e */
		battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		if(md && !mob_is_pcview(md->class_)) {
			// �����ڂ�PC�łȂ��ꍇ�͉��̂��p�������Ă��܂��̂ŗ}������
			clif_skill_nodamage(src,src,skillid,skilllv,1);
		}
		if(battle_config.finger_offensive_type && sd) {
			int i;
			for(i=1; i<sd->spiritball_old; i++)
				skill_addtimerskill(src,tick+i*200,bl->id,0,0,skillid,skilllv,BF_WEAPON,flag);
			sd->ud.canmove_tick = tick + (sd->spiritball_old-1)*200;
		}
		sc = status_get_sc(src);
		if(sc && sc->data[SC_BLADESTOP].timer != -1)
			status_change_end(src,SC_BLADESTOP,-1);
		break;
	case MO_CHAINCOMBO:		/* �A�ŏ� */
		battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		sc = status_get_sc(src);
		if(sc && sc->data[SC_BLADESTOP].timer != -1)
			status_change_end(src,SC_BLADESTOP,-1);
		break;
	case TK_STORMKICK:	/* �t�F�I���`���M */
		if(flag&1) {
			if(bl->id != skill_area_temp[1])
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0);
		} else {
			skill_area_temp[1]=src->id;
			map_foreachinarea(skill_area_sub,
				src->m,src->x-2,src->y-2,src->x+2,src->y+2,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
			clif_skill_nodamage(src,src,skillid,skilllv,1);
		}
		break;

	case TK_TURNKICK:	/* �g�������`���M */
		if(flag&1){
			/* �ʏ��� */
			if(bl->id != skill_area_temp[1]) {
				struct block_list pos;
				memset(&pos,0,sizeof(pos));
				pos.m = bl->m;
				pos.x = skill_area_temp[2];
				pos.y = skill_area_temp[3];
				skill_blown(&pos,bl,skill_area_temp[4] | SAB_NODAMAGE);
				skill_additional_effect(src,bl,skillid,skilllv,BF_WEAPON,tick);
			}
		} else {
			skill_area_temp[1] = bl->id;
			skill_area_temp[2] = bl->x;
			skill_area_temp[3] = bl->y;
			skill_area_temp[4] = skill_get_blewcount(skillid,skilllv);
			/* �܂��^�[�Q�b�g�ɍU���������� */
			if(!battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0))
				break;
			/* ���̌�^�[�Q�b�g�ȊO�͈͓̔��̓G�S�̂ɏ������s�� */
			map_foreachinarea(skill_area_sub,
				bl->m,skill_area_temp[2]-1,skill_area_temp[3]-1,skill_area_temp[2]+1,skill_area_temp[3]+1,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;

	case KN_CHARGEATK:	/* �`���[�W�A�^�b�N */
	case TK_JUMPKICK:	/* �e�B�I�A�v�`���M */
	case NJ_ISSEN:		/* ��M */
		{
			int dist = unit_distance2(src,bl);
			if(sd && (skillid != KN_CHARGEATK || battle_config.gvg_chargeattack_move || !map[sd->bl.m].flag.gvg)) {
				int dx = bl->x - sd->bl.x;
				int dy = bl->y - sd->bl.y;

				if(dx > 0) dx++;
				else if(dx < 0) dx--;
				if(dy > 0) dy++;
				else if(dy < 0) dy--;
				if(dx == 0 && dy == 0) dx++;
				if(path_search(NULL,src->m,sd->bl.x,sd->bl.y,sd->bl.x+dx,sd->bl.y+dy,1) == -1) {
					dx = bl->x - sd->bl.x;
					dy = bl->y - sd->bl.y;
					if(path_search(NULL,src->m,sd->bl.x,sd->bl.y,sd->bl.x+dx,sd->bl.y+dy,1) == -1) {
						clif_skill_fail(sd,sd->ud.skillid,0,0);
						break;
					}
				}
				sd->ud.to_x = sd->bl.x + dx;
				sd->ud.to_y = sd->bl.y + dy;
				clif_skill_poseffect(&sd->bl,skillid,skilllv,sd->bl.x,sd->bl.y,tick);
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,dist);
				clif_walkok(sd);
				clif_move(&sd->bl);
				if(dx < 0) dx = -dx;
				if(dy < 0) dy = -dy;
				sd->ud.attackabletime = sd->ud.canmove_tick = tick + sd->speed * ((dx > dy)? dx:dy);
				if(sd->ud.canact_tick < sd->ud.canmove_tick)
					sd->ud.canact_tick = sd->ud.canmove_tick;
				unit_movepos(&sd->bl,sd->ud.to_x,sd->ud.to_y,0);
			} else {
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,dist);
			}
			if(skillid == TK_JUMPKICK) {
				sc = status_get_sc(src);
				if(sc && sc->data[SC_RUN].timer != -1)
					status_change_end(src,SC_RUN,-1);
			}
		}
		break;
	case MO_COMBOFINISH:	/* �җ��� */
		sc = status_get_sc(src);
		/* �����N�̍���Ԃ̏ꍇ�͔͈͍U�� */
		if(sc && sc->data[SC_MONK].timer != -1) {
			if(flag&1) {
				if(bl->id != skill_area_temp[1])
					battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
			} else {
				skill_area_temp[1] = bl->id;
				skill_area_temp[2] = bl->x;
				skill_area_temp[3] = bl->y;
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
				map_foreachinarea(skill_area_sub,
					src->m,bl->x-2,bl->y-2,bl->x+2,bl->y+2,(BL_CHAR|BL_SKILL),
					src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
					skill_castend_damage_id);
			}
		} else {
			battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		}
		break;
	case CH_PALMSTRIKE:		/* �ҌՍd঎R */
		clif_damage(src,bl,tick,status_get_amotion(src),0,-1,1,4,0);	// �U�����[�V�����̂ݓ����
		skill_addtimerskill(src,tick+1000,bl->id,0,0,skillid,skilllv,BF_WEAPON,flag);
		break;
	case MO_EXTREMITYFIST:	/* ���C���e�P�� */
		if(sd) {
			int dx = bl->x - sd->bl.x;
			int dy = bl->y - sd->bl.y;

			if(dx > 0) dx++;
			else if(dx < 0) dx--;
			if(dy > 0) dy++;
			else if(dy < 0) dy--;
			if(dx == 0 && dy == 0) dx++;
			if(path_search(NULL,src->m,sd->bl.x,sd->bl.y,sd->bl.x+dx,sd->bl.y+dy,1) == -1) {
				dx = bl->x - sd->bl.x;
				dy = bl->y - sd->bl.y;
				if(path_search(NULL,src->m,sd->bl.x,sd->bl.y,sd->bl.x+dx,sd->bl.y+dy,1) == -1) {
					clif_skill_fail(sd,sd->ud.skillid,0,0);
					break;
				}
			}
			sd->ud.to_x = sd->bl.x + dx;
			sd->ud.to_y = sd->bl.y + dy;
			battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
			clif_walkok(sd);
			clif_move(&sd->bl);
			if(dx < 0) dx = -dx;
			if(dy < 0) dy = -dy;
			sd->ud.attackabletime = sd->ud.canmove_tick = tick + 100 + sd->speed * ((dx > dy)? dx:dy);
			if(sd->ud.canact_tick < sd->ud.canmove_tick)
				sd->ud.canact_tick = sd->ud.canmove_tick;
			unit_movepos(&sd->bl,sd->ud.to_x,sd->ud.to_y,0);
			status_change_end(&sd->bl,SC_COMBO,-1);
		} else {
			battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		}
		status_change_end(src, SC_EXPLOSIONSPIRITS, -1);
		sc = status_get_sc(src);
		if(sc && sc->data[SC_BLADESTOP].timer != -1) {
				status_change_end(src,SC_BLADESTOP,-1);
		}
		break;
	case GS_BULLSEYE:		/* �u���Y�A�C */
		{
			int race = status_get_race(bl);
			if(race == RCT_BRUTE || race == RCT_HUMAN) {
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
			} else {
				if(sd)
					clif_skill_fail(sd,skillid,0,0);
			}
		}
		break;
	case NJ_KIRIKAGE:		/* �e�a�� */
		{
			int dist = unit_distance2(src,bl);
			if(sd && pc_checkskill(sd,NJ_SHADOWJUMP) + 4 >= dist) {
				int dx = bl->x - sd->bl.x;
				int dy = bl->y - sd->bl.y;

				if(dx > 0) dx++;
				else if(dx < 0) dx--;
				if(dy > 0) dy++;
				else if(dy < 0) dy--;
				if(dx == 0 && dy == 0) dx++;
				if(path_search(NULL,src->m,sd->bl.x,sd->bl.y,sd->bl.x+dx,sd->bl.y+dy,1) == -1) {
					dx = bl->x - sd->bl.x;
					dy = bl->y - sd->bl.y;
					if(path_search(NULL,src->m,sd->bl.x,sd->bl.y,sd->bl.x+dx,sd->bl.y+dy,1) == -1) {
						clif_skill_fail(sd,sd->ud.skillid,0,0);
						break;
					}
				}
				sd->ud.to_x = sd->bl.x + dx;
				sd->ud.to_y = sd->bl.y + dy;
				clif_skill_poseffect(&sd->bl,skillid,skilllv,sd->bl.x,sd->bl.y,tick);
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,dist);
				clif_walkok(sd);
				clif_move(&sd->bl);
				if(dx < 0) dx = -dx;
				if(dy < 0) dy = -dy;
				sd->ud.attackabletime = sd->ud.canmove_tick = tick + sd->speed * ((dx > dy)? dx:dy);
				if(sd->ud.canact_tick < sd->ud.canmove_tick)
					sd->ud.canact_tick = sd->ud.canmove_tick;
				unit_movepos(&sd->bl,sd->ud.to_x,sd->ud.to_y,0);
			} else {
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,dist);
			}
			status_change_end(src, SC_HIDING, -1);	// �n�C�f�B���O����
		}
		break;

	/* ����n�͈͍U���X�L�� */
	case AC_SHOWER:			/* �A���[�V�����[ */
	case MA_SHOWER:
		{
			// �w��Z�����U�����S�ɂ��邽�߂�src�̑����p�ӂ���
			struct block_list pos;
			memset(&pos,0,sizeof(struct block_list));
			pos.m = bl->m;
			pos.x = skill_area_temp[2];
			pos.y = skill_area_temp[3];
			if( battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500) ) {
				int count = skill_get_blewcount(skillid,skilllv)|SAB_NOPATHSTOP;
				if(bl->x == pos.x && bl->y == pos.y)
					count |= 6<<20;		// �w����W�Ɠ���Ȃ琼�փm�b�N�o�b�N
				skill_blown(&pos,bl,count);
			}
		}
		break;
	case SM_MAGNUM:			/* �}�O�i���u���C�N */
	case MS_MAGNUM:
		if(flag&1) {
			/* �ʂɃ_���[�W��^���� */
			if(bl->id != skill_area_temp[1]) {
				int x = skill_area_temp[2], y = skill_area_temp[3];
				int type;
				if(unit_distance(bl->x,bl->y,x,y) > 1)
					type = 1;	// �O��
				else
					type = 0;	// ����
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500|type);
			}
		} else {
			/* �X�L���G�t�F�N�g�\�� */
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(src,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
			skill_area_temp[1] = src->id;
			skill_area_temp[2] = src->x;
			skill_area_temp[3] = src->y;
			map_foreachinarea(skill_area_sub,
				src->m,src->x-2,src->y-2,src->x+2,src->y+2,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;
	case NPC_SPLASHATTACK:	/* �X�v���b�V���A�^�b�N */
		if(flag&1) {
			/* �ʂɃ_���[�W��^���� */
			if(bl->id != skill_area_temp[1])
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500);
		} else {
			int ar = 3;
			skill_area_temp[1] = bl->id;
			/* �܂��^�[�Q�b�g�ɍU���������� */
			battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0);
			/* ���̌�^�[�Q�b�g�ȊO�͈͓̔��̓G�S�̂ɏ������s�� */
			map_foreachinarea(skill_area_sub,
				bl->m,bl->x-ar,bl->y-ar,bl->x+ar,bl->y+ar,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;
	case AS_SPLASHER:		/* �x�i���X�v���b�V���[ */
		if(flag&1) {
			battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,(0x0f<<20)|0x500);
		} else {
			int ar = 2;
			map_foreachinarea(skill_area_sub,
				bl->m,bl->x-ar,bl->y-ar,bl->x+ar,bl->y+ar,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;
	case AS_GRIMTOOTH:		/* �O�����g�D�[�X */
		if(flag&1) {
			/* �ʂɃ_���[�W��^���� */
			if(bl->id != skill_area_temp[1])
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500);
		} else {
			int ar = 1;
			skill_area_temp[1] = bl->id;
			/* �܂��^�[�Q�b�g�ɍU���������� */
			battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,(is_enemy ? 0 : 0x01000000));
			/* ���̌�^�[�Q�b�g�ȊO�͈͓̔��̓G�S�̂ɏ������s�� */
			map_foreachinarea(skill_area_sub,
				bl->m,bl->x-ar,bl->y-ar,bl->x+ar,bl->y+ar,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
			map_foreachinarea(skill_area_trap_sub,
				bl->m,bl->x-ar,bl->y-ar,bl->x+ar,bl->y+ar,BL_SKILL,
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;
	case MC_CARTREVOLUTION:	/* �J�[�g�����H�����[�V���� */
		if(flag&1) {
			/* �ʂɃ_���[�W��^���� */
			if(bl->id == skill_area_temp[1])
				break;
			if(battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500))
			{
				if(bl->x == skill_area_temp[2] && bl->y == skill_area_temp[3]) {
					skill_blown(src,bl,skill_area_temp[4]|(6<<20));		// �^�[�Q�b�g�Ɠ�����W�Ȃ琼�փm�b�N�o�b�N
				} else {
					struct block_list pos;
					memset(&pos,0,sizeof(pos));
					pos.m = bl->m;
					pos.x = skill_area_temp[2];
					pos.y = skill_area_temp[3];
					skill_blown(&pos,bl,skill_area_temp[4]);		// �^�[�Q�b�g�Ƃ̈ʒu�֌W�Ŕ�΂����������߂�
				}
			}
		} else {
			int x = bl->x, y = bl->y;
			skill_area_temp[1] = bl->id;
			skill_area_temp[2] = x;
			skill_area_temp[3] = y;
			skill_area_temp[4] = skill_get_blewcount(skillid,skilllv);
			/* �܂��^�[�Q�b�g�ɍU���������� */
			if(battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0))
				skill_blown(src,bl,skill_area_temp[4]|(6<<20));		// ���ɋ����m�b�N�o�b�N

			/* ���̌�^�[�Q�b�g�ȊO�͈͓̔��̓G�S�̂ɏ������s�� */
			map_foreachinarea(skill_area_sub,
				bl->m,x-1,y-1,x+1,y+1,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;
	case KN_BOWLINGBASH:	/* �{�E�����O�o�b�V�� */
	case MS_BOWLINGBASH:
		if(flag&1) {
			/* �ʂɃ_���[�W��^���� */
			if(bl->id!=skill_area_temp[1]) {
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500);
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500);
			}
		} else {
			int i,c,dir;	/* ���l���畷���������Ȃ̂ŊԈ���Ă�\���偕������������������ */
			/* �܂��^�[�Q�b�g�ɍU���������� */
			if(!battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0))
				break;
			battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0);
			c   = skill_get_blewcount(skillid,skilllv);
			dir = (status_get_dir(src)+4) & 0x07;
			if(dir == 0)
				dir = 8;
			if(map[bl->m].flag.gvg) c = 0;
			for(i=0; i<c; i++) {
				skill_blown(src,bl,(dir<<20)|SAB_NODAMAGE|1);
				skill_area_temp[0] = 0;
				map_foreachinarea(skill_area_sub,
					bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,(BL_CHAR|BL_SKILL),
					src,skillid,skilllv,tick, flag|BCT_ENEMY ,
					skill_area_sub_count);
				if(skill_area_temp[0] > 1)
					break;
			}
			unit_stop_walking(bl,2);	// �Ō�Ƀ_���[�W�f�B���C������
			skill_area_temp[1] = bl->id;
			/* ���̌�^�[�Q�b�g�ȊO�͈͓̔��̓G�S�̂ɏ������s�� */
			map_foreachinarea(skill_area_sub,
				bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;
	case MO_BALKYOUNG:
		battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,flag);
		map_foreachinarea(skill_balkyoung,bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,(BL_CHAR|BL_SKILL),src,bl);
		break;
	case KN_SPEARSTAB:		/* �X�s�A�X�^�u */
		if(flag&1) {
			/* �ʂɃ_���[�W��^���� */
			if(bl->id == skill_area_temp[1])
				break;
			if(battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500))
				skill_blown(src,bl,skill_area_temp[2]);
		} else {
			int x = bl->x, y = bl->y;
			int i, dir = map_calc_dir(bl,src->x,src->y);
			if(dir == 0)
				dir = 8;
			skill_area_temp[1] = bl->id;
			skill_area_temp[2] = skill_get_blewcount(skillid,skilllv)|(dir<<20);
			if(map[bl->m].flag.gvg)
				skill_area_temp[2] = 0;
			/* �܂��^�[�Q�b�g�ɍU���������� */
			if(battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0))
				skill_blown(src,bl,skill_area_temp[2]);
			for(i=0; i<4; i++) {
				map_foreachinarea(skill_area_sub,bl->m,x,y,x,y,(BL_CHAR|BL_SKILL),
					src,skillid,skilllv,tick,flag|BCT_ENEMY|1,
					skill_castend_damage_id);
				x += dirx[dir];
				y += diry[dir];
			}
		}
		break;
	case SN_SHARPSHOOTING:			/* �V���[�v�V���[�e�B���O */
	case MA_SHARPSHOOTING:
		if(flag&1) {
			/* �ʂɃ_���[�W��^���� */
			battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,(skill_area_temp[1] == 0 ? 0 : 0x0500));
			skill_area_temp[1]++;
		} else {
			int dir = map_calc_dir(src,bl->x,bl->y);
			skill_area_temp[1] = 0;
			map_foreachinshootpath(
				skill_area_sub,bl->m,src->x,src->y,dirx[dir],diry[dir],12,1,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick,flag|BCT_ENEMY|1,skill_castend_damage_id
			);
			if(skill_area_temp[1] == 0) {
				/* �^�[�Q�b�g�ɍU�� */
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,(is_enemy ? 0 : 0x01000000));
			}
		}
		break;
	case GS_SPREADATTACK:	/* �X�v���b�h�A�^�b�N */
		if(flag&1) {
			if(bl->id != skill_area_temp[1])
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500);
		} else {
			int ar = (skilllv-1)/3+1;
			skill_area_temp[1] = bl->id;
			battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,(is_enemy ? 0 : 0x01000000));
			map_foreachinarea(skill_area_sub,
				src->m,bl->x-ar,bl->y-ar,bl->x+ar,bl->y+ar,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick,flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;
	case NJ_HUUMA:		/* �����藠������ */
		if(flag&1) {
			if(bl->id != skill_area_temp[1])
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,skill_area_temp[0]);
		} else {
			skill_area_temp[0] = 0;
			skill_area_temp[1] = bl->id;
			map_foreachinarea(skill_area_sub,
				bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick,flag|BCT_ENEMY,
				skill_area_sub_count);
			if( !battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,skill_area_temp[0]|(is_enemy ? 0 : 0x01000000)) )
				break;
			map_foreachinarea(skill_area_sub,
				bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick,flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;
	case NPC_EARTHQUAKE:		/* �A�[�X�N�G�C�N */
		if(flag&1) {
			if(bl->id != skill_area_temp[1])
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,skill_area_temp[0]);
		} else {
			int ar = 5+(skilllv-1)%5*2;
			skill_area_temp[0] = 0;
			skill_area_temp[1] = bl->id;
			map_foreachinarea(skill_area_sub,
				src->m,src->x-ar,src->y-ar,src->x+ar,src->y+ar,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick,flag|BCT_ENEMY,
				skill_area_sub_count);
			map_foreachinarea(skill_area_sub,
				src->m,src->x-ar,src->y-ar,src->x+ar,src->y+ar,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;
	case NPC_FIREBREATH:		/* �t�@�C�A�u���X */
	case NPC_ICEBREATH:		/* �A�C�X�u���X */
	case NPC_THUNDERBREATH:		/* �T���_�[�u���X */
	case NPC_ACIDBREATH:		/* �A�V�b�h�u���X */
	case NPC_DARKNESSBREATH:	/* �_�[�N�l�X�u���X */
		if(flag&1) {
			battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0);
		} else {
			int dir = map_calc_dir(src,bl->x,bl->y);
			map_foreachinshootpath(
				skill_area_sub,bl->m,src->x,src->y,dirx[dir],diry[dir],14,4,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick,flag|BCT_ENEMY|1,skill_castend_damage_id
			);
		}
		break;
	case NPC_PULSESTRIKE:		/* �p���X�X�g���C�N */
		if(flag&1) {
			/* �ʂɃ_���[�W��^���� */
			if(bl->id != skill_area_temp[1]) {
				if(battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0))
					skill_blown(src,bl,skill_area_temp[2]);
			}
		} else {
			skill_area_temp[1] = bl->id;
			skill_area_temp[2] = skill_get_blewcount(skillid,skilllv);
			map_foreachinarea(skill_area_sub,
				src->m,src->x-7,src->y-7,src->x+7,src->y+7,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;
	case NPC_HELLJUDGEMENT:		/* �w���W���b�W�����g */
		if(flag&1) {
			/* �ʂɃ_���[�W��^���� */
			if(bl->id != skill_area_temp[1]) {
				battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0);
			}
		} else {
			skill_area_temp[1] = bl->id;
			map_foreachinarea(skill_area_sub,
				src->m,src->x-14,src->y-14,src->x+14,src->y+14,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;

	/* ���@�n�X�L�� */
	case MG_SOULSTRIKE:			/* �\�E���X�g���C�N */
	case NPC_DARKSTRIKE:		/* �_�[�N�X�g���C�N */
	case MG_COLDBOLT:			/* �R�[���h�{���g */
	case MG_FIREBOLT:			/* �t�@�C�A�[�{���g */
	case MG_LIGHTNINGBOLT:		/* ���C�g�j���O�{���g*/
	case WZ_EARTHSPIKE:			/* �A�[�X�X�p�C�N */
	case AL_HEAL:				/* �q�[�� */
	case AL_HOLYLIGHT:			/* �z�[���[���C�g */
	case WZ_JUPITEL:			/* ���s�e���T���_�[ */
	case NPC_DARKTHUNDER:		/* �_�[�N�T���_�[ */
	case NPC_MAGICALATTACK:		/* ���@�Ō��U�� */
	case PR_ASPERSIO:			/* �A�X�y���V�I */
	case NJ_KOUENKA:			/* �g���� */
	case NJ_HYOUSENSOU:			/* �X�M�� */
	case NJ_HUUJIN:				/* ���n */
		battle_skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		break;
	case ALL_RESURRECTION:		/* ���U���N�V���� */
	case PR_TURNUNDEAD:			/* �^�[���A���f�b�h */
		if(battle_check_undead(status_get_race(bl),status_get_elem_type(bl))) {
			battle_skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		} else {
			map_freeblock_unlock();
			return 1;
		}
		break;
	case HVAN_CAPRICE:		/* �J�v���X */
		{
			static int caprice[4] = { MG_COLDBOLT,MG_FIREBOLT,MG_LIGHTNINGBOLT,WZ_EARTHSPIKE};
			battle_skill_attack(BF_MAGIC,src,src,bl,caprice[atn_rand()%4],skilllv,tick,flag);
			clif_skill_nodamage(src,src,skillid,skilllv,1);
		}
		break;
	case CG_TAROTCARD:		/* �^���̃^���b�g�J�[�h */
		skill_tarot_card_of_fate(src,bl,skillid,skilllv,0);
		break;
	case MG_FROSTDIVER:		/* �t���X�g�_�C�o�[ */
		{
			int damage;
			int sc_def_mdef = 100 - (3 + status_get_mdef(bl) + status_get_luk(bl) / 3);
			int rate = (skilllv * 3 + 35) * sc_def_mdef / 100 - (status_get_int(bl) + status_get_luk(bl)) / 15;

			rate = (rate <= 5)? 5: rate;
			sc = status_get_sc(bl);
			if(sc && sc->data[SC_FREEZE].timer != -1) {
				battle_skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
				if(sd)
					clif_skill_fail(sd,skillid,0,0);
				break;
			}
			damage = battle_skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
			if(status_get_hp(bl) > 0 && damage > 0 && atn_rand()%100 < rate) {
				int eff_tick = skill_get_time2(skillid,skilllv) * (1-sc_def_mdef/100);
				status_change_start(bl,SC_FREEZE,skilllv,0,0,0,eff_tick,0);
			} else if(sd) {
				clif_skill_fail(sd,skillid,0,0);
			}
		}
		break;
	case WZ_WATERBALL:			/* �E�H�[�^�[�{�[�� */
		battle_skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		if(skilllv > 1) {
			int cnt, range;
			range = (skilllv > 5)? 2: skilllv / 2;
			if(sd && !map[sd->bl.m].flag.rain)
				cnt = skill_count_water(src,range) - 1;
			else
				cnt = skill_get_num(skillid,skilllv) - 1;
			if(cnt > 0)
				skill_addtimerskill(src,tick+150,bl->id,0,0,skillid,skilllv,cnt,flag);
		}
		break;

	case PR_BENEDICTIO:			/* ���̍~�� */
		{
			int race = status_get_race(bl);
			if(battle_check_undead(race,status_get_elem_type(bl)) || race == RCT_DEMON) {
				if(bl->type == BL_MOB || !map[bl->m].flag.normal)
					battle_skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
			}
		}
		break;

	case SL_SMA:				/* �G�X�} */
	case SL_STUN:				/* �G�X�g�� */
	case SL_STIN:				/* �G�X�e�B�� */
		if(sd && bl->type != BL_MOB && !battle_config.allow_es_magic_all)
			clif_skill_fail(sd,skillid,0,0);
		else
			battle_skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,flag);
		break;

	/* ���@�n�͈͍U���X�L�� */
	case MG_NAPALMBEAT:			/* �i�p�[���r�[�g */
	case MG_FIREBALL:			/* �t�@�C���[�{�[�� */
	case WZ_SIGHTRASHER:		/* �T�C�g���b�V���[ */
	case WZ_FROSTNOVA:			/* �t���X�g�m���@ */
	case HW_NAPALMVULCAN:		/* �i�p�[���o���J�� */
	case NJ_HYOUSYOURAKU:		/* �X������ */
	case NJ_RAIGEKISAI:			/* ������ */
		if(flag&1) {
			/* �ʂɃ_���[�W��^���� */
			if(bl->id != skill_area_temp[1]) {
				int count;
				if(skillid == MG_FIREBALL) {
					/* �t�@�C���[�{�[���Ȃ璆�S����̋������v�Z */
					count = unit_distance(bl->x,bl->y,skill_area_temp[2],skill_area_temp[3]);
				} else {
					count = skill_area_temp[0];
				}
				if(skillid != HW_NAPALMVULCAN)
					count |= 0x0500;
				battle_skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,count);
			}
		} else {
			int ar = 0;
			skill_area_temp[0] = 0;
			skill_area_temp[1] = bl->id;
			switch (skillid) {
				case MG_NAPALMBEAT:
				case HW_NAPALMVULCAN:
					ar = 1;
					/* �i�p�[���r�[�g�E�i�p�[���o���J���͕��U�_���[�W�Ȃ̂œG�̐��𐔂��� */
					map_foreachinarea(skill_area_sub,
							bl->m,bl->x-ar,bl->y-ar,bl->x+ar,bl->y+ar,(BL_CHAR|BL_SKILL),
							src,skillid,skilllv,tick,flag|BCT_ENEMY,
							skill_area_sub_count);
					break;
				case MG_FIREBALL:
					ar = 2;
					skill_area_temp[2] = bl->x;
					skill_area_temp[3] = bl->y;
					break;
				case WZ_FROSTNOVA:
					ar = 2;
					skill_area_temp[2] = bl->x;
					skill_area_temp[3] = bl->y;
					bl = src;
					break;
				case NJ_HYOUSYOURAKU:		/* �X������ */
					ar = 7;
					skill_area_temp[2] = bl->x;
					skill_area_temp[3] = bl->y;
					bl = src;
					break;
				case NJ_RAIGEKISAI:			/* ������ */
					ar = (skilllv + 1) / 2 + 1;
					skill_area_temp[2] = bl->x;
					skill_area_temp[3] = bl->y;
					bl = src;
					break;
				case WZ_SIGHTRASHER:
					ar = 7;
					bl = src;
					status_change_end(src,SC_SIGHT,-1);
					break;
			}
			if(skillid == WZ_SIGHTRASHER || skillid == WZ_FROSTNOVA || skillid == NJ_HYOUSYOURAKU || skillid == NJ_RAIGEKISAI) {
				/* �X�L���G�t�F�N�g�\�� */
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
			} else {
				/* �^�[�Q�b�g�ɍU����������(�X�L���G�t�F�N�g�\��) */
				battle_skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,skill_area_temp[0]);
			}
			/* �^�[�Q�b�g�ȊO�͈͓̔��̓G�S�̂ɏ������s�� */
			map_foreachinarea(skill_area_sub,
				bl->m,bl->x-ar,bl->y-ar,bl->x+ar,bl->y+ar,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;
	case NJ_KAMAITACHI:			/* �� */
		if(flag&1) {
			battle_skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,(skill_area_temp[1] == 0 ? 0 : 0x0500));
			skill_area_temp[1]++;
		} else {
			int dir = map_calc_dir(src,bl->x,bl->y);
			skill_area_temp[1] = 0;
			map_foreachinshootpath(
				skill_area_sub,bl->m,src->x,src->y,dirx[dir],diry[dir],skill_get_fixed_range(src,skillid,skilllv),1,
				(BL_CHAR|BL_SKILL),src,skillid,skilllv,tick,flag|BCT_ENEMY|1,skill_castend_damage_id
			);
			if(skill_area_temp[1] == 0) {
				battle_skill_attack(BF_MAGIC,src,src,bl,skillid,skilllv,tick,(is_enemy ? 0 : 0x01000000));
			}
		}
		break;

	/* ���̑� */
	case TF_THROWSTONE:			/* �Γ��� */
	case PA_PRESSURE:			/* �v���b�V���[ */
	case SN_FALCONASSAULT:			/* �t�@���R���A�T���g */
	case NPC_DARKBREATH:
		battle_skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,flag);
		break;
	case HT_BLITZBEAT:			/* �u���b�c�r�[�g */
		if(flag&1) {
			/* �ʂɃ_���[�W��^���� */
			if(bl->id != skill_area_temp[1])
				battle_skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,skill_area_temp[0]|(flag&0xf00000));
		} else {
			skill_area_temp[0] = 0;
			skill_area_temp[1] = bl->id;
			if(flag&0xf00000) {
				map_foreachinarea(skill_area_sub,bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,(BL_CHAR|BL_SKILL),
					src,skillid,skilllv,tick, flag|BCT_ENEMY,skill_area_sub_count);
			}
			/* �܂��^�[�Q�b�g�ɍU���������� */
			battle_skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,skill_area_temp[0]|(flag&0xf00000));
			/* ���̌�^�[�Q�b�g�ȊO�͈͓̔��̓G�S�̂ɏ������s�� */
			map_foreachinarea(skill_area_sub,
				bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,(BL_CHAR|BL_SKILL),
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
			map_foreachinarea(skill_area_trap_sub,
				bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,BL_SKILL,
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_damage_id);
		}
		break;

	case CR_GRANDCROSS:			/* �O�����h�N���X */
	case NPC_GRANDDARKNESS:			/* �O�����h�_�[�N�l�X */
		{
			struct unit_data *ud = unit_bl2ud(src);
			/* �X�L�����j�b�g�z�u */
			skill_castend_pos2(src,bl->x,bl->y,skillid,skilllv,tick,0);
			if(ud)
				ud->canmove_tick = tick + 900;
		}
		break;
	case PF_SOULBURN:		/* �\�E���o�[�� */
		if(bl->type == BL_PC) {
			struct block_list *dstbl;
			int sp, rate;
			if(status_check_no_magic_damage(bl))
				break;
			if(sd && !map[sd->bl.m].flag.pvp && !map[sd->bl.m].flag.gvg && !map[sd->bl.m].flag.pk)
				break;
			rate = (skilllv >= 5)? 70: 10 * skilllv + 30;
			if(atn_rand() % 100 >= rate) {
				dstbl = src;	// �����ɑ΂��ă_���[�W
				if(sd)
					clif_skill_fail(sd,skillid,0,0);
			} else {
				dstbl = bl;
			}
			clif_skill_nodamage(src,dstbl,skillid,skilllv,1);
			sp = status_get_sp(dstbl);
			// SP��0�ɂ���
			if(dstbl->type == BL_PC)
				unit_heal(dstbl,0,-sp);
			if(skilllv >= 5) {
				// SP*2�̃_���[�W��^����(MDEF�Ōv�Z)
				int damage = sp * 2 * (100 - status_get_mdef(dstbl)) / 100 - status_get_mdef2(dstbl);
				if(damage < 1)
					damage = 1;
				battle_damage(src,dstbl,damage,skillid,skilllv,0);
			}
			if(sd)
				sd->skillstatictimer[PF_SOULBURN] = tick + skill_get_time2(skillid,skilllv);
		}
		break;
	case NPC_SELFDESTRUCTION2:	/* ����2 */
		if(flag&1) {
			if(bl->type == BL_PC && !map[src->m].flag.pvp && !map[src->m].flag.gvg)
				break;
		}
		// fall through
	case NPC_SELFDESTRUCTION:	/* ���� */
		if(flag&1) {
			/* �ʂɃ_���[�W��^���� */
			if(md) {
				md->hp = skill_area_temp[2];
				if(bl->id != skill_area_temp[1]) {
					battle_skill_attack(BF_MISC,src,src,bl,NPC_SELFDESTRUCTION,skilllv,tick,flag);
				}
				md->hp = 1;
			}
		} else {
			sc = status_get_sc(src);
			if(sc && sc->data[SC_SELFDESTRUCTION].timer != -1)
				status_change_end(src,SC_SELFDESTRUCTION,-1);

			if(md && md->hp > 0) {
				if(skillid == NPC_SELFDESTRUCTION2 && md->hp >= status_get_max_hp(&md->bl)) {
					// ����2��HP�S�񕜏�ԂȂ甭�����Ȃ�
					break;
				}
				skill_area_temp[1] = bl->id;
				skill_area_temp[2] = md->hp;
				clif_skill_nodamage(src,src,NPC_SELFDESTRUCTION,-1,1);
				map_foreachinarea(skill_area_sub,
					bl->m,bl->x-5,bl->y-5,bl->x+5,bl->y+5,(BL_CHAR|BL_SKILL),
					src,skillid,skilllv,tick, flag|BCT_ALL|1,
					skill_castend_damage_id);
				mob_damage(NULL,md,md->hp,0);
			}
		}
		break;
	case HVAN_EXPLOSION:	/* �o�C�I�G�N�X�v���[�W���� */
		if(flag&1) {
			/* �ʂɃ_���[�W��^���� */
			if(bl->id != skill_area_temp[1])
				battle_skill_attack(BF_MISC,src,src,bl,skillid,skilllv,tick,0x0500);
		} else {
			int ar = 5;
			skill_area_temp[1] = bl->id;
			skill_area_temp[2] = bl->x;
			skill_area_temp[3] = bl->y;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			/* ���̌�^�[�Q�b�g�ȊO�͈͓̔��̓G�S�̂ɏ������s�� */
			if(map[src->m].flag.normal) {
				map_foreachinarea(skill_area_sub,
					bl->m,bl->x-ar,bl->y-ar,bl->x+ar,bl->y+ar,BL_MOB,
					src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
					skill_castend_damage_id);
			} else {
				map_foreachinarea(skill_area_sub,
					bl->m,bl->x-ar,bl->y-ar,bl->x+ar,bl->y+ar,(BL_CHAR|BL_SKILL),
					src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
					skill_castend_damage_id);
			}
			if(hd) {
				hd->intimate = 1;
				if(battle_config.homun_skill_intimate_type)
					hd->status.intimate = 1;
				clif_send_homdata(hd->msd,1,hd->intimate/100);
			}
			battle_damage(NULL,src,status_get_hp(src),skillid,skilllv,flag);
		}
		break;
	case NJ_TATAMIGAESHI:	/* ���Ԃ� */
		status_change_start(src,SC_TATAMIGAESHI,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		skill_castend_pos2(src,bl->x,bl->y,skillid,skilllv,tick,0);
		break;
	case NJ_KAENSIN:	/* �Ή��w */
		bl = src;
		skill_castend_pos2(src,bl->x,bl->y,skillid,skilllv,tick,0);
		break;
	case NJ_BAKUENRYU:	/* �����w */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		skill_castend_pos2(src,bl->x,bl->y,skillid,skilllv,tick,0);
		break;
	case GS_DESPERADO:	/* �f�X�y���[�h */
		{
			int tmpx, tmpy, i, num;
			bl = src;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(sd) {
				int cost = skill_get_arrow_cost(skillid,skilllv);
				if(cost > 0 && !battle_delarrow(sd, cost, skillid))	// �e�̏���
					break;
			}
			num = skill_get_num(skillid,skilllv);
			for(i=0; i<num; i++) {
				tmpx = src->x + (atn_rand()%5 - 2);
				tmpy = src->y + (atn_rand()%5 - 2);
				skill_addtimerskill(src,tick+i*100,0,tmpx,tmpy,skillid,skilllv,0,0);
			}
		}
		break;

	/* HP�z��/HP�z�����@ */
	case NPC_BLOODDRAIN:
	case NPC_ENERGYDRAIN:
		{
			int heal = battle_skill_attack((skillid == NPC_BLOODDRAIN)? BF_WEAPON: BF_MAGIC,
					src,src,bl,skillid,skilllv,tick,flag);
			if(heal > 0) {
				struct block_list tbl;
				memset(&tbl, 0, sizeof(tbl));
				tbl.m = src->m;
				tbl.x = src->x;
				tbl.y = src->y;
				clif_skill_nodamage(&tbl,src,AL_HEAL,heal,1);
				battle_heal(NULL,src,heal,0,0);
			}
		}
		break;
	case 0:
		if(sd) {
			if(flag&3) {
				if(bl->id != skill_area_temp[1])
					battle_skill_attack(BF_WEAPON,src,src,bl,skillid,skilllv,tick,0x0500);
			} else {
				int ar = sd->splash_range;
				skill_area_temp[1] = bl->id;
				map_foreachinarea(skill_area_sub,
					bl->m, bl->x - ar, bl->y - ar, bl->x + ar, bl->y + ar, (BL_CHAR|BL_SKILL),
					src, skillid, skilllv, tick, flag|BCT_ENEMY|1,
					skill_castend_damage_id);
			}
		}
		break;
	default:
		map_freeblock_unlock();
		return 1;
	}
	map_freeblock_unlock();

	return 0;
}

/*==========================================
 * �X�L���g�p�i�r�������AID�w��x���n�j
 *------------------------------------------
 */
int skill_castend_nodamage_id( struct block_list *src, struct block_list *bl,int skillid,int skilllv,unsigned int tick,int flag )
{
	struct map_session_data *sd  = NULL, *dstsd  = NULL;
	struct mob_data         *md  = NULL, *dstmd  = NULL;
	struct homun_data       *hd  = NULL, *dsthd  = NULL;
	struct merc_data        *mcd = NULL, *dstmcd = NULL;
	struct status_change    *sc  = NULL;
	int sc_def_vit,sc_def_mdef;

	nullpo_retr(1, src);
	nullpo_retr(1, bl);

	if(bl->prev == NULL)
		return 1;
	if(unit_isdead(bl) && skillid != ALL_RESURRECTION && skillid != PR_REDEMPTIO)
		return 1;
	if(status_get_class(bl) == 1288)
		return 1;

	sd  = BL_DOWNCAST( BL_PC,   src );
	md  = BL_DOWNCAST( BL_MOB,  src );
	hd  = BL_DOWNCAST( BL_HOM,  src );
	mcd = BL_DOWNCAST( BL_MERC, src );

	dstsd  = BL_DOWNCAST( BL_PC,   bl );
	dstmd  = BL_DOWNCAST( BL_MOB,  bl );
	dsthd  = BL_DOWNCAST( BL_HOM,  bl );
	dstmcd = BL_DOWNCAST( BL_MERC, bl );

	if(sd && unit_isdead(&sd->bl))
		return 1;

	sc_def_vit  = 100 - (3 + status_get_vit(bl)  + status_get_luk(bl)/3);
	sc_def_mdef = 100 - (3 + status_get_mdef(bl) + status_get_luk(bl)/3);

	if(dstmd) {
		if(sc_def_vit > 50)
			sc_def_vit = 50;
		if(sc_def_mdef > 50)
			sc_def_mdef = 50;
	} else {
		if(sc_def_vit < 0)
			sc_def_vit = 0;
		if(sc_def_mdef < 0)
			sc_def_mdef = 0;
	}

	// �G��
	if(md && md->skillidx != -1)
	{
		short emotion = mob_db[md->class_].skill[md->skillidx].emotion;
		if(emotion >= 0)
			clif_emotion(&md->bl,emotion);
	}

	map_freeblock_lock();

	switch(skillid)
	{
	case AL_HEAL:				/* �q�[�� */
		{
			int heal = skill_fix_heal(src, bl, skillid, skill_calc_heal(src, skilllv));
			int heal_get_jobexp;
			sc = status_get_sc(bl);
			if(md && battle_config.monster_skill_over && skilllv >= battle_config.monster_skill_over)
				heal = 9999;	// 9999�q�[��
			if(dstsd && dstsd->special_state.no_magic_damage)
				heal = 0;	// ����峃J�[�h�i�q�[���ʂO�j
			if(sc && sc->data[SC_BERSERK].timer != -1)
				heal = 0; 	// �o�[�T�[�N���̓q�[���O
			if(sd) {
				int skill = pc_checkskill(sd,HP_MEDITATIO);
				if(skill > 0)	// ���f�B�^�e�B�I
					heal += heal * (skill * 2) / 100;
				if(dstsd && sd->status.partner_id == dstsd->status.char_id && sd->s_class.job == 23 && sd->sex == 0)
					heal *= 2;	// �X�p�m�r�̉ł��U�߂Ƀq�[�������2�{�ɂȂ�
			}
			if(sc && sc->data[SC_KAITE].timer != -1) {	// �J�C�g
				clif_misceffect2(bl,438);
				if(--sc->data[SC_KAITE].val2 <= 0)
					status_change_end(bl, SC_KAITE, -1);
				if(src == bl) {		// �������g�ɑ΂��Ă͉񕜗�0
					heal = 0;
				} else {		// �q�[������
					bl = src;
					dstsd = sd;
				}
			}
			clif_skill_nodamage(src,bl,skillid,heal,1);
			heal_get_jobexp = battle_heal(NULL,bl,heal,0,0);

			// JOB�o���l�l��
			if(sd && dstsd && heal > 0 && src != bl && battle_config.heal_exp > 0) {
				heal_get_jobexp = heal_get_jobexp * battle_config.heal_exp / 100;
				if(heal_get_jobexp <= 0)
					heal_get_jobexp = 1;
				pc_gainexp(sd,NULL,0,heal_get_jobexp);
			}
		}
		break;

	case HLIF_HEAL:		/* �����̎菕�� */
		{
			int heal = skill_fix_heal(src, bl, skillid, skill_calc_heal(src, skilllv));
			sc = status_get_sc(bl);
			if(hd) {
				int skill = homun_checkskill(hd,HLIF_BRAIN);
				if(skill > 0)
					heal += heal * skill / 50;
			}
			if(md && battle_config.monster_skill_over && skilllv >= battle_config.monster_skill_over)
				heal = 9999;	// 9999�q�[��
			if(dstsd && dstsd->special_state.no_magic_damage)
				heal = 0;	// ����峃J�[�h�i�q�[���ʂO�j
			if(sc && sc->data[SC_BERSERK].timer != -1)
				heal = 0;	// �o�[�T�[�N���̓q�[���O
			if(sc && sc->data[SC_KAITE].timer != -1) {	// �J�C�g
				clif_misceffect2(bl,438);
				if(--sc->data[SC_KAITE].val2 <= 0)
					status_change_end(bl, SC_KAITE, -1);
				if(src == bl) {		// �������g�ɑ΂��Ă͉񕜗�0
					heal = 0;
				} else {		// �q�[������
					bl = src;
				}
			}
			clif_skill_nodamage(src,bl,skillid,heal,1);
			battle_heal(NULL,bl,heal,0,0);
		}
		break;

	case ALL_RESURRECTION:		/* ���U���N�V���� */
		if(!dstsd)
			break;
		if(map[bl->m].flag.pvp && dstsd->pvp_point < 0)	// PVP�ŕ����s�\���
			break;
		if(!unit_isdead(&dstsd->bl))			// ���S����
			break;

		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		dstsd->status.hp = dstsd->status.max_hp * ((skilllv >= 4)? 80: skilllv*20-10)/100;
		if(dstsd->status.hp <= 0)
			dstsd->status.hp = 1;
		if(dstsd->special_state.restart_full_recover) {	// �I�V���X�J�[�h
			dstsd->status.hp = dstsd->status.max_hp;
			dstsd->status.sp = dstsd->status.max_sp;
			clif_updatestatus(dstsd,SP_SP);
		}
		clif_updatestatus(dstsd,SP_HP);
		pc_setstand(dstsd);
		if(battle_config.pc_invincible_time > 0)
			pc_setinvincibletimer(dstsd,battle_config.pc_invincible_time);
		clif_resurrection(&dstsd->bl,1);

		if(src != bl && sd && battle_config.resurrection_exp > 0)
		{
			atn_bignumber exp = 0, jexp = 0;
			int lv  = dstsd->status.base_level - sd->status.base_level;
			int jlv = dstsd->status.job_level  - sd->status.job_level;
			if(lv > 0) {
				exp = (atn_bignumber)dstsd->status.base_exp * lv * battle_config.resurrection_exp / 1000000;
				if(exp < 1)
					exp = 1;
			}
			if(jlv > 0) {
				jexp = (atn_bignumber)dstsd->status.job_exp * jlv * battle_config.resurrection_exp / 1000000;
				if(jexp < 1)
					jexp = 1;
			}
			if(exp > 0 || jexp > 0)
				pc_gainexp(sd,NULL,exp,jexp);
		}
		break;

	case AL_DECAGI:			/* ���x���� */
	case MER_DECAGI:
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		if( atn_rand()%100 < (50+skilllv*3+(status_get_lv(src)+status_get_int(src)/5)-sc_def_mdef) ) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		}
		break;

	case AL_CRUCIS:
		if(flag&1) {
			int race = status_get_race(bl);
			int ele = status_get_elem_type(bl);
			if(race == RCT_DEMON || battle_check_undead(race,ele)) {
				int rate = 23 + skilllv*4 + status_get_lv(src) - status_get_lv(bl);
				if(atn_rand()%100 < rate)
					status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,0,0);
			}
		} else {
			const int range = AREA_SIZE;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			map_foreachinarea(skill_area_sub,
				src->m,src->x-range,src->y-range,src->x+range,src->y+range,BL_CHAR,
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_nodamage_id);
		}
		break;

	case PR_LEXDIVINA:		/* ���b�N�X�f�B�r�[�i */
	case MER_LEXDIVINA:
		sc = status_get_sc(bl);
		if(sc) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if( dstsd && dstsd->special_state.no_magic_damage )
				break;
			if(sc->data[SC_SILENCE].timer != -1) {
				// ���ْ��Ȃ�G������킸���ى���
				status_change_end(bl,SC_SILENCE,-1);
			}
			else if(battle_check_target(src,bl,BCT_ENEMY) > 0 && atn_rand()%100 < sc_def_vit) {
				status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
			}
		}
		break;
	case SA_ABRACADABRA:
		if( sd && !map[src->m].flag.noabra ) {
			int abra_skillid = 0, maxlv;
			int abra_skilllv = pc_checkskill(sd,SA_ABRACADABRA);
			while(abra_skillid == 0) {
				abra_skillid=skill_abra_dataset(sd,skilllv);
			}
			maxlv = skill_get_max(abra_skillid);
			if(abra_skilllv > maxlv)
				abra_skilllv = maxlv;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			sd->skillitem      = abra_skillid;
			sd->skillitemlv    = abra_skilllv;
			sd->skillitem_flag = 1;		// �g�p�������肷��
			clif_item_skill(sd, abra_skillid, abra_skilllv, msg_txt(179)); // �A�u���J�_�u��
		}
		break;
	case SA_COMA:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd ) {
			if( dstsd->special_state.no_magic_damage )
				break;
			dstsd->status.hp = 1;
			dstsd->status.sp = 1;
			clif_updatestatus(dstsd,SP_HP);
			clif_updatestatus(dstsd,SP_SP);
		}
		else if(dstmd) {
			dstmd->hp = 1;
		}
		break;
	case SA_FULLRECOVERY:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd ) {
			if( dstsd->special_state.no_magic_damage )
				break;
			pc_heal(dstsd,dstsd->status.max_hp,dstsd->status.max_sp);
		}
		else if(dstmd) {
			dstmd->hp = status_get_max_hp(&dstmd->bl);
		}
		break;
	case SA_SUMMONMONSTER:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd)
			mob_once_spawn(sd,map[sd->bl.m].name,sd->bl.x,sd->bl.y,"--ja--",-1,1,"");
		break;
	case SA_LEVELUP:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd && pc_nextbaseexp(sd))
			pc_gainexp(sd,NULL,(atn_bignumber)pc_nextbaseexp(sd)*10/100,0);
		break;

	case SA_INSTANTDEATH:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd)
			pc_damage(NULL,sd,sd->status.max_hp);
		break;

	case SA_QUESTION:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		clif_emotion(bl,1);
		break;
	case SA_GRAVITY:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		break;
	case SA_CLASSCHANGE:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(dstmd) {
			// �N���X�`�F���W�p�{�X�����X�^�[ID
			static int changeclass[] = {
				1038,1039,1046,1059,1086,1087,1112,1115,1147,1150,
				1157,1159,1190,1251,1252,1272,1312,1373,1389,1418,
				1492,1511
			};
			mob_class_change(dstmd,changeclass,sizeof(changeclass)/sizeof(int));
		}
		break;
	case SA_MONOCELL:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(dstmd) {
			// �`�F���W�|�����p�����X�^�[ID
			static int poringclass[] = { 1002,1002 };
			mob_class_change(dstmd,poringclass,sizeof(poringclass)/sizeof(int));
		}
		break;
	case SA_DEATH:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(dstsd)
			pc_damage(NULL,dstsd,dstsd->status.max_hp);
		else if(dstmd)
			mob_damage(NULL,dstmd,dstmd->hp,1);
		break;
	case SA_REVERSEORCISH:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(dstsd)
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case SA_FORTUNE:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd)
			pc_getzeny(sd,status_get_lv(bl)*100);
		break;
	case SA_TAMINGMONSTER:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd && dstmd) {
			if(search_petDB_index(dstmd->class_, PET_CLASS) >= 0)
				pet_catch_process1(sd,dstmd->class_);
		}
		break;
	case PF_SPIDERWEB:		/* �X�p�C�_�[�E�F�b�u */
		skill_castend_pos2(src,bl->x,bl->y,skillid,skilllv,tick,0);
		break;

	case AL_INCAGI:			/* ���x���� */
	case AL_BLESSING:		/* �u���b�V���O */
	case PR_SLOWPOISON:
	case PR_IMPOSITIO:		/* �C���|�V�e�B�I�}�k�X */
	case PR_LEXAETERNA:		/* ���b�N�X�G�[�e���i */
	case PR_SUFFRAGIUM:		/* �T�t���M�E�� */
	case CR_PROVIDENCE:		/* �v�����B�f���X */
	case SA_FLAMELAUNCHER:		/* �t���C�������`���[ */
	case SA_FROSTWEAPON:		/* �t���X�g�E�F�|�� */
	case SA_LIGHTNINGLOADER:	/* ���C�g�j���O���[�_�[ */
	case SA_SEISMICWEAPON:		/* �T�C�Y�~�b�N�E�F�|�� */
		if( !(dstsd && dstsd->special_state.no_magic_damage) ) {
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		}
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		break;
	case PR_BENEDICTIO:		/* ���̍~�� */
		{
			int race = status_get_race(bl);
			if( battle_check_undead(race,status_get_elem_type(bl)) || race == RCT_DEMON )
				break;
			if( !(dstsd && dstsd->special_state.no_magic_damage) ) {
				status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
			}
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case SA_ELEMENTWATER:	/* �E�H�[�^�[�G�������^���`�F���W */
	case SA_ELEMENTGROUND:	/* �A�[�X�G�������^���`�F���W */
	case SA_ELEMENTFIRE:	/* �t�@�C�A�[�G�������^���`�F���W */
	case SA_ELEMENTWIND:	/* �E�B���h�G�������^���`�F���W */
		if(dstmd) {
			// �{�X�����������ꍇ�A�g�p���s
			if(battle_config.boss_no_element_change && dstmd && dstmd->mode&0x20){
				if(sd)
					clif_skill_fail(sd,skillid,0,0);
				break;
			}
			// �G�͑������x�����ێ�����
			switch(skillid) {
				case SA_ELEMENTWATER:	// ��
					dstmd->def_ele = (dstmd->def_ele/20)*20 + ELE_WATER;
					break;
				case SA_ELEMENTGROUND:	// �y
					dstmd->def_ele = (dstmd->def_ele/20)*20 + ELE_EARTH;
					break;
				case SA_ELEMENTFIRE:	// ��
					dstmd->def_ele = (dstmd->def_ele/20)*20 + ELE_FIRE;
					break;
				case SA_ELEMENTWIND:	// ��
					dstmd->def_ele = (dstmd->def_ele/20)*20 + ELE_WIND;
					break;
			}
			// �f�B�X�y���΍���ꉞ���H
			//status_change_start(bl,SkillStatusChangeTable[skillid],dstmd->def_ele/20,0,0,0,skill_get_time(skillid,skilllv),0 );
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		else if(!dstsd || !dstsd->special_state.no_magic_damage) {
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		}
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		break;
	case NPC_CHANGEUNDEAD:		/* �s�������t�^ */
		if( dstsd && dstsd->special_state.no_magic_damage ) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		} else {
			if(status_get_elem_type(bl) == ELE_DARK || status_get_race(bl) == RCT_DEMON)
				break;
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;

	case BA_PANGVOICE:	/* �p���{�C�X */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		sc = status_get_sc(bl);
		if(sc && sc->data[SC_CONFUSION].timer != -1)
			status_change_end(bl,SC_CONFUSION,-1);
		else if( !(status_get_mode(bl)&0x20) && atn_rand()%10000 < 5000 )
			status_change_start(bl,SC_CONFUSION,7,0,0,0,10000+7000,0);
		else if(sd)
			clif_skill_fail(sd,skillid,0,0);
		break;
	case DC_WINKCHARM:	/* ���f�̃E�B���N */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(dstsd) {
			if(atn_rand()%10000 < 7000) {
				status_change_start(bl,SC_CONFUSION,7,0,0,0,10000+7000,0);
				break;
			}
		} else if(dstmd) {
			int race = status_get_race(bl);
			if( !(dstmd->mode&0x20) && (race == RCT_DEMON || race == RCT_HUMAN || race == RCT_ANGEL) && atn_rand()%10000 < 7000)
			{
				status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,10000,0);
				break;
			}
		}
		if(sd)
			clif_skill_fail(sd,skillid,0,0);
		break;
	case TK_RUN:		/* �^�C���M */
		if(sd) {
			if(sd->sc.data[SC_RUN].timer != -1) {
				// 5�Z���ȓ���Lv7�ȏ�őf��Ȃ�X�p�[�g�J�n
				if(sd->sc.data[SC_RUN].val4 >= 2) {
					int lv = sd->sc.data[SC_RUN].val1;
			   		if(lv >= 7 && sd->sc.data[SC_RUN].val4 <= 6 && sd->weapontype1 == WT_FIST && sd->weapontype2 == WT_FIST)
						status_change_start(bl,SC_SPURT,lv,0,0,0,skill_get_time2(TK_RUN,lv),0);
					status_change_end(bl,SC_RUN,-1);
					clif_skill_nodamage(src,bl,skillid,skilllv,1);
				}
			} else {
				status_change_start(bl,SC_RUN,skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
			}
		}
		break;
	case TK_HIGHJUMP:	/* �m�s�e�B�M */
		{
			int dir = status_get_dir(src);
			int x = src->x + dirx[dir]*skilllv*2;
			int y = src->y + diry[dir]*skilllv*2;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			// ���n�n�_�Ƃ��̈���悪�ړ��\�Z����PC,MOB,NPC����������Ȃ��Ȃ�
			if( !map[src->m].flag.pvp && (!map[src->m].flag.noteleport || map[src->m].flag.gvg) &&
			    map_getcell(src->m,x,y,CELL_CHKPASS) && map_getcell(src->m,x+dirx[dir],y+diry[dir],CELL_CHKPASS) &&
			    !map_count_oncell(src->m,x,y,BL_PC|BL_MOB|BL_NPC)
			) {
				unit_movepos(src,x,y,0x11);
			}
		}
		break;
	case TK_MISSION:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd && pc_checkskill(sd,TK_MISSION)>0)
		{
			if(sd->status.class_ == PC_CLASS_TK) {
				int count = ranking_get_point(sd,RK_TAEKWON)%100;
				if(sd->tk_mission_target == 0 || (count == 0 && atn_rand()%100 == 0))
				{
					int i = 0;
					while(i++ < 1000) {
						sd->tk_mission_target = MOB_ID_MIN + atn_rand()%(MOB_ID_MAX-MOB_ID_MIN);
						if(mob_db[sd->tk_mission_target].max_hp <= 0)
							continue;
						if(mob_db[sd->tk_mission_target].summonper[0] == 0)	// �}�ŌĂ΂�Ȃ��̂͏��O
							continue;
						if(mob_db[sd->tk_mission_target].mode&0x20)	// �{�X�������O
							continue;
						break;
					}
					if(i >= 1000)
						sd->tk_mission_target = 0;
					pc_setglobalreg(sd,"PC_MISSION_TARGET",sd->tk_mission_target);
				}
				clif_mission_mob(sd,sd->tk_mission_target,count);
			} else {
				clif_skill_fail(sd,skillid,0,0);
			}
		}
		break;
	case SG_SUN_WARM:		/* ���z�̉����� */
	case SG_MOON_WARM:		/* ���̉����� */
	case SG_STAR_WARM:		/* ���̉����� */
		{
			struct skill_unit_group *sg;
			sc = status_get_sc(src);
			if(sc && sc->data[SC_WARM].timer != -1) {
				status_change_end(src,SC_WARM,-1);
			}
			sg = skill_unitsetting(src,skillid,skilllv,src->x,src->y,0);
			if(sg) {
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
				status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,bl->id,0,(int)sg,skill_get_time(skillid,skilllv),0);
			}
		}
		break;
	case TK_SEVENWIND:		/* �g������ */
		{
			int type = (skilllv < 7)? SkillStatusChangeTable[skillid]: SC_ASPERSIO;
			status_change_start(bl,type,skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case SL_SWOO:			/* �G�X�E */
		if(sd && bl->type != BL_MOB && !battle_config.allow_es_magic_all) {
			if(atn_rand()%100 < sc_def_vit)
				status_change_start(src,SC_STUN,7,0,0,0,500,0);
			clif_skill_fail(sd,skillid,0,0);
			break;
		}
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		sc = status_get_sc(bl);
		if(sc && sc->data[SC_SWOO].timer != -1) {
			status_change_end(bl,SC_SWOO,-1);
			status_change_start(src,SC_STUN,7,0,0,0,500,0);
		} else {
			status_change_start(bl,SC_SWOO,skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		}
		break;

	case SL_SKA:			/* �G�X�J */
	case SL_SKE:			/* �G�X�N */
		if(sd && !dstmd && !battle_config.allow_es_magic_all) {
			if(atn_rand()%100 < sc_def_vit)
				status_change_start(src,SC_STUN,7,0,0,0,500,0);
			clif_skill_fail(sd,skillid,0,0);
			break;
		}
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		if(skillid == SL_SKE)
			status_change_start(src,SC_SMA,skilllv,0,0,0,3000,0);
		break;

	case PR_ASPERSIO:		/* �A�X�y���V�I */
	case PR_KYRIE:			/* �L���G�G���C�\�� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		if(dstmd)
			break;
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case SM_AUTOBERSERK:
	case KN_AUTOCOUNTER:		/* �I�[�g�J�E���^�[ */
	case KN_TWOHANDQUICKEN:		/* �c�[�n���h�N�C�b�P�� */
	case CR_SPEARQUICKEN:		/* �X�s�A�N�C�b�P�� */
	case CR_REFLECTSHIELD:
	case AS_ENCHANTPOISON:		/* �G���`�����g�|�C�Y�� */
	case AS_POISONREACT:		/* �|�C�Y�����A�N�g */
	case MC_LOUD:			/* ���E�h�{�C�X */
	case MG_ENERGYCOAT:		/* �G�i�W�[�R�[�g */
	case AL_RUWACH:			/* ���A�t */
	case MO_EXPLOSIONSPIRITS:	/* �����g�� */
	case MO_STEELBODY:		/* ���� */
	case WE_BABY:
	case LK_AURABLADE:		/* �I�[���u���[�h */
	case WS_CARTBOOST:		/* �J�[�g�u�[�X�g */
	case SN_SIGHT:			/* �g�D���[�T�C�g */
	case WS_MELTDOWN:		/* �����g�_�E�� */
	case ST_REJECTSWORD:		/* ���W�F�N�g�\�[�h */
	case HW_MAGICPOWER:		/* ���@�͑��� */
	case PF_MEMORIZE:		/* �������C�Y */
	case PF_DOUBLECASTING:		/* �_�u���L���X�e�B���O */
	case ASC_EDP:			/* �G���`�����g�f�b�h���[�|�C�Y�� */
	case PA_SACRIFICE:		/* �T�N���t�@�C�X */
	case ST_PRESERVE:		/* �v���U�[�u */
	case WS_OVERTHRUSTMAX:		/* �I�[�o�[�g���X�g�}�b�N�X */
	case WZ_SIGHTBLASTER:		/* �T�C�g�u���X�^�[ */
	case KN_ONEHAND:		/* �����n���h�N�C�b�P�� */
	case TK_READYSTORM:
	case TK_READYDOWN:
	case TK_READYTURN:
	case TK_READYCOUNTER:
	case TK_DODGE:
	case SG_SUN_COMFORT:
	case SG_MOON_COMFORT:
	case SG_STAR_COMFORT:
	case SL_KAIZEL:			/* �J�C�[�� */
	case SL_KAAHI:			/* �J�A�q */
	case SL_KAITE:			/* �J�C�g */
	case SL_KAUPE:			/* �J�E�v */
	case GS_INCREASING:		/* �C���N���[�W���O�A�L���A���V�[ */
	case NJ_UTSUSEMI:		/* ���̏p */
	case NJ_NEN:			/* �O */
	case NPC_POWERUP:		/* ���b�V���A�^�b�N */
	case NPC_AGIUP:			/* ���x���� */
	case NPC_DEFENDER:
	case NPC_MAGICMIRROR:		/* �}�W�b�N�~���[ */
	case MS_REFLECTSHIELD:
	case MER_QUICKEN:		/* �E�F�|���N�C�b�P�� */
	case MER_AUTOBERSERK:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case NPC_STONESKIN:		/* �X�g�[���X�L�� */
	case NPC_ANTIMAGIC:		/* �A���`�}�W�b�N */
		//clif_skill_nodamage(src,bl,skillid,skilllv,1);
		clif_skill_damage(src, bl, tick, 0, 0, -1, 1, skillid, -1, 0);	// �G�t�F�N�g���o�����߂̎b�菈�u
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case LK_PARRYING:		/* �p���C���O */
	case MG_SIGHT:			/* �T�C�g */
	case NPC_WIDESIGHT:		/* ���C�h�T�C�g */
	case MS_PARRYING:
	case MER_SIGHT:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,skillid,skill_get_time(skillid,skilllv),0);
		break;
	case HP_ASSUMPTIO:		/* �A�X���v�e�B�I */
		status_change_start(bl,SC_ASSUMPTIO,skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		clif_skill_nodamage(src,bl,skillid,skilllv,1);	// �A�C�R���p�P�b�g���M��ɑ���
		break;
	case LK_CONCENTRATION:	/* �R���Z���g���[�V���� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		if(sd)
			sd->skillstatictimer[SM_ENDURE] = tick;
		status_change_start(bl,SkillStatusChangeTable[SM_ENDURE],1,0,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case LK_BERSERK:		/* �o�[�T�[�N */
	case MS_BERSERK:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		if(sd) {
			sd->status.hp = sd->status.max_hp;
			clif_updatestatus(sd,SP_HP);
		} else if(hd) {
			hd->status.hp = hd->max_hp;
			clif_send_homstatus(hd->msd,0);
		} else if(mcd) {
			mcd->status.hp = mcd->max_hp;
			clif_send_mercstatus(mcd->msd,0);
		}
		break;
	case SM_ENDURE:			/* �C���f���A */
		if(sd)
			sd->skillstatictimer[SM_ENDURE] = tick + 10000;
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case LK_TENSIONRELAX:	/* �e���V���������b�N�X */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd){
			pc_setsit(sd);
			clif_sitting(&sd->bl, 1);
		}
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case MC_CHANGECART:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		break;
	case AC_CONCENTRATION:	/* �W���͌��� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		map_foreachinarea(status_change_timer_sub,
			src->m,src->x-1,src->y-1,src->x+1,src->y+1,BL_CHAR,
			src,SkillStatusChangeTable[skillid],skilllv,tick);
		break;
	case SM_PROVOKE:		/* �v���{�b�N */
	case MER_PROVOKE:
		// MVPmob�ƕs���ɂ͌����Ȃ�
		if(status_get_mode(bl)&0x20 || battle_check_undead(status_get_race(bl),status_get_elem_type(bl)))
		{
			if(sd)
				clif_skill_fail(sd,skillid,0,0);
			map_freeblock_unlock();
			return 1;
		}
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );

		// �����E�Ή��E����������
		status_change_attacked_end(bl);

		if(dstmd) {
			int range = skill_get_fixed_range(src,skillid,skilllv);
			mob_target(dstmd,src,range);
			battle_join_struggle(dstmd, src);
		}
		unit_skillcastcancel(bl,2);	// �r���W�Q
		break;

	case CG_MARIONETTE:		/* �}���I�l�b�g�R���g���[�� */
		if(sd && dstsd)
		{
			status_change_start(src,SC_MARIONETTE,1,bl->id,0,0,60000,0);
			status_change_start(bl,SC_MARIONETTE2,1,src->id,0,0,60000,0);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);	// �A�C�R���p�P�b�g���M��ɑ���
		}
		break;
	case CR_DEVOTION:		/* �f�B�{�[�V���� */
		if(sd && dstsd) {
			int i, n;
			int lv   = abs(sd->status.base_level - dstsd->status.base_level);
			int type = SkillStatusChangeTable[skillid];

			if( sd->bl.id == dstsd->bl.id ||			// ����͎����̓_��
			    lv > battle_config.devotion_level_difference ||	// ���x����
			    sd->status.party_id <= 0 ||				// ������PT���������ƃ_��
			    dstsd->status.party_id <= 0 ||			// ���肪PT���������ƃ_��
			    sd->status.party_id != dstsd->status.party_id ||	// �����p�[�e�B����Ȃ��ƃ_��
			    dstsd->s_class.job == 14 ||				// ���肪�N���Z���ƃ_��
			    dstsd->s_class.job == 21 ||				// ���肪�N���Z���ƃ_��
			    (type >= 0 && dstsd->sc.data[type].timer != -1 && dstsd->sc.data[type].val1 != sd->bl.id) )	// �Ⴄ�N���Z����f�B�{�[�V�����ς݂Ȃ�_��
			{
				clif_skill_fail(sd,skillid,0,0);
				map_freeblock_unlock();
				return 1;
			}

			for(i = 0, n = -1; i < skilllv && i < 5; i++) {
				if(sd->dev.val1[i] == dstsd->bl.id) {	// ���Ƀf�B�{�[�V�����ς݂̑���
					n = i;
					break;
				}
				if(!sd->dev.val1[i])	// �󂫂���������m�ۂ���
					n = i;
			}
			if(n < 0) {	// ���̂��󂫂��Ȃ�����
				map_freeblock_unlock();
				return 1;
			}
			sd->dev.val1[n] = dstsd->bl.id;
			sd->dev.val2[n] = dstsd->bl.id;
			clif_skill_nodamage(&sd->bl,&dstsd->bl,skillid,skilllv,1);
			clif_devotion(sd);
			status_change_start(&dstsd->bl,type,sd->bl.id,1,0,0,skill_get_time(skillid,skilllv),0 );
		}
		else if(sd) {
			if(dstmd) {
				int range = skill_get_fixed_range(&sd->bl,skillid,skilllv);
				clif_skill_nodamage(&sd->bl,&dstmd->bl,skillid,skilllv,1);
				mob_target(dstmd, &sd->bl, range);
				battle_join_struggle(dstmd, &sd->bl);
			} else {
				clif_skill_fail(sd,skillid,0,0);
			}
		}
		break;
	case MO_CALLSPIRITS:	/* �C�� */
		if(sd) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			pc_addspiritball(sd,skill_get_time(skillid,skilllv),skilllv);
		}
		break;
	case CH_SOULCOLLECT:	/* ���C�� */
		if(sd) {
			int i;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			pc_delspiritball(sd,sd->spiritball,0);
			for(i=0; i<5; i++)
				pc_addspiritball(sd,skill_get_time(skillid,skilllv),5);
		}
		break;
	case MO_BLADESTOP:	/* ���n��� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(src,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0 );
		break;
	case MO_ABSORBSPIRITS:	/* �C�D */
		{
			int val = 0;
			if(dstsd && dstsd->spiritball > 0) {
				if( sd && sd != dstsd && !map[sd->bl.m].flag.pvp && !map[sd->bl.m].flag.gvg )
					break;
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
				val = dstsd->spiritball * 7;
				pc_delspiritball(dstsd,dstsd->spiritball,0);
			} else if(dstmd) { // �Ώۂ������X�^�[�̏ꍇ
				// 20%�̊m���őΏۂ�Lv*2��SP���񕜂���B���������Ƃ��̓^�[�Q�b�e�B���O����B
				if(atn_rand()%100 < 20) {
					val = 2 * mob_db[dstmd->class_].lv;
					mob_target(dstmd,src,0);
					battle_join_struggle(dstmd, src);

					// �����E�Ή��E����������
					status_change_attacked_end(bl);
				}
			}
			if(sd) {
				if(val > 0x7FFF)
					val = 0x7FFF;
				if(sd->status.sp + val > sd->status.max_sp)
					val = sd->status.max_sp - sd->status.sp;
				if(val > 0) {
					sd->status.sp += val;
					clif_heal(sd->fd,SP_SP,val);
					clif_skill_nodamage(src,bl,skillid,skilllv,1);
				} else {
					clif_skill_fail(sd,skillid,0,0);
				}
			}
		}
		break;

	case AC_MAKINGARROW:		/* ��쐬 */
		if(sd) {
			clif_arrow_create_list(sd);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case AM_PHARMACY:			/* �|�[�V�����쐬 */
		if(sd) {
			clif_skill_produce_mix_list(sd,PRD_PHARMACY);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case AM_TWILIGHT1:
	case AM_TWILIGHT2:
	case AM_TWILIGHT3:
		if(sd) {
			skill_am_twilight(sd,skillid);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case AM_CALLHOMUN:	/* �R�[���z�����N���X */
		if(sd && !sd->hd) {
			homun_callhom(sd);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case AM_REST:				/* ���� */
		if(sd && homun_isalive(sd)) {
			homun_return_embryo(sd);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case AM_RESURRECTHOMUN:				/* ���U���N�V�����z�����N���X */
		if(sd && !sd->hd && sd->hom.hp <= 0) {
			homun_revive(sd,skilllv);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case ASC_CDP:				/* �f�b�h���[�|�C�Y���쐬 */
		if(sd) {
			clif_skill_produce_mix_list(sd,PRD_CDP);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case WS_CREATECOIN:			/* �N���G�C�g�R�C�� */
		if(sd) {
			clif_skill_produce_mix_list(sd,PRD_COIN);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case WS_CREATENUGGET:			/* �򐻑� */
		if(sd) {
			clif_skill_produce_mix_list(sd,PRD_NUGGET);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case SA_CREATECON:
		if(sd) {
			clif_skill_produce_mix_list(sd,PRD_CONVERTER);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case BS_HAMMERFALL:		/* �n���}�[�t�H�[�� */
		if( dstsd && dstsd->special_state.no_weapon_damage )
			break;
		if( atn_rand()%100 < ((skilllv > 5)? 100: 20+10*skilllv) * sc_def_vit/100 )
			status_change_start(bl,SC_STUN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;
	case RG_RAID:			/* �T�v���C�Y�A�^�b�N */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		map_foreachinarea(skill_area_sub,
			bl->m,bl->x-1,bl->y-1,bl->x+1,bl->y+1,(BL_CHAR|BL_SKILL),
			src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
			skill_castend_damage_id);
		status_change_end(src, SC_HIDING, -1);	// �n�C�f�B���O����
		break;
	case ASC_METEORASSAULT:	/* ���e�I�A�T���g */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		map_foreachinarea(skill_area_sub,
			bl->m,bl->x-2,bl->y-2,bl->x+2,bl->y+2,(BL_CHAR|BL_SKILL),
			src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
			skill_castend_damage_id);
		break;
	case KN_BRANDISHSPEAR:	/* �u�����f�B�b�V���X�s�A */
	case ML_BRANDISH:
		{
			int c, n = 4;
			int dir = map_calc_dir(src,bl->x,bl->y);
			struct square tc;

			skill_brandishspear_first(&tc,dir,bl->x,bl->y);
			skill_brandishspear_dir(&tc,dir,4);

			/* �͈�4 */
			if(skilllv > 9) {
				for(c=1; c<4; c++) {
					map_foreachinarea(skill_area_sub,
						bl->m,tc.val1[c],tc.val2[c],tc.val1[c],tc.val2[c],(BL_CHAR|BL_SKILL),
						src,skillid,skilllv,tick, flag|BCT_ENEMY|n,
						skill_castend_damage_id);
				}
			}
			/* �͈�3,2 */
			if(skilllv > 6) {
				skill_brandishspear_dir(&tc,dir,-1);
				n--;
			} else {
				skill_brandishspear_dir(&tc,dir,-2);
				n-=2;
			}
			if(skilllv > 3) {
				for(c=0; c<5; c++) {
					map_foreachinarea(skill_area_sub,
						bl->m,tc.val1[c],tc.val2[c],tc.val1[c],tc.val2[c],(BL_CHAR|BL_SKILL),
						src,skillid,skilllv,tick, flag|BCT_ENEMY|n,
						skill_castend_damage_id);
					if(skilllv > 6 && n == 3 && c == 4) {
						skill_brandishspear_dir(&tc,dir,-1);
						n--;
						c = -1;
					}
				}
			}
			/* �͈�1 */
			for(c=0; c<10; c++) {
				if(c == 0 || c == 5)
					skill_brandishspear_dir(&tc,dir,-1);
				map_foreachinarea(skill_area_sub,
					bl->m,tc.val1[c%5],tc.val2[c%5],tc.val1[c%5],tc.val2[c%5],(BL_CHAR|BL_SKILL),
					src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
					skill_castend_damage_id);
			}
		}
		break;
	case GS_GLITTERING:		/* �t���b�v�U�R�C�� */
		if(sd) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(atn_rand()%100 < 20 + skilllv * 10) {
				pc_addcoin(sd,skill_get_time(skillid,skilllv),10);
			} else {
				pc_delcoin(sd,1,0);
			}
		}
		break;

	/* �p�[�e�B�X�L�� */
	case AL_ANGELUS:		/* �G���W�F���X */
	case PR_MAGNIFICAT:		/* �}�O�j�t�B�J�[�g */
	case PR_GLORIA:			/* �O�����A */
	case SN_WINDWALK:		/* �E�C���h�E�H�[�N */
		if((flag&1) || sd == NULL || sd->status.party_id == 0) {
			/* �ʂ̏��� */
			if( dstsd && dstsd->special_state.no_magic_damage )
				break;
			clif_skill_nodamage(bl,bl,skillid,skilllv,1);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		} else {
			/* �p�[�e�B�S�̂ւ̏��� */
			party_foreachsamemap(skill_area_sub,
				sd,PT_AREA_SIZE,
				src,skillid,skilllv,tick, flag|BCT_PARTY|1,
				skill_castend_nodamage_id);
		}
		break;
	case BS_ADRENALINE:		/* �A�h���i�������b�V�� */
	case BS_ADRENALINE2:		/* �A�h���i�������b�V�� */
	case BS_WEAPONPERFECT:		/* �E�F�|���p�[�t�F�N�V���� */
	case BS_OVERTHRUST:		/* �I�[�o�[�g���X�g */
		if((flag&1) || sd == NULL || sd->status.party_id == 0) {
			/* �ʂ̏��� */
			clif_skill_nodamage(bl,bl,skillid,skilllv,1);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,((src == bl)? 1: 0),0,0,skill_get_time(skillid,skilllv),0);
		} else {
			/* �p�[�e�B�S�̂ւ̏��� */
			party_foreachsamemap(skill_area_sub,
				sd,PT_AREA_SIZE,
				src,skillid,skilllv,tick, flag|BCT_PARTY|1,
				skill_castend_nodamage_id);
		}
		break;
	case MER_MAGNIFICAT:
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		clif_skill_nodamage(bl,bl,skillid,skilllv,1);
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);

		if(mcd && mcd->msd) {
			// �b���Ȃ�ق���ɂ�
			if( mcd->msd->special_state.no_magic_damage )
				break;
			clif_skill_nodamage(&mcd->msd->bl,&mcd->msd->bl,skillid,skilllv,1);
			status_change_start(&mcd->msd->bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		}
		break;

	/*�i�t���Ɖ������K�v�j */
	case BS_MAXIMIZE:		/* �}�L�V�}�C�Y�p���[ */
	case NV_TRICKDEAD:		/* ���񂾂ӂ� */
	case CR_DEFENDER:		/* �f�B�t�F���_�[ */
	case CR_AUTOGUARD:		/* �I�[�g�K�[�h */
	case CR_SHRINK:			/* �V�������N */
	case GS_GATLINGFEVER:			/* �K�g�����O�t�B�[�o�[ */
	case ML_DEFENDER:
	case ML_AUTOGUARD:
		{
			int type = GetSkillStatusChangeTable(skillid);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			sc = status_get_sc(bl);
			if(type >= 0 && sc && sc->data[type].timer != -1) {
				/* �������� */
				status_change_end(bl, type, -1);
			} else {
				/* �t������ */
				if(skillid == BS_MAXIMIZE)
					status_change_start(bl,type,skilllv,skill_get_time(skillid,skilllv),0,0,0,0);
				else
					status_change_start(bl,type,skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
			}
		}
		break;

	case TF_HIDING:			/* �n�C�f�B���O */
		{
			int type = SkillStatusChangeTable[skillid];
			clif_skill_nodamage(src,bl,skillid,-1,1);
			sc = status_get_sc(bl);
			if(type >= 0 && sc && sc->data[type].timer != -1) {
				/* �������� */
				status_change_end(bl, type, -1);
			} else {
				/* �t������ */
				status_change_start(bl,type,skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
			}
			if(sc && sc->data[SC_CLOSECONFINE].timer != -1)
				status_change_end(bl,SC_CLOSECONFINE,-1);
		}
		break;

	case ST_CHASEWALK:		/* �`�F�C�X�E�H�[�N */
	case AS_CLOAKING:		/* �N���[�L���O */
	case NPC_INVISIBLE:		/* �C���r�W�u�� */
		{
			int type = SkillStatusChangeTable[skillid];
			clif_skill_nodamage(src,bl,skillid,-1,1);
			sc = status_get_sc(bl);
			if(type >= 0 && sc && sc->data[type].timer != -1) {
				/* �������� */
				status_change_end(bl, type, -1);
			} else {
				/* �t������ */
				status_change_start(bl,type,skilllv,skill_get_time(skillid,skilllv),0,0,0,0);
			}
			if(skillid != ST_CHASEWALK && skilllv < 3)
				skill_check_cloaking(bl);
		}
		break;

	/* �Βn�X�L�� */
	case HP_BASILICA:			/* �o�W���J */
		sc = status_get_sc(src);
		if(sc && sc->data[SC_BASILICA].timer != -1) {
			skill_basilica_cancel(src);
			status_change_end(bl,SC_BASILICA,-1);
			break;
		}
		status_change_start(bl,SC_BASILICA,skilllv,bl->id,0,0,skill_get_time(skillid,skilllv),0);
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		skill_unitsetting(src,skillid,skilllv,src->x,src->y,0);
		break;
	case BD_LULLABY:			/* �q��S */
	case BD_RICHMANKIM:			/* �j�����h�̉� */
	case BD_ETERNALCHAOS:		/* �i���̍��� */
	case BD_DRUMBATTLEFIELD:	/* �푾�ۂ̋��� */
	case BD_RINGNIBELUNGEN:		/* �j�[�x�����O�̎w�� */
	case BD_ROKISWEIL:			/* ���L�̋��� */
	case BD_INTOABYSS:			/* �[���̒��� */
	case BD_SIEGFRIED:			/* �s���g�̃W�[�N�t���[�h */
	case BA_DISSONANCE:			/* �s���a�� */
	case BA_POEMBRAGI:			/* �u���M�̎� */
	case BA_WHISTLE:			/* ���J */
	case BA_ASSASSINCROSS:		/* �[�z�̃A�T�V���N���X */
	case BA_APPLEIDUN:			/* �C�h�D���̗ь� */
	case DC_UGLYDANCE:			/* ��������ȃ_���X */
	case DC_HUMMING:			/* �n�~���O */
	case DC_DONTFORGETME:		/* ����Y��Ȃ��Łc */
	case DC_FORTUNEKISS:		/* �K�^�̃L�X */
	case DC_SERVICEFORYOU:		/* �T�[�r�X�t�H�[���[ */
	case CG_MOONLIT:			/* ������̉��� */
	case CG_HERMODE:			/* �w�����[�h�̏� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		skill_unitsetting(src,skillid,skilllv,src->x,src->y,0);
		break;
	case CG_LONGINGFREEDOM:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SC_LONGINGFREEDOM,skilllv,0,0,0,1000,0);
		break;

	case PA_GOSPEL:				/* �S�X�y�� */
		sc = status_get_sc(src);
		if(sc && sc->data[SC_GOSPEL].timer != -1) {
			status_change_end(bl,SC_GOSPEL,-1);
		} else {
			struct skill_unit_group *sg = skill_unitsetting(src,skillid,skilllv,src->x,src->y,0);
			status_change_release(src,0x04);	// �S�X�y���p�҂̃X�e�[�^�X�ُ����
			if(sg) {
				clif_skill_poseffect(src,skillid,skilllv,src->x,src->y,tick);
				status_change_start(bl,SC_GOSPEL,skilllv,bl->id,0,(int)sg,skill_get_time(skillid,skilllv),0);
			}
		}
		break;

	case BD_ADAPTATION:			/* �A�h���u */
		sc = status_get_sc(src);
		if(sc && sc->data[SC_DANCING].timer != -1) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			skill_stop_dancing(src,0);
		}
		break;

	case BA_FROSTJOKE:			/* �����W���[�N */
	case DC_SCREAM:				/* �X�N���[�� */
		if( sd || (md && mob_is_pcview(md->class_)) ) {
			// �����ڂ�PC�łȂ��ꍇ�͉��̂��p�������Ă��܂��̂ŏ��O����
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		skill_addtimerskill(src,tick+3000,bl->id,0,0,skillid,skilllv,0,flag);
		if(md) {		// Mob�͒���Ȃ�����A�X�L���������΂��Ă݂�
			char output[100];
			if(skillid == BA_FROSTJOKE)
				snprintf(output, sizeof(output), msg_txt(181), md->name); // %s : �����W���[�N !!
			else
				snprintf(output, sizeof(output), msg_txt(182), md->name); // %s : �X�N���[�� !!
			clif_GlobalMessage(&md->bl, output);
		}
		break;

	case TF_STEAL:			/* �X�e�B�[�� */
		if(sd) {
			if(dstmd && pc_steal_item(sd,dstmd))
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
			else
				clif_skill_fail(sd,skillid,0x0a,0);
		}
		break;

	case RG_STEALCOIN:		/* �X�e�B�[���R�C�� */
		if(sd) {
			if(dstmd && pc_steal_coin(sd,dstmd)) {
				int range = skill_get_fixed_range(src,skillid,skilllv);
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
				mob_target(dstmd,src,range);
				battle_join_struggle(dstmd, src);
			} else {
				clif_skill_fail(sd,skillid,0,0);
			}
		}
		break;

	case MG_STONECURSE:			/* �X�g�[���J�[�X */
		if(status_get_mode(bl)&0x20) {
			if(sd)
				clif_skill_fail(sd,skillid,0,0);
			break;
		}
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(dstsd && dstsd->special_state.no_magic_damage)
			break;
		if(dstmd)
			mob_target(dstmd,src,skill_get_fixed_range(src,skillid,skilllv));

		sc = status_get_sc(bl);
		if(sc && sc->data[SC_STONE].timer != -1) {
			status_change_end(bl,SC_STONE,-1);
		}
		else if(!battle_check_undead(status_get_race(bl),status_get_elem_type(bl)) && atn_rand()%100 < skilllv*4+20) {
			status_change_start(bl,SC_STONE,skilllv,0,0,5000,skill_get_time2(skillid,skilllv),0);
		}
		else {
			if(sd)
				clif_skill_fail(sd,skillid,0,0);
			break;
		}

		// �����Ȃ̂�Lv6�ȏ�̓W�F�������
		if(skilllv >= 6) {
			struct map_session_data *msd = NULL;
			if(sd)       msd = sd;
			else if(hd)  msd = hd->msd;
			else if(mcd) msd = mcd->msd;

			if(msd == NULL)
				break;
			if( !msd->special_state.no_gemstone &&
			    msd->sc.data[SC_WIZARD].timer == -1 &&
			    msd->sc.data[SC_INTOABYSS].timer == -1 )
			{
				int i, idx;
				for(i=0; i<10; i++) {
					if(skill_db[skillid].itemid[i] < 715 || skill_db[skillid].itemid[i] > 717)
						continue;
					idx = pc_search_inventory(msd,skill_db[skillid].itemid[i]);
					if(idx < 0)
						continue;
					pc_delitem(msd,idx,skill_db[skillid].amount[i],0);
				}
			}
		}
		break;

	case NV_FIRSTAID:			/* ���}�蓖 */
		{
			int heal = skill_fix_heal(src, bl, skillid, 5);
			clif_skill_nodamage(src,bl,skillid,heal,1);
			battle_heal(NULL,bl,heal,0,0);
		}
		break;

	case AL_CURE:				/* �L���A�[ */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		status_change_end(bl, SC_SILENCE, -1);
		status_change_end(bl, SC_BLIND, -1);
		status_change_end(bl, SC_CONFUSION, -1);
		break;

	case TF_DETOXIFY:			/* ��� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_end(bl, SC_POISON, -1);
		status_change_end(bl, SC_DPOISON, -1);
		break;

	case PR_STRECOVERY:			/* ���J�o���[ */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		status_change_end(bl, SC_FREEZE, -1);
		status_change_end(bl, SC_STONE, -1);
		status_change_end(bl, SC_SLEEP, -1);
		status_change_end(bl, SC_STUN, -1);
		if( battle_check_undead(status_get_race(bl),status_get_elem_type(bl)) ) {	// �A���f�b�h�Ȃ�ÈŌ���
			int int_ = status_get_int(bl);
			int vit  = status_get_vit(bl);
			if(atn_rand()%100 < 100 - int_/2 + vit/3 + status_get_luk(bl)/10) {
				int blind_time = 30 * (100 - (int_ + vit)/2) / 100;
				status_change_start(bl, SC_BLIND,1,0,0,0,blind_time,0);
			}
		}
		if(dstmd) {
			mob_unlocktarget( dstmd, tick );
		}
		break;

	case WZ_ESTIMATION:			/* �����X�^�[��� */
		if(sd) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			clif_skill_estimation(sd,bl);
		}
		break;
	case MER_ESTIMATION:
		if(mcd && mcd->msd) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			clif_skill_estimation(mcd->msd,bl);
		}
		break;

	case MC_IDENTIFY:			/* �A�C�e���Ӓ� */
		if(sd)
			clif_item_identify_list(sd);
		break;

	case WS_WEAPONREFINE:		/* ���퐸�B */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd)
			clif_weapon_refine_list(sd);
		break;

	case BS_REPAIRWEAPON:			/* ����C�� */
		if(sd && dstsd)
			clif_item_repair_list(sd,dstsd);
		break;

	case MC_VENDING:			/* �I�X�J�� */
		if(sd && pc_iscarton(sd))
			clif_openvendingreq(sd,2+sd->ud.skilllv);
		break;

	case AL_TELEPORT:			/* �e���|�[�g */
		{
			int alive = 1;
			if(!md || !(md->mode&0x20) || !battle_config.boss_teleport_on_landprotector) {
				// PC����ш��MOB�̓����h�v���e�N�^�[��ł̓e���|�[�g�s��
				map_foreachinarea(skill_landprotector,src->m,src->x,src->y,src->x,src->y,BL_SKILL,skillid,&alive);
			}
			if(alive) {
				if(sd) {
					clif_skill_nodamage(src,bl,skillid,skilllv,1);
					if(sd->ud.skilllv == 1) {
						clif_skill_warppoint(sd,sd->ud.skillid,"Random","","","");
					} else {
						clif_skill_warppoint(sd,sd->ud.skillid,"Random",sd->status.save_point.map,"","");
					}
				} else if(md) {
					mob_warp(md,-1,-1,-1,3);
				}
			}
		}
		break;

	case AL_HOLYWATER:			/* �A�N�A�x�l�f�B�N�^ */
		if(sd) {
			int eflag;
			struct item item_tmp;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			memset(&item_tmp,0,sizeof(item_tmp));
			item_tmp.nameid   = 523;
			item_tmp.identify = 1;
			if(battle_config.holywater_name_input) {
				item_tmp.card[0] = 0xfe;
				item_tmp.card[1] = 0;
				*((unsigned long *)(&item_tmp.card[2]))=sd->status.char_id;	// �L����ID
			}
			eflag = pc_additem(sd,&item_tmp,1);
			if(eflag) {
				clif_additem(sd,0,0,eflag);
				map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0);
			}
		}
		break;
	case TF_PICKSTONE:
		if(sd) {
			int eflag;
			struct item item_tmp;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			memset(&item_tmp,0,sizeof(item_tmp));
			item_tmp.nameid   = 7049;
			item_tmp.identify = 1;
			clif_takeitem(&sd->bl,0);
			eflag = pc_additem(sd,&item_tmp,1);
			if(eflag) {
				clif_additem(sd,0,0,eflag);
				map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0);
			}
		}
		break;

	case RG_STRIPWEAPON:		/* �X�g���b�v�E�F�|�� */
	case RG_STRIPSHIELD:		/* �X�g���b�v�V�[���h */
	case RG_STRIPARMOR:			/* �X�g���b�v�A�[�}�[ */
	case RG_STRIPHELM:			/* �X�g���b�v�w���� */
		{
			int cp_scid, scid, equip;
			int strip_fix, strip_time;

			scid = SkillStatusChangeTable[skillid];
			if(scid < 0)
				break;
			switch (skillid) {
				case RG_STRIPWEAPON:
					equip   = EQP_WEAPON;
					cp_scid = SC_CP_WEAPON;
					break;
				case RG_STRIPSHIELD:
					equip   = EQP_SHIELD;
					cp_scid = SC_CP_SHIELD;
					break;
				case RG_STRIPARMOR:
					equip   = EQP_ARMOR;
					cp_scid = SC_CP_ARMOR;
					break;
				case RG_STRIPHELM:
					equip   = EQP_HELM;
					cp_scid = SC_CP_HELM;
					break;
				default:
					map_freeblock_unlock();
					return 1;
			}
			sc = status_get_sc(bl);
			if(sc && (sc->data[scid].timer != -1 || sc->data[cp_scid].timer != -1)) {
				if(sd)
					clif_skill_fail(sd,skillid,0,0);
				break;
			}

			strip_fix = status_get_dex(src) - status_get_dex(bl);
			if(strip_fix < 0)
				strip_fix = 0;
			if(atn_rand()%100 >= 5 + 5 * skilllv + strip_fix / 5)
				break;

			if(dstsd) {
				int i;
				if(equip == EQP_SHIELD) {
					// �X�g���b�v�V�[���h�͋|�ȊO�̗��蕐��ɂ͎��s
					if( dstsd->equip_index[8] >= 0 &&
					    dstsd->inventory_data[dstsd->equip_index[8]]->type == 4 &&
					    dstsd->status.weapon != WT_BOW )
						break;
				}
				for(i=0; i<MAX_INVENTORY; i++) {
					if(dstsd->status.inventory[i].equip && (dstsd->status.inventory[i].equip & equip)) {
						pc_unequipitem(dstsd,i,0);
						break;
					}
				}
				if(i >= MAX_INVENTORY) {
					if(sd)
						clif_skill_fail(sd,skillid,0,0);
					break;
				}
			}
			strip_time = skill_get_time(skillid,skilllv) + strip_fix / 2;
			status_change_start(bl,scid,skilllv,0,0,0,strip_time,0);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		}
		break;
	case ST_FULLSTRIP:		/* �t���X�g���b�v */
		{
			int strip_fix, strip_time;
			int fail = 1;

			strip_fix = status_get_dex(src) - status_get_dex(bl);
			if(strip_fix < 0)
				strip_fix = 0;
			if(atn_rand()%100 >= 5 + 2 * skilllv + strip_fix / 5)
				break;
			strip_time = skill_get_time(skillid,skilllv) + strip_fix / 2;

			sc = status_get_sc(bl);
			if(dstsd) {
				int i;
				for(i=0; i<=MAX_INVENTORY; i++) {
					if( dstsd->status.inventory[i].equip & EQP_WEAPON &&
					    (!sc || (sc->data[SC_CP_WEAPON].timer == -1 && sc->data[SC_STRIPWEAPON].timer == -1)) ) {
						pc_unequipitem(dstsd,i,0);
						status_change_start(bl,SkillStatusChangeTable[RG_STRIPWEAPON],skilllv,0,0,0,strip_time,0);
						fail = 0;
					}
					if( dstsd->status.inventory[i].equip & EQP_SHIELD ) {
						// �X�g���b�v�V�[���h�͋|�ȊO�̗��蕐��ɂ͎��s
						if( dstsd->equip_index[8] >= 0 &&
						    dstsd->inventory_data[dstsd->equip_index[8]]->type == 4 &&
						    dstsd->status.weapon != WT_BOW ) {
							;
						}
						else if( !sc || (sc->data[SC_CP_SHIELD].timer == -1 && sc->data[SC_STRIPSHIELD].timer == -1) ) {
							pc_unequipitem(dstsd,i,0);
							status_change_start(bl,SkillStatusChangeTable[RG_STRIPSHIELD],skilllv,0,0,0,strip_time,0);
							fail = 0;
						}
					}
					if( dstsd->status.inventory[i].equip & EQP_ARMOR &&
					    (!sc || (sc->data[SC_CP_ARMOR].timer == -1 && sc->data[SC_STRIPARMOR].timer == -1)) ) {
						pc_unequipitem(dstsd,i,0);
						status_change_start(bl,SkillStatusChangeTable[RG_STRIPARMOR],skilllv,0,0,0,strip_time,0);
						fail = 0;
					}
					if( dstsd->status.inventory[i].equip & EQP_HELM &&
					    (!sc || (sc->data[SC_CP_HELM].timer == -1 && sc->data[SC_STRIPHELM].timer == -1)) ) {
						pc_unequipitem(dstsd,i,0);
						status_change_start(bl,SkillStatusChangeTable[RG_STRIPHELM],skilllv,0,0,0,strip_time,0);
						fail = 0;
					}
				}
			} else {
				if(!sc || (sc->data[SC_CP_WEAPON].timer == -1 && sc->data[SC_STRIPWEAPON].timer == -1)) {
					status_change_start(bl,SkillStatusChangeTable[RG_STRIPWEAPON],skilllv,0,0,0,strip_time,0);
					fail = 0;
				}
				if(!sc || (sc->data[SC_CP_SHIELD].timer == -1 && sc->data[SC_STRIPSHIELD].timer == -1)) {
					status_change_start(bl,SkillStatusChangeTable[RG_STRIPSHIELD],skilllv,0,0,0,strip_time,0);
					fail = 0;
				}
				if(!sc || (sc->data[SC_CP_ARMOR].timer == -1 && sc->data[SC_STRIPARMOR].timer == -1)) {
					status_change_start(bl,SkillStatusChangeTable[RG_STRIPARMOR],skilllv,0,0,0,strip_time,0);
					fail = 0;
				}
				if(!sc || (sc->data[SC_CP_HELM].timer == -1 && sc->data[SC_STRIPHELM].timer == -1)) {
					status_change_start(bl,SkillStatusChangeTable[RG_STRIPHELM],skilllv,0,0,0,strip_time,0);
					fail = 0;
				}
			}
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(fail && sd)
				clif_fullstrip_fail(sd);
		}
		break;
	case AM_POTIONPITCHER:		/* �|�[�V�����s�b�`���[ */
		{
			struct block_list tbl;
			int hp = 0, sp = 0;
			if(sd) {
				int x = (skilllv > 10)? 9: skilllv - 1;
				int i = pc_search_inventory(sd,skill_db[skillid].itemid[x]);

				if(i < 0 || skill_db[skillid].itemid[x] <= 0) {
					clif_skill_fail(sd,skillid,0,0);
					map_freeblock_unlock();
					return 1;
				}
				if(sd->inventory_data[i] == NULL || sd->status.inventory[i].amount < skill_db[skillid].amount[x]) {
					clif_skill_fail(sd,skillid,0,0);
					map_freeblock_unlock();
					return 1;
				}
				sd->state.potionpitcher_flag = 1;
				sd->potion_hp = sd->potion_sp = sd->potion_per_hp = sd->potion_per_sp = 0;
				sd->ud.skilltarget = bl->id;
				if(sd->inventory_data[i]->use_script) {
					run_script(sd->inventory_data[i]->use_script,0,sd->bl.id,0);
				}
				pc_delitem(sd,i,skill_db[skillid].amount[x],0);
				sd->state.potionpitcher_flag = 0;
				if(sd->potion_per_hp > 0 || sd->potion_per_sp > 0) {
					hp = status_get_max_hp(bl) * sd->potion_per_hp / 100;
					hp = hp * (100 + pc_checkskill(sd,AM_POTIONPITCHER)*10 + pc_checkskill(sd,AM_LEARNINGPOTION)*5)/100;
					if(dstsd) {
						sp = dstsd->status.max_sp * sd->potion_per_sp / 100;
						sp = sp * (100 + pc_checkskill(sd,AM_POTIONPITCHER) + pc_checkskill(sd,AM_LEARNINGPOTION)*5)/100;
					}
				} else {
					if(sd->potion_hp > 0) {
						hp = sd->potion_hp * (100 + pc_checkskill(sd,AM_POTIONPITCHER)*10 + pc_checkskill(sd,AM_LEARNINGPOTION)*5)/100;
						hp = hp * (100 + (status_get_vit(bl)<<1)) / 100;
						if(dstsd)
							hp = hp * (100 + pc_checkskill(dstsd,SM_RECOVERY)*10) / 100;
					}
					if(sd->potion_sp > 0) {
						sp = sd->potion_sp * (100 + pc_checkskill(sd,AM_POTIONPITCHER) + pc_checkskill(sd,AM_LEARNINGPOTION)*5)/100;
						sp = sp * (100 + (status_get_int(bl)<<1)) / 100;
						if(dstsd)
							sp = sp * (100 + pc_checkskill(dstsd,MG_SRECOVERY)*10) / 100;
					}
				}
				if(sd->sc.data[SC_ALCHEMIST].timer != -1) {
					hp = hp * (100 + sd->status.base_level) / 100;
					sp = sp * (100 + sd->status.base_level) / 100;
				}
			} else {
				hp = (1 + atn_rand()%400) * (100 + skilllv*10) / 100;
				hp = hp * (100 + (status_get_vit(bl)<<1)) / 100;
				if(dstsd)
					hp = hp * (100 + pc_checkskill(dstsd,SM_RECOVERY)*10) / 100;
			}
			hp = skill_fix_heal(src, bl, skillid, hp);

			memset(&tbl, 0, sizeof(tbl));
			tbl.m = src->m;
			tbl.x = src->x;
			tbl.y = src->y;
			sc = status_get_sc(bl);
			if(sc && sc->data[SC_BERSERK].timer != -1)
				hp = sp = 0;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(hp > 0 || (hp <= 0 && sp <= 0))
				clif_skill_nodamage(&tbl,bl,AL_HEAL,hp,1);
			if(sp > 0)
				clif_skill_nodamage(&tbl,bl,MG_SRECOVERY,sp,1);
			battle_heal(src,bl,hp,sp,0);
		}
		break;

	case CR_SLIMPITCHER:	/* �X�����|�[�V�����s�b�`���[ */
		if(sd && flag&1) {
			struct block_list tbl;
			int hp = sd->potion_hp * (100 + pc_checkskill(sd,CR_SLIMPITCHER)*10 + pc_checkskill(sd,AM_POTIONPITCHER)*10 + pc_checkskill(sd,AM_LEARNINGPOTION)*5)/100;
			hp = hp * (100 + (status_get_vit(bl)<<1))/100;
			if(dstsd) {
				hp = hp * (100 + pc_checkskill(dstsd,SM_RECOVERY)*10)/100;
			}
			hp = skill_fix_heal(&sd->bl, bl, skillid, hp);
			memset(&tbl, 0, sizeof(tbl));
			tbl.m = src->m;
			tbl.x = src->x;
			tbl.y = src->y;
			clif_skill_nodamage(&tbl,bl,AL_HEAL,hp,1);
			battle_heal(NULL,bl,hp,0,0);
		}
		break;

	case AM_BERSERKPITCHER:		/* �o�[�T�[�N�s�b�`���[ */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SC_SPEEDPOTION3,1,0,0,0,900000,0);
		break;
	case AM_CP_WEAPON:
	case AM_CP_SHIELD:
	case AM_CP_ARMOR:
	case AM_CP_HELM:
		{
			int type = SkillStatusChangeTable[skillid];
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			sc = status_get_sc(bl);
			if(type >= 0 && sc && sc->data[type].timer != -1)
				status_change_end(bl, type, -1);
			status_change_start(bl,type,skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		}
		break;
	case CR_FULLPROTECTION:			/* �t���P�~�J���`���[�W */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		sc = status_get_sc(bl);
		if(sc) {
			if(sc->data[SC_STRIPWEAPON].timer != -1)
				status_change_end(bl, SC_STRIPWEAPON, -1);
			if(sc->data[SC_STRIPSHIELD].timer != -1)
				status_change_end(bl, SC_STRIPSHIELD, -1);
			if(sc->data[SC_STRIPARMOR].timer != -1)
				status_change_end(bl, SC_STRIPARMOR, -1);
			if(sc->data[SC_STRIPHELM].timer != -1)
				status_change_end(bl, SC_STRIPHELM, -1);
		}
		status_change_start(bl,SkillStatusChangeTable[AM_CP_WEAPON],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		status_change_start(bl,SkillStatusChangeTable[AM_CP_SHIELD],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		status_change_start(bl,SkillStatusChangeTable[AM_CP_ARMOR],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		status_change_start(bl,SkillStatusChangeTable[AM_CP_HELM],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;

	case SA_DISPELL:			/* �f�B�X�y�� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		// �\�E�������J�[�͖���
		if(dstsd && dstsd->status.class_ == PC_CLASS_SL)
			break;
		sc = status_get_sc(bl);
		if(sc && sc->data[SC_ROGUE].timer != -1)	// ���[�O�̍����͖���
			break;
		if(atn_rand()%100 >= skilllv*10+50)
			break;
		status_change_release(bl,0x02);	// �f�B�X�y���ɂ��X�e�[�^�X�ُ����
		break;

	case TF_BACKSLIDING:		/* �o�b�N�X�e�b�v */
		sc = status_get_sc(src);
		if(!sc || sc->data[SC_ANKLE].timer == -1) {
			// ������ۑ����Ă����āA������΂���ɖ߂�
			int dir = 0, head_dir = 0;
			int count = skill_get_blewcount(skillid,skilllv);
			if(sd) {
				dir = sd->dir;
				head_dir = sd->head_dir;
			}
			unit_stop_walking(src,1);
			skill_blown(src,bl,count|SAB_REVERSEBLOW|SAB_NODAMAGE|SAB_NOPATHSTOP);
			skill_addtimerskill(src,tick + 200,src->id,0,0,skillid,skilllv,0,flag);
			if(sd)
				pc_setdir(sd, dir, head_dir);
			if(sc && sc->data[SC_CLOSECONFINE].timer != -1)
				status_change_end(bl,SC_CLOSECONFINE,-1);
		}
		break;

	case SA_CASTCANCEL:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		unit_skillcastcancel(src,1);
		if(sd) {
			int sp = skill_get_sp(sd->skillid_old,sd->skilllv_old) * (110 - 20 * skilllv) / 100;
			if(sp > 0)
				pc_heal(sd,0,-sp);
		}
		break;
	case SA_SPELLBREAKER:	/* �X�y���u���C�J�[ */
		if(sd && status_get_mode(bl)&0x20 && atn_rand()%100 < 90) {
			clif_skill_fail(sd,skillid,0,0);
			break;
		}
		sc = status_get_sc(bl);
		if(sc && sc->data[SC_MAGICROD].timer != -1) {
			int sp;
			if(dstsd) {
				sp = skill_get_sp(skillid,skilllv) * sc->data[SC_MAGICROD].val2 / 100; 
				if(sp > 0x7fff) sp = 0x7fff;
				else if(sp < 1) sp = 1;
				if(dstsd->status.sp + sp > dstsd->status.max_sp) {
					sp = dstsd->status.max_sp - dstsd->status.sp;
					dstsd->status.sp = dstsd->status.max_sp;
				} else {
					dstsd->status.sp += sp;
				}
				clif_heal(dstsd->fd,SP_SP,sp);
			}
			clif_skill_nodamage(bl,bl,SA_MAGICROD,sc->data[SC_MAGICROD].val1,1);
			if(sd) {
				sp = sd->status.max_sp/5;
				if(sp < 1) sp = 1;
				pc_heal(sd,0,-sp);
			}
		} else {
			struct unit_data *ud = unit_bl2ud(bl);
			if(ud && ud->skilltimer != -1 && ud->skillid > 0 && skill_get_skill_type(ud->skillid) == BF_MAGIC) {
				int sp = skill_get_sp(ud->skillid,ud->skilllv);
				clif_skill_nodamage(src,bl,skillid,skilllv,1);
				unit_skillcastcancel(bl,0);
				if(dstsd)
					pc_heal(dstsd,0,-sp);
				if(sd) {
					sp = sp * 25 * (skilllv - 1) / 100;
					if(skilllv > 1 && sp < 1) sp = 1;
					if(sp > 0x7fff) sp = 0x7fff;
					else if(sp < 1) sp = 1;
					if(sd->status.sp + sp > sd->status.max_sp) {
						sp = sd->status.max_sp - sd->status.sp;
						sd->status.sp = sd->status.max_sp;
					} else {
						sd->status.sp += sp;
					}
					clif_heal(sd->fd,SP_SP,sp);
				}
			} else if(sd) {
				clif_skill_fail(sd,skillid,0,0);
			}
		}
		break;
	case SA_MAGICROD:
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case SA_AUTOSPELL:			/* �I�[�g�X�y�� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd) {
			clif_autospell(sd,skilllv);
		} else {
			int maxlv = 1, spellid = 0;
			if(skilllv >= 10) {
				spellid = MG_FROSTDIVER;
				maxlv = skilllv - 9;
			}
			else if(skilllv >= 8) {
				spellid = MG_FIREBALL;
				maxlv = skilllv - 7;
			}
			else if(skilllv >= 5) {
				spellid = MG_SOULSTRIKE;
				maxlv = skilllv - 4;
			}
			else if(skilllv >= 2) {
				int r = atn_rand()%3;
				if(r == 0)
					spellid = MG_COLDBOLT;
				else if(r == 1)
					spellid = MG_FIREBOLT;
				else
					spellid = MG_LIGHTNINGBOLT;
				maxlv = skilllv - 1;
			}
			else if(skilllv > 0) {
				spellid = MG_NAPALMBEAT;
				maxlv = 3;
			}
			if(spellid > 0)
				status_change_start(src,SC_AUTOSPELL,skilllv,spellid,maxlv,0,skill_get_time(SA_AUTOSPELL,skilllv),0);
		}
		break;
	case PF_MINDBREAKER:
		if(atn_rand()%100 < 55 + skilllv * 5) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);

			// �����E�Ή��E����������
			status_change_attacked_end(bl);

			if(dstmd) {
				int range = skill_get_fixed_range(src,skillid,skilllv);
				mob_target(dstmd,src,range);
				battle_join_struggle(dstmd, src);
			}
			unit_skillcastcancel(bl,2);	// �r���W�Q
		} else if(sd) {
			clif_skill_fail(sd,skillid,0,0);
		}
		break;
	case PF_SOULCHANGE:		/* �\�E���`�F���W */
		if(sd && dstsd) {
			int sp;
			// PVP,GVG�ȊO�ł�PT�����o�[�ɂ̂ݎg�p��
			if(!map[src->m].flag.pvp && !map[src->m].flag.gvg && battle_check_target(src,bl,BCT_PARTY) <= 0)
				break;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			sp = sd->status.sp - dstsd->status.sp;
			pc_heal(sd,0,-sp);
			pc_heal(dstsd,0,sp);
		}
		break;
	case NPC_ATTRICHANGE:	/* �����_�������ω� */
		if(md) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			md->def_ele = atn_rand()%ELE_MAX;
			if(md->def_ele == ELE_UNDEAD)		// �s���͏���
				md->def_ele = ELE_NEUTRAL;
			md->def_ele += (1 + atn_rand()%4) * 20;	// �������x���̓����_��
		} else if(sd) {
			static int armor_element[9]={
				SC_ELEMENTWATER,SC_ELEMENTGROUND,SC_ELEMENTFIRE,SC_ELEMENTWIND,SC_ELEMENTPOISON,
				SC_ELEMENTHOLY,SC_ELEMENTDARK,SC_ELEMENTELEKINESIS,SC_ELEMENTUNDEAD
			};
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(bl,armor_element[atn_rand()%9],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		}
		break;

	case NPC_CHANGEWATER:
	case NPC_CHANGEGROUND:
	case NPC_CHANGEFIRE:
	case NPC_CHANGEWIND:
	case NPC_CHANGEPOISON:
	case NPC_CHANGEHOLY:
	case NPC_CHANGETELEKINESIS:
	case NPC_CHANGEDARKNESS:
		if(md) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			md->def_ele = skill_get_pl(skillid);
			md->def_ele += (1 + atn_rand()%4) * 20;	// �������x���̓����_��
		} else if(sd) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		}
		break;
	case NPC_PROVOCATION:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(md && md->skillidx != -1)
			clif_pet_performance(src,mob_db[md->class_].skill[md->skillidx].val[0]);
		break;

	case NPC_SMOKING:			/* �i�� */
		clif_damage(src,src,tick,status_get_amotion(src),status_get_dmotion(src),3,1,0,0);
		break;

	case NPC_HALLUCINATION:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;

	case NPC_KEEPING:
	case NPC_BARRIER:
		{
			struct unit_data *ud = unit_bl2ud(src);
			int skill_time = skill_get_time(skillid,skilllv);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_time,0);
			if(ud)
				ud->canmove_tick = tick + skill_time;
		}
		break;

	case NPC_DARKBLESSING:
		{
			int sc_def = 100 - status_get_mdef(bl);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(dstsd && dstsd->special_state.no_magic_damage)
				break;
			if(status_get_elem_type(bl) == ELE_DARK || status_get_race(bl) == RCT_DEMON)
				break;
			if(atn_rand()%100 < sc_def * (50 + skilllv * 5) / 100) {
				if(dstsd) {
					int hp = status_get_hp(bl) - 1;
					pc_heal(dstsd,-hp,0);
				}
				else if(dstmd) {
					dstmd->hp = 1;
				}
			}
		}
		break;

	case NPC_LICK:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_weapon_damage )
			break;
		if(dstsd)
			pc_heal(dstsd,0,-100);
		if(atn_rand()%100 < (skilllv * 5) * sc_def_vit / 100)
			status_change_start(bl,SC_STUN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		break;

	case NPC_SUICIDE:			/* ���� */
		if(md) {
			md->state.noexp  = 1;
			md->state.nodrop = 1;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			mob_damage(NULL,md,md->hp,0);
		}
		break;

	case NPC_SUMMONSLAVE:		/* �艺���� */
	case NPC_SUMMONMONSTER:		/* MOB���� */
		if(md && md->skillidx != -1) {
			struct mob_skill *ms = &mob_db[md->class_].skill[md->skillidx];
			mob_summonslave(md,ms->val,sizeof(ms->val)/sizeof(ms->val[0]),skilllv,(skillid == NPC_SUMMONSLAVE)? 1: 0);
		}
		break;
	case NPC_CALLSLAVE:		/* ��芪���Ăі߂� */
		if(md) {
			int mobcount;
			md->recallcount       = 0;	// ������
			md->state.recall_flag = 0;
			mobcount = mob_countslave(md);
			if(mobcount > 0) {
				md->state.recall_flag = 1;	// mob.c��[��芪�������X�^�[�̏���]�ŗ��p
				md->recallmob_count   = mobcount;
			}
		}
		break;
	case NPC_REBIRTH:
		if(md) {
			md->hp = mob_db[md->class_].max_hp * 10 * skilllv / 100;
		}
		unit_stop_walking(src,1);
		unit_stopattack(src);
		unit_skillcastcancel(src,0);
		break;
	case NPC_RUN:		/* ��� */
		if(md) {
			int dx = dirx[md->dir] * skilllv;
			int dy = diry[md->dir] * skilllv;
			mob_unlocktarget(md,tick);
			unit_walktoxy(&md->bl,md->bl.x-dx,md->bl.y-dy);	// �����̌����Ă�������Ƌt�����Ɉړ�
		}
		break;

	case NPC_TRANSFORMATION:
	case NPC_METAMORPHOSIS:
		if(md && md->skillidx != -1) {
			struct mob_skill *ms = &mob_db[md->class_].skill[md->skillidx];
			int size = sizeof(ms->val)/sizeof(ms->val[0]);
			if(skilllv > 1)
				mob_summonslave(md,ms->val,size,skilllv-1,0);
			mob_class_change(md,ms->val,size);
		}
		break;

	case NPC_EMOTION:			/* �G���[�V���� */
	case NPC_EMOTION_ON:			/* ���[�h�`�F���W */
		if(md && md->skillidx != -1) {
			clif_emotion(&md->bl,mob_db[md->class_].skill[md->skillidx].val[0]);
			if(mob_db[md->class_].skill[md->skillidx].val[1]) {	// ���[�h�`�F���W
				md->mode = mob_db[md->class_].skill[md->skillidx].val[1];
				mob_unlocktarget(md, tick);
			}
			status_change_end(src,SC_MODECHANGE,-1);
			if(skillid == NPC_EMOTION_ON)
				status_change_start(src,SC_MODECHANGE,skilllv,skillid,0,0,skill_get_time(skillid,skilllv),0);
		}
		break;

	case WE_MALE:				/* �N�����͌��� */
		if(sd && dstsd) {
			int hp_rate = (skilllv <= 0)? 0: skill_db[skillid].hp_rate[skilllv-1];
			int gain_hp;
			if(battle_config.new_marrige_skill)
				gain_hp = dstsd->status.max_hp;
			else
				gain_hp = sd->status.max_hp;
			gain_hp = gain_hp * abs(hp_rate) / 100;
			gain_hp = skill_fix_heal(&sd->bl, &dstsd->bl, skillid, gain_hp);
			clif_skill_nodamage(src,bl,skillid,gain_hp,1);
			battle_heal(NULL,bl,gain_hp,0,0);
		}
		break;
	case WE_FEMALE:				/* ���Ȃ��ɐs�����܂� */
		if(sd && dstsd) {
			int sp_rate = (skilllv <= 0)? 0: skill_db[skillid].sp_rate[skilllv-1];
			int gain_sp;
			if(battle_config.new_marrige_skill)
				gain_sp = dstsd->status.max_sp;
			else
				gain_sp = sd->status.max_sp;
			gain_sp = gain_sp * abs(sp_rate) / 100;
			clif_skill_nodamage(src,bl,skillid,gain_sp,1);
			battle_heal(NULL,bl,0,gain_sp,0);

			// �X�p�m�r�̉ł��U�߂Ɏg�p�����10%�̊m���ŃX�e�[�^�X�t�^
			if(sd->s_class.job == 23 && sd->sex == 0 && atn_rand()%100 < 10) {
				int sec = skill_get_time2(skillid,skilllv);
				status_change_start(&sd->bl,SkillStatusChangeTable[skillid],skilllv,1,0,0,sec,0);
				status_change_start(&dstsd->bl,SkillStatusChangeTable[skillid],skilllv,2,0,0,sec,0);
			}
		}
		break;

	case WE_CALLPARTNER:			/* ���Ȃ��Ɉ������� */
	case WE_CALLPARENT:			/* �}�}�A�p�p�A���� */
	case WE_CALLBABY:			/* �V��A��������Ⴂ */
		if(sd) {
			int i, d, x, y;
			if(map[sd->bl.m].flag.nomemo) {
				clif_skill_teleportmessage(sd,1);
				map_freeblock_unlock();
				return 0;
			}
			if(skillid == WE_CALLPARTNER)
				clif_callpartner(sd);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);

			if(battle_config.pc_land_skill_limit) {
				int maxcount = skill_get_maxcount(sd->ud.skillid,sd->ud.skilllv);
				if(maxcount > 0) {
					int c = 0;
					struct linkdb_node *node = sd->ud.skillunit;
					while( node ) {
						struct skill_unit_group *group = (struct skill_unit_group *)node->data;
						if(group->alive_count > 0 && group->skill_id == sd->ud.skillid) {
							c++;
						}
						node = node->next;
					}
					if(c >= maxcount) {
						clif_skill_fail(sd,sd->ud.skillid,0,0);
						sd->ud.canact_tick  = tick;
						sd->ud.canmove_tick = tick;
						map_freeblock_unlock();
						return 0;
					}
				}
			}
			// �ڂ̑O�ɌĂяo��
			for(i = 0; i < 8; i++) {
				if(i&1)
					d = (sd->dir - ((i+1)>>1)) & 7;
				else
					d = (sd->dir + ((i+1)>>1)) & 7;

				x = sd->bl.x + dirx[d];
				y = sd->bl.y + diry[d];
				if(map_getcell(sd->bl.m,x,y,CELL_CHKPASS))
					break;
			}
			if(i >= 8) {
				x = sd->bl.x;
				y = sd->bl.y;
			}
			skill_unitsetting(src,skillid,skilllv,x,y,0);
		}
		break;
	case PF_HPCONVERSION:			/* �����͕ϊ� */
		if(sd) {
			if(sd->status.hp <= sd->status.max_hp / 10) {
				clif_skill_fail(sd,skillid,0,0);
			} else {
				int conv_hp = 0, conv_sp = 0;
				clif_skill_nodamage(src, bl, skillid, skilllv, 1);
				conv_hp = sd->status.max_hp / 10; // ��{��MAXHP��10%
				conv_sp = conv_hp * 10 * skilllv / 100;
				if(sd->status.sp + conv_sp > sd->status.max_sp)
					conv_sp = sd->status.max_sp - sd->status.sp;
				pc_heal(sd, -conv_hp, conv_sp);
				clif_heal(sd->fd, SP_SP, conv_sp);
				clif_updatestatus(sd, SP_SP);
			}
		}
		break;
	case HT_REMOVETRAP:				/* �����[�u�g���b�v */
	case MA_REMOVETRAP:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(bl->type == BL_SKILL) {
			struct skill_unit *su = (struct skill_unit *)bl;
			if(!su || !su->group || !skill_unit_istrap(su->group->unit_id))
				break;
			if(su->group->src_id != src->id && !map[bl->m].flag.pvp && !map[bl->m].flag.gvg)
				break;
			if(su->group->unit_id == UNT_ANKLESNARE && su->group->val2) {
				struct block_list *target = map_id2bl(su->group->val2);
				if(target && target->type == BL_PC)
					status_change_end(target,SC_ANKLE,-1);
			} else if(sd) {
				// 㩂����Ԃ�
				struct item item_tmp;
				int eflag;
				if(battle_config.skill_removetrap_type == 1) {
					int i;
					for(i=0; i<10; i++) {
						if(skill_db[su->group->skill_id].itemid[i] <= 0)
							continue;
						memset(&item_tmp,0,sizeof(item_tmp));
						item_tmp.nameid = skill_db[su->group->skill_id].itemid[i];
						item_tmp.amount = skill_db[su->group->skill_id].amount[i];
						item_tmp.identify = 1;
						if((eflag = pc_additem(sd,&item_tmp,item_tmp.amount))) {
							clif_additem(sd,0,0,eflag);
							map_addflooritem(&item_tmp,item_tmp.amount,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0);
						}
					}
				} else {
					memset(&item_tmp,0,sizeof(item_tmp));
					item_tmp.nameid   = 1065;
					item_tmp.identify = 1;
					if((eflag = pc_additem(sd,&item_tmp,1))) {
						clif_additem(sd,0,0,eflag);
						map_addflooritem(&item_tmp,1,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0);
					}
				}

			}
			skill_delunit(su);
		}
		break;
	case HT_SPRINGTRAP:				/* �X�v�����O�g���b�v */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(bl->type == BL_SKILL) {
			struct skill_unit *su = (struct skill_unit *)bl;
			if(!su || !su->group || !skill_unit_istrap(su->group->unit_id))
				break;
			if(su->group->unit_id == UNT_ANKLESNARE && su->group->val2 > 0)		// �⑫���͔j��s��
				break;
			su->group->unit_id = UNT_USED_TRAPS;
			clif_changelook(bl,LOOK_BASE,su->group->unit_id);
			su->group->limit = su->limit = DIFF_TICK(tick+1500,su->group->tick);
		}
		break;
	case BD_ENCORE:					/* �A���R�[�� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if(sd)
			unit_skilluse_id(&sd->bl,src->id,sd->skillid_dance,sd->skilllv_dance);
		break;
	case AS_SPLASHER:		/* �x�i���X�v���b�V���[ */
		if((atn_bignumber)status_get_max_hp(bl)*3/4 < status_get_hp(bl) || status_get_mode(bl)&0x20) {
			// HP��3/4�ȏ�c���Ă��邩���肪�{�X�����Ȃ玸�s
			clif_skill_fail(sd,skillid,0,0);
			map_freeblock_unlock();
			return 1;
		}
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,skillid,src->id,0,skill_get_time(skillid,skilllv),0);
		break;
	case RG_CLOSECONFINE:		/* �N���[�Y�R���t�@�C�� */
		{
			int dir;
			if(status_get_mode(bl)&0x20) {
				if(sd)
					clif_skill_fail(sd,skillid,0,0);
				break;
			}
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(src,SkillStatusChangeTable[skillid],1,1,src->id,bl->id,skill_get_time(skillid,skilllv),0);

			dir = map_calc_dir(src,bl->x,bl->y);
			//unit_setdir(src,dir);
			unit_movepos(bl,src->x+dirx[dir],src->y+diry[dir],0);
			status_change_start(bl,SkillStatusChangeTable[skillid],1,2,bl->id,src->id,skill_get_time(skillid,skilllv),0);
		}
		break;
	case NPC_STOP:			/* �z�[���h�E�F�u */
		status_change_start(src,SkillStatusChangeTable[skillid],1,1,src->id,bl->id,skill_get_time(skillid,skilllv),0);
		status_change_start(bl,SkillStatusChangeTable[skillid],1,2,bl->id,src->id,skill_get_time(skillid,skilllv),0);
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		break;
	case PR_REDEMPTIO:		/* ���f���v�e�B�I */
		if(sd == NULL)
			break;
		if(flag&1) {
			if(unit_isdead(bl)) {
				skill_area_temp[0]++;
				skill_castend_nodamage_id(src,bl,ALL_RESURRECTION,3,tick,1);
			}
		} else {
			skill_area_temp[0] = 0;
			party_foreachsamemap(skill_area_sub,
				sd,AREA_SIZE,
				src,skillid,skilllv,tick, flag|BCT_PARTY|1,
				skill_castend_nodamage_id);

			if(!battle_config.redemptio_penalty_type)
				break;
			if(battle_config.redemptio_penalty_type&1 && skill_area_temp[0] == 0)
				break;
			if(battle_config.redemptio_penalty_type&2 && skill_area_temp[0] >= 5)
				break;

			// HP1, SP0
			pc_heal(sd, -sd->status.hp + 1, -sd->status.sp);

			// �o���l�y�i���e�B
			if(skill_area_temp[0] < 5) {
				int per = (5 - skill_area_temp[0]) * 20;
				pc_exp_penalty(sd, NULL, per, 3);
			}
		}
		break;

	case MO_KITRANSLATION:		/* �C���]�� */
		if(dstsd) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(dstsd->spiritball < 5)
				pc_addspiritball(dstsd,skill_get_time(skillid,skilllv),5);
		}
		break;
	case BS_GREED:			/* �×~ */
		if( sd && !map[src->m].flag.nopenalty && !map[src->m].flag.pvp && !map[src->m].flag.gvg ) {	// �X�EPvP�EGvG�ł͎g�p�s��
			struct party *p = NULL;
			if(sd->status.party_id > 0)
				p = party_search(sd->status.party_id);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			map_foreachinarea(skill_greed,sd->bl.m,sd->bl.x-2,sd->bl.y-2,sd->bl.x+2,sd->bl.y+2,BL_ITEM,sd,p);
		}
		break;

	case GD_BATTLEORDER:		/* �Ր�Ԑ� */
	case GD_REGENERATION:		/* ���� */
	case GD_RESTORE:		/* ���� */
		if(sd) {
			int mi, range;
			struct guild *g = guild_search(sd->status.guild_id);

			if(g == NULL)
				break;
			range = skill_get_range(skillid,skilllv);
			for(mi = 0; mi < g->max_member; mi++)
			{
				struct map_session_data *member = g->member[mi].sd;
				if(member == NULL)
					continue;
				if(sd->bl.m != member->bl.m)
					continue;
				if(unit_distance(sd->bl.x,sd->bl.y,member->bl.x,member->bl.y) <= range) {
					clif_skill_nodamage(src,&member->bl,skillid,skilllv,1);
					if(skillid == GD_RESTORE)
						pc_heal(member, member->status.max_hp * 90 / 100, member->status.max_sp * 90 / 100);
					else
						status_change_start(&member->bl,GetSkillStatusChangeTable(skillid),skilllv,skillid,0,0,skill_get_time(skillid,skilllv),0);
				}
			}
			sd->skillstatictimer[skill_get_skilldb_id(skillid)] = tick + 300000;
		}
		break;
	case GD_EMERGENCYCALL:		/* �ً}���W */
		if(sd) {
			int mi, px, py, count = 0;
			struct guild *g = guild_search(sd->status.guild_id);
			struct cell_xy free_cell[7*7];

			if(g == NULL)
				break;
			clif_skill_nodamage(src,src,skillid,skilllv,1);

			if(battle_config.emergencycall_point_type != 0)
				count = map_searchfreecell(free_cell,sd->bl.m,sd->bl.x-3,sd->bl.y-3,sd->bl.x+3,sd->bl.y+3);

			for(mi = 0; mi < g->max_member; mi++)
			{
				struct map_session_data *member = g->member[mi].sd;
				if(member == NULL)
					continue;
				if(member->bl.id == sd->bl.id)	// �������g�͏��O
					continue;
				if(battle_config.emergencycall_call_limit && sd->bl.m != member->bl.m)	// ���}�b�v�̂�
					continue;
				if(member->state.refuse_emergencycall)
					continue;

				if(count <= 0) {
					// ����
					px = sd->bl.x;
					py = sd->bl.y;
				} else {
					int idx = atn_rand()%count;
					px = free_cell[idx].x;
					py = free_cell[idx].y;
					if(battle_config.emergencycall_point_type == 2 &&
					   path_search(NULL,sd->bl.m,sd->bl.x,sd->bl.y,px,py,0))
					{
						// �R�z��EMC�֎~
						mi--;
						count--;
						free_cell[idx] = free_cell[count];
						continue;
					}
				}
				pc_setpos(member,map[sd->bl.m].name,px,py,3);
			}
			sd->skillstatictimer[skill_get_skilldb_id(skillid)] = tick + 300000;
		}
		break;

	case SG_FEEL:
		if(sd) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			if(sd->feel_index[skilllv-1] == -1)
				clif_feel_display(sd,skilllv);
			else
				clif_feel_info(sd,skilllv);
		}
		break;
	case SG_HATE:
		if(sd) {
			// ���ɓo�^�ς�
			if(sd->hate_mob[skilllv-1] != -1) {
				clif_hate_info(sd,skilllv,sd->hate_mob[skilllv-1]);
				break;
			}
			if(dstsd) {	// �o�^���肪PC
				sd->hate_mob[skilllv-1] = dstsd->status.class_;
				if(battle_config.save_hate_mob)
					pc_setglobalreg(sd,"PC_HATE_MOB_STAR",sd->hate_mob[skilllv-1]+1);
				clif_skill_nodamage(src,src,skillid,skilllv,1);
				clif_hate_mob(sd,skilllv,sd->hate_mob[skilllv-1]);
			} else if(dstmd) {	// �o�^���肪MOB
				switch(skilllv) {
				case 1:
					if(status_get_size(bl) == 0) {
						sd->hate_mob[0] = dstmd->class_;
						if(battle_config.save_hate_mob)
							pc_setglobalreg(sd, "PC_HATE_MOB_SUN", sd->hate_mob[0]+1);
						clif_skill_nodamage(src,src,skillid,skilllv,1);
						clif_hate_mob(sd,skilllv,sd->hate_mob[0]);
					} else {
						clif_skill_fail(sd,skillid,0,0);
					}
					break;
				case 2:
					if(status_get_size(bl) == 1 && status_get_max_hp(bl) >= 6000) {
						sd->hate_mob[1] = dstmd->class_;
						if(battle_config.save_hate_mob)
							pc_setglobalreg(sd, "PC_HATE_MOB_MOON", sd->hate_mob[1]+1);
						clif_skill_nodamage(src,src,skillid,skilllv,1);
						clif_hate_mob(sd,skilllv,sd->hate_mob[1]);
					} else {
						clif_skill_fail(sd,skillid,0,0);
					}
					break;
				case 3:
					if(status_get_size(bl) == 2 && status_get_max_hp(bl) >= 20000) {
						sd->hate_mob[2] = dstmd->class_;
						if(battle_config.save_hate_mob)
							pc_setglobalreg(sd, "PC_HATE_MOB_STAR", sd->hate_mob[2]+1);
						clif_skill_nodamage(src,src,skillid,skilllv,1);
						clif_hate_mob(sd,skilllv,sd->hate_mob[2]);
					} else {
						clif_skill_fail(sd,skillid,0,0);
					}
					break;
				default:
					clif_skill_fail(sd,skillid,0,0);
					break;
				}
			}
		}
		break;
	case SG_FUSION:
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_end(bl,SC_STAR,-1);
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case SL_ALCHEMIST:		/* �A���P�~�X�g�̍� */
	case SL_MONK:			/* �����N�̍� */
	case SL_STAR:			/* �P���Z�C�̍� */
	case SL_SAGE:			/* �Z�[�W�̍� */
	case SL_CRUSADER:		/* �N���Z�C�_�[�̍� */
	case SL_KNIGHT:			/* �i�C�g�̍� */
	case SL_WIZARD:			/* �E�B�U�[�h�̍� */
	case SL_PRIEST:			/* �v���[�X�g�̍� */
	case SL_SUPERNOVICE:		/* �X�[�p�[�m�[�r�X�̍� */
	case SL_BARDDANCER:		/* �o�[�h�ƃ_���T�[�̍� */
	case SL_ROGUE:			/* ���[�O�̍� */
	case SL_ASSASIN:		/* �A�T�V���̍� */
	case SL_BLACKSMITH:		/* �u���b�N�X�~�X�̍� */
	case SL_HUNTER:			/* �n���^�[�̍� */
	case SL_SOULLINKER:		/* �\�E�������J�[�̍� */
	case SL_HIGH:			/* �ꎟ��ʐE�Ƃ̍� */
	case SL_DEATHKNIGHT:		/* �f�X�i�C�g�̍� */
	case SL_COLLECTOR:		/* �_�[�N�R���N�^�[�̍� */
	case SL_NINJA:			/* �E�҂̍� */
	case SL_GUNNER:			/* �K���X�����K�[�̍� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(src,SC_SMA,skilllv,0,0,0,3000,0);
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		break;
	case GS_MADNESSCANCEL:		/* �}��h�l�X�L�����Z���[ */
		sc = status_get_sc(bl);
		if(!sc || sc->data[SC_ADJUSTMENT].timer == -1) {
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		} else if(sd) {
			clif_skill_fail(sd,skillid,0,0);
		}
		break;
	case GS_ADJUSTMENT:			/* �A�W���X�g�����g */
		sc = status_get_sc(bl);
		if(!sc || sc->data[SC_MADNESSCANCEL].timer == -1) {
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
		} else if(sd) {
			clif_skill_fail(sd,skillid,0,0);
		}
		break;
	case GS_CRACKER:			/* �N���b�J�[ */
		{
			int cost, dist;
			cost = skill_get_arrow_cost(skillid,skilllv);
			if(cost > 0 && !battle_delarrow(sd, cost, skillid))	// �e�̏���
				break;
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			dist = unit_distance2(src,bl);
			if( atn_rand()%100 < (50 - dist * 5) * sc_def_vit / 100 ) {
				status_change_start(bl,SC_STUN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
			} else if(sd) {
				clif_skill_fail(sd,skillid,0,0);
			}
		}
		break;
	case NJ_BUNSINJYUTSU:		/* �e���g */
		status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		sc = status_get_sc(bl);
		if(sc && sc->data[SC_NEN].timer != -1)
			status_change_end(bl,SC_NEN,-1);
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		break;
	case HAMI_CASTLE:		/* �L���X�g�����O */
		if(hd && hd->msd && atn_rand()%100 < 20*skilllv)
		{
			int x, y;
			struct map_session_data *msd = hd->msd;
			if( path_search(NULL,hd->bl.m,hd->bl.x,hd->bl.y,msd->bl.x,msd->bl.y,0) != 0 ) {
				// �L���X�g�����O��p�����ǉz���֎~
				if(sd)
					clif_skill_fail(sd,skillid,0,0);
				break;
			}
			clif_skill_nodamage(&hd->bl,&msd->bl,skillid,skilllv,1);

			x = hd->bl.x;
			y = hd->bl.y;

			unit_movepos(&hd->bl,msd->bl.x,msd->bl.y,0);
			unit_movepos(&msd->bl,x,y,0);

			map_foreachinarea(skill_castle_mob_changetarget,hd->bl.m,
				hd->bl.x-AREA_SIZE,hd->bl.y-AREA_SIZE,
				hd->bl.x+AREA_SIZE,hd->bl.y+AREA_SIZE,
				BL_MOB,&msd->bl,&hd->bl);
		}
		break;
	case HVAN_CHAOTIC:		/* �J�I�e�B�b�N�x�l�f�B�N�V���� */
		if(hd) {
			struct block_list* heal_target = NULL;
			int n = (skilllv < 5)? skilllv - 1: 4;
			int rnd = atn_rand()%100;
			static const int per[5][2] = {
				{20,50},{50,60},{25,75},{60,64},{34,67}
			};

			if(rnd < per[n][0]) {
				// �z��
				heal_target = &hd->bl;
			} else if(rnd < per[n][1]) {
				// ��l
				if(!unit_isdead(&hd->msd->bl))	// ����
					heal_target = &hd->msd->bl;
				else
					heal_target = &hd->bl;
			} else {
				// MOB
				heal_target = map_id2bl(hd->target_id);
				if(heal_target == NULL)
					heal_target = &hd->bl;
			}
			if(heal_target) {
				int heal = skill_fix_heal(&hd->bl, heal_target, skillid, skill_calc_heal(src, 1+atn_rand()%skilllv));
				// �G�t�F�N�g�o�Ȃ��̂Ńq�[��
				clif_skill_nodamage(src,heal_target,AL_HEAL,heal,1);
				clif_skill_nodamage(src,heal_target,skillid,heal,1);
				battle_heal(NULL,heal_target,heal,0,0);
				hd->skillstatictimer[skillid-HOM_SKILLID] = tick + skill_get_time2(skillid,skilllv);
			}
		}
		break;
	case HLIF_AVOID:		/* �ً}��� */
	case HAMI_DEFENCE:		/* �f�B�t�F���X */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		if(hd) {
			// ��l�ɂ�
			if(hd->msd && !unit_isdead(&hd->msd->bl)) {
				clif_skill_nodamage(src,&hd->msd->bl,skillid,skilllv,1);
				status_change_start(&hd->msd->bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
			}
			hd->skillstatictimer[skillid-HOM_SKILLID] = tick + skill_get_time2(skillid,skilllv);
		}
		break;
	case HAMI_BLOODLUST:		/* �u���b�h���X�g */
	case HFLI_FLEET:		/* �t���[�g���[�u */
	case HFLI_SPEED:		/* �I�[�o�[�h�X�s�[�h */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		if(hd) {
			hd->skillstatictimer[skillid-HOM_SKILLID] = tick + skill_get_time2(skillid,skilllv);
		}
		break;
	case HLIF_CHANGE:		/* �����^���`�F���W */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		status_change_start(bl,GetSkillStatusChangeTable(skillid),skilllv,0,0,0,skill_get_time(skillid,skilllv),0);
		unit_heal(src,status_get_max_hp(src),0);
		if(hd) {
			hd->skillstatictimer[skillid-HOM_SKILLID] = tick + skill_get_time2(skillid,skilllv);
		}
		break;
	case NPC_DRAGONFEAR:		/* �h���S���t�B�A�[ */
	case NPC_WIDESILENCE:		/* �͈͒��ٍU�� */
	case NPC_WIDEFREEZE:		/* �͈͓����U�� */
	case NPC_WIDEBLEEDING:		/* �͈͏o���U�� */
	case NPC_WIDESTONE:		/* �͈͐Ή��U�� */
	case NPC_WIDECONFUSE:		/* �͈͍����U�� */
	case NPC_WIDESLEEP:		/* �͈͐����U�� */
	case NPC_WIDECURSE:		/* �͈͎􂢍U�� */
	case NPC_WIDESTUN:		/* �͈̓X�^���U�� */
		if(flag&1) {
			if(skillid == NPC_DRAGONFEAR) {
				const int sc_type[4] = { SC_STUN, SC_CURSE, SC_SILENCE, SC_BLEED };
				int n = atn_rand() % 4;
				// upkeep_time2�͔z��̓Y�����ɏ]��
				status_change_start(bl,sc_type[n],skilllv,0,0,0,skill_get_time2(skillid,n+1),0);
			} else {
				status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
			}
		} else {
			int ar = skilllv * 3 - 1;
			//clif_skill_nodamage(src,bl,skillid,skilllv,1);
			clif_skill_damage(src, src, tick, 0, 0, -1, 1, skillid, -1, 0);	// �G�t�F�N�g���o�����߂̎b�菈�u
			map_foreachinarea(skill_area_sub,
				bl->m,bl->x-ar,bl->y-ar,bl->x+ar,bl->y+ar,BL_CHAR,
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_nodamage_id);
		}
		break;
	case NPC_SLOWCAST:		/* �X���E�L���X�g */
		if(flag&1) {
			clif_skill_damage(src, bl, tick, 0, 0, -1, 1, skillid, -1, 0);	// �G�t�F�N�g���o�����߂̎b�菈�u
			status_change_start(bl,SkillStatusChangeTable[skillid],skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
		} else {
			int ar = skilllv * 3 - 1;
			map_foreachinarea(skill_area_sub,
				bl->m,bl->x-ar,bl->y-ar,bl->x+ar,bl->y+ar,BL_CHAR,
				src,skillid,skilllv,tick, flag|BCT_ENEMY|1,
				skill_castend_nodamage_id);
		}
		break;
	case MER_REGAIN:		/* ���Q�C�� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		status_change_end(bl, SC_STUN, -1);
		status_change_end(bl, SC_SLEEP, -1);
		break;
	case MER_TENDER:		/* �e���_�[ */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		status_change_end(bl, SC_FREEZE, -1);
		status_change_end(bl, SC_STONE, -1);
		break;
	case MER_BENEDICTION:		/* �x�l�f�B�N�V���� */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		status_change_end(bl, SC_CURSE, -1);
		status_change_end(bl, SC_BLIND, -1);
		break;
	case MER_RECUPERATE:		/* ���L���|���C�g */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		status_change_end(bl, SC_SILENCE, -1);
		status_change_end(bl, SC_POISON, -1);
		break;
	case MER_MENTALCURE:		/* �����^���L���A */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		status_change_end(bl, SC_CONFUSION, -1);
		status_change_end(bl, SC_HALLUCINATION, -1);
		break;
	case MER_COMPRESS:		/* �R���v���X */
		clif_skill_nodamage(src,bl,skillid,skilllv,1);
		if( dstsd && dstsd->special_state.no_magic_damage )
			break;
		status_change_end(bl, SC_BLEED, -1);
		break;
	case MER_SCAPEGOAT:		/* �g���� */
		if(mcd && mcd->msd) {
			int hp = status_get_hp(&mcd->bl);
			clif_skill_nodamage(src,bl,skillid,skilllv,1);
			pc_heal(mcd->msd,hp,0);
			battle_damage(NULL,&mcd->bl,hp,skillid,skilllv,flag);
		}
		break;
	default:
		printf("skill_castend_nodamage_id: Unknown skill used:%d\n",skillid);
		map_freeblock_unlock();
		return 1;
	}
	map_freeblock_unlock();

	return 0;
}

/*==========================================
 * �X�L���g�p�i�r�������A�ꏊ�w��j
 *------------------------------------------
 */
int skill_castend_pos( int tid, unsigned int tick, int id,int data )
{
	struct block_list *src = map_id2bl(id);
	struct map_session_data *src_sd  = NULL;
	struct mob_data         *src_md  = NULL;
	struct homun_data       *src_hd  = NULL;
	struct merc_data        *src_mcd = NULL;
	struct unit_data        *src_ud  = NULL;
	int range;

	nullpo_retr(0, src);
	nullpo_retr(0, src_ud = unit_bl2ud(src));

	if( src->prev == NULL )
		return 0;

	src_sd  = BL_DOWNCAST( BL_PC ,  src );
	src_md  = BL_DOWNCAST( BL_MOB,  src );
	src_hd  = BL_DOWNCAST( BL_HOM,  src );
	src_mcd = BL_DOWNCAST( BL_MERC, src );

	if( src_ud->skilltimer != tid )	// �^�C�}ID�̊m�F
		return 0;
	if(src_sd && src_ud->skilltimer != -1 && pc_checkskill(src_sd,SA_FREECAST) > 0) {
		src_sd->speed = src_sd->prev_speed;
		clif_updatestatus(src_sd,SP_SPEED);
	}
	src_ud->skilltimer = -1;

	do {
		if(unit_isdead(src))
			break;

		if(src_md) {
			if(src_md->sc.data[SC_ROKISWEIL].timer != -1)
				break;
			if(!(mob_db[src_md->class_].mode & 0x20) && src_md->sc.data[SC_HERMODE].timer != -1)
				break;
			if(src_md->sc.opt1 > 0 || src_md->sc.data[SC_SILENCE].timer != -1 || src_md->sc.data[SC_STEELBODY].timer != -1)
				break;
			if(src_md->sc.data[SC_AUTOCOUNTER].timer != -1 && src_md->ud.skillid != KN_AUTOCOUNTER)
				break;
			if(src_md->sc.data[SC_BLADESTOP].timer != -1)
				break;
			if(src_md->sc.data[SC_BERSERK].timer != -1)
				break;
		}

		if( (src_sd && !battle_config.pc_skill_reiteration) ||
		    (src_md && !battle_config.monster_skill_reiteration) )
		{
			if( skill_get_unit_flag(src_ud->skillid,src_ud->skilllv)&UF_NOREITERATION &&
			    skill_check_unit_range(src->m,src_ud->skillx,src_ud->skilly,src_ud->skillid,src_ud->skilllv) )
				break;
		}
		if( (src_sd && battle_config.pc_skill_nofootset) ||
		    (src_md && battle_config.monster_skill_nofootset) )
		{
			if( skill_get_unit_flag(src_ud->skillid,src_ud->skilllv)&UF_NOFOOTSET &&
			    skill_check_unit_range2(src->m,src_ud->skillx,src_ud->skilly,src_ud->skillid,src_ud->skilllv) )
				break;
		}
		if( (src_sd && battle_config.pc_land_skill_limit) ||
		    (src_md && battle_config.monster_land_skill_limit) )
		{
			int maxcount = skill_get_maxcount(src_ud->skillid,src_ud->skilllv);
			if(maxcount > 0 && skill_count_unitgroup(src_ud,src_ud->skillid) >= maxcount)
				break;
		}

		range = skill_get_fixed_range(src,src_ud->skillid,src_ud->skilllv);
		if(src_sd)
			range += battle_config.pc_skill_add_range;
		else if(src_md)
			range += battle_config.mob_skill_add_range;

		if(!src_sd || battle_config.check_skillpos_range) {	// ��������PC�Ŏ˒��`�F�b�N�����Ȃ炱�̏����͖������ăN���C�A���g�̏���M������
			if(range < unit_distance(src->x,src->y,src_ud->skillx,src_ud->skilly)) {
				if(src_sd && battle_config.skill_out_range_consume)
					skill_check_condition(&src_sd->bl,1);	// �A�C�e������
				break;
			}
		}

		// PC,HOM,MERC�͎g�p�����`�F�b�N
		if(src_sd || src_hd || src_mcd) {
			if(!skill_check_condition(src,1))
				break;
		}
		if(src_sd) {
			src_sd->skillitem      = -1;
			src_sd->skillitemlv    = -1;
			src_sd->skillitem_flag = 0;
		}

		if(src_sd && battle_config.pc_skill_log)
			printf("PC %d skill castend skill=%d\n",src->id,src_ud->skillid);
		if(src_md && battle_config.mob_skill_log)
			printf("MOB skill castend skill=%d, class = %d\n",src_ud->skillid,src_md->class_);

		unit_stop_walking(src,0);
		skill_castend_pos2(src,src_ud->skillx,src_ud->skilly,src_ud->skillid,src_ud->skilllv,tick,0);
		if(src_md)
			src_md->skillidx = -1;
		return 0;
	} while(0);

	// �X�L���g�p���s
	src_ud->canact_tick  = tick;
	src_ud->canmove_tick = tick;
	if(src_sd) {
		clif_skill_fail(src_sd,src_ud->skillid,0,0);
		src_sd->skillitem      = -1;
		src_sd->skillitemlv    = -1;
		src_sd->skillitem_flag = 0;
	} else if(src_md) {
		src_md->skillidx = -1;
	}

	return 0;
}

/*==========================================
 * �X�L���g�p�i�r�������A�ꏊ�w��̎��ۂ̏����j
 *------------------------------------------
 */
int skill_castend_pos2( struct block_list *src, int x,int y,int skillid,int skilllv,unsigned int tick,int flag)
{
	struct map_session_data *sd = NULL;
	struct mob_data *md = NULL;

	nullpo_retr(0, src);

	sd = BL_DOWNCAST( BL_PC,  src );
	md = BL_DOWNCAST( BL_MOB, src );

	if( skillid != WZ_METEOR &&
	    skillid != WZ_ICEWALL &&
	    skillid != AM_CANNIBALIZE &&
	    skillid != AM_SPHEREMINE &&
	    skillid != CR_CULTIVATION )
		clif_skill_poseffect(src,skillid,skilllv,x,y,tick);

	// �G��
	if(md && md->skillidx != -1)
	{
		short emotion = mob_db[md->class_].skill[md->skillidx].emotion;
		if(emotion >= 0)
			clif_emotion(&md->bl,emotion);
	}

	switch(skillid)
	{
	case AC_SHOWER:				/* �A���[�V�����[ */
	case MA_SHOWER:
		if(sd) {
			int cost = skill_get_arrow_cost(skillid,skilllv);
			if(cost > 0 && !battle_delarrow(sd, cost, skillid))	// ��̏���
				break;
		}
		skill_area_temp[1] = src->id;
		skill_area_temp[2] = x;
		skill_area_temp[3] = y;
		map_foreachinarea(skill_area_sub,
			src->m,x-1,y-1,x+1,y+1,(BL_CHAR|BL_SKILL),
			src,skillid,skilllv,tick,flag|BCT_ENEMY|1,
			skill_castend_damage_id);
		map_foreachinarea(skill_area_trap_sub,
			src->m,x-1,y-1,x+1,y+1,BL_SKILL,
			src,skillid,skilllv,tick,flag|BCT_ENEMY|1,
			skill_castend_damage_id);
		break;

	case PR_BENEDICTIO:			/* ���̍~�� */
		skill_area_temp[1] = src->id;
		map_foreachinarea(skill_area_sub,
			src->m,x-1,y-1,x+1,y+1,BL_PC,
			src,skillid,skilllv,tick, flag|BCT_ALL|1,
			skill_castend_nodamage_id);
		map_foreachinarea(skill_area_sub,
			src->m,x-1,y-1,x+1,y+1,BL_CHAR,
			src,skillid,skilllv,tick, flag|BCT_ALL|1,
			skill_castend_damage_id);
		break;

	case BS_HAMMERFALL:			/* �n���}�[�t�H�[�� */
		skill_addtimerskill(src,tick+skill_get_time(skillid,skilllv),0,x,y,skillid,skilllv,BF_WEAPON,flag|BCT_ENEMY|2);
		break;

	case HT_DETECTING:				/* �f�B�e�N�e�B���O */
		map_foreachinarea(status_change_timer_sub,src->m,x-3,y-3,x+3,y+3,BL_CHAR,src,SC_SIGHT,skilllv,tick);
		break;

	case WZ_ICEWALL:			/* �A�C�X�E�H�[�� */
		if(map[src->m].flag.noicewall) {
			if(sd)
				clif_skill_fail(sd,skillid,0,0);
		} else {
			clif_skill_poseffect(src,skillid,skilllv,x,y,tick);
			skill_unitsetting(src,skillid,skilllv,x,y,0);
		}
		break;

	case MG_SAFETYWALL:			/* �Z�C�t�e�B�E�H�[�� */
	case MG_FIREWALL:			/* �t�@�C���[�E�H�[�� */
	case MG_THUNDERSTORM:		/* �T���_�[�X�g�[�� */
	case AL_PNEUMA:				/* �j���[�} */
	case WZ_FIREPILLAR:			/* �t�@�C�A�s���[ */
	case WZ_QUAGMIRE:			/* �N�@�O�}�C�A */
	case WZ_VERMILION:			/* ���[�h�I�u���@�[�~���I�� */
	case WZ_STORMGUST:			/* �X�g�[���K�X�g */
	case WZ_HEAVENDRIVE:		/* �w�����Y�h���C�u */
	case PR_SANCTUARY:			/* �T���N�`���A�� */
	case PR_MAGNUS:				/* �}�O�k�X�G�N�\�V�Y�� */
	case CR_GRANDCROSS:			/* �O�����h�N���X */
	case NPC_GRANDDARKNESS:			/* �O�����h�_�[�N�l�X */
	case HT_SKIDTRAP:			/* �X�L�b�h�g���b�v */
	case HT_LANDMINE:			/* �����h�}�C�� */
	case HT_ANKLESNARE:			/* �A���N���X�l�A */
	case HT_SHOCKWAVE:			/* �V���b�N�E�F�[�u�g���b�v */
	case HT_SANDMAN:			/* �T���h�}�� */
	case HT_FLASHER:			/* �t���b�V���[ */
	case HT_FREEZINGTRAP:		/* �t���[�W���O�g���b�v */
	case HT_BLASTMINE:			/* �u���X�g�}�C�� */
	case HT_CLAYMORETRAP:		/* �N���C���A�[�g���b�v */
	case AS_VENOMDUST:			/* �x�m���_�X�g */
	case AM_DEMONSTRATION:		/* �f�����X�g���[�V���� */
	case PF_SPIDERWEB:			/* �X�p�C�_�[�E�F�b�u */
	case PF_FOGWALL:			/* �t�H�O�E�H�[�� */
	case HT_TALKIEBOX:			/* �g�[�L�[�{�b�N�X */
	case NJ_TATAMIGAESHI:		/* ���Ԃ� */
	case NJ_BAKUENRYU:			/* �����w */
	case NPC_EVILLAND:			/* �C�r�������h */
	case MA_SKIDTRAP:
	case MA_LANDMINE:
	case MA_SANDMAN:
	case MA_FREEZINGTRAP:
		skill_unitsetting(src,skillid,skilllv,x,y,0);
		break;
	case HW_GRAVITATION:		/* �O���r�e�[�V�����t�B�[���h */
		{
			struct skill_unit_group *sg = skill_unitsetting(src,skillid,skilllv,x,y,0);
			if(sg) {
				struct unit_data *ud = unit_bl2ud(src);
				if(ud && DIFF_TICK(ud->canact_tick, tick) < 5000) {
					// ������5�b�Ԃ̓����O�֎~
					ud->canact_tick = tick + 5000;
				}
				status_change_start(src,SC_GRAVITATION_USER,skilllv,0,0,(int)sg,skill_get_time(skillid,skilllv),0);
			}
		}
		break;
	case RG_GRAFFITI:			/* �O���t�B�e�B */
		status_change_start(src,SkillStatusChangeTable[skillid],skilllv,x,y,0,skill_get_time(skillid,skilllv),0);
		break;
	case GS_GROUNDDRIFT:			/* �O���E���h�h���t�g*/
		if(sd) {
			int cost = skill_get_arrow_cost(skillid,skilllv);
			if(cost > 0 && !battle_delarrow(sd, cost, skillid))	// �e�̏���
				break;
		}
		skill_unitsetting(src,skillid,skilllv,x,y,0);
		break;

	case SA_VOLCANO:		/* �{���P�[�m */
	case SA_DELUGE:			/* �f�����[�W */
	case SA_VIOLENTGALE:		/* �o�C�I�����g�Q�C�� */
	case SA_LANDPROTECTOR:		/* �����h�v���e�N�^�[ */
	case NJ_SUITON:			/* ���� */
	case NJ_KAENSIN:		/* �Ή��w*/
		skill_clear_element_field(src);	// ���Ɏ������������Ă��鑮������N���A
		skill_unitsetting(src,skillid,skilllv,x,y,0);
		break;

	case WZ_METEOR:			/* ���e�I�X�g�[�� */
		{
			int i, tmpx = 0, tmpy = 0, x1 = 0, y1 = 0;
			int interval = (skilllv > 10)? 2500: 1000;
			for(i=0; i<2+(skilllv>>1); i++) {
				if(skilllv > 10) {
					tmpx = x + (atn_rand()%29 - 14);
					tmpy = y + (atn_rand()%29 - 14);
				} else {
					tmpx = x + (atn_rand()%7 - 3);
					tmpy = y + (atn_rand()%7 - 3);
				}
				if(i == 0 && map_getcell(src->m,tmpx,tmpy,CELL_CHKPASS)) {
					clif_skill_poseffect(src,skillid,skilllv,tmpx,tmpy,tick);
				} else if(i > 0) {
					skill_addtimerskill(src,tick+i*interval,0,tmpx,tmpy,skillid,skilllv,(x1<<16)|y1,0);
				}
				x1 = tmpx;
				y1 = tmpy;
			}
			skill_addtimerskill(src,tick+i*interval,0,tmpx,tmpy,skillid,skilllv,-1,0);
		}
		break;

	case AL_WARP:				/* ���[�v�|�[�^�� */
		if(sd) {
			const char *p[3];
			int i = 0;
			if(battle_config.noportal_flag) {
				if(map[sd->bl.m].flag.noportal)		// noportal�ŋ֎~
					break;
			} else {
				if(map[sd->bl.m].flag.noteleport)	// noteleport�ŋ֎~
					break;
			}
			for(i=0; i<3; i++) {
				if(sd->ud.skilllv > i+1 && i < MAX_PORTAL_MEMO)
					p[i] = sd->status.memo_point[i].map;
				else
					p[i] = "";
			}
			clif_skill_warppoint(sd,sd->ud.skillid,sd->status.save_point.map, p[0], p[1], p[2]);
		}
		break;
	case HW_GANBANTEIN:			/* �K�o���e�C�� */
		if(atn_rand()%100 < 80) {
			map_foreachinarea(skill_delunit_by_ganbantein,src->m,x-1,y-1,x+1,y+1,BL_SKILL);
		} else if(sd) {
			clif_skill_fail(sd,skillid,0,0);
		}
		break;
	case MO_BODYRELOCATION:			/* �c�e */
		{
			struct status_change *sc = status_get_sc(src);
			if(!sc || sc->data[SC_ANKLE].timer == -1) {
				unit_movepos(src,x,y,0);
				if(sd) {
					sd->skillstatictimer[MO_EXTREMITYFIST] = tick + 2000;
				}
				clif_skill_poseffect(src,skillid,skilllv,x,y,tick);
			}
		}
		break;
	case AM_CANNIBALIZE:	/* �o�C�I�v�����g */
		if(sd) {
			int n, id = 0;
			int summons[5] = { 1589, 1579, 1575, 1555, 1590 };
			struct mob_data *tmpmd = NULL;

			n  = (skilllv > 5)? 4: skilllv - 1;
			id = mob_once_spawn(sd,"this", x, y, sd->status.name, summons[n], 1, "");

			if((tmpmd = map_id2md(id)) != NULL) {
				tmpmd->master_id = sd->bl.id;
				tmpmd->guild_id  = status_get_guild_id(src);
				tmpmd->hp        = 1500 + skilllv * 200 + sd->status.base_level * 10;

				// ��ړ��ŃA�N�e�B�u�Ŕ�������[0x0:��ړ� 0x1:�ړ� 0x4:ACT 0x8:��ACT 0x40:������ 0x80:�����L]
				tmpmd->mode = 0x0 + 0x4 + 0x80;

				tmpmd->deletetimer  = add_timer(gettick()+skill_get_time(skillid,skilllv),mob_timer_delete,id,0);
				tmpmd->state.nodrop = battle_config.cannibalize_no_drop;
				tmpmd->state.noexp  = battle_config.cannibalize_no_exp;
				tmpmd->state.nomvp  = battle_config.cannibalize_no_mvp;
				tmpmd->state.special_mob_ai = 1;
			}
			clif_skill_poseffect(src,skillid,skilllv,x,y,tick);
		}
		break;
	case CR_CULTIVATION:	/* �A���͔| */
		if(sd) {
			int id, n = (skilllv > 2)? 1: 0;
			int summons[2][6] = {
				{ 1084, 1085, 1084, 1085, 1084, 1085 },
				{ 1078, 1079, 1080, 1081, 1082, 1083 }
			};
			struct mob_data *tmpmd = NULL;
			if(atn_rand()%100 < 50) {	// 50%�Ŏ��s
				clif_skill_fail(sd,skillid,0,0);
				break;
			}
			id = mob_once_spawn(sd,"this", x, y,"--ja--",summons[n][atn_rand()%6], 1, "");

			if((tmpmd = map_id2md(id)) != NULL)
				tmpmd->deletetimer = add_timer(gettick()+skill_get_time(skillid,skilllv),mob_timer_delete,id,0);
			clif_skill_poseffect(src,skillid,skilllv,x,y,tick);
		}
		break;
	case AM_SPHEREMINE:	/* �X�t�B�A�[�}�C�� */
		if(sd) {
			int id = 0;
			struct mob_data *tmpmd = NULL;

			id = mob_once_spawn(sd,"this", x, y, sd->status.name, 1142, 1, "");

			if((tmpmd = map_id2md(id)) != NULL) {
				tmpmd->master_id    = sd->bl.id;
				tmpmd->hp           = 2000 + skilllv * 400;
				tmpmd->def_ele      = 40 + ELE_WATER;
				tmpmd->deletetimer  = add_timer(gettick()+skill_get_time(skillid,skilllv),mob_timer_delete,id,0);
				tmpmd->state.nodrop = battle_config.spheremine_no_drop;
				tmpmd->state.noexp  = battle_config.spheremine_no_exp;
				tmpmd->state.nomvp  = battle_config.spheremine_no_mvp;
				tmpmd->state.special_mob_ai = 2;
			}
			clif_skill_poseffect(src,skillid,skilllv,x,y,tick);
		}
		break;
	case CR_SLIMPITCHER:
		if(sd) {
			int i = (skilllv > 10)? 9: skilllv - 1;
			int j, itemid;

			if(battle_config.slimpitcher_nocost && !map[sd->bl.m].flag.pvp && !map[sd->bl.m].flag.gvg) {
				const int potion[10] = { 501, 501, 501, 501, 501, 503, 503, 503, 503, 504 };
				itemid = potion[i];
			} else {
				itemid = skill_db[skillid].itemid[i];
			}
			j = pc_search_inventory(sd,itemid);
			if(j < 0 || itemid <= 0 || sd->inventory_data[j] == NULL ||
			   sd->status.inventory[j].amount < skill_db[skillid].amount[i]) {
				clif_skill_fail(sd,skillid,0,0);
				return 1;
			}
			sd->state.potionpitcher_flag = 1;
			sd->potion_hp = 0;
			if(sd->inventory_data[j]->use_script) {
				run_script(sd->inventory_data[j]->use_script,0,sd->bl.id,0);
			}
			pc_delitem(sd,j,skill_db[skillid].amount[i],0);
			sd->state.potionpitcher_flag = 0;
			clif_skill_poseffect(src,skillid,skilllv,x,y,tick);
			if(sd->potion_hp > 0) {
				map_foreachinarea(skill_area_sub,
					src->m,x-3,y-3,x+3,y+3,BL_CHAR,
					src,skillid,skilllv,tick,flag|BCT_PARTY|1,
					skill_castend_nodamage_id);
			}
		}
		break;
	case NJ_SHADOWJUMP:	/* �e���� */
		{
			struct status_change *sc = status_get_sc(src);
			if(!sc || sc->data[SC_ANKLE].timer == -1) {
				// �R�ł��\�Z���͖������Ĉړ�
				if(map_getcellp(&map[sd->bl.m],x,y,CELL_CHKPASS)) {
					unit_movepos(&sd->bl,x,y,0x21);
					status_change_end(src, SC_HIDING, -1);
				}
			}
		}
		break;
	}
	return 0;
}

/*==========================================
 * �X�L���g�p�i�r�������Amap�w��j
 *------------------------------------------
 */
void skill_castend_map( struct map_session_data *sd,int skill_num, const char *mapname)
{
	nullpo_retv(sd);

	if( sd->bl.prev == NULL || unit_isdead(&sd->bl) )
		return;

	// �s���p�P�b�g
	if(skill_num != sd->ud.skillid)
		return;

	if( sd->sc.opt1 > 0 || sd->sc.option&2 )
		return;

	// �X�L�����g���Ȃ���Ԉُ풆
	if( sd->sc.data[SC_SILENCE].timer != -1 ||
	    sd->sc.data[SC_ROKISWEIL].timer != -1 ||
	    sd->sc.data[SC_HERMODE].timer != -1 ||
	    sd->sc.data[SC_AUTOCOUNTER].timer != -1 ||
	    sd->sc.data[SC_STEELBODY].timer != -1 ||
	    sd->sc.data[SC_BERSERK].timer != -1 ||
	    (skill_num != CG_LONGINGFREEDOM && sd->sc.data[SC_DANCING].timer != -1 && sd->sc.data[SC_LONGINGFREEDOM].timer == -1) )
		return;

	unit_stopattack(&sd->bl);

	if(battle_config.pc_skill_log)
		printf("PC %d skill castend skill =%d map=%s\n",sd->bl.id,skill_num,mapname);
	unit_stop_walking(&sd->bl,0);

	if(strcmp(mapname,"cancel") == 0)
		return;

	switch(skill_num) {
	case AL_TELEPORT:		/* �e���|�[�g */
		{
			int alive = 1;
			map_foreachinarea(skill_landprotector,sd->bl.m,sd->bl.x,sd->bl.y,sd->bl.x,sd->bl.y,BL_SKILL,AL_TELEPORT,&alive);
			if(sd && alive) {
				if(strcmp(mapname,"Random") == 0)
					pc_randomwarp(sd,3);
				else
					pc_setpos(sd,sd->status.save_point.map,sd->status.save_point.x,sd->status.save_point.y,3);
			}
		}
		break;
	case AL_WARP:			/* ���[�v�|�[�^�� */
		{
			const struct point *p[MAX_PORTAL_MEMO+1];
			struct skill_unit_group *group;
			int i, x = 0, y = 0;

			p[0] = &sd->status.save_point;
			for(i=0; i<MAX_PORTAL_MEMO; i++) {
				p[i+1] = &sd->status.memo_point[i];
			}
			if(battle_config.pc_land_skill_limit) {
				int maxcount = skill_get_maxcount(sd->ud.skillid,sd->ud.skilllv);
				if(maxcount > 0 && skill_count_unitgroup(&sd->ud,sd->ud.skillid) >= maxcount) {
					clif_skill_fail(sd,sd->ud.skillid,0,0);
					sd->ud.canact_tick = sd->ud.canmove_tick = gettick();
					break;
				}
			}
			for(i=0; i<sd->ud.skilllv; i++) {
				if(strcmp(mapname,p[i]->map) == 0) {
					x = p[i]->x;
					y = p[i]->y;
					break;
				}
			}
			if(x == 0 || y == 0)	// �s���p�P�b�g�H
				break;

			if(!battle_config.pc_skill_reiteration) {
				if( skill_get_unit_flag(sd->ud.skillid,sd->ud.skilllv)&UF_NOREITERATION &&
				    skill_check_unit_range(sd->bl.m,sd->ud.skillx,sd->ud.skilly,sd->ud.skillid,sd->ud.skilllv) )
					break;
			}
			if(battle_config.pc_skill_nofootset) {
				if( skill_get_unit_flag(sd->ud.skillid,sd->ud.skilllv)&UF_NOFOOTSET &&
				    skill_check_unit_range2(sd->bl.m,sd->ud.skillx,sd->ud.skilly,sd->ud.skillid,sd->ud.skilllv) )
					break;
			}

			if(!skill_check_condition(&sd->bl,3))
				break;
			if((group = skill_unitsetting(&sd->bl,sd->ud.skillid,sd->ud.skilllv,sd->ud.skillx,sd->ud.skilly,0)) == NULL)
				break;
			group->valstr = (char *)aCalloc(16, sizeof(char)); // max map_name is 15 char + NULL
			memcpy(group->valstr, mapname, 15);
			group->val2 = (x<<16)|y;
		}
		break;
	}

	return;
}

/*==========================================
 * �X�L�����j�b�g�ݒ菈��
 *------------------------------------------
 */
struct skill_unit_group *skill_unitsetting( struct block_list *src, int skillid,int skilllv,int x,int y,int flag)
{
	struct skill_unit_group *group;
	int i,limit,val1=0,val2=0,val3=0,on_flag=0;
	int target,interval,range,unit_flag,unit_id;
	struct skill_unit_layout *layout;
	struct map_session_data *sd = NULL;

	nullpo_retr(0, src);

	if(src->type == BL_PC)
		sd = (struct map_session_data *)src;

	limit     = skill_get_time(skillid,skilllv);
	range     = skill_get_unit_range(skillid);
	interval  = skill_get_unit_interval(skillid);
	target    = skill_get_unit_target(skillid);
	unit_flag = skill_get_unit_flag(skillid,skilllv);
	layout    = skill_get_unit_layout(skillid,skilllv,src,x,y);
	unit_id   = skill_get_unit_id(skillid,flag&1);

	if(unit_flag&UF_DEFNOTENEMY && battle_config.defnotenemy)
		target = BCT_NOENEMY;

	switch (skillid) {
	case MG_SAFETYWALL:			/* �Z�C�t�e�B�E�H�[�� */
		val2 = skilllv+1;
		break;
	case WZ_METEOR:
		if(skilllv>10)			/* ���e�I(�L�͈�) */
		range = 10;
		break;
	case WZ_VERMILION:
		if(skilllv>10)			/* ���[�h�I�u�o�[�~���I��(�L�͈�) */
		range = 25;
		break;
	case MG_FIREWALL:			/* �t�@�C���[�E�H�[�� */
		val2 = 4+skilllv;
		break;
	case AL_WARP:				/* ���[�v�|�[�^�� */
		val1 = skilllv+6;
		if(flag==0)
			limit=2000;
		break;
	case PR_SANCTUARY:			/* �T���N�`���A�� */
		val1 = skilllv*2+6;
		val2 = skill_fix_heal(src, NULL, skillid, ((skilllv > 6)? 777: skilllv * 100));
		interval = interval + 500;
		break;
	case WZ_FIREPILLAR:			/* �t�@�C�A�[�s���[ */
		if (flag!=0)
			limit = 150;
		val1 = skilllv+2;
		break;
	case HT_SKIDTRAP:			/* �X�L�b�h�g���b�v */
	case MA_SKIDTRAP:
		val1 = src->x;
		val2 = src->y;
		if(map[src->m].flag.gvg)
			limit <<= 2;
		break;
	case HT_LANDMINE:		/* �����h�}�C�� */
	case HT_ANKLESNARE:		/* �A���N���X�l�A */
	case HT_SHOCKWAVE:		/* �V���b�N�E�F�[�u�g���b�v */
	case HT_SANDMAN:		/* �T���h�}�� */
	case HT_FLASHER:		/* �t���b�V���[ */
	case HT_FREEZINGTRAP:		/* �t���[�W���O�g���b�v */
	case HT_BLASTMINE:		/* �u���X�g�}�C�� */
	case HT_CLAYMORETRAP:		/* �N���C���A�g���b�v */
	case HT_TALKIEBOX:		/* �g�[�L�[�{�b�N�X */
	case MA_LANDMINE:
	case MA_SANDMAN:
	case MA_FREEZINGTRAP:
		if(map[src->m].flag.gvg)
			limit <<= 2;
		break;
	case BA_WHISTLE:			/* ���J */
		if(sd)
			val1 = (pc_checkskill(sd,BA_MUSICALLESSON)+1)>>1;
		val2 = ((status_get_agi(src)/10)&0xffff)<<16;
		val2 |= (status_get_luk(src)/10)&0xffff;
		break;
	case DC_HUMMING:			/* �n�~���O */
		if(sd)
			val1 = (pc_checkskill(sd,DC_DANCINGLESSON)+1)>>1;
		val2 = status_get_dex(src)/10;
		break;
	case DC_DONTFORGETME:		/* ����Y��Ȃ��Łc */
		if(sd)
			val1 = (pc_checkskill(sd,DC_DANCINGLESSON)+1)>>1;
		val2 = ((status_get_str(src)/20)&0xffff)<<16;
		val2 |= (status_get_agi(src)/10)&0xffff;
		break;
	case BA_POEMBRAGI:			/* �u���M�̎� */
		if(sd)
			val1 = pc_checkskill(sd,BA_MUSICALLESSON);
		val2 = ((status_get_dex(src)/10)&0xffff)<<16;
		val2 |= (status_get_int(src)/5)&0xffff;
		break;
	case BA_APPLEIDUN:			/* �C�h�D���̗ь� */
		if(sd)
			val1 = pc_checkskill(sd,BA_MUSICALLESSON);
		val2 = status_get_vit(src);
		val3 = 0;
		break;
	case DC_SERVICEFORYOU:		/* �T�[�r�X�t�H�[���[ */
		if(sd)
			val1 = (pc_checkskill(sd,DC_DANCINGLESSON)+1)>>1;
		val2 = status_get_int(src)/10;
		break;
	case BA_ASSASSINCROSS:		/* �[�z�̃A�T�V���N���X */
		if(sd)
			val1 = (pc_checkskill(sd,BA_MUSICALLESSON)+1)>>1;
		val2 = status_get_agi(src)/20;
		break;
	case DC_FORTUNEKISS:		/* �K�^�̃L�X */
		if(sd)
			val1 = (pc_checkskill(sd,DC_DANCINGLESSON)+1)>>1;
		val2 = status_get_luk(src)/10;
		break;
	case HP_BASILICA:
		val1 = src->id;
		break;
	case SA_VOLCANO:		/* �{���P�[�m */
	case SA_DELUGE:			/* �f�����[�W */
	case SA_VIOLENTGALE:	/* �o�C�I�����g�Q�C�� */
		if(sd) {
			if(sd->sc.data[SC_ELEMENTFIELD].timer != -1)
			{
				// ���x���̒Ⴂ���̂��g�����ꍇ�������Ԍ����H
				// ������̎c�莞�ԎZ�o
				limit = sd->sc.data[SC_ELEMENTFIELD].val2 - DIFF_TICK(gettick(), (unsigned int)sd->sc.data[SC_ELEMENTFIELD].val3);
			} else {
				status_change_start(src,SC_ELEMENTFIELD,1,skill_get_time(skillid,skilllv),gettick(),0,0,0);
			}
		}
		break;
	case GS_GROUNDDRIFT:	/* �O���E���h�h���t�g */
		{
			const unsigned char drift_id[] = {
				UNT_GROUNDDRIFT_FIRE,
				UNT_GROUNDDRIFT_WIND,
				UNT_GROUNDDRIFT_POISON,
				UNT_GROUNDDRIFT_DARK,
				UNT_GROUNDDRIFT_WATER
			};
			if(sd) {
				short idx = sd->equip_index[10];
				if(idx >= 0) {
					int n;
					for(n = 0; n < 5; n++) {
						// �X�t�B�A�̃^�C�v�ŃX�L�����j�b�g�����߂�
						if(sd->inventory_data[idx]->arrow_type & (512 << n)) {
							unit_id = drift_id[n];
							break;
						}
					}
				}
			} else {
				unit_id = drift_id[atn_rand()%5];
			}
		}
		break;
	case NPC_EVILLAND:		/* �C�r�������h */
		val1 = (skilllv > 6)? 666: skilllv*100;
		interval = interval + 500;
		break;
	}

	nullpo_retr( NULL, group = skill_initunitgroup(src,layout->count,skillid,skilllv,unit_id,gettick()) );
	group->limit       = limit;
	group->val1        = val1;
	group->val2        = val2;
	group->val3        = val3;
	group->target_flag = target;
	group->interval    = interval;

	if(skillid == HT_TALKIEBOX || skillid == RG_GRAFFITI) {
		group->valstr = (char *)aCalloc(80,sizeof(char));
		if(sd)
			memcpy(group->valstr,sd->message,80);
	}

	for(i=0; i<layout->count; i++) {
		struct skill_unit *unit;
		int ux, uy, alive = 1;

		ux    = x + layout->dx[i];
		uy    = y + layout->dy[i];
		val1  = skilllv;
		val2  = 0;
		limit = group->limit;

		switch (skillid) {
			case MG_FIREWALL:		/* �t�@�C���[�E�H�[�� */
				val2 = group->val2;
				// �Q�C����Ȃ玞�Ԕ{
				if(map_find_skill_unit_oncell(src,ux,uy,SA_VIOLENTGALE,NULL)!=NULL)
				{
					limit = limit*150/100;
					on_flag = 1;
				}
				break;
			case WZ_ICEWALL:		/* �A�C�X�E�H�[�� */
				val1 = (skilllv<=1)? 500: 200+200*skilllv;
				break;
			case PF_FOGWALL:
				// �f�����[�W��Ȃ玞�Ԕ{
				if(map_find_skill_unit_oncell(src,ux,uy,SA_DELUGE,NULL)!=NULL)
				{
					limit = limit*2;
					on_flag = 1;
				}
				break;
			case HT_LANDMINE:		/* �����h�}�C�� */
			case HT_ANKLESNARE:		/* �A���N���X�l�A */
			case HT_SHOCKWAVE:		/* �V���b�N�E�F�[�u�g���b�v */
			case HT_SANDMAN:		/* �T���h�}�� */
			case HT_FLASHER:		/* �t���b�V���[ */
			case HT_FREEZINGTRAP:		/* �t���[�W���O�g���b�v */
			case HT_TALKIEBOX:		/* �g�[�L�[�{�b�N�X */
			case HT_SKIDTRAP:		/* �X�L�b�h�g���b�v */
			case MA_SKIDTRAP:
			case MA_LANDMINE:
			case MA_SANDMAN:
			case MA_FREEZINGTRAP:
				val1 = 3500;	// 㩂̑ϋvHP
				break;
			case NJ_KAENSIN:		/* �Ή��w */
				val1 = 4+(skilllv+1)/2;
				break;
		}
		// ����X�L���̏ꍇ�ݒu���W��Ƀ����h�v���e�N�^�[���Ȃ����`�F�b�N
		if(range <= 0) {
			switch(skillid) {
				case BD_LULLABY:	/* �q��� */
				case BA_DISSONANCE:	/* �s���a�� */
				case BA_WHISTLE:	/* ���J */
				case BA_ASSASSINCROSS:	/* �[�z�̃A�T�V���N���X */
				case BA_POEMBRAGI:	/* �u���M�̎�*/
				case BA_APPLEIDUN:	/* �C�h�D���̗ь� */
				case DC_UGLYDANCE:	/* ��������ȃ_���X */
				case DC_HUMMING:	/* �n�~���O */
				case DC_DONTFORGETME:	/* ����Y��Ȃ��Łc */
				case DC_FORTUNEKISS:	/* �K�^�̃L�X */
				case DC_SERVICEFORYOU:	/* �T�[�r�X�t�H�[���[ */
					break;
				default:
					map_foreachinarea(skill_landprotector,src->m,ux,uy,ux,uy,BL_SKILL,skillid,&alive);
			}
		}

		if(unit_flag&UF_PATHCHECK) { // �ː��`�F�b�N
			if(!path_search_long(NULL,src->m,src->x,src->y,ux,uy))
				alive = 0;
		}

		if(skillid == WZ_ICEWALL && alive) {
			val2 = map_getcell(src->m,ux,uy,CELL_GETTYPE);
			if(val2 == 5 || val2 == 1) {
				alive = 0;
			} else {
				map_setcell(src->m,ux,uy,5);
				clif_changemapcell(src->m,ux,uy,5,0);
			}
		}

		if(alive) {
			nullpo_retr(NULL, unit = skill_initunit(group,i,ux,uy));
			unit->val1  = val1;
			unit->val2  = val2;
			unit->limit = limit;
			unit->range = range;
			if(range == 0)
				map_foreachinarea(skill_unit_effect,unit->bl.m,
					unit->bl.x,unit->bl.y,unit->bl.x,unit->bl.y,
					(BL_PC|BL_MOB|BL_MERC),&unit->bl,gettick(),1);
		}
	}

	if(on_flag && group)
	{
		switch(skillid) {
		case MG_FIREWALL:
			group->limit = group->limit*150/100;
			break;
		case PF_FOGWALL:
			group->limit *= 2;
			break;
		}
	}

	return group;
}

/*==========================================
 * �X�L�����j�b�g�̔����C�x���g(�ʒu����)
 *------------------------------------------
 */
static int skill_unit_onplace(struct skill_unit *src,struct block_list *bl,unsigned int tick)
{
	struct skill_unit_group *sg;
	struct skill_unit *unit2;
	struct status_change *sc;
	int type;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	if( bl->prev == NULL || !src->alive || unit_isdead(bl) )
		return 0;

	if( bl->type == BL_PC && ((struct map_session_data*)bl)->invincible_timer != -1 )
		return 0; // ���G�^�C�}�[��

	nullpo_retr(0, sg = src->group);

	if(battle_check_target(&src->bl,bl,sg->target_flag) <= 0)
		return 0;

	// �Ώۂ�LP��ɋ���ꍇ�͖���
	if (map_find_skill_unit_oncell(bl,bl->x,bl->y,SA_LANDPROTECTOR,NULL) &&
	    (sg->unit_id < UNT_DISSONANCE || sg->unit_id > UNT_SERVICEFORYOU)) // �Ƒt�X�L����LP��ł��L��
		return 0;

	type = SkillStatusChangeTable[sg->skill_id];
	if(type < 0)
		return 0;

	sc = status_get_sc(bl);

	switch (sg->unit_id) {
	case UNT_PNEUMA:	/* �j���[�} */
	case UNT_SAFETYWALL:	/* �Z�C�t�e�B�E�H�[�� */
		if(!sc || sc->data[type].timer==-1)
			status_change_start(bl,type,sg->skill_lv,src->bl.id,0,0,sg->limit,0);
		break;
	case UNT_QUAGMIRE:	/* �N�@�O�}�C�A */
		if (bl->type==BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage)
			break;
		if (sc && sc->data[type].timer!=-1)
			break;
		status_change_start(bl,type,sg->skill_lv,src->bl.id,0,0,
				skill_get_time2(sg->skill_id,sg->skill_lv),0);
		break;
	case UNT_VOLCANO:	/* �{���P�[�m */
	case UNT_DELUGE:	/* �f�����[�W */
	case UNT_VIOLENTGALE:	/* �o�C�I�����g�Q�C�� */
		if (sc && sc->data[type].timer!=-1) {
			unit2 = map_id2su(sc->data[type].val2);
			if (unit2 && unit2->group && (unit2==src || DIFF_TICK(sg->tick,unit2->group->tick)<=0))
				break;
		}
		status_change_start(bl,type,sg->skill_lv,src->bl.id,0,0,
				skill_get_time2(sg->skill_id,sg->skill_lv),0);
		break;
	case UNT_SUITON:	/* ���� */
		if (sc && sc->data[type].timer!=-1) {
			unit2 = map_id2su(sc->data[type].val2);
			if (unit2 && unit2->group && (unit2==src || DIFF_TICK(sg->tick,unit2->group->tick)<=0))
				break;
		}
		if(status_get_class(bl) == PC_CLASS_NJ || battle_check_target(&src->bl,bl,BCT_ENEMY)<=0) {
			status_change_start(bl,type,sg->skill_lv,src->bl.id,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),0);
		} else {
			int penalty = - ((sg->skill_lv + 1) / 3 * 26 + 4) / 10;
			status_change_start(bl,type,sg->skill_lv,src->bl.id,penalty,1,skill_get_time2(sg->skill_id,sg->skill_lv),0);
		}
		break;
	case UNT_LULLABY:		/* �q��S */
	case UNT_RICHMANKIM:		/* �j�����h�̉� */
	case UNT_ETERNALCHAOS:		/* �i���̍��� */
	case UNT_DRUMBATTLEFIELD:	/* �푾�ۂ̋��� */
	case UNT_RINGNIBELUNGEN:	/* �j�[�x�����O�̎w�� */
	case UNT_ROKISWEIL:		/* ���L�̋��� */
	case UNT_INTOABYSS:		/* �[���̒��� */
	case UNT_SIEGFRIED:		/* �s���g�̃W�[�N�t���[�h */
	case UNT_DISSONANCE:		/* �s���a�� */
	case UNT_UGLYDANCE:		/* ��������ȃ_���X */
		// �_���X���ʂ������ɂ�����H
		if (sg->src_id==bl->id && battle_config.allow_me_concert_effect==0)
			break;

		if (sg->unit_id==0xa3) {
			// ���L�������ɓK�p���Ȃ�
			if(sg->src_id==bl->id && battle_config.allow_me_concert_effect==1 && battle_config.allow_me_rokisweil==1)
				break;
			// ���L�̓{�X����
			if(status_get_mode(bl)&0x20)
				break;
		}
		// �i���̍��ׂ̓{�X����
		if (sg->unit_id==0xa0) {
			if(status_get_mode(bl)&0x20)
				break;
		}
		if (sc && sc->data[type].timer!=-1) {
			unit2 = map_id2su(sc->data[type].val4);
			if (unit2 && unit2->group && (unit2==src || DIFF_TICK(sg->tick,unit2->group->tick)<=0))
				break;
		}
		status_change_start(bl,type,sg->skill_lv,sg->val1,sg->val2,
				src->bl.id,skill_get_time2(sg->skill_id,sg->skill_lv),0);
		break;
	case UNT_WHISTLE:		/* ���J */
	case UNT_ASSASSINCROSS:		/* �[�z�̃A�T�V���N���X */
	case UNT_POEMBRAGI:		/* �u���M�̎� */
	case UNT_APPLEIDUN:		/* �C�h�D���̗ь� */
	case UNT_HUMMING:		/* �n�~���O */
	case UNT_DONTFORGETME:		/* ����Y��Ȃ��Łc */
	case UNT_FORTUNEKISS:		/* �K�^�̃L�X */
	case UNT_SERVICEFORYOU:		/* �T�[�r�X�t�H�[���[ */
		// �_���X���ʂ������ɂ�����H
		if(sg->src_id==bl->id && (!sc || sc->data[SC_BARDDANCER].timer==-1)
							&& battle_config.allow_me_dance_effect==0)
			break;
		if(sc && sc->data[type].timer!=-1) {
			unit2 = map_id2su(sc->data[type].val4);
			if (unit2 && unit2->group && (unit2==src || DIFF_TICK(sg->tick,unit2->group->tick)<=0))
				break;
		}
		status_change_start(bl,type,sg->skill_lv,sg->val1,sg->val2,
				src->bl.id,skill_get_time2(sg->skill_id,sg->skill_lv),0);
		break;
	case UNT_CALLFAMILY:				/* ���Ȃ��Ɉ������� or �}�}�A�p�p�A���� or �V��A��������Ⴂ */
		break;
	case UNT_FOGWALL:				/* �t�H�O�E�H�[�� */
		if(status_check_no_magic_damage(bl))
			break;
		// ���̒�
		if(map[bl->m].flag.normal) {	// �ʏ�}�b�v
			if(bl->type==BL_PC) {
				status_change_start(bl,SC_FOGWALL,sg->skill_id,sg->skill_lv,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),0);
			} else {
				status_change_start(bl,SC_FOGWALLPENALTY,sg->skill_id,sg->skill_lv,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),0);
			}
		} else if(status_get_party_id(&src->bl)>0) {	// ����ȊO��PT��
			if(battle_check_target(bl,&src->bl,BCT_ENEMY)<=0) {
				status_change_start(bl,SC_FOGWALL,sg->skill_id,sg->skill_lv,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),0);
			} else {
				status_change_start(bl,SC_FOGWALLPENALTY,sg->skill_id,sg->skill_lv,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),0);
			}
		} else {	// ����ȊO�Ń\����
			if(bl->id==sg->src_id) {
				status_change_start(bl,SC_FOGWALL,sg->skill_id,sg->skill_lv,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),0);
			} else {
				status_change_start(bl,SC_FOGWALLPENALTY,sg->skill_id,sg->skill_lv,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),0);
			}
		}
		break;
	case UNT_MOONLIT:		/* ������̉��� */
		break;
	case UNT_GRAVITATION:		/* �O���r�e�[�V�����t�B�[���h */
		if (battle_check_target(&src->bl,bl,BCT_ENEMY)>0)	// �G�Ώ�
		{
			status_change_start(bl,SC_GRAVITATION,sg->skill_lv,0,0,0,9000,0);
		}
		break;
	case UNT_HERMODE:		/* �w�����[�h�̏� */
		{
			int same_flag = 0;

			// �����͏��O
			if(sg->src_id==bl->id)
				break;

			// �M���h�ƃp�[�e�B�[�������Ȃ�x���X�L�������Ώ�
			if( status_get_guild_id(&src->bl)==status_get_guild_id(bl) ||
			    status_get_party_id(&src->bl)==status_get_guild_id(bl) )
			{
				same_flag = 1;
				// �\�E�������J�[�ȊO�͎x���X�L������
				if(status_get_class(bl) != PC_CLASS_SL)
					status_change_release(bl,0x20);
			}

			if(sc && sc->data[type].timer!=-1 && same_flag==0) {
				unit2 = map_id2su(sc->data[type].val4);
				if (unit2 && unit2->group && (unit2==src || DIFF_TICK(sg->tick,unit2->group->tick)<=0))
					break;
			}
			status_change_start(bl,type,same_flag,sg->val1,sg->val2,
					src->bl.id,skill_get_time2(sg->skill_id,sg->skill_lv),0);
		}
		break;
/*	default:
		if(battle_config.error_log)
			printf("skill_unit_onplace: Unknown skill unit id=%d block=%d\n",sg->unit_id,bl->id);
		break;*/
	}

	return 0;
}

/*==========================================
 * �X�L�����j�b�g�̔����C�x���g(�^�C�}�[����)
 *------------------------------------------
 */
static int skill_unit_onplace_timer(struct skill_unit *src,struct block_list *bl,unsigned int tick)
{
	struct skill_unit_group *sg;
	struct block_list *ss;
	struct status_change *sc;
	struct unit_data *ud;
	struct linkdb_node **node;
	int tickset_id, diff = 0;
	unsigned int tickset_tick;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	if(!src->alive || unit_isdead(bl))
		return 0;

	nullpo_retr(0, sg = src->group);
	nullpo_retr(0, ss = map_id2bl(sg->src_id));
	nullpo_retr(0, ud = unit_bl2ud(bl));

	// �Ώۂ�LP��ɋ���ꍇ�͖���
	if(map_find_skill_unit_oncell(bl,bl->x,bl->y,SA_LANDPROTECTOR,NULL))
		return 0;

	// �O�ɉe�����󂯂Ă���interval�̊Ԃ͉e�����󂯂Ȃ�
	if(skill_get_unit_flag(sg->skill_id,sg->skill_lv)&UF_NOOVERLAP) {
		tickset_id = sg->skill_id;
		node       = &ud->skilltickset;
	} else {
		tickset_id = bl->id;
		node       = &sg->tickset;
	}
	tickset_tick = (unsigned int)linkdb_search( node, (void*)tickset_id );
	if(tickset_tick == 0)
		tickset_tick = tick;

	diff = DIFF_TICK(tick, tickset_tick);
	if(sg->skill_id == PR_SANCTUARY) {
		diff += 500; // �V�K�ɉ񕜂������j�b�g�����J�E���g���邽�߂̎d�|��
	}
	if(diff < 0)
		return 0;

	tickset_tick = tick + sg->interval;

	// GX�͏d�Ȃ��Ă�����3HIT���Ȃ�
	if(sg->skill_id == CR_GRANDCROSS && !battle_config.gx_allhit) {
		int count = map_count_oncell(bl->m,bl->x,bl->y,BL_PC|BL_MOB);
		if(count > 0)
			tickset_tick += sg->interval * (count-1);
	}
	linkdb_replace( node, (void*)tickset_id, (void*)tickset_tick );

	switch (sg->unit_id) {
	case UNT_WARP_ACTIVE:	/* ���[�v�|�[�^��(������) */
		if (bl->type == BL_PC) {
			struct map_session_data *sd = (struct map_session_data *)bl;
			if(sd) {
				if ((sd->state.warp_waiting || strcmp(map[bl->m].name,sg->valstr) == 0) &&
				    src->bl.m == bl->m &&
				    src->bl.x == bl->x &&
				    src->bl.y == bl->y &&
				    src->bl.x == sd->ud.to_x &&
				    src->bl.y == sd->ud.to_y)
				{
					sd->state.warp_waiting = 0;
					if (battle_config.chat_warpportal || !sd->chatID){
						char mapname[24];
						int  x = sg->val2>>16;
						int  y = sg->val2&0xffff;
						strncpy(mapname,sg->valstr,24);
						if( sg->src_id == bl->id ||
						    (strcmp(map[src->bl.m].name,sg->valstr) == 0 && src->bl.x == (sg->val2>>16) && src->bl.y == (sg->val2&0xffff)) ||
						    (--sg->val1) <= 0 )
						{
							skill_delunitgroup(sg);
						}
						pc_setpos(sd,mapname,x,y,3);
					}
				} else {
				 	sd->state.warp_waiting = 1;
				}
			}
		} else if(bl->type == BL_MOB && battle_config.mob_warpportal) {
			int m = map_mapname2mapid(sg->valstr);
			mob_warp((struct mob_data *)bl,m,sg->val2>>16,sg->val2&0xffff,3);
		}
		break;
	case UNT_SANCTUARY:	/* �T���N�`���A�� */
		{
			int race = status_get_race(bl);

			sc = status_get_sc(bl);
			if (battle_check_undead(race,status_get_elem_type(bl)) || race == RCT_DEMON) {
				if (bl->type == BL_PC) {
					if(!map[bl->m].flag.pvp && !map[bl->m].flag.gvg)
						break;
				}
				if (battle_skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0))
					sg->val1 = sg->val1-2;	// �`���b�g�L�����Z���ɑΉ�
			} else {
				int heal;
				if (status_get_hp(bl) >= status_get_max_hp(bl))
					break;
				heal = sg->val2;
				if(sc && sc->data[SC_CRITICALWOUND].timer != -1)
					heal = heal * (100 - sc->data[SC_CRITICALWOUND].val1 * 10) / 100;
				if(bl->type == BL_PC && ((struct map_session_data *)bl)->special_state.no_magic_damage)
					heal = 0;	/* ����峃J�[�h�i�q�[���ʂO�j */
				if(sc && sc->data[SC_BERSERK].timer != -1) /* �o�[�T�[�N���̓q�[���O */
					heal = 0;
				clif_skill_nodamage(&src->bl,bl,AL_HEAL,heal,1);
				battle_heal(NULL,bl,heal,0,0);
				if (diff >= 500)
					sg->val1--;	// �V�K�ɓ��������j�b�g�����J�E���g
			}
			if (sg->val1 <= 0)
				skill_delunitgroup(sg);
		}
		break;
	case UNT_MAGNUS:	/* �}�O�k�X�G�N�\�V�Y�� */
		{
			int race = status_get_race(bl);
			if (!battle_check_undead(race,status_get_elem_type(bl)) && race != RCT_DEMON)
				return 0;
			battle_skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
			src->val2++;
		}
		break;
	case UNT_FIREWALL:	/* �t�@�C���[�E�H�[�� */
		do {
			battle_skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		} while((--src->val2) > 0 && !unit_isdead(bl) && bl->x == src->bl.x && bl->y == src->bl.y);
		if (src->val2 <= 0)
			skill_delunit(src);
		break;
	case UNT_ATTACK_SKILLS:
		battle_skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		break;
	case UNT_FIREPILLAR_WAITING:	/* �t�@�C�A�[�s���[(�����O) */
		skill_unitsetting(ss,sg->skill_id,sg->skill_lv,src->bl.x,src->bl.y,1);
		skill_delunit(src);
		break;
	case UNT_FIREPILLAR_ACTIVE:	/* �t�@�C�A�[�s���[(������) */
		{
			int i = src->range;
			if(sg->skill_lv>5)
				i += 2;
			map_foreachinarea(battle_skill_attack_area,src->bl.m,src->bl.x-i,src->bl.y-i,src->bl.x+i,src->bl.y+i,
				(BL_CHAR|BL_SKILL),BF_MAGIC,ss,&src->bl,sg->skill_id,sg->skill_lv,tick,0,BCT_ENEMY);
		}
		break;
	case UNT_SKIDTRAP:	/* �X�L�b�h�g���b�v */
		{
			// 㩐ݒu���̃L�����̍��W�ƃ^�[�Q�b�g�̈ʒu�֌W�Ŕ�ԕ��������߂�
			int xs = sg->val1, ys = sg->val2, dir;
			int count = skill_get_blewcount(sg->skill_id,sg->skill_lv);
			if( (bl->x == src->bl.x && bl->y == src->bl.y) || (bl->x == xs && bl->y == ys) ) {
				dir = 6;	// 㩂̒��ォ�ݒu���̈ʒu�ɋ���Ȃ�^���ɔ��
			} else {
				dir = map_calc_dir(bl,xs,ys);
				if(dir == 0)
					dir = 8;
			}
			skill_blown(&src->bl,bl,count|(dir<<20)|SAB_NODAMAGE|SAB_NOPATHSTOP);
			sg->unit_id = UNT_USED_TRAPS;
			clif_changelook(&src->bl,LOOK_BASE,sg->unit_id);
			sg->limit=DIFF_TICK(tick,sg->tick)+1500;
		}
		break;
	case UNT_LANDMINE:	/* �����h�}�C�� */
		battle_skill_attack(BF_MISC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		sg->unit_id = UNT_USED_TRAPS;
		clif_changelook(&src->bl,LOOK_BASE,UNT_FIREPILLAR_ACTIVE);
		sg->limit = DIFF_TICK(tick,sg->tick)+1500;
		break;

	case UNT_BLASTMINE:	/* �u���X�g�}�C�� */
	case UNT_SHOCKWAVE:	/* �V���b�N�E�F�[�u�g���b�v */
	case UNT_SANDMAN:	/* �T���h�}�� */
	case UNT_FLASHER:	/* �t���b�V���[ */
	case UNT_FREEZINGTRAP:	/* �t���[�W���O�g���b�v */
	case UNT_CLAYMORETRAP:	/* �N���C���A�[�g���b�v */
		{
			int splash_count = 0;
			int i = src->range;

			// �T���h�}���ƃN���C���A�͌��ʔ͈͂�1�Z���L����
			if(sg->unit_id == UNT_SANDMAN || sg->unit_id == UNT_CLAYMORETRAP) {
				i++;
			}
			map_foreachinarea(skill_count_target,src->bl.m,
						src->bl.x-i,src->bl.y-i,
						src->bl.x+i,src->bl.y+i,
						(BL_CHAR|BL_SKILL),src,&splash_count);
			map_foreachinarea(skill_trap_splash,src->bl.m,
						src->bl.x-i,src->bl.y-i,
						src->bl.x+i,src->bl.y+i,
						(BL_CHAR|BL_SKILL),src,tick,splash_count);
			sg->unit_id = UNT_USED_TRAPS;
			clif_changelook(&src->bl,LOOK_BASE,sg->unit_id);
			sg->limit=DIFF_TICK(tick,sg->tick)+1500;
		}
		break;

	case UNT_ANKLESNARE:	/* �A���N���X�l�A */
		sc = status_get_sc(bl);
		if (sg->val2 == 0 && (!sc || sc->data[SC_ANKLE].timer == -1)) {
			int sec = skill_get_time2(sg->skill_id,sg->skill_lv) - status_get_agi(bl)*100;
			if(status_get_mode(bl)&0x20)
				sec /= 5;
			// �Œ�S�����ԕ⏞�i����eA�̂��̂��Ƃ肠�����̗p�j
			if(sec < 3000 + 30 * sg->skill_lv)
				sec = 3000 + 30 * sg->skill_lv;
			status_change_start(bl,SC_ANKLE,sg->skill_lv,(int)sg,0,0,sec,0);
			// �{���Ȃ�{�X�����Ȃ�z���񂹂��Ȃ����Askill_delunitgroup() ���̏����Ə�肭�܂荇�����t���Ȃ��̂ŕۗ�
			unit_movepos(bl, src->bl.x, src->bl.y, 0);
			clif_01ac(&src->bl);
			sg->limit    = DIFF_TICK(tick,sg->tick) + sec;
			sg->val2     = bl->id;
			sg->interval = -1;
			src->range   = 0;
		}
		break;
	case UNT_VENOMDUST:	/* �x�m���_�X�g */
		{
			int type = SkillStatusChangeTable[sg->skill_id];
			if(type < 0)
				break;
			sc = status_get_sc(bl);
			if (sc && sc->data[type].timer != -1)
				break;
			status_change_start(bl,type,sg->skill_lv,src->bl.id,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),0);
		}
		break;
	case UNT_DEMONSTRATION:	/* �f�����X�g���[�V���� */
		battle_skill_attack(BF_WEAPON,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		if(bl->type == BL_PC && atn_rand()%100 < sg->skill_lv)
			pc_break_equip((struct map_session_data *)bl, EQP_WEAPON);
		break;
	case UNT_TALKIEBOX:				/* �g�[�L�[�{�b�N�X */
		if(sg->src_id == bl->id) // ����������ł��������Ȃ�
			break;
		if(sg->val2 == 0) {
			clif_talkiebox(&src->bl,sg->valstr);
			sg->unit_id = UNT_USED_TRAPS;
			clif_changelook(&src->bl,LOOK_BASE,sg->unit_id);
			sg->limit = DIFF_TICK(tick,sg->tick) + 5000;
			sg->val2  = -1; // ����
		}
		break;
	case UNT_GOSPEL:	/* �S�X�y�� */
		{
			struct map_session_data *sd = NULL;
			if(bl->type == BL_PC)
				sd = (struct map_session_data *)bl;
			if (sd && sg->src_id == bl->id) {
				int hp = (sg->skill_lv <= 5) ? 30 : 45;
				int sp = (sg->skill_lv <= 5) ? 20 : 35;
				if(sd->status.hp <= hp || sd->status.sp <= sp) {
					status_change_end(bl,SC_GOSPEL,-1);
					break;
				}
				pc_heal(sd,-hp,-sp);
				break;
			}
			if (sd && sd->special_state.no_magic_damage)
				break;
			if (atn_rand()%100 >= 50 + sg->skill_lv * 5)
				break;
			if (battle_check_target(&src->bl,bl,BCT_PARTY) > 0) {	// ����(PT)�Ώ�
				int type = 0;
				switch(atn_rand()%13) {
				case 0:		// HP����(1000�`9999�H)
					battle_heal(NULL,bl,1000+atn_rand()%9000,0,0);
					break;
				case 1:		// MHP��100%����(��������60�b)
					status_change_start(bl,SC_INCMHP2,100,0,0,0,60000,0);
					type = 0x17;
					break;
				case 2:		// MSP��100%����(��������60�b)
					status_change_start(bl,SC_INCMSP2,100,0,0,0,60000,0);
					type = 0x18;
					break;
				case 3:		// �S�ẴX�e�[�^�X+20(��������60�b)
					status_change_start(bl,SC_INCALLSTATUS,20,0,0,0,60000,0);
					type = 0x19;
					break;
				case 4:		// �u���b�V���OLv10���ʕt�^
					status_change_start(bl,SC_BLESSING,10,0,0,0,skill_get_time(AL_BLESSING,10),0);
					break;
				case 5:		// ���x����Lv10���ʕt�^
					status_change_start(bl,SC_INCREASEAGI,10,0,0,0,skill_get_time(AL_INCAGI,10),0);
					break;
				case 6:		// ����ɐ��������ʕt�^
					status_change_start(bl,SC_ASPERSIO,sg->skill_lv,0,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),0);
					type = 0x1c;
					break;
				case 7:		// �Z�ɐ��������ʕt�^
					status_change_start(bl,SC_BENEDICTIO,sg->skill_lv,0,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),0);
					type = 0x1d;
					break;
				case 8:		// ATK��100%����
					status_change_start(bl,SC_INCATK2,100,0,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),0);
					type = 0x1f;
					break;
				case 9:		// HIT, FLEE��+50(��������60�b)
					status_change_start(bl,SC_INCHIT,50,0,0,0,60000,0);
					status_change_start(bl,SC_INCFLEE,50,0,0,0,60000,0);
					type = 0x20;
					break;
				case 10:	// �S�Ă̏�Ԉُ������
					status_change_release(bl,0x08);
					type = 0x15;
					break;
				case 11:	// �S��Ԉُ�̑ϐ�(��������60�b)
					status_change_start(bl,SC_STATUS_UNCHANGE,0,0,0,0,60000,0);
					type = 0x16;
					break;
				case 12:	// �h��͑���(��������10�b)
					status_change_start(bl,SC_INCDAMAGE,-50,0,0,0,10000,0);
					type = 0x1e;
					break;
				}
				if(type > 0 && sd)
					clif_gospel_message(sd,type);
			}
			else if (battle_check_target(&src->bl,bl,BCT_ENEMY) > 0 && !(status_get_mode(bl)&0x20)) {	// �G�ΏۂŃ{�X�ȊO
				switch(atn_rand()%8) {
				case 0:		// �����_���_���[�W(1000�`9999�H)
					battle_skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
					break;
				case 1:		// �􂢌��ʕt�^
					status_change_start(bl,SC_CURSE,sg->skill_lv,0,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),0);
					break;
				case 2:		// �Í����ʕt�^
					status_change_start(bl,SC_BLIND,sg->skill_lv,0,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),0);
					break;
				case 3:		// �Ō��ʕt�^
					status_change_start(bl,SC_POISON,sg->skill_lv,0,0,0,skill_get_time2(sg->skill_id,sg->skill_lv),0);
					break;
				case 4:		// �v���{�b�NLv10���ʕt�^
					status_change_start(bl,SC_PROVOKE,10,0,0,0,skill_get_time(SM_PROVOKE,10),0);
					break;
				case 5:		// ATK��0�Ɍ���(��������20�b)
					status_change_start(bl,SC_INCATK2,-100,0,0,0,20000,0);
					break;
				case 6:		// FLEE��0�Ɍ���(��������20�b)
					status_change_start(bl,SC_INCFLEE2,-100,0,0,0,20000,0);
					break;
				case 7:		// HIT��0�Ɍ���(��������50�b)
					status_change_start(bl,SC_INCHIT2,-100,0,0,0,50000,0);
					break;
				}
			}
		}
		break;
	case UNT_BASILICA:	/* �o�W���J */
		if (sg->src_id == bl->id)
			break;
	   	if ( battle_check_target(&src->bl,bl,BCT_ENEMY) > 0 && !(status_get_mode(bl)&0x20) )
			skill_blown(&src->bl,bl,SAB_NODAMAGE|1);
		if (battle_check_target(&src->bl,bl,BCT_NOENEMY)>0) {
			int type = SkillStatusChangeTable[sg->skill_id];
			if(type < 0)
				break;
			status_change_start(bl,type,sg->skill_lv,sg->val1,sg->val2,src->bl.id,sg->interval+100,0);
		}
		break;
	case UNT_WARM:		/* ������ */
		{
			const int x = bl->x, y = bl->y;
			int hit   = 0;
			int count = skill_get_blewcount(sg->skill_id,sg->skill_lv);

			do {
				if(battle_skill_attack(BF_WEAPON,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0)) {
					if(bl->type != BL_PC)
						skill_blown(&src->bl,bl,count|SAB_REVERSEBLOW|SAB_NOPATHSTOP);
				}
			} while(sg->alive_count > 0 && !unit_isdead(bl) && x == bl->x && y == bl->y &&
				sg->interval > 0 && ++hit < SKILLUNITTIMER_INVERVAL / sg->interval);
		}
		break;
	case UNT_SPIDERWEB:	/* �X�p�C�_�[�E�F�b�u */
		sc = status_get_sc(bl);
		if((!sc || sc->data[SC_SPIDERWEB].timer == -1) && sg->val2 == 0) {
			skill_additional_effect(ss,bl,sg->skill_id,sg->skill_lv,BF_MISC,tick);
			unit_movepos(bl, src->bl.x, src->bl.y, 0);
			sg->limit    = DIFF_TICK(tick,sg->tick) + skill_get_time2(sg->skill_id,sg->skill_lv);
			sg->val2     = bl->id;
			sg->interval = -1;
			src->range   = 0;
		}
		break;
	case UNT_MOONLIT: 	/* ������̉��� */
		sc = status_get_sc(bl);
		if(bl->type != BL_MOB && bl->type != BL_PC)
			break;
		if (sg->src_id == bl->id)
			break;
		// ����
		if(sc && sc->data[SC_DANCING].timer != -1 && sg->src_id == sc->data[SC_DANCING].val4)
			break;
		if(!(status_get_mode(bl)&0x20))
		{
			int d = unit_distance2(&src->bl,bl);
			int range = skill_get_unit_range(CG_MOONLIT);
			int count = (d < range)? range-d+2: 1;
			skill_blown(&src->bl,bl,count|SAB_NODAMAGE);
		}
		break;
	case UNT_GRAVITATION:
		if (battle_check_target(&src->bl,bl,BCT_ENEMY) > 0)		// �G�Ώ�
		{
			battle_skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		//	unit_fixdamage(&src->bl,bl,0, 0, 0,sg->skill_lv*200+200,1, 4, 0);
		}
		break;
	case UNT_DESPERADO:	/* �f�X�y���[�h */
	case UNT_TATAMIGAESHI:	/* ���Ԃ� */
		battle_skill_attack(BF_WEAPON,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0x0500);
		break;
	case UNT_KAENSIN:	/* �Ή��w */
		battle_skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		if(--src->val1 <= 0)
			skill_delunit(src);
		break;
	case UNT_GROUNDDRIFT_WIND:	/* �O���E���h�h���t�g */
	case UNT_GROUNDDRIFT_DARK:
	case UNT_GROUNDDRIFT_POISON:
	case UNT_GROUNDDRIFT_WATER:
	case UNT_GROUNDDRIFT_FIRE:
		battle_skill_attack(BF_MISC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
		sg->unit_id = UNT_USED_TRAPS;
		clif_changelook(&src->bl,LOOK_BASE,UNT_FIREPILLAR_ACTIVE);
		sg->limit = DIFF_TICK(tick,sg->tick) + 1500;
		break;
	case UNT_EVILLAND:	/* �C�r�������h */
		{
			int race = status_get_race(bl);

			if(battle_check_undead(race,status_get_elem_type(bl)) || race == RCT_DEMON) {
				int heal = sg->val1;
				if(status_get_hp(bl) >= status_get_max_hp(bl))
					break;
				clif_skill_nodamage(&src->bl,bl,AL_HEAL,heal,1);
				battle_heal(NULL,bl,heal,0,0);
			} else if(bl->type == BL_PC) {
				battle_skill_attack(BF_MAGIC,ss,&src->bl,bl,sg->skill_id,sg->skill_lv,tick,0);
			}
		}
		break;
	}

	if(bl->type == BL_MOB && ss != bl)	/* �X�L���g�p������MOB�X�L�� */
	{
		struct mob_data *md = (struct mob_data *)bl;
		int target = md->target_id;
		if(battle_config.mob_changetarget_byskill == 1 || target == 0)
		{
			if(ss->type == BL_PC || ss->type == BL_HOM || ss->type == BL_MERC)
				md->target_id = ss->id;
		}
		mobskill_use(md,tick,MSC_SKILLUSED|(sg->skill_id<<16));
		md->target_id = target;
	}
	return 0;
}

/*==========================================
 * �X�L�����j�b�g���痣�E
 *------------------------------------------
 */
static int skill_unit_onout(struct skill_unit *src,struct block_list *bl,unsigned int tick)
{
	struct skill_unit_group *sg;
	struct status_change *sc;
	int type;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);
	nullpo_retr(0, sg=src->group);

	if (bl->prev==NULL || !src->alive || unit_isdead(bl))
		return 0;

	if( bl->type == BL_PC && ((struct map_session_data*)bl)->invincible_timer != -1 )
		return 0; // ���G�^�C�}�[��

	switch(sg->unit_id){
	case UNT_SAFETYWALL:	/* �Z�C�t�e�B�E�H�[�� */
	case UNT_PNEUMA:	/* �j���[�} */
	case UNT_QUAGMIRE:	/* �N�@�O�}�C�A */
	case UNT_VOLCANO:	/* �{���P�[�m */
	case UNT_DELUGE:	/* �f�����[�W */
	case UNT_VIOLENTGALE:	/* �o�C�I�����g�Q�C�� */
		sc = status_get_sc(bl);
		type = SkillStatusChangeTable[sg->skill_id];
		if( type == -1 ) break;
		if (type==SC_QUAGMIRE && bl->type==BL_MOB)
			break;
		if (sc && sc->data[type].timer!=-1 && sc->data[type].val2==src->bl.id) {
			status_change_end(bl,type,-1);
		}
		break;
	case UNT_SUITON:	/* ���� */
		sc = status_get_sc(bl);
		type = SkillStatusChangeTable[sg->skill_id];
		if( type == -1 ) break;
		if (sc && sc->data[type].timer!=-1 && sc->data[type].val2==src->bl.id) {
			status_change_end(bl,type,-1);
		}
		break;
	case UNT_ANKLESNARE:	/* �A���N���X�l�A */
		{
			struct block_list *target=map_id2bl(sg->val2);
			if( target && target==bl ){
				status_change_end(bl,SC_ANKLE,-1);
				sg->limit=DIFF_TICK(tick,sg->tick)+1000;
			}
		}
		break;
	case UNT_LULLABY:	/* �q��S */
	case UNT_RICHMANKIM:	/* �j�����h�̉� */
	case UNT_ETERNALCHAOS:	/* �i���̍��� */
	case UNT_DRUMBATTLEFIELD:	/* �푾�ۂ̋��� */
	case UNT_RINGNIBELUNGEN:	/* �j�[�x�����O�̎w�� */
	case UNT_ROKISWEIL:	/* ���L�̋��� */
	case UNT_INTOABYSS:	/* �[���̒��� */
	case UNT_SIEGFRIED:	/* �s���g�̃W�[�N�t���[�h */
	case UNT_DISSONANCE:	/* �s���a�� */
	case UNT_WHISTLE:	/* ���J */
	case UNT_ASSASSINCROSS:	/* �[�z�̃A�T�V���N���X */
	case UNT_POEMBRAGI:	/* �u���M�̎� */
	case UNT_APPLEIDUN:	/* �C�h�D���̗ь� */
	case UNT_UGLYDANCE:	/* ��������ȃ_���X */
	case UNT_HUMMING:	/* �n�~���O */
	case UNT_FORTUNEKISS:	/* �K�^�̃L�X */
	case UNT_SERVICEFORYOU:	/* �T�[�r�X�t�H�[���[ */
	case UNT_DONTFORGETME:	/* ����Y��Ȃ��Łc */
	case UNT_BASILICA:	/* �o�W���J */
		sc = status_get_sc(bl);
		type = SkillStatusChangeTable[sg->skill_id];
		if( type == -1 ) break;
		if (sc && sc->data[type].timer!=-1 && sc->data[type].val4==src->bl.id) {
			status_change_end(bl,type,-1);
		}
		break;
	case UNT_FOGWALL:	/* �t�H�O�E�H�[�� */
		sc = status_get_sc(bl);
		if(sc){
			if(sc->data[SC_FOGWALL].timer!=-1)
				status_change_end(bl,SC_FOGWALL,-1);
			// PC�Ȃ���ʏ�����
			if(bl->type==BL_PC && sc->data[SC_FOGWALLPENALTY].timer!=-1)
				status_change_end(bl,SC_FOGWALLPENALTY,-1);
		}
		break;
	case UNT_MOONLIT: 	/* ������̉��� */
		break;
	case UNT_SPIDERWEB:	/* �X�p�C�_�[�E�F�u */
		{
			struct block_list *target = map_id2bl(sg->val2);
			if (target && target==bl)
				status_change_end(bl,SC_SPIDERWEB,-1);
			sg->limit = DIFF_TICK(tick,sg->tick)+1000;
			break;
		}

	case UNT_GRAVITATION:	/* �O���r�e�[�V�����t�B�[���h */
		sc = status_get_sc(bl);
		if (sc && sc->data[SC_GRAVITATION].timer!=-1)
			status_change_end(bl,SC_GRAVITATION,-1);
		break;
	case UNT_HERMODE:	/* �w�����[�h�̏� */
		sc = status_get_sc(bl);
		if (sc && sc->data[SC_HERMODE].timer!=-1)
			status_change_end(bl,SC_HERMODE,-1);
		break;
/*	default:
		if(battle_config.error_log)
			printf("skill_unit_onout: Unknown skill unit id=%d block=%d\n",sg->unit_id,bl->id);
		break;*/
	}
	return 0;
}

/*==========================================
 * �X�L�����j�b�g���ʔ���/���E����(foreachinarea)
 *------------------------------------------
 */
static int skill_unit_effect(struct block_list *bl,va_list ap)
{
	struct skill_unit *unit;
	struct skill_unit_group *group;
	int flag;
	unsigned int tick;
	static int called = 0;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, unit = va_arg(ap,struct skill_unit*));
	nullpo_retr(0, group = unit->group);

	tick = va_arg(ap,unsigned int);
	flag = va_arg(ap,unsigned int);

	if(!(bl->type & (BL_PC | BL_MOB | BL_MERC)))
		return 0;

	if(!unit->alive)
		return 0;

	if(flag) {
		skill_unit_onplace(unit,bl,tick);
	} else {
		skill_unit_onout(unit,bl,tick);
		unit = map_find_skill_unit_oncell(bl,bl->x,bl->y,group->skill_id,unit);
		if(unit && called == 0) {
			called = 1;
			skill_unit_onplace(unit,bl,tick);
			called = 0;
		}
	}

	return 0;
}

/*==========================================
 * �X�L�����j�b�g�̌��E�C�x���g
 *------------------------------------------
 */
static int skill_unit_onlimit(struct skill_unit *src,unsigned int tick)
{
	struct skill_unit_group *sg;

	nullpo_retr(0, src);
	nullpo_retr(0, sg=src->group);

	switch(sg->unit_id){
	case UNT_ICEWALL:	/* �A�C�X�E�H�[�� */
		map_setcell(src->bl.m,src->bl.x,src->bl.y,src->val2);
		clif_changemapcell(src->bl.m,src->bl.x,src->bl.y,src->val2,1);
		break;
	case UNT_CALLFAMILY:
		{
			struct map_session_data *sd = map_id2sd(sg->src_id);
			if(sd == NULL)
				break;
			if(sg->skill_id == WE_CALLPARTNER) {		/* ���Ȃ��Ɉ������� */
				if(sd->status.partner_id)
					intif_charmovereq2(sd,map_charid2nick(sd->status.partner_id),map[src->bl.m].name,src->bl.x,src->bl.y,0);
			}
			else if(sg->skill_id == WE_CALLPARENT) {	/* �}�}�A�p�p�A���� */
				if(sd->status.parent_id[0] && sd->status.parent_id[1]) {
					intif_charmovereq2(sd,map_charid2nick(sd->status.parent_id[0]),map[src->bl.m].name,src->bl.x,src->bl.y,2);
					intif_charmovereq2(sd,map_charid2nick(sd->status.parent_id[1]),map[src->bl.m].name,src->bl.x,src->bl.y,2);
				}
			}
			else if(sg->skill_id == WE_CALLBABY) {		/* �V��A��������Ⴂ */
				if(sd->status.baby_id)
					intif_charmovereq2(sd,map_charid2nick(sd->status.baby_id),map[src->bl.m].name,src->bl.x,src->bl.y,2);
			}
		}
		break;
	}
	return 0;
}

/*==========================================
 * �X�L�����j�b�g�̃_���[�W�C�x���g
 *------------------------------------------
 */
int skill_unit_ondamaged(struct skill_unit *src,struct block_list *bl,int damage,unsigned int tick)
{
	struct skill_unit_group *sg;

	nullpo_retr(0, src);
	nullpo_retr(0, sg = src->group);

	switch(sg->unit_id) {
	case UNT_ICEWALL:		/* �A�C�X�E�H�[�� */
	case UNT_SKIDTRAP:		/* �X�L�b�h�g���b�v */
	case UNT_LANDMINE:		/* �����h�}�C�� */
	case UNT_SHOCKWAVE:		/* �V���b�N�E�F�[�u�g���b�v */
	case UNT_SANDMAN:		/* �T���h�}�� */
	case UNT_FLASHER:		/* �t���b�V���[ */
	case UNT_FREEZINGTRAP:		/* �t���[�W���O�g���b�v */
	case UNT_TALKIEBOX:		/* �g�[�L�[�{�b�N�X */
	case UNT_ANKLESNARE:		/* �A���N���X�l�A */
		src->val1 -= damage;
		break;
	case UNT_BLASTMINE:		/* �u���X�g�}�C�� */
		if(bl == NULL) {
			damage = 0;
			break;
		}
		skill_blown(bl,&src->bl,2);	// ������΂��Ă݂�
		break;
	default:
		damage = 0;
		break;
	}
	return damage;
}

/*---------------------------------------------------------------------------- */

/*==========================================
 * �͈͓��L�������݊m�F���菈��(foreachinarea)
 *------------------------------------------
 */
static int skill_check_condition_char_sub(struct block_list *bl,va_list ap)
{
	int *c;
	struct map_session_data *sd, *ssd;
	struct skill_condition *cnd;
	int sp;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd  = (struct map_session_data *)bl);
	nullpo_retr(0, ssd = va_arg(ap,struct map_session_data *));
	nullpo_retr(0, c   = va_arg(ap,int *));
	nullpo_retr(0, cnd = va_arg(ap, struct skill_condition *));

	// �`�F�b�N���Ȃ��ݒ�Ȃ�c�ɂ��肦�Ȃ��傫�Ȑ�����Ԃ��ďI��
	// �{����foreach�̑O�ɂ�肽�����ǐݒ�K�p�ӏ����܂Ƃ߂邽�߂ɂ�����
	if(!battle_config.player_skill_partner_check) {
		(*c) = 0x7fffffff;
		return 0;
	}

	if(sd == ssd)
		return 0;

	sp = skill_get_sp(cnd->id,cnd->lv);

	switch(cnd->id){
	case PR_BENEDICTIO:		/* ���̍~�� */
		if( (*c) < 2 &&
		    (sd->s_class.job == 4 || sd->s_class.job == 8 || sd->s_class.job == 15) &&
		    (sd->bl.y == ssd->bl.y && (sd->bl.x == ssd->bl.x-1 || sd->bl.x == ssd->bl.x+1)) &&
		    sd->status.sp >= sp/2 )
			(*c)++;
		break;
	case BD_LULLABY:		/* �q��� */
	case BD_RICHMANKIM:		/* �j�����h�̉� */
	case BD_ETERNALCHAOS:		/* �i���̍��� */
	case BD_DRUMBATTLEFIELD:	/* �푾�ۂ̋��� */
	case BD_RINGNIBELUNGEN:		/* �j�[�x�����O�̎w�� */
	case BD_ROKISWEIL:		/* ���L�̋��� */
	case BD_INTOABYSS:		/* �[���̒��� */
	case BD_SIEGFRIED:		/* �s���g�̃W�[�N�t���[�h */
	case BD_RAGNAROK:		/* �_�X�̉��� */
	case CG_MOONLIT:		/* ������̉��� */
		if( (*c) < 1 &&
		    ((ssd->s_class.job == 19 && sd->s_class.job == 20) || (ssd->s_class.job == 20 && sd->s_class.job == 19)) &&
		    sd->status.party_id > 0 &&
		    ssd->status.party_id > 0 &&
		    sd->status.party_id == ssd->status.party_id &&
		    !unit_isdead(&sd->bl) &&
		    !pc_issit(sd) &&
		    sd->sc.data[SC_DANCING].timer == -1 &&
		    (skill_get_weapontype(cnd->id) & (1<<sd->status.weapon)) &&
		    sd->status.sp >= sp &&
		    sd->sc.data[SC_STONE].timer == -1 &&
		    sd->sc.data[SC_FREEZE].timer == -1 &&
		    sd->sc.data[SC_SILENCE].timer == -1 &&
		    sd->sc.data[SC_SLEEP].timer == -1 &&
		    sd->sc.data[SC_STUN].timer == -1 )
			(*c) = pc_checkskill(sd,cnd->id);
		break;
	}
	return 0;
}

/*==========================================
 * �͈͓��L�������݊m�F�����X�L���g�p����(foreachinarea)
 *------------------------------------------
 */
static int skill_check_condition_use_sub(struct block_list *bl,va_list ap)
{
	int *c;
	struct map_session_data *sd, *ssd;
	int skillid, skilllv, sp;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd  = (struct map_session_data *)bl);
	nullpo_retr(0, ssd = va_arg(ap,struct map_session_data *));
	nullpo_retr(0, c   = va_arg(ap,int *));

	// �`�F�b�N���Ȃ��ݒ�Ȃ�c�ɂ��肦�Ȃ��傫�Ȑ�����Ԃ��ďI��
	// �{����foreach�̑O�ɂ�肽�����ǐݒ�K�p�ӏ����܂Ƃ߂邽�߂ɂ�����
	if(!battle_config.player_skill_partner_check) {
		(*c) = 0x7fffffff;
		return 0;
	}

	if(sd == ssd)
		return 0;

	skillid = ssd->ud.skillid;
	skilllv = ssd->ud.skilllv;

	sp = skill_get_sp(skillid,skilllv);

	switch(skillid){
	case PR_BENEDICTIO:		/* ���̍~�� */
		if( (*c) < 2 &&
		    (sd->s_class.job == 4 || sd->s_class.job == 8 || sd->s_class.job == 15) &&
		    (sd->bl.y == ssd->bl.y && (sd->bl.x == ssd->bl.x-1 || sd->bl.x == ssd->bl.x+1)) &&
		    sd->status.sp >= sp/2 )
		{
			sd->status.sp -= sp/2;
			clif_updatestatus(sd,SP_SP);
			(*c)++;
		}
		break;
	case BD_LULLABY:		/* �q��� */
	case BD_RICHMANKIM:		/* �j�����h�̉� */
	case BD_ETERNALCHAOS:		/* �i���̍��� */
	case BD_DRUMBATTLEFIELD:	/* �푾�ۂ̋��� */
	case BD_RINGNIBELUNGEN:		/* �j�[�x�����O�̎w�� */
	case BD_ROKISWEIL:		/* ���L�̋��� */
	case BD_INTOABYSS:		/* �[���̒��� */
	case BD_SIEGFRIED:		/* �s���g�̃W�[�N�t���[�h */
	case BD_RAGNAROK:		/* �_�X�̉��� */
	case CG_MOONLIT:		/* ������̉��� */
		if( (*c) < 1 &&
		    ((ssd->s_class.job == 19 && sd->s_class.job == 20) || (ssd->s_class.job == 20 && sd->s_class.job == 19)) &&
		    pc_checkskill(sd,skillid) > 0 &&
		    sd->status.party_id > 0 &&
		    ssd->status.party_id > 0 &&
		    sd->status.party_id == ssd->status.party_id &&
		    !unit_isdead(&sd->bl) &&
		    !pc_issit(sd) &&
		    sd->sc.data[SC_DANCING].timer == -1 &&
		    (skill_get_weapontype(skillid) & (1<<sd->status.weapon)) &&
		    sd->status.sp >= sp &&
		    sd->sc.data[SC_STONE].timer == -1 &&
		    sd->sc.data[SC_FREEZE].timer == -1 &&
		    sd->sc.data[SC_SILENCE].timer == -1 &&
		    sd->sc.data[SC_SLEEP].timer == -1 &&
		    sd->sc.data[SC_STUN].timer == -1 )
		{
			sd->status.sp -= sp;
			clif_updatestatus(sd,SP_SP);
			ssd->sc.data[SC_DANCING].val4 = bl->id;
			clif_skill_nodamage(bl,&ssd->bl,skillid,skilllv,1);
			status_change_start(bl,SC_DANCING,skillid,ssd->sc.data[SC_DANCING].val2,0,ssd->bl.id,skill_get_time(skillid,skilllv)+1000,0);
			sd->skillid_dance = sd->ud.skillid = skillid;
			sd->skilllv_dance = sd->ud.skilllv = skilllv;
			ssd->dance.x = sd->bl.x;
			ssd->dance.y = sd->bl.y;
			(*c)++;
		}
		break;
	}
	return 0;
}

/*==========================================
 * �͈͓��o�C�I�v�����g�A�X�t�B�A�}�C���pMob���݊m�F���菈��(foreachinarea)
 *------------------------------------------
 */
static int skill_check_condition_mob_master_sub(struct block_list *bl,va_list ap)
{
	int *c,src_id=0,mob_class=0;
	struct mob_data *md;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, md=(struct mob_data*)bl);
	nullpo_retr(0, src_id=va_arg(ap,int));
	nullpo_retr(0, mob_class=va_arg(ap,int));
	nullpo_retr(0, c=va_arg(ap,int *));

	if(md->class_ == mob_class && md->master_id == src_id)
		(*c)++;
	return 0;
}

/*==========================================
 * �X�L���g�p�����i�U�Ŏg�p���s�j
 *------------------------------------------
 */
static int skill_check_condition2_pc(struct map_session_data *sd, struct skill_condition *cnd, int type);
static int skill_check_condition2_mob(struct mob_data *md, struct skill_condition *cnd, int type);
static int skill_check_condition2_pet(struct pet_data *pd, struct skill_condition *cnd, int type);
static int skill_check_condition2_hom(struct homun_data *hd, struct skill_condition *cnd, int type);
static int skill_check_condition2_merc(struct merc_data *hd, struct skill_condition *cnd, int type);

int skill_check_condition(struct block_list *bl, int type)
{
	struct unit_data *ud = unit_bl2ud( bl );
	struct skill_condition cnd;
	int r;

	nullpo_retr( 0, ud );

	cnd.id     = ud->skillid;
	cnd.lv     = ud->skilllv;
	cnd.x      = ud->skillx;
	cnd.y      = ud->skilly;
	cnd.target = ud->skilltarget;

	r = skill_check_condition2( bl, &cnd, type );
	// skill_check_condition �����ŏ����������l��߂�
	if( r ) {
		ud->skillid     = cnd.id;
		ud->skilllv     = cnd.lv;
		ud->skillx      = cnd.x;
		ud->skilly      = cnd.y;
		ud->skilltarget = cnd.target;
	}
	return r;
}

int skill_check_condition2(struct block_list *bl, struct skill_condition *cnd, int type)
{
	struct map_session_data *sd;
	struct map_session_data *target_sd;
	struct status_change    *sc;
	struct block_list *target;

	nullpo_retr(0, bl);
	nullpo_retr(0, cnd);

	target = map_id2bl( cnd->target );
	if( target && target->type != BL_PC && target->type != BL_MOB && target->type != BL_HOM && target->type != BL_MERC ) {
		// �X�L���Ώۂ�PC,MOB,HOM,MERC�̂�
		target = NULL;
	}

	sd        = BL_DOWNCAST( BL_PC, bl );
	target_sd = BL_DOWNCAST( BL_PC, target );

	sc = status_get_sc(bl);

	// PC, MOB, PET, HOM, MERC ���ʂ̎��s�͂����ɋL�q

	// ��Ԉُ�֘A
	if(sc && sc->count > 0)
	{
		if( sc->data[SC_SILENCE].timer!=-1 ||
		    sc->data[SC_ROKISWEIL].timer!=-1 ||
		    (sc->data[SC_AUTOCOUNTER].timer != -1 && cnd->id != KN_AUTOCOUNTER) ||
		    sc->data[SC_STEELBODY].timer != -1 ||
		    sc->data[SC_BERSERK].timer != -1 ||
		    (sc->data[SC_MARIONETTE].timer !=-1 && cnd->id != CG_MARIONETTE) )
			return 0;

		if(sc->data[SC_BLADESTOP].timer != -1) {
			int lv = sc->data[SC_BLADESTOP].val1;
			if(sc->data[SC_BLADESTOP].val2 == 1) return 0;	// ���H���ꂽ���Ȃ̂Ń_��
			if(lv==1) return 0;
			if(lv==2 && cnd->id!=MO_FINGEROFFENSIVE) return 0;
			if(lv==3 && cnd->id!=MO_FINGEROFFENSIVE && cnd->id!=MO_INVESTIGATE) return 0;
			if(lv==4 && cnd->id!=MO_FINGEROFFENSIVE && cnd->id!=MO_INVESTIGATE && cnd->id!=MO_CHAINCOMBO) return 0;
			if(lv==5 && cnd->id!=MO_FINGEROFFENSIVE && cnd->id!=MO_INVESTIGATE && cnd->id!=MO_CHAINCOMBO && cnd->id!=MO_EXTREMITYFIST) return 0;
		}

		/* ���t/�_���X�� */
		if(sc->data[SC_DANCING].timer != -1 && sc->data[SC_LONGINGFREEDOM].timer == -1)
		{
			if(!battle_config.player_skill_partner_check &&
			   !(battle_config.sole_concert_type & 2) &&	// �P�ƍ��t���ɖ��/MS���ł��Ȃ��ݒ�
			   cnd->id != BD_ADAPTATION && cnd->id != CG_LONGINGFREEDOM)
			{
				switch (sc->data[SC_DANCING].val1)
				{
				case BD_LULLABY:			// �q���
				case BD_RICHMANKIM:			// �j�����h�̉�
				case BD_ETERNALCHAOS:		// �i���̍���
				case BD_DRUMBATTLEFIELD:	// �푾�ۂ̋���
				case BD_RINGNIBELUNGEN:		// �j�[�x�����O�̎w��
				case BD_ROKISWEIL:			// ���L�̋���
				case BD_INTOABYSS:			// �[���̒���
				case BD_SIEGFRIED:			// �s���g�̃W�[�N�t���[�h
				case BD_RAGNAROK:			// �_�X�̉���
				case CG_MOONLIT:			// ������̉���
					return 0;
				}
			}
			if(cnd->id != BD_ADAPTATION && cnd->id != BA_MUSICALSTRIKE && cnd->id != DC_THROWARROW && cnd->id != CG_LONGINGFREEDOM)
				return 0;
		}
	}

	// ���X�L�����ǂ����̔���
	if( cnd->id == SL_ALCHEMIST ||
	    (cnd->id >= SL_MONK && cnd->id <= SL_SOULLINKER && cnd->id != BS_ADRENALINE2) ||
	    cnd->id == SL_HIGH ||
	    (cnd->id >= SL_DEATHKNIGHT && cnd->id <= SL_GUNNER) )
	{
		int job, fail = 0;

		// ���؂Ɏ��Ԃ�������̂ŉ�n�łQ�v���C���[�����Ȃ��ꍇ�͈ꗥ�e��
		if(!sd || !target_sd)
			return 0;

		job = target_sd->s_class.job;

		switch(cnd->id)
		{
			case SL_ALCHEMIST:   if(job != 18) fail = 1; break; // �A���P�~�X�g�̍�
			case SL_MONK:        if(job != 15) fail = 1; break; // �����N�̍�
			case SL_STAR:        if(job != 25 && job != 26) fail = 1; break; // �P���Z�C�̍�
			case SL_SAGE:        if(job != 16) fail = 1; break; // �Z�[�W�̍�
			case SL_CRUSADER:    if(job != 14) fail = 1; break; // �N���Z�C�_�[�̍�
			case SL_SUPERNOVICE: if(job != 23) fail = 1; break; // �X�[�p�[�m�[�r�X�̍�
			case SL_KNIGHT:      if(job !=  7) fail = 1; break; // �i�C�g�̍�
			case SL_WIZARD:      if(job !=  9) fail = 1; break; // �E�B�U�[�h�̍�
			case SL_PRIEST:      if(job !=  8) fail = 1; break; // �v���[�X�g�̍�
			case SL_BARDDANCER:  if(job != 19 && job !=20) fail = 1; break; // �o�[�h�ƃ_���T�[�̍�
			case SL_ROGUE:       if(job != 17) fail = 1; break; // ���[�O�̍�
			case SL_ASSASIN:     if(job != 12) fail = 1; break; // �A�T�V���̍�
			case SL_BLACKSMITH:  if(job != 10) fail = 1; break; // �u���b�N�X�~�X�̍�
			case SL_HUNTER:      if(job != 11) fail = 1; break; // �n���^�[�̍�
			case SL_SOULLINKER:  if(job != 27) fail = 1; break; // �\�E�������J�[�̍�
			case SL_HIGH:        if(job < 1 || job > 6 || target_sd->s_class.upper != 1) fail = 1; break; // �ꎟ��ʐE�Ƃ̍�
			default: fail = 1;
		}
		if(battle_config.job_soul_check && fail) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		// �ŏI����
		if(!battle_config.soulskill_can_be_used_for_myself && sd == target_sd)
		{
			status_change_start(&sd->bl,SC_STUN,7,0,0,0,3000,0);
			return 0;
		}
	}

	// �X�L�����Ƃ̓��ꔻ��
	switch( cnd->id ) {
	case AM_POTIONPITCHER:		/* �|�[�V�����s�b�`���[ */
		if(target) {
			if(bl == target)	// �Ώۂ������Ȃ�OK
				break;
			if(target->type == BL_HOM) {
				struct homun_data *thd = (struct homun_data *)target;
				if(thd && thd->msd) {
					int pid;
					if(sd && sd == thd->msd)	// �����̃z����OK
						break;
					pid = status_get_party_id(bl);
					if(pid > 0 && pid == status_get_party_id(&thd->msd->bl))	// PTM�̃z����OK
						break;
				}
			} else {
				int pid, gid;
				pid = status_get_party_id(bl);
				if(pid > 0 && pid == status_get_party_id(target))	// ����PT��OK
					break;
				gid = status_get_guild_id(bl);
				if(gid > 0 && gid == status_get_guild_id(target))	// �����M���h��OK
					break;
				if(guild_check_alliance(gid,status_get_guild_id(target),0))	// �����M���h��OK
					break;
			}
		}
		if(sd)
			clif_skill_fail(sd,cnd->id,0,0);
		return 0;

	case MO_KITRANSLATION:	/* �C���]�� */
		if( !target_sd ||
		    bl == target ||
		    target_sd->status.party_id <= 0 ||
		    status_get_party_id(bl) != target_sd->status.party_id ||
		    target_sd->status.class_ == PC_CLASS_GS )
		{
			if(sd)
				clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case ALL_RESURRECTION:	/* ���U���N�V���� */
		if(!target)
			return 0;
		if(!unit_isdead(target) && !battle_check_undead(status_get_race(target),status_get_elem_type(target)))
			return 0;
		break;
	case HP_BASILICA:		/* �o�W���J */
		if(!type) {
			// �r���J�n���̂݃`�F�b�N
			if(skill_check_unit_range(bl->m,bl->x,bl->y,cnd->id,cnd->lv)) {
				if(sd)
					clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
			if(skill_check_unit_range2(bl->m,bl->x,bl->y,cnd->id,cnd->lv)) {
				if(sd)
					clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
		break;
	case HT_POWER:		/* �r�[�X�g�X�g���C�s���O */
		if(sc && sc->data[SC_HUNTER].timer != -1 && sc->data[SC_DOUBLE].timer != -1) {
			int race = status_get_race(target);
			if(race == RCT_BRUTE || race == RCT_INSECT)
				break;
		}
		if(sd)
			clif_skill_fail(sd,cnd->id,0,0);
		return 0;

	case AM_TWILIGHT1:
		if(!sc || sc->data[SC_ALCHEMIST].timer == -1) {
			if(sd)
				clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case AM_TWILIGHT2:
		if(!sd)
			return 0;
		if(battle_config.twilight_party_check) {
			int f = 0;
			if(sd->status.party_id > 0 && sc && sc->data[SC_ALCHEMIST].timer != -1)
			{
				struct party *pt = party_search(sd->status.party_id);
				if(pt) {
					struct map_session_data* psd;
					int i;
					for(i=0; i<MAX_PARTY; i++) {
						psd = pt->member[i].sd;
						if(psd && (psd->status.class_ == PC_CLASS_SNV || psd->status.class_ == PC_CLASS_SNV3)) {
							f = 1;
							break;
						}
					}
				}
			}
			if(f == 0) {
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
		break;
	case AM_TWILIGHT3:
		if(!sd)
			return 0;
		if(battle_config.twilight_party_check) {
			int f = 0;
			if(sd->status.party_id > 0 && sc && sc->data[SC_ALCHEMIST].timer != -1)
			{
				struct party *pt = party_search(sd->status.party_id);
				if(pt) {
					struct map_session_data* psd;
					int i;
					for(i=0; i<MAX_PARTY; i++) {
						psd = pt->member[i].sd;
						if(psd && psd->status.class_ == PC_CLASS_TK) {
							f = 1;
							break;
						}
					}
				}
			}
			if(f == 0) {
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
		break;
	case AM_BERSERKPITCHER:
		if(target_sd && target_sd->status.base_level < 85) {
			if(sd)
				clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case NPC_SUMMONSLAVE:
	case NPC_SUMMONMONSTER:
		if(bl->type != BL_MOB || ((struct mob_data*)bl)->master_id != 0)
			return 0;
		break;
	case WE_BABY:
		if(!sd)
			return 0;
		if( !target_sd ||
		    (sd->status.parent_id[0] != target_sd->status.char_id && sd->status.parent_id[1] != target_sd->status.char_id) )
		{
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case CR_PROVIDENCE:	// �v�����B�f���X
		if(!target_sd)
			return 0;
		if(target_sd->s_class.job == 14 || target_sd->s_class.job == 21) {
			if(sd)
				clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case HP_ASSUMPTIO:	// �A�X���v�e�B�I
		if(map[bl->m].flag.gvg && !battle_config.allow_assumptop_in_gvg) {
			if(sd)
				clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case CG_MARIONETTE:	// �}���I�l�b�g
		if(!sd || !target_sd)
			return 0;

		// ���Ɏ������ڑ����Ă�������Ȃ�~�߂�
		if(sc && sc->data[SC_MARIONETTE].timer != -1 && sc->data[SC_MARIONETTE].val2 == target_sd->bl.id) {
			status_change_end(bl,SC_MARIONETTE,-1);
			return 0;
		}

		// �����E�����N���X�E�}���I�l�b�g��ԂȂ玸�s
		if( sd == target_sd || sd->s_class.job == target_sd->s_class.job ||
		    sd->sc.data[SC_MARIONETTE].timer != -1 || sd->sc.data[SC_MARIONETTE2].timer != -1 ||
		    target_sd->sc.data[SC_MARIONETTE].timer != -1 || target_sd->sc.data[SC_MARIONETTE2].timer != -1)
		{
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	}

	if(bl->type == BL_PC)
		return skill_check_condition2_pc((struct map_session_data*)bl, cnd, type);
	if(bl->type == BL_MOB)
		return skill_check_condition2_mob((struct mob_data*)bl, cnd, type);
	if(bl->type == BL_PET)
		return skill_check_condition2_pet((struct pet_data*)bl, cnd, type);
	if(bl->type == BL_HOM)
		return skill_check_condition2_hom((struct homun_data*)bl, cnd, type);
	if(bl->type == BL_MERC)
		return skill_check_condition2_merc((struct merc_data*)bl, cnd, type);

	return 0;
}

// PC�p����( 0: �g�p���s 1: �g�p���� )
static int skill_check_condition2_pc(struct map_session_data *sd, struct skill_condition *cnd, int type)
{
	int i,hp,sp,hp_rate,sp_rate,zeny,weapon,state,spiritball,coin,skilldb_id,mana,arrow;
	int itemid[10],amount[10];
	int item_nocost = 0;
	struct block_list *bl = NULL, *target = NULL;
	struct unit_data  *ud = NULL;

	nullpo_retr(0, sd);
	nullpo_retr(0, cnd);
	nullpo_retr(0, bl = &sd->bl);
	nullpo_retr(0, ud = unit_bl2ud(bl));

	target = map_id2bl( cnd->target );

	// �`�F�C�X�A�n�C�h�A�N���[�L���O���̃X�L��
	if(sd->sc.option&0x02) {
		if(cnd->id != TF_HIDING &&
		   cnd->id != AS_GRIMTOOTH &&
		   cnd->id != RG_BACKSTAP &&
		   cnd->id != RG_RAID &&
		   cnd->id != NJ_KIRIKAGE &&
		   cnd->id != NJ_SHADOWJUMP)
			return 0;
	}
	if(pc_ischasewalk(sd) && cnd->id != ST_CHASEWALK)	// �`�F�C�X�E�H�[�N
	 	return 0;

	// �\�E�������J�[�Ŏg���Ȃ��X�L��
	if(sd->status.class_ == PC_CLASS_SL) {
		switch(cnd->id) {
			case TK_READYSTORM:
			case TK_READYDOWN:
			case TK_READYTURN:
			case TK_READYCOUNTER:
			case TK_JUMPKICK:
				if( battle_config.soul_linker_battle_mode == 0 ||
				    (battle_config.soul_linker_battle_mode == 1 && sd->sc.data[SC_SOULLINKER].timer == -1) ) {
					clif_skill_fail(sd,cnd->id,0,0);
					return 0;
				}
				break;
			case SL_KAIZEL:		/* �J�C�[�� */
			case SL_KAAHI:		/* �J�A�q */
			case SL_KAITE:		/* �J�C�g */
			case SL_KAUPE:		/* �J�E�v */
				if(target == NULL || target->type != BL_PC) {
					// �Ώۂ��l�ȊO���s
					clif_skill_fail(sd,cnd->id,0,0);
					return 0;
				}
				if(battle_config.soul_linker_battle_mode_ka == 0) {
					struct map_session_data *target_sd = (struct map_session_data *)target;
					if( target_sd->status.char_id == sd->status.char_id ||
					    target_sd->status.char_id == sd->status.partner_id ||
					    target_sd->status.char_id == sd->status.baby_id ||
					    sd->sc.data[SC_SOULLINKER].timer != -1 ) {
						;	// �������Ȃ�
					} else {
						clif_skill_fail(sd,cnd->id,0,0);
						return 0;
					}
				}
				break;
		}
	}

	// GM�n�C�h���ŁA�R���t�B�O�Ńn�C�h���U���s�� GM���x�����w����傫���ꍇ
	if(sd->sc.option&0x40 && battle_config.hide_attack == 0 && pc_isGM(sd) < battle_config.gm_hide_attack_lv)
		return 0;	// �B��ăX�L���g���Ȃ�Ĕڋ���GM�޽�

	if(battle_config.gm_skilluncond > 0 && pc_isGM(sd) >= battle_config.gm_skilluncond)
		return 1;

	if(sd->sc.opt1 > 0) {
		clif_skill_fail(sd,cnd->id,0,0);
		return 0;
	}
	if(pc_is90overweight(sd)) {
		clif_skill_fail(sd,cnd->id,9,0);
		return 0;
	}

	if(cnd->id == AC_MAKINGARROW && sd->state.make_arrow_flag == 1)
		return 0;

	if((cnd->id == AM_PHARMACY || cnd->id == ASC_CDP) && sd->state.produce_flag == 1)
		return 0;

	// �삯�����ɃX�L�����g�����ꍇ�I��
	// �ēx�̋삯������яR��͏��O
	if(sd->sc.data[SC_RUN].timer != -1 && cnd->id != TK_RUN && cnd->id != TK_JUMPKICK)
		status_change_end(bl,SC_RUN,-1);

	/* �A�C�e���̏ꍇ�̔��� */
	if(sd->skillitem == cnd->id) {
		if(type == 0)			// ����̌Ăяo���iunit.c���j�Ȃ疳�����ŋ���
			return 1;
		sd->skillitem   = -1;
		sd->skillitemlv = -1;
		if( !sd->skillitem_flag )	// �t���O�Ȃ��Ȃ狖��
			return 1;
	}

	skilldb_id = skill_get_skilldb_id(cnd->id);
	hp         = skill_get_hp(cnd->id, cnd->lv);	/* ����HP */
	sp         = skill_get_sp(cnd->id, cnd->lv);	/* ����SP */
	hp_rate    = (cnd->lv <= 0)? 0: skill_db[skilldb_id].hp_rate[cnd->lv-1];
	sp_rate    = (cnd->lv <= 0)? 0: skill_db[skilldb_id].sp_rate[cnd->lv-1];
	zeny       = skill_get_zeny(cnd->id,cnd->lv);
	weapon     = skill_db[skilldb_id].weapon;
	state      = skill_db[skilldb_id].state;
	spiritball = (cnd->lv <= 0)? 0: skill_db[skilldb_id].spiritball[cnd->lv-1];
	coin       = (cnd->lv <= 0)? 0: skill_db[skilldb_id].coin[cnd->lv-1];
	arrow      = skill_get_arrow_cost(cnd->id,cnd->lv);

	for(i=0; i<10; i++) {
		itemid[i] = skill_db[skilldb_id].itemid[i];
		amount[i] = skill_db[skilldb_id].amount[i];
	}

	if(hp_rate > 0)
		hp += sd->status.hp * hp_rate / 100;
	else
		hp += sd->status.max_hp * abs(hp_rate) / 100;
	if(sp_rate > 0)
		sp += sd->status.sp * sp_rate / 100;
	else
		sp += sd->status.max_sp * abs(sp_rate) / 100;

	if(sd->skillid_old == BD_ENCORE && cnd->id == sd->skillid_dance)	// �A���R�[������SP�������
		sp /= 2;
	if((mana = pc_checkskill(sd,HP_MANARECHARGE)) > 0)		// �}�i���`���[�W�Ŏg�pSP����
		sp -= sp * mana / 25;

	switch( cnd->id ) {
	case SL_SMA:	/* �G�X�} */
		if(!(type&1) && sd->sc.data[SC_SMA].timer==-1){	// �G�X�}�r���\���
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case CG_LONGINGFREEDOM:
		// ���t�ȊO�g���Ȃ�
		if(sd->sc.data[SC_DANCING].timer == -1) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		} else {
			int dance_id = sd->sc.data[SC_DANCING].val1;
			if( (dance_id >= BA_WHISTLE && dance_id <= BA_APPLEIDUN) ||
			    (dance_id >= DC_HUMMING && dance_id <= DC_SERVICEFORYOU) ||
			    dance_id == CG_MOONLIT || dance_id == CG_HERMODE )
			{
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
		break;
	case CG_MOONLIT:			/* ������̉��� */
		{
			int x1,x2,y1,y2,i,j;
			int range = skill_get_unit_range(CG_MOONLIT)+1;
			x1 = bl->x - range;
			x2 = bl->x + range;
			y1 = bl->y - range;
			y2 = bl->y + range;
			// �I�n�_���}�b�v�O
			if(x1<0 || x2>=map[bl->m].xs-1 || y1<0 || y2>=map[bl->m].ys-1)
				return 0;

			for(i=x1;i<=x2;i++)
			{
				for(j=y1;j<=y2;j++)
				{
					if(map_getcell(bl->m,i,j,CELL_CHKNOPASS))
						return 0;
				}
			}
		}
		// fall through
	case BD_LULLABY:				/* �q��� */
	case BD_RICHMANKIM:				/* �j�����h�̉� */
	case BD_ETERNALCHAOS:			/* �i���̍��� */
	case BD_DRUMBATTLEFIELD:		/* �푾�ۂ̋��� */
	case BD_RINGNIBELUNGEN:			/* �j�[�x�����O�̎w�� */
	case BD_ROKISWEIL:				/* ���L�̋��� */
	case BD_INTOABYSS:				/* �[���̒��� */
	case BD_SIEGFRIED:				/* �s���g�̃W�[�N�t���[�h */
	case BD_RAGNAROK:				/* �_�X�̉��� */
		{
			int range=1;
			int c=0;
			map_foreachinarea(skill_check_condition_char_sub,bl->m,
				bl->x-range,bl->y-range,
				bl->x+range,bl->y+range,BL_PC,sd,&c,cnd);
			// �_���X�J�n�ʒu(���t�p)
			sd->dance.x = bl->x;
			sd->dance.y = bl->y;
			if(c<1){
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}else if(c==0x7fffffff){ // �����s�v�ݒ肾����
				;
			}else{
				cnd->lv = (c + cnd->lv)/2;
			}
		}
		break;
	case SA_ELEMENTWATER:	// ��
	case SA_ELEMENTGROUND:	// �y
	case SA_ELEMENTFIRE:	// ��
	case SA_ELEMENTWIND:	// ��
		{
			// PC -> PC�����֎~����
			if( target && target->type == BL_PC )
			{
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
		break;
	}

	// ����ȏ����
	switch(cnd->id)
	{
		case MC_MAMMONITE:	/* ���}�[�i�C�g */
			if(pc_checkskill(sd,BS_UNFAIRLYTRICK)>0)
				zeny = zeny*90/100;
			break;
		case AL_HOLYLIGHT:	/* �z�[���[���C�g */
			// �v���[�X�g�̍�����SP����ʑ���
			if(sd->sc.data[SC_PRIEST].timer!=-1)
				sp = sp * 5;
			break;
		case SL_SMA:		/* �G�X�} */
		case SL_STUN:		/* �G�X�^�� */
		case SL_STIN:		/* �G�X�e�B�� */
			{
				int kaina_lv = pc_checkskill(sd,SL_KAINA);

				if(kaina_lv==0)
					break;
				if(sd->status.base_level>=90)
					sp -= sp*7*kaina_lv/100;
				else if(sd->status.base_level>=80)
					sp -= sp*5*kaina_lv/100;
				else if(sd->status.base_level>=70)
					sp -= sp*3*kaina_lv/100;
			}
			break;
		case MO_CHAINCOMBO:
		case MO_COMBOFINISH:
		case CH_TIGERFIST:
		case CH_CHAINCRUSH:
			// �����N�̍��@�A�g�X�L����SP�����
			if(sd->sc.data[SC_MONK].timer!=-1)
				sp -= sp*sd->sc.data[SC_MONK].val1/10;
			break;
		case NJ_ZENYNAGE:
			if(!(type&1)) {
				if(zeny>=2) {
					zeny /= 2;
					sd->zenynage_damage = zeny + atn_rand()%zeny;
					zeny = sd->zenynage_damage;
				} else {
					// ����������̃f�t�H���g�_���[�W
					sd->zenynage_damage = 500*cnd->lv + atn_rand()%(500*cnd->lv);
				}
			} else {
				zeny = sd->zenynage_damage;
			}
			break;
	}
	if(sd->dsprate!=100)
		sp=sp*sd->dsprate/100;	/* ����SP�C�� */

	switch(cnd->id) {
	case SA_CASTCANCEL:
		if(ud->skilltimer == -1) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case BS_MAXIMIZE:		/* �}�L�V�}�C�Y�p���[ */
	case NV_TRICKDEAD:		/* ���񂾂ӂ� */
	case TF_HIDING:			/* �n�C�f�B���O */
	case AS_CLOAKING:		/* �N���[�L���O */
	case CR_AUTOGUARD:		/* �I�[�g�K�[�h */
	case PA_GOSPEL:			/* �S�X�y�� */
	case ST_CHASEWALK:		/* �`�F�C�X�E�H�[�N */
	case NPC_INVISIBLE:		/* �C���r�W�u�� */
	case TK_RUN:			/* �^�C���M */
	case GS_GATLINGFEVER:		/* �K�g�����O�t�B�[�o�[ */
	case CR_SHRINK:			/* �V�������N */
	case ML_AUTOGUARD:
		{
			int sc_type = GetSkillStatusChangeTable(cnd->id);
			if(sc_type > 0 && sd->sc.data[sc_type].timer != -1)
				sp = 0;	// ��������ꍇ��SP����Ȃ�
		}
		break;
	case AL_TELEPORT:
		{
			int alive;
			if(map[bl->m].flag.noteleport) {
				alive = 0;
			} else {
				alive = 1;
				map_foreachinarea(skill_landprotector,bl->m,bl->x,bl->y,bl->x,bl->y,BL_SKILL,AL_TELEPORT,&alive);
			}
			if(!alive) {
				clif_skill_teleportmessage(sd,0);
				return 0;
			}
		}
		break;
	case AL_WARP:
		if(map[bl->m].flag.noportal) {
			clif_skill_teleportmessage(sd,0);
			return 0;
		}
		break;
	case MO_CALLSPIRITS:		/* �C�� */
		if(sd->spiritball >= cnd->lv) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case MO_BODYRELOCATION:		/* �c�e */
		if(sd->sc.data[SC_EXPLOSIONSPIRITS].timer != -1)
			spiritball = 0;
		break;
	case CH_SOULCOLLECT:		/* ���C�� */
		if(battle_config.soulcollect_max_fail) {
			if(sd->spiritball >= 5) {
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
		break;
	case MO_FINGEROFFENSIVE:	/* �w�e */
		if (sd->spiritball > 0 && sd->spiritball < spiritball) {
			spiritball = sd->spiritball;
			sd->spiritball_old = sd->spiritball;
		} else {
			sd->spiritball_old = cnd->lv;
		}
		break;
	case MO_CHAINCOMBO:		/* �A�ŏ� */
		if(sd->sc.data[SC_BLADESTOP].timer==-1){
			if(sd->sc.data[SC_COMBO].timer == -1 || sd->sc.data[SC_COMBO].val1 != MO_TRIPLEATTACK)
				return 0;
		}
		break;
	case MO_COMBOFINISH:		/* �җ��� */
		if(sd->sc.data[SC_COMBO].timer == -1 || sd->sc.data[SC_COMBO].val1 != MO_CHAINCOMBO)
			return 0;
		break;
	case CH_TIGERFIST:		/* ���Ռ� */
		if(sd->sc.data[SC_COMBO].timer == -1 || (sd->sc.data[SC_COMBO].val1 != MO_COMBOFINISH && sd->sc.data[SC_COMBO].val1 != CH_CHAINCRUSH))
			return 0;
		break;
	case CH_CHAINCRUSH:		/* �A������ */
		if(sd->sc.data[SC_COMBO].timer == -1)
			return 0;
		if(sd->sc.data[SC_COMBO].val1 != MO_COMBOFINISH && sd->sc.data[SC_COMBO].val1 != CH_TIGERFIST)
			return 0;
		break;
	case MO_EXTREMITYFIST:		/* ���C���e�P�� */
		if((sd->sc.data[SC_COMBO].timer != -1 && (sd->sc.data[SC_COMBO].val1 == MO_COMBOFINISH || sd->sc.data[SC_COMBO].val1 == CH_CHAINCRUSH)) || sd->sc.data[SC_BLADESTOP].timer!=-1)
		{
			if(sd->sc.data[SC_COMBO].timer != -1 && sd->sc.data[SC_COMBO].val1 == CH_CHAINCRUSH)
				spiritball = 1;
			else
				spiritball--;	// =4�ł��ǂ���
			if(spiritball<0)
				spiritball=0;
		}
		break;
	case TK_STORMKICK:		/* �t�F�I���`���M */
	case TK_DOWNKICK:		/* �l�����`���M */
	case TK_TURNKICK:		/* �g�������`���M */
	case TK_COUNTER:		/* �A�v�`���I�����M */
		if(sd->sc.data[SC_TKCOMBO].timer == -1 ||
		   (sd->sc.data[SC_TKCOMBO].val4 != TK_MISSION && sd->sc.data[SC_TKCOMBO].val1 != cnd->id) ||
		   (sd->sc.data[SC_TKCOMBO].val4 == TK_MISSION && sd->sc.data[SC_TKCOMBO].val1 == cnd->id)	// �����J�[�͒��O�̃X�L���Ɠ���Ȃ�R���{�I��
		) {
			clif_skill_fail(sd,cnd->id,0,0);
			status_change_end(&sd->bl,SC_TKCOMBO,-1);
			return 0;
		}
		break;
	case BD_ADAPTATION:		/* �A�h���u */
		{
			struct skill_unit_group *group=NULL;
			if(
				sd->sc.data[SC_DANCING].timer==-1 ||
				((group=(struct skill_unit_group*)sd->sc.data[SC_DANCING].val2) &&
				(skill_get_time(sd->sc.data[SC_DANCING].val1,group->skill_lv) -
				sd->sc.data[SC_DANCING].val3*1000) <= skill_get_time2(cnd->id,cnd->lv))
			){
				// �_���X���Ŏg�p��5�b�ȏ�̂݁H
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
		break;
	case PR_BENEDICTIO:		/* ���̍~�� */
		{
			int range=1;
			int c=0;
			if(!(type&1)){
				map_foreachinarea(skill_check_condition_char_sub,bl->m,
					bl->x-range,bl->y-range,
					bl->x+range,bl->y+range,BL_PC,sd,&c,cnd);
				if(c<2){
					clif_skill_fail(sd,cnd->id,0,0);
					return 0;
				}
			}else{
				map_foreachinarea(skill_check_condition_use_sub,bl->m,
					bl->x-range,bl->y-range,
					bl->x+range,bl->y+range,BL_PC,sd,&c);
			}
		}
		break;
	case WE_CALLPARTNER:		/* ���Ȃ��Ɉ������� */
		if(!sd->status.partner_id){
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case WE_CALLPARENT:		/* �}�}�A�p�p�A���� */
		if(!sd->status.parent_id[0] && !sd->status.parent_id[1]){
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case WE_CALLBABY:		/* �V��A��������Ⴂ */
		if(!sd->status.baby_id){
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case AM_CANNIBALIZE:		/* �o�C�I�v�����g */
	case AM_SPHEREMINE:		/* �X�t�B�A�[�}�C�� */
		if(type&1){
			int c,n=0;
			int summons[5] = { 1589, 1579, 1575, 1555, 1590 };
			int maxcount = skill_get_maxcount(cnd->id,cnd->lv);

			if(battle_config.pc_land_skill_limit && maxcount>0) {
				do{
					c=0;
					map_foreachinarea(
						skill_check_condition_mob_master_sub, bl->m, 0, 0, map[bl->m].xs,
						map[bl->m].ys, BL_MOB, bl->id,
						(cnd->id==AM_CANNIBALIZE)? summons[n]: 1142, &c
					);
					// ���񏢊�����mob�Ƃ͕ʂ̎�ނ�mob���������Ă��Ȃ������`�F�b�N
					if((cnd->id==AM_CANNIBALIZE && ((c > 0 && n != cnd->lv-1) || (n == cnd->lv-1 && c >= maxcount)))
						|| (cnd->id==AM_SPHEREMINE && c >= maxcount)){
						clif_skill_fail(sd,cnd->id,0,0);
						return 0;
					}
				}while(cnd->id != AM_SPHEREMINE && ++n < 5);
			}
		}
		break;
	case AM_CALLHOMUN:			/* �R�[���z�����N���X */
		// �쐬�ς݂ň�����
		if(sd->hom.homun_id > 0 && sd->status.homun_id == sd->hom.homun_id && !sd->hom.incubate)
			break;
		if(sd->hd) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		if(sd->hom.homun_id > 0) {	// �쐬�ς݃z�������鎞�ɃG���u���I�����Ă��玸�s
			for(i=0; i<MAX_INVENTORY; i++) {
				if(sd->status.inventory[i].nameid == 7142) {
					clif_skill_fail(sd,cnd->id,0,0);
					return 0;
				}
			}
		}
		break;
	case AM_REST:			/* ���� */
		if(!homun_isalive(sd) || status_get_hp(&sd->hd->bl) < sd->hd->max_hp * 80 / 100) {
			// �z����HP��MHP��80%�ȏ�ł��邱��
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case AM_RESURRECTHOMUN:			/* ���U���N�V�����z�����N���X */
		if(sd->hd || sd->hom.hp > 0) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case WZ_FIREPILLAR:
		if(cnd->lv <= 5)	// no gems required at level 1-5
			item_nocost = 1;
		// fall through
	case PF_SPIDERWEB:		/* �X�p�C�_�[�E�F�b�u */
	case MG_FIREWALL:		/* �t�@�C�A�[�E�H�[�� */
		/* ������ */
		if(battle_config.pc_land_skill_limit) {
			int maxcount = skill_get_maxcount(cnd->id,cnd->lv);
			if(maxcount > 0 && skill_count_unitgroup(ud,cnd->id) >= maxcount) {
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
		break;
	case PF_HPCONVERSION:		/* �����͕ϊ� */
		if(sd->status.sp >= sd->status.max_sp)
			return 0;
		break;
	case PA_PRESSURE:		/* �v���b�V���[ */
		if(status_get_class(target) == 1288) {	// �G���y�͎g�p�s��
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case CR_DEVOTION:		/* �f�B�{�[�V���� */
		if(target && target->type == BL_PC) {
			for(i = 0; i < cnd->lv && i < 5; i++) {
				if(sd->dev.val1[i] <= 0)
					break;
			}
			if(i >= cnd->lv || i >= 5) {
				// ����I�[�o�[
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
		break;

	case BD_LULLABY:			/* �q��S */
	case BD_RICHMANKIM:			/* �j�����h�̉� */
	case BD_ETERNALCHAOS:		/* �i���̍��� */
	case BD_DRUMBATTLEFIELD:	/* �푾�ۂ̋��� */
	case BD_RINGNIBELUNGEN:		/* �j�[�x�����O�̎w�� */
	case BD_ROKISWEIL:			/* ���L�̋��� */
	case BD_INTOABYSS:			/* �[���̒��� */
	case BD_SIEGFRIED:			/* �s���g�̃W�[�N�t���[�h */
	case BA_DISSONANCE:			/* �s���a�� */
	case BA_POEMBRAGI:			/* �u���M�̎� */
	case BA_WHISTLE:			/* ���J */
	case BA_ASSASSINCROSS:		/* �[�z�̃A�T�V���N���X */
	case BA_APPLEIDUN:			/* �C�h�D���̗ь� */
	case DC_UGLYDANCE:			/* ��������ȃ_���X */
	case DC_HUMMING:			/* �n�~���O */
	case DC_DONTFORGETME:		/* ����Y��Ȃ��Łc */
	case DC_FORTUNEKISS:		/* �K�^�̃L�X */
	case DC_SERVICEFORYOU:		/* �T�[�r�X�t�H�[���[ */
	case CG_MOONLIT:			/* ������̉��� */
		if(sd->sc.data[SC_LONGINGFREEDOM].timer!=-1)
			return 0;
		break;
	case CG_HERMODE:			/* �w�����[�h�̏� */
		if(sd->sc.data[SC_LONGINGFREEDOM].timer!=-1)
			return 0;
		if(battle_config.hermode_gvg_only && map[bl->m].flag.gvg==0){	// �V�[�Y�ȊO�g���Ȃ�
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		// WP�`�F�b�N�H
		if(battle_config.hermode_wp_check && !skill_hermode_wp_check(bl))
		{
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SG_SUN_WARM:
		if(sd->sc.data[SC_MIRACLE].timer==-1)
		{
			if(bl->m != sd->feel_index[0]){
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
		break;
	case SG_SUN_COMFORT:
		if(sd->sc.data[SC_MIRACLE].timer==-1)
		{
			if(bl->m == sd->feel_index[0] && (battle_config.allow_skill_without_day || is_day_of_sun()))
				break;
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SG_MOON_WARM:
		if(sd->sc.data[SC_MIRACLE].timer==-1)
		{
			if(bl->m != sd->feel_index[1]){
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
		break;
	case SG_MOON_COMFORT:
		if(sd->sc.data[SC_MIRACLE].timer==-1)
		{
			if(bl->m == sd->feel_index[1] && (battle_config.allow_skill_without_day || is_day_of_moon()))
				break;
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SG_STAR_WARM:
		if(sd->sc.data[SC_MIRACLE].timer==-1)
		{
			if(bl->m != sd->feel_index[2]){
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
		break;
	case SG_STAR_COMFORT:
		if(sd->sc.data[SC_MIRACLE].timer==-1)
		{
			if(bl->m == sd->feel_index[2] && (battle_config.allow_skill_without_day || is_day_of_star()))
				break;
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SG_HATE:
		if(status_get_class(target) == 1288) {	// �G���y�͓o�^�s��
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SG_FUSION:
		if(sd->sc.data[SC_FUSION].timer != -1) {	// ��������Ƃ���sp����Ȃ�
			sp = 0;
		}
		if(sd->sc.data[SC_STAR].timer == -1) {	// �P���Z�C�̍����
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case PR_REDEMPTIO:
		if(battle_config.redemptio_penalty_type) {
			int exp = pc_nextbaseexp(sd);
			if(exp <= 0) {
				// �I�[���̏ꍇ�͌��݂̃��x���ɕK�v�Ȍo���l���Q�Ƃ���
				sd->status.base_level--;
				exp = pc_nextbaseexp(sd);
				sd->status.base_level++;
				if(exp <= 0) {
					// ����ł�exp�����ݒ�Ȃ狖��
					break;
				}
			}
			if(sd->status.base_exp < exp / 100 * battle_config.death_penalty_base / 100) {
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
		break;
	case GS_GLITTERING:		/* �t���b�v�U�R�C�� */
		if(sd->coin >= 10) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case HT_POWER:			/* �r�[�X�g�X�g���C�t�B���O */
		if(sd->sc.data[SC_DOUBLE].timer == -1) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;

	case GD_BATTLEORDER:		/* �Ր�Ԑ� */
	case GD_REGENERATION:		/* ���� */
	case GD_RESTORE:		/* ���� */
	case GD_EMERGENCYCALL:		/* �ً}���W */
		if(!battle_config.guild_skill_available){
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		if(battle_config.allow_guild_skill_in_gvg_only && !map[bl->m].flag.gvg){
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		if(battle_config.guild_skill_in_pvp_limit && map[bl->m].flag.pvp){
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		if(cnd->id == GD_EMERGENCYCALL && battle_config.no_emergency_call){
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	}

	if(!(type&2)) {
		if(!sd->skillitem_flag) {		// �A�C�e���X�L���R���Ȃ�SP�`�F�b�N�͕s�v
			if(sp > 0 && sd->status.sp < sp) {		/* SP�`�F�b�N */
				clif_skill_fail(sd,cnd->id,1,0);
				return 0;
			}
		}
		if(hp > 0 && sd->status.hp < hp) {			/* HP�`�F�b�N */
			clif_skill_fail(sd,cnd->id,2,0);
			return 0;
		}
		if(zeny > 0 && sd->status.zeny < zeny) {
			sd->zenynage_damage = 0;
			clif_skill_fail(sd,cnd->id,5,0);
			return 0;
		}
		if(!(weapon & (1<<sd->status.weapon))) {
			clif_skill_fail(sd,cnd->id,6,0);
			return 0;
		}
		if(spiritball > 0 && sd->spiritball < spiritball) {
			clif_skill_fail(sd,cnd->id,0,0);		// �C���s��
			return 0;
		}
		if(coin > 0 && sd->coin < coin) {
			clif_skill_fail(sd,cnd->id,0,0);		// �R�C���s��
			return 0;
		}
		if(arrow > 0) {						// ��s��
			int idx = sd->equip_index[10];
			if( idx == -1 ||
			    !(sd->inventory_data[idx]->arrow_type & skill_get_arrow_type(cnd->id)) ||
			    sd->status.inventory[idx].amount < arrow )
			{
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
	}

	switch(state) {
	case SST_HIDING:
		if(!(sd->sc.option&2)) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SST_CLOAKING:
		if(!pc_iscloaking(sd)) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SST_CHASEWALKING:
		if(!pc_ischasewalk(sd)) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SST_HIDDEN:
		if(!pc_ishiding(sd)) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SST_RIDING:
		if(!pc_isriding(sd)) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SST_FALCON:
		if(!pc_isfalcon(sd)) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SST_CART:
		if(!pc_iscarton(sd)) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SST_SHIELD:
		if(sd->status.shield <= 0) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SST_SIGHT:
		if(sd->sc.data[SC_SIGHT].timer == -1) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SST_EXPLOSIONSPIRITS:
		if(sd->sc.data[SC_EXPLOSIONSPIRITS].timer == -1) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SST_CARTBOOST:
		if(sd->sc.data[SC_CARTBOOST].timer == -1) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SST_NEN:
		if(sd->sc.data[SC_NEN].timer == -1) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SST_RECOV_WEIGHT_RATE:
		if(battle_config.natural_heal_weight_rate <= 100 && sd->weight*100/sd->max_weight >= battle_config.natural_heal_weight_rate) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SST_MOVE_ENABLE:
		if(path_search(NULL,bl->m,bl->x,bl->y,cnd->x,cnd->y,1) == -1) {
			clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		break;
	case SST_WATER:
		if(!map[bl->m].flag.rain) {
			// ���ꔻ��
			if( !map_getcell(bl->m,bl->x,bl->y,CELL_CHKWATER) &&
			    sd->sc.data[SC_DELUGE].timer == -1 &&
			    sd->sc.data[SC_SUITON].timer == -1 )
			{
				clif_skill_fail(sd,cnd->id,0,0);
				return 0;
			}
		}
		break;
	}

	// ������p �o�Ă���Ԃ̓R�X�g����
	switch(cnd->id)
	{
		case SA_VOLCANO:		/* �{���P�[�m */
		case SA_DELUGE:			/* �f�����[�W */
		case SA_VIOLENTGALE:	/* �o�C�I�����g�Q�C�� */
			if(sd->sc.data[SC_ELEMENTFIELD].timer != -1)
				item_nocost = 1;
			break;
	}

	// GVG PVP�ȊO�̃}�b�v�ł̓��ꏈ��
	if(map[bl->m].flag.pvp==0 && map[bl->m].flag.gvg==0)
	{
		switch(cnd->id)
		{
			case AM_DEMONSTRATION:
				if(battle_config.demonstration_nocost)
					item_nocost = 1;
				break;
			case AM_ACIDTERROR:
				if(battle_config.acidterror_nocost)
					item_nocost = 1;
				break;
			case AM_CANNIBALIZE:
				if(battle_config.cannibalize_nocost)
					item_nocost = 1;
				break;
			case AM_SPHEREMINE:
				if(battle_config.spheremine_nocost)
					item_nocost = 1;
				break;
			case AM_CP_WEAPON:
			case AM_CP_SHIELD:
			case AM_CP_ARMOR:
			case AM_CP_HELM:
			case CR_FULLPROTECTION:
				if(battle_config.chemical_nocost)
					item_nocost = 1;
				break;
			case CR_ACIDDEMONSTRATION:
				if(battle_config.aciddemonstration_nocost)
					item_nocost = 1;
				break;
			case CR_SLIMPITCHER:
				if(battle_config.slimpitcher_nocost)
				{
					for(i=0;i<5;i++) {
						// �ԃ|�[�V����
						itemid[i] = 501;
						amount[i] = 1;
					}
					for(;i<9;i++) {
						// ���|�[�V����
						itemid[i] = 503;
						amount[i] = 1;
					}
					// ���|�[�V����
					itemid[i] = 504;
					amount[i] = 1;
				}
			default:
				break;
		}
	}

	if(!item_nocost) {
		if(skill_item_consume(&sd->bl, cnd, type, itemid, amount) == 0)
			return 0;
	}

	if(type == 1) {
		if(!sd->skillitem_flag) {	// �A�C�e���X�L���R���Ȃ�SP����Ȃ�
			if(sp > 0) {				// SP����
				sd->status.sp -= sp;
				clif_updatestatus(sd,SP_SP);
			}
		}
		if(hp > 0) {					// HP����
			sd->status.hp -= hp;
			clif_updatestatus(sd,SP_HP);
		}
		if(zeny > 0)					// Zeny����
			pc_payzeny(sd,zeny);
		if(spiritball > 0)				// �C������
			pc_delspiritball(sd,spiritball,0);
		if(coin > 0)					// �R�C������
			pc_delcoin(sd,coin,0);
	}
	return 1;
}

// MOB�p����( 0: �g�p���s 1: �g�p���� )
static int skill_check_condition2_mob(struct mob_data *md, struct skill_condition *cnd, int type)
{
	nullpo_retr(0, md);
	nullpo_retr(0, cnd);

	if(md->sc.option&4 && cnd->id == TF_HIDING)
		return 0;

	if(md->sc.option&0x02) {
		if(cnd->id != TF_HIDING &&
		   cnd->id != AS_GRIMTOOTH &&
		   cnd->id != RG_BACKSTAP &&
		   cnd->id != RG_RAID &&
		   cnd->id != NJ_KIRIKAGE &&
		   cnd->id != NJ_SHADOWJUMP)
			return 0;
	}
	if(md->sc.opt1 > 0)
		return 0;

	return 1;
}

// PET�p����( 0: �g�p���s 1: �g�p���� )
static int skill_check_condition2_pet(struct pet_data *pd, struct skill_condition *cnd, int type)
{
	nullpo_retr(0, pd);
	nullpo_retr(0, cnd);

	// �y�b�g���g���Ȃ��ق����悢�X�L��
	switch(cnd->id)
	{
		case CG_MOONLIT:	/* ������̉��� */
			// �y�b�g�Ɏg����ƒʍs�̎ז�
			return 0;
	}
	return 1;
}

// HOM�p����( 0: �g�p���s 1: �g�p���� )
static int skill_check_condition2_hom(struct homun_data *hd, struct skill_condition *cnd, int type)
{
	int i,hp,sp,hp_rate,sp_rate,zeny,state,skilldb_id;
	struct map_session_data* msd;
	struct block_list *bl;
	int itemid[10],amount[10];

	nullpo_retr(0, hd);
	nullpo_retr(0, cnd);
	nullpo_retr(0, msd = hd->msd);
	nullpo_retr(0, bl = &hd->bl);

	skilldb_id = skill_get_skilldb_id(cnd->id);
	hp         = skill_get_hp(cnd->id, cnd->lv);	/* ����HP */
	sp         = skill_get_sp(cnd->id, cnd->lv);	/* ����SP */
	hp_rate    = (cnd->lv <= 0)? 0: skill_db[skilldb_id].hp_rate[cnd->lv-1];
	sp_rate    = (cnd->lv <= 0)? 0: skill_db[skilldb_id].sp_rate[cnd->lv-1];
	zeny       = skill_get_zeny(cnd->id,cnd->lv);
	state      = skill_db[skilldb_id].state;

	for(i=0; i<10; i++) {
		itemid[i] = skill_db[skilldb_id].itemid[i];
		amount[i] = skill_db[skilldb_id].amount[i];
	}

	if(hp_rate > 0)
		hp += hd->status.hp * hp_rate / 100;
	else
		hp += hd->status.max_hp * abs(hp_rate) / 100;
	if(sp_rate > 0)
		sp += hd->status.sp * sp_rate / 100;
	else
		sp += hd->status.max_sp * abs(sp_rate) / 100;

	switch(cnd->id) {
		case HLIF_HEAL:
		case HLIF_AVOID:
			if(hd->sc.data[SC_CHANGE].timer != -1)
				return 0;
			break;
	}

	if(!(type&2)) {
		if(hp > 0 && hd->status.hp < hp)	/* HP�`�F�b�N */
			return 0;
		if(sp > 0 && hd->status.sp < sp)	/* SP�`�F�b�N */
			return 0;
		if(zeny > 0 && msd->status.zeny < zeny)
			return 0;

		switch(cnd->id)
		{
			case HFLI_SBR44:	/* S.B.R.44 */
				if(hd->intimate < 200)
					return 0;
				break;
			case HVAN_EXPLOSION:	/* �o�C�I�G�N�X�v���[�W���� */
				if(hd->intimate < battle_config.hvan_explosion_intimate)
					return 0;
				break;
		}
	}

	switch(state) {
	case SST_SIGHT:
		if(hd->sc.data[SC_SIGHT].timer == -1) {
			return 0;
		}
		break;
	case SST_EXPLOSIONSPIRITS:
		if(hd->sc.data[SC_EXPLOSIONSPIRITS].timer == -1) {
			return 0;
		}
		break;
	case SST_CARTBOOST:
		if(hd->sc.data[SC_CARTBOOST].timer == -1) {
			return 0;
		}
		break;
	case SST_NEN:
		if(hd->sc.data[SC_NEN].timer == -1) {
			return 0;
		}
		break;
	case SST_MOVE_ENABLE:
		if(path_search(NULL,bl->m,bl->x,bl->y,cnd->x,cnd->y,1) == -1) {
			return 0;
		}
		break;
	case SST_WATER:
		if(!map[bl->m].flag.rain) {
			// ���ꔻ��
			if( !map_getcell(bl->m,bl->x,bl->y,CELL_CHKWATER) &&
			    hd->sc.data[SC_DELUGE].timer == -1 &&
			    hd->sc.data[SC_SUITON].timer == -1 )
				return 0;
		}
		break;
	}

	if(skill_item_consume(&hd->bl, cnd, type, itemid, amount) == 0)
		return 0;

	if(type == 1) {
		if(sp > 0) {				// SP����
			hd->status.sp -= sp;
			clif_send_homstatus(msd,0);
		}
		if(hp > 0) {				// HP����
			hd->status.hp -= hp;
			clif_send_homstatus(msd,0);
		}
		if(zeny > 0)				// Zeny����
			pc_payzeny(msd,zeny);
	}
	return 1;
}

// MERC�p����( 0: �g�p���s 1: �g�p���� )
static int skill_check_condition2_merc(struct merc_data *mcd, struct skill_condition *cnd, int type)
{
	int i,hp,sp,hp_rate,sp_rate,zeny,state,skilldb_id;
	struct map_session_data* msd;
	struct block_list *bl;
	int itemid[10],amount[10];

	nullpo_retr(0, mcd);
	nullpo_retr(0, cnd);
	nullpo_retr(0, msd = mcd->msd);
	nullpo_retr(0, bl = &mcd->bl);

	skilldb_id = skill_get_skilldb_id(cnd->id);
	hp         = skill_get_hp(cnd->id, cnd->lv);	/* ����HP */
	sp         = skill_get_sp(cnd->id, cnd->lv);	/* ����SP */
	hp_rate    = (cnd->lv <= 0)? 0: skill_db[skilldb_id].hp_rate[cnd->lv-1];
	sp_rate    = (cnd->lv <= 0)? 0: skill_db[skilldb_id].sp_rate[cnd->lv-1];
	zeny       = skill_get_zeny(cnd->id,cnd->lv);
	state      = skill_db[skilldb_id].state;

	for(i=0; i<10; i++) {
		itemid[i] = skill_db[skilldb_id].itemid[i];
		amount[i] = skill_db[skilldb_id].amount[i];
	}

	if(hp_rate > 0)
		hp += mcd->status.hp * hp_rate / 100;
	else
		hp += mcd->status.max_hp * abs(hp_rate) / 100;
	if(sp_rate > 0)
		sp += mcd->status.sp * sp_rate / 100;
	else
		sp += mcd->status.max_sp * abs(sp_rate) / 100;

	if(!(type&2)) {
		if(hp > 0 && mcd->status.hp < hp)	/* HP�`�F�b�N */
			return 0;
		if(sp > 0 && mcd->status.sp < sp)	/* SP�`�F�b�N */
			return 0;
		if(zeny > 0 && msd->status.zeny < zeny)
			return 0;
	}

	switch(state) {
	case SST_SIGHT:
		if(mcd->sc.data[SC_SIGHT].timer == -1) {
			return 0;
		}
		break;
	case SST_EXPLOSIONSPIRITS:
		if(mcd->sc.data[SC_EXPLOSIONSPIRITS].timer == -1) {
			return 0;
		}
		break;
	case SST_CARTBOOST:
		if(mcd->sc.data[SC_CARTBOOST].timer == -1) {
			return 0;
		}
		break;
	case SST_NEN:
		if(mcd->sc.data[SC_NEN].timer == -1) {
			return 0;
		}
		break;
	case SST_MOVE_ENABLE:
		if(path_search(NULL,bl->m,bl->x,bl->y,cnd->x,cnd->y,1) == -1) {
			return 0;
		}
		break;
	case SST_WATER:
		if(!map[bl->m].flag.rain) {
			// ���ꔻ��
			if( !map_getcell(bl->m,bl->x,bl->y,CELL_CHKWATER) &&
			    mcd->sc.data[SC_DELUGE].timer == -1 &&
			    mcd->sc.data[SC_SUITON].timer == -1 )
				return 0;
		}
		break;
	}

	if(skill_item_consume(&mcd->bl, cnd, type, itemid, amount) == 0)
		return 0;

	if(type == 1) {
		if(sp > 0) {				// SP����
			mcd->status.sp -= sp;
			clif_send_mercstatus(msd,0);
		}
		if(hp > 0) {				// HP����
			mcd->status.hp -= hp;
			clif_send_mercstatus(msd,0);
		}
		if(zeny > 0)				// Zeny����
			pc_payzeny(msd,zeny);
	}
	return 1;
}

/*==========================================
 * �X�L���ɂ��A�C�e������
 *------------------------------------------
 */
static int skill_item_consume(struct block_list *bl, struct skill_condition *cnd, int type, int *itemid, int *amount)
{
	struct map_session_data *sd = NULL;
	struct status_change *sc;
	int i, idx[10];

	nullpo_retr(0, bl);
	nullpo_retr(0, cnd);

	if(bl->type == BL_PC)
		sd = (struct map_session_data *)bl;
	else if(bl->type == BL_HOM)
		sd = ((struct homun_data *)bl)->msd;
	else if(bl->type == BL_MERC)
		sd = ((struct merc_data *)bl)->msd;

	if(sd == NULL)
		return 0;

	sc = status_get_sc(bl);

	for(i=0; i<10; i++) {
		int x = (cnd->lv > 10)? 9: cnd->lv - 1;

		idx[i] = -1;
		if(itemid[i] <= 0)
			continue;

		if(cnd->id != HW_GANBANTEIN) {
			if(itemid[i] >= 715 && itemid[i] <= 717) {
				if(sd->special_state.no_gemstone || (sc && sc->data[SC_WIZARD].timer != -1))
					continue;
			}
			if((itemid[i] >= 715 && itemid[i] <= 717) || itemid[i] == 1065) {
				if(sc && sc->data[SC_INTOABYSS].timer != -1)
					continue;
			}
		}
		if((cnd->id == AM_POTIONPITCHER || cnd->id == CR_SLIMPITCHER || cnd->id == CR_CULTIVATION) && i != x)
			continue;

		idx[i] = pc_search_inventory(sd,itemid[i]);
		if(idx[i] < 0 || sd->status.inventory[idx[i]].amount < amount[i]) {
			if(itemid[i] == 716 || itemid[i] == 717)
				clif_skill_fail(sd,cnd->id,(7+(itemid[i]-716)),0);
			else
				clif_skill_fail(sd,cnd->id,0,0);
			return 0;
		}
		if(cnd->id == MG_STONECURSE && cnd->lv >= 6 && itemid[i] >= 715 && itemid[i] <= 717) {
			// �X�g�[���J�[�XLv6�ȏ�̓W�F������Ȃ��ɂ��Ă���
			idx[i] = -1;
		}
	}

	if(type&1 && (cnd->id != AL_WARP || type&2)) {
		if(cnd->id != AM_POTIONPITCHER && cnd->id != CR_SLIMPITCHER) {
			for(i=0; i<10; i++) {
				if(idx[i] >= 0)
					pc_delitem(sd,idx[i],amount[i],0);	// �A�C�e������
			}
		}
	}

	return 1;
}

/*==========================================
 * �r�����Ԍv�Z
 *------------------------------------------
 */
int skill_castfix(struct block_list *bl, int skillid, int casttime, int fixedtime)
{
	struct status_change *sc;
	struct map_session_data *sd = NULL;

	nullpo_retr(0, bl);

	if(bl->type == BL_PC)
		sd = (struct map_session_data *)bl;

	sc = status_get_sc(bl);

	// ���@�͑����̌��ʏI��
	if(sc && sc->data[SC_MAGICPOWER].timer != -1) {
		if(sc->data[SC_MAGICPOWER].val2 > 0) {
			/* �ŏ��ɒʂ������ɂ̓A�C�R���������� */
			sc->data[SC_MAGICPOWER].val2--;
			clif_status_change(bl, SI_MAGICPOWER, 0);
		} else {
			status_change_end(bl, SC_MAGICPOWER, -1);
		}
	}

	if(casttime > 0) {
		/* �T�t���M�E�� */
		if(sc && sc->data[SC_SUFFRAGIUM].timer != -1) {
			casttime = casttime * (100 - sc->data[SC_SUFFRAGIUM].val1 * 15) / 100;
			status_change_end(bl, SC_SUFFRAGIUM, -1);
		}

		// dex�̉e�����v�Z����
		if(bl->type != BL_MOB) {
			int dex = status_get_dex(bl);
			if(battle_config.no_cast_dex > dex) {
				casttime = casttime * (battle_config.no_cast_dex - dex) / battle_config.no_cast_dex;
				if(sd)
					casttime = casttime * sd->castrate * battle_config.cast_rate / 10000;
			} else {
				casttime = 0;
			}
		}

		if(sc) {
			int type = -1;

			/* �u���M�̎� */
			if(sc->data[SC_POEMBRAGI].timer != -1)
				type = SC_POEMBRAGI;
			else if(sc->data[SC_POEMBRAGI_].timer != -1)
				type = SC_POEMBRAGI_;

			if(type >= 0) {
				casttime = casttime * (100-(sc->data[type].val1*3+sc->data[type].val2+(sc->data[type].val3>>16))) / 100;
			}

			/* �X���E�L���X�g */
			if(sc->data[SC_SLOWCAST].timer != -1)
				casttime = casttime * (100 + sc->data[SC_SLOWCAST].val1 * 20) / 100;
		}
	}
	if(casttime < 0)
		casttime = 0;

	if(fixedtime > 0) {
		// �J�[�h�ɂ��Œ�r�����Ԍ�������
		if(sd && sd->skill_fixcastrate.count > 0) {
			int i;
			for(i=0; i<sd->skill_fixcastrate.count; i++) {
				if(skillid == sd->skill_fixcastrate.id[i])
					fixedtime -= fixedtime * sd->skill_fixcastrate.rate[i] / 100;
			}
		}
	}
	if(fixedtime < 0)
		fixedtime = 0;

	return casttime + fixedtime;
}

/*==========================================
 * �f�B���C�v�Z
 *------------------------------------------
 */
int skill_delayfix(struct block_list *bl, int delay, int cast)
{
	struct status_change *sc;

	nullpo_retr(0, bl);

	if(delay <= 0 && cast <= 0)
		return ( status_get_adelay(bl) / 2 );

	sc = status_get_sc(bl);

	if(bl->type == BL_PC) {
		struct map_session_data *sd = (struct map_session_data *)bl;
		if(battle_config.delay_dependon_dex) {	// dex�̉e�����v�Z����
			if(battle_config.no_delay_dex > status_get_dex(bl)) {
				delay = delay * (battle_config.no_delay_dex - status_get_dex(bl)) / battle_config.no_delay_dex;
			} else {
				delay = 0;
			}
		}
		delay = delay * battle_config.delay_rate / 100;
		if(sd && sd->skill_delay_rate)
			delay = delay * (100 + sd->skill_delay_rate) / 100;
	}

	/* �u���M�̎� */
	if(sc) {
		if(sc->data[SC_POEMBRAGI].timer != -1) {
			delay = delay * (100 - (sc->data[SC_POEMBRAGI].val1 * 5 + sc->data[SC_POEMBRAGI].val2 * 2
					+ (sc->data[SC_POEMBRAGI].val3 & 0xffff))) / 100;
		} else if(sc->data[SC_POEMBRAGI_].timer != -1) {
			delay = delay * (100 - (sc->data[SC_POEMBRAGI_].val1 * 5 + sc->data[SC_POEMBRAGI_].val2 * 2
					+ (sc->data[SC_POEMBRAGI_].val3 & 0xffff))) / 100;
		}
	}

	return (delay > 0) ? delay : 0;
}

/*=========================================
 * �u�����f�B�b�V���X�s�A �����͈͌���
 *----------------------------------------
 */
static void skill_brandishspear_first(struct square *tc,int dir,int x,int y)
{
	nullpo_retv(tc);

	if(dir == 0){
		tc->val1[0]=x-2;
		tc->val1[1]=x-1;
		tc->val1[2]=x;
		tc->val1[3]=x+1;
		tc->val1[4]=x+2;
		tc->val2[0]=
		tc->val2[1]=
		tc->val2[2]=
		tc->val2[3]=
		tc->val2[4]=y-1;
	}
	else if(dir==2){
		tc->val1[0]=
		tc->val1[1]=
		tc->val1[2]=
		tc->val1[3]=
		tc->val1[4]=x+1;
		tc->val2[0]=y+2;
		tc->val2[1]=y+1;
		tc->val2[2]=y;
		tc->val2[3]=y-1;
		tc->val2[4]=y-2;
	}
	else if(dir==4){
		tc->val1[0]=x-2;
		tc->val1[1]=x-1;
		tc->val1[2]=x;
		tc->val1[3]=x+1;
		tc->val1[4]=x+2;
		tc->val2[0]=
		tc->val2[1]=
		tc->val2[2]=
		tc->val2[3]=
		tc->val2[4]=y+1;
	}
	else if(dir==6){
		tc->val1[0]=
		tc->val1[1]=
		tc->val1[2]=
		tc->val1[3]=
		tc->val1[4]=x-1;
		tc->val2[0]=y+2;
		tc->val2[1]=y+1;
		tc->val2[2]=y;
		tc->val2[3]=y-1;
		tc->val2[4]=y-2;
	}
	else if(dir==1){
		tc->val1[0]=x-1;
		tc->val1[1]=x;
		tc->val1[2]=x+1;
		tc->val1[3]=x+2;
		tc->val1[4]=x+3;
		tc->val2[0]=y-4;
		tc->val2[1]=y-3;
		tc->val2[2]=y-1;
		tc->val2[3]=y;
		tc->val2[4]=y+1;
	}
	else if(dir==3){
		tc->val1[0]=x+3;
		tc->val1[1]=x+2;
		tc->val1[2]=x+1;
		tc->val1[3]=x;
		tc->val1[4]=x-1;
		tc->val2[0]=y-1;
		tc->val2[1]=y;
		tc->val2[2]=y+1;
		tc->val2[3]=y+2;
		tc->val2[4]=y+3;
	}
	else if(dir==5){
		tc->val1[0]=x+1;
		tc->val1[1]=x;
		tc->val1[2]=x-1;
		tc->val1[3]=x-2;
		tc->val1[4]=x-3;
		tc->val2[0]=y+3;
		tc->val2[1]=y+2;
		tc->val2[2]=y+1;
		tc->val2[3]=y;
		tc->val2[4]=y-1;
	}
	else if(dir==7){
		tc->val1[0]=x-3;
		tc->val1[1]=x-2;
		tc->val1[2]=x-1;
		tc->val1[3]=x;
		tc->val1[4]=x+1;
		tc->val2[1]=y;
		tc->val2[0]=y+1;
		tc->val2[2]=y-1;
		tc->val2[3]=y-2;
		tc->val2[4]=y-3;
	}
}

/*=========================================
 * �u�����f�B�b�V���X�s�A �������� �͈͊g��
 *-----------------------------------------
 */
static void skill_brandishspear_dir(struct square *tc,int dir,int are)
{
	int c;

	nullpo_retv(tc);

	for(c=0;c<5;c++){
		if(dir==0){
			tc->val2[c]+=are;
		}else if(dir==1){
			tc->val1[c]-=are; tc->val2[c]+=are;
		}else if(dir==2){
			tc->val1[c]-=are;
		}else if(dir==3){
			tc->val1[c]-=are; tc->val2[c]-=are;
		}else if(dir==4){
			tc->val2[c]-=are;
		}else if(dir==5){
			tc->val1[c]+=are; tc->val2[c]-=are;
		}else if(dir==6){
			tc->val1[c]+=are;
		}else if(dir==7){
			tc->val1[c]+=are; tc->val2[c]+=are;
		}
	}
}

/*----------------------------------------------------------------------------
 * �ʃX�L���̊֐�
 */

/*==========================================
 * �f�B�{�[�V�����L���̑��m�F
 *------------------------------------------
 */
void skill_devotion(struct map_session_data *msd)
{
	int n;

	nullpo_retv(msd);

	for(n=0; n<5; n++) {
		if(msd->dev.val1[n]) {
			struct map_session_data *sd = map_id2sd(msd->dev.val1[n]);
			// ���肪������Ȃ� or ������f�B�{���Ă�̂���������Ȃ� or ����������Ă�
			if( sd == NULL || msd->bl.id != sd->sc.data[SC_DEVOTION].val1 || skill_devotion3(msd,msd->dev.val1[n]) ) {
				msd->dev.val1[n] = 0;
				msd->dev.val2[n] = 0;
				if(sd && sd->sc.data[SC_DEVOTION].timer != -1 && sd->sc.data[SC_DEVOTION].val1)
					status_change_end(&sd->bl, SC_DEVOTION, -1);
				clif_devotion(msd);
			}
		}
	}
}

/*==========================================
 * ��f�B�{�[�V���������������̋����`�F�b�N
 *------------------------------------------
 */
void skill_devotion2(struct block_list *bl,int crusader)
{
	struct map_session_data *sd;

	nullpo_retv(bl);

	if((sd = map_id2sd(crusader)) != NULL)
		skill_devotion3(sd,bl->id);
}

/*==========================================
 * �N���Z�����������̋����`�F�b�N
 *------------------------------------------
 */
int skill_devotion3(struct map_session_data *msd,int target_id)
{
	struct map_session_data *sd;

	nullpo_retr(1, msd);

	if((sd = map_id2sd(target_id)) == NULL)
		return 1;

	if(unit_distance2(&msd->bl, &sd->bl) > pc_checkskill(msd,CR_DEVOTION) + 6) {	// ���e�͈͂𒴂��Ă�
		int n;
		for(n=0; n<5; n++) {
			if(msd->dev.val1[n] == sd->bl.id) {
				if(msd->dev.val2[n]) {
					msd->dev.val2[n] = 0;	// ���ꂽ���́A����؂邾��
					clif_devotion(msd);
				}
				break;
			}
		}
		return 1;
	}
	return 0;
}

/*==========================================
 * �}���I�l�b�g�傪���������̋����`�F�b�N
 *------------------------------------------
 */
int skill_marionette(struct map_session_data *sd,int target_id)
{
	struct map_session_data *tsd;

	nullpo_retr(1, sd);

	if((tsd = map_id2sd(target_id)) == NULL) {
		if(sd->sc.data[SC_MARIONETTE].timer != -1)
			status_change_end(&sd->bl,SC_MARIONETTE,-1);
		return 1;
	}

	if(unit_distance2(&sd->bl, &tsd->bl) > 7) {	// ���e�͈͂𒴂��Ă�
		status_change_end(&sd->bl,SC_MARIONETTE,-1);
		return 1;
	}

	return 0;
}

/*==========================================
 * ��}���I�l�b�g�����������̋����`�F�b�N
 *------------------------------------------
 */
void skill_marionette2(struct map_session_data *dstsd,int src_id)
{
	struct map_session_data *sd;

	nullpo_retv(dstsd);

	if( (sd = map_id2sd(src_id)) != NULL )
		skill_marionette(sd,dstsd->bl.id);
	else
		status_change_end(&dstsd->bl,SC_MARIONETTE2,-1);
}

/*==========================================
 * �I�[�g�X�y��
 *------------------------------------------
 */
void skill_autospell(struct map_session_data *sd, int skillid)
{
	int skilllv, lv;
	int maxlv = 1;

	nullpo_retv(sd);

	skilllv = pc_checkskill(sd,SA_AUTOSPELL);

	switch(skillid) {
		case MG_NAPALMBEAT:
			maxlv = 3;
			break;
		case MG_COLDBOLT:
		case MG_FIREBOLT:
		case MG_LIGHTNINGBOLT:
			if(skilllv == 2)
				maxlv = 1;
			else if(skilllv == 3)
				maxlv = 2;
			else if(skilllv >= 4)
				maxlv = 3;
			break;
		case MG_SOULSTRIKE:
			if(skilllv == 5)
				maxlv = 1;
			else if(skilllv == 6)
				maxlv = 2;
			else if(skilllv >= 7)
				maxlv = 3;
			break;
		case MG_FIREBALL:
			if(skilllv == 8)
				maxlv = 1;
			else if(skilllv >= 9)
				maxlv = 2;
			break;
		case MG_FROSTDIVER:
			maxlv = 1;
			break;
		default:
			return;
	}

	if(maxlv > (lv = pc_checkskill(sd,skillid)))
		maxlv = lv;

	// if player doesn't have the skill (hacker?)
	if(lv == 0)
		return;

	status_change_start(&sd->bl,SC_AUTOSPELL,skilllv,skillid,maxlv,0,	// val1:�X�L��ID val2:�g�p�ő�Lv
		skill_get_time(SA_AUTOSPELL,skilllv),0);// �ɂ��Ă݂�����bscript�������Ղ��E�E�E�H

	return;
}

/*==========================================
 * �M�����O�X�^�[�p���_�C�X����уe�R���x��
 * ���菈�����菈��
 *------------------------------------------
 */
static int skill_sit_count(struct block_list *bl,va_list ap)
{
	int *c;
	int flag;
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd = (struct map_session_data *)bl);

	flag = va_arg(ap,int);
	c    = va_arg(ap,int *);

	if(!pc_issit(sd))
		return 0;

	if(flag&1 && pc_checkskill(sd,RG_GANGSTER) > 0)
		(*c)++;
	else if(flag&2 && sd->s_class.job >= 24 && sd->s_class.job <= 27)
		(*c)++;

	return 0;
}

static int skill_sit_in(struct block_list *bl,va_list ap)
{
	int flag;
	struct map_session_data *sd;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd = (struct map_session_data *)bl);

	flag = va_arg(ap,int);

	if(!pc_issit(sd))
		return 0;

	if(flag&1 && pc_checkskill(sd,RG_GANGSTER) > 0)
		sd->state.gangsterparadise = 1;
	else if(flag&2 && sd->s_class.job >= 24 && sd->s_class.job <= 27)
		sd->state.taekwonrest = 1;

	return 0;
}

static int skill_sit_out(struct block_list *bl,va_list ap)
{
	struct map_session_data *sd;
	int flag;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, sd = (struct map_session_data *)bl);

	flag = va_arg(ap,int);

	if((flag&1 && sd->state.gangsterparadise) || (flag&2 && sd->state.taekwonrest)) {
		int c = 0;
		map_foreachinarea(skill_sit_count,bl->m,
			bl->x-1,bl->y-1,
			bl->x+1,bl->y+1,BL_PC,flag,&c);
		if(c < 2) {
			if(flag&1)
				sd->state.gangsterparadise = 0;
			if(flag&2)
				sd->state.taekwonrest = 0;
		}
	}
	return 0;
}

int skill_sit(struct map_session_data *sd, int type)
{
	int flag = 0;

	nullpo_retr(0, sd);

	if(pc_checkskill(sd,RG_GANGSTER) > 0)
		flag |= 1;
	if(pc_checkskill(sd,TK_HPTIME) > 0 || pc_checkskill(sd,TK_SPTIME) > 0)
		flag |= 2;

	if(!flag)
		return 0;

	if(type) {
		// ���������̏���
		int c = 0;
		map_foreachinarea(skill_sit_count,sd->bl.m,
			sd->bl.x-1,sd->bl.y-1,
			sd->bl.x+1,sd->bl.y+1,BL_PC,flag,&c);
		if(c > 1) {
			// ������������ʕt�^
			map_foreachinarea(skill_sit_in,sd->bl.m,
				sd->bl.x-1,sd->bl.y-1,
				sd->bl.x+1,sd->bl.y+1,BL_PC,flag);
		}
	} else {
		// �����オ�����Ƃ��̏���
		map_foreachinarea(skill_sit_out,sd->bl.m,
			sd->bl.x-1,sd->bl.y-1,
			sd->bl.x+1,sd->bl.y+1,BL_PC,flag);
		if(flag&1)
			sd->state.gangsterparadise = 0;
		if(flag&2)
			sd->state.taekwonrest = 0;
	}

	return 0;
}

/*==========================================
 * �����W���[�N�E�X�N���[�����菈��(foreachinarea)
 *------------------------------------------
 */
static int skill_frostjoke_scream(struct block_list *bl,va_list ap)
{
	struct block_list *src;
	int skillnum,skilllv;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, src=va_arg(ap,struct block_list*));

	skillnum = va_arg(ap,int);
	skilllv  = va_arg(ap,int);
	tick     = va_arg(ap,unsigned int);

	if(src == bl)	// �����ɂ͌����Ȃ�
		return 0;

	if(battle_check_target(src,bl,BCT_ENEMY) > 0) {
		skill_additional_effect(src,bl,skillnum,skilllv,BF_MISC,tick);
	} else if(battle_check_target(src,bl,BCT_PARTY) > 0) {
		if(atn_rand()%100 < 10)	// PT�����o�ɂ���m���ł�����(�Ƃ肠����10%)
			skill_additional_effect(src,bl,skillnum,skilllv,BF_MISC,tick);
	}

	return 0;
}

/*==========================================
 * �A�u���J�_�u���̎g�p�X�L������(����X�L�����_���Ȃ�0��Ԃ�)
 *------------------------------------------
 */
static int skill_abra_dataset(struct map_session_data *sd, int skilllv)
{
	int skill = atn_rand()%MAX_SKILL_ABRA_DB;

	// �Z�[�W�̓]���X�L���g�p�������Ȃ�
	if( battle_config.extended_abracadabra == 0 &&
	    sd->s_class.upper == 0 &&
	    skill_upperskill(skill_abra_db[skill].nameid) )
		return 0;

	// db�Ɋ�Â����x���E�m������
	if(skill_abra_db[skill].req_lv > skilllv || atn_rand()%10000 >= skill_abra_db[skill].per)
		return 0;
	// NPC�E�����E�{�q�E�A�C�e���X�L���̓_��
	if( skill_mobskill( skill_abra_db[skill].nameid ) ||
	   (skill_abra_db[skill].nameid >= WE_BABY && skill_abra_db[skill].nameid <= WE_CALLBABY))
		return 0;

	// ���t�X�L���̓_��
	if (skill_get_unit_flag(skill_abra_db[skill].nameid, skilllv)&UF_DANCE)
		return 0;

	return skill_abra_db[skill].nameid;
}

/*==========================================
 * �o�W���J�̃Z����ݒ肷��
 *------------------------------------------
 */
static void skill_basilica_cell(struct skill_unit *unit,int flag)
{
	int i,x,y;
	int range = skill_get_unit_range(HP_BASILICA);
	int size = range*2+1;

	for (i=0;i<size*size;i++) {
		x = unit->bl.x+(i%size-range);
		y = unit->bl.y+(i/size-range);
		map_setcell(unit->bl.m,x,y,flag);
	}
}

/*==========================================
 * �o�W���J�̔������~�߂�
 *------------------------------------------
 */
void skill_basilica_cancel( struct block_list *bl )
{
	struct unit_data *ud = unit_bl2ud( bl );
	struct linkdb_node *node, *node2;
	struct skill_unit_group   *group;

	nullpo_retv(ud);

	node = ud->skillunit;
	while( node ) {
		node2 = node->next;
		group = (struct skill_unit_group *)node->data;
		if(group->skill_id == HP_BASILICA)
			skill_delunitgroup(group);
		node = node2;
	}
}

/*==========================================
 *
 *------------------------------------------
 */
static int skill_clear_element_field(struct block_list *bl)
{
	struct unit_data *ud = unit_bl2ud( bl );
	struct linkdb_node *node, *node2;
	struct skill_unit_group   *group;
	int skillid;

	nullpo_retr(0, ud);

	node = ud->skillunit;
	while( node ) {
		node2   = node->next;
		group   = (struct skill_unit_group *)node->data;
		skillid = group->skill_id;
		if(skillid==SA_DELUGE || skillid==SA_VOLCANO || skillid==SA_VIOLENTGALE || skillid==SA_LANDPROTECTOR || skillid==NJ_SUITON || skillid == NJ_KAENSIN)
			skill_delunitgroup(group);
		node = node2;
	}
	return 0;
}

/*==========================================
 * �����h�v���e�N�^�[�`�F�b�N(foreachinarea)
 *------------------------------------------
 */
static int skill_landprotector(struct block_list *bl, va_list ap )
{
	int skillid;
	int *alive;
	struct skill_unit *unit;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, unit = (struct skill_unit *)bl);

	skillid = va_arg(ap,int);
	alive   = va_arg(ap,int *);

	if(skillid==SA_LANDPROTECTOR){
		if(alive && unit->group->skill_id==SA_LANDPROTECTOR)
			(*alive)=0;
		if((unit->group->skill_id!=BA_DISSONANCE &&    // �s���a��
		    unit->group->skill_id!=BA_WHISTLE &&       // ���J
		    unit->group->skill_id!=BA_ASSASSINCROSS && // �[�z�̃A�T�V���N���X
		    unit->group->skill_id!=BA_POEMBRAGI &&     // �u���M�̎�
		    unit->group->skill_id!=BA_APPLEIDUN &&     // �C�h�D���̗ь�
		    unit->group->skill_id!=DC_UGLYDANCE &&     // ��������ȃ_���X
		    unit->group->skill_id!=DC_HUMMING &&       // �n�~���O
		    unit->group->skill_id!=DC_DONTFORGETME &&  // ����Y��Ȃ��Łc
		    unit->group->skill_id!=DC_FORTUNEKISS &&   // �K�^�̃L�X
		    unit->group->skill_id!=DC_SERVICEFORYOU))  // �T�[�r�X�t�H�[���[
			skill_delunit(unit);
	}else if(skillid == PF_FOGWALL){
		if(alive && (unit->group->skill_id==SA_LANDPROTECTOR ||
			unit->group->skill_id == SA_VIOLENTGALE || unit->group->skill_id == SA_VOLCANO))
			(*alive)=0;
	}else{
		if(alive && unit->group->skill_id==SA_LANDPROTECTOR)
			(*alive)=0;
	}
	return 0;
}

/*==========================================
 * �C�h�D���̗ь�̉񕜏���(foreachinarea)
 *------------------------------------------
 */
static int skill_idun_heal(struct block_list *bl, va_list ap )
{
	struct skill_unit *unit;
	struct skill_unit_group *sg;
	struct block_list *src;
	int heal;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, unit = va_arg(ap,struct skill_unit *));
	nullpo_retr(0, src = va_arg(ap,struct block_list *));
	nullpo_retr(0, sg = unit->group);

	if(bl->id == sg->src_id)
		return 0;

	if(bl->type != BL_PC && bl->type != BL_MOB)
		return 0;

	heal = 30+sg->skill_lv*5+((sg->val1)>>16)*5+((sg->val2)&0xfff)/2;
	heal = skill_fix_heal(src, bl, sg->skill_id, heal);

	clif_skill_nodamage(&unit->bl,bl,AL_HEAL,heal,1);
	battle_heal(NULL,bl,heal,0,0);

	return 0;
}

/*==========================================
 * �^���̃^���b�g�J�[�h
 *------------------------------------------
 */
static int skill_tarot_card_of_fate(struct block_list *src,struct block_list *target,int skillid,int skilllv,int wheel)
{
	struct map_session_data* tsd=NULL;
	struct mob_data* tmd=NULL;
	int card_num,rate;

	nullpo_retr(0, src);
	nullpo_retr(0, target);

	if(target->type != BL_PC && target->type != BL_MOB)
		return 0;
	if(status_get_class(target) == 1288)
		return 0;

	// �^���̗ւ���100%����
	if(wheel == 0 && atn_rand()%100 >= skilllv*8)
		return 0;

	tsd = BL_DOWNCAST( BL_PC,  target );
	tmd = BL_DOWNCAST( BL_MOB, target );

	rate = atn_rand()%10000;

	// ���v�T�C�g���Q�l�ɓK���Ɋm���ݒ�
	if(rate < 1022)      card_num =  0;	// 10.22%
	else if(rate < 1911) card_num =  1;	//  8.89%
	else if(rate < 3068) card_num =  2;	// 11.57%
	else if(rate < 3839) card_num =  3;	//  7.71%
	else if(rate < 4954) card_num =  4;	// 11.15%
	else if(rate < 6454) card_num =  5;	// 15.00%
	else if(rate < 6513) card_num =  6;	//  0.59%
	else if(rate < 7150) card_num =  7;	//  6.37%
	else if(rate < 7636) card_num =  8;	//  4.86%
	else if(rate < 8374) card_num =  9;	//  7.38%
	else if(rate < 8458) card_num = 10;	//  0.84%
	else if(rate < 8642) card_num = 11;	//  1.84%
	else if(rate < 9036) card_num = 12;	//  3.94%
	else                 card_num = 13;	//  9.64%

	if(wheel == 0)	// �^���̗ւ��ƃG�t�F�N�g�Ȃ��H
	{
		switch(battle_config.tarotcard_display_position)
		{
			case 1:
				clif_misceffect2(src,523+card_num);
				break;
			case 2:
				clif_misceffect2(target,523+card_num);
				break;
			case 3:
				clif_misceffect2(src,523+card_num);
				clif_misceffect2(target,523+card_num);
				break;
			default:
				break;
		}
	}

	switch(card_num)
	{
		case 0:
			/* ����(The Fool) - SP��0�ɂȂ� */
			if(tsd) {
				tsd->status.sp = 0;
				clif_updatestatus(tsd,SP_SP);
			}
			break;
		case 1:
			/* ���@�t(The Magician) - 30�b��Matk�������ɗ����� */
			if(!(status_get_mode(target)&0x20))	// �{�X�����ȊO
				status_change_start(target,SC_THE_MAGICIAN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
			break;
		case 2:
			/* �����c(The High Priestess) - ���ׂĂ̕⏕���@�������� */
			status_change_release(target,0x40);
			break;
		case 3:
			/* ���(The Chariot) - �h��͖�����1000�_���[�W �h������_���Ɉ�j�󂳂�� */
			if(tsd){
				switch(atn_rand()%4) {
					case 0: pc_break_equip(tsd,EQP_WEAPON); break;
					case 1: pc_break_equip(tsd,EQP_ARMOR);  break;
					case 2: pc_break_equip(tsd,EQP_SHIELD); break;
					case 3: pc_break_equip(tsd,EQP_HELM);   break;
				}
			}
			unit_fixdamage(src,target,0, 0, 0,1000,1, 4, 0);
			break;
		case 4:
			/* ��(Strength) - 30�b��ATK�������ɗ����� */
			if(!(status_get_mode(target)&0x20))	// �{�X�����ȊO
				status_change_start(target,SC_STRENGTH,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
			break;
		case 5:
			/* ���l(The Lovers) - �ǂ����Ƀe���|�[�g������- HP��2000�񕜂���� */
			unit_heal(target, 2000, 0);
			// �e���|�[�g�s�̏ꍇ�͉񕜂̂�
			if(tsd) {
				if(!map[tsd->bl.m].flag.noteleport)
					pc_randomwarp(tsd,0);
			} else if(tmd) {
				if(!map[tmd->bl.m].flag.monster_noteleport)
					mob_warp(tmd,tmd->bl.m,-1,-1,0);
			}
			break;
		case 6:
			/* �^���̗�(Wheel of Fortune) - �����_���ɑ��̃^���b�g�J�[�h�񖇂̌��ʂ𓯎��ɗ^���� */
			if(wheel > 0 && wheel < 50) {	// ����1�x���s�i50��őł��؂�j
				skill_tarot_card_of_fate(src,target,skillid,skilllv,wheel+1);
			} else {			// �Q���s
				skill_tarot_card_of_fate(src,target,skillid,skilllv,1);
				skill_tarot_card_of_fate(src,target,skillid,skilllv,1);
			}
			break;
		case 7:
			/* �݂�ꂽ�j(The Hanged Man) - �����A�����A�Ή��̒������������������� */
			if(!(status_get_mode(target)&0x20))	// �{�X�����ȊO
			{
				switch(atn_rand()%3)
				{
					case 0:	// ����
						status_change_start(target,SC_SLEEP,7,0,0,0,skill_get_time2(NPC_SLEEPATTACK,7),0);
						break;
					case 1:	// ����
						status_change_start(target,SC_FREEZE,7,0,0,0,skill_get_time2(MG_FROSTDIVER,7),0);
						break;
					case 2:	// �Ή�
						status_change_start(target,SC_STONE,7,0,0,0,skill_get_time2(MG_STONECURSE,7),0);
						break;
				}
			}
			break;
		case 8:
			/* ���_(Death) - �� + �R�[�} + �łɂ����� */
			status_change_start(target,SC_CURSE,7,0,0,0,skill_get_time2(NPC_CURSEATTACK,7),0);
			status_change_start(target,SC_POISON,7,0,0,0,skill_get_time2(TF_POISON,7),0);
			// �R�[�}
			if(tsd) {
				tsd->status.hp = 1;
				clif_updatestatus(tsd,SP_HP);
			} else if(tmd && !(status_get_mode(&tmd->bl)&0x20)) {	// �{�X�����ȊO
				tmd->hp = 1;
			}
			break;
		case 9:
			/* �ߐ�(Temperance) - 30�b�ԍ����ɂ����� */
			if(!(status_get_mode(target)&0x20))	// �{�X�����ȊO
				status_change_start(target,SC_CONFUSION,7,0,0,0,30000,0);
			break;
		case 10:
			/* ����(The Devil) - �h��͖���6666�_���[�W + 30�b��ATK�����AMATK�����A�� */
			status_change_start(target,SC_CURSE,7,0,0,0,skill_get_time2(NPC_CURSEATTACK,7),0);
			if(!(status_get_mode(target)&0x20))	// �{�X�����ȊO
				status_change_start(target,SC_THE_DEVIL,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
			unit_fixdamage(src,target,0, 0, 0,6666,1, 4, 0);
			break;
		case 11:
			/* ��(The Tower) - �h��͖���4444�Œ�_���[�W */
			unit_fixdamage(src,target,0, 0, 0,4444,1, 4, 0);
			break;
		case 12:
			/* ��(The Star) - ������� ���Ȃ킿�A5�b�ԃX�^���ɂ����� */
			status_change_start(target,SC_STUN,7,0,0,0,5000,0);
			break;
		case 13:
			/* ���z(The Sun) - 30�b��ATK�AMATK�A����A�����A�h��͂��S��20%���������� */
			if(!(status_get_mode(target)&0x20))	// �{�X�����ȊO
				status_change_start(target,SC_THE_SUN,skilllv,0,0,0,skill_get_time2(skillid,skilllv),0);
			break;
	}
	return 1;
}

/*==========================================
 * �w��͈͓���src�ɑ΂��ėL���ȃ^�[�Q�b�g��bl�̐��𐔂���(foreachinarea)
 *------------------------------------------
 */
static int skill_count_target(struct block_list *bl, va_list ap )
{
	struct skill_unit *unit;
	int *c;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	if((unit = va_arg(ap,struct skill_unit *)) == NULL)
		return 0;
	if((c = va_arg(ap,int *)) == NULL)
		return 0;
	if(battle_check_target(&unit->bl,bl,BCT_ENEMY) > 0)
		(*c)++;
	return 0;
}

/*==========================================
 * �g���b�v�͈͏���(foreachinarea)
 *------------------------------------------
 */
static int skill_trap_splash(struct block_list *bl, va_list ap )
{
	int tick;
	int splash_count;
	struct skill_unit *unit;
	struct skill_unit_group *sg;
	struct block_list *ss;
	int i;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, unit = va_arg(ap,struct skill_unit *));
	nullpo_retr(0, sg = unit->group);
	nullpo_retr(0, ss = map_id2bl(sg->src_id));

	tick = va_arg(ap,int);
	splash_count = va_arg(ap,int);

	if(battle_check_target(&unit->bl,bl,BCT_ENEMY) > 0){
		switch(sg->unit_id){
			case UNT_SANDMAN:	/* �T���h�}�� */
			case UNT_FLASHER:	/* �t���b�V���[ */
			case UNT_SHOCKWAVE:	/* �V���b�N�E�F�[�u�g���b�v */
				skill_additional_effect(ss,bl,sg->skill_id,sg->skill_lv,BF_MISC,tick);
				break;
			case UNT_BLASTMINE:	/* �u���X�g�}�C�� */
			case UNT_CLAYMORETRAP:	/* �N���C���A�[�g���b�v */
				for(i=0;i<splash_count;i++){
					battle_skill_attack(BF_MISC,ss,&unit->bl,bl,sg->skill_id,sg->skill_lv,tick,(sg->val2)?0x0500:0);
				}
				break;
			case UNT_FREEZINGTRAP:	/* �t���[�W���O�g���b�v */
				battle_skill_attack(BF_MISC,ss,&unit->bl,bl,sg->skill_id,sg->skill_lv,tick,(sg->val2)?0x0500:0);
				break;
			default:
				break;
		}
	}

	return 0;
}

/*----------------------------------------------------------------------------
 * �X�e�[�^�X�ُ�
 *----------------------------------------------------------------------------
 */

/* �N���[�L���O�����i����Ɉړ��s�\�n�т����邩�j */
int skill_check_cloaking(struct block_list *bl)
{
	int i;
	struct status_change *sc;

	nullpo_retr(0, bl);

	if(bl->type == BL_PC && battle_config.pc_cloak_check_type&1)
		return 0;
	if(bl->type == BL_MOB && battle_config.monster_cloak_check_type&1)
		return 0;

	for(i=0;i<8;i++){
		if(map_getcell(bl->m,bl->x+dirx[i],bl->y+diry[i],CELL_CHKNOPASS))
			return 0;
	}
	status_change_end(bl, SC_CLOAKING, -1);
	sc = status_get_sc(bl);
	if(sc)
		sc->option &= ~4;	/* �O�̂��߂̏��� */

	return 1;
}

/*
 *----------------------------------------------------------------------------
 * �X�L�����j�b�g
 *----------------------------------------------------------------------------
 */

/*==========================================
 * ���t/�_���X����߂�
 * flag 1�ō��t���Ȃ瑊���Ƀ��j�b�g��C����
 *------------------------------------------
 */
void skill_stop_dancing(struct block_list *src, int flag)
{
	struct map_session_data *sd = NULL;
	struct status_change* sc;
	struct skill_unit_group* group;

	nullpo_retv(src);

	sc=status_get_sc(src);
	if(sc==NULL)
		return;
	if(sc->data[SC_DANCING].timer==-1)
		return;

	if(src->type == BL_PC)
		sd = (struct map_session_data *)src;

	group=(struct skill_unit_group *)sc->data[SC_DANCING].val2; // �_���X�̃X�L�����j�b�gID��val2�ɓ����Ă�

	if(group && sd && sc->data[SC_DANCING].val4){ // ���t���f
		struct map_session_data* dsd=map_id2sd(sc->data[SC_DANCING].val4); // ������sd�擾
		if(flag){ // ���O�A�E�g�ȂǕЕ��������Ă����t���p�������
			if(dsd && src->id == group->src_id){ // �O���[�v�������Ă�PC��������
				group->src_id=sc->data[SC_DANCING].val4; // �����ɃO���[�v��C����
				linkdb_insert( &dsd->ud.skillunit, group, group );
				linkdb_erase( &sd->ud.skillunit, group );
				if(flag&1) // ���O�A�E�g
					dsd->sc.data[SC_DANCING].val4=0; // �����̑�����0�ɂ��č��t�I�����ʏ�̃_���X���
				if(flag&2) // �n�G��тȂ�
					return; // ���t���_���X��Ԃ��I�������Ȃ����X�L�����j�b�g�͒u���Ă��ڂ�
			}else if(dsd && dsd->bl.id == group->src_id){ // �������O���[�v�������Ă���PC��������(�����̓O���[�v�������Ă��Ȃ�)
				if(flag&1) // ���O�A�E�g
					dsd->sc.data[SC_DANCING].val4=0; // �����̑�����0�ɂ��č��t�I�����ʏ�̃_���X���
				if(flag&2) // �n�G��тȂ�
					return; // ���t���_���X��Ԃ��I�������Ȃ����X�L�����j�b�g�͒u���Ă��ڂ�
			}
			status_change_end(src,SC_DANCING,-1);// �����̃X�e�[�^�X���I��������
			// �����ăO���[�v�͏����Ȃ��������Ȃ��̂ŃX�e�[�^�X�v�Z������Ȃ��H
			return;
		}else{
			if(dsd && src->id == group->src_id){ // �O���[�v�������Ă�PC���~�߂�
				status_change_end(&dsd->bl,SC_DANCING,-1);// ����̃X�e�[�^�X���I��������
			}
			if(dsd && dsd->bl.id == group->src_id){ // �������O���[�v�������Ă���PC���~�߂�(�����̓O���[�v�������Ă��Ȃ�)
				status_change_end(src,SC_DANCING,-1);// �����̃X�e�[�^�X���I��������
			}
		}
	}
	if(flag&2 && group && sd) { // �n�G�Ŕ�񂾂Ƃ��Ƃ��̓��j�b�g�����
		skill_unit_move_unit_group(group, sd->bl.m,(sd->ud.to_x - sd->bl.x),(sd->ud.to_y - sd->bl.y));
		return;
	}
	if( group )
		skill_delunitgroup(group);
	if(sd)
		status_calc_pc(sd,0);
}

/*==========================================
 * �w�����[�h�̏�`�F�b�N
 *------------------------------------------
 */
static int skill_hermode_wp_check_sub(struct block_list *bl, va_list ap )
{
	int *flag;
	struct npc_data *nd;

	nullpo_retr(0, bl);
	nullpo_retr(0, nd = (struct npc_data *)bl);

	flag = va_arg(ap,int*);
	if(*flag)
		return 1;

	if(nd->subtype == WARP || (nd->subtype == SCRIPT && nd->class_ == WARP_CLASS))
		*flag = 1;

	return *flag;
}

int skill_hermode_wp_check(struct block_list *bl)
{
	int wp_flag = 0;
	int range   = battle_config.hermode_wp_check_range;

	nullpo_retr(0, bl);

	map_foreachinarea(skill_hermode_wp_check_sub,bl->m,bl->x-range,bl->y-range,bl->x+range,bl->y+range,BL_NPC,&wp_flag);
	return wp_flag;
}

/*==========================================
 * �K���o���e�C���ɂ�郆�j�b�g�폜
 *------------------------------------------
 */
static int skill_delunit_by_ganbantein(struct block_list *bl, va_list ap )
{
	struct skill_unit *unit;

	nullpo_retr(0, bl);
	nullpo_retr(0, unit = (struct skill_unit *)bl);
	nullpo_retr(0, unit->group);

	switch(unit->group->skill_id)
	{
		case SA_LANDPROTECTOR:
		case MG_SAFETYWALL:
		case AL_PNEUMA:
		case AL_WARP:
		case PR_SANCTUARY:
		case PR_MAGNUS:
		case WZ_FIREPILLAR:
		case WZ_ICEWALL:
		case WZ_QUAGMIRE:
		case AS_VENOMDUST:
		case SA_VOLCANO:
		case SA_DELUGE:
		case SA_VIOLENTGALE:
		case PF_FOGWALL:
		case NJ_SUITON:
			skill_delunit(unit);
			break;
	}
	return 0;
}

/*==========================================
 * �X�L�����j�b�g������
 *------------------------------------------
 */
static struct skill_unit *skill_initunit(struct skill_unit_group *group,int idx,int x,int y)
{
	struct skill_unit *unit;

	nullpo_retr(NULL, group);
	nullpo_retr(NULL, unit = &group->unit[idx]);

	if(!unit->alive)
		group->alive_count++;

	unit->bl.id   = map_addobject(&unit->bl);
	unit->bl.type = BL_SKILL;
	unit->bl.m    = group->m;
	unit->bl.x    = x;
	unit->bl.y    = y;
	unit->group   = group;
	unit->val1    = 0;
	unit->val2    = 0;
	unit->alive   = 1;

	map_addblock(&unit->bl);
	clif_skill_setunit(unit);

	if(group->skill_id == HP_BASILICA)
		skill_basilica_cell(unit,CELL_SETBASILICA);

	return unit;
}

/*==========================================
 * �X�L�����j�b�g�폜
 *------------------------------------------
 */
int skill_delunit(struct skill_unit *unit)
{
	struct skill_unit_group *group;

	nullpo_retr(0, unit);
	nullpo_retr(0, group = unit->group);

	if(!unit->alive)
		return 0;

	/* onlimit�C�x���g�Ăяo�� */
	skill_unit_onlimit(unit,gettick());

	/* onout�C�x���g�Ăяo�� */
	if(!unit->range) {
		map_foreachinarea(skill_unit_effect,unit->bl.m,
			unit->bl.x,unit->bl.y,unit->bl.x,unit->bl.y,(BL_PC|BL_MOB|BL_MERC),
			&unit->bl,gettick(),0);
	}

	if(group->skill_id == HP_BASILICA)
		skill_basilica_cell(unit,CELL_CLRBASILICA);

	clif_skill_delunit(unit);

	unit->group = NULL;
	unit->alive = 0;
	map_delobjectnofree(unit->bl.id);
	if(group->alive_count > 0 && (--group->alive_count) <= 0)
		skill_delunitgroup(group);

	return 0;
}

/*==========================================
 * �X�L�����j�b�g�O���[�v������
 *------------------------------------------
 */
static int skill_unit_group_newid = MAX_SKILL;

static struct skill_unit_group *skill_initunitgroup(struct block_list *src,int count,int skillid,int skilllv,int unit_id,unsigned int tick)
{
	struct unit_data *ud = unit_bl2ud(src);
	struct skill_unit_group *group;
	int unit_flag;

	nullpo_retr(NULL, ud);

	group             = (struct skill_unit_group *)aCalloc(1,sizeof(struct skill_unit_group));
	group->src_id     = src->id;
	group->party_id   = status_get_party_id(src);
	group->guild_id   = status_get_guild_id(src);
	group->group_id   = skill_unit_group_newid++;
	group->unit       = (struct skill_unit *)aCalloc(count,sizeof(struct skill_unit));
	group->unit_count = count;
	group->val1       = 0;
	group->val2       = 0;
	group->skill_id   = skillid;
	group->skill_lv   = skilllv;
	group->unit_id    = unit_id;
	group->m          = src->m;
	group->limit      = 10000;
	group->interval   = 1000;
	group->tick       = tick;
	group->valstr     = NULL;
	linkdb_insert( &ud->skillunit, group, group );

	if(skill_unit_group_newid <= 0)
		skill_unit_group_newid = MAX_SKILL;

	unit_flag = skill_get_unit_flag(skillid, skilllv);
	if(unit_flag&UF_DANCE) {
		struct map_session_data *sd = NULL;
		if(src->type == BL_PC && (sd = (struct map_session_data *)src)) {
			sd->skillid_dance = skillid;
			sd->skilllv_dance = skilllv;
		}
		status_change_start(src,SC_DANCING,skillid,(int)group,0,0,skill_get_time(skillid,skilllv)+1000,0);
		// ���t�X�L���͑������_���X��Ԃɂ���
		if(sd && unit_flag&UF_ENSEMBLE) {
			int c = 0;
			map_foreachinarea(skill_check_condition_use_sub,sd->bl.m,
				sd->bl.x-1,sd->bl.y-1,sd->bl.x+1,sd->bl.y+1,BL_PC,sd,&c);
		}
	}
	return group;
}

/*==========================================
 * �X�L�����j�b�g�O���[�v�폜
 *------------------------------------------
 */
int skill_delunitgroup(struct skill_unit_group *group)
{
	struct block_list *src;
	struct unit_data  *ud = NULL;
	int i;

	nullpo_retr(0, group);

	if(group->unit_count <= 0)
		return 0;

	src = map_id2bl(group->src_id);
	if(src)
		ud = unit_bl2ud(src);

	// �_���X�X�L���̓_���X��Ԃ���������
	if(skill_get_unit_flag(group->skill_id,group->skill_lv)&UF_DANCE) {
		if(src)
			status_change_end(src,SC_DANCING,-1);
	}

	// ��Ԉُ�Ƀ��j�b�g�O���[�v���ۑ�����Ă���ꍇ�̓N���A����
	switch(group->unit_id) {
	case UNT_GOSPEL:
	case UNT_GRAFFITI:
	case UNT_WARM:
	case UNT_GRAVITATION:
		if(src) {
			struct status_change *sc = status_get_sc(src);
			int type = SkillStatusChangeTable[group->skill_id];
			if(type >= 0 && sc && sc->data[type].timer != -1) {
				sc->data[type].val4 = 0;
				status_change_end(src,type,-1);
			}
		}
		break;
	}

	if(ud) {
		if( linkdb_erase( &ud->skillunit, group ) == NULL ) {
			// ������Ȃ�����
			return 0;
		}
	}

	group->alive_count = 0;
	if(group->unit != NULL) {
		for(i=0; i<group->unit_count; i++) {
			if(group->unit[i].alive)
				skill_delunit(&group->unit[i]);
		}
	}
	if(group->valstr != NULL) {
		aFree(group->valstr);
		group->valstr = NULL;
	}

	linkdb_final( &group->tickset );
	map_freeblock(group->unit);	/* free()�̑ւ�� */
	map_freeblock(group);

	return 0;
}

/*==========================================
 * �X�L�����j�b�g�O���[�v�S�폜
 *------------------------------------------
 */
int skill_clear_unitgroup(struct block_list *src)
{
	struct skill_unit_group *group;
	struct unit_data *ud;
	struct linkdb_node *node, *node2;

	nullpo_retr(0, src);
	nullpo_retr(0, ud = unit_bl2ud(src));

	node = ud->skillunit;
	while( node ) {
		node2 = node->next;
		group = (struct skill_unit_group *)node->data;
		if(group->src_id == src->id)
			skill_delunitgroup(group);
		node = node2;
	}
	linkdb_final( &ud->skillunit );

	return 0;
}

/*==========================================
 * �X�L�����j�b�g�^�C�}�[���������p(foreachinarea)
 *------------------------------------------
 */
static int skill_unit_timer_sub_onplace(struct block_list *bl, va_list ap)
{
	struct skill_unit *unit;
	struct skill_unit_group *group;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);

	unit = va_arg(ap,struct skill_unit *);
	tick = va_arg(ap,unsigned int);

	if(!unit || !unit->alive)
		return 0;
	if(!(bl->type & (BL_PC | BL_MOB | BL_MERC)))
		return 0;

	nullpo_retr(0, group = unit->group);

	if(battle_check_target(&unit->bl,bl,group->target_flag) <= 0)
		return 0;

	// �ǔ����h�~�̎ː��`�F�b�N
	if(!path_search_long(NULL,bl->m,bl->x,bl->y,unit->bl.x,unit->bl.y))
		return 0;

	skill_unit_onplace_timer(unit,bl,tick);

	return 0;
}

/*==========================================
 * �X�L�����j�b�g�^�C�}�[�����p(foreachobject)
 *------------------------------------------
 */
static int skill_unit_timer_sub( struct block_list *bl, va_list ap )
{
	struct skill_unit *unit;
	struct skill_unit_group *group;
	int range;
	unsigned int tick;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, unit = (struct skill_unit *)bl);
	nullpo_retr(0, group = unit->group);

	tick = va_arg(ap,unsigned int);

	if(!unit->alive)
		return 0;

	range = unit->range;

	/* onplace_timer�C�x���g�Ăяo�� */
	if(range >= 0 && group->interval != -1)
	{
		map_foreachinarea(skill_unit_timer_sub_onplace, bl->m,
			bl->x-range,bl->y-range,bl->x+range,bl->y+range,(BL_PC|BL_MOB|BL_MERC),unit,tick);
		if(!unit->alive)
			return 0;
		// �}�O�k�X�͔����������j�b�g�͍폜����
		if(group->skill_id == PR_MAGNUS && unit->val2) {
			skill_delunit(unit);
			return 0;
		}
	}

	// �C�h�D���̗ь�ɂ���
	if(group->unit_id == UNT_APPLEIDUN && DIFF_TICK(tick,group->tick) >= 6000 * group->val3)
	{
		struct block_list *src = map_id2bl(group->src_id);
		if(src == NULL)
			return 0;
		if(src->type == BL_PC || src->type == BL_MOB) {
			range = skill_get_unit_layout_type(group->skill_id,group->skill_lv);
			map_foreachinarea(skill_idun_heal,src->m,
				src->x-range,src->y-range,src->x+range,src->y+range,src->type,unit,src);
		}
		group->val3++;
	}

	if(DIFF_TICK(tick,group->tick) >= group->limit || DIFF_TICK(tick,group->tick) >= unit->limit) {
		/* ���Ԑ؂�폜 */
		switch(group->unit_id) {
			case UNT_WARP_WAITING:	/* ���[�v�|�[�^��(�����O) */
				group->unit_id = UNT_WARP_ACTIVE;
				clif_changelook(bl,LOOK_BASE,group->unit_id);
				group->limit = unit->limit = skill_get_time(group->skill_id,group->skill_lv);
				return 0;
			case UNT_BLASTMINE:		/* �u���X�g�}�C�� */
				group->unit_id = UNT_USED_TRAPS;
				clif_changelook(bl,LOOK_BASE,group->unit_id);
				group->limit = unit->limit = DIFF_TICK(tick+1500,group->tick);
				return 0;

			case UNT_ANKLESNARE:	/* �A���N���X�l�A */
				if(group->val2 > 0) {
					break;
				}
				// fall through
			case UNT_SKIDTRAP:	/* �X�L�b�h�g���b�v */
			case UNT_LANDMINE:	/* �����h�}�C�� */
			case UNT_SHOCKWAVE:	/* �V���b�N�E�F�[�u�g���b�v */
			case UNT_SANDMAN:	/* �T���h�}�� */
			case UNT_FLASHER:	/* �t���b�V���[ */
			case UNT_FREEZINGTRAP:	/* �t���[�W���O�g���b�v */
			case UNT_TALKIEBOX:	/* �g�[�L�[�{�b�N�X */
				if(unit->val1 <= 0) {
					break;
				}
				// fall through
			case UNT_CLAYMORETRAP:	/* �N���C���A�[�g���b�v */
				{
					struct block_list *src = map_id2bl(group->src_id);
					if(src && src->type == BL_PC) {
						struct item item_tmp;
						memset(&item_tmp,0,sizeof(item_tmp));
						item_tmp.nameid   = 1065;
						item_tmp.identify = 1;
						map_addflooritem(&item_tmp,1,bl->m,bl->x,bl->y,0,0,0,0);	// 㩕Ԋ�
					}
				}
				break;
			default:
				break;
		}
		skill_delunit(unit);
	} else {
		/* �ϋv�؂�`�F�b�N */
		switch(group->unit_id) {
			case UNT_ICEWALL:	/* �A�C�X�E�H�[�� */
				unit->val1 -= 5;
				if(unit->val1 <= 0 && unit->limit + group->tick > tick + 700)
					unit->limit = DIFF_TICK(tick+700,group->tick);
				break;
			case UNT_SKIDTRAP:	/* �X�L�b�h�g���b�v */
			case UNT_ANKLESNARE:	/* �A���N���X�l�A */
			case UNT_LANDMINE:	/* �����h�}�C�� */
			case UNT_SHOCKWAVE:	/* �V���b�N�E�F�[�u�g���b�v */
			case UNT_SANDMAN:	/* �T���h�}�� */
			case UNT_FLASHER:	/* �t���b�V���[ */
			case UNT_FREEZINGTRAP:	/* �t���[�W���O�g���b�v */
			case UNT_TALKIEBOX:	/* �g�[�L�[�{�b�N�X */
				if(unit->val1 <= 0) {
					if(group->unit_id == UNT_ANKLESNARE && group->val2 > 0) {	// �ߊl���̃A���N���Ȃ瑦���ɍ폜
						skill_delunit(unit);
					} else {
						group->unit_id = UNT_USED_TRAPS;
						group->limit = DIFF_TICK(tick,group->tick)+1500;
					}
				}
				break;
		}
	}

	return 0;
}

/*==========================================
 * �X�L�����j�b�g�^�C�}�[����
 *------------------------------------------
 */
static int skill_unit_timer( int tid,unsigned int tick,int id,int data)
{
	map_freeblock_lock();

	map_foreachobject( skill_unit_timer_sub, BL_SKILL, tick );

	map_freeblock_unlock();

	return 0;
}

/*==========================================
 * ���j�b�g�ړ������p(foreachinarea)
 *------------------------------------------
 */
static int skill_unit_move_sub(struct block_list *bl, va_list ap)
{
	struct skill_unit *unit;
	struct skill_unit_group *group;
	struct block_list *target;
	unsigned int tick;
	int flag;

	nullpo_retr(0, bl);
	nullpo_retr(0, ap);
	nullpo_retr(0, target = va_arg(ap,struct block_list*));
	nullpo_retr(0, unit = (struct skill_unit *)bl);
	nullpo_retr(0, group = unit->group);

	tick = va_arg(ap,unsigned int);
	flag = va_arg(ap,int);

	if(!unit->alive || group->interval != -1)
		return 0;

	if(flag)
		skill_unit_onplace(unit,target,tick);
	else
		skill_unit_onout(unit,target,tick);

	return 0;
}

/*==========================================
 * ���j�b�g�ړ�������
 *   flag 0:�ړ��O����(���j�b�g�ʒu�̃X�L�����j�b�g�𗣒E)
 *        1:�ړ��㏈��(���j�b�g�ʒu�̃X�L�����j�b�g�𔭓�)
 *------------------------------------------
 */
int skill_unit_move(struct block_list *bl,unsigned int tick,int flag)
{
	nullpo_retr(0, bl);

	if(bl->prev == NULL)
		return 0;

	if(bl->type != BL_PC && bl->type != BL_MOB && bl->type != BL_MERC)
		return 0;

	map_foreachinarea(skill_unit_move_sub,bl->m,bl->x,bl->y,bl->x,bl->y,BL_SKILL,bl,tick,flag);

	return 0;
}

/*==========================================
 * �X�L�����j�b�g���̂̈ړ�������
 * �����̓O���[�v�ƈړ���
 *------------------------------------------
 */
int skill_unit_move_unit_group(struct skill_unit_group *group,int m,int dx,int dy)
{
	int i,j,moveblock;
	unsigned int tick = gettick();
	unsigned char m_flag[MAX_SKILL_UNIT_COUNT];	// group->unit_count��MAX_SKILL_UNIT_COUNT���z���邱�Ƃ͂Ȃ�
	struct skill_unit *unit1, *unit2;

	nullpo_retr(0, group);

	if(group->unit_count <= 0)
		return 0;
	if(group->unit == NULL)
		return 0;

	// �ړ��\�ȃX�L���̓_���X�n��㩂Ɖ�����̂�
	if( !(skill_get_unit_flag(group->skill_id,group->skill_lv)&UF_DANCE) &&
	     !skill_unit_istrap(group->unit_id) &&
	     group->unit_id != UNT_WARM )
		return 0;
	if( group->unit_id == UNT_ANKLESNARE && group->val2 > 0 )	// �⑫���̃A���N���͈ړ��s��
		return 0;

	// �ړ��t���O
	memset(m_flag, 0, sizeof(m_flag));

	// ��Ƀt���O��S�����߂�
	//	m_flag
	//	  0: �P���ړ�
	//	  1: ���j�b�g���ړ�����(���ʒu���烆�j�b�g���Ȃ��Ȃ�)
	//	  2: �c�����V�ʒu���ړ���ƂȂ�(�ړ���Ƀ��j�b�g�����݂��Ȃ�)
	//	  3: �c��
	for(i=0; i<group->unit_count; i++) {
		unit1= &group->unit[i];
		if(!unit1->alive || unit1->bl.m != m)
			continue;
		for(j=0; j<group->unit_count; j++) {
			unit2 = &group->unit[j];
			if(!unit2->alive)
				continue;
			if(unit1->bl.x + dx == unit2->bl.x && unit1->bl.y + dy == unit2->bl.y) {
				// �ړ���Ƀ��j�b�g�����Ԃ��Ă���
				m_flag[i] |= 0x1;
			}
			if(unit1->bl.x - dx == unit2->bl.x && unit1->bl.y - dy == unit2->bl.y) {
				// ���j�b�g�����̏ꏊ�ɂ���Ă���
				m_flag[i] |= 0x2;
			}
		}
	}
	// �t���O�Ɋ�Â��ă��j�b�g�ړ�
	// �t���O��1��unit��T���A�t���O��2��unit�̈ړ���Ɉڂ�
	j = 0;
	for(i=0; i<group->unit_count; i++) {
		unit1 = &group->unit[i];
		if(!unit1->alive)
			continue;
		if(!(m_flag[i]&0x2)) {
			// ���j�b�g���Ȃ��Ȃ�ꏊ�ŃX�L�����j�b�g�e��������
			map_foreachinarea(skill_unit_effect,unit1->bl.m,
				unit1->bl.x,unit1->bl.y,unit1->bl.x,unit1->bl.y,(BL_PC|BL_MOB|BL_MERC),
				&unit1->bl,tick,0);
		}
		if(m_flag[i] == 0) {
			// �P���ړ�
			moveblock = map_block_is_differ(&unit1->bl, m, unit1->bl.x+dx, unit1->bl.y+dy);
			if(moveblock)
				map_delblock(&unit1->bl);
			unit1->bl.m = m;
			unit1->bl.x += dx;
			unit1->bl.y += dy;
			if(moveblock)
				map_addblock(&unit1->bl);
			clif_skill_setunit(unit1);
		} else if(m_flag[i] == 1) {
			// �t���O��2�̂��̂�T���Ă��̃��j�b�g�̈ړ���Ɉړ�
			for( ; j<group->unit_count; j++) {
				if(m_flag[j] == 2) {
					// �p���ړ�
					unit2 = &group->unit[j];
					if(!unit2->alive)
						continue;
					moveblock = map_block_is_differ(&unit1->bl, m, unit2->bl.x+dx, unit2->bl.y+dy);
					if(moveblock)
						map_delblock(&unit1->bl);
					unit1->bl.m = m;
					unit1->bl.x = unit2->bl.x+dx;
					unit1->bl.y = unit2->bl.y+dy;
					if(moveblock)
						map_addblock(&unit1->bl);
					clif_skill_setunit(unit1);
					j++;
					break;
				}
			}
		}
		if(!(m_flag[i]&0x2)) {
			// �ړ���̏ꏊ�ŃX�L�����j�b�g�𔭓�
			map_foreachinarea(skill_unit_effect,unit1->bl.m,
				unit1->bl.x,unit1->bl.y,unit1->bl.x,unit1->bl.y,(BL_PC|BL_MOB|BL_MERC),
				&unit1->bl,tick,1);
		}
	}
	return 0;
}

/*==========================================
 * �ݒu�ς݃X�L�����j�b�g�̐���Ԃ�
 *------------------------------------------
 */
static int skill_count_unitgroup(struct unit_data *ud,int skillid)
{
	int c = 0;
	struct skill_unit_group *group;
	struct linkdb_node *node;

	nullpo_retr(0, ud);

	node = ud->skillunit;
	while( node ) {
		group = (struct skill_unit_group *)node->data;
		if( group->alive_count > 0 && group->skill_id == skillid ) {
			c++;
		}
		node = node->next;
	}
	return c;
}

/*----------------------------------------------------------------------------
 * �A�C�e������
 *----------------------------------------------------------------------------
 */

/*==========================================
 * �A�C�e�������\����
 *------------------------------------------
 */
int skill_can_produce_mix( struct map_session_data *sd, int idx, int trigger)
{
	int i,j,req_skill;

	nullpo_retr(0, sd);

	if(idx < 0 || idx >= MAX_SKILL_PRODUCE_DB)
		return 0;

	if(skill_produce_db[idx].nameid <= 0)
		return 0;

	if(trigger >= 0) {
		if(skill_produce_db[idx].itemlv != trigger)
			return 0;
	}

	// req_skill��0�ȉ��̂Ƃ���req_skilllv�̔�������Ȃ�
	if((req_skill = skill_produce_db[idx].req_skill) > 0 && pc_checkskill(sd,req_skill) < skill_produce_db[idx].req_skilllv)
		return 0;

	for(i=0; i<MAX_PRODUCE_RESOURCE; i++) {
		int amount, count = 0;
		int id = skill_produce_db[idx].mat_id[i];

		if(id <= 0)	// ����ȏ�͍ޗ��v��Ȃ�
			break;
		amount = skill_produce_db[idx].mat_amount[i];
		if(amount <= 0)
			amount = 1;	// ���Ղ���Ȃ�����鎞�K�v�ȃA�C�e��

		for(j=0; j<MAX_INVENTORY; j++) {
			if(sd->status.inventory[j].nameid == id) {
				count += sd->status.inventory[j].amount;
				if(count >= amount)
					break;	// ���肽�̂Ō����I��
			}
		}
		if(count < amount)	// �A�C�e��������Ȃ�
			return 0;
	}
	return 1;
}

/*==========================================
 * �A�C�e�������̐����m���v�Z
 *------------------------------------------
 */
static int skill_calc_produce_rate(struct map_session_data *sd, int idx, int sc, int ele)
{
	int make_per, skill_lv;
	int int_ = sd->paramc[3];
	int dex  = sd->paramc[4];
	int luk  = sd->paramc[5];

	nullpo_retr(0, sd);

	if(idx < 0 || idx >= MAX_SKILL_PRODUCE_DB)
		return 0;

	make_per = skill_produce_db[idx].per;
	skill_lv = pc_checkskill(sd,skill_produce_db[idx].req_skill);

	/* ��{�m�����Z�o����make_per�ɉ��Z */

	switch (skill_produce_db[idx].itemlv)
	{
	case PRD_WEAPON_L1:	// ���퐻��
	case PRD_WEAPON_L2:
	case PRD_WEAPON_L3:
		make_per += sd->status.job_level*20 + dex*10 + luk*10 + skill_lv*500 + pc_checkskill(sd,BS_WEAPONRESEARCH)*100;

		if(pc_search_inventory(sd,989) >= 0)		// �G���y���E���̋��~
			make_per += 1000;
		else if(pc_search_inventory(sd,988) >= 0)	// �����̋��~
			make_per += 500;
		else if(pc_search_inventory(sd,987) >= 0)	// �I���f�I�R���̋��~
			make_per += 300;
		//else if(pc_search_inventory(sd,986) >= 0)	// ���~
		//	make_per += 0:

		if(ele)
			make_per -= 2000;	// �����΂̊m���ቺ
		if(sc > 0)
			make_per -= sc * 1500;	// ���̊m���ቺ

		if(skill_produce_db[idx].itemlv == PRD_WEAPON_L3)
			make_per += pc_checkskill(sd,BS_ORIDEOCON)*100;	// �I���f�I�R�������͎b��

		if(battle_config.wp_rate != 100)
			make_per = make_per * battle_config.wp_rate/100;
		break;

	case PRD_ORE:		// �z��
		make_per += sd->status.job_level*20 + dex*10 + luk*10 + skill_lv*500;
		if(battle_config.wp_rate != 100)
			make_per = make_per * battle_config.wp_rate/100;
		break;

	case PRD_PHARMACY:	// �t�@�[�}�V�[
		if(skill_produce_db[idx].nameid == 7142 && pc_checkskill(sd,AM_BIOETHICS) <= 0) {	// �����ϗ����C�����͐�����0
			make_per = 0;
		} else {
			make_per += pc_checkskill(sd,AM_LEARNINGPOTION)*100 + skill_lv*300 + sd->status.job_level*20 + dex*10 + luk*10 + int_*5;
			if(battle_config.pp_rate != 100)
				make_per = make_per * battle_config.pp_rate/100;
		}
		break;

	case PRD_CDP:		// �f�b�h���[�|�C�Y��
		make_per += dex*40 + luk*20;
		if(battle_config.cdp_rate != 100)
			make_per = make_per * battle_config.cdp_rate/100;
		break;

	case PRD_CONVERTER:	// �R���o�[�^�[
		switch(skill_produce_db[idx].nameid)
		{
			case 12114: skill_lv = pc_checkskill(sd,SA_FLAMELAUNCHER);	break;
			case 12115: skill_lv = pc_checkskill(sd,SA_FROSTWEAPON);	break;
			case 12116: skill_lv = pc_checkskill(sd,SA_LIGHTNINGLOADER);	break;
			case 12117: skill_lv = pc_checkskill(sd,SA_SEISMICWEAPON);	break;
			default: skill_lv = 5; break;
		}
		make_per += skill_lv*1000 + sd->status.job_level*20 + int_*10 + dex*10;
		if(battle_config.scroll_produce_rate != 100)
			make_per = make_per * battle_config.scroll_produce_rate/100;
		break;

	case PRD_COOKING:	// ����
		make_per += sd->making_base_success_per + sd->status.job_level*10 + int_*10 + dex*10 + luk*10;
		if(battle_config.cooking_rate != 100)
			make_per = make_per * battle_config.cooking_rate/100;
		break;

	/* �ȉ����������� */
	case PRD_SCROLL:	// �X�N���[��
		make_per += sd->making_base_success_per + sd->status.job_level*10 + int_*10 + dex*10;
		if(battle_config.scroll_produce_rate != 100)
			make_per = make_per * battle_config.scroll_produce_rate/100;
		break;

	case PRD_SYN_POTION:	// �|�[�V��������
		make_per += sd->making_base_success_per + sd->status.job_level*10 + int_*10 + dex*10 - skill_lv*200;
		if(battle_config.making_rate != 100)
			make_per = make_per * battle_config.making_rate/100;
		break;

	case PRD_COIN:		// �R�C��
	case PRD_NUGGET:	// ��
	case PRD_ORIDEOCON:	// �I���f�I�R������
		make_per += sd->status.base_level*30 + dex*20 + luk*10 + skill_lv*500;
		if(battle_config.wp_rate != 100)
			make_per = make_per * battle_config.wp_rate/100;
		break;
	}

	if(make_per < 1)
		make_per = 1;

	// �{�q�̐�����70%
	if(pc_isbaby(sd))
		make_per = make_per * 70/100;

	return make_per;
}

/*==========================================
 * �t�@�[�}�V�[�ɂ�郉���L���O�|�C���g�v�Z
 *------------------------------------------
 */
static int skill_am_ranking_point(struct map_session_data *sd, int nameid, int success)
{
	int point = 0;

	nullpo_retr(0, sd);

	// ������
	if(success) {
		// �S�Ẵt�@�[�}�V�[�Ώېݒ肩�A�X�����n�̏ꍇ�ɏ���
		if(battle_config.pharmacy_get_point_type || nameid == 545 || nameid == 546 || nameid == 547) {
			// �A������������
			sd->am_pharmacy_success++;
			if(sd->am_pharmacy_success > 10) {
				sd->am_pharmacy_success = 10;
			}
			// +10���������獇�v�|�C���g+64?
			// ���� �K�萬�������ƂɃ|�C���g��Ⴆ��悤�ɐݒ�
			if(battle_config.alchemist_point_type) {
				if(sd->am_pharmacy_success == 10) {
					point = 50;
					sd->am_pharmacy_success = 0;
				}
			} else {
				switch(sd->am_pharmacy_success) {
					case 3:
						point = 1;
						break;
					case 5:
						point = 3;
						break;
					case 7:
						point= 10;
						break;
					case 10:
						point = 50;
						sd->am_pharmacy_success = 0;
						break;
				}
			}
		}
	} else {	// ���s��
		sd->am_pharmacy_success = 0;
		if(battle_config.alchemist_point_type) {
			if(sd->am_pharmacy_success >= 7)
				point = 10;
			else if(sd->am_pharmacy_success >= 5)
				point = 3;
			else if(sd->am_pharmacy_success >= 3)
				point = 1;
		}
	}
	return point;
}

/*==========================================
 * �A�C�e������
 *------------------------------------------
 */
void skill_produce_mix(struct map_session_data *sd, int nameid, int slot1, int slot2, int slot3)
{
	int slot[3];
	int i, sc, ele, type;
	int idx= -1, cnt=0;

	nullpo_retv(sd);

	if(nameid <= 0)		// cancel
		return;

	for(i=0; i<MAX_SKILL_PRODUCE_DB; i++) {
		if(skill_produce_db[i].nameid == nameid) {
			idx = i;
			break;
		}
	}
	if(idx < 0)
		return;

	if(!skill_can_produce_mix(sd,idx,-1))	/* �����s�� */
		return;

	slot[0] = slot1;
	slot[1] = slot2;
	slot[2] = slot3;

	/* ���ߍ��ݏ��� */
	for(i=0, sc=0, ele=0; i<3; i++) {
		int j;
		if(slot[i] <= 0)
			continue;
		j = pc_search_inventory(sd,slot[i]);
		if(j < 0)	/* �s���p�P�b�g(�A�C�e������)�`�F�b�N */
			continue;
		if(slot[i] == 1000) {	/* ���̂����� */
			pc_delitem(sd,j,1,1);
			sc++;
			cnt++;
		}
		if(slot[i] >= 994 && slot[i] <= 997 && ele == 0) {	/* ������ */
			static const int ele_table[4] = { ELE_FIRE, ELE_WATER, ELE_WIND, ELE_EARTH };
			pc_delitem(sd,j,1,1);
			ele = ele_table[slot[i]-994];
			cnt++;
		}
	}

	/* �ޗ����� */
	for(i=0; i<MAX_PRODUCE_RESOURCE; i++) {
		int j, amount;
		int id = skill_produce_db[idx].mat_id[i];

		if(id <= 0)	// ����ȏ�͍ޗ��v��Ȃ�
			break;
		amount = skill_produce_db[idx].mat_amount[i];	/* �K�v�Ȍ� */
		do {	/* �Q�ȏ�̃C���f�b�N�X�ɂ܂������Ă��邩������Ȃ� */
			int c = 0;

			j = pc_search_inventory(sd,id);
			if(j >= 0) {
				c = sd->status.inventory[j].amount;
				if(c > amount)
					c = amount;	/* ����Ă��� */
				pc_delitem(sd,j,c,0);
			} else {
				if(battle_config.error_log)
					printf("skill_produce_mix: material item error\n");
				return;
			}
			amount -= c;	/* �܂�����Ȃ������v�Z */
		} while(amount > 0);	/* �ޗ��������܂ŌJ��Ԃ� */
	}

	type = skill_produce_db[idx].itemlv;

	if(atn_rand()%10000 < skill_calc_produce_rate(sd, idx, sc, ele)) {	// �m������
		/* ���� */
		struct item tmp_item;
		memset(&tmp_item, 0, sizeof(tmp_item));
		tmp_item.nameid   = nameid;
		tmp_item.amount   = 1;
		tmp_item.identify = 1;

		if(type == PRD_WEAPON_L1 || type == PRD_WEAPON_L2 || type == PRD_WEAPON_L3)
		{
			tmp_item.card[0] = 0x00ff;					// ��������t���O
			tmp_item.card[1] = ((sc * 5) << 8) + ele;			// �����΂Ɛ�
			*((unsigned long *)(&tmp_item.card[2])) = sd->status.char_id;	// �L����ID
		}
		else {
			int flag = 0;
			if(type == PRD_PHARMACY || type == PRD_SYN_POTION)
				flag = battle_config.produce_potion_name_input;
			else if(type == PRD_CONVERTER || type == PRD_SCROLL)
				flag = battle_config.scroll_item_name_input;
			else
				flag = battle_config.produce_item_name_input;

			if(flag) {
				tmp_item.card[0] = 0x00fe;
				tmp_item.card[1] = 0;
				*((unsigned long *)(&tmp_item.card[2])) = sd->status.char_id;	// �L����ID
			}
		}

		switch (type) {
			case PRD_PHARMACY:
			{
				int point = skill_am_ranking_point(sd, nameid, 1);
				if(point > 0) {
					ranking_gain_point(sd,RK_ALCHEMIST,point);
					ranking_setglobalreg(sd,RK_ALCHEMIST);
					ranking_update(sd,RK_ALCHEMIST);
				}
				clif_produceeffect(sd,2,nameid);	/* ����G�t�F�N�g */
				clif_misceffect(&sd->bl,5);		/* ���l�ɂ�������ʒm */
				break;
			}
			case PRD_CDP:
			case PRD_CONVERTER:
			case PRD_SYN_POTION:
				clif_produceeffect(sd,2,nameid);	/* �b��Ő���G�t�F�N�g */
				clif_misceffect(&sd->bl,5);
				break;
			case PRD_WEAPON_L1:
			case PRD_WEAPON_L2:
			case PRD_WEAPON_L3:
				if(tmp_item.card[0] == 0x00ff && cnt == 3 && itemdb_wlv(nameid) == 3) {
					ranking_gain_point(sd,RK_BLACKSMITH,10);
					ranking_setglobalreg(sd,RK_BLACKSMITH);
					ranking_update(sd,RK_BLACKSMITH);
				}
				clif_produceeffect(sd,0,nameid);	/* ���퐻���G�t�F�N�g */
				clif_misceffect(&sd->bl,3);
				break;
			case PRD_ORE:
			case PRD_COIN:
			case PRD_NUGGET:
			case PRD_ORIDEOCON:
				clif_produceeffect(sd,0,nameid);	/* ���퐻���G�t�F�N�g */
				clif_misceffect(&sd->bl,3);
				break;
			case PRD_COOKING:
				clif_misceffect2(&sd->bl,608);
				break;
			case PRD_SCROLL:
				clif_misceffect2(&sd->bl,610);
				break;
		}
		pc_additem(sd,&tmp_item,1);	// �d�ʃI�[�o�[�Ȃ����
	} else {
		/* ���s */
		switch (type) {
			case PRD_PHARMACY:
			{
				int point = skill_am_ranking_point(sd, nameid, 0);
				if(point > 0) {
					ranking_gain_point(sd,RK_ALCHEMIST,point);
					ranking_setglobalreg(sd,RK_ALCHEMIST);
					ranking_update(sd,RK_ALCHEMIST);
				}
				clif_produceeffect(sd,3,nameid);	/* ���򎸔s�G�t�F�N�g */
				clif_misceffect(&sd->bl,6);		/* ���l�ɂ����s��ʒm */
				break;
			}
			case PRD_CDP:
				clif_produceeffect(sd,3,nameid);	/* �b��Ő���G�t�F�N�g */
				clif_misceffect(&sd->bl,6);		/* ���l�ɂ����s��ʒm */
				pc_heal(sd, -(sd->status.max_hp>>2), 0);
				break;
			case PRD_CONVERTER:
			case PRD_SYN_POTION:
				clif_produceeffect(sd,3,nameid);	/* �b��Ő���G�t�F�N�g */
				clif_misceffect(&sd->bl,6);		/* ���l�ɂ����s��ʒm */
				break;
			case PRD_WEAPON_L1:
			case PRD_WEAPON_L2:
			case PRD_WEAPON_L3:
			case PRD_ORE:
			case PRD_COIN:
			case PRD_NUGGET:
			case PRD_ORIDEOCON:
				clif_produceeffect(sd,1,nameid);	/* ���퐻�����s�G�t�F�N�g */
				clif_misceffect(&sd->bl,2);		/* ���l�ɂ����s��ʒm */
				break;
			case PRD_COOKING:
				clif_misceffect2(&sd->bl,609);
				break;
			case PRD_SCROLL:
				clif_misceffect2(&sd->bl,611);
				break;
		}
	}
	return;
}

/*==========================================
 * �g���C���C�g�t�@�[�}�V�[
 *------------------------------------------
 */
static int skill_am_twilight_sub(struct map_session_data* sd,int nameid,int count)
{
	int i, make_per = 0, amount = 0, point = 0;

	nullpo_retr(0, sd);

	for(i=0; i<MAX_SKILL_PRODUCE_DB; i++) {
		if(skill_produce_db[i].nameid == nameid)
			break;
	}
	if(i >= MAX_SKILL_PRODUCE_DB)
		return 0;	// ���݂��Ȃ������A�C�e��

	make_per = skill_calc_produce_rate(sd, i, 0, 0);

	for(i=0; i<count; i++) {
		int n = (atn_rand()%10000 < make_per)? 1: 0;
		amount += n;
		point += skill_am_ranking_point(sd, nameid, n);
	}

	if(amount > 0) {
		struct item tmp_item;
		clif_produceeffect(sd,2,nameid);	/* ����G�t�F�N�g */
		clif_misceffect(&sd->bl,5);		/* ���l�ɂ�������ʒm */

		memset(&tmp_item, 0, sizeof(tmp_item));
		tmp_item.nameid   = nameid;
		tmp_item.amount   = amount;
		tmp_item.identify = 1;

		if(battle_config.produce_potion_name_input)
		{
			tmp_item.card[0] = 0x00fe;
			tmp_item.card[1] = 0;
			*((unsigned long *)(&tmp_item.card[2])) = sd->status.char_id;	// �L����ID
		}
		pc_additem(sd, &tmp_item, amount);	// �d�ʃI�[�o�[�Ȃ����
	} else {
		// ���s
		clif_produceeffect(sd,3,nameid);	/* ���򎸔s�G�t�F�N�g */
		clif_misceffect(&sd->bl,6);		/* ���l�ɂ����s��ʒm */
	}

	if(point > 0) {
		ranking_gain_point(sd,RK_ALCHEMIST,point);
		ranking_setglobalreg(sd,RK_ALCHEMIST);
		ranking_update(sd,RK_ALCHEMIST);
	}
	return 1;
}

static int skill_am_twilight(struct map_session_data *sd, int skillid)
{
	nullpo_retr(0, sd);

	switch(skillid) {
		case AM_TWILIGHT1:
			skill_am_twilight_sub(sd,504,200);
			break;
		case AM_TWILIGHT2:
			skill_am_twilight_sub(sd,547,200);
			break;
		case AM_TWILIGHT3:
			skill_am_twilight_sub(sd,970,100);
			skill_am_twilight_sub(sd,7135,50);
			skill_am_twilight_sub(sd,7136,50);
			break;
	}

	return 1;
}

void skill_arrow_create(struct map_session_data *sd, int nameid)
{
	int i, j, flag, idx;
	struct item tmp_item;

	nullpo_retv(sd);

	sd->state.make_arrow_flag = 0;

	if(nameid <= 0)
		return;

	for(idx = 0; idx < MAX_SKILL_ARROW_DB; idx++) {
		if (nameid == skill_arrow_db[idx].nameid)
			break;
	}
	if (idx == MAX_SKILL_ARROW_DB)
		return;

	if ((j = pc_search_inventory(sd, nameid)) < 0 || sd->status.inventory[j].equip)
		return;

	pc_delitem(sd,j,1,0);
	for(i=0;i<5;i++) {
		memset(&tmp_item,0,sizeof(tmp_item));
		tmp_item.nameid = skill_arrow_db[idx].cre_id[i];
		if (tmp_item.nameid <= 0)
			continue;
		tmp_item.amount = skill_arrow_db[idx].cre_amount[i];
		if (tmp_item.amount <= 0)
			continue;
		tmp_item.identify = 1;
		if(battle_config.making_arrow_name_input) {
			tmp_item.card[0]=0x00fe;
			tmp_item.card[1]=0;
			*((unsigned long *)(&tmp_item.card[2]))=sd->status.char_id;	/* �L����ID */
		}
		if((flag = pc_additem(sd,&tmp_item,tmp_item.amount))) {
			clif_additem(sd,0,0,flag);
			map_addflooritem(&tmp_item,tmp_item.amount,sd->bl.m,sd->bl.x,sd->bl.y,0,0,0,0);
		}
	}

	return;
}

/*==========================================
 * ����C��
 *------------------------------------------
 */
void skill_repair_weapon(struct map_session_data *sd, int idx)
{
	const int material[5] = { 999, 1002, 998, 999, 756 };	// �|�S�E�S�z�΁E�S�E�|�S�E�I���f�I�R������
	int itemid, n;
	int skillid = BS_REPAIRWEAPON;
	struct map_session_data *dstsd;
	struct item_data *data;

	nullpo_retv(sd);

	if(idx == 0xffff || idx < 0 || idx >= MAX_INVENTORY)	// cencel or invalid range
		return;

	dstsd = map_id2sd(sd->repair_target);
	if(!dstsd || dstsd->status.inventory[idx].nameid <= 0 || dstsd->status.inventory[idx].attribute == 0) {
		clif_skill_fail(sd,skillid,0,0);
		return;
	}

	if(sd != dstsd) {	// �Ώۂ������łȂ��Ȃ�˒��`�F�b�N
		int range = skill_get_fixed_range(&sd->bl,skillid,1);
		if(!battle_check_range(&sd->bl, &dstsd->bl, range+1)) {
			clif_item_repaireffect(sd, 1, dstsd->status.inventory[idx].nameid);
			return;
		}
	}

	data = dstsd->inventory_data[idx];

	if(data && data->type == 4) {	// ����
		if(data->wlv >= 1 && data->wlv <= 4)
			itemid = material[data->wlv];
		else
			itemid = material[4];		// ����Lv��5�ȏ�Ȃ�Lv4�Ɠ����ޗ��ɂ��Ă���
	} else {			// �h��
		itemid = material[0];
	}

	if((n = pc_search_inventory(sd, itemid)) < 0) {
		clif_item_repaireffect(sd, 1, dstsd->status.inventory[idx].nameid);
	} else {
		clif_skill_nodamage(&sd->bl,&dstsd->bl,skillid,1,1);
		pc_delitem(sd,n,1,0);
		dstsd->status.inventory[idx].attribute = 0;
		clif_delitem(dstsd, idx, 1);
		clif_additem(dstsd, idx, 1, 0);
		clif_item_repaireffect(sd, 0, dstsd->status.inventory[idx].nameid);
	}

	return;
}

/*==========================================
 * �A�C�e���{�[�i�X�I�[�g�X�y��
 *  mode : �U����1 ����2
 *------------------------------------------
 */
static int skill_use_bonus_autospell(struct map_session_data *sd,struct block_list *bl,int skillid,int skilllv,int rate,unsigned long asflag,unsigned int tick,int flag)
{
	struct block_list *target;
	int f=0,sp=0;

	nullpo_retr(0, sd);
	nullpo_retr(0, bl);

	// ���̊Ԃɂ������������͍U���Ώۂ�����ł���
	if(unit_isdead(&sd->bl) || unit_isdead(bl))
		return 0;

	// ��������
	if(skillid <= 0 || skilllv <= 0)
		return 0;

	// ��������������
	if(asflag&EAS_LONG) {
		if(atn_rand()%10000 > (rate/2))
			return 0;
	} else {
		if(atn_rand()%10000 > rate)
			return 0;
	}

	// �X�y���Ώ�
	// �w�肠�邪����Ȃ���
	//if(asflag&EAS_TARGET)
	//	target = bl;	// ����
	//else
	if(asflag&EAS_SELF)
		target = &sd->bl;	// ����
	else if(asflag&EAS_TARGET_RAND && atn_rand()%100 < 50)
		target = &sd->bl;	// ����
	else
		target = bl;		// ����

	// ���x������
	if(asflag & (EAS_USEMAX | EAS_USEBETTER)) {
		int lv;
		if(battle_config.allow_cloneskill_at_autospell)
			lv = pc_checkskill(sd,skillid);
		else
			lv = pc_checkskill2(sd,skillid);

		if(asflag&EAS_USEMAX && lv == pc_get_skilltree_max(&sd->s_class,skillid)) {
			// Max������ꍇ�̂�
			skilllv = lv;
		} else if(asflag&EAS_USEBETTER && lv > skilllv) {
			// ����ȏ�̃��x��������ꍇ�̂�
			skilllv = lv;
		}
	}

	// ���x���̕ϓ�
	if(asflag&EAS_FLUCT) {	// ���x���ϓ� ����`�r�p
		int j = atn_rand()%100;
		if (j >= 50) skilllv -= 2;
		else if(j >= 15) skilllv--;
		if(skilllv < 1) skilllv = 1;
	} else if(asflag&EAS_RANDOM) {	// 1�`�w��܂ł̃����_��
		skilllv = atn_rand()%skilllv+1;
	}

	// SP����
	sp = skill_get_sp(skillid,skilllv);
	if(asflag&EAS_NOSP)
		sp = 0;
	else if(asflag&EAS_SPCOST1)
		sp = sp*2/3;
	else if(asflag&EAS_SPCOST2)
		sp = sp/2;
	else if(asflag&EAS_SPCOST3)
		sp = sp*3/2;

	// SP������Ȃ��I
	if(sd->status.sp < sp)
		return 0;

	if(battle_config.bonus_autospell_delay_enable) {
		int delay = skill_delayfix(&sd->bl, skill_get_delay(skillid,skilllv), skill_get_cast(skillid,skilllv));
		sd->ud.canact_tick = tick + delay;
	}

	// ���s
	if(skillid == AL_TELEPORT && skilllv == 1) {	// Lv1�e���|�̓_�C�A���O�\���Ȃ��ő����ɔ�΂�
		f = pc_randomwarp(sd,3);
	} else if(skill_get_inf(skillid) & 0x02) {	// �ꏊ
		f = skill_castend_pos2(&sd->bl,target->x,target->y,skillid,skilllv,tick,flag);
	} else {
		switch( skill_get_nk(skillid)&3 ) {
			case 0:	// �ʏ�
			case 2:	// ������΂�
				f = skill_castend_damage_id(&sd->bl,target,skillid,skilllv,tick,flag);
				break;
			case 1:	// �x���n
				if( (skillid == AL_HEAL || (skillid == ALL_RESURRECTION && target->type != BL_PC)) &&
				    battle_check_undead(status_get_race(target),status_get_element(target)) )
					f = skill_castend_damage_id(&sd->bl,target,skillid,skilllv,tick,flag);
				else
					f = skill_castend_nodamage_id(&sd->bl,target,skillid,skilllv,tick,flag);
				break;
		}
	}
	if(!f)
		pc_heal(sd,0,-sp);

	return 1;	// ����
}

int skill_bonus_autospell(struct block_list *src,struct block_list *bl,unsigned long mode,unsigned int tick,int flag)
{
	int i;
	static int lock = 0;
	struct map_session_data *sd = NULL;

	nullpo_retr(0, src);
	nullpo_retr(0, bl);

	if(src->type != BL_PC || lock++)
		return 0;

	nullpo_retr(0, sd = (struct map_session_data *)src);

	for(i=0;i<sd->autospell.count;i++)
	{
		if(!(mode&EAS_SHORT) && !(mode&EAS_LONG) && !(mode&EAS_MAGIC) && !(mode&EAS_MISC))
			mode += EAS_SHORT|EAS_LONG;
		if(!(sd->autospell.flag[i]&EAS_SHORT) && !(sd->autospell.flag[i]&EAS_LONG) &&
		   !(sd->autospell.flag[i]&EAS_MAGIC) && !(sd->autospell.flag[i]&EAS_MISC))
			sd->autospell.flag[i] += EAS_SHORT|EAS_LONG;
		if(mode&EAS_SHORT && !(sd->autospell.flag[i]&EAS_SHORT))
			continue;
		if(mode&EAS_LONG && !(sd->autospell.flag[i]&EAS_LONG))
			continue;
		if(mode&EAS_MAGIC && !(sd->autospell.flag[i]&EAS_MAGIC))
			continue;
		if(mode&EAS_MISC && !(sd->autospell.flag[i]&EAS_MISC))
			continue;

		if(!(mode&EAS_ATTACK) && !(mode&EAS_REVENGE))
			mode += EAS_ATTACK;
		if(!(sd->autospell.flag[i]&EAS_ATTACK) && !(sd->autospell.flag[i]&EAS_REVENGE))
			sd->autospell.flag[i] += EAS_ATTACK;
		if(mode&EAS_REVENGE && !(sd->autospell.flag[i]&EAS_REVENGE))
			continue;
		if(mode&EAS_ATTACK && sd->autospell.flag[i]&EAS_REVENGE)
			continue;

		if(!(mode&EAS_NORMAL) && !(mode&EAS_SKILL))
			mode += EAS_NORMAL;
		if(!(sd->autospell.flag[i]&EAS_NORMAL) && !(sd->autospell.flag[i]&EAS_SKILL))
			sd->autospell.flag[i] += EAS_NORMAL;
		if(mode&EAS_NORMAL && !(sd->autospell.flag[i]&EAS_NORMAL))
			continue;
		if(mode&EAS_SKILL && !(sd->autospell.flag[i]&EAS_SKILL))
			continue;

		if(skill_use_bonus_autospell(
			sd,bl,sd->autospell.id[i],sd->autospell.lv[i],
			sd->autospell.rate[i],sd->autospell.flag[i],tick,flag) )
		{
			// �I�[�g�X�y���͂ǂꂩ��x�����������Ȃ�
			if(battle_config.once_autospell)
				break;
		}
	}
	lock = 0;

	return 1;
}

/*==========================================
 * ���퐸�B
 *------------------------------------------
 */
void skill_weapon_refine(struct map_session_data *sd, int idx)
{
	int refine_item[5] = { 0, 1010, 1011, 984, 984 };
	int skilllv,wlv,n;

	nullpo_retv(sd);

	if (idx < 0 || idx >= MAX_INVENTORY)
		return;

	skilllv = pc_checkskill(sd,WS_WEAPONREFINE);

	wlv = itemdb_wlv(sd->status.inventory[idx].nameid);

	// �s���`�F�b�N
	if( wlv <= 0 ||
	    sd->status.inventory[idx].nameid <= 0 ||
	    sd->status.inventory[idx].identify != 1 ||
	    !sd->inventory_data[idx]->refine )
		return;
	if(sd->status.inventory[idx].refine >= skilllv){
		clif_weapon_refine_res(sd,2,sd->status.inventory[idx].nameid);
		return;
	}

	if(wlv > 4)	// ����Lv5�ȏ��Lv4�Ɠ����Ƃ݂Ȃ�
		wlv = 4;

	// �A�C�e���`�F�b�N
	n = pc_search_inventory(sd,refine_item[wlv]);
	if(n < 0) {
		clif_weapon_refine_res(sd,3,refine_item[wlv]);
		return;
	}

	if(atn_rand()%10000 < status_percentrefinery_weaponrefine(sd,&sd->status.inventory[idx])) {
		// ����
		clif_weapon_refine_res(sd,0,sd->status.inventory[idx].nameid);
		skill_success_weaponrefine(sd,idx);
	} else {
		// ���s
		clif_weapon_refine_res(sd,1,sd->status.inventory[idx].nameid);
		skill_fail_weaponrefine(sd,idx);
	}

	// �A�C�e������
	pc_delitem(sd,n,1,0);

	return;
}

/*==========================================
 * ���퐸�B����
 *------------------------------------------
 */
int skill_success_weaponrefine(struct map_session_data *sd,int idx)
{
	nullpo_retr(0, sd);

	if(idx < 0)
		return 0;

	sd->status.inventory[idx].refine++;
	if(sd->status.inventory[idx].refine > MAX_REFINE)
		sd->status.inventory[idx].refine = MAX_REFINE;

	clif_refine(sd->fd,0,idx,sd->status.inventory[idx].refine);
	clif_misceffect(&sd->bl,3);

	// �u���b�N�X�~�X �����l
	if(sd->status.inventory[idx].refine==MAX_REFINE && (*((unsigned long *)(&sd->status.inventory[idx].card[2]))) == sd->status.char_id)
	{
		switch(itemdb_wlv(sd->status.inventory[idx].nameid))
		{
			case 1:
				ranking_gain_point(sd,RK_BLACKSMITH,1);
				ranking_setglobalreg(sd,RK_BLACKSMITH);
				ranking_update(sd,RK_BLACKSMITH);
				break;
			case 2:
				ranking_gain_point(sd,RK_BLACKSMITH,25);
				ranking_setglobalreg(sd,RK_BLACKSMITH);
				ranking_update(sd,RK_BLACKSMITH);
				break;
			case 3:
				ranking_gain_point(sd,RK_BLACKSMITH,1000);
				ranking_setglobalreg(sd,RK_BLACKSMITH);
				ranking_update(sd,RK_BLACKSMITH);
				break;
			default:
				break;
		};
	}

	return 0;
}

/*==========================================
 * ���퐸�B���s
 *------------------------------------------
 */
int skill_fail_weaponrefine(struct map_session_data *sd,int idx)
{
	nullpo_retr(0, sd);

	if(idx < 0)
		return 0;

	sd->status.inventory[idx].refine = 0;
	pc_delitem(sd,idx,1,0);
	// ���B���s�G�t�F�N�g�̃p�P�b�g
	clif_refine(sd->fd,1,idx,sd->status.inventory[idx].refine);
	// ���̐l�ɂ����s��ʒm
	clif_misceffect(&sd->bl,2);

	return 0;
}

/*==========================================
 * �×~
 *------------------------------------------
 */
static int skill_greed( struct block_list *bl,va_list ap )
{
	struct map_session_data *sd;
	struct flooritem_data *fitem;
	struct party *p;

	nullpo_retr(0, bl);
	nullpo_retr(0, sd = va_arg(ap,struct map_session_data *));
	nullpo_retr(0, fitem = (struct flooritem_data *)bl);

	p = va_arg(ap,struct party *);

	pc_takeitem_sub(p, sd, fitem);
	return 0;
}

/*==========================================
 * ����
 *------------------------------------------
 */
static int skill_balkyoung( struct block_list *bl,va_list ap )
{
	struct block_list *src;
	struct block_list *tbl;
	int sc_def_vit;

	nullpo_retr(0, bl);
	nullpo_retr(0, src = va_arg(ap,struct block_list *));
	nullpo_retr(0, tbl = va_arg(ap,struct block_list *));

	if(!(bl->type & (BL_CHAR | BL_SKILL)))
		return 0;

	// �{�l�ɂ͓K�p���Ȃ�?
	if(bl->id == tbl->id)
		return 0;
	if(battle_check_target(src,bl,BCT_ENEMY) <= 0)
		return 0;

	sc_def_vit = 100 - (3 + status_get_vit(bl) + status_get_luk(bl)/3);
	skill_blown(src,bl,2);	// ������΂��Ă݂�
	if(atn_rand()%100 < 70*sc_def_vit/100)
		status_change_start(bl,SC_STUN,1,0,0,0,2000,0);

	return 0;
}

/*==========================================
 * �L���X�����O�̃^�[�Q�b�g�ύX
 *------------------------------------------
 */
static int skill_castle_mob_changetarget(struct block_list *bl,va_list ap)
{
	struct mob_data* md;
	struct block_list *from_bl;
	struct block_list *to_bl;

	nullpo_retr(0, bl);
	nullpo_retr(0, md = (struct mob_data*)bl);
	nullpo_retr(0, from_bl = va_arg(ap,struct block_list *));
	nullpo_retr(0, to_bl = va_arg(ap,struct block_list *));

	if(md->target_id == from_bl->id)
		md->target_id = to_bl->id;
	return 0;
}

/*==========================================
 * ����\���H�i�]���X�L���܂ށj
 *------------------------------------------
 */
int skill_cloneable(int skillid)
{
	return skill_get_cloneable(skillid);
}

/*==========================================
 * �]���X�L�����H
 *------------------------------------------
 */
int skill_upperskill(int skillid)
{
	if(LK_AURABLADE <= skillid && skillid <= ASC_CDP)
		return 1;
	if(ST_PRESERVE <= skillid && skillid <= CR_CULTIVATION)
		return 1;
	return 0;
}

/*==========================================
 * �G�̃X�L�����H
 *------------------------------------------
 */
int skill_mobskill(int skillid)
{
	if(NPC_PIERCINGATT <= skillid && skillid <= NPC_SUMMONMONSTER)
		return 1;

	if(NPC_DARKCROSS <= skillid && skillid <= NPC_RUN)
		return 1;

	if(NPC_EARTHQUAKE <= skillid && skillid <= NPC_WIDESOULDRAIN)
		return 1;

	if(skillid == NPC_SELFDESTRUCTION2)
		return 1;

	return 0;
}

/*==========================================
 * ����p�X�L����
 *------------------------------------------
 */
int skill_abraskill(int skillid)
{
	if(SA_MONOCELL <= skillid && skillid <= SA_COMA)
		return 1;
	return 0;
}

/*==========================================
 * �N���[���X�L��
 *------------------------------------------
 */
int skill_clone(struct map_session_data* sd,int skillid,int skilllv)
{
	nullpo_retr(0, sd);

	if(skillid <= 0 || skilllv <= 0)
		return 0;
	// �����x�����擾���Ă���
	if(pc_checkskill(sd,skillid) >= skilllv)
		return 0;

	// �擾�\�X�L�����H
	if(skill_get_cloneable(skillid)&(1<<sd->s_class.upper))
	{
		int cloneskilllv;
		// �T���N�`���A�����󂯂��ꍇ�A��Lv�̃q�[�����N���[��
		if(skillid == PR_SANCTUARY)
		{
			skillid = AL_HEAL;
			if(pc_checkskill(sd,skillid) >= skilllv)
				return 0;
		}
		cloneskilllv = pc_checkskill(sd,RG_PLAGIARISM);
		sd->cloneskill_id = skillid;
		sd->cloneskill_lv = (skilllv > cloneskilllv)? cloneskilllv: skilllv;
		clif_skillinfoblock(sd);
		return 1;
	}
	return 0;
}

/*==========================================
 * �񕜗ʕ␳
 *------------------------------------------
 */
int skill_fix_heal(struct block_list *src, struct block_list *bl, int skill_id, int heal)
{
	struct map_session_data *sd = NULL;
	struct status_change *sc = NULL;

	nullpo_retr(0, src);

	if(src->type == BL_PC)
		sd = (struct map_session_data *)src;
	if(bl)
		sc = status_get_sc(bl);

	if(sc && sc->data[SC_CRITICALWOUND].timer != -1)
		heal = heal * (100 - sc->data[SC_CRITICALWOUND].val1 * 10) / 100;

	if(sd && sd->skill_healup.count > 0 && heal > 0 && skill_id > 0) {
		int i;
		for(i = 0; i < sd->skill_healup.count; i++) {
			if(skill_id == sd->skill_healup.id[i]) {
				heal += heal * sd->skill_healup.rate[i] / 100;
				break;
			}
		}
	}

	return heal;
}

/*----------------------------------------------------------------------------
 * �������n
 */

/*==========================================
 * �����񏈗�
 *   ',' �ŋ�؂���val�ɖ߂�
 *------------------------------------------
 */
static int skill_split_str(char *str,char **val,int num)
{
	int i;

	for (i=0; i<num && str; i++){
		val[i] = str;
		str = strchr(str,',');
		if (str)
			*str++=0;
	}
	return i;
}

/*==========================================
 * �����񏈗�
 *   ':' �ŋ�؂���atoi����val�ɖ߂�
 *------------------------------------------
 */
static int skill_split_atoi(char *str,int *val,int num)
{
	int i, max = 0;

	for (i=0; i<num; i++) {
		if (str) {
			val[i] = max = atoi(str);
			str = strchr(str,':');
			if (str)
				*str++=0;
		} else {
			val[i] = max;
		}
	}
	return i;
}

/*==========================================
 * �����񏈗�
 *   ':' �ŋ�؂���strtol����val�ɖ߂�
 *------------------------------------------
 */
static int skill_split_strtol(char *str,int *val,int num,int base)
{
	int i, max = 0;

	for (i=0; i<num; i++) {
		if (str) {
			val[i] = max = strtol(str, NULL, base);
			str = strchr(str,':');
			if (str)
				*str++=0;
		} else {
			val[i] = max;
		}
	}
	return i;
}

/*==========================================
 * �X�L�����j�b�g�̔z�u���쐬
 *------------------------------------------
 */
static void skill_init_unit_layout(void)
{
	int i,j,size,pos = 0;

	memset(skill_unit_layout,0,sizeof(skill_unit_layout));
	// ��`�̃��j�b�g�z�u���쐬����
	for (i=0; i<=MAX_SQUARE_LAYOUT; i++) {
		size = i*2+1;
		skill_unit_layout[i].count = size*size;
		for (j=0; j<size*size; j++) {
			skill_unit_layout[i].dx[j] = (j%size-i);
			skill_unit_layout[i].dy[j] = (j/size-i);
		}
	}
	pos = i;
	// ��`�ȊO�̃��j�b�g�z�u���쐬����
	for (i=0;i<MAX_SKILL_DB;i++) {
		if (!skill_db[i].unit_id[0] || skill_db[i].unit_layout_type[0] != -1)
			continue;
		switch (skill_db[i].id) {
			case MG_FIREWALL:
			case WZ_ICEWALL:
				// �t�@�C�A�[�E�H�[���A�A�C�X�E�H�[���͕����ŕς��̂ŕʏ���
				break;
			case PR_SANCTUARY:
			{
				static const int dx[] = {
					-1, 0, 1,-2,-1, 0, 1, 2,-2,-1,
					 0, 1, 2,-2,-1, 0, 1, 2,-1, 0, 1};
				static const int dy[]={
					-2,-2,-2,-1,-1,-1,-1,-1, 0, 0,
					 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 2};
				skill_unit_layout[pos].count = 21;
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
				break;
			}
			case PR_MAGNUS:
			{
				static const int dx[] = {
					-1, 0, 1,-1, 0, 1,-3,-2,-1, 0,
					 1, 2, 3,-3,-2,-1, 0, 1, 2, 3,
					-3,-2,-1, 0, 1, 2, 3,-1, 0, 1,-1, 0, 1};
				static const int dy[] = {
					-3,-3,-3,-2,-2,-2,-1,-1,-1,-1,
					-1,-1,-1, 0, 0, 0, 0, 0, 0, 0,
					 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3, 3};
				skill_unit_layout[pos].count = 33;
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
				break;
			}
			case AS_VENOMDUST:
			{
				static const int dx[] = {-1, 0, 0, 0, 1};
				static const int dy[] = { 0,-1, 0, 1, 0};
				skill_unit_layout[pos].count = 5;
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
				break;
			}
			case CR_GRANDCROSS:
			case NPC_GRANDDARKNESS:
			{
				static const int dx[] = {
					 0, 0,-1, 0, 1,-2,-1, 0, 1, 2,
					-4,-3,-2,-1, 0, 1, 2, 3, 4,-2,
					-1, 0, 1, 2,-1, 0, 1, 0, 0};
				static const int dy[] = {
					-4,-3,-2,-2,-2,-1,-1,-1,-1,-1,
					 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
					 1, 1, 1, 1, 2, 2, 2, 3, 4};
				skill_unit_layout[pos].count = 29;
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
				break;
			}
			case PF_FOGWALL:
			{
				static const int dx[] = {
					-2,-1, 0, 1, 2,-2,-1, 0, 1, 2,-2,-1, 0, 1, 2};
				static const int dy[] = {
					-1,-1,-1,-1,-1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1};
				skill_unit_layout[pos].count = 15;
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
				break;
			}
			case PA_GOSPEL:
			{
				static const int dx[] = {
					-1, 0, 1,-1, 0, 1,-3,-2,-1, 0,
					 1, 2, 3,-3,-2,-1, 0, 1, 2, 3,
					-3,-2,-1, 0, 1, 2, 3,-1, 0, 1,
					-1, 0, 1};
				static const int dy[] = {
					-3,-3,-3,-2,-2,-2,-1,-1,-1,-1,
					-1,-1,-1, 0, 0, 0, 0, 0, 0, 0,
					 1, 1, 1, 1, 1, 1, 1, 2, 2, 2,
					 3, 3, 3};
				skill_unit_layout[pos].count = 33;
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
				break;
			}
			case NJ_TATAMIGAESHI:
			{
				// Lv1�i3x3�Ńg�[�^��4�Z���j
				static const int dx1[] = {-1, 1, 0, 0};
				static const int dy1[] = { 0, 0,-1, 1};
				// Lv2,3�i5x5�Ńg�[�^��8�Z���j
				static const int dx2[] = {-2,-1, 1, 2, 0, 0, 0, 0};
				static const int dy2[] = { 0, 0, 0, 0,-2,-1, 1, 2};
				// Lv4,5�i7x7�Ńg�[�^��12�Z���j
				static const int dx3[] = {-3,-2,-1, 1, 2, 3, 0, 0, 0, 0, 0, 0};
				static const int dy3[] = { 0, 0, 0, 0, 0, 0,-3,-2,-1, 1, 2, 3};
				// Lv1�̃Z�b�g
				j = 0;
				skill_unit_layout[pos].count = 4;
				memcpy(skill_unit_layout[pos].dx,dx1,sizeof(dx1));
				memcpy(skill_unit_layout[pos].dy,dy1,sizeof(dy1));
				skill_db[i].unit_layout_type[j] = pos;
				// Lv2,3�̃Z�b�g
				j++;
				pos++;
				skill_unit_layout[pos].count = 8;
				memcpy(skill_unit_layout[pos].dx,dx2,sizeof(dx2));
				memcpy(skill_unit_layout[pos].dy,dy2,sizeof(dy2));
				skill_db[i].unit_layout_type[j] = pos;
				skill_db[i].unit_layout_type[++j] = pos;
				// Lv4,5�̃Z�b�g
				j++;
				pos++;
				skill_unit_layout[pos].count = 12;
				memcpy(skill_unit_layout[pos].dx,dx3,sizeof(dx3));
				memcpy(skill_unit_layout[pos].dy,dy3,sizeof(dy3));
				skill_db[i].unit_layout_type[j] = pos;
				skill_db[i].unit_layout_type[++j] = pos;
				// Lv6�ȏ�͓���type�Ŗ��߂�
				for (;j<MAX_SKILL_LEVEL;j++)
					skill_db[i].unit_layout_type[j] = pos;
				pos++;
				continue;	// ����Lv����pos��ݒ肵���̂ňȉ��̏����͔�΂�
			}
			default:
				printf("unknown unit layout at skill %d\n",i);
				break;
		}
		if (!skill_unit_layout[pos].count)
			continue;
		for (j=0;j<MAX_SKILL_LEVEL;j++)
			skill_db[i].unit_layout_type[j] = pos;
		pos++;
	}
	// �t�@�C���[�E�H�[��
	firewall_unit_pos = pos;
	for (i=0;i<8;i++) {
		if (i&1) {	/* �΂ߔz�u */
			skill_unit_layout[pos].count = 5;
			if (i&0x2) {
				int dx[] = {-1,-1, 0, 0, 1};
				int dy[] = { 1, 0, 0,-1,-1};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			} else {
				int dx[] = { 1, 1 ,0, 0,-1};
				int dy[] = { 1, 0, 0,-1,-1};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			}
		} else {	/* �c���z�u */
			skill_unit_layout[pos].count = 3;
			if (i%4==0) {	/* �㉺ */
				int dx[] = {-1, 0, 1};
				int dy[] = { 0, 0, 0};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			} else {			/* ���E */
				int dx[] = { 0, 0, 0};
				int dy[] = {-1, 0, 1};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			}
		}
		pos++;
	}
	// �A�C�X�E�H�[��
	icewall_unit_pos = pos;
	for (i=0;i<8;i++) {
		skill_unit_layout[pos].count = 5;
		if (i&1) {	/* �΂ߔz�u */
			if (i&0x2) {
				int dx[] = {-2,-1, 0, 1, 2};
				int dy[] = { 2, 1, 0,-1,-2};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			} else {
				int dx[] = { 2, 1 ,0,-1,-2};
				int dy[] = { 2, 1, 0,-1,-2};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			}
		} else {	/* �c���z�u */
			if (i%4==0) {	/* �㉺ */
				int dx[] = {-2,-1, 0, 1, 2};
				int dy[] = { 0, 0, 0, 0, 0};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			} else {			/* ���E */
				int dx[] = { 0, 0, 0, 0, 0};
				int dy[] = {-2,-1, 0, 1, 2};
				memcpy(skill_unit_layout[pos].dx,dx,sizeof(dx));
				memcpy(skill_unit_layout[pos].dy,dy,sizeof(dy));
			}
		}
		pos++;
	}
}

/*==========================================
 * �X�L���֌W�t�@�C���ǂݍ���
 * skill_db.txt �X�L���f�[�^
 * skill_cast_db.txt �X�L���̉r�����Ԃƃf�B���C�f�[�^
 * produce_db.txt �A�C�e���쐬�X�L���p�f�[�^
 * create_arrow_db.txt ��쐬�X�L���p�f�[�^
 * abra_db.txt �A�u���J�_�u�������X�L���f�[�^
 *------------------------------------------
 */
static int skill_readdb(void)
{
	int i,j,k,m;
	FILE *fp;
	char line[1024],*p;
	const char *filename[] = {
		"db/skill_db.txt",         "db/addon/skill_db_add.txt",
		"db/skill_require_db.txt", "db/addon/skill_require_db_add.txt",
		"db/skill_cast_db.txt",    "db/addon/skill_cast_db_add.txt",
		"db/produce_db.txt",       "db/addon/produce_db_add.txt"
	};

	memset(skill_db,0,sizeof(skill_db));

	/* �X�L���f�[�^�x�[�X */
	for(m=0; m<2; m++){
		fp=fopen(filename[m],"r");
		if(fp==NULL){
			if(m>0)
				continue;
			printf("can't read %s\n",filename[m]);
			return 1;
		}
		k = 0;
		while(fgets(line,1020,fp)){
			char *split[50];
			if(line[0]=='/' && line[1]=='/')
				continue;
			j = skill_split_str(line,split,14);
			if(split[13]==NULL || j<14)
				continue;

			i = skill_get_skilldb_id(atoi(split[0]));
			if(i == 0)
				continue;

			skill_db[i].id = atoi(split[0]);
			skill_split_atoi(split[1],skill_db[i].range,MAX_SKILL_LEVEL);
			skill_db[i].hit = atoi(split[2]);
			skill_db[i].inf = atoi(split[3]);
			skill_db[i].pl  = atoi(split[4]);
			skill_db[i].nk  = atoi(split[5]);

			skill_db[i].max = atoi(split[6]);
			if(skill_db[i].max > MAX_SKILL_LEVEL)
				skill_db[i].max = MAX_SKILL_LEVEL;

			skill_split_atoi(split[7],skill_db[i].num,MAX_SKILL_LEVEL);

			if(strcmpi(split[8],"yes") == 0)
				skill_db[i].castcancel = 1;
			else
				skill_db[i].castcancel = 0;
			skill_db[i].cast_def_rate = atoi(split[9]);
			skill_db[i].inf2          = atoi(split[10]);
			skill_split_atoi(split[11],skill_db[i].maxcount,MAX_SKILL_LEVEL);
			if(strcmpi(split[12],"weapon") == 0)
				skill_db[i].skill_type = BF_WEAPON;
			else if(strcmpi(split[12],"magic") == 0)
				skill_db[i].skill_type = BF_MAGIC;
			else if(strcmpi(split[12],"misc") == 0)
				skill_db[i].skill_type = BF_MISC;
			else
				skill_db[i].skill_type = 0;
			skill_split_atoi(split[13],skill_db[i].blewcount,MAX_SKILL_LEVEL);
			k++;
		}
		fclose(fp);
		printf("read %s done (count=%d)\n",filename[m],k);
	}

	/* �X�L���f�[�^�x�[�X2 */
	fp=fopen("db/skill_db2.txt","r");
	if(fp==NULL){
		printf("can't read db/skill_db2.txt\n");
		return 1;
	}
	k = 0;
	while(fgets(line,1020,fp)){
		char *split[50];
		if(line[0]=='/' && line[1]=='/')
			continue;
		j = skill_split_str(line,split,9);
		if(split[8]==NULL || j<9)
			continue;

		i = skill_get_skilldb_id(atoi(split[0]));
		if(i == 0)
			continue;

		skill_db[i].cloneable = atoi(split[1]);
		skill_db[i].misfire   = atoi(split[2]);
		skill_db[i].zone      = atoi(split[3]);
		skill_split_atoi(split[4],skill_db[i].damage_rate,sizeof(skill_db[i].damage_rate)/sizeof(int));
		k++;
	}
	fclose(fp);
	printf("read db/skill_db2.txt done (count=%d)\n",k);

	/* �X�L���v���f�[�^�x�[�X */
	for(m=2; m<4; m++){
		int n;
		fp=fopen(filename[m],"r");
		if(fp==NULL){
			if(m>2)
				continue;
			printf("can't read %s\n",filename[m]);
			return 1;
		}
		k = 0;
		while(fgets(line,1020,fp)){
			char *split[50];
			if(line[0]=='/' && line[1]=='/')
				continue;
			j = skill_split_str(line,split,29);
			if(split[28]==NULL || j<29)
				continue;

			i = skill_get_skilldb_id(atoi(split[0]));
			if(i == 0)
				continue;

			skill_split_atoi(split[1],skill_db[i].hp,MAX_SKILL_LEVEL);
			skill_split_atoi(split[2],skill_db[i].sp,MAX_SKILL_LEVEL);
			skill_split_atoi(split[3],skill_db[i].hp_rate,MAX_SKILL_LEVEL);
			skill_split_atoi(split[4],skill_db[i].sp_rate,MAX_SKILL_LEVEL);
			skill_split_atoi(split[5],skill_db[i].zeny,MAX_SKILL_LEVEL);

			p = split[6];
			for(j=0;j<32;j++){
				n = atoi(p);
				if(n == 99) {
					skill_db[i].weapon = 0xffffffff;
					break;
				} else {
					skill_db[i].weapon |= 1<<n;
				}
				p=strchr(p,':');
				if(!p)
					break;
				p++;
			}

			if( strcmpi(split[7],"hiding") == 0 )                   skill_db[i].state = SST_HIDING;
			else if( strcmpi(split[7],"cloaking") == 0 )            skill_db[i].state = SST_CLOAKING;
			else if( strcmpi(split[7],"chasewalking") == 0 )        skill_db[i].state = SST_CHASEWALKING;
			else if( strcmpi(split[7],"hidden") == 0 )              skill_db[i].state = SST_HIDDEN;
			else if( strcmpi(split[7],"riding") == 0 )              skill_db[i].state = SST_RIDING;
			else if( strcmpi(split[7],"falcon") == 0 )              skill_db[i].state = SST_FALCON;
			else if( strcmpi(split[7],"cart") == 0 )                skill_db[i].state = SST_CART;
			else if( strcmpi(split[7],"shield") == 0 )              skill_db[i].state = SST_SHIELD;
			else if( strcmpi(split[7],"sight") == 0 )               skill_db[i].state = SST_SIGHT;
			else if( strcmpi(split[7],"explosionspirits") == 0 )    skill_db[i].state = SST_EXPLOSIONSPIRITS;
			else if( strcmpi(split[7],"cartboost") == 0 )           skill_db[i].state = SST_CARTBOOST;
			else if( strcmpi(split[7],"nen") == 0 )                 skill_db[i].state = SST_NEN;
			else if( strcmpi(split[7],"recover_weight_rate") == 0 ) skill_db[i].state = SST_RECOV_WEIGHT_RATE;
			else if( strcmpi(split[7],"move_enable") == 0 )         skill_db[i].state = SST_MOVE_ENABLE;
			else if( strcmpi(split[7],"water") == 0 )               skill_db[i].state = SST_WATER;
			else                                                    skill_db[i].state = SST_NONE;

			skill_split_atoi(split[8],skill_db[i].spiritball,MAX_SKILL_LEVEL);
			skill_db[i].itemid[0] = atoi(split[9]);
			skill_db[i].amount[0] = atoi(split[10]);
			skill_db[i].itemid[1] = atoi(split[11]);
			skill_db[i].amount[1] = atoi(split[12]);
			skill_db[i].itemid[2] = atoi(split[13]);
			skill_db[i].amount[2] = atoi(split[14]);
			skill_db[i].itemid[3] = atoi(split[15]);
			skill_db[i].amount[3] = atoi(split[16]);
			skill_db[i].itemid[4] = atoi(split[17]);
			skill_db[i].amount[4] = atoi(split[18]);
			skill_db[i].itemid[5] = atoi(split[19]);
			skill_db[i].amount[5] = atoi(split[20]);
			skill_db[i].itemid[6] = atoi(split[21]);
			skill_db[i].amount[6] = atoi(split[22]);
			skill_db[i].itemid[7] = atoi(split[23]);
			skill_db[i].amount[7] = atoi(split[24]);
			skill_db[i].itemid[8] = atoi(split[25]);
			skill_db[i].amount[8] = atoi(split[26]);
			skill_db[i].itemid[9] = atoi(split[27]);
			skill_db[i].amount[9] = atoi(split[28]);
			k++;
		}
		fclose(fp);
		printf("read %s done (count=%d)\n",filename[m],k);
	}

	/* �X�L���v���f�[�^�x�[�X2 */
	fp=fopen("db/skill_require_db2.txt","r");
	if(fp==NULL){
		printf("can't read db/skill_require_db2.txt\n");
		return 1;
	}
	k = 0;
	while(fgets(line,1020,fp)){
		char *split[50];
		if(line[0]=='/' && line[1]=='/')
			continue;
		j = skill_split_str(line,split,6);
		if(split[5]==NULL || j<6)
			continue;

		i = skill_get_skilldb_id(atoi(split[0]));
		if(i == 0)
			continue;

		skill_split_atoi(split[1],skill_db[i].coin,MAX_SKILL_LEVEL);
		skill_db[i].arrow_type = atoi(split[2]);
		skill_split_atoi(split[3],skill_db[i].arrow_cost,MAX_SKILL_LEVEL);
		k++;
	}
	fclose(fp);
	printf("read db/skill_require_db2.txt done (count=%d)\n",k);

	/* �L���X�e�B���O�f�[�^�x�[�X */
	for(m=4; m<6; m++){
		fp=fopen(filename[m],"r");
		if(fp==NULL){
			if(m>4)
				continue;
			printf("can't read %s\n",filename[m]);
			return 1;
		}
		k = 0;
		while(fgets(line,1020,fp)){
			char *split[50];
			if(line[0]=='/' && line[1]=='/')
				continue;
			j = skill_split_str(line,split,6);
			if(split[5]==NULL || j<6)
				continue;

			i = skill_get_skilldb_id(atoi(split[0]));
			if(i == 0)
				continue;

			skill_split_atoi(split[1],skill_db[i].cast,MAX_SKILL_LEVEL);
			skill_split_atoi(split[2],skill_db[i].fixedcast,MAX_SKILL_LEVEL);
			skill_split_atoi(split[3],skill_db[i].delay,MAX_SKILL_LEVEL);
			skill_split_atoi(split[4],skill_db[i].upkeep_time,MAX_SKILL_LEVEL);
			skill_split_atoi(split[5],skill_db[i].upkeep_time2,MAX_SKILL_LEVEL);
			k++;
		}
		fclose(fp);
		printf("read %s done (count=%d)\n",filename[m],k);
	}

	/* �X�L�����j�b�g�f�[�^�x�[�X */
	fp = fopen("db/skill_unit_db.txt","r");
	if (fp==NULL) {
		printf("can't read db/skill_unit_db.txt\n");
		return 1;
	}
	k = 0;
	while (fgets(line,1020,fp)) {
		char *split[50];
		if (line[0]=='/' && line[1]=='/')
			continue;
		j = skill_split_str(line,split,8);
		if (split[7]==NULL || j<8)
			continue;

		i = skill_get_skilldb_id(atoi(split[0]));
		if(i == 0)
			continue;

		skill_db[i].unit_id[0] = strtol(split[1],NULL,16);
		skill_db[i].unit_id[1] = strtol(split[2],NULL,16);
		skill_split_atoi(split[3],skill_db[i].unit_layout_type,MAX_SKILL_LEVEL);
		skill_db[i].unit_range    = atoi(split[4]);
		skill_db[i].unit_interval = atoi(split[5]);
		skill_db[i].unit_target   = strtol(split[6],NULL,16);
		skill_split_strtol(split[7],skill_db[i].unit_flag,MAX_SKILL_LEVEL,16);
		k++;
	}
	fclose(fp);
	printf("read db/skill_unit_db.txt done (count=%d)\n",k);
	skill_init_unit_layout();

	/* �����n�X�L���f�[�^�x�[�X */
	memset(skill_produce_db,0,sizeof(skill_produce_db));

	for(m=6; m<8; m++){
		int count=0;
		fp=fopen(filename[m],"r");
		if(fp==NULL){
			if(m>6)
				continue;
			printf("can't read %s\n",filename[m]);
			return 1;
		}
		while(fgets(line,1020,fp)){
			char *split[6 + MAX_PRODUCE_RESOURCE * 2];
			int x,y;
			if(line[0]=='/' && line[1]=='/')
				continue;
			memset(split,0,sizeof(split));
			for(j=0,p=line;j<6 + MAX_PRODUCE_RESOURCE * 2 && p;j++){
				split[j]=p;
				p=strchr(p,',');
				if(p) *p++=0;
			}
			if(split[0]==NULL)
				continue;
			i=atoi(split[0]);
			if(i<=0)
				continue;

			for(k=0; k<MAX_SKILL_PRODUCE_DB; k++) {
				if(skill_produce_db[k].nameid <= 0 || skill_produce_db[k].nameid == i)
					break;
			}
			if(k >= MAX_SKILL_PRODUCE_DB)
				break;

			skill_produce_db[k].nameid      = i;
			skill_produce_db[k].itemlv      = atoi(split[1]);
			skill_produce_db[k].req_skill   = atoi(split[2]);
			skill_produce_db[k].req_skilllv = atoi(split[3]);
			skill_produce_db[k].per         = atoi(split[4]);

			for(x=5,y=0; split[x] && split[x+1] && y<MAX_PRODUCE_RESOURCE; x+=2,y++){
				skill_produce_db[k].mat_id[y]     = atoi(split[x]);
				skill_produce_db[k].mat_amount[y] = atoi(split[x+1]);
			}
			count++;
		}
		fclose(fp);
		printf("read %s done (count=%d)\n",filename[m],count);
	}

	/* ��쐬�f�[�^�x�[�X */
	memset(skill_arrow_db,0,sizeof(skill_arrow_db));
	fp=fopen("db/create_arrow_db.txt","r");
	if(fp==NULL){
		printf("can't read db/create_arrow_db.txt\n");
		return 1;
	}
	k=0;
	while(fgets(line,1020,fp)){
		char *split[16];
		int x,y;
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<13 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(split[0]==NULL)
			continue;
		i=atoi(split[0]);
		if(i<=0)
			continue;

		skill_arrow_db[k].nameid=i;

		for(x=1,y=0;split[x] && split[x+1] && y<5;x+=2,y++){
			skill_arrow_db[k].cre_id[y]     = atoi(split[x]);
			skill_arrow_db[k].cre_amount[y] = atoi(split[x+1]);
		}
		k++;
		if(k >= MAX_SKILL_ARROW_DB)
			break;
	}
	fclose(fp);
	printf("read db/create_arrow_db.txt done (count=%d)\n",k);

	/* �A�u���J�^�u���f�[�^�x�[�X */
	memset(skill_abra_db,0,sizeof(skill_abra_db));
	fp=fopen("db/abra_db.txt","r");
	if(fp==NULL){
		printf("can't read db/abra_db.txt\n");
		return 1;
	}
	k=0;
	while(fgets(line,1020,fp)){
		char *split[16];
		if(line[0]=='/' && line[1]=='/')
			continue;
		memset(split,0,sizeof(split));
		for(j=0,p=line;j<13 && p;j++){
			split[j]=p;
			p=strchr(p,',');
			if(p) *p++=0;
		}
		if(split[0]==NULL)
			continue;
		i=atoi(split[0]);
		if(i<=0)
			continue;
		i=atoi(split[3]);
		if(i<=0)
			continue;

		skill_abra_db[k].nameid = atoi(split[0]);
		skill_abra_db[k].req_lv = atoi(split[2]);
		skill_abra_db[k].per    = atoi(split[3]);

		k++;
		if(k >= MAX_SKILL_ABRA_DB)
			break;
	}
	fclose(fp);
	printf("read db/abra_db.txt done (count=%d)\n",k);

	return 0;
}

void skill_reload(void)
{
	skill_readdb();
}

/*==========================================
 * �X�L���֌W����������
 *------------------------------------------
 */
int do_init_skill(void)
{
	skill_readdb();

	add_timer_func_list(skill_unit_timer,"skill_unit_timer");
	add_timer_func_list(skill_castend_id,"skill_castend_id");
	add_timer_func_list(skill_castend_pos,"skill_castend_pos");
	add_timer_func_list(skill_timerskill,"skill_timerskill");
	add_timer_func_list(skill_castend_delay_sub,"skill_castend_delay_sub");

	add_timer_interval(gettick()+SKILLUNITTIMER_INVERVAL,skill_unit_timer,0,0,SKILLUNITTIMER_INVERVAL);

	return 0;
}
