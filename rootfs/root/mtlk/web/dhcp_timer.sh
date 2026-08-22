#!/bin/sh

n=1
while [ "$n" -lt $1 ]
do
	sleep 1
    	n=$(( $n + 1 ))
done

killall -HUP webs
echo 1 > /var/dhcp_timeout
echo "dhcp_timer.sh: We can't get ip address from dhcp server.($1 seconds)." > /dev/console
/root/mtlk/web/gui_wps_init.sh stop
rm -f /var/gui_wps_waiting
rm -f /var/success_get_ip
rm -f /var/assign_get_ip
rm -f /var/webCommit
