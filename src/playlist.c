/*
	playlist: playlist logic

	copyright 1995-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp, outsourced/reorganized by Thomas Orgis

	If we officially support Windows again, we should have this reworked to really cope with Windows paths, too.
*/

#include "mpg123.h"
#include "getlopt.h" /* for loptind */
#include "term.h" /* for term_restore */
#include "playlist.h"

#include <time.h>

/* increase linebuf in blocks of ... bytes */
#define LINEBUF_STEP 100

/* one global instance... add a pointer to this to every function definition and you have OO-style... */
playlist_struct pl;

/*
	functions to be called from outside and thus declared in the header:

	void prepare_playlist(int argc, char** argv);
	char *get_next_file(int argc, char **argv);
	void free_playlist();
*/

/* local functions */

int add_next_file (int argc, char *argv[]);
void shuffle_playlist();
void print_playlist();
void init_playlist();
int add_copy_to_playlist(char* new_entry);
int add_to_playlist(char* new_entry, char freeit);

/* used to be init_input */
void prepare_playlist(int argc, char** argv)
{
	/*
		fetch all playlist entries ... I don't consider playlists to be an endless stream.
		If you want to intentionally hang mpg123 on some other prog that may take infinite time to produce the full list (perhaps load tracks on demand), then just use the remote control and let that program print "load filename" instead of "filename".
		We may even provide a simple wrapper script that emulates the old playlist reading behaviour (for files and stdin, http playlists are actually a strong point on reading the list in _before_ starting playback since http connections don't last forever).
	*/
	init_playlist();
	while (add_next_file(argc, argv)) {}
	if(param.verbose > 1)
	{
		fprintf(stderr, "\nplaylist in normal order:\n");
		print_playlist();
		fprintf(stderr, "\n");
	}
	if(param.shuffle == 1) shuffle_playlist();
	/* Don't need these anymore, we have copies! */
	free_stringbuf(&pl.linebuf);
	free_stringbuf(&pl.dir);
}

char *get_next_file()
{
	char *newfile;

	if(pl.fill == 0) newfile = NULL;
	else
	/* normal order, just pick next thing */
	if(param.shuffle < 2)
	{
		if(pl.pos < pl.fill) newfile = pl.list[pl.pos].url;
		else newfile = NULL;
		++pl.pos;
	}
	/* randomly select files, with repeating */
	else newfile = pl.list[ (size_t) rand() % pl.fill ].url;

	return newfile;
}

/* It doesn't really matter on program exit, but anyway...
   Make sure you don't free() an item of argv! */
void free_playlist()
{
	if(pl.list != NULL)
	{
		debug("going to free() the playlist");
		while(pl.fill)
		{
			--pl.fill;
			debug1("free()ing entry %lu", (unsigned long)pl.fill);
			if(pl.list[pl.fill].freeit) free(pl.list[pl.fill].url);
		}
		free(pl.list);
		pl.list = NULL;
		pl.size = 0;
		debug("free()d the playlist");
	}
	free_stringbuf(&pl.linebuf);
	free_stringbuf(&pl.dir);
}

/* the constructor... */
void init_playlist()
{
	srand(time(NULL));
	pl.file = NULL;
	pl.entry = 0;
	pl.size = 0;
	pl.fill = 0;
	pl.pos = 0;
	pl.list = NULL;
	pl.alloc_step = 10;
	init_stringbuf(&pl.dir);
	init_stringbuf(&pl.linebuf);
	pl.type = UNKNOWN;
}

/*
	slightly modified find_next_file from mpg123.c
	now doesn't return the next entry but adds it to playlist struct
	returns 1 if it found something, 0 on end
*/
int add_next_file (int argc, char *argv[])
{
	int firstline = 0;

	/* hack for url that has been detected as track, not playlist */
	if(pl.type == NO_LIST) return 0;

	/* Get playlist dirname to append it to the files in playlist */
	if (param.listname)
	{
		char* slashpos;
		/* Oh, right... that doesn't look good for Windows... */
		if ((slashpos=strrchr(param.listname, '/')))
		{
			/* up to and including /, with space for \0 */
			if(resize_stringbuf(&pl.dir, 2 + slashpos - param.listname))
			{
				memcpy(pl.dir.p, param.listname, pl.dir.size-1);
				pl.dir.p[pl.dir.size-1] = 0;
			}
			else
			{
				error("cannot allocate memory for list directory!");
				pl.dir.size = 0;
			}
		}
	}

	if (param.listname || pl.file)
	{
		size_t line_offset = 0;
		if (!pl.file)
		{
			/* empty or "-" */
			if (!*param.listname || !strcmp(param.listname, "-"))
			{
				pl.file = stdin;
				param.listname = NULL;
				pl.entry = 0;
			}
			else if (!strncmp(param.listname, "http://", 7))
			{
				int fd;
				char *listmime = NULL;
				fd = http_open(param.listname, &listmime);
				debug1("listmime: %p", (void*) listmime);
				if(listmime != NULL)
				{
					debug1("listmime value: %s", listmime);
					if(!strcmp("audio/x-mpegurl", listmime))	pl.type = M3U;
					else if(!strcmp("audio/x-scpls", listmime) || !strcmp("application/pls", listmime))	pl.type = PLS;
					else
					{
						if(fd >= 0) close(fd);
						if(!strcmp("audio/mpeg", listmime) || !strcmp("audio/x-mpeg", listmime))
						{
							pl.type = NO_LIST;
							if(param.listentry < 0)
							{
								printf("#note you gave me a file url, no playlist, so...\n#entry 1\n%s\n", param.listname);
								return 0;
							}
							else
							{
								fprintf(stderr, "Note: MIME type indicates that this is no playlist but an mpeg audio file... reopening as such.\n");
								add_to_playlist(param.listname, 0);
								return 1;
							}
						}
						fprintf(stderr, "Error: unknown playlist MIME type %s; maybe "PACKAGE_NAME" can support it in future if you report this to the maintainer.\n", listmime);
						fd = -1;
					}
					free(listmime);
				}
				if(fd < 0)
				{
					param.listname = NULL;
					pl.file = NULL;
					fprintf(stderr, "Error: invalid playlist from http_open()!\n");
				}
				else
				{
					pl.entry = 0;
					pl.file = fdopen(fd,"r");
				}
			}
			else if (!(pl.file = fopen(param.listname, "rb")))
			{
				perror (param.listname);
				return 0;
			}
			else
			{
				debug("opened ordinary list file");
				pl.entry = 0;
			}
			if (param.verbose && pl.file) fprintf (stderr, "Using playlist from %s ...\n",	param.listname ? param.listname : "standard input");
			firstline = 1; /* just opened */
		}
		/* reading the file line by line */
		while (pl.file)
		{
			/*
				now read a string of arbitrary size...
				read a chunk, see if lineend, realloc, read another chunk
				
				fgets reads at most size-1 bytes and always appends the \0 
			*/
			size_t have = 0;
			do
			{
				/* have is the length of the string read, without the closing \0 */
				if(pl.linebuf.size <= have+1)
				{
					if(!resize_stringbuf(&pl.linebuf, pl.linebuf.size+LINEBUF_STEP))
					{
						error("cannot increase line buffer");
						break;
					}
				}
				/* I rely on fgets writing the \0 at the end! */
				if(fgets(pl.linebuf.p+have, pl.linebuf.size-have, pl.file))
				{
					have += strlen(pl.linebuf.p+have);
					debug2("have read %lu characters into linebuf: [%s]", (unsigned long)have, pl.linebuf.p);
				}
				else
				{
					debug("fgets failed to deliver something... file ended?");
					break;
				}
			} while(have && pl.linebuf.p[have-1] != '\r' && pl.linebuf.p[have-1] != '\n');
			if(have)
			{
				size_t i;
				pl.linebuf.p[strcspn(pl.linebuf.p, "\t\n\r")] = '\0';
				/* a bit of fuzzyness */
				if(firstline)
				{
					if(pl.type == UNKNOWN)
					{
						if(!strcmp("[playlist]", pl.linebuf.p))
						{
							fprintf(stderr, "Note: detected Shoutcast/Winamp PLS playlist\n");
							pl.type = PLS;
							continue;
						}
						else if
						(
							(!strncasecmp("#M3U", pl.linebuf.p ,4))
							||
							(!strncasecmp("#EXTM3U", pl.linebuf.p ,7))
							||
							(param.listname != NULL && (strrchr(param.listname, '.')) != NULL && !strcasecmp(".m3u", strrchr(param.listname, '.')))
						)
						{
							if(param.verbose) fprintf(stderr, "Note: detected M3U playlist type\n");
							pl.type = M3U;
						}
						else
						{
							if(param.verbose) fprintf(stderr, "Note: guessed M3U playlist type\n");
							pl.type = M3U;
						}
					}
					else
					{
						if(param.verbose)
						{
							fprintf(stderr, "Note: Interpreting as ");
							switch(pl.type)
							{
								case M3U: fprintf(stderr, "M3U"); break;
								case PLS: fprintf(stderr, "PLS (Winamp/Shoutcast)"); break;
								default: fprintf(stderr, "???");
							}
							fprintf(stderr, " playlist\n");
						}
					}
					firstline = 0;
				}
				#if !defined(WIN32)
				/* convert \ to / (from MS-like directory format) */
				for (i=0;pl.linebuf.p[i]!='\0';i++)
				{
					if (pl.linebuf.p[i] == '\\')	pl.linebuf.p[i] = '/';
				}
				#endif
				if (pl.linebuf.p[0]=='\0') continue; /* skip empty lines... */
				if (((pl.type == M3U) && (pl.linebuf.p[0]=='#')))
				{
					/* a comment line in m3u file */
					if(param.listentry < 0) printf("%s\n", pl.linebuf.p);
					continue;
				}

				/* real filename may start at an offset */
				line_offset = 0;
				/* extract path out of PLS */
				if(pl.type == PLS)
				{
					if(!strncasecmp("File", pl.linebuf.p, 4))
					{
						/* too lazy to really check for file number... would have to change logic to support unordered file entries anyway */
						char* in_line;
						if((in_line = strchr(pl.linebuf.p+4, '=')) != NULL)
						{
							/* FileN=? */
							if(in_line[1] != 0)
							{
								++in_line;
								line_offset = (size_t) (in_line-pl.linebuf.p);
							}
							else
							{
								fprintf(stderr, "Warning: Invalid PLS line (empty filename) - corrupt playlist file?\n");
								continue;
							}
						}
						else
						{
							fprintf(stderr, "Warning: Invalid PLS line (no '=' after 'File') - corrupt playlist file?\n");
							continue;
						}
					}
					else
					{
						if(param.listentry < 0) printf("#metainfo %s\n", pl.linebuf.p);
						continue;
					}
				}

				/* make paths absolute */
				/* Windows knows absolute paths with c: in front... should handle this if really supporting win32 again */
				if
				(
					(pl.dir.p != NULL)
					&& (pl.linebuf.p[line_offset]!='/')
					&& (pl.linebuf.p[line_offset]!='\\')
					&& strncmp(pl.linebuf.p+line_offset, "http://", 7)
				)
				{
					size_t need;
					need = pl.dir.size + strlen(pl.linebuf.p+line_offset);
					if(pl.linebuf.size < need)
					{
						if(!resize_stringbuf(&pl.linebuf, need))
						{
							error("unable to enlarge linebuf for appending path! skipping");
							continue;
						}
					}
					/* move to have the space at beginning */
					memmove(pl.linebuf.p+pl.dir.size-1, pl.linebuf.p+line_offset, strlen(pl.linebuf.p+line_offset)+1);
					/* prepend path */
					memcpy(pl.linebuf.p, pl.dir.p, pl.dir.size-1);
					line_offset = 0;
				}
				++pl.entry;
				if(param.listentry < 0) printf("#entry %lu\n%s\n", (unsigned long)pl.entry,pl.linebuf.p+line_offset);
				else if((param.listentry == 0) || (param.listentry == pl.entry))
				{
					add_copy_to_playlist(pl.linebuf.p+line_offset);
					return 1;
				}
			}
			else
			{
				if (param.listname)
				fclose (pl.file);
				param.listname = NULL;
				pl.file = NULL;
			}
		}
	}
	if(loptind < argc)
	{
		add_to_playlist(argv[loptind++], 0);
		return 1;
	}
	return 0;
}

void shuffle_playlist()
{
	size_t loop;
	size_t rannum;
	if(pl.fill >= 2)
	{
		for (loop = 0; loop < pl.fill; loop++)
		{
			struct listitem tmp;
			/*
				rand gives integer 0 <= RAND_MAX
				dividing this by (fill-1)*4 and taking the rest gives something 0 <= x < (fill-1)*4
				now diving x by 4 gives 0 <= y < fill-1
				then adding 1 if y >= loop index... makes 1 <= z <= fill-1
				if y not >= loop index that means... what?

				rannum = (rand() % (fill * 4 - 4)) / 4;
				rannum += (rannum >= loop);

				why not simply

				rannum = ( rand() % fill);
				
				That directly results in a random number in the allowed range. I'm using this now until someone convinces me of the numerical benefits of the other.
			*/
			rannum = (size_t) rand() % pl.fill;
			/*
				Small test on your binary operation skills (^ is XOR):
				a = b^(a^b)
				b = (a^b)^(b^(a^b))
				And, understood? ;-)
				
				pl.list[loop] ^= pl.list[rannum];
				pl.list[rannum] ^= pl.list[loop];
				pl.list[loop] ^= pl.list[rannum];
				
				But since this is not allowed with pointers and any speed gain questionable (well, saving _some_ memory...), doing it the lame way:
			*/
			tmp = pl.list[rannum];
			pl.list[rannum] = pl.list[loop];
			pl.list[loop] = tmp;
		}
	}

	if(param.verbose > 1)
	{
		/* print them */
		fprintf(stderr, "\nshuffled playlist:\n");
		print_playlist();
		fprintf(stderr, "\n");
	}
}

void print_playlist()
{
	size_t loop;
	for (loop = 0; loop < pl.fill; loop++)
	fprintf(stderr, "%s\n", pl.list[loop].url);
}

int add_copy_to_playlist(char* new_entry)
{
	char* cop;
	if((cop = (char*) malloc(strlen(new_entry)+1)) != NULL)
	{
		strcpy(cop, new_entry);
		return add_to_playlist(cop, 1);
	}
	else return 0;
}

/* add new entry to playlist - no string copy, just the pointer! */
int add_to_playlist(char* new_entry, char freeit)
{
	if(pl.fill == pl.size)
	{
		struct listitem* tmp = NULL;
		/* enlarge the list */
		tmp = (struct listitem*) realloc(pl.list, (pl.size + pl.alloc_step) * sizeof(struct listitem));
		if(!tmp)
		{
			error("unable to allocate more memory for playlist");
			perror("");
			return 0;
		}
		else
		{
			pl.list = tmp;
			pl.size += pl.alloc_step;
		}
	}
	/* paranoid */
	if(pl.fill < pl.size)
	{
		pl.list[pl.fill].freeit = freeit;
		pl.list[pl.fill].url = new_entry;
		++pl.fill;
	}
	else
	{
		error("playlist memory still too small?!");
		return 0;
	}
	return 1;
}

