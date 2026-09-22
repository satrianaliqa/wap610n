#!/bin/bash
# Build Prerequisites Validation Script
# Purpose: Verify all required files and configurations before compilation
# This ensures Docker builds and local compilation won't fail midway through

set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$DIR/.." && pwd)"

echo "=========================================================="
echo " 🔍 Validating WAP610N/WET610N Build Prerequisites"
echo "=========================================================="
echo ""

ERRORS=0

# Color codes
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Helper functions
check_file() {
    local file="$1"
    local description="$2"
    
    if [ -f "$file" ]; then
        echo -e "${GREEN}✓${NC} $description: $(ls -lh "$file" | awk '{print $5}')"
        return 0
    else
        echo -e "${RED}✗${NC} $description: NOT FOUND at $file"
        ((ERRORS++))
        return 1
    fi
}

check_dir() {
    local dir="$1"
    local description="$2"
    
    if [ -d "$dir" ]; then
        echo -e "${GREEN}✓${NC} $description: exists ($(find "$dir" -type f | wc -l) files)"
        return 0
    else
        echo -e "${RED}✗${NC} $description: NOT FOUND at $dir"
        ((ERRORS++))
        return 1
    fi
}

check_executable() {
    local exe="$1"
    local description="$2"
    
    if command -v "$exe" >/dev/null 2>&1; then
        VERSION=$(command -v "$exe" 2>/dev/null || echo "unknown")
        echo -e "${GREEN}✓${NC} $description: $VERSION"
        return 0
    else
        echo -e "${RED}✗${NC} $description: NOT FOUND in PATH"
        ((ERRORS++))
        return 1
    fi
}

# === 1. Check Archive Files ===
echo "1️⃣  Checking Archive Files..."
check_file "$ROOT_DIR/wlan.tar.gz" "  Wireless driver archive (wlan.tar.gz)"
check_file "$ROOT_DIR/rootfs-star.tgz" "  Pre-built rootfs (rootfs-star.tgz)"
echo ""

# === 2. Check Source Directories ===
echo "2️⃣  Checking Source Directories..."
check_dir "$ROOT_DIR/kernel/linux-2.6.16-star" "  Linux kernel 2.6.16-star"
check_dir "$ROOT_DIR/apps" "  Applications source"
check_dir "$ROOT_DIR/boards/star-6.7.2-mtlk-U-Media-vela" "  Board configuration (star-6.7.2-mtlk)"
check_dir "$ROOT_DIR/u-boot-1.1.4" "  U-Boot bootloader"
check_dir "$ROOT_DIR/tools/arm-uclibc-3.4.6" "  ARM GCC 3.4.6 toolchain"
echo ""

# === 3. Check Config Files ===
echo "3️⃣  Checking Configuration Files..."
check_file "$ROOT_DIR/config/WAP610N_VERSION" "  WAP610N version info"
check_file "$ROOT_DIR/config/WET610N_VERSION" "  WET610N version info"
check_file "$ROOT_DIR/Makefile" "  Top-level Makefile"
check_file "$ROOT_DIR/make_nv.sh" "  Build orchestration script"
check_file "$ROOT_DIR/boards/star-6.7.2-mtlk-U-Media-vela/kernel/.config" "  Kernel .config"
check_file "$ROOT_DIR/boards/star-6.7.2-mtlk-U-Media-vela/busybox/.config" "  BusyBox .config"
echo ""

# === 4. Check Build Scripts ===
echo "4️⃣  Checking Build Scripts..."
check_file "$ROOT_DIR/docker/compile.sh" "  Docker compile script"
check_file "$ROOT_DIR/docker/Dockerfile" "  Docker build definition"
check_file "$ROOT_DIR/docker/verify-firmware.sh" "  Firmware verification script"
check_file "$ROOT_DIR/.github/workflows/compile.yml" "  GitHub Actions CI workflow"
echo ""

# === 5. Check Toolchain ===
echo "5️⃣  Checking Toolchain Executables..."
if command -v docker >/dev/null 2>&1 || command -v podman >/dev/null 2>&1; then
    if command -v docker >/dev/null 2>&1; then
        echo -e "${GREEN}✓${NC} Docker container runtime: available"
    else
        echo -e "${GREEN}✓${NC} Podman container runtime: available"
    fi
else
    echo -e "${RED}✗${NC} Container runtime (docker/podman): NOT FOUND"
    ((ERRORS++))
fi
echo ""

# === 6. Check Required Commands ===
echo "6️⃣  Checking System Requirements..."
check_executable "bash" "  Bash shell"
check_executable "make" "  GNU Make"
check_executable "gcc" "  GCC compiler"
check_executable "tar" "  tar utility"
check_executable "gzip" "  gzip compression"
check_executable "find" "  find utility"
echo ""

# === 7. Verify Output Directory ===
echo "7️⃣  Checking Output Directory..."
if [ ! -d "$ROOT_DIR/output" ]; then
    echo -e "${YELLOW}⚠${NC} Output directory does not exist (will be created during build)"
    mkdir -p "$ROOT_DIR/output" || {
        echo -e "${RED}✗${NC} Failed to create output directory"
        ((ERRORS++))
    }
else
    echo -e "${GREEN}✓${NC} Output directory exists"
fi
echo ""

# === Summary ===
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
