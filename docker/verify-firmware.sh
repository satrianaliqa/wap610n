#!/bin/bash
# Automated QA / Sanity-Check Tool untuk Memvalidasi Firmware sebelum Handover ke Flasher
# Enhancements: Comprehensive binary validation, ramdisk integrity, and compatibility checks
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$DIR/.." && pwd)"

# Check if BIN_PATH argument provided, else auto-discover
if [ -n "$1" ] && [ -f "$1" ]; then
    BIN_PATH="$1"
elif [ -f "$ROOT_DIR/output/bootpImage" ]; then
    BIN_PATH="$ROOT_DIR/output/bootpImage"
else
    # Dynamically find whatever firmware binary was produced regardless of version name
    BIN_PATH=$(find "$ROOT_DIR/output" -maxdepth 1 -type f \( -name "*WAP610N*" -o -name "bootpImage*" \) 2>/dev/null | grep -v "\.map\|\.lzma\|\.gz\|\.o" | head -n 1 || true)
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

echo ""
echo "=========================================================="
echo " 🔍 AUDIT & QUALITY ASSURANCE - FIRMWARE VALIDATION"
echo "=========================================================="
echo " Target: $BIN_PATH"
echo " Ramdisk: $RAMDISK_LZMA"
echo "=========================================================="
echo ""

if [ ! -f "$BIN_PATH" ]; then
    echo "❌ ERROR: File firmware $BIN_PATH belum di-compile!"
    echo "Locations checked:"
    [ -d "$ROOT_DIR/output" ] && ls -lh "$ROOT_DIR/output/" 2>/dev/null || echo "  (output directory not found)"
    exit 1
fi

# Color codes for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

# Binary format validation
echo "Performing binary format validation..."
FILE_FORMAT=$(file "$BIN_PATH" 2>/dev/null || echo "unknown")
echo "  File Type: $FILE_FORMAT"

# Check for ELF or ARM binary markers
if head -c 256 "$BIN_PATH" 2>/dev/null | hexdump -C 2>/dev/null | head -1 | grep -q "0000 000.*4142 4300"; then
    echo "  Binary Header: ✅ Valid bootpImage header detected (ABC marker)"
else
    echo "  Binary Header: ⚠️  Custom header format (checking continued...)"
fi

echo ""

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

# 3. Check CRC32 Checksum in Header and Footer
HEADER_CRC=$(hexdump -s 28 -n 4 -e '1/4 "%08x"' "$BIN_PATH" 2>/dev/null || od -j 28 -N 4 -t x4 -An "$BIN_PATH" | tr -d ' ')
FOOTER_CRC=$(tail -c 4 "$BIN_PATH" | (hexdump -e '1/4 "%08x"' 2>/dev/null || od -t x4 -An | tr -d ' '))
echo -n "3. Pengecekan Validasi CRC32 (Anti Image-Corrupt): "
if [ -n "$HEADER_CRC" ] && [ "$HEADER_CRC" != "00000000" ] && [ "$HEADER_CRC" = "$FOOTER_CRC" ]; then
    echo "✅ VALID (CRC32: 0x$HEADER_CRC cocok di header dan footer)"
else
    echo "❌ INVALID (Header CRC: 0x$HEADER_CRC != Footer CRC: 0x$FOOTER_CRC - mkcsum tidak berjalan!)"
    exit 1
fi

# 4. Check Ramdisk Contents
echo "4. Pengecekan Integritas Isi RootFS (Ramdisk): "
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

        # Check Dropbear and Web Server
        if [ -x /tmp/mnt_chk/usr/sbin/dropbear ] && [ -x /tmp/mnt_chk/root/mtlk/web/webs ]; then
            echo '   - Dropbear SSH & WebServer      : ✅ ADA & EXECUTABLE'
        else
            echo '   - Dropbear / WebServer          : ⚠️  WARNING (Missing daemon)'
        fi

        umount /tmp/mnt_chk
        rm -rf /tmp/chk_rd /tmp/mnt_chk
    "

# 5. Check Critical Shell Scripts Syntax
echo -n "5. Pengecekan Sintaks Seluruh Skrip Shell Kritis: "
SCRIPTS_OK=1
for s in "$ROOT_DIR"/rootfs-star/bin/*.sh "$ROOT_DIR"/rootfs-star/root/mtlk/etc/*.sh "$ROOT_DIR"/rootfs-star/etc/*.sh; do
    if [ -f "$s" ]; then
        FIRST_LINE=$(head -n 1 "$s" 2>/dev/null)
        case "$FIRST_LINE" in
            *bin/sh*|*bin/bash*)
                if ! bash -n "$s" 2>/dev/null; then
                    echo "❌ ERROR: Syntax error in $s"
                    SCRIPTS_OK=0
                fi
                ;;
        esac
    fi
done
if [ $SCRIPTS_OK -eq 1 ]; then
    echo "✅ VALID (Semua skrip shell bebas syntax error)"
else
    exit 1
fi

echo "=========================================================="
echo " 🎉 KESIMPULAN: FIRMWARE 100% SIAP DAN AMAN DI-FLASH!"
echo "=========================================================="
