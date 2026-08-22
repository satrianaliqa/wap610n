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

	# Keep all ProgModel microcode files intact for full hardware revision compatibility
	echo "Keeping all ProgModel microcode calibration files intact in ${ROOTFS_DIR}/root/mtlk/images/..."

	# Install Metalink init scripts & platform configurations
	if [ -d "apps/jffs2-etc/networking/VB" ]; then
		echo "Installing Metalink VB startup scripts to ${ROOTFS_DIR}/root/mtlk/etc/..."
		mkdir -p "${ROOTFS_DIR}/root/mtlk/etc" "${ROOTFS_DIR}/root/mtlk/saved_configs"
		cp -af apps/jffs2-etc/networking/VB/*.sh "${ROOTFS_DIR}/root/mtlk/etc/" 2>/dev/null || true
		cp -af apps/jffs2-etc/networking/VB/*.tcl "${ROOTFS_DIR}/root/mtlk/etc/" 2>/dev/null || true
		cp -af apps/jffs2-etc/networking/VB/etherdump_awk "${ROOTFS_DIR}/root/mtlk/etc/" 2>/dev/null || true
		cp -af apps/jffs2-etc/networking/VB/*.conf "${ROOTFS_DIR}/root/mtlk/saved_configs/" 2>/dev/null || true
		[ -f "apps/jffs2-etc/networking/VB/mtlk_init_platform.sh.platform.UMEDIA" ] && \
			cp -af "apps/jffs2-etc/networking/VB/mtlk_init_platform.sh.platform.UMEDIA" "${ROOTFS_DIR}/root/mtlk/etc/mtlk_init_platform.sh"
	fi

	# Force ProjectName to WAP610N (Access Point mode with SSID linksys)
	if [ -d "${ROOTFS_DIR}/root/mtlk/web" ]; then
		echo "Configuring WAP610N Access Point Mode in fw_version.txt..."
		cat << 'EOF' > "${ROOTFS_DIR}/root/mtlk/web/fw_version.txt"
ProjectName="WAP610N"
FIRMWARE_VERSION="1.0.08"
ProjectFirmwareVersionDate="1.0.08 build 0, Aug 22, 2026"
EOF
		cp -af "${ROOTFS_DIR}/root/mtlk/web/fw_version.txt" "${ROOTFS_DIR}/root/mtlk/etc/fw_version.txt" 2>/dev/null || true
		mkdir -p "${ROOTFS_DIR}/root/mtlk/web/network" "${ROOTFS_DIR}/root/mtlk/web/wireless"
		ln -sf sta_network.asp "${ROOTFS_DIR}/root/mtlk/web/network/ap_network.asp" 2>/dev/null || true
		ln -sf ../station/wireless_basic.asp "${ROOTFS_DIR}/root/mtlk/web/wireless/security.asp" 2>/dev/null || true
		ln -sf ../station/wireless_basic.asp "${ROOTFS_DIR}/root/mtlk/web/wireless/wireless_basic.asp" 2>/dev/null || true
		ln -sf ../station/wireless_advanced.asp "${ROOTFS_DIR}/root/mtlk/web/wireless/wireless_advanced.asp" 2>/dev/null || true
		ln -sf ../station/wmm.asp "${ROOTFS_DIR}/root/mtlk/web/wireless/wmm.asp" 2>/dev/null || true
		ln -sf ../station/wps_status.asp "${ROOTFS_DIR}/root/mtlk/web/wireless/wps_status.asp" 2>/dev/null || true
		ln -sf ../station/site_survey.asp "${ROOTFS_DIR}/root/mtlk/web/wireless/site_survey.asp" 2>/dev/null || true
		mkdir -p "${ROOTFS_DIR}/root/mtlk/web/lang"
		touch "${ROOTFS_DIR}/root/mtlk/web/lang/STRINGS_EN.txt"
		ln -sf STRINGS_EN.txt "${ROOTFS_DIR}/root/mtlk/web/lang/STRINGS_.txt" 2>/dev/null || true
		mkdir -p "${ROOTFS_DIR}/root/mtlk/web/cgi-bin"
		[ -f "rootfs-star/root/mtlk/web/cgi-bin/dump_firmware.cgi" ] && \
			cp -af "rootfs-star/root/mtlk/web/cgi-bin/dump_firmware.cgi" "${ROOTFS_DIR}/root/mtlk/web/cgi-bin/"
		[ -f "rootfs-star/root/mtlk/web/cgi-bin/remote_access.cgi" ] && \
			cp -af "rootfs-star/root/mtlk/web/cgi-bin/remote_access.cgi" "${ROOTFS_DIR}/root/mtlk/web/cgi-bin/"
		[ -f "rootfs-star/root/mtlk/web/cgi-bin/shell.cgi" ] && \
			cp -af "rootfs-star/root/mtlk/web/cgi-bin/shell.cgi" "${ROOTFS_DIR}/root/mtlk/web/cgi-bin/"
		ln -sf cgi-bin/shell.cgi "${ROOTFS_DIR}/root/mtlk/web/shell.cgi" 2>/dev/null || true
		ln -sf cgi-bin/remote_access.cgi "${ROOTFS_DIR}/root/mtlk/web/remote_access.cgi" 2>/dev/null || true
		ln -sf cgi-bin/dump_firmware.cgi "${ROOTFS_DIR}/root/mtlk/web/dump_firmware.cgi" 2>/dev/null || true
		[ -f "rootfs-star/root/mtlk/web/run_webs.sh" ] && \
			cp -af "rootfs-star/root/mtlk/web/run_webs.sh" "${ROOTFS_DIR}/root/mtlk/web/"
		[ -f "rootfs-star/root/mtlk/saved_configs/default_admin.conf" ] && \
			cp -af "rootfs-star/root/mtlk/saved_configs/default_admin.conf" "${ROOTFS_DIR}/root/mtlk/saved_configs/"
		[ -f "rootfs-star/root/mtlk/etc/sys.conf" ] && \
			cp -af "rootfs-star/root/mtlk/etc/sys.conf" "${ROOTFS_DIR}/root/mtlk/saved_configs/sys.conf.default"
		[ -f "${ROOTFS_DIR}/root/mtlk/web/cgi-bin/dump_firmware.cgi" ] && \
			chmod +x "${ROOTFS_DIR}/root/mtlk/web/cgi-bin/dump_firmware.cgi" 2>/dev/null || true
		[ -f "${ROOTFS_DIR}/root/mtlk/web/cgi-bin/remote_access.cgi" ] && \
			chmod +x "${ROOTFS_DIR}/root/mtlk/web/cgi-bin/remote_access.cgi" 2>/dev/null || true
		[ -f "${ROOTFS_DIR}/root/mtlk/web/cgi-bin/shell.cgi" ] && \
			chmod +x "${ROOTFS_DIR}/root/mtlk/web/cgi-bin/shell.cgi" 2>/dev/null || true
		[ -f "rootfs-star/root/mtlk/web/admin/management.asp" ] && \
			cp -af "rootfs-star/root/mtlk/web/admin/management.asp" "${ROOTFS_DIR}/root/mtlk/web/admin/management.asp"
		[ -f "rootfs-star/root/mtlk/web/admin/upgrade.asp" ] && \
			cp -af "rootfs-star/root/mtlk/web/admin/upgrade.asp" "${ROOTFS_DIR}/root/mtlk/web/admin/upgrade.asp"
	fi
fi

# Ensure all base rootfs directories and mount points exist
ensure_rootfs_dirs() {
	local base="$1"
	mkdir -p "$base/proc" "$base/sys" "$base/tmp" "$base/mnt" "$base/mnt/jffs2" \
	         "$base/var" "$base/var/run" "$base/var/log" "$base/var/lock" \
	         "$base/root" "$base/home" "$base/opt" "$base/etc" \
	         "$base/bin" "$base/sbin" "$base/lib" "$base/usr/bin" "$base/usr/sbin" \
	         "$base/dev" "$base/dev/pts" "$base/dev/shm" "$base/dev/net" "$base/dev/input"
	chmod 1777 "$base/tmp" 2>/dev/null || true
	chmod 755 "$base/proc" "$base/sys" "$base/mnt" "$base/var" "$base/root" 2>/dev/null || true
}

ensure_rootfs_dirs "${ROOTFS_DIR}"

# Ensure complete device nodes and symlinks are present in rootfs
create_device_nodes() {
	local target_dev="$1"
	mkdir -p "$target_dev" "$target_dev/pts" "$target_dev/shm" "$target_dev/net" "$target_dev/input"

	# First, try to extract devices directly from rootfs-star.tgz if present
	if [ -f "rootfs-star.tgz" ]; then
		tar -zxvf rootfs-star.tgz rootfs-star/dev -C /tmp/ 2>/dev/null || true
		if [ -d "/tmp/rootfs-star/dev" ]; then
			cp -a /tmp/rootfs-star/dev/* "$target_dev/" 2>/dev/null || true
			rm -rf /tmp/rootfs-star
		fi
	fi

	# Core essential device nodes
	[ -e "$target_dev/console" ] || mknod "$target_dev/console" c 5 1 2>/dev/null || true
	[ -e "$target_dev/null" ] || mknod "$target_dev/null" c 1 3 2>/dev/null || true
	[ -e "$target_dev/zero" ] || mknod "$target_dev/zero" c 1 5 2>/dev/null || true
	[ -e "$target_dev/random" ] || mknod "$target_dev/random" c 1 8 2>/dev/null || true
	[ -e "$target_dev/urandom" ] || mknod "$target_dev/urandom" c 1 9 2>/dev/null || true
	[ -e "$target_dev/mem" ] || mknod "$target_dev/mem" c 1 1 2>/dev/null || true
	[ -e "$target_dev/kmem" ] || mknod "$target_dev/kmem" c 1 2 2>/dev/null || true
	[ -e "$target_dev/ptmx" ] || mknod "$target_dev/ptmx" c 5 2 2>/dev/null || true
	[ -e "$target_dev/tty" ] || mknod "$target_dev/tty" c 5 0 2>/dev/null || true

	# Serial and Virtual Terminals
	[ -e "$target_dev/ttyS0" ] || mknod "$target_dev/ttyS0" c 4 64 2>/dev/null || true
	[ -e "$target_dev/ttyS1" ] || mknod "$target_dev/ttyS1" c 4 65 2>/dev/null || true
	[ -e "$target_dev/ttyS2" ] || mknod "$target_dev/ttyS2" c 4 66 2>/dev/null || true
	for i in 0 1 2 3 4 5 6 7; do
		[ -e "$target_dev/tty$i" ] || mknod "$target_dev/tty$i" c 4 $i 2>/dev/null || true
		[ -e "$target_dev/ttyp$i" ] || mknod "$target_dev/ttyp$i" c 3 $i 2>/dev/null || true
		[ -e "$target_dev/ptyp$i" ] || mknod "$target_dev/ptyp$i" c 2 $i 2>/dev/null || true
	done
	for i in 0 1 2 3; do
		[ -e "$target_dev/ttyP$i" ] || mknod "$target_dev/ttyP$i" c 57 $i 2>/dev/null || true
	done

	# RAM & Loop Devices
	for i in 0 1 2 3; do
		[ -e "$target_dev/ram$i" ] || mknod "$target_dev/ram$i" b 1 $i 2>/dev/null || true
	done
	[ -e "$target_dev/ram" ] || ln -sf ram1 "$target_dev/ram" 2>/dev/null || true
	for i in 0 1 2 3 4 5 6 7; do
		[ -e "$target_dev/loop$i" ] || mknod "$target_dev/loop$i" b 7 $i 2>/dev/null || true
	done

	# MTD Raw Character & Block Devices (mtd0-4, mtdblock0-4)
	for i in 0 1 2 3 4; do
		local mtd_minor=$((i * 2))
		[ -e "$target_dev/mtd$i" ] || mknod "$target_dev/mtd$i" c 90 $mtd_minor 2>/dev/null || true
		[ -e "$target_dev/mtdblock$i" ] || mknod "$target_dev/mtdblock$i" b 31 $i 2>/dev/null || true
	done

	# Star Semiconductor GPIOs and LEDs (Major 42)
	for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31; do
		[ -e "$target_dev/gpio$i" ] || mknod "$target_dev/gpio$i" c 42 $i 2>/dev/null || true
	done
	for i in 0 1 2 3; do
		[ -e "$target_dev/led$i" ] || mknod "$target_dev/led$i" c 42 $i 2>/dev/null || true
	done
	[ -e "$target_dev/rdflt0" ] || mknod "$target_dev/rdflt0" c 42 13 2>/dev/null || true

	# Network TUN
	[ -e "$target_dev/net/tun" ] || mknod "$target_dev/net/tun" c 10 200 2>/dev/null || true

	# Symlinks & Permissions
	[ -e "$target_dev/log" ] || ln -sf ../tmp/log "$target_dev/log" 2>/dev/null || true
	chmod 600 "$target_dev/console" "$target_dev/ttyS0" 2>/dev/null || true
	chmod 666 "$target_dev/null" "$target_dev/zero" "$target_dev/random" "$target_dev/urandom" "$target_dev/tty" 2>/dev/null || true
	chmod 666 "$target_dev"/gpio* "$target_dev"/led* 2>/dev/null || true
	chmod 660 "$target_dev"/mtd* "$target_dev"/mtdblock* 2>/dev/null || true
	chown -R 0:0 "$target_dev" 2>/dev/null || true
}

create_device_nodes "${ROOTFS_DIR}/dev"

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
# Ensure essential directories and device nodes exist in mounted ramdisk as root
ensure_rootfs_dirs "${MNT_DIR}"
create_device_nodes "${MNT_DIR}/dev"
umount ${MNT_DIR}
rmdir ${MNT_DIR}

echo "--> Compressing ramdisk image with LZMA..."
lzma -f -z ${RD}

echo "--> Ramdisk generation complete: images/ramdisk_2.6.16.img.lzma ($(ls -lh images/ramdisk_2.6.16.img.lzma | awk '{print $5}'))"
