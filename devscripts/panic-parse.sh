#!/bin/sh

# USAGE: ./panic-parse.sh <kernel-panic-log> <System.map file>

if [ ! $2 ]
then
	echo
	echo "USAGE: panic-parse.sh <kernel-panic-log> <System.map file>"
	echo "kernel-panic-log is a copy-paste of the serial output, including kernel panic"
	echo "e.g."
	echo "devscripts/panic-parse.sh panic1.log output/System.map"
	echo
	exit 1
fi

ADDRS=`cat $1 | awk   '/Function entered/ {print $4}' | tr -d [:punct:]`
echo $ADDRS
for addr in $ADDRS
do
	grep $addr $2
done


