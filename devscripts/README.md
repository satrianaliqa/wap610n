# ⚙️ Build Orchestration & Deterministic Rootless Packaging

Skrip otomatisasi build di `devscripts/` bersama orchestrator root [`make_nv.sh`](../make_nv.sh) dan [`Makefile`](../Makefile) untuk arsitektur terpadu **1 Board (`STR8132`), 1 Product (`WAP610N`)**.

---

## 1. Target Developer Granular (`Makefile` & `make_nv.sh`)

| Target `Makefile` | Perintah `make_nv.sh` | Fungsi |
|---|---|---|
| `make image` / `make all` | `reconf` + `build vendor/mtlk WAP610N` | Full build (kernel + modules + apps + webs + ramdisk + `bootpImage`) |
| `make configure` | `./make_nv.sh reconf star-6.7.2-mtlk-U-Media-vela` | Sinkronisasi `.config` kernel/busybox/apps & `device_id.h` |
| `make kernel` | `./make_nv.sh kernel` | Kompilasi `zImage` & kernel modules saja |
| `make apps` | `./make_nv.sh apps` | Kompilasi & install `MTLK_APPS` ke `rootfs/` |
| `make webs` | `./make_nv.sh webs` | Kompilasi & strip hanya GoAhead WebServer (`apps/WebServer/LINUX/webs`) |
| `make pack` | `./make_nv.sh pack` | Assembly `rootfs/`, build `ramdisk_2.6.16.img.lzma` + `bootpImage` |
| `make verify` | `./docker/verify-firmware.sh output/bootpImage` | Jalankan audit QA 6-tahap pada firmware |
| `make mtlk-driver` | — | Hook build `mtlk.ko` dari `MTLK_DRIVER_SRC=/path/to/src` |
| `make clean` | `./make_nv.sh clean WAP610N` | Bersihkan object build, `images/`, dan `output/` |

---

## 2. Alur Assembly Deterministik (`net-ver-mkrootfs.sh`)

[`net-ver-mkrootfs.sh`](./net-ver-mkrootfs.sh) menyusun `ramdisk_2.6.16.img.lzma` secara satu arah (tanpa saling timpa):
1. Menyiapkan staging `rootfs/` dari sumber tunggal [`rootfs-star/`](../rootfs-star).
2. Menyalin blob proprietary radio/binari dari [`vendor/mtlk/`](../vendor/mtlk) ke `rootfs/root/mtlk/`.
3. Menyinkronkan kustomisasi WAP610N dari `rootfs-star/root/mtlk/` (mengecualikan artifak hasil compile: `gpio.ko`, `hnap_wps_status_monitor`, `force_ethpc_renew_ip`, `web/webs`).
4. Memasang `apps/WebServer/LINUX/webs` hasil compile terbaru.
5. Menjalankan build/install plugin opsional di `packages/*/Makefile.pkg`.
6. Meng-generate otomatis `rootfs/root/mtlk/etc/config.img.gz` dari file plain-text [`config/mtdblock4-defaults/`](../config/mtdblock4-defaults).
7. Menormalkan permission, men-strip binary & `.ko` via `arm-linux-uclibc-strip`, memvalidasi marker WAP610N, dan mengemas ext2 ramdisk via `genext2fs -U -b 12288` (dengan [`config/rootfs-device-table.txt`](../config/rootfs-device-table.txt) saat non-root) lalu kompresi `lzma`.

---

## 3. Daftar Utilitas di `devscripts/`

- `validate-build-prerequisites.sh` — Pre-flight check seluruh direktori, `vendor/mtlk/`, `rootfs-star/`, `config/`, dan container runtime.
- `net-ver-select-star.sh` — Selector BSP yang melewati `make clean` kernel jika `.config` tidak berubah.
- `net-ver-mkrootfs.sh` — Pembuat `ramdisk_2.6.16.img.lzma` deterministik & rootless (men-sync `rootfs-star/` incremental sambil mengecualikan binari hasil compile, serta menerapkan `FIRMWARE_VERSION` & `CUSTOM_*`).
- `count_version.sh` & `fw_rename.sh` — Pembaca `config/WAP610N_VERSION` (dengan dukungan env override), generator metadata build di `output/.config_WAP610N`, dan penamaan output `WAP610N_v<FIRMWARE_VERSION>.bin`.
- `disassemble.sh` & `panic-parse.sh` — Alat bantu disassembly kernel & analisis alamat kernel panic terhadap `System.map`.
