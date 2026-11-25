#!/bin/sh
# Real-time Remote Access (SSH / Telnet / SSH-J Tunnel) Manager for Cisco WAP610N Web GUI

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

get_conf_val() {
    key="$1"
    for conf in /mnt/jffs2/sys.conf /tmp/sys.conf; do
        if [ -f "$conf" ]; then
            val=$(awk -v k="$key" '
                $0 ~ "^[ \t]*" k "[ \t]*=" {
                    idx = index($0, "=")
                    v = substr($0, idx + 1)
                    sub(/^[ \t]+/, "", v)
                    sub(/[ \t\r]+$/, "", v)
                    print v
                }
            ' "$conf" 2>/dev/null | tail -n 1)
            if [ -n "$val" ]; then
                printf '%s' "$val"
                return 0
            fi
        fi
    done
    return 1
}

is_pidfile_alive() {
    pf="$1"
    if [ -f "$pf" ]; then
        pid=$(cat "$pf" 2>/dev/null)
        case "$pid" in
            ""|*[!0-9]*)
                return 1
                ;;
            *)
                kill -0 "$pid" 2>/dev/null && return 0
                ;;
        esac
    fi
    return 1
}

is_valid_ident() {
    case "$1" in
        ""|*[!A-Za-z0-9_-]*)
            return 1
            ;;
        *)
            return 0
            ;;
    esac
}

json_error() {
    echo "Content-Type: application/json"
    echo "Pragma: no-cache"
    echo "Cache-Control: no-cache"
    echo ""
    echo "{\"status\":\"error\",\"message\":\"$1\"}"
    exit 0
}

QUERY="${QUERY_STRING}"
if [ -z "$QUERY" ] && [ -n "$CONTENT_LENGTH" ]; then
    case "$CONTENT_LENGTH" in
        *[!0-9]*)
            CONTENT_LENGTH=0
            ;;
    esac
    if [ "$CONTENT_LENGTH" -gt 512 ] 2>/dev/null; then
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
            SSHJ_EN=$(get_conf_val "SSHJ_Enabled")
            [ -z "$SSHJ_EN" ] && SSHJ_EN=0
            SSHJ_USER=$(get_conf_val "SSHJ_User")
            is_valid_ident "$SSHJ_USER" || SSHJ_USER=""
            SSHJ_HOST=$(get_conf_val "SSHJ_Host")
            is_valid_ident "$SSHJ_HOST" || SSHJ_HOST="wap610n"
            SSHJ_RUNNING=0
            is_pidfile_alive /tmp/sshj_ssh.pid && SSHJ_RUNNING=1
            SSHJ_WATCHDOG=0
            is_pidfile_alive /tmp/sshj_tunnel.pid && SSHJ_WATCHDOG=1
            echo "Content-Type: application/json"
            echo "Pragma: no-cache"
            echo "Cache-Control: no-cache"
            echo ""
            echo "{\"status\":\"success\",\"ssh\":$CURRENT_SSH,\"telnet\":$CURRENT_TELNET,\"sshj\":$SSHJ_EN,\"sshj_running\":$SSHJ_RUNNING,\"sshj_watchdog\":$SSHJ_WATCHDOG,\"sshj_user\":\"$SSHJ_USER\",\"sshj_host\":\"$SSHJ_HOST\"}"
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

# Require at least one supported parameter before changing state
case "$QUERY" in
    *ssh=*|*telnet=*|*sshj=*)
        ;;
    *)
        json_error "Missing parameters"
        ;;
esac

# Initialize from current live daemon / config state so toggling one preserves others
SSH_VAL=0
pidof dropbear >/dev/null 2>&1 && SSH_VAL=1
TELNET_VAL=0
pidof telnetd >/dev/null 2>&1 && TELNET_VAL=1
SSHJ_VAL=$(get_conf_val "SSHJ_Enabled")
[ -z "$SSHJ_VAL" ] && SSHJ_VAL=0
SSHJ_USER=$(get_conf_val "SSHJ_User")
is_valid_ident "$SSHJ_USER" || SSHJ_USER=""
SSHJ_HOST=$(get_conf_val "SSHJ_Host")
is_valid_ident "$SSHJ_HOST" || SSHJ_HOST="wap610n"

OLD_IFS="$IFS"
IFS='&'
for kv in $QUERY; do
    key="${kv%%=*}"
    val="${kv#*=}"
    val=$(printf '%s' "$val" | tr -d '\r\n')
    case "$key" in
        ssh)
            [ "$val" = "0" ] && SSH_VAL=0
            [ "$val" = "1" ] && SSH_VAL=1
            ;;
        telnet)
            [ "$val" = "0" ] && TELNET_VAL=0
            [ "$val" = "1" ] && TELNET_VAL=1
            ;;
        sshj)
            [ "$val" = "0" ] && SSHJ_VAL=0
            [ "$val" = "1" ] && SSHJ_VAL=1
            ;;
        sshj_user)
            SSHJ_USER="$val"
            ;;
        sshj_host)
            SSHJ_HOST="$val"
            ;;
    esac
done
IFS="$OLD_IFS"

if [ -n "$SSHJ_USER" ]; then
    if ! is_valid_ident "$SSHJ_USER" || [ ${#SSHJ_USER} -gt 48 ]; then
        json_error "Invalid characters in SSH-J username"
    fi
fi
[ -z "$SSHJ_HOST" ] && SSHJ_HOST="wap610n"
if ! is_valid_ident "$SSHJ_HOST" || [ ${#SSHJ_HOST} -gt 32 ]; then
    json_error "Invalid characters in SSH-J device name"
fi
if [ "$SSHJ_VAL" = "1" ] && [ -z "$SSHJ_USER" ]; then
    json_error "SSH-J username is required"
fi

# Persist to /mnt/jffs2/sys.conf under config lock
LOCKED=0
if [ -x /bin/config_lock.sh ] && /bin/config_lock.sh $$ >/dev/null 2>&1; then
    LOCKED=1
fi
mkdir -p /mnt/jffs2 2>/dev/null || true
update_conf() {
    target="$1"
    sed -i \
        -e '/^[[:space:]]*SSH_Enabled[[:space:]]*=/d' \
        -e '/^[[:space:]]*Telnet_Enabled[[:space:]]*=/d' \
        -e '/^[[:space:]]*SSHJ_Enabled[[:space:]]*=/d' \
        -e '/^[[:space:]]*SSHJ_User[[:space:]]*=/d' \
        -e '/^[[:space:]]*SSHJ_Host[[:space:]]*=/d' \
        "$target" 2>/dev/null || true
    {
        echo "SSH_Enabled = $SSH_VAL"
        echo "Telnet_Enabled = $TELNET_VAL"
        echo "SSHJ_Enabled = $SSHJ_VAL"
        echo "SSHJ_User = $SSHJ_USER"
        echo "SSHJ_Host = $SSHJ_HOST"
    } >> "$target" 2>/dev/null || true
}
if [ -d /mnt/jffs2 ]; then
    touch /mnt/jffs2/sys.conf 2>/dev/null || true
    update_conf /mnt/jffs2/sys.conf
    cp -af /mnt/jffs2/sys.conf /tmp/sys.conf 2>/dev/null || true
elif [ -f /tmp/sys.conf ]; then
    update_conf /tmp/sys.conf
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
    if [ ! -f "/etc/dropbear/dropbear_ed25519_host_key" ] && [ -f "/mnt/jffs2/dropbear/dropbear_ed25519_host_key" ]; then
        cp -f "/mnt/jffs2/dropbear/dropbear_ed25519_host_key" "/etc/dropbear/dropbear_ed25519_host_key" 2>/dev/null || true
    fi
    if [ ! -f "/etc/dropbear/dropbear_ed25519_host_key" ] && [ -x /usr/bin/dropbearkey ]; then
        /usr/bin/dropbearkey -t ed25519 -f "/etc/dropbear/dropbear_ed25519_host_key" >/dev/null 2>&1 || true
    fi
    if [ ! -f "/etc/dropbear/dropbear_rsa_host_key" ] && [ -f "/mnt/jffs2/dropbear/dropbear_rsa_host_key" ]; then
        cp -f "/mnt/jffs2/dropbear/dropbear_rsa_host_key" "/etc/dropbear/dropbear_rsa_host_key" 2>/dev/null || true
    fi
    chmod 600 /etc/dropbear/*_host_key 2>/dev/null || true
    if ! pidof dropbear >/dev/null 2>&1; then
        /usr/sbin/dropbear -R -p 22 </dev/null >/dev/null 2>&1 &
    fi
fi

if [ "$TELNET_VAL" = "0" ]; then
    killall telnetd 2>/dev/null || true
else
    if ! pidof telnetd >/dev/null 2>&1; then
        telnetd -l /bin/sh </dev/null >/dev/null 2>&1 &
    fi
fi

if [ -x /bin/sshj_tunnel.sh ]; then
    if [ "$SSHJ_VAL" = "1" ]; then
        /bin/sshj_tunnel.sh restart </dev/null >/dev/null 2>&1 &
    else
        /bin/sshj_tunnel.sh stop </dev/null >/dev/null 2>&1 || true
    fi
fi

# Persist changes to physical flash in background without blocking HTTP response
if [ -x /bin/config_save.sh ]; then
    /bin/config_save.sh </dev/null >/dev/null 2>&1 &
fi

SSHJ_RUNNING=0
is_pidfile_alive /tmp/sshj_ssh.pid && SSHJ_RUNNING=1
SSHJ_WATCHDOG=0
is_pidfile_alive /tmp/sshj_tunnel.pid && SSHJ_WATCHDOG=1

# apps/WebServer/cgi.c:318 already writes "HTTP/1.0 200 OK\r\n"
echo "Content-Type: application/json"
echo "Pragma: no-cache"
echo "Cache-Control: no-cache"
echo ""
echo "{\"status\":\"success\",\"ssh\":$SSH_VAL,\"telnet\":$TELNET_VAL,\"sshj\":$SSHJ_VAL,\"sshj_running\":$SSHJ_RUNNING,\"sshj_watchdog\":$SSHJ_WATCHDOG,\"sshj_user\":\"$SSHJ_USER\",\"sshj_host\":\"$SSHJ_HOST\"}"
