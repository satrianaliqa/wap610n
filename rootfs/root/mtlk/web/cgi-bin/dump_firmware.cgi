#!/bin/sh
# Real-time physical MTD flash dumper for Cisco WAP610N Web Management GUI

ADMIN_PASS=`awk -F "=" '/^AdminPassword/ {str = $2; gsub(/ /, "", str); sub(/\r/, "", str); print str}' /tmp/sys.conf 2>/dev/null`
if [ -n "$ADMIN_PASS" ] && [ -z "$HTTP_AUTHORIZATION" ]; then
    echo "Status: 401 Unauthorized"
    echo "Content-Type: text/plain"
    echo ""
    echo "Unauthorized"
    exit 0
fi

QUERY_STRING="${QUERY_STRING:-type=full}"

case "$QUERY_STRING" in
    *type=kernel*)
        DEV="/dev/mtdblock2"
        [ ! -e "$DEV" ] && DEV="/dev/mtd2"
        FILENAME="WAP610N_kernel_rootfs_dump.bin"
        ;;
    *type=config*)
        DEV="/dev/mtdblock4"
        [ ! -e "$DEV" ] && DEV="/dev/mtd4"
        FILENAME="WAP610N_config_dump.bin"
        ;;
    *)
        DEV="/dev/mtdblock0"
        [ ! -e "$DEV" ] && DEV="/dev/mtd0"
        FILENAME="WAP610N_full_flash_dump.bin"
        ;;
esac

echo "Content-Type: application/octet-stream"
echo "Content-Disposition: attachment; filename=\"$FILENAME\""
echo "Pragma: no-cache"
echo "Cache-Control: no-cache"
echo ""

if [ -e "$DEV" ]; then
    cat "$DEV"
elif [ -e "/dev/mtd0" ]; then
    cat /dev/mtd0
fi
