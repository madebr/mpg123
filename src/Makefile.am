## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://mpg123.org
## initially written by Nicholas J. Humfrey

AM_CFLAGS = @AUDIO_CFLAGS@
#AM_LDFLAGS = 
mpg123_LDADD = @AUDIO_OBJ@ @CPU_TYPE_LIB@ @AUDIO_LIBS@
mpg123_DEPENDENCIES = @AUDIO_OBJ@ @CPU_TYPE_LIB@

EXTRA_DIST = Makefile.legacy config.h.legacy

CLEANFILES = *.a

bin_PROGRAMS = mpg123
EXTRA_PROGRAMS = testcpu

testcpu_SOURCES = \
	testcpu.c \
	getcpuflags.S
	
mpg123_SOURCES = \
	audio.c \
	audio.h \
	buffer.c \
	buffer.h \
	common.c \
	common.h \
	control_generic.c \
	debug.h \
	decode.h \
	decode_2to1.c \
	decode_4to1.c \
	decode_ntom.c \
	equalizer.c \
	getbits.c \
	getbits.h \
	getlopt.c \
	getlopt.h \
	httpget.c \
	huffman.h \
	icy.c \
	icy.h \
	id3.c \
	id3.h \
	genre.h \
	l2tables.h \
	layer1.c \
	layer2.c \
	layer3.c \
	layer3.h \
	mangle.h \
	mpg123.c \
	mpg123.h \
	optimize.h \
	optimize.c \
	playlist.c \
	playlist.h \
	readers.c \
	tabinit.c \
	sfifo.c \
	sfifo.h \
	stringbuf.c \
	stringbuf.h \
	term.c \
	term.h \
	wav.c \
	xfermem.c \
	xfermem.h

EXTRA_mpg123_SOURCES = \
	audio_aix.c \
	audio_alib.c \
	audio_alsa.c \
	audio_dummy.c \
	audio_esd.c \
	audio_hp.c \
	audio_jack.c \
	audio_coreaudio.c \
	audio_mint.c \
	audio_nas.c \
	audio_os2.c \
	audio_oss.c \
	audio_portaudio.c \
	audio_sdl.c \
	audio_sgi.c \
	audio_sun.c \
	audio_win32.c \
	getcpuflags.h


# Processor dependent decoder code
EXTRA_LIBRARIES = \
	libcpu_generic.a \
	libcpu_generic_fpu.a \
	libcpu_generic_nofpu.a \
	libcpu_altivec.a \
	libcpu_i386.a \
	libcpu_i386_fpu.a \
	libcpu_i386_nofpu.a \
	libcpu_i486.a \
	libcpu_i586.a \
	libcpu_i586_dither.a \
	libcpu_3dnow.a \
	libcpu_3dnowext.a \
	libcpu_3dnowext_alone.a \
	libcpu_mmx.a \
	libcpu_mmx_alone.a \
	libcpu_sse.a \
	libcpu_sse_alone.a \
	libcpu_x86.a \
	libcpu_x86_dither.a

libcpu_generic_a_SOURCES = \
	decode.c \
	dct64.c

libcpu_generic_fpu_a_SOURCES = \
	decode.c \
	dct64.c

libcpu_generic_nofpu_a_SOURCES = \
	decode.c \
	dct64.c

libcpu_altivec_a_SOURCES = \
	decode_altivec.c \
	dct64_altivec.c

libcpu_i386_a_SOURCES = \
	decode_i386.c \
	dct64_i386.c

libcpu_i386_fpu_a_SOURCES = \
	decode_i386.c \
	dct64_i386.c

libcpu_i386_nofpu_a_SOURCES = \
	decode_i386.c \
	dct64_i386.c

libcpu_i486_a_SOURCES = \
	decode_i386.c \
	dct64_i386.c \
	decode_i486.c \
	dct64_i486.c

libcpu_i586_a_SOURCES = \
	decode_i386.c \
	dct64_i386.c \
	decode_i586.S

libcpu_i586_dither_a_SOURCES = \
	decode_i386.c \
	dct64_i386.c \
	dnoise.c \
	decode_i586_dither.S

libcpu_3dnow_a_SOURCES = \
	decode_i386.c \
	decode_3dnow.S \
	decode_i586.S \
	dct64_i386.c \
	dct64_3dnow.S \
	dct36_3dnow.S \
	equalizer_3dnow.S \
	getcpuflags.S

libcpu_mmx_alone_a_SOURCES = \
	decode_i386.c \
	dct64_mmx.S \
	tabinit_mmx.S \
	decode_mmx.S

libcpu_mmx_a_SOURCES = \
	decode_i386.c \
	dct64_mmx.S \
	tabinit_mmx.S \
	decode_mmx.S \
	dct64_i386.c \
	decode_i586.S \
	getcpuflags.S

libcpu_sse_alone_a_SOURCES = \
	decode_i386.c \
	dct64_sse.S \
	dct64_mmx.S \
	tabinit_mmx.S \
	decode_mmxsse.S

libcpu_sse_a_SOURCES = \
	decode_i386.c \
	dct64_sse.S \
	dct64_mmx.S \
	tabinit_mmx.S \
	decode_mmxsse.S \
	dct64_i386.c \
	decode_i586.S \
	getcpuflags.S

libcpu_3dnowext_alone_a_SOURCES = \
	decode_i386.c \
	dct64_3dnowext.S \
	dct36_3dnowext.S \
	dct64_mmx.S \
	tabinit_mmx.S \
	decode_mmxsse.S

libcpu_3dnowext_a_SOURCES = \
	decode_i386.c \
	dct64_3dnowext.S \
	dct36_3dnowext.S \
	dct64_mmx.S \
	tabinit_mmx.S \
	decode_mmxsse.S \
	decode_3dnow.S \
	dct64_i386.c \
	dct64_3dnow.S \
	dct36_3dnow.S \
	equalizer_3dnow.S \
	getcpuflags.S

libcpu_x86_dither_a_SOURCES = \
	decode.c \
	decode_i386.c \
	decode_i586.S \
	dnoise.c \
	decode_i586_dither.S \
	decode_3dnow.S \
	decode_mmx.S \
	decode_mmxsse.S \
	dct64.c \
	dct64_i386.c \
	dct64_sse.S \
	dct64_mmx.S \
	dct64_3dnow.S \
	dct36_3dnow.S \
	dct64_3dnowext.S \
	dct36_3dnowext.S \
	tabinit_mmx.S \
	equalizer_3dnow.S \
	getcpuflags.S

libcpu_x86_a_SOURCES = \
	decode.c \
	decode_i386.c \
	decode_i586.S \
	decode_3dnow.S \
	decode_mmx.S \
	decode_mmxsse.S \
	dct64.c \
	dct64_i386.c \
	dct64_sse.S \
	dct64_mmx.S \
	dct64_3dnow.S \
	dct36_3dnow.S \
	dct64_3dnowext.S \
	dct36_3dnowext.S \
	tabinit_mmx.S \
	equalizer_3dnow.S \
	getcpuflags.S

# explicit preprocessing since mingw32 does not honor the big .S
.S.o:
	$(CPP) $(AM_CPPFLAGS) $(DEFAULT_INCLUDES) $(CPPFLAGS) $< > $<.s
	$(CCAS) $(CCASFLAGS) -c -o $@ $<.s && rm $<.s
