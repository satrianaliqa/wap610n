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

if [ "$is_ap" != "0" ] && [ "$is_ap" != "1" ]; then
	echo "Invalid restore mode: $is_ap" > /dev/console
	exit 1
fi

if [ "$is_Reboot" != "0" ] && [ "$is_Reboot" != "1" ]; then
	echo "Invalid reboot mode: $is_Reboot" > /dev/console
	exit 1
fi


if [ -e /root/mtlk/etc/mtlk_init_platform.sh ]
then
	. /root/mtlk/etc/mtlk_init_platform.sh
else
	. /mnt/jffs2/etc/mtlk_init_platform.sh
fi

if [ ! -x "$ETC_PATH/mtlk_set_config.tcl" ]; then
	echo "Missing configuration restore utility" > /dev/console
	exit 1
fi

if [ -e "$TMP_CONF_FILE" ]
then
	rm -f "$TMP_CONF_FILE" || exit 1
fi

echo "ProjectName:${ProjectName}"
if [ "$is_ap" = "0" ]; then
	STA_FILE="$SAVED_CONFIG_PATH/default_sta_WAP610N.conf"
	[ ! -f "$STA_FILE" ] && STA_FILE="$SAVED_CONFIG_PATH/$STA_CONF"
else
	AP_FILE="$SAVED_CONFIG_PATH/default_ap_WAP610N.conf"
	[ ! -f "$AP_FILE" ] && AP_FILE="$SAVED_CONFIG_PATH/$AP_CONF"
fi

if [ "$is_ap" = "0" ]; then
	[ -f "$STA_FILE" ] || {
		echo "Missing STA factory configuration" > /dev/console
		exit 1
	}
	PRIMARY_CONF="$STA_FILE"
else
	[ -f "$AP_FILE" ] || {
		echo "Missing AP factory configuration" > /dev/console
		exit 1
	}
	PRIMARY_CONF="$AP_FILE"
fi

[ -f "$SAVED_CONFIG_PATH/$ADMIN_CONF" ] || {
	echo "Missing administrator factory configuration" > /dev/console
	exit 1
}
[ -f "$SAVED_CONFIG_PATH/$AUTO_AP_OFF_CONF" ] || {
	echo "Missing network factory configuration" > /dev/console
	exit 1
}

if ! cat "$PRIMARY_CONF" "$SAVED_CONFIG_PATH/$ADMIN_CONF" \
	"$SAVED_CONFIG_PATH/$AUTO_AP_OFF_CONF" > "$TMP_CONF_FILE"; then
	echo "Failed creating factory configuration" > /dev/console
	exit 1
fi

if [ -f "$CONFIGS_PATH/sys.conf" ]; then
	cp -f "$CONFIGS_PATH/sys.conf" "$CONFIGS_PATH/sys.old.conf" || exit 1
fi
if [ -f "$CONFIGS_PATH/wlan0.conf" ]; then
	cp -f "$CONFIGS_PATH/wlan0.conf" "$CONFIGS_PATH/wlan0.old.conf" || exit 1
fi

if [ -e /tmp/sys.conf ]; then rm -f /tmp/sys.conf || exit 1; fi
if [ -e /tmp/wlan0.conf ]; then rm -f /tmp/wlan0.conf || exit 1; fi

dos2unix -u "$TMP_CONF_FILE" || exit 1

if ! "$ETC_PATH/mtlk_set_config.tcl" "$TMP_CONF_FILE"; then
	[ -f "$CONFIGS_PATH/sys.old.conf" ] && cp -f "$CONFIGS_PATH/sys.old.conf" "$CONFIGS_PATH/sys.conf"
	[ -f "$CONFIGS_PATH/wlan0.old.conf" ] && cp -f "$CONFIGS_PATH/wlan0.old.conf" "$CONFIGS_PATH/wlan0.conf"
	exit 1
fi

if [ ! -f "$CONFIGS_PATH/sys.conf" ] || [ ! -f "$CONFIGS_PATH/wlan0.conf" ]; then
	[ -f "$CONFIGS_PATH/sys.old.conf" ] && cp -f "$CONFIGS_PATH/sys.old.conf" "$CONFIGS_PATH/sys.conf"
	[ -f "$CONFIGS_PATH/wlan0.old.conf" ] && cp -f "$CONFIGS_PATH/wlan0.old.conf" "$CONFIGS_PATH/wlan0.conf"
	exit 1
fi
cp "$CONFIGS_PATH/sys.conf" /tmp/sys.conf || exit 1
cp "$CONFIGS_PATH/wlan0.conf" /tmp/wlan0.conf || exit 1

# Persist restored configuration to physical flash partition (/dev/mtdblock4)
[ -x /bin/config_save.sh ] || exit 1
/bin/config_save.sh || exit 1
sync

if [ "$is_Reboot" = "1" ] || [ "$is_Reboot" = 1 ]; then
	killall -SIGINT upnpd 2>/dev/null || true
	reboot
fi
