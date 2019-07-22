#!/bin/sh

# This gives a view at the scaling of resampling runtime with channel count.
# The tested resampling mode involves 2X upsampling, lowpass, and interpolation.

set -e

export LANG=C
export LC_NUMERIC=C
chan_min=1
chan_max=10
chan_fit=3

out123=src/out123
generate="--wave-freq 300 --inputrate 44100 --timelimit 4410000 -q"

wd=$(mktemp -d channelscaling.XXXX)
echo "workdir: $wd"
for n in $(seq $chan_min $chan_max)
do
  printf "generate with %d channels\n" "$n" >&2
  /usr/bin/time -f "$n\t%e" $out123 $generate -c $n --rate 44100 -t 2>&1
done > $wd/generate.txd 
for n in $(seq $chan_min $chan_max)
do
  printf "resample with %d channels\n" "$n" >&2
  /usr/bin/time -f "$n\t%e" $out123 $generate -c $n --rate 44101 -t 2>&1
done > $wd/resample.txd 
txdcalc '[3]=[2]-[1,2]' $wd/generate.txd  < $wd/resample.txd > $wd/resampling-overhead.txd
gpfit --plot -g=1 -r='[3:]' $wd/resampling-overhead.txd 

echo "Check results in $wd/."
