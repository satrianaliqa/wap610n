#!/bin/sh
# Real-time Remote Access (SSH / Telnet) Manager for Cisco WAP610N Web GUI

# Retrieve admin password from system configuration (/mnt/jffs2 first)
ADMIN_PASS=""
for conf in /mnt/jffs2/sys.conf /tmp/sys.conf /etc/sys.conf /root/mtlk/etc/sys.conf; do
    if [ -f "$conf" ] && [ -z "$ADMIN_PASS" ]; then
        ADMIN_PASS=$(awk '/^\^?(Admin)?Password[ \t]*=/ {idx = index($0, "="); val = substr($0, idx + 1); sub(/^[ \t]+/, "", val); sub(/[ \t\r]+$/, "", val); print val}' "$conf" 2>/dev/null | tail -n 1)
    fi
done

# Authenticate request fail-closed:
# 1. If HTTP_AUTHORIZATION is present, verify Base64 credentials using BusyBox uuencode.
# 2. Otherwise require GoAhead CGI execution context (REMOTE_USER or SERVER_SOFTWARE + GATEWAY_INTERFACE).
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

QUERY="${QUERY_STRING}"
if [ -z "$QUERY" ] && [ -n "$CONTENT_LENGTH" ]; then
    case "$CONTENT_LENGTH" in
        *[!0-9]*)
            CONTENT_LENGTH=0
            ;;
    esac
    if [ "$CONTENT_LENGTH" -gt 256 ] 2>/dev/null; then
        echo "Status: 413 Payload Too Large"
        echo "Content-Type: application/json"
        echo ""
        echo '{"status":"error","message":"Payload too large"}'
        exit 0
    fi
    if [ "$CONTENT_LENGTH" -gt 0 ] 2>/dev/null; then
        POST_DATA=$(dd bs=1 count="$CONTENT_LENGTH" 2>/dev/null)
        QUERY="$POST_DATA"
    fi
fi

# CSRF Protection: GET is strictly read-only status query; state-changing actions require POST
if [ "$REQUEST_METHOD" = "GET" ]; then
    case "$QUERY" in
        ""|action=status|status)
            CURRENT_SSH=0
            pidof dropbear >/dev/null 2>&1 && CURRENT_SSH=1
            CURRENT_TELNET=0
            pidof telnetd >/dev/null 2>&1 && CURRENT_TELNET=1
            echo "Content-Type: application/json"
            echo "Pragma: no-cache"
            echo "Cache-Control: no-cache"
            echo ""
            echo "{\"status\":\"success\",\"ssh\":$CURRENT_SSH,\"telnet\":$CURRENT_TELNET}"
            exit 0
            ;;
        *)
            echo "Status: 405 Method Not Allowed"
            echo "Content-Type: application/json"
            echo ""
            echo '{"status":"error","message":"Method Not Allowed"}'
            exit 0
            ;;
    esac
elif [ -n "$REQUEST_METHOD" ] && [ "$REQUEST_METHOD" != "POST" ]; then
    echo "Status: 405 Method Not Allowed"
    echo "Content-Type: application/json"
    echo ""
    echo '{"status":"error","message":"Method Not Allowed"}'
    exit 0
fi

# Require explicit ssh= or telnet= parameter before changing state
case "$QUERY" in
    *ssh=*|*telnet=*)
        ;;
    *)
        echo "Content-Type: application/json"
        echo ""
        echo '{"status":"error","message":"Missing parameters"}'
        exit 0
        ;;
esac

# Initialize from current live daemon state so toggling one service preserves the other
SSH_VAL=0
pidof dropbear >/dev/null 2>&1 && SSH_VAL=1
TELNET_VAL=0
pidof telnetd >/dev/null 2>&1 && TELNET_VAL=1

case "$QUERY" in
    *ssh=0*)
        SSH_VAL=0
        ;;
    *ssh=1*)
        SSH_VAL=1
        ;;
esac

case "$QUERY" in
    *telnet=0*)
        TELNET_VAL=0
        ;;
    *telnet=1*)
        TELNET_VAL=1
        ;;
esac

# Persist to /mnt/jffs2/sys.conf under config lock
LOCKED=0
if [ -x /bin/config_lock.sh ] && /bin/config_lock.sh $$ >/dev/null 2>&1; then
    LOCKED=1
fi
mkdir -p /mnt/jffs2 2>/dev/null || true
if [ -d /mnt/jffs2 ]; then
    touch /mnt/jffs2/sys.conf 2>/dev/null || true
    sed -i '/^SSH_Enabled/d' /mnt/jffs2/sys.conf 2>/dev/null || true
    echo "SSH_Enabled = $SSH_VAL" >> /mnt/jffs2/sys.conf 2>/dev/null || true
    sed -i '/^Telnet_Enabled/d' /mnt/jffs2/sys.conf 2>/dev/null || true
    echo "Telnet_Enabled = $TELNET_VAL" >> /mnt/jffs2/sys.conf 2>/dev/null || true
    # Synchronize to /tmp/sys.conf
    cp -af /mnt/jffs2/sys.conf /tmp/sys.conf 2>/dev/null || true
elif [ -f /tmp/sys.conf ]; then
    sed -i '/^SSH_Enabled/d' /tmp/sys.conf 2>/dev/null || true
    echo "SSH_Enabled = $SSH_VAL" >> /tmp/sys.conf 2>/dev/null || true
    sed -i '/^Telnet_Enabled/d' /tmp/sys.conf 2>/dev/null || true
    echo "Telnet_Enabled = $TELNET_VAL" >> /tmp/sys.conf 2>/dev/null || true
fi
if [ "$LOCKED" = "1" ] && [ -x /bin/config_unlock.sh ]; then
    /bin/config_unlock.sh $$ >/dev/null 2>&1 || true
fi

# Dynamic process management
if [ "$SSH_VAL" = "0" ]; then
    killall dropbear 2>/dev/null || true
else
    mkdir -p /etc/dropbear 2>/dev/null || true
    chmod 700 /etc/dropbear 2>/dev/null || true
    if [ ! -f /etc/dropbear/dropbear_rsa_host_key ]; then
        /usr/bin/dropbearkey -t rsa -s 1024 -f /etc/dropbear/dropbear_rsa_host_key >/dev/null 2>&1 || true
    fi
    if [ ! -f /etc/dropbear/dropbear_dss_host_key ]; then
        /usr/bin/dropbearkey -t dss -s 1024 -f /etc/dropbear/dropbear_dss_host_key >/dev/null 2>&1 || true
    fi
    chmod 600 /etc/dropbear/*_host_key 2>/dev/null || true
    if ! pidof dropbear >/dev/null 2>&1; then
        /usr/sbin/dropbear -p 22 </dev/null >/dev/null 2>&1 &
    fi
fi

if [ "$TELNET_VAL" = "0" ]; then
    killall telnetd 2>/dev/null || true
else
    if ! pidof telnetd >/dev/null 2>&1; then
        telnetd -l /bin/sh </dev/null >/dev/null 2>&1 &
    fi
fi

# Persist changes to physical flash in background without blocking HTTP response
if [ -x /bin/config_save.sh ]; then
    /bin/config_save.sh >/dev/null 2>&1 &
fi

# apps/WebServer/cgi.c:318 already writes "HTTP/1.0 200 OK\r\n"
echo "Content-Type: application/json"
echo "Pragma: no-cache"
echo "Cache-Control: no-cache"
echo ""
echo "{\"status\":\"success\",\"ssh\":$SSH_VAL,\"telnet\":$TELNET_VAL}"
