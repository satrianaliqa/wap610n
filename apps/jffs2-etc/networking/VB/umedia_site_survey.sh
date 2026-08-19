#!/bin/sh

echo "Going site survey .."
echo 1 > /var/hnapSiteSurvey
cd /root/mtlk/web/
/root/mtlk/web/init_security.tcl reactivate
sleep 2
/sbin/iwlist wlan0 scan > /var/ap_list
rm /var/hnapSiteSurvey
