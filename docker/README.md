# 🐳 Container Build Engine, CI/CD & Firmware QA Verification

Lingkungan kompilasi 32-bit terisolasi berbasis **Debian 8 (Jessie i386)** untuk toolchain legacy GCC 3.4.6 ARM uClibc, beserta gerbang audit biner otomatis sebelum flashing.

---

## 1. Arsitektur Container Build (`docker/Dockerfile`)

- Base image: `docker.io/i386/debian:8` (dibutuhkan karena binary host `arm-linux-uclibc-gcc` 3.4.6 dikompilasi untuk ABI i386 32-bit).
- Membangun **GNU Make 3.81** dari `docker/make-3.81.tar.bz2` agar sintaks Makefile kernel `2.6.16-star` berjalan deterministik.
- Memasang `genext2fs`, `lzma`, `e2fsprogs`, `zlib1g-dev`, dan `sudo` idempotent.

---

## 2. Kompilasi Lokal via Docker

```bash
# Build container (jika belum ada) dan jalankan full build WAP610N + audit QA
./docker/compile.sh WAP610N

# Atau melalui top-level Makefile:
make docker-image
make docker-shell   # Masuk ke shell interaktif container 32-bit
```

Artifak hasil build tersimpan di `output/`:
- `output/WAP610N_v1.0.05.bin` — Binary firmware utama siap flash via WebUI.
- `output/bootpImage` — Identik dengan `.bin` untuk U-Boot TFTP (`0x10040000`).
- `output/ramdisk_2.6.16.img.lzma` — RootFS ext2 terkompresi LZMA.
- `output/System.map` — Simbol fungsi kernel Linux 2.6.16-star.

---

## 3. Pipeline GitHub Actions (`.github/workflows/compile.yml`)

Setiap `push` (branch `master`, `main`, `testing`, atau tag `v*`) dan `pull_request` otomatis menjalankan:
1. **Image Cache (`actions/cache@v4`)**: Menyimpan dan memuat ulang `/tmp/wap610n-builder.tar.gz` berdasarkan hash `docker/Dockerfile` & `docker/build-builder.sh`.
2. **Pre-Flight Check**: Menjalankan `./devscripts/validate-build-prerequisites.sh`.
3. **Container Build**: Menyiapkan `rootfs/` bersih dari `rootfs-star/`, membangun `mkcsum`, dan menjalankan `make_nv.sh reconf` + `make_nv.sh build vendor/mtlk WAP610N`.
4. **QA Gate & Telemetry**: Menjalankan `./docker/verify-firmware.sh` dan mencetak ukuran + headroom flash ke GitHub Step Summary.
5. **Artifact Upload**: Mengunggah `WAP610N_v*.bin`, `ramdisk_2.6.16.img.lzma`, dan `System.map`.

---

## 4. Automated QA Gatekeeper (`./docker/verify-firmware.sh`)

```bash
./docker/verify-firmware.sh output/WAP610N_v1.0.05.bin
```

Enam tahap verifikasi wajib:
1. **ARM Branch & Flash Partition Limit**: Magic `0xEA000006` dan ukuran `<= 3,801,088 bytes` (`3.625 MB` / `0x3A0000`).
2. **Device ID Header**: Memastikan Word 3 = `0x0012` (WAP610N Resmi).
3. **CRC32 Checksum**: Memastikan CRC32 header sama persis dengan trailer 4 byte akhir.
4. **Ramdisk Loop-Mount Inspection**:
   - Memverifikasi `/dev/{console,kmsg,gpio*,rdflt0}` (`rdflt0` minor `3`), `/etc/shadow` (`0600`), shared libs (`ld-uClibc-0.9.29.so`, `libssl`, `libexpat`), `/sbin/init -> busybox`.
   - Memastikan `/usr/sbin/dropbear` ada dan `/root/mtlk/web/webs` mengandung marker `"HTTP listener initialized"` (bukan blob 2011).
   - Memastikan zero backdoor CGI (`shell.cgi`, symlink root CGI).
   - Memverifikasi marker kustomisasi WAP610N (`min_free_kbytes`, `echo 1 > /dev/led0`, `Access Point Information`, `objectDisplay("freqBand", value)`).
   - Memeriksa sintaks seluruh skrip shell di dalam ramdisk dan mendeteksi zero host-path leak (`/home/satszz`, `/workspaces/wap610n`).
5. **Host Shell Syntax Check**: `sh -n` / `bash -n` pada seluruh skrip di `rootfs-star/`.
6. **Flash & Ramdisk Telemetry**: Mencetak ukuran `bootpImage`, `ramdisk_2.6.16.img.lzma`, sisa headroom flash (KB), serta Top 10 file terbesar di ramdisk.
