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

# TODO: You don't have to immediately bring up br0. You can leave it down, and bring it up again
# after carrier is detected. (Already tested: The phy status bit works even if br0 is down)
# i.e. implement a state machine with two states: carrier down and carrier up, and the transition
# between states will call ifconfig down+l2nat_clear, or ifconfig up

# Set registers according to HW type - Star91xx or Star81xx

if [ ! -e /proc/str9100 ] && [ ! -e /proc/str8131 ]; then
	echo Skipping carrier detect mechanism on this platform.
	exit 0
fi

while [ 1 ]
do
	if [ -e /proc/str9100 ]; then
		REG_DEBUG=`echo "dump 0x70000008" >> /proc/str9100/reg_debug; cat /proc/str9100/reg_debug | awk '/content/ {print $5}'`
	elif [ -e /proc/str8131 ]; then
		REG_DEBUG=`echo "dump 0x70000004" >> /proc/str8131/reg_debug; cat /proc/str8131/reg_debug | awk '/content/ {print $5}'`
	fi
	
	if [ ! "$REG_DEBUG" = "" ]; then
		break
	fi

	sleep 1
done

DOWN_UP_BY_FORCE_RENEW_IP=0

PORT_STATUS_PREVIOUS=$(($REG_DEBUG & 1))

while [ 1 ]
do
	if [ -e /proc/str9100 ]; then
		REG_DEBUG=`echo "dump 0x70000008" >> /proc/str9100/reg_debug; cat /proc/str9100/reg_debug | awk '/content/ {print $5}'`
	elif [ -e /proc/str8131 ]; then
		REG_DEBUG=`echo "dump 0x70000004" >> /proc/str8131/reg_debug; cat /proc/str8131/reg_debug | awk '/content/ {print $5}'`
	fi
	
	if [ ! "$REG_DEBUG" = "" ]; then

		PORT_STATUS_NEXT=$(($REG_DEBUG & 1))

		if [ $PORT_STATUS_NEXT -lt  $PORT_STATUS_PREVIOUS ] && [ -e /var/run/force_ethpc_renew_ip ]; then
			#echo "##### Set DOWN_UP_BY_FORCE_RENEW_IP to 1" > /dev/console
			DOWN_UP_BY_FORCE_RENEW_IP=1
		fi

		if [ $PORT_STATUS_NEXT -gt  $PORT_STATUS_PREVIOUS ]; then
			ETH0_STATUS=`brctl showstp br0 | grep "port id" | grep 8001 | awk '{print $5}'`
			if [ "$ETH0_STATUS" = "blocking" ]; then
				# Bring br0 down and up - because carrier detect was lost
				echo "Carrier detect down" - resetting bridge > /dev/console
				ifconfig br0 down

				sleep 3

				# Clean up the wls l2nat table
				if [ -e /proc/net/mtlk/wlan0/Debug/L2NAT_ClearTable ]; then
					echo 1 > /proc/net/mtlk/wlan0/Debug/L2NAT_ClearTable
				fi

				ifconfig br0 up

				sleep 3
			fi

			#If the MAC Clone mode is configured to AUTO,
			#then DUT need to re-learning a MAC address after ethernet cable be re-pluged.
			#Ricky CAO
			#MAC_CLONE_MODE=`awk -F "=" '/^MacCloneMode/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
			#CLONED_MAC=`awk -F "=" '/^ClonedMAC/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/wlan0.conf`
			
			#if [ $MAC_CLONE_MODE = 1 ] && [ $DOWN_UP_BY_FORCE_RENEW_IP = 0 ]; then
			#	echo "Clear cloned MAC in bridge" > /dev/console
			#	echo "00:00:00:00:00:00" > /sys/class/net/br0/bridge/cloned_mac

			#	sleep 2

				#Run the MAC Clone Handler Script when MAC Clone is configured to AUTO mode.
			#	/root/mtlk/etc/mac_clone_handler.sh &
			#fi

			if [ $DOWN_UP_BY_FORCE_RENEW_IP = 1 ]; then
				#echo "##### Clear DOWN_UP_BY_FORCE_RENEW_IP to 0" > /dev/console
				DOWN_UP_BY_FORCE_RENEW_IP=0
			fi
		fi

		PORT_STATUS_PREVIOUS=$PORT_STATUS_NEXT

	fi

	sleep 1
done
