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

#include "nullpo.h"
#include "utils.h"
#include "mmo.h"

#include "battle.h"
#include "buyingstore.h"
#include "chat.h"
#include "chrif.h"
#include "clif.h"
#include "itemdb.h"
#include "map.h"
#include "path.h"
#include "npc.h"
#include "pc.h"
#include "trade.h"
#include "unit.h"

// ����I�XID
static unsigned int buyingstore_id = 0;

// ����I�X�J�ݎ��s��`
enum e_open_failstore
{
	FAILED_OPEN_INVALIDDATA	= 1,
	FAILED_OPEN_WEIGHT		= 2,
	FAILED_OPEN_NODATA		= 8,
};

// ����I�X����`
enum e_close_store
{
	CLOSE_ZENY		= 3,
	CLOSE_NOITEM	= 4,
};

// ����I�X������莸�s��`
enum e_trade_failstore
{
	FAILED_TRADE_INVALIDDATA	= 5,
	FAILED_TRADE_COUNT			= 6,
	FAILED_TRADE_ZENY			= 7,
};

/*==========================================
 * ����I�X�E�C���h�E�\������
 *
 * @param sd ����I�X�J�ݗv����
 * @param count ����I�X�ő�o�^�A�C�e����
 * @return false�Ȃ�E�C���h�E�\���s���
 *------------------------------------------
 */
bool buyingstore_openstorewindow(struct map_session_data *sd, unsigned char count)
{
	nullpo_retr(false, sd);

	// ����I�X�E�C���h�E���J�����Ԃ��`�F�b�N
	if( sd->state.store || sd->state.deal_mode )
	{
		return false;
	}

	// count�͈̔̓`�F�b�N
	if( count > MAX_BUYINGSTORE_COUNT )
	{
		return false;
	}

	sd->buyingstore.count = count;
	clif_openwindow_buyingstore(sd);

	return true;
}

/*==========================================
 * ����I�X�J�ݏ���
 *
 * @param sd ����I�X�J�ݗv����
 * @param limit_zeny �ő唃�����z
 * @param result true�Ȃ�J�݂���
 * @param store_name ����I�X��
 * @param data �A�C�e���f�[�^
 * @param count �A�C�e���f�[�^��
 *------------------------------------------
 */
void buyingstore_openstore(struct map_session_data *sd, int limit_zeny, bool result, const char *store_name, const unsigned char *data, int count)
{
	int i, weight;
	struct item_data *id;

	nullpo_retv(sd);

	// ����I�X�L�����Z��
	if( result == false )
		return;

	// ���z�͈̔̓`�F�b�N
	if( limit_zeny <= 0 || limit_zeny > sd->status.zeny )
	{
		sd->buyingstore.count = 0;
		clif_failed_openbuyingstore(sd, FAILED_OPEN_INVALIDDATA, 0);
		return;
	}

	// count�͈̔̓`�F�b�N
	if( count <= 0 || count > sd->buyingstore.count )
	{
		sd->buyingstore.count = 0;
		clif_failed_openbuyingstore(sd, FAILED_OPEN_INVALIDDATA, 0);
		return;
	}

	// �I�X�J�ݒ��͊J�ݕs��
	if( sd->state.store )
	{
		sd->buyingstore.count = 0;
		clif_failed_openbuyingstore(sd, FAILED_OPEN_INVALIDDATA, 0);
		return;
	}

	// ������͊J�ݕs��
	if( sd->state.deal_mode )
	{
		sd->buyingstore.count = 0;
		clif_failed_openbuyingstore(sd, FAILED_OPEN_INVALIDDATA, 0);
		return;
	}

	// �擪��null�������`�F�b�N
	if( store_name[0] == '\0' )
	{
		sd->buyingstore.count = 0;
		clif_failed_openbuyingstore(sd, FAILED_OPEN_INVALIDDATA, 0);
		return;
	}

	// �f�^�b�`������
	if( sd->npc_id )
		npc_event_dequeue(sd);
	if( sd->trade.partner )
		trade_tradecancel(sd);
	if( sd->chatID )
		chat_leavechat(sd, 0);

	// �v���C���[�̏d�ʎ擾
	weight = sd->weight;

	// ����I�X�f�[�^������
	memset(&sd->buyingstore, 0, sizeof(struct buyingstore));

	// �A�C�e�����X�g�̃`�F�b�N
	for( i = 0; i < count; i++ )
	{
		short nameid = *(short *)(data + 8*i + 0);
		short amount = *(short *)(data + 8*i + 2);
		int value    = *(int *)(data + 8*i + 4);
		int idx;

		// �������������Ă���A�C�e���łȂ��Ɣ���o���Ȃ�
		if( (idx = pc_search_inventory(sd, nameid)) == -1 )
		{
			memset(&sd->buyingstore, 0, sizeof(struct buyingstore));
			clif_failed_openbuyingstore(sd, FAILED_OPEN_INVALIDDATA, 0);
			return;
		}

		// ���̃`�F�b�N
		if( amount <= 0 || sd->status.inventory[idx].amount+amount > battle_config.max_buyingstore_amount )
		{
			memset(&sd->buyingstore, 0, sizeof(struct buyingstore));
			clif_failed_openbuyingstore(sd, FAILED_OPEN_INVALIDDATA, 0);
			return;
		}

		// �A�C�e��ID�̑��݃`�F�b�N
		if( (id = itemdb_exists(nameid)) == NULL )
		{
			memset(&sd->buyingstore, 0, sizeof(struct buyingstore));
			clif_failed_openbuyingstore(sd, FAILED_OPEN_INVALIDDATA, 0);
			return;
		}

		// ���z�`�F�b�N
		if( value <= 0 || value > battle_config.max_buyingstore_zeny )
		{
			memset(&sd->buyingstore, 0, sizeof(struct buyingstore));
			clif_failed_openbuyingstore(sd, FAILED_OPEN_INVALIDDATA, 0);
			return;
		}

		// ����I�X�Ŕ������\�ȃA�C�e�����`�F�b�N
		if( itemdb_isbuyingable(nameid) != 1 )
		{
			memset(&sd->buyingstore, 0, sizeof(struct buyingstore));
			clif_failed_openbuyingstore(sd, FAILED_OPEN_INVALIDDATA, 0);
			return;
		}

		// �d������A�C�e��ID���Ȃ����`�F�b�N
		if( i )
		{
			int j;
			for( j = 0; j < i; j++ )
			{
				if( sd->buyingstore.item[j].nameid == nameid )
				{
					memset(&sd->buyingstore, 0, sizeof(struct buyingstore));
					clif_failed_openbuyingstore(sd, FAILED_OPEN_INVALIDDATA, 0);
					return;
				}
			}
		}

		// ����I�X�f�[�^�ɃZ�b�g
		weight += id->weight * amount;
		sd->buyingstore.item[i].nameid = nameid;
		sd->buyingstore.item[i].amount = amount;
		sd->buyingstore.item[i].value  = value;
	}

	// �d�ʃ`�F�b�N
	if( (sd->max_weight*90) / 100 < weight )
	{
		memset(&sd->buyingstore, 0, sizeof(struct buyingstore));
		clif_failed_openbuyingstore(sd, FAILED_OPEN_WEIGHT, 0);
		return;
	}

	// �ړ��ƍU�����~�߂�
	unit_stop_walking(&sd->bl, 1);
	unit_stopattack(&sd->bl);

	// ����I�X�I�[�v��
	sd->state.store = STORE_TYPE_BUYINGSTORE;
	sd->buyer_id = ++buyingstore_id;
	sd->buyingstore.limit_zeny = limit_zeny;
	sd->buyingstore.count = i;
	strncpy(sd->message, store_name, 80);
	sd->message[79] = '\0';

	clif_showmylist_buyingstore(sd);
	clif_show_buyingstore(&sd->bl, sd->message, -1);

	return;
}

/*==========================================
 * ����I�X������
 *
 * @param sd ����I�X���v����
 *------------------------------------------
 */
void buyingstore_close(struct map_session_data *sd)
{
	nullpo_retv(sd);

	if( sd->state.store == STORE_TYPE_BUYINGSTORE )
	{
		sd->state.store = STORE_TYPE_NONE;
		memset(&sd->buyingstore, 0, sizeof(struct buyingstore));
		clif_close_buyingstore(&sd->bl, -1);
	}

	return;
}

/*==========================================
 * ����I�X�A�C�e�����X�g�\��
 *
 * @param sd ����I�X�A�C�e�����X�g�񗗗v����
 * @param account_id ����I�X�J�ݎ҃A�J�E���gID
 *------------------------------------------
 */
void buyingstore_itemlist(struct map_session_data* sd, int account_id)
{
	struct map_session_data *ssd;

	nullpo_retv(sd);

	// �Ώۂ����݂��邩�`�F�b�N
	if( (ssd = map_id2sd(account_id)) == NULL )
		return;

	// �Ώۂ�block_list���甲���Ă��Ȃ����`�F�b�N
	if( ssd->bl.prev == NULL )
		return;

	// �ΏۂƎ��g���������`�F�b�N
	if( ssd->bl.id == sd->bl.id )
		return;

	// ���g���I�X�J�ݒ����`�F�b�N
	if( sd->state.store )
		return;

	// �Ώۂ�����I�X�����
	if( ssd->state.store != STORE_TYPE_BUYINGSTORE )
		return;

	// �Ώۂ�������������͎��g����������`�F�b�N
	if( ssd->state.deal_mode != 0 || sd->state.deal_mode != 0 )
		return;

	// �ΏۂƎ��g������MAP���`�F�b�N
	if( sd->bl.m != ssd->bl.m )
		return;

	// �ΏۂƎ��g��AREA_SIZE���ɋ��邩�`�F�b�N
	if( path_distance(sd->bl.x,sd->bl.y,ssd->bl.x,ssd->bl.y) > AREA_SIZE )
		return;

	// �A�C�e�����X�g�𑗂�
	clif_itemlist_buyingstore(sd,ssd);

	return;
}

/*==========================================
 * ����I�X�A�C�e�����p����
 *
 * @param sd ���p��
 * @param account_id ����I�X�J�ݎ҃A�J�E���gID
 * @param buyer_id ����I�X�J�ݎ҃X�g�AID
 * @param data �A�C�e���f�[�^
 * @param count �A�C�e���f�[�^��
 *------------------------------------------
 */
void buyingstore_sell(struct map_session_data *sd, int account_id, unsigned int buyer_id, const unsigned char *data, int count)
{
	double zeny = 0.;
	int new_ = 0;
	int blank;
	int i, weight, listidx;
	struct map_session_data *ssd;

	nullpo_retv(sd);

	// �Ώۂ����݂��邩�`�F�b�N
	if( (ssd = map_id2sd(account_id)) == NULL )
	{
		clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
		return;
	}

	// �Ώۂ�block_list���甲���Ă��Ȃ����`�F�b�N
	if( ssd->bl.prev == NULL )
	{
		clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
		return;
	}

	// �ΏۂƎ��g���������`�F�b�N
	if( ssd->bl.id == sd->bl.id )
	{
		clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
		return;
	}

	// �ΏۂƎ��g������MAP���`�F�b�N
	if( ssd->bl.m != sd->bl.m )
	{
		clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
		return;
	}

	// �ΏۂƎ��g��AREA_SIZE�����`�F�b�N
	if( path_distance(sd->bl.x,sd->bl.y,ssd->bl.x,ssd->bl.y) > AREA_SIZE )
	{
		clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
		return;
	}

	// �Ώۂ��I�X�J�ݒ����`�F�b�N
	if( ssd->state.store != STORE_TYPE_BUYINGSTORE )
	{
		clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
		return;
	}

	// buyer_id���Ⴆ�Εs��
	if( ssd->buyer_id != buyer_id )
	{
		clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
		return;
	}

	// count�͈̔̓`�F�b�N
	if( count <= 0 || count > ssd->buyingstore.count )
	{
		clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
		return;
	}

	// �I�X�J�ݒ��͔��p�s��
	if( sd->state.store )
	{
		clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
		return;
	}

	// ������͔��p�s��
	if( sd->state.deal_mode )
	{
		clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
		return;
	}

	// ����I�X�J�ݎ҂̏d�ʎ擾
	weight = ssd->weight;

	// ����I�X�J�ݎ҂̋󂫃A�C�e�����̌��擾
	blank = pc_inventoryblank(ssd);

	// �A�C�e���f�[�^�m�F����
	for( i = 0; i < count; i++ )
	{
		short idx    = *(short *)(data + 6*i + 0) - 2;
		short nameid = *(short *)(data + 6*i + 2);
		short amount = *(short *)(data + 6*i + 4);

		// index�̏d���`�F�b�N
		if( i )
		{
			int j;
			for( j = 0; j < i; j++ )
			{
				short idx_tmp = *(short *)(data + 6*j + 0) - 2;
				if( idx == idx_tmp )
				{
					clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
					return;
				}
			}
		}

		// index�͈̔̓`�F�b�N
		if( idx < 0 || idx >= MAX_INVENTORY )
		{
			clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
			return;
		}

		// index�ɃA�C�e���͑��݂��邩�`�F�b�N
		if( sd->inventory_data[idx] == NULL )
		{
			clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
			return;
		}

		// �A�C�e��ID�̃`�F�b�N
		if( sd->status.inventory[idx].nameid != nameid )
		{
			clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
			return;
		}

		// ���̃`�F�b�N
		if( amount <= 0 || sd->status.inventory[idx].amount < amount )
		{
			clif_failed_tradebuyingstore(sd, FAILED_TRADE_COUNT, 0);
			return;
		}

		// �������I���`�F�b�N
		for( listidx = 0; listidx < ssd->buyingstore.count; listidx++ )
		{
			if( ssd->buyingstore.item[listidx].nameid == nameid )
			{
				if( listidx >= ssd->buyingstore.count || ssd->buyingstore.item[listidx].amount <= 0 )
				{
					clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
					return;
				}
				break;
			}
		}

		// ���`�F�b�N
		if( ssd->buyingstore.item[listidx].amount < amount )
		{
			clif_failed_tradebuyingstore(sd, FAILED_TRADE_COUNT, 0);
			return;
		}

		// �d�ʌv�Z
		weight += itemdb_weight(sd->inventory_data[idx]->nameid) * amount;

		// �d�ʃ`�F�b�N
		if( weight > ssd->max_weight )
		{
			clif_failed_tradebuyingstore(sd, FAILED_TRADE_INVALIDDATA, 0);
			return;
		}

		// Zeny�v�Z
		zeny += amount * ssd->buyingstore.item[listidx].value;

		// Zeny�`�F�b�N
		if( sd->status.zeny > MAX_ZENY - zeny )
		{
			// ����ΏۃA�C�e���̍��v���z���A�L�����N�^�[�������\�ȍő���z(2,147,483,647 Zeny)�𒴉߂��Ă��܂��B
			clif_msgstringtable(sd, 0x74e);
			return;
		}

		// ���������x�`�F�b�N
		if( zeny > ssd->buyingstore.limit_zeny )
		{
			clif_failed_tradebuyingstore(sd, FAILED_TRADE_ZENY, 0);
			return;
		}

		// �C���x���g���󂫃`�F�b�N
		switch(pc_checkadditem(ssd, sd->inventory_data[idx]->nameid, amount))
		{
			case ADDITEM_EXIST:
				break;
			case ADDITEM_NEW:
				new_++;
				if (new_ > blank)
					return;
				break;
			case ADDITEM_OVERAMOUNT:
				return;
		}
	}

	// ������菈��
	for( i = 0; i < count; i++ )
	{
		short idx    = *(short *)(data + 6*i + 0) - 2;
		short nameid = *(short *)(data + 6*i + 2);
		short amount = *(short *)(data + 6*i + 4);

		// ������菤�i��Zeny�v�Z
		for( listidx = 0; listidx < ssd->buyingstore.count; listidx++ )
		{
			if( ssd->buyingstore.item[listidx].nameid == nameid )
			{
				zeny = amount * ssd->buyingstore.item[listidx].value;
				break;
			}
		}

		// �A�C�e���̈ړ�
		pc_additem(ssd, &sd->status.inventory[idx], amount);
		pc_delitem(sd, idx, amount, 1, 0);
		ssd->buyingstore.item[listidx].amount -= amount;

		// Zeny�̎x����
		pc_payzeny(ssd, (int)zeny);
		pc_getzeny(sd, (int)zeny);
		ssd->buyingstore.limit_zeny -= (int)zeny;

		// �N���C�A���g�֑��M
		clif_delete_buyingstore(sd, idx, amount, ssd->buyingstore.item[listidx].value);
		clif_update_buyingstore(ssd, nameid, amount);
	}

	// �������i���`�F�b�N
	for( i = 0; (i < ssd->buyingstore.count) && (ssd->buyingstore.item[i].amount <= 0); i++ );
	if( i == ssd->buyingstore.count )
	{
		clif_failed_trybuyingstore(ssd, CLOSE_NOITEM);
		buyingstore_close(ssd);
	}

	// limit_zeny�̃`�F�b�N
	if( ssd->buyingstore.limit_zeny <= 0 )
	{
		clif_failed_trybuyingstore(ssd, CLOSE_ZENY);
		buyingstore_close(ssd);
	}

	// �f�[�^���Z�[�u
	chrif_save(sd,0);
	chrif_save(ssd,0);

	return;
}
