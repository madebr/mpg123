/*
	metaprint: display of meta data (including filtering of UTF8 to ASCII)

	copyright 2006-2020 by the mpg123 project
	free software under the terms of the LGPL 2.1

	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis
*/

#ifndef MPG123_METAPRINT_H
#define MPG123_METAPRINT_H

#include "mpg123.h"
#include <stdio.h>

void print_id3_tag(mpg123_handle *mh, int long_meta, FILE *out, int linelimit);
void print_icy(mpg123_handle *mh, FILE *out);

// Prepare a string that is safe and sensible to print to the console.
// Input: UTF-8 or 7-bit ASCII string
// Output: sanitized output string for UTF-8 or ASCII, control characters
//   dropped (including line breaks)
// Return value: An estimate of the printed string width. In an UTF-8
// locale, this should often be right, but there never is a guarantee
// with Unicode.
size_t outstr(mpg123_string *dest, mpg123_string *source);
// Wrapper around the above just for printing the string to some stream.
// Return value is directly from fprintf or also -1 if there was trouble
// processing the string.
int print_outstr(FILE *out, const char *str);

// Set to true if lyrics shall be printed.
extern int meta_show_lyrics;

#endif
