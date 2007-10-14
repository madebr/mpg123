## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://mpg123.org
## initially written by Nicholas J. Humfrey

#AM_CFLAGS = @AUDIO_CFLAGS@
#AM_LDFLAGS = 

EXTRA_DIST =

CLEANFILES = *.a


#lib_LIBRARIES = libmpg123.a
lib_LTLIBRARIES = libmpg123.la
include_HEADERS = mpg123.h

#libmpg123_a_LIBADD = @DECODER_OBJ@
#libmpg123_a_DEPENDENCIES = @DECODER_OBJ@

libmpg123_la_LDFLAGS = -no-undefined -version-info @LIBMPG123_VERSION@ -export-symbols libmpg123.sym
libmpg123_la_LIBADD = @DECODER_LOBJ@
libmpg123_la_DEPENDENCIES = @DECODER_LOBJ@

libmpg123_la_SOURCES = \
	parse.c \
	parse.h \
	frame.c \
	format.c \
	frame.h \
	reader.h \
	debug.h \
	decode.h \
	decode_2to1.c \
	decode_4to1.c \
	decode_ntom.c \
	equalizer.c \
	huffman.h \
	icy.c \
	icy.h \
	id3.c \
	id3.h \
	true.h \
	l2tables.h \
	layer1.c \
	layer2.c \
	layer3.c \
	getbits.h \
	optimize.h \
	optimize.c \
	readers.c \
	tabinit.c \
	stringbuf.c \
	libmpg123.c \
	mpg123.h \
	mpg123lib_intern.h \
	mangle.h \
	getcpuflags.h \
	libmpg123.sym

EXTRA_libmpg123_la_SOURCES = \
	dct36_3dnowext.S \
	dct36_3dnow.S \
	dct64_3dnowext.S \
	dct64_3dnow.S \
	dct64_altivec.c \
	dct64.c \
	dct64_i386.c \
	dct64_mmx.S \
	dct64_sse.S \
	decode_3dnowext.S \
	decode_3dnow.S \
	decode_altivec.c \
	decode.c \
	decode_i386.c \
	decode_i586_dither.S \
	decode_i586.S \
	decode_mmx.S \
	decode_sse3d.h \
	decode_sse.S \
	dnoise.c \
	equalizer_3dnow.S \
	tabinit_mmx.S \
	getcpuflags.S

# explicit preprocessing since mingw32 does not honor the big .S
.S.o:
	$(CPP) $(CPPFLAGS) $< > $<.s
	$(CCAS) $(CCASFLAGS) -c -o $@ $<.s && rm $<.s
