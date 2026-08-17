#!/bin/sh

# This init script tries to download all wireless images, if needed
# For replacing wireless from tftp
# Currently, only driver and images are brought - not init scripts, etc.

DOWNLOAD_WLAN=`awk -F "=" '/^tftp_load_wlan/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`
IP_TFTPD=`awk -F "=" '/^ip_tftpd/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf`

DOWNLOAD_WC=`echo $DOWNLOAD_WLAN | wc -c`
if expr $DOWNLOAD_WC = 1 
then
	DOWNLOAD_WLAN=0
fi

# TODO: Add error handling if tftp fails. In this case - avoid loading driver?
download ()
{
	path=$1
	cd $path
	for file in `ls`
	do
		tftp -gr $file $IP_TFTPD
	done
	cd -
}

if expr $DOWNLOAD_WLAN = 1
then
	download $IMAGES_PATH
	download $DRIVER_PATH

fi
