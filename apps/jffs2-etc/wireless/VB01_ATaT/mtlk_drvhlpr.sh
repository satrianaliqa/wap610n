#!/bin/sh

echo start drvhlpr with param
mask=0


SW_RESET_ENABLE=`awk -F "=" '/^Debug_SoftwareWatchdogEnable/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`

echo the sw reset status is $SW_RESET_ENABLE

#TODO -mask can have other fields, use mask & $SW_RESET_ENABLE in tcl script
if expr $SW_RESET_ENABLE = 0
then
  if [ ! -e /tmp/init_failed ]; then (./drvhlpr -a eth0 -a eth1 -m 1 -r 1); fi
else
  if [ ! -e /tmp/init_failed ]; then (./drvhlpr -a eth0 -a eth1 -m 0 -r 1); fi
fi

if expr $? = 1
  then
  echo drvhlpr return MAC HANG
  ./reload_mtlk_driver.sh
elif expr $? = 2
  then
  echo drvhlpr return rmmod
else
  echo return from drvhlpr with error
fi

