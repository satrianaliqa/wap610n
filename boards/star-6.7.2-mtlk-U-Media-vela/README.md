# ⚡ Hardware Architecture, GPIO Matrix & NOR Flash MTD Map

Spesifikasi hardware, pinout GPIO, dan layout partisi NOR Flash 4 MB untuk **Cisco Linksys WAP610N** (`star-6.7.2-mtlk-U-Media-vela`, Device ID `0x0012`).

---

## 1. Spesifikasi Hardware

| Komponen | Spesifikasi Teknis | Keterangan Register / File |
| :--- | :--- | :--- |
| **SoC / CPU** | Cavium / Star Semi STR8132 (CNS2132) | ARM922T Core (ARMv4T), 200 MHz |
| **RAM** | 32 MB SDRAM | Memory map: `0x00000000 - 0x02000000` |
| **Flash ROM** | CFeon EN29LV320BB-70TIP (4 MiB NOR) | Memory map: `0x10000000 - 0x10400000` |
| **Ethernet PHY** | Realtek RTL8201CP (10/100 Mbps) | `rtl8201cp_init(0)` di `umedia_vela.h` (`eth0`) |
| **Wireless Radio** | Metalink (MTLK) 802.11a/b/g/n Dual-Band | Driver: `mtlk.ko` (2.4 GHz / 5 GHz, `wlan0`) |
| **Device ID** | `0x0012` (WAP610N Resmi) | `kernel/device_id.h` & header 32-byte U-Boot |
| **UART0 Console** | 38400 baud, 8N1 (3.3V TTL — VCC jangan dicolok) | `/dev/ttyS0`, `inittab` (`ttyS0::respawn:/bin/sh`) |

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          CISCO LINKSYS WAP610N                          │
├─────────────────────────────────────────────────────────────────────────┤
│  Cavium / Star Semiconductor STR8132 (CNS2132) SoC                      │
│  • ARM922T Core @ 200 MHz (ARMv4T)  • 32 MB SDRAM  • 4 MB Parallel NOR  │
├──────────────────┬────────────────────┬────────────────┬────────────────┤
│ Flash ROM        │ Ethernet PHY       │ Wireless Radio │ UART0 Console  │
│ EON EN29LV320BB  │ Realtek RTL8201CP  │ Metalink MTLK  │ 38400 8N1 3.3V │
│ 4 MB NOR         │ 10/100 BaseT MII   │ 2.4 / 5 GHz    │ Pin Header     │
└──────────────────┴────────────────────┴────────────────┴────────────────┘
```

---

## 2. GPIO Pinout Matrix

| Line | Fungsi Hardware | Active Level | Default State | Device Path | Handler Daemon |
|---|---|---|---|---|---|
| **GPIO 13** | WPS PushButton | Active Low (`0`) | Pulled-up High (`1`) | `/dev/gpio13` | `/root/mtlk/etc/WPS_PBC.sh` |
| **GPIO 3** | Hardware Reset (tahan 3 detik) | Active Low (`0`) | Pulled-up High (`1`) | `/dev/gpio3`, `/dev/rdflt0` | `/root/mtlk/etc/mtlk_pbc_reboot.sh` |
| **GPIO 0** | Power / Diagnostic LED | Output | Green (Normal) | `/dev/led0` | `mtlk_init.sh` (`echo 1 > /dev/led0`) |
| **GPIO 2** | WPS Activity LED | Output | Off | `/dev/gpio2` | `mtlk_wps_msgloop.tcl` / `upldform.c` |

---

## 3. NOR Flash Memory Map (4 MiB)

```
0x10000000 ┌──────────────────────────────────────────────┐
           │ mtdblock1: U-Boot 1.1.4 Bootloader (128 KB)  │  [READ ONLY]
0x10020000 ├──────────────────────────────────────────────┤
           │ mtdblock3: U-Boot Environment / NVRAM (64 KB)│  [RW: CRC-checked]
0x10030000 ├──────────────────────────────────────────────┤
           │ Reserved / Board Parameters (64 KB)          │  [READ ONLY]
0x10040000 ├──────────────────────────────────────────────┤
           │ mtdblock2: Kernel + Ramdisk LZMA             │  [RW: Firmware]
           │ Filename: bootpImage / WAP610N_v1.0.05.bin   │
           │ HARD LIMIT: 3,801,088 BYTES (3.625 MB)       │
0x103E0000 ├──────────────────────────────────────────────┤
           │ mtdblock4: ConfigFS (config.img.gz, 128 KB)  │  [RW: ext2 loopback]
0x10400000 └──────────────────────────────────────────────┘
```

> [!CAUTION]
> **Hard Flash Limit**: Jika `bootpImage` melebihi **3,801,088 bytes (`0x3A0000`)**, penulisan ke `mtdblock2` akan menimpa `mtdblock4` (`0x103E0000`) dan menyebabkan bootloop. Seluruh pipeline (`mkbootp.sh`, `make_nv.sh`, `verify-firmware.sh`, `upldform.c`, `hnap_config_metalink.c`) menegakkan batas ini.

---

## 4. Isi Direktori Board

- `kernel/.config` — Konfigurasi produksi kernel Linux 2.6.16-star.
- `kernel/device_id.h` — Identitas Device ID `0x0012` untuk header `bootpImage`.
- `busybox/.config` — Konfigurasi applet BusyBox 1.8.1.
- `appsconfig/.config` — Daftar aktif `MTLK_APPS` untuk target WAP610N.
