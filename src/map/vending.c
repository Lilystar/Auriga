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

/*==========================================
 * �I�X��
 *------------------------------------------
*/
void vending_closevending(struct map_session_data *sd)
{
	nullpo_retv(sd);

	sd->vender_id = 0;
	sd->vend_num  = 0; // on principle
	clif_closevendingboard(&sd->bl,-1);

	return;
}

/*==========================================
 * �I�X�A�C�e�����X�g�v��
 *------------------------------------------
 */
void vending_vendinglistreq(struct map_session_data *sd,int id)
{
	struct map_session_data *vsd;

	nullpo_retv(sd);

	if((vsd = map_id2sd(id)) == NULL)
		return;
	if(vsd->vender_id == 0 || vsd->state.deal_mode != 0)
		return;
	if(sd->vender_id != 0 || sd->state.deal_mode != 0)
		return;
	if(sd->bl.m != vsd->bl.m)
		return;
	if(unit_distance(sd->bl.x,sd->bl.y,vsd->bl.x,vsd->bl.x) > AREA_SIZE)
		return;

	clif_vendinglist(sd, vsd);

	return;
}

/*==========================================
 * �I�X�A�C�e���w��
 *------------------------------------------
 */
void vending_purchasereq(struct map_session_data *sd, unsigned short len, int id, unsigned char *p)
{
	int i, j, w, new_, blank, vend_list[MAX_VENDING];
	double z;
	short amount, idx;
	struct map_session_data *vsd;
	struct vending vending[MAX_VENDING]; // against duplicate packets/items

	nullpo_retv(sd);

	vsd = map_id2sd(id);
	if (vsd == NULL)
		return;
	if (vsd->vender_id == 0)
		return;
	if (vsd->vender_id == sd->bl.id)
		return;
	if (sd->bl.m != vsd->bl.m)
		return;
	if (unit_distance(sd->bl.x,sd->bl.y,vsd->bl.x,vsd->bl.x) > AREA_SIZE)
		return;

	// check number of buying items
	if (len < 8 + 4 || len > 8 + 4 * MAX_VENDING) {
		clif_buyvending(sd, 0, 0x7fff, 4); // not enough quantity (index and amount are unknown)
		return;
	}

	blank = pc_inventoryblank(sd);

	// duplicate items in vending to check hacker with multiple packets
	memcpy(&vending, &vsd->vending, sizeof(struct vending) * MAX_VENDING); // copy vending list

	// some checks
	z = 0.;
	w = 0;
	new_ = 0;
	for(i = 0; 8 + 4 * i < len; i++) {
		amount = *(short*)(p + 4 * i);
		if (amount <= 0)
			return;

		idx = *(short*)(p + 2 + 4 * i) - 2;
		// check of index
		if (idx < 0 || idx >= MAX_CART)
			return;

		for(j = 0; j < vsd->vend_num; j++) {
			if (vsd->vending[j].index == idx) {
				vend_list[i] = j;
				break;
			}
		}
		if (j == vsd->vend_num)
			return; // ����؂�

		z += ((double)vsd->vending[j].value * (double)amount);
		if (z > (double)sd->status.zeny || z < 0. || z > (double)MAX_ZENY) { // fix positiv overflow (buyer)
			clif_buyvending(sd, idx, amount, 1); // you don't have enough zenys
			return; // zeny�s��
		}
		if (z + (double)vsd->status.zeny > (double)MAX_ZENY) { // fix positiv overflow (merchand)
			clif_buyvending(sd, idx, vsd->vending[j].amount, 4); // not enough quantity
			return; // zeny�s��
		}
		w += itemdb_weight(vsd->status.cart[idx].nameid) * amount;
		if (w + sd->weight > sd->max_weight) {
			clif_buyvending(sd, idx, amount, 2); // you can not buy, because overweight
			return; // �d�ʒ���
		}
		// if they try to add packets (example: get twice or more 2 apples if marchand has only 3 apples).
		// here, we check cumulativ amounts
		if (vending[j].amount < amount) { // send more quantity is not a hack (an other player can have buy items just before)
			clif_buyvending(sd, idx, vsd->vending[j].amount, 4); // not enough quantity
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
	pc_payzeny(sd, (int)z);
	// �ŋ�����������
	if(battle_config.tax_rate)
		z = z * (100 - battle_config.tax_rate) / 100;
	// �[�j�[�󂯎��
	pc_getzeny(vsd, (int)z);
	// vending items
	for(i = 0; 8 + 4 * i < len; i++) {
		amount = *(short*)(p + 4 * i);
		idx = *(short*)(p + 2 + 4 * i) - 2;
		pc_additem(sd, &vsd->status.cart[idx], amount);
		vsd->vending[vend_list[i]].amount -= amount;
		pc_cart_delitem(vsd, idx, amount, 0);
		clif_vendingreport(vsd, idx, amount);
		if (battle_config.buyer_name) {
			char output[128];
			snprintf(output, sizeof(output), msg_txt(148), sd->status.name);
			clif_disp_onlyself(vsd->fd, output);
		}
	}

	// save both players to avoid crash: they always have no advantage/disadvantage between the 2 players
	chrif_save(sd,0);
	chrif_save(vsd,0);

	return;
}

/*==========================================
 * �I�X�J��
 *------------------------------------------
 */
void vending_openvending(struct map_session_data *sd, int len, char *shop_title, unsigned char flag, unsigned char *p)
{
	int i, vending_skill_lvl;

	nullpo_retv(sd);

	// has vender ability to open a shop?
	vending_skill_lvl = pc_checkskill(sd, MC_VENDING);
	if (vending_skill_lvl < 1 || !pc_iscarton(sd)) {
		clif_skill_fail(sd, MC_VENDING, 0, 0);
		return;
	}

	// player must close its actual shop before
	if (sd->vender_id != 0)
		return;

	// normal client can not send 'void' shop title
	if (shop_title[0] == '\0')
		return;

	if (sd->state.deal_mode != 0)
		return;

	if (sd->npc_id)
		npc_event_dequeue(sd);
	if (sd->trade_partner)
		trade_tradecancel(sd);
	if (sd->chatID)
		chat_leavechat(sd, 0);

	// normal client send NULL -> force it (against hacker)
	shop_title[79] = '\0';

	if (flag) {
		len -= 85;
		// check if at least 1 item, and not more than possible
		if (len < 8 || len > 8 * MAX_VENDING || len > 8 * (2 + vending_skill_lvl)) {
			clif_skill_fail(sd, MC_VENDING, 0, 0);
			return;
		}

		memset(&sd->vending[0], 0, sizeof(struct vending) * MAX_VENDING);

		// check if items are valid
		for(i = 0; (8 * i) < len; i++) {
			short idx, amount;
			idx = *(short*)(p + 8 * i) - 2;
			amount = *(short*)(p + 2 + 8 * i);
			if (idx < 0 || idx >= MAX_CART || amount <= 0) {
				memset(&sd->vending[0], 0, sizeof(struct vending) * MAX_VENDING);
				clif_skill_fail(sd, MC_VENDING, 0, 0);
				return;
			}

			sd->vending[i].index  = idx;
			sd->vending[i].amount = amount;
			sd->vending[i].value  = *(int*)(p + 4 + 8 * i);

			// �J�[�g���̃A�C�e�����Ɣ̔�����A�C�e�����ɑ��Ⴊ�������璆�~
			// ���łɁ|�l�̒l�i��|�l�̌��`�F�b�N������Ă܂�
			if (pc_cartitem_amount(sd, idx, amount) < 0) {
				memset(&sd->vending[0], 0, sizeof(struct vending) * MAX_VENDING);
				clif_skill_fail(sd, MC_VENDING, 0, 0);
				return;
			}

			if (sd->vending[i].value > battle_config.vending_max_value) {
				sd->vending[i].value = battle_config.vending_max_value;
			} else if (sd->vending[i].value < 0) { // hack
				memset(&sd->vending[0], 0, sizeof(struct vending) * MAX_VENDING);
				clif_skill_fail(sd, MC_VENDING, 0, 0);
				return;
			}
		}

		// shop can be opened
		unit_stop_walking(&sd->bl, 1);
		unit_stopattack(&sd->bl);

		sd->vender_id = sd->bl.id;
		sd->vend_num  = i;
		memset(sd->message, 0, sizeof(sd->message));
		strncpy(sd->message, shop_title, 80);
		if (clif_openvending(sd) > 0) {
			clif_showvendingboard(&sd->bl, shop_title, -1);
		} else {
			sd->vender_id = 0;
			sd->vend_num = 0; // on principle
		}
	}

	return;
}
