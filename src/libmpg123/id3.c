/*
	id3: ID3v2.3 and ID3v2.4 parsing (a relevant subset)

	copyright 2006-2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis
*/

#include "mpg123lib_intern.h"
#include "id3.h"

/* UTF support definitions */

typedef void (*text_converter)(mpg123_string *sb, unsigned char* source, size_t len);

static void convert_latin1  (mpg123_string *sb, unsigned char* source, size_t len);
static void convert_utf16   (mpg123_string *sb, unsigned char* source, size_t len, int str_be);
static void convert_utf16bom(mpg123_string *sb, unsigned char* source, size_t len);
static void convert_utf16be (mpg123_string *sb, unsigned char* source, size_t len);
static void convert_utf8    (mpg123_string *sb, unsigned char* source, size_t len);

static const text_converter text_converters[4] = 
{
	convert_latin1,
	convert_utf16bom,
	convert_utf16be,
	convert_utf8
};

const int encoding_widths[4] = { 1, 2, 2, 1 };

/* the code starts here... */

void init_id3(mpg123_handle *fr)
{
	fr->id3v2.version = 0; /* nothing there */
	mpg123_init_string(&fr->id3v2.title);
	mpg123_init_string(&fr->id3v2.artist);
	mpg123_init_string(&fr->id3v2.album);
	mpg123_init_string(&fr->id3v2.year);
	mpg123_init_string(&fr->id3v2.comment);
	mpg123_init_string(&fr->id3v2.genre);
}

void exit_id3(mpg123_handle *fr)
{
	mpg123_free_string(&fr->id3v2.title);
	mpg123_free_string(&fr->id3v2.artist);
	mpg123_free_string(&fr->id3v2.album);
	mpg123_free_string(&fr->id3v2.year);
	mpg123_free_string(&fr->id3v2.comment);
	mpg123_free_string(&fr->id3v2.genre);
}

void reset_id3(mpg123_handle *fr)
{
	fr->id3v2.version = 0;
	fr->id3v2.title.fill = 0;
	fr->id3v2.artist.fill = 0;
	fr->id3v2.album.fill = 0;
	fr->id3v2.year.fill = 0;
	fr->id3v2.comment.fill = 0;
	fr->id3v2.genre.fill = 0;
}

/*
	Store any text in UTF8 encoding; preserve the zero string separator (I don't need strlen for the total size).
	ID3v2 standard says that there should be one text frame of specific type per tag, and subsequent tags overwrite old values.
	So, I always replace the text that may be stored already (perhaps with a list of zero-separated strings, though).
*/
void store_id3_text(mpg123_string *sb, char *source, size_t source_size)
{
	int encoding;
	int bwidth;
	if(!source_size)
	{
		debug("Empty id3 data!");
		return;
	}
	encoding = source[0];
	++source;
	--source_size;
	debug1("encoding: %i", encoding);
	/* A note: ID3v2.3 uses UCS-2 non-variable 16bit encoding, v2.4 uses UTF16.
	   UTF-16 uses a reserved/private range in UCS-2 to add the magic, so we just always treat it as UTF. */
	if(encoding > 3)
	{
		warning1("Unknown text encoding %d, assuming ISO8859-1 - I will probably screw a bit up!", encoding);
		encoding = 0;
	}
	bwidth = encoding_widths[encoding];
	/* Hack! I've seen a stray zero byte before BOM. Is that supposed to happen? */
	while(source_size > bwidth && source[0] == 0)
	{
		--source_size;
		++source;
		debug("skipped leading zero");
	}
	if(source_size % bwidth)
	{
		/* When we need two bytes for a character, it's strange to have an uneven bytestream length. */
		warning2("Weird tag size %d for encoding %d - I will probably trim too early or something but I think the MP3 is broken.", (int)source_size, encoding);
		source_size -= source_size % bwidth;
	}
	text_converters[encoding](sb, (unsigned char*)source, source_size);
	if(sb->size) debug1("UTF-8 string (the first one): %s", sb->p);
	else error("unable to convert string to UTF-8 (out of memory, junk input?)!");
}

/*
	trying to parse ID3v2.3 and ID3v2.4 tags...

	returns:  0 = read-error... or so... soft issue... ok... somehow...
	         ... = illegal ID3 header; maybe extended to mean unparseable (to new) header in future
	          1 = somehow ok...
	         ...or READER_MORE...
*/
int parse_new_id3(mpg123_handle *fr, unsigned long first4bytes)
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
	int ret2;
	unsigned char* tagdata = NULL;
	unsigned char major = first4bytes & 0xff;
	debug1("ID3v2: major tag version: %i", major);
	if(major == 0xff) return 0; /* used to be -1 */
	if((ret2 = fr->rd->read_frame_body(fr, buf, 6)) < 0) /* read more header information */
	return ret2;

	if(buf[0] == 0xff) /* major version, will never be 0xff */
	return 0; /* used to be -1 */
	/* second new byte are some nice flags, if these are invalid skip the whole thing */
	flags = buf[1];
	debug1("ID3v2: flags 0x%08x", flags);
	/* use 4 bytes from buf to construct 28bit uint value and return 1; return 0 if bytes are not synchsafe */
	#define synchsafe_to_long(buf,res) \
	( \
		(((buf)[0]|(buf)[1]|(buf)[2]|(buf)[3]) & 0x80) ? 0 : \
		(res =  (((unsigned long) (buf)[0]) << 21) \
		     | (((unsigned long) (buf)[1]) << 14) \
		     | (((unsigned long) (buf)[2]) << 7) \
		     |  ((unsigned long) (buf)[3]) \
		,1) \
	)
	/* id3v2.3 does not store synchsafe frame sizes, but synchsafe tag size - doh! */
	#define bytes_to_long(buf,res) \
	( \
		major == 3 ? \
		(res =  (((unsigned long) (buf)[0]) << 24) \
		     | (((unsigned long) (buf)[1]) << 16) \
		     | (((unsigned long) (buf)[2]) << 8) \
		     |  ((unsigned long) (buf)[3]) \
		,1) : synchsafe_to_long(buf,res) \
	)
	/* length-10 or length-20 (footer present); 4 synchsafe integers == 28 bit number  */
	/* we have already read 10 bytes, so left are length or length+10 bytes belonging to tag */
	if(!synchsafe_to_long(buf+2,length)) return -1;
	debug1("ID3v2: tag data length %lu", length);
	if(VERBOSE2) fprintf(stderr,"Note: ID3v2.%i rev %i tag of %lu bytes\n", major, buf[0], length);
	/* skip if unknown version/scary flags, parse otherwise */
	if((flags & UNKNOWN_FLAGS) || (major > 4) || (major < 3))
	{
		/* going to skip because there are unknown flags set */
		warning2("ID3v2: Won't parse the ID3v2 tag with major version %u and flags 0x%xu - some extra code may be needed", major, flags);
		if((ret2 = fr->rd->skip_bytes(fr,length)) < 0) /* will not store data in backbuff! */
		ret = ret2;
	}
	else
	{
		fr->id3v2.version = major;
		/* try to interpret that beast */
		if((tagdata = (unsigned char*) malloc(length+1)) != NULL)
		{
			debug("ID3v2: analysing frames...");
			if((ret2 = fr->rd->read_frame_body(fr,tagdata,length)) > 0)
			{
				unsigned long tagpos = 0;
				debug1("ID3v2: have read at all %lu bytes for the tag now", (unsigned long)length+6);
				/* going to apply strlen for strings inside frames, make sure that it doesn't overflow! */
				tagdata[length] = 0;
				if(flags & EXTHEAD_FLAG)
				{
					debug("ID3v2: skipping extended header");
					if(!bytes_to_long(tagdata, tagpos)) ret = -1;
				}
				if(ret > 0)
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
							ret = 0; /* used to be -1 */
							break;
						}
						if(ret > 0)
						{
							/* 4 bytes id */
							strncpy(id, (char*) tagdata+pos, 4);
							pos += 4;
							/* size as 32 bits */
							if(!bytes_to_long(tagdata+pos, framesize))
							{
								ret = -1;
								error1("ID3v2: non-syncsafe size of %s frame, skipping the remainder of tag", id);
								break;
							}
							if(VERBOSE3) fprintf(stderr, "Note: ID3v2 %s frame of size %lu\n", id, framesize);
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
									case comment: /* a comment that perhaps is a RVA / fr->rva.ALBUM/AUDIOPHILE / fr->rva.MIX/RADIO one */
									{
										/* Text encoding          $xx */
										/* Language               $xx xx xx */
										/* policy about encodings: do not care for now here */
										/* if(realdata[0] == 0)  */
										{
											/* don't care about language */
											pos = 4;
											if(   !strcasecmp((char*)realdata+pos, "rva")
											   || !strcasecmp((char*)realdata+pos, "fr->rva.mix")
											   || !strcasecmp((char*)realdata+pos, "fr->rva.radio"))
											rva_mode = 0;
											else if(   !strcasecmp((char*)realdata+pos, "fr->rva.album")
											        || !strcasecmp((char*)realdata+pos, "fr->rva.audiophile")
											        || !strcasecmp((char*)realdata+pos, "fr->rva.user"))
											rva_mode = 1;
											if((rva_mode > -1) && (fr->rva.level[rva_mode] <= tt+1))
											{
												char* comstr;
												size_t comsize = realsize-4-(strlen((char*)realdata+pos)+1);
												if(VERBOSE3) fprintf(stderr, "Note: evaluating %s data for RVA\n", realdata+pos);
												if((comstr = (char*) malloc(comsize+1)) != NULL)
												{
													memcpy(comstr,realdata+realsize-comsize, comsize);
													comstr[comsize] = 0;
													/* hm, what about utf16 here? */
													fr->rva.gain[rva_mode] = atof(comstr);
													if(VERBOSE3) fprintf(stderr, "Note: RVA value %fdB\n", fr->rva.gain[rva_mode]);
													fr->rva.peak[rva_mode] = 0;
													fr->rva.level[rva_mode] = tt+1;
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
													debug("storing a comment");
													store_id3_text(&fr->id3v2.comment, (char*)realdata+pos, realsize-4);
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
											if((rva_mode > -1) && (fr->rva.level[rva_mode] <= tt+1))
											{
												char* comstr;
												size_t comsize = realsize-1-(strlen((char*)realdata+pos)+1);
												if(VERBOSE3) fprintf(stderr, "Note: evaluating %s data for RVA\n", realdata+pos);
												if((comstr = (char*) malloc(comsize+1)) != NULL)
												{
													memcpy(comstr,realdata+realsize-comsize, comsize);
													comstr[comsize] = 0;
													if(is_peak)
													{
														fr->rva.peak[rva_mode] = atof(comstr);
														if(VERBOSE3) fprintf(stderr, "Note: RVA peak %fdB\n", fr->rva.peak[rva_mode]);
													}
													else
													{
														fr->rva.gain[rva_mode] = atof(comstr);
														if(VERBOSE3) fprintf(stderr, "Note: RVA gain %fdB\n", fr->rva.gain[rva_mode]);
													}
													fr->rva.level[rva_mode] = tt+1;
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
										if(VERBOSE3) fprintf(stderr, "Note: RVA2 identification \"%s\"\n", realdata);
										/* default: some individual value, mix mode */
										rva_mode = 0;
										if( !strncasecmp((char*)realdata, "album", 5)
										    || !strncasecmp((char*)realdata, "audiophile", 10)
										    || !strncasecmp((char*)realdata, "user", 4))
										rva_mode = 1;
										if(fr->rva.level[rva_mode] <= tt+1)
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
												fr->rva.gain[rva_mode] = (float) ((((short) realdata[pos]) << 8) | ((short) realdata[pos+1])) / 512;
												pos += 2;
												if(VERBOSE3) fprintf(stderr, "Note: RVA value %fdB\n", fr->rva.gain[rva_mode]);
												/* heh, the peak value is represented by a number of bits - but in what manner? Skipping that part */
												fr->rva.peak[rva_mode] = 0;
												fr->rva.level[rva_mode] = tt+1;
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
										store_id3_text(&fr->id3v2.artist, (char*) realdata, realsize);
									break;
									case album:
										debug("ID3v2: parsing album info");
										store_id3_text(&fr->id3v2.album, (char*) realdata, realsize);
									break;
									case title:
										debug("ID3v2: parsing title info");
										store_id3_text(&fr->id3v2.title, (char*) realdata, realsize);
									break;
									case year:
										debug("ID3v2: parsing year info");
										store_id3_text(&fr->id3v2.year, (char*) realdata, realsize);
									break;
									case genre:
										debug("ID3v2: parsing genre info");
										store_id3_text(&fr->id3v2.genre, (char*) realdata, realsize);
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
				ret = ret2;
			}
			free(tagdata);
		}
		else
		{
			error1("ID3v2Arrg! Unable to allocate %lu bytes for interpreting ID3v2 data - trying to skip instead.", length);
			if((ret2 = fr->rd->skip_bytes(fr,length)) < 0) ret = ret2; /* will not store data in backbuff! */
			else ret = 0;
		}
	}
	/* skip footer if present */
	if((ret > 0) && (flags & FOOTER_FLAG) && ((ret2 = fr->rd->skip_bytes(fr,length)) < 0)) ret = ret2;
	return ret;
	#undef UNSYNC_FLAG
	#undef EXTHEAD_FLAG
	#undef EXP_FLAG
	#undef FOOTER_FLAG
	#undef UNKOWN_FLAGS
}

static void convert_latin1(mpg123_string *sb, unsigned char* s, size_t l)
{
	size_t length = l;
	size_t i;
	unsigned char *p;
	/* determine real length, a latin1 character can at most take 2  in UTF8 */
	for(i=0; i<l; ++i)
	if(s[i] >= 0x80) ++length;

	debug1("UTF-8 length: %lu", (unsigned long)length);
	/* one extra zero byte for paranoia */
	if(!mpg123_resize_string(sb, length+1)){ mpg123_free_string(sb); return ; }

	p = (unsigned char*) sb->p; /* Signedness doesn't matter but it shows I thought about the non-issue */
	for(i=0; i<l; ++i)
	if(s[i] < 0x80){ *p = s[i]; ++p; }
	else /* two-byte encoding */
	{
		*p     = 0xc0 | (s[i]>>6);
		*(p+1) = 0x80 | (s[i] & 0x3f);
		p+=2;
	}

	sb->p[length] = 0;
	sb->fill = length+1;
}

#define FULLPOINT(f,s) ( (((f)&0x3ff)<<10) + ((s)&0x3ff) + 0x10000 )
/* Remember: There's a limit at 0x1ffff. */
#define UTF8LEN(x) ( (x)<0x80 ? 1 : ((x)<0x800 ? 2 : ((x)<0x10000 ? 3 : 4)))
static void convert_utf16(mpg123_string *sb, unsigned char* s, size_t l, int str_be)
{
	size_t i;
	unsigned char *p;
	size_t length = 0; /* the resulting UTF-8 length */
	/* Determine real length... extreme case can be more than utf-16 length. */
	size_t high = 0;
	size_t low  = 1;
	debug1("convert_utf16 with length %lu", (unsigned long)l);
	if(!str_be) /* little-endian */
	{
		high = 1; /* The second byte is the high byte. */
		low  = 0; /* The first byte is the low byte. */
	}
	/* first: get length, check for errors -- stop at first one */
	for(i=0; i < l-1; i+=2)
	{
		unsigned long point = ((unsigned long) s[i+high]<<8) + s[i+low];
		if((point & 0xd800) == 0xd800) /* lead surrogate */
		{
			unsigned short second = (i+3 < l) ? (s[i+2+high]<<8) + s[i+2+low] : 0;
			if((second & 0xdc00) == 0xdc00) /* good... */
			{
				point = FULLPOINT(point,second);
				length += UTF8LEN(point); /* possibly 4 bytes */
				i+=2; /* We overstepped one word. */
			}
			else /* if no valid pair, break here */
			{
				debug1("Invalid UTF16 surrogate pair at %li.", (unsigned long)i);
				l = i; /* Forget the half pair, END! */
				break;
			}
		}
		else length += UTF8LEN(point); /* 1,2 or 3 bytes */
	}

	if(l < 1){ mpg123_set_string(sb, ""); return; }

	if(!mpg123_resize_string(sb, length+1)){ mpg123_free_string(sb); return ; }

	/* Now really convert, skip checks as these have been done just before. */
	p = (unsigned char*) sb->p; /* Signedness doesn't matter but it shows I thought about the non-issue */
	for(i=0; i < l-1; i+=2)
	{
		unsigned long codepoint = ((unsigned long) s[i+high]<<8) + s[i+low];
		if((codepoint & 0xd800) == 0xd800) /* lead surrogate */
		{
			unsigned short second = (s[i+2+high]<<8) + s[i+2+low];
			codepoint = FULLPOINT(codepoint,second);
			i+=2; /* We overstepped one word. */
		}
		if(codepoint < 0x80) *p++ = (unsigned char) codepoint;
		else if(codepoint < 0x800)
		{
			*p++ = 0xc0 | (codepoint>>6);
			*p++ = 0x80 | (codepoint & 0x3f);
		}
		else if(codepoint < 0x10000)
		{
			*p++ = 0xe0 | (codepoint>>12);
			*p++ = 0x80 | ((codepoint>>6) & 0x3f);
			*p++ = 0x80 | (codepoint & 0x3f);
		}
		else if (codepoint < 0x200000) 
		{
			*p++ = 0xf0 | codepoint>>18;
			*p++ = 0x80 | ((codepoint>>12) & 0x3f);
			*p++ = 0x80 | ((codepoint>>6) & 0x3f);
			*p++ = 0x80 | (codepoint & 0x3f);
		} /* ignore bigger ones (that are not possible here anyway) */
	}
	sb->p[sb->size-1] = 0; /* paranoia... */
	sb->fill = sb->size;
}
#undef UTF8LEN
#undef FULLPOINT

static void convert_utf16be(mpg123_string *sb, unsigned char* source, size_t len)
{
	convert_utf16(sb, source, len, 1);
}

static void convert_utf16bom(mpg123_string *sb, unsigned char* source, size_t len)
{
	if(len < 2){ mpg123_free_string(sb); return; }

	if(source[0] == 0xff && source[1] == 0xfe) /* Little-endian */
	convert_utf16(sb, source + 2, len - 2, 0);
	else /* Big-endian */
	convert_utf16(sb, source + 2, len - 2, 1);
}

static void convert_utf8(mpg123_string *sb, unsigned char* source, size_t len)
{
	if(mpg123_resize_string(sb, len+1))
	{
		memcpy(sb->p, source, len);
		sb->p[len] = 0;
		sb->fill = len+1;
	}
	else mpg123_free_string(sb);
}
