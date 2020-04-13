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
// Modifer: Output is destined for a terminal and non-printing characters
// shall be filtered out. Otherwise, things are just adjusted for non-utf8
// locale.
// Return value: An estimate of the printed string width. In an UTF-8
// locale, this should often be right, but there never is a guarantee
// with Unicode.
size_t outstr(mpg123_string *dest, mpg123_string *source, int to_terminal);
// Take an unspecified (assumed ASCII-based) encoding and at least
// construct valid UTF-8 with 7-bit ASCII and replacement characters.
// This looses C1 control characters.
// If count is >= 0, it is used instead of strlen(source), enabling
// processing of data without closing zero byte.
int unknown2utf8(mpg123_string *dest, const char *source, int count);
// Wrapper around the above for printing the string to some stream.
// Return value is directly from fprintf or also -1 if there was trouble
// processing the string.
// If the output is a terminal as indicated by is_terminal, this applies
// all the filtering to avoid non-print characters, non-utf8 stuff is reduced
// to safe ASCII. If not printing to a terminal, only utf8 is reduced for
// non-utf8-locales. Non-utf8 strings are just passed through and no characters
// are filtered.
int print_outstr(FILE *out, char *str, int is_utf8, int is_terminal);

// Set to true if lyrics shall be printed.
extern int meta_show_lyrics;

#endif
