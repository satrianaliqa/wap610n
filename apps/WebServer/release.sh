#!/bin/sh
set -e

WEBROOTDIR=`pwd`
cd "${WEBROOTDIR}/LINUX"

make -f Makefile.STAR clean
make -f Makefile.STAR install
