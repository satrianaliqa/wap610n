#!/bin/sh
LC_ALL=C
LANG=C

make zImage
cp -v arch/arm/boot/zImage ../../output/zImage
