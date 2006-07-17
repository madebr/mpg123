/*
	buffer.h: output buffer

	copyright 1999-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Daniel Kobras / Oliver Fromme
*/

/*
 * Application specific interaction between main and buffer
 * process. This is much less generic than the functions in
 * xfermem so I chose to put it in buffer.[hc].
 * 01/28/99 [dk]
 */

#ifndef _MPG123_BUFFER_H_
#define _MPG123_BUFFER_H_

void buffer_ignore_lowmem(void);
void buffer_end(void);
void buffer_resync(void);
void buffer_reset(void);
void buffer_start(void);
void buffer_stop(void);

#endif
