#!/bin/sh

# Restore defaults, using bcl_util.tcl to load the default config file.
# TODO: Replace bcl_util with a simple cp for production systems? 
# (bcl_util knows how to parse the defaults file and separate to wlan0.conf and sys.conf by using the mt_params.db file)

# TODO: Fix the Hardcoded path to mtlk_init_platform.sh
if [ -e /root/mtlk/etc/mtlk_init_platform.sh ]
then
	. /root/mtlk/etc/mtlk_init_platform.sh
else
	. /mnt/jffs2/etc/mtlk_init_platform.sh
fi

if [ -e $BCL_PATH/bcl_util.tcl ]
then 
	$BCL_PATH/bcl_util.tcl loadconfig $SAVED_CONFIG_PATH/default.conf 0 
else 
	/mnt/jffs2/bcl/bcl_util.tcl loadconfig /mnt/jffs2/saved_configs/default.conf 0
	/root/mtlk/bcl/bcl_util.tcl loadconfig /root/mtlk/saved_configs/default.conf 0 
fi 


