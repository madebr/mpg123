#!/bin/sh
# autogen.sh: Run this to set up the build system: configure, makefiles, etc.

# copyright by the mpg123 project - free software under the terms of the LGPL 2.1
# see COPYING and AUTHORS files in distribution or http://mpg123.org
# initially written by Nicholas J. Humfrey

package="mpg123"


srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

cd "$srcdir" &&
echo "Generating configuration files for $package, please wait...." &&
autoreconf -iv &&
echo "Generating mpg123.spec ... this needs to be present in the dist tarball."
NAME=`perl -ne    'if(/^AC_INIT\(\[([^,]*)\]/)         { print $1; exit; }' < configure.ac` &&
VERSION=`perl -ne 'if(/^AC_INIT\([^,]*,\s*\[([^,]*)\]/){ print $1; exit; }' < configure.ac` &&
perl -pe 's/\@PACKAGE_NAME\@/'$NAME'/; s/\@PACKAGE_VERSION\@/'$VERSION'/;' < mpg123.spec.in > mpg123.spec
