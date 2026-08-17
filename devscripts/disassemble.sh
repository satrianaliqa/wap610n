#!/bin/sh
# disassemble.sh  - tool to help disassemble kernel obj files

# USAGE:
# disassemble.sh <obj file> <map file>

if [ $# != 2 ]
then
	echo USAGE:
	echo "$0 <obj file> <map file>"

	exit 1
fi

PATH=`pwd`/tools/arm-uclibc-3.4.6/bin/:$PATH

FNAME=`arm-linux-objdump -dlS $1 | grep -m 1 -o \<.*\> | sed s/\<// | sed s/\>//`
echo $FNAME 
FOFFSET=`grep $FNAME $2 | awk '{print $1}'`
echo $FOFFSET 
arm-linux-objdump -dlS $1 --adjust-vma=0x$FOFFSET 

