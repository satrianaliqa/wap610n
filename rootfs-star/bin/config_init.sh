#!/bin/sh

# config_init.sh
#
# This script creates a new config filesystem on flash
# by downloading from tftp

# TODO: This script is probably obsolete now and should be deleted.
# config_mount creates a default fs if there is none.

IP=192.168.1.100
CONFIG_BLOCK=/dev/mtdblock4

MODEL=`awk '/^Demo/ {print $4}' /proc/str9100/gsw`
if [ "$MODEL" = "GPB239S" ]
then
        CONFIG_IMG=config_239S.img.gz
else
        CONFIG_IMG=config.img.gz
fi

if [ $1 ]
then
	IP=$1
fi

tftp -gr $CONFIG_IMG -l /dev/mtdblock4 $IP

