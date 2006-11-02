#include <stdlib.h>
#include "config.h"
#include "debug.h"
#include "mpg123.h"
#include "common.h"
#include "stringbuf.h"
#include "genre.h"
#include "id3.h"

struct taginfo
{
	unsigned char version; /* 1, 2 */
	struct stringbuf title;
	struct stringbuf artist;
	struct stringbuf album;
	struct stringbuf year; /* be ready for 20570! */
	struct stringbuf comment;
	struct stringbuf genre;
};

struct taginfo id3;

/* UTF support definitions */

typedef int (*text_decoder)(char* dest, unsigned char* source, size_t len);

static int decode_il1(char* dest, unsigned char* source, size_t len);
static int decode_utf16(char* dest, unsigned char* source, size_t len, int str_be);
static int decode_utf16bom(char* dest, unsigned char* source, size_t len);
static int decode_utf16be(char* dest, unsigned char* source, size_t len);
static int decode_utf8(char* dest, unsigned char* source, size_t len);
int wide_bytelen(int width, char* string, size_t string_size);

static text_decoder text_decoders[4] =
{
	decode_il1,
	decode_utf16bom,
	decode_utf16be,
	decode_utf8
};

const int encoding_widths[4] = { 1, 2, 2, 1 };

/* the code starts here... */

void init_id3()
{
	id3.version = 0; /* nothing there */
	init_stringbuf(&id3.title);
	init_stringbuf(&id3.artist);
	init_stringbuf(&id3.album);
	init_stringbuf(&id3.year);
	init_stringbuf(&id3.comment);
	init_stringbuf(&id3.genre);
}

void exit_id3()
{
	free_stringbuf(&id3.title);
	free_stringbuf(&id3.artist);
	free_stringbuf(&id3.album);
	free_stringbuf(&id3.year);
	free_stringbuf(&id3.comment);
	free_stringbuf(&id3.genre);
}

void reset_id3()
{
	id3.version = 0;
	id3.title.fill = 0;
	id3.artist.fill = 0;
	id3.album.fill = 0;
	id3.year.fill = 0;
	id3.comment.fill = 0;
	id3.genre.fill = 0;
}

void store_id3_text(struct stringbuf* sb, char* source, size_t source_size)
{
	size_t pos = 1; /* skipping the encoding */
	int encoding;
	int bwidth;
	if(! source_size) return;
	encoding = source[0];
	debug1("encoding: %i\n", encoding);
	if(encoding > 3)
	{
		warning1("Unknown text encoding %d, assuming ISO8859-1 - I will probably screw a bit up!", encoding);
		encoding = 0;
	}
	bwidth = encoding_widths[encoding];
	if((source_size-1) % bwidth)
	{
		/* Uh. (BTW, the -1 is for the encoding byte.) */
		warning2("Weird tag size %d for encoding %d - I will probably trim too early or something but I think the MP3 is broken.", source_size, encoding);
		source_size -= (source_size-1) % bwidth;
	}
	/*
		first byte: Text encoding          $xx
		Text fields store a list of strings terminated by null, whatever that is for the encoding.
		That's not funny. Trying to work that by joining them into one string separated by line breaks...
		...and assume a series of \0 being the separator for any encoding
	*/
	while(pos < source_size)
	{
		size_t l = wide_bytelen(bwidth, source+pos, source_size-pos);
		debug2("wide bytelen of %lu: %lu", (unsigned long)(source_size-pos), (unsigned long)l);
		/* we need space for the stuff plus the closing zero */
		if((sb->size > sb->fill+l) || resize_stringbuf(sb, sb->fill+l+1))
		{
			/* append with line break - sb is in latin1 mode! */
			if(sb->fill) sb->p[sb->fill-1] = '\n';
			/* do not include the ending 0 in the conversion */
			sb->fill += text_decoders[encoding](sb->p+sb->fill, (unsigned char *) source+pos, l-(source_size==pos+l ? 0 : bwidth));
			sb->p[sb->fill++] = 0;
			/* advance to beginning of next string */
			pos += l;
		}
		else break;
	}
}

/*
	trying to parse ID3v2.3 and ID3v2.4 tags...

	returns:  0 = read-error
	         -1 = illegal ID3 header; maybe extended to mean unparseable (to new) header in future
	          1 = somehow ok...
*/
int parse_new_id3(unsigned long first4bytes, struct reader *rds)
{
	#define UNSYNC_FLAG 128
	#define EXTHEAD_FLAG 64
	#define EXP_FLAG 32
	#define FOOTER_FLAG 16
	#define UNKNOWN_FLAGS 15 /* 00001111*/
	unsigned char buf[6];
	unsigned long length=0;
	unsigned char flags = 0;
	int ret = 1;
	unsigned char* tagdata = NULL;
	unsigned char major = first4bytes & 0xff;
	debug1("ID3v2: major tag version: %i", major);
	if(major == 0xff) return -1;
	if(!rds->read_frame_body(rds,buf,6))       /* read more header information */
	return 0;

	if(buf[0] == 0xff) /* major version, will never be 0xff */
	return -1;
	/* second new byte are some nice flags, if these are invalid skip the whole thing */
	flags = buf[1];
	debug1("ID3v2: flags 0x%08x", flags);
	/* use 4 bytes from buf to construct 28bit uint value and return 1; return 0 if bytes are not syncsafe */
	#define syncsafe_to_long(buf,res) \
	( \
		(((buf)[0]|(buf)[1]|(buf)[2]|(buf)[3]) & 0x80) ? 0 : \
		(res =  (((unsigned long) (buf)[0]) << 27) \
		     | (((unsigned long) (buf)[1]) << 14) \
		     | (((unsigned long) (buf)[2]) << 7) \
		     |  ((unsigned long) (buf)[3]) \
		,1) \
	)
	/* length-10 or length-20 (footer present); 4 synchsafe integers == 28 bit number  */
	/* we have already read 10 bytes, so left are length or length+10 bytes belonging to tag */
	if(!syncsafe_to_long(buf+2,length)) return -1;
	debug1("ID3v2: tag data length %lu", length);
	if(param.verbose > 1) fprintf(stderr,"Note: ID3v2.%i rev %i tag of %lu bytes\n", major, buf[0], length);
	/* skip if unknown version/scary flags, parse otherwise */
	if((flags & UNKNOWN_FLAGS) || (major > 4))
	{
		/* going to skip because there are unknown flags set */
		warning2("ID3v2: Won't parse the ID3v2 tag with major version %u and flags 0x%xu - some extra code may be needed", major, flags);
		if(!rds->skip_bytes(rds,length)) /* will not store data in backbuff! */
		ret = 0;
	}
	else
	{
		id3.version = major;
		/* try to interpret that beast */
		if((tagdata = (unsigned char*) malloc(length+1)) != NULL)
		{
			debug("ID3v2: analysing frames...");
			if(rds->read_frame_body(rds,tagdata,length))
			{
				unsigned long tagpos = 0;
				debug1("ID3v2: have read at all %lu bytes for the tag now", (unsigned long)length+6);
				/* going to apply strlen for strings inside frames, make sure that it doesn't overflow! */
				tagdata[length] = 0;
				if(flags & EXTHEAD_FLAG)
				{
					debug("ID3v2: skipping extended header");
					if(!syncsafe_to_long(tagdata, tagpos)) ret = -1;
				}
				if(ret >= 0)
				{
					char id[5];
					unsigned long framesize;
					unsigned long fflags; /* need 16 bits, actually */
					id[4] = 0;
					/* pos now advanced after ext head, now a frame has to follow */
					while(tagpos < length-10) /* I want to read at least a full header */
					{
						int i = 0;
						unsigned long pos = tagpos;
						/* level 1,2,3 - 0 is info from lame/info tag! */
						/* rva tags with ascending significance, then general frames */
						#define KNOWN_FRAMES 8
						const char frame_type[KNOWN_FRAMES][5] = { "COMM", "TXXX", "RVA2", "TPE1", "TALB", "TIT2", "TYER", "TCON" };
						enum { egal = -1, comment, extra, rva2, artist, album, title, year, genre } tt = egal;
						/* we may have entered the padding zone or any other strangeness: check if we have valid frame id characters */
						for(; i< 4; ++i) if( !( ((tagdata[tagpos+i] > 47) && (tagdata[tagpos+i] < 58))
						                     || ((tagdata[tagpos+i] > 64) && (tagdata[tagpos+i] < 91)) ) )
						{
							debug5("ID3v2: real tag data apparently ended after %lu bytes with 0x%02x%02x%02x%02x", tagpos, tagdata[tagpos], tagdata[tagpos+1], tagdata[tagpos+2], tagdata[tagpos+3]);
							ret = -1;
							break;
						}
						if(ret >= 0)
						{
							/* 4 bytes id */
							strncpy(id, (char*) tagdata+pos, 4);
							pos += 4;
							/* size as 32 syncsafe bits */
							if(!syncsafe_to_long(tagdata+pos, framesize))
							{
								ret = -1;
								error("ID3v2: non-syncsafe frame size, aborting");
								break;
							}
							if(param.verbose > 2) fprintf(stderr, "Note: ID3v2 %s frame of size %lu\n", id, framesize);
							tagpos += 10 + framesize; /* the important advancement in whole tag */
							pos += 4;
							fflags = (((unsigned long) tagdata[pos]) << 8) | ((unsigned long) tagdata[pos+1]);
							pos += 2;
							/* for sanity, after full parsing tagpos should be == pos */
							/* debug4("ID3v2: found %s frame, size %lu (as bytes: 0x%08lx), flags 0x%016lx", id, framesize, framesize, fflags); */
							/* %0abc0000 %0h00kmnp */
							#define BAD_FFLAGS (unsigned long) 36784
							#define PRES_TAG_FFLAG 16384
							#define PRES_FILE_FFLAG 8192
							#define READ_ONLY_FFLAG 4096
							#define GROUP_FFLAG 64
							#define COMPR_FFLAG 8
							#define ENCR_FFLAG 4
							#define UNSYNC_FFLAG 2
							#define DATLEN_FFLAG 1
							/* shall not or want not handle these */
							if(fflags & (BAD_FFLAGS | COMPR_FFLAG | ENCR_FFLAG))
							{
								warning("ID3v2: skipping invalid/unsupported frame");
								continue;
							}
							
							for(i = 0; i < KNOWN_FRAMES; ++i)
							if(!strncmp(frame_type[i], id, 4)){ tt = i; break; }
							
							if(tt != egal)
							{
								int rva_mode = -1; /* mix / album */
								unsigned long realsize = framesize;
								unsigned char* realdata = tagdata+pos;
								if((flags & UNSYNC_FLAG) || (fflags & UNSYNC_FFLAG))
								{
									unsigned long ipos = 0;
									unsigned long opos = 0;
									debug("Id3v2: going to de-unsync the frame data");
									/* de-unsync: FF00 -> FF; real FF00 is simply represented as FF0000 ... */
									/* damn, that means I have to delete bytes from withing the data block... thus need temporal storage */
									/* standard mandates that de-unsync should always be safe if flag is set */
									realdata = (unsigned char*) malloc(framesize); /* will need <= bytes */
									if(realdata == NULL)
									{
										error("ID3v2: unable to allocate working buffer for de-unsync");
										continue;
									}
									/* now going byte per byte through the data... */
									realdata[0] = tagdata[pos];
									opos = 1;
									for(ipos = pos+1; ipos < pos+framesize; ++ipos)
									{
										if(!((tagdata[ipos] == 0) && (tagdata[ipos-1] == 0xff)))
										{
											realdata[opos++] = tagdata[ipos];
										}
									}
									realsize = opos;
									debug2("ID3v2: de-unsync made %lu out of %lu bytes", realsize, framesize);
								}
								pos = 0; /* now at the beginning again... */
								switch(tt)
								{
									case comment: /* a comment that perhaps is a RVA / RVA_ALBUM/AUDIOPHILE / RVA_MIX/RADIO one */
									{
										/* Text encoding          $xx */
										/* Language               $xx xx xx */
										/* policy about encodings: do not care for now here */
										/* if(realdata[0] == 0)  */
										{
											/* don't care about language */
											pos = 4;
											if(   !strcasecmp((char*)realdata+pos, "rva")
											   || !strcasecmp((char*)realdata+pos, "rva_mix")
											   || !strcasecmp((char*)realdata+pos, "rva_radio"))
											rva_mode = 0;
											else if(   !strcasecmp((char*)realdata+pos, "rva_album")
											        || !strcasecmp((char*)realdata+pos, "rva_audiophile")
											        || !strcasecmp((char*)realdata+pos, "rva_user"))
											rva_mode = 1;
											if((rva_mode > -1) && (rva_level[rva_mode] <= tt+1))
											{
												char* comstr;
												size_t comsize = realsize-4-(strlen((char*)realdata+pos)+1);
												if(param.verbose > 2) fprintf(stderr, "Note: evaluating %s data for RVA\n", realdata+pos);
												if((comstr = (char*) malloc(comsize+1)) != NULL)
												{
													memcpy(comstr,realdata+realsize-comsize, comsize);
													comstr[comsize] = 0;
													/* hm, what about utf16 here? */
													rva_gain[rva_mode] = atof(comstr);
													if(param.verbose > 2) fprintf(stderr, "Note: RVA value %fdB\n", rva_gain[rva_mode]);
													rva_peak[rva_mode] = 0;
													rva_level[rva_mode] = tt+1;
													free(comstr);
												}
												else error("could not allocate memory for rva comment interpretation");
											}
											else
											{
												if(!strcasecmp((char*)realdata+pos, ""))
												{
													/* only add general comments */
													realdata[pos] = realdata[pos-4]; /* the encoding field copied */
													store_id3_text(&id3.comment, (char*)realdata+pos, realsize-4);
												}
											}
										}
									}
									break;
									case extra: /* perhaps foobar2000's work */
									{
										/* Text encoding          $xx */
										/* unicode would hurt in string comparison... */
										if(realdata[0] == 0)
										{
											int is_peak = 0;
											pos = 1;
											
											if(!strncasecmp((char*)realdata+pos, "replaygain_track_",17))
											{
												debug("ID3v2: track gain/peak");
												rva_mode = 0;
												if(!strcasecmp((char*)realdata+pos, "replaygain_track_peak")) is_peak = 1;
												else if(strcasecmp((char*)realdata+pos, "replaygain_track_gain")) rva_mode = -1;
											}
											else
											if(!strncasecmp((char*)realdata+pos, "replaygain_album_",17))
											{
												debug("ID3v2: album gain/peak");
												rva_mode = 1;
												if(!strcasecmp((char*)realdata+pos, "replaygain_album_peak")) is_peak = 1;
												else if(strcasecmp((char*)realdata+pos, "replaygain_album_gain")) rva_mode = -1;
											}
											if((rva_mode > -1) && (rva_level[rva_mode] <= tt+1))
											{
												char* comstr;
												size_t comsize = realsize-1-(strlen((char*)realdata+pos)+1);
												if(param.verbose > 2) fprintf(stderr, "Note: evaluating %s data for RVA\n", realdata+pos);
												if((comstr = (char*) malloc(comsize+1)) != NULL)
												{
													memcpy(comstr,realdata+realsize-comsize, comsize);
													comstr[comsize] = 0;
													if(is_peak)
													{
														rva_peak[rva_mode] = atof(comstr);
														if(param.verbose > 2) fprintf(stderr, "Note: RVA peak %fdB\n", rva_peak[rva_mode]);
													}
													else
													{
														rva_gain[rva_mode] = atof(comstr);
														if(param.verbose > 2) fprintf(stderr, "Note: RVA gain %fdB\n", rva_gain[rva_mode]);
													}
													rva_level[rva_mode] = tt+1;
													free(comstr);
												}
												else error("could not allocate memory for rva comment interpretation");
											}
										}
									}
									break;
									case rva2: /* "the" RVA tag */
									{
										#ifdef HAVE_INTTYPES_H
										/* starts with null-terminated identification */
										if(param.verbose > 2) fprintf(stderr, "Note: RVA2 identification \"%s\"\n", realdata);
										/* default: some individual value, mix mode */
										rva_mode = 0;
										if( !strncasecmp((char*)realdata, "album", 5)
										    || !strncasecmp((char*)realdata, "audiophile", 10)
										    || !strncasecmp((char*)realdata, "user", 4))
										rva_mode = 1;
										if(rva_level[rva_mode] <= tt+1)
										{
											pos += strlen((char*) realdata) + 1;
											if(realdata[pos] == 1)
											{
												++pos;
												/* only handle master channel */
												debug("ID3v2: it is for the master channel");
												/* two bytes adjustment, one byte for bits representing peak - n bytes for peak */
												/* 16 bit signed integer = dB * 512 */
												/* we already assume short being 16 bit */
												rva_gain[rva_mode] = (float) ((((short) realdata[pos]) << 8) | ((short) realdata[pos+1])) / 512;
												pos += 2;
												if(param.verbose > 2) fprintf(stderr, "Note: RVA value %fdB\n", rva_gain[rva_mode]);
												/* heh, the peak value is represented by a number of bits - but in what manner? Skipping that part */
												rva_peak[rva_mode] = 0;
												rva_level[rva_mode] = tt+1;
											}
										}
										#else
										warning("ID3v2: Cannot parse RVA2 value because I don't have a guaranteed 16 bit signed integer type");
										#endif
									}
									break;
									/* non-rva metainfo, simply store... */
									case artist:
										debug("ID3v2: parsing artist info");
										store_id3_text(&id3.artist, (char*) realdata, realsize);
									break;
									case album:
										debug("ID3v2: parsing album info");
										store_id3_text(&id3.album, (char*) realdata, realsize);
									break;
									case title:
										debug("ID3v2: parsing title info");
										store_id3_text(&id3.title, (char*) realdata, realsize);
									break;
									case year:
										debug("ID3v2: parsing year info");
										store_id3_text(&id3.year, (char*) realdata, realsize);
									break;
									case genre:
										debug("ID3v2: parsing genre info");
										store_id3_text(&id3.genre, (char*) realdata, realsize);
									break;
									default: error1("ID3v2: unknown frame type %i", tt);
								}
								if((flags & UNSYNC_FLAG) || (fflags & UNSYNC_FFLAG)) free(realdata);
							}
							#undef BAD_FFLAGS
							#undef PRES_TAG_FFLAG
							#undef PRES_FILE_FFLAG
							#undef READ_ONLY_FFLAG
							#undef GROUP_FFLAG
							#undef COMPR_FFLAG
							#undef ENCR_FFLAG
							#undef UNSYNC_FFLAG
							#undef DATLEN_FFLAG
						}
						else break;
						#undef KNOWN_FRAMES
					}
				}
			}
			else
			{
				error("ID3v2: Duh, not able to read ID3v2 tag data.");
				ret = 0;
			}
			free(tagdata);
		}
		else
		{
			error1("ID3v2Arrg! Unable to allocate %lu bytes for interpreting ID3v2 data - trying to skip instead.", length);
			if(!rds->skip_bytes(rds,length)) /* will not store data in backbuff! */
			ret = 0;
		}
	}
	/* skip footer if present */
	if((flags & FOOTER_FLAG) && (!rds->skip_bytes(rds,length))) ret = 0;
	return ret;
	#undef UNSYNC_FLAG
	#undef EXTHEAD_FLAG
	#undef EXP_FLAG
	#undef FOOTER_FLAG
	#undef UNKOWN_FLAGS
}

void print_id3_tag(unsigned char *id3v1buf)
{
	if(!(id3.version || id3v1buf)) return;
	char genre_from_v1 = 0;
	if(id3v1buf != NULL)
	{
		/* fill gaps in id3v2 info with id3v1 info */
		struct id3tag {
			char tag[3];
			char title[30];
			char artist[30];
			char album[30];
			char year[4];
			char comment[30];
			unsigned char genre;
		};
		struct id3tag *tag = (struct id3tag *) id3v1buf;
		/* I _could_ skip the recalculation of fill ... */
		if(!id3.title.fill)
		{
			if(id3.title.size >= 31 || resize_stringbuf(&id3.title, 31))
			{
				strncpy(id3.title.p,tag->title,30);
				id3.title.p[30] = 0;
				id3.title.fill = strlen(id3.title.p) + 1;
			}
		}
		if(!id3.artist.fill)
		{
			if(id3.artist.size >= 31 || resize_stringbuf(&id3.artist,31))
			{
				strncpy(id3.artist.p,tag->artist,30);
				id3.artist.p[30] = 0;
				id3.artist.fill = strlen(id3.artist.p) + 1;
			}
		}
		if(!id3.album.fill)
		{
			if(id3.album.size >= 31 || resize_stringbuf(&id3.album,31))
			{
				strncpy(id3.album.p,tag->album,30);
				id3.album.p[30] = 0;
				id3.album.fill = strlen(id3.album.p) + 1;
			}
		}
		if(!id3.comment.fill)
		{
			if(id3.comment.size >= 31 || resize_stringbuf(&id3.comment,31))
			{
				strncpy(id3.comment.p,tag->comment,30);
				id3.comment.p[30] = 0;
				id3.comment.fill = strlen(id3.comment.p) + 1;
			}
		}
		if(!id3.year.fill)
		{
			if(id3.year.size >= 5 || resize_stringbuf(&id3.year,5))
			{
				strncpy(id3.year.p,tag->year,4);
				id3.year.p[4] = 0;
				id3.year.fill = strlen(id3.year.p) + 1;
			}
		}
		/*
			genre is special... tag->genre holds an index, id3v2 genre may contain indices in textual form and raw textual genres...
		*/
		if(!id3.genre.fill)
		{
			if(id3.genre.size >= 31 || resize_stringbuf(&id3.genre,31))
			{
				if (tag->genre <= genre_count)
				{
					strncpy(id3.genre.p, genre_table[tag->genre], 30);
				}
				else
				{
					strncpy(id3.genre.p,"Unknown",30);
				}
				id3.genre.p[30] = 0;
				id3.genre.fill = strlen(id3.genre.p) + 1;
				genre_from_v1 = 1;
			}
		}
	}
	
	if(id3.genre.fill && !genre_from_v1)
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
		struct stringbuf tmp;
		init_stringbuf(&tmp);
		debug1("interpreting genre: %s\n", id3.genre.p);
		if(copy_stringbuf(&id3.genre, &tmp))
		{
			size_t num = 0;
			size_t nonum = 0;
			size_t i;
			enum { nothing, number, outtahere } state = nothing;
			id3.genre.fill = 0; /* going to be refilled */
			/* number\n -> id3v1 genre */
			/* (number) -> id3v1 genre */
			/* (( -> ( */
			for(i = 0; i < tmp.fill; ++i)
			{
				debug1("i=%lu", (unsigned long) i);
				switch(state)
				{
					case nothing:
						nonum = i;
						if(tmp.p[i] == '(')
						{
							num = i+1; /* number starting as next? */
							state = number;
							debug1("( before number at %lu?", (unsigned long) num);
						}
						/* you know an encoding where this doesn't work? */
						else if(tmp.p[i] >= '0' && tmp.p[i] <= '9')
						{
							num = i;
							state = number;
							debug1("direct number at %lu", (unsigned long) num);
						}
						else state = outtahere;
					break;
					case number:
						/* fake number alert: (( -> ( */
						if(tmp.p[i] == '(')
						{
							nonum = i;
							state = outtahere;
							debug("no, it was ((");
						}
						else if(tmp.p[i] == ')' || tmp.p[i] == '\n' || tmp.p[i] == 0)
						{
							if(i-num > 0)
							{
								/* we really have a number */
								int gid;
								char* genre = "Unknown";
								tmp.p[i] = 0;
								gid = atoi(tmp.p+num);

								/* get that genre */
								if (gid >= 0 && gid <= genre_count) genre = genre_table[gid];
								debug1("found genre: %s", genre);

								if(id3.genre.fill) add_to_stringbuf(&id3.genre, ", ");
								add_to_stringbuf(&id3.genre, genre);
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
						else if(!(tmp.p[i] >= '0' && tmp.p[i] <= '9'))
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
			if(nonum < tmp.fill-1)
			{
				if(id3.genre.fill) add_to_stringbuf(&id3.genre, ", ");
				add_to_stringbuf(&id3.genre, tmp.p+nonum);
			}
		}
		free_stringbuf(&tmp);
	}

	if(param.long_id3)
	{
		fprintf(stderr,"\n");
		/* print id3v2 */
		/* dammed, I use pointers as bool again! It's so convenient... */
		fprintf(stderr,"\tTitle:   %s\n", id3.title.fill ? id3.title.p : "");
		fprintf(stderr,"\tArtist:  %s\n", id3.artist.fill ? id3.artist.p : "");
		fprintf(stderr,"\tAlbum:   %s\n", id3.album.fill ? id3.album.p : "");
		fprintf(stderr,"\tYear:    %s\n", id3.year.fill ? id3.year.p : "");
		fprintf(stderr,"\tGenre:   %s\n", id3.genre.fill ? id3.genre.p : "");
		fprintf(stderr,"\tComment: %s\n", id3.comment.fill ? id3.comment.p : "");
		fprintf(stderr,"\n");
	}
	else
	{
		/* We are trying to be smart here and conserve vertical space.
		   So we will skip tags not set, and try to show them in two parallel columns if they are short, which is by far the	most common case. */
		/* one _could_ circumvent the strlen calls... */
		if(id3.title.fill && id3.artist.fill && strlen(id3.title.p) <= 30 && strlen(id3.title.p) <= 30)
		{
			fprintf(stderr,"Title:   %-30s  Artist: %s\n",id3.title.p,id3.artist.p);
		}
		else
		{
			if(id3.title.fill) fprintf(stderr,"Title:   %s\n", id3.title.p);
			if(id3.artist.fill) fprintf(stderr,"Artist:  %s\n", id3.artist.p);
		}
		if (id3.comment.fill && id3.album.fill && strlen(id3.comment.p) <= 30 && strlen(id3.album.p) <= 30)
		{
			fprintf(stderr,"Comment: %-30s  Album:  %s\n",id3.comment.p,id3.album.p);
		}
		else
		{
			if (id3.comment.fill)
				fprintf(stderr,"Comment: %s\n", id3.comment.p);
			if (id3.album.fill)
				fprintf(stderr,"Album:   %s\n", id3.album.p);
		}
		if (id3.year.fill && id3.genre.fill && strlen(id3.year.p) <= 30 && strlen(id3.genre.p) <= 30)
		{
			fprintf(stderr,"Year:    %-30s  Genre:  %s\n",id3.year.p,id3.genre.p);
		}
		else
		{
			if (id3.year.fill)
				fprintf(stderr,"Year:    %s\n", id3.year.p);
			if (id3.genre.fill)
				fprintf(stderr,"Genre:   %s\n", id3.genre.p);
		}
	}
}

/*
	Preliminary UTF support routines

	Text decoder decodes the ID3 text content from whatever encoding to ISO-8859-1 or ASCII, substituting unconvertable characters with '*' and returning the final length of decoded string.
	TODO: iconv() to whatever locale. But we will want to keep this code anyway for systems w/o iconv(). But we currently assume that it is enough to allocate @len bytes in dest. That might not be true when converting to Unicode encodings.
*/

static int decode_il1(char* dest, unsigned char* source, size_t len)
{
	memcpy(dest, source, len);
	return len;
}

static int decode_utf16(char* dest, unsigned char* source, size_t len, int str_be)
{
	int spos = 0;
	int dlen = 0;

	len -= len % 2;
	/* Just ASCII, we take it easy. */
	for (; spos < len; spos += 2)
	{
		unsigned short word;
		if(str_be) word = source[spos] << 8 | source[spos+1];
		else word = source[spos] | source[spos+1] << 8;
		/* utf16 continuation byte */
		if(word & 0xdc00) continue;
		/* utf16 out-of-range codepoint */
		else if(word > 255) dest[dlen++] = '*';
		/* an old-school character */
		else dest[dlen++] = word; /* would a cast be good here? */
	}
	return dlen;
}

static int decode_utf16bom(char* dest, unsigned char* source, size_t len)
{
	if(len < 2) return 0;
	if(source[0] == 0xFF && source[1] == 0xFE) /* Little-endian */
	return decode_utf16(dest, source + 2, len - 2, 0);
	else /* Big-endian */
	return decode_utf16(dest, source + 2, len - 2, 1);
}

static int decode_utf16be(char* dest, unsigned char* source, size_t len)
{
	return decode_utf16(dest, source, len, 1);
}

static int decode_utf8(char* dest, unsigned char* source, size_t len)
{
	int spos = 0;
	int dlen = 0;
	/* Just ASCII, we take it easy. */
	for(; spos < len; spos++)
	{
		/* utf8 continuation byte bo, lead!*/
		if(source[spos] & 0xc0) continue;
		/* utf8 lead byte, no, cont! */
		else if(source[spos] & 0x80) dest[dlen++] = '*';
		else dest[dlen++] = source[spos];
	}
	return dlen;
}

/* determine byte length of string with characters wide @width;
   terminating 0 will be included, too, if there is any */
int wide_bytelen(int width, char* string, size_t string_size)
{
	size_t l = 0;
	while(l < string_size)
	{
		int b;
		for(b = 0; b < width; b++)
		if(string[l + b])
		break;

		l += width;
		if(b == width) /* terminating zero */
		return l;
	}
	return l;
}
