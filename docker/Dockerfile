FROM docker.io/i386/debian:8

ENV DEBIAN_FRONTEND=noninteractive

# Configure archive repositories for Debian 8 Jessie
RUN echo "deb [check-valid-until=no] http://archive.debian.org/debian jessie main contrib non-free" > /etc/apt/sources.list && \
    echo "deb [check-valid-until=no] http://archive.debian.org/debian-security jessie/updates main contrib non-free" >> /etc/apt/sources.list && \
    echo 'Acquire::Check-Valid-Until "false";' > /etc/apt/apt.conf.d/99no-check-valid-until && \
    echo 'Acquire::AllowInsecureRepositories "true";' > /etc/apt/apt.conf.d/99allow-insecure && \
    apt-get update && \
    apt-get install -y --force-yes \
        build-essential \
        gcc \
        g++ \
        bison \
        flex \
        lzma \
        zlib1g-dev \
        e2fsprogs \
        libncurses5-dev \
        bc \
        file \
        subversion \
        dosfstools \
        genromfs \
        sudo \
        tar \
        gzip \
        bzip2 \
        patch \
        locales \
        vim \
        tree \
    && apt-get clean

# Copy and build GNU Make 3.81 (100% compatible with Linux 2.6.16 and Busybox 1.8.1 Makefiles)
COPY make-3.81.tar.bz2 /tmp/
RUN cd /tmp && \
    tar -xjf make-3.81.tar.bz2 && \
    cd make-3.81 && \
    ./configure --prefix=/usr && \
    make -j4 && \
    make install && \
    rm -rf /tmp/make-3.81*

# Generate locale
RUN echo "en_US.UTF-8 UTF-8" > /etc/locale.gen && locale-gen
ENV LANG=en_US.UTF-8
ENV LC_ALL=en_US.UTF-8
ENV PATH="/workspace/tools/arm-uclibc-3.4.6/bin:${PATH}"

# Allow passwordless sudo for root / any user
RUN echo "ALL ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers

WORKDIR /workspace

CMD ["/bin/bash"]
