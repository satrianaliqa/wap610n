#!/bin/sh

CONFIGURED_SSID="$1"
CONFIGURED_WPAPSK_KEY="$2"

if [ -z "$CONFIGURED_SSID" ]; then
	CONFIGURED_SSID=`awk -F "= " '/^NonProc_ESSID/ {str = $2; sub(/\r/, "", str); print str}' /mnt/jffs2/wlan0.conf`
	if [ "$CONFIGURED_SSID" = "" ]; then
		CONFIGURED_SSID=`awk -F "=" '/^NonProc_ESSID/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /mnt/jffs2/wlan0.conf`
	fi
fi

if [ -z "$CONFIGURED_WPAPSK_KEY" ]; then
	#june.chen 2010/11/15, for solving WPA key could not accept '=' and blank space. 
	#The format is NonProc_WPA_Personal_PSK = <WPA Key>, so we just begin substring form index 28 to eliminate prefix "NonProc_WPA_Personal_PSK = "(length 27)
	CONFIGURED_WPAPSK_KEY=`awk '/^NonProc_WPA_Personal_PSK/ {str = substr($0, 28); sub(/\r/, "", str); print str}' /mnt/jffs2/wlan0.conf`
fi

WPA_TOOL=/root/mtlk/etc/wpa_passphrase
[ -x "$WPA_TOOL" ] || WPA_TOOL=/tmp/wpa_passphrase
[ -x "$WPA_TOOL" ] || WPA_TOOL=wpa_passphrase

"$WPA_TOOL" "$CONFIGURED_SSID" "$CONFIGURED_WPAPSK_KEY"
