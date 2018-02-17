/*
	syn123_int: internal header for libsyn123

	copyright 2018 by the mpg123 project,
	licensed under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org

	initially written by Thomas Orgis
*/

#ifndef _MPG123_SYN123_INT_H_
#define _MPG123_SYN123_INT_H_

#include "config.h"
#include "intsym.h"
#include "compat.h"
#include "abi_align.h"
#include "syn123.h"

// Generally, a number of samples we work on in one go to
// allow the compiler to know our loops.
// An enum is the best integer constant you can define in plain C.
enum { bufblock = 512 };

struct syn123_wave
{
	enum syn123_wave_id id;
	int backwards; /* TRUE or FALSE */
	double freq; /* actual frequency */
	double phase; /* current phase */
};

struct syn123_struct
{
	// Temporary storage in internal precision.
	// This is a set of two to accomodate x and y=function(x, y).
	// Working in blocks reduces function call overhead and gives
	// chance of vectorization.
	// This may also be used as buffer for data with output encoding,
	// exploiting the fact that double is the biggest data type we
	// handle, also with the biggest alignment.
	double workbuf[2][bufblock];
	struct mpg123_fmt fmt;
	// Pointer to a generator function that writes a bit of samples
	// into workbuf[1], possibly using workbuf[0] internally.
	// Given count of sampls <= bufblock!
	void (*generator)(syn123_handle*, int);
	// Generator configuration.
	// SYN123_WAVES
	size_t wave_count;
	struct syn123_wave* waves;
	// Extraction of initially-computed waveform from buffer.
	void *buf;      // period buffer
	size_t bufs;    // allocated size of buffer in bytes
	size_t maxbuf;  // maximum period buffer size in bytes
	size_t samples; // samples (PCM frames) in period buffer
	size_t offset;  // offset in buffer for extraction helper
};

#endif
