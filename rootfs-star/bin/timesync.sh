#!/bin/sh
# Offline-safe HTTP Date time synchronization for WAP610N (zero permanent RAM)

LOG_FILE="/tmp/timesync.log"

set_fallback_time() {
	cur_yr=$(date -u +"%Y" 2>/dev/null || echo 1970)
	case "$cur_yr" in
		''|*[!0-9]*) cur_yr=1970 ;;
	esac
	if [ "$cur_yr" -lt 2025 ] && [ -f /etc/build_time_utc ]; then
		ts=$(tr -d ' \t\r\n' < /etc/build_time_utc 2>/dev/null)
		case "$ts" in
			[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9].[0-9][0-9])
				date -u "$ts" >/dev/null 2>&1 || true
				;;
		esac
	fi
}

has_default_route() {
	route -n 2>/dev/null | awk '$1 == "0.0.0.0" { found = 1 } END { exit !found }'
}

try_http_date() {
	host="$1"
	port="$2"
	parsed=$(printf 'HEAD / HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n' "$host" \
		| nc -w 4 "$host" "$port" 2>/dev/null \
		| awk '
			BEGIN {
				m["Jan"]="01"; m["Feb"]="02"; m["Mar"]="03"; m["Apr"]="04"
				m["May"]="05"; m["Jun"]="06"; m["Jul"]="07"; m["Aug"]="08"
				m["Sep"]="09"; m["Oct"]="10"; m["Nov"]="11"; m["Dec"]="12"
			}
			tolower($1) == "date:" {
				gsub(/\r/, "", $0)
				# Format: Date: Wed, 25 Nov 2026 02:05:10 GMT
				dd = $3 + 0
				mon = m[$4]
				yr = $5 + 0
				split($6, t, ":")
				if (mon != "" && dd >= 1 && dd <= 31 && yr >= 2025 && yr <= 2099 && t[1] != "" && t[2] != "" && t[3] != "") {
					printf "%s%02d%02d%02d%04d.%02d", mon, dd, t[1]+0, t[2]+0, yr, t[3]+0
					exit
				}
			}
		')
	if [ -n "$parsed" ]; then
		if date -u "$parsed" >/dev/null 2>&1; then
			return 0
		fi
	fi
	return 1
}

set_fallback_time

if [ "$1" = "--once" ]; then
	if try_http_date 1.1.1.1 80 || try_http_date connectivitycheck.gstatic.com 80; then
		echo "synced: $(date -u 2>/dev/null)" > "$LOG_FILE"
		exit 0
	fi
	echo "offline: using build fallback time ($(date -u 2>/dev/null))" > "$LOG_FILE"
	exit 1
fi

for delay in 12 45 120; do
	sleep "$delay"
	if has_default_route; then
		if try_http_date 1.1.1.1 80 || try_http_date connectivitycheck.gstatic.com 80; then
			echo "synced: $(date -u 2>/dev/null)" > "$LOG_FILE"
			exit 0
		fi
	fi
done

echo "offline: using build fallback time ($(date -u 2>/dev/null))" > "$LOG_FILE"
exit 0
