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
#include <errno.h>

#include "lock.h"
#include "utils.h"

// �������݃t�@�C���̕ی쏈��
// �i�������݂��I���܂ŁA���t�@�C����ۊǂ��Ă����j

// �V�����t�@�C���̏������݊J�n
FILE* lock_fopen(const char* filename,int *info)
{
	char newfile[2048];
	FILE *fp;
	int  no = 0;

	// ���S�ȃt�@�C�����𓾂�i�蔲���j
	do {
		snprintf(newfile, sizeof(newfile), "%s_%04d.tmp", filename, ++no);
	} while( (fp = fopen(newfile,"r")) && (fclose(fp), no < 9999) );

	if(no >= 9999)
		return NULL;

	*info = no;
	return fopen(newfile,"w");
}

// ���t�@�C�����폜���V�t�@�C�������l�[��
int lock_fclose(FILE *fp,const char* filename,int *info)
{
	int  ret = 0;
	char newfile[2048];

	if(fp != NULL) {
		ret = fclose(fp);
		snprintf(newfile, sizeof(newfile), "%s_%04d.tmp", filename, *info);
		if(remove(filename) == 0 || errno == ENOENT) {
			// ���̃^�C�~���O�ŗ�����ƍň��B
			rename(newfile,filename);
		}
		return ret;
	}
	return 1;
}
