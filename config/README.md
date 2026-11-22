# ⚙️ Konfigurasi Versi, Default MTD4 & Device Table

Direktori pusat konfigurasi versi firmware, default partisi konfigurasi (`mtdblock4`), dan tabel node device untuk packaging rootless.

---

## 1. File di Direktori Ini

- **`WAP610N_VERSION`** & **`.config_WAP610N`** — Mendefinisikan versi semantik firmware (`FIRMWARE_VERSION="1.0.05"`) dan nama file output (`WAP610N_v1.0.05.bin`).
- **`mtdblock4-defaults/`** (`sys.conf` & `wlan0.conf`) — Sumber teks biasa (*plain-text*) untuk factory default Access Point WAP610N. Setiap build via [`devscripts/net-ver-mkrootfs.sh`](../devscripts/net-ver-mkrootfs.sh) otomatis menyinkronkan file ini ke `saved_configs/` dan membentuk `/root/mtlk/etc/config.img.gz` (ext2 gzip 128 KB, mode `0600`).
- **`rootfs-device-table.txt`** — Tabel device node (`/dev/console`, `/dev/gpio0..31`, `/dev/led0..2`, `/dev/rdflt0`, `/dev/mtd*`, `/dev/mtdblock*`) dan permission (`/etc/shadow` `0600`, `/etc/dropbear` `0700`) untuk `genext2fs -D`.
