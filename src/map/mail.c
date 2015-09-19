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
void mail_setitem(struct map_session_data *sd,int idx,int amount)
{
	nullpo_retv(sd);

	idx -= 2;
	if(idx == -2) {
		if(sd->status.zeny < amount)
			amount = sd->status.zeny;
		sd->mail_append.zeny = amount;
	} else {
		if(idx < 0 || idx >= MAX_INVENTORY)
			return;

		if( sd->status.inventory[idx].nameid > 0 &&
		    sd->status.inventory[idx].amount >= amount &&
		    itemdb_isdropable(sd->status.inventory[idx].nameid) ) {
			sd->mail_append.amount = amount;
			sd->mail_append.index  = idx;
		} else {
			// �Y�t�s��
			clif_res_sendmail_setappend(sd->fd,idx,1);
			return;
		}
	}

	// �Y�t����
	sd->state.mail_appending = 1;
	clif_res_sendmail_setappend(sd->fd,idx,0);

	return;
}

/*==========================================
 * �Y�t�A�C�e����������
 *------------------------------------------
 */
int mail_removeitem(struct map_session_data *sd, int flag)
{
	nullpo_retr(0, sd);

	if(!(flag&2) && sd->mail_append.index >= 0) {
		clif_additem(sd, sd->mail_append.index, sd->mail_append.amount, 0);	// �A�C�e���ԋp
		sd->mail_append.index  = -1;
		sd->mail_append.amount = 0;
	}
	if(!(flag&1) && sd->mail_append.zeny > 0) {
		clif_updatestatus(sd, SP_ZENY);		// Zeny�ԋp
		sd->mail_append.zeny = 0;
	}

	if(sd->mail_append.index == -1 && sd->mail_append.zeny == 0)
		sd->state.mail_appending = 0;

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

	if(sd->mail_append.zeny < 0 || sd->mail_append.zeny > sd->status.zeny)
		return 1;

	md->zeny = sd->mail_append.zeny;

	if(sd->mail_append.amount > 0) {
		int idx = sd->mail_append.index;
		if( idx < 0 ||
		    sd->status.inventory[idx].nameid <= 0 ||
		    sd->status.inventory[idx].amount < sd->mail_append.amount )
			return 1;

		memcpy(&md->item, &sd->status.inventory[idx], sizeof(struct item));
		md->item.amount = sd->mail_append.amount;
		pc_delitem(sd, idx, md->item.amount, 1);
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
	struct map_session_data *rd = map_nick2sd(name);
	struct mail_data md;

	nullpo_retr(0, sd);

	// �{���̒������s���A�܂��͑��M���肪�����Ȃ�_��
	if(len <= 0 || len > sizeof(md.body) || rd == sd) {
		clif_res_sendmail(sd->fd,1);
		mail_removeitem(sd,0);
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

	sd->mail_append.index    = -1;
	sd->mail_append.amount   = 0;
	sd->mail_append.zeny     = 0;
	sd->state.mail_appending = 0;

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
		if(item->nameid > 0 && item->amount > 0) {
			if(pc_additem(sd,item,item->amount))
				clif_mail_getappend(sd->fd,1);
			else
				clif_mail_getappend(sd->fd,0);
		}
	}
	return 0;
}
