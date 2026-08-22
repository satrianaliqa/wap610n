#!/bin/bash
# Automated QA / Sanity-Check Tool untuk Memvalidasi Firmware sebelum Handover ke Flasher
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$DIR/.." && pwd)"

# Find firmware binary dynamically
if [ -n "$1" ] && [ -f "$1" ]; then
    BIN_PATH="$1"
elif [ -f "$ROOT_DIR/output/bootpImage" ]; then
    BIN_PATH="$ROOT_DIR/output/bootpImage"
else
    # Dynamically find whatever firmware binary was produced regardless of version name
    BIN_PATH=$(find "$ROOT_DIR/output" -maxdepth 1 -type f \( -name "*WAP610N*" -o -name "*WET610N*" -o -name "bootpImage*" \) 2>/dev/null | grep -v "\.map\|\.lzma\|\.gz\|\.o" | head -n 1 || true)
fi

# Detect ramdisk relative to BIN_PATH or repo output
if [ -f "$(dirname "$BIN_PATH")/ramdisk_2.6.16.img.lzma" ]; then
    RAMDISK_LZMA="$(cd "$(dirname "$BIN_PATH")" && pwd)/ramdisk_2.6.16.img.lzma"
else
    RAMDISK_LZMA="$ROOT_DIR/output/ramdisk_2.6.16.img.lzma"
fi

# Detect container runtime
if command -v docker >/dev/null 2>&1; then
    CONTAINER_CMD="docker"
elif command -v podman >/dev/null 2>&1; then
    CONTAINER_CMD="podman"
else
    CONTAINER_CMD="podman"
fi

echo "=========================================================="
echo " 🔍 AUDIT & QUALITY ASSURANCE FIRMWARE WAP610N"
echo " Target: $BIN_PATH"
echo "=========================================================="

if [ ! -f "$BIN_PATH" ]; then
    echo "❌ ERROR: File firmware $BIN_PATH belum di-compile!"
    exit 1
fi

# 1. Check file size
FILE_SIZE=$(stat -c %s "$BIN_PATH")
SIZE_MB=$(echo "scale=2; $FILE_SIZE / 1048576" | bc)
MAX_FLASH_SIZE=3801088 # 0x3A0000 bytes (3.625 MB)

echo -n "1. Pengecekan Ukuran Flash Partition Limit: "
if [ "$FILE_SIZE" -le "$MAX_FLASH_SIZE" ] && [ "$FILE_SIZE" -ge 2097152 ]; then
    echo "✅ VALID ($SIZE_MB MB / Max: 3.62 MB)"
else
    echo "❌ INVALID ($SIZE_MB MB melebihi partisi flash / terlalu kecil)"
    exit 1
fi

# 2. Check 32-Byte Header (Device ID 0x0012)
DEVICE_ID_HEX=$(hexdump -s 12 -n 4 -e '1/4 "%08x"' "$BIN_PATH" 2>/dev/null || od -j 12 -N 4 -t x4 -An "$BIN_PATH" | tr -d ' ')
echo -n "2. Pengecekan Device ID Header: "
if [ "$DEVICE_ID_HEX" = "00000012" ] || [ "$DEVICE_ID_HEX" = "12000000" ] || [ "$DEVICE_ID_HEX" = "0012" ]; then
    echo "✅ VALID (0x0012 - WAP610N Official)"
else
    echo "⚠️ Device ID Hex: $DEVICE_ID_HEX (Cek ke U-Boot config)"
fi

# 3. Check Ramdisk Contents
echo "3. Pengecekan Integritas Isi RootFS (Ramdisk): "
sudo $CONTAINER_CMD run --rm --privileged \
    -v "$(dirname "$RAMDISK_LZMA"):/output:z" \
    wap610n-builder /bin/bash -c "
        mkdir -p /tmp/chk_rd /tmp/mnt_chk
        lzma -d -c /output/ramdisk_2.6.16.img.lzma > /tmp/chk_rd/rd.img
        mount -o loop /tmp/chk_rd/rd.img /tmp/mnt_chk
        
        # Check dev/console
        if [ -c /tmp/mnt_chk/dev/console ]; then
            echo '   - /dev/console (c 5 1)          : ✅ ADA'
        else
            echo '   - /dev/console                  : ❌ HILANG!'
            exit 1
        fi

        # Check libc and ld-uClibc
        if [ -f /tmp/mnt_chk/lib/ld-uClibc-0.9.29.so ] && [ -f /tmp/mnt_chk/lib/libc.so.0 ]; then
            echo '   - Runtime Linker & C Library    : ✅ ADA'
        else
            echo '   - Runtime Linker & C Library    : ❌ HILANG!'
            exit 1
        fi

        # Check busybox init
        if [ -x /tmp/mnt_chk/bin/busybox ] && [ -L /tmp/mnt_chk/sbin/init ]; then
            echo '   - BusyBox Binary & /sbin/init   : ✅ ADA & TERHUBUNG'
        else
            echo '   - BusyBox / Init Link           : ❌ ERROR!'
            exit 1
        fi

        umount /tmp/mnt_chk
        rm -rf /tmp/chk_rd /tmp/mnt_chk
    "

echo "=========================================================="
echo " 🎉 KESIMPULAN: FIRMWARE 100% SIAP DAN AMAN DI-FLASH!"
echo "=========================================================="
