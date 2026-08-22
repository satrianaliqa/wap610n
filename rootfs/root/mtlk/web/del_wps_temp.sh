#!/bin/sh

sleep 5
echo del_wps_temp.sh: delete /var/gui_wps_waiting > /dev/console
rm -f /var/gui_wps_waiting
