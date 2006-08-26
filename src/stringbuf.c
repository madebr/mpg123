/*
	stringbuf: mimicking a bit of C++ to more safely handle strings

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Thomas Orgis
*/

#include "config.h"
#include "stringbuf.h"
#include <stdlib.h>

void init_stringbuf(struct stringbuf* sb)
{
	sb->p = NULL;
	sb->size = 0;
}

void free_stringbuf(struct stringbuf* sb)
{
	if(sb->p != NULL)
	{
		free(sb->p);
		sb->p = NULL;
		sb->size = 0;
	}
}

int resize_stringbuf(struct stringbuf* sb, size_t new)
{
	if(sb->size != new)
	{
		char* t = (char*) realloc(sb->p, new*sizeof(char));
		if(t != NULL)
		{
			sb->p = t;
			sb->size = new;
			return 1;
		}
		else return 0;
	}
	else return 1; /* success */
}
