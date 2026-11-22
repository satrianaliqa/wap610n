#!/bin/sh
ROOTDIR=${ROOTDIR:-$(pwd)}
MODENAME=${1:-WAP610N}
ENV_CONF=${ROOTDIR}/config/.config_${MODENAME}
VER_FILE=${ROOTDIR}/config/${MODENAME}_VERSION

[ -f "${VER_FILE}" ] || VER_FILE=${ROOTDIR}/config/WAP610N_VERSION
[ -f "${ENV_CONF}" ] && . "${ENV_CONF}"
[ -f "${VER_FILE}" ] && [ -z "${FIRMWARE_VERSION:-}" ] && . "${VER_FILE}"

BIN_TAG="${ProjectName:-$MODENAME}_v${FIRMWARE_VERSION:-1.0.05}.bin"
if [ -n "${FIRMWARE_FILENAME:-}" ]; then
	cp -p -v "${ROOTDIR}/output/bootpImage" "${ROOTDIR}/output/${FIRMWARE_FILENAME}"
fi
cp -p -v "${ROOTDIR}/output/bootpImage" "${ROOTDIR}/output/${BIN_TAG}"
