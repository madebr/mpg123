## Makefile.am: produce Makefile.in from this

## copyright by the mpg123 project - free software under the terms of the LGPL 2.1
## see COPYING and AUTHORS files in distribution or http://mpg123.org
## initially written by Nicholas J. Humfrey
dist_man_MANS = man1/mpg123.1

# libltdl first as this recursive make mess cannot resolve interdependencies
# Why is recursive make the standard procedure when MAKE DOES NOT WORK THAT WAY??!
SUBDIRS = @LTDLDIR@ src doc
DIST_SUBDIRS = libltdl src doc

# pkg-config file for the mpg123 library
pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libmpg123.pc

EXTRA_DIST = \
	MakeLegacy.sh \
	libmpg123.pc.in \
	xmms2-plugin/README \
	xmms2-plugin/mpg123/mpg123.c \
	xmms2-plugin/mpg123/wscript \
	makedll.sh \
	test/rms16.c
