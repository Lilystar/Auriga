#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "nullpo.h"
#include "malloc.h"

#include "map.h"
#include "mail.h"
#include "clif.h"
#include "intif.h"
#include "itemdb.h"
#include "pc.h"

/*==========================================
 * �A�C�e����Zeny��Y�t
 *------------------------------------------
 */
int mail_setitem(struct map_session_data *sd,int idx,int amount)
{
	nullpo_retr(-1, sd);

	if(idx == 0) {
		if(sd->status.zeny < amount)
			amount = sd->status.zeny;
		sd->mail_zeny = amount;
	} else {
		idx -= 2;
		if(idx >= 0 && idx < MAX_INVENTORY) {
			if(sd->mail_item.nameid > 0 && sd->mail_item.amount > amount) {
				sd->mail_item.amount -= amount;
			} else if(sd->status.inventory[idx].amount >= amount) {
				if(itemdb_isdropable(sd->status.inventory[idx].nameid) == 0)
					return 2;
				memcpy(&sd->mail_item,&sd->status.inventory[idx],sizeof(struct item));
				sd->mail_amount = amount;
				return 0;
			}
			return 1;
		}
	}
	return -1;
}

/*==========================================
 * �Y�t�A�C�e����������
 *------------------------------------------
 */
int mail_removeitem(struct map_session_data *sd)
{
	nullpo_retr(0, sd);

	memset(&sd->mail_item,0,sizeof(struct item));
	sd->mail_amount = 0;
	sd->mail_zeny   = 0;
	return 0;
}

/*==========================================
 * �Y�t�A�C�e����Zeny���`�F�b�N���Č��炷
 *------------------------------------------
 */
static int mail_checkappend(struct map_session_data *sd,struct mail_data *md)
{
	nullpo_retr(1, sd);
	nullpo_retr(1, md);

	if(sd->mail_zeny < 0 || sd->mail_zeny > sd->status.zeny)
		return 0;

	md->zeny = sd->mail_zeny;

	if(sd->mail_item.nameid > 0 && sd->mail_amount > 0) {
		int i,idx = -1;
		for(i=0; i<MAX_INVENTORY; i++) {
			if(memcmp(&sd->mail_item,&sd->status.inventory[i],sizeof(struct item)) == 0) {
				idx = i;
				break;
			}
		}
		if(idx < 0 || sd->status.inventory[idx].amount < sd->mail_amount)
			return 1;

		memcpy(&md->item,&sd->mail_item,sizeof(struct item));
		md->item.amount = sd->mail_amount;
		pc_delitem(sd,idx,md->item.amount,0);
	}
	sd->status.zeny -= md->zeny;
	clif_updatestatus(sd,SP_ZENY);

	return 0;
}

/*==========================================
 * ���M�O�`�F�b�N
 *------------------------------------------
 */
int mail_checkmail(struct map_session_data *sd,char *name,char *title,char *body,int len)
{
	struct map_session_data *rd;
	struct mail_data md;

	nullpo_retr(0, sd);

	// �Z�����܂��͒������A��������
	if(len <= 0 || len > sizeof(md.body)) {
		clif_res_sendmail(sd->fd,2);	// ���b�Z�[�W���o�Ȃ��p�P�b�g�𑗂�K�v������
		mail_removeitem(sd);
		return 0;
	}

	rd = map_nick2sd(name);
	if(rd == sd) {		// �����̓_��
		clif_res_sendmail(sd->fd,1);
		mail_removeitem(sd);
		return 0;
	}

	memset(&md, 0, sizeof(md));

	strncpy(md.char_name, sd->status.name, 24);
	md.char_id = sd->status.char_id;

	memcpy(md.receive_name, name, 24);
	md.receive_name[23] = '\0';	// force \0 terminal
	strncpy(md.title, title, sizeof(md.title));
	md.title[sizeof(md.title)-1] = '\0';

	memcpy(md.body, body, len);
	md.body_size = len;

	if(rd)
		mail_sendmail(sd,&md);
	else
		intif_mail_checkmail(sd->status.account_id,&md);	// �󂯎��l�����邩Inter�T�[�o�Ɋm�F�v��

	return 0;
}

/*==========================================
 * ���M��Inter��
 *------------------------------------------
 */
int mail_sendmail(struct map_session_data *sd,struct mail_data *md)
{
	nullpo_retr(0, sd);
	nullpo_retr(0, md);

	// ���t�̕ۑ�
	md->times = (unsigned int)time(NULL);
	// �A�C�e���EZeny�`�F�b�N
	if(mail_checkappend(sd,md) == 0)
		intif_sendmail(md);

	mail_removeitem(sd);
	return 0;
}

/*==========================================
 * �Y�t�A�C�e����Zeny���擾
 *------------------------------------------
 */
int mail_getappend(int account_id,int zeny,struct item *item)
{
	struct map_session_data *sd;

	nullpo_retr(0, item);

	sd = map_id2sd(account_id);
	if(sd) {
		if(zeny > 0) {
			sd->status.zeny += zeny;
			clif_updatestatus(sd,SP_ZENY);
		}
		if(item->nameid > 0 && item->amount > 0)
			pc_additem(sd,item,item->amount);
	}
	return 0;
}
