#!/bin/sh
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
  gcc -O2 arch/arm/boot/bootp/mkcsum.c -lz -o arch/arm/boot/bootp/mkcsum 2>/dev/null || true
  chmod +x arch/arm/boot/bootp/mkcsum 2>/dev/null || true
fi

##make bootpImage INITRD=$ROOT_DIR/images/ramdisk_2.6.16.img.gz 
make bootpImage INITRD=$ROOT_DIR/images/ramdisk_2.6.16.img.lzma 

# Double check that CRC is applied to bootpImage
if [ -x "arch/arm/boot/bootp/mkcsum" ] && [ -f "arch/arm/boot/bootpImage" ]; then
  arch/arm/boot/bootp/mkcsum arch/arm/boot/bootpImage 2>/dev/null || true
fi

cp -vf System.map arch/arm/boot/bootpImage $ROOT_DIR/output/

# ArnonM's setup
if [ -e /tftpboot ]
then
	cp -vf System.map arch/arm/boot/bootpImage /tftpboot
fi
