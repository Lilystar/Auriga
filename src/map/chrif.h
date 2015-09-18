#ifndef _CHRIF_H_
#define _CHRIF_H_

void chrif_setuserid(char*);
void chrif_setpasswd(char*);
void chrif_setip(char*);
void chrif_setport(unsigned short);
int chrif_parse(int fd);

int chrif_isconnect(void);

int chrif_authreq(struct map_session_data *);
int chrif_save(struct map_session_data*);
int chrif_charselectreq(struct map_session_data *);

int chrif_changemapserver(struct map_session_data *sd,char *name,int x,int y,unsigned long ip,unsigned short port);

void chrif_searchcharid(int char_id);
int chrif_changegm(int id,const char *pass,int len);
int chrif_changesex(int id,int sex);
int chrif_saveaccountreg2(struct map_session_data *sd);

int chrif_mapactive(int active);
int chrif_maintenance(int maintenance);
int chrif_chardisconnect(struct map_session_data *sd);

int chrif_reqdivorce(int char_id);

int chrif_friend_delete( struct map_session_data* sd, int account_id, int char_id );
int chrif_friend_online( struct map_session_data *sd, int flag );

int chrif_req_break_adoption(int char_id, const char *name);

int chrif_ranking_request(int fd);
int chrif_ranking_update(struct Ranking_Data *rd,int ranking_id,int rank);

int chrif_disconnect_sub(struct map_session_data* sd,va_list va);
int chrif_flush_fifo(void);
int do_final_chrif(void);
int do_init_chrif(void);

#endif
