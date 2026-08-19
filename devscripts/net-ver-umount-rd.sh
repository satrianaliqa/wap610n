#!/bin/bash

# This script unmounts the STAR rootfs, and creates a backup
# of the image, along with a file list.
# Use the "-n" option to skip creating a backup (NO BACKUP)

# Path for ramdisk images
RDPATH=images

#Jacky.Yang 6-May-2009
ROOTPATH=`cd ; pwd`
mkdir -p ${ROOTPATH}/METALINK/tmp/mnt/rd
TMPFOLDER=${ROOTPATH}/METALINK/tmp/mnt/rd

# Check if a backup must be made
SKIP_BAK=0
getopts n nobak
if [ $nobak = "n" ]
then
	SKIP_BAK=1
fi
 
# Create a timestamp for versioning
VER=`date +%H%M_%d%m%y`

if [ ! -e ${RDPATH}/BAK ]
then
	mkdir $RDPATH/BAK
fi

if [ $SKIP_BAK = 0 ]
then
	# Save a time-stamped dump of all the files on the rootfs
	#ls -lR /mnt/rd > $RDPATH/BAK/ramdisk_2.6.16_${VER}.txt 2> /dev/null
	ls -lR ${TMPFOLDER} > $RDPATH/BAK/ramdisk_2.6.16_${VER}.txt 2> /dev/null
fi

# unmount and zip the filesystem
#sudo umount /mnt/rd 
sudo umount ${TMPFOLDER}
##gzip -f -9 $RDPATH/ramdisk_2.6.16.img
lzma -f -z  $RDPATH/ramdisk_2.6.16.img

if [ "$SKIP_BAK" = "0" ]
then
	# Save a time-stamped backup copy of the rootfs
	cp $RDPATH/ramdisk_2.6.16.img.lzma $RDPATH/BAK/ramdisk_2.6.16_$VER.img.lzma
fi
