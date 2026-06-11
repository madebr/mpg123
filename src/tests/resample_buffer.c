// Test the various sample count computations and against output
// buffer overflow of libsyn123.

#include "mpg123config.h"
#include <syn123.h>
#include <inttypes.h>

#include <stdlib.h>
#include <stdio.h>

#include "common/debug.h"

const long arate = 22050;
const long brate = 48000;
const size_t outs = 0x10000;
const int buffer_runs = 100;
const float canary = 2317.;

int main()
{
	size_t ins = syn123_resample_incount(arate, brate, outs);
	size_t outs2 = syn123_resample_count(arate, brate, ins);
	size_t fillins = syn123_resample_fillcount(arate, brate, outs);
	size_t fillouts = syn123_resample_count(arate, brate, fillins);
	printf("%zu output buffer size\n", outs);
	printf("%zu minimal input samples to ensure we can fill that buffer\n", ins);
	printf("%zu maximum output samples to result from the above\n", outs2);
	printf("%zu maximum input samples to not overflow that buffer\n", fillins);
	printf("%zu maximum output samples to result from the above\n", fillouts);
	int err = 0;

	syn123_handle *sh = syn123_new( brate, 1, MPG123_ENC_FLOAT_32, 1024, &err);
	if(!sh)
		mereturn(1, "handle alloc failure: %s", syn123_strerror(err));

	// A dirty resampler.
	if(SYN123_OK != (err =syn123_setup_resample(sh, arate, brate, 1, 1, 0)))
		mereturn(1, "resample setup failure: %s", syn123_strerror(err));

	// Default sine wave as source signal.
	if( SYN123_OK !=
		(err =syn123_setup_waves( sh, 0, NULL, NULL, NULL, NULL, NULL)) )
		mereturn(1, "wave setup failure: %s", syn123_strerror(err));

	float *inbuf = malloc(sizeof(float)*ins);
	float *outbuf = malloc(sizeof(float)*(outs2+10));
	if(!inbuf || !outbuf)
		ereturn(1, "buffer alloc failure");

	for(int r=0; r<buffer_runs; ++r)
	{
		// Prepare input, 
		size_t got = syn123_read(sh, inbuf, sizeof(float)*ins);
		if(got != sizeof(float)*ins)
			mereturn( 1
			,	"unexpected byte count from generator in run %d: %zu != %zu"
			,	r, got, sizeof(float)*ins);
		// Prime output buffer with some weird large number.
		for(int o=0; o<(outs2+10); ++o) outbuf[o] = canary;
		size_t exp = syn123_resample_out(sh, ins, NULL);
		// Resample, check if anything is overwritten beyond the desired end.
		got = syn123_resample(sh, outbuf, inbuf, ins);
		if(!got || exp != got)
			mereturn( 1
			,	"got nothing or at least not what was expected"
				" in run %d: %zu != %zu"
			,	r, got, exp );
		if(got < outs)
			mereturn( 1
			,	"got less than promised minimum output sample count"
				" in run %d: %zu < %zu"
			,	r, got, outs );
		if(got > outs2)
			mereturn( 1
			,	"got more than promised maximum output sample count"
				" in run %d: %zu > %zu"
			,	r, got, outs2 );
		size_t over = 0;
		for(int o=outs2; o<(outs2+10); ++o)
			if(outbuf[o] != canary)
			{
				merror( "resample output overflow at %zu+%zu in run %d: %f != %f"
				,	outs2, o-outs2, r, outbuf[o], canary );
				++over;
			}
		if(over)
			mereturn(1, "%zu samples overflow in run %d", over, r);
	}
	printf("%d resampler runs without issue\n", buffer_runs);
	printf("PASS\n");

	free(outbuf);
	free(inbuf);
	syn123_del(sh);

	return 0;
}
