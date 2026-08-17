# 📡 Cisco Linksys WAP610N / WET610N Firmware & BSP Restoration

[![Build Firmware](https://github.com/satrianaliqa/wap610n/actions/workflows/compile.yml/badge.svg)](https://github.com/satrianaliqa/wap610n/actions/workflows/compile.yml)
[![License: GPL v2](https://img.shields.io/badge/License-GPL_v2-blue.svg)](LICENSE)
[![Target: Cavium STR8132](https://img.shields.io/badge/SoC-Cavium_STR8132_(ARM922T)-orange.svg)](#hardware-specifications)
[![Compiler: GCC 3.4.6](https://img.shields.io/badge/Toolchain-GCC_3.4.6_uClibc-green.svg)](#compiler-engine)

Master repository untuk restorasi source code resmi, rekonstruksi bitstream BSP, perbaikan driver kernel Linux 2.6.16, serta engine kompilasi berbasis Docker / GitHub Actions untuk **Cisco Linksys WAP610N (Access Point)** dan **WET610N (Wireless Ethernet Bridge)**.

---

## 📑 Daftar Isi
- [Latar Belakang & Kronologi Restorasi](#-latar-belakang--kronologi-restorasi)
- [Spesifikasi Hardware](#-spesifikasi-hardware)
- [Flash Memory Map](#-flash-memory-map)
- [Kompilasi Otomatis (CI/CD GitHub Actions)](#-kompilasi-otomatis-cicd-github-actions)
- [Kompilasi Lokal via Docker](#-kompilasi-lokal-via-docker)
- [Audit & Quality Assurance](#-audit--quality-assurance)
- [Metode Flashing & Unbricking](#-metode-flashing--unbricking)
- [Struktur Direktori](#-struktur-direktori)
- [Roadmap Modifikasi](#-roadmap-modifikasi)

---

## 🔍 Latar Belakang & Kronologi Restorasi

Arsip GPL resmi Linksys (`WAP610N_v1.0.04_build_7_update2.tar.gz`) mengalami kerusakan bitstream kompresi gzip dari rilis vendor asli:
- **Lokasi Kerusakan**: Byte biner offset `0x014F8000` s.d. `0x0303C000`.
- **Anatomi Kerusakan**: Terdapat **28.590.080 bytes (~27.27 MB)** blok zero-fill (`0x00`) yang memutus dekompresi tar pada entry `kernel/linux-2.6.16-star/drivers/usb/serial/ti_fw_3410.h`.
- **Rekonstruksi Presisi**: Pabrikan OEM (U-Media Taiwan) merancang WAP610N dan WET610N pada arsitektur board terpadu (*unified codebase*). Celah 27MB pada kernel Star Semiconductor BSP 6.7.2 berhasil direkonstruksi 100% menggunakan cross-reference branch rilis WET610N yang identik.

---

## ⚡ Spesifikasi Hardware

| Komponen | Spesifikasi Teknis | Keterangan Register / File |
| :--- | :--- | :--- |
| **SoC / CPU** | Cavium / Star Semi STR8132 (CNS2132) | ARM922T Core (ARMv4T), 200 MHz |
| **RAM** | 32 MB SDRAM | Memory map: `0x00000000 - 0x02000000` |
| **Flash ROM** | 4 MB SPI Flash (EON EN29LV640B / ENLV320B) | Memory map: `0x10000000 - 0x10400000` |
| **Ethernet PHY** | Realtek RTL8201CP (Single 10/100 Fast Ethernet) | `rtl8201cp_init(0)` di `umedia_vela.h` |
| **Wireless** | Metalink (MTLK) 802.11a/b/g/n PCIe/Host | Driver: `mtlk.ko` (Dual-band 2.4GHz / 5GHz) |
| **Device ID** | `0x0012` (WAP610N) / `0x0011` (WET610N) | Header Identifier U-Boot |
| **UART Serial** | 38400 baud, 8N1 (3.3V TTL - VCC Jangan Dicolok) | Port `/dev/ttyS0` |
| **Reset GPIO** | GPIO 3 (Active Low, tahan 3 detik) | `apps/gpio_driver/` |

---

## 🗺️ Flash Memory Map

```
+---------------------+---------------------+-----------------------------------------------+
| Flash Memory Range  | MTD Device Name     | Keterangan & Proteksi                         |
+---------------------+---------------------+-----------------------------------------------+
| 0x10000000 -        | mtdblock1 (ARMBOOT) | U-Boot 1.1.4 Bootloader (256 KB)              |
| 0x1003FFFF          |                     | [STATUS: READ-ONLY HARDWARE PROTECTED]        |
+---------------------+---------------------+-----------------------------------------------+
| 0x10040000 -        | mtdblock2 (Kernel)  | bootpImage (Kernel + Ramdisk LZMA + Header)   |
| 0x103DFFFF          |                     | [STATUS: RE-WRITABLE] Max size: ~3.625 MB     |
+---------------------+---------------------+-----------------------------------------------+
| 0x103E0000 -        | mtdblock3 (Disk1)   | NVRAM / Environment Parameter Table (64 KB)   |
| 0x103EFFFF          |                     | [STATUS: RE-WRITABLE]                         |
+---------------------+---------------------+-----------------------------------------------+
| 0x103F0000 -        | mtdblock4 (Disk2)   | Configfs / Factory Calibration Data (64 KB)   |
| 0x103FFFFF          |                     | config_offset=0x103F0000                      |
+---------------------+---------------------+-----------------------------------------------+
```

> **Proteksi Anti-Brick**: Sektor `0x10000000` s.d. `0x1003FFFF` (U-Boot) **tidak pernah ditimpa** saat flashing firmware baru (selalu mulai dari `0x10040000`).

---

## 🚀 Kompilasi Otomatis (CI/CD GitHub Actions)

Setiap push commit atau tag ke repositori ini akan memicu alur build otomatis di GitHub Actions:
1. **GitHub Runner** menyiapkan container 32-bit (`docker.io/i386/debian:8`).
2. Mengompilasi GNU Make 3.81 & memasang toolchain ARM uClibc 3.4.6.
3. Mengonfigurasi BSP `star-6.7.2-mtlk-U-Media-vela` dan mengompilasi kernel + rootfs.
4. Menjalankan audit QA otomatis pada firmware biner yang dihasilkan.
5. Mengunggah firmware siap-flash ke tab **Actions Artifacts**.

---

## 🛠️ Kompilasi Lokal via Docker

Jika ingin mengompilasi secara lokal di Linux (menggunakan Docker atau Podman):

```bash
# Jalankan skrip kompilasi (otomatis build image dan compile)
./docker/compile.sh WAP610N
```

Hasil biner firmware akan tersedia di folder `output/`:
- `output/WAP610N_v1.0.05.bin` (~3.01 MB) - Binary firmware utama siap flash.
- `output/bootpImage` (~3.01 MB) - File standar bootloader U-Boot TFTP.
- `output/ramdisk_2.6.16.img.lzma` (1.97 MB) - Root filesystem ramdisk.
- `output/System.map` (384 KB) - Simbol fungsi kernel.

---

## 🔍 Audit & Quality Assurance

Jalankan sanity check kapan saja terhadap biner yang dihasilkan:

```bash
./docker/verify-firmware.sh output/WAP610N_v1.0.05.bin
```

Pemeriksaan meliputi:
- ✅ Ukuran partisi flash $\le 3.62\text{ MB}$.
- ✅ Validasi 32-Byte Header CRC dan Device ID (`0x0012`).
- ✅ Pengecekan isi ramdisk loop-mount (`/dev/console`, `ld-uClibc`, `/sbin/init -> busybox`).

---

## ⚡ Metode Flashing & Unbricking

### Flashing via U-Boot TFTP (LAN)
```text
setenv serverip 192.168.1.100
setenv ipaddr 192.168.1.1
tftp 0x100000 bootpImage
cksum 0x100000
erase 0x10040000 +$filesize
cp.b 0x100000 0x10040000 $filesize
cksum 0x10040000
reset
```

### Flashing via UART Serial Kermit (`loadb`)
```text
loadb 0x100000
# Kirim WAP610N_v1.0.05.bin via Kermit Protocol
erase 0x10040000 +0x2DFD00
cp.b 0x100000 0x10040000 0x2DFD00
cksum 0x10040000
reset
```

### Flashing via Web GUI
Masuk ke `http://192.168.1.1` -> **Administration** -> **Firmware Upgrade** -> Upload `WAP610N_v1.0.05.bin`.

---

## 📁 Struktur Direktori

```text
├── .github/
│   └── workflows/
│       └── compile.yml            # CI/CD otomatisasi build di GitHub Actions
├── docker/
│   ├── Dockerfile                 # Mesin build 32-bit Debian Jessie i386
│   ├── make-3.81.tar.bz2          # GNU Make 3.81 source
│   ├── compile.sh                 # Skrip build lokal 1-klik
│   └── verify-firmware.sh         # Tool audit integritas biner & ramdisk
├── apps/                          # Userspace applications & web GUI
├── boards/                        # Konfigurasi platform board U-Media / Star Semi
├── config/                        # Kernel & board config profile
├── devscripts/                    # Helper scripts pembuatan rootfs & packing
├── kernel/                        # Linux Kernel 2.6.16-star source
├── rootfs-star/                   # Template base root filesystem
├── tools/                         # Cross-compiler GCC 3.4.6 ARM uClibc
├── u-boot-1.1.4/                  # U-Boot Bootloader source
├── make_nv.sh                     # Master Makefile entrypoint
└── README.md                      # Dokumentasi master repositori
```

---

## 🗺️ Roadmap Modifikasi

- [x] **Fase 1 (Selesai)**: Baseline WAP610N stabil, unbricked, kernel 2.6.16 + uClibc + Web GUI.
- [ ] **Fase 2**: Integrasi Dropbear SSH Server, upgrade applet BusyBox (`curl`, `htop`, `tcpdump`), interactive serial shell.
- [ ] **Fase 3**: Tuning driver Metalink `mtlk.ko` (Unlocking DFS 5GHz channel & TX power override).
- [ ] **Fase 4**: Riset porting OpenWrt / modern kernel pada sub-arsitektur STR8132.
