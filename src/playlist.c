#include "config.h"
#include "mpg123.h"
#include "getlopt.h" /* for loptind */
#include "debug.h"
#include "term.h" /* for term_restore */
#include "playlist.h"

#include <time.h>

enum playlist_type { UNKNOWN = 0, M3U, PLS, NO_LIST };
enum playlist_type listtype = UNKNOWN;
static char *listnamedir = NULL;
static FILE *listfile = NULL;
static char line[1024];
static long entry;
static int firstline = 1;

typedef struct listitem
{
	char* url; /* the filename */
	char freeit; /* if it was allocated and should be free()d here */
} listitem;

typedef struct playlist_struct
{
	size_t size;
	size_t fill;
	size_t pos;
	size_t alloc_step;
	struct listitem* list;
} playlist_struct;

/* one file-global instance... add a pointer to this to every function definition and you have OO-style... */
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
		debug("going free() the playlist");
		while(pl.fill)
		{
			--pl.fill;
			debug1("free()ing entry %zu", pl.fill);
			if(pl.list[pl.fill].freeit) free(pl.list[pl.fill].url);
		}
		free(pl.list);
		debug("free()d the playlist");
	}
}

/* the constructor... */
void init_playlist()
{
	srand(time(NULL));
	pl.size = 0;
	pl.fill = 0;
	pl.pos = 0;
	pl.list = NULL;
	pl.alloc_step = 10;
}

/*
	slightly modified find_next_file from mpg123.c
	now doesn't return the next entry but adds it to playlist struct
	returns 1 if it found something, 0 on end
*/
int add_next_file (int argc, char *argv[])
{
	/* static... */
	char* in_line = NULL;
	char linetmp [1024];
	char * slashpos;
	int i;

	/* hack for url that has been detected as track, not playlist */
	if(listtype == NO_LIST) return 0;

	/* Get playlist dirname to append it to the files in playlist */
	if (param.listname)
	{
		if ((slashpos=strrchr(param.listname, '/')))
		{
			/* memory gets lost here! */
			listnamedir=strdup (param.listname);
			listnamedir[1 + slashpos - param.listname] = 0;
		}
	}

	if (param.listname || listfile)
	{
		if (!listfile)
		{
			if (!*param.listname || !strcmp(param.listname, "-"))
			{
				listfile = stdin;
				param.listname = NULL;
				entry = 0;
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
					if(!strcmp("audio/x-mpegurl", listmime))	listtype = M3U;
					else if(!strcmp("audio/x-scpls", listmime))	listtype = PLS;
					else
					{
						if(fd >= 0) close(fd);
						if(!strcmp("audio/mpeg", listmime))
						{
							listtype = NO_LIST;
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
					listfile = NULL;
					fprintf(stderr, "Error: invalid playlist from http_open()!\n");
				}
				else
				{
					entry = 0;
					listfile = fdopen(fd,"r");
				}
			}
			else if (!(listfile = fopen(param.listname, "rb")))
			{
				perror (param.listname);
				#ifdef HAVE_TERMIOS
				if(param.term_ctrl)
				term_restore();
				#endif
				exit (1);
			}
			else
			{
				debug("opened ordinary list file");
				entry = 0;
			}
			if (param.verbose && listfile) fprintf (stderr, "Using playlist from %s ...\n",	param.listname ? param.listname : "standard input");
		}
		debug1("going to get busy with listfile ... listentry=%li\n", param.listentry);
		/* read the whole listfile in */
		while (listfile)
		{
			if (fgets(line, 1023, listfile))
			{
				line[strcspn(line, "\t\n\r")] = '\0';
				/* a bit of fuzzyness */
				if(firstline)
				{
					if(listtype == UNKNOWN)
					{
						if(!strcmp("[playlist]", line))
						{
							fprintf(stderr, "Note: detected Shoutcast/Winamp PLS playlist\n");
							listtype = PLS;
							continue;
						}
						else if
						(
							(!strncasecmp("#M3U", line ,4))
							||
							(!strncasecmp("#EXTM3U", line ,7))
							||
							(param.listname != NULL && (strrchr(param.listname, '.')) != NULL && !strcasecmp(".m3u", strrchr(param.listname, '.')))
						)
						{
							fprintf(stderr, "Note: detected M3U playlist type\n");
							listtype = M3U;
						}
						else
						{
							fprintf(stderr, "Note: guessed M3U playlist type\n");
							listtype = M3U;
						}
					}
					else
					{
						fprintf(stderr, "Note: Interpreting as ");
						switch(listtype)
						{
							case M3U: fprintf(stderr, "M3U"); break;
							case PLS: fprintf(stderr, "PLS (Winamp/Shoutcast)"); break;
							default: fprintf(stderr, "???");
						}
						fprintf(stderr, " playlist\n");
					}
					firstline = 0;
				}
				#if !defined(WIN32)
				/* convert \ to / (from MS-like directory format) */
				for (i=0;line[i]!='\0';i++)
				if (line [i] == '\\')
				line [i] = '/';
				#endif
				if (line[0]=='\0') continue;
				if (((listtype == M3U) && (line[0]=='#')))
				{
					if(param.listentry < 0) printf("%s\n", line);
					continue;
				}

				in_line = line;
				/* extract path out of PLS */
				if(listtype == PLS)
				{
					if(!strncasecmp("File", line, 4))
					{
						/* too lazy to reall check for file number... would have to change logic to support unordered file entries anyway */
						if((in_line = strchr(line+4, '=')) != NULL)
						{
							if(in_line[0] != 0) ++in_line;
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
						if(param.listentry < 0) printf("#metainfo %s\n", line);
						continue;
					}
				}

				/* make paths absolute */
				/* Windows knows absolute paths with c: in front... should handle this if really supporting win32 again */
				if ((listnamedir) && (in_line[0]!='/') && (in_line[0]!='\\')
					 && strncmp(in_line, "http://", 7))
				{
					/* prepend path */
					memset(linetmp,'\0',sizeof(linetmp));
					snprintf(linetmp, sizeof(linetmp)-1, "%s%s",
					listnamedir, in_line);
					strcpy (in_line, linetmp);
				}
				++entry;
				if(param.listentry < 0) printf("#entry %li\n%s\n", entry,in_line);
				else if((param.listentry == 0) || (param.listentry == entry))
				{
					add_copy_to_playlist(in_line);
					return 1;
				}
			}
			else
			{
				if (param.listname)
				fclose (listfile);
				param.listname = NULL;
				listfile = NULL;
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
