#!/bin/bash
set -o pipefail

RECONFIGURE_PLATFORM=0
MAKE_CLEAN=0
MAKE_BUILD=0
MAKE_KERNEL=0
MAKE_APPS=0
MAKE_WEBS=0
MAKE_PACK=0
NUM_OF_PARAMS=$#

STAR_DIR=.
NEW_PLATFORM=
CM_FILE=${STAR_DIR}/net_ver.cm
VENDOR_DIR=vendor/mtlk
MODENAME=WAP610N
ROOTFS_DIR=rootfs-star

ROOTDIR=$(pwd)
WEBSERVER_PATH="${ROOTDIR}/apps/WebServer"

function display_usage()
{
	[ -n "$1" ] && echo "$1"
	echo "Usage: make_nv.sh <command> [<options>]"
	echo ""
	echo "Supported commands (WAP610N Unified Platform):"
	echo "       reconf [<platform_name>]"
	echo "              Select/configure platform (default: star-6.7.2-mtlk-U-Media-vela)"
	echo "       clean"
	echo "              Clean kernel, applications, images, and output artifacts"
	echo "       build [<vendor_dir>] [WAP610N]"
	echo "              Full build: kernel, modules, apps, WebServer, rootfs ramdisk, bootpImage"
	echo "       kernel"
	echo "              Compile Linux kernel zImage and modules only"
	echo "       apps"
	echo "              Compile and install MTLK_APPS into rootfs/"
	echo "       webs"
	echo "              Compile and install GoAhead WebServer (apps/WebServer/LINUX/webs)"
	echo "       pack [<vendor_dir>]"
	echo "              Assemble rootfs, compress ramdisk, build bootpImage, and rename output"
	exit 1
}

function check_params()
{
	case "${1}" in
		reconf)
			[ ${NUM_OF_PARAMS} -gt 2 ] && display_usage ""
			RECONFIGURE_PLATFORM=1
			NEW_PLATFORM="${2:-star-6.7.2-mtlk-U-Media-vela}"
			;;
		clean)
			[ ${NUM_OF_PARAMS} -gt 2 ] && display_usage ""
			MAKE_CLEAN=1
			;;
		build)
			[ ${NUM_OF_PARAMS} -gt 3 ] && display_usage ""
			MAKE_BUILD=1
			VENDOR_DIR="${2:-vendor/mtlk}"
			MODENAME="${3:-WAP610N}"
			if [ "${MODENAME}" != "WAP610N" ]; then
				display_usage "ERROR: Only WAP610N is supported in this unified repository."
			fi
			ENV_CONF="${ROOTDIR}/config/.config_${MODENAME}"
			echo "WAP610N mode (vendor source: ${VENDOR_DIR})"
			;;
		kernel)
			MAKE_KERNEL=1
			;;
		apps)
			MAKE_APPS=1
			;;
		webs)
			MAKE_WEBS=1
			;;
		pack)
			MAKE_PACK=1
			VENDOR_DIR="${2:-vendor/mtlk}"
			MODENAME="WAP610N"
			ENV_CONF="${ROOTDIR}/config/.config_${MODENAME}"
			;;
		*)
			display_usage "Unknown command $1"
			;;
	esac
	return 0
}

function build_kernel()
{
	pushd kernel/linux-2.6.16-star > /dev/null
	export PATH="$(pwd)/../../tools/arm-uclibc-3.4.6/bin/:$PATH"
	make zImage -j3 2>&1 | tee -a "$LOGFILE"
	if [ $? -ne 0 ]; then
		echo "Failed to build kernel zImage" | tee -a "$LOGFILE"
		exit 1
	fi
	echo "Making dynamic modules" | tee -a "$LOGFILE"
	make modules 2>&1 | tee -a "$LOGFILE"
	if [ $? -ne 0 ]; then
		echo "Failed to build kernel modules" | tee -a "$LOGFILE"
		exit 1
	fi
	popd > /dev/null
}

function build_apps()
{
	if [ ! -d "${ROOTDIR}/rootfs/bin" ]; then
		mkdir -p "${ROOTDIR}/rootfs"
		cp -a "${ROOTDIR}/rootfs-star/." "${ROOTDIR}/rootfs/"
	fi
	pushd apps/scripts > /dev/null
	local APPS_PWD
	APPS_PWD=$(pwd)
	E_CROSS="${APPS_PWD}/../../tools/arm-uclibc-3.4.6" \
	E_KERNEL="${APPS_PWD}/../../kernel/linux-2.6.16-star" \
	make -f Makefile.MTLK comp install strip customize 2>&1 | tee -a "$LOGFILE"
	if [ $? -ne 0 ]; then
		echo "Failed to build applications" | tee -a "$LOGFILE"
		exit 1
	fi
	popd > /dev/null
}

function build_webs()
{
	echo "--> Building GoAhead WebServer (apps/WebServer)..."
	cd "${WEBSERVER_PATH}"
	./release.sh 2>&1 | tee -a "$LOGFILE"
	if [ $? -ne 0 ]; then
		echo "Failed to build WebServer" | tee -a "$LOGFILE"
		exit 1
	fi
	cd "${ROOTDIR}"
}

function pack_firmware()
{
	cd "${ROOTDIR}"
	if [ ! -f "${ENV_CONF}" ] || ! grep -q "^ProjectName=${MODENAME}$" "${ENV_CONF}" 2>/dev/null; then
		sed -i '/ProjectName/d' "${ENV_CONF}" 2>/dev/null || true
		echo "ProjectName=${MODENAME}" >> "${ENV_CONF}"
	fi
	devscripts/count_version.sh "${MODENAME}"

	mkdir -p "${ROOTDIR}/output" "${ROOTDIR}/images"
	rm -f "${ROOTDIR}/images/ramdisk_2.6.16.img.lzma"
	devscripts/net-ver-mkrootfs.sh "${VENDOR_DIR}" 2>&1 | tee -a "$LOGFILE"
	if [ $? -ne 0 ] || [ ! -f "${ROOTDIR}/images/ramdisk_2.6.16.img.lzma" ]; then
		echo "Failed to create rootfs ramdisk (see net-ver-mkrootfs.sh errors above)" | tee -a "$LOGFILE"
		exit 1
	fi

	cd kernel/linux-2.6.16-star > /dev/null
	export PATH="$(pwd)/../../tools/arm-uclibc-3.4.6/bin/:$PATH"
	./mkbootp.sh ../.. 2>&1 | tee -a "$LOGFILE"
	if [ $? -ne 0 ]; then
		echo "Failed to build kernel image" | tee -a "$LOGFILE"
		exit 1
	fi
	BOOTP_SIZE=$(stat -c%s arch/arm/boot/bootpImage 2>/dev/null || wc -c < arch/arm/boot/bootpImage)
	if [ "$BOOTP_SIZE" -gt 3801088 ]; then
		echo "FATAL: bootpImage ($BOOTP_SIZE bytes) exceeds 3,801,088 bytes (0x3A0000) flash limit!" | tee -a "$LOGFILE" >&2
		exit 1
	fi
	cd - > /dev/null

	cp kernel/linux-2.6.16-star/arch/arm/boot/bootp/kernel.o output/
	cp kernel/linux-2.6.16-star/arch/arm/boot/bootp/init.o output/
	cp kernel/linux-2.6.16-star/arch/arm/boot/bootp/initrd.S output/
	cp kernel/linux-2.6.16-star/arch/arm/boot/bootp/bootp.lds output/
	cp images/ramdisk_2.6.16.img.lzma output/
	[ -f "${CM_FILE}" ] && cp "${CM_FILE}" output/

	cd output/
	tar czf na.tar.gz kernel.o init.o initrd.S bootp.lds ramdisk_2.6.16.img.lzma
	cd - > /dev/null

	cd "${ROOTDIR}"
	devscripts/fw_rename.sh "${MODENAME}" 2>&1 | tee -a "$LOGFILE" || {
		echo "Failed to rename final firmware image" | tee -a "$LOGFILE"
		exit 1
	}
}

####################################
# Script body
####################################
LOGFILE="${ROOTDIR}/log.log"
echo "" > "$LOGFILE"

if [ ${NUM_OF_PARAMS} -eq 0 ] || [ ${NUM_OF_PARAMS} -gt 6 ]; then
	display_usage ""
else
	check_params "$@"
fi

if [ $RECONFIGURE_PLATFORM -eq 1 ]; then
	devscripts/net-ver-select-star.sh "$NEW_PLATFORM" 2>&1 | tee -a "$LOGFILE" || exit 1
	pushd apps/scripts > /dev/null
	APPS_PWD=$(pwd)
	E_CROSS="${APPS_PWD}/../../tools/arm-uclibc-3.4.6" \
	E_KERNEL="${APPS_PWD}/../../kernel/linux-2.6.16-star" \
	make -f Makefile.MTLK link clean config 2>&1 | tee -a "$LOGFILE" || exit 1
	popd > /dev/null
	rm -rf images/* output/*
fi

if [ $MAKE_CLEAN -eq 1 ]; then
	pushd apps/scripts > /dev/null
	APPS_PWD=$(pwd)
	E_CROSS="${APPS_PWD}/../../tools/arm-uclibc-3.4.6" \
	E_KERNEL="${APPS_PWD}/../../kernel/linux-2.6.16-star" \
	make -f Makefile.MTLK link clean 2>&1 | tee -a "$LOGFILE"
	popd > /dev/null

	cd kernel/linux-2.6.16-star
	export PATH="$(pwd)/../../tools/arm-uclibc-3.4.6/bin/:$PATH"
	make clean 2>&1 | tee -a "$LOGFILE"
	cd - > /dev/null
	rm -rf images/* output/*
	rm -f "${ROOTDIR}/kernel/linux-2.6.16-star/.config.old" "${ROOTDIR}/kernel/linux-2.6.16-star/.version"
	if [ -f "${ROOTDIR}/boards/star-6.7.2-mtlk-U-Media-vela/kernel/.config" ]; then
		cp -f "${ROOTDIR}/boards/star-6.7.2-mtlk-U-Media-vela/kernel/.config" "${ROOTDIR}/kernel/linux-2.6.16-star/.config"
	else
		rm -f "${ROOTDIR}/kernel/linux-2.6.16-star/.config"
	fi
fi

if [ $MAKE_KERNEL -eq 1 ]; then
	build_kernel
fi

if [ $MAKE_APPS -eq 1 ]; then
	build_apps
fi

if [ $MAKE_WEBS -eq 1 ]; then
	build_webs
fi

if [ $MAKE_PACK -eq 1 ]; then
	pack_firmware
fi

if [ $MAKE_BUILD -eq 1 ]; then
	build_kernel
	build_apps
	build_webs
	pack_firmware
fi

NUM_ERRS=$(grep -v ignored "$LOGFILE" | grep -cE "(: error:|make(\[[0-9]+\])?: \*\*\*)" || true)
if [ "$NUM_ERRS" -gt 0 ]; then
	echo -e "\n-----------------------------------------------------" | tee -a "$LOGFILE"
	echo "SUMMARY OF COMPILATION ERRORS:" | tee -a "$LOGFILE"
	grep -v ignored "$LOGFILE" | grep -E "(: error:|make(\[[0-9]+\])?: \*\*\*)" -B 4 -A 1 | tee -a "$LOGFILE"
	echo -e "\nEND OF COMPILATION ERRORS." | tee -a "$LOGFILE"
	echo -e "-----------------------------------------------------" | tee -a "$LOGFILE"
fi
rm -f "$LOGFILE"

exit 0
