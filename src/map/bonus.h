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

#ifndef _BONUS_H_
#define _BONUS_H_

#include "map.h"

// �g���I�[�g�X�y��
enum {
	EAS_SHORT       = 0x00000001,	// �ߋ�������
	EAS_LONG        = 0x00000002,	// ����������
	EAS_WEAPON      = 0x00000003,	// ����
	EAS_MAGIC       = 0x00000004,	// ���@
	EAS_MISC        = 0x00000008,	// misc�i㩁E��E�Ή��r���j
	EAS_TARGET      = 0x00000010,	// �����Ɏg��
	EAS_SELF        = 0x00000020,	// �����Ɏg��
	EAS_TARGET_RAND = 0x00000040,	// �������U���ΏۂɎg��
	//EAS_TARGET    = 0x00000080,	// �������U���ΏۂɎg��
	EAS_FLUCT       = 0x00000100,	// ��AS�p 1�`3�̂���
	EAS_RANDOM      = 0x00000200,	// 1�`�w��܂Ń����_��
	EAS_USEMAX      = 0x00000400,	// MAX���x��������΂����
	EAS_USEBETTER   = 0x00000800,	// �w��ȏ�̂��̂�����΂����(MAX����Ȃ��Ă��\)
	EAS_NOSP        = 0x00001000,	// SP0
	EAS_SPCOST1     = 0x00002000,	// SP2/3
	EAS_SPCOST2     = 0x00004000,	// SP1/2
	EAS_SPCOST3     = 0x00008000,	// SP1.5�{
	EAS_ATTACK      = 0x00010000,	// �U��
	EAS_REVENGE     = 0x00020000,	// ����
	EAS_NORMAL      = 0x00040000,	// �ʏ�U��
	EAS_SKILL       = 0x00080000,	// �X�L��
};

// �J�[�h���ʂ̃I�[�g�X�y��
int bonus_autospell(struct block_list *src,struct block_list *bl,unsigned int mode,unsigned int tick,int flag);
int bonus_skillautospell(struct block_list *src,struct block_list *bl,int skillid,unsigned int tick,int flag);

int bonus_activeitem(struct map_session_data* sd,int skillid,int id,short rate,unsigned int tick,unsigned int flag);
int bonus_activeitem_start(struct map_session_data* sd,unsigned int mode,unsigned int tick);
int bonus_activeitemskill_start(struct map_session_data* sd,int skillid,unsigned int tick);

int bonus_param1(struct map_session_data *sd,int type,int val);
int bonus_param2(struct map_session_data *sd,int type,int type2,int val);
int bonus_param3(struct map_session_data *sd,int type,int type2,int type3,int val);
int bonus_param4(struct map_session_data *sd,int type,int type2,int type3,int type4,unsigned int val);

int do_final_bonus(void);
int do_init_bonus(void);

#endif
