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
#include <string.h>

#include "clif.h"
#include "itemdb.h"
#include "map.h"
#include "vending.h"
#include "pc.h"
#include "skill.h"
#include "battle.h"
#include "nullpo.h"
#include "chat.h"
#include "npc.h"
#include "trade.h"
#include "chrif.h"
#include "atcommand.h"
#include "unit.h"
#include "status.h"

// �I�XID
static unsigned int vending_id = 0;

// �I�X�A�C�e���w�����s
enum e_fail_vending
{
	VENDING_FAIL_NOTHING    = 0,
	VENDING_FAIL_ZENY       = 1,
	VENDING_FAIL_WEIGHT     = 2,
	VENDING_FAIL_NOTHING2   = 3,
	VENDING_FAIL_AMOUNT     = 4,
	VENDING_FAIL_TRADING    = 5,
	VENDING_FAIL_INVALIDVID = 6,
};

/*==========================================
 * �I�X��
 *------------------------------------------
*/
void vending_closevending(struct map_session_data *sd)
{
	nullpo_retv(sd);

	if(sd->state.store == STORE_TYPE_VENDING)
	{
		sd->vend_num  = 0; // on principle
		sd->state.store = STORE_TYPE_NONE;
		clif_closevendingboard(&sd->bl,-1);
	}

	return;
}

/*==========================================
 * �I�X�A�C�e�����X�g�v��
 *------------------------------------------
 */
void vending_vendinglistreq(struct map_session_data *sd, int id)
{
	struct map_session_data *vsd;

	nullpo_retv(sd);

	if( (vsd = map_id2sd(id)) == NULL )
		return;
	if( vsd->bl.prev == NULL )
		return;
	if( vsd->state.store != STORE_TYPE_VENDING )
		return;
	if( sd->state.store )
		return;
	if( vsd->state.deal_mode != 0 || sd->state.deal_mode != 0 )
	{
		clif_buyvending(sd, 0, 0, VENDING_FAIL_TRADING);
		return;
	}
	if( sd->bl.m != vsd->bl.m )
		return;
	if( unit_distance(sd->bl.x,sd->bl.y,vsd->bl.x,vsd->bl.y) > AREA_SIZE )
		return;

	clif_vendinglist(sd, vsd);

	return;
}

/*==========================================
 * �I�X�A�C�e���w��
 *------------------------------------------
 */
void vending_purchasereq(struct map_session_data *sd, short count, int account_id, unsigned int vender_id, const unsigned char *data)
{
	int i, j, cursor, blank, vend_list[MAX_VENDING];
	int weight = 0;
	int new_ = 0;
	double zeny = 0.;
	short amount, idx;
	struct map_session_data *vsd;
	struct vending vending[MAX_VENDING]; // against duplicate packets/items

	nullpo_retv(sd);

	vsd = map_id2sd(account_id);
	if( vsd == NULL )
		return;
	if( vsd->bl.prev == NULL )
		return;
	if( vsd->bl.id == sd->bl.id )
		return;
	if( sd->bl.m != vsd->bl.m )
		return;
	if( unit_distance(sd->bl.x,sd->bl.y,vsd->bl.x,vsd->bl.y) > AREA_SIZE )
		return;
	if( sd->state.store )
		return;
	if( sd->state.deal_mode )
		return;

	// vender_id���Ⴆ�Εs��
	if( vsd->vender_id != vender_id )
	{
		clif_buyvending(sd, 0, 0, VENDING_FAIL_INVALIDVID);
		return;
	}

	// check number of buying items
	if( count < 1 || count > MAX_VENDING || count > vsd->vend_num )
	{
		clif_buyvending(sd, 0, 0x7fff, VENDING_FAIL_AMOUNT); // not enough quantity (index and amount are unknown)
		return;
	}

	blank = pc_inventoryblank(sd);

	// duplicate items in vending to check hacker with multiple packets
	memcpy(&vending, &vsd->vending, sizeof(struct vending) * MAX_VENDING); // copy vending list

	// some checks
	for( i = 0; i < count; i++ )
	{
		short amount = *(short *)(data + 4*i + 0);
		short idx    = *(short *)(data + 4*i + 2) - 2;

		if( amount <= 0 )
			return;
		// check of index
		if( idx < 0 || idx >= MAX_CART )
			return;

		for( j = 0; j < vsd->vend_num; j++ )
		{
			if( vsd->vending[j].index == idx )
			{
				vend_list[i] = j;
				break;
			}
		}
		if( j == vsd->vend_num )
			return; // ����؂�

		zeny += ((double)vsd->vending[j].value * (double)amount);
		if( zeny > (double)sd->status.zeny || zeny < 0. || zeny > (double)MAX_ZENY )
		{ // fix positiv overflow (buyer)
			clif_buyvending(sd, idx, amount, 1); // you don't have enough zenys
			return; // zeny�s��
		}

		if( zeny + (double)vsd->status.zeny > (double)MAX_ZENY )
		{ // fix positiv overflow (merchand)
			clif_buyvending(sd, idx, vsd->vending[j].amount, VENDING_FAIL_ZENY); // not enough quantity
			return; // zeny�s��
		}

		weight += itemdb_weight(vsd->status.cart[idx].nameid) * amount;
		if( weight + sd->weight > sd->max_weight )
		{
			clif_buyvending(sd, idx, amount, VENDING_FAIL_WEIGHT); // you can not buy, because overweight
			return; // �d�ʒ���
		}

		// if they try to add packets (example: get twice or more 2 apples if marchand has only 3 apples).
		// here, we check cumulativ amounts
		if( vending[j].amount < amount )
		{ // send more quantity is not a hack (an other player can have buy items just before)
			clif_buyvending(sd, idx, vsd->vending[j].amount, VENDING_FAIL_AMOUNT); // not enough quantity
			return;
		}
		vending[j].amount -= amount;

		switch(pc_checkadditem(sd, vsd->status.cart[idx].nameid, amount)) {
		case ADDITEM_EXIST:
			break;
		case ADDITEM_NEW:
			new_++;
			if (new_ > blank)
				return; // ��ސ�����
			break;
		case ADDITEM_OVERAMOUNT:
			return; // �A�C�e��������
		}
	}

	// �[�j�[�x����
	pc_payzeny(sd, (int)zeny);
	// �ŋ�����������
	if( battle_config.tax_rate )
		zeny = zeny * (100 - battle_config.tax_rate) / 100;
	// �[�j�[�󂯎��
	pc_getzeny(vsd, (int)zeny);

	// vending items
	for( i = 0; i < count; i++ )
	{
		amount = *(short *)(data + 4*i + 0);
		idx    = *(short *)(data + 4*i + 2) - 2;

		pc_additem(sd, &vsd->status.cart[idx], amount);
		vsd->vending[vend_list[i]].amount -= amount;
		pc_cart_delitem(vsd, idx, amount, 0);
		clif_vendingreport(vsd, idx, amount);
		if( battle_config.buyer_name )
		{
			char output[128];
			snprintf(output, sizeof(output), msg_txt(148), sd->status.name);
			clif_disp_onlyself(vsd->fd, output);
		}
	}

	// vend_num��؂�l�߂�
	for( i = 0, cursor = 0; i < vsd->vend_num; i++ )
	{
		if( vsd->vending[i].amount == 0 )
			continue;
		
		if( cursor != i )
		{
			vsd->vending[cursor].index = vsd->vending[i].index;
			vsd->vending[cursor].amount = vsd->vending[i].amount;
			vsd->vending[cursor].value = vsd->vending[i].value;
		}

		cursor++;
	}
	vsd->vend_num = cursor;

	// save both players to avoid crash: they always have no advantage/disadvantage between the 2 players
	chrif_save(sd,0);
	chrif_save(vsd,0);

	return;
}

/*==========================================
 * �I�X�J��
 *------------------------------------------
 */
void vending_openvending(struct map_session_data *sd, short count, char *shop_title, bool is_open, const unsigned char *data)
{
	int i, vending_skill_lv;

	nullpo_retv(sd);

	if( is_open == false )
		return;

	// has vender ability to open a shop?
	vending_skill_lv = pc_checkskill(sd, MC_VENDING);
	if( vending_skill_lv < 1 || !pc_iscarton(sd) )
	{
		clif_skill_fail(sd, MC_VENDING, 0, 0, 0);
		return;
	}

	// player must close its actual shop before
	if( sd->state.store )
		return;

	// normal client can not send 'void' shop title
	if( shop_title[0] == '\0' )
		return;

	if( sd->state.deal_mode != 0 )
		return;

	if( sd->npc_id )
		npc_event_dequeue(sd);
	if( sd->trade.partner )
		trade_tradecancel(sd);
	if( sd->chatID )
		chat_leavechat(sd, 0);

	// normal client send NULL -> force it (against hacker)
	shop_title[79] = '\0';

	// check if at least 1 item, and not more than possible
	if( count < 1 || count > MAX_VENDING || count > 2 + vending_skill_lv )
	{
		clif_skill_fail(sd, MC_VENDING, 0, 0, 0);
		return;
	}

	memset(&sd->vending[0], 0, sizeof(struct vending) * MAX_VENDING);

	// check if items are valid
	for( i = 0; i < count; i++ )
	{
		short idx    = *(short *)(data + 8*i + 0) - 2;
		short amount = *(short *)(data + 8*i + 2);
		int value    = *(int *)(data + 8*i + 4);

		// �A�C�e���C���f�b�N�X�A���̃`�F�b�N
		if( idx < 0 || idx >= MAX_CART || amount <= 0 )
		{
			memset(&sd->vending[0], 0, sizeof(struct vending) * MAX_VENDING);
			clif_skill_fail(sd, MC_VENDING, 0, 0, 0);
			return;
		}

		// �I�X�f�[�^�ɃZ�b�g
		sd->vending[i].index  = idx;
		sd->vending[i].amount = amount;
		sd->vending[i].value  = value;

		// �J�[�g���̃A�C�e�����Ɣ̔�����A�C�e�����ɑ��Ⴊ�������璆�~
		// ���łɁ|�l�̒l�i��|�l�̌��`�F�b�N������Ă܂�
		if( pc_cartitem_amount(sd, idx, amount) < 0 )
		{
			memset(&sd->vending[0], 0, sizeof(struct vending) * MAX_VENDING);
			clif_skill_fail(sd, MC_VENDING, 0, 0, 0);
			return;
		}

		// �l�i�`�F�b�N
		if( sd->vending[i].value > battle_config.vending_max_value )
		{
			sd->vending[i].value = battle_config.vending_max_value;
		}
		else if( sd->vending[i].value < 0 )
		{ // hack
			memset(&sd->vending[0], 0, sizeof(struct vending) * MAX_VENDING);
			clif_skill_fail(sd, MC_VENDING, 0, 0, 0);
			return;
		}
	}

	// shop can be opened
	unit_stop_walking(&sd->bl, 1);
	unit_stopattack(&sd->bl);

	sd->vender_id = ++vending_id;
	sd->vend_num  = i;
	sd->state.store = STORE_TYPE_VENDING;
	memset(sd->message, 0, sizeof(sd->message));
	strncpy(sd->message, shop_title, 80);
	if( clif_openvending(sd) > 0 )
	{
		clif_showvendingboard(&sd->bl, shop_title, -1);
	}
	else
	{
		sd->vender_id = 0;
		sd->vend_num = 0; // on principle
		sd->state.store = STORE_TYPE_NONE;
	}

	return;
}
