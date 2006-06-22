#!/bin/sh


#AC_INIT([mpg123], [0.60-devel], [mpg123-devel@lists.sourceforge.net])
PACKAGE_NAME=`sed -n 's/^AC_INIT(\[\([^,]*\)\], .*$/\1/p' < configure.ac`
PACKAGE_VERSION=`sed -n 's/^AC_INIT([^,]*, \[\([^,]*\)\], .*$/\1/p' < configure.ac`
PACKAGE_BUGREPORT=`sed -n 's/^AC_INIT([^,]*, [^,]*, \[\(.*\)\])$/\1/p' < configure.ac`

cd src

# Need to extract this automatically from configure.ac
echo "// Created by MakeLegacy.sh" > config.h
echo "#define PACKAGE_NAME \"$PACKAGE_NAME\"" >> config.h
echo "#define PACKAGE_VERSION \"$PACKAGE_VERSION\"" >> config.h
echo "#define PACKAGE_BUGREPORT \"$PACKAGE_BUGREPORT\"" >> config.h

exec make -f Makefile.legacy $*
