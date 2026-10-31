#!/bin/sh
set -e
LC_ALL=C
LANG=C

if [ "$1" ]
then
  ROOT_DIR=$(cd "$1" && pwd)
else
  ROOT_DIR=/opt/star
fi

rm -f arch/arm/boot/bootp/*.o
rm -f arch/arm/boot/bootp/bootp
rm -f arch/arm/boot/Image
rm -f arch/arm/boot/zImage
rm -f arch/arm/boot/bootpImage

# Pre-compile mkcsum host utility
if [ -f "arch/arm/boot/bootp/mkcsum.c" ]; then
  gcc -O2 arch/arm/boot/bootp/mkcsum.c -lz -o arch/arm/boot/bootp/mkcsum
  chmod +x arch/arm/boot/bootp/mkcsum
fi

##make bootpImage INITRD=$ROOT_DIR/images/ramdisk_2.6.16.img.gz 
make bootpImage INITRD=$ROOT_DIR/images/ramdisk_2.6.16.img.lzma 

# Verify bootpImage exists and is non-empty
if [ ! -s "arch/arm/boot/bootpImage" ]; then
  echo "ERROR: arch/arm/boot/bootpImage was not generated or is empty!" >&2
  exit 1
fi

# Hard flash partition size gate (3.625 MB = 3,801,088 bytes)
BOOTP_SIZE=$(stat -c%s arch/arm/boot/bootpImage 2>/dev/null || wc -c < arch/arm/boot/bootpImage)
if [ "$BOOTP_SIZE" -gt 3801088 ]; then
    echo "FATAL: bootpImage ($BOOTP_SIZE bytes) exceeds 3,801,088 bytes (0x3A0000) flash limit!" >&2
    exit 1
fi

# Double check that CRC is applied to bootpImage
if [ -x "arch/arm/boot/bootp/mkcsum" ] && [ -f "arch/arm/boot/bootpImage" ]; then
  arch/arm/boot/bootp/mkcsum arch/arm/boot/bootpImage
fi

mkdir -p "$ROOT_DIR/output"
cp -vf System.map arch/arm/boot/bootpImage "$ROOT_DIR/output/"

# ArnonM's setup
if [ -e /tftpboot ]; then
	cp -vf System.map arch/arm/boot/bootpImage /tftpboot
fi

exit 0
