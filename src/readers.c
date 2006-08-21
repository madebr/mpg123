/*
	readers.c: reading input data

	copyright ?-2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Michael Hipp
*/

#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"
#include "mpg123.h"
#include "debug.h"
#include "buffer.h"
#include "common.h"

#ifdef READ_MMAP
#include <sys/mman.h>
#ifndef MAP_FAILED
#define MAP_FAILED ( (void *) -1 )
#endif
#endif

static off_t get_fileinfo(struct reader *,char *buf);


/*******************************************************************
 * stream based operation
 * Oh... that count should be size_t or at least long...
 */
static ssize_t fullread(struct reader *rds,unsigned char *buf, ssize_t count)
{
  ssize_t ret,cnt=0;

  /*
   * We check against READER_ID3TAG instead of rds->filelen >= 0 because
   * if we got the ID3 TAG we know we have the end of the file.  If we
   * don't have an ID3 TAG, then it is possible the file has grown since
   * we started playing, so we want to keep reading from it if possible.
   */
  if ((rds->flags & READER_ID3TAG) && rds->filepos + count > rds->filelen)
    count = rds->filelen - rds->filepos;
  while(cnt < count) {
    ret = read(rds->filept,buf+cnt,count-cnt);
    if(ret < 0)
      return ret;
    if(ret == 0)
      break;
    rds->filepos += ret;
    cnt += ret;
  } 

  return cnt;
}

static off_t stream_lseek(struct reader *rds, off_t pos, int whence)
{
  off_t ret;

  ret = lseek(rds->filept, pos, whence);
  if (ret >= 0)
    rds->filepos = ret;

  return ret;
}

static int default_init(struct reader *rds)
{
  char buf[128];

  rds->filelen = get_fileinfo(rds,buf);
  rds->filepos = 0;
	
  if(rds->filelen >= 0) {
	  rds->flags |= READER_SEEKABLE;
    if(!strncmp(buf,"TAG",3)) {
      rds->flags |= READER_ID3TAG;
      memcpy(rds->id3buf,buf,128);
    }
  }

  return 0;
}

void stream_close(struct reader *rds)
{
    if (rds->flags & READER_FD_OPENED)
        close(rds->filept);
}

/**************************************** 
 * HACK,HACK,HACK: step back <num> frames 
 * can only work if the 'stream' isn't a real stream but a file
 * returns 0 on success; 
 */
static int stream_back_bytes(struct reader *rds, off_t bytes)
{
  if(stream_lseek(rds,-bytes,SEEK_CUR) < 0)
    return -1;
  if(param.usebuffer)
	  buffer_resync();
  return 0;
}


/* this function strangely is define to seek num frames _back_ (and is called with -offset - duh!) */
/* also... let that int be a long in future! */
#ifdef VBR_SEEK
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

		if(fr->lay == 3) {
			set_pointer(512);
		}

		debug1("arrived at %lu", fr->num);

		if(param.usebuffer)
			buffer_resync();

		return 0;

	}
	else return -1; /* invalid, no seek happened */
}
	
#else
/* There's something bogus about the return value... 0 is success when looking at usage, but here... */
static int stream_back_frame(struct reader *rds,struct frame *fr,long num)
{
	long bytes;

	if(!firsthead)
		return 0;

	/* why +8? header = 4 bytes ... + data */
	bytes = (fr->framesize+8)*(num+2);

	/* Skipping back/forth requires a bit more work in buffered mode. 
	 * See mapped_back_frame(). 
	 */
	if(param.usebuffer)
		bytes += (long)(xfermem_get_usedspace(buffermem) /
			(buffermem->buf[0] * buffermem->buf[1]
				* (buffermem->buf[2] & AUDIO_FORMAT_MASK ?
					16.0 : 8.0 ))
				* (tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index] << 10));
/*
		bytes += (long)(compute_buffer_offset(fr)*compute_bpf(fr));
*/	
	if(stream_lseek(rds,-bytes,SEEK_CUR) < 0)
		return -1;

#if 0
	{
	unsigned char buf[4];
	unsigned long newhead;

	/* This is to find a new valid frame - read_frame can do that itself! */
	/* Problem:
	   Searching for a header identical to firsthead won't do it for files that vary that (ever heard of vbr?).
	   Just searching for a header that passed the check (in read_frame_recover) gives false positives.
	*/
	if(fullread(rds,buf,4) != 4)
		return -1;

	newhead = (buf[0]<<24) + (buf[1]<<16) + (buf[2]<<8) + buf[3];
	
	while( (newhead & HDRCMPMASK) != (firsthead & HDRCMPMASK) ) {
		fprintf(stderr, "0x%08lx / 0x%08lx\n", newhead, firsthead);
		if(fullread(rds,buf,1) != 1)
			return -1;
		newhead <<= 8;
		newhead |= buf[0];
		newhead &= 0xffffffff;
	}

	if( stream_lseek(rds,-4,SEEK_CUR) < 0)
		return -1;
	}
#endif
	
	/* why two times? */
	/* to retain old behaviour: decrease frameNum on success (read_frame increased it) since main wants to set that */
	if(read_frame_recover(fr)) --fr->num;
	if(read_frame_recover(fr)) --fr->num;

	if(fr->lay == 3) {
		set_pointer(512);
	}

	if(param.usebuffer)
		buffer_resync();
	
	return 0;
}
#endif

static int stream_head_read(struct reader *rds,unsigned long *newhead)
{
  unsigned char hbuf[4];

  if(fullread(rds,hbuf,4) != 4)
    return FALSE;
  
  *newhead = ((unsigned long) hbuf[0] << 24) |
    ((unsigned long) hbuf[1] << 16) |
    ((unsigned long) hbuf[2] << 8)  |
    (unsigned long) hbuf[3];
  
  return TRUE;
}

static int stream_head_shift(struct reader *rds,unsigned long *head)
{
  unsigned char hbuf;

  if(fullread(rds,&hbuf,1) != 1)
    return 0;
  *head <<= 8;
  *head |= hbuf;
  *head &= 0xffffffff;
  return 1;
}

/* returns reached position... negative ones are bad */
static off_t stream_skip_bytes(struct reader *rds,off_t len)
{
  if (rds->filelen >= 0) {
    off_t ret = stream_lseek(rds, len, SEEK_CUR);
    if (param.usebuffer)
      buffer_resync();
    return ret;
  } else if (len >= 0) {
    unsigned char buf[1024]; /* ThOr: Compaq cxx complained and it makes sense to me... or should one do a cast? What for? */
    off_t ret;
    while (len > 0) {
      off_t num = len < sizeof(buf) ? len : sizeof(buf);
      ret = fullread(rds, buf, num);
      if (ret < 0)
	return ret;
      len -= ret;
    }
    return rds->filepos;
  } else
    return -1;
}

static int stream_read_frame_body(struct reader *rds,unsigned char *buf,
				  int size)
{
  long l;

  if( (l=fullread(rds,buf,size)) != size)
  {
    if(l <= 0)
      return 0;
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
  if(param.usebuffer) 
	  buffer_resync();
}

/*
 * returns length of a file (if filept points to a file)
 * reads the last 128 bytes information into buffer
 * ... that is not totally safe...
 */
static off_t get_fileinfo(struct reader *rds,char *buf)
{
	off_t len;

        if((len=lseek(rds->filept,0,SEEK_END)) < 0) {
                return -1;
        }
        if(lseek(rds->filept,-128,SEEK_END) < 0)
                return -1;
        if(fullread(rds,(unsigned char *)buf,128) != 128) {
                return -1;
        }
        if(!strncmp(buf,"TAG",3)) {
                len -= 128;
        }
        if(lseek(rds->filept,0,SEEK_SET) < 0)
                return -1;
        if(len <= 0)
                return -1;
	return len;
}


#ifdef READ_MMAP
/*********************************************************+
 * memory mapped operation 
 *
 */
static unsigned char *mapbuf;
static unsigned char *mappnt;
static unsigned char *mapend;

static int mapped_init(struct reader *rds) 
{
	long len;
	char buf[128];

	len = get_fileinfo(rds,buf);
	rds->flags |= READER_SEEKABLE;
	if(len < 0)
		return -1;

	if(!strncmp(buf,"TAG",3)) {
	  rds->flags |= READER_ID3TAG;
	  memcpy(rds->id3buf,buf,128);
	}
        mappnt = mapbuf = (unsigned char *)
		mmap(NULL, len, PROT_READ, MAP_SHARED , rds->filept, 0);
	if(!mapbuf || mapbuf == MAP_FAILED)
		return -1;

	mapend = mapbuf + len;
	
	if(param.verbose > 1)
		fprintf(stderr,"Using memory mapped IO for this stream.\n");

	rds->filelen = len;
	return 0;
}

static void mapped_rewind(struct reader *rds)
{
	mappnt = mapbuf;
	if (param.usebuffer) 
		buffer_resync();	
}

static void mapped_close(struct reader *rds)
{
	munmap((void *)mapbuf,mapend-mapbuf);
	if (rds->flags & READER_FD_OPENED)
		close(rds->filept);
}

static int mapped_head_read(struct reader *rds,unsigned long *newhead) 
{
	unsigned long nh;

	if(mappnt + 4 > mapend)
		return FALSE;

	nh = (*mappnt++)  << 24;
	nh |= (*mappnt++) << 16;
	nh |= (*mappnt++) << 8;
	nh |= (*mappnt++) ;

	*newhead = nh;
	return TRUE;
}

static int mapped_head_shift(struct reader *rds,unsigned long *head)
{
  if(mappnt + 1 > mapend)
    return FALSE;
  *head <<= 8;
  *head |= *mappnt++;
  *head &= 0xffffffff;
  return TRUE;
}

static int mapped_skip_bytes(struct reader *rds,int len)
{
  if(mappnt + len > mapend)
    return FALSE;
  mappnt += len;
  if (param.usebuffer)
	  buffer_resync();
  return TRUE;
}

static int mapped_read_frame_body(struct reader *rds,unsigned char *buf,
				  int size)
{
  if(size <= 0) {
    fprintf(stderr,"Ouch. Read_frame called with size <= 0\n");
    return FALSE;
  }
  if(mappnt + size > mapend)
    return FALSE;
  memcpy(buf,mappnt,size);
  mappnt += size;

  return TRUE;
}

static int mapped_back_bytes(struct reader *rds,int bytes)
{
    if( (mappnt - bytes) < mapbuf || (mappnt - bytes + 4) > mapend)
        return -1;
    mappnt -= bytes;
    if(param.usebuffer)
	    buffer_resync();
    return 0;
}

static int mapped_back_frame(struct reader *rds,struct frame *fr,long num)
{
    long bytes;
    unsigned long newhead;


    if(!firsthead)
        return 0;

    bytes = (fr->framesize+8)*(num+2);

    /* Buffered mode is a bit trickier. From the size of the buffered
     * output audio stream we have to make a guess at the number of frames
     * this corresponds to.
     */
    if(param.usebuffer) 
		bytes += (long)(xfermem_get_usedspace(buffermem) /
			(buffermem->buf[0] * buffermem->buf[1] 
				* (buffermem->buf[2] & AUDIO_FORMAT_MASK ?
			16.0 : 8.0 )) 
			* (tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index] << 10));
/*
	    	bytes += (long)(compute_buffer_offset(fr)*compute_bpf(fr));  
*/

    if( (mappnt - bytes) < mapbuf || (mappnt - bytes + 4) > mapend)
        return -1;
    mappnt -= bytes;

    newhead = (mappnt[0]<<24) + (mappnt[1]<<16) + (mappnt[2]<<8) + mappnt[3];
    mappnt += 4;

    while( (newhead & HDRCMPMASK) != (firsthead & HDRCMPMASK) ) {
        if(mappnt + 1 > mapend)
            return -1;
        newhead <<= 8;
        newhead |= *mappnt++;
        newhead &= 0xffffffff;
    }
    mappnt -= 4;

    read_frame(fr);
    read_frame(fr);

    if(fr->lay == 3)
        set_pointer(512);

    if(param.usebuffer)
	    buffer_resync();
    
    return 0;
}

static long mapped_tell(struct reader *rds)
{
  return mappnt - mapbuf;
}

#endif

/*****************************************************************
 * read frame helper
 */

struct reader *rd;
struct reader readers[] = {
#ifdef READ_SYSTEM
 { system_init,
   NULL,	/* filled in by system_init() */
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL,
   NULL } ,
#endif
#ifdef READ_MMAP
 { mapped_init,
   mapped_close,
   mapped_head_read,
   mapped_head_shift,
   mapped_skip_bytes,
   mapped_read_frame_body,
   mapped_back_bytes,
   mapped_back_frame,
   mapped_tell,
   mapped_rewind } , 
#endif 
 { default_init,
   stream_close,
   stream_head_read,
   stream_head_shift,
   stream_skip_bytes,
   stream_read_frame_body,
   stream_back_bytes,
   stream_back_frame,
   stream_tell,
   stream_rewind } ,
 { NULL, }
};


/* open the device to read the bit stream from it */

int open_stream(char *bs_filenam,int fd)
{
    int i;
    int filept_opened = 1;
    int filept;

    if (!bs_filenam)
		{
			if(fd < 0)
			{
				filept = 0;
				filept_opened = 0;
			}
			else filept = fd;
		}
		else if (!strncmp(bs_filenam, "http://", 7))
		{
			char* mime = NULL;
			filept = http_open(bs_filenam, &mime);
			if((filept >= 0) && (mime != NULL) && (strcmp(mime, "audio/mpeg")))
			{
				fprintf(stderr, "Error: unknown mpeg MIME type %s - is it perhaps a playlist (use -@)?\nError: If you know the stream is mpeg1/2 audio, then please report this as "PACKAGE_NAME" bug\n", mime == NULL ? "<nil>" : mime);
				filept = -1;
			}
			if(mime != NULL) free(mime);
			if(filept < 0) return filept;
		}
#ifndef O_BINARY
#define O_BINARY (0)
#endif
	else if ( (filept = open(bs_filenam, O_RDONLY|O_BINARY)) < 0) {
		perror (bs_filenam);
               return filept;
	}

    rd = NULL;
    for(i=0;;i++) {
      readers[i].filelen = -1;
      readers[i].filept  = filept;
      readers[i].flags = 0;
      if(filept_opened)
        readers[i].flags |= READER_FD_OPENED;
      if(!readers[i].init) {
	fprintf(stderr,"Fatal error!\n");
	exit(1);
      }
      if(readers[i].init(readers+i) >= 0) {
        rd = &readers[i];
        break;
      }
    }

    if(rd && rd->flags & READER_ID3TAG) {
      print_id3_tag(rd->id3buf);
    }

    return filept;
}
