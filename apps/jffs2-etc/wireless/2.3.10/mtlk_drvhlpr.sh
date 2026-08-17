#!/bin/sh

echo start drvhlpr with param

if [ ! -e /tmp/init_failed ]; 
then 
	echo $CONFIGS_PATH > /dev/console
	if [ ! -f $CONFIGS_PATH/drvhlpr.conf ];
	then
			cd $WEB_PATH
			./update_config_files.tcl onlySaveDrvHlprConf	
			cd -						
	fi	
	(./drvhlpr -p $CONFIGS_PATH/drvhlpr.conf); 
fi

#Use for print msg
#if [ ! -e /tmp/init_failed ]; then (./drvhlpr -a eth0 -a eth1 -r 1 -i wlan0 0</dev/console 1>/dev/console 2>&1); fi

if [ $? = 1 ]
then
  echo drvhlpr return MAC HANG
  reboot
  #drvhlpr will reboot the system while we have bug in SW Watchdog mechanism
  #(. ./reload_mtlk_driver.sh > /dev/null);
elif [ $? = 2 ]
  then
  echo drvhlpr return rmmod
else
  echo return from drvhlpr with error
fi
