#!/bin/sh



LAST_STATUS=`awk -F "=" '/^WLSLinksStatus/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wls_link_stat`
NETWORK_TYPE=`awk -F "=" '/^network_type/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
DEFAULT_IP=`awk -F "=" '/^ip_lan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
STATIC_IP=`awk -F "=" '/^ip_config_method/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`

if [ $1 = 'w1' ]
then
	echo mtlk_linkstat_event.sh:Wireless Link is UP >/dev/console
	echo "WLSLinksStatus = 1" > /tmp/wls_link_stat
	#if [ $LAST_STATUS = '1' ] 
	#then
	#	echo mtlk_linkstat_event.sh:Already up >/dev/console
	#else
		if [ $NETWORK_TYPE = 0 ]
		then
			if [ $STATIC_IP = 0 ]
			then		
		
				#If this DOWN/UP is caused by site survey, then we need not require to renew IP - Ricky Cao on Nov. 28 2008
				if [ -e /var/scanning ]
				then
					echo "This wlan DOWN/UP is caused by site survey, so we don't to renew IP" > /dev/console
				elif [ -e /var/wpsRunning ]; then
					echo "This wlan DOWN/UP is caused by WPS progress, so we don't to renew IP" > /dev/console
				elif [ -e /var/webCommit ]; then
					#Jacky.Yang 25-Jun-2009, if wps.c can found /var/retry_dhcp_client then we can re-try renew udhcpc.
					if [ -e /var/retry_dhcp_client ]
					then
						rm -f /var/retry_dhcp_client
						#Jacky.Yang 29-Nov-2008, add for GUI WPS
						echo mtlk_linkstat_event.sh:DHCP renew, change to WPS Session Done >/dev/console
						killall -USR1 udhcpc
					fi
				else
					#Jacky.Yang 25-Jun-2009, if wps.c can found /var/retry_dhcp_client then we can re-try renew udhcpc.
					if [ -e /var/retry_dhcp_client ]
					then
						rm -f /var/retry_dhcp_client
						#Jacky.Yang 29-Nov-2008, drop it.
						#ifconfig br0 $DEFAULT_IP
						echo mtlk_linkstat_event.sh:DHCP renew >/dev/console
						killall -USR1 udhcpc
					fi
				fi
				# Add to force PC connected at ethernet port of station to renew IP > /dev/console - U-Media Ricky Cao on Oct. 15 2008 
				# "reset_phy" command is add by ourself, this command will force phy to power on/off
				if [ -e /var/scanning ]
				then
					echo "This wlan DOWN/UP is caused by site survey, so we don't force PC on ethernet port to renew IP" > /dev/console
					rm /var/scanning
				elif [ -e /var/firmware_upgrading ]; then
					echo "Device is upgrading firmware, so we don't force PC on ethernet port to renew IP" > /dev/console
				elif [ -e /var/wpsRunning ]; then
					echo "WPS is running, so we don't force PC on ethernet port to renew IP" > /dev/console
				else
					if [ -e /var/run/force_ethpc_renew_ip ]
					then
						echo "mtlk_linkstat_event.sh: killall -SIGUSR1 force_ethpc_renew_ip" > /dev/console
						killall -SIGUSR1 force_ethpc_renew_ip
					fi
					#echo "Not Scanning.." > /dev/console
					/root/mtlk/etc/force_ethpc_renew_ip > /dev/console &
				fi
			fi
		fi
	#fi
	exit
fi

if [ $1 = 'w0' ]
then
	echo mtlk_linkstat_event.sh:Wireless Link is DOWN >/dev/console
	echo "WLSLinksStatus = 0" > /tmp/wls_link_stat
	if [ $LAST_STATUS = '0' ] 
	then
		echo mtlk_linkstat_event.sh:Already down >/dev/console
	fi
	if [ -e /var/run/force_ethpc_renew_ip ]
	then
		killall -SIGUSR1 force_ethpc_renew_ip	
	fi
	exit
fi

echo mtlk_linkstat_event.sh:Unknown event $1 >/dev/console

exit


