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

// generic cos tables
static double cos64[16],cos32[8],cos16[4],cos8[2],cos4[1];
double *cpnts[] = { cos64,cos32,cos16,cos8,cos4 };

static void compute_costabs(void)
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

// layer I+II tables
static double layer12_table[27][64];
static const double mulmul[27] =
{
	0.0 , -2.0/3.0 , 2.0/3.0 ,
	2.0/7.0 , 2.0/15.0 , 2.0/31.0, 2.0/63.0 , 2.0/127.0 , 2.0/255.0 ,
	2.0/511.0 , 2.0/1023.0 , 2.0/2047.0 , 2.0/4095.0 , 2.0/8191.0 ,
	2.0/16383.0 , 2.0/32767.0 , 2.0/65535.0 ,
	-4.0/5.0 , -2.0/5.0 , 2.0/5.0, 4.0/5.0 ,
	-8.0/9.0 , -4.0/9.0 , -2.0/9.0 , 2.0/9.0 , 4.0/9.0 , 8.0/9.0
};

// Storing only values 0 to 26, so char is fine.
// The size of those might be reduced ... 
static char grp_3tab[32 * 3] = { 0, };   /* used: 27 */
static char grp_5tab[128 * 3] = { 0, };  /* used: 125 */
static char grp_9tab[1024 * 3] = { 0, }; /* used: 729 */

static void compute_layer12(void)
{
	// void init_layer12_stuff()
	// real* init_layer12_table()
	for(int k=0;k<27;k++)
	{
		int i,j;
		double *table = layer12_table[k];
		for(j=3,i=0;i<63;i++,j--)
			*table++ = mulmul[k] * pow(2.0,(double) j / 3.0);
		*table++ = 0.0;
	}

	// void init_layer12()
	const char base[3][9] =
	{
		{ 1 , 0, 2 , } ,
		{ 17, 18, 0 , 19, 20 , } ,
		{ 21, 1, 22, 23, 0, 24, 25, 2, 26 }
	};
	int i,j,k,l,len;
	const int tablen[3] = { 3 , 5 , 9 };
	char *itable;
	char *tables[3] = { grp_3tab , grp_5tab , grp_9tab };

	for(i=0;i<3;i++)
	{
		itable = tables[i];
		len = tablen[i];
		for(j=0;j<len;j++)
		for(k=0;k<len;k++)
		for(l=0;l<len;l++)
		{
			*itable++ = base[i][l];
			*itable++ = base[i][k];
			*itable++ = base[i][j];
		}
	}
}

static void print_char_array( const char *indent, const char *name
,	size_t count, char tab[] )
{
	size_t block = 72/4;
	size_t i = 0;
	if(name)
		printf("static const unsigned char %s[%zu] = \n", name, count);
	printf("%s{\n", indent);
	while(i<count)
	{
		size_t line = block > count-i ? count-i : block;
		printf("%s", indent);
		for(size_t j=0; j<line; ++j, ++i)
			printf("%s%c%3u", i ? "," : "", j ? ' ' : '\t', tab[i]);
		printf("\n");
	}
	printf("%s}%s\n", indent, name ? ";" : "");
}

static void print_array( int fixed, double fixed_scale
,	const char *indent, const char *name
,	size_t count, double tab[] )
{
	size_t block = 72/17;
	size_t i = 0;
	if(name)
		printf( "static const%s real %s[%zu] = \n", fixed ? "" : " ALIGNED(16)"
		,	name, count );
	printf("%s{\n", indent);
	while(i<count)
	{
		size_t line = block > count-i ? count-i : block;
		printf("%s", indent);
		if(fixed) for(size_t j=0; j<line; ++j, ++i)
			printf( "%s%c%11ld", i ? "," : "", j ? ' ' : '\t'
			,	(long)(DOUBLE_TO_REAL(fixed_scale*tab[i])) );
		else for(size_t j=0; j<line; ++j, ++i)
			printf("%s%c%15.8e", i ? "," : "", j ? ' ' : '\t', tab[i]);
		printf("\n");
	}
	printf("%s}%s\n", indent, name ? ";" : "");
}

// C99 allows passing VLA with the fast dimensions first.
static void print_array2d( int fixed, double fixed_scale
,	const char *name, size_t x, size_t y
, double tab[][y] )
{
	printf( "static const%s real %s[%zu][%zu] = \n{\n", fixed ? "" : " ALIGNED(16)"
	,	name, x, y );
	for(size_t i=0; i<x; ++i)
	{
		if(i)
			printf(",");
		print_array(fixed, fixed_scale, "\t", NULL, y, tab[i]);
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
		printf("\n#ifdef %s\n\n", fixed ? "REAL_IS_FIXED" : "REAL_IS_FLOAT");
		if(!fixed)
			printf("// aligned to 16 bytes for vector instructions, e.g. AltiVec\n\n");
		if(!strcmp("cos", argv[1]))
		{
			compute_costabs();
			print_array(fixed, 1., "", "cos64", sizeof(cos64)/sizeof(*cos64), cos64);
			print_array(fixed, 1., "", "cos32", sizeof(cos32)/sizeof(*cos64), cos32);
			print_array(fixed, 1., "", "cos16", sizeof(cos16)/sizeof(*cos64), cos16);
			print_array(fixed, 1., "", "cos8",  sizeof(cos8) /sizeof(*cos64), cos8 );
			print_array(fixed, 1., "", "cos4",  sizeof(cos4) /sizeof(*cos64), cos4 );
		}
		if(!strcmp("l12", argv[1]))
		{
			compute_layer12();
			print_array2d(fixed, SCALE_LAYER12/REAL_FACTOR, "layer12_table", 27, 64, layer12_table);
		}
		printf("\n#endif\n");
	}
	if(!strcmp("l12", argv[1]))
	{
		printf("\n");
		print_char_array("", "grp_3tab", sizeof(grp_3tab), grp_3tab);
		printf("\n");
		print_char_array("", "grp_5tab", sizeof(grp_5tab), grp_5tab);
		printf("\n");
		print_char_array("", "grp_9tab", sizeof(grp_9tab), grp_9tab);
	}

	return 0;
}
