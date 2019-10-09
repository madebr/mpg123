// A little test to verify my logic of precomputing the interpolator
// output in of the resampler.

#include <stdint.h>
#include <stdio.h>

// The real code uses 64 bit numbers and hence has to emulate
// multiplication into 128 bits. The test just replaces all
// 64 bit numbers by 8 bit numbers to be able to simply enumerate
// and evaluate all possibilities.

static void mul8(uint8_t a, uint8_t b, uint8_t * m1, uint8_t * m0)
{
	uint8_t lowmask = ((uint8_t)1 << 4) - 1;
	uint8_t a1 = a >> 4;
	uint8_t a0 = a & lowmask;
	uint8_t b1 = b >> 4;
	uint8_t b0 = b & lowmask;
	uint8_t prod1 = a1 * b1;
	uint8_t prod0 = a0 * b0;
	uint8_t a1b0 = a1 * b0;
	uint8_t a0b1 = a0 * b1;
	prod1 += a0b1 >> 4;
	prod1 += a1b0 >> 4;
	uint8_t prod0t = prod0 + ((a0b1 & lowmask) << 4);
	if(prod0t < prod0)
		prod1 += 1;
	prod0 = prod0t + ((a1b0 & lowmask) << 4);
	if(prod0 < prod0t)
		prod1 += 1;
	*m1 = prod1;
	*m0 = prod0;
}

static uint8_t muloffdiv8( uint8_t a, uint8_t b, int8_t off
,	uint8_t c, int *err, uint8_t *m )
{
	uint8_t prod1, prod0;
	uint8_t div1, div0, div0old;
	if(c < 1)
	{
		if (err)
			*err = 2;
		return 0;
	}
	mul8(a, b, &prod1, &prod0);
	if(off)
	{
		uint8_t prod0old = prod0;
		prod0 += off;
		// Offset can be positive or negative, small or large.
		// When adding to prod0, these cases can happen:
		// offset > 0, result > prod0: All fine.
		// offset > 0, result < prod0: Overflow.
		// offset < 0, result < prod0: All fine.
		// offset < 0, result > prod0: Underflow.
		if(off > 0 && prod0 < prod0old)
		{
			if(prod1 == UINT8_MAX)
			{
				if(err)
					*err = 1;
				return 0;
			}
			++prod1;
		}
		if(off < 0 && prod0 > prod0old)
		{
			// Pin to zero on total underflow.
			if(prod1 == 0)
				prod0 = 0;
			else
				--prod1;
		}
	}
	if(c == 1)
	{
		div1 = prod1;
		div0 = prod0;
		if(m)
			*m = 0;
	} else
	{
		div1 = 0;
		div0 = 0;
		uint8_t ctimes = ((uint8_t) - 1) / c;
		uint8_t cblock = ctimes * c;
		while(prod1)
		{
			uint8_t cmult1, cmult0;
			mul8(ctimes, prod1, &cmult1, &cmult0);
			div1 += cmult1;
			div0old = div0;
			div0 += cmult0;
			if(div0 < div0old)
				div1++;
			mul8(cblock, prod1, &cmult1, &cmult0);
			prod1 -= cmult1;
			if(prod0 < cmult0)
				prod1--;
			prod0 -= cmult0;
		}
		div0old = div0;
		div0 += prod0 / c;
		if(m)
			*m = prod0 % c;
		if(div0 < div0old)
			div1++;
	}
	if(div1)
	{
		if(err)
			*err = 1;
		return 0;
	}
	if(err)
		*err = 0;
	return div0;
}

// The reference: Loops following the actual interpolation code.
uint8_t simulate_interpolate( int8_t vinrate, int8_t outrate
,	int8_t offset, uint8_t ins )
{
	uint8_t outs = 0;
	for(uint8_t n = 0; n<ins; ++n)
	{
		while(offset+vinrate < outrate)
		{
			if(outs == UINT8_MAX)
				return 0; // overflow!
			else
				++outs;
			offset += vinrate;
		}
		offset -= outrate;
	}
	return outs;
}


int run_test(uint8_t oversample
,	int8_t inrate, int8_t outrate, int8_t offset, uint8_t ins)
{
	if(oversample < 1)
		oversample = 1;
	if(ins > UINT8_MAX/oversample)
		return 0;
	uint8_t vins = ins*oversample;
	uint8_t vinrate = inrate*oversample;

	// Here it's all oversampled samples.

	// This is the truth.
	uint8_t simout = simulate_interpolate(vinrate, outrate, offset, vins);
	// Prediction of output samples.
	uint8_t compout = muloffdiv8( vins, outrate, -offset-1
	,	vinrate, NULL, NULL );
	// Reverse prediction of needed input samples.
	uint8_t compin = simout
	?	muloffdiv8(simout, vinrate, offset, outrate, NULL, NULL) + 1
	:	0;
	if(compin % oversample)
		++compin;
	compin /= oversample;
	uint8_t compinsimout =
		simulate_interpolate(vinrate, outrate, offset, oversample*compin);

	if(simout != compout)
	{
		fprintf( stderr
		,	"out mismatch for inrate=%d outrate=%d offset=%d ins=%d:"
			" %d != %d\n"
		,	inrate, outrate, offset, ins, simout, compout);
		return 1;
	}
	if(compinsimout != simout)
	{
		fprintf( stderr
		,	"out mismatch for inrate=%d outrate=%d offset=%d"
			" ins=%d compin=%d:"
			" %d != %d\n"
		,	inrate, outrate, offset, ins, compin, simout, compinsimout);
		return 2;
	}
	// More strict: compin should be the _smallest_ inpout count needed.
	if(compin)
	{
		uint8_t lessout =
			simulate_interpolate(vinrate, outrate, offset, oversample*(compin-1));
		if(!(lessout < simout))
		{
			fprintf( stderr, "compin not minimal for "
				"inrate=%d outrate=%d offset=%d ins=%d compin=%d outs=%d\n"
			,	inrate, outrate, offset, ins, compin, simout );
			return 3;
		}
	}
	return 0;
}


int main()
{
	for(int8_t inrate = 1; inrate < INT8_MAX/2; ++inrate)
	for(int8_t outrate = 1; outrate < INT8_MAX/2; ++outrate)
	for(uint8_t oversample = 1; oversample <= 2; ++oversample)
	for(int8_t offset = -(oversample*inrate); offset < oversample*inrate; ++offset)
	{
		fprintf(stderr, "%dx %d -> %d off %d\n", oversample, inrate, outrate, offset);
		uint8_t ins = 0;
		do
		{
			int ret = run_test(oversample, inrate, outrate, offset, ins);
			if(ret)
				return ret;
		} while(++ins);
	}
	return 0;
}
