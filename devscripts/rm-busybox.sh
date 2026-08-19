#!/bin/bash

# A script to delete the current busybox apps + links before copying a new version

export BB_INSTALL=/opt/rootfs-star

BB_PATHS=(. bin sbin usr/bin usr/sbin)

for BB_PATH in ${BB_PATHS[*]}
do
	export BB_PATH
	rm `ls -l $BB_INSTALL/$BB_PATH | awk '/busybox/ {print ENVIRON["BB_INSTALL"]"/"ENVIRON["BB_PATH"]"/"$8}'`

done
exit


