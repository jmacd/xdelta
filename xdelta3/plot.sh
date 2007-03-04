#!/bin/sh

G=/usr/bin/gnuplot

I=$1
O=$2

$G > $O <<EOF
set terminal jpeg

f(x) = 1331000 + 30000 * (1 / (x - 2.45))

# plot [x=1:10] [1:10] f(x)
# plot sin(x), cos(x)
# , f(x)

plot "$I" using 1:2, f(x)

EOF
