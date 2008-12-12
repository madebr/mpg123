#!/bin/sh
SF=6615samples_1000Hz_150periods.raw_s16
if test -z "$MPG123"; then
	MPG123=mpg123
fi
# I need -x for little endian.
lame -r --signed --little-endian --bitwidth 16 --preset standard $SF lame.mp3
$MPG123 --gapless -s lame.mp3 > gapless.raw_s16
$MPG123 --no-gapless -s lame.mp3 > gappy.raw_s16

#overall
buntstift --all-binary='format="%short%short"' -u='1,($1+65636),($1+2*65536)' -r='x,-100:6650,y,-36768:(-36768+3*65536)' -j -s=l,l,l $SF gapless.raw_s16 gappy.raw_s16
#end
buntstift --all-binary='format="%short%short"' -u -r=6350:7050 -j -s=l,p,p $SF gapless.raw_s16 gappy.raw_s16
#begin
buntstift --all-binary='format="%short%short"' -u -r=-50:1250 -j -s=l,p,p $SF gapless.raw_s16 gappy.raw_s16
