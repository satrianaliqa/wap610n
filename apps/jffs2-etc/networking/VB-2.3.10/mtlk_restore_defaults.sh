#!/bin/sh

STA_CONF=default_sta.conf
AP_CONF=default_ap.conf
ADMIN_CONF=default_admin.conf
AUTO_AP_ON_CONF=auto_ap_turn_on.conf
AUTO_AP_OFF_CONF=auto_ap_turn_off.conf
TMP_CONF_FILE=/tmp/load_config_tmp.conf
is_ap=$1
is_Reboot=$2

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
		
if [ "$is_ap" = "1" ]
then
	echo Do restore default as AP without auto_ap > /dev/console
	cat $SAVED_CONFIG_PATH/$AP_CONF $SAVED_CONFIG_PATH/$ADMIN_CONF $SAVED_CONFIG_PATH/$AUTO_AP_OFF_CONF > $TMP_CONF_FILE
elif [ "$is_ap" = "0" ]
then
	echo Do restore default as STA without auto_ap > /dev/console
	cat $SAVED_CONFIG_PATH/$STA_CONF $SAVED_CONFIG_PATH/$ADMIN_CONF $SAVED_CONFIG_PATH/$AUTO_AP_OFF_CONF > $TMP_CONF_FILE
else
	echo Do restore default as STA with auto_ap > /dev/console
	cat $SAVED_CONFIG_PATH/$STA_CONF $SAVED_CONFIG_PATH/$ADMIN_CONF $SAVED_CONFIG_PATH/$AUTO_AP_ON_CONF > $TMP_CONF_FILE
fi


mv $CONFIGS_PATH/sys.conf $CONFIGS_PATH/sys.old.conf
mv $CONFIGS_PATH/wlan0.conf $CONFIGS_PATH/wlan0.old.conf

if [ -e /tmp/sys.conf ]; then rm /tmp/sys.conf; fi
if [ -e /tmp/wlan0.conf ]; then rm /tmp/wlan0.conf; fi

dos2unix -u $TMP_CONF_FILE

$ETC_PATH/mtlk_set_config.tcl $TMP_CONF_FILE

cp $CONFIGS_PATH/sys.conf /tmp/sys.conf
cp $CONFIGS_PATH/wlan0.conf /tmp/wlan0.conf


if [ "$is_Reboot" = "1" ]
then
	killall -SIGINT upnpd
	reboot
fi
