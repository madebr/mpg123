/*
	playlist: playlist logic

	copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Michael Hipp, outsourced/reorganized by Thomas Orgis
*/

void prepare_playlist(int argc, char** argv);
char *get_next_file();
void free_playlist();
