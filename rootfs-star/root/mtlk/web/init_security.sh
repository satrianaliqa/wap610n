#!/bin/sh
#
# init_security.sh - POSIX shell replacement for init_security.tcl + update_security.tcl
# Eliminates heavy tinytcl startup overhead while preserving exact hostapd0.conf,
# wpa_supplicant0.conf, /proc/sys/dev/mtlk/wlan0, and flash persistence behavior.

WPA_CONFIG_DIR="/mnt/jffs2/etc"
WPA_APP_DIR="/mnt/jffs2/etc"
CONFIG_DIR="/mnt/jffs2"
SAVED_CONFIGS_DIR="/mnt/jffs/saved_configs"
PROC_ROOT_BASE="/proc/sys/dev/mtlk/wlan"

WLAN_IDX="0"
SET_SECURITY_ONLY="0"
REACTIVATE="0"
NO_SAVE="0"

case "${1:-}" in
	0|1)
		WLAN_IDX="$1"
		;;
	Set_security_only)
		SET_SECURITY_ONLY="1"
		NO_SAVE="1"
		;;
	reactivate)
		REACTIVATE="1"
		NO_SAVE="1"
		;;
esac

CONFIG_FILE="${CONFIG_DIR}/wlan${WLAN_IDX}.conf"
SYS_CONFIG_FILE="${CONFIG_DIR}/sys.conf"
WPA_CONF_FILE="${WPA_CONFIG_DIR}/wpa_supplicant${WLAN_IDX}.conf"
HOSTAPD_CONF_FILE="${WPA_CONFIG_DIR}/hostapd${WLAN_IDX}.conf"
WPA_SUPPLICANT="${WPA_APP_DIR}/wpa_supplicant"
HOSTAPD="${WPA_APP_DIR}/hostapd"
MTPRIV="${WPA_APP_DIR}/mtpriv"
CERT_FILE="${SAVED_CONFIGS_DIR}/certificate.pem"
IFACE="wlan${WLAN_IDX}"

get_param() {
	key="$1"
	val=""
	if [ -f "$CONFIG_FILE" ]; then
		val=$(awk -v k="$key" '$0 ~ "^[ \t]*" k "[ \t]*=" { idx=index($0,"="); s=substr($0,idx+1); sub(/^ ?/,"",s); sub(/\r$/,"",s); print s; exit }' "$CONFIG_FILE" 2>/dev/null)
	fi
	if [ -z "$val" ] && [ -f "$SYS_CONFIG_FILE" ]; then
		val=$(awk -v k="$key" '$0 ~ "^[ \t]*" k "[ \t]*=" { idx=index($0,"="); s=substr($0,idx+1); sub(/^ ?/,"",s); sub(/\r$/,"",s); print s; exit }' "$SYS_CONFIG_FILE" 2>/dev/null)
	fi
	printf '%s' "$val"
}

set_proc_var() {
	"$MTPRIV" "$IFACE" "$2" "$1" >/dev/null 2>&1 || true
}

strip_cr_lf() {
	printf '%s' "$1" | tr -d '\r\n'
}

wpa_escape() {
	printf '%s' "$1" | tr -d '\r\n' | sed -e 's/\\/\\\\/g' -e 's/"/\\"/g'
}

write_hostapd_conf() {
	enc_mode="$1"
	ssid=$(strip_cr_lf "$2")
	wep_len="$3"
	auth="$4"
	used_key="$5"
	k1="$6"
	k2="$7"
	k3="$8"
	k4="$9"
	shift 9
	wpa_mode="$1"
	encap_mode="$2"
	psk=$(strip_cr_lf "$3")
	rad_ip="$4"
	rad_port="$5"
	rad_key="$6"
	rad_int="$7"

	card_count=$(grep -c mtlk /proc/modules 2>/dev/null || echo 1)
	case "$card_count" in
		''|*[!0-9]*|0) card_count=1 ;;
	esac
	bridging=$(get_param BridgeMode)
	[ -n "$bridging" ] || bridging=0

	mkdir -p "$WPA_CONFIG_DIR"
	{
		echo "logger_syslog_level=3"
		echo "interface=$IFACE"
		echo "driver=mtlk"
		echo "ssid=$ssid"
		echo "macaddr_acl=0"
		if [ "$bridging" -gt 0 ] 2>/dev/null; then
			echo "bridge=br0"
		elif [ "$card_count" -gt 1 ]; then
			echo "bridge=br1"
		fi

		case "$enc_mode" in
			OPEN)
				echo "# ------AP is in Plain Text mode------"
				;;
			WEP)
				case "$auth" in
					1|2|3) ;;
					*) auth=1 ;;
				esac
				echo "auth_algs=$auth"
				idx=0
				for kv in "$k1" "$k2" "$k3" "$k4"; do
					if [ -n "$kv" ]; then
						case "$kv" in
							0x*|0X*)
								echo "wep_key${idx}=${kv#??}"
								;;
							*)
								echo "wep_key${idx}=\"${kv}\""
								;;
						esac
					fi
					idx=$((idx + 1))
				done
				echo "wep_default_key=$used_key"
				echo "# ------AP is in WEP mode------"
				;;
			WEP_RADIUS)
				case "$rad_port" in
					''|*[!0-9]*) rad_port=1812 ;;
				esac
				rad_port2=$((rad_port + 1))
				echo "auth_server_addr=$rad_ip"
				echo "auth_server_port=$rad_port"
				echo "auth_server_shared_secret=$rad_key"
				echo "acct_server_addr=$rad_ip"
				echo "acct_server_port=$rad_port2"
				echo "acct_server_shared_secret=$rad_key"
				echo "# ------AP is in WEP RADIUS mode------"
				;;
			WPA_PERSONAL)
				group_rekey=$(get_param wpa_group_rekey)
				case "$group_rekey" in
					''|*[!0-9]*) group_rekey=3600 ;;
				esac
				echo "wpa=$wpa_mode"
				echo "wpa_key_mgmt=WPA-PSK"
				echo "wpa_group_rekey=$group_rekey"
				echo "wpa_gmk_rekey=86400"
				case "$encap_mode" in
					0) pairwise="TKIP"; encap_txt="TKIP" ;;
					1) pairwise="CCMP"; encap_txt="CCMP" ;;
					*) pairwise="TKIP CCMP"; encap_txt="TKIP + CCMP" ;;
				esac
				echo "wpa_pairwise=$pairwise"
				if [ "$(printf '%s' "$psk" | wc -c)" -eq 64 ] && ! printf '%s' "$psk" | grep -q '[^0-9a-fA-F]'; then
					echo "wpa_psk=$psk"
				else
					echo "wpa_passphrase=$psk"
				fi
				echo "# ------AP is in WPA/WPA2 PSK, $encap_txt mode------"
				;;
			WPA_ENTERPRISE)
				case "$rad_port" in
					''|*[!0-9]*) rad_port=1812 ;;
				esac
				case "$rad_int" in
					''|*[!0-9]*) rad_int=600 ;;
				esac
				rad_port2=$((rad_port + 1))
				echo "ieee8021x=1"
				echo "auth_server_addr=$rad_ip"
				echo "auth_server_port=$rad_port"
				echo "auth_server_shared_secret=$rad_key"
				echo "acct_server_addr=$rad_ip"
				echo "acct_server_port=$rad_port2"
				echo "acct_server_shared_secret=$rad_key"
				echo "wpa=$wpa_mode"
				echo "wpa_key_mgmt=WPA-EAP"
				echo "eap_reauth_period=$rad_int"
				case "$encap_mode" in
					0) pairwise="TKIP"; encap_txt="TKIP" ;;
					1) pairwise="CCMP"; encap_txt="CCMP" ;;
					*) pairwise="TKIP CCMP"; encap_txt="TKIP + CCMP" ;;
				esac
				echo "wpa_pairwise=$pairwise"
				echo "# ------AP is in WPA/WPA2 Enterprise, $encap_txt mode------"
				;;
		esac
	} > "$HOSTAPD_CONF_FILE"
}

write_supplicant_conf() {
	enc_mode="$1"
	ssid="$2"
	wep_auth="$3"
	k1="$4"
	k2="$5"
	k3="$6"
	k4="$7"
	used_key="$8"
	wpa_pw=$(strip_cr_lf "$9")
	shift 9
	ent_user="$1"
	ent_pass="$2"
	ent_cert="$3"
	wpa_mode="$4"
	encap_mode="$5"

	esc_ssid=$(wpa_escape "$ssid")
	esc_user=$(wpa_escape "$ent_user")
	esc_pass=$(wpa_escape "$ent_pass")

	mkdir -p "$WPA_CONFIG_DIR"
	if [ "$enc_mode" = "WPA_PERSONAL" ] && ! { [ "$(printf '%s' "$wpa_pw" | wc -c)" -eq 64 ] && ! printf '%s' "$wpa_pw" | grep -q '[^0-9a-fA-F]'; }; then
		psk_out=$(/root/mtlk/web/run_wpa_passphrase_for_update_security.sh "$ssid" "$wpa_pw" 2>/dev/null)
		derived_psk=$(printf '%s\n' "$psk_out" | awk -F '=' '/^[ \t]*psk=[0-9a-fA-F]+/ { gsub(/[ \t\r]/,"",$2); print $2; exit }')
		if [ -z "$derived_psk" ]; then
			echo "Failed to derive WPA PMK from passphrase"
			return 1
		fi
	fi

	{
		echo "ctrl_interface=/var/run/wpa_supplicant"
		echo "ctrl_interface_group=0"
		echo "network={"
		echo "	ssid=\"$esc_ssid\""
		case "$enc_mode" in
			OPEN)
				echo "	key_mgmt=NONE"
				echo "	auth_alg=OPEN"
				;;
			WEP)
				echo "	key_mgmt=NONE"
				idx=0
				for kv in "$k1" "$k2" "$k3" "$k4"; do
					if [ -n "$kv" ]; then
						case "$kv" in
							0x*|0X*)
								echo "	wep_key${idx}=${kv#??}"
								;;
							*)
								esc_wep=$(wpa_escape "$kv")
								echo "	wep_key${idx}=\"${esc_wep}\""
								;;
						esac
					fi
					idx=$((idx + 1))
				done
				echo "	wep_tx_keyidx=$used_key"
				echo "	group=WEP40 WEP104"
				case "$wep_auth" in
					1) echo "	auth_alg=OPEN" ;;
					2) echo "	auth_alg=SHARED" ;;
					3) echo "	auth_alg=OPEN SHARED" ;;
				esac
				;;
			WPA_ENTERPRISE)
				echo "	key_mgmt=WPA-EAP"
				echo "	eap=TTLS PEAP TLS"
				echo "	phase2=\"auth=MSCHAPV2 auth=GTC\""
				if [ "$ent_cert" = "1" ]; then
					echo "	client_cert=\"$CERT_FILE\""
				fi
				echo "	identity=\"$esc_user\""
				echo "	password=\"$esc_pass\""
				echo "	auth_alg=OPEN"
				;;
			*)
				if [ "$(printf '%s' "$wpa_pw" | wc -c)" -eq 64 ] && ! printf '%s' "$wpa_pw" | grep -q '[^0-9a-fA-F]'; then
					echo "	#psk=\"$wpa_pw\""
					echo "	psk=$wpa_pw"
				else
					echo "	psk=$derived_psk"
				fi
				if [ "$enc_mode" = "WPA_PERSONAL" ]; then
					if ! { [ "$wpa_mode" = "3" ] && [ "$encap_mode" = "2" ]; }; then
						case "$wpa_mode" in
							1) echo "	proto=WPA" ;;
							2) echo "	proto=RSN" ;;
						esac
						case "$encap_mode" in
							0) echo "	pairwise=TKIP"; echo "	group=TKIP" ;;
							1) echo "	pairwise=CCMP"; echo "	group=CCMP TKIP" ;;
							2) echo "	pairwise=CCMP TKIP"; echo "	group=CCMP TKIP" ;;
						esac
					fi
				fi
				;;
		esac
		echo "	scan_ssid=1"
		echo "}"
	} > "$WPA_CONF_FILE"
}

save_security() {
	sec_mode=$(get_param NonProcSecurityMode)
	dev_type=$(get_param network_type)
	essid=$(get_param NonProc_ESSID)
	force_disconn=$(get_param forceWlanDisconnect)

	if [ "$force_disconn" = "1" ]; then
		echo "forceWlanDisconnect=1, so clear configuration of wpa_supplicant" > /dev/console 2>/dev/null || true
		: > "$WPA_CONF_FILE"
		return 0
	fi

	case "$sec_mode" in
		1)
			[ "$dev_type" = "2" ] && set_proc_var 0 Authentication
			set_proc_var 0 WepEncryption
			if [ "$dev_type" = "0" ] || [ "$dev_type" = "1" ]; then
				write_supplicant_conf "OPEN" "$essid" "" "" "" "" "" "" "Open_System" "" "" "" "" ""
			elif [ "$dev_type" = "2" ]; then
				write_hostapd_conf "OPEN" "$essid" "" "" "" "" "" "" "" "" "" "" "" "" "" ""
			fi
			;;
		2)
			wep_len=$(get_param NonProc_WepKeyLength)
			used_key=$(get_param WepTxKeyIdx)
			if [ "$dev_type" = "2" ]; then
				wep_auth=$(get_param Authentication)
			else
				wep_auth=$(get_param NonProc_Authentication)
			fi
			k1=$(get_param WepKeys_DefaultKey0)
			k2=$(get_param WepKeys_DefaultKey1)
			k3=$(get_param WepKeys_DefaultKey2)
			k4=$(get_param WepKeys_DefaultKey3)

			[ -n "$k1" ] && set_proc_var "$k1" WepKeys_DefaultKey0
			[ -n "$k2" ] && set_proc_var "$k2" WepKeys_DefaultKey1
			[ -n "$k3" ] && set_proc_var "$k3" WepKeys_DefaultKey2
			[ -n "$k4" ] && set_proc_var "$k4" WepKeys_DefaultKey3
			case "$used_key" in
				0|1|2|3) set_proc_var "$used_key" WepTxKeyIdx ;;
			esac
			set_proc_var 1 WepEncryption
			[ "$dev_type" = "2" ] && set_proc_var "$wep_auth" Authentication

			if [ "$dev_type" = "0" ] || [ "$dev_type" = "1" ]; then
				write_supplicant_conf "WEP" "$essid" "$wep_auth" "$k1" "$k2" "$k3" "$k4" "$used_key" "Wep_System" "" "" "" "" ""
			elif [ "$dev_type" = "2" ]; then
				write_hostapd_conf "WEP" "$essid" "$wep_len" "$wep_auth" "$used_key" "$k1" "$k2" "$k3" "$k4" "" "" "" "" "" "" ""
			fi
			;;
		3)
			wpa_mode=$(get_param NonProc_WPA_Personal_Mode)
			encap=$(get_param NonProc_WPA_Personal_Encapsulation)
			psk=$(get_param NonProc_WPA_Personal_PSK)
			set_proc_var 0 WepEncryption
			[ "$dev_type" = "2" ] && set_proc_var 0 Authentication
			if [ "$dev_type" = "0" ] || [ "$dev_type" = "1" ]; then
				write_supplicant_conf "WPA_PERSONAL" "$essid" "" "" "" "" "" "" "$psk" "" "" "" "$wpa_mode" "$encap"
			elif [ "$dev_type" = "2" ]; then
				write_hostapd_conf "WPA_PERSONAL" "$essid" "" "" "" "" "" "" "" "$wpa_mode" "$encap" "$psk" "" "" "" ""
			fi
			;;
		4)
			ent_mode=$(get_param NonProc_WPA_Enterprise_Mode)
			ent_encap=$(get_param NonProc_WPA_Enterprise_Encapsulation)
			rad_ip=$(get_param NonProc_WPA_Enterprise_Radius_IP)
			rad_port=$(get_param NonProc_WPA_Enterprise_Radius_Port)
			rad_key=$(get_param NonProc_WPA_Enterprise_Radius_Key)
			rad_int=$(get_param NonProc_WPA_Enterprise_Radius_ReKey_Interval)
			ent_user=$(get_param NonProc_WPA_Enterprise_Radius_Username)
			ent_pass=$(get_param NonProc_WPA_Enterprise_Radius_Password)
			ent_cert=$(get_param NonProc_WPA_Enterprise_Certificate)
			set_proc_var 0 WepEncryption
			[ "$dev_type" = "2" ] && set_proc_var 0 Authentication
			if [ "$dev_type" = "0" ]; then
				write_supplicant_conf "WPA_ENTERPRISE" "$essid" "" "" "" "" "" "" "Enterprise" "$ent_user" "$ent_pass" "$ent_cert" "" ""
			elif [ "$dev_type" = "2" ]; then
				write_hostapd_conf "WPA_ENTERPRISE" "$essid" "" "" "" "" "" "" "" "$ent_mode" "$ent_encap" "" "$rad_ip" "$rad_port" "$rad_key" "$rad_int"
			fi
			;;
		*)
			wr_len=$(get_param NonProc_WepRadiusKeyLength)
			wr_used=$(get_param WepRadiusTxKeyIdx)
			wr_auth=$(get_param WepRadiusAuthentication)
			wr_k1=$(get_param WepRadiusKeys_DefaultKey0)
			wr_k2=$(get_param WepRadiusKeys_DefaultKey1)
			wr_k3=$(get_param WepRadiusKeys_DefaultKey2)
			wr_k4=$(get_param WepRadiusKeys_DefaultKey3)
			wr_key=$(get_param NonProc_WEP_Radius_Key)
			wr_port=$(get_param NonProc_WEP_Radius_Port)
			wr_ip=$(get_param NonProc_WEP_Radius_IP)
			set_proc_var 1 WepEncryption
			[ "$dev_type" = "2" ] && set_proc_var "$wr_auth" Authentication
			if [ "$dev_type" = "2" ]; then
				write_hostapd_conf "WEP_RADIUS" "$essid" "$wr_len" "$wr_auth" "$wr_used" "$wr_k1" "$wr_k2" "$wr_k3" "$wr_k4" "" "" "" "$wr_ip" "$wr_port" "$wr_key" ""
			fi
			;;
	esac

	if [ "$NO_SAVE" = "0" ] && [ "$dev_type" = "2" ] && [ -n "$essid" ]; then
		clean_ssid=$(strip_cr_lf "$essid")
		freq_band=$(get_param FrequencyBand)
		case "$freq_band" in
			0|1|2) set_proc_var "$freq_band" FrequencyBand ;;
		esac
		if [ -n "$clean_ssid" ]; then
			iwconfig "$IFACE" essid -- "$clean_ssid" >/dev/null 2>&1 || true
		fi
		killall hostapd >/dev/null 2>&1 || true
		sleep 2
		if [ "${sec_mode:-0}" -gt 1 ] 2>/dev/null && [ -x "$HOSTAPD" ] && [ -f "$HOSTAPD_CONF_FILE" ]; then
			"$HOSTAPD" -d "$HOSTAPD_CONF_FILE" >/dev/null 2>&1 &
		fi
	fi
}

reactivate_sta() {
	pids=$(ps | awk '/wpa_supplicant/ && !/awk/ { print $1 }')
	if [ -z "$pids" ]; then
		bridging=$(get_param BridgeMode)
		[ -n "$bridging" ] || bridging=0
		if [ "$bridging" -gt 0 ] 2>/dev/null; then
			if [ "$bridging" = "3" ] && [ -f /tmp/mac_cloning.addr ]; then
				mac_addr=$(tr 'a-f' 'A-F' < /tmp/mac_cloning.addr | tr -d '\r\n' | awk '{print $1}')
				"$WPA_SUPPLICANT" -Dwext -bbr0 -i"$IFACE" -c "$WPA_CONF_FILE" -p "maclone=$mac_addr" >/dev/null 2>&1 &
			else
				"$WPA_SUPPLICANT" -Dwext -bbr0 -i"$IFACE" -c "$WPA_CONF_FILE" >/dev/null 2>&1 &
			fi
		else
			"$WPA_SUPPLICANT" -Dwext -i"$IFACE" -c "$WPA_CONF_FILE" >/dev/null 2>&1 &
		fi
	else
		first=""
		for p in $pids; do
			if [ -z "$first" ]; then
				first="$p"
			else
				kill -9 "$p" >/dev/null 2>&1 || true
			fi
		done
		kill -HUP "$first" >/dev/null 2>&1 || true
		sleep 4
	fi
}

save_security

unconf=$(get_param unconfigured)
if [ "$unconf" = "1" ]; then
	echo "unconfigured=1, so clear configuration of wpa_supplicant" > /dev/console 2>/dev/null || true
	: > "$WPA_CONF_FILE"
fi
if [ -f "$WPA_CONF_FILE" ]; then
	cat "$WPA_CONF_FILE"
fi

if [ "$SET_SECURITY_ONLY" = "0" ]; then
	cb_proc="${PROC_ROOT_BASE}${WLAN_IDX}/ChannelBonding"
	cfg_cb=$(get_param ChannelBonding | tr -d ' \t\r\n')
	if { [ "$cfg_cb" = "0" ] || [ "$cfg_cb" = "1" ]; } && [ -f "$cb_proc" ]; then
		cur_cb=$(tr -d ' \t\r\n' < "$cb_proc" 2>/dev/null)
		if [ -n "$cur_cb" ] && [ "$cur_cb" != "$cfg_cb" ]; then
			ifconfig "$IFACE" down >/dev/null 2>&1 || true
			printf '%s\n' "$cfg_cb" > "$cb_proc" 2>/dev/null || true
			ifconfig "$IFACE" up >/dev/null 2>&1 || true
		fi
	fi

	if [ -x /bin/config_umount.sh ] && [ -x /bin/config_mount.sh ]; then
		/bin/config_umount.sh
		/bin/config_mount.sh
	fi

	if [ "$REACTIVATE" = "1" ]; then
		dev_type=$(get_param network_type)
		echo "Connecting"
		if [ "$dev_type" = "2" ]; then
			ap_ssid=$(get_param NonProc_ESSID)
			sec_mode=$(get_param NonProcSecurityMode)
			if [ -n "$ap_ssid" ]; then
				iwconfig "$IFACE" essid -- "$(strip_cr_lf "$ap_ssid")" >/dev/null 2>&1 || true
			fi
			killall hostapd >/dev/null 2>&1 || true
			sleep 2
			if [ "${sec_mode:-0}" -gt 1 ] 2>/dev/null && [ -x "$HOSTAPD" ] && [ -f "$HOSTAPD_CONF_FILE" ]; then
				"$HOSTAPD" -d "$HOSTAPD_CONF_FILE" >/dev/null 2>&1 &
			fi
		else
			reactivate_sta
		fi
	fi
fi
