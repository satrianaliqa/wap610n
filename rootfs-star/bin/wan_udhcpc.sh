#!/bin/sh
# BusyBox udhcpc event script strictly scoped to WAN uplink interface in NAT modes

WAN_STATUS="/tmp/wan_status.conf"
[ -n "$interface" ] || exit 1

write_status() {
    tmp_st="${WAN_STATUS}.tmp.$$"
    {
        printf 'WAN_IF=%s\n' "$interface"
        printf 'WAN_IP=%s\n' "$ip"
        printf 'WAN_MASK=%s\n' "${subnet:-255.255.255.0}"
        printf 'WAN_GW=%s\n' "${router%% *}"
        printf 'WAN_DNS=%s\n' "$dns"
        printf 'WAN_LEASE=%s\n' "${lease:-0}"
    } > "$tmp_st" 2>/dev/null || return 1
    chmod 600 "$tmp_st" 2>/dev/null || true
    mv -f "$tmp_st" "$WAN_STATUS" 2>/dev/null || true
}

case "$1" in
    deconfig)
        ifconfig "$interface" 0.0.0.0 2>/dev/null || true
        rm -f "$WAN_STATUS" 2>/dev/null || true
        ;;
    renew|bound)
        ifconfig "$interface" "$ip" netmask "${subnet:-255.255.255.0}" up 2>/dev/null || true
        if [ -f /mnt/jffs2/router.conf ]; then
            mtu=$(awk -F '=' '$1=="WAN_MTU"{gsub(/[ \t\r]/,"",$2);print $2}' /mnt/jffs2/router.conf 2>/dev/null | tail -n 1)
            case "$mtu" in
                ''|*[!0-9]*) ;;
                *) [ "$mtu" -ge 576 ] && [ "$mtu" -le 1500 ] && ifconfig "$interface" mtu "$mtu" 2>/dev/null || true ;;
            esac
        fi
        if [ -n "$router" ]; then
            gw="${router%% *}"
            while route del default >/dev/null 2>&1; do :; done
            route add default gw "$gw" dev "$interface" 2>/dev/null || true
        fi
        : > /tmp/resolv.conf.tmp.$$
        for d in $dns; do
            printf 'nameserver %s\n' "$d" >> /tmp/resolv.conf.tmp.$$
        done
        if [ ! -s /tmp/resolv.conf.tmp.$$ ]; then
            printf 'nameserver 1.1.1.1\nnameserver 8.8.8.8\n' > /tmp/resolv.conf.tmp.$$
        fi
        mv -f /tmp/resolv.conf.tmp.$$ /tmp/resolv.conf 2>/dev/null || true
        write_status
        [ -x /bin/router_apply.sh ] && /bin/router_apply.sh firewall >/dev/null 2>&1 || true
        ;;
esac
exit 0
