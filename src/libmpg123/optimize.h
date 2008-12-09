#ifndef MPG123_H_OPTIMIZE
#define MPG123_H_OPTIMIZE
/*
	optimize: get a grip on the different optimizations

	copyright 2007 by the mpg123 project - free software under the terms of the LGPL 2.1
	see COPYING and AUTHORS files in distribution or http://mpg123.org
	initially written by Thomas Orgis, taking from mpg123.[hc]

	for building mpg123 with one optimization only, you have to choose exclusively between
	OPT_GENERIC (generic C code for everyone)
	OPT_GENERIC_DITHER (same with dithering for 1to1)
	OPT_I386 (Intel i386)
	OPT_I486 (Somewhat special code for i486; does not work together with others.)
	OPT_I586 (Intel Pentium)
	OPT_I586_DITHER (Intel Pentium with dithering/noise shaping for enhanced quality)
	OPT_MMX (Intel Pentium and compatibles with MMX, fast, but not the best accuracy)
	OPT_3DNOW (AMD 3DNow!, K6-2/3, Athlon, compatibles...)
	OPT_3DNOWEXT (AMD 3DNow! extended, generally Athlon, compatibles...)
	OPT_ALTIVEC (Motorola/IBM PPC with AltiVec under MacOSX)

	or you define OPT_MULTI and give a combination which makes sense (do not include i486, do not mix altivec and x86).

	I still have to examine the dynamics of this here together with REAL_IS_FIXED.
	Basic point is: Don't use REAL_IS_FIXED with something else than generic or i386.

	Also, one should minimize code size by really ensuring that only functions that are really needed are included.
	Currently, all generic functions will be always there (to be safe for fallbacks for advanced decoders).
	Strictly, at least the synth_1to1 should not be necessary for single-decoder mode.
*/


/* Runtime optimization interface now here: */

enum optdec
{
	autodec=0, nodec, generic, generic_dither, idrei,
	ivier, ifuenf, ifuenf_dither, mmx,
	dreidnow, dreidnowext, altivec, sse
};
enum optcla { nocla=0, normal, mmxsse };

/*  - Set up the table of synth functions for current decoder choice. */
int frame_cpu_opt(mpg123_handle *fr, const char* cpu);
/*  - Choose, from the synth table, the synth functions to use for current output format/rate. */
int set_synth_functions(mpg123_handle *fr);
/*  - Parse decoder name and return numerical code. */
enum optdec dectype(const char* decoder);
/*  - Return the default decoder type. */
enum optdec defdec(void);
/*  - Return the class of a decoder type (mmxsse or normal). */
enum optcla decclass(const enum optdec);

/* Now comes a whole lot of definitions, for multi decoder mode and single decoder mode.
   Because of the latter, it may look redundant at times. */

/* this is included in mpg123.h, which includes config.h */
#ifdef CCALIGN
#define ALIGNED(a) __attribute__((aligned(a)))
#else
#define ALIGNED(a)
#endif

/* Safety catch for invalid decoder choice. */
#ifdef REAL_IS_FIXED
#if (defined OPT_I486)  || (defined OPT_I586) || (defined OPT_I586_DITHER) \
 || (defined OPT_MMX)   || (defined OPT_SSE)  || (defined_OPT_ALTIVEC) \
 || (defined OPT_3DNOW) || (defined OPT_3DNOWEXT) || (defined OPT_GENERIC_DITHER)
#error "Bad decoder choice together with fixed point math!"
#endif
#endif

#ifdef OPT_GENERIC
#ifndef OPT_MULTI
#	define defopt generic
#endif
#endif

#ifdef OPT_GENERIC_DITHER
#define OPT_DITHER
#ifndef OPT_MULTI
#	define defopt generic_dither
#	define opt_synth_1to1(fr) synth_1to1_dither
#	define opt_synth_2to1(fr) synth_2to1_dither
#	define opt_synth_4to1(fr) synth_4to1_dither
#endif
#endif

/* i486 is special... always alone! */
#ifdef OPT_I486
#define OPT_X86
#define defopt ivier
#ifdef OPT_MULTI
#error "i486 can only work alone!"
#endif
#define FIR_BUFFER_SIZE  128
#define FIR_SIZE 16
#endif

#ifdef OPT_I386
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt idrei
#endif
#endif

#ifdef OPT_I586
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt ifuenf
#	define opt_synth_1to1(fr) synth_1to1_i586
#endif
#endif

#ifdef OPT_I586_DITHER
#define OPT_X86
#define OPT_DITHER
#ifndef OPT_MULTI
#	define defopt ifuenf_dither
#	define opt_synth_1to1(fr) synth_1to1_i586_dither
#endif
#endif

/* We still have some special code around MMX tables. */

#ifdef OPT_MMX
#define OPT_MMXORSSE
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt mmx
#	define opt_synth_1to1(fr) synth_1to1_mmx
#endif
#endif

#ifdef OPT_SSE
#define OPT_MMXORSSE
#define OPT_MPLAYER
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt sse
#	define opt_synth_1to1(fr) synth_1to1_sse
#endif
#endif

#ifdef OPT_3DNOWEXT
#define OPT_MMXORSSE
#define OPT_MPLAYER
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt dreidnowext
#	define opt_dct36(fr) dct36_3dnowext
#	define opt_synth_1to1(fr) synth_1to1_3dnowext
#endif
#endif

#ifdef OPT_MPLAYER
extern const int costab_mmxsse[];
#endif

/* 3dnow used to use synth_1to1_i586 for mono / 8bit conversion - was that intentional? */
/* I'm trying to skip the pentium code here ... until I see that that is indeed a bad idea */
#ifdef OPT_3DNOW
#define OPT_X86
#ifndef OPT_MULTI
#	define defopt dreidnow
#	define opt_dct36(fr) dct36_3dnow
#	define opt_synth_1to1(fr) synth_1to1_3dnow
#endif
#endif

#ifdef OPT_ALTIVEC
#ifndef OPT_MULTI
#	define defopt altivec
#	define opt_synth_1to1(fr) synth_1to1_altivec
#endif
#endif

/* used for multi opt mode and the single 3dnow mode to have the old 3dnow test flag still working */
void check_decoders(void);

/* Announce the data in dnoise.c ... */
#ifdef OPT_DITHER
#define DITHERSIZE 65536
extern float dithernoise[DITHERSIZE];
#endif

/*
	Now come two blocks of standard definitions for multi-decoder mode and single-decoder mode.
	Most stuff is so automatic that it's indeed generated by some inline shell script.
	Remember to use these scripts when possible, instead of direct repetitive hacking.
*/

#ifdef OPT_MULTI

#	define defopt nodec

/*
	## This is an inline bourne shell script for execution in nedit to generate the lines below.
	## The ## is a quote for just #
	star="*"; slash="/"; 
	for i in 1to1 2to1 4to1 ntom;
	do
		echo
		echo "$slash$star $i $star$slash"
		for t in "" _8bit _real; do for f in "" _mono _mono2stereo;
		do
			echo "##	define opt_synth_${i}${t}${f}(fr) ((fr)->cpu_opts.synth_${i}${t}${f})"
		done; done
	done
*/

/* 1to1 */
#	define opt_synth_1to1(fr) ((fr)->cpu_opts.synth_1to1)
#	define opt_synth_1to1_mono(fr) ((fr)->cpu_opts.synth_1to1_mono)
#	define opt_synth_1to1_mono2stereo(fr) ((fr)->cpu_opts.synth_1to1_mono2stereo)
#	define opt_synth_1to1_8bit(fr) ((fr)->cpu_opts.synth_1to1_8bit)
#	define opt_synth_1to1_8bit_mono(fr) ((fr)->cpu_opts.synth_1to1_8bit_mono)
#	define opt_synth_1to1_8bit_mono2stereo(fr) ((fr)->cpu_opts.synth_1to1_8bit_mono2stereo)
#	define opt_synth_1to1_real(fr) ((fr)->cpu_opts.synth_1to1_real)
#	define opt_synth_1to1_real_mono(fr) ((fr)->cpu_opts.synth_1to1_real_mono)
#	define opt_synth_1to1_real_mono2stereo(fr) ((fr)->cpu_opts.synth_1to1_real_mono2stereo)

/* 2to1 */
#	define opt_synth_2to1(fr) ((fr)->cpu_opts.synth_2to1)
#	define opt_synth_2to1_mono(fr) ((fr)->cpu_opts.synth_2to1_mono)
#	define opt_synth_2to1_mono2stereo(fr) ((fr)->cpu_opts.synth_2to1_mono2stereo)
#	define opt_synth_2to1_8bit(fr) ((fr)->cpu_opts.synth_2to1_8bit)
#	define opt_synth_2to1_8bit_mono(fr) ((fr)->cpu_opts.synth_2to1_8bit_mono)
#	define opt_synth_2to1_8bit_mono2stereo(fr) ((fr)->cpu_opts.synth_2to1_8bit_mono2stereo)
#	define opt_synth_2to1_real(fr) ((fr)->cpu_opts.synth_2to1_real)
#	define opt_synth_2to1_real_mono(fr) ((fr)->cpu_opts.synth_2to1_real_mono)
#	define opt_synth_2to1_real_mono2stereo(fr) ((fr)->cpu_opts.synth_2to1_real_mono2stereo)

/* 4to1 */
#	define opt_synth_4to1(fr) ((fr)->cpu_opts.synth_4to1)
#	define opt_synth_4to1_mono(fr) ((fr)->cpu_opts.synth_4to1_mono)
#	define opt_synth_4to1_mono2stereo(fr) ((fr)->cpu_opts.synth_4to1_mono2stereo)
#	define opt_synth_4to1_8bit(fr) ((fr)->cpu_opts.synth_4to1_8bit)
#	define opt_synth_4to1_8bit_mono(fr) ((fr)->cpu_opts.synth_4to1_8bit_mono)
#	define opt_synth_4to1_8bit_mono2stereo(fr) ((fr)->cpu_opts.synth_4to1_8bit_mono2stereo)
#	define opt_synth_4to1_real(fr) ((fr)->cpu_opts.synth_4to1_real)
#	define opt_synth_4to1_real_mono(fr) ((fr)->cpu_opts.synth_4to1_real_mono)
#	define opt_synth_4to1_real_mono2stereo(fr) ((fr)->cpu_opts.synth_4to1_real_mono2stereo)

/* ntom */
#	define opt_synth_ntom(fr) ((fr)->cpu_opts.synth_ntom)
#	define opt_synth_ntom_mono(fr) ((fr)->cpu_opts.synth_ntom_mono)
#	define opt_synth_ntom_mono2stereo(fr) ((fr)->cpu_opts.synth_ntom_mono2stereo)
#	define opt_synth_ntom_8bit(fr) ((fr)->cpu_opts.synth_ntom_8bit)
#	define opt_synth_ntom_8bit_mono(fr) ((fr)->cpu_opts.synth_ntom_8bit_mono)
#	define opt_synth_ntom_8bit_mono2stereo(fr) ((fr)->cpu_opts.synth_ntom_8bit_mono2stereo)
#	define opt_synth_ntom_real(fr) ((fr)->cpu_opts.synth_ntom_real)
#	define opt_synth_ntom_real_mono(fr) ((fr)->cpu_opts.synth_ntom_real_mono)
#	define opt_synth_ntom_real_mono2stereo(fr) ((fr)->cpu_opts.synth_ntom_real_mono2stereo)

/* End of generated output. */

#	ifdef OPT_3DNOW
#		define opt_dct36(fr) ((fr)->cpu_opts.dct36)
#	endif

#else /* OPT_MULTI */

/* Define missing opt functions, for generic or x86. */
#	ifdef opt_synth_1to1
/* If there is an optimized 1to1, we'll reuse it for 8bit stuff. */
#		ifndef opt_synth_1to1_8bit
#			define opt_synth_1to1_8bit(fr)               synth_1to1_8bit_wrap
#		endif
#		ifndef opt_synth_1to1_8bit_mono
#				define opt_synth_1to1_8bit_mono(fr)        synth_1to1_8bit_wrap_mono
#		endif
#		ifndef opt_synth_1to1_8bit_mono2stereo
#				define opt_synth_1to1_8bit_mono2stereo(fr) synth_1to1_8bit_wrap_mono2stereo
#		endif
#	endif

/*
	## This is an inline bourne shell script for execution in nedit to generate the lines below.
	## The ## is a quote for just #
	star="*"; slash="/"; 
	for c in "ifdef OPT_X86" "else $slash$star generic code $star$slash"
	do
		if test "$c" = "ifdef OPT_X86"; then cpu=_i386; else cpu=; fi
		echo "##	$c"
		for i in 1to1 2to1 4to1 ntom;
		do
			if test $i = ntom; then cpu=; fi
			echo "$slash$star $i $star$slash"
			for t in "" _8bit _real; do
				echo "##		ifndef opt_synth_${i}${t}"
				echo "##			define opt_synth_${i}${t}(fr) synth_${i}${t}$cpu"
				echo "##		endif"
			done
		done
	done
	echo "##	endif $slash$star x86 / generic $star$slash"
*/
#	ifdef OPT_X86
/* 1to1 */
#		ifndef opt_synth_1to1
#			define opt_synth_1to1(fr) synth_1to1_i386
#		endif
#		ifndef opt_synth_1to1_8bit
#			define opt_synth_1to1_8bit(fr) synth_1to1_8bit_i386
#		endif
#		ifndef opt_synth_1to1_real
#			define opt_synth_1to1_real(fr) synth_1to1_real_i386
#		endif
/* 2to1 */
#		ifndef opt_synth_2to1
#			define opt_synth_2to1(fr) synth_2to1_i386
#		endif
#		ifndef opt_synth_2to1_8bit
#			define opt_synth_2to1_8bit(fr) synth_2to1_8bit_i386
#		endif
#		ifndef opt_synth_2to1_real
#			define opt_synth_2to1_real(fr) synth_2to1_real_i386
#		endif
/* 4to1 */
#		ifndef opt_synth_4to1
#			define opt_synth_4to1(fr) synth_4to1_i386
#		endif
#		ifndef opt_synth_4to1_8bit
#			define opt_synth_4to1_8bit(fr) synth_4to1_8bit_i386
#		endif
#		ifndef opt_synth_4to1_real
#			define opt_synth_4to1_real(fr) synth_4to1_real_i386
#		endif
/* ntom */
#		ifndef opt_synth_ntom
#			define opt_synth_ntom(fr) synth_ntom
#		endif
#		ifndef opt_synth_ntom_8bit
#			define opt_synth_ntom_8bit(fr) synth_ntom_8bit
#		endif
#		ifndef opt_synth_ntom_real
#			define opt_synth_ntom_real(fr) synth_ntom_real
#		endif
#	else /* generic code */
/* 1to1 */
#		ifndef opt_synth_1to1
#			define opt_synth_1to1(fr) synth_1to1
#		endif
#		ifndef opt_synth_1to1_8bit
#			define opt_synth_1to1_8bit(fr) synth_1to1_8bit
#		endif
#		ifndef opt_synth_1to1_real
#			define opt_synth_1to1_real(fr) synth_1to1_real
#		endif
/* 2to1 */
#		ifndef opt_synth_2to1
#			define opt_synth_2to1(fr) synth_2to1
#		endif
#		ifndef opt_synth_2to1_8bit
#			define opt_synth_2to1_8bit(fr) synth_2to1_8bit
#		endif
#		ifndef opt_synth_2to1_real
#			define opt_synth_2to1_real(fr) synth_2to1_real
#		endif
/* 4to1 */
#		ifndef opt_synth_4to1
#			define opt_synth_4to1(fr) synth_4to1
#		endif
#		ifndef opt_synth_4to1_8bit
#			define opt_synth_4to1_8bit(fr) synth_4to1_8bit
#		endif
#		ifndef opt_synth_4to1_real
#			define opt_synth_4to1_real(fr) synth_4to1_real
#		endif
/* ntom */
#		ifndef opt_synth_ntom
#			define opt_synth_ntom(fr) synth_ntom
#		endif
#		ifndef opt_synth_ntom_8bit
#			define opt_synth_ntom_8bit(fr) synth_ntom_8bit
#		endif
#		ifndef opt_synth_ntom_real
#			define opt_synth_ntom_real(fr) synth_ntom_real
#		endif
#	endif /* x86 / generic */

/* Common mono stuff, wrapping over possibly optimized basic synth. */
/*
	## This is an inline bourne shell script for execution in nedit to generate the lines below.
	## The ## is a quote for just #
	for i in 1to1 2to1 4to1 ntom; do
	star="*"; slash="/"; echo "$slash$star $i mono $star$slash"
	for t in "" _8bit _real; do for m in mono mono2stereo; do
	echo "##	ifndef opt_synth_${i}${t}_${m}"
	echo "##		define opt_synth_${i}${t}_${m}(fr) synth_${i}${t}_${m}"
	echo "##	endif"
	done; done; done
*/
/* 1to1 mono */
#	ifndef opt_synth_1to1_mono
#		define opt_synth_1to1_mono(fr) synth_1to1_mono
#	endif
#	ifndef opt_synth_1to1_mono2stereo
#		define opt_synth_1to1_mono2stereo(fr) synth_1to1_mono2stereo
#	endif
#	ifndef opt_synth_1to1_8bit_mono
#		define opt_synth_1to1_8bit_mono(fr) synth_1to1_8bit_mono
#	endif
#	ifndef opt_synth_1to1_8bit_mono2stereo
#		define opt_synth_1to1_8bit_mono2stereo(fr) synth_1to1_8bit_mono2stereo
#	endif
#	ifndef opt_synth_1to1_real_mono
#		define opt_synth_1to1_real_mono(fr) synth_1to1_real_mono
#	endif
#	ifndef opt_synth_1to1_real_mono2stereo
#		define opt_synth_1to1_real_mono2stereo(fr) synth_1to1_real_mono2stereo
#	endif
/* 2to1 mono */
#	ifndef opt_synth_2to1_mono
#		define opt_synth_2to1_mono(fr) synth_2to1_mono
#	endif
#	ifndef opt_synth_2to1_mono2stereo
#		define opt_synth_2to1_mono2stereo(fr) synth_2to1_mono2stereo
#	endif
#	ifndef opt_synth_2to1_8bit_mono
#		define opt_synth_2to1_8bit_mono(fr) synth_2to1_8bit_mono
#	endif
#	ifndef opt_synth_2to1_8bit_mono2stereo
#		define opt_synth_2to1_8bit_mono2stereo(fr) synth_2to1_8bit_mono2stereo
#	endif
#	ifndef opt_synth_2to1_real_mono
#		define opt_synth_2to1_real_mono(fr) synth_2to1_real_mono
#	endif
#	ifndef opt_synth_2to1_real_mono2stereo
#		define opt_synth_2to1_real_mono2stereo(fr) synth_2to1_real_mono2stereo
#	endif
/* 4to1 mono */
#	ifndef opt_synth_4to1_mono
#		define opt_synth_4to1_mono(fr) synth_4to1_mono
#	endif
#	ifndef opt_synth_4to1_mono2stereo
#		define opt_synth_4to1_mono2stereo(fr) synth_4to1_mono2stereo
#	endif
#	ifndef opt_synth_4to1_8bit_mono
#		define opt_synth_4to1_8bit_mono(fr) synth_4to1_8bit_mono
#	endif
#	ifndef opt_synth_4to1_8bit_mono2stereo
#		define opt_synth_4to1_8bit_mono2stereo(fr) synth_4to1_8bit_mono2stereo
#	endif
#	ifndef opt_synth_4to1_real_mono
#		define opt_synth_4to1_real_mono(fr) synth_4to1_real_mono
#	endif
#	ifndef opt_synth_4to1_real_mono2stereo
#		define opt_synth_4to1_real_mono2stereo(fr) synth_4to1_real_mono2stereo
#	endif
/* ntom mono */
#	ifndef opt_synth_ntom_mono
#		define opt_synth_ntom_mono(fr) synth_ntom_mono
#	endif
#	ifndef opt_synth_ntom_mono2stereo
#		define opt_synth_ntom_mono2stereo(fr) synth_ntom_mono2stereo
#	endif
#	ifndef opt_synth_ntom_8bit_mono
#		define opt_synth_ntom_8bit_mono(fr) synth_ntom_8bit_mono
#	endif
#	ifndef opt_synth_ntom_8bit_mono2stereo
#		define opt_synth_ntom_8bit_mono2stereo(fr) synth_ntom_8bit_mono2stereo
#	endif
#	ifndef opt_synth_ntom_real_mono
#		define opt_synth_ntom_real_mono(fr) synth_ntom_real_mono
#	endif
#	ifndef opt_synth_ntom_real_mono2stereo
#		define opt_synth_ntom_real_mono2stereo(fr) synth_ntom_real_mono2stereo
#	endif

/* End of generated output. */

#	ifndef opt_dct36
#		define opt_dct36(fr) dct36
#	endif

#endif /* OPT_MULTI else */

#endif /* MPG123_H_OPTIMIZE */

