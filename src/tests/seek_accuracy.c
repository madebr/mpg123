/*
	seek-accuracy: Take some given mpeg file and validate that seeks are indeed accurate.

	copyright 2009 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis

	arguments: decoder testfile.mpeg
*/

#include <mpg123.h>
#include <compat.h>

/* Use getlopt.c in future? But heck, don't bloat. */
enum theargs
{
	 arg_binname = 0
	,arg_decoder
	,arg_preframes
	,arg_file
	,arg_total
};

const char* filename;
int channels;
mpg123_handle *m;

int check_seeking();

int main(int argc, char **argv)
{
	int ret = 0;
	if(argc < arg_total)
	{
		fprintf(stderr, "\nUsage: %s <decoder> <mpeg audio file>\n\n", argv[0]);
		return -1;
	}
	mpg123_init();
	m = mpg123_new(argv[arg_decoder], NULL);
	mpg123_param(m, MPG123_RESYNC_LIMIT, -1, 0);

	if(mpg123_param(m, MPG123_PREFRAMES, atol(argv[arg_preframes]), 0) == MPG123_OK)
	printf("Testing library with preframes set to %li\n", atol(argv[arg_preframes]));

	filename = argv[arg_file];
	ret = check_seeking();

	if(ret == 0)
	{
		printf("Now with NtoM resampling to 33333 Hz!");
		mpg123_param(m, MPG123_FORCE_RATE, 33333, 0);
		ret = check_seeking();
		if(ret != 0)
		printf("This libmpg123 is sample-accurate for normal decoding but _not_ for NtoM resampling!\n");
		else
		printf("Congratulations, all seeks were accurate!\n");
	}
	else{ printf("This libmpg123 is _not_ sample-accurate for normal decoding!\n"); }

	mpg123_delete(m);
	mpg123_exit();

	return ret;
}

int open_file()
{
	if( mpg123_open(m, filename) == MPG123_OK
	     &&
	    mpg123_getformat(m, NULL, &channels, NULL) == MPG123_OK )
	{
		printf("Channels: %i\n", channels);
		return 0;
	}
	else
	{
		printf("Opening file failed: %s\n", mpg123_strerror(m));
		return -1;
	}
}

/* Operation:
	Read through the whole file and remember selected samples and their position.
	Then, seek to the positions selectively and compare.
	Second mode: Check if this is identical to fresh decoding and seek (without frame index table)... less important now, focusing on the first one.
*/

#define SAMPLES 1000
struct seeko
{
	off_t position[SAMPLES];
	short left[SAMPLES];
	short right[SAMPLES];
};

/* select some positions to check seek accuracy on */
void fix_positions(struct seeko *so, off_t length)
{
	off_t step = (length-1)/(SAMPLES-1);
	size_t i;
	/* simple fixed stepping, in order (one could shuffle this) */
	for(i=0; i<SAMPLES-1; ++i)
	so->position[i] = i*step;

	/* plus the last one! */
	so->position[SAMPLES-1] = length-1;
}

int collect(struct seeko *so)
{
	off_t pos_count = 0;
	off_t length;
	int err = MPG123_OK;
	size_t posi = 0;
	mpg123_scan(m);
	length = mpg123_length(m);
	printf("Estimated length: %"OFF_P"\n", (off_p)length);
	/* Compute the interesting positions */
	fix_positions(so, length);
	/*
		Default format is always 16bit int, rate does not matter.
		Let's just get the channel count and not bother.
	*/
	while(err == MPG123_OK)
	{
		short buff[1024]; /* choosing a non-divider of mpeg frame size on purpose */
		size_t got = 0;
		off_t buffsamples;
		err = mpg123_read(m, (unsigned char*)buff, 1024*sizeof(short), &got);
		buffsamples = got/(channels*sizeof(short));
		while(so->position[posi] < pos_count+buffsamples)
		{
			size_t i = (so->position[posi]-pos_count)*channels;
			printf("got sample %"SIZE_P" (%"OFF_P")\n", (size_p)posi, (off_p)so->position[posi]);
			so->left[posi] = buff[i];
			if(channels == 2)
			so->right[posi] = buff[i+1];

			if(++posi >= SAMPLES) break;
		}
		if(posi >= SAMPLES) break;

		pos_count += buffsamples;
	}
	if(err != MPG123_DONE && err != MPG123_OK)
	{
		printf("An error occured (not done)?: %s\n", mpg123_strerror(m));
		return -1;
	}
	return 0;
}

int check_sample(struct seeko *so, size_t i)
{
	short buf[2]; /* at max one left and right sample */
	size_t got = 0;

	if(mpg123_seek(m, so->position[i], SEEK_SET) != so->position[i])
	{
		printf("Error seeking to %"OFF_P": %s\n", (off_p)so->position[i],  mpg123_strerror(m));
		return -1;
	}

	if( mpg123_read(m, (unsigned char*)buf, channels*sizeof(short), &got) != MPG123_OK
		 ||
		 got/sizeof(short) != channels )
	{
		printf("Error occured on reading sample %"SIZE_P"! (%s)\n", (size_p)i, mpg123_strerror(m));
		return -1;
	}
	if(buf[0] == so->left[i] && (channels == 1 || buf[1] == so->right[i]))
	{
		printf("sample %"SIZE_P" PASS\n", (size_p)i);
	}
	else
	{
		printf("sample %"SIZE_P" FAIL\n", (size_p)i);
		return -1;
	}
	return 0;
}

int check_positions(struct seeko *so)
{
	size_t i;
	printf("Seeking and comparing...\n");
	for(i=0; i<SAMPLES; ++i)
	{
		if(check_sample(so, i) != 0)
		return -1;
	}
	printf("Check the first one again, seeking back from the end.\n");
	if(check_sample(so, 0) != 0)
	return -1;

	return 0;
}

int check_seeking()
{
	struct seeko so;
	int ret = 0;

	/* First opening. */
	if(open_file() != 0) return -1;

	printf("Collecting samples...\n");
	ret = collect(&so);
	if(ret != 0) return ret;

	ret = check_positions(&so);
	if(ret != 0) return ret;

	printf("With fresh opening (empty seek index):\n");
	if(open_file() != 0) return -1;

	ret = check_positions(&so);

	return ret;
}
