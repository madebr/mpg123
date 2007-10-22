#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BS 16384

int main(int argc, char** argv)
{
	short buf1[BS], buf2[BS];
	ssize_t fill1, fill2;
	int a = open(argv[1], O_RDONLY);
	int b = open(argv[2], O_RDONLY);
	if(a < 0 || b < 0){ fprintf(stderr, "cannot open files\n");return 1; }
	short max = 0;
	double rms = 0;
	long count = 0;
	long diffcount = 0;
	while( (fill1=read(a, buf1, sizeof(short)*BS)) > 0
	   &&  (fill2=read(b, buf2, sizeof(short)*BS)) > 0 )
	{
		size_t i;
		if(fill2 < fill1) fill1 = fill2;
		fill1 /= sizeof(short);
		for(i = 0; i < fill1; ++i)
		{
		short av = buf1[i];
		short bv = buf2[i];
		short diff = av-bv;
		if(diff < 0) diff *= -1;
		if(diff > max) max = diff;
		if(diff) ++diffcount;
		/* if(diff > 10) fprintf(stderr, "diff %i at %li\n", diff, count); */
		++count;
		rms += pow((double)av/32768-(double)bv/32768,2);
		}
	}
	rms /= count;
	printf("RMS=%g; max=%i; diffcout=%li in count=%li\n", sqrt(rms), max, diffcount, count);
	close(a);
	close(b);
	return 0;
}
