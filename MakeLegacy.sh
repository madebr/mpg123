	#!/bin/sh

# Check that configure.ac exists
if test ! -f configure.ac; then
	echo "Can't find configure.ac"
	exit
fi

#AC_INIT([mpg123], [0.60-devel], [mpg123-devel@lists.sourceforge.net])
PACKAGE_NAME=`sed -n 's/^AC_INIT(\[\([^,]*\)\], .*$/\1/p' < configure.ac`
PACKAGE_VERSION=`sed -n 's/^AC_INIT([^,]*, \[\([^,]*\)\], .*$/\1/p' < configure.ac`
PACKAGE_BUGREPORT=`sed -n 's/^AC_INIT([^,]*, [^,]*, \[\(.*\)\])$/\1/p' < configure.ac`

cd src

# Write out our own very basic config.h
echo "Creating basic config.h to reproduce pre-autoconf days."
{
	echo "// Created by MakeLegacy.sh" 
	echo "#ifdef LINUX"
	echo "#define HAVE_LINUX_SOUNDCARD_H"
	echo "#elif defined(__bsdi__)"
	echo "#define HAVE_SYS_SOUNDCARD_H"
	echo "#else"
	echo "#define HAVE_MACHINE_SOUNDCARD_H"
	echo "#endif"
	echo "#define PACKAGE_NAME \"$PACKAGE_NAME\"" 
	echo "#define PACKAGE_VERSION \"$PACKAGE_VERSION\""
	echo "#define PACKAGE_BUGREPORT \"$PACKAGE_BUGREPORT\""
} > config.h

exec make -f Makefile.legacy $*
