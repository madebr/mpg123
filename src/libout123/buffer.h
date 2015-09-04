/*
	buffer.h: output buffer

	copyright 1999-2015 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
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

#include "out123_int.h"
#include "compat.h"

int  buffer_init(audio_output_t *ao, size_t bytes);
void buffer_exit(audio_output_t *ao);

/* Messages with payload. */

int buffer_sync_param(audio_output_t *ao);
int buffer_open(audio_output_t *ao, const char* driver, const char* device);
int buffer_encodings(audio_output_t *ao);
int buffer_start(audio_output_t *ao);
void buffer_ndrain(audio_output_t *ao, size_t bytes);

/* Simple messages to be deal with after playback. */

void buffer_stop(audio_output_t *ao);
void buffer_close(audio_output_t *ao);
void buffer_continue(audio_output_t *ao);
/* Still undecided if that one is to be used anywhere. */
void buffer_ignore_lowmem(audio_output_t *ao);
void buffer_drain(audio_output_t *ao);
void buffer_end(audio_output_t *ao);

/* Simple messages with interruption of playback. */

void buffer_pause(audio_output_t *ao);
void buffer_drop(audio_output_t *ao);

/* The actual work: Hand over audio data. */
size_t buffer_write(audio_output_t *ao, void *buffer, size_t bytes);

/* Thin wrapper over xfermem giving the current buffer fill. */
size_t buffer_fill(audio_output_t *ao);

/* Special handler to safely read values from command channel with
   an additional buffer handed in. Exported for read_parameters(). */
int read_buf(int fd, void *addr, size_t size
,	byte *prebuf, int *preoff, int presize);

#endif
