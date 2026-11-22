#!/bin/sh
ROOTDIR=${ROOTDIR:-$(pwd)}
MODENAME=${1:-WAP610N}
ENV_CONF=${ROOTDIR}/output/.config_${MODENAME}
VER_FILE=${ROOTDIR}/config/${MODENAME}_VERSION

[ -f "${VER_FILE}" ] || VER_FILE=${ROOTDIR}/config/WAP610N_VERSION
[ -f "${ENV_CONF}" ] && . "${ENV_CONF}"
[ -f "${VER_FILE}" ] && [ -z "${FIRMWARE_VERSION:-}" ] && . "${VER_FILE}"

BIN_TAG="${FIRMWARE_FILENAME:-${ProjectName:-$MODENAME}_v${FIRMWARE_VERSION:-2.0.00}_${BUILD_GIT_SHA:-local}.bin}"
cp -p -v "${ROOTDIR}/output/bootpImage" "${ROOTDIR}/output/${BIN_TAG}"
