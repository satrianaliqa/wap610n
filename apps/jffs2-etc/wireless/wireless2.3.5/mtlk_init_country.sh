#!/bin/sh

#
# This script takes the country code from the EEPROM and configures the driver with it. In addition, it updates the wlan0.conf with the new Country code if relevant, and a variable is used to signal that the EEPROMCountry is valid or non valid.
# This implementation is relevant for wlan0.conf only. For other configuration methods this file should be modified.

echo 025,0x03,EEPROM,HWCountry > /proc/sys/dev/mtlk/wlan0/ConfigCommand
CONFIG_COMMAND_RESULT=`cat /proc/sys/dev/mtlk/wlan0/ConfigCommand`
WLAN_PATH=$1
WLAN_INDEX=$2

WLAN_MAIN=${WLAN_PATH}/wlan${WLAN_INDEX}.conf
WLAN_TMP=/tmp/wlan${WLAN_INDEX}.conf

if expr $? == 0
then
	CONFIG_COMMAND_RET=`echo $CONFIG_COMMAND_RESULT | awk -F "," '{print $2 }'`
	COUNTRY_CODE=`echo $CONFIG_COMMAND_RESULT | awk -F "," '{print $3 }'`
	
	if expr $CONFIG_COMMAND_RET == 200 && expr $COUNTRY_CODE : '[A-Z][A-Z]'
	then
		echo $COUNTRY_CODE > /proc/sys/dev/mtlk/wlan0/Country
		grep -v "Country" $WLAN_TMP | grep -v "EEPROMCountryValid" > /tmp/tmpwlan.conf
		echo Country = $COUNTRY_CODE >> /tmp/tmpwlan.conf
		echo EEPROMCountryValid = 1 >> /tmp/tmpwlan.conf
		mv /tmp/tmpwlan.conf $WLAN_MAIN
		cp $WLAN_MAIN $WLAN_TMP
	else
		grep -v "EEPROMCountryValid" $WLAN_TMP > /tmp/tmpwlan.conf
		echo EEPROMCountryValid = 0 >> /tmp/tmpwlan.conf
		mv /tmp/tmpwlan.conf $WLAN_MAIN
		cp $WLAN_MAIN $WLAN_TMP
	fi
fi
