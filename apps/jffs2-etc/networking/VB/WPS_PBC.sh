#!/bin/sh
# A pushbutton daemon for dorango
# Hardware Safe-Shutdown button monitor daemon for WAP610N

# Wait for /tmp/HW.ini to be generated
for i in 1 2 3 4 5; do
	[ -f /tmp/HW.ini ] && break
	sleep 1
done

WPS_PBC_GPIO=`awk -F "=" '/^WPS_PB/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/HW.ini 2>/dev/null`

if [ -z "$WPS_PBC_GPIO" ] || [ "$WPS_PBC_GPIO" = "null" ]; then
	for g in /dev/gpio10 /dev/gpio1 /dev/gpio0 /dev/gpio14; do
		if [ -e "$g" ]; then
			WPS_PBC_GPIO="$g"
			break
		fi
	done
fi

if [ -n "$WPS_PBC_GPIO" ] && [ -e "$WPS_PBC_GPIO" ]; then
	echo "--> [Hardware Daemon] WPS Safe-Shutdown Monitor started on $WPS_PBC_GPIO" > /dev/console
	while true
	do
		# Wait for physical WPS button press
		cat $WPS_PBC_GPIO > /dev/null
		echo "[HARDWARE EVENT] WPS Button Pressed! Executing Safe System Shutdown..." > /dev/console
		
		# Turn off activity LEDs
		echo 0 > /dev/led0 2>/dev/null || true
		echo 0 > /dev/led1 2>/dev/null || true
		
		# Sync storage buffers and safely halt CPU
		sync
		poweroff
	done
else
	echo "WPS pushbutton: not active (GPIO device not found)" > /dev/console
fi
