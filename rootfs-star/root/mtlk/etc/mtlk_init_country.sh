#!/bin/sh

#
# This script takes the country code from the EEPROM and configures the driver with it. In addition, it updates the wlan0.conf with the new Country code if relevant, and a variable is used to signal that the EEPROMCountry is valid or non valid.
# This implementation is relevant for wlan0.conf only. For other configuration methods this file should be modified.


WLAN_PATH=$1
WLAN_INDEX=$2

WLAN_MAIN=${WLAN_PATH}/wlan${WLAN_INDEX}.conf
WLAN_TMP=/tmp/wlan${WLAN_INDEX}.conf

echo 025,0x03,EEPROM,HWCountry > /proc/sys/dev/mtlk/wlan0/ConfigCommand
CONFIG_COMMAND_RESULT=`cat /proc/sys/dev/mtlk/wlan0/ConfigCommand`

if [ $CONFIG_COMMAND_RESULT ]
then
	CONFIG_COMMAND_RET=`echo $CONFIG_COMMAND_RESULT | awk -F "," '{print $2 }'`
	COUNTRY_CODE=`echo $CONFIG_COMMAND_RESULT | awk -F "," '{print $3 }'`

#june.chen, 2011-08-05, workaround for FR domain, since we need to see channel 120-132, but the driver seems to have problem.
#So when see FR domain, just change it to antoher country useing Europe domain, say IT(Italy).
	if [ $COUNTRY_CODE = "FR"  ]; then
		COUNTRY_CODE="IT"
	fi
#end june.chen
	
	if expr $CONFIG_COMMAND_RET == 200 && [ $COUNTRY_CODE ] && expr $COUNTRY_CODE : '[A-Z][A-Z]'
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
	
	# Files will be saved only once in init process, Flag will down by the mtlk_init.sh
	if [ ! -e /tmp/NeedSave.txt ]
	then
		echo 1 > /tmp/NeedSave.txt
	fi
fi
