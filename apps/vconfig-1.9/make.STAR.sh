#!/bin/sh
PATH=/opt/Networking-versions/star/tools/arm-uclibc-3.4.6/bin:$PATH
which arm-linux-gcc

make $* CC=arm-linux-gcc STRIP=arm-linux-strip

