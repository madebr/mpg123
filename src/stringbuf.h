/*
	stringbuf: mimicking a bit of C++ to more safely handle strings

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Thomas Orgis
*/
#ifndef MPG123_STRINGBUF_H
#define MPG123_STRINGBUF_H

#include <string.h>

typedef struct stringbuf
{
	char* p;
	size_t size;
} stringbuf;

void init_stringbuf(struct stringbuf* sb);
void free_stringbuf(struct stringbuf* sb);
int resize_stringbuf(struct stringbuf* sb, size_t new);

#endif
