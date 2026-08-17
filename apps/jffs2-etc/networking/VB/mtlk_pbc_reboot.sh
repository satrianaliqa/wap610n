#!/bin/sh
# A pushbutton daemon for rebooting
# If the reboot gpio pin is defined, use it to trigger the reboot.

# Reading from the PB is blocking. If a value is read, trigger a SW reboot

#Revised by Ricky CAO on Aug. 27 2009
PBC_REBOOT_GPIO=`awk -F "=" '/^PBC_REBOOT/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /root/mtlk/etc/mtlk_init_platform.sh`

if [ $PBC_REBOOT_GPIO ]
then
#by Ricky CAO on Aug. 27 2009
	cat /dev/gpio$PBC_REBOOT_GPIO
#Tim Wang, PBC_RESET_TO_DEFAULT	
	echo 2 > /dev/gpio2
	#./mtlk_restore_defaults.sh
	echo killconfig > /dev/mtdblock4
	reboot
fi
