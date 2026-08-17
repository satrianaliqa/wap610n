#!/bin/sh

echo start drvhlpr with param


if [ ! -e /tmp/init_failed ]; then (./drvhlpr -a eth0 -a eth1 -r 1 -i wlan0); fi

if [ $? == 1 ]
  then
  echo drvhlpr return MAC HANG
  ./reload_mtlk_driver.sh
elif [ $? == 2 ]
  then
  echo drvhlpr return rmmod
else
  echo return from drvhlpr with error
fi

