#!/bin/sh

# Disable SA learning in the STAR internal switch, to prevent blocking of traffic
# when changing network topology. 
# (otherwise there is a 5 min timeout until aging and new topology is learned)

if [ -e /proc/str9100 ]
then
	echo "Disabling SA learning in the STAR internal switch"
	reg_debug=/proc/str9100/reg_debug
	MAC0_config_reg=0x70000008

	# Get current register value
	echo "dump $MAC0_config_reg" >> $reg_debug
	MAC0_config_val=`cat $reg_debug | awk '/content/ {print $5}'`

	# Add SA_disable bit and convert number back to hex
	SA_disable=$(( 1 << 19 ))
	MAC0_config_val=$(( $MAC0_config_val | $SA_disable ))
	MAC0_config_val=`printf "%0x" $MAC0_config_val`

	# Write new value to debug register
	echo "write 0x70000008 $MAC0_config_val" >> $reg_debug

fi
