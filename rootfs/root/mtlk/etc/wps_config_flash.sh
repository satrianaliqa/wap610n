#!/bin/sh

#Jacky.Yang 14-Oct-2008, If wireless is unconfigured, we doesn't want connect to default ssid.
unconfigured=`awk -F "=" '/^unconfigured/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
echo unconfigured is $unconfigured > /dev/console
if [ $unconfigured = '1' ]
then
	#Jacky.Yang 23-Oct-2008, change unconfigured tag value to 0.
	sed -i '/unconfigured/d' /mnt/jffs2/wlan0.conf
	echo "unconfigured = 0" >> /mnt/jffs2/wlan0.conf

	cp /mnt/jffs2/wlan0.conf /tmp/wlan0.conf
	/bin/config_umount.sh
	/bin/config_mount.sh

	sleep 7
fi

