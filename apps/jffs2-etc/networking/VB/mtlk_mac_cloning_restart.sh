#!/bin/sh


# This script reloads the wireless interface in the case of mac cloning.
# It is called if mac cloning mode is specified, but the wireless interface was brought up 
# before a packet was sniffed over ethernet.
# It is run in the background, to allow the system to wait for a packet to be sniffed.


# Wait for a packet to be sniffed
while [ ! -e /tmp/mac_cloning.addr ]
do
	sleep 2
done

# Reload the wireless
echo Sniffed MAC address from eth packet - Reloading wireless driver with cloned MAC
./reload_mtlk_driver.sh > /tmp/reload.log