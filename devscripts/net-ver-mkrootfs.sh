#!/bin/bash
set -e
# Assemble an LZMA-compressed ext2 ramdisk rootfs deterministically for WAP610N.
ROOTFS_DIR=rootfs
STAR_DIR=rootfs-star
VENDOR_DIR="${1:-vendor/mtlk}"

if [ -d "${STAR_DIR}" ] && [ -e "${ROOTFS_DIR}" ] && \
   [ "$(readlink -f "${ROOTFS_DIR}")" = "$(readlink -f "${STAR_DIR}")" ]; then
	echo "ERROR: ${ROOTFS_DIR} resolves to ${STAR_DIR}. Use a real copy, not a symlink."
	exit 1
fi

echo "--> Preparing rootfs files and version..."
if [ ! -d "${ROOTFS_DIR}/bin" ]; then
	mkdir -p "${ROOTFS_DIR}"
	cp -a "${STAR_DIR}/." "${ROOTFS_DIR}/"
else
	for subdir in bin etc sbin usr var dev; do
		if [ -d "${STAR_DIR}/${subdir}" ]; then
			SUBDIR_EXCLUDES=""
			for compiled_bin in \
				bin/busybox bin/simpdiff bin/get_env_param bin/set_env_param \
				bin/crc bin/vconfig bin/tclsh bin/send_event \
				sbin/udhcpc usr/sbin/udhcpd usr/sbin/dropbear usr/sbin/dropbearmulti \
				usr/bin/dropbearkey usr/bin/dropbearconvert usr/bin/dbclient usr/bin/ssh usr/bin/scp \
				usr/bin/dumpleases usr/bin/DetectedBrLoop etc/Detected_BrLoop; do
				case "${compiled_bin}" in
					${subdir}/*)
						rel_bin="${compiled_bin#${subdir}/}"
						if [ -e "${ROOTFS_DIR}/${compiled_bin}" ]; then
							SUBDIR_EXCLUDES="${SUBDIR_EXCLUDES} --exclude=${rel_bin}"
						fi
						;;
				esac
			done
			mkdir -p "${ROOTFS_DIR}/${subdir}"
			tar -C "${STAR_DIR}/${subdir}" ${SUBDIR_EXCLUDES} -cf - . | tar -C "${ROOTFS_DIR}/${subdir}" -xf - || {
				echo "ERROR: Failed to sync ${STAR_DIR}/${subdir}"
				exit 1
			}
		fi
	done
fi
mkdir -p "${ROOTFS_DIR}/root/mtlk" "${ROOTFS_DIR}/etc"

# 1. Install proprietary Metalink blobs from open directory vendor/mtlk/
if [ -d "${VENDOR_DIR}" ]; then
	echo "Installing proprietary Metalink blobs from ${VENDOR_DIR} to ${ROOTFS_DIR}/root/mtlk/..."
	cp -af "${VENDOR_DIR}/." "${ROOTFS_DIR}/root/mtlk/"
elif [ -f "${VENDOR_DIR}" ]; then
	echo "Extracting WLAN archive ${VENDOR_DIR} to ${ROOTFS_DIR}/root/mtlk/..."
	tar -xzf "${VENDOR_DIR}" --exclude=web/webs --exclude=./web/webs -C "${ROOTFS_DIR}/root/mtlk/"
else
	echo "ERROR: Vendor directory ${VENDOR_DIR} not found!"
	exit 1
fi

# 2. Overlay the WAP610N single source of truth (rootfs-star/root/mtlk) excluding
# build-generated artifacts when freshly compiled copies already exist in rootfs/.
if [ -d "${STAR_DIR}/root/mtlk" ]; then
	echo "Syncing ${STAR_DIR}/root/mtlk onto ${ROOTFS_DIR}/root/mtlk..."
	TAR_EXCLUDES=""
	for built_art in etc/gpio.ko etc/hnap_wps_status_monitor etc/force_ethpc_renew_ip web/webs; do
		if [ -e "${ROOTFS_DIR}/root/mtlk/${built_art}" ]; then
			TAR_EXCLUDES="${TAR_EXCLUDES} --exclude=${built_art}"
		fi
	done
	tar -C "${STAR_DIR}/root/mtlk" ${TAR_EXCLUDES} -cf - . | tar -C "${ROOTFS_DIR}/root/mtlk" -xf - || {
		echo "ERROR: Failed to sync ${STAR_DIR}/root/mtlk"
		exit 1
	}
fi

# 3. Never allow stale prebuilt web/webs blobs to replace a freshly compiled WebServer binary
if [ -x "apps/WebServer/LINUX/webs" ]; then
	echo "Installing freshly compiled GoAhead WebServer (apps/WebServer/LINUX/webs)..."
	install -m 755 "apps/WebServer/LINUX/webs" "${ROOTFS_DIR}/root/mtlk/web/webs" || {
		echo "ERROR: Failed to install compiled WebServer binary"
		exit 1
	}
fi

# 4. Install optional standardized packages from packages/*/
if [ -d "packages" ]; then
	for pkg_mk in packages/*/Makefile.pkg; do
		[ -f "$pkg_mk" ] || continue
		pkg_dir="$(dirname "$pkg_mk")"
		echo "Building and installing package: $pkg_dir..."
		E_TOPDIR="$(pwd)/apps" E_DEST="$(pwd)/${ROOTFS_DIR}" \
			make -C "$pkg_dir" -f Makefile.pkg COMP STRIP INSTALL || exit 1
	done
fi

# Load config/WAP610N_VERSION while preserving any caller environment overrides
load_custom_build_vars() {
	local var val
	if [ -f "config/WAP610N_VERSION" ]; then
		while IFS='=' read -r var val; do
			case "$var" in
				''|\#*) continue ;;
			esac
			var=$(echo "$var" | tr -d ' \t\r')
			val=$(echo "$val" | sed -e 's/^[ \t]*//' -e 's/[ \t\r]*$//' -e 's/^"//' -e 's/"$//')
			[ -n "$var" ] || continue
			if [ -z "${!var:-}" ]; then
				export "${var}=${val}"
			fi
		done < "config/WAP610N_VERSION"
	fi
	if [ -z "${FIRMWARE_VERSION:-}" ]; then
		FIRMWARE_VERSION="${MAJOR:-2}.${MINOR:-0}.${PATCH:-00}"
	fi
	BUILD="${BUILD:-0}"

	# Resolve Git commit SHA & branch (env -> output/.config_WAP610N -> git -> local)
	if [ -z "${BUILD_GIT_SHA:-}" ] && [ -f "output/.config_WAP610N" ]; then
		BUILD_GIT_SHA=$(grep '^BUILD_GIT_SHA=' "output/.config_WAP610N" 2>/dev/null | head -n 1 | cut -d'"' -f2)
	fi
	if [ -z "${BUILD_GIT_SHA:-}" ] && command -v git >/dev/null 2>&1; then
		BUILD_GIT_SHA=$(git -c safe.directory='*' rev-parse --short=7 HEAD 2>/dev/null || true)
	fi
	BUILD_GIT_SHA=$(printf '%s' "${BUILD_GIT_SHA:-local}" | tr -cd 'A-Za-z0-9._-' | cut -c1-7)
	[ -n "${BUILD_GIT_SHA}" ] || BUILD_GIT_SHA="local"

	if [ -z "${BUILD_GIT_BRANCH:-}" ] && [ -f "output/.config_WAP610N" ]; then
		BUILD_GIT_BRANCH=$(grep '^BUILD_GIT_BRANCH=' "output/.config_WAP610N" 2>/dev/null | head -n 1 | cut -d'"' -f2)
	fi
	if [ -z "${BUILD_GIT_BRANCH:-}" ] && command -v git >/dev/null 2>&1; then
		BUILD_GIT_BRANCH=$(git -c safe.directory='*' rev-parse --abbrev-ref HEAD 2>/dev/null || true)
	fi
	BUILD_GIT_BRANCH=$(printf '%s' "${BUILD_GIT_BRANCH:-local}" | tr '/' '-' | tr -cd 'A-Za-z0-9._-')
	[ -n "${BUILD_GIT_BRANCH}" ] || BUILD_GIT_BRANCH="local"
	export BUILD_GIT_SHA BUILD_GIT_BRANCH
}

set_conf_kv() {
	local file="$1" key="$2" val="$3"
	[ -f "$file" ] || return 0
	[ -n "$val" ] || return 0
	awk -v k="$key" -v v="$val" '
		BEGIN { found = 0 }
		{
			line = $0
			sub(/\r$/, "", line)
			if (match(line, /^[ \t]*\^?[A-Za-z0-9_]+[ \t]*=/)) {
				lhs = line
				sub(/[ \t]*=.*/, "", lhs)
				gsub(/^[ \t]+|[ \t]+$/, "", lhs)
				clean_lhs = lhs
				sub(/^\^/, "", clean_lhs)
				if (clean_lhs == k) {
					if (!found) {
						prefix = (substr(lhs, 1, 1) == "^") ? "^" : ""
						print prefix k " = " v
						found = 1
					}
					next
				}
			}
			print line
		}
		END {
			if (!found) print k " = " v
		}
	' "$file" > "${file}.tmp" && mv "${file}.tmp" "$file"
}

load_custom_build_vars

# Create /etc/version with full OpenWrt-style build identity
echo "WAP610N v${FIRMWARE_VERSION} (${BUILD_GIT_BRANCH} / git-${BUILD_GIT_SHA}) - $(date -u '+%Y-%m-%d %H:%M:%S UTC')" > "${ROOTFS_DIR}/etc/version"

# 5. Auto-generate mtdblock4 factory default config.img.gz from plain-text config/mtdblock4-defaults/
if [ -f "config/mtdblock4-defaults/sys.conf" ] && [ -f "config/mtdblock4-defaults/wlan0.conf" ]; then
	echo "Generating factory default config.img.gz from config/mtdblock4-defaults/..."
	TMP_CFG_DIR=$(mktemp -d)
	TMP_CFG_IMG=$(mktemp)
	install -m 600 "config/mtdblock4-defaults/sys.conf" "${TMP_CFG_DIR}/sys.conf"
	install -m 600 "config/mtdblock4-defaults/wlan0.conf" "${TMP_CFG_DIR}/wlan0.conf"

	# Apply custom build variables & firmware version to staged default configs
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "FirmwareVersion" "v${FIRMWARE_VERSION} (git-${BUILD_GIT_SHA})"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "VendorName" "${CUSTOM_VENDOR_NAME:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "ModelName" "${CUSTOM_MODEL_NAME:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "HostName" "${CUSTOM_HOSTNAME:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "DeviceName" "${CUSTOM_HOSTNAME:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "ip_lan" "${CUSTOM_LAN_IP:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "subnet_lan" "${CUSTOM_LAN_SUBNET:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "default_gw" "${CUSTOM_DEFAULT_GW:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "DHCPDEnabled" "${CUSTOM_DHCPD_ENABLED:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "DHCPDStartAddress" "${CUSTOM_DHCP_START:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "DHCPDEndAddress" "${CUSTOM_DHCP_END:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "DHCPDDNS1" "${CUSTOM_DNS1:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "DHCPDDNS2" "${CUSTOM_DNS2:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "AdminUserName" "${CUSTOM_ADMIN_USER:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "AdminPassword" "${CUSTOM_ADMIN_PASS:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "AuthenticationTimeout" "${CUSTOM_AUTH_TIMEOUT:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "SSH_Enabled" "${CUSTOM_SSH_ENABLED:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "Telnet_Enabled" "${CUSTOM_TELNET_ENABLED:-}"
	set_conf_kv "${TMP_CFG_DIR}/sys.conf" "WirelessMgmtEnabled" "${CUSTOM_WIRELESS_MGMT:-}"

	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "HostName" "${CUSTOM_HOSTNAME:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "ip_lan" "${CUSTOM_LAN_IP:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "subnet_lan" "${CUSTOM_LAN_SUBNET:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "default_gw" "${CUSTOM_DEFAULT_GW:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "DHCPDEnabled" "${CUSTOM_DHCPD_ENABLED:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "DHCPDStartAddress" "${CUSTOM_DHCP_START:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "DHCPDEndAddress" "${CUSTOM_DHCP_END:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "DHCPDDNS1" "${CUSTOM_DNS1:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "DHCPDDNS2" "${CUSTOM_DNS2:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "WirelessMgmtEnabled" "${CUSTOM_WIRELESS_MGMT:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "NonProc_ESSID" "${CUSTOM_WIFI_SSID:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "FrequencyBand" "${CUSTOM_WIFI_BAND:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "Channel" "${CUSTOM_WIFI_CHANNEL:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "ChannelBonding" "${CUSTOM_WIFI_BONDING:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "Country" "${CUSTOM_WIFI_COUNTRY:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "NonProcSecurityMode" "${CUSTOM_WIFI_SEC_MODE:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "NonProc_WPA_Personal_PSK" "${CUSTOM_WIFI_WPA_PSK:-}"
	set_conf_kv "${TMP_CFG_DIR}/wlan0.conf" "NonProc_WPS_ActivateWPS" "${CUSTOM_WPS_ENABLED:-}"

	cp -af "${TMP_CFG_DIR}/sys.conf" "${ROOTFS_DIR}/root/mtlk/etc/sys.conf"
	cp -af "${TMP_CFG_DIR}/sys.conf" "${ROOTFS_DIR}/root/mtlk/saved_configs/sys.conf.default"
	cp -af "${TMP_CFG_DIR}/wlan0.conf" "${ROOTFS_DIR}/root/mtlk/saved_configs/default_ap_WAP610N.conf"

	if [ -f "${ROOTFS_DIR}/root/mtlk/saved_configs/default_admin.conf" ]; then
		set_conf_kv "${ROOTFS_DIR}/root/mtlk/saved_configs/default_admin.conf" "FirmwareVersion" "v${FIRMWARE_VERSION} (git-${BUILD_GIT_SHA})"
		set_conf_kv "${ROOTFS_DIR}/root/mtlk/saved_configs/default_admin.conf" "VendorName" "${CUSTOM_VENDOR_NAME:-}"
		set_conf_kv "${ROOTFS_DIR}/root/mtlk/saved_configs/default_admin.conf" "ModelName" "${CUSTOM_MODEL_NAME:-}"
		set_conf_kv "${ROOTFS_DIR}/root/mtlk/saved_configs/default_admin.conf" "DeviceName" "${CUSTOM_HOSTNAME:-}"
		set_conf_kv "${ROOTFS_DIR}/root/mtlk/saved_configs/default_admin.conf" "AdminUserName" "${CUSTOM_ADMIN_USER:-}"
		set_conf_kv "${ROOTFS_DIR}/root/mtlk/saved_configs/default_admin.conf" "AdminPassword" "${CUSTOM_ADMIN_PASS:-}"
		set_conf_kv "${ROOTFS_DIR}/root/mtlk/saved_configs/default_admin.conf" "AuthenticationTimeout" "${CUSTOM_AUTH_TIMEOUT:-}"
		set_conf_kv "${ROOTFS_DIR}/root/mtlk/saved_configs/default_admin.conf" "SSH_Enabled" "${CUSTOM_SSH_ENABLED:-}"
		set_conf_kv "${ROOTFS_DIR}/root/mtlk/saved_configs/default_admin.conf" "Telnet_Enabled" "${CUSTOM_TELNET_ENABLED:-}"
		set_conf_kv "${ROOTFS_DIR}/root/mtlk/saved_configs/default_admin.conf" "WirelessMgmtEnabled" "${CUSTOM_WIRELESS_MGMT:-}"
	fi

	if command -v genext2fs >/dev/null 2>&1; then
		genext2fs -U -b 128 -d "${TMP_CFG_DIR}" "${TMP_CFG_IMG}" >/dev/null 2>&1
	else
		mke2fs -d "${TMP_CFG_DIR}" -F -b 1024 -m 0 "${TMP_CFG_IMG}" 128 >/dev/null 2>&1
	fi
	gzip -n -9 -c "${TMP_CFG_IMG}" > "${ROOTFS_DIR}/root/mtlk/etc/config.img.gz"
	rm -rf "${TMP_CFG_DIR}" "${TMP_CFG_IMG}"
fi

# 6. Normalize permissions & symlinks under rootfs/root/mtlk
find "${ROOTFS_DIR}/root/mtlk" -type d -exec chmod 755 {} + 2>/dev/null || true
find "${ROOTFS_DIR}/root/mtlk" -type f -exec chmod 644 {} + 2>/dev/null || true
find "${ROOTFS_DIR}/root/mtlk" -type f \( -name '*.sh' -o -name '*.tcl' -o -name '*.cgi' -o -name '*_awk' \) -exec chmod 755 {} + 2>/dev/null || true
for exec_bin in \
	"${ROOTFS_DIR}/root/mtlk/web/webs" \
	"${ROOTFS_DIR}/root/mtlk/bcl/BclSockServer" \
	"${ROOTFS_DIR}/root/mtlk/etc/mtpriv" \
	"${ROOTFS_DIR}/root/mtlk/etc/mtdump" \
	"${ROOTFS_DIR}/root/mtlk/etc/drvhlpr" \
	"${ROOTFS_DIR}/root/mtlk/etc/hostapd" \
	"${ROOTFS_DIR}/root/mtlk/etc/hostapd_cli" \
	"${ROOTFS_DIR}/root/mtlk/etc/wpa_supplicant" \
	"${ROOTFS_DIR}/root/mtlk/etc/wpa_cli" \
	"${ROOTFS_DIR}/root/mtlk/etc/wpa_passphrase" \
	"${ROOTFS_DIR}/root/mtlk/etc/wsccmd" \
	"${ROOTFS_DIR}/root/mtlk/etc/upnpd" \
	"${ROOTFS_DIR}/root/mtlk/etc/hnap_wps_status_monitor" \
	"${ROOTFS_DIR}/root/mtlk/etc/force_ethpc_renew_ip" \
	"${ROOTFS_DIR}/root/mtlk/etc/wlan_logger"; do
	[ -f "$exec_bin" ] && chmod 755 "$exec_bin"
done

rm -f "${ROOTFS_DIR}/root/mtlk/web/crc"
rm -f "${ROOTFS_DIR}"/etc/udhcpd/LICENSE "${ROOTFS_DIR}"/etc/udhcpd/README \
      "${ROOTFS_DIR}"/etc/udhcpc/LICENSE "${ROOTFS_DIR}"/etc/udhcpc/README \
      "${ROOTFS_DIR}"/etc/hotplug/LICENSE "${ROOTFS_DIR}"/etc/hotplug/README \
      "${ROOTFS_DIR}"/etc/hotplug.d/LICENSE "${ROOTFS_DIR}"/etc/hotplug.d/README 2>/dev/null || true

if [ -x "tools/arm-uclibc-3.4.6/bin/arm-linux-uclibc-strip" ]; then
	STRIP_BIN="tools/arm-uclibc-3.4.6/bin/arm-linux-uclibc-strip"
	for ubin in "${ROOTFS_DIR}/root/mtlk/web/webs" "${ROOTFS_DIR}/root/mtlk/etc/drvhlpr" "${ROOTFS_DIR}/root/mtlk/etc/wpa_cli" "${ROOTFS_DIR}/root/mtlk/etc/wpa_passphrase"; do
		[ -x "$ubin" ] && "$STRIP_BIN" "$ubin" 2>/dev/null || true
	done
	find "${ROOTFS_DIR}" -name '*.ko' -exec "$STRIP_BIN" --strip-unneeded {} + 2>/dev/null || true
fi

# 7. Fail hard if any WAP610N customization was lost during packaging.
OVERLAY_OK=1
check_marker() {
	if ! grep -qF -- "$2" "${ROOTFS_DIR}/$1" 2>/dev/null; then
		echo "ERROR: Packaging regression: '$2' missing in ${ROOTFS_DIR}/$1"
		OVERLAY_OK=0
	fi
}
check_marker root/mtlk/etc/mtlk_init.sh "/proc/sys/vm/min_free_kbytes"
check_marker root/mtlk/etc/mtlk_init.sh "echo 1 > /dev/led0"
check_marker root/mtlk/etc/mtlk_init_hostname.sh 'if [ -z "$1" ]'
check_marker root/mtlk/web/status/device_status.asp "Access Point Information"
check_marker root/mtlk/web/station/wireless_basic.asp 'objectDisplay("freqBand", value)'
if [ "$OVERLAY_OK" != 1 ]; then
	exit 1
fi

# 8. Force ProjectName to WAP610N and ensure WebUI symlinks/files
if [ -d "${ROOTFS_DIR}/root/mtlk/web" ]; then
	echo "Configuring WAP610N Access Point Mode in fw_version.txt..."
	CURR_VER="${FIRMWARE_VERSION:-2.0.00}"
	CURR_BUILD="${BUILD:-0}"
	CURR_SHA="${BUILD_GIT_SHA:-local}"
	CURR_BRANCH="${BUILD_GIT_BRANCH:-local}"
	cat << EOF > "${ROOTFS_DIR}/root/mtlk/web/fw_version.txt"
ProjectName="WAP610N"
FIRMWARE_VERSION="${CURR_VER} (git-${CURR_SHA})"
ProjectFirmwareVersionDate="${CURR_VER} (${CURR_BRANCH} / git-${CURR_SHA} build ${CURR_BUILD}), $(date '+%b %d, %Y')"
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
	rm -f "${ROOTFS_DIR}/root/mtlk/web/remote_access.cgi" "${ROOTFS_DIR}/root/mtlk/web/dump_firmware.cgi"
	[ -f "${ROOTFS_DIR}/root/mtlk/web/cgi-bin/dump_firmware.cgi" ] && chmod +x "${ROOTFS_DIR}/root/mtlk/web/cgi-bin/dump_firmware.cgi" 2>/dev/null || true
	[ -f "${ROOTFS_DIR}/root/mtlk/web/cgi-bin/remote_access.cgi" ] && chmod +x "${ROOTFS_DIR}/root/mtlk/web/cgi-bin/remote_access.cgi" 2>/dev/null || true
	if [ ! -e "${ROOTFS_DIR}/bin/simpdiff" ] && [ -e "${STAR_DIR}/bin/simpdiff" ]; then
		cp -fv "${STAR_DIR}/bin/simpdiff" "${ROOTFS_DIR}/bin/simpdiff"
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
	chmod 755 "$base/proc" "$base/sys" "$base/mnt" "$base/var" 2>/dev/null || true
	chmod 700 "$base/root" "$base/etc/dropbear" 2>/dev/null || true
	chown -R 0:0 "$base/root" "$base/etc/dropbear" 2>/dev/null || true
	chmod 600 "$base/etc/dropbear/"*_host_key 2>/dev/null || true
	chmod 600 "$base/etc/shadow" 2>/dev/null || true
	chmod 644 "$base/etc/passwd" "$base/etc/group" "$base/etc/inittab" "$base/etc/fstab" "$base/etc/profile" 2>/dev/null || true
}

ensure_rootfs_dirs "${ROOTFS_DIR}"

# Ensure complete device nodes and symlinks are present in rootfs
create_device_nodes() {
	local target_dev="$1"
	mkdir -p "$target_dev" "$target_dev/pts" "$target_dev/shm" "$target_dev/net" "$target_dev/input"

	[ -e "$target_dev/console" ] || mknod "$target_dev/console" c 5 1 2>/dev/null || true
	[ -e "$target_dev/null" ] || mknod "$target_dev/null" c 1 3 2>/dev/null || true
	[ -e "$target_dev/zero" ] || mknod "$target_dev/zero" c 1 5 2>/dev/null || true
	[ -e "$target_dev/random" ] || mknod "$target_dev/random" c 1 8 2>/dev/null || true
	[ -e "$target_dev/urandom" ] || mknod "$target_dev/urandom" c 1 9 2>/dev/null || true
	[ -e "$target_dev/mem" ] || mknod "$target_dev/mem" c 1 1 2>/dev/null || true
	[ -e "$target_dev/kmem" ] || mknod "$target_dev/kmem" c 1 2 2>/dev/null || true
	[ -e "$target_dev/kmsg" ] || mknod -m 644 "$target_dev/kmsg" c 1 11 2>/dev/null || true
	[ -e "$target_dev/ptmx" ] || mknod -m 666 "$target_dev/ptmx" c 5 2 2>/dev/null || true
	[ -e "$target_dev/tty" ] || mknod "$target_dev/tty" c 5 0 2>/dev/null || true

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

	for i in 0 1 2 3; do
		[ -e "$target_dev/ram$i" ] || mknod "$target_dev/ram$i" b 1 $i 2>/dev/null || true
	done
	[ -e "$target_dev/ram" ] || ln -sf ram1 "$target_dev/ram" 2>/dev/null || true
	for i in 0 1 2 3 4 5 6 7; do
		[ -e "$target_dev/loop$i" ] || mknod "$target_dev/loop$i" b 7 $i 2>/dev/null || true
	done

	for i in 0 1 2 3 4; do
		local mtd_minor=$((i * 2))
		[ -e "$target_dev/mtd$i" ] || mknod "$target_dev/mtd$i" c 90 $mtd_minor 2>/dev/null || true
		[ -e "$target_dev/mtdblock$i" ] || mknod "$target_dev/mtdblock$i" b 31 $i 2>/dev/null || true
	done

	for i in 0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31; do
		[ -e "$target_dev/gpio$i" ] || mknod "$target_dev/gpio$i" c 42 $i 2>/dev/null || true
	done
	rm -f "$target_dev/led3" 2>/dev/null || true
	for i in 0 1 2; do
		[ -e "$target_dev/led$i" ] || mknod "$target_dev/led$i" c 42 $i 2>/dev/null || true
	done
	rm -f "$target_dev/rdflt0" 2>/dev/null || true
	mknod -m 644 "$target_dev/rdflt0" c 42 3 2>/dev/null || mknod -m 644 "$target_dev/rdflt0" c 240 3 2>/dev/null || true

	[ -e "$target_dev/net/tun" ] || mknod "$target_dev/net/tun" c 10 200 2>/dev/null || true

	ln -sf /tmp/log "$target_dev/log" 2>/dev/null || true
	chmod 600 "$target_dev/console" "$target_dev/ttyS0" 2>/dev/null || true
	chmod 666 "$target_dev/null" "$target_dev/zero" "$target_dev/random" "$target_dev/urandom" "$target_dev/tty" "$target_dev/ptmx" 2>/dev/null || true
	chmod 644 "$target_dev"/gpio* "$target_dev"/rdflt* 2>/dev/null || true
	chmod 666 "$target_dev"/led* 2>/dev/null || true
	chmod 660 "$target_dev"/mtd* "$target_dev"/mtdblock* 2>/dev/null || true
	chown -R 0:0 "$target_dev" 2>/dev/null || true
}

create_device_nodes "${ROOTFS_DIR}/dev"

# Ensure init symlinks
ln -sf bin/busybox "${ROOTFS_DIR}/init" 2>/dev/null || true
ln -sf ../bin/busybox "${ROOTFS_DIR}/sbin/init" 2>/dev/null || true
ln -sf bin/busybox "${ROOTFS_DIR}/linuxrc" 2>/dev/null || true
ln -sf busybox "${ROOTFS_DIR}/bin/sh" 2>/dev/null || true

# Strip compile-time static libraries (*.a) from runtime rootfs
rm -f "${ROOTFS_DIR}"/lib/*.a "${ROOTFS_DIR}"/usr/lib/*.a 2>/dev/null || true

echo "--> Generating ext2 ramdisk image from ${ROOTFS_DIR}..."
mkdir -p images
rm -f images/ramdisk_2.6.16.img images/ramdisk_2.6.16.img.lzma images/ramdisk_2.6.16.img.gz

RD=images/ramdisk_2.6.16.img
if command -v genext2fs >/dev/null 2>&1; then
	DEV_TABLE_ARGS=""
	if [ ! -c "${ROOTFS_DIR}/dev/console" ] && [ -f "config/rootfs-device-table.txt" ]; then
		DEV_TABLE_ARGS="-D config/rootfs-device-table.txt"
	fi
	genext2fs -U -b 12288 ${DEV_TABLE_ARGS} -d "${ROOTFS_DIR}" "${RD}" || {
		echo "ERROR: genext2fs failed to generate ext2 ramdisk!"
		exit 1
	}
elif mke2fs -d "${ROOTFS_DIR}" -F -b 1024 -m 0 "${RD}" 12288 >/dev/null 2>&1; then
	:
else
	dd if=/dev/zero of="${RD}" bs=1k count=12288 >/dev/null 2>&1
	mke2fs -F -b 1024 -m 0 "${RD}" >/dev/null 2>&1
	MNT_DIR=/tmp/mnt_rd_$$
	mkdir -p "${MNT_DIR}"
	if mount -o loop "${RD}" "${MNT_DIR}" 2>/dev/null || sudo mount -o loop "${RD}" "${MNT_DIR}" 2>/dev/null; then
		(cp -a "${ROOTFS_DIR}/." "${MNT_DIR}/" || sudo cp -a "${ROOTFS_DIR}/." "${MNT_DIR}/") || {
			echo "ERROR: Failed to copy rootfs to ramdisk mount!"
			umount "${MNT_DIR}" 2>/dev/null || sudo umount "${MNT_DIR}" 2>/dev/null || true
			exit 1
		}
		ensure_rootfs_dirs "${MNT_DIR}"
		create_device_nodes "${MNT_DIR}/dev"
		(chown -R 0:0 "${MNT_DIR}" || sudo chown -R 0:0 "${MNT_DIR}") 2>/dev/null || true
		umount "${MNT_DIR}" 2>/dev/null || sudo umount "${MNT_DIR}" || {
			echo "ERROR: Failed to unmount ramdisk mount!"
			exit 1
		}
		rmdir "${MNT_DIR}" 2>/dev/null || true
	else
		echo "ERROR: Failed to generate ext2 ramdisk (neither genext2fs nor loop mount worked)!"
		exit 1
	fi
fi

if [ ! -f "${RD}" ] || [ $(stat -c %s "${RD}" 2>/dev/null || echo 0) -lt 1000000 ]; then
	echo "ERROR: Ramdisk image ${RD} is missing or empty!"
	exit 1
fi

echo "--> Compressing ramdisk image with LZMA..."
lzma -f -z "${RD}"

if [ ! -f "${RD}.lzma" ]; then
	echo "ERROR: Failed to compress ${RD} with LZMA!"
	exit 1
fi

echo "--> Ramdisk generation complete: images/ramdisk_2.6.16.img.lzma ($(ls -lh images/ramdisk_2.6.16.img.lzma | awk '{print $5}'))"
