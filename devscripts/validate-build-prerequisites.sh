#!/bin/bash
# Build Prerequisites Validation Script (Unified WAP610N Platform)
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$DIR/.." && pwd)"

echo "=========================================================="
echo " 🔍 Validating WAP610N Build Prerequisites"
echo "=========================================================="
echo ""

ERRORS=0
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

check_file() {
    local file="$1"
    local description="$2"
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓${NC} $description: $(ls -lh "$file" | awk '{print $5}')"
    else
        echo -e "${RED}✗${NC} $description: NOT FOUND at $file"
        ERRORS=$((ERRORS + 1))
    fi
}

check_dir() {
    local dir="$1"
    local description="$2"
    if [ -d "$dir" ]; then
        echo -e "${GREEN}✓${NC} $description: exists ($(find "$dir" -type f | wc -l) files)"
    else
        echo -e "${RED}✗${NC} $description: NOT FOUND at $dir"
        ERRORS=$((ERRORS + 1))
    fi
}

check_executable() {
    local exe="$1"
    local description="$2"
    if command -v "$exe" >/dev/null 2>&1; then
        echo -e "${GREEN}✓${NC} $description: $(command -v "$exe")"
    else
        echo -e "${RED}✗${NC} $description: NOT FOUND in PATH"
        ERRORS=$((ERRORS + 1))
    fi
}

echo "1️⃣  Checking Open Vendor & RootFS Trees..."
check_dir "$ROOT_DIR/vendor/mtlk" "  Open Metalink vendor blobs (vendor/mtlk/)"
check_file "$ROOT_DIR/vendor/mtlk/driver/mtlk.ko" "  Metalink wireless driver (mtlk.ko)"
check_file "$ROOT_DIR/vendor/mtlk/images/ap_upper.bin" "  Metalink radio firmware (ap_upper.bin)"
check_file "$ROOT_DIR/vendor/mtlk/etc/drvhlpr" "  Metalink driver helper (drvhlpr)"
check_dir "$ROOT_DIR/rootfs-star" "  Single source of truth RootFS (rootfs-star/)"
echo ""

echo "2️⃣  Checking Source Directories..."
check_dir "$ROOT_DIR/kernel/linux-2.6.16-star" "  Linux kernel 2.6.16-star"
check_dir "$ROOT_DIR/apps" "  Applications source"
check_dir "$ROOT_DIR/boards/star-6.7.2-mtlk-U-Media-vela" "  Board configuration (star-6.7.2-mtlk)"
check_dir "$ROOT_DIR/u-boot-1.1.4" "  U-Boot bootloader"
check_dir "$ROOT_DIR/tools/arm-uclibc-3.4.6" "  ARM GCC 3.4.6 toolchain"
echo ""

echo "3️⃣  Checking Configuration Files..."
check_file "$ROOT_DIR/config/WAP610N_VERSION" "  WAP610N version info"
check_file "$ROOT_DIR/config/mtdblock4-defaults/sys.conf" "  Default sys.conf (mtdblock4)"
check_file "$ROOT_DIR/config/mtdblock4-defaults/wlan0.conf" "  Default wlan0.conf (mtdblock4)"
check_file "$ROOT_DIR/config/rootfs-device-table.txt" "  Rootless genext2fs device table"
check_file "$ROOT_DIR/Makefile" "  Top-level Makefile"
check_file "$ROOT_DIR/make_nv.sh" "  Build orchestration script"
check_file "$ROOT_DIR/boards/star-6.7.2-mtlk-U-Media-vela/kernel/.config" "  Kernel .config"
check_file "$ROOT_DIR/boards/star-6.7.2-mtlk-U-Media-vela/busybox/.config" "  BusyBox .config"
echo ""

echo "4️⃣  Checking Build Scripts..."
check_file "$ROOT_DIR/docker/compile.sh" "  Docker compile script"
check_file "$ROOT_DIR/docker/Dockerfile" "  Docker build definition"
check_file "$ROOT_DIR/docker/verify-firmware.sh" "  Firmware verification script"
check_file "$ROOT_DIR/.github/workflows/compile.yml" "  GitHub Actions CI workflow"
echo ""

echo "5️⃣  Checking Container Runtime..."
if command -v docker >/dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Docker container runtime: available"
elif command -v podman >/dev/null 2>&1; then
    echo -e "${GREEN}✓${NC} Podman container runtime: available"
else
    echo -e "${RED}✗${NC} Container runtime (docker/podman): NOT FOUND"
    ERRORS=$((ERRORS + 1))
fi
echo ""

echo "6️⃣  Checking System Requirements..."
check_executable "bash" "  Bash shell"
check_executable "make" "  GNU Make"
check_executable "gcc" "  GCC compiler"
check_executable "tar" "  tar utility"
check_executable "gzip" "  gzip compression"
check_executable "find" "  find utility"
echo ""

echo "7️⃣  Checking Output Directory..."
if [ ! -d "$ROOT_DIR/output" ]; then
    echo -e "${YELLOW}⚠${NC} Output directory does not exist (will be created during build)"
    mkdir -p "$ROOT_DIR/output" || {
        echo -e "${RED}✗${NC} Failed to create output directory"
        ERRORS=$((ERRORS + 1))
    }
else
    echo -e "${GREEN}✓${NC} Output directory exists"
fi
echo ""

echo "=========================================================="
if [ $ERRORS -eq 0 ]; then
    echo -e "${GREEN}✅ All prerequisites validated successfully!${NC}"
    echo "You can now run: ./docker/compile.sh WAP610N"
    exit 0
else
    echo -e "${RED}❌ Found $ERRORS prerequisite issue(s)!${NC}"
    echo "Please fix the above issues before proceeding."
    exit 1
fi
