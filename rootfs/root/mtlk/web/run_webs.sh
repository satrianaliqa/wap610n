#!/bin/sh
# Supervisor wrapper for GoAhead Web Server (managed by /etc/inittab respawn)

touch /root/mtlk/web/lang/STRINGS_EN.txt 2>/dev/null || true
ln -sf STRINGS_EN.txt /root/mtlk/web/lang/STRINGS_.txt 2>/dev/null || true
rm -f /var/old_web_pid /var/startToWait 2>/dev/null || true

# Pre-seed non-blocking idle WPS status so legacy scripts never block
echo "0,-1" > /var/WPS_History 2>/dev/null || true
echo "0,-1" > /var/WPS_History_temp 2>/dev/null || true

cd /root/mtlk/web
exec ./webs
