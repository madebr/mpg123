#include "config.h"
#include "compat.h"
#include "dither.h"

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
	
	for(i=0;i<DITHERSIZE;i++)
	{
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
		xv[8] = input_noise / 1.046605543e+07;
		yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; yv[4] = yv[5]; yv[5] = yv[6]; yv[6] = yv[7]; yv[7] = yv[8]; 
		yv[8] = (xv[0] + xv[8]) - 8 * (xv[1] + xv[7]) + 28 * (xv[2] + xv[6])
				- 56 * (xv[3] + xv[5]) + 70 * xv[4]
				+ ( -0.6610337226 * yv[0]) + ( -5.2856836445 * yv[1])
				+ (-18.7646200370 * yv[2]) + (-38.6287198220 * yv[3])
				+ (-50.4388759960 * yv[4]) + (-42.7846655830 * yv[5])
				+ (-23.0310886710 * yv[6]) + ( -7.1965249172 * yv[7]);
		dithertable[i] = yv[8] * 3.0f;
	}
}
