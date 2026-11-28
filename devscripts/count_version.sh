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

# Resolve BUILD: explicit numeric env -> CI GITHUB_RUN_NUMBER -> local git commit count -> 0
if [ -n "${ENV_BUILD}" ] && [ "${ENV_BUILD}" != "auto" ]; then
	BUILD="${ENV_BUILD}"
elif [ -z "${BUILD:-}" ] || [ "${BUILD}" = "auto" ] || [ "${BUILD}" = "0" ]; then
	if [ -n "${GITHUB_RUN_NUMBER:-}" ]; then
		BUILD="${GITHUB_RUN_NUMBER}"
	elif command -v git >/dev/null 2>&1; then
		BUILD=$(git -c safe.directory='*' -C "${ROOTDIR}" rev-list --count HEAD 2>/dev/null || true)
	fi
fi
BUILD=$(printf '%s' "${BUILD:-0}" | tr -cd '0-9')
[ -n "${BUILD}" ] || BUILD="0"

# Resolve Git commit SHA (7-char short) and branch for OpenWrt-style build identity
GIT_SHA="${BUILD_GIT_SHA:-}"
if [ -z "${GIT_SHA}" ] && command -v git >/dev/null 2>&1; then
	GIT_SHA=$(git -c safe.directory='*' -C "${ROOTDIR}" rev-parse --short=7 HEAD 2>/dev/null || true)
fi
GIT_SHA=$(printf '%s' "${GIT_SHA:-local}" | tr -cd 'A-Za-z0-9._-' | cut -c1-7)
[ -n "${GIT_SHA}" ] || GIT_SHA="local"

GIT_BRANCH="${BUILD_GIT_BRANCH:-}"
if [ -z "${GIT_BRANCH}" ] && command -v git >/dev/null 2>&1; then
	GIT_BRANCH=$(git -c safe.directory='*' -C "${ROOTDIR}" rev-parse --abbrev-ref HEAD 2>/dev/null || true)
fi
GIT_BRANCH=$(printf '%s' "${GIT_BRANCH:-local}" | tr '/' '-' | tr -cd 'A-Za-z0-9._-')
[ -n "${GIT_BRANCH}" ] || GIT_BRANCH="local"

VER_STR="${FIRMWARE_VERSION} (${GIT_BRANCH} / git-${GIT_SHA} build ${BUILD})"
VER_FILENAME="${ProjectName}_v${FIRMWARE_VERSION}_b${BUILD}_${GIT_SHA}.bin"
echo "Current version: ${VER_STR}"

# Write generated build state into output/.config_${MODENAME} (never into config/)
mkdir -p "${ROOTDIR}/output"
cat > "${ENV_CONF}" << EOF
FIRMWARE_MAJOR=${FIRMWARE_MAJOR}
FIRMWARE_MINOR=${FIRMWARE_MINOR}
FIRMWARE_PATCH=${FIRMWARE_PATCH}
ProjectName=${ProjectName}
FIRMWARE_BUILD=${BUILD}
BUILD_GIT_SHA="${GIT_SHA}"
BUILD_GIT_BRANCH="${GIT_BRANCH}"
FIRMWARE_VERSION="${FIRMWARE_VERSION}"
FIRMWARE_FILENAME="${VER_FILENAME}"
ProjectFirmwareVersionDate="${VER_STR}, $(date +'%b %e, %Y')"
EOF
