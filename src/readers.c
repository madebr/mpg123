/*
	readers.c: reading input data

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp
*/

#include "mpg123.h"
#include <sys/stat.h>
#include <fcntl.h>

#include "buffer.h"
#include "common.h"
#include "icy.h"

static off_t get_fileinfo(struct reader *,char *buf);

static ssize_t icy_fullread(struct reader *rds,unsigned char *buf, ssize_t count);
static ssize_t plain_fullread(struct reader *rds,unsigned char *buf, ssize_t count);
ssize_t (*fullread)(struct reader *,unsigned char *, ssize_t) = plain_fullread;
static ssize_t timeout_read(struct reader *rds, void *buf, size_t count);
static ssize_t plain_read(struct reader *rds, void *buf, size_t count);
ssize_t (*fdread)(struct reader *rds, void *buf, size_t count) = plain_read;

static ssize_t plain_read(struct reader *rds, void *buf, size_t count){ return read(rds->filept, buf, count); }

#ifndef WIN32
/* Wait for data becoming available, allowing soft-broken network connection to die
   This is needed for Shoutcast servers that have forgotten about us while connection was temporarily down. */
static ssize_t timeout_read(struct reader *rds, void *buf, size_t count)
{
	struct timeval tv;
	ssize_t ret = 0;
	fd_set fds;
	tv.tv_sec = rds->timeout_sec;
	tv.tv_usec = 0;
	FD_ZERO(&fds);
	FD_SET(rds->filept, &fds);
	ret = select(rds->filept+1, &fds, NULL, NULL, &tv);
	if(ret > 0) ret = read(rds->filept, buf, count);
	else
	{
		ret=-1; /* no activity is the error */
		error("stream timed out");
	}
	return ret;
}
#endif

/* stream based operation  with icy meta data*/
static ssize_t icy_fullread(struct reader *rds,unsigned char *buf, ssize_t count)
{
	ssize_t ret,cnt;
	cnt = 0;

	/*
		We check against READER_ID3TAG instead of rds->filelen >= 0 because if we got the ID3 TAG we know we have the end of the file.
		If we don't have an ID3 TAG, then it is possible the file has grown since we started playing, so we want to keep reading from it if possible.
	*/
	if((rds->flags & READER_ID3TAG) && rds->filepos + count > rds->filelen) count = rds->filelen - rds->filepos;

	while(cnt < count)
	{
		/* all icy code is inside this if block, everything else is the plain fullread we know */
		/* debug1("read: %li left", (long) count-cnt); */
		if(icy.interval && (rds->filepos+count > icy.next))
		{
			unsigned char temp_buff;
			size_t meta_size;
			ssize_t cut_pos;

			/* we are near icy-metaint boundary, read up to the boundary */
			cut_pos = icy.next-rds->filepos;
			ret = fdread(rds,buf,cut_pos);
			if(ret < 0) return ret;

			rds->filepos += ret;
			cnt += ret;

			/* now off to read icy data */

			/* one byte icy-meta size (must be multiplied by 16 to get icy-meta length) */
			ret = fdread(rds,&temp_buff,1);
			if(ret < 0) return ret;
			if(ret == 0) break;

			debug2("got meta-size byte: %u, at filepos %li", temp_buff, (long)rds->filepos );
			rds->filepos += ret; /* 1... */

			if((meta_size = ((size_t) temp_buff) * 16))
			{
				/* we have got some metadata */
				char *meta_buff;
				meta_buff = (char*) malloc(meta_size+1);
				if(meta_buff != NULL)
				{
					ret = fdread(rds,meta_buff,meta_size);
					meta_buff[meta_size] = 0; /* string paranoia */
					if(ret < 0) return ret;

					rds->filepos += ret;

					if(icy.data) free(icy.data);
					icy.data = meta_buff;
					icy.changed = 1;
					debug2("icy-meta: %s size: %d bytes", icy.data, (int)meta_size);
				}
				else
				{
					error1("cannot allocate memory for meta_buff (%lu bytes) ... trying to skip the metadata!", (unsigned long)meta_size);
					rds->skip_bytes(rds, meta_size);
				}
			}
			icy.next = rds->filepos+icy.interval;
		}

		ret = fdread(rds,buf+cnt,count-cnt);
		if(ret < 0) return ret;
		if(ret == 0) break;

		rds->filepos += ret;
		cnt += ret;
	}
	/* debug1("done reading, got %li", (long)cnt); */
	return cnt;
}

/* stream based operation */
static ssize_t plain_fullread(struct reader *rds,unsigned char *buf, ssize_t count)
{
	ssize_t ret,cnt=0;

	/*
		We check against READER_ID3TAG instead of rds->filelen >= 0 because if we got the ID3 TAG we know we have the end of the file.
		If we don't have an ID3 TAG, then it is possible the file has grown since we started playing, so we want to keep reading from it if possible.
	*/
	if((rds->flags & READER_ID3TAG) && rds->filepos + count > rds->filelen) count = rds->filelen - rds->filepos;
	while(cnt < count)
	{
		ret = fdread(rds,buf+cnt,count-cnt);
		if(ret < 0) return ret;
		if(ret == 0) break;
		rds->filepos += ret;
		cnt += ret;
	}
	return cnt;
}

static off_t stream_lseek(struct reader *rds, off_t pos, int whence)
{
	off_t ret;
	ret = lseek(rds->filept, pos, whence);
	if (ret >= 0)	rds->filepos = ret;

	return ret;
}

static int default_init(struct reader *rds)
{
	char buf[128];
#ifndef WIN32
	if(param.timeout > 0)
	{
		fcntl(rds->filept, F_SETFL, O_NONBLOCK);
		fdread = timeout_read;
		rds->timeout_sec = param.timeout;
		rds->flags |= READER_NONBLOCK;
	}
	else
#endif
fdread = plain_read;

	rds->filelen = get_fileinfo(rds,buf);
	rds->filepos = 0;
	if(rds->filelen >= 0)
	{
		rds->flags |= READER_SEEKABLE;
		if(!strncmp(buf,"TAG",3))
		{
			rds->flags |= READER_ID3TAG;
			memcpy(rds->id3buf,buf,128);
		}
	}
	return 0;
}

void stream_close(struct reader *rds)
{
	if (rds->flags & READER_FD_OPENED) close(rds->filept);
}

/**************************************** 
 * HACK,HACK,HACK: step back <num> frames 
 * can only work if the 'stream' isn't a real stream but a file
 * returns 0 on success; 
 */
static int stream_back_bytes(struct reader *rds, off_t bytes)
{
	if(stream_lseek(rds,-bytes,SEEK_CUR) < 0) return -1;
	/* you sure you want the buffer to resync here? */
	if(param.usebuffer)	buffer_resync();

	return 0;
}

/* this function strangely is defined to seek num frames _back_ (and is called with -offset - duh!) */
/* also... let that int be a long in future! */
static int stream_back_frame(struct reader *rds,struct frame *fr,long num)
{
	if(rds->flags & READER_SEEKABLE)
	{
		unsigned long newframe, preframe;
		if(num > 0) /* back! */
		{
			if(num > fr->num) newframe = 0;
			else newframe = fr->num-num;
		}
		else newframe = fr->num-num;

		/* two leading frames? hm, doesn't seem to be really needed... */
		/*if(newframe > 1) newframe -= 2;
		else newframe = 0;*/

		/* now seek to nearest leading index position and read from there until newframe is reached */
		if(stream_lseek(rds,frame_index_find(newframe, &preframe),SEEK_SET) < 0)
		return -1;
		debug2("going to %lu; just got %lu", newframe, preframe);
		fr->num = preframe;
		while(fr->num < newframe)
		{
			/* try to be non-fatal now... frameNum only gets advanced on success anyway */
			if(!read_frame(fr)) break;
		}
		/* this is not needed at last? */
		/*read_frame(fr);
		read_frame(fr);*/

		if(fr->lay == 3) set_pointer(512);

		debug1("arrived at %lu", fr->num);

		if(param.usebuffer) buffer_resync();

		return 0;
	}
	else return -1; /* invalid, no seek happened */
}

static int stream_head_read(struct reader *rds,unsigned long *newhead)
{
	unsigned char hbuf[4];

	if(fullread(rds,hbuf,4) != 4) return FALSE;

	*newhead = ((unsigned long) hbuf[0] << 24) |
	           ((unsigned long) hbuf[1] << 16) |
	           ((unsigned long) hbuf[2] << 8)  |
	            (unsigned long) hbuf[3];

	return TRUE;
}

static int stream_head_shift(struct reader *rds,unsigned long *head)
{
	unsigned char hbuf;

	if(fullread(rds,&hbuf,1) != 1) return 0;

	*head <<= 8;
	*head |= hbuf;
	*head &= 0xffffffff;
	return 1;
}

/* returns reached position... negative ones are bad */
static off_t stream_skip_bytes(struct reader *rds,off_t len)
{
	if (rds->filelen >= 0)
	{
		off_t ret = stream_lseek(rds, len, SEEK_CUR);
		if(param.usebuffer) buffer_resync();

		return ret;
	}
	else if(len >= 0)
	{
		unsigned char buf[1024]; /* ThOr: Compaq cxx complained and it makes sense to me... or should one do a cast? What for? */
		off_t ret;
		while (len > 0)
		{
			off_t num = len < sizeof(buf) ? len : sizeof(buf);
			ret = fullread(rds, buf, num);
			if (ret < 0) return ret;
			len -= ret;
		}
		return rds->filepos;
	}
	else return -1;
}

static int stream_read_frame_body(struct reader *rds,unsigned char *buf, int size)
{
	long l;

	if( (l=fullread(rds,buf,size)) != size)
	{
		if(l <= 0) return 0;

		memset(buf+l,0,size-l);
	}

	return 1;
}

static off_t stream_tell(struct reader *rds)
{
	return rds->filepos;
}

static void stream_rewind(struct reader *rds)
{
	stream_lseek(rds,0,SEEK_SET);
	if(param.usebuffer) buffer_resync();
}

/*
 * returns length of a file (if filept points to a file)
 * reads the last 128 bytes information into buffer
 * ... that is not totally safe...
 */
static off_t get_fileinfo(struct reader *rds,char *buf)
{
	off_t len;

	if((len=lseek(rds->filept,0,SEEK_END)) < 0)	return -1;

	if(lseek(rds->filept,-128,SEEK_END) < 0) return -1;

	if(fullread(rds,(unsigned char *)buf,128) != 128)	return -1;

	if(!strncmp(buf,"TAG",3))	len -= 128;

	if(lseek(rds->filept,0,SEEK_SET) < 0)	return -1;

	if(len <= 0)	return -1;

	return len;
}

/*****************************************************************
 * read frame helper
 */

struct reader *rd;
struct reader readers[] =
{
#ifdef READ_SYSTEM
	{
		system_init,
		NULL,	/* filled in by system_init() */
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL,
		NULL
	} ,
#endif
	{
		default_init,
		stream_close,
		stream_head_read,
		stream_head_shift,
		stream_skip_bytes,
		stream_read_frame_body,
		stream_back_bytes,
		stream_back_frame,
		stream_tell,
		stream_rewind
	} ,
	{
		NULL
	}
};


/* open the device to read the bit stream from it */
int open_stream(char *bs_filenam,int fd)
{
	int i;
	int filept_opened = 1;
	int filept; /* descriptor of opened file/stream */

	clear_icy();
	if(!bs_filenam) /* no file to open, got a descriptor (stdin) */
	{
		if(fd < 0) /* special: read from stdin */
		{
			filept = 0;
			filept_opened = 0; /* and don't try to close it... */
		}
		else filept = fd;
	}
	else if (!strncmp(bs_filenam, "http://", 7)) /* http stream */
	{
		char* mime = NULL;
		filept = http_open(bs_filenam, &mime);
		/* now check if we got sth. and if we got sth. good */
		if((filept >= 0) && (mime != NULL) && strcmp(mime, "audio/mpeg") && strcmp(mime, "audio/x-mpeg"))
		{
			fprintf(stderr, "Error: unknown mpeg MIME type %s - is it perhaps a playlist (use -@)?\nError: If you know the stream is mpeg1/2 audio, then please report this as "PACKAGE_NAME" bug\n", mime == NULL ? "<nil>" : mime);
			filept = -1;
		}
		if(mime != NULL) free(mime);
		if(filept < 0) return filept; /* error... */
	}
	#ifndef O_BINARY
	#define O_BINARY (0)
	#endif
	else if((filept = open(bs_filenam, O_RDONLY|O_BINARY)) < 0) /* a plain old file to open... */
	{
		perror(bs_filenam);
		return filept; /* error... */
	}

	/* now we have something behind filept and can init the reader */
	rd = NULL;
	/* strongly considering removal of that loop...*/
	for(i=0;;i++)
	{
		readers[i].filelen = -1;
		readers[i].filept  = filept;
		readers[i].flags = 0;
		if(filept_opened)	readers[i].flags |= READER_FD_OPENED;
		if(!readers[i].init)
		{
			error1("no init for reader %i!", i);
			return -1;
		}
		/* now use this reader if it successfully inits */
		if(readers[i].init(readers+i) >= 0)
		{
			rd = &readers[i];
			break;
		}
	}

	if(icy.interval)
	{
		fullread = icy_fullread;
		icy.next = icy.interval;
	}
	else
	{
		fullread = plain_fullread;
	}

	/* id3tag printing moved to read_frame */
	return filept;
}
