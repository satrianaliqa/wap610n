#!/bin/sh
# A pushbutton daemon for dorango

if expr $WPS_ON = 1 || expr $NETWORK_TYPE > 2
then
	cd /root/mtlk/web

	action_type='get_conf_via_pbc'
	if expr $NETWORK_TYPE = 2
	then
		action_type='conf_via_pbc'
	fi

	while [ 1 ]
	do
		cat /dev/pbc0
		wget -q -O - "http://127.0.0.1/cgi-bin/wps.tcl?action_type=$action_type&HW_PBC=1"
	done
else
	echo 'PBCLed: cannot be active'
fi
