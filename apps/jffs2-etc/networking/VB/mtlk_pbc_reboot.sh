#!/bin/sh
# A pushbutton daemon for factory reset / reboot
# If the reset gpio pin is defined, use it to trigger the reboot.

RESET_GPIO=`awk -F "=" '/^RESET_PB/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/HW.ini 2>/dev/null`
if [ -z "$RESET_GPIO" ] || [ "$RESET_GPIO" = "null" ]; then
	RESET_GPIO=`awk -F "=" '/^PBC_REBOOT/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /root/mtlk/etc/mtlk_init_platform.sh 2>/dev/null`
fi
case "$RESET_GPIO" in
	''|null) RESET_GPIO="" ;;
	/dev/gpio[0-9]*) : ;;
	*[!0-9]*) RESET_GPIO="" ;;
	*) RESET_GPIO="/dev/gpio$RESET_GPIO" ;;
esac

if [ -n "$RESET_GPIO" ]; then
	while [ 1 ]; do
		if [ -c "$RESET_GPIO" ]; then
			# Wait for physical Reset button press (blocking read via driver EOF)
			if cat "$RESET_GPIO" >/dev/null 2>&1; then
				echo "Reset button pressed! Restoring factory defaults..." > /dev/console
				echo 2 > /dev/gpio2 2>/dev/null || true
				if [ -x /root/mtlk/etc/mtlk_restore_defaults.sh ]; then
					/root/mtlk/etc/mtlk_restore_defaults.sh 1 1
				else
					reboot
				fi
				break
			fi
		fi
		sleep 2
	done
fi
