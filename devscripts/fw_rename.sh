#!/bin/sh
ROOTDIR=${ROOTDIR:-$(pwd)}
MODENAME=$1
ENV_CONF=${ROOTDIR}/config/.config_${MODENAME}
. ${ENV_CONF}
cp -p -v $ROOTDIR/output/bootpImage $ROOTDIR/output/${FIRMWARE_FILENAME}
