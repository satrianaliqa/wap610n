#!/bin/bash
# Create an LZMA compressed ext2 ramdisk rootfs file system, containing all files in rootfs.
ROOTFS_DIR=rootfs

echo "--> Preparing rootfs files and version..."
mkdir -p ${ROOTFS_DIR}/root/mtlk
mkdir -p ${ROOTFS_DIR}/etc

# Create version file
echo -n "SVN Revision $(svnversion .. 2>/dev/null || echo 1)  " > ${ROOTFS_DIR}/etc/version
date >> ${ROOTFS_DIR}/etc/version

# Extract WLAN files directly into rootfs/root/mtlk/
if [ -n "$1" ]; then
	if [ -d "$1" ]; then
		echo "Copying WLAN files from $1 to ${ROOTFS_DIR}/root/mtlk/"
		cp -rf "$1"/* "${ROOTFS_DIR}/root/mtlk/"
	elif [ -f "$1" ]; then
		echo "Extracting WLAN files from $1 to ${ROOTFS_DIR}/root/mtlk/"
		tar -xzf "$1" -C "${ROOTFS_DIR}/root/mtlk/"
	fi
	chmod 777 -R "${ROOTFS_DIR}/root/mtlk/"

	# Save only needed Progmodels and delete the rest
	if [ -f ./apps/.config ]; then
		grep HWTYPE ./apps/.config > "${ROOTFS_DIR}/root/mtlk/etc/hwtype.sh" || true
		if [ -f "${ROOTFS_DIR}/root/mtlk/etc/hwtype.sh" ]; then
			. "${ROOTFS_DIR}/root/mtlk/etc/hwtype.sh"
			if [ -n "$HWTYPE" ] && [ -d "${ROOTFS_DIR}/root/mtlk/images" ]; then
				echo "Cleaning unused Progmodels for $HWTYPE..."
				pushd "${ROOTFS_DIR}/root/mtlk/images" >/dev/null
				ls ProgModel* 2>/dev/null | egrep -v "$HWTYPE|CB.bin" | xargs rm -f 2>/dev/null || true
				popd >/dev/null
			fi
		fi
	fi

	# Install Metalink init scripts & platform configurations
	if [ -d "apps/jffs2-etc/networking/VB" ]; then
		echo "Installing Metalink VB startup scripts to ${ROOTFS_DIR}/root/mtlk/etc/..."
		mkdir -p "${ROOTFS_DIR}/root/mtlk/etc"
		cp -af apps/jffs2-etc/networking/VB/*.sh "${ROOTFS_DIR}/root/mtlk/etc/" 2>/dev/null || true
		cp -af apps/jffs2-etc/networking/VB/*.tcl "${ROOTFS_DIR}/root/mtlk/etc/" 2>/dev/null || true
		cp -af apps/jffs2-etc/networking/VB/etherdump_awk "${ROOTFS_DIR}/root/mtlk/etc/" 2>/dev/null || true
		[ -f "apps/jffs2-etc/networking/VB/mtlk_init_platform.sh.platform.UMEDIA" ] && \
			cp -af "apps/jffs2-etc/networking/VB/mtlk_init_platform.sh.platform.UMEDIA" "${ROOTFS_DIR}/root/mtlk/etc/mtlk_init_platform.sh"
	fi

	# Force ProjectName to WAP610N (Access Point mode with SSID linksys)
	if [ -d "${ROOTFS_DIR}/root/mtlk/web" ]; then
		echo "Configuring WAP610N Access Point Mode in fw_version.txt..."
		cat << 'EOF' > "${ROOTFS_DIR}/root/mtlk/web/fw_version.txt"
ProjectName="WAP610N"
FIRMWARE_VERSION="1.0.05"
ProjectFirmwareVersionDate="1.0.05 build 0, Aug 17, 2026"
EOF
		cp -af "${ROOTFS_DIR}/root/mtlk/web/fw_version.txt" "${ROOTFS_DIR}/root/mtlk/etc/fw_version.txt" 2>/dev/null || true
	fi
fi

# Ensure device nodes and symlinks are present in rootfs
mkdir -p ${ROOTFS_DIR}/dev
[ -e ${ROOTFS_DIR}/dev/console ] || mknod ${ROOTFS_DIR}/dev/console c 5 1 2>/dev/null || true
[ -e ${ROOTFS_DIR}/dev/null ] || mknod ${ROOTFS_DIR}/dev/null c 1 3 2>/dev/null || true
[ -e ${ROOTFS_DIR}/dev/ttyS0 ] || mknod ${ROOTFS_DIR}/dev/ttyS0 c 4 64 2>/dev/null || true
[ -e ${ROOTFS_DIR}/dev/ram0 ] || mknod ${ROOTFS_DIR}/dev/ram0 b 1 0 2>/dev/null || true

# Ensure init symlinks
ln -sf bin/busybox ${ROOTFS_DIR}/init 2>/dev/null || true
ln -sf ../bin/busybox ${ROOTFS_DIR}/sbin/init 2>/dev/null || true
ln -sf bin/busybox ${ROOTFS_DIR}/linuxrc 2>/dev/null || true
ln -sf busybox ${ROOTFS_DIR}/bin/sh 2>/dev/null || true

echo "--> Generating ext2 ramdisk image directly from ${ROOTFS_DIR}..."
mkdir -p images
rm -f images/ramdisk_2.6.16.img images/ramdisk_2.6.16.img.lzma images/ramdisk_2.6.16.img.gz

RD=images/ramdisk_2.6.16.img
dd if=/dev/zero of=${RD} bs=1k count=12288 >/dev/null 2>&1
mke2fs -F -b 1024 -m 0 ${RD} >/dev/null 2>&1

MNT_DIR=/tmp/mnt_rd
mkdir -p ${MNT_DIR}
mount -o loop ${RD} ${MNT_DIR}
cp -a ${ROOTFS_DIR}/. ${MNT_DIR}/
umount ${MNT_DIR}
rmdir ${MNT_DIR}

echo "--> Compressing ramdisk image with LZMA..."
lzma -f -z ${RD}

echo "--> Ramdisk generation complete: images/ramdisk_2.6.16.img.lzma ($(ls -lh images/ramdisk_2.6.16.img.lzma | awk '{print $5}'))"
