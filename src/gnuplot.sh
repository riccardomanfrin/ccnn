#!/bin/bash

#hw_{[x][y]} is hidden node x weight when coming from backward node y
#ow_{[x][y]} is output node x weight when coming from hidden node y
#v_{kq} is the output value for input i[0] = k and input i[1] = q

gnuplot -persist <<EOF
set terminal wxt size 1024,768;
set title "Live Data";

set xlabel "Time";
set ylabel "Value";
set autoscale;

plot "out.csv" \
   using 0:1 with lines linewidth 2 lc rgb "black" title "error_{overall}", \
"" using 0:2 with lines linewidth 2 lc rgb "blue" title "hw_{[0][0]}", \
"" using 0:3 with lines linewidth 2 lc rgb "blue" title "hw_{[0][1]}", \
"" using 0:4 with lines linewidth 2 lc rgb "blue" title "hw_{[1][0]}", \
"" using 0:5 with lines linewidth 2 lc rgb "blue" title "hw_{[1][1]}", \
"" using 0:6 with lines linewidth 2 lc rgb "blue" title "ow_{[0][0]}", \
"" using 0:7 with lines linewidth 2 lc rgb "blue" title "ow_{[0][1]}", \
"" using 0:8 with lines lc rgb "green" title "v_{00}" , \
"" using 0:9 with lines lc rgb "orange" title "v_{01}" , \
"" using 0:10 with lines lc rgb "red" title "v_{10}" , \
"" using 0:11 with lines lc rgb "purple" title "v_{11}" ;
EOF