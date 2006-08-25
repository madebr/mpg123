/*
	playlist: playlist logic

	copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Michael Hipp, outsourced/reorganized by Thomas Orgis
*/
#ifndef MPG123_PLAYLIST_H
#define MPG123_PLAYLIST_H

enum playlist_type { UNKNOWN = 0, M3U, PLS, NO_LIST };

typedef struct listitem
{
	char* url; /* the filename */
	char freeit; /* if it was allocated and should be free()d here */
} listitem;

typedef struct stringbuf
{
	char* p;
	size_t size;
} stringbuf;

typedef struct playlist_struct
{
	FILE* file; /* the current playlist stream */
	size_t entry; /* entry in the playlist file */
	size_t size;
	size_t fill;
	size_t pos;
	size_t alloc_step;
	struct listitem* list;
	struct stringbuf linebuf;
	struct stringbuf dir;
	enum playlist_type type;
} playlist_struct;

extern struct playlist_struct pl;

/* create playlist form argv including reading of playlist file */
void prepare_playlist(int argc, char** argv);
/* returns the next url to play or NULL when there is none left */
char *get_next_file();
/* frees memory that got allocated in prepare_playlist */
void free_playlist();

#endif
