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

##make bootpImage INITRD=$ROOT_DIR/images/ramdisk_2.6.16.img.gz 
make bootpImage INITRD=$ROOT_DIR/images/ramdisk_2.6.16.img.lzma 
  ##V=1
cp -vf System.map arch/arm/boot/bootpImage $ROOT_DIR/output/

# ArnonM's setup
if [ -e /tftpboot ]
then
	cp -vf System.map arch/arm/boot/bootpImage /tftpboot
fi
