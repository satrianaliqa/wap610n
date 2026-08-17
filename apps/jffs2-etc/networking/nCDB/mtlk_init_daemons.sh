#!/bin/sh


if expr $LEDMAN = 1
then
	# Start the LEDs and LED manager
	## TODO: Change the path of led.o if it is part of the rootfs
	insmod ./led.$MOD_EXT
	nice -n 15 ./mtlk_ledman.sh &
fi

# start the DHCP client
/etc/udhcpc/dhcp.tcl startup &

# Start the upnp Daemon
upnpd &
