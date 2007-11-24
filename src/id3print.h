/*
	id3print: display of ID3 tags (including filtering of UTF8 to ASCII)

	copyright 2006-2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis
*/

#ifndef MPG123_ID3PRINT_H
#define MPG123_ID3PRINT_H

#include "mpg123.h"

void print_id3_tag(mpg123_handle *mh, int long_id3, FILE *out);

#endif
