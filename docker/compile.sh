#!/bin/bash
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$DIR/.." && pwd)"
TARGET_NAME="${1:-WAP610N}"
CURRENT_UID="$(id -u)"
CURRENT_GID="$(id -g)"

if command -v docker >/dev/null 2>&1; then
    CONTAINER_CMD="docker"
elif command -v podman >/dev/null 2>&1; then
    CONTAINER_CMD="podman"
else
    echo "❌ Error: Neither docker nor podman found."
    exit 1
fi

echo "=========================================================="
echo " 🔨 Ensuring wap610n-builder container image..."
echo "=========================================================="
if ! $CONTAINER_CMD image inspect wap610n-builder >/dev/null 2>&1; then
    "$DIR/build-builder.sh"
else
    echo "✅ Image wap610n-builder already exists."
fi

# Validate prerequisites
echo "Validating prerequisites..."
if [ ! -f "$ROOT_DIR/vendor/mtlk/driver/mtlk.ko" ]; then
    echo "❌ ERROR: vendor/mtlk/driver/mtlk.ko not found"
    exit 1
fi
if [ ! -d "$ROOT_DIR/rootfs-star" ]; then
    echo "❌ ERROR: rootfs-star directory not found"
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

EXTRA_ENV_ARGS=()
for ev in FIRMWARE_VERSION MAJOR MINOR PATCH BUILD; do
    if [ -n "${!ev:-}" ]; then
        EXTRA_ENV_ARGS+=(-e "${ev}=${!ev}")
    fi
done
while IFS='=' read -r ev _; do
    case "$ev" in
        CUSTOM_*)
            if [ -n "${!ev:-}" ]; then
                EXTRA_ENV_ARGS+=(-e "${ev}=${!ev}")
            fi
            ;;
    esac
done < <(env)

sudo $CONTAINER_CMD run --rm \
    --privileged \
    -e TARGET_NAME="$TARGET_NAME" \
    -e CURRENT_UID="$CURRENT_UID" \
    -e CURRENT_GID="$CURRENT_GID" \
    "${EXTRA_ENV_ARGS[@]}" \
    -v "$ROOT_DIR:/workspace:z" \
    -w /workspace \
    wap610n-builder /bin/bash -c '
        set -e
        export PATH="/workspace/tools/arm-uclibc-3.4.6/bin:$PATH"
        mkdir -p /workspace/output

        echo "--> Step 1: Preparing clean staging rootfs from rootfs-star..."
        [ -f /workspace/vendor/mtlk/driver/mtlk.ko ] || { echo "ERROR: vendor/mtlk/driver/mtlk.ko not found"; exit 1; }
        [ -d /workspace/rootfs-star ] || { echo "ERROR: rootfs-star not found"; exit 1; }

        rm -rf rootfs
        cp -a rootfs-star rootfs

        ln -sf bin/busybox rootfs/init 2>/dev/null || true
        ln -sf ../bin/busybox rootfs/sbin/init 2>/dev/null || true
        ln -sf bin/busybox rootfs/linuxrc 2>/dev/null || true
        ln -sf busybox rootfs/bin/sh 2>/dev/null || true

        echo "--> Step 2: Configuring board target star-6.7.2-mtlk-U-Media-vela..."
        ./make_nv.sh reconf star-6.7.2-mtlk-U-Media-vela

        echo "--> Step 3: Compiling Kernel, Applications, and building Firmware..."
        ./make_nv.sh build vendor/mtlk "$TARGET_NAME"

        VER_FILE="config/${TARGET_NAME}_VERSION"
        [ -f "$VER_FILE" ] || VER_FILE="config/WAP610N_VERSION"
        if [ -e output/bootpImage ]; then
            VERSION="${FIRMWARE_VERSION:-}"
            if [ -z "$VERSION" ] && [ -f "$VER_FILE" ]; then
                VERSION=$(grep "FIRMWARE_VERSION" "$VER_FILE" | cut -d"\"" -f2)
            fi
            VERSION="${VERSION:-1.0.05}"
            BINNAME="${TARGET_NAME}_v${VERSION}.bin"
            echo "--> Copying bootpImage to output/$BINNAME"
            cp -vf output/bootpImage "output/$BINNAME"
        fi

        echo "--> Fixing permissions for workspace output..."
        chown -R $CURRENT_UID:$CURRENT_GID /workspace/output 2>/dev/null || true
        chown -R $CURRENT_UID:$CURRENT_GID /workspace/images 2>/dev/null || true
    '

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
