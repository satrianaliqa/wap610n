#!/bin/sh

if [ $WPS_PB = 1 ]
then
	# Start the GPIO driver for WPS PB and LEDs
	if [ $GPIO_LEDS ] 
	then
		insmod ./gpio.$MOD_EXT outputLeds=$GPIO_LEDS pushButtons=$GPIO_PUSHBUTTONS
	else
		insmod ./gpio.$MOD_EXT 
	fi
fi

if [ $LEDMAN = 1 ]
then
	# Start the LEDs and LED manager
	## TODO: Change the path of led.o if it is part of the rootfs
	insmod ./led.$MOD_EXT
	nice -n 15 ./mtlk_ledman.sh &
fi

# Start the DHCP Client
nice -n 18 ./dhcp.tcl startup &

# Start the DHCP Server
DHCPD_ENABLED=`awk -F "=" '/^DHCPDEnabled/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
if [ $DHCPD_ENABLED ]
then
	if [ $DHCPD_ENABLED = 1 ]
	then
		./mtlk_init_dhcpd.sh 
		nice -n 18 udhcpd /tmp/udhcpd.conf
	fi
fi

# Start the upnp Daemon
nice -n 18 upnpd &

# Start the SW reboot daemon (This will do nothing if board supports HW reboot)
./mtlk_pbc_reboot.sh &

# Turn on loop-breaking features in the STA. 
LOOP_BREAKING_ENABLED=`awk -F "=" '/^LoopBreakingEnabled/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
if [ ! $LOOP_BREAKING_ENABLED ] 
then
	LOOP_BREAKING_ENABLED=0
fi
if [ $LOOP_BREAKING_ENABLED -eq 1 -a $NETWORK_TYPE -eq 0 ]
then
	# Turn on the STP-like loop breaking feature 
	nice -n 18 ./mtlk_stp.sh &

	# Start the carrier detect signal, needed for bringing the eth device down and up when reconnecting the port
	nice -n 18 ./mtlk_carrier_detect.sh &
fi
