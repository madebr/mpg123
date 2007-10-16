#ifndef MPG123_READER_H
#define MPG123_READER_H

#include "config.h"
#include "mpg123.h"

struct buffy
{
	unsigned char *data;
	off_t size;
	struct buffy *next;
};

struct reader_data
{
	off_t filelen; /* total file length or total buffer size */
	off_t filepos; /* position in file or position in buffer chain */
	int   filept;
	int   flags;
#ifndef WIN32
	long timeout_sec;
#endif
	ssize_t (*fdread)(mpg123_handle *, void *, size_t);
	/* variables specific to feed reader */
	off_t firstpos; /* the point of return on non-forget() */
	struct buffy *buf;  /* first in buffer chain */
};

/* start to use off_t to properly do LFS in future ... used to be long */
struct reader
{
	int     (*init)           (mpg123_handle *);
	void    (*close)          (mpg123_handle *);
	ssize_t (*fullread)       (mpg123_handle *, unsigned char *, ssize_t);
	int     (*head_read)      (mpg123_handle *, unsigned long *newhead);    /* succ: TRUE, else <= 0 (FALSE or READER_MORE) */
	int     (*head_shift)     (mpg123_handle *, unsigned long *head);       /* succ: TRUE, else <= 0 (FALSE or READER_MORE) */
	off_t   (*skip_bytes)     (mpg123_handle *, off_t len);                 /* succ: >=0, else error or READER_MORE         */
	int     (*read_frame_body)(mpg123_handle *, unsigned char *, int size);
	int     (*back_bytes)     (mpg123_handle *, off_t bytes);
	int     (*seek_frame)     (mpg123_handle *, off_t num);
	off_t   (*tell)           (mpg123_handle *);
	void    (*rewind)         (mpg123_handle *);
	void    (*forget)         (mpg123_handle *);
};

/* Open a file by path or use an opened file descriptor. */
int open_stream(mpg123_handle *, char *path, int fd);

/* feed based operation has some specials */
int open_feed(mpg123_handle *);
/* externally called function, returns 0 on success, -1 on error */
int  feed_more(mpg123_handle *fr, unsigned char *in, long count);
void feed_forget(mpg123_handle *fr);  /* forget the data that has been read (free some buffers) */
off_t feed_set_pos(mpg123_handle *fr, off_t pos); /* Set position (inside available data if possible), return wanted byte offset of next feed. */

#define READER_FD_OPENED 0x1
#define READER_ID3TAG    0x2
#define READER_SEEKABLE  0x4
#define READER_BUFFERED  0x8
#define READER_MICROSEEK 0x10
#define READER_NONBLOCK  0x20

#define READER_STREAM 0
#define READER_ICY_STREAM 1
#define READER_FEED       2

#ifdef READ_SYSTEM
#define READER_SYSTEM 3
#define READERS 4
#else
#define READERS 3
#endif

#define READER_ERROR -1
#define READER_MORE  MPG123_NEED_MORE

#endif
