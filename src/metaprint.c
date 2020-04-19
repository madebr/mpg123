/*
	metaprint: display routines for ID3 tags (including filtering of UTF8 to ASCII)

	copyright 2006-2020 by the mpg123 project
	free software under the terms of the LGPL 2.1

	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis
*/

/* Need snprintf(). */
#define _DEFAULT_SOURCE
#define _BSD_SOURCE
// wchar stuff
#define _XOPEN_SOURCE 600
#define _POSIX_C_SOURCE 200112L
#include "common.h"
#include "genre.h"

#include "metaprint.h"

#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif
#ifdef HAVE_WCTYPE_H
#include <wctype.h>
#endif

#include "debug.h"

int meta_show_lyrics = 0;

static const char joker_symbol = '?';
static const char *uni_repl = "\xef\xbf\xbd";
const int uni_repl_len = 3;

/* Metadata name field texts with index enumeration. */
enum tagcode { TITLE=0, ARTIST, ALBUM, COMMENT, YEAR, GENRE, FIELDS };
static const char* name[FIELDS] =
{
	"Title"
,	"Artist"
,	"Album"
,	"Comment"
,	"Year"
,	"Genre"
};

/* Two-column printing: max length of left and right name strings.
   see print_id3 for what goes left or right.
   Choose namelen[0] >= namelen[1]! */
static const int namelen[2] = {7, 6};
/* Overhead is Name + ": " and also plus "  " for right column. */
/* pedantic C89 does not like:
const int overhead[2] = { namelen[0]+2, namelen[1]+4 }; */
static const int overhead[2] = { 9, 10 };

static void utf8_ascii_print(mpg123_string *dest, mpg123_string *source);
static void utf8_ascii(mpg123_string *dest, mpg123_string *source);

// If the given ID3 string is empty, possibly replace it with ID3v1 data.
static void id3_gap(mpg123_string *dest, int count, char *v1, size_t *len, int is_term)
{
	if(dest->fill)
		return;
	mpg123_string utf8tmp;
	mpg123_init_string(&utf8tmp);
	// First construct some UTF-8 from the id3v1 data, then run through
	// the same filter as everything else.
	*len = unknown2utf8(&utf8tmp, v1, count) == 0 ? utf8outstr(dest, &utf8tmp, is_term) : 0;
	mpg123_free_string(&utf8tmp);
}

/* Print one metadata entry on a line, aligning the beginning. */
static void print_oneline( FILE* out
,	const mpg123_string *tag, enum tagcode fi, int long_mode )
{
	char fmt[14]; /* "%s:%-XXXs%s\n" plus one null */
	if(!tag[fi].fill && !long_mode)
		return;

	if(long_mode)
		fprintf(out, "\t");
	snprintf( fmt, sizeof(fmt)-1, "%%s:%%-%ds%%s\n"
	,	1+namelen[0]-(int)strlen(name[fi]) );
	fprintf(out, fmt, name[fi], " ", tag[fi].fill ? tag[fi].p : "");
}

/*
	Print a pair of tag name-value pairs along each other in two columns or
	each on a line if that is not sensible.
	This takes a given length (in columns) into account, not just bytes.
	If that length would be computed taking grapheme clusters into account, things
	could be fine for the whole world of Unicode. So far we ride only on counting
	possibly multibyte characters (unless mpg123_strlen() got adapted meanwhile).
*/
static void print_pair
(
	FILE* out /* Output stream. */
,	const int *climit /* Maximum width of columns (two values). */
,	const mpg123_string *tag /* array of tag value strings */
,	const size_t *len /* array of character/column lengths */
,	enum tagcode f0, enum tagcode f1 /* field indices for column 0 and 1 */
){
	/* Two-column printout if things match, dumb printout otherwise. */
	if(  tag[f0].fill         && tag[f1].fill
	  && len[f0] <= (size_t)climit[0] && len[f1] <= (size_t)climit[1] )
	{
		char cfmt[35]; /* "%s:%-XXXs%-XXXs  %s:%-XXXs%-XXXs\n" plus one extra null from snprintf */
		int chardiff[2];
		size_t bytelen;

		/* difference between character length and byte length */
		bytelen = strlen(tag[f0].p);
		chardiff[0] = len[f0] < bytelen ? bytelen-len[f0] : 0;
		bytelen = strlen(tag[f1].p);
		chardiff[1] = len[f1] < bytelen ? bytelen-len[f1] : 0;

		/* Two-column format string with added padding for multibyte chars. */
		snprintf( cfmt, sizeof(cfmt)-1, "%%s:%%-%ds%%-%ds  %%s:%%-%ds%%-%ds\n"
		,	1+namelen[0]-(int)strlen(name[f0]), climit[0]+chardiff[0]
		,	1+namelen[1]-(int)strlen(name[f1]), climit[1]+chardiff[1] );
		/* Actual printout of name and value pairs. */
		fprintf(out, cfmt, name[f0], " ", tag[f0].p, name[f1], " ", tag[f1].p);
	}
	else
	{
		print_oneline(out, tag, f0, FALSE);
		print_oneline(out, tag, f1, FALSE);
	}
}

/* Print tags... limiting the UTF-8 to ASCII, if necessary. */
void print_id3_tag(mpg123_handle *mh, int long_id3, FILE *out, int linelimit)
{
	enum tagcode ti;
	mpg123_string tag[FIELDS];
	mpg123_string genretmp;
	size_t len[FIELDS];
	mpg123_id3v1 *v1;
	mpg123_id3v2 *v2;
	int is_term = term_width(fileno(out)) >= 0;
	if(!is_term)
		long_id3 = 1;
	/* no memory allocated here, so return is safe */
	for(ti=0; ti<FIELDS; ++ti){ len[ti]=0; mpg123_init_string(&tag[ti]); }
	/* extract the data */
	mpg123_id3(mh, &v1, &v2);
	{
		// Ignore v1 data for Frankenstein streams. It is just somewhere in between.
		long frank;
		if(mpg123_getstate(mh, MPG123_FRANKENSTEIN, &frank, NULL) == MPG123_OK && frank)
			v1 = NULL;
	}

	/* Only work if something there... */
	if(v1 == NULL && v2 == NULL) return;

	if(v2 != NULL) /* fill from ID3v2 data */
	{
		len[TITLE]   = utf8outstr(&tag[TITLE],   v2->title,   is_term);
		len[ARTIST]  = utf8outstr(&tag[ARTIST],  v2->artist,  is_term);
		len[ALBUM]   = utf8outstr(&tag[ALBUM],   v2->album,   is_term);
		len[COMMENT] = utf8outstr(&tag[COMMENT], v2->comment, is_term);
		len[YEAR]    = utf8outstr(&tag[YEAR],    v2->year,    is_term);
	}
	if(v1 != NULL) /* fill gaps with ID3v1 data */
	{
		/* I _could_ skip the recalculation of fill ... */
		id3_gap(&tag[TITLE],   30, v1->title,   &len[TITLE],   is_term);
		id3_gap(&tag[ARTIST],  30, v1->artist,  &len[ARTIST],  is_term);
		id3_gap(&tag[ALBUM],   30, v1->album,   &len[ALBUM],   is_term);
		id3_gap(&tag[COMMENT], 30, v1->comment, &len[COMMENT], is_term);
		id3_gap(&tag[YEAR],    4,  v1->year,    &len[YEAR],    is_term);
	}
	// Genre is special... v1->genre holds an index, id3v2 genre may contain
	// indices in textual form and raw textual genres...
	mpg123_init_string(&genretmp);
	if(v2 && v2->genre && v2->genre->fill)
	{
		/*
			id3v2.3 says (id)(id)blabla and in case you want ot have (blabla) write ((blabla)
			also, there is
			(RX) Remix
			(CR) Cover
			id3v2.4 says
			"one or several of the ID3v1 types as numerical strings"
			or define your own (write strings), RX and CR 

			Now I am very sure that I'll encounter hellishly mixed up id3v2 frames, so try to parse both at once.
		*/
		size_t num = 0;
		size_t nonum = 0;
		size_t i;
		enum { nothing, number, outtahere } state = nothing;
		/* number\n -> id3v1 genre */
		/* (number) -> id3v1 genre */
		/* (( -> ( */
		debug1("interpreting genre: %s\n", v2->genre->p);
		for(i = 0; i < v2->genre->fill; ++i)
		{
			debug1("i=%lu", (unsigned long) i);
			switch(state)
			{
				case nothing:
					nonum = i;
					if(v2->genre->p[i] == '(')
					{
						num = i+1; /* number starting as next? */
						state = number;
						debug1("( before number at %lu?", (unsigned long) num);
					}
					else if(v2->genre->p[i] >= '0' && v2->genre->p[i] <= '9')
					{
						num = i;
						state = number;
						debug1("direct number at %lu", (unsigned long) num);
					}
					else state = outtahere;
				break;
				case number:
					/* fake number alert: (( -> ( */
					if(v2->genre->p[i] == '(')
					{
						nonum = i;
						state = outtahere;
						debug("no, it was ((");
					}
					else if(v2->genre->p[i] == ')' || v2->genre->p[i] == '\n' || v2->genre->p[i] == 0)
					{
						if(i-num > 0)
						{
							/* we really have a number */
							int gid;
							char* genre = "Unknown";
							v2->genre->p[i] = 0;
							gid = atoi(v2->genre->p+num);

							/* get that genre */
							if(gid >= 0 && gid <= genre_count) genre = genre_table[gid];
							debug1("found genre: %s", genre);

							if(genretmp.fill) mpg123_add_string(&genretmp, ", ");
							mpg123_add_string(&genretmp, genre);
							nonum = i+1; /* next possible stuff */
							state = nothing;
							debug1("had a number: %i", gid);
						}
						else
						{
							/* wasn't a number, nonum is set */
							state = outtahere;
							debug("no (num) thing...");
						}
					}
					else if(!(v2->genre->p[i] >= '0' && v2->genre->p[i] <= '9'))
					{
						/* no number at last... */
						state = outtahere;
						debug("nothing numeric here");
					}
					else
					{
						debug("still number...");
					}
				break;
				default: break;
			}
			if(state == outtahere) break;
		}
		/* Small hack: Avoid repeating genre in case of stuff like
			(144)Thrash Metal being given. The simple cases. */
		if(
			nonum < v2->genre->fill-1 &&
			(!genretmp.fill || strncmp(genretmp.p, v2->genre->p+nonum, genretmp.fill))
		)
		{
			if(genretmp.fill) mpg123_add_string(&genretmp, ", ");
			mpg123_add_string(&genretmp, v2->genre->p+nonum);
		}
	}
	else if(v1)
	{
		// Fill from v1 tag.
		if(mpg123_resize_string(&genretmp, 31))
		{
			if(v1->genre <= genre_count)
				strncpy(genretmp.p, genre_table[v1->genre], 30);
			else
				strncpy(genretmp.p,"Unknown",30);
			genretmp.p[30] = 0;
			genretmp.fill = strlen(genretmp.p)+1;
		}
	}
	// Finally convert to safe output string and get display width.
	len[GENRE] = utf8outstr(&tag[GENRE], &genretmp, is_term);
	mpg123_free_string(&genretmp);

	if(long_id3)
	{
		fprintf(out,"\n");
		/* print id3v2 */
		print_oneline(out, tag, TITLE,   TRUE);
		print_oneline(out, tag, ARTIST,  TRUE);
		print_oneline(out, tag, ALBUM,   TRUE);
		print_oneline(out, tag, YEAR,    TRUE);
		print_oneline(out, tag, GENRE,   TRUE);
		print_oneline(out, tag, COMMENT, TRUE);
		fprintf(out,"\n");
	}
	else
	{
		/* We are trying to be smart here and conserve some vertical space.
		   So we will skip tags not set, and try to show them in two parallel
		   columns if they are short, which is by far the most common case. */
		int climit[2];

		/* Adapt formatting width to terminal if possible. */
		if(linelimit < 0)
			linelimit=overhead[0]+30+overhead[1]+30; /* the old style, based on ID3v1 */
		if(linelimit > 200)
			linelimit = 200; /* Not too wide. Also for format string safety. */
		/* Divide the space between the two columns, not wasting any. */
		climit[1] = linelimit/2-overhead[0];
		climit[0] = linelimit-linelimit/2-overhead[1];
		debug3("linelimits: %i  < %i | %i >", linelimit, climit[0], climit[1]);

		if(climit[0] <= 0 || climit[1] <= 0)
		{
			/* Ensure disabled column printing, no play with signedness in comparisons. */
			climit[0] = 0;
			climit[1] = 0;
		}
		fprintf(out,"\n"); /* Still use one separator line. Too ugly without. */
		print_pair(out, climit, tag, len, TITLE,   ARTIST);
		print_pair(out, climit, tag, len, COMMENT, ALBUM );
		print_pair(out, climit, tag, len, YEAR,    GENRE );
	}
	for(ti=0; ti<FIELDS; ++ti) mpg123_free_string(&tag[ti]);

	if(v2 != NULL && meta_show_lyrics)
	{
		/* find and print texts that have USLT IDs */
		size_t i;
		for(i=0; i<v2->texts; ++i)
		{
			if(!memcmp(v2->text[i].id, "USLT", 4))
			{
				/* split into lines, ensure usage of proper local line end */
				size_t a=0;
				size_t b=0;
				char lang[4]; /* just a 3-letter ASCII code, no fancy encoding */
				mpg123_string innline;
				mpg123_string outline;
				mpg123_string *uslt = &v2->text[i].text;

				memcpy(lang, &v2->text[i].lang, 3);
				lang[3] = 0;
				printf("Lyrics begin, language: %s; %s\n\n", lang,  v2->text[i].description.fill ? v2->text[i].description.p : "");

				mpg123_init_string(&innline);
				mpg123_init_string(&outline);
				while(a < uslt->fill)
				{
					b = a;
					while(b < uslt->fill && uslt->p[b] != '\n' && uslt->p[b] != '\r') ++b;
					/* Either found end of a line or end of the string (null byte) */
					mpg123_set_substring(&innline, uslt->p, a, b-a);
					utf8outstr(&outline, &innline, is_term);
					printf(" %s\n", outline.p);

					if(uslt->p[b] == uslt->fill) break; /* nothing more */

					/* Swallow CRLF */
					if(uslt->fill-b > 1 && uslt->p[b] == '\r' && uslt->p[b+1] == '\n') ++b;

					a = b + 1; /* next line beginning */
				}
				mpg123_free_string(&innline);
				mpg123_free_string(&outline);

				printf("\nLyrics end.\n");
			}
		}
	}
}

void print_icy(mpg123_handle *mh, FILE *outstream)
{
	int is_term = term_width(fileno(outstream)) >= 0;
	char* icy;
	if(MPG123_OK == mpg123_icy(mh, &icy))
	{
		mpg123_string in;
		mpg123_init_string(&in);
		if(mpg123_store_utf8(&in, mpg123_text_icy, (unsigned char*)icy, strlen(icy)+1))
		{
			mpg123_string out;
			mpg123_init_string(&out);

			utf8outstr(&out, &in, is_term);
			if(out.fill)
				fprintf(outstream, "\nICY-META: %s\n", out.p);

			mpg123_free_string(&out);
		}
		mpg123_free_string(&in);
	}
}

int unknown2utf8(mpg123_string *dest, const char *source, int len)
{
	if(!dest)
		return -1;
	dest->fill = 0;
	if(!source)
		return 0;
	size_t count = len < 0 ? strlen(source) : (size_t)len;		
	// Make a somewhat proper UTF-8 string out of this. Testing for valid
	// UTF-8 is futile. It will be some unspecified legacy 8-bit encoding.
	// I am keeping C0 chars, but replace everything above 7 bits with
	// the Unicode replacement character as most custom 8-bit encodings
	// placed some symbols into the C1 range, we just don't know which.
	size_t ulen = 0;
	for(size_t i=0; i<count; ++i)
	{
		unsigned char c = ((unsigned char*)source)[i];
		if(!c)
			break;
		ulen += c >= 0x80 ? uni_repl_len : 1;
	}
	++ulen; // trailing zero

	if(!mpg123_grow_string(dest, ulen))
		return -1;

	unsigned char *p = (unsigned char*)dest->p;
	for(size_t i=0; i<count; ++i)
	{
		unsigned char c = ((unsigned char*)source)[i];
		if(!c)
			break;
		if(c >= 0x80)
		{
			for(int r=0; r<uni_repl_len; ++r)
				*p++ = uni_repl[r];
		}
		else
			*p++ = c;
	}
	*p = 0;
	dest->fill = ulen;
	return 0;
}

static void ascii_space(unsigned char *c, int *wasspace)
{
	switch(*c)
	{
		case '\f':
		case '\r':
		case '\n':
		case '\t':
		case '\v':
			if(!*wasspace)
				*c = ' '; // Will be dropped by < 0x20 check otherwise.
			*wasspace = 1;
		break;
		default:
			*wasspace = 0;
	}
}

// Filter C1 control chars, using c2lead state.
#define ASCII_C1(c, append) \
	if(c2lead) \
	{ \
		if((c) >= 0x80 && (c) <= 0x9f) \
		{ \
			c2lead = 0; \
			continue; \
		} \
		else \
		{ \
			append; \
		} \
	} \
	c2lead = ((c) == 0xc2); \
	if(c2lead) \
		continue;

static void utf8_ascii_work(mpg123_string *dest, mpg123_string *source
,	int keep_nonprint)
{
	size_t spos = 0;
	size_t dlen = 0;
	unsigned char *p;

	// Find length of ASCII string (count non-continuation bytes).
	// Do _not_ change this to mpg123_strlen()!
	// It needs to match the loop below. 
	// No UTF-8 continuation byte 0x10??????, nor control char.
#define ASCII_PRINT_SOMETHING(c) \
	(((c) & 0xc0) != 0x80 && (keep_nonprint || ((c) != 0x7f && (c) >= 0x20)))
	int c2lead = 0;
	int wasspace = 0;
	for(spos=0; spos < source->fill; ++spos)
	{
		unsigned char c = ((unsigned char*)source->p)[spos];
		if(!keep_nonprint)
			ascii_space(&c, &wasspace);
		ASCII_C1(c, ++dlen);
		if(ASCII_PRINT_SOMETHING(c))
			++dlen;
	}
	++dlen; // trailing zero
	// Do nothing with nothing or if allocation fails. Neatly catches overflow
	// of ++dlen.
	if(!dlen || !mpg123_resize_string(dest, dlen))
	{
		mpg123_free_string(dest);
		return;
	}

	p = (unsigned char*)dest->p;
	c2lead = 0;
	wasspace = 0;
	for(spos=0; spos < source->fill; ++spos)
	{
		unsigned char c = ((unsigned char*)source->p)[spos];
		if(!keep_nonprint)
			ascii_space(&c, &wasspace);
		ASCII_C1(c, *p++ = joker_symbol)
		if(!ASCII_PRINT_SOMETHING(c))
			continue;
		else if(c & 0x80) // UTF-8 lead byte 0x11??????
			c = joker_symbol;
		*p++ = c;
	}
#undef ASCII_PRINT_SOMETHING
	// Always close the string, trailing zero might be missing.
	if(dest->size)
		dest->p[dest->size-1] = 0;
	dest->fill = dest->size;
}

// Reduce UTF-8 data to 7-bit ASCII, dropping non-printable characters.
// Non-printable ASCII == everything below 0x20 (space), including
// line breaks.
// Also: 0x7f (DEL) and the C1 chars. The C0 and C1 chars should just be
// dropped, not rendered. Or should they?
static void utf8_ascii_print(mpg123_string *dest, mpg123_string *source)
{
	utf8_ascii_work(dest, source, 0);
}

// Same as above, but keeping non-printable and control chars in the
// 7 bit realm.
static void utf8_ascii(mpg123_string *dest, mpg123_string *source)
{
	utf8_ascii_work(dest, source, 1);
}

size_t utf8outstr(mpg123_string *dest, mpg123_string *source, int to_terminal)
{
	if(dest)
		dest->fill = 0;
	if(!source || !dest || !source->fill) return 0;

	size_t width = 0;

	if(utf8env)
	{
#if defined(HAVE_MBSTOWCS) && defined(HAVE_WCSWIDTH) && \
    defined(HAVE_ISWPRINT) && defined(HAVE_WCSTOMBS)
		if(utf8loc && to_terminal)
		{
			// Best case scenario: Convert to wide string, filter,
			// compute printing width.
			size_t wcharlen = mbstowcs(NULL, source->p, 0);
			if(wcharlen == (size_t)-1)
				return 0;
			if(wcharlen+1 > SIZE_MAX/sizeof(wchar_t))
				return 0;
			wchar_t *pre = malloc(sizeof(wchar_t)*(wcharlen+1));
			wchar_t *flt = malloc(sizeof(wchar_t)*(wcharlen+1));
			if(!pre || !flt)
			{
				free(flt);
				free(pre);
				return 0;
			}
			if(mbstowcs(pre, source->p, wcharlen+1) == wcharlen)
			{
				size_t nwl = 0;
				int wasspace = 0;
				for(size_t i=0;  i<wcharlen; ++i)
				{
					// Turn any funky space sequence (including line breaks) into
					// one normal space.
					if(iswspace(pre[i]) && pre[i] != ' ')
					{
						if(!wasspace)
							flt[nwl++] = ' ';
						wasspace = 1;
					} else // Anything non-printing is skipped.
					{
						if(iswprint(pre[i]))
							flt[nwl++] = pre[i];
						wasspace = 0;
					}
				}
				flt[nwl] = 0;
				int columns = wcswidth(flt, nwl);
				size_t bytelen = wcstombs(NULL, flt, 0);
				if(
					columns >= 0 && bytelen != (size_t)-1
					&& mpg123_resize_string(dest, bytelen+1)
					&& wcstombs(dest->p, flt, dest->size) == bytelen
				){
					dest->fill = bytelen+1;
					width = columns;
				}
				else
					mpg123_free_string(dest);
			}
			free(flt);
			free(pre);
		}
		else
#endif
		if(to_terminal)
		{
			// Only filter C0 and C1 control characters.
			// That is, 0x01 to 0x19 (keeping 0x20, space) and 0x7f (DEL) to 0x9f.
			// Since the input and output is UTF-8, we'll keep that intact.
			// C1 is mapped to 0xc280 till 0xc29f.
			if(!mpg123_grow_string(dest, source->fill))
				return 0;
			dest->fill = 0;
			int c2lead = 0;
			int wasspace = 0;
			unsigned char *p = (unsigned char*)dest->p;
			for(size_t i=0; i<source->fill; ++i)
			{
				unsigned char c = ((unsigned char*)source->p)[i];
				ascii_space(&c, &wasspace);
				ASCII_C1(c, *p++ = 0xc2)
				if(c && c < 0x20)
					continue; // no C0 control chars, except space
				if(c == 0x7f)
					continue; // also no DEL
				*p++ = c;
				if(!c)
					break; // Up to zero is enough.
				// Assume each 7 bit char and each sequence start make one character.
				// So only continuation bytes need to be ignored.
				if((c & 0xc0) != 0x80)
					++width;
			}
			// Make damn sure that it ends.
			dest->fill = (char*)p - dest->p;
			dest->p[dest->fill-1] = 0;
		} else
		{
			if(!mpg123_grow_string(dest, source->fill))
				return 0;
			dest->fill = 0;
			unsigned char *p = (unsigned char*)dest->p;
			for(size_t i=0; i<source->fill; ++i)
			{
				unsigned char c = ((unsigned char*)source->p)[i];
				*p++ = c;
				if(!c)
					break; // Up to zero is enough.
				// Actual width should not matter that much for non-terminal,
				// as we should use less formatting in that case, but anyway.
				if((c & 0xc0) != 0x80)
					++width;
			}
			dest->fill = (char*)p - dest->p;
			dest->p[dest->fill-1] = 0;
		}
	} else if(to_terminal)
	{
		// Last resort: just printable 7-bit ASCII.
		utf8_ascii_print(dest, source);
		width = dest->fill ? dest->fill-1 : 0;
	} else
	{
		utf8_ascii(dest, source);
		// Width will be possibly very wrong.
		width = dest->fill ? dest->fill-1 : 0;
	}
	return width;
}

#undef ASCII_C1

// I tried saving some malloc using provided work buffers, but
// realized that the path of Unicode transformations is so full
// of them regardless.
// Can this include all the necessary logic?
// - If UTF-8 input: Use utf8outstr(), which includes terminal switch.
// - If not:
// -- If terminal: construct safe UTF-8, pass on to outstr().
// -- If not: assume env encoding, unprocessed string that came
//    from the environment.

int outstr(mpg123_string *dest, char *str, int is_utf8, int is_term)
{
	int ret = 0;
	if(dest)
		dest->fill = 0;
	if(!str || !dest)
		return -1;
	if(is_utf8 || utf8env)
	{
		// Just a structure around the input.
		mpg123_string src;
		mpg123_init_string(&src);
		src.p = str;
		src.size = src.fill = strlen(str)+1;
		size_t len = utf8outstr(dest, &src, is_term);
		if(src.fill > 1 && !len)
			ret = -1;
	} else if(is_term)
	{
		mpg123_string usrc;
		mpg123_init_string(&usrc);
		ret = unknown2utf8(&usrc, str, -1);
		if(!ret)
		{
			size_t len = utf8outstr(dest, &usrc, is_term);
			if(usrc.fill > 1 && !len)
				ret = -1;
		}
		mpg123_free_string(&usrc);
	} else
		ret = mpg123_set_string(dest, str) ? 0 : -1;
	if(ret)
		dest->fill = 0;
	return ret;
}

int print_outstr(FILE *out, char *str, int is_utf8, int is_term)
{
	int ret = 0;
	if(!str)
		return -1;
	mpg123_string outbuf;
	mpg123_init_string(&outbuf);
	ret = outstr(&outbuf, str, is_utf8, is_term);
	if(!ret)
		ret = fprintf(out, "%s", outbuf.fill ? outbuf.p : "");
	mpg123_free_string(&outbuf);
	return ret;
}
