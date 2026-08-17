#!/bin/sh

# This is a script to make the MTLK wireless driver.
# It is added here, so that driver compilation options can be managed in our SVN repository.

KERNELPATH=`pwd`
MAKEFILE=Makefile.wls.STAR

WLS_PATH=/opt/apps-rootfs/driver-latest/REL_2.3.0/wireless/driver/linux/
ROOTFS_PATH=../../rootfs-star/root/mtlk/driver

cd $WLS_PATH
make -f $KERNELPATH/$MAKEFILE DEBUG=0 && cp $WLS_PATH/mtlk.ko $ROOTFS_PATH



cd -
