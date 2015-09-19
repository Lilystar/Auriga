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

#include "utils.h"
#include "malloc.h"


/*==========================================
 * Hex Dump
 *  �W���o�͂̕\�����X���[�Y�ɂ��邽��
 *  ���O�Ŋ��S�o�b�t�@�����O����
 *------------------------------------------
 */
void hex_dump(FILE *fp, const unsigned char *buf, size_t len)
{
	size_t i, j;
	char *output, *p;

	output = (char *)aCalloc((3 + (len - 1) / 16) * 96, sizeof(char));	// 1�s������96�����Ƃ��Čv�Z
	p = output;

	p += sprintf(p, "      00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F   0123456789ABCDEF" RETCODE);
	p += sprintf(p, "----  -----------------------------------------------   ----------------");

	for(i = 0; i < len; i += 16) {
		p += sprintf(p, RETCODE "%04X  ", i);
		for(j = i; j < i + 16; j++) {
			if(j < len)
				p += sprintf(p, "%02x ", buf[j]);
			else
				p += sprintf(p, "   ");
		}

		p += sprintf(p, "  ");
		for(j = i; j < i + 16; j++) {
			if(j < len)
				p += sprintf(p, "%c", (buf[j] <= 0x20) ? '.' : buf[j]);
			else
				p += sprintf(p, " ");
		}
	}
	p += sprintf(p, "\n");

	fprintf(fp, output);
	fflush(fp);

	aFree(output);
}

/*==========================================
 * 32bit���`�����@(�߂�l��24�r�b�g�L��)
 *------------------------------------------
 */
#ifdef RANDOM32
static int seed32 = 1;

void atn_int24_srandom32( int seed )
{
	seed32 = seed;
}
int atn_int24_random32(void)
{
	seed32 = seed32 * 1103515245 + 12345;
	return ( seed32 >> 8 )&0x00ffffff;
}
#endif	// ifdef RANDOM32

/*==========================================
 * 64bit���`�����@(�߂�l��31�r�b�g�L��)
 *------------------------------------------
 */
#ifdef RANDOM64
static int64 seed64 = 0x1234ABCD330E;

void atn_int31_srandom64( int seed )
{
	seed64 = seed;
	seed64 = (seed64<<16)+0x330E;
}
int atn_int31_random64(void)
{
	seed64 = seed64 * 0x5DEECE66D + 0xB;
	return (int)( seed64>>17 )&0x7FFFFFFF;
}
#endif	// ifdef RANDOM64

/*==========================================
 * �����Z���k�c�C�X�^�[
 *------------------------------------------
 */
// http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/mt.html
#ifdef RANDOMMT

/* Period parameters */
#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   /* constant vector a */
#define UPPER_MASK 0x80000000UL /* most significant w-r bits */
#define LOWER_MASK 0x7fffffffUL /* least significant r bits */

static unsigned long mt[N]; /* the array for the state vector  */
static int mti=N+1; /* mti==N+1 means mt[N] is not initialized */

/* initializes mt[N] with a seed */
void atn_srandommt(unsigned long s)
{
	mt[0]= s & 0xffffffffUL;
	for (mti=1; mti<N; mti++) {
		mt[mti] = (1812433253UL * (mt[mti-1] ^ (mt[mti-1] >> 30)) + mti);

		/* See Knuth TAOCP Vol2. 3rd Ed. P.106 for multiplier. */
		/* In the previous versions, MSBs of the seed affect   */
		/* only MSBs of the array mt[].                        */
		/* 2002/01/09 modified by Makoto Matsumoto             */
		mt[mti] &= 0xffffffffUL;
		/* for >32 bit machines */
	}
}

/* generates a random number on [0,0xffffffff]-interval */
static unsigned long atn_int32_randommt(void)
{
	unsigned long y;
	static unsigned long mag01[2]={0x0UL, MATRIX_A};

	/* mag01[x] = x * MATRIX_A  for x=0,1 */
	if (mti >= N) {		/* generate N words at one time */
		int kk;

		if (mti == N+1)		/* if init_genrand() has not been called, */
			atn_srandommt(5489UL);	/* a default initial seed is used */

		for (kk=0;kk<N-M;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
		}
		for (;kk<N-1;kk++) {
			y = (mt[kk]&UPPER_MASK)|(mt[kk+1]&LOWER_MASK);
			mt[kk] = mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
		}
		y = (mt[N-1]&UPPER_MASK)|(mt[0]&LOWER_MASK);
		mt[N-1] = mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

		mti = 0;
	}

	y = mt[mti++];

	/* Tempering */
	y ^= (y >> 11);
	y ^= (y << 7) & 0x9d2c5680UL;
	y ^= (y << 15) & 0xefc60000UL;
	y ^= (y >> 18);

	return y;
}

long atn_int31_randommt(void)
{
	return (long)(atn_int32_randommt()>>1);
}

#endif	// ifdef RANDOMMT
