#include <unistd.h>
#include <math.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(int argc, char** argv)
{
	FILE *fa, *fb;
	double iso_rms_limit, iso_diff_limit;
	double rms, maxdiff;
	size_t bufs = 1024;
	size_t got;
	double av[bufs], bv[bufs];
	long count;

	fprintf(stderr,"Computing RMS full scale for double float data (thus, full scale is 2).\n");
	/* Reference values relative to full scale 2 (from -1 to +1). They are _defined_ relative to full scale. */
	iso_rms_limit  = pow(2.,-15)/sqrt(12.);
	iso_diff_limit = pow(2.,-14);
	fprintf(stderr, "ISO limit values: RMS=%g maxdiff=%g\n", iso_rms_limit, iso_diff_limit);

	rms = 0;
	maxdiff = 0;
	count = 0;

	fa = fopen(argv[1], "r");
	fb = argc > 2 ? fopen(argv[2], "r") : stdin;
	if(fa == NULL || fb == NULL){ fprintf(stderr, "cannot open files\n");return 1; }

	while( (got = fread(av, sizeof(double), bufs, fa))
	           == fread(bv, sizeof(double), bufs, fb) && got )
	{
		size_t i;
		for(i=0; i<got; ++i)
		{
			double vd = (double)av[i]-(double)bv[i];
			++count;
			rms += vd*vd;
			if(vd < 0) vd *= -1;
			if(vd > maxdiff) maxdiff = vd;
		}
	}

	fclose(fa);
	if(fb != stdin) fclose(fb);

	rms /= count;
	rms  = sqrt(rms);
	rms /= 2.; /* full scale... */
	maxdiff /= 2.; /* full scale again */
	printf("RMS=%g (%s) maxdiff=%g (%s)\n",
		rms, rms<iso_rms_limit ? "PASS" : "FAIL",
		maxdiff, maxdiff<iso_diff_limit ? "PASS" : "FAIL");
	return 0;
}
