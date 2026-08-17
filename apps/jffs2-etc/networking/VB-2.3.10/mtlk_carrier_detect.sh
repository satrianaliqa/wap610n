#!/bin/sh

# Detect if carrier signal is dropped on ethernet.
# If so, bring the device up and down.
# This will cause the device to reenter the disabled and forwarding 
# states in the bridge, in case it was blocked
# Disabling br0 (instead of just eth0) will also clear the arp table, 
# so that new values can be learned.
# This code is specific to the STAR platform, which has no carrier detect support in the eth driver
# TODO: On dongle: How do you detect carrier detect to clear the L2Nat table?
# Note: This code requires compiling busybox ash with math support


# Only run on star platform

if [ -e /proc/str9100 ]
then

	reg_debug=/proc/str9100/reg_debug
	addr=0x70000008


	while [ 1 ]
	do
		echo "dump $addr" >> $reg_debug
		REG_DEBUG=`cat $reg_debug | awk '/content/ {print $5}'`

		if [  $(($REG_DEBUG & 1)) = 0 ] 
		then
			# Bring br0 down and up - because carrier detect was lost
			ifconfig br0 down
			sleep 3

		# Clean up the wls l2nat table
		if [ -e /proc/net/mtlk/wlan0/Debug/L2NAT_ClearTable ] 
		then
			echo 1 > /proc/net/mtlk/wlan0/Debug/L2NAT_ClearTable
		fi
		
			ifconfig br0 up
			sleep 3
		fi
		sleep 1
	done

fi
