#!/bin/sh
# Real-time Remote Access (SSH / Telnet) Manager for Cisco WAP610N Web GUI

# Retrieve admin password from system configuration
ADMIN_PASS=""
for conf in /tmp/sys.conf /etc/sys.conf /root/mtlk/etc/sys.conf; do
    if [ -f "$conf" ] && [ -z "$ADMIN_PASS" ]; then
        ADMIN_PASS=$(awk -F "=" '/^(Admin)?Password/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' "$conf" 2>/dev/null | tail -n 1)
    fi
done

# Authenticate request:
# 1. If HTTP_AUTHORIZATION is present (e.g. direct CLI/curl invocation), validate credentials.
# 2. If executed via GoAhead WebServer (cgi.c strips HTTP_AUTHORIZATION after websSecurityHandler
#    authenticates the session), allow the request when SERVER_SOFTWARE or GATEWAY_INTERFACE is set.
# 3. Otherwise, reject as unauthorized.
if [ -n "$HTTP_AUTHORIZATION" ]; then
    AUTH_IN=$(echo "$HTTP_AUTHORIZATION" | sed 's/^[Bb]asic //')
    EXPECTED_AUTH=$(printf "%s" "admin:$ADMIN_PASS" | base64 2>/dev/null || true)
    if [ -z "$EXPECTED_AUTH" ] && command -v openssl >/dev/null 2>&1; then
        EXPECTED_AUTH=$(printf "%s" "admin:$ADMIN_PASS" | openssl enc -base64 2>/dev/null)
    fi
    if [ -n "$ADMIN_PASS" ] && [ -n "$EXPECTED_AUTH" ] && [ "$AUTH_IN" != "$EXPECTED_AUTH" ]; then
        echo "Content-Type: application/json"
        echo ""
        echo '{"status":"error","message":"Unauthorized"}'
        exit 0
    fi
elif [ -z "$SERVER_SOFTWARE" ] && [ -z "$GATEWAY_INTERFACE" ]; then
    echo "Content-Type: application/json"
    echo ""
    echo '{"status":"error","message":"Unauthorized"}'
    exit 0
fi

QUERY="${QUERY_STRING}"
if [ -z "$QUERY" ] && [ -n "$CONTENT_LENGTH" ] && [ "$CONTENT_LENGTH" -gt 0 ] 2>/dev/null; then
    POST_DATA=$(dd bs=1 count="$CONTENT_LENGTH" 2>/dev/null)
    QUERY="$POST_DATA"
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

# Default values
SSH_VAL=1
TELNET_VAL=1

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

# Persist to /mnt/jffs2/sys.conf
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

# Dynamic process management
if [ "$SSH_VAL" = "0" ]; then
    killall dropbear 2>/dev/null || true
else
    mkdir -p /etc/dropbear 2>/dev/null || true
    chmod 700 /etc/dropbear 2>/dev/null || true
    if [ ! -f /etc/dropbear/dropbear_rsa_host_key ]; then
        /usr/bin/dropbearkey -t rsa -s 1024 -f /etc/dropbear/dropbear_rsa_host_key 2>/dev/null || true
    fi
    if [ ! -f /etc/dropbear/dropbear_dss_host_key ]; then
        /usr/bin/dropbearkey -t dss -s 1024 -f /etc/dropbear/dropbear_dss_host_key 2>/dev/null || true
    fi
    if ! pidof dropbear >/dev/null 2>&1; then
        /usr/sbin/dropbear -p 22 2>/dev/null &
    fi
fi

if [ "$TELNET_VAL" = "0" ]; then
    killall telnetd 2>/dev/null || true
else
    if ! pidof telnetd >/dev/null 2>&1; then
        telnetd -l /bin/sh 2>/dev/null &
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
