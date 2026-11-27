#!/bin/sh
# Dynamic DNS updater for WAP610N (DuckDNS, No-IP, DynDNS, Custom URL)

CONF_FILE="/mnt/jffs2/ddns.conf"
STATUS_FILE="/tmp/ddns.status"
PID_FILE="/tmp/ddns_updater.pid"

read_conf() {
    ENABLED="0"
    PROVIDER="duckdns"
    HOSTNAME=""
    USERNAME=""
    PASSWORD=""
    TOKEN=""
    CUSTOM_URL=""
    INTERVAL="600"
    [ -f "$CONF_FILE" ] || return 1
    while IFS='=' read -r k v; do
        v=$(printf '%s' "$v" | tr -d '\r\n')
        case "$k" in
            ENABLED) ENABLED="$v" ;;
            PROVIDER) PROVIDER="$v" ;;
            HOSTNAME) HOSTNAME="$v" ;;
            USERNAME) USERNAME="$v" ;;
            PASSWORD) PASSWORD="$v" ;;
            TOKEN) TOKEN="$v" ;;
            CUSTOM_URL) CUSTOM_URL="$v" ;;
            INTERVAL) INTERVAL="$v" ;;
        esac
    done < "$CONF_FILE"
    case "$INTERVAL" in
        ''|*[!0-9]*) INTERVAL=600 ;;
    esac
    [ "$INTERVAL" -lt 300 ] && INTERVAL=300
    [ "$INTERVAL" -gt 86400 ] && INTERVAL=86400
    return 0
}

is_safe_token() {
    case "$1" in
        ''|*[!A-Za-z0-9._:@/-]*) return 1 ;;
        *) return 0 ;;
    esac
}

run_once() {
    if ! read_conf; then
        echo "$(date '+%Y-%m-%d %H:%M:%S' 2>/dev/null) | unconfigured" > "$STATUS_FILE"
        return 1
    fi
    if [ "$ENABLED" != "1" ] && [ "$1" != "--force" ]; then
        echo "$(date '+%Y-%m-%d %H:%M:%S' 2>/dev/null) | disabled" > "$STATUS_FILE"
        return 0
    fi

    url=""
    auth=""
    case "$PROVIDER" in
        duckdns)
            if ! is_safe_token "$HOSTNAME" || ! is_safe_token "$TOKEN"; then
                echo "$(date '+%Y-%m-%d %H:%M:%S' 2>/dev/null) | duckdns | invalid host/token" > "$STATUS_FILE"
                return 1
            fi
            url="https://www.duckdns.org/update?domains=${HOSTNAME}&token=${TOKEN}&ip="
            ;;
        noip)
            if ! is_safe_token "$HOSTNAME" || ! is_safe_token "$USERNAME" || [ -z "$PASSWORD" ]; then
                echo "$(date '+%Y-%m-%d %H:%M:%S' 2>/dev/null) | noip | invalid credentials" > "$STATUS_FILE"
                return 1
            fi
            url="http://dynupdate.no-ip.com/nic/update?hostname=${HOSTNAME}"
            auth="${USERNAME}:${PASSWORD}"
            ;;
        dyndns)
            if ! is_safe_token "$HOSTNAME" || ! is_safe_token "$USERNAME" || [ -z "$PASSWORD" ]; then
                echo "$(date '+%Y-%m-%d %H:%M:%S' 2>/dev/null) | dyndns | invalid credentials" > "$STATUS_FILE"
                return 1
            fi
            url="http://members.dyndns.org/nic/update?hostname=${HOSTNAME}"
            auth="${USERNAME}:${PASSWORD}"
            ;;
        custom)
            case "$CUSTOM_URL" in
                http://*|https://*)
                    case "$CUSTOM_URL" in
                        *[!A-Za-z0-9._:/?\&=%~+-]*)
                            echo "$(date '+%Y-%m-%d %H:%M:%S' 2>/dev/null) | custom | invalid URL chars" > "$STATUS_FILE"
                            return 1
                            ;;
                    esac
                    url="$CUSTOM_URL"
                    ;;
                *)
                    echo "$(date '+%Y-%m-%d %H:%M:%S' 2>/dev/null) | custom | invalid URL scheme" > "$STATUS_FILE"
                    return 1
                    ;;
            esac
            ;;
        *)
            echo "$(date '+%Y-%m-%d %H:%M:%S' 2>/dev/null) | unknown provider" > "$STATUS_FILE"
            return 1
            ;;
    esac

    resp=""
    if [ -x /bin/curl ]; then
        if [ -n "$auth" ]; then
            resp=$(/bin/curl -s -L -m 10 -u "$auth" "$url" 2>/dev/null)
        else
            resp=$(/bin/curl -s -L -m 10 "$url" 2>/dev/null)
        fi
    fi
    if [ -z "$resp" ] && command -v wget >/dev/null 2>&1; then
        if [ "$PROVIDER" = "duckdns" ]; then
            resp=$(wget -q -O - "http://www.duckdns.org/update?domains=${HOSTNAME}&token=${TOKEN}&ip=" 2>/dev/null)
        elif [ -n "$auth" ] && [ "${url#http://}" != "$url" ]; then
            resp=$(wget -q -O - "http://${USERNAME}:${PASSWORD}@${url#http://}" 2>/dev/null)
        else
            resp=$(wget -q -O - "$url" 2>/dev/null)
        fi
    fi

    clean_resp=$(printf '%s' "$resp" | tr '\r\n' '  ' | cut -c 1-80)
    [ -z "$clean_resp" ] && clean_resp="no response (offline or TLS unavailable)"
    echo "$(date '+%Y-%m-%d %H:%M:%S' 2>/dev/null) | ${PROVIDER} | ${clean_resp}" > "$STATUS_FILE"
    return 0
}

if [ "$1" = "--once" ]; then
    run_once --force
    exit $?
fi

SLEEP_PID=""
echo $$ > "$PID_FILE"
trap '[ -n "$SLEEP_PID" ] && kill "$SLEEP_PID" 2>/dev/null || true; rm -f "$PID_FILE" 2>/dev/null || true; exit 0' 0 1 2 15

# Wait briefly after boot for bridge/DHCP readiness
up_sec=$(awk '{printf "%d", $1}' /proc/uptime 2>/dev/null || echo 100)
if [ "$up_sec" -lt 25 ] 2>/dev/null; then
    sleep 15
fi

while :; do
    read_conf || break
    [ "$ENABLED" = "1" ] || break
    run_once
    sleep "$INTERVAL" &
    SLEEP_PID=$!
    wait "$SLEEP_PID" 2>/dev/null || true
    SLEEP_PID=""
done

rm -f "$PID_FILE" 2>/dev/null || true
exit 0
