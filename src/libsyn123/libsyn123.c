/*
	libsyn123: libsyn123 entry code and wave generators

	copyright 2017-2018 by the mpg123 project
	licensed under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	initially written by Thomas Orgis

	This code directly contains wave generators and the generic entry
	code for libsyn123. The waves started the whole thing and stay here
	for now. Other signal generators go into separate files.

	This code is testament to the ultimate programmer hybris: Just add
	something because it looks easy, a fun dirty hack. Then spend days
	doing it properly, including fun like squeezing multiple waves of
	differing shapes into periodic packets. Oh, and caring about
	vectorization for the heck of it. Offering sample formats down to
	8 bit alaw. Madness.
*/

#include "syn123_int.h"
#include "sample.h"
#include "debug.h"

static const double freq_error = 1e-4;
/* For our precisions, that value will always be good enough. */
static const double twopi = 2.0*3.14159265358979323846;

/* absolute value */
static double myabs(double a)
{
	return a < 0 ? -a : a;
}

/* round floating point to size_t */
static size_t round2size(double a)
{
	return a < 0 ? 0 : (size_t)(a+0.5);
}

/* fractional part, relating to frequencies (so long matches) */
static double myfrac(double a)
{
	return a-(long)a;
}

/* arbitrary phase -> [0..1) */
static double phasefrac(double p)
{
	return p-floor(p);
}

/*
	Given a set of wave frequencies, compute an approximate common
	period for the combined signal. Invalid frequencies are set to
	the error bound for some sanity.
	TODO: sort waves and begin with highest freq for stable/best result
*/
static double common_samples_per_period( long rate, size_t count
,	struct syn123_wave *waves, size_t size_limit )
{
	double spp = 0;
	size_t i;
	for(i=0; i<count; ++i)
	{
		double sppi;
		size_t periods = 1;
		/* Limiting sensible frequency range. */
		if(waves[i].freq < freq_error)
			waves[i].freq = freq_error;
		if(waves[i].freq > rate/2)
			waves[i].freq = rate/2;
		sppi = myabs((double)rate/waves[i].freq);
		debug2("freq=%g sppi=%g", waves[i].freq, sppi);
		if(spp == 0)
			spp = sppi;
		while
		(
			(periods+1)*spp <= size_limit &&
			myabs( myfrac(periods*spp / sppi) ) > freq_error
		)
			periods++;
		spp*=periods;
		debug3( "samples_per_period + %f Hz = %g (%" SIZE_P " periods)"
		,	waves[i].freq, spp, periods );
	}
	return spp;
}

/* Compute a good size of a table covering the common period for all waves. */
static size_t tablesize( long rate, size_t count
,	struct syn123_wave *waves, size_t size_limit )
{
	size_t ts, nts;
	double fts, tolerance;
	double samples_per_period;
	size_t periods;

	samples_per_period = common_samples_per_period( rate, count
	,	waves, size_limit );
	tolerance = freq_error*samples_per_period;

	periods = 0;
	do
	{
		periods++;
		fts = periods*samples_per_period;
		ts  = round2size(fts);
		nts = round2size((periods+1)*samples_per_period);
	}
	while(myabs(fts-ts) > periods*tolerance && nts <= size_limit);

	/* Ensure size limit. Even it is ridiculous. */
	ts = smin(ts, size_limit);
	debug1("table size: %" SIZE_P, ts);
	return ts;
}

/* The wave functions. Argument is the phase normalised to the period. */
/* The argument is guaranteed to be 0 <= p < 1. */

/* _________ */
/*           */
static double wave_none(double p)
{
	return 0;
}

/*   __       */
/*  /  \      */
/*      \__/  */
static double wave_sine(double p)
{
	return sin(twopi*p);
}

/*      ___   */
/*  ___|      */
static double wave_square(double p)
{
	return (p < 0.5 ? -1 : 1);
}

/*    1234    Avoid jump from zero at beginning. */
/*    /\      */
/*      \/    */
static double wave_triangle(double p)
{
	return 4*p < 1
	?	4*p        /* 1 */
	:	( 4*p < 3
		?	2.-4*p  /* 2 and 3 */
		:	-4+4*p  /* 4 */
		);
}

/*   /|    Avoid jump from zero ... */
/*    |/   */
static double wave_sawtooth(double p)
{
	return 2*p < 1 ? 2*p : -2+2*p;
}

/*    _    */
/* __/ \__ */
/*         */
static double wave_gauss(double p)
{
	double v = p-0.5;
	return exp(-30*v*v);
}

/*    _      */
/*  _/ -___  */
/*           */
/* p**2*exp(-a*p**2) */
/* Scaling: maximum at sqrt(1/a), value 1/a*exp(-1). */
static double wave_pulse(double p)
{
	return p*p*exp(-50*p*p)/0.00735758882342885;
}

/*  _     */
/* / -___ */
/*        */
/* p**2*exp(-a*p) */
/* Scaling: maximum at 4/a, value 4/a**2*exp(-2). */
static double wave_shot(double p)
{
	return p*p*exp(-100*p)/5.41341132946451e-05;
}

#define WAVE_SWITCH(id) \
	switch(id) \
	{ \
		case SYN123_WAVE_NONE: \
			PI_LOOP( wave_none(PHASE) ) \
		break; \
		case SYN123_WAVE_SINE: \
			PI_LOOP( wave_sine(PHASE) ) \
		break; \
		case SYN123_WAVE_SQUARE: \
			PI_LOOP( wave_square(PHASE) ) \
		break; \
		case SYN123_WAVE_TRIANGLE: \
			PI_LOOP( wave_triangle(PHASE) ) \
		break; \
		case SYN123_WAVE_SAWTOOTH: \
			PI_LOOP( wave_sawtooth(PHASE) ) \
		break; \
		case SYN123_WAVE_GAUSS: \
			PI_LOOP( wave_gauss(PHASE) ) \
		break; \
		case SYN123_WAVE_PULSE: \
			PI_LOOP( wave_pulse(PHASE) ) \
		break; \
		case SYN123_WAVE_SHOT: \
			PI_LOOP( wave_shot(PHASE) ) \
		break; \
		default: \
			PI_LOOP( wave_none(PHASE) ) \
	}

static const char* wave_names[] =
{
	"null", "sine", "square", "triangle"
,	"sawtooth", "gauss", "pulse", "shot"
};

const char* attribute_align_arg
syn123_wave_name(int id)
{
	if(id < 0 || id >= sizeof(wave_names)/sizeof(char*))
		return "???";
	else
		return wave_names[id];
}

int attribute_align_arg
syn123_wave_id(const char *name)
{
	if(name)
		for(int i=0; i<sizeof(wave_names)/sizeof(char*); ++i)
			if(!strcmp(name, wave_names[i]))
				return i;
	return SYN123_WAVE_INVALID;
}

const char* syn123_strerror(int errcode)
{
	switch(errcode)
	{
		case SYN123_OK:
			return "no error";
		case SYN123_BAD_HANDLE:
			return "bad handle";
		case SYN123_BAD_FMT:
			return "bad format";
		case SYN123_BAD_ENC:
			return "bad encoding";
		case SYN123_BAD_CONV:
			return "unsupported conversion";
		case SYN123_BAD_SIZE:
			return "wrong buffer size";
		case SYN123_BAD_BUF:
			return "bad buffer pointer";
		case SYN123_BAD_CHOP:
			return "byte count not matching sample boundaries";
		case SYN123_DOOM:
			return "out of memory";
		case SYN123_WEIRD:
			return "Call the Ghostbusters!";
		default:
			return "unkown error";
	}
}

// Actual wave worker function, to be used to give up to
// bufblock samples in one go.
// With a good compiler, I suppose the two variants should
// perform identically. Not totally sure, though. And what is
// a good compiler?
#ifndef NO_VECTOR_FUN
// Give vectorization a chance.
// This precomputes a vector of phases for the option of applying
// vectorized functions directly.
// It _seems_ that compilers are smart enough to figure that out.
// Might drop the second workbuf.
static void add_some_wave( double outbuf[bufblock], int samples
,	enum syn123_wave_id id, double pps, double phase
,	double workbuf[bufblock] )
{
	#define PHASE workbuf[pi]
	for(int pi=0; pi<samples; ++pi)
		PHASE = phasefrac(pi*pps+phase);
	#define PI_LOOP( code ) \
		for(int pi=0; pi<samples; ++pi) \
			outbuf[pi] *= code;
	WAVE_SWITCH(id)
	#undef PI_LOOP
	#undef PHASE
}
#else
// A variant that does not specifically prepare for vectorization by
// precomputing phases, but still hardcodes the function calls.
static void add_some_wave( double outbuf[bufblock], int samples
,	enum syn123_wave_id id, double pps, double phase
,	double workbuf[bufblock] )
{
	#define PHASE phasefrac(pi*pps+phase)
	#define PI_LOOP( code ) \
		for(int pi=0; pi<samples; ++pi) \
			outbuf[pi] *= code;
	WAVE_SWITCH(id)
	#undef PI_LOOP
	#undef PHASE
}
#endif

// Fit waves into given table size.
static void wave_fit_table( size_t samples
, long rate, struct syn123_wave *wave )
{
	double pps = wave->freq/rate;
	debug3("wave_fit_table %" SIZE_P " %ld %g", samples, rate, wave->freq);
	size_t periods = smax(round2size(pps*samples), 1);
	pps = (double)periods/samples;
	wave->freq = pps*rate;
	debug4( "final wave: %c %i @ %g Hz + %g"
	,	wave->backwards ? '<' : '>', wave->id, wave->freq, wave->phase );
}

// Evaluate an additional wave into the given buffer (multiplying
// with existing values in output.
static void wave_add_buffer( double outbuf[bufblock], size_t samples
,	long rate, struct syn123_wave *wave, double workbuf[bufblock] )
{
	double pps = wave->freq/rate;
	debug3("wave_add_buffer %" SIZE_P " %ld %g", samples, rate, wave->freq);
	debug4( "adding wave: %c %i @ %g Hz + %g"
	,	wave->backwards ? '<' : '>', wave->id, wave->freq, wave->phase );
	if(wave->backwards)
		pps = -pps;
	add_some_wave( outbuf, samples
	,	wave->id, pps, wave->phase, workbuf );
	// Advance the wave.	
	wave->phase = phasefrac(wave->phase+samples*pps);
}

// The most basic generator of all.
static void silence_generator(syn123_handle *sh, int samples)
{
	for(int i=0; i<samples; ++i)
		sh->workbuf[1][i] = 0;
}

// Clear the handle of generator data structures.
// Well, except the one generating silence.
int attribute_align_arg
syn123_setup_silence(syn123_handle *sh)
{
	if(!sh)
		return SYN123_BAD_HANDLE;
	sh->generator = silence_generator;
	if(sh->wave_count && sh->waves)
		free(sh->waves);
	sh->waves = NULL;
	sh->wave_count = 0;
	if(sh->handle)
		free(sh->handle);
	sh->handle = NULL;
	sh->samples = 0;
	sh->offset = 0;
	return SYN123_OK;
}

// The generator function is called upon to fill sh->workbuf[1] with the
// given number of samples in double precision. It may use sh->workbuf[0]
// for its own purposes.
static void wave_generator(syn123_handle *sh, int samples)
{
	/* Initialise to zero amplitude. */
	for(int i=0; i<samples; ++i)
		sh->workbuf[1][i] = 1;
	/* Add individual waves. */
	for(int c=0; c<sh->wave_count; ++c)
		wave_add_buffer( sh->workbuf[1], samples, sh->fmt.rate, sh->waves+c
		,	sh->workbuf[0] );
	/* Amplification could have caused clipping. */
	for(int i=0; i<samples; ++i)
	{
		if(sh->workbuf[1][i] >  1.0)
			sh->workbuf[1][i] =  1.0;
		if(sh->workbuf[1][i] < -1.0)
			sh->workbuf[1][i] = -1.0;
		//debug2("sh->workbuf[1][%i]=%f", i, sh->workbuf[1][i]);
	}
}

/* Build internal table, allocate external table, convert to that one, */
/* adjusting sample storage format and channel count. */
int attribute_align_arg
syn123_setup_waves( syn123_handle *sh, size_t count
,	int *id, double *freq, double *phase, int *backwards
,	size_t *common_period )
{
	struct syn123_wave defwave = { SYN123_WAVE_SINE, FALSE, 440., 0. };
	int ret = SYN123_OK;

	if(!sh)
		return SYN123_BAD_HANDLE;
	syn123_setup_silence(sh);

	if(!count)
	{
		count = 1;
		id = NULL;
		freq = NULL;
		phase = NULL;
		backwards = NULL;
	}

	sh->waves = malloc(sizeof(struct syn123_wave)*count);
	if(!sh->waves)
		return SYN123_DOOM;
	for(size_t c=0; c<count; ++c)
	{
		sh->waves[c].id = id ? id[c] : defwave.id;
		sh->waves[c].backwards = backwards ? backwards[c] : defwave.backwards;
		sh->waves[c].freq = freq ? freq[c] : defwave.freq;
		sh->waves[c].phase = phase ? phase[c] : defwave.phase;
	}
	sh->wave_count = count;
	sh->generator = wave_generator;

	if(sh->maxbuf)
	{
		// 1. Determine buffer size to use.
		size_t samplesize = MPG123_SAMPLESIZE(sh->fmt.encoding);
		size_t size_limit = sh->maxbuf / samplesize;
		size_t buffer_samples = tablesize(sh->fmt.rate, count, sh->waves
		,	size_limit);
		// 2. Actually allocate the buffer.
		grow_buf(sh, buffer_samples*samplesize);
		if(sh->bufs/samplesize < buffer_samples)
		{
			ret = SYN123_DOOM;
			goto setup_wave_end;
		}
		// 2. Adjust the waves to fit into the buffer.
		for(size_t c=0; c<count; ++c)
		{
			wave_fit_table(buffer_samples, sh->fmt.rate, sh->waves+c);
			if(freq)
				freq[c] = sh->waves[c].freq;
		}
		// 3. fill the buffer using workbuf as intermediate. As long as
		// the buffer is not ready, we can just use syn123_read() to fill it.
		// Just need to ensure mono storage. Once sh->samples is set, the
		// buffer is used.
		int outchannels = sh->fmt.channels;
		sh->fmt.channels = 1;
		size_t buffer_bytes = syn123_read(sh, sh->buf, buffer_samples*samplesize);
		sh->fmt.channels = outchannels;
		// 4. Restore wave phases to the beginning, for tidyness.
		for(size_t c=0; c<count; ++c)
			sh->waves[c].phase = phase ? phase[c] : defwave.phase;
		// 5. Last check for sanity.
		if(buffer_bytes != buffer_samples*samplesize)
		{
			ret = SYN123_WEIRD;
			goto setup_wave_end;
		}
		sh->samples = buffer_samples;
	}

setup_wave_end:
	if(ret != SYN123_OK)
		syn123_setup_silence(sh);
	if(common_period)
		*common_period = sh->samples;
	return ret;
}

syn123_handle* attribute_align_arg
syn123_new(long rate, int channels, int encoding
,	size_t maxbuf, int *err)
{
	int myerr = SYN123_OK;
	syn123_handle *sh = NULL;
	size_t sbytes = MPG123_SAMPLESIZE(encoding);

	if(!sbytes)
	{
		myerr = SYN123_BAD_ENC;
		goto syn123_new_end;
	}
	if(rate < 1 || channels < 1)
	{
		myerr = SYN123_BAD_FMT;
		goto syn123_new_end;
	}

	sh = malloc(sizeof(syn123_handle));
	if(!sh){ myerr = SYN123_DOOM; goto syn123_new_end; }

	sh->fmt.rate = rate;
	sh->fmt.channels = channels;
	sh->fmt.encoding = encoding;
	sh->buf     = NULL;
	sh->bufs    = 0;
	sh->maxbuf  = maxbuf;
	sh->samples = 0;
	sh->offset  = 0;
	sh->wave_count = 0;
	sh->waves = NULL;
	sh->handle = NULL;
	syn123_setup_silence(sh);

syn123_new_end:
	if(err)
		*err = myerr;
	if(myerr)
	{
		syn123_del(sh);
		sh = NULL;
	}
	return sh;
}

void attribute_align_arg
syn123_del(syn123_handle* sh)
{
	if(!sh)
		return;
	syn123_setup_silence(sh);
	if(sh->buf)
		free(sh->buf);
	free(sh);
}

// Copy from period buffer or generate on the fly.
size_t attribute_align_arg
syn123_read( syn123_handle *sh, void *dest, size_t dest_bytes )
{
	char *cdest = dest; /* Want to do arithmetic. */
	size_t samplesize, framesize;
	size_t dest_samples;
	size_t extracted = 0;

	if(!sh)
		return 0;
	samplesize = MPG123_SAMPLESIZE(sh->fmt.encoding);
	framesize  = samplesize*sh->fmt.channels;
	dest_samples = dest_bytes/framesize;
	if(sh->samples) // Got buffered samples to work with.
	{
		while(dest_samples)
		{
			size_t block = smin(dest_samples, sh->samples - sh->offset);
			debug3( "offset: %"SIZE_P" block: %" SIZE_P" out of %"SIZE_P
			,	sh->offset, block, sh->samples );
			syn123_mono2many(cdest, (char*)sh->buf+sh->offset*samplesize
			,	sh->fmt.channels, samplesize, block );
			cdest  += framesize*block;
			sh->offset += block;
			sh->offset %= sh->samples;
			dest_samples -= block;
			extracted    += block;
		}
	}
	else // Compute directly, employing the work buffers.
	{
		while(dest_samples)
		{
			int block = (int)smin(dest_samples, bufblock);
			debug2( "out offset: %ld block: %i"
			,	(long)(cdest-(char*)dest)/framesize, block );
			// Compute data into workbuf[1], possibly using workbuf[0]
			// in the process.
			// TODO for the future: Compute only in single precision if
			// it is enough.
			sh->generator(sh, block);
			// Convert to external format, mono. We are abusing workbuf[0] here,
			// because it is big enough.
			int err = syn123_conv(
				sh->workbuf[0], sh->fmt.encoding, sizeof(sh->workbuf[0])
			,	sh->workbuf[1], MPG123_ENC_FLOAT_64, sizeof(double)*block
			,	NULL, NULL );
			if(err)
			{
				debug1("conv error: %i", err);
				break;
			}
//debug2("buf[1][0]=%f buf[0][0]=%f", sh->workbuf[1][0], *(float*)(&sh->workbuf[0][0]));
			syn123_mono2many( cdest, sh->workbuf[0]
			,	sh->fmt.channels, samplesize, block );
			cdest += framesize*block;
			dest_samples -= block;
			extracted += block;
		}
	}
	debug1("extracted: %" SIZE_P, extracted);
	return extracted*framesize;
}
