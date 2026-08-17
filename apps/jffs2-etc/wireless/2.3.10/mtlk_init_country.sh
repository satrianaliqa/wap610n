#!/bin/sh

echo "Start mtlk_init_contry.sh" > /dev/console

#
# This script takes the country code from the EEPROM and configures the driver with it. In addition, it updates the wlan0.conf with the new Country code if relevant, and a variable is used to signal that the EEPROMCountry is valid or non valid.
# This implementation is relevant for wlan0.conf only. For other configuration methods this file should be modified.


WLAN_PATH=$1

WLAN_MAIN=${WLAN_PATH}/wlan0.conf
WLAN_TMP=/tmp/wlan0.conf

eeprom_country=`iwpriv wlan0 gEEPROM | grep country`

if [ $? = 0 ]
then
	COUNTRY_CODE=`echo $eeprom_country | awk '{print $3 }'`
	
	if [ $COUNTRY_CODE ] && expr $COUNTRY_CODE : '[A-Z][A-Z]'
	then
		grep -v "Country" $WLAN_TMP | grep -v "EEPROMCountryValid" > /tmp/tmpwlan.conf
		echo EEPROMCountryValid = 1 >> /tmp/tmpwlan.conf
		mv /tmp/tmpwlan.conf $WLAN_MAIN
		cp $WLAN_MAIN $WLAN_TMP
	else
		grep -v "EEPROMCountryValid" $WLAN_TMP > /tmp/tmpwlan.conf
		echo EEPROMCountryValid = 0 >> /tmp/tmpwlan.conf
		mv /tmp/tmpwlan.conf $WLAN_MAIN
		cp $WLAN_MAIN $WLAN_TMP
	fi
	
	# Files will be saved only once in init process, Flag will down by the mtlk_init.sh
	if [ ! -e /tmp/NeedSave.txt ]
	then
		echo 1 > /tmp/NeedSave.txt
	fi
fi
