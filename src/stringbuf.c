/*
	stringbuf: mimicking a bit of C++ to more safely handle strings

	copyright 2006-7 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis
*/

#include "config.h"
#include "debug.h"
#include "stringbuf.h"
#include "mpg123.h" /* actually just for safe_realloc */

void init_stringbuf(struct stringbuf* sb)
{
	sb->p = NULL;
	sb->size = 0;
	sb->fill = 0;
}

void free_stringbuf(struct stringbuf* sb)
{
	if(sb->p != NULL)
	{
		free(sb->p);
		init_stringbuf(sb);
	}
}

int resize_stringbuf(struct stringbuf* sb, size_t new)
{
	debug3("resizing string pointer %p from %lu to %lu", (void*) sb->p, (unsigned long)sb->size, (unsigned long)new);
	if(sb->size != new)
	{
		char* t;
		debug("really!");
		t = (char*) safe_realloc(sb->p, new*sizeof(char));
		debug1("realloc returned %p", (void*) t); 
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

int copy_stringbuf(struct stringbuf* from, struct stringbuf* to)
{
	if(resize_stringbuf(to, from->fill))
	{
		memcpy(to->p, from->p, to->size);
		to->fill = to->size;
		return 1;
	}
	else return 0;
}

int add_to_stringbuf(struct stringbuf* sb, char* stuff)
{
	size_t addl = strlen(stuff)+1;
	debug1("adding %s", stuff);
	if(sb->fill)
	{
		if(sb->size >= sb->fill-1+addl || resize_stringbuf(sb, sb->fill-1+addl))
		{
			memcpy(sb->p+sb->fill-1, stuff, addl);
			sb->fill += addl-1;
		}
		else return 0;
	}
	else
	{
		if(resize_stringbuf(sb, addl))
		{
			memcpy(sb->p, stuff, addl);
			sb->fill = addl;
		}
		else return 0;
	}
	return 1;
}

int set_stringbuf(struct stringbuf* sb, char* stuff)
{
	sb->fill = 0;
	return add_to_stringbuf(sb, stuff);
}
