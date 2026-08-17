#!/bin/sh
# A pushbutton daemon for rebooting
# If the reboot gpio pin is defined, use it to trigger the reboot.

# Reading from the PB is blocking. If a value is read, trigger a SW reboot

if [ $PBC_REBOOT ]
then
	cat /dev/gpio$PBC_REBOOT
	reboot
fi
