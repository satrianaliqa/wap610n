#!/bin/sh
#Jacky.Yang 24-Jul-2008
WEBDIR=/root/mtlk/web
DIAG_LOG=/bin/diag_log.sh
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
	$DIAG_LOG "factory restore: invalid restore mode $is_ap" 2>/dev/null || true
	exit 1
fi

if [ "$is_Reboot" != "0" ] && [ "$is_Reboot" != "1" ]; then
	$DIAG_LOG "factory restore: invalid reboot mode $is_Reboot" 2>/dev/null || true
	exit 1
fi


if [ -e /root/mtlk/etc/mtlk_init_platform.sh ]
then
	. /root/mtlk/etc/mtlk_init_platform.sh
else
	. /mnt/jffs2/etc/mtlk_init_platform.sh
fi

if [ ! -x "$ETC_PATH/mtlk_set_config.tcl" ]; then
	$DIAG_LOG "factory restore: configuration generator utility is missing" 2>/dev/null || true
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
		$DIAG_LOG "factory restore: STA factory configuration is missing" 2>/dev/null || true
		exit 1
	}
	PRIMARY_CONF="$STA_FILE"
else
	[ -f "$AP_FILE" ] || {
		$DIAG_LOG "factory restore: AP factory configuration is missing" 2>/dev/null || true
		exit 1
	}
	PRIMARY_CONF="$AP_FILE"
fi

[ -f "$SAVED_CONFIG_PATH/$ADMIN_CONF" ] || {
	$DIAG_LOG "factory restore: administrator factory configuration is missing" 2>/dev/null || true
	exit 1
}
[ -f "$SAVED_CONFIG_PATH/$AUTO_AP_OFF_CONF" ] || {
	$DIAG_LOG "factory restore: network factory configuration is missing" 2>/dev/null || true
	exit 1
}

if ! cat "$PRIMARY_CONF" "$SAVED_CONFIG_PATH/$ADMIN_CONF" \
	"$SAVED_CONFIG_PATH/$AUTO_AP_OFF_CONF" > "$TMP_CONF_FILE"; then
	$DIAG_LOG "factory restore: failed creating factory configuration" 2>/dev/null || true
	exit 1
fi

if [ -f "$CONFIGS_PATH/sys.conf" ]; then
	cp -f "$CONFIGS_PATH/sys.conf" "$CONFIGS_PATH/sys.old.conf" || exit 1
fi
if [ -f "$CONFIGS_PATH/wlan0.conf" ]; then
	cp -f "$CONFIGS_PATH/wlan0.conf" "$CONFIGS_PATH/wlan0.old.conf" || exit 1
fi

dos2unix -u "$TMP_CONF_FILE" || exit 1

if ! "$ETC_PATH/mtlk_set_config.tcl" "$TMP_CONF_FILE"; then
	$DIAG_LOG "factory restore: configuration generator failed; rollback attempted" 2>/dev/null || true
	[ -f "$CONFIGS_PATH/sys.old.conf" ] && cp -f "$CONFIGS_PATH/sys.old.conf" "$CONFIGS_PATH/sys.conf" && cp -f "$CONFIGS_PATH/sys.old.conf" /tmp/sys.conf
	[ -f "$CONFIGS_PATH/wlan0.old.conf" ] && cp -f "$CONFIGS_PATH/wlan0.old.conf" "$CONFIGS_PATH/wlan0.conf" && cp -f "$CONFIGS_PATH/wlan0.old.conf" /tmp/wlan0.conf
	exit 1
fi

if [ ! -f "$CONFIGS_PATH/sys.conf" ] || [ ! -f "$CONFIGS_PATH/wlan0.conf" ]; then
	$DIAG_LOG "factory restore: generated configuration files are incomplete" 2>/dev/null || true
	[ -f "$CONFIGS_PATH/sys.old.conf" ] && cp -f "$CONFIGS_PATH/sys.old.conf" "$CONFIGS_PATH/sys.conf" && cp -f "$CONFIGS_PATH/sys.old.conf" /tmp/sys.conf
	[ -f "$CONFIGS_PATH/wlan0.old.conf" ] && cp -f "$CONFIGS_PATH/wlan0.old.conf" "$CONFIGS_PATH/wlan0.conf" && cp -f "$CONFIGS_PATH/wlan0.old.conf" /tmp/wlan0.conf
	exit 1
fi
rm -f /tmp/sys.conf /tmp/wlan0.conf
cp "$CONFIGS_PATH/sys.conf" /tmp/sys.conf || exit 1
cp "$CONFIGS_PATH/wlan0.conf" /tmp/wlan0.conf || exit 1
rm -f "$CONFIGS_PATH/sys.old.conf" "$CONFIGS_PATH/wlan0.old.conf"

# Persist restored configuration to physical flash partition (/dev/mtdblock4)
if [ ! -x /bin/config_save.sh ]; then
	$DIAG_LOG "factory restore: config_save.sh is missing" 2>/dev/null || true
	exit 1
fi
if ! /bin/config_save.sh; then
	$DIAG_LOG "factory restore: configuration flash save failed" 2>/dev/null || true
	exit 1
fi
sync

if [ "$is_Reboot" = "1" ] || [ "$is_Reboot" = 1 ]; then
	killall -SIGINT upnpd 2>/dev/null || true
	reboot
fi
