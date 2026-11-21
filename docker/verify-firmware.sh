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
    echo "❌ ERROR: Neither docker nor podman found in PATH"
    exit 1
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

# Check for ARM bootpImage reset branch instruction (0xEA000006 -> LE: 06 00 00 ea)
if head -c 256 "$BIN_PATH" 2>/dev/null | hexdump -C 2>/dev/null | grep -qE "^00000000  06 00 00 ea"; then
    echo "  Binary Header: ✅ Valid bootpImage ARM branch header (0xEA000006)"
else
    echo "  Binary Header: ❌ Invalid bootpImage header (missing 0xEA000006 branch magic)"
    exit 1
fi

echo ""

# 1. Check file size
FILE_SIZE=$(stat -c %s "$BIN_PATH")
SIZE_MB=$(awk -v s="$FILE_SIZE" 'BEGIN {printf "%.2f", s / 1048576}')
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
    echo "❌ INVALID Device ID Hex: $DEVICE_ID_HEX (Expected 0x0012)"
    exit 1
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
        set -e
        mkdir -p /tmp/chk_rd /tmp/mnt_chk
        lzma -d -c /output/ramdisk_2.6.16.img.lzma > /tmp/chk_rd/rd.img
        mount -o loop /tmp/chk_rd/rd.img /tmp/mnt_chk
        
        # Check dev/console and critical GPIO/MTD/kmsg device nodes
        for devchk in console kmsg gpio3 gpio13 led0 rdflt0; do
            if [ ! -c \"/tmp/mnt_chk/dev/\$devchk\" ]; then
                echo \"   - /dev/\$devchk                   : ❌ HILANG!\"
                exit 1
            fi
        done
        if ls -l /tmp/mnt_chk/dev/rdflt0 | grep -qE ' 42,[[:space:]]*3 | 240,[[:space:]]*3 '; then
            echo '   - /dev/{console,kmsg,gpio*,rdflt0}: ✅ ADA (rdflt0 minor 3)'
        else
            echo '   - /dev/rdflt0 minor 3            : ❌ SALAH!'
            exit 1
        fi

        # Check shadow permissions (0600)
        if [ \"\$(stat -c %a /tmp/mnt_chk/etc/shadow 2>/dev/null)\" = '600' ]; then
            echo '   - /etc/shadow (0600)            : ✅ SECURE'
        else
            echo '   - /etc/shadow permissions       : ❌ TIDAK 0600!'
            exit 1
        fi

        # Check libc, ld-uClibc, expat, and openssl symlinks
        if [ -f /tmp/mnt_chk/lib/ld-uClibc-0.9.29.so ] && [ -f /tmp/mnt_chk/lib/libc.so.0 ] && \
           { [ -f /tmp/mnt_chk/lib/libexpat.so ] || [ -L /tmp/mnt_chk/lib/libexpat.so ]; } && \
           [ -f /tmp/mnt_chk/lib/libssl.so.0.9.8 ] && \
           [ -L /tmp/mnt_chk/lib/libcrypto.so ] && [ -L /tmp/mnt_chk/lib/libssl.so ]; then
            echo '   - Shared Libs (uClibc/SSL/Expat): ✅ ADA & TERSYMLINK'
        else
            echo '   - Shared Libraries              : ❌ HILANG / BROKEN SYMLINK!'
            exit 1
        fi

        # Check busybox init
        if [ -x /tmp/mnt_chk/bin/busybox ] && [ -L /tmp/mnt_chk/sbin/init ]; then
            echo '   - BusyBox Binary & /sbin/init   : ✅ ADA & TERHUBUNG'
        else
            echo '   - BusyBox / Init Link           : ❌ ERROR!'
            exit 1
        fi

        # Check Dropbear and Web Server (ensure freshly compiled webs, not legacy 2011 wlan.tar.gz blob)
        WEBS_BIN=/tmp/mnt_chk/root/mtlk/web/webs
        if [ -x /tmp/mnt_chk/usr/sbin/dropbear ] && [ -x \"\$WEBS_BIN\" ]; then
            if ! grep -qaF 'HTTP listener initialized' \"\$WEBS_BIN\" 2>/dev/null; then
                WEBS_SZ=\$(stat -c %s \"\$WEBS_BIN\" 2>/dev/null || echo 0)
                echo \"   - Dropbear SSH & WebServer      : ❌ STALE WEBS BINARY (\${WEBS_SZ} bytes, missing custom webs build marker)!\"
                exit 1
            fi
            echo '   - Dropbear SSH & WebServer      : ✅ ADA, EXECUTABLE & FRESHLY COMPILED'
        else
            echo '   - Dropbear / WebServer          : ❌ ERROR (Missing daemon)'
            exit 1
        fi

        # Backdoor eradication: shell.cgi and leaking root CGI symlinks must NOT exist
        if [ -e /tmp/mnt_chk/root/mtlk/web/cgi-bin/shell.cgi ] || [ -e /tmp/mnt_chk/root/mtlk/web/shell.cgi ] || \
           [ -e /tmp/mnt_chk/root/mtlk/web/remote_access.cgi ] || [ -e /tmp/mnt_chk/root/mtlk/web/dump_firmware.cgi ]; then
            echo '   - Security CGI check            : ❌ TERDETEKSI (shell.cgi/root symlink)!'
            exit 1
        else
            echo '   - Security CGI check            : ✅ BERSIH (No shell.cgi or root symlinks)'
        fi

        # Packaging regression: WAP610N customizations must survive wlan.tar.gz / VB overwrite
        PKG_OK=1
        grep -qF '/proc/sys/vm/min_free_kbytes' /tmp/mnt_chk/root/mtlk/etc/mtlk_init.sh || PKG_OK=0
        grep -qF 'echo 1 > /dev/led0' /tmp/mnt_chk/root/mtlk/etc/mtlk_init.sh || PKG_OK=0
        grep -qF 'Access Point Information' /tmp/mnt_chk/root/mtlk/web/status/device_status.asp || PKG_OK=0
        grep -qF 'objectDisplay(\"freqBand\", value)' /tmp/mnt_chk/root/mtlk/web/station/wireless_basic.asp || PKG_OK=0
        [ -f /tmp/mnt_chk/root/mtlk/etc/mtlk_init_platform.sh ] || PKG_OK=0
        if [ \$PKG_OK -eq 1 ]; then
            echo '   - WAP610N Customizations (pkg)  : ✅ UTUH (sysctl, LED, AP UI, 5GHz band)'
        else
            echo '   - WAP610N Customizations (pkg)  : ❌ HILANG (wlan.tar.gz/VB overwrite regression)!'
            exit 1
        fi

        # Shell syntax check of scripts as actually packaged in the ramdisk
        RD_SH_OK=1
        for s in /tmp/mnt_chk/bin/*.sh /tmp/mnt_chk/etc/*.sh /tmp/mnt_chk/root/mtlk/etc/*.sh /tmp/mnt_chk/root/mtlk/web/*.sh; do
            [ -f \"\$s\" ] || continue
            case \"\$(head -n 1 \"\$s\" 2>/dev/null)\" in
                *bin/sh*)
                    if ! sh -n \"\$s\" 2>/dev/null; then
                        echo \"   ❌ Syntax error: \${s#/tmp/mnt_chk}\"
                        RD_SH_OK=0
                    fi
                    ;;
                *bin/bash*)
                    if ! bash -n \"\$s\" 2>/dev/null; then
                        echo \"   ❌ Syntax error: \${s#/tmp/mnt_chk}\"
                        RD_SH_OK=0
                    fi
                    ;;
            esac
        done
        if [ \$RD_SH_OK -eq 1 ]; then
            echo '   - Ramdisk Shell Scripts Syntax  : ✅ VALID'
        else
            exit 1
        fi

        umount /tmp/mnt_chk
        rm -rf /tmp/chk_rd /tmp/mnt_chk
    "

# 5. Check Critical Shell Scripts Syntax
echo -n "5. Pengecekan Sintaks Seluruh Skrip Shell Kritis: "
SCRIPTS_OK=1
for s in "$ROOT_DIR"/rootfs-star/bin/*.sh "$ROOT_DIR"/rootfs-star/root/mtlk/etc/*.sh "$ROOT_DIR"/rootfs-star/etc/*.sh "$ROOT_DIR"/apps/jffs2-etc/networking/VB/*.sh; do
    if [ -f "$s" ]; then
        FIRST_LINE=$(head -n 1 "$s" 2>/dev/null)
        case "$FIRST_LINE" in
            *bin/sh*)
                if ! sh -n "$s" 2>/dev/null; then
                    echo "❌ ERROR: Syntax error in $s"
                    SCRIPTS_OK=0
                fi
                ;;
            *bin/bash*)
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
