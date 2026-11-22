# ⚙️ Konfigurasi Versi, Default MTD4 & Device Table

Direktori pusat konfigurasi versi firmware, default partisi konfigurasi (`mtdblock4`), dan tabel node device untuk packaging rootless.

---

## 1. File di Direktori Ini

- **`WAP610N_VERSION`** & **`.config_WAP610N`** — `WAP610N_VERSION` adalah sumber tunggal (*single source of truth*) untuk versi semantik (`MAJOR`, `MINOR`, `PATCH`, `BUILD`, `FIRMWARE_VERSION`) sekaligus variabel kustomisasi build (`CUSTOM_VENDOR_NAME`, `CUSTOM_MODEL_NAME`, `CUSTOM_HOSTNAME`, `CUSTOM_LAN_IP`, `CUSTOM_ADMIN_USER`, `CUSTOM_ADMIN_PASS`, `CUSTOM_SSH_ENABLED`, `CUSTOM_TELNET_ENABLED`, `CUSTOM_WIFI_SSID`, `CUSTOM_WIFI_BAND`, `CUSTOM_WIFI_SEC_MODE`, `CUSTOM_WIFI_WPA_PSK`, `CUSTOM_WPS_ENABLED`). Semua variabel dapat di-override langsung via environment CLI saat build:
  ```bash
  FIRMWARE_VERSION="1.0.06" CUSTOM_WIFI_SSID="MyAP" CUSTOM_ADMIN_PASS="secret" ./docker/compile.sh WAP610N
  ```
- **`mtdblock4-defaults/`** (`sys.conf` & `wlan0.conf`) — Sumber teks biasa (*plain-text*) untuk factory default Access Point WAP610N. Setiap build via [`devscripts/net-ver-mkrootfs.sh`](../devscripts/net-ver-mkrootfs.sh) otomatis menyinkronkan file ini ke `saved_configs/` dan membentuk `/root/mtlk/etc/config.img.gz` (ext2 gzip 128 KB, mode `0600`).
- **`rootfs-device-table.txt`** — Tabel device node (`/dev/console`, `/dev/gpio0..31`, `/dev/led0..2`, `/dev/rdflt0`, `/dev/mtd*`, `/dev/mtdblock*`) dan permission (`/etc/shadow` `0600`, `/etc/dropbear` `0700`) untuk `genext2fs -D`.
