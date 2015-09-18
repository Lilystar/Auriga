
#include <stdio.h>
#include "lock.h"

// �������݃t�@�C���̕ی쏈��
// �i�������݂��I���܂ŁA���t�@�C����ۊǂ��Ă����j

// �V�����t�@�C���̏������݊J�n
FILE* lock_fopen(const char* filename,int *info) {
	char newfile[512];
	FILE *fp;
	int  no = 0;

	// ���S�ȃt�@�C�����𓾂�i�蔲���j
	do {
		sprintf(newfile,"%s_%04d.tmp",filename,++no);
	} while((fp = fopen(newfile,"r")) && (fclose(fp), no<9999) );
	*info = no;
	return fopen(newfile,"w");
}

// ���t�@�C�����폜���V�t�@�C�������l�[��
int lock_fclose(FILE *fp,const char* filename,int *info) {
	int  ret = 0;
	char newfile[512];
	if(fp != NULL) {
		ret = fclose(fp);
		sprintf(newfile,"%s_%04d.tmp",filename,*info);
		remove(filename);
		// ���̃^�C�~���O�ŗ�����ƍň��B
		rename(newfile,filename);
		return ret;
	} else {
		return 1;
	}
}

