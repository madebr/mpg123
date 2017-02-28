#ifndef MPG123_WAVES_H
#define MPG123_WAVES_H
/*
	waves: some oscillators, for fun

	copyright 2017 by the mpg123 project, license: LGPL 2.1

	This originates from Thomas' DerMixD, but the idea is probably generic
	enough: Construct a buffer containing a lookup table that covers a full
	period and serve that to the outside world.

	For added fun, this not only has a single oscillator, but combines any
	number of them (multiplicative).

	Storage format is interleaved PCM samples.
*/

#include "config.h"
#include "compat.h"
#include "fmt123.h"

struct wave_table
{
	void *buf; /* period buffer */
	struct mpg123_fmt fmt;
	size_t samples; /* samples (PCM frames) in period buffer */
	size_t offset;  /* offset in buffer for extraction helper */
	size_t count;   /* number of combined waves */
	double *freq;   /* actual wave frequency list */
};

extern const char *wave_pattern_list;

struct wave_table* wave_table_new(
	long rate, int channels, int encoding /* desired output format */
,	size_t count, long *freq   /* required: number and frequencies of waves */
,	const char** pattern       /* optional: wave pattern list */
,	double *phase /* optional: phase shift list  */
);

/* The destructor. Returns NULL, always. */
void* wave_table_del(struct wave_table* handle);

/* Extract the desired amount of samples (PCM frames). */
/* Returns said amount of samples, too, if successful. */
size_t wave_table_extract( struct wave_table *handle
,	void *dest, size_t samples );

#endif
