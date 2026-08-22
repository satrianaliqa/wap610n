#!/bin/sh

CONFIGURED_SSID=`awk -F "= " '/^NonProc_ESSID/ {str = $2; sub(/\r/, "", str); print str}' /mnt/jffs2/wlan0.conf`
if [ "$CONFIGURED_SSID" == "" ]; then
	CONFIGURED_SSID=`awk -F "=" '/^NonProc_ESSID/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /mnt/jffs2/wlan0.conf`
fi
#june.chen 2010/11/15, for solving WPA key could not accept '=' and blank space. 
#The format is NonProc_WPA_Personal_PSK = <WPA Key>, so we just begin substring form index 28 to eliminate prefix "NonProc_WPA_Personal_PSK = "(length 27)
#CONFIGURED_WPAPSK_KEY=`awk -F "=" '/^NonProc_WPA_Personal_PSK/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /mnt/jffs2/wlan0.conf`
CONFIGURED_WPAPSK_KEY=`awk '/^NonProc_WPA_Personal_PSK/ {str = substr($0, 28); print str}' /mnt/jffs2/wlan0.conf`

/tmp/wpa_passphrase "$CONFIGURED_SSID" "$CONFIGURED_WPAPSK_KEY"
