#!/bin/sh
# Real-time physical MTD flash dumper redirector for Cisco WAP610N
# Validates dump type and redirects via HTML/JS since GoAhead cgi.c forces HTTP 200.

DUMP_TYPE=""
for kv in $(printf '%s' "$QUERY_STRING" | tr '&' ' '); do
	case "$kv" in
		type=*)
			DUMP_TYPE="${kv#type=}"
			;;
	esac
done

case "$DUMP_TYPE" in
	full|kernel|config)
		TARGET_URL="/goform/dumpFirmware?type=${DUMP_TYPE}"
		echo "Content-Type: text/html; charset=utf-8"
		echo "Pragma: no-cache"
		echo "Cache-Control: no-cache"
		echo ""
		printf '<!DOCTYPE html><html><head><meta http-equiv="refresh" content="0;url=%s"><script>window.location.replace("%s");</script></head><body>Redirecting to firmware dump...</body></html>\n' "$TARGET_URL" "$TARGET_URL"
		;;
	*)
		echo "Content-Type: text/plain"
		echo "Pragma: no-cache"
		echo ""
		echo "400 Bad Request: invalid dump type (expected full, kernel, or config)"
		;;
esac
