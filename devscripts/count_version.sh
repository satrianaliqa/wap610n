#!/bin/sh
ROOTDIR=${ROOTDIR:-$(pwd)}
MODENAME=${1:-WAP610N}
VER_PATH=${ROOTDIR}/config
ENV_CONF=${ROOTDIR}/output/.config_${MODENAME}
VER_FILE=${VER_PATH}/${MODENAME}_VERSION

# Preserve any caller environment overrides before sourcing VER_FILE
ENV_MAJOR="${MAJOR:-}"
ENV_MINOR="${MINOR:-}"
ENV_PATCH="${PATCH:-}"
ENV_BUILD="${BUILD:-}"
ENV_FW_VER="${FIRMWARE_VERSION:-}"

ProjectName="${MODENAME}"

if [ ! -f "${VER_FILE}" ]; then
	VER_FILE="${VER_PATH}/WAP610N_VERSION"
fi

if [ -f "${VER_FILE}" ]; then
	. "${VER_FILE}"
fi

# Re-apply explicit environment overrides
[ -n "${ENV_MAJOR}" ] && MAJOR="${ENV_MAJOR}"
[ -n "${ENV_MINOR}" ] && MINOR="${ENV_MINOR}"
[ -n "${ENV_PATCH}" ] && PATCH="${ENV_PATCH}"
[ -n "${ENV_BUILD}" ] && BUILD="${ENV_BUILD}"
[ -n "${ENV_FW_VER}" ] && FIRMWARE_VERSION="${ENV_FW_VER}"

# Synchronize MAJOR/MINOR/PATCH <-> FIRMWARE_VERSION cleanly
if [ -n "${ENV_MAJOR}" ] || [ -n "${ENV_MINOR}" ] || [ -n "${ENV_PATCH}" ]; then
	FIRMWARE_VERSION="${MAJOR:-2}.${MINOR:-0}.${PATCH:-00}"
elif [ -n "${FIRMWARE_VERSION:-}" ]; then
	case "${FIRMWARE_VERSION}" in
		[0-9]*.[0-9]*.[0-9]*)
			MAJOR=$(echo "${FIRMWARE_VERSION}" | cut -d. -f1)
			MINOR=$(echo "${FIRMWARE_VERSION}" | cut -d. -f2)
			PATCH=$(echo "${FIRMWARE_VERSION}" | cut -d. -f3)
			;;
	esac
else
	FIRMWARE_VERSION="${MAJOR:-2}.${MINOR:-0}.${PATCH:-00}"
fi

FIRMWARE_MAJOR="${MAJOR:-2}"
FIRMWARE_MINOR="${MINOR:-0}"
FIRMWARE_PATCH="${PATCH:-00}"
BUILD="${BUILD:-0}"

VER_STR="${FIRMWARE_VERSION} build ${BUILD}"
VER_FILENAME="${ProjectName}_v${FIRMWARE_VERSION}_build_${BUILD}"
echo "Current version: ${VER_STR}"

# Write generated build state into output/.config_${MODENAME} (never into config/)
mkdir -p "${ROOTDIR}/output"
cat > "${ENV_CONF}" << EOF
FIRMWARE_MAJOR=${FIRMWARE_MAJOR}
FIRMWARE_MINOR=${FIRMWARE_MINOR}
FIRMWARE_PATCH=${FIRMWARE_PATCH}
ProjectName=${ProjectName}
FIRMWARE_BUILD=${BUILD}
FIRMWARE_VERSION="${FIRMWARE_VERSION}"
FIRMWARE_FILENAME="${VER_FILENAME}"
ProjectFirmwareVersionDate="${VER_STR}, $(date +'%b %e, %Y')"
EOF
