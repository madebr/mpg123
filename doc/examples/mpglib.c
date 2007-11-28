/*
	mpglib: test program for libmpg123, in the style of the legacy mpglib test program

	copyright 2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis
*/

#include "mpg123.h"
#include <unistd.h>
#include <stdio.h>

#define INBUFF 16384

unsigned char buf[INBUFF];

int main(int argc, char **argv)
{
	size_t size;
	unsigned char out[8192];
	ssize_t len;
	int ret;
	size_t in = 0, outc = 0;
	mpg123_handle *m;

	mpg123_init();
	m = mpg123_new(argc > 1 ? argv[1] : NULL, &ret);
	if(m == NULL)
	{
		fprintf(stderr,"Unable to create mpg123 handle: %s\n", mpg123_plain_strerror(ret));
		return -1;
	}
	mpg123_param(m, MPG123_VERBOSE, 2, 0);
	/* mpg123_param(m, MPG123_ADD_FLAGS, MPG123_GAPLESS, 0); */
	/* mpg123_param(m, MPG123_START_FRAME, 2300, 0); */
	mpg123_open_feed(m);
	if(m == NULL) return -1;
	while(1) {
		len = read(0,buf,INBUFF);
		if(len <= 0)
		{
			fprintf(stderr, "input data end\n");
			break;
		}
		in += len;
		/*fprintf(stderr, ">> %lu KiB in\n", (unsigned long)in>>10);*/
		ret = mpg123_decode(m,buf,len,out,8192,&size);
		if(ret == MPG123_NEW_FORMAT)
		{
			long rate;
			int channels, enc;
			mpg123_getformat(m, &rate, &channels, &enc);
			fprintf(stderr, "New format: %li Hz, %i channels, encoding value %i\n", rate, channels, enc);
		}
		write(1,out,size);
		outc += size;
		/*fprintf(stderr, "<< %lu KiB out, ret=%i\n", (unsigned long)outc>>10, ret);*/
		while(ret != MPG123_ERR && ret != MPG123_NEED_MORE) {
			ret = mpg123_decode(m,NULL,0,out,8192,&size);
			write(1,out,size);
			outc += size;
			/*fprintf(stderr, "<< %lu KiB out, ret=%i\n", (unsigned long)outc>>10, ret);*/
		}
		if(ret == MPG123_ERR){ fprintf(stderr, "some error: %s", mpg123_strerror(m)); break; }
	}
	fprintf(stderr, "%lu bytes in, %lu bytes out\n", (unsigned long)in, (unsigned long)outc);
	mpg123_delete(m);
	mpg123_exit();
	return 0;
}

