# 📁 Firmware Flasher: PANDUAN LENGKAP & SOP HANDOVER
**Pusat operasional hardware, serial flashing, binary auditing, dan unbricking WAP610N**

Dokumen ini disusun sebagai panduan operasional mandiri (*self-contained handover guide*) agar siapa pun anggota tim / penerus di **Divisi Flasher** dapat langsung mengoperasikan, mem-flashing, dan menyelamatkan (*unbrick*) perangkat Cisco Linksys WAP610N tanpa kebingungan.

---

## ⚡ Quick Start (Dashboard 1-Pintu)

Untuk mempermudah seluruh operasi flashing dan diagnostik tanpa perlu mengingat nama-nama script terpisah, cukup jalankan dashboard interaktif:

```bash
cd firmware-flasher
./flasher.sh
```

Dashboard ini otomatis mendeteksi port USB UART, memverifikasi ketersediaan biner firmware, memperbaiki izin serial, dan menyediakan menu 1-klik untuk semua operasi.

---

## 🔌 1. Anatomi Fisik & Panduan Wiring Serial UART

### A. Pinout Header UART pada PCB WAP610N
Header UART terletak di dekat CPU/RAM pada board PCB WAP610N berupa **4 lubang pin segaris (2.54mm pitch)**:

```text
    +-------------------------------------------------------------+
    |                     PCB WAP610N TOP VIEW                    |
    |                                                             |
    |    [ PIN 1 ]   [ PIN 2 ]   [ PIN 3 ]   [ PIN 4 ]            |
    |     (Kotak)    (Bulat)     (Bulat)     (Bulat)              |
    |       VCC         TX          RX         GND                |
    |      (3.3V)                                                 |
    +-------------------------------------------------------------+
```

### B. Tabel Sambungan Kabel ke USB-TTL Adapter (CP2102 / CH340 / FTDI)

| Pin PCB Router | Fungsi | Sambungkan ke USB TTL | Catatan Sangat Penting |
| :--- | :--- | :--- | :--- |
| **Pin 1 (Pad Kotak)** | **VCC (+3.3V)** | **JANGAN DICOLOK! (BIARKAN KOSONG)** | ⚠️ **BAHAYA**: Router mendapat daya dari adaptor 12V sendiri. Menghubungkan VCC akan merusak SoC/USB TTL! |
| **Pin 2** | **TX Router** | **RX** Adapter USB | Mengirim data log bootloader/kernel ke PC |
| **Pin 3** | **RX Router** | **TX** Adapter USB | Menerima perintah/keystroke dari PC ke router |
| **Pin 4** | **GND Router** | **GND** Adapter USB | Ground referensi sinyal (Wajib terhubung) |

### C. Parameter Serial Komunikasi
* **Baud Rate**: `38400` *(Bukan 115200! U-Boot dan Kernel WAP610N disetel ke 38400)*
* **Data Bits**: `8`
* **Parity**: `None`
* **Stop Bits**: `1` (Format: **8N1**)
* **Flow Control**: `None`
* **Level Tegangan**: **3.3V TTL** *(Jangan gunakan RS-232 12V standar PC lama)*

---

## 📂 2. Struktur Direktori `firmware-flasher`

```text
firmware-flasher/
├── 🚀 flasher.sh                     <-- Interactive Master Control Center (Dashboard 1-Pintu)
├── 📄 README.md                      <-- Panduan Teknis & SOP Operasional ini
│
├── 📁 tools/                         <-- Toolkit Automasi & Protokol Hardware
│   ├── flash_auto.py                 <-- Auto-Flasher 1-Klik (Dual-Mode: TFTP LAN + fallback UART Kermit)
│   ├── verify_firmware.py            <-- Auditor biner 32-byte header, Device ID 0x0012, & CRC32
│   ├── uboot_terminal.py             <-- Terminal serial interaktif dua-arah (dengan auto-stop autoboot)
│   ├── serial_logger.py              <-- Logger serial real-time (Baudrate 38400 8N1)
│   └── tftp_server.py                <-- Server TFTP lokal mandiri (UDP Port 69)
│
├── 📁 firmware/                     <-- Binary firmware yang sudah diaudit dan siap flash
│   ├── WAP610N_v1.0.05.bin           <-- File binary utama (~3.01 MB, Checksum Valid)
│   ├── bootpImage                    <-- Format identik untuk TFTP U-Boot
│   ├── ramdisk_2.6.16.img.lzma       <-- Rootfs LZMA biner (1.97 MB) berisi uClibc & /dev/console
│   └── System.map                    <-- Simbol fungsi kernel Linux
│
└── 📁 logs/                          <-- Arsip Forensik Boot & Log Flashing
    ├── initial_panic_log.txt         <-- Log forensik saat brick awal (Kernel Panic: No init found)
    ├── uboot_environment.txt         <-- Dump printenv, flinfo, bdinfo hardware register
    └── latest_success_boot.txt       <-- Log boot sukses setelah perbaikan rootfs (Loaded GPIO)
```

---

## 🛠️ 3. Panduan Penggunaan Tools Divisi Flasher

### A. Validasi Biner Sebelum Flashing (`verify_firmware.py`)
Selalu audit biner baru hasil build Divisi Compiler sebelum di-burn ke perangkat:

```bash
python3 tools/verify_firmware.py [path/ke/firmware.bin]
```
Kriteria Kelulusan:
1. Ukuran $\le$ `3.625 MB` (`0x3A0000`).
2. Word 0 = `0xEA000006` (ARM Branch).
3. Word 3 = `0x0012` (**WAP610N Resmi**, bukan WET610N `0x0011`).
4. Header CRC32 cocok 1:1 dengan Trailer CRC32 di akhir file.

---

### B. Flashing Otomatis 1-Klik (`flash_auto.py`)
Tool ini adalah senjata utama Divisi Flasher. Script bekerja secara pintar dengan sistem **Dual-Mode**:
1. **Mode LAN TFTP (2 Detik)**: Script menguji `ping 192.168.1.100`. Jika kabel LAN aktif dan server merespons, firmware diunduh dalam 2 detik.
2. **Mode UART Kermit (Fallback Aman)**: Jika kabel LAN tidak terpasang/tidak merespons, script otomatis beralih mentransfer via serial protocol Kermit (`loadb`) tanpa perlu mengulang dari awal.

```bash
# 1. Jalankan flasher
python3 tools/flash_auto.py

# 2. Cabut lalu colokkan adaptor power router saat diminta.
# 3. Script akan menangani autoboot interrupt, upload biner, erase flash di 0x10040000,
#    copy RAM ke flash, verifikasi checksum, reboot, dan menampilkan live log boot baru!
```

---

### C. Terminal Serial Interaktif (`uboot_terminal.py`)
Gunakan tool ini untuk mengetik langsung ke U-Boot CLI (`MTLK U-MEDIA VB #`) atau Linux shell console:

```bash
python3 tools/uboot_terminal.py
```

---

## 🗺️ 4. Peta Memori Flash SPI (4MB) & Aturan Keamanan

Flash ROM WAP610N menggunakan chip EON EN29LV640B / ENLV320B (4MB / 71 Sektor):

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
| 0x103FFFFF          |                     | `config_offset=0x103F0000`                    |
+---------------------+---------------------+-----------------------------------------------+
```

> [!CAUTION]
> **Aturan Emas Flashing SPI Flash**:
> 1. Alamat awal penulisan kernel/firmware **SELALU** di `0x10040000`.
> 2. **WAJIB ERASE SEBELUM COPY**: Sektor flash harus di-erase terlebih dahulu (`erase 0x10040000 +<size>`) sebelum menjalankan `cp.b 0x100000 0x10040000 <size>`. NOR flash hanya bisa mengubah bit `1` menjadi `0`, sehingga menulis tanpa erase akan merusak bit biner.
> 3. Jangan pernah melakukan erase pada rentang `0x10000000 - 0x1003FFFF` (Sektor Bootloader U-Boot).

---

## ❓ 5. Troubleshooting Matrix & FAQ untuk Anggota Baru

| Gejala Masalah | Kemungkinan Penyebab | Solusi Langsung |
| :--- | :--- | :--- |
| **Port `/dev/ttyUSB0` tidak ditemukan** | Kabel USB belum terpasang atau USB adapter tidak terbaca oleh kernel host. | Cek dengan `lsusb` dan `dmesg \| grep tty`. Pastikan modul `cp210x` atau `ch341` aktif. Coba cabut-colok USB ke port lain. |
| **Permission Denied saat buka port serial** | User Linux host belum masuk grup `uucp` / `dialout`. | Jalankan menu `[6]` di `./flasher.sh` atau ketik `sudo chmod 666 /dev/ttyUSB*`. |
| **Karakter serial muncul kotak-kotak / aneh (garbled)** | Baud rate terminal tidak sesuai. | Ubah baud rate ke **`38400`** (8N1). WAP610N tidak menggunakan baud 115200. |
| **Script flasher tidak bisa stop countdown U-Boot** | Kabel TX dan RX tertukar, atau script baru dinyalakan setelah router terlanjur boot. | Pastikan TX adapter terhubung ke RX router, dan RX adapter ke TX router. Jalankan script flasher terlebih dahulu, baru tancapkan adaptor power router. |
| **Kabel LAN dicolok langsung ke PC status `No Carrier`** | Chip Ethernet PHY RTL8201CP tidak memiliki fitur Auto-MDIX di level U-Boot. | Colokkan kabel LAN dari router dan PC ke **Switch / Router Rumah (Modem Wi-Fi)**, atau gunakan flashing via **UART Kermit** yang tidak butuh LAN. |
| **Kernel Panic `No init found`** | Ramdisk kekurangan file dynamic linker uClibc (`/lib/ld-uClibc.so.0`) atau `/dev/console`. | Hubungi Divisi Coder/Compiler untuk me-rebuild ramdisk dengan `rootfs` yang berisi library uClibc lengkap. |
| **Log serial berhenti setelah `Loaded GPIO driver`** | Firmware Linksys secara default mengeksekusi `ln -fs /dev/null /dev/console` di `/etc/inittab` untuk mode produksi. | Ini normal pada firmware rilis resmi. Jika ingin mengaktifkan shell serial, minta Divisi Coder meng-comment baris tersebut di `inittab` dan mengaktifkan `ttyS0::respawn:/bin/sh`. |

---
*Divisi Flasher berkomitmen menjaga integritas hardware dan kelancaran proses flashing di setiap siklus rilis firmware.* 🚀
