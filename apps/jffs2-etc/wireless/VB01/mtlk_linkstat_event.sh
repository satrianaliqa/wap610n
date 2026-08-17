#!/bin/sh



LAST_STATUS=`awk -F "=" '/^WLSLinksStatus/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wls_link_stat`
NETWORK_TYPE=`awk -F "=" '/^network_type/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
STATIC_IP=`awk -F "=" '/^ip_config_method/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`

if [ $1 = 'w1' ]
then
	echo mtlk_linkstat_event.sh:Wireless Link is UP >/dev/console
	echo "WLSLinksStatus = 1" > /tmp/wls_link_stat
	if [ $LAST_STATUS = '1' ] 
	then
		echo mtlk_linkstat_event.sh:Already up >/dev/console
	else
		if [ $NETWORK_TYPE = 0 ]
		then
			if [ $STATIC_IP = 0 ]
			then
				# Force our device to renew its IP
				echo mtlk_linkstat_event.sh:DHCP renew >/dev/console
				killall -USR1 udhcpc
			fi
			
			# Force devices connected to the eth port to renew their IP, by resetting the eth PHY
			if [ -e /proc/str9100/phy_reset ]
			then
				echo 1 > /proc/str9100/phy_reset
			fi
		fi
	fi
	exit
fi

if [ $1 = 'w0' ]
then
	echo mtlk_linkstat_event.sh:Wireless Link is DOWN >/dev/console
	echo "WLSLinksStatus = 0" > /tmp/wls_link_stat
	if [ $LAST_STATUS = '0' ] 
	then
		echo mtlk_linkstat_event.sh:Already down >/dev/console
	else
		# Bring the bridge down and up, to restore all interfaces to forwarding state,
		# in case eth0 was blocked by loop breaking 
		ifconfig br0 down
		ifconfig br0 up
	fi
	exit
fi

echo mtlk_linkstat_event.sh:Unknown event $1 >/dev/console

exit


