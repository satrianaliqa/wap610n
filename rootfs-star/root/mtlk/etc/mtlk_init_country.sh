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

if [ -n "$CONFIG_COMMAND_RESULT" ]
then
	CONFIG_COMMAND_RET=`echo "$CONFIG_COMMAND_RESULT" | awk -F "," '{gsub(/ /, "", $2); print $2}'`
	COUNTRY_CODE=`echo "$CONFIG_COMMAND_RESULT" | awk -F "," '{gsub(/ /, "", $3); print $3}'`

#june.chen, 2011-08-05, workaround for FR domain, since we need to see channel 120-132, but the driver seems to have problem.
#So when see FR domain, just change it to antoher country useing Europe domain, say IT(Italy).
	if [ "$COUNTRY_CODE" = "FR" ]; then
		COUNTRY_CODE="IT"
	fi
#end june.chen
	
	case "$COUNTRY_CODE" in
		[A-Z][A-Z]) valid_cc=1 ;;
		*) valid_cc=0 ;;
	esac

	if [ "$CONFIG_COMMAND_RET" = "200" ] && [ "$valid_cc" = "1" ]
	then
		echo "$COUNTRY_CODE" > /proc/sys/dev/mtlk/wlan0/Country
		grep -v '^[ \t]*Country[ \t]*=' "$WLAN_TMP" | grep -v '^[ \t]*EEPROMCountryValid[ \t]*=' > /tmp/tmpwlan.conf
		echo "Country = $COUNTRY_CODE" >> /tmp/tmpwlan.conf
		echo "EEPROMCountryValid = 1" >> /tmp/tmpwlan.conf
	else
		grep -v '^[ \t]*EEPROMCountryValid[ \t]*=' "$WLAN_TMP" > /tmp/tmpwlan.conf
		echo "EEPROMCountryValid = 0" >> /tmp/tmpwlan.conf
	fi
	if [ -s /tmp/tmpwlan.conf ]; then
		[ -x /bin/config_lock.sh ] && /bin/config_lock.sh >/dev/null 2>&1 || true
		cp -f /tmp/tmpwlan.conf "$WLAN_MAIN" 2>/dev/null && mv -f /tmp/tmpwlan.conf "$WLAN_TMP" 2>/dev/null
		[ -x /bin/config_unlock.sh ] && /bin/config_unlock.sh >/dev/null 2>&1 || true
	fi
	rm -f /tmp/tmpwlan.conf 2>/dev/null || true
	
	# Files will be saved only once in init process, Flag will down by the mtlk_init.sh
	if [ ! -e /tmp/NeedSave.txt ]
	then
		echo 1 > /tmp/NeedSave.txt
	fi
fi
