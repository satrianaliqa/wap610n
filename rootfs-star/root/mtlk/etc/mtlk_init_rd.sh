#!/bin/sh
# mtlk_init_rd.sh
# try to download a restore default file and run it.


# Give the VB a default IP on the LAN interface
ifconfig eth0 2.2.2.1 netmask 255.255.255.0

# run tftp in bg and wait for 3 seconds
echo "Attempting to download system-recovery file from 2.2.2.2:2555"
tftp -g -r restore.def -l /tmp/restore.def 2.2.2.2 2555 &

sleep 3
# if there was time out, just break it

if [ -s /tmp/restore.def ]
then 
	echo Restoring defaults ...
	chmod +x /tmp/restore.def
	. /tmp/restore.def
	rm /tmp/restore.def
	echo Done.

fi

# Add to bridge
ifconfig eth0 down
