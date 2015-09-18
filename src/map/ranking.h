#ifndef _RANKING_H_
#define _RANKING_H_

#include "map.h"

int ranking_get_pc_rank(struct map_session_data * sd,int ranking_id);
int ranking_get_id2rank(int char_id,int ranking_id);

int ranking_get_point(struct map_session_data * sd,int ranking_id);
int ranking_set_point(struct map_session_data * sd,int ranking_id,int point);
int ranking_gain_point(struct map_session_data * sd,int ranking_id,int point);

int ranking_readreg(struct map_session_data * sd);
int ranking_setglobalreg(struct map_session_data * sd,int ranking_id);
int ranking_setglobalreg_all(struct map_session_data * sd);

int ranking_update(struct map_session_data * sd,int ranking_id);

int ranking_clif_display(struct map_session_data * sd,int ranking_id);
int ranking_display(struct map_session_data * sd,int ranking_id,int begin,int end);
int ranking_display_point(struct map_session_data * sd,int ranking_id);

int ranking_set_data(int ranking_id,struct Ranking_Data *rd);

//�����L���O
enum {
	RK_BLACKSMITH = 0,	// �u���b�N�X�~�X
	RK_ALCHEMIST  = 1,	// �A���P�~�X�g
	RK_TAEKWON    = 2,	// �e�R�������J�[
	RK_PK         = 3,	// �s�E
	//RK_PVP      = 4,	// PVP�����L���O
};

int do_init_ranking(void);

#endif
