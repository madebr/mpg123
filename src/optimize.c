/*
	optimize: get a grip on the different optimizations

	copyright 2006 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.de
	initially written by Thomas Orgis, inspired by 3DNow stuff in mpg123.[hc]

	Currently, this file contains the struct and function to choose an optimization variant and works only when OPT_MULTI is in effect.
*/

#include "config.h"
#include "mpg123.h" /* includes optimize.h */
#ifdef OPT_MULTI

struct_opts opts;

int set_optmization()
{
	long flags = 0;
	int auto_choose = 0;
	int done = 0;
	if(   (param.cpu == NULL)
	   || (param.cpu[0] == 0)
	   || !strcasecmp(param.cpu, "auto") )
	auto_choose = 1;

	/* covers any i386+ cpu; they actually differ only in the synth_1to1 function... */
	#ifdef OPT_X86
	flags = getcpuflags();
	#define FLAG_MMX   0x00800000
	#define FLAG_3DNOW 0x80000000

	/* Place SSE option here! */

	#ifdef OPT_3DNOW
	opts.dct36 = dct36;
	/* TODO: make autodetection for _all_ x86 optimizations (maybe just for i586+ and keep separate 486 build?) */
	/* check cpuflags bit 31 (3DNow!) and 23 (MMX) */
	if(   !done && (auto_choose || !strcasecmp(param.cpu, "3dnow"))
	   && (param.stat_3dnow < 2)
	   && ((param.stat_3dnow == 1) || ((flags & FLAG_3DNOW) && (flags & FLAG_MMX))))
	{
		debug("decoder: 3DNow! ... I hope it works with replacing the synth_1to1 in the *_i386 functions");
		opts.dct36 = dct36_3dnow; /* 3DNow! optimized dct36() */
		opts.synth_1to1 = synth_1to1_3dnow;
		opts.dct64 = dct64_i386; /* use the 3dnow one? */
		done = 1;
	}
	#endif
	#ifdef OPT_MMX
	if(   !done && (auto_choose || !strcasecmp(param.cpu, "mmx"))
	   && (flags & FLAG_MMX) )
	{
		debug("decoder: MMX");
		opts.synth_1to1 = synth_1to1_mmx;
		opts.dct64 = dct64_mmx;
		opts.decwin = decwin_mmx;
		opts.make_decode_tables   = make_decode_tables_mmx;
		opts.init_layer3_gainpow2 = init_layer3_gainpow2_mmx;
		opts.init_layer2_table    = init_layer2_table_mmx;
		done = 1;
	}
	else
	{
		opts.decwin = decwin;
		opts.make_decode_tables   = make_decode_tables;
		opts.init_layer3_gainpow2 = init_layer3_gainpow2;
		opts.init_layer2_table    = init_layer2_table;
	}
	#endif
	#ifdef OPT_I586
	if(!done && (auto_choose || !strcasecmp(param.cpu, "i586")))
	{
		debug("decoder: i586/pentium");
		opts.synth_1to1 = synth_1to1_i586;
		opts.synth_1to1_i586_asm = synth_1to1_i586_asm;
		opts.dct64 = dct64_i386;
		done = 1;
	}
	#endif
	#ifdef OPT_I586_DITHER
	if(!done && (auto_choose || !strcasecmp(param.cpu, "i586_dither")))
	{
		debug("decoder: dithered i586/pentium");
		opts.synth_1to1 = synth_1to1_i586_dither;
		opts.dct64 = dct64_i386;
		opts.synth_1to1_i586_asm = synth_1to1_i586_asm_dither;
		done = 1;
	}
	#endif
	#ifdef OPT_I486 /* that won't cooperate nicely in multi opt mode - forcing i486 in layer3.c */
	if(!done && (auto_choose || !strcasecmp(param.cpu, "i486")))
	{
		debug("decoder: i486");
		opts.synth_1to1 = synth_1to1_i386; /* i486 function is special */
		opts.dct64 = dct64_i386;
		done = 1;
	}
	#endif
	#ifdef OPT_I386
	if(!done && (auto_choose || !strcasecmp(param.cpu, "i386")))
	{
		debug("decoder: i386");
		opts.synth_1to1 = synth_1to1_i386;
		opts.dct64 = dct64_i386;
		done = 1;
	}
	#endif
	if(done) /* set common x86 functions */
	{
		opts.synth_1to1_mono = synth_1to1_mono_i386;
		opts.synth_1to1_mono2stereo = synth_1to1_mono2stereo_i386;
		opts.synth_1to1_8bit = synth_1to1_8bit_i386;
		opts.synth_1to1_8bit_mono = synth_1to1_8bit_mono_i386;
		opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_mono2stereo_i386;
	}
	#endif /* OPT_X86 */

	#ifdef OPT_ALTIVEC
	if(!done && (auto_choose || !strcasecmp(param.cpu, "altivec")))
	{
		debug("decoder: AltiVec");
		opts.dct64 = dct64_altivec;
		opts.synth_1to1 = synth_1to1_altivec;
		opts.synth_1to1_mono = synth_1to1_mono_altivec;
		opts.synth_1to1_mono2stereo = synth_1to1_mono2stereo_altivec;
		opts.synth_1to1_8bit = synth_1to1_8bit_altivec;
		opts.synth_1to1_8bit_mono = synth_1to1_8bit_mono_altivec;
		opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_mono2stereo_altivec;
		done = 1;
	}
	#endif

	#ifdef OPT_GENERIC
	if(!done && (auto_choose || !strcasecmp(param.cpu, "generic")))
	{
		debug("decoder: generic");
		opts.dct64 = dct64;
		opts.synth_1to1 = synth_1to1;
		opts.synth_1to1_mono = synth_1to1_mono;
		opts.synth_1to1_mono2stereo = synth_1to1_mono2stereo;
		opts.synth_1to1_8bit = synth_1to1_8bit;
		opts.synth_1to1_8bit_mono = synth_1to1_8bit_mono;
		opts.synth_1to1_8bit_mono2stereo = synth_1to1_8bit_mono2stereo;
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
