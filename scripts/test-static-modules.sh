#!/bin/bash
# test if build of supported modules works when linking in fixed one
modlist=$(./configure | perl -ne 'print $1 if /Detected audio support \.*\s*(.*)$/')
errsum=0
for m in $modlist
do
  echo "==== testing $m ===="
  make clean
  # You can provide -j to speed things up.
  ./configure --with-audio=$m --disable-modules && make "$@" || exit 1
  echo "==== success with $m ===="
done
