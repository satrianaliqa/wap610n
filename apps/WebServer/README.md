# 🌐 GoAhead WebServer C89, JSON API & Audit Keamanan

Daemon HTTP tunggal WAP610N pada port `80` ([`apps/WebServer/`](./)), dikompilasi dengan GCC 3.4.6 C89 dan diawasi tunggal oleh `/sbin/init` via `/root/mtlk/web/run_webs.sh`.

---

## 1. Arsitektur Modern & Modular Blob Switches

- **Single Supervisor**: Menghilangkan arsitektur legacy dual-process storm (`webs` port 80 + fallback port 81) dan `vfork()` zombie loop; background apply memakai `fork()` + child `_exit(0)`.
- **DWARF v2 Header Reconstruction**: Rekonstruksi signature C89 penuh untuk [`mt_api.h`](./mt_api.h), [`mt_task.h`](./mt_task.h), dan [`mt_validation.h`](./mt_validation.h).
- **Modular Blob Switches ([`LINUX/Makefile`](./LINUX/Makefile))**:
  Lima object proprietary yang tersisa (`mt_api.o`, `mt_task.o`, `mt_validation.o`, `utils.o`, `hnap_implementation.o`) dikontrol via flag `USE_BLOB_<MOD> ?= 1`. Modul [`wireless.c`](./wireless.c) (`USE_BLOB_WIRELESS ?= 0`) dan [`wps.c`](./wps.c) (`USE_BLOB_WPS ?= 0`) kini telah terbuka penuh menjadi kode sumber C89.
- **Zero-Tcl Security Applier (`init_security.sh`)**: Konfigurasi `hostapd0.conf` / `wpa_supplicant0.conf` dan reload keamanan nirkabel dijalankan langsung via script POSIX shell cepat [`init_security.sh`](../../rootfs-star/root/mtlk/web/init_security.sh), dengan [`init_security.tcl`](../../rootfs-star/root/mtlk/web/init_security.tcl) sebagai shim kompatibilitas.

---

## 2. Native Zero-Fork JSON Telemetry API (`LINUX/main.c`)

Endpoint C89 tanpa `fork()` / `popen()` untuk monitoring ringan di RAM 32 MB:
- **`GET /api/status.json`**: Mengembalikan `model`, `hostname`, `network_type`, `version`, `uptime_sec`, statistik `/proc/meminfo` (`total`, `free`, `buffers`, `cached`, `commit_limit`, `committed_as`), dan status soket `/proc/net/tcp` (`established`, `listen`, `close_wait`).
- **`GET /api/wifi.json`**: Mengembalikan `essid`, `channel`, `network_mode`, `channel_bonding`, `authentication`, `rf_band`, `wps_mode`, serta counter RX/TX `wlan0` & `eth0` dari `/proc/net/dev`.

---

## 3. Proteksi Pre-Flash Anti-Brick (`upldform.c` & `hnap_config_metalink.c`)

- Memverifikasi magic ARM `0xEA000006`, batas ukuran `<= 3,801,088` bytes (`0x3A0000`), `pcksum2`, CRC32 header == footer, dan **Device ID ketat `0x0012` (WAP610N)** secara langsung di C tanpa spawn shell `system("get_env_param ...")` saat menahan buffer upload 3.6 MB.
- Penulisan ke `/dev/mtdblock2` dilakukan secara kontinu (`fwrite` + `fflush` + `fsync` + `BLKFLSBUF`) tanpa `O_TRUNC` ulang di tengah jalan, lalu verifikasi bit-per-bit sebelum reboot.

---

## 4. Ringkasan Audit Keamanan & Stabilitas (`X1–X6`, `S1–S18`, `W-01–W-20`, `F-01–F-16`)

| Kode | Kategori | Masalah Utama yang Telah Diperbaiki |
|---|---|---|
| **X1–X6** | Keamanan Kritis | Perbaikan bypass autentikasi `/HNAP1` (`strncmp` path prefix), penghapusan backdoor `shell.cgi`, proteksi traversal `/goform/<form>/HNAP1`, proteksi `dump_firmware.cgi` & `remote_access.cgi` (POST + auth), dan guard buffer upgrade pra-auth. |
| **S1–S18** | Stabilitas HTTP & Flash | Break EOF/error pada loop reject upload (`S1`), batas pra-auth 64 KB (`S2`), overflow boundary multipart (`S3`), `strcmp` bridge & bounded `sscanf` (`S4–S5`), `ferror` UAF (`S6`), chunked dump (`S7`), non-blocking WPS poll (`S10`), live ESSID apply (`S12`), `Connection: close` ASP (`S14`), bounded `socketSelect` (`S15`), drain payload form (`S16`), continuous stream `MT_BurnAndVerifyImage` (`S17`), dan state machine `upgrade.asp` (`S18`). |
| **W-01–W-29** | Backend C89 & Blob Overrides | `malloc`/`bfree` mismatch (`WEBS_SYS_MALLOC_POST`), reset retry `locWrite`, batas `0x3A0000` di HNAP, `fork()` replace `vfork()`, bounds di `dd_process_packet()` & `startMultiLangParser()`, reset `lastAuth = 0` saat `AuthTimeout` agar tidak 401 lockout permanen (`W-21`), parameter asli `wlan0.conf` + JSON escaping di `/api/wifi.json` (`W-22`), buffer IPC `1025` + `MSG_NOERROR` anti-`E2BIG` (`W-23`), `-fno-strict-aliasing` & deklarasi `debug_printf` (`W-24`), `-Wl,--wrap=vfork` + `__wrap_vfork()` (`W-25`), override C89 `networkBasic` & `wirelessBasic` untuk mode AP/STA penuh (`W-26`), `safeAspWaitToRedirect` anti-FD leak & `safeAspGetWirelessChannel`/`safeAspGetBSSID` anti-`SIGSEGV` (`W-27`), `EINTR`/`EAGAIN` retry di `sockGen.c` + zero-fork `writeFwUpgradeStatus()` + `/var/firmware_upgrading` pra-alokasi (`W-28`), serta redirect `run_webs.sh` & `/dev/kmsg`-only `websDiag()` anti-hang `/dev/console` (`W-29`). |
| **F-01–F-16** | Frontend WebUI | Perbaikan sintaks JS `u-media.js` & `sta_network.asp`, mode AP & WPA/WPA2 terpisah di `wireless_basic.asp` (termasuk dropdown `20 MHz / 40 MHz` & `nowMode="apMode"`), sanitasi XSS SSID/error, `redirectFile.map` 25s, dan perbaikan `update_security.tcl` + `driver_api.ini`. |
