#!/bin/sh

if [ -e /root/mtlk/etc/mtlk_init_platform.sh ]
then
	. /root/mtlk/etc/mtlk_init_platform.sh
else
	. /mnt/jffs2/etc/mtlk_init_platform.sh
fi

$ETC_PATH/mtlk_load_config.tcl $*


