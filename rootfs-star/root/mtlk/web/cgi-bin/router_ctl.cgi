#!/bin/sh
# Unified Hardened Router Control CGI for Cisco Linksys WAP610N (Phase 1)

JFFS="/mnt/jffs2"
set -f

# Retrieve admin password from system configuration (/mnt/jffs2 first)
ADMIN_PASS=""
for conf in "$JFFS/sys.conf" /tmp/sys.conf /etc/sys.conf /root/mtlk/etc/sys.conf; do
    if [ -f "$conf" ] && [ -z "$ADMIN_PASS" ]; then
        ADMIN_PASS=$(awk '/^\^?(Admin)?Password[ \t]*=/ {idx = index($0, "="); val = substr($0, idx + 1); sub(/^[ \t]+/, "", val); sub(/[ \t\r]+$/, "", val); print val}' "$conf" 2>/dev/null | tail -n 1)
    fi
done

# Authenticate request fail-closed
if [ -n "$HTTP_AUTHORIZATION" ]; then
    AUTH_IN=$(printf '%s' "$HTTP_AUTHORIZATION" | sed 's/^[Bb]asic //')
    EXPECTED_AUTH=$(printf '%s' "admin:$ADMIN_PASS" | uuencode -m - 2>/dev/null | sed '1d;$d' | tr -d '\r\n')
    if [ -z "$ADMIN_PASS" ] || [ -z "$EXPECTED_AUTH" ] || [ "$AUTH_IN" != "$EXPECTED_AUTH" ]; then
        echo "Content-Type: application/json"
        echo ""
        echo '{"status":"error","message":"Unauthorized"}'
        exit 0
    fi
elif [ -z "$REMOTE_USER" ] && { [ -z "$SERVER_SOFTWARE" ] || [ -z "$GATEWAY_INTERFACE" ]; }; then
    echo "Content-Type: application/json"
    echo ""
    echo '{"status":"error","message":"Unauthorized"}'
    exit 0
fi

# CSRF / Same-Origin check on state-changing POST requests
if [ "$REQUEST_METHOD" = "POST" ] && [ -n "$HTTP_HOST" ]; then
    SRC_ORIGIN="${HTTP_ORIGIN:-$HTTP_REFERER}"
    if [ -n "$SRC_ORIGIN" ]; then
        SRC_HOST=$(printf '%s' "$SRC_ORIGIN" | sed -e 's#^https\?://##' -e 's#/.*$##')
        if [ -n "$SRC_HOST" ] && [ "$SRC_HOST" != "$HTTP_HOST" ]; then
            echo "Status: 403 Forbidden"
            echo "Content-Type: application/json"
            echo ""
            echo '{"status":"error","message":"Forbidden: Cross-origin request rejected"}'
            exit 0
        fi
    fi
fi

json_header() {
    echo "Content-Type: application/json"
    echo "Pragma: no-cache"
    echo "Cache-Control: no-store"
    echo ""
}

json_error() {
    json_header
    printf '{"status":"error","message":"%s"}\n' "$(printf '%s' "$1" | tr -d '\"\\\r\n')"
    exit 0
}

json_esc() {
    awk '{
        gsub(/\\/, "\\\\")
        gsub(/"/, "\\\"")
        gsub(/\r/, "")
        gsub(/\t/, "\\t")
        if (NR > 1) printf "\\n"
        printf "%s", $0
    }'
}

url_decode() {
    printf '%s' "$1" | awk '
    BEGIN {
        for (i = 0; i < 256; i++) hex[sprintf("%02x", i)] = sprintf("%c", i)
        for (i = 0; i < 256; i++) hex[sprintf("%02X", i)] = sprintf("%c", i)
    }
    {
        gsub(/\+/, " ", $0)
        out = ""
        n = length($0)
        for (i = 1; i <= n; i++) {
            c = substr($0, i, 1)
            if (c == "%" && i + 2 <= n) {
                code = substr($0, i + 1, 2)
                if (code in hex) {
                    out = out hex[code]
                    i += 2
                    continue
                }
            }
            out = out c
        }
        printf "%s", out
    }'
}

is_ipv4() {
    case "$1" in ''|*[!0-9.]*) return 1 ;; esac
    awk -v ip="$1" 'BEGIN {
        n = split(ip, o, ".")
        if (n != 4) exit 1
        for (i = 1; i <= 4; i++) {
            if (o[i] == "" || o[i] < 0 || o[i] > 255) exit 1
        }
        if (o[1] == 0 || o[1] == 127 || o[1] >= 224) exit 1
        exit 0
    }'
}

is_netmask() {
    case "$1" in ''|*[!0-9.]*) return 1 ;; esac
    awk -v m="$1" 'BEGIN {
        n = split(m, o, ".")
        if (n != 4) exit 1
        for (i = 1; i <= 4; i++) {
            if (o[i] == "" || o[i] < 0 || o[i] > 255) exit 1
        }
        if (o[1] != 255) exit 1
        ok = 1; z = 0
        for (i = 1; i <= 4; i++) {
            v = o[i]
            for (b = 7; b >= 0; b--) {
                bit = int(v / (2 ^ b)) % 2
                if (z && bit == 1) { ok = 0; break }
                if (bit == 0) z = 1
            }
        }
        if (!ok || (o[4] == 255)) exit 1
        exit 0
    }'
}

is_mac() {
    case "$1" in
        [0-9A-Fa-f][0-9A-Fa-f]:[0-9A-Fa-f][0-9A-Fa-f]:[0-9A-Fa-f][0-9A-Fa-f]:[0-9A-Fa-f][0-9A-Fa-f]:[0-9A-Fa-f][0-9A-Fa-f]:[0-9A-Fa-f][0-9A-Fa-f])
            return 0 ;;
        *) return 1 ;;
    esac
}

is_uint_range() {
    case "$1" in ''|*[!0-9]*) return 1 ;; esac
    [ "$1" -ge "$2" ] 2>/dev/null && [ "$1" -le "$3" ] 2>/dev/null
}

is_host() {
    case "$1" in
        ''|*[!A-Za-z0-9._-]*) return 1 ;;
    esac
    [ ${#1} -le 64 ]
}

is_tz() {
    case "$1" in
        ''|*[!A-Za-z0-9+:,-]*) return 1 ;;
    esac
    [ ${#1} -le 32 ]
}

is_pidfile_alive() {
    pf="$1"
    [ -f "$pf" ] || return 1
    pid=$(cat "$pf" 2>/dev/null)
    case "$pid" in ''|*[!0-9]*) return 1 ;; esac
    kill -0 "$pid" 2>/dev/null
}

get_conf() {
    file_name="$1"
    key="$2"
    def="$3"
    for f in "$JFFS/$file_name" "/tmp/$file_name"; do
        if [ -f "$f" ]; then
            val=$(awk -v k="$key" '
                $0 ~ "^[ \t]*" k "[ \t]*=" {
                    idx = index($0, "=")
                    v = substr($0, idx + 1)
                    sub(/^[ \t]+/, "", v)
                    sub(/[ \t\r]+$/, "", v)
                    print v
                }
            ' "$f" 2>/dev/null | tail -n 1)
            if [ -n "$val" ]; then
                printf '%s' "$val"
                return 0
            fi
        fi
    done
    printf '%s' "$def"
}

get_kv_file() {
    f="$1"
    k="$2"
    def="$3"
    if [ -f "$f" ]; then
        val=$(awk -F '=' -v key="$k" '$1 == key { sub(/^[^=]*=/, ""); gsub(/\r/, ""); print; }' "$f" 2>/dev/null | tail -n 1)
        if [ -n "$val" ]; then
            printf '%s' "$val"
            return 0
        fi
    fi
    printf '%s' "$def"
}

ensure_jffs_conf() {
    name="$1"
    mkdir -p "$JFFS" 2>/dev/null || true
    if [ ! -s "$JFFS/$name" ]; then
        if [ -s "/tmp/$name" ]; then
            cp -af "/tmp/$name" "$JFFS/$name" 2>/dev/null || true
        elif [ -s "/root/mtlk/etc/$name" ]; then
            cp -af "/root/mtlk/etc/$name" "$JFFS/$name" 2>/dev/null || true
        else
            : > "$JFFS/$name" 2>/dev/null || true
        fi
    fi
}

set_conf_kv() {
    target="$1"
    key="$2"
    val="$3"
    tmp_f="${target}.tmp.$$"
    if [ -f "$target" ]; then
        awk -v k="$key" '$0 !~ "^[ \t]*" k "[ \t]*=" { print }' "$target" > "$tmp_f" 2>/dev/null || : > "$tmp_f"
    else
        : > "$tmp_f"
    fi
    printf '%s = %s\n' "$key" "$val" >> "$tmp_f" 2>/dev/null || true
    chmod 600 "$tmp_f" 2>/dev/null || true
    mv -f "$tmp_f" "$target" 2>/dev/null || true
}

atomic_write() {
    target="$1"
    mode="$2"
    tmp_f="${target}.tmp.$$"
    cat > "$tmp_f" 2>/dev/null || return 1
    chmod "$mode" "$tmp_f" 2>/dev/null || true
    mv -f "$tmp_f" "$target" 2>/dev/null || return 1
    return 0
}

sync_sys_and_wlan() {
    [ -s "$JFFS/sys.conf" ] && cp -af "$JFFS/sys.conf" /tmp/sys.conf 2>/dev/null || true
    [ -s "$JFFS/wlan0.conf" ] && cp -af "$JFFS/wlan0.conf" /tmp/wlan0.conf 2>/dev/null || true
    killall -HUP webs 2>/dev/null || true
}

trigger_save() {
    if [ -x /bin/config_save.sh ]; then
        /bin/config_save.sh </dev/null >/dev/null 2>&1 &
    fi
}

set_proc_wlan() {
    p="/proc/sys/dev/mtlk/wlan0/$1"
    if [ -w "$p" ]; then
        printf '%s\n' "$2" > "$p" 2>/dev/null || true
    fi
}

lines_to_json_array() {
    f="$1"
    if [ ! -s "$f" ]; then
        printf '[]'
        return 0
    fi
    awk '
    BEGIN { printf "["; first = 1 }
    /^[ \t]*($|#)/ { next }
    {
        gsub(/\r/, "")
        gsub(/\\/, "\\\\")
        gsub(/"/, "\\\"")
        if (!first) printf ","
        first = 0
        printf "\"%s\"", $0
    }
    END { printf "]" }' "$f" 2>/dev/null || printf '[]'
}

active_leases_json() {
    if [ -s /tmp/udhcpd.leases ] && command -v dumpleases >/dev/null 2>&1; then
        dumpleases -f /tmp/udhcpd.leases 2>/dev/null | awk '
        BEGIN { printf "["; first = 1 }
        NR > 1 && NF >= 3 && $1 ~ /^([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2}$/ {
            mac = $1; ip = $2
            exp_str = ""
            for (i = 3; i <= NF; i++) exp_str = exp_str (i == 3 ? "" : " ") $i
            gsub(/\\/, "\\\\", exp_str); gsub(/"/, "\\\"", exp_str)
            if (!first) printf ","
            first = 0
            printf "{\"mac\":\"%s\",\"ip\":\"%s\",\"expires\":\"%s\"}", mac, ip, exp_str
        }
        END { printf "]" }' 2>/dev/null || printf '[]'
    else
        printf '[]'
    fi
}

emit_state_json() {
    net_type=$(get_conf wlan0.conf network_type 2)
    br_mode=$(get_conf sys.conf BridgeMode 4)
    loop_br=$(get_conf sys.conf LoopBreakingEnabled 0)
    mac_clone=$(get_conf sys.conf MacCloningAddr 0)

    dhcp_en=$(get_conf sys.conf DHCPDEnabled 1)
    dhcp_st=$(get_conf sys.conf DHCPDStartAddress 192.168.1.10)
    dhcp_end=$(get_conf sys.conf DHCPDEndAddress 192.168.1.20)
    dhcp_sub=$(get_conf sys.conf DHCPDSubnet 255.255.255.0)
    dhcp_lt=$(get_conf sys.conf DHCPDLeaseTime 864000)
    dhcp_dom=$(get_conf sys.conf DHCPDDomain Local)
    dhcp_d1=$(get_conf sys.conf DHCPDDNS1 1.1.1.1)
    dhcp_d2=$(get_conf sys.conf DHCPDDNS2 8.8.8.8)
    dhcp_run=0
    pidof udhcpd >/dev/null 2>&1 && dhcp_run=1

    acl_mode=$(get_conf wlan0.conf AclMode 0)
    acl_raw=$(get_conf wlan0.conf ACL "")

    rf_maxc=$(get_conf wlan0.conf MaxConnections 16)
    rf_bcn=$(get_conf wlan0.conf BeaconPeriod 100)
    rf_dtim=$(get_conf wlan0.conf DTIMPeriod 5)
    rf_rts=$(get_conf wlan0.conf RTSThreshold 1600)
    rf_sp=$(get_conf wlan0.conf ShortPreamble 1)
    rf_ss=$(get_conf wlan0.conf ShortSlotTimeOptionEnabled 1)
    rf_stbc=$(get_conf wlan0.conf STBC 0)
    rf_sgi=$(get_conf wlan0.conf ShortCyclicPrefix 0)
    rf_ska=$(get_conf wlan0.conf StaKeepaliveTimeout 60000)

    qos_en=$(get_kv_file "$JFFS/qos.conf" ENABLED 0)
    qos_dn=$(get_kv_file "$JFFS/qos.conf" DOWN_KBPS 0)
    qos_up=$(get_kv_file "$JFFS/qos.conf" UP_KBPS 0)

    dd_en=$(get_kv_file "$JFFS/ddns.conf" ENABLED 0)
    dd_pv=$(get_kv_file "$JFFS/ddns.conf" PROVIDER duckdns)
    dd_hn=$(get_kv_file "$JFFS/ddns.conf" HOSTNAME "")
    dd_un=$(get_kv_file "$JFFS/ddns.conf" USERNAME "")
    dd_pw=$(get_kv_file "$JFFS/ddns.conf" PASSWORD "")
    dd_tk=$(get_kv_file "$JFFS/ddns.conf" TOKEN "")
    dd_cu=$(get_kv_file "$JFFS/ddns.conf" CUSTOM_URL "")
    dd_iv=$(get_kv_file "$JFFS/ddns.conf" INTERVAL 600)
    dd_has_pw=0; [ -n "$dd_pw" ] && dd_has_pw=1
    dd_has_tk=0; [ -n "$dd_tk" ] && dd_has_tk=1
    dd_run=0; is_pidfile_alive /tmp/ddns_updater.pid && dd_run=1
    dd_st=$(cat /tmp/ddns.status 2>/dev/null || echo "")

    wd_en=$(get_kv_file "$JFFS/watchdog.conf" ENABLED 0)
    wd_tg=$(get_kv_file "$JFFS/watchdog.conf" TARGET 1.1.1.1)
    wd_iv=$(get_kv_file "$JFFS/watchdog.conf" INTERVAL 60)
    wd_mf=$(get_kv_file "$JFFS/watchdog.conf" MAX_FAIL 5)
    wd_ac=$(get_kv_file "$JFFS/watchdog.conf" ACTION recover)
    wd_run=0; is_pidfile_alive /tmp/net_watchdog.pid && wd_run=1
    wd_log=$(cat /tmp/watchdog.log 2>/dev/null || echo "")

    tz_val=$(head -n 1 "$JFFS/tz.conf" 2>/dev/null | tr -d ' \t\r\n')
    [ -z "$tz_val" ] && tz_val=$(head -n 1 /etc/TZ 2>/dev/null | tr -d ' \t\r\n')
    now_str=$(date '+%Y-%m-%d %H:%M:%S %Z' 2>/dev/null || date 2>/dev/null || echo "")
    ts_log=$(cat /tmp/timesync.log 2>/dev/null || echo "")
    led_night=$(get_kv_file "$JFFS/led.conf" NIGHT_MODE 0)
    has_vlan=0; [ -f /lib/modules/2.6.16-star/net/8021q.ko ] && has_vlan=1
    ssh_keys=$(cat "$JFFS/dropbear/authorized_keys" 2>/dev/null || cat /root/.ssh/authorized_keys 2>/dev/null || echo "")
    rc_local=$(cat "$JFFS/rc.local" 2>/dev/null || echo "")

    j_static=$(lines_to_json_array "$JFFS/dhcp_static.conf")
    j_active=$(active_leases_json)
    j_routes=$(lines_to_json_array "$JFFS/routes.conf")
    j_vlans=$(lines_to_json_array "$JFFS/vlan.conf")

    json_header
    printf '{"status":"success",'
    printf '"opmode":{"network_type":"%s","bridge_mode":"%s","loop_breaking":"%s","mac_cloning_addr":"%s"},' \
        "$(printf '%s' "$net_type" | json_esc)" "$(printf '%s' "$br_mode" | json_esc)" \
        "$(printf '%s' "$loop_br" | json_esc)" "$(printf '%s' "$mac_clone" | json_esc)"
    printf '"dhcp":{"enabled":"%s","start":"%s","end":"%s","subnet":"%s","lease_time":"%s","domain":"%s","dns1":"%s","dns2":"%s","running":%s,"static_leases":%s,"active_leases":%s},' \
        "$(printf '%s' "$dhcp_en" | json_esc)" "$(printf '%s' "$dhcp_st" | json_esc)" \
        "$(printf '%s' "$dhcp_end" | json_esc)" "$(printf '%s' "$dhcp_sub" | json_esc)" \
        "$(printf '%s' "$dhcp_lt" | json_esc)" "$(printf '%s' "$dhcp_dom" | json_esc)" \
        "$(printf '%s' "$dhcp_d1" | json_esc)" "$(printf '%s' "$dhcp_d2" | json_esc)" \
        "$dhcp_run" "$j_static" "$j_active"
    printf '"acl":{"mode":"%s","raw":"%s"},' \
        "$(printf '%s' "$acl_mode" | json_esc)" "$(printf '%s' "$acl_raw" | json_esc)"
    printf '"rf":{"max_conn":"%s","beacon":"%s","dtim":"%s","rts":"%s","short_preamble":"%s","short_slot":"%s","stbc":"%s","short_gi":"%s","sta_ka_to":"%s"},' \
        "$(printf '%s' "$rf_maxc" | json_esc)" "$(printf '%s' "$rf_bcn" | json_esc)" \
        "$(printf '%s' "$rf_dtim" | json_esc)" "$(printf '%s' "$rf_rts" | json_esc)" \
        "$(printf '%s' "$rf_sp" | json_esc)" "$(printf '%s' "$rf_ss" | json_esc)" \
        "$(printf '%s' "$rf_stbc" | json_esc)" "$(printf '%s' "$rf_sgi" | json_esc)" \
        "$(printf '%s' "$rf_ska" | json_esc)"
    printf '"routes":%s,"vlans":%s,' "$j_routes" "$j_vlans"
    printf '"qos":{"enabled":"%s","down_kbps":"%s","up_kbps":"%s"},' \
        "$(printf '%s' "$qos_en" | json_esc)" "$(printf '%s' "$qos_dn" | json_esc)" "$(printf '%s' "$qos_up" | json_esc)"
    printf '"ddns":{"enabled":"%s","provider":"%s","hostname":"%s","username":"%s","has_pass":%s,"has_token":%s,"custom_url":"%s","interval":"%s","running":%s,"status":"%s"},' \
        "$(printf '%s' "$dd_en" | json_esc)" "$(printf '%s' "$dd_pv" | json_esc)" \
        "$(printf '%s' "$dd_hn" | json_esc)" "$(printf '%s' "$dd_un" | json_esc)" \
        "$dd_has_pw" "$dd_has_tk" "$(printf '%s' "$dd_cu" | json_esc)" \
        "$(printf '%s' "$dd_iv" | json_esc)" "$dd_run" "$(printf '%s' "$dd_st" | json_esc)"
    printf '"watchdog":{"enabled":"%s","target":"%s","interval":"%s","max_fail":"%s","action":"%s","running":%s,"log":"%s"},' \
        "$(printf '%s' "$wd_en" | json_esc)" "$(printf '%s' "$wd_tg" | json_esc)" \
        "$(printf '%s' "$wd_iv" | json_esc)" "$(printf '%s' "$wd_mf" | json_esc)" \
        "$(printf '%s' "$wd_ac" | json_esc)" "$wd_run" "$(printf '%s' "$wd_log" | json_esc)"
    printf '"system":{"tz":"%s","now":"%s","timesync_log":"%s","led_night":"%s","has_vlan_mod":%s,"ssh_keys":"%s","rc_local":"%s"}}\n' \
        "$(printf '%s' "$tz_val" | json_esc)" "$(printf '%s' "$now_str" | json_esc)" \
        "$(printf '%s' "$ts_log" | json_esc)" "$(printf '%s' "$led_night" | json_esc)" \
        "$has_vlan" "$(printf '%s' "$ssh_keys" | json_esc)" "$(printf '%s' "$rc_local" | json_esc)"
}

QUERY="${QUERY_STRING}"
if [ "$REQUEST_METHOD" = "POST" ] && [ -n "$CONTENT_LENGTH" ]; then
    case "$CONTENT_LENGTH" in *[!0-9]*) CONTENT_LENGTH=0 ;; esac
    if [ "$CONTENT_LENGTH" -gt 8192 ] 2>/dev/null; then
        echo "Status: 413 Payload Too Large"
        json_header
        echo '{"status":"error","message":"Payload too large"}'
        exit 0
    fi
    if [ "$CONTENT_LENGTH" -gt 0 ] 2>/dev/null; then
        POST_DATA=$(dd bs=1 count="$CONTENT_LENGTH" 2>/dev/null)
        [ -z "$QUERY" ] && QUERY="$POST_DATA" || QUERY="${QUERY}&${POST_DATA}"
    fi
fi

# Strictly read-only GET handler
if [ "$REQUEST_METHOD" = "GET" ]; then
    case "$QUERY" in
        ""|action=state*|state)
            emit_state_json
            exit 0
            ;;
        *)
            echo "Status: 405 Method Not Allowed"
            json_header
            echo '{"status":"error","message":"Method Not Allowed"}'
            exit 0
            ;;
    esac
elif [ -n "$REQUEST_METHOD" ] && [ "$REQUEST_METHOD" != "POST" ]; then
    echo "Status: 405 Method Not Allowed"
    json_header
    echo '{"status":"error","message":"Method Not Allowed"}'
    exit 0
fi

# Parse POST parameters safely
P_ACTION="" P_CMD="" P_NET_TYPE="" P_BR_MODE="" P_MAC_CLONE="" P_LOOP="" P_REBOOT=""
P_ENABLED="" P_START="" P_END="" P_SUBNET="" P_LEASE="" P_DOMAIN="" P_DNS1="" P_DNS2=""
P_STATIC_LEASES="" P_MODE="" P_MACS="" P_MAX_CONN="" P_BEACON="" P_DTIM="" P_RTS=""
P_SHORT_PRE="" P_SHORT_SLOT="" P_STBC="" P_SHORT_GI="" P_STA_KA=""
P_ROUTES="" P_VLANS="" P_DOWN="" P_UP="" P_PROVIDER="" P_HOSTNAME="" P_USERNAME=""
P_PASSWORD="" P_TOKEN="" P_CUSTOM_URL="" P_INTERVAL="" P_TEST_NOW=""
P_TARGET="" P_MAX_FAIL="" P_ACT_MODE="" P_TZ="" P_NIGHT="" P_SSH_KEYS="" P_RC_LOCAL=""
P_BAND="" P_ESSID="" P_SEC="" P_PSK="" P_IPCFG="" P_IP="" P_GW=""

OLD_IFS="$IFS"
IFS='&'
for kv in $QUERY; do
    k="${kv%%=*}"
    v="${kv#*=}"
    [ "$k" = "$kv" ] && v=""
    dv=$(url_decode "$v" | tr -d '\r\000')
    sl=$(printf '%s' "$dv" | tr -d '\n')
    case "$k" in
        action) P_ACTION="$sl" ;;
        cmd) P_CMD="$sl" ;;
        network_type) P_NET_TYPE="$sl" ;;
        bridge_mode) P_BR_MODE="$sl" ;;
        mac_cloning_addr) P_MAC_CLONE="$sl" ;;
        loop_breaking) P_LOOP="$sl" ;;
        reboot) P_REBOOT="$sl" ;;
        enabled) P_ENABLED="$sl" ;;
        start) P_START="$sl" ;;
        end) P_END="$sl" ;;
        subnet) P_SUBNET="$sl" ;;
        lease_time) P_LEASE="$sl" ;;
        domain) P_DOMAIN="$sl" ;;
        dns1) P_DNS1="$sl" ;;
        dns2) P_DNS2="$sl" ;;
        static_leases) P_STATIC_LEASES="$dv" ;;
        mode) P_MODE="$sl" ;;
        macs) P_MACS="$dv" ;;
        max_conn) P_MAX_CONN="$sl" ;;
        beacon) P_BEACON="$sl" ;;
        dtim) P_DTIM="$sl" ;;
        rts) P_RTS="$sl" ;;
        short_preamble) P_SHORT_PRE="$sl" ;;
        short_slot) P_SHORT_SLOT="$sl" ;;
        stbc) P_STBC="$sl" ;;
        short_gi) P_SHORT_GI="$sl" ;;
        sta_ka_to) P_STA_KA="$sl" ;;
        routes) P_ROUTES="$dv" ;;
        vlans) P_VLANS="$dv" ;;
        down_kbps) P_DOWN="$sl" ;;
        up_kbps) P_UP="$sl" ;;
        provider) P_PROVIDER="$sl" ;;
        hostname) P_HOSTNAME="$sl" ;;
        username) P_USERNAME="$sl" ;;
        password) P_PASSWORD="$sl" ;;
        token) P_TOKEN="$sl" ;;
        custom_url) P_CUSTOM_URL="$sl" ;;
        interval) P_INTERVAL="$sl" ;;
        test_now) P_TEST_NOW="$sl" ;;
        target) P_TARGET="$sl" ;;
        max_fail) P_MAX_FAIL="$sl" ;;
        action_mode) P_ACT_MODE="$sl" ;;
        tz) P_TZ="$sl" ;;
        night) P_NIGHT="$sl" ;;
        ssh_keys) P_SSH_KEYS="$dv" ;;
        rc_local) P_RC_LOCAL="$dv" ;;
        band) P_BAND="$sl" ;;
        essid) P_ESSID="$sl" ;;
        sec_mode) P_SEC="$sl" ;;
        psk) P_PSK="$sl" ;;
        ip_config_method) P_IPCFG="$sl" ;;
        ip) P_IP="$sl" ;;
        gw) P_GW="$sl" ;;
    esac
done
IFS="$OLD_IFS"

lock_cfg() {
    LOCKED=0
    if [ -x /bin/config_lock.sh ] && /bin/config_lock.sh $$ >/dev/null 2>&1; then
        LOCKED=1
    fi
}

unlock_cfg() {
    if [ "$LOCKED" = "1" ] && [ -x /bin/config_unlock.sh ]; then
        /bin/config_unlock.sh $$ >/dev/null 2>&1 || true
    fi
}

case "$P_ACTION" in
    opmode)
        [ "$P_NET_TYPE" = "0" ] || [ "$P_NET_TYPE" = "2" ] || json_error "Invalid network_type (must be 0 or 2)"
        case "$P_BR_MODE" in 2|3|4) ;; *) json_error "Invalid bridge_mode (must be 2, 3, or 4)" ;; esac
        [ -z "$P_LOOP" ] && P_LOOP="0"
        [ "$P_LOOP" = "0" ] || [ "$P_LOOP" = "1" ] || json_error "Invalid loop_breaking value"
        if [ -n "$P_MAC_CLONE" ] && [ "$P_MAC_CLONE" != "0" ]; then
            is_mac "$P_MAC_CLONE" || json_error "Invalid cloned MAC address"
        else
            P_MAC_CLONE="0"
        fi
        cur_nt=$(get_conf wlan0.conf network_type 2)
        lock_cfg
        ensure_jffs_conf sys.conf
        ensure_jffs_conf wlan0.conf
        set_conf_kv "$JFFS/sys.conf" BridgeMode "$P_BR_MODE"
        set_conf_kv "$JFFS/sys.conf" MacCloningAddr "$P_MAC_CLONE"
        set_conf_kv "$JFFS/sys.conf" LoopBreakingEnabled "$P_LOOP"
        set_conf_kv "$JFFS/wlan0.conf" network_type "$P_NET_TYPE"
        set_conf_kv "$JFFS/wlan0.conf" BridgeMode "$P_BR_MODE"
        set_conf_kv "$JFFS/wlan0.conf" LoopBreakingEnabled "$P_LOOP"
        sync_sys_and_wlan
        unlock_cfg
        need_reboot=0
        [ "$P_NET_TYPE" != "$cur_nt" ] && need_reboot=1
        iw_bm="$P_BR_MODE"
        [ "$iw_bm" = "4" ] && iw_bm="0"
        [ "$need_reboot" = "0" ] && iwpriv wlan0 s_BridgeMode "$iw_bm" >/dev/null 2>&1 || true
        if [ "$need_reboot" = "1" ] && [ "$P_REBOOT" = "1" ]; then
            [ -x /bin/config_save.sh ] && /bin/config_save.sh >/dev/null 2>&1 || true
            ( sleep 2; sync; /sbin/reboot ) </dev/null >/dev/null 2>&1 &
        else
            trigger_save
        fi
        json_header
        printf '{"status":"success","reboot":%s}\n' "$need_reboot"
        ;;

    quick_setup)
        [ "$P_NET_TYPE" = "0" ] || [ "$P_NET_TYPE" = "2" ] || json_error "Invalid operation mode"
        case "$P_BR_MODE" in 2|3|4) ;; *) P_BR_MODE="4" ;; esac
        [ "$P_BAND" = "0" ] || [ "$P_BAND" = "1" ] || json_error "Invalid frequency band"
        [ -n "$P_ESSID" ] && [ ${#P_ESSID} -le 32 ] || json_error "SSID must be 1-32 characters"
        case "$P_ESSID" in *\"*|*\`*|*\$*|*\\*) json_error "SSID contains unsupported shell quote characters" ;; esac
        [ "$P_SEC" = "1" ] || [ "$P_SEC" = "3" ] || json_error "Quick Setup supports Open (1) or WPA2-AES (3)"
        if [ "$P_SEC" = "3" ]; then
            [ ${#P_PSK} -ge 8 ] && [ ${#P_PSK} -le 64 ] || json_error "Passphrase must be 8-64 characters"
        fi
        [ "$P_IPCFG" = "0" ] || [ "$P_IPCFG" = "1" ] || json_error "Invalid IP config method"
        if [ "$P_IPCFG" = "1" ]; then
            is_ipv4 "$P_IP" || json_error "Invalid Bridge IPv4 address"
            is_netmask "$P_SUBNET" || json_error "Invalid Subnet Mask"
            [ -n "$P_GW" ] && { is_ipv4 "$P_GW" || json_error "Invalid Gateway IPv4"; }
        fi
        [ "$P_ENABLED" = "1" ] || P_ENABLED="0"
        [ "$P_IPCFG" = "0" ] && P_ENABLED="0"

        cur_nt=$(get_conf wlan0.conf network_type 2)
        net_mode="23"
        [ "$P_BAND" = "0" ] && net_mode="14"

        lock_cfg
        ensure_jffs_conf sys.conf
        ensure_jffs_conf wlan0.conf
        set_conf_kv "$JFFS/sys.conf" BridgeMode "$P_BR_MODE"
        set_conf_kv "$JFFS/sys.conf" ip_config_method "$P_IPCFG"
        set_conf_kv "$JFFS/sys.conf" DHCPDEnabled "$P_ENABLED"
        set_conf_kv "$JFFS/wlan0.conf" network_type "$P_NET_TYPE"
        set_conf_kv "$JFFS/wlan0.conf" BridgeMode "$P_BR_MODE"
        set_conf_kv "$JFFS/wlan0.conf" ip_config_method "$P_IPCFG"
        set_conf_kv "$JFFS/wlan0.conf" DHCPDEnabled "$P_ENABLED"
        set_conf_kv "$JFFS/wlan0.conf" FrequencyBand "$P_BAND"
        set_conf_kv "$JFFS/wlan0.conf" NetworkMode "$net_mode"
        set_conf_kv "$JFFS/wlan0.conf" NonProc_ESSID "$P_ESSID"
        set_conf_kv "$JFFS/wlan0.conf" Wildcard_ESSID "$P_ESSID"
        set_conf_kv "$JFFS/wlan0.conf" NonProcSecurityMode "$P_SEC"
        if [ "$P_SEC" = "3" ]; then
            set_conf_kv "$JFFS/wlan0.conf" NonProc_WPA_Personal_Mode "2"
            set_conf_kv "$JFFS/wlan0.conf" NonProc_WPA_Personal_Encapsulation "1"
            set_conf_kv "$JFFS/wlan0.conf" NonProc_WPA_Personal_PSK "$P_PSK"
        fi
        set_conf_kv "$JFFS/wlan0.conf" unconfigured "0"
        if [ "$P_IPCFG" = "1" ]; then
            set_conf_kv "$JFFS/sys.conf" ip_lan "$P_IP"
            set_conf_kv "$JFFS/sys.conf" subnet_lan "$P_SUBNET"
            set_conf_kv "$JFFS/wlan0.conf" ip_lan "$P_IP"
            set_conf_kv "$JFFS/wlan0.conf" subnet_lan "$P_SUBNET"
            if [ -n "$P_GW" ]; then
                set_conf_kv "$JFFS/sys.conf" default_gw "$P_GW"
                set_conf_kv "$JFFS/wlan0.conf" default_gw "$P_GW"
            fi
        fi
        sync_sys_and_wlan
        unlock_cfg

        need_reboot=0
        [ "$P_NET_TYPE" != "$cur_nt" ] && need_reboot=1
        if [ "$need_reboot" = "1" ]; then
            [ -x /bin/config_save.sh ] && /bin/config_save.sh >/dev/null 2>&1 || true
            ( sleep 2; sync; /sbin/reboot ) </dev/null >/dev/null 2>&1 &
        else
            (
                sleep 1
                if [ "$P_IPCFG" = "0" ]; then
                    killall udhcpd udhcpc 2>/dev/null || true
                    /etc/udhcpc/dhcp.tcl startup >/dev/null 2>&1 &
                else
                    killall udhcpc 2>/dev/null || true
                    ifconfig br0 "$P_IP" netmask "$P_SUBNET" 2>/dev/null || true
                    if [ -n "$P_GW" ] && [ "$P_GW" != "$P_IP" ] && [ "$P_GW" != "0.0.0.0" ]; then
                        route del default >/dev/null 2>&1 || true
                        route add default gw "$P_GW" dev br0 >/dev/null 2>&1 || true
                    fi
                    /bin/router_apply.sh dhcp >/dev/null 2>&1 || true
                fi
                iwpriv wlan0 sNetworkMode "$net_mode" >/dev/null 2>&1 || true
                iw_bm="$P_BR_MODE"; [ "$iw_bm" = "4" ] && iw_bm="0"
                iwpriv wlan0 s_BridgeMode "$iw_bm" >/dev/null 2>&1 || true
                [ -x /root/mtlk/web/init_security.tcl ] && ( cd /root/mtlk/web && ./init_security.tcl reactivate ) >/dev/null 2>&1 || true
            ) </dev/null >/dev/null 2>&1 &
            trigger_save
        fi
        json_header
        printf '{"status":"success","reboot":%s,"ip":"%s"}\n' "$need_reboot" "$(printf '%s' "$P_IP" | json_esc)"
        ;;

    dhcp_action)
        case "$P_CMD" in
            renew) killall -USR1 udhcpc 2>/dev/null || true ;;
            release) killall -USR2 udhcpc 2>/dev/null || true ;;
            *) json_error "Invalid DHCP client command" ;;
        esac
        json_header
        echo '{"status":"success"}'
        ;;

    dhcp_server)
        [ "$P_ENABLED" = "1" ] || P_ENABLED="0"
        is_ipv4 "$P_START" || json_error "Invalid DHCP Start IPv4"
        is_ipv4 "$P_END" || json_error "Invalid DHCP End IPv4"
        is_netmask "$P_SUBNET" || json_error "Invalid DHCP Subnet Mask"
        is_uint_range "$P_LEASE" 60 2592000 || json_error "Lease time must be 60..2592000s"
        [ -z "$P_DOMAIN" ] && P_DOMAIN="Local"
        is_host "$P_DOMAIN" || json_error "Invalid DHCP domain name"
        [ -n "$P_DNS1" ] && { is_ipv4 "$P_DNS1" || json_error "Invalid Primary DNS"; }
        [ -n "$P_DNS2" ] && { is_ipv4 "$P_DNS2" || json_error "Invalid Secondary DNS"; }

        # Validate & stage static leases (MAC|IP|HOST per line, max 32)
        tmp_sl="/tmp/dhcp_static.stage.$$"
        : > "$tmp_sl"
        sl_count=0
        old_ifs="$IFS"
        IFS='
'
        for line in $P_STATIC_LEASES; do
            line=$(printf '%s' "$line" | sed 's/^[ \t]*//;s/[ \t]*$//')
            [ -z "$line" ] && continue
            IFS='|' read -r s_mac s_ip s_host <<EOF
$line
EOF
            is_mac "$s_mac" || { rm -f "$tmp_sl"; IFS="$old_ifs"; json_error "Invalid MAC in static lease: $s_mac"; }
            is_ipv4 "$s_ip" || { rm -f "$tmp_sl"; IFS="$old_ifs"; json_error "Invalid IPv4 in static lease: $s_ip"; }
            [ -n "$s_host" ] && ! is_host "$s_host" && { rm -f "$tmp_sl"; IFS="$old_ifs"; json_error "Invalid hostname in static lease"; }
            sl_count=$((sl_count + 1))
            [ "$sl_count" -gt 32 ] && { rm -f "$tmp_sl"; IFS="$old_ifs"; json_error "Maximum 32 static leases supported"; }
            printf '%s|%s|%s\n' "$(printf '%s' "$s_mac" | tr '[:upper:]' '[:lower:]')" "$s_ip" "$s_host" >> "$tmp_sl"
        done
        IFS="$old_ifs"

        lock_cfg
        ensure_jffs_conf sys.conf
        ensure_jffs_conf wlan0.conf
        set_conf_kv "$JFFS/sys.conf" DHCPDEnabled "$P_ENABLED"
        set_conf_kv "$JFFS/sys.conf" DHCPDStartAddress "$P_START"
        set_conf_kv "$JFFS/sys.conf" DHCPDEndAddress "$P_END"
        set_conf_kv "$JFFS/sys.conf" DHCPDSubnet "$P_SUBNET"
        set_conf_kv "$JFFS/sys.conf" DHCPDLeaseTime "$P_LEASE"
        set_conf_kv "$JFFS/sys.conf" DHCPDDomain "$P_DOMAIN"
        set_conf_kv "$JFFS/sys.conf" DHCPDDNS1 "${P_DNS1:-1.1.1.1}"
        set_conf_kv "$JFFS/sys.conf" DHCPDDNS2 "${P_DNS2:-8.8.8.8}"
        chmod 600 "$tmp_sl" 2>/dev/null || true
        mv -f "$tmp_sl" "$JFFS/dhcp_static.conf" 2>/dev/null || rm -f "$tmp_sl"
        sync_sys_and_wlan
        unlock_cfg

        /bin/router_apply.sh dhcp >/dev/null 2>&1 || true
        trigger_save
        json_header
        echo '{"status":"success"}'
        ;;

    wlan_acl)
        case "$P_MODE" in 0|1|2) ;; *) json_error "Invalid ACL mode (must be 0, 1, or 2)" ;; esac
        acl_list=""
        acl_count=0
        old_ifs="$IFS"
        IFS=' ,
'
        for mac in $P_MACS; do
            [ -z "$mac" ] && continue
            is_mac "$mac" || { IFS="$old_ifs"; json_error "Invalid MAC address: $mac"; }
            acl_count=$((acl_count + 1))
            [ "$acl_count" -gt 32 ] && { IFS="$old_ifs"; json_error "Maximum 32 MAC entries supported"; }
            mac_up=$(printf '%s' "$mac" | tr '[:lower:]' '[:upper:]')
            [ -z "$acl_list" ] && acl_list="$mac_up" || acl_list="$acl_list $mac_up"
        done
        IFS="$old_ifs"

        lock_cfg
        ensure_jffs_conf wlan0.conf
        set_conf_kv "$JFFS/wlan0.conf" AclMode "$P_MODE"
        set_conf_kv "$JFFS/wlan0.conf" ACL "$acl_list"
        sync_sys_and_wlan
        unlock_cfg

        set_proc_wlan AclMode "$P_MODE"
        set_proc_wlan ACL "$acl_list"
        trigger_save
        json_header
        echo '{"status":"success"}'
        ;;

    wlan_rf)
        is_uint_range "$P_MAX_CONN" 1 32 || json_error "MaxConnections must be 1..32"
        is_uint_range "$P_BEACON" 20 1000 || json_error "BeaconPeriod must be 20..1000 ms"
        is_uint_range "$P_DTIM" 1 255 || json_error "DTIMPeriod must be 1..255"
        is_uint_range "$P_RTS" 256 2346 || json_error "RTSThreshold must be 256..2346"
        [ "$P_SHORT_PRE" = "1" ] || P_SHORT_PRE="0"
        [ "$P_SHORT_SLOT" = "1" ] || P_SHORT_SLOT="0"
        [ "$P_STBC" = "1" ] || P_STBC="0"
        [ "$P_SHORT_GI" = "1" ] || P_SHORT_GI="0"
        is_uint_range "$P_STA_KA" 5000 600000 || json_error "StaKeepaliveTimeout must be 5000..600000 ms"

        lock_cfg
        ensure_jffs_conf wlan0.conf
        set_conf_kv "$JFFS/wlan0.conf" MaxConnections "$P_MAX_CONN"
        set_conf_kv "$JFFS/wlan0.conf" BeaconPeriod "$P_BEACON"
        set_conf_kv "$JFFS/wlan0.conf" DTIMPeriod "$P_DTIM"
        set_conf_kv "$JFFS/wlan0.conf" RTSThreshold "$P_RTS"
        set_conf_kv "$JFFS/wlan0.conf" ShortPreamble "$P_SHORT_PRE"
        set_conf_kv "$JFFS/wlan0.conf" ShortSlotTimeOptionEnabled "$P_SHORT_SLOT"
        set_conf_kv "$JFFS/wlan0.conf" STBC "$P_STBC"
        set_conf_kv "$JFFS/wlan0.conf" ShortCyclicPrefix "$P_SHORT_GI"
        set_conf_kv "$JFFS/wlan0.conf" StaKeepaliveTimeout "$P_STA_KA"
        sync_sys_and_wlan
        unlock_cfg

        set_proc_wlan MaxConnections "$P_MAX_CONN"
        set_proc_wlan BeaconPeriod "$P_BEACON"
        set_proc_wlan DTIMPeriod "$P_DTIM"
        set_proc_wlan ShortPreamble "$P_SHORT_PRE"
        set_proc_wlan ShortSlotTimeOptionEnabled "$P_SHORT_SLOT"
        set_proc_wlan STBC "$P_STBC"
        set_proc_wlan ShortCyclicPrefix "$P_SHORT_GI"
        set_proc_wlan StaKeepaliveTimeout "$P_STA_KA"
        /sbin/iwconfig wlan0 rts "$P_RTS" >/dev/null 2>&1 || true
        trigger_save
        json_header
        echo '{"status":"success"}'
        ;;

    routes)
        tmp_rt="/tmp/routes.stage.$$"
        : > "$tmp_rt"
        rt_cnt=0
        old_ifs="$IFS"
        IFS='
'
        for line in $P_ROUTES; do
            line=$(printf '%s' "$line" | sed 's/^[ \t]*//;s/[ \t]*$//')
            [ -z "$line" ] && continue
            IFS='|' read -r r_dest r_mask r_gw r_if r_met <<EOF
$line
EOF
            is_ipv4 "$r_dest" || { rm -f "$tmp_rt"; IFS="$old_ifs"; json_error "Invalid destination IPv4: $r_dest"; }
            is_netmask "$r_mask" || { rm -f "$tmp_rt"; IFS="$old_ifs"; json_error "Invalid netmask: $r_mask"; }
            [ -n "$r_gw" ] && [ "$r_gw" != "0.0.0.0" ] && { is_ipv4 "$r_gw" || { rm -f "$tmp_rt"; IFS="$old_ifs"; json_error "Invalid gateway IPv4: $r_gw"; }; }
            case "$r_if" in ""|br0|eth0|wlan0|eth0.[0-9]*) ;; *) rm -f "$tmp_rt"; IFS="$old_ifs"; json_error "Invalid interface: $r_if" ;; esac
            [ -n "$r_met" ] && ! is_uint_range "$r_met" 0 9999 && { rm -f "$tmp_rt"; IFS="$old_ifs"; json_error "Invalid metric: $r_met"; }
            rt_cnt=$((rt_cnt + 1))
            [ "$rt_cnt" -gt 16 ] && { rm -f "$tmp_rt"; IFS="$old_ifs"; json_error "Maximum 16 static routes supported"; }
            printf '%s|%s|%s|%s|%s\n' "$r_dest" "$r_mask" "${r_gw:-0.0.0.0}" "${r_if:-br0}" "${r_met:-0}" >> "$tmp_rt"
        done
        IFS="$old_ifs"
        mkdir -p "$JFFS" 2>/dev/null || true
        chmod 600 "$tmp_rt" 2>/dev/null || true
        mv -f "$tmp_rt" "$JFFS/routes.conf" 2>/dev/null || rm -f "$tmp_rt"
        /bin/router_apply.sh routes >/dev/null 2>&1 || true
        trigger_save
        json_header
        echo '{"status":"success"}'
        ;;

    vlan)
        tmp_vl="/tmp/vlan.stage.$$"
        : > "$tmp_vl"
        vl_cnt=0
        old_ifs="$IFS"
        IFS='
'
        for line in $P_VLANS; do
            line=$(printf '%s' "$line" | sed 's/^[ \t]*//;s/[ \t]*$//')
            [ -z "$line" ] && continue
            IFS='|' read -r v_id v_if v_mode v_ip v_mask <<EOF
$line
EOF
            is_uint_range "$v_id" 2 4094 || { rm -f "$tmp_vl"; IFS="$old_ifs"; json_error "VLAN ID must be 2..4094"; }
            [ "$v_if" = "eth0" ] || { rm -f "$tmp_vl"; IFS="$old_ifs"; json_error "Only eth0 VLAN trunking is supported"; }
            [ "$v_mode" = "routed" ] || v_mode="bridge"
            if [ "$v_mode" = "routed" ]; then
                is_ipv4 "$v_ip" && is_netmask "$v_mask" || { rm -f "$tmp_vl"; IFS="$old_ifs"; json_error "Routed VLAN requires valid IPv4 and netmask"; }
            else
                v_ip=""; v_mask=""
            fi
            vl_cnt=$((vl_cnt + 1))
            [ "$vl_cnt" -gt 8 ] && { rm -f "$tmp_vl"; IFS="$old_ifs"; json_error "Maximum 8 VLANs supported"; }
            printf '%s|%s|%s|%s|%s\n' "$v_id" "$v_if" "$v_mode" "$v_ip" "$v_mask" >> "$tmp_vl"
        done
        IFS="$old_ifs"
        mkdir -p "$JFFS" 2>/dev/null || true
        chmod 600 "$tmp_vl" 2>/dev/null || true
        mv -f "$tmp_vl" "$JFFS/vlan.conf" 2>/dev/null || rm -f "$tmp_vl"
        /bin/router_apply.sh vlan >/dev/null 2>&1 || true
        trigger_save
        json_header
        echo '{"status":"success"}'
        ;;

    qos)
        [ "$P_ENABLED" = "1" ] || P_ENABLED="0"
        [ -z "$P_DOWN" ] && P_DOWN="0"
        [ -z "$P_UP" ] && P_UP="0"
        is_uint_range "$P_DOWN" 0 100000 || json_error "Download limit must be 0..100000 kbps"
        is_uint_range "$P_UP" 0 100000 || json_error "Upload limit must be 0..100000 kbps"
        mkdir -p "$JFFS" 2>/dev/null || true
        printf 'ENABLED=%s\nDOWN_KBPS=%s\nUP_KBPS=%s\n' "$P_ENABLED" "$P_DOWN" "$P_UP" | atomic_write "$JFFS/qos.conf" 600
        /bin/router_apply.sh qos >/dev/null 2>&1 || true
        trigger_save
        json_header
        echo '{"status":"success"}'
        ;;

    ddns)
        [ "$P_ENABLED" = "1" ] || P_ENABLED="0"
        case "$P_PROVIDER" in duckdns|noip|dyndns|custom) ;; *) json_error "Unsupported DDNS provider" ;; esac
        [ -n "$P_INTERVAL" ] || P_INTERVAL="600"
        is_uint_range "$P_INTERVAL" 300 86400 || json_error "DDNS interval must be 300..86400s"
        if [ "$P_PROVIDER" != "custom" ] && [ -n "$P_HOSTNAME" ]; then
            is_host "$P_HOSTNAME" || json_error "Invalid DDNS hostname"
        fi
        if [ -n "$P_USERNAME" ]; then
            case "$P_USERNAME" in *[!A-Za-z0-9._:@-]*) json_error "Invalid characters in DDNS username" ;; esac
        fi
        if [ -n "$P_CUSTOM_URL" ]; then
            case "$P_CUSTOM_URL" in
                http://*|https://*)
                    case "$P_CUSTOM_URL" in *[!A-Za-z0-9._:/?\&=%~+-]*) json_error "Invalid characters in custom DDNS URL" ;; esac
                    ;;
                *) json_error "Custom URL must start with http:// or https://" ;;
            esac
        fi
        # Preserve stored secrets when form submits empty password/token
        [ -z "$P_PASSWORD" ] && P_PASSWORD=$(get_kv_file "$JFFS/ddns.conf" PASSWORD "")
        [ -z "$P_TOKEN" ] && P_TOKEN=$(get_kv_file "$JFFS/ddns.conf" TOKEN "")
        case "$P_TOKEN" in *[!A-Za-z0-9._:-]*) [ -n "$P_TOKEN" ] && json_error "Invalid characters in DDNS token" ;; esac

        mkdir -p "$JFFS" 2>/dev/null || true
        {
            printf 'ENABLED=%s\n' "$P_ENABLED"
            printf 'PROVIDER=%s\n' "$P_PROVIDER"
            printf 'HOSTNAME=%s\n' "$P_HOSTNAME"
            printf 'USERNAME=%s\n' "$P_USERNAME"
            printf 'PASSWORD=%s\n' "$P_PASSWORD"
            printf 'TOKEN=%s\n' "$P_TOKEN"
            printf 'CUSTOM_URL=%s\n' "$P_CUSTOM_URL"
            printf 'INTERVAL=%s\n' "$P_INTERVAL"
        } | atomic_write "$JFFS/ddns.conf" 600
        /bin/router_apply.sh ddns >/dev/null 2>&1 || true
        if [ "$P_TEST_NOW" = "1" ] && [ -x /bin/ddns_updater.sh ]; then
            /bin/ddns_updater.sh --once >/dev/null 2>&1 || true
        fi
        trigger_save
        dd_st=$(cat /tmp/ddns.status 2>/dev/null || echo "")
        json_header
        printf '{"status":"success","ddns_status":"%s"}\n' "$(printf '%s' "$dd_st" | json_esc)"
        ;;

    watchdog)
        [ "$P_ENABLED" = "1" ] || P_ENABLED="0"
        [ -z "$P_TARGET" ] && P_TARGET="1.1.1.1"
        is_host "$P_TARGET" || json_error "Invalid watchdog target host or IPv4"
        is_uint_range "$P_INTERVAL" 30 3600 || json_error "Interval must be 30..3600s"
        is_uint_range "$P_MAX_FAIL" 2 20 || json_error "Max failures must be 2..20"
        [ "$P_ACT_MODE" = "reboot" ] || P_ACT_MODE="recover"
        mkdir -p "$JFFS" 2>/dev/null || true
        printf 'ENABLED=%s\nTARGET=%s\nINTERVAL=%s\nMAX_FAIL=%s\nACTION=%s\n' \
            "$P_ENABLED" "$P_TARGET" "$P_INTERVAL" "$P_MAX_FAIL" "$P_ACT_MODE" | atomic_write "$JFFS/watchdog.conf" 600
        /bin/router_apply.sh watchdog >/dev/null 2>&1 || true
        trigger_save
        json_header
        echo '{"status":"success"}'
        ;;

    tz)
        is_tz "$P_TZ" || json_error "Invalid POSIX TZ string"
        mkdir -p "$JFFS" 2>/dev/null || true
        printf '%s\n' "$P_TZ" | atomic_write "$JFFS/tz.conf" 644
        /bin/router_apply.sh tz >/dev/null 2>&1 || true
        trigger_save
        now_str=$(date '+%Y-%m-%d %H:%M:%S %Z' 2>/dev/null || date 2>/dev/null || echo "")
        json_header
        printf '{"status":"success","now":"%s"}\n' "$(printf '%s' "$now_str" | json_esc)"
        ;;

    timesync)
        ok=0
        if [ -x /bin/timesync.sh ]; then
            /bin/timesync.sh --once >/dev/null 2>&1 && ok=1 || ok=0
        fi
        now_str=$(date '+%Y-%m-%d %H:%M:%S %Z' 2>/dev/null || date 2>/dev/null || echo "")
        ts_log=$(cat /tmp/timesync.log 2>/dev/null || echo "")
        json_header
        printf '{"status":"success","synced":%s,"now":"%s","timesync_log":"%s"}\n' \
            "$ok" "$(printf '%s' "$now_str" | json_esc)" "$(printf '%s' "$ts_log" | json_esc)"
        ;;

    led)
        [ "$P_NIGHT" = "1" ] || P_NIGHT="0"
        mkdir -p "$JFFS" 2>/dev/null || true
        printf 'NIGHT_MODE=%s\n' "$P_NIGHT" | atomic_write "$JFFS/led.conf" 600
        /bin/router_apply.sh led >/dev/null 2>&1 || true
        trigger_save
        json_header
        echo '{"status":"success"}'
        ;;

    ssh_keys)
        [ ${#P_SSH_KEYS} -le 4096 ] || json_error "Authorized keys exceeds 4096 bytes"
        tmp_ak="/tmp/authkeys.stage.$$"
        : > "$tmp_ak"
        old_ifs="$IFS"
        IFS='
'
        for line in $P_SSH_KEYS; do
            line=$(printf '%s' "$line" | sed 's/^[ \t]*//;s/[ \t]*$//')
            [ -z "$line" ] && continue
            case "$line" in
                \#*) printf '%s\n' "$line" >> "$tmp_ak" ;;
                ssh-ed25519\ *|ssh-rsa\ *|ecdsa-sha2-*\ *|sk-*\ *) printf '%s\n' "$line" >> "$tmp_ak" ;;
                *) rm -f "$tmp_ak"; IFS="$old_ifs"; json_error "Unsupported SSH key type line" ;;
            esac
        done
        IFS="$old_ifs"
        mkdir -p "$JFFS/dropbear" /root/.ssh 2>/dev/null || true
        chmod 700 "$JFFS/dropbear" /root /root/.ssh 2>/dev/null || true
        cp -f "$tmp_ak" /root/.ssh/authorized_keys 2>/dev/null || true
        chmod 600 /root/.ssh/authorized_keys 2>/dev/null || true
        chmod 600 "$tmp_ak" 2>/dev/null || true
        mv -f "$tmp_ak" "$JFFS/dropbear/authorized_keys" 2>/dev/null || rm -f "$tmp_ak"
        trigger_save
        json_header
        echo '{"status":"success"}'
        ;;

    rc_local)
        [ ${#P_RC_LOCAL} -le 4096 ] || json_error "Startup script exceeds 4096 bytes"
        mkdir -p "$JFFS" 2>/dev/null || true
        trimmed=$(printf '%s' "$P_RC_LOCAL" | sed '/^[ \t]*$/d')
        if [ -z "$trimmed" ]; then
            rm -f "$JFFS/rc.local" 2>/dev/null || true
        else
            first_line=$(printf '%s\n' "$P_RC_LOCAL" | head -n 1)
            case "$first_line" in
                \#!*) printf '%s\n' "$P_RC_LOCAL" | atomic_write "$JFFS/rc.local" 755 ;;
                *) { echo "#!/bin/sh"; printf '%s\n' "$P_RC_LOCAL"; } | atomic_write "$JFFS/rc.local" 755 ;;
            esac
        fi
        trigger_save
        json_header
        echo '{"status":"success"}'
        ;;

    *)
        json_error "Unsupported action"
        ;;
esac
