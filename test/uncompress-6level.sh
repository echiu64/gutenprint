#!/bin/bash

# compression of 5-level inks (irrespective of bits per level)
# 3-level, 5 pixels = 1024 combinations: 10 bits long
# but since not all levels used: unique are 242, or 1 byte
# 4-level, 4 pixels = 65536 combinations: 16 bits long
# but since only 2 bits per pixel used there are 256 combinations, or 1 byte
# 5-level, 3 pixels: 15 bits long
# but since only 3 bits per pixel used there are 125 unique combinations, or 1 byte


# m: max number of levels used
m=6
maxl=$(( $m-1 )) # the max level numerical value
# z: max number allowed by bits (numerical value)
z=15
# a: iteration of valid array subscript (1-based)
a=1
# iteration of pixels
#i=0
j=0
k=0
l=0

echo "static const unsigned short Table6Level[] ="
echo "{"

#for (( i=0; i<=$z; i++ ))
#do
    for (( j=0; j<=$z; j++ ))
    do
	for (( k=0; k<=$z; k++ ))
	do
	    for (( l=0; l<=$z; l++ ))
	    do
		if test $j -le ${maxl} -a $k -le ${maxl} -a $l -le ${maxl}
		then
		    bj=$(echo "ibase=16;obase=2; $j" | bc | awk '{if ($0<10){print "000"$0} else if ($0<100){print "00"$0} else if ($0<1000){print "0"$0} else {print $0} }')
		    bk=$(echo "ibase=16;obase=2; $k" | bc | awk '{if ($0<10){print "000"$0} else if ($0<100){print "00"$0} else if ($0<1000){print "0"$0} else {print $0} }')
		    bl=$(echo "ibase=16;obase=2; $l" | bc | awk '{if ($0<10){print "000"$0} else if ($0<100){print "00"$0} else if ($0<1000){print "0"$0} else {print $0} }')
		    binput="${bj}${bk}${bl}"
		    #echo "test:" ${bj} ${bk} ${bl} ":" ${binput} ":" $(echo "ibase=2;obase=10000; ${binput}" | bc)
		    array[$a]=$(echo "ibase=2;obase=10000; ${binput}" | bc)
		    remainder=$(( $a % 16 ))
		    a=$(( $a+1 ))
		    #echo "test:" ${remainder}
		    # print line of output
		    if test ${remainder} -eq 0
		    then
			printf "  "
			pstart=$(( $a-16 ))
			pfinal=$(( $a-1 ))
			#echo "array subscript: $a, pstart: $pstart, pfinal: $pfinal, iterators: $j $k $l"
			for (( p=$pstart; p<=$pfinal; p++ ))
			do
			    printf "0x%x," 0x${array[$p]}
			done
			printf "\n"
		    fi
		fi
	    done
	done
    done
#done

# need to print out the last set of combinations
dividend=$(( $a-1 ))
divisor=${dividend}/16 # the number of completed rows already printed
printf "  "
pstart=$(( ${divisor}*16+1 ))
pfinal=${dividend}
#echo "array subscript: $a, pstart: $pstart, pfinal: $pfinal, iterators: $j $k $l"
for (( p=$pstart; p<=$pfinal; p++ ))
do
    printf "0x%x," 0x${array[$p]}
done
printf "\n"



echo "};"
echo "max number of valid combinations: $(( a-1 ))"
