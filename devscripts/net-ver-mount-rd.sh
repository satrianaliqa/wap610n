#!/bin/bash

# This script mounts the STAR rootfs at /mnt/rd
# Use it to add/modify files in the rootfs

# mountpoint
#MOUNT_POINT=/mnt/rd
#Jacky.Yang 6-May-2009
ROOTPATH=`cd ; pwd`
mkdir -p ${ROOTPATH}/METALINK/tmp/mnt/rd
MOUNT_POINT=${ROOTPATH}/METALINK/tmp/mnt/rd

# Path for ramdisk images
RD=images/ramdisk_2.6.16.img

rm -f ${RD} ${RD}.gz
dd if=/dev/zero of=${RD} bs=1024 count=12K  > /dev/null
mke2fs -F ${RD} > /dev/null

# create $MOUNT_POINT if doesn't exist
if [ ! -e $MOUNT_POINT ]
then
	sudo mkdir $MOUNT_POINT
fi

# mount just if $MOUNT_POINT is not mounted
mount | grep "$MOUNT_POINT"

if [ $? == 1 ]
then
	sudo mount $RD $MOUNT_POINT  -o loop
else
	echo -n "Warning: $MOUNT_POINT is mounted. " 
	echo "Will not mount!"
fi


