# 📡 Cisco Linksys WAP610N Unified Firmware & BSP Platform

[![Build Firmware](https://github.com/satrianaliqa/wap610n/actions/workflows/compile.yml/badge.svg)](https://github.com/satrianaliqa/wap610n/actions/workflows/compile.yml)
[![License: GPL v2](https://img.shields.io/badge/License-GPL_v2-blue.svg)](LICENSE)
[![Target: Cavium STR8132](https://img.shields.io/badge/SoC-Cavium_STR8132_(ARM922T)-orange.svg)](boards/star-6.7.2-mtlk-U-Media-vela/README.md)
[![Compiler: GCC 3.4.6](https://img.shields.io/badge/Toolchain-GCC_3.4.6_uClibc-green.svg)](docker/README.md)

Repositori terpadu (**1 Board `STR8132`, 1 Product `WAP610N`, 1 Source Tree**) untuk restorasi kernel Linux `2.6.16-star`, userspace uClibc, GoAhead WebServer C89, serta pipeline kompilasi & audit Docker / GitHub Actions bagi **Cisco Linksys WAP610N (Dual-Band Access Point)**.

---

## ✨ Fitur Utama

- **Arsitektur Bersih Tanpa Arsip Tertutup**: [`rootfs-star/`](rootfs-star/README.md) sebagai satu-satunya sumber kebenaran RootFS, [`vendor/mtlk/`](vendor/mtlk/README.md) sebagai direktori blob radio terbuka, dan [`config/mtdblock4-defaults/`](config/README.md) untuk auto-generate `config.img.gz` dari plain text.
- **Akses Remote & CLI Diagnostik**: **Dropbear SSH v0.52** (`22` + `scp` + persistent keys di `/mnt/jffs2/dropbear`), **Telnet** (`23`), serta applet `sysmon`, `curl`, `json`, `nettest`, `flashdump`, dan `help`.
- **WebServer C89 Hardened & JSON API**: Perbaikan keamanan & stabilitas penuh pada [`apps/WebServer/`](apps/WebServer/README.md), zero-fork JSON telemetry (`GET /api/status.json`, `GET /api/wifi.json`), selector bandwidth `20 MHz / 40 MHz`, One-Click MTD Flash Dumper, serta pre-flash guard `0x0012` anti-brick.
- **Optimasi RAM 32 MB & Flash 4 MB**: Tuning sysctl (`min_free_kbytes=2048`, `vfs_cache_pressure=150`, socket buffer), watchdog ConfigFS anti-korupsi, dan gerbang batas keras `bootpImage <= 3,801,088 bytes`.

---

## 🚀 Quick Start (Build & Audit)

```bash
# 1. Validasi seluruh prasyarat build
./devscripts/validate-build-prerequisites.sh

# 2. Full compile dalam container 32-bit Debian 8 + audit QA otomatis
./docker/compile.sh WAP610N

# 2b. (Opsional) Override versi & default AP langsung dari CLI atau edit config/WAP610N_VERSION
FIRMWARE_VERSION="1.0.06" CUSTOM_WIFI_SSID="MyAP" ./docker/compile.sh WAP610N

# 3. Verifikasi manual biner hasil build
./docker/verify-firmware.sh output/WAP610N_v1.0.05.bin
```

Target developer granular di [`Makefile`](Makefile): `make image`, `make configure`, `make kernel`, `make apps`, `make webs`, `make pack`, `make verify`, `make docker-image`, `make docker-shell`, `make mtlk-driver`, `make clean`.

---

## ⚡ Default Akses & Flashing

- **Wi-Fi Default**: SSID `linksys` (2.4 GHz Open), DHCP server aktif (`192.168.1.10–192.168.1.20`), DNS `1.1.1.1` / `8.8.8.8`.
- **WebUI**: `http://192.168.1.1` (`admin` / `admin`) -> **Administration -> Firmware Upgrade** -> upload `output/WAP610N_v1.0.05.bin`.
- **SSH (`22`) / Telnet (`23`) / UART (`38400 8N1`)**: `root` / `admin`.
- **Unbricking & Serial/TFTP Recovery**: Lihat panduan lengkap di [`firmware-flasher/README.md`](firmware-flasher/README.md).

---

## 📚 Indeks Dokumentasi Modular

Setiap subsistem memiliki dokumentasi teknis lengkap di foldernya masing-masing:

| Direktori | Cakupan Dokumentasi |
|---|---|
| [`boards/star-6.7.2-mtlk-U-Media-vela/README.md`](boards/star-6.7.2-mtlk-U-Media-vela/README.md) | Spesifikasi SoC STR8132, pinout GPIO (`0/2/3/13`), & Flash MTD Map (`4 MiB`) |
| [`docker/README.md`](docker/README.md) | Container build 32-bit, GitHub Actions CI cache, & audit QA 6-tahap |
| [`devscripts/README.md`](devscripts/README.md) | Target `Makefile`/`make_nv.sh` & alur packaging rootless `genext2fs` |
| [`rootfs-star/README.md`](rootfs-star/README.md) | Sequence diagram boot, CLI applets, ConfigFS, & prosedur diagnosa live |
| [`apps/WebServer/README.md`](apps/WebServer/README.md) | GoAhead C89, switch 7 `.o` blob, `/api/*.json`, & log audit `X1–X6`/`S1–S18` |
| [`vendor/mtlk/README.md`](vendor/mtlk/README.md) | Inventori blob Metalink terbuka & panduan substitusi dari source |
| [`config/README.md`](config/README.md) | Versi firmware, plain-text `mtdblock4-defaults/`, & device table rootless |
| [`packages/README.md`](packages/README.md) | Template plugin C89/uClibc modular (`Makefile.pkg`) |
| [`kernel/patches-wap610n/README.md`](kernel/patches-wap610n/README.md) | Aturan ABI kernel `2.6.16-star` & hook `make mtlk-driver` |
| [`firmware-flasher/README.md`](firmware-flasher/README.md) | SOP serial UART, TFTP U-Boot, Kermit, & toolkit `flasher.sh` |
| [`AGENTS.md`](AGENTS.md) | Aturan operasional wajib, invarian toolchain/flash, & checklist QA |
