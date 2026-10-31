#!/bin/bash
# -----------------------------------------------------------------------------
# wap610n-builder Image Creator
# Builds the 32-bit Debian 8 Jessie build container using privileged runtime
# to completely bypass host seccomp restrictions on legacy i386 socketcall(2).
# -----------------------------------------------------------------------------
set -e

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IMAGE_NAME="wap610n-builder"
SETUP_CONTAINER="wap610n-setup-$(date +%s)"

echo "=========================================================="
echo " 🔨 Preparing $IMAGE_NAME container environment..."
echo "=========================================================="

# Cleanup any stale setup container
docker rm -f "$SETUP_CONTAINER" 2>/dev/null || true

# Run setup in a privileged 32-bit container
docker run --name "$SETUP_CONTAINER" --privileged \
    -v "$DIR/make-3.81.tar.bz2:/tmp/make-3.81.tar.bz2:ro" \
    i386/debian:8 /bin/bash -c '
        set -e
        export DEBIAN_FRONTEND=noninteractive

        # Configure archive repositories
        echo "deb [check-valid-until=no] http://archive.debian.org/debian jessie main contrib non-free" > /etc/apt/sources.list
        echo "deb [check-valid-until=no] http://archive.debian.org/debian-security jessie/updates main contrib non-free" >> /etc/apt/sources.list
        echo "Acquire::Check-Valid-Until \"false\";" > /etc/apt/apt.conf.d/99no-check-valid-until
        echo "Acquire::AllowInsecureRepositories \"true\";" > /etc/apt/apt.conf.d/99allow-insecure
        echo "Acquire::ForceIPv4 \"true\";" > /etc/apt/apt.conf.d/99force-ipv4
        echo "Acquire::Retries \"5\";" > /etc/apt/apt.conf.d/99retries

        # Update package indexes and install required dependencies
        apt-get update
        apt-get install -y --force-yes \
            build-essential gcc g++ bison flex lzma zlib1g-dev e2fsprogs \
            libncurses5-dev bc file subversion dosfstools genromfs genext2fs \
            sudo tar gzip bzip2 patch locales vim tree
        apt-get clean

        # Build & Install GNU Make 3.81
        cd /tmp
        tar -xjf make-3.81.tar.bz2
        cd make-3.81
        ./configure --prefix=/usr
        make -j4
        make install
        cd /tmp
        rm -rf /tmp/make-3.81

        # Generate English UTF-8 locale
        echo "en_US.UTF-8 UTF-8" > /etc/locale.gen
        locale-gen

        # Ensure passwordless sudo
        grep -q "NOPASSWD: ALL" /etc/sudoers || echo "ALL ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
    '

echo "💾 Committing container to $IMAGE_NAME image..."
docker commit \
    --change 'ENV DEBIAN_FRONTEND=noninteractive' \
    --change 'ENV LANG=en_US.UTF-8' \
    --change 'ENV LC_ALL=en_US.UTF-8' \
    --change 'ENV PATH=/workspace/tools/arm-uclibc-3.4.6/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin' \
    --change 'WORKDIR /workspace' \
    --change 'CMD ["/bin/bash"]' \
    "$SETUP_CONTAINER" "$IMAGE_NAME"

docker rm -f "$SETUP_CONTAINER" 2>/dev/null || true

echo "✅ $IMAGE_NAME ready!"
