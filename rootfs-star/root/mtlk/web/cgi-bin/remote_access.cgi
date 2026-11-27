#!/bin/sh
# Real-time Remote Access (SSH / Telnet / SSH-J Tunnel) Manager for Cisco WAP610N Web GUI
set -f

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
    if [ -f /tmp/sys.conf ] && [ -f /mnt/jffs2/sys.conf ]; then
        if [ /tmp/sys.conf -nt /mnt/jffs2/sys.conf ]; then
            conf_order="/tmp/sys.conf /mnt/jffs2/sys.conf"
        else
            conf_order="/mnt/jffs2/sys.conf /tmp/sys.conf"
        fi
    else
        conf_order="/mnt/jffs2/sys.conf /tmp/sys.conf /etc/sys.conf /root/mtlk/etc/sys.conf"
    fi
    for conf in $conf_order; do
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

is_tcp_listening() {
    hex_port="$1"
    awk -v p="$hex_port" 'NR>1 && $4 == "0A" && $2 ~ ":" p "$" { found=1; exit } END { exit !found }' /proc/net/tcp 2>/dev/null
}

is_ssh_established() {
    awk 'NR>1 && $4 == "01" && $3 ~ /:0016$/ { found=1; exit } END { exit !found }' /proc/net/tcp 2>/dev/null
}

probe_live_state() {
    CURRENT_SSH=0
    if is_tcp_listening 0016 || pidof dropbear >/dev/null 2>&1; then
        CURRENT_SSH=1
    fi
    CURRENT_TELNET=0
    if is_tcp_listening 0017 || pidof telnetd >/dev/null 2>&1; then
        CURRENT_TELNET=1
    fi
    CURRENT_UPNP=0
    if pidof upnpd >/dev/null 2>&1; then
        CURRENT_UPNP=1
    fi
    SSHJ_CONF_EN=$(get_conf_val "SSHJ_Enabled")
    [ -z "$SSHJ_CONF_EN" ] && SSHJ_CONF_EN=0
    SSHJ_USER=$(get_conf_val "SSHJ_User")
    is_valid_ident "$SSHJ_USER" || SSHJ_USER=""
    SSHJ_HOST=$(get_conf_val "SSHJ_Host")
    is_valid_ident "$SSHJ_HOST" || SSHJ_HOST="wap610n"

    SSHJ_WATCHDOG=0
    if is_pidfile_alive /tmp/sshj_tunnel.pid || ps w 2>/dev/null | grep -q '[s]shj_tunnel\.sh'; then
        SSHJ_WATCHDOG=1
    fi
    SSHJ_PROC=0
    if is_pidfile_alive /tmp/sshj_ssh.pid || ps w 2>/dev/null | grep -qE '[ /](ssh|dbclient) .*ssh-j\.com'; then
        SSHJ_PROC=1
    fi
    SSHJ_RUNNING=0
    if [ "$SSHJ_PROC" = "1" ] && is_ssh_established; then
        SSHJ_RUNNING=1
    fi
    SSHJ_ACTIVE=0
    if [ "$SSHJ_CONF_EN" = "1" ] || [ "$SSHJ_RUNNING" = "1" ] || [ "$SSHJ_WATCHDOG" = "1" ] || [ "$SSHJ_PROC" = "1" ]; then
        SSHJ_ACTIVE=1
    fi
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

json_error() {
    echo "Content-Type: application/json"
    echo "Pragma: no-cache"
    echo "Cache-Control: no-cache"
    echo ""
    echo "{\"status\":\"error\",\"message\":\"$1\"}"
    exit 0
}

QUERY="${QUERY_STRING}"
if [ "$REQUEST_METHOD" = "POST" ] && [ -n "$CONTENT_LENGTH" ]; then
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
        [ -z "$QUERY" ] && QUERY="$POST_DATA" || QUERY="${QUERY}&${POST_DATA}"
    fi
fi

# CSRF Protection: GET is strictly read-only status query; state-changing actions require POST
if [ "$REQUEST_METHOD" = "GET" ]; then
    case "$QUERY" in
        ""|action=status|action=status\&*|status|status\&*)
            probe_live_state
            echo "Content-Type: application/json"
            echo "Pragma: no-cache"
            echo "Cache-Control: no-cache"
            echo ""
            echo "{\"status\":\"success\",\"ssh\":$CURRENT_SSH,\"telnet\":$CURRENT_TELNET,\"upnp\":$CURRENT_UPNP,\"sshj\":$SSHJ_ACTIVE,\"sshj_running\":$SSHJ_RUNNING,\"sshj_watchdog\":$SSHJ_WATCHDOG,\"sshj_user\":\"$SSHJ_USER\",\"sshj_host\":\"$SSHJ_HOST\"}"
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
    *ssh=*|*telnet=*|*upnp=*|*sshj=*)
        ;;
    *)
        json_error "Missing parameters"
        ;;
esac

# Initialize from persisted config state (avoid transient boot race on pidof)
SSH_VAL=$(get_conf_val "SSH_Enabled")
[ -z "$SSH_VAL" ] && SSH_VAL=1
[ "$SSH_VAL" != "0" ] && SSH_VAL=1 || SSH_VAL=0
TELNET_VAL=$(get_conf_val "Telnet_Enabled")
[ -z "$TELNET_VAL" ] && TELNET_VAL=1
[ "$TELNET_VAL" != "0" ] && TELNET_VAL=1 || TELNET_VAL=0
UPNP_VAL=$(get_conf_val "UPnP_Enabled")
[ -z "$UPNP_VAL" ] && { pidof upnpd >/dev/null 2>&1 && UPNP_VAL=1 || UPNP_VAL=0; }
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
    [ "$key" = "$kv" ] && val=""
    val=$(url_decode "$val" | tr -d '\r\n')
    case "$key" in
        ssh)
            [ "$val" = "0" ] && SSH_VAL=0
            [ "$val" = "1" ] && SSH_VAL=1
            ;;
        telnet)
            [ "$val" = "0" ] && TELNET_VAL=0
            [ "$val" = "1" ] && TELNET_VAL=1
            ;;
        upnp)
            [ "$val" = "0" ] && UPNP_VAL=0
            [ "$val" = "1" ] && UPNP_VAL=1
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
if [ "$SSHJ_VAL" = "1" ] && [ "$SSH_VAL" = "0" ]; then
    json_error "SSH must be enabled to use SSH-J tunnel"
fi

# Persist to /mnt/jffs2/sys.conf under config lock
LOCKED=0
if [ -x /bin/config_lock.sh ] && /bin/config_lock.sh $$ >/dev/null 2>&1; then
    LOCKED=1
fi
mkdir -p /mnt/jffs2 2>/dev/null || true
update_conf() {
    target="$1"
    tmp_target="${target}.tmp.$$"
    if [ -f "$target" ]; then
        awk '!/^[ \t]*(SSH_Enabled|Telnet_Enabled|UPnP_Enabled|SSHJ_Enabled|SSHJ_User|SSHJ_Host)[ \t]*=/' "$target" > "$tmp_target" 2>/dev/null || : > "$tmp_target"
    else
        : > "$tmp_target"
    fi
    {
        echo "SSH_Enabled = $SSH_VAL"
        echo "Telnet_Enabled = $TELNET_VAL"
        echo "UPnP_Enabled = $UPNP_VAL"
        echo "SSHJ_Enabled = $SSHJ_VAL"
        echo "SSHJ_User = $SSHJ_USER"
        echo "SSHJ_Host = $SSHJ_HOST"
    } >> "$tmp_target" 2>/dev/null || true
    chmod 600 "$tmp_target" 2>/dev/null || true
    mv -f "$tmp_target" "$target" 2>/dev/null || true
}
if [ -d /mnt/jffs2 ]; then
    if [ ! -s /mnt/jffs2/sys.conf ]; then
        if [ -s /tmp/sys.conf ]; then
            cp -af /tmp/sys.conf /mnt/jffs2/sys.conf 2>/dev/null || true
        elif [ -s /root/mtlk/etc/sys.conf ]; then
            cp -af /root/mtlk/etc/sys.conf /mnt/jffs2/sys.conf 2>/dev/null || true
        else
            touch /mnt/jffs2/sys.conf 2>/dev/null || true
        fi
    fi
    update_conf /mnt/jffs2/sys.conf
    if grep -qE '^[[:space:]]*ip_lan[[:space:]]*=' /mnt/jffs2/sys.conf 2>/dev/null; then
        cp -af /mnt/jffs2/sys.conf /tmp/sys.conf 2>/dev/null || true
    elif [ -f /tmp/sys.conf ]; then
        update_conf /tmp/sys.conf
    fi
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
        telnetd -l /bin/login </dev/null >/dev/null 2>&1 &
    fi
fi

if [ "$UPNP_VAL" = "0" ]; then
    pkill -f '[m]tlk_upnpd_monitor\.sh' 2>/dev/null || true
    killall -SIGINT upnpd 2>/dev/null || true
    if pidof upnpd >/dev/null 2>&1; then
        sleep 1
        killall -9 upnpd 2>/dev/null || true
    fi
else
    if ! pidof upnpd >/dev/null 2>&1 && [ -x /bin/upnpd ]; then
        nice -n 18 /bin/upnpd br0 </dev/null >/dev/null 2>&1 &
    fi
    if ! pgrep -f '[m]tlk_upnpd_monitor\.sh' >/dev/null 2>&1 && [ -x /root/mtlk/etc/mtlk_upnpd_monitor.sh ]; then
        ( cd /root/mtlk/etc && nice -n 18 ./mtlk_upnpd_monitor.sh ) </dev/null >/dev/null 2>&1 &
    fi
fi

if [ -x /bin/sshj_tunnel.sh ]; then
    if [ "$SSHJ_VAL" = "1" ]; then
        /bin/sshj_tunnel.sh restart </dev/null >/dev/null 2>&1 &
    else
        /bin/sshj_tunnel.sh stop </dev/null >/dev/null 2>&1 || true
    fi
fi

# Notify GoAhead webs to reload config cache and persist flash in background
killall -HUP webs 2>/dev/null || true
if [ -x /bin/config_save.sh ]; then
    /bin/config_save.sh </dev/null >/dev/null 2>&1 &
fi

probe_live_state
if [ "$SSHJ_VAL" = "1" ] && [ "$SSHJ_ACTIVE" = "0" ]; then
    SSHJ_ACTIVE=1
fi

# apps/WebServer/cgi.c:318 already writes "HTTP/1.0 200 OK\r\n"
echo "Content-Type: application/json"
echo "Pragma: no-cache"
echo "Cache-Control: no-cache"
echo ""
echo "{\"status\":\"success\",\"ssh\":$CURRENT_SSH,\"telnet\":$CURRENT_TELNET,\"upnp\":$CURRENT_UPNP,\"sshj\":$SSHJ_ACTIVE,\"sshj_running\":$SSHJ_RUNNING,\"sshj_watchdog\":$SSHJ_WATCHDOG,\"sshj_user\":\"$SSHJ_USER\",\"sshj_host\":\"$SSHJ_HOST\"}"
