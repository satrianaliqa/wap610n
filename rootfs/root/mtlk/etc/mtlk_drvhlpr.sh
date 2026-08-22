#!/bin/sh

echo start drvhlpr with param

if [ -e /root/mtlk/etc/mtlk_wps_event.sh ]
then
	export WPS_EVENT_SCRIPT=/root/mtlk/etc/mtlk_wps_event.sh
else
	export WPS_EVENT_SCRIPT=/mnt/jffs2/etc/mtlk_wps_event.sh
fi

if [ -e /root/mtlk/etc/mtlk_linkstat_event.sh ]
then
	export WLS_LINK_EVENT_SCRIPT=/root/mtlk/etc/mtlk_linkstat_event.sh
else
	export WLS_LINK_EVENT_SCRIPT=/mnt/jffs2/etc/mtlk_linkstat_event.sh
fi
	

if [ ! -e /tmp/init_failed ]; then (./drvhlpr -a eth0 -a eth1 -r 1 -i wlan0); fi

if [ $? == 1 ]
  then
  echo drvhlpr return MAC HANG
  reboot
  #drvhlpr will reboot the system while we have bug in SW Watchdog mechanism
  #(. ./reload_mtlk_driver.sh > /dev/null);
elif [ $? == 2 ]
  then
  echo drvhlpr return rmmod
else
  echo return from drvhlpr with error
fi
