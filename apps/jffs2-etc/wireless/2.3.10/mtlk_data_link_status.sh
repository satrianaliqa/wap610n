#!/bin/sh
if [ $1 = 'w1' ] 
then
  echo delete tmp_wpa_state_mirr
  rm /tmp/tmp_wpa_state_mirr
else
  echo create tmp_wpa_state_mirr
	if [ -e /root/mtlk ]
	then
		/root/mtlk/etc/wpa_cli status | awk -F '=' '/^wpa_state/ {print $2}' > /tmp/tmp_wpa_state_mirr
	else
		/mnt/jffs2/etc/wpa_cli status | awk -F '=' '/^wpa_state/ {print $2}' > /tmp/tmp_wpa_state_mirr
	fi
fi

