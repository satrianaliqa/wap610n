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

echo "=========================================================="
echo " 🚀 Starting Full Compilation for $TARGET_NAME in Container"
echo " Workspace: $ROOT_DIR"
echo "=========================================================="

sudo $CONTAINER_CMD run --rm \
    --privileged \
    -v "$ROOT_DIR:/workspace:z" \
    -w /workspace \
    wap610n-builder /bin/bash -c "
        set -e
        echo '--> Step 1: Checking and preparing rootfs...'
        if [ ! -d rootfs-star ]; then
            tar -zxvf rootfs-star.tgz
        elif [ ! -e rootfs-star/dev/console ]; then
            echo '--> Extracting device nodes from rootfs-star.tgz...'
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

        echo '--> Step 2: Configuring board target star-6.7.2-mtlk-U-Media-vela...'
        ./make_nv.sh reconf star-6.7.2-mtlk-U-Media-vela

        echo '--> Step 3: Compiling Kernel, Applications, and building Firmware & Rootfs ($TARGET_NAME)...'
        ./make_nv.sh build wlan.tar.gz $TARGET_NAME

        # Copy to clean .bin name
        if [ -e output/bootpImage ]; then
            cp -vf output/bootpImage output/WAP610N_v1.0.05.bin
        fi

        echo '--> Fixing permissions for workspace output...'
        chown -R $CURRENT_UID:$CURRENT_GID /workspace/output 2>/dev/null || true
        chown -R $CURRENT_UID:$CURRENT_GID /workspace/images 2>/dev/null || true
    "

# Audit & Verifikasi hasil compile otomatis
if [ -f "$ROOT_DIR/output/WAP610N_v1.0.08.bin" ]; then
    "$DIR/verify-firmware.sh" "$ROOT_DIR/output/WAP610N_v1.0.08.bin"
fi
