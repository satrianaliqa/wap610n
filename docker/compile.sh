#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$DIR/.." && pwd)"
TARGET_NAME="${1:-WAP610N}"
CURRENT_UID="$(id -u)"
CURRENT_GID="$(id -g)"

# Detect container runtime (docker or podman)
if command -v docker >/dev/null 2>&1; then
    CONTAINER_CMD="docker"
elif command -v podman >/dev/null 2>&1; then
    CONTAINER_CMD="podman"
else
    echo "❌ Error: Neither docker nor podman found."
    exit 1
fi

echo "=========================================================="
echo " 🔨 Building wap610n-builder container image..."
echo "=========================================================="
$CONTAINER_CMD build -t wap610n-builder "$DIR"

# Validate prerequisites
echo "Validating prerequisites..."
if [ ! -f "$ROOT_DIR/wlan.tar.gz" ]; then
    echo "❌ ERROR: wlan.tar.gz not found at $ROOT_DIR/wlan.tar.gz"
    exit 1
fi
if [ ! -f "$ROOT_DIR/rootfs-star.tgz" ]; then
    echo "❌ ERROR: rootfs-star.tgz not found at $ROOT_DIR/rootfs-star.tgz"
    exit 1
fi
if [ ! -f "$ROOT_DIR/config/WAP610N_VERSION" ]; then
    echo "❌ ERROR: config/WAP610N_VERSION not found"
    exit 1
fi
mkdir -p "$ROOT_DIR/output"

echo "=========================================================="
echo " 🚀 Starting Full Compilation for $TARGET_NAME in Container"
echo " Workspace: $ROOT_DIR"
echo " Prerequisites: ✅ Validated"
echo "=========================================================="

sudo $CONTAINER_CMD run --rm \
    --privileged \
    -v "$ROOT_DIR:/workspace:z" \
    -w /workspace \
    wap610n-builder /bin/bash -c '
        set -e
        export PATH="/workspace/tools/arm-uclibc-3.4.6/bin:$PATH"
        mkdir -p /workspace/output
        
        echo "--> Step 1: Checking and preparing rootfs..."
        echo "--> Validating required archives..."
        [ -f /workspace/wlan.tar.gz ] || { echo "ERROR: wlan.tar.gz not found"; exit 1; }
        [ -f /workspace/rootfs-star.tgz ] || { echo "ERROR: rootfs-star.tgz not found"; exit 1; }
        if [ ! -d rootfs-star ]; then
            tar -zxvf rootfs-star.tgz
        elif [ ! -e rootfs-star/dev/console ]; then
            echo "--> Extracting device nodes from rootfs-star.tgz..."
            tar -zxvf rootfs-star.tgz rootfs-star/dev 2>/dev/null || true
        fi
        if [ ! -e rootfs ]; then
            ln -sf rootfs-star rootfs
        fi

        # Ensure init symlinks
        ln -sf bin/busybox rootfs/init 2>/dev/null || true
        ln -sf ../bin/busybox rootfs/sbin/init 2>/dev/null || true
        ln -sf bin/busybox rootfs/linuxrc 2>/dev/null || true
        ln -sf busybox rootfs/bin/sh 2>/dev/null || true

        echo "--> Step 2: Configuring board target star-6.7.2-mtlk-U-Media-vela..."
        ./make_nv.sh reconf star-6.7.2-mtlk-U-Media-vela

        echo "--> Step 3: Compiling Kernel, Applications, and building Firmware..."
        ./make_nv.sh build wlan.tar.gz $TARGET_NAME

        # Dynamically copy bootpImage with version from config
        if [ -e output/bootpImage ] && [ -f config/WAP610N_VERSION ]; then
            VERSION=$(grep "FIRMWARE_VERSION" config/WAP610N_VERSION | cut -d"\"" -f2)
            BINNAME="WAP610N_v${VERSION}.bin"
            echo "--> Copying bootpImage to output/$BINNAME"
            cp -vf output/bootpImage "output/$BINNAME"
        fi

        echo "--> Fixing permissions for workspace output..."
        chown -R $CURRENT_UID:$CURRENT_GID /workspace/output 2>/dev/null || true
        chown -R $CURRENT_UID:$CURRENT_GID /workspace/images 2>/dev/null || true
    "'

# Audit & Verifikasi hasil compile otomatis (dynamically find firmware)
echo ""
echo "Firmware artifacts generated:"
ls -lh "$ROOT_DIR/output"/ 2>/dev/null || echo "  (output directory not found)"

if [ -f "$ROOT_DIR/docker/verify-firmware.sh" ] && [ -f "$ROOT_DIR/output/bootpImage" ]; then
    echo ""
    echo "Running firmware audit..."
    "$DIR/verify-firmware.sh"
else
    echo "⚠️  Warning: verify-firmware.sh or bootpImage not found, skipping audit"
fi
