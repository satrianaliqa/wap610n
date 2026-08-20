#!/bin/sh
# A pushbutton daemon for factory reset / reboot
# If the reboot gpio pin is defined, use it to trigger the reboot.

PBC_REBOOT_GPIO=`awk -F "=" '/^PBC_REBOOT/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /root/mtlk/etc/mtlk_init_platform.sh 2>/dev/null`

if [ -n "$PBC_REBOOT_GPIO" ]; then
	while [ 1 ]; do
		if [ -c "/dev/gpio${PBC_REBOOT_GPIO}" ]; then
			PBC_VAL=$(head -c 1 "/dev/gpio${PBC_REBOOT_GPIO}" 2>/dev/null)
			if [ "$PBC_VAL" = "1" ]; then
				echo "Reset button pressed! Restoring factory defaults..." > /dev/console
				echo 2 > /dev/gpio2 2>/dev/null || true
				if [ -x /root/mtlk/etc/mtlk_restore_defaults.sh ]; then
					/root/mtlk/etc/mtlk_restore_defaults.sh 0 1
				else
					reboot
				fi
				break
			fi
		fi
		sleep 2
	done
fi
