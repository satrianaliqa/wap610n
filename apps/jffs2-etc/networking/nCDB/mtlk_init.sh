#!/bin/sh

# This is the init script run from the jffs2
#
# It calls a series of init scripts.
# $Id: mtlk_init.sh 2476 2008-04-03 13:14:54Z arnonm $

### DEBUG: Uncomment to turn on printing of shell commands
##set -x

# Source platform-specific settings
. ./mtlk_init_platform.sh

# try to run restore defaults script
. ./mtlk_init_rd.sh

# check if config files exists. if not create them
if [ ! -e $CONFIGS_PATH/sys.conf ]
then
	echo Restoring sys.conf
	cp $SAVED_CONFIG_PATH/sys.conf.default $CONFIGS_PATH/sys.conf 
	dos2unix -u $CONFIGS_PATH/sys.conf
fi

if [ ! -e $CONFIGS_PATH/wlan0.conf ]
then
	echo Restoring wlan0.conf
	cp $SAVED_CONFIG_PATH/xAP-wls.conf $CONFIGS_PATH/wlan0.conf 
	dos2unix -u $CONFIGS_PATH/wlan0.conf
	# apply changes
	cd $WEB_PATH
	./init_security.tcl
	cd -
fi


# Copy the config files to /tmp
# Safety: Try to fix wlan.conf and sys.conf files edited on windows
# Copy whatever files are on the dongle - both legacy format, and new dual band filenames.
if [ -e $CONFIGS_PATH/wlan.conf ]
then
	cp $CONFIGS_PATH/wlan.conf /tmp/wlan.conf
	dos2unix -u /tmp/wlan.conf
fi

if [ -e $CONFIGS_PATH/wlan0.conf ]
then
	cp $CONFIGS_PATH/wlan0.conf /tmp/wlan0.conf
	dos2unix -u /tmp/wlan0.conf
fi

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
if expr $KERNEL_VER = "2.4"
then
	MOD_EXT=o
else
	MOD_EXT=ko
fi

# Get the single-band / dual-band mode, 
# according to the number of wls interfaces on the PCI bus.
# Currently, there is no dual-band STA, so only allow dual band in AP mode, 
# (even if the HW supports dual band).
if [ -e /tmp/wlan0.conf ] 
then 
	wlan_filename="wlan0.conf"
else
	wlan_filename="wlan.conf"
fi
NETWORK_TYPE=`awk -F "=" '/^network_type/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/$wlan_filename`

if expr $NETWORK_TYPE = 2
then
	WLAN_NUM_BANDS=`lspci | grep Wireless | wc -l`
else
	WLAN_NUM_BANDS=1
fi

#WPS additions
WPS_ON=`awk -F "=" '/^NonProc_WPS_ActivateWPS/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/$wlan_filename`
#check whether it is defined
WPS_ON_WC=`echo $WPS_ON | wc -c`
if expr $WPS_ON_WC = 1 
then 
	WPS_ON=0
fi
# Get the MAC address and network type (STA/AP) from sys.conf or wlan.conf:
IP_WLAN=`awk -F "=" '/^ip_wlan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
SUBNET_WLAN=`awk -F "=" '/^subnet_wlan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`

JTAG_DEBUG=`awk -F "=" '/^jtag_debug/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`


# Make sure that sys.conf has the network values. If not, get from the old wlan.conf
# This is only kept to support upgrading from old versions, that won't have the IP stored in sys.conf.
# (Hence, it looks in wlan.conf, even though that file was replaced by wlan0.conf on new versions).
IP_WLAN_sys=`echo $IP_WLAN | wc -c`
SUBNET_WLAN_sys=`echo $SUBNET_WLAN | wc -c`

if expr $SUBNET_WLAN_sys = 1 || expr $IP_WLAN_sys = 1 
then
	IP_WLAN=`awk -F "=" '/^ip_wlan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan.conf`
	SUBNET_WLAN=`awk -F "=" '/^subnet_wlan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan.conf`
fi
	
export KERNEL_VER
export MOD_EXT
export NETWORK_TYPE
export WLAN_NUM_BANDS
export WPS_ON
export IP_WLAN
export SUBNET_WLAN

######################################
# Security script is currently disabled - 
# this is created by the web interface
### Init security for each interface
##./mtlk_init_security.sh 0
##
##if expr $WLAN_NUM_BANDS = 2
##then
##	./mtlk_init_security.sh 1
##fi
######################################


# Run the remaining mtlk init scripts (LAN, wireless, routing)
# Skip if any part of the init produces an error (written to /tmp/init_failed)

if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_ip.sh); fi


if [ ! -e /tmp/init_failed ]; 
then 
	# if disableDriver flag is on (done by RestoreDefault app), don't start the wireless driver
	if [ ! -e /tmp/disableDriver ]; 
	then
		# First try to download new images from tftp
		(. ./mtlk_init_download_wls.sh);

		# Bring up the wireless interface
		(. ./mtlk_init_wls.sh); 
	fi
fi

if expr $WPS_PB = 1
then
	# Start the GPIO driver for WPS PB
	insmod ./gpio.$MOD_EXT
fi

if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_route.sh); fi
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_wls_apps.sh); fi

# Always init the daemons - these are the config interfaces to the world,
# and should be available even if the other inits are dead
(. ./mtlk_init_wls_daemons.sh)
(. ./mtlk_init_daemons.sh)


# Save the head of the message log (useful for debugging when there were many msgs and the log was rotated)
./mtlk_save_log.sh &

# Dump errors (seen if the user has a serial interface)
if [ -e /tmp/init_failed ]; then echo -e "\n\nInit failed, due to error:"; cat /tmp/init_failed; fi

# Workaround against rmmode of the driver during init process
sleep 3
if [ ! -e /tmp/init_failed ]; then (. ./mtlk_init_route.sh); fi

