/*
	equalizer.c: equalizer settings

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/


#include "mpg123.h"

real equalizer[2][32];
real equalizer_sum[2][32];
int equalizer_cnt;

real equalizerband[2][SBLIMIT*SSLIMIT];

void do_equalizer(real *bandPtr,int channel) 
{
	int i;

	if(have_eq_settings) {
		for(i=0;i<32;i++)
			bandPtr[i] = REAL_MUL(bandPtr[i], equalizer[channel][i]);
	}

/*	if(param.equalizer & 0x2) {
		
		for(i=0;i<32;i++)
			equalizer_sum[channel][i] += bandPtr[i];
	}
*/
}

void do_equalizerband(real *bandPtr,int channel)
{
  int i;
  for(i=0;i<576;i++) {
    bandPtr[i] = REAL_MUL(bandPtr[i], equalizerband[channel][i]);
  }
}

