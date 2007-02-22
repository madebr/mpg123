/*
	optimize: get a grip on the different optimizations

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Thomas Orgis, inspired by 3DNow stuff in mpg123.[hc]

	Currently, this file contains the struct and function to choose an optimization variant and works only when OPT_MULTI is in effect.
*/

#include "config.h"
#include "mpg123.h" /* includes optimize.h */
#include "debug.h"
#ifdef OPT_MULTI

struct_opts cpu_opts;

/* standard level flags part 1 */
#define FLAG_SSE3      0x00000001
/* standard level flags part 2 */
#define FLAG2_MMX       0x00800000
#define FLAG2_SSE       0x02000000
#define FLAG2_SSE2      0x04000000
#define FLAG2_FPU       0x00000001
/* cpuid extended level 1 (AMD) */
#define XFLAG_MMX      0x00800000
#define XFLAG_3DNOW    0x80000000
#define XFLAG_3DNOWEXT 0x40000000

/* identify pentium-class cpus by family, just excluding the few 486 */
/* bits 11...8, so the value is ((id) & 0xf00)>>8 */
int is_i586()
{
	int family = getcpuid();
	if(!family) return 0; /* no flags at all - cpuid not supported? */
	family = (family & 0xf00)>>8;
	/* fprintf(stderr, "family: %i\n", family); */
	return (family > 4 || family == 0);
}

void list_cpu_opt()
{
	printf("CPU options:");
	#ifdef OPT_SSE
	printf(" SSE");
	#endif
	#ifdef OPT_3DNOW
	printf(" 3DNow");
	#endif
	#ifdef OPT_MMX
	printf(" MMX");
	#endif
	#ifdef OPT_I586
	printf(" i586");
	#endif
	#ifdef OPT_I586_DITHER
	printf(" i586_dither");
	#endif
	#ifdef OPT_I486
	printf(" i486");
	#endif
	#ifdef OPT_I386
	printf(" i386");
	#endif
	#ifdef OPT_ALTIVEC
	printf(" AltiVec");
	#endif
	#ifdef OPT_GENERIC
	printf(" generic");
	#endif
	printf("\n");
}

void test_cpu_flags()
{
	if(is_i586())
	{
		unsigned int stdflags = 0;
		unsigned int std2flags = 0;
		unsigned int extflags = 0;
		stdflags = getstdcpuflags();
		std2flags = getstd2cpuflags();
		extflags = getextcpuflags();
		printf("CPU supports: i586");
		if(std2flags & FLAG2_MMX) printf(" mmx");
		if(extflags & XFLAG_3DNOW) printf(" 3dnow");
		if(extflags & XFLAG_3DNOWEXT) printf(" 3dnowext");
		if(std2flags & FLAG2_SSE) printf(" sse");
		if(std2flags & FLAG2_SSE2) printf(" sse2");
		if(stdflags & FLAG_SSE3) printf(" sse3");
		printf("\n");
	}
	else
	{
		printf("You have an i386 or i486... or perhaps a non-x86-32bit CPU?\n");
	}
}

int set_cpu_opt()
{
	unsigned int std2flags = 0;
	unsigned int extflags = 0;
	int auto_choose = 0;
	int done = 0;
	if(   (param.cpu == NULL)
	   || (param.cpu[0] == 0)
	   || !strcasecmp(param.cpu, "auto") )
	auto_choose = 1;

	/* covers any i386+ cpu; they actually differ only in the synth_1to1 function... */
	#ifdef OPT_X86

	#ifdef OPT_MMXORSSE
	cpu_opts.decwin = decwin;
	cpu_opts.make_decode_tables   = make_decode_tables;
	cpu_opts.init_layer3_gainpow2 = init_layer3_gainpow2;
	cpu_opts.init_layer2_table    = init_layer2_table;
	#endif

	if(is_i586())
	{
		std2flags = getstd2cpuflags();
		extflags = getextcpuflags();
		debug2("standard flags: 0x%08lx\textended flags: 0x%08lx\n", stdflags, extflags);
		#ifdef OPT_SSE
		if(!done && ((auto_choose && (std2flags & FLAG2_SSE) && (std2flags & FLAG2_MMX))|| !strcasecmp(param.cpu, "sse")) )
		{
			fprintf(stderr, "decoder: SSE\n");
			cpu_opts.synth_1to1 = synth_1to1_sse;
			cpu_opts.dct64 = dct64_mmx; /* only use the sse version in the synth_1to1_sse */
			cpu_opts.decwin = decwin_mmx;
			cpu_opts.make_decode_tables   = make_decode_tables_mmx;
			cpu_opts.init_layer3_gainpow2 = init_layer3_gainpow2_mmx;
			cpu_opts.init_layer2_table    = init_layer2_table_mmx;
			done = 1;
		}
		#endif
		#ifdef OPT_3DNOW
		cpu_opts.dct36 = dct36;
		/* TODO: make autodetection for _all_ x86 optimizations (maybe just for i586+ and keep separate 486 build?) */
		/* check cpuflags bit 31 (3DNow!) and 23 (MMX) */
		if(   !done && (auto_choose || !strcasecmp(param.cpu, "3dnow"))
	  	 && (param.stat_3dnow < 2)
	  	 && ((param.stat_3dnow == 1) || ((extflags & XFLAG_3DNOW) && (extflags & XFLAG_MMX))))
		{
			fprintf(stderr, "decoder: 3DNow! ... I hope it works with replacing the synth_1to1 in the *_i386 functions\n");
			cpu_opts.dct36 = dct36_3dnow; /* 3DNow! optimized dct36() */
			cpu_opts.synth_1to1 = synth_1to1_3dnow;
			cpu_opts.dct64 = dct64_i386; /* use the 3dnow one? */
			done = 1;
		}
		#endif
		#ifdef OPT_MMX
		if(!done && ((auto_choose && (std2flags & FLAG2_MMX))|| !strcasecmp(param.cpu, "mmx")))
		{
			fprintf(stderr, "decoder: MMX\n");
			cpu_opts.synth_1to1 = synth_1to1_mmx;
			cpu_opts.dct64 = dct64_mmx;
			cpu_opts.decwin = decwin_mmx;
			cpu_opts.make_decode_tables   = make_decode_tables_mmx;
			cpu_opts.init_layer3_gainpow2 = init_layer3_gainpow2_mmx;
			cpu_opts.init_layer2_table    = init_layer2_table_mmx;
			done = 1;
		}
		#endif
		#ifdef OPT_I586
		if(!done && (auto_choose || !strcasecmp(param.cpu, "i586")))
		{
			fprintf(stderr, "decoder: i586/pentium\n");
			cpu_opts.synth_1to1 = synth_1to1_i586;
			cpu_opts.synth_1to1_i586_asm = synth_1to1_i586_asm;
			cpu_opts.dct64 = dct64_i386;
			done = 1;
		}
		#endif
		#ifdef OPT_I586_DITHER
		if(!done && (auto_choose || !strcasecmp(param.cpu, "i586_dither")))
		{
			fprintf(stderr, "decoder: dithered i586/pentium\n");
			cpu_opts.synth_1to1 = synth_1to1_i586;
			cpu_opts.dct64 = dct64_i386;
			cpu_opts.synth_1to1_i586_asm = synth_1to1_i586_asm_dither;
			done = 1;
		}
	}
	#endif
	#ifdef OPT_I486 /* that won't cooperate nicely in multi opt mode - forcing i486 in layer3.c */
	if(!done && (auto_choose || !strcasecmp(param.cpu, "i486")))
	{
		fprintf(stderr, "decoder: i486\n");
		cpu_opts.synth_1to1 = synth_1to1_i386; /* i486 function is special */
		cpu_opts.dct64 = dct64_i386;
		done = 1;
	}
	#endif
	#ifdef OPT_I386
	if(!done && (auto_choose || !strcasecmp(param.cpu, "i386")))
	{
		fprintf(stderr, "decoder: i386\n");
		cpu_opts.synth_1to1 = synth_1to1_i386;
		cpu_opts.dct64 = dct64_i386;
		done = 1;
	}
	#endif
	if(done) /* set common x86 functions */
	{
		cpu_opts.synth_1to1_mono = synth_1to1_mono_i386;
		cpu_opts.synth_1to1_mono2stereo = synth_1to1_mono2stereo_i386;
		cpu_opts.synth_1to1_8bit = synth_1to1_8bit_i386;
		cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_mono_i386;
		cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_mono2stereo_i386;
	}
	#endif /* OPT_X86 */

	#ifdef OPT_ALTIVEC
	if(!done && (auto_choose || !strcasecmp(param.cpu, "altivec")))
	{
		fprintf(stderr, "decoder: AltiVec\n");
		cpu_opts.dct64 = dct64_altivec;
		cpu_opts.synth_1to1 = synth_1to1_altivec;
		cpu_opts.synth_1to1_mono = synth_1to1_mono_altivec;
		cpu_opts.synth_1to1_mono2stereo = synth_1to1_mono2stereo_altivec;
		cpu_opts.synth_1to1_8bit = synth_1to1_8bit_altivec;
		cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_mono_altivec;
		cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_mono2stereo_altivec;
		done = 1;
	}
	#endif

	#ifdef OPT_GENERIC
	if(!done && (auto_choose || !strcasecmp(param.cpu, "generic")))
	{
		fprintf(stderr, "decoder: generic\n");
		cpu_opts.dct64 = dct64;
		cpu_opts.synth_1to1 = synth_1to1;
		cpu_opts.synth_1to1_mono = synth_1to1_mono;
		cpu_opts.synth_1to1_mono2stereo = synth_1to1_mono2stereo;
		cpu_opts.synth_1to1_8bit = synth_1to1_8bit;
		cpu_opts.synth_1to1_8bit_mono = synth_1to1_8bit_mono;
		cpu_opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_mono2stereo;
		done = 1;
	}
	#endif

	if(!done)
	{
		error("Could not set optimization!");
		return 0;
	}
	else return 1;
}
#endif
