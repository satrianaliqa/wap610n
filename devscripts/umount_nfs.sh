#!/bin/sh

LOCAL_DIR=$1


if [ $1 = "--help" -o $1 = "-h" ]
then
	echo "This script umount remote NFS server"
	echo "Parameters:"
	echo "LOCAL_DIR - local directory, to mount remote durectory to."
	echo "If local directory doesn't exist then the script creates it"
	exit 0
fi
	

# 1. Umount NFS

umount ${LOCAL_DIR}

if [ $? != 0 ]	
then
	echo "Can't umount the local directory ${LOCAL_DIR}"
	echo "Exit"
	exit 255
fi

rmdir ${LOCAL_DIR}
if [ $? != 0 ]	
then
	echo "Can't remove the local directory ${LOCAL_DIR}"
	echo "Exit"
	exit 255
fi

exit 0
