#!/bin/sh

###########################################################################
# This is the init script run from the mtlk etc folder
#
# It calls a series of init scripts.
# $Id: mtlk_init.sh 3529 2009-02-26 08:17:31Z ediv $

### DEBUG: Uncomment to turn on printing of shell commands
##set -x

get_uptime ()
{
	uptime_val=`cat /proc/uptime | awk -F ' ' '{print $1}'`
	echo $uptime_val
}

#echo Start mtlk_init.sh `get_uptime` > /tmp/timestep.txt

#Disable SA learning featrue of internal switch of STAR CPU - Ricky Cao on Nov. 27 2008
#Because it will result abnormal packet forwarding
echo "@@@@@@@@@@ Before disable SA learning in STAR CPU @@@@@@@@@@"
echo "dump 0x70000008" >> /proc/str9100/reg_debug; cat /proc/str9100/reg_debug
echo "dump 0x7000000c" >> /proc/str9100/reg_debug; cat /proc/str9100/reg_debug
echo "write 0x70000008 0x004e3df6" >> /proc/str9100/reg_debug
echo "write 0x7000000c 0x004e3df6" >> /proc/str9100/reg_debug
echo "@@@@@@@@@@ After disable SA learning in STAR CPU @@@@@@@@@@"
echo "dump 0x70000008" >> /proc/str9100/reg_debug; cat /proc/str9100/reg_debug
echo "dump 0x7000000c" >> /proc/str9100/reg_debug; cat /proc/str9100/reg_debug

# Source platform-specific settings
. ./mtlk_init_platform.sh

# try to run restore defaults script
# . ./mtlk_init_rd.sh

# TODO: Delete the following file restore code - it should never need to be called on the VB
# check if config files exists. if not create them
if [ ! -e $CONFIGS_PATH/sys.conf ]
then
	echo Restoring sys.conf
	cp $SAVED_CONFIG_PATH/sys.conf.default $CONFIGS_PATH/sys.conf 
	dos2unix -u $CONFIGS_PATH/sys.conf
fi

#if [ ! -e $CONFIGS_PATH/wlan0.conf ]
#then
#	echo Restoring wlan0.conf
#	cp $SAVED_CONFIG_PATH/xAP-wls.conf $CONFIGS_PATH/wlan0.conf 
#	dos2unix -u $CONFIGS_PATH/wlan0.conf
	# apply changes
#	cd $WEB_PATH
#	./init_security.tcl
#	cd -
#fi


# Copy the config files to /tmp
# Safety: Try to fix wlan.conf and sys.conf files edited on windows
dos2unix -u $CONFIGS_PATH/wlan0.conf

cp $CONFIGS_PATH/wlan0.conf /tmp/wlan0.conf
dos2unix -u /tmp/wlan0.conf

dos2unix -u $CONFIGS_PATH/sys.conf
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
HOST_NAME=`awk -F "=" '/^HostName/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
	
export KERNEL_VER
export MOD_EXT
export NETWORK_TYPE
export WLAN_NUM_BANDS
export WPS_ON
export IP_WLAN
export SUBNET_WLAN
export HOST_NAME

#Initialize the host name for the platform
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_hostname.sh $HOST_NAME); fi

# Run the remaining mtlk init scripts (LAN, wireless, routing)
# Skip if any part of the init produces an error (written to /tmp/init_failed)

if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_ip.sh); fi

# networking daemons
(. ./mtlk_init_daemons.sh);

if [ ! -e /tmp/init_failed ]
then 
	# if disableDriver flag is on (done by RestoreDefault app), don't start the wireless driver
	#if [ ! -e /tmp/disableDriver ]; 
	#then
		# Bring up the wireless interface
		(. ./mtlk_init_wls.sh); 
	#fi
fi

# Arg = 1 - Add wlan0 to the bridge
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_route.sh 1); fi


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

#Add for create site survey list catch - Ricky Cao on Nov. 27 2008
iwlist wlan0 scanning > /tmp/ap_list.catch &

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

# Arg = 2 - Add known MAC ADDR for MAC Cloning
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_route.sh 2); fi

# Dump errors (seen if the user has a serial interface)
if [ -e /tmp/init_failed ]; then echo -e "\n\nInit failed, due to error:"; cat /tmp/init_failed; fi

# Init finished  dhcp script can write to the flash
echo 1 > /tmp/init_done

# Apply kernel network buffer & VM performance tuning for 32MB RAM
echo 262144 > /proc/sys/net/core/rmem_max 2>/dev/null || true
echo 262144 > /proc/sys/net/core/wmem_max 2>/dev/null || true
echo 262144 > /proc/sys/net/core/rmem_default 2>/dev/null || true
echo 262144 > /proc/sys/net/core/wmem_default 2>/dev/null || true
echo 1000 > /proc/sys/net/core/netdev_max_backlog 2>/dev/null || true
echo "4096 87380 262144" > /proc/sys/net/ipv4/tcp_rmem 2>/dev/null || true
echo "4096 65536 262144" > /proc/sys/net/ipv4/tcp_wmem 2>/dev/null || true
echo 50 > /proc/sys/vm/vfs_cache_pressure 2>/dev/null || true
echo 2048 > /proc/sys/vm/min_free_kbytes 2>/dev/null || true

# Stop power LED blinking and turn on solid green Power LED
echo 0 > /dev/gpio2 2>/dev/null || true
echo 1 > /dev/gpio0 2>/dev/null || true
echo 1 > /dev/led0 2>/dev/null || true
