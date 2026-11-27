#!/bin/sh
# Hardened Real-time Diagnostics & System Log CGI for Cisco Linksys WAP610N
set -f

# Retrieve admin password from system configuration (/mnt/jffs2 first)
ADMIN_PASS=""
for conf in /mnt/jffs2/sys.conf /tmp/sys.conf /etc/sys.conf /root/mtlk/etc/sys.conf; do
    if [ -f "$conf" ] && [ -z "$ADMIN_PASS" ]; then
        ADMIN_PASS=$(awk '/^\^?(Admin)?Password[ \t]*=/ {idx = index($0, "="); val = substr($0, idx + 1); sub(/^[ \t]+/, "", val); sub(/[ \t\r]+$/, "", val); print val}' "$conf" 2>/dev/null | tail -n 1)
    fi
done

# Authenticate request fail-closed
if [ -n "$HTTP_AUTHORIZATION" ]; then
    AUTH_IN=$(printf '%s' "$HTTP_AUTHORIZATION" | sed 's/^[Bb]asic //')
    EXPECTED_AUTH=$(printf '%s' "admin:$ADMIN_PASS" | uuencode -m - 2>/dev/null | sed '1d;$d' | tr -d '\r\n')
    if [ -z "$ADMIN_PASS" ] || [ -z "$EXPECTED_AUTH" ] || [ "$AUTH_IN" != "$EXPECTED_AUTH" ]; then
        echo "Status: 401 Unauthorized"
        echo "Content-Type: text/plain; charset=utf-8"
        echo ""
        echo "Unauthorized"
        exit 0
    fi
elif [ -z "$REMOTE_USER" ] && { [ -z "$SERVER_SOFTWARE" ] || [ -z "$GATEWAY_INTERFACE" ]; }; then
    echo "Status: 401 Unauthorized"
    echo "Content-Type: text/plain; charset=utf-8"
    echo ""
    echo "Unauthorized"
    exit 0
fi

# CSRF / Same-Origin check on state-changing/active POST requests
if [ "$REQUEST_METHOD" = "POST" ] && [ -n "$HTTP_HOST" ]; then
    SRC_ORIGIN="${HTTP_ORIGIN:-$HTTP_REFERER}"
    if [ -n "$SRC_ORIGIN" ]; then
        SRC_HOST=$(printf '%s' "$SRC_ORIGIN" | sed -e 's#^https\?://##' -e 's#/.*$##')
        if [ -n "$SRC_HOST" ] && [ "$SRC_HOST" != "$HTTP_HOST" ]; then
            echo "Status: 403 Forbidden"
            echo "Content-Type: text/plain; charset=utf-8"
            echo ""
            echo "Forbidden: Cross-origin request rejected"
            exit 0
        fi
    fi
fi

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

send_err() {
    st="$1"
    msg="$2"
    echo "Status: $st"
    echo "Content-Type: text/plain; charset=utf-8"
    echo "Pragma: no-cache"
    echo "Cache-Control: no-store"
    echo ""
    echo "$msg"
    exit 0
}

QUERY="${QUERY_STRING}"
if [ "$REQUEST_METHOD" = "POST" ] && [ -n "$CONTENT_LENGTH" ]; then
    case "$CONTENT_LENGTH" in
        *[!0-9]*)
            CONTENT_LENGTH=0
            ;;
    esac
    if [ "$CONTENT_LENGTH" -gt 256 ] 2>/dev/null; then
        send_err "413 Payload Too Large" "Payload too large"
    fi
    if [ "$CONTENT_LENGTH" -gt 0 ] 2>/dev/null; then
        POST_DATA=$(dd bs=1 count="$CONTENT_LENGTH" 2>/dev/null)
        [ -z "$QUERY" ] && QUERY="$POST_DATA" || QUERY="${QUERY}&${POST_DATA}"
    fi
fi

TOOL=""
TARGET=""
OLD_IFS="$IFS"
IFS='&'
for kv in $QUERY; do
    key="${kv%%=*}"
    val="${kv#*=}"
    [ "$key" = "$kv" ] && val=""
    val=$(url_decode "$val" | tr -d '\r\n')
    case "$key" in
        tool)
            TOOL="$val"
            ;;
        target)
            TARGET="$val"
            ;;
    esac
done
IFS="$OLD_IFS"

if [ -n "$REQUEST_METHOD" ] && [ "$REQUEST_METHOD" != "GET" ] && [ "$REQUEST_METHOD" != "POST" ]; then
    send_err "405 Method Not Allowed" "Method Not Allowed"
fi

if [ "$REQUEST_METHOD" = "GET" ]; then
    case "$TOOL" in
        dmesg|syslog|arp|route|ps|wlan_stats|tc_stats|nat_stats)
            ;;
        *)
            send_err "405 Method Not Allowed" "Method Not Allowed (POST required for active probes)"
            ;;
    esac
fi

case "$TOOL" in
    ping|trace)
        if [ -z "$TARGET" ] || [ ${#TARGET} -gt 64 ]; then
            send_err "400 Bad Request" "Invalid target host or IPv4 address"
        fi
        case "$TARGET" in
            [A-Za-z0-9]*)
                ;;
            *)
                send_err "400 Bad Request" "Invalid target host or IPv4 address"
                ;;
        esac
        case "$TARGET" in
            *[!A-Za-z0-9._-]*)
                send_err "400 Bad Request" "Invalid characters in target"
                ;;
        esac
        ;;
    dmesg|syslog|arp|route|ps|wlan_stats|tc_stats|nat_stats)
        ;;
    *)
        send_err "400 Bad Request" "Unsupported diagnostic tool"
        ;;
esac

echo "Content-Type: text/plain; charset=utf-8"
echo "Pragma: no-cache"
echo "Cache-Control: no-store"
echo ""

case "$TOOL" in
    ping)
        ping -c 3 "$TARGET" 2>&1 || true
        ;;
    trace)
        traceroute -n -m 15 -w 2 "$TARGET" 2>&1 || true
        ;;
    arp)
        arp -n 2>&1 || cat /proc/net/arp 2>&1 || true
        ;;
    route)
        route -n 2>&1 || true
        ;;
    ps)
        if command -v top >/dev/null 2>&1; then
            top -b -n 1 2>&1 | head -n 25
        else
            ps 2>&1
        fi
        ;;
    dmesg)
        dmesg 2>&1 | tail -n 80
        ;;
    syslog)
        { logread 2>/dev/null || busybox logread 2>&1; } | tail -n 80
        ;;
    wlan_stats)
        iwconfig wlan0 2>&1 || true
        if [ -r /proc/net/mtlk/wlan0/Debug/General ]; then
            echo "--- /proc/net/mtlk/wlan0/Debug/General ---"
            cat /proc/net/mtlk/wlan0/Debug/General 2>&1 || true
        fi
        if [ -r /proc/net/mtlk/wlan0/Debug/MACStats ]; then
            echo "--- /proc/net/mtlk/wlan0/Debug/MACStats ---"
            head -n 40 /proc/net/mtlk/wlan0/Debug/MACStats 2>&1 || true
        fi
        ;;
    tc_stats)
        echo "--- Bridge (brctl show) ---"
        brctl show 2>&1 || true
        echo "--- Interfaces (ip addr) ---"
        ip -oneline -4 addr show 2>/dev/null || ifconfig 2>&1 || true
        echo "--- Traffic Control (wlan0) ---"
        tc -s qdisc show dev wlan0 2>&1 || true
        echo "--- Traffic Control (eth0) ---"
        tc -s qdisc show dev eth0 2>&1 || true
        ;;
    nat_stats)
        if [ -x /sbin/mini-nat ]; then
            /sbin/mini-nat --status 2>&1 || true
        else
            echo "mini-nat binary unavailable"
        fi
        if [ -r /proc/net/ip_conntrack ]; then
            echo "--- Active /proc/net/ip_conntrack (first 40) ---"
            head -n 40 /proc/net/ip_conntrack 2>&1 || true
        else
            echo "/proc/net/ip_conntrack unavailable (netfilter inactive)"
        fi
        ;;
esac
