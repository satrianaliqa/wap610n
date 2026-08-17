#!/bin/sh

###########################################################################
# This is the init script run from the mtlk etc folder
#
# It calls a series of init scripts.
# $Id: mtlk_init.sh 3585 2009-03-09 16:01:59Z ediv $

### DEBUG: Uncomment to turn on printing of shell commands
##set -x

cat /proc/uptime > /dev/console

# Source platform-specific settings
. ./mtlk_init_platform.sh

# Make the platform file accessible from tmp
ln -s `pwd`/mtlk_init_platform.sh /tmp/mtlk_init_platform.sh

# try to run restore defaults script
if expr $SW_RESTORE_DEFAULTS = 1
then
	. ./mtlk_init_rd.sh
fi

# check if config files exists. if not create them
if [ ! -e $CONFIGS_PATH/sys.conf ]
then
	NEED_RESTORE_FILES=1
fi

if [ ! -e $CONFIGS_PATH/wlan0.conf ]
then
	NEED_RESTORE_FILES=1
fi

if [ $NEED_RESTORE_FILES ]
then
	# Recreate conf files
	./mtlk_restore_defaults.sh
	reboot
fi


# Copy the config files to /tmp
# Safety: Try to fix wlan.conf and sys.conf files edited on windows
cp $CONFIGS_PATH/wlan0.conf /tmp/wlan0.conf
dos2unix -u /tmp/wlan0.conf

if [ -e $CONFIGS_PATH/wlan1.conf ]
then
	cp $CONFIGS_PATH/wlan1.conf /tmp/wlan1.conf
	dos2unix -u /tmp/wlan1.conf
fi

cp $CONFIGS_PATH/sys.conf /tmp/sys.conf
dos2unix -u /tmp/sys.conf


# Get the linux kernel version and set the module extension, 
# so that these scripts can run on both 2.4 and 2.6
KERNEL_VER=`uname -r | awk -F '.' '{print $1"."$2}'`
if [ "$KERNEL_VER" = "2.4" ]
then
	MOD_EXT=o
else
	MOD_EXT=ko
fi

wlan_filename="wlan0.conf"
NETWORK_TYPE=`awk -F "=" '/^network_type/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/$wlan_filename`

# Set single-band mode for VB. (Currently, there is no dual-band VB) 
WLAN_NUM_BANDS=1

#WPS additions
WPS_ON=`awk -F "=" '/^NonProc_WPS_ActivateWPS/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/$wlan_filename`
if [ ! $WPS_ON ]
then 
	WPS_ON=0
fi

# Get the IP and subnet from sys.conf:
IP_WLAN=`awk -F "=" '/^ip_wlan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
SUBNET_WLAN=`awk -F "=" '/^subnet_wlan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`

JTAG_DEBUG=`awk -F "=" '/^jtag_debug/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
	
export KERNEL_VER
export MOD_EXT
export NETWORK_TYPE
export WLAN_NUM_BANDS
export WPS_ON
export IP_WLAN
export SUBNET_WLAN
export CONFIGURE_TC


# Run the remaining mtlk init scripts (LAN, wireless, routing)
# Skip if any part of the init produces an error (written to /tmp/init_failed)

if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_ip.sh); fi

# networking daemons
(. ./mtlk_init_daemons.sh);

if [ ! -e /tmp/init_failed ]
then 
	# if disableDriver flag is on (done by RestoreDefault app), don't start the wireless driver
	if [ ! -e /tmp/disableDriver ]; 
	then
		# Bring up the wireless interface
		(. ./mtlk_init_wls.sh); 
	fi
fi

# Start Init WLS Apps
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_wls_apps.sh); fi

# Save files under /mnt/jffs2 into the flash and clean up a flag
if [ ! -e /tmp/init_failed ] && [ -e /tmp/NeedSave.txt ]
then
	echo Save files into the flash
	
	config_umount.sh
	config_mount.sh
	if [ $? != 0 ]
	then
		echo ERROR: Failed to save files into the flash
	fi
	rm  /tmp/NeedSave.txt
fi

# Save the head of the message log (useful for debugging when there were many msgs and the log was rotated)
# ./mtlk_save_log.sh &

# Always init the daemons - these are the config interfaces to the world,
# and should be available even if the other inits are dead

# Run drvhlpr script
if [ ! -e /tmp/init_failed ]
then
	DRVHLPR_COUNT=`ps | grep drvhlpr | grep -c -v grep`
	if [ $DRVHLPR_COUNT = 0 ]
	then
		echo call drvhlpr
		(. ./mtlk_drvhlpr.sh &);
	fi
fi

(. ./mtlk_init_wls_daemons.sh);

# set MAC Clone
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_mac_cloning.sh); fi

# Dump errors (seen if the user has a serial interface)
if [ -e /tmp/init_failed ]; then echo -e "\n\nInit failed, due to error:"; cat /tmp/init_failed; fi

# Init finished  dhcp script can write to the flash
echo 1 > /tmp/init_done

cat /proc/uptime > /dev/console
