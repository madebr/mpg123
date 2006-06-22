#!/bin/sh


#LINE=`grep "AC_INIT(" configure.ac`
#AC_INIT([mpg123], [0.60-devel], [mpg123-devel@lists.sourceforge.net])



cd src

# Need to extract this automatically from configure.ac
echo "// Created by MakeLegacy.sh" > config.h
echo "#define PACKAGE_NAME \"mpg123\"" >> config.h
echo "#define PACKAGE_VERSION \"0.60-devel\"" >> config.h


exec make -f Makefile.legacy $*
