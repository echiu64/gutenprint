#!/bin/bash

# compression of 5-level inks (irrespective of bits per level)
# 3-level, 5 pixels = 1024 combinations: 10 bits long
# but since not all levels used: unique are 242, or 1 byte
# 4-level, 4 pixels = 65536 combinations: 16 bits long
# but since only 2 bits per pixel used there are 256 combinations, or 1 byte
# 5-level, 3 pixels: 15 bits long
# but since only 3 bits per pixel used there are 125 unique combinations, or 1 byte


# m: max number of levels used
m=5
# z: max number allowed by bits
z=16
# a: iteration of array subscript
a=1
# n: iteration of valid combinations
n=0
# iteration of pixels
#i=0
j=0
k=0
l=0

echo "static const unsigned char ninetoeight[] ="
echo "{"

#for (( i=1; i<=$z; i++ ))
#do
    for (( j=1; j<=$z; j++ ))
    do
	for (( k=1; k<=$z; k++ ))
	do
	    for (( l=1; l<=$z; l++ ))
	    do
#		if test $i -gt $m -o $j -gt $m -o $k -gt $m -o $l -gt $m
		if test $j -gt $m -o $k -gt $m -o $l -gt $m
		then
		    array[$a]=0
		else
		    array[$a]=$n
		    n=$(( $n+1 ))
		fi
		# print line of output
		remainder=$(( $a % 16 ))
		a=$(( $a+1 ))
		if test $remainder -eq 0
		then
		    printf "  "
		    pstart=$(( 256*($j-1)+16*($k-1)+1 ))
		    pfinal=$(( 256*($j-1)+16*$k ))
		    #echo "array subscript: $a, pstart: $pstart, pfinal: $pfinal, iterators: $j $k $l"
		    for (( p=$pstart; p<=$pfinal; p++ ))
		    do
			printf "%3d," ${array[$p]}
		    done
		    printf "\n"
		fi
	    done
	done
    done
#done

echo "};"
echo "max number of valid combinations: $n"
