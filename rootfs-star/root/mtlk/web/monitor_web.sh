#!/bin/sh
# Real-time Web Server Watchdog for Cisco WAP610N

while true
do
    # Check if webs is running and not a defunct zombie
    WEBS_RUNNING=`ps | grep -v grep | grep webs | grep -v '\[webs\]'`
    if [ -z "$WEBS_RUNNING" ]; then
        touch /root/mtlk/web/lang/STRINGS_EN.txt 2>/dev/null || true
        ln -sf STRINGS_EN.txt /root/mtlk/web/lang/STRINGS_.txt 2>/dev/null || true
        cd /root/mtlk/web
        ./webs >/dev/null 2>&1 &
    fi
    sleep 2
done
