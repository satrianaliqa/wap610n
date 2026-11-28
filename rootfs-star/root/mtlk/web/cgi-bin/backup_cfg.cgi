#!/bin/sh
# Full ConfigFS (/mnt/jffs2) tar.gz Exporter for Cisco WAP610N
set -f

ADMIN_PASS=""
for conf in /mnt/jffs2/sys.conf /tmp/sys.conf /etc/sys.conf /root/mtlk/etc/sys.conf; do
    if [ -f "$conf" ] && [ -z "$ADMIN_PASS" ]; then
        ADMIN_PASS=$(awk '/^\^?(Admin)?Password[ \t]*=/ {idx = index($0, "="); val = substr($0, idx + 1); sub(/^[ \t]+/, "", val); sub(/[ \t\r]+$/, "", val); print val}' "$conf" 2>/dev/null | tail -n 1)
    fi
done

if [ -n "$HTTP_AUTHORIZATION" ]; then
    AUTH_IN=$(printf '%s' "$HTTP_AUTHORIZATION" | sed 's/^[Bb]asic //')
    EXPECTED_AUTH=$(printf '%s' "admin:$ADMIN_PASS" | uuencode -m - 2>/dev/null | sed '1d;$d' | tr -d '\r\n')
    if [ -z "$ADMIN_PASS" ] || [ -z "$EXPECTED_AUTH" ] || [ "$AUTH_IN" != "$EXPECTED_AUTH" ]; then
        printf 'Content-Type: text/plain\r\n\r\nUnauthorized\n'
        exit 0
    fi
elif [ -z "$REMOTE_USER" ] && { [ -z "$SERVER_SOFTWARE" ] || [ -z "$GATEWAY_INTERFACE" ]; }; then
    printf 'Content-Type: text/plain\r\n\r\nUnauthorized\n'
    exit 0
fi

[ -d /mnt/jffs2 ] || { printf 'Content-Type: text/plain\r\n\r\nConfigFS not mounted\n'; exit 0; }

FILES=$(cd /mnt/jffs2 && ls -1 2>/dev/null | grep -v '^lost+found$')
[ -n "$FILES" ] || { printf 'Content-Type: text/plain\r\n\r\nConfigFS empty\n'; exit 0; }

printf 'Content-Type: application/x-gzip\r\n'
printf 'Content-Disposition: attachment; filename="WAP610N_jffs2_backup.tar.gz"\r\n'
printf 'Pragma: no-cache\r\n'
printf 'Cache-Control: no-store\r\n\r\n'
( cd /mnt/jffs2 && tar -czf - $FILES 2>/dev/null )
exit 0
