/*
	optimize: get a grip on the different optimizations

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Thomas Orgis, taking from mpg123.[hc]

	for building mpg123 with one optimization only, you have to choose exclusively between
	OPT_GENERIC (generic C code for everyone)
	OPT_I386 (Intel i386)
	OPT_I486 (...)
	OPT_I586 (Intel Pentium)
	OPT_I586_DITHER (Intel Pentium with dithering/noise shaping for enhanced quality)
	OPT_MMX (Intel Pentium and compatibles with MMX, fast, but not the best accuracy)
	OPT_3DNOW (AMD 3DNow!, K6-2/3, Athlon, compatibles...)
	OPT_ALTIVEC (Motorola/IBM PPC with AltiVec under MacOSX)

	or you define OPT_MULTI and give a combination which makes sense (do not include i486, do not mix altivec and x86).

	I still have to examine the dynamics of this here together with REAL_IS_FIXED.
*/

/* the optimizations only cover the synth1to1 mode and the dct36 function */
/* the first two types are needed in set_synth_functions regardless of optimizations */
typedef int (*func_synth)(real *,int,unsigned char *,int *);
typedef int (*func_synth_mono)(real *,unsigned char *,int *);
typedef void (*func_dct36)(real *,real *,real *,real *,real *);
typedef	void (*func_dct64)(real *,real *,real *);
typedef void (*func_make_decode_tables)(long);
typedef real (*func_init_layer3_gainpow2)(int);
typedef real* (*func_init_layer2_table)(real*, double);
typedef int (*func_synth_pent)(real *,int,unsigned char *);

/* last headaches about getting mmx hardcode out */
real init_layer3_gainpow2(int i);
real* init_layer2_table(real *table, double m);
void make_decode_tables(long scale);

/* only 3dnow replaces that one, it's internal to layer3.c otherwise */
void dct36(real *,real *,real *,real *,real *);
#define opt_dct36 dct36
/* only mmx replaces those */
#define opt_make_decode_tables make_decode_tables
#define opt_decwin decwin
#define opt_init_layer3_gainpow2 init_layer3_gainpow2
#define opt_init_layer2_table init_layer2_table

#ifdef OPT_GENERIC
	void dct64(real *,real *,real *);
	int synth_1to1(real *bandPtr,int channel,unsigned char *out,int *pnt);
	int synth_1to1 (real *,int,unsigned char *,int *);
	int synth_1to1_8bit (real *,int,unsigned char *,int *);
	int synth_1to1_mono (real *,unsigned char *,int *);
	int synth_1to1_mono2stereo (real *,unsigned char *,int *);
	int synth_1to1_8bit_mono (real *,unsigned char *,int *);
	int synth_1to1_8bit_mono2stereo (real *,unsigned char *,int *);
	#ifndef OPT_MULTI
	#define opt_dct64 dct64
	#define opt_synth_1to1 synth_1to1
	#define opt_synth_1to1_mono synth_1to1_mono
	#define opt_synth_1to1_mono2stereo synth_1to1_mono2stereo
	#define opt_synth_1to1_8bit synth_1to1_8bit
	#define opt_synth_1to1_8bit_mono synth_1to1_8bit_mono
	#define opt_synth_1to1_8bit_mono2stereo synth_1to1_8bit_mono2stereo
	#endif
#endif

/* i486 is special */
#ifdef OPT_I486
	#define OPT_I386
	#define FIR_BUFFER_SIZE  128
	int synth_1to1_486(real *bandPtr,int channel,unsigned char *out,int nb_blocks);
	void dct64_i486(int *a,int *b,real *c); /* not used generally */
#endif

#ifdef OPT_I386
	#define OPT_X86
	int synth_1to1_i386(real *bandPtr,int channel,unsigned char *out,int *pnt);
	#ifndef OPT_MULTI
	#define opt_synth_1to1 synth_1to1_i386
	#endif
#endif

#ifdef OPT_I586
	#define OPT_PENTIUM
	#define OPT_X86
	int synth_1to1_i586(real *bandPtr,int channel,unsigned char *out,int *pnt);
	int synth_1to1_i586_asm(real *,int,unsigned char *);
	#ifndef OPT_MULTI
	#define opt_synth_1to1 synth_1to1_i586
	#define opt_synth_1to1_i586_asm synth_1to1_i586_asm
	#endif
#endif

#ifdef OPT_I586_DITHER
	#define OPT_PENTIUM
	#define OPT_X86
	int synth_1to1_i586(real *bandPtr,int channel,unsigned char *out,int *pnt);
	int synth_1to1_i586_asm_dither(real *,int,unsigned char *);
	#ifndef OPT_MULTI
	#define opt_synth_1to1 synth_1to1_i586
	#define opt_synth_1to1_i586_asm synth_1to1_i586_asm_dither
	#endif
#endif

/* That one has by far the most ugly hacks to make it cooperative. */
#ifdef OPT_MMX
	#define OPT_MMXORSSE
	#define OPT_X86
	real init_layer3_gainpow2_mmx(int i);
	real* init_layer2_table_mmx(real *table, double m);
	/* I think one can optimize storage here with the normal decwin */
	extern real decwin_mmx[512+32];
	void dct64_mmx(real *,real *,real *);
	int synth_1to1_mmx(real *bandPtr,int channel,unsigned char *out,int *pnt);
	void make_decode_tables_mmx(long scaleval); /* tabinit_mmx.s */
	#ifndef OPT_MULTI
	#undef opt_decwin
	#define opt_decwin decwin_mmx
	#define opt_dct64 dct64_mmx
	#define opt_synth_1to1 synth_1to1_mmx
	#undef opt_make_decode_tables
	#define opt_make_decode_tables make_decode_tables_mmx
	#undef opt_init_layer3_gainpow2
	#define opt_init_layer3_gainpow2 init_layer3_gainpow2_mmx
	#undef opt_init_layer2_table
	#define opt_init_layer2_table init_layer2_table_mmx
	#define OPT_MMX_ONLY
	#endif
#endif

/* first crude hack into our source */
#ifdef OPT_SSE
	#define OPT_MMXORSSE
	#define MPLAYER
	#define OPT_X86
	real init_layer3_gainpow2_mmx(int i);
	real* init_layer2_table_mmx(real *table, double m);
	/* I think one can optimize storage here with the normal decwin */
	extern real decwin_mmx[512+32];
	void dct64_mmx(real *,real *,real *);
	void dct64_sse(real *,real *,real *);
	int synth_1to1_sse(real *bandPtr,int channel,unsigned char *out,int *pnt);
	void make_decode_tables_mmx(long scaleval); /* tabinit_mmx.s */
	/* ugly! */
	extern func_dct64 mpl_dct64;
	#ifndef OPT_MULTI
	#define opt_mpl_dct64 dct64_sse
	#undef opt_decwin
	#define opt_decwin decwin_mmx
	#define opt_dct64 dct64_mmx /* dct64_sse is silent in downsampling modes */
	#define opt_synth_1to1 synth_1to1_sse
	#undef opt_make_decode_tables
	#define opt_make_decode_tables make_decode_tables_mmx
	#undef opt_init_layer3_gainpow2
	#define opt_init_layer3_gainpow2 init_layer3_gainpow2_mmx
	#undef opt_init_layer2_table
	#define opt_init_layer2_table init_layer2_table_mmx
	#define OPT_MMX_ONLY /* watch out! */
	#endif
#endif

/* first crude hack into our source */
#ifdef OPT_3DNOWEXT
	#define OPT_MMXORSSE
	#define OPT_MPLAYER
	#define OPT_X86
	real init_layer3_gainpow2_mmx(int i);
	real* init_layer2_table_mmx(real *table, double m);
	/* I think one can optimize storage here with the normal decwin */
	extern real decwin_mmx[512+32];
	void dct64_mmx(real *,real *,real *);
	void dct64_3dnowext(real *,real *,real *);
	void dct36_3dnowext(real *,real *,real *,real *,real *);
	int synth_1to1_sse(real *bandPtr,int channel,unsigned char *out,int *pnt);
	void make_decode_tables_mmx(long scaleval); /* tabinit_mmx.s */
	/* ugly! */
	extern func_dct64 mpl_dct64;
	#ifndef OPT_MULTI
	#define opt_mpl_dct64 dct64_3dnowext
	#undef opt_dct36
	#define opt_dct36 dct36_3dnowext
	#undef opt_decwin
	#define opt_decwin decwin_mmx
	#define opt_dct64 dct64_mmx /* dct64_sse is silent in downsampling modes */
	#define opt_synth_1to1 synth_1to1_sse
	#undef opt_make_decode_tables
	#define opt_make_decode_tables make_decode_tables_mmx
	#undef opt_init_layer3_gainpow2
	#define opt_init_layer3_gainpow2 init_layer3_gainpow2_mmx
	#undef opt_init_layer2_table
	#define opt_init_layer2_table init_layer2_table_mmx
	#define OPT_MMX_ONLY /* watch out! */
	#endif
#endif


#ifndef OPT_MMX_ONLY
extern real *pnts[5];
extern real decwin[512+32];
#endif

/* 3dnow used to use synth_1to1_i586 for mono / 8bit conversion - was that intentional? */
/* I'm trying to skip the pentium code here ... until I see that that is indeed a bad idea */
#ifdef OPT_3DNOW
	#define OPT_X86
	void dct36_3dnow(real *,real *,real *,real *,real *);
	int synth_1to1_3dnow(real *,int,unsigned char *,int *);
	#ifndef OPT_MULTI
	#undef opt_dct36
	#define opt_dct36 dct36_3dnow
	#define opt_synth_1to1 synth_1to1_3dnow
	#endif
#endif

#ifdef OPT_X86
	/* these have to be merged back into one! */
	unsigned int getcpuid();
	unsigned int getextcpuflags();
	unsigned int getstdcpuflags();
	unsigned int getstd2cpuflags();

	void dct64_i386(real *,real *,real *);
	int synth_1to1_mono_i386(real *,unsigned char *,int *);
	int synth_1to1_mono2stereo_i386(real *,unsigned char *,int *);
	int synth_1to1_8bit_i386(real *,int,unsigned char *,int *);
	int synth_1to1_8bit_mono_i386(real *,unsigned char *,int *);
	int synth_1to1_8bit_mono2stereo_i386(real *,unsigned char *,int *);
	#ifndef OPT_MULTI
	#ifndef opt_dct64
	#define opt_dct64 dct64_i386 /* default one even for 3dnow and i486 in decode_2to1, decode_ntom */
	#endif
	#define opt_synth_1to1_mono synth_1to1_mono_i386
	#define opt_synth_1to1_mono2stereo synth_1to1_mono2stereo_i386
	#define opt_synth_1to1_8bit synth_1to1_8bit_i386
	#define opt_synth_1to1_8bit_mono synth_1to1_8bit_mono_i386
	#define opt_synth_1to1_8bit_mono2stereo synth_1to1_8bit_mono2stereo_i386
	#endif
#endif

#ifdef OPT_ALTIVEC
	void dct64_altivec(real *out0,real *out1,real *samples);
	int synth_1to1_altivec(real *,int,unsigned char *,int *);
	int synth_1to1_mono_altivec(real *,unsigned char *,int *);
	int synth_1to1_mono2stereo_altivec(real *,unsigned char *,int *);
	int synth_1to1_8bit_altivec(real *,int,unsigned char *,int *);
	int synth_1to1_8bit_mono_altivec(real *,unsigned char *,int *);
	int synth_1to1_8bit_mono2stereo_altivec(real *,unsigned char *,int *);
	#ifndef OPT_MULTI
	#define opt_dct64 dct64_altivec
	#define opt_synth_1to1 synth_1to1_altivec
	#define opt_synth_1to1_mono synth_1to1_mono_altivec
	#define opt_synth_1to1_mono2stereo synth_1to1_mono2stereo_altivec
	#define opt_synth_1to1_8bit synth_1to1_8bit_altivec
	#define opt_synth_1to1_8bit_mono synth_1to1_8bit_mono_altivec
	#define opt_synth_1to1_8bit_mono2stereo synth_1to1_8bit_mono2stereo_altivec
	#endif
#endif
		
/* used for multi opt mode and the single 3dnow mode to have the old 3dnow test flag still working */
void test_cpu_flags();
void list_cpu_opt();

#ifdef OPT_MULTI
	int set_cpu_opt();
	/* a simple global struct to hold the decoding function pointers, could be localized later if really wanted */
	typedef struct
	{
		func_synth synth_1to1;
		func_synth_mono synth_1to1_mono;
		func_synth_mono synth_1to1_mono2stereo;
		func_synth synth_1to1_8bit;
		func_synth_mono synth_1to1_8bit_mono;
		func_synth_mono synth_1to1_8bit_mono2stereo;
		#ifdef OPT_PENTIUM
		func_synth_pent synth_1to1_i586_asm;
		#endif
		#ifdef OPT_MMXORSSE
		real *decwin; /* ugly... needed to get mmx together with folks*/
		func_make_decode_tables   make_decode_tables;
		func_init_layer3_gainpow2 init_layer3_gainpow2;
		func_init_layer2_table    init_layer2_table;
		#endif
		#ifdef OPT_3DNOW
		func_dct36 dct36;
		#endif
		func_dct64 dct64;
		#ifdef MPLAYER
		func_dct64 mpl_dct64;
		#endif
	} struct_opts;
	extern struct_opts cpu_opts;

	#define opt_synth_1to1 (cpu_opts.synth_1to1)
	#define opt_synth_1to1_mono (cpu_opts.synth_1to1_mono)
	#define opt_synth_1to1_mono2stereo (cpu_opts.synth_1to1_mono2stereo)
	#define opt_synth_1to1_8bit (cpu_opts.synth_1to1_8bit)
	#define opt_synth_1to1_8bit_mono (cpu_opts.synth_1to1_8bit_mono)
	#define opt_synth_1to1_8bit_mono2stereo (cpu_opts.synth_1to1_8bit_mono2stereo)
	#ifdef OPT_PENTIUM
	#define opt_synth_1to1_i586_asm (cpu_opts.synth_1to1_i586_asm)
	#endif
	#ifdef OPT_MMXORSSE
	#undef opt_make_decode_tables
	#define opt_make_decode_tables (cpu_opts.make_decode_tables)
	#undef opt_decwin
	#define opt_decwin cpu_opts.decwin
	#undef opt_init_layer3_gainpow2
	#define opt_init_layer3_gainpow2 (cpu_opts.init_layer3_gainpow2)
	#undef opt_init_layer2_table
	#define opt_init_layer2_table (cpu_opts.init_layer2_table)
	#endif
	#ifdef OPT_3DNOW
	#undef opt_dct36
	#define opt_dct36 (cpu_opts.dct36)
	#endif
	#define opt_dct64 (cpu_opts.dct64)
	#ifdef MPLAYER
	#define opt_mpl_dct64 (cpu_opts.mpl_dct64)
	#endif
#endif
