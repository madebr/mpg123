/*
	out123.h: Wrapper header for MSVC++

	copyright 2017 by the mpg123 project - free software under the terms of the LGPL 2.1
	initially written by Patrick Dehne and Thomas Orgis.
*/
#ifndef OUT123_MSVC_H
#define OUT123_MSVC_H

#include <stdlib.h>
#include <stdint.h>
#include <io.h>

typedef __int64 intmax_t;
// ftell returns long, _ftelli64 returns __int64
// off_t is long, not __int64, use ftell
#define ftello ftell

// MSVC++doesn't have unistd.h, define STDOUT_FILENO
// using the filno function instead.
#define STDOUT_FILENO _fileno(stdout)

#define DEFAULT_OUTPUT_MODULE "win32_wasapi"

#define OUT123_NO_CONFIGURE
#include "out123.h.in" /* Yes, .h.in; we include the configure template! */

#endif
