#!/bin/sh

# This script unmounts the STAR rootfs, and creates a backup
# of the image, along with a file list.
# Use the "-n" option to skip creating a backup (NO BACKUP)

ROOTPATH=`cd ; pwd`
MOUNT_POINT=${MOUNT_POINT:-"${ROOTPATH}/METALINK/tmp/mnt/rd"}
RDPATH=${RDPATH:-/opt/star/images}

# Check if a backup must be made
SKIP_BAK=0
getopts n nobak
if [ "$nobak" = "n" ]
then
	SKIP_BAK=1
fi
 
# Create a timestamp for versioning
VER=`date +%H%M_%d%m%y`

if [ ! -e $RDPATH/BAK ]
then
	mkdir $RDPATH/BAK
fi

if [ "$SKIP_BAK" = "0" ]
then
	# Save a time-stamped dump of all the files on the rootfs
	sudo ls -lR $MOUNT_POINT > $RDPATH/BAK/ramdisk_2.6.16_${VER}.txt 2> /dev/null
fi

# unmount and zip the filesystem
sudo umount $MOUNT_POINT 
gzip -f $RDPATH/ramdisk_2.6.16.img

if [ "$SKIP_BAK" = "0" ]
then
	# Save a time-stamped backup copy of the rootfs
	cp $RDPATH/ramdisk_2.6.16.img.gz $RDPATH/BAK/ramdisk_2.6.16_$VER.img.gz
fi
