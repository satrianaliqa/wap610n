#!/bin/sh
# mtlk_init_rd.sh
# try to download a restore default file and run it.


# Give the dongle a default IP on all the LAN interfaces
ifconfig eth0 0.0.0.0
ifconfig eth1 0.0.0.0

brctl addbr br0
# NEW Init optimization: Remove the 30 second delay of the bridge interface:
brctl setfd br0 0
brctl addif br0 eth0
brctl addif br0 eth1
ifconfig br0 2.2.2.1 netmask 255.255.255.0

# run tftp in BG and wait for 3 seconds
echo "Attempting to download system-recovery file from 2.2.2.2:2555"
tftp -g -r restore.def -l /tmp/restore.def 2.2.2.2 2555 &

sleep 3
# if there was time out, just break it

RD_SIZE=`cat /tmp/restore.def | wc -c`
if expr $RD_SIZE \> 1 
then 
	echo Restoring defaults ...
	chmod +x /tmp/restore.def
	. /tmp/restore.def
	rm /tmp/restore.def
	echo Done.

fi

# Add to bridge
ifconfig br0 down
brctl delbr br0
