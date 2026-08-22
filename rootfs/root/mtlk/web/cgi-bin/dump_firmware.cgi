#!/bin/sh
# Real-time physical MTD flash dumper for Cisco WAP610N Web Management GUI

QUERY_STRING="${QUERY_STRING:-type=full}"

case "$QUERY_STRING" in
    *type=kernel*)
        DEV="/dev/mtdblock1"
        [ ! -e "$DEV" ] && DEV="/dev/mtd1"
        FILENAME="WAP610N_kernel_rootfs_dump.bin"
        ;;
    *type=config*)
        DEV="/dev/mtdblock2"
        [ ! -e "$DEV" ] && DEV="/dev/mtd2"
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
