#ifndef _CLIF_H_
#define _CLIF_H_

#ifndef _WIN32
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#else
	#include <winsock.h>
 	#define in_addr_t unsigned long
#endif
#include "map.h"

#define MAX_PACKET_DB 0x300

struct packet_db {
	short len;
	void (*func)(int fd,struct map_session_data *sd, int cmd);
	short pos[20];
};

void clif_setip(char*);
void clif_setport(int);

in_addr_t clif_getip(void);
int clif_getport(void);
int clif_countusers(void);
void clif_setwaitclose(int fd);
int clif_parse(int fd);

void clif_authok(struct map_session_data *sd);
void clif_authfail_fd(int fd, unsigned int type);
void clif_charselectok(int id);
void clif_dropflooritem(struct flooritem_data *fitem);
void clif_clearflooritem(struct flooritem_data *fitem, int fd);
void clif_clearchar(struct block_list *bl, int type); // area or fd
int clif_clearchar_delay(unsigned int,struct block_list *,int);
#define clif_clearchar_area(bl,type) clif_clearchar(bl,type)
void clif_spawnpc(struct map_session_data *sd);	//area
void clif_spawnnpc(struct npc_data *nd);	// area
void clif_spawnmob(struct mob_data *md);	// area
void clif_spawnpet(struct pet_data *pd);	// area
void clif_movepet(struct pet_data *pd);	//area
void clif_walkok(struct map_session_data *sd);	// self
void clif_movechar(struct map_session_data *sd);	// area
void clif_movemob(struct mob_data *md);	//area
void clif_movehom(struct homun_data *hd);	//area
void clif_changemap(struct map_session_data *sd,char *mapname,int x,int y);	//self
void clif_changemapserver(struct map_session_data *sd, char *mapname, int x, int y, int ip, int port);	//self
int clif_fixpos(struct block_list *bl);	// area
int clif_fixpos2(struct block_list *bl, int x[4], int y[4]);	// custom
void clif_blown(struct block_list *bl, int x, int y);
void clif_fixwalkpos(struct block_list *bl);	// area
void clif_npcbuysell(struct map_session_data* sd, int id);	//self
void clif_buylist(struct map_session_data *sd, struct npc_data *nd);	//self
void clif_selllist(struct map_session_data *sd);	//self
void clif_scriptmes(struct map_session_data *sd, int npcid, char *mes);	//self
void clif_scriptnext(struct map_session_data *sd, int npcid);	//self
void clif_scriptclose(struct map_session_data *sd, int npcid);	//self
void clif_scriptmenu(struct map_session_data *sd, int npcid, char *mes);	//self
void clif_scriptinput(struct map_session_data *sd, int npcid);	//self
void clif_scriptinputstr(struct map_session_data *sd, int npcid);	// self
void clif_viewpoint(struct map_session_data *sd, int npc_id, int type, int x, int y, int id, int color);	//self
void clif_cutin(struct map_session_data *sd, char *image, int type);	//self
void clif_additem(struct map_session_data *sd, int n, int amount, unsigned char fail);	//self
void clif_delitem(struct map_session_data *sd, int n, int amount);	//self
void clif_updatestatus(struct map_session_data *sd, int type);	//self
void clif_changestatus(struct block_list *bl, int type, int val);	//area
void clif_damage(struct block_list *src, struct block_list *dst, unsigned int tick, int sdelay, int ddelay, int damage, int div, int type, int damage2);	// area
#define clif_takeitem(src,dst) clif_damage(src,dst,0,0,0,0,0,1,0)
void clif_changelook(struct block_list *bl, int type, int val);	// area
void clif_send_clothcolor(struct block_list *bl);
void clif_arrowequip(struct map_session_data *sd, int val); //self
void clif_arrow_fail(struct map_session_data *sd, unsigned short type); //self
void clif_arrow_create_list(struct map_session_data *sd);	//self
void clif_statusupack(struct map_session_data *sd, int type, int ok, int val);	// self
void clif_equipitemack(struct map_session_data *sd, int n, int pos, unsigned char ok);	// self
void clif_unequipitemack(struct map_session_data *sd, int n, int pos, unsigned char ok);	// self
void clif_misceffect(struct block_list* bl, int type);	// area
void clif_misceffect2(struct block_list *bl, int type);	// area
void clif_misceffect3(struct block_list *bl, int type);	// self
void clif_changeoption(struct block_list *bl);	// area
void clif_useitemack(struct map_session_data *sd, int idx, int amount, unsigned char ok);	// self
void clif_GlobalMessage(struct block_list *bl,char *message);
void clif_createchat(struct map_session_data *sd, unsigned char fail);	// self
void clif_dispchat(struct chat_data *cd, int fd);	// area or fd
void clif_joinchatfail(struct map_session_data *sd, unsigned char fail);	// self
void clif_joinchatok(struct map_session_data *sd, struct chat_data* cd);	// self
void clif_addchat(struct chat_data* cd, struct map_session_data *sd);	// chat
void clif_changechatowner(struct chat_data* cd, struct map_session_data *sd);	// chat
void clif_clearchat(struct chat_data *cd, int fd);	// area or fd
void clif_leavechat(struct chat_data* cd, struct map_session_data *sd, unsigned char flag);	// chat
void clif_changechatstatus(struct chat_data*);	// chat
void clif_changedir( struct block_list *bl, int headdir, int dir );
void clif_emotion(struct block_list *bl,int type);
void clif_talkiebox(struct block_list *bl,char* talkie);
void clif_wedding_effect(struct block_list *bl);
void clif_callpartner(struct map_session_data *sd);
void clif_divorced(struct map_session_data *sd, char *name);
void clif_baby_req_fail(struct map_session_data *sd, int type);
void clif_sitting(struct map_session_data *sd);
void clif_soundeffect(struct map_session_data *sd,struct block_list *bl,char *name,int type);

// trade
void clif_traderequest(struct map_session_data *sd, char *name);
void clif_tradestart(struct map_session_data *sd, unsigned char type);
void clif_tradeadditem(struct map_session_data *sd,struct map_session_data *tsd, int idx, int amount);
void clif_tradeitemok(struct map_session_data *sd, unsigned short idx, unsigned char fail);
void clif_tradedeal_lock(struct map_session_data *sd, unsigned char fail);
void clif_tradecancelled(struct map_session_data *sd);
void clif_tradecompleted(struct map_session_data *sd, unsigned char fail);

// storage
#include "storage.h"
void clif_storageitemlist(struct map_session_data *sd, struct storage *stor);
void clif_storageequiplist(struct map_session_data *sd, struct storage *stor);
void clif_updatestorageamount(struct map_session_data *sd, struct storage *stor);
void clif_storageitemadded(struct map_session_data *sd, struct storage *stor, int idx, int amount);
void clif_storageitemremoved(struct map_session_data *sd, int idx, int amount);
void clif_storageclose(struct map_session_data *sd);
void clif_guildstorageitemlist(struct map_session_data *sd, struct guild_storage *stor);
void clif_guildstorageequiplist(struct map_session_data *sd, struct guild_storage *stor);
void clif_updateguildstorageamount(struct map_session_data *sd, struct guild_storage *stor);
void clif_guildstorageitemadded(struct map_session_data *sd, struct guild_storage *stor, int idx, int amount);

int clif_pcinsight(struct block_list *bl, va_list ap);	// map_forallinmovearea callback
int clif_pcoutsight(struct block_list *bl, va_list ap);	// map_forallinmovearea callback
int clif_mobinsight(struct block_list *bl, va_list ap);	// map_forallinmovearea callback
int clif_moboutsight(struct block_list *bl, va_list ap);	// map_forallinmovearea callback
int clif_petinsight(struct block_list *bl, va_list ap);
int clif_petoutsight(struct block_list *bl, va_list ap);
int clif_hominsight(struct block_list *bl, va_list ap);
int clif_homoutsight(struct block_list *bl, va_list ap);

void clif_class_change(struct block_list *bl, int class, int type);

void clif_skillinfo(struct map_session_data *sd, int skillid, int type, int range);
void clif_skillinfoblock(struct map_session_data *sd);
void clif_skillup(struct map_session_data *sd, int skill_num);

void clif_skillcasting(struct block_list* bl,
	int src_id,int dst_id,int dst_x,int dst_y,int skill_num,int casttime);
void clif_skillcastcancel(struct block_list* bl);
void clif_skill_fail(struct map_session_data *sd, int skill_id, unsigned char type, unsigned short btype);
void clif_skill_damage(struct block_list *src,struct block_list *dst,
	unsigned int tick,int sdelay,int ddelay,int damage,int div,
	int skill_id,int skill_lv,int type);
/*int clif_skill_damage2(struct block_list *src,struct block_list *dst,
	unsigned int tick,int sdelay,int ddelay,int damage,int div,
	int skill_id,int skill_lv,int type);*/
void clif_skill_nodamage(struct block_list *src,struct block_list *dst,
	int skill_id,int heal,int fail);
void clif_skill_poseffect(struct block_list *src,int skill_id,
	int val,int x,int y,int tick);
void clif_skill_estimation(struct map_session_data *sd, struct block_list *dst);
void clif_skill_warppoint(struct map_session_data *sd,int skill_num,
	const char *map1,const char *map2,const char *map3,const char *map4);
void clif_skill_memo(struct map_session_data *sd, unsigned char flag);
void clif_skill_teleportmessage(struct map_session_data *sd, unsigned short flag);
void clif_skill_produce_mix_list(struct map_session_data *sd, int trigger);

void clif_produceeffect(struct map_session_data *sd, unsigned short flag, int nameid);

void clif_skill_setunit(struct skill_unit *unit);
void clif_skill_delunit(struct skill_unit *unit);

void clif_01ac(struct block_list *bl);

void clif_autospell(struct map_session_data *sd, int skilllv);
void clif_devotion(struct map_session_data *sd, int target);

void clif_spiritball(struct map_session_data *sd);
void clif_coin(struct map_session_data *sd);
void clif_combo_delay(struct block_list *bl, int wait);
void clif_bladestop(struct block_list *src, int dst_id, int flag);
void clif_gospel_message(struct map_session_data *sd, int type);
void clif_changemapcell(int m, int x, int y, int cell_type, int type);

void clif_feel_info(struct map_session_data *sd, int skilllv);
void clif_hate_mob(struct map_session_data *sd, int skilllv, int id);
void clif_hate_info(struct map_session_data *sd, int skilllv, int id);
void clif_mission_mob(struct map_session_data *sd, int id, int count);
void clif_angel_message(struct map_session_data *sd);
void clif_feel_display(struct map_session_data *sd, int skilllv);

void clif_status_change(struct block_list *bl, int type, unsigned char flag);

void clif_wis_message(int fd,char *nick,char *mes, int mes_len);
void clif_wis_end(int fd, unsigned short flag);

void clif_solved_charname(struct map_session_data *sd, int char_id);

void clif_insert_card(struct map_session_data *sd, int idx_equip, int idx_card, unsigned char flag);

void clif_itemlist(struct map_session_data *sd);
void clif_equiplist(struct map_session_data *sd);

void clif_cart_additem(struct map_session_data *sd, int n, int amount);
void clif_cart_delitem(struct map_session_data *sd, int n, int amount);
void clif_cart_itemlist(struct map_session_data *sd);
void clif_cart_equiplist(struct map_session_data *sd);

void clif_item_identify_list(struct map_session_data *sd);
void clif_weapon_refine_list(struct map_session_data *sd);
void clif_weapon_refine_res(struct map_session_data *sd, int flag, int nameid);
void clif_item_identified(struct map_session_data *sd, int idx, unsigned char flag);
void clif_item_repair_list(struct map_session_data *sd, struct map_session_data *dstsd);
void clif_item_repaireffect(struct map_session_data *sd, unsigned char flag, int nameid);

void clif_item_skill(struct map_session_data *sd, int skillid, int skilllv, const char *name);

void clif_mvp_effect(struct block_list *bl);
void clif_mvp_item(struct map_session_data *sd, int nameid);
void clif_mvp_fail_item(struct map_session_data *sd);
void clif_mvp_exp(struct map_session_data *sd, int exp);

// vending
void clif_openvendingreq(struct map_session_data *sd, int num);
void clif_showvendingboard(struct block_list* bl, const char *shop_title, int fd);
void clif_closevendingboard(struct block_list* bl, int fd);
void clif_vendinglist(struct map_session_data *sd, struct map_session_data *vsd);
void clif_buyvending(struct map_session_data *sd, int idx, int amount, unsigned char fail);
int clif_openvending(struct map_session_data *sd);
void clif_vendingreport(struct map_session_data *sd, int idx, int amount);

void clif_movetoattack(struct map_session_data *sd, struct block_list *bl);

// party
void clif_party_created(struct map_session_data *sd, unsigned char flag);
void clif_party_info(struct party *p, int fd);
void clif_party_invite(struct map_session_data *sd, struct map_session_data *tsd);
void clif_party_inviteack(struct map_session_data *sd, char *nick, unsigned char flag);
void clif_party_option(struct party *p, struct map_session_data *sd, int flag);
void clif_party_leaved(struct party *p, struct map_session_data *sd, int account_id, char *name, int flag);
void clif_party_message(struct party *p, int account_id, char *mes, int len);
void clif_party_xy(struct map_session_data *sd);
void clif_party_hp(struct map_session_data *sd);
//void clif_party_move(struct party *p, struct map_session_data *sd, unsigned char online);

// guild
void clif_guild_created(struct map_session_data *sd, unsigned char flag);
void clif_guild_belonginfo(struct map_session_data *sd, struct guild *g);
void clif_guild_memberlogin_notice(struct guild *g, int idx, unsigned char flag);
void clif_guild_basicinfo(struct map_session_data *sd, struct guild *g);
void clif_guild_allianceinfo(struct map_session_data *sd, struct guild *g);
void clif_guild_memberlist(struct map_session_data *sd, struct guild *g);
void clif_guild_positionchanged(struct guild *g, int idx);
void clif_guild_memberpositionchanged(struct guild *g, int idx);
void clif_guild_emblem(struct map_session_data *sd, struct guild *g);
void clif_guild_skillinfo(struct map_session_data *sd, struct guild *g);
void clif_guild_notice(struct map_session_data *sd, struct guild *g);
void clif_guild_invite(struct map_session_data *sd, struct guild *g);
void clif_guild_inviteack(struct map_session_data *sd, unsigned char flag);
void clif_guild_leave(struct map_session_data *sd, const char *name, const char *mes);
void clif_guild_explusion(struct map_session_data *sd, const char *name, const char *mes, int account_id);
void clif_guild_message(struct guild *g, int account_id, const char *mes, int len);
void clif_guild_skillup(struct map_session_data *sd, int skill_num, int lv);
void clif_guild_reqalliance(struct map_session_data *sd, int account_id, const char *name);
void clif_guild_allianceack(struct map_session_data *sd, unsigned int flag);
void clif_guild_delalliance(struct map_session_data *sd, int guild_id, int flag);
void clif_guild_oppositionack(struct map_session_data *sd, unsigned char flag);
void clif_guild_broken(struct map_session_data *sd, unsigned int flag);
void clif_guild_xy(struct map_session_data *sd);


// atcommand
void clif_displaymessage(const int fd, char* mes);
void clif_disp_onlyself(struct map_session_data *sd, char *mes, int len);
void clif_GMmessage(struct block_list *bl, char* mes, int len, int flag);
void clif_announce(struct block_list *bl, char* mes, int len, unsigned long color, int flag);
void clif_heal(int fd, int type, int val);
void clif_resurrection(struct block_list *bl, unsigned short type);
void clif_set0199(int fd, unsigned short type);
void clif_pvpset(struct map_session_data *sd, int pvprank, int pvpnum, char type);
void clif_send0199(int map, unsigned short type);
void clif_refine(int fd, struct map_session_data *sd, unsigned short fail, int idx, int val);
void clif_send_packet(struct map_session_data *sd, const char *message);

//petsystem
void clif_catch_process(struct map_session_data *sd);
void clif_pet_rulet(struct map_session_data *sd, unsigned char data);
void clif_sendegg(struct map_session_data *sd);
void clif_send_petdata(struct map_session_data *sd, unsigned char type, int param);
void clif_send_petstatus(struct map_session_data *sd);
void clif_pet_performance(struct block_list *bl, int param);
void clif_pet_equip(struct pet_data *pd, int nameid);
void clif_pet_food(struct map_session_data *sd, int foodid, unsigned char fail);

//friend
void clif_friend_send_info( struct map_session_data *sd );
void clif_friend_send_online(const int fd, int account_id, int char_id, int flag );
void clif_friend_add_request(const int fd, struct map_session_data *from_sd );
void clif_friend_add_ack(const int fd, int account_id, int char_id, char* name, unsigned short flag);
void clif_friend_del_ack(const int fd, int account_id, int char_id );

//ranking
void clif_blacksmith_point(const int fd,const int total,const int point);
void clif_alchemist_point(const int fd,const int total,const int point);
void clif_taekwon_point(const int fd,const int total,const int point);
void clif_pk_point(const int fd,const int total,const int point);
void clif_blacksmith_ranking(const int fd,char *charname[10],const int point[10]);
void clif_alchemist_ranking(const int fd,char *charname[10],const int point[10]);
void clif_taekwon_ranking(const int fd,char *charname[10],const int point[10]);
void clif_pk_ranking(const int fd,char *charname[10],const int point[10]);

// mail
void clif_openmailbox(const int fd);
void clif_send_mailbox(struct map_session_data *sd,int num,struct mail_data *md[MAIL_STORE_MAX]);
void clif_receive_mail(struct map_session_data *sd,struct mail_data *md);
void clif_res_sendmail(const int fd,int flag);
void clif_arrive_newmail(const int fd,struct mail_data *md);
void clif_deletemail_res(const int fd,int mail_num,int flag);

//homun
void clif_send_homdata(struct map_session_data *sd, int type, int param);
void clif_spawnhom(struct homun_data *hd);
void clif_send_homstatus(struct map_session_data *sd,int flag);
void clif_hom_food(struct map_session_data *sd, int foodid, unsigned char fail);
void clif_homskillinfoblock(struct map_session_data *sd);
void clif_homskillup(struct map_session_data *sd, int skill_num);

void clif_GM_kickack(struct map_session_data *sd, int id);
void clif_GM_kick(struct map_session_data *sd, struct map_session_data *tsd, int type);

int clif_foreachclient(int (*)(struct map_session_data*,va_list),...);

void do_final_clif(void);
void do_init_clif(void);

// httpd chat system

struct httpd_session_data;
void clif_webchat_message(const char* head,const char *mes1,const char *mes2);
void clif_webchat(struct httpd_session_data* sd,const char* url);

#endif


