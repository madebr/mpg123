#!/bin/bash
complbit="$(dirname $0)/MPEG1-layer3/compl.bit"
compldouble="$(dirname $0)/MPEG1-layer3/compl.double"
rms="$(dirname $0)/rmsdouble.bin"
f32conv="$(dirname $0)/f32_double.bin"
s16conv="$(dirname $0)/s16_double.bin"

echo "16bit compliance (unlikely...)"
$@ --no-gapless -q -e s16 -s "$complbit" | "$s16conv" | "$rms" "$compldouble"

echo "float compliance (hopefully fine)"
$@ --no-gapless -q -e f32 -s "$complbit" | "$f32conv" | "$rms" "$compldouble"
