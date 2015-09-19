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

#ifndef _UNIT_H_
#define _UNIT_H_

// PC, MOB, PET �ɋ��ʂ��鏈�����P�ɂ܂Ƃ߂�v��

// ���s�J�n
//     �߂�l�́A0 ( ���� ), 1 ( ���s )
int unit_walktoxy( struct block_list *bl, int x, int y);

int unit_walktodir(struct block_list *bl,int step);
int unit_forcewalktodir(struct block_list *bl,int distance);

int unit_distance2( struct block_list *bl, struct block_list *bl2);
// ���s��~
// type�͈ȉ��̑g�ݍ��킹 :
//     1: �ʒu���̑��M( ���̊֐��̌�Ɉʒu���𑗐M����ꍇ�͕s�v )
//     2: �_���[�W�f�B���C�L��
//     4: �s��(MOB�̂݁H)
int unit_stop_walking(struct block_list *bl,int type);

// �ʒu�ړ�(������΂��Ȃ�)
int unit_movepos(struct block_list *bl,int dst_x,int dst_y,int flag);
int unit_setdir(struct block_list *bl,int dir);

// �����܂ŕ��s�ł��ǂ蒅���邩�̔���
int unit_can_reach(struct block_list *bl,int x,int y);

// �ړ��\�ȏ�Ԃ��̔���
int unit_can_move(struct block_list *bl);
int unit_isrunning(struct block_list *bl);

// �U���֘A
void unit_stopattack(struct block_list *bl);
int unit_attack(struct block_list *src,int target_id,int type);

// int unit_setpos( struct block_list *bl, const char* map, int x, int y);

// �X�L���g�p
int unit_skilluse_id(struct block_list *src, int target_id, int skill_num, int skill_lv);
int unit_skilluse_pos(struct block_list *src, int skill_x, int skill_y, int skill_num, int skill_lv);

// �X�L���g�p( �␳�ς݃L���X�g���ԁA�L�����Z���s�ݒ�t�� )
int unit_skilluse_id2(struct block_list *src, int target_id, int skill_num, int skill_lv, int casttime, int castcancel);
int unit_skilluse_pos2( struct block_list *src, int skill_x, int skill_y, int skill_num, int skill_lv, int casttime, int castcancel);

// �r���L�����Z��
int unit_skillcastcancel(struct block_list *bl,int type);

int unit_counttargeted(struct block_list *bl,int target_lv);

// unit_data �̏���������
int unit_dataset(struct block_list *bl);

int unit_heal(struct block_list *bl,int hp,int sp);
int unit_fixdamage(struct block_list *src,struct block_list *target,unsigned int tick,int sdelay,int ddelay,int damage,int div,int type,int damage2);
// ���̑�
int unit_isdead(struct block_list *bl);
int unit_iscasting(struct block_list *bl);
int unit_iswalking(struct block_list *bl);

struct unit_data* unit_bl2ud(struct block_list *bl);
int unit_remove_map(struct block_list *bl, int clrtype, int flag);
int unit_distance(int x0,int y0,int x1,int y1);
int unit_free(struct block_list *bl, int clrtype);
int unit_changeviewsize(struct block_list *bl,int size);

// ���������[�`��
int do_init_unit(void);
int do_final_unit(void);

#endif /* _UNIT_H_ */
