#!/bin/sh
# A pushbutton daemon for dorango

#########################################################################
#
#              ###### OBSOLETE VERSION!!!! ######
#
# WPS_PBC.sh is now part of the WPS scripts product, not part of the nv init scripts release.
# This file is left here just to save the svn history.
# It is NOT INSTALLED by the Makefile.MTLK
#
# The new location for this script is:
# http://narnia/svn/sw/tools/wps_scripts/WPS_PBC.sh
#########################################################################3

if expr $1 != ""
then
	WPS_ON=$1
fi

if expr $2 != ""
then
	NETWORK_TYPE=$2
fi

if [ $WPS_ON == 1 ] || [ $NETWORK_TYPE > 2 ]
then
	cd /root/mtlk/etc

	action_type='get_conf_via_pbc'
	if [ $NETWORK_TYPE == 2 ]
	then
		action_type='conf_via_pbc'
	fi

	# Get WPS gpio device from HW.ini
	WPS_PBC_GPIO=`awk -F "=" '/^WPS_PB/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/HW.ini`

	if [ "$WPS_PBC_GPIO" != "null" ]
	then
		while [ 1 ]
		do
			cat $WPS_PBC_GPIO > /dev/null
			./mtlk_wps_cmd.tcl $action_type
		done
	fi
fi


echo 'WPS pushbutton: not active'
