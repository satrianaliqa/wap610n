# 📡 WAP610N Master Engineering Directives & Agent Specification

Master operational directives and invariants for the **Cisco Linksys WAP610N Unified Firmware & BSP Platform** (**1 Board `STR8132`, 1 Product `WAP610N`, 1 Source Tree**).

---

## 1. 🔒 Strict Operational Directives & Invariants

Rules that **MUST ALWAYS BE FOLLOWED WITHOUT EXCEPTION**:

### 1.1 Workspace & Path Boundaries
- **Workspace Confinement**: Restrict all read, write, search, and execution commands strictly to `/workspaces/wap610n`.
- **Forbidden Paths**: NEVER access, inspect, or reference `/home/satszz/sdcard/` or any directory outside the repository root.
- **Path Portability**: NEVER introduce local host absolute paths (e.g., `/home/satszz/...`, `/workspaces/wap610n/...`) into Makefiles, `.config`, C headers, or shell scripts. Always enforce relative paths.

### 1.2 GCC 3.4.6 & Linux 2.6.16-star ABI Discipline
- **Legacy C Dialect**: Target compiler is **GCC 3.4.6 (ARM uClibc)**. Code must strictly comply with **C89 / GNU89** (declare variables at top of blocks; no C99 mid-block declarations or modern GCC extensions).
- **Binary ABI Compatibility**: `vendor/mtlk/driver/mtlk.ko` relies on exact kernel struct layouts and exported symbols from `kernel/linux-2.6.16-star`. NEVER modify core kernel headers (`include/linux/skbuff.h`, `netdevice.h`, etc.).

### 1.3 Physical Flash (4 MB) & RAM (32 MB) Guardrails
- **Hard Flash Partition Limit (`mtdblock2`)**: `output/bootpImage` & `WAP610N_v*.bin` **MUST NEVER EXCEED 3,801,088 bytes (`3.625 MB` / `0x3A0000`)**. Exceeding this limit overwrites `mtdblock4` (`0x103E0000`) and bricks the device.
- **RAM Footprint (32 MB)**: No memory-hungry daemons. Strip userspace binaries and `.ko` modules with `arm-linux-uclibc-strip`. Runtime logs/temp files must strictly use `/tmp` (`tmpfs`), never repeatedly writing to physical flash.

### 1.4 CI-First Build Workflow
- Avoid heavy full local kernel/toolchain builds unless explicitly requested; verify targeted C files with `tools/arm-uclibc-3.4.6/bin/arm-linux-uclibc-gcc` and let GitHub Actions CI ([`.github/workflows/compile.yml`](.github/workflows/compile.yml)) run the full container build + `verify-firmware.sh`.
- Never break [`docker/Dockerfile`](docker/Dockerfile), [`docker/compile.sh`](docker/compile.sh), or [`docker/verify-firmware.sh`](docker/verify-firmware.sh).

### 1.5 OEM Feature & Custom Enhancement Continuity
- Preserve all stock AP capabilities: Dual-Band 2.4/5 GHz, WPA2, WPS PushButton (GPIO 13), Hardware Reset (GPIO 3), Power/WPS LEDs (GPIO 0/2), RTL8201CP PHY.
- Preserve custom enhancements: Dropbear SSH (`22` + `scp`), Telnet (`23`), GoAhead WebServer C89 + JSON telemetry (`/api/status.json`, `/api/wifi.json`), MTD Flash Dumper, CLI applets (`sysmon`, `curl`, `json`, `nettest`, `flashdump`, `help`), and sysctl RAM tuning.

---

## 2. 🏗️ Hardware, GPIO & Flash Quick Reference

- **SoC / Memory / Flash**: Cavium STR8132 (ARM922T @ 200 MHz), 32 MB SDRAM, 4 MiB NOR Flash (`0x10000000–0x10400000`), Device ID `0x0012`.
- **GPIOs**: `GPIO 13` WPS Button (`/dev/gpio13`), `GPIO 3` Reset Button (`/dev/gpio3`, `/dev/rdflt0`), `GPIO 0` Power LED (`/dev/led0`), `GPIO 2` WPS LED (`/dev/gpio2`), `UART0` `/dev/ttyS0` (`38400 8N1`).
- **Flash MTD Map**: `mtdblock1` U-Boot (128 KB) | `mtdblock3` NVRAM (64 KB) | `mtdblock2` Firmware (`0x10040000–0x103DFFFF`, max `3,801,088 B`) | `mtdblock4` ConfigFS (`0x103E0000`, 128 KB `config.img.gz`).
- Full hardware details & diagrams: [`boards/star-6.7.2-mtlk-U-Media-vela/README.md`](boards/star-6.7.2-mtlk-U-Media-vela/README.md).

---

## 3. 📁 Repository Architecture & Modular Documentation Map

```text
/workspaces/wap610n/
├── Makefile                        # Granular targets (image/kernel/apps/webs/pack/verify/mtlk-driver)
├── make_nv.sh                      # Unified WAP610N build orchestrator
├── README.md & AGENTS.md           # Project overview & master engineering directives
├── apps/                           # Userspace apps (BusyBox, Dropbear, WebServer, OpenSSL, Intel-WPS, etc.)
│   └── WebServer/README.md         # GoAhead C89, 7 .o blob switches, /api/*.json, security audit log
├── boards/star-6.7.2-mtlk-U-Media-vela/
│   └── README.md                   # Hardware specs, GPIO matrix, MTD map, production .configs
├── config/README.md                # WAP610N_VERSION, mtdblock4-defaults/, rootfs-device-table.txt
├── devscripts/README.md            # validate-build-prerequisites.sh, net-ver-mkrootfs.sh, helpers
├── docker/README.md                # 32-bit Debian 8 container build, CI cache, verify-firmware.sh QA
├── firmware-flasher/README.md      # UART/TFTP/Kermit unbricking & flasher.sh toolkit
├── kernel/patches-wap610n/README.md# Linux 2.6.16-star ABI rules & make mtlk-driver hook
├── packages/README.md              # Standardized C89/uClibc plugin template (Makefile.pkg)
├── rootfs-star/README.md           # Single source of truth RootFS, boot sequence, CLI & live diagnostics
├── vendor/mtlk/README.md           # Open proprietary Metalink blobs (mtlk.ko, ProgModel_*, wsccmd, etc.)
├── tools/arm-uclibc-3.4.6/         # GCC 3.4.6 ARM uClibc cross-compiler
└── u-boot-1.1.4/                   # U-Boot 1.1.4 bootloader source
```

---

## 4. 🧪 Build Pipeline & Pre-Commit QA Checklist

Quick commands:
```bash
./devscripts/validate-build-prerequisites.sh   # Pre-flight validation
./docker/compile.sh WAP610N                    # Full 32-bit container build + verify-firmware.sh
./docker/verify-firmware.sh output/bootpImage  # 6-stage binary, CRC32, ramdisk & headroom audit
```

Before staging or committing any change:
- [x] **Flash Budget**: Compiled `bootpImage <= 3,801,088 bytes` (`0x3A0000`).
- [x] **C89 Compliance**: Userspace C compiles cleanly with `arm-linux-uclibc-gcc -Wall` (no C99 constructs).
- [x] **Syntax & Diff Sanity**: `sh -n` / `bash -n` on modified shell scripts, `tclsh` syntax check on `.tcl`, and `git diff --check` clean (no CRLF or trailing whitespace).
- [x] **Single Rootfs Source of Truth**: Edit runtime files strictly under `rootfs-star/` (never edit generated `rootfs/` staging directory).
- [x] **No Host Path Leaks**: Zero `/home/satszz` or `/workspaces/wap610n` paths in scripts/configs.

---

## 5. 🛠️ Subsystem Audit & Unification Summary

- **80-Bug 5-Domain & 8-Domain Stabilization**: Remediated GoAhead WebServer C89 (`X1–X6`, `S1–S18`, `W-01–W-20`), WebUI ASP/JS/Tcl (`F-01–F-16`), runtime init/WPS/ConfigFS (`RT-01–RT-16`), build/packaging (`B-01–B-18`), and kernel/U-Boot/native C (`K-01–K-10`). Detailed tables live in [`apps/WebServer/README.md`](apps/WebServer/README.md) and [`rootfs-star/README.md`](rootfs-star/README.md).
- **Platform Unification (Phases 1–6)**: Eliminated `wlan.tar.gz` / `rootfs-star.tgz` / tracked `rootfs/` / `apps/jffs2-etc/` / non-WAP610N targets; introduced open `vendor/mtlk/`, auto-generated plain-text `config/mtdblock4-defaults/`, rootless `genext2fs`, CI container caching, zero-fork `/api/status.json` & `/api/wifi.json`, strict zero-fork `0x0012` pre-flash guard, `config_umount.sh` non-empty ConfigFS guard, and `packages/Makefile.pkg`.
