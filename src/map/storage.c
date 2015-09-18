// storage.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "malloc.h"
#include "nullpo.h"
#include "itemdb.h"
#include "clif.h"
#include "intif.h"
#include "pc.h"
#include "storage.h"
#include "guild.h"
#include "battle.h"

#ifdef MEMWATCH
#include "memwatch.h"
#endif

static struct dbt *storage_db;
static struct dbt *guild_storage_db;

/*==========================================
 * �q�ɓ��A�C�e���\�[�g
 *------------------------------------------
 */
static int storage_comp_item(const void *_i1, const void *_i2){
	struct item *i1=(struct item *)_i1;
	struct item *i2=(struct item *)_i2;

	if (i1->nameid == i2->nameid) {
		return 0;
	} else if (!(i1->nameid) || !(i1->amount)){
		return 1;
	} else if (!(i2->nameid) || !(i2->amount)){
		return -1;
	} else {
		return i1->nameid - i2->nameid;
	}
}

static void sortage_sortitem(struct storage* stor){
	nullpo_retv(stor);

	if(battle_config.storagesort_by_itemid & 1)
		qsort(stor->store_item, MAX_STORAGE, sizeof(struct item), storage_comp_item);

	return;
}

static void sortage_gsortitem(struct guild_storage* gstor){
	nullpo_retv(gstor);

	if(battle_config.storagesort_by_itemid & 2)
		qsort(gstor->store_item, MAX_GUILD_STORAGE, sizeof(struct item), storage_comp_item);

	return;
}

/*==========================================
 * �J�v���q�Ƀf�[�^�x�[�X
 *------------------------------------------
 */
struct storage *account2storage(int account_id)
{
	struct storage *stor;
	stor = (struct storage *)numdb_search(storage_db,account_id);
	if(stor == NULL) {
		stor = (struct storage *)aCalloc(1,sizeof(struct storage));
		stor->account_id=account_id;
		numdb_insert(storage_db,stor->account_id,stor);
	}
	return stor;
}

/*==========================================
 * �J�v���q�ɂ��폜
 *------------------------------------------
 */
void storage_delete(int account_id)
{
	struct storage *stor = (struct storage *)numdb_search(storage_db,account_id);
	if(stor) {
		numdb_erase(storage_db,account_id);
		aFree(stor);
	}

	return;
}

/*==========================================
 * �J�v���q�ɂ��J��
 *------------------------------------------
 */
int storage_storageopen(struct map_session_data *sd)
{
	struct storage *stor;

	nullpo_retr(0, sd);

	if(sd->state.storage_flag == 1)
		return 0;	// ���ɃJ�v���q�ɂ��J���Ă���

	if((stor = (struct storage *)numdb_search(storage_db,sd->status.account_id)) != NULL) {
		if(stor->storage_status)
			return 1;
		stor->storage_status = 1;
	}

	if(sd->state.storage_flag == 2)
		storage_guild_storageclose(sd); // �M���h�q�ɂ����łɊJ���Ă���ꍇ����

	if(stor != NULL) {
		sd->state.storage_flag = 1;
		clif_storageitemlist(sd,stor);
		clif_storageequiplist(sd,stor);
		clif_updatestorageamount(sd,stor);
	} else {
		intif_request_storage(sd->status.account_id);
	}
	return 0;
}

/*==========================================
 * �J�v���q�ɂփA�C�e���ǉ�
 *------------------------------------------
 */
static int storage_additem(struct map_session_data *sd,struct storage *stor,struct item *item_data,int amount)
{
	struct item_data *data;
	int i;

	nullpo_retr(1, sd);
	nullpo_retr(1, stor);
	nullpo_retr(1, item_data);

	if(item_data->nameid <= 0 || amount <= 0)
		return 1;

	nullpo_retr(1, data = itemdb_search(item_data->nameid));

	if(!itemdb_isequip2(data)){
		// �����i�ł͂Ȃ��̂ŁA�����L�i�Ȃ���̂ݕω�������
		for(i=0;i<MAX_STORAGE;i++){
			if(stor->store_item[i].nameid == item_data->nameid &&
				stor->store_item[i].card[0] == item_data->card[0] && stor->store_item[i].card[1] == item_data->card[1] &&
				stor->store_item[i].card[2] == item_data->card[2] && stor->store_item[i].card[3] == item_data->card[3]){
				if(stor->store_item[i].amount+amount > MAX_AMOUNT)
					return 1;
				stor->store_item[i].amount+=amount;
				clif_storageitemadded(sd,stor,i,amount);
				return 0;
			}
		}
	} else if (amount != 1)
		return 1;

	// �����i�������L�i�������̂ŋ󂫗��֒ǉ�
	for(i=0;i<MAX_STORAGE;i++){
		if(stor->store_item[i].nameid==0){
			memcpy(&stor->store_item[i],item_data,sizeof(stor->store_item[0]));
			stor->store_item[i].amount=amount;
			stor->storage_amount++;
			clif_storageitemadded(sd,stor,i,amount);
			clif_updatestorageamount(sd,stor);
			return 0;
		}
	}

	return 1;
}

/*==========================================
 * �J�v���q�ɂ���A�C�e�������炷
 *------------------------------------------
 */
static void storage_delitem(struct map_session_data *sd,struct storage *stor,int n,int amount)
{
	nullpo_retv(sd);
	nullpo_retv(stor);

	if(stor->store_item[n].nameid==0 || stor->store_item[n].amount<amount)
		return;

	stor->store_item[n].amount-=amount;
	if(stor->store_item[n].amount==0){
		memset(&stor->store_item[n],0,sizeof(stor->store_item[0]));
		stor->storage_amount--;
		clif_updatestorageamount(sd,stor);
	}
	clif_storageitemremoved(sd,n,amount);

	return;
}

/*==========================================
 * �J�v���q�ɂ֓����
 *------------------------------------------
 */
void storage_storageadd(struct map_session_data *sd, int idx, int amount)
{
	struct storage *stor;

	nullpo_retv(sd);
	nullpo_retv(stor = account2storage(sd->status.account_id));

	if (!stor->storage_status)
		return;
	if (idx < 0 || idx >= MAX_INVENTORY)
		return;
	if (itemdb_isstorageable(sd->status.inventory[idx].nameid) == 0)
		return;
	if (amount < 1 || amount > sd->status.inventory[idx].amount)
		return;

	if (storage_additem(sd, stor, &sd->status.inventory[idx], amount) == 0)
		pc_delitem(sd, idx, amount, 0);

	return;
}

/*==========================================
 * �J�v���q�ɂ���o��
 *------------------------------------------
 */
void storage_storageget(struct map_session_data *sd, int idx, int amount)
{
	struct storage *stor;
	int flag;

	nullpo_retv(sd);
	nullpo_retv(stor = account2storage(sd->status.account_id));

	if (!stor->storage_status)
		return;
	if (idx < 0 || idx >= MAX_STORAGE)
		return;
	if (amount < 1 || amount > stor->store_item[idx].amount)
		return;

	if ((flag = pc_additem(sd, &stor->store_item[idx], amount)) == 0)
		storage_delitem(sd, stor, idx, amount);
	else
		clif_additem(sd,0,0,flag);

	return;
}

/*==========================================
 * �J�v���q�ɂփJ�[�g��������
 *------------------------------------------
 */
void storage_storageaddfromcart(struct map_session_data *sd, int idx, int amount)
{
	struct storage *stor;

	nullpo_retv(sd);
	nullpo_retv(stor = account2storage(sd->status.account_id));

	if (!stor->storage_status)
		return;
	if (idx < 0 || idx >= MAX_CART)
		return;
	if (itemdb_isstorageable(sd->status.cart[idx].nameid) == 0)
		return;
	if (amount < 1 || amount > sd->status.cart[idx].amount)
		return;

	if (storage_additem(sd, stor, &sd->status.cart[idx], amount) == 0)
		pc_cart_delitem(sd, idx, amount, 0);

	return;
}

/*==========================================
 * �J�v���q�ɂ���J�[�g�֏o��
 *------------------------------------------
 */
void storage_storagegettocart(struct map_session_data *sd, int idx, int amount)
{
	struct storage *stor;

	nullpo_retv(sd);
	nullpo_retv(stor = account2storage(sd->status.account_id));

	if (!stor->storage_status)
		return;
	if (idx < 0 || idx >= MAX_STORAGE)
		return;
	if (itemdb_iscartable(stor->store_item[idx].nameid) == 0)	// �J�[�g�֏o���邩�`�F�b�N
		return;
	if (amount < 1 || amount > stor->store_item[idx].amount)
		return;

	if (pc_cart_additem(sd, &stor->store_item[idx], amount) == 0)
		storage_delitem(sd, stor, idx, amount);

	return;
}

/*==========================================
 * �J�v���q�ɂ����
 *------------------------------------------
 */
void storage_storageclose(struct map_session_data *sd)
{
	struct storage *stor;

	nullpo_retv(sd);
	nullpo_retv(stor = account2storage(sd->status.account_id));

	stor->storage_status=0;
	sd->state.storage_flag = 0;
	clif_storageclose(sd);

	sortage_sortitem(stor);

	return;
}

/*==========================================
 * ���O�A�E�g���J���Ă���J�v���q�ɂ̕ۑ�
 *------------------------------------------
 */
void storage_storage_quit(struct map_session_data *sd)
{
	struct storage *stor;

	nullpo_retv(sd);

	stor = (struct storage *)numdb_search(storage_db,sd->status.account_id);
	if(stor) {
		stor->storage_status = 0;
	}
	sd->state.storage_flag = 0;
	return;
}

/*==========================================
 * �J�v���q�ɂ̕ۑ�
 *------------------------------------------
 */
void storage_storage_save(struct map_session_data *sd)
{
	struct storage *stor;

	nullpo_retv(sd);

	stor = (struct storage *)numdb_search(storage_db,sd->status.account_id);
	if(stor)
		intif_send_storage(stor);

	return;
}


/*==========================================
 * �M���h�q�Ƀf�[�^�x�[�X
 *------------------------------------------
 */
struct guild_storage *guild2storage(int guild_id)
{
	struct guild_storage *gs = NULL;
	if(guild_search(guild_id) != NULL) {
		gs = (struct guild_storage *)numdb_search(guild_storage_db,guild_id);
		if(gs == NULL) {
			gs = (struct guild_storage *)aCalloc(1,sizeof(struct guild_storage));
			gs->guild_id=guild_id;
			numdb_insert(guild_storage_db,gs->guild_id,gs);
		}
	}

	return gs;
}

/*==========================================
 * �M���h�q�ɂ��폜
 *------------------------------------------
 */
void guild_storage_delete(int guild_id)
{
	struct guild_storage *gstor = (struct guild_storage *)numdb_search(guild_storage_db,guild_id);
	if(gstor) {
		numdb_erase(guild_storage_db,guild_id);
		aFree(gstor);
	}

	return;
}

/*==========================================
 * �M���h�q�ɂ��J��
 *------------------------------------------
 */
int storage_guild_storageopen(struct map_session_data *sd)
{
	struct guild_storage *gstor;

	nullpo_retr(0, sd);

	if(sd->status.guild_id <= 0)
		return 2;
	if(sd->state.storage_flag == 2)
		return 3;	// ���ɃM���h�q�ɂ��J���Ă���

	if(sd->state.storage_flag == 1)
		storage_storageclose(sd); // �J�v���q�ɂ����łɊJ���Ă���ꍇ����

	if((gstor = (struct guild_storage *)numdb_search(guild_storage_db,sd->status.guild_id)) != NULL) {
		gstor->storage_status = 1;
		sd->state.storage_flag = 2;
		clif_guildstorageitemlist(sd,gstor);
		clif_guildstorageequiplist(sd,gstor);
		clif_updateguildstorageamount(sd,gstor);
	} else {
		intif_request_guild_storage(sd->status.account_id,sd->status.guild_id);
	}

	return 0;
}

/*==========================================
 * �M���h�q�ɂփA�C�e���ǉ�
 *------------------------------------------
 */
static int guild_storage_additem(struct map_session_data *sd,struct guild_storage *stor,struct item *item_data,int amount)
{
	struct item_data *data;
	int i;

	nullpo_retr(1, sd);
	nullpo_retr(1, stor);
	nullpo_retr(1, item_data);

	if(item_data->nameid <= 0 || amount <= 0)
		return 1;

	nullpo_retr(1, data = itemdb_search(item_data->nameid));

	if(!itemdb_isequip2(data)){
		// �����i�ł͂Ȃ��̂ŁA�����L�i�Ȃ���̂ݕω�������
		for(i=0;i<MAX_GUILD_STORAGE;i++){
			if(stor->store_item[i].nameid == item_data->nameid &&
				stor->store_item[i].card[0] == item_data->card[0] && stor->store_item[i].card[1] == item_data->card[1] &&
				stor->store_item[i].card[2] == item_data->card[2] && stor->store_item[i].card[3] == item_data->card[3]){
				if(stor->store_item[i].amount+amount > MAX_AMOUNT)
					return 1;
				stor->store_item[i].amount+=amount;
				clif_guildstorageitemadded(sd,stor,i,amount);
				return 0;
			}
		}
	} else if (amount != 1)
		return 1;

	// �����i�������L�i�������̂ŋ󂫗��֒ǉ�
	for(i=0;i<MAX_GUILD_STORAGE;i++){
		if(stor->store_item[i].nameid==0){
			memcpy(&stor->store_item[i],item_data,sizeof(stor->store_item[0]));
			stor->store_item[i].amount=amount;
			stor->storage_amount++;
			clif_guildstorageitemadded(sd,stor,i,amount);
			clif_updateguildstorageamount(sd,stor);
			return 0;
		}
	}

	return 1;
}

/*==========================================
 * �M���h�q�ɂ���A�C�e�������炷
 *------------------------------------------
 */
static void guild_storage_delitem(struct map_session_data *sd,struct guild_storage *stor,int n,int amount)
{
	nullpo_retv(sd);
	nullpo_retv(stor);

	if(stor->store_item[n].nameid==0 || stor->store_item[n].amount<amount)
		return;

	stor->store_item[n].amount-=amount;
	if(stor->store_item[n].amount==0){
		memset(&stor->store_item[n],0,sizeof(stor->store_item[0]));
		stor->storage_amount--;
		clif_updateguildstorageamount(sd,stor);
	}
	clif_storageitemremoved(sd,n,amount);

	return;
}

/*==========================================
 * �M���h�q�ɂ֓����
 *------------------------------------------
 */
void storage_guild_storageadd(struct map_session_data *sd, int idx, int amount)
{
	struct guild_storage *stor;

	nullpo_retv(sd);

	if ((stor=guild2storage(sd->status.guild_id)) == NULL)
		return;
	if (!stor->storage_status)
		return;
	if (idx < 0 || idx >= MAX_INVENTORY)
		return;
	if (itemdb_isstorageable(sd->status.inventory[idx].nameid) == 0)
		return;
	if (amount < 1 || amount > sd->status.inventory[idx].amount)
		return;

	if (guild_storage_additem(sd, stor, &sd->status.inventory[idx], amount) == 0)
		pc_delitem(sd, idx, amount, 0);

	return;
}

/*==========================================
 * �M���h�q�ɂ���o��
 *------------------------------------------
 */
void storage_guild_storageget(struct map_session_data *sd, int idx, int amount)
{
	struct guild_storage *stor;
	int flag;

	nullpo_retv(sd);

	if ((stor=guild2storage(sd->status.guild_id)) == NULL)
		return;
	if (!stor->storage_status)
		return;
	if (idx < 0 || idx >= MAX_GUILD_STORAGE)
		return;
	if (amount < 1 || amount > stor->store_item[idx].amount)
		return;

	if ((flag = pc_additem(sd,&stor->store_item[idx],amount)) == 0)
		guild_storage_delitem(sd, stor, idx, amount);
	else
		clif_additem(sd,0,0,flag);

	return;
}

/*==========================================
 * �M���h�q�ɂփJ�[�g��������
 *------------------------------------------
 */
void storage_guild_storageaddfromcart(struct map_session_data *sd, int idx, int amount)
{
	struct guild_storage *stor;

	nullpo_retv(sd);

	if ((stor=guild2storage(sd->status.guild_id)) == NULL)
		return;
	if (!stor->storage_status)
		return;
	if (idx < 0 || idx >= MAX_CART)
		return;
	if (itemdb_isstorageable(sd->status.cart[idx].nameid) == 0)
		return;
	if (amount < 1 || amount > sd->status.cart[idx].amount)
		return;

	if (guild_storage_additem(sd, stor, &sd->status.cart[idx], amount) == 0)
		pc_cart_delitem(sd, idx, amount, 0);

	return;
}

/*==========================================
 * �M���h�q�ɂ���J�[�g�֏o��
 *------------------------------------------
 */
void storage_guild_storagegettocart(struct map_session_data *sd, int idx, int amount)
{
	struct guild_storage *stor;

	nullpo_retv(sd);

	if ((stor=guild2storage(sd->status.guild_id)) == NULL)
		return;
	if (!stor->storage_status)
		return;
	if (idx < 0 || idx >= MAX_GUILD_STORAGE)
		return;
	if (itemdb_iscartable(stor->store_item[idx].nameid) == 0)	// �J�[�g�֏o���邩�`�F�b�N
		return;
	if (amount < 1 || amount > stor->store_item[idx].amount)
		return;

	if (pc_cart_additem(sd, &stor->store_item[idx], amount) == 0)
		guild_storage_delitem(sd, stor, idx, amount);

	return;
}

/*==========================================
 * �M���h�q�ɂ����
 *------------------------------------------
 */
void storage_guild_storageclose(struct map_session_data *sd)
{
	struct guild_storage *stor;

	nullpo_retv(sd);

	if((stor=guild2storage(sd->status.guild_id)) != NULL) {
		intif_send_guild_storage(sd->status.account_id,stor);
		stor->storage_status = 0;
		sortage_gsortitem(stor);
	}
	sd->state.storage_flag = 0;
	clif_storageclose(sd);
	intif_unlock_guild_storage(sd->status.guild_id);

	return;
}

/*==========================================
 * ���O�A�E�g���J���Ă���M���h�q�ɂ̕ۑ�
 *------------------------------------------
 */
void storage_guild_storage_quit(struct map_session_data *sd, char flag)
{
	struct guild_storage *stor;

	nullpo_retv(sd);

	stor = (struct guild_storage *)numdb_search(guild_storage_db,sd->status.guild_id);
	if(stor) {
		if(!flag)
			intif_send_guild_storage(sd->status.account_id,stor);
		stor->storage_status = 0;
	}
	sd->state.storage_flag = 0;
	intif_unlock_guild_storage(sd->status.guild_id);

	return;
}

/*==========================================
 * �M���h�q�ɂ̕ۑ�
 *------------------------------------------
 */
void storage_guild_storagesave(struct map_session_data *sd)
{
	struct guild_storage *stor;

	nullpo_retv(sd);

	if((stor=guild2storage(sd->status.guild_id)) != NULL) {
		intif_send_guild_storage(sd->status.account_id,stor);
	}

	return;
}

/*==========================================
 * �I��
 *------------------------------------------
 */
static int storage_db_final(void *key,void *data,va_list ap)
{
	aFree(data);
	return 0;
}
static int guild_storage_db_final(void *key,void *data,va_list ap)
{
	aFree(data);
	return 0;
}
void do_final_storage(void) // map.c::do_final()����Ă΂��
{
	if(storage_db)
		numdb_final(storage_db,storage_db_final);
	if(guild_storage_db)
		numdb_final(guild_storage_db,guild_storage_db_final);

	return;
}

/*==========================================
 * ������
 *------------------------------------------
 */
void do_init_storage(void) // map.c::do_init()����Ă΂��
{
	storage_db=numdb_init();
	guild_storage_db=numdb_init();

	return;
}
