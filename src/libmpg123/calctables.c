/*
	calctables: compute fixed decoder table values

	copyright ?-2021 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Michael Hipp (as tabinit.c, and parts of layer2.c and layer3.c)

	This is supposed to compute the suppesdly fixed tables that used to be computed
	live on library startup in mpg123_init().
*/

#define FORCE_FIXED
#include "mpg123lib_intern.h"
#include "debug.h"

/* That altivec alignment part here should not hurt generic code, I hope */
static double cos64[16],cos32[8],cos16[4],cos8[2],cos4[1];
double *cpnts[] = { cos64,cos32,cos16,cos8,cos4 };

static void compute_costabs()
{
  int i,k,kr,divv;
  double *costab;

  for(i=0;i<5;i++)
  {
    kr=0x10>>i; divv=0x40>>i;
    costab = cpnts[i];
    for(k=0;k<kr;k++)
      costab[k] = 1.0 / (2.0 * cos(M_PI * ((double) k * 2.0 + 1.0) / (double) divv));
  }
}

static void print_array(int fixed, const char *name, double *tab, size_t count)
{
	size_t block = 72/17;
	size_t i = 0;
	printf( "static const%s real %s[%zu] = \n{\n", fixed ? "" : " ALIGNED(16)"
	,	name, count );
	while(i<count)
	{
		size_t line = block > count-i ? count-i : block;
		if(fixed) for(size_t j=0; j<line; ++j, ++i)
			printf( "%c%c%11ld", i ? ',' : ' ', j ? ' ' : '\t'
			,	(long)(DOUBLE_TO_REAL(tab[i])) );
		else for(size_t j=0; j<line; ++j, ++i)
			printf("%c%c%15.8e", i ? ',' : ' ', j ? ' ' : '\t', tab[i]);
		printf("\n");
	}
	printf("};\n");
}

int main(int argc, char **argv)
{
	if(argc != 2)
	{
		fprintf(stderr, "usage:\n\t%s <cos|l12|l3>\n\n", argv[0]);
		return 1;
	}
	printf("// output of:\n// %s", argv[0]);
	for(int i=1; i<argc; ++i)
		printf(" %s", argv[i]);
	printf("\n\n");

	for(int fixed=0; fixed < 2; ++fixed)
	{
		printf("#ifdef %s\n\n", fixed ? "REAL_IS_FIXED" : "REAL_IS_FLOAT");
		if(!fixed)
			printf("// aligned to 16 bytes for vector instructions, e.g. AltiVec\n\n");
		if(!strcmp("cos", argv[1]))
		{
			compute_costabs();
			print_array(fixed, "cos64", cos64, sizeof(cos64)/sizeof(*cos64));
			print_array(fixed, "cos32", cos32, sizeof(cos32)/sizeof(*cos64));
			print_array(fixed, "cos16", cos16, sizeof(cos16)/sizeof(*cos64));
			print_array(fixed, "cos8",   cos8, sizeof(cos8) /sizeof(*cos64));
			print_array(fixed, "cos4",   cos4, sizeof(cos4) /sizeof(*cos64));
		}
		printf("\n#endif\n\n");
	}

	return 0;
}
