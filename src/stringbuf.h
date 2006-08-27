/*
	stringbuf: mimicking a bit of C++ to more safely handle strings

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Thomas Orgis
*/
#ifndef MPG123_STRINGBUF_H
#define MPG123_STRINGBUF_H

#include <string.h>

struct stringbuf
{
	char* p;
	size_t size;
	size_t fill;
};

void init_stringbuf(struct stringbuf* sb);
void free_stringbuf(struct stringbuf* sb);
/* returning 0 on error, 1 on success */
int resize_stringbuf(struct stringbuf* sb, size_t new);
int copy_stringbuf(struct stringbuf* from, struct stringbuf* to);
int add_to_stringbuf(struct stringbuf* sb, char* stuff);

#endif
