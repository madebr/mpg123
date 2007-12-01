/*
	id3dump: Print ID3 tags of files, scanned using libmpg123.

	copyright 2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis
*/

#include "mpg123.h"
#include <string.h>
/* mpg123.h does include these, but this example program shall demonstrate not to depend on that. */
#include "stdio.h"
#include "sys/types.h"

/* This looping code poses the question if one shouldn't store the tags in something loopable to begin with. */
#define V1FIELDS 6
#define V2FIELDS 6

void print_v1(mpg123_id3v1 *v1)
{
	int i;
	const char *names[] = { "TAG", "Title", "Artist", "Album", "Year", "Comment" };
	char *sources[sizeof(names)/sizeof(char*)];
	size_t sizes[sizeof(names)/sizeof(char*)];
	sources[0] = v1->tag;     sizes[0] = sizeof(v1->tag);
	sources[1] = v1->title;   sizes[1] = sizeof(v1->title);
	sources[2] = v1->artist;  sizes[2] = sizeof(v1->artist);
	sources[3] = v1->album;   sizes[3] = sizeof(v1->album);
	sources[4] = v1->year;    sizes[4] = sizeof(v1->year);
	sources[5] = v1->comment; sizes[5] = sizeof(v1->comment);
	for(i=1; i<V1FIELDS; ++i)
	{
		char safe[31];
		memcpy(safe, sources[i], sizes[i]);
		safe[sizes[i]] = 0;
		printf("%s: %s\n", names[i], safe);
	}
	printf("Genre: %i", v1->genre);
}

void print_lines(const char* prefix, mpg123_string *inlines)
{
	size_t i;
	int hadcr = 0, hadlf = 0;
	char *lines = NULL;
	char *line  = NULL;
	size_t len = 0;
	if(inlines != NULL && inlines->fill)
	{
		lines = inlines->p;
		len   = inlines->fill;
	}
	else return;

	line = lines;
	for(i=0; i<len; ++i)
	{
		#define HAD_CR 1
		#define HAD_LF 2
		if(lines[i] == '\n' || lines[i] == '\r' || lines[i] == 0)
		{
			if(lines[i] == '\n') ++hadlf;
			if(lines[i] == '\r') ++hadcr;

			if((hadcr || hadlf) && hadlf % 2 == 0 && hadcr % 2 == 0)
			{
				line = "";
			}

			lines[i] = 0;
			if(line)
			{
				printf("%s: %s\n", prefix, line);
				line = NULL;
			}
		}
		else
		{
			hadlf = hadcr = 0;
			if(line == NULL) line = lines+i;
		}
	}
}

void print_v2(mpg123_id3v2 *v2)
{
	int i;
	const char *names[] = { "Title", "Artist", "Album", "Year", "Comment", "Genre" };
	mpg123_string *sources[sizeof(names)/sizeof(char*)];
	sources[0] = &v2->title;
	sources[1] = &v2->artist;
	sources[2] = &v2->album;
	sources[3] = &v2->year;
	sources[4] =  v2->generic_comment;
	sources[5] = &v2->genre;
	for(i=0; i<V1FIELDS; ++i)
	{
		print_lines(names[i], sources[i]);
	}
}

int main(int argc, char **argv)
{
	int i;
	mpg123_handle* m;
	if(argc < 2)
	{
		fprintf(stderr, "\nI will print some ID3 tag fields of MPEG audio files.\n");
		fprintf(stderr, "\nUsage: %s <mpeg audio file list>\n\n", argv[0]);
		return -1;
	}
	mpg123_init();
	m = mpg123_new(NULL, NULL);
mpg123_param(m, MPG123_VERBOSE, 4, 0);
	
	for(i=1; i < argc; ++i)
	{
		mpg123_id3v1 *v1;
		mpg123_id3v2 *v2;
		int meta;
		if(mpg123_open(m, argv[i]) != MPG123_OK)
		{
			fprintf(stderr, "Cannot open %s: %s\n", argv[i], mpg123_strerror(m));
			continue;
		}
		mpg123_scan(m);
		meta = mpg123_meta_check(m);
		if(meta & MPG123_ID3 && mpg123_id3(m, &v1, &v2) == MPG123_OK)
		{
			printf("Tag data on %s:\n", argv[i]);
			printf("\n==== ID3v1 ====\n");
			if(v1 != NULL) print_v1(v1);

			printf("\n==== ID3v2 ====\n");
			if(v2 != NULL) print_v2(v2);
		}
		else printf("Nothing found for %s.\n", argv[i]);

		mpg123_close(m);
	}
	mpg123_delete(m);
	mpg123_exit();
	return 0;
}
