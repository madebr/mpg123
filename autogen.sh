#!/bin/sh
# autogen.sh: Run this to set up the build system: configure, makefiles, etc.

# copyright by the mpg123 project - free software under the terms of the LGPL 2.1
# see COPYING and AUTHORS files in distribution or http://mpg123.org
# initially written by Nicholas J. Humfrey

package="mpg123"


srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

cd "$srcdir"
DIE=0

(autoheader --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "You must have autoconf installed to compile $package."
    echo "Download the appropriate package for your distribution,"
    echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
    DIE=1
}

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "You must have autoconf installed to compile $package."
    echo "Download the appropriate package for your distribution,"
    echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
    DIE=1
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "You must have automake installed to compile $package."
    echo "Download the appropriate package for your system,"
    echo "or get the source from one of the GNU ftp sites"
    echo "listed in http://www.gnu.org/order/ftp.html"
    DIE=1
}

(libtoolize --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have libtool installed to compile $package."
	echo "Download the appropriate package for your system,"
	echo "or get the source from one of the GNU ftp sites"
	echo "listed in http://www.gnu.org/order/ftp.html"
	DIE=1
}


if test "$DIE" -eq 1; then
    exit 1
fi



echo "Generating configuration files for $package, please wait...."

# Create the (empty) build directory if it doesn't exist
# Note the avoidance of the ! operator -- it is not portable!
if [ -d build ]; then
  echo "  build dir is there"
else
	echo "  creating build directory"
	mkdir build
fi



run_cmd() {
    echo "  running $* ..."
    if $*; then
		echo "  fine"
    else
		echo failed!
		exit 1
    fi
}


run_cmd libtoolize --force --copy &&
run_cmd aclocal
run_cmd autoheader
run_cmd automake --add-missing --copy
run_cmd autoconf

# Create mpg123.spec here... so that configure doesn't recreate it all the time.
NAME=`perl -ne    'if(/^AC_INIT\(\[([^,]*)\]/)         { print $1; exit; }' < configure.ac`
VERSION=`perl -ne 'if(/^AC_INIT\([^,]*,\s*\[([^,]*)\]/){ print $1; exit; }' < configure.ac`
perl -pe 's/\@PACKAGE_NAME\@/'$NAME'/; s/\@PACKAGE_VERSION\@/'$VERSION'/;' < mpg123.spec.in > mpg123.spec

$srcdir/configure --enable-debug && echo
