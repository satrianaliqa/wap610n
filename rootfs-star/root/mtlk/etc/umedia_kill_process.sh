#!/bin/sh
i=0
while [ "$i" = 0 ]
do
	A=`ps | grep $1 | grep -v grep | awk '{print $1}'`
	echo "U-Media: $1=$A" > /dev/console
	if [ "$A" != "" ]; then
		for a in $A
		do 
			echo "U-Media($1): kill -9 $a" > /dev/console
			kill -9 $a
		done
	else
		i=1
		echo "U-Media: Leave kill $1 loop." > /dev/console
	fi
done
