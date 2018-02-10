set terminal postscript eps size 3.5,2.62 enhanced color
set output 'random-read.eps'
labels="write read"
set xrange[1:20] # must be set for '+'
set yrange[0:450]
set samples words(labels)   # number of colors to use
key_x = 17     # x-value of the points, must be given in units of the x-axis
key_y = 430
key_dy = 20
set palette model RGB defined ( 0 'red', 1 'green')
unset colorbox
unset key
set xlabel "I/O Index"
set ylabel "I/O Latency (ms)"
set title "Random-Read.trace per-I/O Latencies"
plot "random-read.dat" u ($1/1000):($7/1000):( $4 == 0 ? 0 : 1 ) notitle with points pt 7 ps 1 palette, \
	 "random-read.dat" u ($1/1000):($7/1000) notitle with lines, \
     '+' using (key_x):(key_y - $0*key_dy):(word(labels, int($0+1))):0 \
         with labels left offset 1,-0.5 point pt 7 ps 1 palette