/*
	dither: Generate noise for dithering / noise shaping.

	copyright 2009 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#include "config.h"
#include "compat.h"
#include "dither.h"

#define WRAP 100

void dither_table_init(float *dithertable)
{
	int32_t i;
	uint32_t seed = 2463534242UL;
	float input_noise;
	float xv[9], yv[9];
	union
	{
		uint32_t i;
		float f;
	} dither_noise;
	
	for(i=0;i<9;i++)
	{
		xv[i] = yv[i] = 0.0f;
	}
	
	for(i=0;i<DITHERSIZE+WRAP;i++)
	{
		if(i==DITHERSIZE) seed=2463534242UL;
		/* generate 1st pseudo-random number (xorshift32) */
		seed ^= (seed<<13);
		seed ^= (seed>>17);
		seed ^= (seed<<5);
		
		/* scale the number to [-0.5, 0.5] */
#ifdef IEEE_FLOAT
		dither_noise.i = (seed>>9)|0x3f800000;
		dither_noise.f -= 1.5f;
#else
		dither_noise.f = (double)seed / 4294967295.0;
		dither_noise.f -= 0.5f;
#endif
		
		input_noise = dither_noise.f;
		
		/* generate 2nd pseudo-random number, to make a TPDF distribution */
		seed ^= (seed<<13);
		seed ^= (seed>>17);
		seed ^= (seed<<5);
		
		/* scale the number to [-0.5, 0.5] */
#ifdef IEEE_FLOAT
		dither_noise.i = (seed>>9)|0x3f800000;
		dither_noise.f -= 1.5f;
#else
		dither_noise.f = (double)seed / 4294967295.0;
		dither_noise.f -= 0.5f;
#endif
		
		input_noise += dither_noise.f;
		
		/* apply 8th order Chebyshev high-pass IIR filter */
		xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; xv[4] = xv[5]; xv[5] = xv[6]; xv[6] = xv[7]; xv[7] = xv[8]; 
		xv[8] = input_noise / 1.382814179e+07;
		yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
		yv[8] = (xv[0] + xv[8]) - 8 * (xv[1] + xv[7]) + 28 * (xv[2] + xv[6])
				- 56 * (xv[3] + xv[5]) + 70 * xv[4]
				+ ( -0.6706204984 * yv[0]) + ( -5.3720827038 * yv[1])
				+ (-19.0865382480 * yv[2]) + (-39.2831607860 * yv[3])
				+ (-51.2308985070 * yv[4]) + (-43.3590135780 * yv[5])
				+ (-23.2632305320 * yv[6]) + ( -7.2370122050 * yv[7]);
		if(i>=WRAP) dithertable[i-WRAP] = yv[8] * 3.0f;
	}
}
