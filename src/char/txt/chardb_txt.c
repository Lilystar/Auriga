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
#include <stdlib.h>
#include <string.h>

#include "db.h"
#include "mmo.h"
#include "socket.h"
#include "lock.h"
#include "nullpo.h"
#include "malloc.h"
#include "journal.h"
#include "md5calc.h"
#include "utils.h"

#include "../char.h"
#include "chardb_txt.h"

#ifdef TXT_JOURNAL
static int char_journal_enable = 1;
static struct journal char_journal;
static char char_journal_file[1024] = "./save/auriga.journal";
static int char_journal_cache = 1000;
#endif

static struct mmo_chardata *char_dat = NULL;
static int  char_num,char_max;
static char char_txt[1024] = "save/auriga.txt";
static int  char_id_count = 150000;

// �L����ID����char_dat�̃C���f�b�N�X��Ԃ�
static int char_id2idx(int char_id)
{
	int min = -1;
	int max = char_num;

	// binary search
	while(max - min > 1) {
		int mid = (min + max) / 2;
		if(char_dat[mid].st.char_id == char_id)
			return (char_dat[mid].st.account_id > 0)? mid: -1;

		if(char_dat[mid].st.char_id > char_id)
			max = mid;
		else
			min = mid;
	}
	return -1;
}

static int mmo_char_tostr(char *str,struct mmo_chardata *p)
{
	int i;
	char *str_p = str;
	unsigned short sk_lv;

	nullpo_retr(-1, p);

	str_p += sprintf(str_p,"%d\t%d,%d\t%s\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%u,%d,%d,%d\t%d,%d,%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d,%d"
		"\t%s,%d,%d\t%s,%d,%d,%d,%d,%d,%d,%u,%d,%d,%d\t",
		p->st.char_id,p->st.account_id,p->st.char_num,p->st.name,
		p->st.class_,p->st.base_level,p->st.job_level,
		p->st.base_exp,p->st.job_exp,p->st.zeny,
		p->st.hp,p->st.max_hp,p->st.sp,p->st.max_sp,
		p->st.str,p->st.agi,p->st.vit,p->st.int_,p->st.dex,p->st.luk,
		p->st.status_point,p->st.skill_point,
		p->st.option,p->st.karma,p->st.manner,p->st.die_counter,
		p->st.party_id,p->st.guild_id,p->st.pet_id,p->st.homun_id,p->st.merc_id,
		p->st.hair,p->st.hair_color,p->st.clothes_color,
		p->st.weapon,p->st.shield,p->st.robe,p->st.head_top,p->st.head_mid,p->st.head_bottom,
		p->st.last_point.map,p->st.last_point.x,p->st.last_point.y,
		p->st.save_point.map,p->st.save_point.x,p->st.save_point.y,
		p->st.partner_id,p->st.parent_id[0],p->st.parent_id[1],p->st.baby_id,
		p->st.delete_date,p->st.refuse_partyinvite,p->st.show_equip,p->st.font
	);
	for(i = 0; i < MAX_PORTAL_MEMO; i++) {
		if(p->st.memo_point[i].map[0])
			str_p += sprintf(str_p,"%d,%s,%d,%d ",i,p->st.memo_point[i].map,p->st.memo_point[i].x,p->st.memo_point[i].y);
	}
	*(str_p++)='\t';

	for(i=0;i<MAX_INVENTORY;i++) {
		if(p->st.inventory[i].nameid) {
			str_p += sprintf(str_p,"%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u ",
			p->st.inventory[i].id,p->st.inventory[i].nameid,p->st.inventory[i].amount,p->st.inventory[i].equip,
			p->st.inventory[i].identify,p->st.inventory[i].refine,p->st.inventory[i].attribute,
			p->st.inventory[i].card[0],p->st.inventory[i].card[1],p->st.inventory[i].card[2],p->st.inventory[i].card[3],
			p->st.inventory[i].limit);
		}
	}
	*(str_p++)='\t';

	for(i=0;i<MAX_CART;i++) {
		if(p->st.cart[i].nameid) {
			str_p += sprintf(str_p,"%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u ",
			p->st.cart[i].id,p->st.cart[i].nameid,p->st.cart[i].amount,p->st.cart[i].equip,
			p->st.cart[i].identify,p->st.cart[i].refine,p->st.cart[i].attribute,
			p->st.cart[i].card[0],p->st.cart[i].card[1],p->st.cart[i].card[2],p->st.cart[i].card[3],
			p->st.cart[i].limit);
		}
	}
	*(str_p++)='\t';

	for(i=0;i<MAX_PCSKILL;i++) {
		if(p->st.skill[i].id && p->st.skill[i].flag!=1){
			sk_lv = (p->st.skill[i].flag==0)? p->st.skill[i].lv: p->st.skill[i].flag-2;
			str_p += sprintf(str_p,"%d,%d ",p->st.skill[i].id,sk_lv);
		}
	}
	*(str_p++)='\t';

	for(i=0;i<p->reg.global_num;i++) {
		if(p->reg.global[i].str[0] && p->reg.global[i].value != 0)
			str_p += sprintf(str_p,"%s,%d ",p->reg.global[i].str,p->reg.global[i].value);
	}
	*(str_p++)='\t';

	for(i=0;i<p->st.friend_num;i++) {
		str_p += sprintf(str_p,"%d,%d ",p->st.friend_data[i].account_id, p->st.friend_data[i].char_id );
	}
	*(str_p++)='\t';

	for(i = 0; i < 3; i++) {
		if(p->st.feel_map[i][0])
			str_p += sprintf(str_p,"%s,%d ",p->st.feel_map[i],i);
	}
	*(str_p++)='\t';

	for(i = 0; i < MAX_HOTKEYS; i++) {
		if(p->st.hotkey[i].id > 0)
			str_p += sprintf(str_p,"%d,%d,%d,%d ",i,p->st.hotkey[i].type,p->st.hotkey[i].id,p->st.hotkey[i].lv);
	}
	*(str_p++)='\t';

	for(i = 0; i < MAX_MERC_TYPE; i++) {
		if(p->st.merc_fame[i] > 0 || p->st.merc_call[i] > 0)
			str_p += sprintf(str_p,"%d,%d,%d ",i,p->st.merc_fame[i],p->st.merc_call[i]);
	}
	*(str_p++)='\t';

	*str_p='\0';

	return 0;
}

static int mmo_char_fromstr(char *str,struct mmo_chardata *p)
{
	char tmp_str[3][256];
	int tmp_int[55];
	int set,next,len,i,n;

	nullpo_retr(0, p);

	// Auriga-0904�ȍ~�̌`��
	set=sscanf(str,"%d\t%d,%d\t%255[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
		"\t%u,%d,%d,%d\t%d,%d,%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d,%d"
		"\t%255[^,],%d,%d\t%255[^,],%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
		&tmp_int[0],&tmp_int[1],&tmp_int[2],tmp_str[0],
		&tmp_int[3],&tmp_int[4],&tmp_int[5],
		&tmp_int[6],&tmp_int[7],&tmp_int[8],
		&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
		&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
		&tmp_int[19],&tmp_int[20],
		&tmp_int[21],&tmp_int[22],&tmp_int[23],&tmp_int[24],
		&tmp_int[25],&tmp_int[26],&tmp_int[27],&tmp_int[28],&tmp_int[29],
		&tmp_int[30],&tmp_int[31],&tmp_int[32],
		&tmp_int[33],&tmp_int[34],&tmp_int[35],&tmp_int[36],&tmp_int[37],&tmp_int[38],
		tmp_str[1],&tmp_int[39],&tmp_int[40],
		tmp_str[2],&tmp_int[41],&tmp_int[42],&tmp_int[43],&tmp_int[44],&tmp_int[45],&tmp_int[46],&tmp_int[47],
		&tmp_int[48],&tmp_int[49],&tmp_int[50],&next
	);

	if(set != 54)
	{
		// Auriga-0904�ȍ~�̌`��
		tmp_int[50] = 0;	// font
		set=sscanf(str,"%d\t%d,%d\t%255[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
			"\t%u,%d,%d,%d\t%d,%d,%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d,%d"
			"\t%255[^,],%d,%d\t%255[^,],%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
			&tmp_int[0],&tmp_int[1],&tmp_int[2],tmp_str[0],
			&tmp_int[3],&tmp_int[4],&tmp_int[5],
			&tmp_int[6],&tmp_int[7],&tmp_int[8],
			&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
			&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
			&tmp_int[19],&tmp_int[20],
			&tmp_int[21],&tmp_int[22],&tmp_int[23],&tmp_int[24],
			&tmp_int[25],&tmp_int[26],&tmp_int[27],&tmp_int[28],&tmp_int[29],
			&tmp_int[30],&tmp_int[31],&tmp_int[32],
			&tmp_int[33],&tmp_int[34],&tmp_int[35],&tmp_int[36],&tmp_int[37],&tmp_int[38],
			tmp_str[1],&tmp_int[39],&tmp_int[40],
			tmp_str[2],&tmp_int[41],&tmp_int[42],&tmp_int[43],&tmp_int[44],&tmp_int[45],&tmp_int[46],&tmp_int[47],
			&tmp_int[48],&tmp_int[49],&next
		);
		if(set != 53)
		{
			// Auriga-0902�ȍ~�̌`��
			set=sscanf(str,"%d\t%d,%d\t%255[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
				"\t%u,%d,%d,%d\t%d,%d,%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d"
				"\t%255[^,],%d,%d\t%255[^,],%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
				&tmp_int[0],&tmp_int[1],&tmp_int[2],tmp_str[0],
				&tmp_int[3],&tmp_int[4],&tmp_int[5],
				&tmp_int[6],&tmp_int[7],&tmp_int[8],
				&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
				&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
				&tmp_int[19],&tmp_int[20],
				&tmp_int[21],&tmp_int[22],&tmp_int[23],&tmp_int[24],
				&tmp_int[25],&tmp_int[26],&tmp_int[27],&tmp_int[28],&tmp_int[29],
				&tmp_int[30],&tmp_int[31],&tmp_int[32],
				&tmp_int[33],&tmp_int[34],&tmp_int[35],&tmp_int[36],&tmp_int[37],&tmp_int[38],
				&tmp_int[39],&tmp_int[40],&tmp_int[41],&tmp_int[42],&tmp_int[43],
				tmp_str[1],&tmp_int[44],&tmp_int[45],
				tmp_str[2],&tmp_int[46],&tmp_int[47],&tmp_int[48],&tmp_int[49],&tmp_int[50],&tmp_int[51],&tmp_int[52],
				&tmp_int[53],&tmp_int[54],&next
			);
			tmp_int[39] = tmp_int[44];
			tmp_int[40] = tmp_int[45];
			tmp_int[41] = tmp_int[46];
			tmp_int[42] = tmp_int[47];
			tmp_int[43] = tmp_int[48];
			tmp_int[44] = tmp_int[49];
			tmp_int[45] = tmp_int[50];
			tmp_int[46] = tmp_int[51];
			tmp_int[47] = tmp_int[52];
			tmp_int[48] = tmp_int[53];
			tmp_int[49] = tmp_int[54];
			if(set != 58)
			{
				// Auriga-0888�ȍ~�̌`��
				tmp_int[42] = 0;	// costume_robe
				tmp_int[43] = 0;	// costume_floor
				set=sscanf(str,"%d\t%d,%d\t%255[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
					"\t%u,%d,%d,%d\t%d,%d,%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d,%d,%d,%d,%d"
					"\t%255[^,],%d,%d\t%255[^,],%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
					&tmp_int[0],&tmp_int[1],&tmp_int[2],tmp_str[0],
					&tmp_int[3],&tmp_int[4],&tmp_int[5],
					&tmp_int[6],&tmp_int[7],&tmp_int[8],
					&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
					&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
					&tmp_int[19],&tmp_int[20],
					&tmp_int[21],&tmp_int[22],&tmp_int[23],&tmp_int[24],
					&tmp_int[25],&tmp_int[26],&tmp_int[27],&tmp_int[28],&tmp_int[29],
					&tmp_int[30],&tmp_int[31],&tmp_int[32],
					&tmp_int[33],&tmp_int[34],&tmp_int[35],&tmp_int[36],&tmp_int[37],&tmp_int[38],
					&tmp_int[39],&tmp_int[40],&tmp_int[41],
					tmp_str[1],&tmp_int[44],&tmp_int[45],
					tmp_str[2],&tmp_int[46],&tmp_int[47],&tmp_int[48],&tmp_int[49],&tmp_int[50],&tmp_int[51],&tmp_int[52],
					&tmp_int[53],&tmp_int[54],&next
				);
				if(set != 56)
				{
					// Auriga-0837�ȍ~�̌`��
					tmp_int[35] = 0;	// robe
					tmp_int[51] = 0;	// refuse_partyinvite
					tmp_int[52] = 0;	// show_equip
					set=sscanf(str,"%d\t%d,%d\t%255[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
						"\t%u,%d,%d,%d\t%d,%d,%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d,%d,%d,%d"
						"\t%255[^,],%d,%d\t%255[^,],%d,%d,%d,%d,%d,%d,%d%n",
						&tmp_int[0],&tmp_int[1],&tmp_int[2],tmp_str[0],
						&tmp_int[3],&tmp_int[4],&tmp_int[5],
						&tmp_int[6],&tmp_int[7],&tmp_int[8],
						&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
						&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
						&tmp_int[19],&tmp_int[20],
						&tmp_int[21],&tmp_int[22],&tmp_int[23],&tmp_int[24],
						&tmp_int[25],&tmp_int[26],&tmp_int[27],&tmp_int[28],&tmp_int[29],
						&tmp_int[30],&tmp_int[31],&tmp_int[32],
						&tmp_int[33],&tmp_int[34],&tmp_int[36],&tmp_int[37],&tmp_int[38],
						&tmp_int[39],&tmp_int[40],&tmp_int[41],
						tmp_str[1],&tmp_int[42],&tmp_int[43],
						tmp_str[2],&tmp_int[44],&tmp_int[45],&tmp_int[46],&tmp_int[47],&tmp_int[48],&tmp_int[49],&tmp_int[50],&next
					);
					if(set != 53)
					{
						// Auriga-0309�`0596�����0600�ȍ~�̌`��
						tmp_int[38] = 0;	// costume_head_top
						tmp_int[39] = 0;	// costume_head_mid
						tmp_int[40] = 0;	// costume_head_bottom
						tmp_int[49] = 0;	// delete_date
						set=sscanf(str,"%d\t%d,%d\t%255[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
							"\t%u,%d,%d,%d\t%d,%d,%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
							"\t%255[^,],%d,%d\t%255[^,],%d,%d,%d,%d,%d,%d%n",
							&tmp_int[0],&tmp_int[1],&tmp_int[2],tmp_str[0],
							&tmp_int[3],&tmp_int[4],&tmp_int[5],
							&tmp_int[6],&tmp_int[7],&tmp_int[8],
							&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
							&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
							&tmp_int[19],&tmp_int[20],
							&tmp_int[21],&tmp_int[22],&tmp_int[23],&tmp_int[24],
							&tmp_int[25],&tmp_int[26],&tmp_int[27],&tmp_int[28],&tmp_int[29],
							&tmp_int[30],&tmp_int[31],&tmp_int[32],
							&tmp_int[33],&tmp_int[34],&tmp_int[35],&tmp_int[36],&tmp_int[37],
							tmp_str[1],&tmp_int[41],&tmp_int[42],
							tmp_str[2],&tmp_int[43],&tmp_int[44],&tmp_int[45],&tmp_int[46],&tmp_int[47],&tmp_int[48],&next
						);

						if(set != 49) {
							// Auriga-0597�`0599�̌`��
							set=sscanf(str,"%d\t%d,%d\t%255[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
								"\t%u,%d,%d,%d\t%d,%d,%d,%d,%d\t%*d,%*d,%*d,%*d,%*d,%*d\t%d,%d,%d\t%d,%d,%d,%d,%d"
								"\t%255[^,],%d,%d\t%255[^,],%d,%d,%d,%d,%d,%d%n",
								&tmp_int[0],&tmp_int[1],&tmp_int[2],tmp_str[0],
								&tmp_int[3],&tmp_int[4],&tmp_int[5],
								&tmp_int[6],&tmp_int[7],&tmp_int[8],
								&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
								&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
								&tmp_int[19],&tmp_int[20],
								&tmp_int[21],&tmp_int[22],&tmp_int[23],&tmp_int[24],
								&tmp_int[25],&tmp_int[26],&tmp_int[27],&tmp_int[28],&tmp_int[29],
								&tmp_int[30],&tmp_int[31],&tmp_int[32],
								&tmp_int[33],&tmp_int[34],&tmp_int[35],&tmp_int[36],&tmp_int[37],
								tmp_str[1],&tmp_int[41],&tmp_int[42],
								tmp_str[2],&tmp_int[43],&tmp_int[44],&tmp_int[45],&tmp_int[46],&tmp_int[47],&tmp_int[48],&next
							);

							if(set != 49) {
								// Auriga-089�ȍ~�̌`��
								tmp_int[29] = 0;	// merc_id
								set=sscanf(str,"%d\t%d,%d\t%255[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
									"\t%u,%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
									"\t%255[^,],%d,%d\t%255[^,],%d,%d,%d,%d,%d,%d%n",
									&tmp_int[0],&tmp_int[1],&tmp_int[2],tmp_str[0],
									&tmp_int[3],&tmp_int[4],&tmp_int[5],
									&tmp_int[6],&tmp_int[7],&tmp_int[8],
									&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
									&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
									&tmp_int[19],&tmp_int[20],
									&tmp_int[21],&tmp_int[22],&tmp_int[23],&tmp_int[24],
									&tmp_int[25],&tmp_int[26],&tmp_int[27],&tmp_int[28],
									&tmp_int[30],&tmp_int[31],&tmp_int[32],
									&tmp_int[33],&tmp_int[34],&tmp_int[35],&tmp_int[36],&tmp_int[37],
									tmp_str[1],&tmp_int[41],&tmp_int[42],
									tmp_str[2],&tmp_int[43],&tmp_int[44],&tmp_int[45],&tmp_int[46],&tmp_int[47],&tmp_int[48],&next
								);

								if(set != 48) {
									tmp_int[24] = 0;	// die_counter
									set=sscanf(str,"%d\t%d,%d\t%255[^\t]\t%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d\t%d,%d,%d,%d,%d,%d\t%d,%d"
										"\t%u,%d,%d\t%d,%d,%d,%d\t%d,%d,%d\t%d,%d,%d,%d,%d"
										"\t%255[^,],%d,%d\t%255[^,],%d,%d,%d,%d,%d,%d%n",
										&tmp_int[0],&tmp_int[1],&tmp_int[2],tmp_str[0],
										&tmp_int[3],&tmp_int[4],&tmp_int[5],
										&tmp_int[6],&tmp_int[7],&tmp_int[8],
										&tmp_int[9],&tmp_int[10],&tmp_int[11],&tmp_int[12],
										&tmp_int[13],&tmp_int[14],&tmp_int[15],&tmp_int[16],&tmp_int[17],&tmp_int[18],
										&tmp_int[19],&tmp_int[20],
										&tmp_int[21],&tmp_int[22],&tmp_int[23],
										&tmp_int[25],&tmp_int[26],&tmp_int[27],&tmp_int[28],
										&tmp_int[30],&tmp_int[31],&tmp_int[32],
										&tmp_int[33],&tmp_int[34],&tmp_int[35],&tmp_int[36],&tmp_int[37],
										tmp_str[1],&tmp_int[41],&tmp_int[42],
										tmp_str[2],&tmp_int[43],&tmp_int[44],&tmp_int[45],&tmp_int[46],&tmp_int[47],&tmp_int[48],&next
									);

								if(set != 47)
									return 0;	// Athena1881�ȑO�̌Â��`���̓T�|�[�g���Ȃ�
								}
							}
						}
					}
				}
			}
		}
	}

	strncpy(p->st.name, tmp_str[0], 24);
	strncpy(p->st.last_point.map, tmp_str[1], 24);
	strncpy(p->st.save_point.map, tmp_str[2], 24);

	// force \0 terminal
	p->st.name[23]           = '\0';
	p->st.last_point.map[23] = '\0';
	p->st.save_point.map[23] = '\0';

	p->st.char_id             = tmp_int[0];
	p->st.account_id          = tmp_int[1];
	p->st.char_num            = tmp_int[2];
	p->st.class_              = tmp_int[3];
	p->st.base_level          = tmp_int[4];
	p->st.job_level           = tmp_int[5];
	p->st.base_exp            = tmp_int[6];
	p->st.job_exp             = tmp_int[7];
	p->st.zeny                = tmp_int[8];
	p->st.hp                  = tmp_int[9];
	p->st.max_hp              = tmp_int[10];
	p->st.sp                  = tmp_int[11];
	p->st.max_sp              = tmp_int[12];
	p->st.str                 = tmp_int[13];
	p->st.agi                 = tmp_int[14];
	p->st.vit                 = tmp_int[15];
	p->st.int_                = tmp_int[16];
	p->st.dex                 = tmp_int[17];
	p->st.luk                 = tmp_int[18];
	p->st.status_point        = tmp_int[19];
	p->st.skill_point         = tmp_int[20];
	p->st.option              = (unsigned int)tmp_int[21];
	p->st.karma               = tmp_int[22];
	p->st.manner              = tmp_int[23];
	p->st.die_counter         = tmp_int[24];
	p->st.party_id            = tmp_int[25];
	p->st.guild_id            = tmp_int[26];
	p->st.pet_id              = tmp_int[27];
	p->st.homun_id            = tmp_int[28];
	p->st.merc_id             = tmp_int[29];
	p->st.hair                = tmp_int[30];
	p->st.hair_color          = tmp_int[31];
	p->st.clothes_color       = tmp_int[32];
	p->st.weapon              = tmp_int[33];
	p->st.shield              = tmp_int[34];
	p->st.robe                = tmp_int[35];
	p->st.head_top            = tmp_int[36];
	p->st.head_mid            = tmp_int[37];
	p->st.head_bottom         = tmp_int[38];
	p->st.last_point.x        = tmp_int[39];
	p->st.last_point.y        = tmp_int[40];
	p->st.save_point.x        = tmp_int[41];
	p->st.save_point.y        = tmp_int[42];
	p->st.partner_id          = tmp_int[43];
	p->st.parent_id[0]        = tmp_int[44];
	p->st.parent_id[1]        = tmp_int[45];
	p->st.baby_id             = tmp_int[46];
	p->st.delete_date         = (unsigned int)tmp_int[47];
	p->st.refuse_partyinvite  = tmp_int[48];
	p->st.show_equip          = tmp_int[49];
	p->st.font                = tmp_int[50];

	if(str[next]=='\n' || str[next]=='\r')
		return 1;	// �V�K�f�[�^
	next++;

	for(i = 0; str[next] && str[next] != '\t'; i++) {
		// Auriga-0587�ȍ~�̌`��
		set=sscanf(str+next,"%d,%255[^,],%d,%d%n",&tmp_int[0],tmp_str[0],&tmp_int[1],&tmp_int[2],&len);
		if(set!=4) {
			n = i;
			set=sscanf(str+next,"%255[^,],%d,%d%n",tmp_str[0],&tmp_int[1],&tmp_int[2],&len);
			if(set!=3)
				return 0;
		} else {
			n = tmp_int[0];
		}
		if(n >= 0 && n < MAX_PORTAL_MEMO) {
			strncpy(p->st.memo_point[n].map, tmp_str[0], 24);
			p->st.memo_point[n].map[23] = '\0';	// force \0 terminal
			p->st.memo_point[n].x       = tmp_int[1];
			p->st.memo_point[n].y       = tmp_int[2];
		}
		next+=len;
		if(str[next]==' ')
			next++;
	}
	next++;
	for(i=0;str[next] && str[next]!='\t';i++){
		// Auriga-0300�ȍ~�̌`��
		set=sscanf(str+next,"%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u%n",
			&tmp_int[0],&tmp_int[1],&tmp_int[2],&tmp_int[3],
			&tmp_int[4],&tmp_int[5],&tmp_int[6],
			&tmp_int[7],&tmp_int[8],&tmp_int[9],&tmp_int[10],&tmp_int[11],&len);
		if(set!=12) {
			tmp_int[11] = 0;	// limit
			set=sscanf(str+next,"%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
				&tmp_int[0],&tmp_int[1],&tmp_int[2],&tmp_int[3],
				&tmp_int[4],&tmp_int[5],&tmp_int[6],
				&tmp_int[7],&tmp_int[8],&tmp_int[9],&tmp_int[10],&len);
			if(set!=11)
				return 0;
		}
		if(i < MAX_INVENTORY) {
			p->st.inventory[i].id        = (unsigned int)tmp_int[0];
			p->st.inventory[i].nameid    = tmp_int[1];
			p->st.inventory[i].amount    = tmp_int[2];
			p->st.inventory[i].equip     = tmp_int[3];
			p->st.inventory[i].identify  = tmp_int[4];
			p->st.inventory[i].refine    = tmp_int[5];
			p->st.inventory[i].attribute = tmp_int[6];
			p->st.inventory[i].card[0]   = tmp_int[7];
			p->st.inventory[i].card[1]   = tmp_int[8];
			p->st.inventory[i].card[2]   = tmp_int[9];
			p->st.inventory[i].card[3]   = tmp_int[10];
			p->st.inventory[i].limit     = (unsigned int)tmp_int[11];
		}
		next+=len;
		if(str[next]==' ')
			next++;
	}
	next++;
	for(i=0;str[next] && str[next]!='\t';i++){
		// Auriga-0300�ȍ~�̌`��
		set=sscanf(str+next,"%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u%n",
			&tmp_int[0],&tmp_int[1],&tmp_int[2],&tmp_int[3],
			&tmp_int[4],&tmp_int[5],&tmp_int[6],
			&tmp_int[7],&tmp_int[8],&tmp_int[9],&tmp_int[10],&tmp_int[11],&len);
		if(set!=12) {
			tmp_int[11] = 0;	// limit
			set=sscanf(str+next,"%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%n",
				&tmp_int[0],&tmp_int[1],&tmp_int[2],&tmp_int[3],
				&tmp_int[4],&tmp_int[5],&tmp_int[6],
				&tmp_int[7],&tmp_int[8],&tmp_int[9],&tmp_int[10],&len);
			if(set!=11)
				return 0;
		}
		if(i < MAX_CART) {
			p->st.cart[i].id        = (unsigned int)tmp_int[0];
			p->st.cart[i].nameid    = tmp_int[1];
			p->st.cart[i].amount    = tmp_int[2];
			p->st.cart[i].equip     = tmp_int[3];
			p->st.cart[i].identify  = tmp_int[4];
			p->st.cart[i].refine    = tmp_int[5];
			p->st.cart[i].attribute = tmp_int[6];
			p->st.cart[i].card[0]   = tmp_int[7];
			p->st.cart[i].card[1]   = tmp_int[8];
			p->st.cart[i].card[2]   = tmp_int[9];
			p->st.cart[i].card[3]   = tmp_int[10];
			p->st.cart[i].limit     = (unsigned int)tmp_int[11];
		}
		next+=len;
		if(str[next]==' ')
			next++;
	}
	next++;
	for(i=0;str[next] && str[next]!='\t';i++){
		set=sscanf(str+next,"%d,%d%n",&tmp_int[0],&tmp_int[1],&len);
		if(set!=2)
			return 0;
		n = tmp_int[0];
		if(n >= 0 && n < MAX_PCSKILL) {
			p->st.skill[n].id = tmp_int[0];
			p->st.skill[n].lv = tmp_int[1];
		}
		next+=len;
		if(str[next]==' ')
			next++;
	}
	next++;

	for(i=0;str[next] && str[next]!='\t';i++){
		set=sscanf(str+next,"%255[^,],%d%n",tmp_str[0],&tmp_int[0],&len);
		if(set!=2)
			return 0;
		if(i < GLOBAL_REG_NUM) {
			strncpy(p->reg.global[i].str, tmp_str[0], 32);
			p->reg.global[i].str[31] = '\0';	// force \0 terminal
			p->reg.global[i].value   = tmp_int[0];
		} else {
			printf("char_load %d: couldn't load %s (GLOBAL_REG_NUM = %d)\a\n", p->st.char_id, tmp_str[0], GLOBAL_REG_NUM);
		}
		next+=len;
		if(str[next]==' ')
			next++;
	}
	p->reg.global_num = (i < GLOBAL_REG_NUM)? i: GLOBAL_REG_NUM;
	next++;

	for(i=0;str[next] && str[next]!='\t';i++){
		set=sscanf(str+next,"%d,%d%n",&tmp_int[0],&tmp_int[1],&len); // name �͌�ŉ�������
		if(set!=2)
			return 0;
		if(i < MAX_FRIEND) {
			p->st.friend_data[i].account_id = tmp_int[0];
			p->st.friend_data[i].char_id    = tmp_int[1];
		}
		next+=len;
		if(str[next]==' ')
			next++;
	}
	p->st.friend_num = (i < MAX_FRIEND)? i: MAX_FRIEND;
	next++;

	for(i = 0; str[next] && str[next] != '\t' && str[next] != '\n' && str[next] != '\r'; i++) {
		set=sscanf(str+next,"%255[^,],%d%n",tmp_str[0],&tmp_int[0],&len);
		if(set!=2)
			return 0;
		n = tmp_int[0];
		if(n >= 0 && n < 3) {
			strncpy(p->st.feel_map[n], tmp_str[0], 24);
			p->st.feel_map[n][23] = '\0';	// force \0 terminal
		}
		next+=len;
		if(str[next]==' ')
			next++;
	}
	next++;

	for(i = 0; str[next] && str[next] != '\t' && str[next] != '\n' && str[next] != '\r'; i++) {
		set=sscanf(str+next,"%d,%d,%d,%d%n",&tmp_int[0],&tmp_int[1],&tmp_int[2],&tmp_int[3],&len);
		if(set!=4) {
			// Athena�`�����L�蓾��̂Ő��������ɂ���
			return 1;
		}
		n = tmp_int[0];
		if(n >= 0 && n < MAX_HOTKEYS) {
			p->st.hotkey[n].type = (char)tmp_int[1];
			p->st.hotkey[n].id   = tmp_int[2];
			p->st.hotkey[n].lv   = (unsigned short)tmp_int[3];
		}
		next+=len;
		if(str[next]==' ')
			next++;
	}

	for(i = 0; str[next] && str[next] != '\t' && str[next] != '\n' && str[next] != '\r'; i++) {
		set=sscanf(str+next,"%d,%d,%d%n",&tmp_int[0],&tmp_int[1],&tmp_int[2],&len);
		if(set!=3) {
			// Athena�`�����L�蓾��̂Ő��������ɂ���
			return 1;
		}
		n = tmp_int[0];
		if(n >= 0 && n < MAX_MERC_TYPE) {
			p->st.merc_fame[n] = tmp_int[1];
			p->st.merc_call[n] = tmp_int[2];
		}
		next+=len;
		if(str[next]==' ')
			next++;
	}

	return 1;
}

#ifdef TXT_JOURNAL
// ==========================================
// �L�����N�^�[�f�[�^�̃W���[�i���̃��[���t�H���[�h�p�R�[���o�b�N�֐�
// ------------------------------------------
static int char_journal_rollforward( int key, void* buf, int flag )
{
	int idx = char_id2idx( key );

	// �O�̂��߃`�F�b�N
	if( flag == JOURNAL_FLAG_WRITE && key != ((struct mmo_chardata*)buf)->st.char_id )
	{
		printf("char_journal: key != char_id !\n");
		return 0;
	}

	// �f�[�^�̒u������
	if( idx >= 0 )
	{
		if( flag == JOURNAL_FLAG_DELETE ) {
			memset( &char_dat[idx], 0, sizeof(struct mmo_chardata) );
		} else {
			memcpy( &char_dat[idx], buf, sizeof(struct mmo_chardata) );
		}
		return 1;
	}

	// �ǉ�
	if( flag != JOURNAL_FLAG_DELETE )
	{
		if(char_num>=char_max)
		{
			// ������������Ȃ��Ȃ�g��
			char_max+=256;
			char_dat=(struct mmo_chardata *)aRealloc(char_dat,sizeof(char_dat[0])*char_max);
			memset(char_dat + (char_max - 256), '\0', 256 * sizeof(char_dat[0]));
		}

		memcpy( &char_dat[char_num], buf, sizeof(struct mmo_chardata) );
		if(char_dat[char_num].st.char_id>=char_id_count)
			char_id_count=char_dat[char_num].st.char_id+1;
		char_num++;
		return 1;
	}

	return 0;
}
#endif

bool chardb_txt_init(void)
{
	char line[65536];
	int ret, i, j;
	FILE *fp;

	fp=fopen(char_txt,"r");
	char_dat=(struct mmo_chardata *)aCalloc(256,sizeof(char_dat[0]));
	char_max=256;
	if(fp==NULL)
		return 0;
	while(fgets(line,65535,fp)){
		j = -1;
		if( sscanf(line,"%d\t%%newid%%%n",&i,&j)==1 && j > 0 && (line[j]=='\n' || line[j]=='\r') ){
			if(char_id_count<i)
				char_id_count=i;
			continue;
		}

		if(char_num>=char_max){
			char_max+=256;
			char_dat=(struct mmo_chardata *)aRealloc(char_dat,sizeof(char_dat[0])*char_max);
			memset(char_dat + (char_max - 256), '\0', 256 * sizeof(char_dat[0]));
		}

		ret=mmo_char_fromstr(line,&char_dat[char_num]);
		if(ret){
			int char_id = char_dat[char_num].st.char_id;
			if(char_id >= char_id_count)
				char_id_count = char_id+1;

			if(char_num > 0 && char_id < char_dat[char_num-1].st.char_id) {
				struct mmo_chardata tmp;
				int k = char_num;

				// ���̂��L����ID�̏����ɕ���łȂ��ꍇ�͑}���\�[�g����
				while(--k > 0 && char_id < char_dat[k-1].st.char_id);

				memcpy(&tmp, &char_dat[char_num], sizeof(char_dat[0]));
				memmove(&char_dat[k+1], &char_dat[k], (char_num-k)*sizeof(char_dat[0]));
				memcpy(&char_dat[k], &tmp, sizeof(char_dat[0]));
			}
			char_num++;
		}
	}
	fclose(fp);

#ifdef TXT_JOURNAL
	if( char_journal_enable )
	{
		// �W���[�i���f�[�^�̃��[���t�H���[�h
		if( journal_load( &char_journal, sizeof(struct mmo_chardata), char_journal_file ) )
		{
			int c = journal_rollforward( &char_journal, char_journal_rollforward );

			printf("char: journal: roll-forward (%d)\n", c );

			// ���[���t�H���[�h�����̂ŁAtxt �f�[�^��ۑ����� ( journal ���V�K�쐬�����)
			chardb_txt_sync();
		}
		else
		{
			// �W���[�i����V�K�쐬����
			journal_final( &char_journal );
			journal_create( &char_journal, sizeof(struct mmo_chardata), char_journal_cache, char_journal_file );
		}
	}
#endif

	// �F�B���X�g�̖��O������
	for( i=0; i<char_num; i++ )
	{
		for( j=0; j<char_dat[i].st.friend_num; j++ )
		{
			struct friend_data* frd = char_dat[i].st.friend_data;
			const struct mmo_chardata* p = chardb_txt_load(frd[j].char_id);
			if( p ) {
				memcpy( frd[j].name, p->st.name, 24 );
			} else {
				char_dat[i].st.friend_num--;
				memmove( &frd[j], &frd[j+1], sizeof(frd[0])*( char_dat[i].st.friend_num - j ) );
				j--;
			}
		}
	}

	return true;
}

void chardb_txt_sync(void)
{
	char line[65536];
	int i,lock;
	FILE *fp;

	if( !char_dat )
		return;

	fp=lock_fopen(char_txt,&lock);
	if(fp==NULL)
		return;
	for(i=0;i<char_num;i++){
		if(char_dat[i].st.char_id > 0 && char_dat[i].st.account_id > 0) {
			mmo_char_tostr(line,&char_dat[i]);
			fprintf(fp,"%s" RETCODE,line);
		}
	}
	fprintf(fp,"%d\t%%newid%%" RETCODE,char_id_count);
	lock_fclose(fp,char_txt,&lock);

#ifdef TXT_JOURNAL
	if( char_journal_enable )
	{
		// �R�~�b�g�����̂ŃW���[�i����V�K�쐬����
		journal_final( &char_journal );
		journal_create( &char_journal, sizeof(struct mmo_chardata), char_journal_cache, char_journal_file );
	}
#endif
}

void chardb_txt_final(void)
{
	//chardb_txt_sync(); // do_final �ŌĂ�ł�͂�
	aFree(char_dat);

#ifdef TXT_JOURNAL
	if( char_journal_enable )
	{
		journal_final( &char_journal );
	}
#endif
}

const struct mmo_chardata *chardb_txt_make(int account_id,unsigned char *dat,int *flag)
{
	int n, idx;
	short str, agi, vit, int_, dex, luk;
	short hair, hair_color;
	unsigned char slot;
	char name[24];

	memset(name, 0, sizeof(name));
	for(n = 0; n < 24 && dat[n]; n++) {
		if(dat[n] < 0x20 || dat[n] == 0x7f)
			return NULL;
		name[n] = dat[n];
	}
	name[23] = '\0';	// force \0 terminal

	slot = dat[30];
	if(slot >= max_char_slot) {
		*flag = 0x03;
		printf("make new char over slot!! %s (%d / %d)\n", name, slot + 1, max_char_slot);
		return NULL;
	}

	str  = dat[24];
	agi  = dat[25];
	vit  = dat[26];
	int_ = dat[27];
	dex  = dat[28];
	luk  = dat[29];

	if(str > 9 || agi > 9 || vit > 9 || int_ > 9 || dex > 9 || luk > 9)
		return NULL;

	// �X�e�[�^�X�|���S���̃`�F�b�N
	if(check_status_polygon == 1 && str + agi + vit + int_ + dex + luk > 5 * 6) {
		charlog_log(
			"make new char error: status point over %d %s %d,%d,%d,%d,%d,%d",
			slot, name, str, agi, vit, int_, dex, luk
		);
		return NULL;
	}
	if(check_status_polygon == 2 && (str + int_ != 10 || agi + luk != 10 || vit + dex != 10)) {
		charlog_log(
			"make new char error: invalid status point %d %s %d,%d,%d,%d,%d,%d",
			slot, name, str, agi, vit, int_, dex, luk
		);
		return NULL;
	}

	hair       = dat[33];
	hair_color = dat[31];

	if(hair == 0 || hair >= MAX_HAIR_STYLE || hair_color >= MAX_HAIR_COLOR) {
		charlog_log("make new char error: invalid hair %d %s %d,%d", slot, name, hair, hair_color);
		return NULL;
	}

	for(n = 0; n < char_num; n++) {
		if(strcmp(char_dat[n].st.name, name) == 0) {
			*flag = 0x00;
			return NULL;
		}
		if(char_dat[n].st.account_id > 0 && char_dat[n].st.account_id == account_id && char_dat[n].st.char_num == slot)
			return NULL;
	}

	charlog_log("make new char %d %s", slot, name);

	if(char_num >= char_max) {
		// realloc() �����char_dat�̈ʒu���ς��̂ŁAsession �̃f�[�^������
		// �����I�ɒu�������鏈�������Ȃ��ƃ_���B
		int i, j;
		struct mmo_chardata *char_dat_old = char_dat;
		struct mmo_chardata *char_dat_new = (struct mmo_chardata *)aMalloc(sizeof(struct mmo_chardata) * (char_max + 256));

		memcpy(char_dat_new, char_dat_old, sizeof(struct mmo_chardata) * char_max);
		memset(char_dat_new + char_max, 0, 256 * sizeof(struct mmo_chardata));
		char_max += 256;
		for(i = 0; i < fd_max; i++) {
			struct char_session_data *sd;
			if(session[i] && session[i]->func_parse == parse_char && (sd = (struct char_session_data *)session[i]->session_data)) {
				for(j = 0; j < max_char_slot; j++) {
					if(sd->found_char[j]) {
						sd->found_char[j] = char_dat_new + (sd->found_char[j] - char_dat_old);
					}
				}
			}
		}
		char_dat = char_dat_new;
		aFree(char_dat_old);
	}
	memset(&char_dat[n], 0, sizeof(char_dat[0]));

	char_dat[n].st.char_id       = char_id_count++;
	char_dat[n].st.account_id    = account_id;
	char_dat[n].st.char_num      = slot;
	strncpy(char_dat[n].st.name, name, 24);
	char_dat[n].st.class_        = 0;
	char_dat[n].st.base_level    = 1;
	char_dat[n].st.job_level     = 1;
	char_dat[n].st.base_exp      = 0;
	char_dat[n].st.job_exp       = 0;
	char_dat[n].st.zeny          = start_zeny;
	char_dat[n].st.str           = str;
	char_dat[n].st.agi           = agi;
	char_dat[n].st.vit           = vit;
	char_dat[n].st.int_          = int_;
	char_dat[n].st.dex           = dex;
	char_dat[n].st.luk           = luk;
	char_dat[n].st.max_hp        = 40 * (100 + vit)  / 100;
	char_dat[n].st.max_sp        = 11 * (100 + int_) / 100;
	char_dat[n].st.hp            = char_dat[n].st.max_hp;
	char_dat[n].st.sp            = char_dat[n].st.max_sp;
	char_dat[n].st.status_point  = 0;
	char_dat[n].st.skill_point   = 0;
	char_dat[n].st.option        = 0;
	char_dat[n].st.karma         = 0;
	char_dat[n].st.manner        = 0;
	char_dat[n].st.die_counter   = 0;
	char_dat[n].st.party_id      = 0;
	char_dat[n].st.guild_id      = 0;
	char_dat[n].st.hair          = hair;
	char_dat[n].st.hair_color    = hair_color;
	char_dat[n].st.clothes_color = 0;

	idx = 0;
	if(start_weapon > 0) {
		char_dat[n].st.inventory[idx].id       = 1;
		char_dat[n].st.inventory[idx].nameid   = start_weapon;
		char_dat[n].st.inventory[idx].amount   = 1;
		char_dat[n].st.inventory[idx].equip    = 0x02;
		char_dat[n].st.inventory[idx].identify = 1;
		char_dat[n].st.weapon = 1;
		idx++;
	}
	if(start_armor > 0) {
		char_dat[n].st.inventory[idx].id       = 2;
		char_dat[n].st.inventory[idx].nameid   = start_armor;
		char_dat[n].st.inventory[idx].amount   = 1;
		char_dat[n].st.inventory[idx].equip    = 0x10;
		char_dat[n].st.inventory[idx].identify = 1;
	}
	char_dat[n].st.shield      = 0;
	char_dat[n].st.head_top    = 0;
	char_dat[n].st.head_mid    = 0;
	char_dat[n].st.head_bottom = 0;
	memcpy(&char_dat[n].st.last_point, &start_point, sizeof(start_point));
	memcpy(&char_dat[n].st.save_point, &start_point, sizeof(start_point));
	char_num++;

#ifdef TXT_JOURNAL
	if( char_journal_enable )
		journal_write( &char_journal, char_dat[n].st.char_id, &char_dat[n] );
#endif

	return &char_dat[n];
}

int chardb_txt_load_all(struct char_session_data* sd,int account_id)
{
	int i;
	int found_num = 0;

	for(i = 0; i < char_num; i++) {
		if(char_dat[i].st.account_id > 0 && char_dat[i].st.account_id == account_id && char_dat[i].st.char_num < max_char_slot) {
			sd->found_char[found_num++] = &char_dat[i];
			if(found_num == max_char_slot)
				break;
		}
	}
	for(i = found_num; i < max_char_slot; i++) {
		sd->found_char[i] = NULL;
	}

	return found_num;
}

const struct mmo_chardata* chardb_txt_load(int char_id)
{
	int idx = char_id2idx(char_id);

	return (idx >= 0) ? &char_dat[idx] : NULL;
}

bool chardb_txt_save_reg(int account_id,int char_id,int num,struct global_reg *reg)
{
	int idx = char_id2idx(char_id);

	if(idx >= 0) {
		if(char_dat[idx].st.account_id == account_id) {
			memcpy(&char_dat[idx].reg.global,reg,sizeof(char_dat[idx].reg.global));
			char_dat[idx].reg.global_num = num;
#ifdef TXT_JOURNAL
			if( char_journal_enable )
				journal_write( &char_journal, char_id, &char_dat[idx] );
#endif
			return true;
		}
	}

	return true;
}

bool chardb_txt_save(struct mmo_charstatus *st)
{
	int idx = char_id2idx(st->char_id);

	if(idx >= 0) {
		if(char_dat[idx].st.account_id == st->account_id) {
			memcpy(&char_dat[idx].st,st,sizeof(struct mmo_charstatus));
#ifdef TXT_JOURNAL
			if( char_journal_enable )
				journal_write( &char_journal, st->char_id, &char_dat[idx] );
#endif
			return true;
		}
	}
	return true;
}

bool chardb_txt_delete_sub(int char_id)
{
	int idx = char_id2idx(char_id);

	if(idx >= 0) {
		memset(&char_dat[idx],0,sizeof(char_dat[0]));
		char_dat[idx].st.char_id = char_id;	// �L����ID�͈ێ�
#ifdef TXT_JOURNAL
		if( char_journal_enable )
			journal_write( &char_journal, char_id, NULL );
#endif
		return true;
	}
	return true;
}

int chardb_txt_config_read_sub(const char* w1,const char* w2)
{
	if( strcmpi(w1,"char_txt") == 0 )
		strncpy(char_txt,w2,sizeof(char_txt) - 1);
	else if( strcmpi(w1,"gm_account_filename") == 0 )
		strncpy(GM_account_filename,w2,sizeof(GM_account_filename) - 1);
#ifdef TXT_JOURNAL
	else if( strcmpi(w1,"char_journal_enable") == 0 )
		char_journal_enable = atoi( w2 );
	else if( strcmpi(w1,"char_journal_file") == 0 )
		strncpy( char_journal_file, w2, sizeof(char_journal_file) - 1 );
	else if(strcmpi( w1,"char_journal_cache_interval") == 0 )
		char_journal_cache = atoi( w2 );
#endif
	else
		return 0;

	return 1;
}

const struct mmo_chardata* chardb_txt_nick2chardata(const char *char_name)
{
	int i;

	for(i=0;i<char_num;i++){
		if(char_dat[i].st.account_id > 0 && strcmp(char_dat[i].st.name,char_name)==0) {
			return &char_dat[i];
		}
	}

	return NULL;
}

bool chardb_txt_set_online(int char_id, bool is_online)
{
	// nothing to do
	return true;
}

bool chardb_txt_build_ranking(void)
{
	int i,j,k;
	int count[MAX_RANKING];
	struct Ranking_Data *rd[MAX_RANKING];

	if(char_num <= 0)
		return true;

	memset(&ranking_data, 0, sizeof(ranking_data));
	memset(&count, 0, sizeof(count));

	for(i=0; i<MAX_RANKING; i++)
		rd[i] = (struct Ranking_Data *)aCalloc(char_num, sizeof(struct Ranking_Data));

	for(i=0; i<char_num; i++) {
		for(j=0; j<char_dat[i].reg.global_num; j++) {
			for(k=0; k<MAX_RANKING; k++) {
				if(strcmp(char_dat[i].reg.global[j].str, ranking_reg[k]) == 0 && char_dat[i].reg.global[j].value > 0) {
					strncpy(rd[k][count[k]].name, char_dat[i].st.name, 24);
					rd[k][count[k]].point   = char_dat[i].reg.global[j].value;
					rd[k][count[k]].char_id = char_dat[i].st.char_id;
					count[k]++;
				}
			}
		}
	}

	for(k=0; k<MAX_RANKING; k++) {
		if(count[k] > 0) {
			int max = (char_num < MAX_RANKER)? char_num: MAX_RANKER;
			qsort(rd[k], char_num, sizeof(struct Ranking_Data), compare_ranking_data);
			memcpy(&ranking_data[k], rd[k], max * sizeof(struct Ranking_Data));
		}
		aFree(rd[k]);
	}

	return true;
}
