#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <limits.h>

#define BS 16384

int main(int argc, char** argv)
{
	short buf1[BS], buf2[BS];
	ssize_t fill1, fill2;
	int a = open(argv[1], O_RDONLY);
	int b = open(argv[2], O_RDONLY);
	if(a < 0 || b < 0){ fprintf(stderr, "cannot open files\n");return 1; }
	long max = 0;
	/* amplitude spread for normalization */
	short maxin = SHRT_MIN;
	short minin = SHRT_MAX;
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
		long diff = (long)av-(long)bv;
		if(av < minin) minin = av;
		if(av > maxin) maxin = av;
		if(diff < 0) diff *= -1;
		if(diff > max) max = diff;
		if(diff) ++diffcount;
		/* if(diff > 10) fprintf(stderr, "diff %i at %li\n", diff, count); */
		++count;
		rms += pow(diff,2);
		}
	}
	rms /= count*pow((long)maxin-(long)minin, 2);
	printf("RMS=%g; max=%i; diffcount=%li in count=%li; input amplitude range %li, from %i to %i\n", sqrt(rms), max, diffcount, count, (long)maxin-(long)minin, minin, maxin);
	close(a);
	close(b);
	return 0;
}
