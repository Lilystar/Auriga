
#include <stdio.h>

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
		remove(filename);
		// ���̃^�C�~���O�ŗ�����ƍň��B
		rename(newfile,filename);
		return ret;
	}
	return 1;
}
