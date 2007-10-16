/*
	stringbuf: mimicking a bit of C++ to more safely handle strings

	copyright 2006-7 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis
*/

#include "config.h"
#include "debug.h"
#include "mpg123.h"
#include "compat.h"
#include <stdlib.h>
#include <string.h>

void mpg123_init_string(mpg123_string* sb)
{
	sb->p = NULL;
	sb->size = 0;
	sb->fill = 0;
}

void mpg123_free_string(mpg123_string* sb)
{
	if(sb->p != NULL) free(sb->p);
	mpg123_init_string(sb);
}

int mpg123_resize_string(mpg123_string* sb, size_t new)
{
	debug3("resizing string pointer %p from %lu to %lu", (void*) sb->p, (unsigned long)sb->size, (unsigned long)new);
	if(new == 0)
	{
		if(sb->size && sb->p != NULL) free(sb->p);
		mpg123_init_string(sb);
		return 1;
	}
	if(sb->size != new)
	{
		char* t;
		debug("really!");
		t = (char*) safe_realloc(sb->p, new*sizeof(char));
		debug1("safe_realloc returned %p", (void*) t); 
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

int mpg123_copy_string(mpg123_string* from, mpg123_string* to)
{
	if(mpg123_resize_string(to, from->fill))
	{
		memcpy(to->p, from->p, to->size);
		to->fill = to->size;
		return 1;
	}
	else return 0;
}

int mpg123_add_string(mpg123_string* sb, char* stuff)
{
	size_t addl = strlen(stuff)+1;
	debug1("adding %s", stuff);
	if(sb->fill)
	{
		if(sb->size >= sb->fill-1+addl || mpg123_resize_string(sb, sb->fill-1+addl))
		{
			memcpy(sb->p+sb->fill-1, stuff, addl);
			sb->fill += addl-1;
		}
		else return 0;
	}
	else
	{
		if(mpg123_resize_string(sb, addl))
		{
			memcpy(sb->p, stuff, addl);
			sb->fill = addl;
		}
		else return 0;
	}
	return 1;
}

int mpg123_set_string(mpg123_string* sb, char* stuff)
{
	sb->fill = 0;
	return mpg123_add_string(sb, stuff);
}
