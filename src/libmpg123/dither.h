/*
	dither: Generate noise for dithering / noise shaping.

	copyright 2009 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Taihei Monma
*/

#ifndef MPG123_DITHER_H
#define MPG123_DITHER_H

#define DITHERSIZE 65536
void dither_table_init(float *dithertable);

#endif
