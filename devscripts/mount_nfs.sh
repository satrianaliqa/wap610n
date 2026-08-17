#!/bin/sh

NFS_SERVER=$1
NFS_DIR=$2
LOCAL_DIR=$3


if [ $1 = "--help" -o $1 = "-h" ]
then
	echo "This script mount remote NFS server"
	echo "Parameters:"
	echo "NFS_SERVER NFS_DIR LOCAL_DIR"
	echo "NFS_SERVER - IP or name of NFS server"
	echo "NFS_DIR - directory on NFS server to mount"
	echo "LOCAL_DIR - local directory, to mount remote durectory to."
	echo "If local directory doesn't exist then the script creates it"
	exit 0
fi
	

# 1. Creeate local dir

if [ ! -d ${LOCAL_DIR} ]
then

	mkdir ${LOCAL_DIR}
	if [ $? != 0 ]
	then
		echo "Can't mount the local directory ${LOCAL_DIR}"
		echo "Exit"
		exit 255
	fi
fi

# 2. Mount NFS

mount -t nfs ${NFS_SERVER}:/${NFS_DIR} ${LOCAL_DIR}

if [ $? != 0 ]	
then
	echo "Can't mount the local directory ${LOCAL_DIR}"
	echo "Exit"
	exit 255
fi

exit 0
