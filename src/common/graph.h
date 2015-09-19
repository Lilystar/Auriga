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

#ifndef _GRAPH_H_
#define _GRAPH_H_

void do_init_graph(void);

// auriga�̏�Ԃ𒲍�����Z���T�[��ǉ�����B
// string        : �Z���T�[�̖���(Login Users �Ȃ�)
// inetrval      : �Z���T�[�̒l���擾����Ԋu(msec)
// callback_func : �Z���T�[�̒l��Ԃ��֐�( unsigned int login_users(void); �Ȃ�)

void graph_add_sensor(const char* string, int interval, double (*callback_func)(void));

#endif

