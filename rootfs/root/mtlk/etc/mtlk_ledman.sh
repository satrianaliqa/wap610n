#!/bin/tclsh

# This simple script turns on the LEDs, according to whether there is a connection.
# led0 is the power led
# led1 is the connection led
# led2 is the Tx led
# led3 is the Rx led

# Make sure the power led is on
set LED_OFF 0
set TX_TRAFFIC_LED 3456
set RX_TRAFFIC_LED 5678
set CONNECTION_LED 6789
exec echo 1 > /dev/led0


# The connected flag prevents unnecessary rewrites to the proc
set connected ""

while {1==1} {
	
		set cur_connect 0
		
		if {![catch {set connected_str [exec ./mtdump wlan0 constatus]}]} {
		
			# search for a MAC address in the connection string
			if {[regexp {(..:..:..:..:..:..)} $connected_str all mac]} {
				set cur_connect 1
			}


	}	
	
		
	if {$cur_connect == 0} {
		if {$connected != 0} {
               exec echo $LED_OFF > /dev/led1
               exec echo $LED_OFF > /dev/led2
               exec echo $LED_OFF > /dev/led3
			set connected 0
		}
	} else {
		if {$connected != 1} {
               exec echo $CONNECTION_LED > /dev/led1
               exec echo $TX_TRAFFIC_LED > /dev/led2
               exec echo $RX_TRAFFIC_LED > /dev/led3
			set connected 1

			# Safety feature for demos:
			# If connected once, exit, so that the LEDs aren't turned off...
			# (In a real product, it would be nice to leave the LED manager running,
			# to know when disconnected.)
			#exit
		}
	}
	sleep 3
}
