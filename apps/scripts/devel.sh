#!/bin/sh

# Description.

# This script installs on a board the development environment.
# It downloads one by tftp modules nfs.ko, lockd.ko and sunrpc.ko
# Then it loads them onto the kernel.
# Next step it mounts remote NFS filesystem what contains all 
# development environment.
# As the last step it adds path of bin/ and /sbin directories to PATH.


# Variables.

# Ip address of tftp server.
TFTP_SERVER=172.16.20.39

# Ip adderess of BFS server
NFS_SERVER=172.16.20.39

# Directory on NFS server to mount
NFS_DIR=tftp

# Module names.
NFS_NAME=nfs
LOCKD_NAME=lockd
SUNRPC_NAME=sunrpc

# Temporary directory.
TMP_DIR=/tmp/debug.$$

# Directory to mount.
MOUNT_DIR=/mnt/devel

# Get current directory

CDIR=`pwd`


# if there are parametres then parse it

if [ $1 ]
then
	TFTP_SERVER=$1
	if [ $2 ]
	then
		NFS_SERVER=$2
	else
		NFS_SERVER=$1
	fi
fi

# 1. Create temporary directory

if [ ! -d ${TMP_DIR} ]
then
	mkdir ${TMP_DIR}
	if [ $? != 0 ]
	then
		echo "Error, can't create temp dir ${TMP_DIR}"
		exit -1
	fi
fi

# 2.Change the dir to temp

cd ${TMP_DIR}
if [ $? != 0 ]
then
	echo "Can't change dir to temp dir ${TMP_DIR}"
	exit -1
fi

# 3. Downloads modules into the temporary directory.

tftp -g -r ${NFS_NAME}.ko   ${TFTP_SERVER} 
if [ $? != 0 ]
then
	echo "Can't receive ${NFS_NAME}.ko"
	exit -1
fi

tftp -g -r ${LOCKD_NAME}.ko ${TFTP_SERVER} 
if [ $? != 0 ]
then
	echo "Can't receive ${LOCKD_NAME}.ko"
	exit -1
fi

tftp -g -r ${SUNRPC_NAME}.ko  ${TFTP_SERVER} 
if [ $? != 0 ]
then
	echo "Can't receive ${SUNRPC_NAME}.ko"
	exit -1
fi

# 4. Remove the modules, if they are in memory. Do it with the forced manner.

rmmod ${NFS_NAME} > /dev/null
rmmod ${LOCKD_NAME} > /dev/null
rmmod ${SUNRPC_NAME} > /dev/null

# 5. Load the fresh downloaded modules.

insmod ${SUNRPC_NAME}.ko
if [ $? != 0 ]
then
	echo "Can't load ${SUNRPC_NAME}.ko"
	exit -1
fi

insmod ${LOCKD_NAME}.ko
if [ $? != 0 ]
then
	echo "Can't receive ${LOCKD_NAME}.ko"
	exit -1
fi

insmod ${NFS_NAME}.ko
if [ $? != 0 ]
then
	echo "Can't receive ${NFS_NAME}.ko"
	exit -1
fi


# 6. Leave the directory.

cd ${CDIR}
if [ $? != 0 ]
then
	echo "Can't change directory to previous ${CDIR}"
	exit -1
fi

rm -fr ${TMP_DIR}
if [ $? != 0 ]
then
	echo "Can't rm ${TMP_DIR}"
	exit -1
fi


# 6. Create a directory for mounting.

if [ ! -d ${MOUNT_DIR} ]
then
	mkdir ${MOUNT_DIR}
	if [ $? != 0 ] 
	then
		echo "Can't create directory ${MOUNT_DIR}"
		exit -1
	fi

	chmod 777 ${MOUNT_DIR}
	if [ $? != 0 ]
	then
		echo "Can't chmod ${MOUNT_DIR} to 777"
		exit -1
	fi

fi

# 7. Mount remote NFS.

echo "Going to mount NFS filesystem. It can take time."
echo "Dont be afraid of errors and warnings".

mount ${NFS_SERVER}:/${NFS_DIR} ${MOUNT_DIR} -o nolock> /dev/null

if [ $? != 0 ]
then
	echo "Can't mount ${NFS_SERVER}:/${NFS_DIR} to ${MOUNT_DIR}"
	exit -1
fi

# 8. Validate that mount is done.
mount | grep ${NFS_SERVER} > /dev/null

if [ $? != 0 ]
then
	echo "The remote directory on ${NFS_SERVER} didn't mount. Stopped."
	exit -1
fi



echo "Done, configuring pathes"

# 7. Run extern script of debugfs.

if [ -f ${MOUNT_DIR}/etc/debugfs.mount ]
then
        ${MOUNT_DIR}/etc/debugfs.mount
        if [ $? != 0 ]
        then
                echo "An error occurred when running ${MOUNT_DIR}/etc/debugfs.mount"
        fi

else
        echo "Can't find ${MOUNT_DIR}/etc/debugfs.mount"
fi

# 9. Done. Now setup executables and libs


cd /lib > /dev/null
ln -s ${MOUNT_DIR}/lib/* . > /dev/null
cd - > /dev/null
cd /bin > /dev/null
ln -s ${MOUNT_DIR}/bin/* . > /dev/null
cd - > /dev/null
cd /sbin > /dev/null
ln -s ${MOUNT_DIR}/sbin/* . > /dev/null
cd - > /dev/null

exit 0
