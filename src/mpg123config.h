/*
	mpg123config: basic config for mpg123 apps

	copyright 2025-2026 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis

	This includes config.h and enables 64 bit offsets where the system supports and
	reacts to the switch, as determined by our custom largefile detection.
	Include first, esp. before mpg123.h. Just use it instead of plain config.h.

	It also prepares use of mpg123 library headers as client code, where needed.
*/

#ifndef MPG123CONFIG_H
#define MPG123CONFIG_H

#include "config.h"

// This early settlement of _FILE_OFFSET_BITS is important to prevent
// subtle bugs where you could end up with 32 bit off_t but mappings
// to functions that expect 64 bit off_t.
#ifndef _FILE_OFFSET_BITS
#ifdef LFS_SENSITIVE
#ifdef LFS_LARGEFILE_64
#define _FILE_OFFSET_BITS 64
#endif
#endif
#endif

// Ensure that headers are switched to DLL linking mode on Windows.
#if defined(_WIN32) && defined(DYNAMIC_BUILD)
#define LINK_MPG123_DLL
#endif

#endif
