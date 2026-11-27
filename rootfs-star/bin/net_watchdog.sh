#!/bin/sh
# Network Ping Watchdog for WAP610N (boot-loop guarded, BusyBox 1.8.1 safe)

CONF_FILE="/mnt/jffs2/watchdog.conf"
LOG_FILE="/tmp/watchdog.log"
PID_FILE="/tmp/net_watchdog.pid"

append_log() {
    line="$(date '+%Y-%m-%d %H:%M:%S' 2>/dev/null) $1"
    tmp_log="${LOG_FILE}.tmp.$$"
    if [ -f "$LOG_FILE" ]; then
        { cat "$LOG_FILE"; echo "$line"; } | tail -n 30 > "$tmp_log" 2>/dev/null || echo "$line" > "$tmp_log"
    else
        echo "$line" > "$tmp_log"
    fi
    mv -f "$tmp_log" "$LOG_FILE" 2>/dev/null || true
}

read_conf() {
    ENABLED="0"
    TARGET="1.1.1.1"
    INTERVAL="60"
    MAX_FAIL="5"
    ACTION="recover"
    [ -f "$CONF_FILE" ] || return 1
    while IFS='=' read -r k v; do
        v=$(printf '%s' "$v" | tr -d '\r\n')
        case "$k" in
            ENABLED) ENABLED="$v" ;;
            TARGET) TARGET="$v" ;;
            INTERVAL) INTERVAL="$v" ;;
            MAX_FAIL) MAX_FAIL="$v" ;;
            ACTION) ACTION="$v" ;;
        esac
    done < "$CONF_FILE"
    case "$INTERVAL" in
        ''|*[!0-9]*) INTERVAL=60 ;;
    esac
    [ "$INTERVAL" -lt 30 ] && INTERVAL=30
    [ "$INTERVAL" -gt 3600 ] && INTERVAL=3600
    case "$MAX_FAIL" in
        ''|*[!0-9]*) MAX_FAIL=5 ;;
    esac
    [ "$MAX_FAIL" -lt 2 ] && MAX_FAIL=2
    [ "$MAX_FAIL" -gt 20 ] && MAX_FAIL=20
    [ "$ACTION" != "reboot" ] && ACTION="recover"
    case "$TARGET" in
        ''|*[!A-Za-z0-9._-]*) return 1 ;;
    esac
    return 0
}

echo $$ > "$PID_FILE"
trap 'rm -f "$PID_FILE" 2>/dev/null || true; exit 0' 0 1 2 15

# Boot grace period: never probe until uptime >= 90s to avoid false failure / boot loops
while :; do
    up_sec=$(awk '{printf "%d", $1}' /proc/uptime 2>/dev/null || echo 100)
    [ "$up_sec" -ge 90 ] 2>/dev/null && break
    sleep 15
done

fail_count=0
was_down=0
append_log "Watchdog started"

while :; do
    if ! read_conf || [ "$ENABLED" != "1" ]; then
        append_log "Watchdog stopped (disabled or invalid config)"
        break
    fi

    # BusyBox 1.8.1 ping does not support -W; -c 1 keeps probes lightweight
    if ping -c 1 "$TARGET" >/dev/null 2>&1; then
        if [ "$was_down" = "1" ]; then
            append_log "Target $TARGET recovered"
            was_down=0
        fi
        fail_count=0
    else
        fail_count=$((fail_count + 1))
        append_log "Probe $TARGET failed ($fail_count/$MAX_FAIL)"
        if [ "$fail_count" -ge "$MAX_FAIL" ]; then
            was_down=1
            if [ "$ACTION" = "reboot" ]; then
                up_sec=$(awk '{printf "%d", $1}' /proc/uptime 2>/dev/null || echo 0)
                if [ "$up_sec" -ge 180 ] 2>/dev/null; then
                    append_log "Max failures reached ($fail_count); rebooting device"
                    sync
                    /sbin/reboot
                    exit 0
                else
                    append_log "Reboot suppressed (uptime ${up_sec}s < 180s)"
                fi
            else
                append_log "Max failures reached ($fail_count); recovering DHCP and wlan0"
                killall -USR1 udhcpc 2>/dev/null || true
                ifconfig wlan0 down 2>/dev/null || true
                sleep 2
                ifconfig wlan0 up 2>/dev/null || true
                fail_count=0
            fi
        fi
    fi

    sleep "$INTERVAL" &
    wait $! 2>/dev/null || sleep "$INTERVAL"
done

rm -f "$PID_FILE" 2>/dev/null || true
exit 0
