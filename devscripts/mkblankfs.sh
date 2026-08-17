#!/bin/sh

# A script to make a blank ramdisk
CONFIG=ramdisk_2.6.16.img

if [ $1 ]
then
	CONFIG=$1
fi

dd if=/dev/zero of=$CONFIG bs=1024 count=12K  > /dev/null
mke2fs -F $CONFIG > /dev/null

if [ -e $CONFIG.gz ]
then
	rm $CONFIG.gz
fi
gzip $CONFIG


