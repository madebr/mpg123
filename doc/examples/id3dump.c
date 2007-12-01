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

/* The "nice" printing functions need loop limits. */
#define V1FIELDS 6
#define V2FIELDS 6

void safe_print(char* name, char *data, size_t size)
{
	char safe[31];
	if(size>30) return;

	memcpy(safe, data, size);
	safe[size] = 0;
	printf("%s: %s\n", name, safe);
}

void print_v1(mpg123_id3v1 *v1)
{
	int i;
	safe_print("Title",   v1->title,   sizeof(v1->title));
	safe_print("Artist",  v1->artist,  sizeof(v1->artist));
	safe_print("Album",   v1->album,   sizeof(v1->album));
	safe_print("Year",    v1->year,    sizeof(v1->year));
	safe_print("Comment", v1->comment, sizeof(v1->comment));
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
	sources[0] = v2->title;
	sources[1] = v2->artist;
	sources[2] = v2->album;
	sources[3] = v2->year;
	sources[4] = v2->comment;
	sources[5] = v2->genre;
	printf("title = %p\n", (void*)v2->title);
	for(i=0; i<V1FIELDS; ++i)
	{
		print_lines(names[i], sources[i]);
	}
}

void print_raw_v2(mpg123_id3v2 *v2)
{
	size_t i;
	for(i=0; i<v2->texts; ++i)
	{
		char id[5];
		memcpy(id, v2->text[i].id, 4);
		id[4] = 0;
		printf("%p %s\n", (void*)(&v2->text[i].text), id);
		print_lines("", &v2->text[i].text);
	}
	for(i=0; i<v2->extras; ++i)
	{
		char id[5];
		memcpy(id, v2->extra[i].id, 4);
		id[4] = 0;
		printf( "%s description(%s)\n",
		        id,
		        v2->extra[i].description.fill ? v2->extra[i].description.p : "" );
		print_lines("", &v2->extra[i].text);
	}
	for(i=0; i<v2->comments; ++i)
	{
		char id[5];
		char lang[3];
		memcpy(id, v2->comment_list[i].id, 4);
		id[4] = 0;
		memcpy(lang, v2->comment_list[i].lang, 3);
		lang[3] = 0;
		printf( "%s description(%s) language(%s): \n",
		        id,
		        v2->comment_list[i].description.fill ? v2->comment_list[i].description.p : "",
		        lang );
		print_lines("", &v2->comment_list[i].text);
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
			printf("\n====      ID3v1       ====\n");
			if(v1 != NULL) print_v1(v1);

			printf("\n====      ID3v2       ====\n");
			if(v2 != NULL) print_v2(v2);

			printf("\n==== ID3v2 Raw frames ====\n");
			if(v2 != NULL) print_raw_v2(v2);
		}
		else printf("Nothing found for %s.\n", argv[i]);

		mpg123_close(m);
	}
	mpg123_delete(m);
	mpg123_exit();
	return 0;
}
