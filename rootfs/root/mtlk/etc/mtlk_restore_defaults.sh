#!/bin/sh
#Jacky.Yang 24-Jul-2008
WEBDIR=/root/mtlk/web
. ${WEBDIR}/fw_version.txt
ProjectName=${ProjectName}

STA_CONF=default_sta.conf
AP_CONF=default_ap.conf
ADMIN_CONF=default_admin.conf
AUTO_AP_ON_CONF=auto_ap_turn_on.conf

#Jacky.Yang 23-Jul-2008, Disable auto detect lan mode
AUTO_AP_OFF_CONF=auto_ap_turn_off.conf
TMP_CONF_FILE=/tmp/load_config_tmp.conf
is_ap=$1
is_Reboot=$2

# Jacky.Yang 30-Jun-2009, Get HW type.
if [ "$ProjectName" != "WAP610N" ]; then
	REGISTER3=`echo "read_phy" >> /proc/str9100/reg_debug ; cat /proc/str9100/reg_debug | awk -F= '/reg/ {str = $5;  print str}'`

	#echo "@@@@@ reg3 in /proc/str9100/reg_debug is $REGISTER3 @@@@@" > /dev/console
	if [ "$REGISTER3" = "0xc852" ]; then
		#echo "ProjectName:${ProjectName}"
		ProjectName="WES610N"
	else
		#echo "ProjectName:${ProjectName}"
		ProjectName="WET610N"
	fi
fi



if [ -e /root/mtlk/etc/mtlk_init_platform.sh ]
then
	. /root/mtlk/etc/mtlk_init_platform.sh
else
	. /mnt/jffs2/etc/mtlk_init_platform.sh
fi

if [ -e $TMP_CONF_FILE ]
then
	rm $TMP_CONF_FILE
fi

echo "ProjectName:${ProjectName}"
if [ "${ProjectName}" = "WAP610N" ]; then
	AP_FILE="$SAVED_CONFIG_PATH/default_ap_WAP610N.conf"
	[ ! -f "$AP_FILE" ] && AP_FILE="$SAVED_CONFIG_PATH/$AP_CONF"
	cat "$AP_FILE" $SAVED_CONFIG_PATH/$ADMIN_CONF $SAVED_CONFIG_PATH/$AUTO_AP_OFF_CONF > $TMP_CONF_FILE
elif [ "${ProjectName}" = "WET610N" ]; then
	STA_FILE="$SAVED_CONFIG_PATH/default_sta_WET610N.conf"
	[ ! -f "$STA_FILE" ] && STA_FILE="$SAVED_CONFIG_PATH/$STA_CONF"
	cat "$STA_FILE" $SAVED_CONFIG_PATH/$ADMIN_CONF $SAVED_CONFIG_PATH/$AUTO_AP_OFF_CONF > $TMP_CONF_FILE
elif [ "${ProjectName}" = "WES610N" ]; then
	STA_FILE="$SAVED_CONFIG_PATH/default_sta_WES610N.conf"
	[ ! -f "$STA_FILE" ] && STA_FILE="$SAVED_CONFIG_PATH/$STA_CONF"
	cat "$STA_FILE" $SAVED_CONFIG_PATH/$ADMIN_CONF $SAVED_CONFIG_PATH/$AUTO_AP_OFF_CONF > $TMP_CONF_FILE
else
	cat $SAVED_CONFIG_PATH/$AP_CONF $SAVED_CONFIG_PATH/$ADMIN_CONF $SAVED_CONFIG_PATH/$AUTO_AP_ON_CONF > $TMP_CONF_FILE
fi


mv $CONFIGS_PATH/sys.conf $CONFIGS_PATH/sys.old.conf
mv $CONFIGS_PATH/wlan0.conf $CONFIGS_PATH/wlan0.old.conf

if [ -e /tmp/sys.conf ]; then rm /tmp/sys.conf; fi
if [ -e /tmp/wlan0.conf ]; then rm /tmp/wlan0.conf; fi

dos2unix -u $TMP_CONF_FILE

$ETC_PATH/mtlk_set_config.tcl $TMP_CONF_FILE

cp $CONFIGS_PATH/sys.conf /tmp/sys.conf
cp $CONFIGS_PATH/wlan0.conf /tmp/wlan0.conf


if [ "$is_Reboot" == "1" ]
then
	killall -SIGINT upnpd
	reboot
fi
