## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://mpg123.org
## initially written by Nicholas J. Humfrey

mpg123_LDADD = @LIBLTDL@ libmpg123/libmpg123.la
mpg123_LDFLAGS = -export-dynamic
mpg123_INCLUDES = -Ilibmpg123 @LTDLINCL@
mpg123_DEPENDENCIES = @LIBLTDL@ libmpg123/libmpg123.la

SUBDIRS = output libmpg123
EXTRA_DIST = Makefile.legacy config.h.legacy

CLEANFILES = *.a

bin_PROGRAMS = mpg123
EXTRA_PROGRAMS = mpglib

mpglib_LDADD = libmpg123/libmpg123.la
mpglib_DEPENDENCIES = libmpg123/libmpg123.la

mpglib_SOURCES = \
	mpglib.c

mpg123_SOURCES = \
	audio.c \
	audio.h \
	buffer.c \
	buffer.h \
	common.c \
	common.h \
	control_generic.c \
	getlopt.c \
	getlopt.h \
	httpget.c \
	httpget.h \
	genre.h \
	genre.c \
	module.c \
	module.h \
	mpg123.c \
	mpg123app.h \
	id3print.c \
	id3print.h \
	playlist.c \
	playlist.h \
	sfifo.c \
	sfifo.h \
	term.c \
	term.h \
	wav.c \
	xfermem.c \
	xfermem.h
