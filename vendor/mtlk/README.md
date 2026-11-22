# 📡 Open Metalink Proprietary Blobs (`vendor/mtlk/`)

Direktori terbuka pengganti arsip tertutup `wlan.tar.gz`. Hanya berisi komponen radio dan binary proprietary yang belum memiliki active source build di repositori.

---

## 1. Daftar Komponen

| Subdirektori | Isi | Keterangan |
|---|---|---|
| `driver/` | `mtlk.ko`, `LICENSE.txt` | Driver kernel wireless Metalink 802.11a/b/g/n (`2.6.16-star`) |
| `images/` | `ap_upper.bin`, `sta_upper.bin`, `contr_lm.bin`, `bootloader.bin`, 156 `ProgModel_*.bin` | Microcode radio & kalibrasi hardware |
| `bcl/` | `BclSockServer`, `bcl_util.tcl`, `LICENSE.txt` | Server & skrip kalibrasi Baseband Command Language |
| `etc/` | `drvhlpr`, `mtdump`, `mtpriv`, `wsccmd`, `wpa_supplicant`, `wpa_cli`, `wpa_passphrase`, `LICENSE.txt` | Helper driver & daemon WPA/WPS prebuilt |

---

## 2. Panduan Substitusi dari Source Code

- **Kernel Driver (`mtlk.ko`)**: Jangan mengubah layout struct inti kernel `kernel/linux-2.6.16-star/include/linux/` selama `mtlk.ko` masih berupa blob. Jika source driver Metalink tersedia, build dengan:
  ```bash
  make mtlk-driver MTLK_DRIVER_SRC=/path/to/mtlk/src
  ```
- **WPS & WPA Supplicant (`wsccmd`, `wpa_supplicant`)**: Source code lengkap tersedia di [`apps/Intel-WPS/`](../../apps/Intel-WPS) dan [`apps/wpa_supplicant-0.5/`](../../apps/wpa_supplicant-0.5) (Makefile build-from-source sudah disiapkan untuk pengujian hardware bertahap).
