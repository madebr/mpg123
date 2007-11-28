/*
	mpg123_to_wav.c

	copyright 2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Nicholas Humfrey
*/

#include <stdio.h>
#include <strings.h>
#include <mpg123.h>
#include <sndfile.h>


void usage()
{
	printf("Usage: mpg123_to_wav <input> <output>\n");
	exit(99);
}


int main(int argc, char *argv[])
{
	SNDFILE* sndfile = NULL;
	SF_INFO sfinfo;
	mpg123_handle *mh = NULL;
	unsigned char* buffer = NULL;
	size_t buffer_size = 0;
	size_t done = 0;
	int err=0;
	
	if (argc!=3) usage();
	printf( "Input file: %s\n", argv[1]);
	printf( "Output file: %s\n", argv[2]);
	
	err = mpg123_init();
	mh = mpg123_new( NULL, &err );
	
	err = mpg123_open( mh, argv[1] );

	buffer_size = mpg123_outblock( mh );
	buffer = malloc( buffer_size );

	//mpg123_decode( mh, NULL, NULL, NULL, NULL, NULL );
	bzero(&sfinfo, sizeof(sfinfo) );
	sfinfo.samplerate = 44100;
	sfinfo.channels = 2;
	sfinfo.format = SF_FORMAT_WAV|SF_FORMAT_PCM_16;
	sndfile = sf_open(argv[2], SFM_WRITE, &sfinfo);


	do
	{
		err = mpg123_read( mh, buffer, buffer_size, &done );
		printf("done=%d\n", (int)done );
		printf("err=%d\n", (int)err );
		
		sf_write_short( sndfile, (short*)buffer, buffer_size/sizeof(short) );
		
	} while (err==MPG123_NEED_MORE || err==MPG123_NEW_FORMAT || err==MPG123_OK);
	
	
	sf_close( sndfile );

	err = mpg123_close( mh );

	mpg123_delete( mh );
	mpg123_exit();



	return 0;
}
