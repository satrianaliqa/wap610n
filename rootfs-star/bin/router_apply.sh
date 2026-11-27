#!/bin/sh
# Unified Idempotent Runtime & Boot Configuration Applier for WAP610N

JFFS="/mnt/jffs2"
MOD_8021Q="/lib/modules/2.6.16-star/net/8021q.ko"

is_ipv4() {
    case "$1" in
        ''|*[!0-9.]*) return 1 ;;
    esac
    awk -v ip="$1" 'BEGIN {
        n = split(ip, o, ".")
        if (n != 4) exit 1
        for (i = 1; i <= 4; i++) {
            if (o[i] == "" || o[i] < 0 || o[i] > 255) exit 1
        }
        exit 0
    }'
}

stop_pidfile() {
    pf="$1"
    if [ -f "$pf" ]; then
        pid=$(cat "$pf" 2>/dev/null)
        case "$pid" in
            ''|*[!0-9]*) ;;
            *) kill "$pid" 2>/dev/null || true ;;
        esac
        rm -f "$pf" 2>/dev/null || true
    fi
}

apply_tz() {
    if [ -s "$JFFS/tz.conf" ]; then
        tz=$(head -n 1 "$JFFS/tz.conf" 2>/dev/null | tr -d ' \t\r\n')
        case "$tz" in
            ''|*[!A-Za-z0-9+:,-]*) ;;
            *)
                printf '%s\n' "$tz" > /etc/TZ 2>/dev/null || true
                chmod 644 /etc/TZ 2>/dev/null || true
                ;;
        esac
    fi
}

apply_led() {
    night="0"
    if [ -f "$JFFS/led.conf" ]; then
        night=$(sed -n 's/^NIGHT_MODE=\([01]\).*/\1/p' "$JFFS/led.conf" 2>/dev/null | tail -n 1)
    fi
    if [ "$night" = "1" ]; then
        echo 0 > /dev/led0 2>/dev/null || true
        echo 0 > /dev/gpio0 2>/dev/null || true
        echo 0 > /dev/gpio2 2>/dev/null || true
    else
        echo 0 > /dev/gpio2 2>/dev/null || true
        echo 1 > /dev/gpio0 2>/dev/null || true
        echo 1 > /dev/led0 2>/dev/null || true
    fi
}

apply_routes() {
    state="/tmp/managed_routes.state"
    if [ -f "$state" ]; then
        while IFS='|' read -r dest mask gw iface metric; do
            [ -z "$dest" ] && continue
            is_ipv4 "$dest" && is_ipv4 "$mask" || continue
            cmd="route del -net $dest netmask $mask"
            [ -n "$gw" ] && [ "$gw" != "0.0.0.0" ] && is_ipv4 "$gw" && cmd="$cmd gw $gw"
            case "$iface" in
                br0|eth0|wlan0|eth0.[0-9]*) cmd="$cmd dev $iface" ;;
            esac
            $cmd 2>/dev/null || true
        done < "$state"
        rm -f "$state" 2>/dev/null || true
    fi

    [ -s "$JFFS/routes.conf" ] || return 0
    applied=0
    while IFS='|' read -r dest mask gw iface metric; do
        case "$dest" in ''|\#*) continue ;; esac
        is_ipv4 "$dest" && is_ipv4 "$mask" || continue
        [ -n "$gw" ] && [ "$gw" != "0.0.0.0" ] && { is_ipv4 "$gw" || continue; }
        cmd="route add -net $dest netmask $mask"
        [ -n "$gw" ] && [ "$gw" != "0.0.0.0" ] && cmd="$cmd gw $gw"
        case "$metric" in
            ''|*[!0-9]*) ;;
            0) ;;
            *) cmd="$cmd metric $metric" ;;
        esac
        case "$iface" in
            br0|eth0|wlan0|eth0.[0-9]*) cmd="$cmd dev $iface" ;;
        esac
        if $cmd 2>/dev/null; then
            printf '%s|%s|%s|%s|%s\n' "$dest" "$mask" "$gw" "$iface" "$metric" >> "$state"
            applied=1
        fi
    done < "$JFFS/routes.conf"
    if [ "$applied" = "1" ]; then
        echo 1 > /proc/sys/net/ipv4/ip_forward 2>/dev/null || true
    fi
}

apply_vlan() {
    state="/tmp/managed_vlans.state"
    if [ -f "$state" ]; then
        while IFS= read -r vif; do
            [ -z "$vif" ] && continue
            brctl delif br0 "$vif" 2>/dev/null || true
            ifconfig "$vif" down 2>/dev/null || true
            vconfig rem "$vif" 2>/dev/null || true
        done < "$state"
        rm -f "$state" 2>/dev/null || true
    fi

    [ -s "$JFFS/vlan.conf" ] || return 0
    if ! grep -q '^8021q ' /proc/modules 2>/dev/null; then
        [ -f "$MOD_8021Q" ] || return 1
        insmod "$MOD_8021Q" 2>/dev/null || return 1
    fi

    routed=0
    while IFS='|' read -r vid iface mode ip mask; do
        case "$vid" in ''|\#*|*[!0-9]*) continue ;; esac
        [ "$vid" -ge 2 ] 2>/dev/null && [ "$vid" -le 4094 ] 2>/dev/null || continue
        [ "$iface" = "eth0" ] || continue
        vif="${iface}.${vid}"
        vconfig add "$iface" "$vid" 2>/dev/null || continue
        if [ "$mode" = "routed" ] && is_ipv4 "$ip" && is_ipv4 "$mask"; then
            ifconfig "$vif" "$ip" netmask "$mask" up 2>/dev/null || true
            routed=1
        else
            ifconfig "$vif" 0.0.0.0 up 2>/dev/null || true
            brctl addif br0 "$vif" 2>/dev/null || true
        fi
        printf '%s\n' "$vif" >> "$state"
    done < "$JFFS/vlan.conf"
    if [ "$routed" = "1" ]; then
        echo 1 > /proc/sys/net/ipv4/ip_forward 2>/dev/null || true
    fi
}

apply_qos() {
    enabled="0"
    down_kbps="0"
    up_kbps="0"
    if [ -f "$JFFS/qos.conf" ]; then
        while IFS='=' read -r k v; do
            v=$(printf '%s' "$v" | tr -d ' \t\r\n')
            case "$k" in
                ENABLED) enabled="$v" ;;
                DOWN_KBPS) down_kbps="$v" ;;
                UP_KBPS) up_kbps="$v" ;;
            esac
        done < "$JFFS/qos.conf"
    fi
    case "$down_kbps" in ''|*[!0-9]*) down_kbps=0 ;; esac
    case "$up_kbps" in ''|*[!0-9]*) up_kbps=0 ;; esac

    tc qdisc del dev wlan0 root 2>/dev/null || true
    tc qdisc del dev eth0 root 2>/dev/null || true

    if [ "$enabled" != "1" ]; then
        if [ -f /root/mtlk/etc/qos_d.sh ]; then
            ( cd /root/mtlk/etc && . ./qos_d.sh ) >/dev/null 2>&1 || true
        fi
        return 0
    fi

    if [ "$down_kbps" -gt 0 ] 2>/dev/null; then
        tc qdisc add dev wlan0 root handle 1: cbq avpkt 1000 bandwidth 100Mbit 2>/dev/null || true
        tc class add dev wlan0 parent 1: classid 1:1 cbq rate "${down_kbps}kbit" allot 1514 prio 5 bounded isolated 2>/dev/null || true
        tc filter add dev wlan0 parent 1: protocol ip prio 1 u32 match ip dst 0.0.0.0/0 flowid 1:1 2>/dev/null || true
    elif [ -f /root/mtlk/etc/qos_d.sh ]; then
        ( cd /root/mtlk/etc && . ./qos_d.sh ) >/dev/null 2>&1 || true
    fi

    if [ "$up_kbps" -gt 0 ] 2>/dev/null; then
        tc qdisc add dev eth0 root handle 1: cbq avpkt 1000 bandwidth 100Mbit 2>/dev/null || true
        tc class add dev eth0 parent 1: classid 1:1 cbq rate "${up_kbps}kbit" allot 1514 prio 5 bounded isolated 2>/dev/null || true
        tc filter add dev eth0 parent 1: protocol ip prio 1 u32 match ip src 0.0.0.0/0 flowid 1:1 2>/dev/null || true
    fi
}

apply_ddns() {
    stop_pidfile /tmp/ddns_updater.pid
    en=""
    [ -f "$JFFS/ddns.conf" ] && en=$(sed -n 's/^ENABLED=\([01]\).*/\1/p' "$JFFS/ddns.conf" 2>/dev/null | tail -n 1)
    if [ "$en" = "1" ] && [ -x /bin/ddns_updater.sh ]; then
        /bin/ddns_updater.sh </dev/null >/dev/null 2>&1 &
    fi
}

apply_watchdog() {
    stop_pidfile /tmp/net_watchdog.pid
    en=""
    [ -f "$JFFS/watchdog.conf" ] && en=$(sed -n 's/^ENABLED=\([01]\).*/\1/p' "$JFFS/watchdog.conf" 2>/dev/null | tail -n 1)
    if [ "$en" = "1" ] && [ -x /bin/net_watchdog.sh ]; then
        /bin/net_watchdog.sh </dev/null >/dev/null 2>&1 &
    fi
}

apply_dhcp() {
    killall udhcpd 2>/dev/null || true
    sysf="/tmp/sys.conf"
    [ -f "$JFFS/sys.conf" ] && sysf="$JFFS/sys.conf"
    ip_cfg=$(awk -F '=' '/^[ \t]*ip_config_method[ \t]*=/ {s=$2; gsub(/[ \t\r]/,"",s); print s}' "$sysf" 2>/dev/null | tail -n 1)
    dhcp_en=$(awk -F '=' '/^[ \t]*DHCPDEnabled[ \t]*=/ {s=$2; gsub(/[ \t\r]/,"",s); print s}' "$sysf" 2>/dev/null | tail -n 1)
    if [ "$dhcp_en" = "1" ] && [ "$ip_cfg" != "0" ]; then
        if ( cd /root/mtlk/etc && ./mtlk_init_dhcpd.sh ) >/dev/null 2>&1; then
            nice -n 18 udhcpd /tmp/udhcpd.conf </dev/null >/dev/null 2>&1 &
        fi
    fi
}

case "$1" in
    tz) apply_tz ;;
    led) apply_led ;;
    routes) apply_routes ;;
    vlan) apply_vlan ;;
    qos) apply_qos ;;
    ddns) apply_ddns ;;
    watchdog) apply_watchdog ;;
    dhcp) apply_dhcp ;;
    all)
        apply_tz
        apply_routes
        apply_vlan
        apply_qos
        apply_led
        apply_ddns
        apply_watchdog
        ;;
    *)
        echo "Usage: $0 {tz|led|routes|vlan|qos|ddns|watchdog|dhcp|all}" >&2
        exit 1
        ;;
esac
exit 0
