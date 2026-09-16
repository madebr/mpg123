#!/bin/sh

set -e
export LC_ALL=C

in=$srcdir/src/tests/lots-of-id3text.mp3
out=src/tests/$(basename "${0%.sh}").out.txt
ref=$srcdir/src/tests/$(basename "${0%.sh}").txt

src/mpg123-id3dump "$in" | grep -v ^FILE: > "$out"

if ! diff -q "$ref" "$out"; then
  exit 1
fi
echo PASS
