/*
	icy: support for SHOUTcast ICY meta info, an attempt to keep it organized

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Thomas Orgis and modelled after patch by Honza
*/

#include <sys/types.h>
#include "stringbuf.h"

struct icy_meta
{
	struct stringbuf name;
	struct stringbuf url;
	char* data;
	off_t interval;
	off_t next;
	int changed;
};

/* bah, just make it global... why bother with all that poiner passing to "methods" when there will be only one? */
extern struct icy_meta icy;

void init_icy();
void clear_icy();
