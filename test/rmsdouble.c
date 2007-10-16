#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
	long negmax = 8388608;
	int a = open(argv[1], O_RDONLY);
	int b = open(argv[2], O_RDONLY);
	if(a < 0 || b < 0){ fprintf(stderr, "cannot open files\n");return 1; }
	double av;
	double bv;
	double rms = 0;
	double intrms = 0;
	int32_t intmax = 0;
	double max = 0;
	long count = 0;
	while( (read(a, &av, sizeof(double)) == sizeof(double))
	   &&  (read(b, &bv, sizeof(double)) == sizeof(double)) )
	{
		/* quantization to 24 bit... is that different ? */
		int32_t ai = (int32_t) rint(bv*negmax);
		int32_t bi = (int32_t) rint(av*negmax);
		int32_t ad = ai-bi;
		if(ad < 0) ad *= -1;
		if(ad > intmax) intmax = ad;
		if(ad > 1){ fprintf(stderr, "big diff at %li (%g vs. %g)\n", count, av*negmax, bv*negmax); break; }
		else if(ad) fprintf(stderr, "diff at %li (%g vs. %g)\n", count, av*negmax, bv*negmax);
		++count;
		rms += pow((double)av-bv,2);
		intrms += pow( (ai-bi), 2 );
		double vd = av-bv;
		if(vd < 0) vd *= -1;
		if(vd > max) max = vd;
	}
	rms /= count;
	intrms /= count;
	printf("RMS=%g (max=%g); intRMS=%g (max=%i)\n", sqrt(rms), max, sqrt(intrms)/negmax, intmax);
	close(a);
	close(b);
	return 0;
}
