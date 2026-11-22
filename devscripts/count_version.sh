#!/bin/sh
ROOTDIR=${ROOTDIR:-$(pwd)}
MODENAME=${1:-WAP610N}
VER_PATH=${ROOTDIR}/config
ENV_CONF=${ROOTDIR}/config/.config_${MODENAME}
VER_FILE=${VER_PATH}/${MODENAME}_VERSION

# Preserve any caller environment overrides before sourcing VER_FILE
ENV_MAJOR="${MAJOR:-}"
ENV_MINOR="${MINOR:-}"
ENV_PATCH="${PATCH:-}"
ENV_BUILD="${BUILD:-}"
ENV_FW_VER="${FIRMWARE_VERSION:-}"

if [ -f "${ENV_CONF}" ]; then
	. "${ENV_CONF}"
fi

ProjectName="${ProjectName:-$MODENAME}"

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
if [ -n "${ENV_FW_VER}" ] && [ -z "${ENV_MAJOR}" ] && [ -z "${ENV_MINOR}" ] && [ -z "${ENV_PATCH}" ]; then
	case "${FIRMWARE_VERSION}" in
		[0-9]*.[0-9]*.[0-9]*)
			MAJOR=$(echo "${FIRMWARE_VERSION}" | cut -d. -f1)
			MINOR=$(echo "${FIRMWARE_VERSION}" | cut -d. -f2)
			PATCH=$(echo "${FIRMWARE_VERSION}" | cut -d. -f3)
			;;
	esac
elif [ -z "${FIRMWARE_VERSION}" ] || [ -n "${ENV_MAJOR}" ] || [ -n "${ENV_MINOR}" ] || [ -n "${ENV_PATCH}" ]; then
	FIRMWARE_VERSION="${MAJOR:-1}.${MINOR:-0}.${PATCH:-05}"
elif [ "${FIRMWARE_VERSION}" != "${MAJOR}.${MINOR}.${PATCH}" ]; then
	case "${FIRMWARE_VERSION}" in
		[0-9]*.[0-9]*.[0-9]*)
			MAJOR=$(echo "${FIRMWARE_VERSION}" | cut -d. -f1)
			MINOR=$(echo "${FIRMWARE_VERSION}" | cut -d. -f2)
			PATCH=$(echo "${FIRMWARE_VERSION}" | cut -d. -f3)
			;;
		*)
			FIRMWARE_VERSION="${MAJOR:-1}.${MINOR:-0}.${PATCH:-05}"
			;;
	esac
fi

FIRMWARE_MAJOR="${MAJOR:-1}"
FIRMWARE_MINOR="${MINOR:-0}"
FIRMWARE_PATCH="${PATCH:-05}"
BUILD="${BUILD:-0}"

VER_STR="${FIRMWARE_VERSION} build ${BUILD}"
VER_FILENAME="${ProjectName}_v${FIRMWARE_VERSION}_build_${BUILD}"
echo "Current version: ${VER_STR}"

# Update generated build state in config/.config_${MODENAME} (never clobbering config/WAP610N_VERSION)
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
