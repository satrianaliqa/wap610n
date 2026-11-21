#!/bin/sh

# Get current Passphrase
existing=""
curr=""
newPass=""

curr=`awk -F "=" '/^NonProc_WPA_Personal_PSK/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str; exit}' "$CONFIGS_PATH/wlan0.conf" 2>/dev/null`
# If it's not empty, abort
if [ -z "$curr" ]
then		
	newPass=`get_env_param ethaddr | tr "[:lower:]" "[:upper:]" | tr -d :`
	# make sure the passprase was not deleted
	if [ -z "$newPass" ]
	then
		echo "ERROR:MAC Address hasn't been found, Aborting"
		return
	else
		# writing passphrase into wlan0.conf
		grep -v "^NonProc_WPA_Personal_PSK" /tmp/wlan0.conf > /tmp/tmpwlan.conf
		echo "NonProc_WPA_Personal_PSK = $newPass" >> /tmp/tmpwlan.conf
		mv /tmp/tmpwlan.conf "$CONFIGS_PATH/wlan0.conf"
		cp "$CONFIGS_PATH/wlan0.conf" /tmp/wlan0.conf
		
		
		# change passphrase in hostapd if setup is WPA
		if [ -f "$CONFIGS_PATH/hostapd0.conf" ]; then
			old_passphrase=`grep wpa_passphrase "$CONFIGS_PATH/hostapd0.conf"`
			if [ -n "$old_passphrase" ]
			then
				sed "s|$old_passphrase|wpa_passphrase=$newPass|" "$CONFIGS_PATH/hostapd0.conf" > /tmp/tmphostapd.conf
			else
				# Insert new passphrase in correct location
				sed "/wpa_group_rekey/ i\\
wpa_passphrase=$newPass" "$CONFIGS_PATH/hostapd0.conf" > /tmp/tmphostapd.conf
			fi
			mv /tmp/tmphostapd.conf "$CONFIGS_PATH/hostapd0.conf"
			verify_pass=`grep wpa_passphrase "$CONFIGS_PATH/hostapd0.conf" | awk -F "=" '{print $2}'`
			if [ "$verify_pass" != "$newPass" ]; then
				echo ERROR: Writing passphrase into the hostapd0.conf is failed
				return
			fi
		fi
		
		# Files will be saved only once in init process, Flag will down by the mtlk_init.sh
		if [ ! -e /tmp/NeedSave.txt ]
		then
			echo 1 > /tmp/NeedSave.txt
		fi	
	fi
fi



