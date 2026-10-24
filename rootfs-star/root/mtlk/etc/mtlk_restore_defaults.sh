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

# Enforce WAP610N Access Point identity
ProjectName="WAP610N"



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
if [ "$is_ap" = "0" ]; then
	STA_FILE="$SAVED_CONFIG_PATH/default_sta_WAP610N.conf"
	[ ! -f "$STA_FILE" ] && STA_FILE="$SAVED_CONFIG_PATH/$STA_CONF"
	cat "$STA_FILE" $SAVED_CONFIG_PATH/$ADMIN_CONF $SAVED_CONFIG_PATH/$AUTO_AP_OFF_CONF > $TMP_CONF_FILE
else
	AP_FILE="$SAVED_CONFIG_PATH/default_ap_WAP610N.conf"
	[ ! -f "$AP_FILE" ] && AP_FILE="$SAVED_CONFIG_PATH/$AP_CONF"
	cat "$AP_FILE" $SAVED_CONFIG_PATH/$ADMIN_CONF $SAVED_CONFIG_PATH/$AUTO_AP_OFF_CONF > $TMP_CONF_FILE
fi


mv $CONFIGS_PATH/sys.conf $CONFIGS_PATH/sys.old.conf
mv $CONFIGS_PATH/wlan0.conf $CONFIGS_PATH/wlan0.old.conf

if [ -e /tmp/sys.conf ]; then rm /tmp/sys.conf; fi
if [ -e /tmp/wlan0.conf ]; then rm /tmp/wlan0.conf; fi

dos2unix -u $TMP_CONF_FILE

$ETC_PATH/mtlk_set_config.tcl $TMP_CONF_FILE

cp $CONFIGS_PATH/sys.conf /tmp/sys.conf
cp $CONFIGS_PATH/wlan0.conf /tmp/wlan0.conf

# Persist restored configuration to physical flash partition (/dev/mtdblock4)
if [ -x /bin/config_save.sh ]; then
	/bin/config_save.sh
fi
sync

if [ "$is_Reboot" = "1" ] || [ "$is_Reboot" = 1 ]; then
	killall -SIGINT upnpd 2>/dev/null || true
	reboot
fi
