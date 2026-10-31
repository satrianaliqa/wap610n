# 📡 WAP610N Project Rules, Invariants & Master Engineering Specification

Master operational directives, architecture blueprints, subsystem audit logs, and development guidelines for the **Cisco Linksys WAP610N / WET610N Firmware Restoration & BSP Engine**.

---

## 1. 🔒 Strict Operational Directives & Invariants

Master rules that **MUST ALWAYS BE FOLLOWED WITHOUT EXCEPTION** by any AI coding agent or developer working on this codebase:

### 1.1 🔒 Strict Workspace & Path Boundaries
- **Workspace Confinement**: Restrict all read, write, search, and execution commands strictly to the project root directory (`./` or `/workspaces/wap610n` / `/home/satszz/wap610n`).
- **Forbidden Paths**: NEVER access, inspect, or reference `/home/satszz/sdcard/` or any directory outside the repository root.
- **Path Portability Invariant**: NEVER introduce local host absolute paths (e.g., `/home/satszz/...`, `/root/...`) into Makefiles, `.config`, header files, or shell scripts. Always enforce relative paths (e.g., `CONFIG_PREFIX="../../rootfs-star"`).

### 1.2 ⚡ GCC 3.4.6 & Ancient Kernel (2.6.16) Toolchain Discipline
- **Legacy C Dialect**: Target compiler is **GCC 3.4.6 (ARM uClibc)**.
  - DO NOT use modern C standards (C99/C11 features like variable declarations inside `for` loops, GNU C extensions from modern GCC, etc.). Code must strictly comply with C89 / GNU89 syntax.
  - DO NOT replace legacy macros or refactor working legacy syntax to "modern best practices".
- **Binary Compatibility**:
  - The Metalink wireless driver (`mtlk.ko`) and proprietary modules rely on exact kernel struct layouts and exported symbols from Linux 2.6.16-star.
  - DO NOT modify core kernel headers or struct definitions (`include/linux/skbuff.h`, netdevice structs, etc.) that could alter binary alignment or ABI for proprietary `.ko` blobs.

### 1.3 💾 Physical Flash (4 MB) & RAM (32 MB) Budget Guardrails
- **Hard Flash Partition Limit**:
  - The maximum allowable size for `bootpImage` / `WAP610N_v1.0.05.bin` is **3.625 MB (3,801,088 bytes)**.
  - Any build exceeding this limit **WILL CORRUPT NVRAM / CONFIGFS PARTITIONS (`0x103E0000`) AND PERMANENTLY BRICK HARDWARE**.
  - Always verify binary size before proposing or committing userspace additions.
- **RAM Footprint (32 MB)**:
  - Do not introduce memory-hungry daemons. All new utilities must be lightweight applets compiled against **uClibc** (strip all debug symbols with `arm-linux-uclibc-strip`).
  - Temporary files and runtime logs must strictly use `/tmp` (`tmpfs`), never writing frequently to physical flash sectors.

### 1.4 🛠️ CI-First Build Workflow
- **No Heavy Local Compilation**: Do not attempt full local kernel toolchain builds unless explicitly requested by the user.
- **Validation Before Commit**:
  - Check `git diff` carefully before staging changes.
  - Ensure changes do not break the Docker build engine (`docker/Dockerfile`, `docker/compile.sh`, and `.github/workflows/compile.yml`).
- **Clean Commit & Push**: Commit with clear, descriptive commit messages documenting modified subsystem(s), then push to GitHub (`satrianaliqa/wap610n`) to let GitHub Actions CI do the heavy lifting.

### 1.5 🛡️ OEM Feature & Hardware Integrity
- **Core Functionality Preservation**:
  - Maintain all stock Access Point capabilities: Dual-Band (2.4 GHz / 5 GHz), WPA2-PSK/Enterprise, WPS PushButton (GPIO 13), Hardware Reset (GPIO 3), WMM QoS, and Ethernet PHY (`RTL8201CP`).
- **Custom Enhancements Continuity**:
  - Preserve integrated enhancements: **Dropbear SSH (Port 22)**, **Telnet (Port 23)**, **Custom Web GUI MTD Flash Dumper**, and memory sysctl tunings.

---

## 2. 🏗️ Hardware Architecture & Pinout Specification

```
┌─────────────────────────────────────────────────────────────────────────┐
│                          CISCO LINKSYS WAP610N                          │
├─────────────────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────────────────┐    │
│  │  Cavium / Star Semiconductor STR8132 (CNS2132) SoC              │    │
│  │  • ARM922T Core @ 200 MHz (ARMv4T instruction set)              │    │
│  │  • 32 MB SDRAM Memory Space (0x00000000 - 0x02000000)           │    │
│  │  • 4 MB Parallel Flash Memory (0x10000000 - 0x10400000)         │    │
│  └─────────────────────────────────────────────────────────────────┘    │
│                         │                                               │
│  ┌────────────────┬─────┴───────────────┬─────────────────┐             │
│  │                │                     │                 │             │
│  ▼                ▼                     ▼                 ▼             │
│┌───────────────┐┌───────────────────┐ ┌────────────────┐┌──────────────┐│
││  Flash ROM    ││ Ethernet PHY      │ │ Wireless Radio ││ UART Console ││
││  EON          ││ Realtek RTL8201CP │ │ Metalink (MTLK)││ 38400 baud,  ││
││  EN29LV320BB  ││ 10/100 Mbps BaseT │ │ 802.11a/b/g/n  ││ 8N1, 3.3V    ││
││  4 MB Flash   ││ MII Interface     │ │ Dual-Band 2.4/5││ Pin Header   ││
│└───────────────┘└───────────────────┘ └────────────────┘└──────────────┘│
│                                                                         │
│  Hardware GPIO Lines:                                                   │
│  • GPIO 13: WPS PushButton (Active Low, read via /dev/gpio13)           │
│  • GPIO 3:  Hardware Reset Button (Active Low, hold 3s for factory reset│
│  • GPIO 0:  Power/Status LED (Green / Amber solid/blink)                │
│  • GPIO 2:  WPS Status LED (Blue / Amber solid/blink)                   │
└─────────────────────────────────────────────────────────────────────────┘
```

### Hardware Pinout & GPIO Matrix

| Line | Hardware Function | Active Level | Default State | Sysfs / Device Path | Handler Daemon |
|---|---|---|---|---|---|
| **GPIO 13** | WPS PushButton | Active Low (0) | Pulled-up High (1) | `/dev/gpio13` | `/root/mtlk/etc/WPS_PBC.sh` |
| **GPIO 3** | Hardware Reset Button | Active Low (0) | Pulled-up High (1) | `/dev/gpio3` | `/root/mtlk/etc/mtlk_pbc_reboot.sh` |
| **GPIO 0** | Power / Diagnostic LED | Output | Green (Normal) | `/dev/led0` | `mtlk_init.sh` (accepts numeric `echo 1 > /dev/led0` or string `echo green > /dev/led0` / `echo on > /dev/led0`) |
| **GPIO 2** | WPS Activity LED | Output | Off | Driver controlled | `mtlk_wps_msgloop.tcl` |
| **UART0** | Serial Debug Console | 3.3V TTL | 38400 8N1 | `/dev/ttyS0` | `/etc/inittab` (`ttyS0::respawn:/bin/sh`) |
| **MII** | Realtek RTL8201CP | 100BASE-TX | Auto-MDIX | `eth0` | `ifconfig eth0 192.168.1.1` |

---

## 3. 💾 Flash Memory Partitioning & MTD Map

Physical 4 MB NOR Flash layout (`0x10000000` – `0x10400000`):

```
0x10000000 ┌──────────────────────────────────────────────┐
           │ mtdblock1: U-Boot 1.1.4 Bootloader (128 KB)  │  [READ ONLY]
0x10020000 ├──────────────────────────────────────────────┤
           │ mtdblock3: U-Boot Environment / NVRAM (64 KB)│  [RW: CRC-checked]
0x10030000 ├──────────────────────────────────────────────┤
           │ Reserved Partition / Board Parameters (64 KB)│  [READ ONLY]
0x10040000 ├──────────────────────────────────────────────┤
           │ mtdblock2: Kernel + Ramdisk LZMA             │  [RW: Firmware]
           │ Filename: bootpImage / WAP610N_v1.0.05.bin   │
           │ MAXIMUM PERMISSIBLE SIZE: 3,801,088 BYTES    │
           │ (Exact Budget: 3.625 MB)                     │
0x103E0000 ├──────────────────────────────────────────────┤
           │ mtdblock4: Runtime Configuration File System │  [RW: ConfigFS/JFFS2]
           │ Storage for config.tar.gz / sys.conf         │
0x10400000 └──────────────────────────────────────────────┘
```

> [!CAUTION]
> **Hard Limit Warning**:
> If the compiled `bootpImage` exceeds **3,801,088 bytes**, writing the image will overwrite the starting sector of `mtdblock4` (`0x103E0000`). This corrupts persistent settings and the NVRAM environment, causing an irrecoverable hardware bootloop. The build script `docker/verify-firmware.sh` enforces this size gate with a hard fail.

---

## 4. 📁 Repository Layout & Subsystem Matrix

```
/workspaces/wap610n/
├── 📄 Makefile                     [Top-level build orchestrator]
├── 📄 make_nv.sh                   [Platform build script: reconf/build/clean]
├── 📄 net_ver.cm                   [Historical SVN manifest]
├── 📄 AGENTS.md                    [Master Specification & Engineering Directives]
├── 📄 README.md                    [Public project documentation & flash guide]
│
├── 📁 apps/                        [User-space applications & daemons]
│   ├── busybox-1.8.1/              ├→ Core shell, utilities & lightweight daemons (udhcpd, telnetd)
│   ├── dropbear-0.52/              ├→ Secure SSH daemon (port 22)
│   ├── WebServer/                  ├→ GoAhead HTTP web daemon (port 80) + ASP engine
│   ├── gpio_driver/                ├→ Linux kernel module for GPIO button and LED control
│   ├── wpa_supplicant-0.5/         ├→ Wi-Fi Protected Access authenticator
│   ├── openssl-0.9.8a/             ├→ SSL/TLS cryptography library
│   ├── hnap_wps_status_monitor/    ├→ Background daemon reporting WPS session states
│   ├── Intel-WPS/                  ├→ Wi-Fi Protected Setup state machine
│   ├── mtlk_vb_upnpd/              ├→ UPnP / AV Media Server daemon
│   ├── tinytcl/                    ├→ Lightweight Tcl interpreter for network config scripts
│   ├── force_ethpc_renew_ip/       ├→ Ethernet DHCP renewal helper
│   ├── simpdiff/                   ├→ Fast config comparison applet
│   └── set_env_param/              ├→ U-Boot environment parameter writer
│
├── 📁 kernel/                      [Linux kernel 2.6.16-star source]
│   └── linux-2.6.16-star/
│       ├── arch/arm/               ├→ ARM922T architecture support (Cavium STR8132)
│       ├── drivers/mtlk/           ├→ Metalink wireless driver framework
│       ├── drivers/net/            ├→ Realtek RTL8201CP fast ethernet MAC/PHY driver
│       └── include/linux/          ├→ Frozen kernel headers (Preserve struct alignment!)
│
├── 📁 boards/                      [Board Support Package (BSP)]
│   └── star-6.7.2-mtlk-U-Media-vela/
│       ├── kernel/.config          ├→ Production kernel build configuration
│       └── busybox/.config         ├→ BusyBox applet configuration
│
├── 📁 rootfs/                      [Root filesystem base template]
├── 📁 rootfs-star/                 [Pre-built source-of-truth root filesystem template]
│   ├── etc/                        ├→ Inittab, udhcpc scripts, hostname, network templates
│   └── root/mtlk/                  ├→ Metalink wireless scripts, web UI assets, firmware binaries
│
├── 📁 u-boot-1.1.4/                [U-Boot 1.1.4 bootloader source tree]
├── 📁 tools/                       [Precompiled cross-compilation toolchains]
│   └── arm-uclibc-3.4.6/           ├→ GCC 3.4.6, uClibc, binutils for ARMv4T
│
├── 📁 docker/                      [Reproducible container build engine]
│   ├── Dockerfile                  ├→ Debian 8 (i386) container image definition
│   ├── compile.sh                  ├→ Container entry point & firmware pipeline runner
│   └── verify-firmware.sh          ├→ Strict automated firmware QA & size gatekeeper
│
├── 📁 .github/workflows/           [CI/CD automation pipeline]
│   └── compile.yml                 ├→ GitHub Actions workflow for automated firmware builds
│
├── 📁 devscripts/                  [Build automation & developer tools]
│   ├── validate-build-prerequisites.sh ├→ Pre-flight environment sanity checks
│   ├── net-ver-mkrootfs.sh         ├→ Rootfs assembler & overlay builder
│   └── select-star.sh              ├→ BSP platform selector
│
└── 📁 config/                      [Version & product configurations]
    └── WAP610N_VERSION             ├→ Semantic version configuration file
```

---

## 5. 🔄 Boot Sequence & Daemon Architecture

```mermaid
sequenceDiagram
    autonumber
    participant HW as Hardware / ROM
    participant UB as U-Boot 1.1.4
    participant K as Linux Kernel 2.6.16-star
    participant I as /sbin/init (inittab)
    participant WLS as Wireless & Net Subsystem
    participant GO as GoAhead WebServer

    HW->>UB: Power-on reset (0x10000000)
    UB->>UB: Init ARM922T @ 200MHz, SDRAM 32MB, UART 38400 baud
    UB->>K: Decompress & boot bootpImage from 0x10040000
    K->>K: Init memory pages, interrupt controller, RTL8201 PHY
    K->>K: Mount LZMA compressed ext2 ramdisk as rootfs (/)
    K->>I: Execute /sbin/init (PID 1)

    rect rgb(240, 248, 255)
    Note over I: Init & Early System Initialization
    I->>I: Mount /proc, /sys, tmpfs on /tmp
    I->>I: mkdir -p /tmp/var/log /tmp/var/lock
    I->>I: mount -o bind /tmp/var /var
    I->>I: Spawn syslogd, klogd, ttyS0 console
    end

    rect rgb(255, 250, 240)
    Note over I,WLS: Wireless & Network Setup
    I->>WLS: Run /etc/mtlk_init_start.sh -> /root/mtlk/etc/mtlk_init.sh
    WLS->>WLS: Spawn dropbear (port 22) & telnetd (port 23)
    WLS->>WLS: Insmod gpio.ko, mtlk.ko (Metalink dual-band 802.11a/n)
    WLS->>WLS: Configure bridge br0 (eth0 + wlan0) @ 192.168.1.1
    WLS->>WLS: Start hostapd (AP mode), udhcpd (DHCP server)
    WLS->>WLS: Start WPS monitor (WPS_PBC.sh on GPIO 13)
    WLS->>WLS: Start Hardware Reset monitor (mtlk_pbc_reboot.sh on GPIO 3)
    end

    rect rgb(240, 255, 240)
    Note over I,GO: Web Management Supervisor
    I->>GO: inittab spawns /root/mtlk/web/run_webs.sh
    GO->>GO: Launch single webs instance on port 80
    GO->>GO: Serve ASP WebUI, HNAP1 SOAP, and MTD flash dumper
    end
```

---

## 6. ⚙️ Build System & Container Pipeline Engineering

### 6.1 Containerized Build Flow (`docker/Dockerfile` & `docker/compile.sh`)
- Builds run in a dedicated **32-bit Debian 8 (Jessie i386)** Docker environment to match GCC 3.4.6 host requirements.
- Sudoers configuration inside Docker is strictly idempotent:
  ```bash
  grep -q "NOPASSWD: ALL" /etc/sudoers || echo "$USER ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
  ```
- Workspace directory `rootfs/` is isolated cleanly from `rootfs-star/`. `compile.sh` uses `cp -a rootfs-star rootfs` rather than symlinking to prevent clean rules from wiping the reference source tree.

### 6.2 Pre-Build Validation Suite (`devscripts/validate-build-prerequisites.sh`)
Runs before any compilation begins (fail-fast principle):
1. **Archive integrity**: Verifies `wlan.tar.gz` and `rootfs-star.tgz`.
2. **Directory validation**: Verifies presence of `kernel/`, `apps/`, `boards/`, `tools/`.
3. **Toolchain check**: Validates `arm-linux-uclibc-gcc` (3.4.6) executable and libraries.
4. **Output directories**: Automatically creates `output/` and scratch staging areas.

### 6.3 Fail-Fast Make Orchestration (`devscripts/make_nv.sh`)
- Enforces `set -o pipefail` so that compilation failures hidden behind pipes (`make 2>&1 | tee ...`) correctly propagate non-zero exit codes.
- Explicit `exit 1` statements replace ambiguous bare exits across all configuration routines.
- `apps/scripts/Makefile.MTLK` enforces strict `|| exit 1` error termination in applet build loops.

### 6.4 Dynamic Versioning
Firmware versioning is decoupled from hardcoded strings and read dynamically from `config/WAP610N_VERSION`:
```bash
VERSION=$(grep FIRMWARE_VERSION config/WAP610N_VERSION | cut -d'"' -f2)
BINNAME="WAP610N_v${VERSION}.bin"
```

---

## 7. 🌐 WebUI GoAhead Refactoring & Security Audit Log

### 7.1 Architecture Modernization: Legacy Dual-Process Storm vs Unified Single Supervisor

```mermaid
flowchart TD
    subgraph Legacy["Kondisi Legacy (Dead Pipeline & Dual-Process Storm)"]
        A1["apps/WebServer/*.c\n(Source C mati terputus dari build)"] -. "Tidak dikompilasi" .-> B1["rootfs-star/root/mtlk/web/webs\n(Binary blob 2011 dari wlan.tar.gz)"]
        I1["/etc/inittab\n(::respawn:run_webs.sh)"] --> W1["webs #1 (Port 80)\n~1.5 MB RAM"]
        M1["mtlk_init_wls_daemons.sh\n(webs -c 0 &)"] --> W2["webs #2 (Port 81)\nFallback port ~1.5 MB RAM"]
        W1 -- "Form Submit: vfork() + killchildweb.sh" --> Z1["Parent mati -> inittab respawn\nChild return to main() -> ZOMBIE PROCESS!"]
    end

    subgraph Target["Kondisi Modern (Restorasi Penuh Fase 1 - 4)"]
        A2["apps/WebServer/*.c + Reconstructed Modules\n(GCC 3.4.6 C89)"] -- "Dikompilasi otomatis di make_nv.sh" --> B2["rootfs-star/root/mtlk/web/webs\n(Binary baru + Fix S1-S9 + Flash Dumper)"]
        I2["/etc/inittab\n(::respawn:run_webs.sh)"] --> W3["Single webs Instance (Port 80)\nDiawasi tunggal oleh init"]
        W3 -- "Form Submit: fork() -> child _exit(0)" --> Z2["Parent webs tetap hidup melayani HTTP\nChild eksekusi background task bersih"]
    end
```

### 7.2 DWARF v2 Header Reconstruction & Object Integration
Restored legacy object files (`mt_api.o`, `mt_task.o`, `mt_validation.o`) and reconstructed complete C89 header signatures via DWARF v2 symbol extraction (`readelf -wi`):
- `apps/WebServer/mt_api.h`: Rebuilt `MT_Action_t`, `MT_Burn_t`, `gMTBurnParams`, security enum flags, and function prototypes.
- `apps/WebServer/mt_validation.h`: Parameter validator prototypes and `validateParam_t`.
- `apps/WebServer/mt_task.h`: Task scheduler queue, `TaskId`, and `TaskType` structures.
- **Object Linking & Source Architecture**:
  - Preserved binary objects linked with reconstructed C89 headers: `utils.o`, `wireless.o`, `wps.o`, `hnap_implementation.o`, `mt_api.o`, `mt_task.o`, and `mt_validation.o`.
  - Direct C89 compilation from restored source: `management.c`, `xml_tree.c`, `stack.c`, `linked_list.c`, `hnap_config_metalink.c`, `device_discover.c`, `upldform.c`, `webs.c`, `asp.c`, `security.c`, `default.c`, `cgi.c`, and `LINUX/main.c`.

### 7.3 WebUI Audit & Security Remediation Matrix

| Vulnerability / Defect | Severity | Affected File | Root Cause & Security Impact | Engineering Resolution |
|---|---|---|---|---|
| **X1: HNAP Auth Bypass** | **CRITICAL** | `apps/WebServer/security.c:333`, `webs.c:905` | `strstr(wp->url, "/HNAP1")` matches query strings (`GET /secret?x=/HNAP1`), allowing universal unauthenticated bypass. | Switched to strict path prefix match: `strncmp(wp->path, "/HNAP1", 6) == 0`. |
| **X2: Unauthenticated Web Shell** | **CRITICAL** | `rootfs/root/mtlk/web/cgi-bin/shell.cgi` | Backdoor CGI script evaluating raw `eval "$cmd"` as root without authentication. | Removed `shell.cgi` completely from all rootfs templates. |
| **X3: Form Traversal Bypass** | **HIGH** | `apps/WebServer/form.c:50`, `LINUX/main.c:340` | `websFormHandler` truncated URL at next `/`, allowing `/goform/<form>/HNAP1` to trigger admin forms without credentials. | Fixed path tokenizer and enforced prefix authentication check. |
| **X4: Unprotected Flash Dumper CGI** | **HIGH** | `rootfs/root/mtlk/web/cgi-bin/dump_firmware.cgi` | Dumped raw `/dev/mtdblock*` containing plain-text passwords when accessed via X1 bypass. | Protected under strict admin session authentication. |
| **X5: Arbitrary Firmware Upgrade Flag** | **HIGH** | `apps/WebServer/webs.c:1291` | Any client sending `SOAPAction: ...FirmwareUpload` elevated upload buffer size before authentication, causing OOM. | Requires pre-authenticated session before granting large buffer allocation. |
| **X6: Remote Root Telnet Injection** | **HIGH** | `rootfs/root/mtlk/web/cgi-bin/remote_access.cgi` | Unauthenticated GET requests could activate `telnetd -l /bin/sh` on port 23 without authentication. | Requires administrative authentication and CSRF token verification. |
| **S1: Infinite Read Loop Hang** | **HIGH** | `apps/WebServer/webs.c:508-513` | Upload reject loop didn't handle `nbytes <= 0`, spinning at 100% CPU when clients disconnected early. | Added EOF (`nbytes == 0`) and error (`nbytes < 0`) break conditions. |
| **S2: Unauthenticated Upload OOM** | **HIGH** | `apps/WebServer/webs.c:497` | Large upload buffers (`malloc(3.6MB)`) allocated before validating client credentials. | Restricted unauthenticated request buffer sizes to 64 KB. |
| **S3: Multipart Boundary Overflow** | **HIGH** | `apps/WebServer/webs.c:1233-1236` | `balloc(strlen(p) + 2)` with `gsprintf("--%s")` caused 1-byte heap overflow off client header. | Corrected allocation to `balloc(strlen(p) + 3)` to include null terminator. |
| **S4: Bridged Client Management Lockout**| **MEDIUM** | `apps/WebServer/security.c:301` | `strstr("0", val)` returned non-NULL on empty string, blocking Wi-Fi management clients (403). Repeated `system(brctl)` slowed requests. | Replaced with `strcmp(val, "0") == 0` and cached bridge port query. |
| **S5: Bridge Parser Buffer Overflow** | **MEDIUM** | `apps/WebServer/security.c:56-126` | Unbounded `sscanf("%s %s")` on `brctl` output risked stack buffer overflow. | Enforced width specifiers `sscanf("%19s %29s")` and zeroed stack buffers. |
| **S6: Firmware Flash Use-After-Free** | **MEDIUM** | `apps/WebServer/upldform.c:319` | `ferror(fp)` invoked after `fclose(fp)` during firmware flashing pipeline. | Moved `ferror(fp)` check prior to `fclose(fp)`. |
| **S7: Flash Dump Single-Thread Freeze** | **MEDIUM** | `apps/WebServer/upldform.c:992-1035`| `formDumpFirmware` streamed 4 MB synchronously without chunking or timeouts, blocking WebUI. | Added non-blocking chunking with socket select timeouts. |
| **S8: Insecure CGI Filename Collision** | **LOW** | `apps/WebServer/cgi.c:100` | Temp filenames generated via predictable `rand() % 10000`, vulnerable to collisions. | Converted to process-isolated naming template. |
| **S9: HNAP Gateway Null Dereference** | **LOW** | `apps/WebServer/device_discover.c:1026` | HNAP static IP config with missing Gateway passed `NULL` to `route add`, risking segfault. | Added fallback guard: default to `"0.0.0.0"` or ignore default route assignment. |
| **S10: WPS Poll Freeze Deadlock** | **HIGH** | `apps/WebServer/LINUX/main.c`, `run_webs.sh` | Legacy `getCurrentWPSStatus` looped with 5s usleep when `/var/WPS_History` missing, blocking single-threaded GoAhead. | Replaced with non-blocking C handler `safeFormGetCurrentWPSStatus` and pre-seeded `/var/WPS_History`. |
| **S11: Manual Config Blank Screen** | **MEDIUM** | `apps/WebServer/LINUX/main.c` | `wirelessTypeSet` form returned 200 without redirect or body, replacing UI with blank page. | Added `safeFormWirelessTypeSet` redirecting to `/wireless/security.asp`. |
| **S12: Live Beacon Update on SSID Change** | **MEDIUM** | `rootfs-star/root/mtlk/web/update_security.tcl` | WebUI saved SSID to flash but did not reload hostapd or set in-flight ESSID. | Added live `iwconfig wlan0 essid "$ESSID"` and hostapd reload. |
| **S13: Dual-Band UI Cleanup & Mode Guard** | **LOW** | `station/wireless_basic.asp`, `station/wps_status.asp` | UI showed unsupported concurrent "both" option for single-radio WAP610N and enabled client-bridge `waitPad` screen blocker. | Removed "both" option, disabled `waitPad` and status poll loop when running in AP mode. |
| **S14: Dynamic ASP Keep-Alive Socket Hang** | **HIGH** | `apps/WebServer/default.c:134-142` | GoAhead sent `Connection: keep-alive` on dynamic ASP pages without `Content-Length`. Browsers waited indefinitely for connection close or length delimiter, resulting in endless spinner / hanging favicon. | Cleared `WEBS_KEEP_ALIVE` flag (`wp->flags &= ~WEBS_KEEP_ALIVE;`) in `websDefaultHandler` when processing dynamic ASP files, forcing clean `Connection: close`. |
| **S15: Socket Buffered Input Busy-Spin** | **HIGH** | `apps/WebServer/sock.c:284-307` | When `socketInputBuffered(sid)` was true, `socketSelect` set select timeout to 0ms. If client socket stalled mid-chunk or sent partial frames, GoAhead spun in a tight 100% CPU busy-loop. | Added bounded select timeout (10-50ms) and sanity loop counter to allow socket drain without CPU starvation. |
| **S16: Form Handler CLOSE_WAIT Socket Leaks** | **MEDIUM** | `apps/WebServer/form.c`, `handler.c` | Unconsumed request bodies on form redirection or validation errors left unread bytes in socket buffer, causing socket leaks in `CLOSE_WAIT` and descriptor exhaustion. | Drained unread request payload before closing or redirecting; ensured proper socket teardown in error branches. |
| **S17: Firmware Flashing Worker Premature Exit & MTD Truncation** | **CRITICAL** | `apps/WebServer/upldform.c:131-180` | (1) Accidental leftover `gMTBurnParams.precentComplete = 100; return 1;` before the write loop killed the background worker `MT_BurnAndVerifyImage` before writing the firmware image to flash. (2) Legacy code closed and reopened `/dev/mtdblock2` with `"w+b"` every 64KB, issuing `O_TRUNC` to the MTD block device mid-flash! | Removed the premature `return 1;`, removed the periodic close/reopen `w+b` loop, maintained a single continuous write stream (`fwrite` + `fflush`), restored bit-for-bit verification loop against original image file, and triggered system reboot cleanly. |
| **S18: WebUI Firmware Upgrade State Machine & XML Parser Mismatch** | **HIGH** | `rootfs-star/root/mtlk/web/admin/upgrade.asp` | (1) Modern browser XMLHttpRequest crashed because `overrideMimeType('text/xml')` was called on a plain text response `"5,20"` (format: `status,percent`), causing `responseXML` parse failure. (2) State machine failed to handle state `6` (`verifying_e`), leading to infinite waiting. (3) Transient HTTP errors during status polling permanently deadlocked `stopToWait`. (4) Missing client-side file selection validation allowed submitting empty firmware upgrade forms. | Removed `overrideMimeType('text/xml')`, parsed plain text CSV (`split(',')`), added handler for state `6` (Verifying image...), added automatic retry counter (up to 5 retries) on status polling errors before declaring failure, enforced file selection validation before form submit, and prevented browser caching on polling URLs with `_nocache` timestamp. |

### 7.4 Flash Config Lock & Elimination of Flash Wear
- Legacy firmware wrote directly to `/dev/mtdblock4` on every WebUI button click without concurrency guards.
- **Atomic Mutex**: Created POSIX-atomic lock `/tmp/save_config.lock` (`mkdir` based) in `save_config.sh` and `submitToFlash`.
- **Zero-Downtime Apply**: Background configuration tasks run in a cleanly forked child (`fork()` + `_exit(0)`), ensuring the main HTTP listener never drops connections while saving to flash.

---

## 8. 🛠️ Non-WebUI Subsystem Audit & Remediation Log (Fase 1 – 6)

Complete record of the 6-phase audit and remediation performed across the non-WebUI subsystems.

### FASE 1: Build & Packaging Integrity (Overlays & Pipeline Verification)
- **Problem**: Build sequence `apps install (E_DEST=rootfs)` → `release.sh (webs)` → `net-ver-mkrootfs.sh` extracted legacy 2011 `wlan.tar.gz` and `apps/jffs2-etc/networking/VB/*.sh` **on top of** `rootfs/root/mtlk/`, silently wiping WAP610N WebUI files and sysctl tunings.
- **Fixes Applied**:
  1. `devscripts/net-ver-mkrootfs.sh`: Overlaid `rootfs-star/root/mtlk/.` → `rootfs/root/mtlk/` **after** extraction and VB installation, establishing `rootfs-star` as absolute source-of-truth.
  2. `apps/jffs2-etc/networking/VB/mtlk_init.sh`: Synchronized sysctl tuning (`vm.min_free_kbytes=2048`, `vm.dirty_ratio=10`) and solid green power LED logic.
  3. `rootfs-star/root/mtlk/web/wireless/`: Replaced stale 209 KB ASP files with correct relative symlinks to `../station/*.asp`.
  4. `docker/compile.sh`: Removed dangerous `rootfs → rootfs-star` symlinking, replaced with `cp -a`.
  5. `docker/verify-firmware.sh`: Added automated ramdisk mount assertions verifying that WAP610N markers, sysctl settings, and dual-band menus exist in the output binary.
  6. `make_nv.sh`: Fixed bare exits, added `set -o pipefail`, and added no-op `Makefile.MTLK` for `apps/mtlk_vb_upnpd/`.

### FASE 2: Hardware GPIO, WPS Button & Reset Lock Recovery
- **Problem**: GPIO 13 (WPS) and GPIO 3 (Reset) suffered from resource contention, lockfile abandonment on `kill -9`, and reliance on disabled BusyBox applets.
- **Fixes Applied**:
  1. **BusyBox 1.8.1 `head` Applet Absence Fix**: BusyBox 1.8.1 in this firmware has `CONFIG_HEAD` disabled (`# CONFIG_HEAD is not set`). Scripts calling `head -c 1` or `head -n 1` failed silently, disabling the hardware reset button completely!
     - In `rootfs-star/root/mtlk/etc/mtlk_pbc_reboot.sh` and VB: Replaced `head -c 1 "$RESET_GPIO"` with blocking kernel driver EOF read: `cat "$RESET_GPIO" >/dev/null 2>&1`.
  2. **WPS / Reset Contention**: Enforced that `WPS_PBC.sh` monitors `/dev/gpio13` only, while `mtlk_pbc_reboot.sh` monitors `/dev/gpio3`.
  3. **Lock Directory Recovery**: In `reload_mtlk_driver.sh` and `mtlk_restart_wps.sh`, replaced unsafe `kill -9` with graceful `kill` (SIGTERM), followed by `rm -rf /tmp/WPS_PBC_monitor.lock` with PID validation.
  4. **GPIO Variable Guard**: Guarded `[ -n "$B" ]` in `reload_mtlk_driver.sh` to prevent `grep "cat "` from killing unrelated processes.

### FASE 3: Init, Network, DHCP & Daemon Background Subsystems
- **Problem**: Faulty `/var` symlink causing `/var/var`, missing DHCP subnet options, infinite CPU spinning loops, and unhandled Tcl errors.
- **Fixes Applied**:
  1. **`/var` Mount Bind Architecture**: `rootfs-star/etc/inittab` previously ran `ln -s /tmp/var /var`, creating recursive `/var/var`. Corrected to create `/tmp/var/log` and `/tmp/var/lock` before executing `mount -o bind /tmp/var /var`, enabling `syslogd` to function reliably.
  2. **`udhcpdconf.tcl` Subnet Support**: Added CLI argument handling for `subnet` in `rootfs-star/etc/udhcpd/udhcpdconf.tcl` and `rootfs/etc/udhcpd/udhcpdconf.tcl`, preventing DHCP from ignoring custom netmasks.
  3. **`umedia_kill_process.sh` Loop Guard**: Added argument validation, self-PID exclusion (`$$`), 10-iteration ceiling, and `sleep 1` to prevent 100% CPU lockups.
  4. **`mtlk_upnpd_monitor.sh` Parsing**: Replaced disabled `head -n 1` with pure Awk single-line parsing: `awk 'NR==1 {print $3}'`, and resolved shell syntax error.
  5. **`mtlk_drvhlpr.sh` RMMOD Recovery**: Captured `rc=$?` immediately after executing `drvhlpr`, re-enabling driver reload branch on error code 2.
  6. **`auto_ap_restore_defaults.tcl`**: Added fallback path checking `/etc/udhcpc/*.conf` and synchronized configs to `saved_configs/`.
  7. **`mtlk_wps_msgloop.tcl`**: Prevented suicide during stop action by adding `$process_num != [pid]` guard.

### FASE 4: Userspace C Daemons & Flash Diet Hardening
- **Problem**: Severe buffer overflows and pointer dereferences in ancient C applications; unused static libraries wasting flash budget.
- **Fixes Applied**:
  1. **`apps/hnap_wps_status_monitor/hnap_wps_status_monitor.c`**:
     - Initialized `FILE *fptr = NULL;` in `main()` to prevent SIGSEGV in `gotSignal` handler upon receiving `SIGUSR1`.
     - Preserved status `99` instead of overwriting with status `14`.
     - Replaced unsafe `sscanf("%s")` with bounded `%15s`.
  2. **`apps/simpdiff/simpdiff.c`**:
     - Added boundary check `if (nameParam + 1 >= (unsigned int)argc)` to prevent `open(NULL)` (EFAULT crash) when executed as `simpdiff -s file1`.
     - Added explicit `close(file1)` on error exit paths.
  3. **`apps/force_ethpc_renew_ip/force_ethpc_renew_ip.c`**:
     - Guarded `getClientGateway` while-loop with `sizeof(buff) - 1` and checked for `buff[ifl] == '\0'` to prevent infinite memory reads.
     - Enforced width specifiers on `device[8]` buffer formatting.
  4. **`apps/busybox-1.8.1/networking/udhcp/` Options Hardening**:
     - Corrected offset index in `options.c`: `optionptr[i + OPT_LEN]`.
     - Guarded `end_option` against `DHCP_OPTIONS_BUFSIZE` stack buffer overflow.
     - Added stack capacity guard in `clientpacket.c` `add_requests` (`len < 308 - end - 3`).
     - Added null pointer checks on static lease queries in `files.c`.
  5. **Flash Diet Optimization**:
     - Removed unused `lib/libstdc++.so.6.0.3` (705 KB) and `usr/share/pci.ids` (489 KB) from `rootfs-star/` and `rootfs/`.
     - Dynamic linker verification confirmed zero userspace daemons depend on C++ runtime.
     - **Flash Savings**: Freed ~1.2 MB uncompressed (~400 KB compressed LZMA), ensuring the firmware remains safely at **~3.21 MB**, well below the **3.625 MB** hard brick ceiling.

### FASE 5: Deep Multi-Subsystem Audit & Production Hardening
Comprehensive audit and parallel remediation across all core firmware domains:

1. **Kernel, U-Boot, BSP & Hardware Drivers**:
   - **U-Boot 1.1.4 Recovery Overflow Fix**: Corrected safe recovery copy size from 6 MB (`0x5fffff`) to 3.625 MB (`0x39ffff`), corrected `CONFIGFS_OFFSET` to `0x103E0000`, and set `PHYS_FLASH_SIZE` to 4 MB in `star_orion_umedia.h`. This eliminates an instant hardware brick caused by address wrap-around into sector 0 (`0x10000000`).
   - **Ethernet MAC (`star_gsw.c`) NULL Pointer Guard**: Uncommented `goto freepacket;` when `skb_ptr->dev == NULL`, preventing kernel panic in `eth_type_trans()` and statistics increment.
   - **Ethernet Frame Short Padding Heap Guard**: Added `skb_tailroom` check prior to `memset` padding runt frames (<60 bytes), preventing out-of-bounds corruption of `skb_shared_info`.
   - **PHY GPIO Reset Guard (`star_gsw_phy.c`)**: Guarded `PHY_RESET_GPIO 3` with `#if defined(CONFIG_PHY_RESET_GPIO3)` to avoid clashing with the hardware factory reset button.
   - **Checksum Generator (`mkcsum.c`) Exit Code Fix**: Added argument validation, open descriptor checks, and changed exit code on invalid magic from 0 to 1 to fail the build cleanly on corruption.
   - **Kconfig ConfigFS Offset**: Changed default `CFG_OFFSET` from `0x7f0000` (8 MB) to `0x3e0000` (4 MB) in `mach-str9100/Kconfig`.
   - **GPIO Interrupt Masking (`str9100_gpio.c`)**: Serviced interrupt status registers using specific handled bitmasks rather than wiping all interrupts (`0x1FFFFF`).
   - **GPIO Userspace Stack Guard (`gpioDevice.c`)**: Replaced 512-byte static buffer with a bounded 16-byte stack buffer and handled `-EFAULT` on `copy_from_user`.

2. **WebServer, GoAhead Backend & Management UI**:
   - **Management Form Logic Inversion**: Inverted `rebootTag` logic in `management.c` (reboot only when `rebootTag="true"`, save configuration when `"false"`), added hidden `rebootTag` in `management.asp`, prevented blank password overwrites, and removed insecure web terminal markup.
   - **File Download Path Traversal Whitelist**: Enforced strict path whitelist in `sendFileForm` (`upldform.c`), permitting only `/tmp/configfile.conf`.
   - **Upload File Overwrite Restriction**: Hardcoded `upldForm` destination filename strictly to `/tmp/configfile.cfg`, ignoring client-supplied paths.
   - **BurnImage Partition Bounds & MTD Target**: Enforced hardcoded `/dev/mtdblock2` target and strict 3.625 MB partition limit (`KERNEL_MTD_PARTITION_SIZE`), preventing bootloader partition overwrites.
   - **Child Process Split-Brain Elimination**: Added `_exit(0)` to the child process in `upldForm` fork branch, preventing concurrent duplicate socket processing and zombie leaks.
   - **UDP Discovery Port 58000 Remote DoS Guard**: Validated authorization and header pointers in `device_discover.c` before executing `strchr` and `memcpy`.
   - **HNAP Dual-Band & 802.11n Throughput Restoration**: Restored dynamic `FrequencyBand` detection and fixed `ChannelBonding` being written to `IsHTEnabled` in `hnap_config_metalink.c`.
   - **CGI Execution Directory Isolation**: Confined CGI execution in `cgi.c` strictly to `/root/mtlk/web/cgi-bin/`.
   - **Firmware Flash Dumper Form Mapping**: Updated `upgrade.asp` links to call `/goform/dumpFirmware?type=...`.

3. **Wireless Radio, WPS & Network Automation Scripts**:
   - **WPS Button Poweroff Elimination**: Restored OEM WPS pairing trigger (`conf_via_pbc` / `get_conf_via_pbc`) in `WPS_PBC.sh`, eliminating rogue `poweroff` command.
   - **WPS Supervisor Tcl Sleep Crash**: Replaced invalid Tcl command `sleep 1` with native `after 1000` in `mtlk_wps_msgloop.tcl`.
   - **WPS Hardware LED Indication**: Integrated GPIO 2 LED controls in `mtlk_wps_event.sh` (blinking on session start, solid blue on success, off on completion).
   - **Dual-Band WSC & WEP Indexing**: Set `RF_BAND=3` for dual-band negotiation and normalized WEP transmit key to 1-based indexing in `mtlk_init_wps.sh`.
   - **Wireless Driver Recovery Loop**: Invoked `reload_mtlk_driver.sh` on `rc=2` in `mtlk_drvhlpr.sh`, shut down `wlan0` prior to `rmmod`, and added graceful timeout before SIGKILL.
   - **Hostapd Config Generation Optimization**: Rewrote `update_security.tcl` to use native Tcl file I/O instead of 25 subshell `exec echo` forks, validated 64-char hex PSKs, and stripped CRLF `\r` from passphrase generators.
   - **UPnP Watchdog Revival**: Added active process monitoring to `mtlk_upnpd_monitor.sh` and 1s delay in `dhcp.tcl` `restart_upnpd` to avoid port bind conflicts.

4. **System Initialization, BusyBox & Security Hardening**:
   - **Dropbear SSH Flag Fix**: Added compatibility no-op handling for `-B` in `svr-runopts.c` and cleaned up `dropbear` invocations across scripts, restoring SSH on port 22.
   - **Backdoor Removal & Assertion**: Deleted `shell.cgi` backdoor from all rootfs templates and added automated presence assertion in `docker/verify-firmware.sh`.
   - **BusyBox Applet Expansion**: Enabled `CONFIG_BB_SYSCTL=y`, `CONFIG_HEAD=y`, standalone IP applets, and created rootfs symlinks for `/sbin/sysctl` and `/bin/head`.
   - **Circular Buffer Logging & Serial Console**: Enabled in-memory `syslogd` (64 KB) and `klogd` in `inittab`, and configured serial console to respawn.
   - **Credential Security Hardening**: Locked empty `default` account (`*`) in `/etc/shadow`, enforced `0600` permissions on shadow and `0644` on passwd.
   - **Configuration Tools Reliability**: Added -1 return on read errors in `simpdiff.c`, and integrated `flock` and `fsync` in `set_env_param.c`.
   - **Build Cleansing**: Preserved `pgrep` in rootfs packaging and pruned stale test scripts and symlinks (`devel.sh`, `crc`, `iptables`).

### FASE 6: CLI Expansion, Remote Administration & Advanced System Diagnostics
Complete expansion of the system CLI capabilities, interactive monitoring, secure remote management, and flash footprint optimization:

1. **Dropbear SSH Remote Administration & Reverse Tunneling**:
   - Rebuilt the Dropbear multi-binary suite to produce `dropbear`, `dbclient`, and `dropbearkey`.
   - Linked `/usr/bin/ssh` directly to `/usr/bin/dbclient`.
   - Enabled reverse SSH tunneling (`ssh -R <remote_port>:localhost:22 user@remote_server` / `dbclient -R`), allowing seamless remote shell debugging and administration through NAT and firewall boundaries without port forwarding.
   - Root Shadow Password Authentication: Replaced empty root password hash in `/etc/shadow` with encrypted hash `admin` (`$1$x$zwNyxSLbfNOhQ2tYG39J10` MD5 crypt), enabling immediate authenticated remote access over both SSH (port 22) and Telnet (port 23).
   - Enabled Dropbear server daemon via network startup scripts (`/etc/mtlk_init_start.sh` and `/root/mtlk/etc/mtlk_init_daemons.sh`: `/usr/sbin/dropbear -p 22 &`), with `svr-runopts.c` supporting `-B` as a compatibility no-op.

2. **Advanced Real-Time Process & Resource Monitor (`/bin/sysmon`)**:
   - Developed a dedicated, zero-dependency ANSI-colored interactive and continuous process/resource monitor specifically tuned for 32 MB RAM and ARM922T 200 MHz.
   - **Accurate Memory Profiling**: Parses `/proc/meminfo` and calculates real available memory (`Free + Buffers + Cached`), Active/Inactive memory pages, Dirty cache, and Kernel Slab allocations.
   - **Granular CPU Breakdown**: Evaluates `/proc/stat` delta counters across configurable sampling intervals to display User, System, Idle, I/O Wait, and Hardware/Software IRQ percentages.
   - **Per-Process Resource Tracking**: Iterates over `/proc/[0-9]*/stat` and `/proc/[0-9]*/status` to calculate real-time CPU% and Resident Set Size (`VmRSS` in KB) per PID, sorting and displaying the top resource-consuming processes.
   - **Interactive & Batch Modes**: Supports continuous terminal refresh (`sysmon -l [sec]`, `sysmon -c`), custom sampling interval (`-d <sec>`), limited update cycles (`-n <count>`), and single-shot snapshots (`sysmon` without flags).
   - **Flash & RAM Diet**: Authored entirely in native POSIX shell and Awk, requiring 0 KB compiled binary storage and avoiding heavy third-party daemons like `htop` (~500 KB saving).

3. **Turing-Complete Lightweight CLI Utilities (`curl`, `json`, `nettest`, `flashdump`)**:
   - **`/bin/curl`**: Zero-dependency POSIX shell wrapper simulating modern `curl` functionality using BusyBox `wget` and direct raw `nc` socket streaming:
     - Supports HTTP methods: `GET`, `POST`, `HEAD`, `PUT`, `DELETE`.
     - Supports custom headers (`-H "Header: value"`), request payloads (`-d "data"` / `--data-binary`), and Basic Authentication (`-u username:password` powered by an in-line AWK Base64 encoder).
     - Supports output redirect (`-o <file>`, `-O`), silent mode (`-s`), and HTTP status/header inspection (`-i`, `-I`).
   - **`/bin/json`**: Lightweight streaming JSON parser and key extractor in pure Awk, enabling CLI scripts to inspect RESTful API endpoints and structured payloads.
   - **`/bin/nettest`**: Fast network throughput and socket latency benchmark tool using `nc` (netcat).
   - **`/bin/flashdump`**: Direct MTD partition dump utility to `/tmp` or stdout streaming (`flashdump <part> -`, e.g. `flashdump full -` over SSH) with partition boundary safety guards.

4. **Rich Interactive CLI Catalog (`/bin/help`) & Shell Environment (`/etc/profile`)**:
   - Replaced uninformative BusyBox help with a comprehensive, ANSI-colored command catalog covering:
     - Network & Wireless control (`iwconfig`, `iwpriv`, `brctl`, `hostapd_cli`).
     - System diagnostics & resource monitoring (`sysmon`, `top`, `free`, `dmesg`, `logread`).
     - Remote administration & transfer (`ssh`, `dbclient`, `dropbear`, `curl`, `wget`, `nc`).
     - Physical Flash MTD map reference with exact byte ranges and safety limits.
     - Hardware GPIO pinout matrix and diagnostic LED control (`/dev/led0`, `/dev/gpio*`).
   - Enhanced `/etc/profile` shell environment with custom prompt `[WAP610N:\w]# `, history settings, and convenient shortcuts (`ll`, `sysmon`, `ports`, `mem`).

5. **Flash Diet & WebUI Language Simplification (English-Only Enforced)**:
   - Replaced 12 redundant foreign language dictionary files (`STRINGS_FR.txt`, `STRINGS_DE.txt`, `STRINGS_ES.txt`, `STRINGS_IT.txt`, `STRINGS_NL.txt`, `STRINGS_PT.txt`, `STRINGS_RU.txt`, `STRINGS_PL.txt`, `STRINGS_DK.txt`, `STRINGS_SE.txt`, `STRINGS_NO.txt`, `STRINGS_FI.txt`) with relative symlinks pointing to `STRINGS_EN.txt`.
   - Replaced RTL stylesheet `linksys_arabic.css` with a symlink to `linksys.css`.
   - Removed non-functional language selector dropdown from `sta_network.asp`, locking the WebUI permanently to clean English.
   - **Flash Savings**: Freed ~180 KB uncompressed flash assets and prevented GoAhead heap fragmentation from parsing multi-megabyte language string tables into RAM.

---

## 9. 📋 Summary of Verified Defects Remediated

### 9.1 Phase 1–4 Non-WebUI Verified Remediation Matrix

| # | Subsystem | File Location | Defect Remediated | Status |
|---|---|---|---|---|
| 1 | Hardware Reset | `rootfs-star/root/mtlk/etc/mtlk_pbc_reboot.sh`<br>`apps/jffs2-etc/networking/VB/mtlk_pbc_reboot.sh` | Replaced disabled `head -c 1` with blocking driver read `cat "$RESET_GPIO" >/dev/null 2>&1`. | ✅ VERIFIED |
| 2 | UPnP Monitor | `rootfs-star/root/mtlk/etc/mtlk_upnpd_monitor.sh`<br>`apps/jffs2-etc/networking/VB/mtlk_upnpd_monitor.sh` | Replaced disabled `head -n 1` with `awk 'NR==1 {print $3}'`; removed extraneous `fi`. | ✅ VERIFIED |
| 3 | HNAP Monitor | `apps/hnap_wps_status_monitor/hnap_wps_status_monitor.c` | Moved `FILE *fptr = NULL;` into `main()`, eliminating SIGSEGV crash on signal reception. | ✅ VERIFIED |
| 4 | Config Diff | `apps/simpdiff/simpdiff.c` | Added `argc` boundary check preventing `open(NULL)` on incomplete CLI arguments. | ✅ VERIFIED |
| 5 | Network Helper | `apps/force_ethpc_renew_ip/force_ethpc_renew_ip.c` | Added buffer length boundary guard and null terminator checks in `getClientGateway`. | ✅ VERIFIED |
| 6 | DHCP Client | `apps/busybox-1.8.1/networking/udhcp/clientpacket.c` | Added stack overflow check in `add_requests` preventing corrupt option writes. | ✅ VERIFIED |
| 7 | Init & Logging | `rootfs-star/etc/inittab`<br>`rootfs/etc/inittab` | Added `mkdir -p /tmp/var/log /tmp/var/lock` prior to bind mount; restored `syslogd`. | ✅ VERIFIED |
| 8 | DHCP Server | `rootfs-star/etc/udhcpd/udhcpdconf.tcl`<br>`rootfs/etc/udhcpd/udhcpdconf.tcl` | Added CLI argument handling for `subnet` option configuration. | ✅ VERIFIED |
| 9 | Wireless Helper| `rootfs-star/root/mtlk/etc/mtlk_drvhlpr.sh`<br>`rootfs/root/mtlk/etc/mtlk_drvhlpr.sh` | Captured `rc=$?` immediately after `drvhlpr` execution to enable driver rmmod recovery. | ✅ VERIFIED |
| 10 | Restore Utility| `rootfs-star/etc/udhcpc/auto_ap_restore_defaults.tcl`<br>`rootfs/etc/udhcpc/restore_defaults.tcl` | Added fallback path checking `/etc/udhcpc/*.conf` and copied configs to `saved_configs/`. | ✅ VERIFIED |
| 11 | Network Init | `rootfs-star/etc/udhcpc/dhcp.tcl`<br>`rootfs/etc/udhcpc/dhcp.tcl` | Pre-initialized `current_ip` and `current_netmask` with fallback `get_parameter`. | ✅ VERIFIED |
| 12 | WPS Loop | `rootfs-star/root/mtlk/etc/mtlk_wps_msgloop.tcl`<br>`rootfs/root/mtlk/etc/mtlk_wps_msgloop.tcl` | Added `$process_num != [pid]` guard to prevent daemon from killing itself during stop. | ✅ VERIFIED |
| 13 | Rootfs Parity | Entire repository tree (`rootfs-star/`, `rootfs/`, `VB/`) | Synchronized all 23 modified paths 100% identically across source and build staging. | ✅ VERIFIED |

### 9.2 Comprehensive Master Codebase Remediation Matrix (Multi-Subagent Parallel Audit)

| # | Subsystem | File Location | Defect Remediated | Status |
|---|---|---|---|---|
| 14 | U-Boot Flash Safety | `u-boot-1.1.4/include/configs/star_orion_umedia.h` | Fixed recovery copy size from 6MB (`0x5fffff`) to 3.625MB (`0x39ffff`), corrected `CONFIGFS_OFFSET` to `0x103E0000`, and set `PHYS_FLASH_SIZE` to 4MB, eliminating hardware brick hazard on recovery. | ✅ VERIFIED |
| 15 | Ethernet Driver Panic | `kernel/linux-2.6.16-star/drivers/net/str9100/star_gsw.c` | Uncommented `goto freepacket;` when `skb_ptr->dev == NULL`, preventing NULL pointer dereference in `eth_type_trans()` and `priv->stats.rx_packets++`. | ✅ VERIFIED |
| 16 | Ethernet Frame Padding | `kernel/linux-2.6.16-star/drivers/net/str9100/star_gsw.c` | Added `skb_tailroom` check before `memset` on short frames (<60B), preventing heap corruption of `skb_shared_info`. | ✅ VERIFIED |
| 17 | Ethernet PHY Reset | `kernel/linux-2.6.16-star/drivers/net/str9100/star_gsw_phy.c` | Guarded `PHY_RESET_GPIO 3` with `#if defined(CONFIG_PHY_RESET_GPIO3)` to prevent driving the hardware factory reset button low. | ✅ VERIFIED |
| 18 | Packaging Checksum Tool | `kernel/linux-2.6.16-star/arch/arm/boot/bootp/mkcsum.c` | Added `argc < 2` usage check, file descriptor check, and changed error exit code from `exit(0)` to `exit(1)` on bad header magic. | ✅ VERIFIED |
| 19 | Kconfig Flash Offset | `kernel/linux-2.6.16-star/arch/arm/mach-str9100/Kconfig` | Changed default `CFG_OFFSET` from `0x7f0000` to `0x3e0000` for 4MB flash geometry. | ✅ VERIFIED |
| 20 | GPIO Interrupt Clear | `kernel/linux-2.6.16-star/drivers/char/str9100_gpio.c` | Replaced wholesale status wipe (`0x1FFFFF`) with serviced interrupt mask, preventing dropped concurrent button presses. | ✅ VERIFIED |
| 21 | Kernel GPIO Overflow | `apps/gpio_driver/gpioDevice.c` | Replaced static 512-byte buffer with bounded local stack buffer `char kbuf[16]`, bounded `count`, null-terminated, and returned `-EFAULT` on error. | ✅ VERIFIED |
| 22 | Management Logic Invert | `apps/WebServer/management.c`<br>`rootfs-star/.../admin/management.asp` | Inverted `rebootTag` handler logic (rebootTag="true" reboots; "false" saves config), prevented empty password overwrite, and removed web shell terminal. | ✅ VERIFIED |
| 23 | Arbitrary File Read | `apps/WebServer/upldform.c` | Enforced strict whitelist in `sendFileForm` allowing only `/tmp/configfile.conf`, eliminating arbitrary file read (CWE-22) of `/etc/passwd` or `/tmp/sys.conf`. | ✅ VERIFIED |
| 24 | Arbitrary File Overwrite| `apps/WebServer/upldform.c` | Locked upload destination in `upldForm` strictly to `/tmp/configfile.cfg`, preventing user-controlled `remote_filename` from overwriting system binaries. | ✅ VERIFIED |
| 25 | Bootloader Protection | `apps/WebServer/upldform.c` | Hardcoded `BurnImage` target to `/dev/mtdblock2` and enforced `KERNEL_MTD_PARTITION_SIZE` (3.625MB), preventing overwriting U-Boot bootloader at `mtdblock1`. | ✅ VERIFIED |
| 26 | Process Split-Brain | `apps/WebServer/upldform.c` | Added `_exit(0);` in child fork process of `upldForm` after configuration commit, eliminating socket duplication and memory leak. | ✅ VERIFIED |
| 27 | UDP Port 58000 DoS | `apps/WebServer/device_discover.c` | Added pointer validation in `dd_verify_authorization` and `dd_extract_header_value` before `strchr` dereference and `memcpy`, preventing remote DoS crash. | ✅ VERIFIED |
| 28 | HNAP 5GHz & HT Restore | `apps/WebServer/hnap_config_metalink.c` | Replaced hardcoded `FrequencyBand 1` with dynamic config read; fixed `ChannelBonding` being written to `IsHTEnabled`, restoring 802.11n high throughput on 20MHz width. | ✅ VERIFIED |
| 29 | CGI Path Traversal | `apps/WebServer/cgi.c` | Restricted CGI execution strictly to `/root/mtlk/web/cgi-bin/`, disallowing execution of scripts in the web document root directory. | ✅ VERIFIED |
| 30 | Flash Dump Links | `rootfs-star/.../admin/upgrade.asp`<br>`rootfs/.../admin/upgrade.asp` | Updated WebUI flash dump buttons to call `/goform/dumpFirmware?type=...` instead of broken `/cgi-bin/dump_firmware.cgi`. | ✅ VERIFIED |
| 31 | WPS Button Restore | `rootfs-star/.../mtlk/etc/WPS_PBC.sh`<br>`rootfs/.../mtlk/etc/WPS_PBC.sh`<br>`apps/jffs2-etc/networking/VB/WPS_PBC.sh` | Restored OEM WPS PBC pairing trigger (`conf_via_pbc` / `get_conf_via_pbc`), completely removing accidental system shutdown (`poweroff`) logic. | ✅ VERIFIED |
| 32 | WPS Tcl Loop Crash | `rootfs-star/.../mtlk/etc/mtlk_wps_msgloop.tcl`<br>`rootfs/.../mtlk/etc/mtlk_wps_msgloop.tcl` | Replaced non-existent Tcl command `sleep 1` in catch blocks with native `after 1000`, preventing supervisor crash. | ✅ VERIFIED |
| 33 | WPS Status LED | `rootfs-star/.../mtlk/etc/mtlk_wps_event.sh`<br>`rootfs/.../mtlk/etc/mtlk_wps_event.sh` | Added GPIO 2 LED handlers for WPS events (blinking on handshake, solid blue on success, off on error/timeout). | ✅ VERIFIED |
| 34 | WPS RF Band & WEP Index| `rootfs-star/.../mtlk/etc/mtlk_init_wps.sh`<br>`rootfs/.../mtlk/etc/mtlk_init_wps.sh` | Configured `RF_BAND=3` for Dual-Band WSC and normalized WEP Transmit Key Index to 1-based indexing. | ✅ VERIFIED |
| 35 | Wireless Driver Reload | `rootfs-star/.../mtlk/etc/mtlk_drvhlpr.sh`<br>`rootfs/.../mtlk/etc/reload_mtlk_driver.sh` | Invoked `reload_mtlk_driver.sh` on `rc=2`, brought `wlan0` down before `rmmod`, and added graceful delay between SIGTERM and SIGKILL. | ✅ VERIFIED |
| 36 | Security Script Rewrite | `rootfs-star/.../mtlk/web/update_security.tcl`<br>`rootfs/.../mtlk/web/update_security.tcl` | Replaced 25 `exec echo` subshell forks with native Tcl file I/O, added hex validation on 64-char PSK, and stripped CRLF `\r` in passphrase helper. | ✅ VERIFIED |
| 37 | UPnP Daemon Watchdog | `rootfs-star/.../mtlk/etc/mtlk_upnpd_monitor.sh`<br>`rootfs/.../mtlk/etc/mtlk_upnpd_monitor.sh`<br>`rootfs-star/etc/udhcpc/dhcp.tcl` | Added dead-process detection to restart crashed `upnpd`, and added 1s delay in `restart_upnpd` to prevent port 1900/49152 bind collisions. | ✅ VERIFIED |
| 38 | Dropbear Boot Crash | `rootfs-star/etc/mtlk_init_start.sh`<br>`rootfs-star/.../mtlk_init_daemons.sh`<br>`rootfs-star/.../remote_access.cgi` (and rootfs/VB) | Removed unsupported `-B` flag from Dropbear 0.52 invocations across all scripts, restoring SSH on port 22 on boot. | ✅ VERIFIED |
| 39 | Backdoor Eradication | `rootfs-star/.../web/cgi-bin/shell.cgi`<br>`devscripts/net-ver-mkrootfs.sh`<br>`docker/verify-firmware.sh` | Permanently deleted `shell.cgi` backdoor, purged mkrootfs copy rules, and added automated assertion gatekeeper in `verify-firmware.sh`. | ✅ VERIFIED |
| 40 | BusyBox Sysctl & Head | `boards/.../busybox/.config`<br>`rootfs-star/sbin/sysctl`<br>`rootfs-star/bin/head` | Enabled `CONFIG_BB_SYSCTL=y`, `CONFIG_HEAD=y`, standalone IP applets, and created symlinks in rootfs, restoring sysctl memory tuning and web monitor. | ✅ VERIFIED |
| 41 | Syslogd & Klogd Active | `rootfs-star/etc/inittab`<br>`rootfs/etc/inittab` | Activated circular buffer logging (`syslogd -n -m 0 -C 64`, `klogd -n`), and updated serial console to `ttyS0::respawn:-/bin/sh`. | ✅ VERIFIED |
| 42 | Shadow Hardening | `rootfs-star/etc/shadow`<br>`devscripts/net-ver-mkrootfs.sh` | Locked empty `default` account (`*`), and enforced restrictive permissions `0600` for shadow and `0644` for passwd. | ✅ VERIFIED |
| 43 | Simpdiff I/O Error Fix | `apps/simpdiff/simpdiff.c` | Added check for read error (`numRead < 0`) returning -1, preventing failed reads from falling through to return 0 (identical). | ✅ VERIFIED |
| 44 | Set Env Param Fsync | `apps/set_env_param/set_env_param.c` | Added `flock(fileno(f), LOCK_EX)` on `/dev/mtdblock3` and `fsync(fileno(tmp_f))` after write, preventing corrupt NVRAM environment. | ✅ VERIFIED |
| 45 | Pgrep Preservation | `apps/scripts/Makefile.MTLK.files` | Removed `/usr/bin/pgrep` from `FILES_UNUSED` so it is preserved in rootfs across builds. | ✅ VERIFIED |
| 46 | Stale Files Cleanup | `rootfs-star/.../web/crc`<br>`rootfs-star/usr/local/lib/iptables`<br>`apps/scripts/Makefile.MTLK` (`devel.sh`) | Removed unstripped 12KB `crc` from web root, removed dangling `iptables` symlink, and stopped copying internal test script `devel.sh`. | ✅ VERIFIED |
| 47 | Dropbear Boot Freeze | `rootfs-star/etc/mtlk_init_start.sh`<br>`rootfs/etc/mtlk_init_start.sh` | Backgrounded Dropbear host key generation `( ... ) &`, eliminating 10-minute early boot freeze on ARM9 200MHz. | ✅ VERIFIED |
| 48 | Console Shell Conflict | `rootfs-star/etc/inittab`<br>`rootfs/etc/inittab` | Removed duplicate `console::askfirst:-/bin/sh` from inittab, leaving a single clean `ttyS0::respawn:-/bin/sh` listener. | ✅ VERIFIED |
| 49 | Platform Path Missing | `rootfs-star/root/mtlk/etc/mtlk_init_platform.sh` | Restored missing `mtlk_init_platform.sh` to `rootfs-star`, defining `$CONFIGS_PATH` and config locations. | ✅ VERIFIED |
| 50 | Wi-Fi ESSID & Radio Beacon | `rootfs-star/.../driver_api.ini`<br>`rootfs-star/.../mtlk_init.sh`<br>`rootfs-star/.../mtlk_init_wls.sh`<br>`rootfs-star/.../mtlk_init_route.sh` | Mapped `NonProc_ESSID` in `driver_api.ini`, added default AP config restore fallback, and executed `ifconfig wlan0 up` for AP mode, restoring live 802.11n beacon transmission. | ✅ VERIFIED |
| 51 | Pre-Baked Dropbear Keys | `rootfs-star/etc/dropbear/`<br>`rootfs/etc/dropbear/` | Pre-generated static 1024-bit RSA and DSS host keys in rootfs, completely eliminating CPU-intensive background keygen on boot (~8 min at 100% CPU) and preserving persistent SSH host key fingerprints. | ✅ VERIFIED |
| 52 | Plug-and-Play DHCP Server | `rootfs-star/.../default_ap_WAP610N.conf`<br>`rootfs-star/.../sys.conf`<br>`rootfs-star/etc/udhcpd/udhcpd.conf` | Enabled internal `udhcpd` DHCP server by default (`DHCPDEnabled = 1`, `ip_config_method = 1`) handing out `192.168.1.10`-`.20` with DNS `192.168.1.1`/`8.8.8.8`, enabling instant zero-config Wi-Fi & LAN client connectivity. | ✅ VERIFIED |
| 53 | Sys.conf Fallback Restoration | `rootfs-star/.../mtlk_init.sh`<br>`rootfs-star/.../sys.conf.default` | Fixed `mtlk_init.sh` fallback to restore full `sys.conf` from `$ETC_PATH/sys.conf` or complete `sys.conf.default`, preventing truncated system config when `/mnt/jffs2` is unpopulated. | ✅ VERIFIED |
| 54 | Factory Reset & Hardware Reset Button Failsafe | `rootfs-star/root/mtlk/etc/mtlk_pbc_reboot.sh`<br>`rootfs-star/root/mtlk/etc/mtlk_restore_defaults.sh`<br>`rootfs-star/bin/config_mount.sh`<br>`rootfs-star/.../factory_defaults.asp` | Changed reset button GPIO 3 callback from `mtlk_restore_defaults.sh 0 1` (STA mode) to `1 1` (AP mode), locked WAP610N AP identity in restore script, added `/bin/config_save.sh` before reboot, and changed WebUI factory defaults button from `restoreToSTA()` to `restoreToAP()`. | ✅ VERIFIED |
| 55 | Clean AP Ext2 Flash Fallback Image | `rootfs-star/root/mtlk/etc/config.img.gz`<br>`rootfs/root/mtlk/etc/config.img.gz` | Baked clean ext2 loopback image with `ip_lan = 192.168.1.1`, `DHCPDEnabled = 1`, `network_type = 2`, `WirelessMgmtEnabled = 1`, and `NonProc_WPS_ActivateWPS = 0`, eliminating legacy fallback to `192.168.1.227` and disabled DHCP. | ✅ VERIFIED |
| 56 | DHCP Client/Server Bridge Race Elimination | `rootfs-star/root/mtlk/etc/mtlk_init_daemons.sh`<br>`rootfs-star/etc/udhcpc/dhcp.tcl` | Disabled `udhcpc` client startup when `DHCPDEnabled == 1` (AP mode), preventing IP conflicts on `br0`, and guarded `dhcp.tcl` to never reset bridge IP to `0.0.0.0` on failed lease. | ✅ VERIFIED |
| 57 | Wi-Fi Carrier Flapping & Mobile Association | `rootfs-star/.../default_ap_WAP610N.conf`<br>`rootfs-star/.../config.img.gz` | Disabled WPS on default Open SSID (`NonProc_WPS_ActivateWPS = 0`), preventing modern smartphone association/handshake failures that caused continuous `mtlk.ko` carrier off/on bridge state flapping. | ✅ VERIFIED |
| 58 | WebUI Ping Deadlock & Fork Storm Elimination | `apps/WebServer/hnap_config_metalink.c`<br>`apps/WebServer/mt_api.o`<br>`apps/WebServer/security.c`<br>`apps/WebServer/webs.c`<br>`apps/WebServer/default.c`<br>`apps/WebServer/LINUX/main.c` | Fixed `/bin/ping` without `-c` hanging `pclose()` in GoAhead by providing non-blocking C implementation, weakened legacy `MT_detect_link_with_ping` in `mt_api.o`, cached `brctl showmacs` for 5s in `security.c` (cutting CPU load by 95%), enabled `WirelessMgmtEnabled = 1`, removed `socketSetBlock(1)` in `websDone()`, and removed ASP `waitOtherProcess` redirect loop. | ✅ VERIFIED |
| 59 | Dynamic ASP Keep-Alive Socket Hang | `apps/WebServer/default.c` | Cleared `WEBS_KEEP_ALIVE` flag on dynamic ASP pages to force `Connection: close`, preventing browser request hangs and endless spinner. | ✅ VERIFIED |
| 60 | Socket Input Buffered Busy-Spin 100% CPU | `apps/WebServer/sock.c` | Added bounded select timeout (10-50ms) and sanity counter when sockets have buffered input, eliminating 100% CPU busy-loop on stalled clients. | ✅ VERIFIED |
| 61 | Form Handler CLOSE_WAIT Socket Leaks | `apps/WebServer/form.c`<br>`apps/WebServer/handler.c` | Drained unread request payloads and ensured socket cleanup on form validation/redirect errors, eliminating descriptor exhaustion and `CLOSE_WAIT` leaks. | ✅ VERIFIED |
| 62 | Firmware Flashing Worker Premature Exit & MTD Truncation | `apps/WebServer/upldform.c` | Removed premature `return 1;` that killed worker thread before write, eliminated buggy `fopen(..., "w+b")` reopening loop that truncated `/dev/mtdblock2` mid-flash, and restored bit-for-bit verification loop. | ✅ VERIFIED |
| 63 | WebUI Upgrade State Machine & XML Parser Crash | `rootfs-star/.../admin/upgrade.asp`<br>`rootfs/.../admin/upgrade.asp` | Removed invalid `overrideMimeType('text/xml')` for CSV status responses, added handler for state 6 (`verifying_e`), added automatic retry on transient HTTP error, enforced file selection validation, and added cache-busting timestamp. | ✅ VERIFIED |
| 64 | Non-Blocking MTD Flash Dumper | `apps/WebServer/upldform.c` | Rewrote `formDumpFirmware` with 64KB non-blocking chunked streaming, socket select timeouts, zero RAM buffering, and live partition streaming. | ✅ VERIFIED |
| 65 | Outbound SSH Client & Reverse Tunneling | `rootfs-star/usr/bin/dbclient`<br>`rootfs-star/usr/bin/ssh`<br>`rootfs-star/etc/shadow` | Built Dropbear multi-binary with `dbclient` and `ssh` symlink, enabled reverse SSH tunneling (`-R`), and set root shadow password to `admin`, restoring password authentication. | ✅ VERIFIED |
| 66 | Real-Time Process & Resource Monitor | `rootfs-star/bin/sysmon`<br>`rootfs/bin/sysmon` | Implemented lightweight (~3.5 KB) ANSI-colored monitor in POSIX sh + Awk displaying accurate memory (Buffers+Cached+Slab), CPU breakdown, and per-PID CPU% & VmRSS usage with 0 KB binary footprint. | ✅ VERIFIED |
| 67 | Lightweight CLI Applet Suite | `rootfs-star/bin/curl`<br>`rootfs-star/bin/json`<br>`rootfs-star/bin/nettest`<br>`rootfs-star/bin/flashdump` | Implemented Turing-complete CLI suite using native BusyBox tools: POSIX `curl` wrapper with Basic Auth, Awk JSON parser, netcat bandwidth tester, and partition flash dumper. | ✅ VERIFIED |
| 68 | Command Catalog & WebUI Language Diet | `rootfs-star/bin/help`<br>`rootfs-star/etc/profile`<br>`rootfs-star/.../STRINGS_*.txt`<br>`rootfs-star/.../sta_network.asp` | Created rich ANSI command catalog `/bin/help`, enhanced shell prompt and aliases, symlinked 12 unused foreign language tables to English, and locked WebUI to English-only (saving ~180 KB flash). | ✅ VERIFIED |

### 9.3 Future Architectural Roadmap & Planned Next Steps

Planned enhancements and features to be implemented in subsequent iterations:

1. **Custom Dropbear SSH Key Upload & Sovereign Cryptography**:
   - **Context & Motivation**: Static host keys (`dropbear_rsa_host_key` and `dropbear_dss_host_key`) are pre-baked into `rootfs-star/etc/dropbear/` to eliminate CPU-intensive boot-time key generation (~8 minutes at 100% CPU on 200MHz ARM9). To give users complete transparency, cryptographic trust, and ownership, users should have the ability to supply their own externally generated RSA host key and SSH public keys (`authorized_keys`).
   - **Architecture & Implementation Plan**:
     - **External Keygen**: User generates their own 1024-bit RSA host key on their trusted PC (e.g. `dropbearkey -t rsa -f custom_host_key` or `ssh-keygen`).
     - **Upload Mechanism**: Add a secure file upload handler in GoAhead WebServer (`management.asp` / `upldform.c`) or CLI utility that saves the custom key into persistent non-volatile storage (`/mnt/jffs2/dropbear/`).
     - **Boot Priority Fallback**:
       ```bash
       if [ -f /mnt/jffs2/dropbear/dropbear_rsa_host_key ]; then
           cp /mnt/jffs2/dropbear/dropbear_rsa_host_key /etc/dropbear/dropbear_rsa_host_key
       elif [ -f /root/mtlk/etc/dropbear/dropbear_rsa_host_key ]; then
           cp /root/mtlk/etc/dropbear/dropbear_rsa_host_key /etc/dropbear/dropbear_rsa_host_key
       fi
       ```
     - **Result**: Zero boot-time CPU penalty, user-controlled cryptography, and complete transparency without relying solely on image-bundled keys.

---

## 10. 🧪 QA Protocols & Build Reference

### 10.1 Quick Build Commands Reference

```bash
# ━━━ 1. Validate Prerequisites (Run Before Every Build) ━━━
./devscripts/validate-build-prerequisites.sh

# ━━━ 2. Containerized Full Compilation (Recommended) ━━━
./docker/compile.sh WAP610N

# ━━━ 3. Verify Firmware Integrity & Partition Budget ━━━
./docker/verify-firmware.sh output/bootpImage

# ━━━ 4. Local Make Pipeline (Requires Local Toolchain) ━━━
make configure   # Reconfigures board platform
make build       # Compiles kernel, apps, and packages rootfs
make clean       # Cleans intermediate build objects
```

### 10.2 Quality Assurance Verification Checklist
Before staging or committing any code changes:
- [x] **Flash Budget Check**: Ensure compiled `bootpImage` ≤ 3,801,088 bytes (3.625 MB).
- [x] **C89 Compliance**: Compile userspace C code using `arm-linux-uclibc-gcc` without C99 constructs.
- [x] **Shell Syntax Check**: Verify all shell scripts across `rootfs-star/`, `rootfs/`, and `devscripts/` with `bash -n` or `sh -n`.
- [x] **Tcl Syntax Check**: Verify Tcl scripts using `echo 'source script.tcl; puts OK' | tclsh`.
- [x] **Whitespace & Diff Sanity**: Run `git diff --check` to ensure no whitespace errors, carriage returns (`\r`), or merge conflicts.
- [x] **Rootfs Synchronization**: Ensure any script modified in `rootfs-star/` is copied identically to `rootfs/` and `apps/jffs2-etc/networking/VB/`.

---

## 11. 🚨 Deep Codebase Audit & 80-Bug Remediation Specification (Phases 1–5 Complete)


Dokumen ini adalah spesifikasi teknis lengkap (**Super-Detailed Engineering Audit & Remediation Blueprint**) yang mendokumentasikan **80 temuan bug terverifikasi** di seluruh *codebase* **Cisco Linksys WAP610N / WET610N (`linux-2.6.16-star` / `STR8132` / `ARM922T`)**.

Setiap temuan dilengkapi dengan:
1. **ID & Klasifikasi Prioritas** (`P0-CRITICAL`, `P1-HIGH`, `P2-MEDIUM`, `P3-LOW`)
2. **Lokasi File & Baris Presisi**
3. **Jejak Eksekusi & Akar Masalah Teknis (*Root Cause Analysis*)**
4. **Dampak Sistem / Hardware** (RAM 32 MB, Flash NOR 4 MB batas `0x3A0000`, CPU ARM922T 200 MHz, ABI `uClibc` / GCC 3.4.6 C89)
5. **Cuplikan Kode Bermasalah vs Spesifikasi Patch Perbaikan (*Exact Remediation Diff*)**

---

### 11.1 📊 Matriks Ringkasan Eksekutif & Peta Prioritas (80 Verified Bugs)

| Domain | Scope Subsystem | P0 (Critical) | P1 (High) | P2 (Medium) | P3 (Low) | Total |
|---|---|---:|---:|---:|---:|---:|
| **Domain 1** | GoAhead WebServer C Backend (`apps/WebServer/`) | 6 | 8 | 5 | 1 | **20** |
| **Domain 2** | WebUI Frontend, JS, ASP & Tcl (`rootfs-star/root/mtlk/web/`) | 3 | 6 | 5 | 2 | **16** |
| **Domain 3** | Runtime Shell, Tcl, Init & CLI Applets (`rootfs-star/etc/`, `bin/`, `root/mtlk/etc/`) | 3 | 5 | 6 | 2 | **16** |
| **Domain 4** | Build Pipeline, Packaging & RootFS Parity (`Makefile`, `make_nv.sh`, `devscripts/`, `docker/`) | 3 | 6 | 6 | 3 | **18** |
| **Domain 5** | Kernel 2.6.16-star, U-Boot 1.1.4, Drivers & Native Apps (`kernel/`, `u-boot-1.1.4/`, `apps/`) | 1 | 4 | 4 | 1 | **10** |
| **TOTAL** | **Seluruh Repository `/workspaces/wap610n`** | **16** | **29** | **26** | **9** | **80** |

#### Urutan Fase Eksekusi Perbaikan (Remediation Roadmap)

```mermaid
flowchart LR
    P0["🔥 FASE 1: P0 CRITICAL (16 Bug)\n• Cegah SIGSEGV/Crash WebServer\n• Perbaiki JS u-media.js & AP Mode\n• Hidupkan Dropbear SSH Saat Boot\n• Perbaiki Docker Build & simpdiff\n• Perbaiki U-Boot NVRAM Writer"] --> P1["⚡ FASE 2: P1 HIGH (29 Bug)\n• Perbaiki Upgrade UI & Flash Dump\n• Ganti after -> sleep di tinytcl\n• Cegah OOM /tmp di nettest/flashdump\n• Aktifkan Build Dropbear & Fix Perms\n• Tambal Etherleak & GPIO OOB"]
    P1 --> P2["🛠️ FASE 3: P2 MEDIUM (26 Bug)\n• Kepatuhan Penuh C89 GCC 3.4.6\n• Optimasi CPU hnap_wps & simpdiff\n• Perbaiki Alias BusyBox & Skrip Tcl\n• Hardening Build Fail-Fast"]
    P2 --> P3["🧹 FASE 4: P3 LOW (9 Bug)\n• Cleanup File/Sisa Multi-Bahasa\n• Strip Modul & Hapus Static Lib\n• Sinkronisasi Dokumentasi AGENTS.md"]
```

---

### 11.2 🔥 Domain 1: GoAhead WebServer C Backend (`apps/WebServer/` — `W-01` s/d `W-20`)

#### 2.1 `[W-01]` [P0-CRITICAL] Allocator Mismatch (`malloc`/`free` vs `balloc`/`bfree`) pada `wp->postData`
- **Lokasi File**:
  - `apps/WebServer/webs.c` (baris `488–537`, `2348–2354`)
  - `apps/WebServer/upldform.c` (baris `942–946`)
- **Akar Masalah Teknis**:
  GoAhead memiliki allocator pool internal (`balloc` / `bfree` / `brealloc`) yang menyimpan metadata ukuran blok tepat di bawah pointer yang dikembalikan. Namun, untuk menampung upload firmware berukuran hingga 3.625 MB yang melampaui kapasitas `balloc`, `webs.c:532` dan `webs.c:536` mengalokasikan `wp->postData` menggunakan fungsi `malloc()` dari sistem (`uClibc`):
  ```c
  /* apps/WebServer/webs.c:528-537 */
  if (strcmp(wp->url, "/goform/upldImage") == 0) {
      if(wp->postData == NULL) {
          wp->postData = (char *)malloc(wp->lenPostData * sizeof(char));
      }
  } else {
      /* WEBS_HNAP_FW_UPGRADE */
      wp->postData = (char *)malloc(wp->lenPostData * sizeof(char));
  }
  ```
  Sebaliknya, fungsi pembersih koneksi `websFree(webs_t wp)` di `webs.c:2350` **selalu** memanggil `bfree(B_L, wp->postData)`:
  ```c
  /* apps/WebServer/webs.c:2349-2352 */
  if (wp->postData) {
      bfree(B_L, wp->postData);
      wp->postData = NULL;
  }
  ```
  Terdapat **3 skenario fatal**:
  1. **Upload `/goform/upldImage` terputus di tengah jalan** (user menutup tab, jaringan putus, atau timeout sebelum `BurnImage()` sempat mengeksekusi `free(wp->postData); wp->postData = NULL;` di `upldform.c:944`): Saat koneksi ditutup, `websFree()` memanggil `bfree()` pada pointer `malloc()`. `bfree()` membaca header pool palsu di `wp->postData - sizeof(bhead)` → **Instant `SIGSEGV` / korupsi heap**.
  2. **Setiap request `WEBS_HNAP_FW_UPGRADE`**: Handler HNAP tidak pernah men-set `wp->postData = NULL`. Begitu request selesai dan `websFree()` dipanggil, `bfree()` dipanggil pada buffer `malloc()` → **100% crash `SIGSEGV`**.
  3. **Bypass `//goform/upldImage` (lihat `[W-12]`)**: `wp->postData` dialokasikan dengan `brealloc(B_L, ...)` di cabang `else` (`webs.c:580`), tetapi handler form tetap memanggil `BurnImage()` yang menjalankan `free(wp->postData)` pada pointer `balloc()` → **Instant crash**.
- **Spesifikasi Perbaikan**:
  1. Definisikan flag baru di `apps/WebServer/webs.h`:
     ```c
     #define WEBS_SYS_MALLOC_POST 0x8000000 /* wp->postData allocated via system malloc() */
     ```
  2. Saat mengalokasikan `wp->postData` dengan `malloc()` di `webs.c:532, 536`, pasang flag `wp->flags |= WEBS_SYS_MALLOC_POST;`.
  3. Di `websFree()` (`webs.c:2350`) dan `BurnImage()` (`upldform.c:944`), periksa `(wp->flags & WEBS_SYS_MALLOC_POST)` untuk memilih `free(wp->postData)` vs `bfree(B_L, wp->postData)`.

---

#### 2.2 `[W-02]` [P0-CRITICAL] Retry Loop Rusak di `MT_BurnAndVerifyImage()` & File Descriptor `/dev/mtdblock2` Bocor
- **Lokasi File**:
  - `apps/WebServer/upldform.c` (baris `135–194`, `232–239`)
  - `apps/WebServer/hnap_config_metalink.c` (baris `1468–1545`)
- **Akar Masalah Teknis**:
  Di dalam `MT_BurnAndVerifyImage()`, variabel penghitung progres tulis `locWrite = 0` dan `numLeft = gMTBurnParams.imageSize` diinisialisasi **di luar** loop `for (retryCount = 0; retryCount < MAX_IMAGE_WRITE_RETRIES; retryCount++)`:
  ```c
  /* apps/WebServer/upldform.c:135-185 */
  locWrite = 0;
  numLeft = gMTBurnParams.imageSize;
  writeSize = gMTBurnParams.imageSize > 16384 ? 16384 : gMTBurnParams.imageSize;
  for (retryCount = 0 ; retryCount < MAX_IMAGE_WRITE_RETRIES ; retryCount++) {
      if ((fp = fopen("/dev/mtdblock2", "w+b")) == NULL) { ... }
      while (numLeft > 0) {
          ...
          numLeft -= numWrite;
          locWrite += numWrite;
      }
      if (numLeft == 0) {
          fflush(fp);
          fsync(fileno(fp));
          fclose(fp);
          if(VerifyImage() == TRUE) { ... break; }
      }
  }
  ```
  - Pada iterasi pertama (`retryCount = 0`), loop `while (numLeft > 0)` berjalan hingga `numLeft == 0`.
  - Jika `VerifyImage()` gagal (misalnya ada bit-flip atau gangguan tulis blok NOR flash), loop masuk ke `retryCount = 1`.
  - Pada `retryCount = 1` dan `retryCount = 2`, `numLeft` **masih bernilai `0`**! Fungsi membuka `/dev/mtdblock2` dengan `"w+b"`, melewati seluruh `while (numLeft > 0)` tanpa menulis 1 byte pun, langsung memanggil `VerifyImage()` terhadap isi flash yang rusak tadi, dan gagal permanen!
  - Lebih buruk lagi: jika `fwrite()` gagal di tengah jalan (`numWrite < writeSize`), kode melakukan `break` keluar dari `while` saat `numLeft > 0`. Karena `fclose(fp)` berada di dalam `if (numLeft == 0)`, file descriptor `fp` ke `/dev/mtdblock2` **bocor (tidak pernah di-close)** pada setiap retry!
  - Bug identik juga terduplikasi di `hnap_BurnAndVerifyImage()` (`apps/WebServer/hnap_config_metalink.c:1468–1545`).
- **Spesifikasi Perbaikan**:
  - Pindahkan inisialisasi `locWrite = 0;`, `numLeft = gMTBurnParams.imageSize;`, dan `writeSize = ...;` ke **dalam** awal body `for (retryCount = 0; ...)`.
  - Jika `fwrite` gagal (`numLeft != 0`), panggil `fclose(fp); fp = NULL;` sebelum melanjutkan ke iterasi retry berikutnya.

---

#### 2.3 `[W-03]` [P0-CRITICAL] HNAP Firmware Upgrade Tanpa Gate Ukuran Partisi `0x3A0000` (Risiko Brick Permanen) & Bug `fopen("ab")` + `fseek()`
- **Lokasi File**:
  - `apps/WebServer/webs.c` (baris `498`)
  - `apps/WebServer/hnap_config_metalink.c` (baris `1449–1455`, `1613–1709`, `1785`)
- **Akar Masalah Teknis**:
  1. **Ketiadaan Pengecekan Batas `0x3A0000` (3,801,088 byte)**:
     Di `webs.c:498`, batas alokasi untuk `WEBS_HNAP_FW_UPGRADE` diset `getFlashSize() + 0x50000` (`0x3D0000 + 0x50000 = 0x420000` = **4,325,376 byte**). Kemudian di `hnap_validate_Image()` (`hnap_config_metalink.c:1613–1709`), kode hanya mengecek header dan checksum tanpa pernah memeriksa `gHnapBurnParams.imageSize <= KERNEL_MTD_PARTITION_SIZE` (`0x3A0000`). Jika firmware > 3.625 MB dikirim melalui SOAP `FirmwareUpload`, proses akan menulis melewati batas `mtdblock2` atau gagal di tengah jalan dan meninggalkan flash korup!
  2. **Bug Standar C89 `fopen("/dev/mtdblock2", "ab")` + `fseek()`**:
     Di `hnap_config_metalink.c:1449–1452`:
     ```c
     if ((fp = fopen("/dev/mtdblock2", "ab")) != NULL) {
         fseek(fp, imageHeader.pcksum2, SEEK_SET);
         fwrite(dummy_crc, 1, sizeof(dummy_crc), fp);
         fclose(fp);
     }
     ```
     Dalam spesifikasi C89 dan POSIX `fopen`, membuka file dengan mode `"a"` atau `"ab"` (*append mode*) memaksa **semua operasi `fwrite()` selalu menulis di akhir file (EOF)**, mengabaikan posisi yang diset oleh `fseek()`! Akibatnya `dummy_crc` tidak pernah ditulis di offset `imageHeader.pcksum2`, melainkan di ujung akhir `/dev/mtdblock2` (`0x3A0000`)!
- **Spesifikasi Perbaikan**:
  - Di `webs.c:498`, samakan batas maksimum `WEBS_HNAP_FW_UPGRADE` menjadi `0x3A0000 + 65536` (sama dengan `/goform/upldImage`).
  - Di `hnap_validate_Image()`, tambahkan validasi tegas `if (gHnapBurnParams.imageSize <= 0 || gHnapBurnParams.imageSize > 0x3A0000) return FALSE;`.
  - Di `hnap_config_metalink.c:1449`, ubah `fopen("/dev/mtdblock2", "ab")` menjadi `fopen("/dev/mtdblock2", "r+b")`.

---

#### 2.4 `[W-04]` [P0-CRITICAL] Penggunaan `vfork()` di `management.c` Membekukan HTTP Response, Merusak Flag `waitOtherProcess`, dan Menjalankan `killchildweb.sh`
- **Lokasi File**:
  - `apps/WebServer/management.c` (baris `72–88`, `101–177`)
- **Akar Masalah Teknis**:
  Saat refactoring proses GoAhead sebelumnya, `vfork()` di `upldform.c` telah diubah menjadi `fork()`, namun **2 fungsi kritis di `apps/WebServer/management.c` terlewat**:
  1. `resetToDefault()` (baris `72`):
     ```c
     pid = vfork();
     if(pid == 0) {
         waitOtherProcess = 1;
         waitRedirectPage();
         system("/root/mtlk/etc/default_settings.sh > /dev/null");
         system("sync");
         system("reboot");
         waitOtherProcess = 0;
         _exit(0);
     }
     ```
  2. `management()` (baris `113–175`):
     ```c
     pid = vfork();
     if(pid == 0) {
         waitRedirectPage();
         ...
         system("cd /tmp; tar zcvf /tmp/config.tar.gz ...");
         system("dd if=/tmp/config.tar.gz of=/dev/mtdblock4");
         system("/root/mtlk/web/killchildweb.sh&");
         waitOtherProcess = 0;
         _exit(0);
     } else {
         waitOtherProcess = 1;
     }
     ```
  **3 Dampak Fatal dari `vfork()` di atas**:
  - **Parent HTTP Membeku**: `vfork()` menghentikan eksekusi parent (`webs`) sampai child memanggil `_exit(0)` atau `execve()`. Karena parent baru memanggil `websRedirect(wp, "reboot_page.asp")` **setelah** blok `if (pid == 0)`, browser tidak pernah menerima HTTP redirect selama child sedang menjalankan `waitRedirectPage()`, `tar`, dan `dd` ke flash!
  - **Korupsi State `waitOtherProcess` (Shared Memory)**: Karena `vfork()` berbagi ruang alamat memori dengan parent, di `management()` child berjalan lebih dulu dan men-set `waitOtherProcess = 0` tepat sebelum `_exit(0)`. Begitu child keluar, parent baru bangun dan mengeksekusi cabang `else { waitOtherProcess = 1; }`! Akibatnya `waitOtherProcess` **terkunci permanen di nilai `1`** setelah admin menyimpan password!
  - **Pemanggilan Skrip Legacy `killchildweb.sh`**: Baris `163` memanggil `/root/mtlk/web/killchildweb.sh&` yang merupakan warisan arsitektur dual-process lama.
- **Spesifikasi Perbaikan**:
  - Ganti `vfork()` menjadi `fork()` di `resetToDefault()` dan `management()`.
  - Di dalam child process (`pid == 0`), tutup socket HTTP (`close(wp->fd)` jika perlu atau biarkan parent menutupnya setelah redirect), hapus manipulasi `waitOtherProcess` yang tidak sinkron, dan hapus panggilan `system("/root/mtlk/web/killchildweb.sh&")`.

---

#### 2.5 `[W-05]` [P0-CRITICAL] Unauthenticated Remote Crash (`SIGSEGV`) via Paket UDP 2-Byte ke Port 58000 (`dd_process_packet`)
- **Lokasi File**:
  - `apps/WebServer/device_discover.c` (baris `1256–1298`)
- **Akar Masalah Teknis**:
  Daemon `webs` membuka socket UDP pada port `58000` (`dd_init`) yang dapat diakses tanpa autentikasi dari LAN. Saat paket diterima, `dd_process_packet(char *buf, int dataLen)` mem-parsing header:
  ```c
  /* apps/WebServer/device_discover.c:1263-1285 */
  while(dataRead < dataLen) {
      if((*ePtr == '\r') && (*(ePtr+1) == '\n')) {
          if ((*(ePtr+2) == '\r') && (*(ePtr+3) == '\n')) {
              ...
              break;
          }
          dataRead = dataRead + 2;
      }
      ePtr++;
      dataRead++;
  }
  dataRead = dataRead + 2;
  bodyLen = dataLen - dataRead;
  if(bodyLen) {
      body = (char *)malloc(bodyLen * sizeof(char));
      memset(body, 0, bodyLen);
  ```
  1. **Out-of-Bounds Read**: Ketika `dataRead == dataLen - 2` dan dua byte terakhir adalah `\r\n`, ekspresi `*(ePtr+2)` dan `*(ePtr+3)` membaca 2 byte di luar batas buffer `buf`.
  2. **Integer Underflow → `memset(NULL, 0, -2)` Crash**: Jika penyerang mengirim paket UDP 2 byte berisi `"\r\n"` (`dataLen = 2`):
     - Di dalam loop, `dataRead` ditambah `2` lalu `1` menjadi `3`, sehingga loop berhenti.
     - Di luar loop, `dataRead = dataRead + 2` menjadi `5`.
     - `bodyLen = dataLen - dataRead = 2 - 5 = -3` (atau `-2` untuk paket kosong `""` di mana `dataLen = 0, dataRead = 2, bodyLen = -2`).
     - Karena `if (bodyLen)` bernilai **TRUE** untuk angka negatif (`-2` atau `-3`), `malloc((size_t)-2)` meminta alokasi ~4 GB pada RAM 32 MB, yang langsung mengembalikan `NULL`.
     - Baris berikutnya langsung memanggil `memset(NULL, 0, -2)` → **Instant `SIGSEGV` crash pada proses `webs`!**
- **Spesifikasi Perbaikan**:
  - Cek batas `if (dataLen < 4) return -1;` di awal `dd_process_packet()`.
  - Pastikan pengecekan `(*(ePtr+2) == '\r')` hanya dilakukan jika `dataRead + 3 < dataLen`.
  - Ubah `if (bodyLen)` menjadi `if (bodyLen > 0 && bodyLen <= 65536)` dan selalu cek `if (body == NULL) return -1;`.

---

#### 2.6 `[W-06]` [P0-CRITICAL] Unchecked `gstrstr()` `NULL` Dereference & 32-Byte Stack Buffer Overflow di `startMultiLangParser()`
- **Lokasi File**:
  - `apps/WebServer/asp.c` (baris `165–245`)
- **Akar Masalah Teknis**:
  Fungsi `startMultiLangParser()` mem-parsing tag `<!--#tr id="..." -->...<!--#endtr-->` pada setiap file `.asp`:
  ```c
  /* apps/WebServer/asp.c:171-236 */
  char tagTmp[32], tagBuf[32];
  if((cp = gstrstr(newBuf, "<!--#tr")) != NULL) {
      ep = gstrstr(cp, "-->");
      *ep = '\0'; /* [BUG 1] Crash jika "-->" tidak ditemukan (ep == NULL)! */
      ...
      begin = gstrstr(cp, "id=\"");
      end = gstrstr(begin+4, "\"");
      memcpy(tagTmp, begin, end - begin); /* [BUG 2] Overflow jika panjang > 32 byte! */
      ...
      ep = gstrstr(cp, "-->");
      *ep = '\0'; /* [BUG 3] Crash jika "<!--#endtr" tidak punya "-->"! */
  }
  ```
  - Jika ada tag `<!--#tr` atau `<!--#endtr` yang tidak ditutup dengan `-->`, `ep` bernilai `NULL` dan `*ep = '\0'` langsung memicu `SIGSEGV`.
  - Jika atribut `id="..."` memiliki panjang lebih dari 28 karakter, `memcpy(tagTmp, begin, end - begin)` dan `memcpy(tagBuf, begin+4, end - begin - 4)` menimpa stack frame (`tagTmp[32]` dan `tagBuf[32]`).
- **Spesifikasi Perbaikan**:
  - Tambahkan pengecekan `if (ep == NULL || begin == NULL || end == NULL) break;`.
  - Batasi panjang copy: `size_t idLen = (size_t)(end - (begin + 4)); if (idLen >= sizeof(tagBuf)) idLen = sizeof(tagBuf) - 1;`.

---

#### 2.7 `[W-07]` [P1-HIGH] Pre-Auth 3.8 MB Upload Allocation Guard (`S2`) Dapat Di-bypass dengan Header `Authorization:` Palsu Apa Saja
- **Lokasi File**:
  - `apps/WebServer/webs.c` (baris `496–503`, `1051–1060`, `1083–1092`)
- **Akar Masalah Teknis**:
  Di `webs.c:497–502`, kode mencoba membatasi alokasi `maxUpload` bagi request yang belum terautentikasi:
  ```c
  int isAuth = (wp->flags & (WEBS_AUTH_BASIC | WEBS_AUTH_DIGEST)) ||
               (wp->user && wp->user[0] != '\0') ||
               (wp->password && wp->password[0] != '\0');
  int maxUpload = isAuth ? (getFlashSize() + 0x50000) : WEBS_MAX_REQUEST_UNAUTH;
  ```
  Namun, saat GoAhead mem-parsing header HTTP di `websGetInput()` (`webs.c:1059` dan `1091`), **begitu melihat header `Authorization: Basic ...` apa pun**, GoAhead langsung men-set `wp->flags |= WEBS_AUTH_BASIC` dan mengisi `wp->password` — **sebelum** `websSecurityHandler()` sempat dipanggil untuk memvalidasi apakah password tersebut benar!
  Akibatnya, penyerang tanpa password cukup mengirim:
  ```http
  POST /goform/upldImage HTTP/1.1
  Authorization: Basic YWRtaW46wrongpassword
  Content-Length: 3866624
  ```
  `isAuth` bernilai `1`, dan `webs` mengalokasikan 3.86 MB RAM per koneksi. Cukup 5–6 koneksi paralel untuk menghabiskan sisa RAM 6 MB dan memicu Kernel Panic OOM!
- **Spesifikasi Perbaikan**:
  Buat fungsi helper `websCheckUploadAuth(webs_t wp)` yang memverifikasi `wp->password` terhadap `umGetUserPassword("admin")` (atau session cookie yang valid) sebelum mengizinkan `maxUpload > WEBS_MAX_REQUEST_UNAUTH`.

---

#### 2.8 `[W-08]` [P1-HIGH] Upload `/goform/upldImage` yang Terputus (`wp->clen > 0`) Tidak Memanggil `websDone()`
- **Lokasi File**:
  - `apps/WebServer/webs.c` (baris `538–568`)
- **Akar Masalah Teknis**:
  Saat menerima body `/goform/upldImage`, ketika `nbytes == 0` (EOF / koneksi diputus klien) atau `socketEof(wp->sid)` tercapai tetapi `wp->clen > 0` (upload belum lengkap), blok `if (wp->clen == 0)` dilewati dan **tidak ada cabang `else`**. Koneksi tidak ditutup dengan `websDone(wp, 400)` sehingga buffer `wp->postData` sebesar 3.8 MB tetap menggantung di RAM selama 60 detik hingga socket timeout.
- **Spesifikasi Perbaikan**:
  Tambahkan cabang `else { websError(wp, 400, T("Incomplete upload")); }` ketika stream berakhir sementara `wp->clen > 0`.

---

#### 2.9 `[W-09]` [P1-HIGH] `formDumpFirmware()` Mengirim `Content-Length` yang Salah untuk `mtd2` (+64 KB) dan `mtd4` (-64 KB) → `ERR_CONTENT_LENGTH_MISMATCH`
- **Lokasi File**:
  - `apps/WebServer/upldform.c` (baris `963–1052`)
  - `kernel/linux-2.6.16-star/drivers/mtd/maps/str9100.c` (baris `175–210`)
- **Akar Masalah Teknis**:
  Di `formDumpFirmware()` (`upldform.c:984–991`), ukuran partisi yang dikirim pada header HTTP `Content-Length` di-hardcode secara keliru:
  ```c
  /* apps/WebServer/upldform.c:984-991 */
  if (strcmp(part, "mtd0") == 0)      { dev = "/dev/mtd0"; fname = "mtd0_full_4MB.bin";   size = 0x400000; }
  else if (strcmp(part, "mtd1") == 0) { dev = "/dev/mtd1"; fname = "mtd1_uboot.bin";      size = 0x20000; }
  else if (strcmp(part, "mtd2") == 0) { dev = "/dev/mtd2"; fname = "mtd2_kernel_fw.bin";  size = 0x3B0000; } /* SALAH! */
  else if (strcmp(part, "mtd3") == 0) { dev = "/dev/mtd3"; fname = "mtd3_nvram_env.bin";  size = 0x10000; }
  else if (strcmp(part, "mtd4") == 0) { dev = "/dev/mtd4"; fname = "mtd4_configfs.bin";   size = 0x10000; }  /* SALAH! */
  ```
  Berdasarkan tabel partisi kernel `str9100.c` dan `/proc/mtd` (serta `AGENTS.md` §3):
  - `mtd2` (`0x10040000` – `0x103E0000`) berukuran **`0x3A0000` (3,801,088 byte)**, bukan `0x3B0000` (3,866,624 byte). Saat mendownload `mtd2`, `fread()` berhenti di EOF pada 3,801,088 byte (kurang 65,536 byte dari `Content-Length`), sehingga browser modern (Chrome/Firefox/Edge) dan `curl` menggagalkan download dengan **`net::ERR_CONTENT_LENGTH_MISMATCH`**!
  - `mtd4` (`0x103E0000` – `0x10400000`) berukuran **`0x20000` (131,072 byte)**, bukan `0x10000` (65,536 byte). Header mengirim `Content-Length: 65536`, tetapi loop `while ((n = fread(...)) > 0)` mengirim 131,072 byte!
- **Spesifikasi Perbaikan**:
  Ubah `size` untuk `mtd2` menjadi `0x3A0000` dan `mtd4` menjadi `0x20000`, atau lebih baik lagi: baca ukuran aktual menggunakan `fseek(fp, 0, SEEK_END); size = ftell(fp); fseek(fp, 0, SEEK_SET);` (atau parse `/proc/mtd`) sebelum mengirim header `Content-Length`.

---

#### 2.10 `[W-10]` [P1-HIGH] `NULL` Dereference (`memcmp(wp->FileContent, ...)`) & Bypass `numLeft <= 0` di `upldForm()`
- **Lokasi File**:
  - `apps/WebServer/upldform.c` (baris `450–476`)
- **Akar Masalah Teknis**:
  Di `upldForm()` (handler restore konfigurasi `/goform/upldForm`):
  ```c
  numLeft = wp->FileContentLen;
  if(numLeft > maxWrite) {
      error_flag = 1;
  } else {
      if(memcmp(wp->FileContent, ConfSIG, sizeof(ConfSIG)-1) == 0) { ... }
  ```
  Jika klien mengirim POST tanpa file (`wp->FileContent == NULL`, `wp->FileContentLen == 0`) atau upload gagal dengan kode negatif (`UPLOAD_FILE_TO_BIG = -1`), kondisi `numLeft > maxWrite` bernilai `FALSE`, lalu kode masuk ke `else` dan memanggil `memcmp(NULL, ConfSIG, 24)` → **Instant `SIGSEGV` crash!**
- **Spesifikasi Perbaikan**:
  Ubah kondisi menjadi `if (wp->FileContent == NULL || numLeft <= (int)sizeof(ConfSIG) || numLeft > maxWrite)`.

---

#### 2.11 `[W-11]` [P1-HIGH] Stack Buffer Overflow (`redirectPage[512]`) & Reflected XSS via Parameter `statusPage` di `BurnImage()`
- **Lokasi File**:
  - `apps/WebServer/upldform.c` (baris `305–315`, `805–820`, `950`)
- **Akar Masalah Teknis**:
  Di `BurnImage()` (`upldform.c:805–819`):
  ```c
  char_t statusPageName[MT_MAX_LINE_LENGTH]; /* 512 bytes */
  char_t redirectPage[MT_MAX_LINE_LENGTH];   /* 512 bytes */
  ...
  gstrncpy(statusPageName, statusPage, MT_MAX_LINE_LENGTH - 1);
  sprintf(redirectPage, "%s?burnState=%d", statusPageName, gMTBurnParams.burnState);
  ```
  1. `statusPageName` adalah array stack yang tidak diinisialisasi (`statusPageName[511]` tidak diset `'\0'`). Jika input `statusPage` >= 511 byte, `statusPageName` tidak memiliki null-terminator.
  2. `sprintf(redirectPage, "%s?burnState=%d", statusPageName, ...)` menulis hingga `511 + 22 = 533` byte ke dalam buffer `redirectPage[512]` → **Stack Buffer Overflow 21 byte**.
  3. Di `MT_WriteRedirectScript()` (`upldform.c:309`), `redirectPage` diinjeksikan tanpa sanitasi ke `<script language=javascript>window.location = '/%s'</script>` → **Reflected XSS**.
- **Spesifikasi Perbaikan**:
  Gunakan `snprintf(redirectPage, sizeof(redirectPage), ...)` dan validasi bahwa `statusPage` hanya berisi karakter path relatif yang aman (`[a-zA-Z0-9_./-]`).

---

#### 2.12 `[W-12]` [P1-HIGH] `//goform/upldImage` Melewati Streaming Upload Handler di `webs.c:488`
- **Lokasi File**:
  - `apps/WebServer/webs.c` (baris `488`)
  - `apps/WebServer/handler.c` (baris `267`)
- **Akar Masalah Teknis**:
  Saat menerima body POST di `webs.c:488`, kode memeriksa `if (strcmp(wp->url, "/goform/upldImage") == 0)`. Padahal normalisasi slash ganda (`websCondenseMultipleChars(wp->path, '/')`) baru dijalankan belakangan di `handler.c:267`! Jika klien mengirim POST ke `//goform/upldImage`, `strcmp` di `webs.c:488` bernilai `FALSE` (jatuh ke `brealloc` di memori `balloc`), tetapi saat dispatch handler, `wp->path` dinormalisasi menjadi `/goform/upldImage` dan memanggil `BurnImage()` (yang kemudian memanggil `free()` pada buffer `balloc()`).
- **Spesifikasi Perbaikan**:
  Gunakan helper perbandingan path yang melewati leading `/` berlebih (mis. ` { const char *u = wp->url; while (*u == '/') u++; if (strcmp(u, "goform/upldImage") == 0) ... }`).

---

#### 2.13 `[W-13]` [P1-HIGH] Out-of-Bounds Read Melewati Null Terminator & Negative `FileContentLen` di `extractUploadedFileContent()`
- **Lokasi File**:
  - `apps/WebServer/webs.c` (baris `2880–2920`)
- **Akar Masalah Teknis**:
  ```c
  /* apps/WebServer/webs.c:2895-2913 */
  tmpStartStr = strstr(part, "filename=");
  if(tmpStartStr == NULL) return -1;
  tmpStartStr = &tmpStartStr[10]; /* "filename=" hanya 9 byte! Jika tanpa '"', lompat melewati '\0'! */
  ...
  tmpStartStr = strstr(part, "form-data; name=");
  tmpStartStr = &tmpStartStr[17]; /* "form-data; name=" hanya 16 byte! */
  ...
  wp->FileContentLen = endPart - part - 2; /* Bisa bernilai -1 atau -2 jika endPart == part! */
  ```
- **Spesifikasi Perbaikan**:
  Cek keberadaan karakter kutip `"` secara eksplisit (`if (tmpStartStr[9] != '"') return -1;`) dan pastikan `endPart > part + 2` sebelum menghitung `wp->FileContentLen`.

---

#### 2.14 `[W-14]` [P1-HIGH] Unchecked `NULL` Dereference & 5-Byte Stack Overflow (`quality[5]`) di `hnap_prepare_ap_list()`
- **Lokasi File**:
  - `apps/WebServer/hnap_config_metalink.c` (baris `769–1103`)
- **Akar Masalah Teknis**:
  1. Array `char quality[5], signal[5], noise[5];` dideklarasikan hanya 5 byte, namun diisi dengan nilai seperti `"100/100"` (8 byte termasuk `\0`) dari output `iwlist wlan0 scanning` → **Stack Buffer Overflow**.
  2. Hasil pencarian `index()`, `strstr()`, dan `rindex(listBuf, '\t')` langsung di-dereference (`*endPtr = 0x00` di baris `1092`) tanpa cek `NULL`. Jika semua AP di sekitar menyembunyikan SSID (*hidden SSID*), tidak ada karakter `\t` di `listBuf`, `rindex` mengembalikan `NULL`, dan `*endPtr = 0x00` langsung men-crash `webs` (`SIGSEGV`).
- **Spesifikasi Perbaikan**:
  Perbesar ukuran buffer `quality[16], signal[16], noise[16]` dan tambahkan pengecekan `if (endPtr != NULL)` pada seluruh pemanggilan `index`/`rindex`/`strstr`.

---

#### 2.15 `[W-15]` s/d `[W-20]` [P2-MEDIUM / P3-LOW] Temuan Tambahan Backend C GoAhead
- **`[W-15]` [P2-MEDIUM] Pelanggaran Disiplin C89 (Mid-Block Declarations & VLA Struct)** (`AGENTS.md` §1.2):
  - `apps/WebServer/LINUX/main.c:108, 128, 138` (`char_t *clean_path`, `FILE *fp`)
  - `apps/WebServer/upldform.c:71, 207` (`char tmp[64]`, `unsigned char buf[16384]`)
  - `apps/WebServer/default.c:80` (`char_t *clean_lpath`)
  - `apps/WebServer/asp.c:287, 356` (`char *newBuf`, `int len`)
  - `apps/WebServer/hnap_config_metalink.c:131` (VLA struct `struct { char data[size]; } Data;` di dalam `write_data()`)
- **`[W-16]` [P2-MEDIUM] Unchecked `popen()` Return Value (`NULL` → `fgets` Crash)**:
  - `apps/WebServer/hnap_config_metalink.c:517, 2223`: `fp = popen(...)` langsung diteruskan ke `fgets(buf, ..., fp)` tanpa cek `if (fp == NULL)`.
- **`[W-17]` [P2-MEDIUM] Unchecked `strstr(tmpPtr, "dBm")` `NULL` Dereference**:
  - `apps/WebServer/hnap_config_metalink.c:594–631`: Jika output `iwconfig` tidak memuat string `"dBm"`, `endPtr` bernilai `NULL` dan `*endPtr = 0x00` men-crash `webs`.
- **`[W-18]` [P2-MEDIUM] Memory Leak `xml_encode_tempbuf` via UDP Port 58000**:
  - `apps/WebServer/device_discover.c:795`: `dd_encode_str()` mengalokasikan `xml_encode_tempbuf` dengan `malloc()` tetapi pemanggilnya tidak pernah memanggil `free()`, memungkinkan penyerang LAN menguras RAM 32 MB lewat paket UDP port 58000.
- **`[W-19]` [P2-MEDIUM] Synchronous `sleep(1)` Loop (15 Detik) Memblokir Event Loop Single-Thread GoAhead**:
  - `apps/WebServer/hnap_config_metalink.c:2129, 2183, 2274`: Loop menunggu hasil Site Survey memanggil `sleep(1)` hingga 15 kali secara sinkron di thread utama HTTP.
- **`[W-20]` [P3-LOW] Unchecked `tempnam()` `NULL` Dereference**:
  - `apps/WebServer/LINUX/main.c:461`: `websGetCgiCommName()` memanggil `strcpy(pname, tempnam(NULL, T("cgi")))` tanpa mengecek apakah `tempnam` mengembalikan `NULL`.

---

### 11.3 🌐 Domain 2: WebUI Frontend, JS, ASP & Tcl (`rootfs-star/root/mtlk/web/` — `F-01` s/d `F-16`)

```mermaid
flowchart TD
    subgraph Bug_F01["🔥 Bug F-01: Kerusakan Global u-media.js"]
        J1["Browser meminta GET /u-media.js"] --> J2["GoAhead default.c:218\nhanya mem-parse <!--#tr--> di .asp, BUKAN .js!"]
        J2 --> J3["Browser menerima literal:\nalert(<!--#tr id=...-->'IP is empty.'<!--#endtr-->);"]
        J3 --> J4["Di JS, '<!--' adalah komentar 1 baris!\nMenelan ');' di ujung baris"]
        J4 --> J5["💥 Uncaught SyntaxError: Unexpected token '}'\nSeluruh fungsi u-media.js UNDEFINED di semua halaman!"]
    end
```

#### 3.1 `[F-01]` [P0-CRITICAL] `SyntaxError` JavaScript di `u-media.js` (Mematikan Seluruh Fungsi Helper di Semua Halaman WebUI) & `network/sta_network.asp`
- **Lokasi File**:
  - `rootfs-star/root/mtlk/web/u-media.js` (baris `87`, `90`, `110`, `166`)
  - `apps/WebServer/default.c` (baris `218`)
  - `rootfs-star/root/mtlk/web/network/sta_network.asp` (baris `101`)
- **Akar Masalah Teknis**:
  1. **Kerusakan Total `/u-media.js`**:
     Di `apps/WebServer/default.c:218`, preprocessor multi-bahasa `MT_PreParsePage()` hanya dipanggil jika ekstensi file adalah `.asp`:
     ```c
     if (strcmp(ext, ".asp") == 0) {
         MT_PreParsePage(lpath, &newBuf, &size);
     }
     ```
     File `/u-media.js` disajikan sebagai file statis biasa **tanpa** melalui `MT_PreParsePage()`. Padahal di dalam `u-media.js` baris `87`, `90`, `110`, dan `166` terdapat tag translasi:
     ```javascript
     /* rootfs-star/root/mtlk/web/u-media.js:87 */
     alert(<!--#tr id="u-media.js.1" -->"IP Address is empty."<!--#endtr-->);
     ```
     Dalam spesifikasi ECMAScript (semua browser), token `<!--` di dalam kode JavaScript diperlakukan sebagai **komentar satu baris** (setara dengan `//`) hingga karakter newline `\n`!
     Akibatnya, browser melihat baris 87 sebagai:
     ```javascript
     alert( // #tr id="u-media.js.1" -->"IP Address is empty."<!--#endtr-->);
     ```
     Kurung buka `alert(` tidak pernah ditutup, sehingga baris berikutnya (`return false;` diikuti `}`) melempar **`Uncaught SyntaxError: Unexpected token '}'`**!
     Karena `u-media.js` gagal di-parse oleh engine JS browser, **seluruh fungsi global di dalamnya (`objectDisplay`, `transSSID`, `hexToString`, `checkIP`, `checkMask`, `checkInteger`, dll.) berstatus `undefined` di setiap halaman WebUI**!
  2. **Kerusakan Halaman `network/sta_network.asp:101`**:
     ```javascript
     /* rootfs-star/root/mtlk/web/network/sta_network.asp:101 */
     alert(<!--#tr id="u-media.js.11" -->"Please input 1 ~ 9999"<!--#endtr-->);
     ```
     Berbeda dengan tag lain yang menaruh tanda kutip di *luar* `<!--#tr-->`, baris 101 menaruh tanda kutip `"..."` di *dalam* blok `<!--#tr-->`. Ketika `startMultiLangParser` (`asp.c:228`) mengganti seluruh blok `<!--#tr...#endtr-->` dengan teks kamus `Please input 1 ~ 9999` (yang tidak memiliki tanda kutip), HTML yang dikirim ke browser menjadi:
     ```javascript
     alert(Please input 1 ~ 9999);
     ```
     Ini langsung memicu **`Uncaught SyntaxError: missing ) after argument list`**, yang mematikan seluruh fungsi `page_load()`, `apply()`, dan `cancel()` di halaman Network!
- **Spesifikasi Perbaikan**:
  1. Di `rootfs-star/root/mtlk/web/u-media.js` (dan `rootfs/root/mtlk/web/u-media.js`), hapus semua tag `<!--#tr ...-->` dan `<!--#endtr-->` pada baris `87`, `90`, `110`, `166` sehingga menjadi string JS bersih:
     ```javascript
     alert("IP Address is empty.");
     alert("IP length is over 15Characters.");
     alert("IP is invalid.");
     alert("The IP address cannot be a broadcast address.");
     ```
  2. Di `rootfs-star/root/mtlk/web/network/sta_network.asp:101`, pindahkan tanda kutip keluar dari tag translasi (atau langsung gunakan `"Please input 1 ~ 9999"`).

---

#### 3.2 `[F-02]` [P0-CRITICAL] Halaman `station/wireless_basic.asp` Gagal Menampilkan Security di Mode AP (`mode == 2`) & Menghapus Konfigurasi Radio Saat Disimpan
- **Lokasi File**:
  - `rootfs-star/root/mtlk/web/station/wireless_basic.asp` (baris `225–327`, `483`, `573–590`, `755–761`)
  - `apps/WebServer/wireless.o` (fungsi `wirelessBasic` pada offset `0xd14..0xf74`)
- **Akar Masalah Teknis**:
  Terdapat **4 bug kritis** yang saling bertumpuk di `station/wireless_basic.asp`:
  1. **Mode AP (`mode == 2`) Tidak Diinisialisasi di `selectMode(mode)`**:
     Di baris `227`:
     ```javascript
     function selectMode(mode) {
         if (mode*1 == 0) //STA mode
         {
             ... /* Mengisi seluruh field security & memanggil selectSecurityMode() */
         }
     }
     ```
     Saat WAP610N beroperasi sebagai **Access Point (`mode == 2`)**, `selectMode(2)` tidak mengeksekusi apa pun! Seluruh tabel pengaturan keamanan Wi-Fi (WPA2-PSK, WEP, passphrase) tetap tersembunyi (`display: none`) dan kosong!
  2. **Menyimpan Form dengan `nowMode == "apMode"` Merusak `/mnt/jffs2/wlan0.conf` & Melewati `wirelessSecurity`**:
     Ketika `mode == 2`, `page_load()` men-set hidden input `nowMode = "apMode"`. Saat form di-submit ke `/goform/wirelessBasic`, fungsi C `wirelessBasic()` di `wireless.o` melihat `nowMode == "apMode"` lalu:
     - Membaca parameter form `country`, `channel`, `channelBounding`, dan `upperLower` — **padahal keempat input tersebut tidak ada di `wireless_basic.asp`** (adanya di `wireless_advanced.asp`)!
     - Menulis nilai kosong `""` untuk `country`/`channel` dan memaksa `IsHTEnabled 0` (mematikan 802.11n) ke `/mnt/jffs2/wlan0.conf`!
     - **Tidak memanggil `wirelessSecurity()`** sama sekali (karena `wirelessSecurity` di `wireless.o` hanya dipanggil pada cabang `staMode`)!
  3. **Bug `checkWPAPSK(docTemp.length == 64)` di Baris `483`**:
     ```javascript
     /* station/wireless_basic.asp:483 */
     if (checkWPAPSK(docTemp.length == 64))
     ```
     Fungsi `checkWPAPSK(str)` di `u-media.js:239` mengharapkan parameter berupa **string** untuk dicek karakter hex-nya satu per satu (`for (i=0; i<str.length; i++)`). Karena dipanggil dengan ekspresi boolean `docTemp.length == 64` (`false` untuk passphrase 8–63 karakter), `str.length` bernilai `undefined`, loop tidak berjalan, dan `checkWPAPSK(false)` **selalu mengembalikan `true`**! Akibatnya barisan kode berikutnya selalu men-set `document.getElementById("wpaToHex").value = "hex"` untuk semua passphrase ASCII biasa!
  4. **Duplikasi `value="3"` pada `<select id="securityMode">` & Hilangnya `WPAPersonalMode`**:
     Di baris `759–760`:
     ```html
     <option value="3" id="wpa2">WPA2 - Personal</option>
     <option value="3" id="wpa">WPA - Personal</option>
     ```
     Kedua opsi memiliki `value="3"`, sehingga memilih WPA2 maupun WPA menghasilkan nilai yang sama, dan hidden input `WPAPersonalMode` yang dibutuhkan oleh `wireless.o:wirelessSecurity` tidak ada di form!
- **Spesifikasi Perbaikan**:
  - Di `selectMode(mode)`, izinkan inisialisasi field security baik untuk `mode == 0` maupun `mode == 2`.
  - Di `checkSubmit()`, perbaiki baris `483` menjadi `if (docTemp.length == 64 && checkWPAPSK(docTemp))`.
  - Tambahkan hidden input `WPAPersonalMode`, bedakan opsi WPA vs WPA2 (`selectSecurityMode`), dan pastikan parameter radio (`country`, `channel`, `channelBounding`, `upperLower`) disertakan sebagai hidden input dari `getParam` agar `wirelessBasic` tidak menimpa `wlan0.conf` dengan nilai kosong.

---

#### 3.3 `[F-03]` [P0-CRITICAL] `document.getElementById("wpsMode")` / `"wirelessAdv"` Bernilai `null` → `TypeError` Menghentikan `page_load()` di 3 Halaman Wireless
- **Lokasi File**:
  - `rootfs-star/root/mtlk/web/station/wireless_advanced.asp` (baris `139`)
  - `rootfs-star/root/mtlk/web/station/wmm.asp` (baris `446`, `572`)
  - `rootfs-star/root/mtlk/web/station/site_survey.asp` (baris `406`, `537`)
- **Akar Masalah Teknis**:
  Ketiga halaman menjalankan kode berikut di dalam `page_load()`:
  ```javascript
  document.getElementById("wirelessAdv").href="wireless_advanced.asp?random="+Math.random();
  document.getElementById("wpsMode").href="wps.asp?random="+Math.random();
  ```
  - Elemen dengan `id="wpsMode"` **tidak ada** di HTML ketiga halaman tersebut.
  - Elemen `id="wirelessAdv"` juga **tidak ada** pada tag `<a href="/station/wireless_advanced.asp">` di `wmm.asp:446` dan `site_survey.asp:537`.
  - Akibatnya, `document.getElementById(...)` mengembalikan `null`, dan pengaksesan `.href` melempar **`Uncaught TypeError: Cannot set properties of null (setting 'href')`**.
  - Eksekusi `page_load()` langsung berhenti di baris tersebut: di `wireless_advanced.asp` form gagal diinisialisasi; di `site_survey.asp` fungsi `makeRequest("/goform/updateSiteSurvey", ...)` di baris `409` tidak pernah terpanggil!
- **Spesifikasi Perbaikan**:
  Tambahkan guard `var el = document.getElementById(...); if (el) el.href = ...;` dan tambahkan atribut `id="wirelessAdv"` pada elemen `<a href="/station/wireless_advanced.asp">` di `wmm.asp` serta `site_survey.asp`.

---

#### 3.4 `[F-04]` [P1-HIGH] Autentikasi `cgi-bin/remote_access.cgi` Selalu Gagal (`AdminPassword` vs `Password`, Stripped `HTTP_AUTHORIZATION`, & Duplikasi Status Line HTTP)
- **Lokasi File**:
  - `rootfs-star/root/mtlk/web/cgi-bin/remote_access.cgi` (baris `4–14`, `89`)
  - `apps/WebServer/cgi.c` (baris `233`, `318`)
- **Akar Masalah Teknis**:
  1. `remote_access.cgi:4` mencari `grep -i "^AdminPassword"` di `/tmp/sys.conf`. Padahal file `/root/mtlk/etc/sys.conf` (yang disalin ke `/tmp/sys.conf`) menggunakan key **`Password = admin`**, bukan `AdminPassword`!
  2. Ketika password diset, `remote_access.cgi:6` memeriksa `[ -z "$HTTP_AUTHORIZATION" ]`. Namun `apps/WebServer/cgi.c:233` **tidak pernah meneruskan header `HTTP_AUTHORIZATION`** ke array environment CGI (demi keamanan standar CGI)! Akibatnya `$HTTP_AUTHORIZATION` selalu kosong dan CGI selalu menolak request dengan `401 Unauthorized`! (Padahal GoAhead sendiri sudah mengautentikasi sesi sebelum memanggil `websCgiHandler`).
  3. Di baris `89`, `remote_access.cgi` mencetak `echo "HTTP/1.0 200 OK"`, padahal `apps/WebServer/cgi.c:318` sudah menulis `HTTP/1.0 200 OK\r\n` ke socket klien!
- **Spesifikasi Perbaikan**:
  - Karena akses ke `/cgi-bin/*` sudah melewati `websSecurityHandler` di GoAhead (`security.c`), atau jika ingin validasi tambahan di `cgi.c`, teruskan `REMOTE_USER` dari `wp->user` di `cgi.c` dan hapus `echo "HTTP/1.0 200 OK"` di `remote_access.cgi:89`.

---

#### 3.5 `[F-05 & F-06]` [P1-HIGH] Kerentanan XSS (Reflected, Stored, dan Wi-Fi Proximity SSID XSS)
- **Lokasi File**:
  - `rootfs-star/root/mtlk/web/admin/management.asp` (baris `418–425`)
  - `rootfs-star/root/mtlk/web/station/site_survey.asp` (baris `58`)
  - `rootfs-star/root/mtlk/web/station/wps_status.asp` (baris `254`, `355–356`)
- **Akar Masalah Teknis**:
  1. Di `admin/management.asp:418–425`, parameter URL `?error=` diambil dari `window.location.search`, di-`unescape()`, lalu diinjeksikan ke `document.write('alert("'+error+'");')` → **Reflected XSS**.
  2. Di `station/site_survey.asp:58` dan `station/wps_status.asp:254`, nama SSID dari paket beacon Wi-Fi di udara dimasukkan langsung ke `.innerHTML` tanpa HTML-escaping → **Wi-Fi Proximity XSS** (AP penyerang di sekitar dengan SSID `<img src=x onerror=...>` dapat mengeksekusi JS di sesi admin).
- **Spesifikasi Perbaikan**:
  Gunakan helper encoding HTML (`& < > " '`) sebelum memasukkan SSID ke `.innerHTML` (atau gunakan `.textContent`), dan validasi/escape parameter `error` di `management.asp`.

---

#### 3.6 `[F-07]` [P1-HIGH] Ketiadaan Proteksi CSRF & State-Changing `GET` di `remote_access.cgi`
- **Lokasi File**:
  - `rootfs-star/root/mtlk/web/cgi-bin/remote_access.cgi` (baris `12–20`)
- **Akar Masalah Teknis**:
  `remote_access.cgi` menerima parameter `action=start&service=telnet` dari `QUERY_STRING` pada request `GET` biasa. Cukup dengan menyisipkan `<img src="http://192.168.1.1/cgi-bin/remote_access.cgi?service=telnet&action=start">` di halaman web luar, layanan Telnet/SSH dapat diaktifkan/dimatikan secara paksa jika admin sedang login.
- **Spesifikasi Perbaikan**:
  Wajibkan metode `POST` (`[ "$REQUEST_METHOD" = "POST" ]`) untuk semua aksi perubahan state (`start`/`stop`/`restart`) di `remote_access.cgi`.

---

#### 3.7 `[F-08]` [P1-HIGH] 5 Bug Kritis di `update_security.tcl` & `run_wpa_passphrase_for_update_security.sh`
- **Lokasi File**:
  - `rootfs-star/root/mtlk/web/update_security.tcl` (baris `26`, `53`, `107–113`, `276–280`, `378`)
  - `rootfs-star/root/mtlk/web/run_wpa_passphrase_for_update_security.sh` (baris `12`)
- **Akar Masalah Teknis**:
  1. **Path Binary Salah (`/tmp/wpa_passphrase`)**: `run_wpa_passphrase_for_update_security.sh:12` memanggil `/tmp/wpa_passphrase "$1" "$2"`, padahal binary `wpa_passphrase` berada di `/root/mtlk/etc/wpa_passphrase` (tidak pernah disalin ke `/tmp`)!
  2. **`Update_Supplicant_Conf` Gagal pada Mode `OPEN` / `WEP`**: Di `update_security.tcl:378`, `Update_Supplicant_Conf` selalu memanggil `run_wpa_passphrase_for_update_security.sh` di awal fungsi tanpa memeriksa apakah `NonProc_SecurityMode` adalah WPA-PSK! Saat mode `OPEN` atau `WEP`, PSK kosong membuat `wpa_passphrase` gagal (exit non-zero), lalu `Update_Supplicant_Conf` langsung `return` sebelum menulis `key_mgmt=NONE` atau kunci WEP!
  3. **Penimpaan `WEP_Authentication`**: Baris `26` dan `53` menyalin `NonProc_Authentication` ke `WEP_Authentication`, menimpa nilai autentikasi WEP.
  4. **Kunci WEP Tidak Ditulis ke `hostapd.conf`**: Di `Update_Hostapd_Conf` (baris `276–280`), cabang `WEP` tidak menulis `wep_default_key` dan `wep_key0..3`.
  5. **Perintah `after 2000` Tidak Valid di `tinytcl`**: Baris `109` memanggil `after 2000` yang menyebabkan crash `invalid command name "after"` (lihat `[RT-04]`).
- **Spesifikasi Perbaikan**:
  - Ubah `/tmp/wpa_passphrase` menjadi `/root/mtlk/etc/wpa_passphrase` di `run_wpa_passphrase_for_update_security.sh`.
  - Panggil `run_wpa_passphrase_for_update_security.sh` di `Update_Supplicant_Conf` **hanya** jika mode keamanan adalah WPA/WPA2-Personal.
  - Ganti `after 2000` di baris `109` dengan `sleep 2`.

---

#### 3.8 `[F-09]` [P1-HIGH] 4 Bug Alur Frontend Firmware Upgrade di `admin/upgrade.asp` & `upldform.c`
- **Lokasi File**:
  - `rootfs-star/root/mtlk/web/admin/upgrade.asp` (baris `151–195`, `368`, `374`)
  - `apps/WebServer/upldform.c` (baris `693`, `805–951`)
- **Akar Masalah Teknis**:
  1. **Redirect ke `/admin/admin/upgrade.asp` (404 Not Found) Saat Upgrade Selesai (`100%`)**:
     Di `admin/upgrade.asp:166`:
     ```javascript
     else if (burnState*1 == 3 && precentComplete*1 == 100) {
         clearTimeout(timeID);
         alert("Firmware Upgrade Completely! Wait a minute for warm reboot Finishing!");
         top.location.href = "admin/upgrade.asp"; /* BUG: Relative terhadap /admin/upgrade.asp! */
     }
     ```
     Karena halaman saat ini berada di `/admin/upgrade.asp`, navigasi relatif `"admin/upgrade.asp"` mengarahkan browser ke **`http://192.168.1.1/admin/admin/upgrade.asp` (404 Not Found)**!
  2. **False-Failure Alert Akibat State `precentComplete = -1` Basi Saat Upload Kedua**:
     Jika percobaan upload pertama gagal (misal file salah), `upldform.c:693` men-set `gMTBurnParams.precentComplete = -1`. Ketika user memilih file yang benar dan klik **Submit**, `checkSubmit()` memulai polling XHR `getBurnStatus.asp` setelah **1.5 detik** (`setTimeout("request Start()", 1500)`). Padahal file firmware 3.6 MB membutuhkan ~3–5 detik untuk selesai di-upload melalui HTTP sebelum `BurnImage()` dipanggil dan mereset `gMTBurnParams.precentComplete = 0`! Akibatnya, polling XHR pada detik ke-1.5 membaca nilai `-1` dari percobaan sebelumnya, langsung memunculkan `alert("Firmware Upgrade failed!")`, dan menghentikan polling (`stopProcess = true`)!
  3. **Duplikasi Halaman `upgrade.asp` di Dalam Hidden Iframe `rebootRedirect`**:
     Form upload memiliki `target="rebootRedirect"` (iframe 1x1 piksel). Setelah `BurnImage()` memvalidasi image dan mem-fork proses flash, `upldform.c:951` memanggil `MT_WriteRedirectScript(wp, "admin/upgrade.asp", "")` ke dalam iframe tersebut, sehingga iframe tersembunyi memuat salinan kedua `upgrade.asp`!
  4. **Tampilan Awal `#upgradeProgress` & Typo Tag `#tr`**:
     Di `upgrade.asp:368`, `<div id="upgradeProgress">` tidak disembunyikan (`style="display:none"` tidak ada) saat halaman pertama dibuka, dan baris `374` memiliki kutip ganda `<!--#tr id="adm.fu.4"" -->`.
- **Spesifikasi Perbaikan**:
  - Di `upgrade.asp:166`, ubah `top.location.href = "admin/upgrade.asp"` menjadi `top.location.href = "/reboot_page.asp"` (atau `"/admin/upgrade.asp"`).
  - Di `webs.c` saat menerima header awal `/goform/upldImage` (`wp->flags & WEBS_POST_REQUEST`), langsung reset `gMTBurnParams.burnState = 0; gMTBurnParams.precentComplete = 0;` agar polling XHR selama proses upload melihat state `0` (sedang mengunggah), bukan `-1` basi.
  - Di `BurnImage()` (`upldform.c:951`), kirim respons HTML ringan kosong `<html><body>OK</body></html>` ke hidden iframe `rebootRedirect`, bukan me-redirect iframe tersebut ke `admin/upgrade.asp`.
  - Sembunyikan `#upgradeProgress` saat `page_load()` dan perbaiki typo `<!--#tr id="adm.fu.4" -->`.

---

#### 3.9 `[F-10]` s/d `[F-16]` [P2-MEDIUM / P3-LOW] Temuan Tambahan WebUI Frontend
- **`[F-10]` [P2-MEDIUM] Form Restore Config Mati di `admin/management.asp:238`**:
  Atribut `action="/goform/uploadConfigFile"` mengarah ke handler yang tidak pernah didaftarkan di `LINUX/main.c` (handler yang benar adalah `/goform/upldForm`).
- **`[F-11]` [P2-MEDIUM] Bug `getHTMLContent(IDName)` & `stringToHex` di `u-media.js:376–419`**:
  `getHTMLContent(IDName)` mengabaikan parameter `IDName` dan memakai variabel global `loopCount` yang tidak dideklarasikan.
- **`[F-12]` [P2-MEDIUM] Infinite XHR Loop (Setiap 500ms) di `waitDHCPInfo()` (`func.js:126–140`)**:
  Kondisi berhenti `if (waitOtherProcess == "0")` dikomentari, sehingga browser menembakkan XHR tanpa henti setiap 0.5 detik.
- **`[F-13]` [P2-MEDIUM] Race Condition Variabel Global `http_request` di `status/sta_link_status.asp:94–98`**:
  Tiga panggilan `makeRequest()` dijalankan berurutan secara sinkron menggunakan satu variabel global `http_request` di `func.js`, sehingga request pertama dan kedua tertimpa.
- **`[F-14]` [P2-MEDIUM] Link & Path Redirect Rusak (404)**:
  - `/normal_ws.css` tidak ada di `rootfs-star/root/mtlk/web/` (diacu oleh `upgrade.asp:12`, `reboot_page.asp:11`, `wait_page.asp:11`).
  - `wait_page.asp:129` me-redirect ke `basic/network.asp` (direktori yang benar adalah `/network/sta_network.asp`).
  - `reboot_page.asp:127` me-redirect ke `management.asp` secara relatif dari `/reboot_page.asp` (seharusnya `/admin/management.asp`).
- **`[F-15]` [P3-LOW] Sisa Pengecekan Bahasa Arab `if (lang == "SA")` di `station/wps_status.asp:633–639`**.
- **`[F-16]` [P3-LOW] Opsi STA-Only `Re-EstablishEthernet` Tidak Disembunyikan saat `apMode()` di `station/wireless_advanced.asp:104`**.

---

### 11.4 ⚙️ Domain 3: Runtime Shell, Tcl, Init & CLI Applets (`rootfs-star/` — `RT-01` s/d `RT-16`)

#### 4.1 `[RT-01]` [P0-CRITICAL] Dropbear SSH Daemon Gagal Start Saat Boot Karena Flag `-B` Tidak Didukung oleh Server Dropbear 0.52
- **Lokasi File**:
  - `rootfs-star/etc/mtlk_init_start.sh` (baris `27`)
  - `rootfs-star/root/mtlk/etc/mtlk_init_daemons.sh` (baris `45`)
  - `apps/dropbear-0.52/svr-runopts.c` (baris `163–250`)
- **Akar Masalah Teknis**:
  Di `mtlk_init_start.sh:27` dan `mtlk_init_daemons.sh:45`, daemon SSH dijalankan dengan perintah:
  ```sh
  /usr/sbin/dropbear -B -p 22 &
  ```
  Namun, jika kita memeriksa parser argumen command-line server Dropbear 0.52 di `apps/dropbear-0.52/svr-runopts.c:163–250`, opsi yang dikenali hanyalah `-b`, `-d`, `-r`, `-F`, `-E`, `-m`, `-w`, `-g`, `-s`, `-a`, `-p`, `-P`, `-i`, `-W`, `-K`, `-I`, `-V`. **Opsi `-B` tidak ada di `svr-runopts.c`** (patch *allow blank password* di `svr-authpasswd.c` sudah aktif secara langsung di kode C tanpa memerlukan flag `-B`)!
  Akibatnya, saat `svr_getopts()` membaca `-B`, ia jatuh ke cabang `default:` → mencetak `Unknown argument -B` ke stderr dan memanggil **`exit(EXIT_FAILURE)`**!
  Inilah alasan mengapa layanan SSH port 22 selalu mati saat router menyala dan hanya bisa nyala jika dijalankan manual tanpa `-B` atau melalui `remote_access.cgi` (yang memanggil `/usr/sbin/dropbear -p 22` tanpa `-B`)!
- **Spesifikasi Perbaikan**:
  1. Hapus flag `-B` di `rootfs-star/etc/mtlk_init_start.sh:27` dan `rootfs-star/root/mtlk/etc/mtlk_init_daemons.sh:45` menjadi `/usr/sbin/dropbear -p 22 &`.
  2. Sebagai pengaman ganda (*defense-in-depth*), tambahkan `case 'B': break;` di `apps/dropbear-0.52/svr-runopts.c` agar meskipun seseorang menjalankan `dropbear -B` secara manual, daemon tidak keluar dengan error.

---

#### 4.2 `[RT-02]` [P0-CRITICAL] `read -n` pada Pipe Non-TTY Diabaikan oleh BusyBox 1.8.1 `ash` (`remote_access.cgi`)
- **Lokasi File**:
  - `rootfs-star/root/mtlk/web/cgi-bin/remote_access.cgi` (baris `13`)
  - `apps/busybox-1.8.1/shell/ash.c` (baris `11579–11605`)
- **Akar Masalah Teknis**:
  Di `remote_access.cgi:13`, body POST dibaca menggunakan:
  ```sh
  read -n "$CONTENT_LENGTH" POST_DATA
  ```
  Pada implementasi `readcmd()` di `apps/busybox-1.8.1/shell/ash.c:11579`, opsi `-n` (jumlah karakter) diimplementasikan dengan memanggil `tcgetattr(0, &tty)` untuk mengubah mode terminal `stdin` menjadi non-canonical (`~ICANON`). Ketika `stdin` adalah **pipe CGI** (bukan TTY), `tcgetattr(0)` gagal dan `ash` **mengabaikan batas `-n`**, lalu terus membaca hingga menemukan karakter newline `\n` atau `EOF`! Karena browser tidak menambahkan `\n` di akhir body `application/x-www-form-urlencoded` dan socket pipe belum ditutup, `read` dapat menggantung hingga CGI timeout.
- **Spesifikasi Perbaikan**:
  Ganti `read -n "$CONTENT_LENGTH" POST_DATA` dengan:
  ```sh
  POST_DATA=$(dd bs=1 count="$CONTENT_LENGTH" 2>/dev/null)
  ```

---

#### 4.3 `[RT-03]` [P0-CRITICAL] Command Injection via `eval`, Infinite Loop 100% CPU (`shift 2`), dan Header `-H` Hilang di `/bin/curl`
- **Lokasi File**:
  - `rootfs-star/bin/curl` (baris `68–105`, `148–166`)
- **Akar Masalah Teknis**:
  1. **Command Injection via `eval`**:
     Di baris `161` dan `163`:
     ```sh
     eval "wget $WGET_ARGS -O \"$OUT_FILE\" \"$FINAL_URL\""
     ```
     Jika `$FINAL_URL` atau `$OUT_FILE` mengandung karakter shell seperti `$(...)`, `` `...` ``, atau `"`, `eval` akan mengeksekusinya sebagai perintah shell!
  2. **Infinite Loop 100% CPU pada `shift 2`**:
     Di dalam parser argumen `while [ $# -gt 0 ]` (baris `68–105`), opsi seperti `-X|--request`, `-H|--header`, `-d|--data`, `-u|--user`, `-o|--output` langsung menjalankan `shift 2` tanpa memeriksa apakah `$# -ge 2`. Dalam BusyBox `ash`, jika `$# == 1` (misal user mengetik `curl -H`), perintah `shift 2` gagal dan **tidak mengubah nilai `$#`**, sehingga `while [ $# -gt 0 ]` berputar tanpa henti dan memakan 100% CPU selamanya!
  3. **Header `-H` Hilang pada Request `GET`**:
     Di baris `148`:
     ```sh
     if [ "$METHOD" = "GET" ] && [ -z "$DATA" ] && [ "$INCLUDE_HEADERS" -eq 0 ] && [ "$FOLLOW_LOC" -eq 0 ]; then
     ```
     Kondisi ini lupa mengecek `[ -z "$CUSTOM_HEADERS" ]`! Akibatnya jika user menjalankan `curl -H "Authorization: Bearer xxx" http://...`, skrip masuk ke jalur cepat `wget` (BusyBox `wget` tidak mendukung custom header) dan membuang seluruh header `-H`!
- **Spesifikasi Perbaikan**:
  - Hapus penggunaan `eval`; panggil `wget` secara langsung dengan ekspansi argumen yang aman (`wget $WGET_ARGS -O "$OUT_FILE" "$FINAL_URL"`).
  - Tambahkan pengecekan `[ $# -lt 2 ] && { echo "curl: option $1 requires an argument" >&2; exit 2; }` sebelum setiap `shift 2`.
  - Tambahkan `&& [ -z "$CUSTOM_HEADERS" ]` pada kondisi fast-path `wget` di baris `148`.

---

#### 4.4 `[RT-04]` [P1-HIGH] `tinytcl` Tidak Memiliki Perintah `after` (Hanya Memiliki `sleep`) → Crash pada Skrip DHCP, WPS & Security
- **Lokasi File**:
  - `rootfs-star/etc/udhcpc/dhcp.tcl` (baris `226`)
  - `rootfs-star/root/mtlk/etc/mtlk_wps_msgloop.tcl` (baris `79`, `132`, `139`, `157`, `162`)
  - `rootfs-star/root/mtlk/web/update_security.tcl` (baris `109`)
  - `apps/tinytcl/tclXgen.c` (baris `418`)
- **Akar Masalah Teknis**:
  Interpreter Tcl pada WAP610N adalah **`tinytcl` (Tcl 6.7 + ekstensi TclX)**. Di dalam source code `apps/tinytcl/`, perintah `after` (yang baru diperkenalkan di Tcl 7.5) **tidak ada sama sekali**! Sebaliknya, `apps/tinytcl/tclXgen.c:418` mendaftarkan perintah `sleep <detik>`:
  ```c
  Tcl_CreateCommand (interp, "sleep", Tcl_SleepCmd, (ClientData)NULL, (void (*)())NULL);
  ```
  Ketika `sleep 1` diubah menjadi `after 1000` di `dhcp.tcl` dan `mtlk_wps_msgloop.tcl` (serta `after 2000` di `update_security.tcl`), `tinytcl` langsung melempar fatal error:
  ```
  Error: invalid command name "after"
  ```
  Selain itu, di `mtlk_wps_msgloop.tcl:79` terdapat bug sintaks Tcl `[llength ::argv]==0` (kurang tanda `$`, sehingga menghitung panjang string literal `"::argv"` yang selalu `1`, bukan panjang list `$::argv`)!
- **Spesifikasi Perbaikan**:
  - Ganti seluruh `after 1000` menjadi `sleep 1` di `dhcp.tcl` dan `mtlk_wps_msgloop.tcl`, serta `after 2000` menjadi `sleep 2` di `update_security.tcl`.
  - Perbaiki baris `79` di `mtlk_wps_msgloop.tcl` menjadi `if {[llength $::argv] == 0}`.

---

#### 4.5 `[RT-05]` [P1-HIGH] `/bin/nettest` & `/bin/flashdump` Menulis File Multi-MB ke `/tmp` (`tmpfs`) → Kernel Panic OOM pada RAM 32 MB
- **Lokasi File**:
  - `rootfs-star/bin/nettest` (baris `21–24`, `42`)
  - `rootfs-star/bin/flashdump` (baris `37`, `59`, `76`)
- **Akar Masalah Teknis**:
  1. Di `/bin/nettest` mode `server` (baris `21–24`):
     ```sh
     start_time=$(cut -d. -f1 /proc/uptime)
     nc -l -p "$PORT" > /tmp/.nettest_sink
     end_time=$(cut -d. -f1 /proc/uptime)
     bytes=$(wc -c < /tmp/.nettest_sink)
     ```
     - Menulis stream jaringan (default 10 MB dari client) ke `/tmp/.nettest_sink` di `tmpfs` padahal sisa RAM bebas router hanya ~5–6 MB → **100% memicu Kernel OOM Panic / Reboot**!
     - `start_time` dicatat *sebelum* `nc -l` menerima koneksi, sehingga waktu tunggu sebelum client jalan ikut dihitung sebagai waktu transfer!
  2. Di `/bin/flashdump`:
     Menjalankan `flashdump full` atau `flashdump kernel` menulis file 4.0 MB / 3.625 MB ke `/tmp/wap610n_full_flash.bin` tanpa memeriksa sisa RAM di `/proc/meminfo`, yang menyisakan < 1.5 MB RAM dan berisiko menjatuhkan kernel jika WebUI atau daemon lain mengalokasikan memori.
- **Spesifikasi Perbaikan**:
  - Di `/bin/nettest`: Alirkan output `nc` langsung ke `wc -c` tanpa menulis ke `/tmp` (`bytes=$(nc -l -p "$PORT" | wc -c)`).
  - Di `/bin/flashdump`: Tambahkan mode streaming ke stdout (`flashdump kernel -`) agar dapat di-pipe langsung melalui SSH/nc (`ssh root@192.168.1.1 "flashdump full -" > full.bin`) tanpa menyentuh `/tmp`, dan tambahkan pengecekan `MemFree + Cached` sebelum mengizinkan penulisan ke `/tmp`.

---

#### 4.6 `[RT-06]` [P1-HIGH] Bug Perhitungan CPU% di `/bin/sysmon`
- **Lokasi File**:
  - `rootfs-star/bin/sysmon` (baris `19–40`, `80–86`)
- **Akar Masalah Teknis**:
  1. Di baris `84` (pre-seed loop `-l`), `PREV_TOTAL=$((PREV_U + PREV_S + PREV_I + PREV_W))` **lupa menjumlahkan `PREV_N` (`$3` = nice ticks)**, sedangkan di dalam loop (baris `25`) `TOTAL=$((U + N + S + I + W))` menjumlahkan `N`. Akibatnya selisih `D_TOTAL` pada iterasi pertama selalu membengkak sebesar akumulasi `nice` ticks sejak boot!
  2. Di baris `28`, `D_U=$((U - PREV_U))` **tidak menambahkan selisih `nice` (`N - PREV_N`)**. Padahal hampir seluruh daemon di WAP610N (`hostapd`, `upnpd`, `WPS_PBC.sh`, `mtlk_pbc_reboot.sh`) dijalankan menggunakan `nice -n 18`! Akibatnya beban CPU dari daemon-daemon tersebut tidak terhitung di `usr%` dan hanya muncul secara samar di `total%`!
  3. Saat user menjalankan `sysmon` sekali jalan (tanpa `-l`), `PREV_TOTAL` bernilai `0` sehingga blok `if [ "$PREV_TOTAL" -gt 0 ]` dilewati dan **baris `CPU:` tidak ditampilkan sama sekali**!
  4. Argumen `-d <sec>` dan `-n <count>` yang umum dipakai (dan tertulis di dokumentasi) belum didukung (hanya mengenal `-l [sec]` dan `-c`).
- **Spesifikasi Perbaikan**:
  - Ambil 2 sampel `/proc/stat` berjarak 1 detik pada mode sekali jalan agar baris `CPU:` selalu tampil akurat.
  - Sertakan `N` (`nice`), `IRQ` (`$7`), dan `SOFTIRQ` (`$8`) secara konsisten di `PREV_TOTAL` dan `TOTAL`, serta dukung flag `-d` dan `-n` di samping `-l` dan `-c`.

---

#### 4.7 `[RT-07]` [P1-HIGH] `mtlk_init_start.sh:34` Menjalankan `WPS_PBC.sh` Tanpa Argumen dan Mencuri Lock dari `mtlk_init_wls_apps.sh:112`
- **Lokasi File**:
  - `rootfs-star/etc/mtlk_init_start.sh` (baris `34`)
  - `rootfs-star/root/mtlk/etc/mtlk_init_wls_apps.sh` (baris `112`)
  - `rootfs-star/root/mtlk/etc/WPS_PBC.sh` (baris `4–25`)
- **Akar Masalah Teknis**:
  `WPS_PBC.sh` mengharapkan 2 argumen posisi: `WPS_ON=$1` dan `NETWORK_TYPE=$2` (`2` untuk AP, `0` untuk STA), lalu membuat direktori lock `/tmp/WPS_PBC_monitor.lock`.
  Namun, `mtlk_init_start.sh:34` menjalankan `/root/mtlk/etc/WPS_PBC.sh &` **tanpa argumen** sebelum memanggil `mtlk_init.sh`! Instance tanpa argumen ini merebut `/tmp/WPS_PBC_monitor.lock` dengan `$WPS_ON=""` dan `$NETWORK_TYPE=""`. Ketika `mtlk_init_wls_apps.sh:112` kemudian memanggil `./WPS_PBC.sh $WPS_ON $NETWORK_TYPE &` dengan argumen yang benar, proses kedua gagal membuat `/tmp/WPS_PBC_monitor.lock` dan langsung keluar (`exit 0`)! Akibatnya tombol fisik WPS (GPIO 13) tidak pernah berfungsi karena daemon yang berjalan memiliki `$WPS_ON` kosong!
- **Spesifikasi Perbaikan**:
  Hapus pemanggilan prematur `/root/mtlk/etc/WPS_PBC.sh &` dari `rootfs-star/etc/mtlk_init_start.sh:34` dan biarkan `mtlk_init_wls_apps.sh:112` yang menjalankannya dengan argumen lengkap.

---

#### 4.8 `[RT-08]` [P1-HIGH] `mtlk_init_wps.sh` Menghapus `WEP_TX_KEY_IDX` yang Baru Ditulis & Mencetak Baris Angka `1234567890...` ke Konfigurasi WPS
- **Lokasi File**:
  - `rootfs-star/root/mtlk/etc/mtlk_init_wps.sh` (baris `176`, `250`, `380–397`)
- **Akar Masalah Teknis**:
  1. Pada baris `380–384`, skrip menulis `WEP_TX_KEY_IDX=...` ke `${WSC_CONF_FILE}`, lalu pada baris `397` memanggil fungsi `write_wps_config_file` yang di baris pertama (`176`) langsung menjalankan `rm -rf ${WSC_CONF_FILE}`!
  2. Pada baris `250`, di dalam blok heredoc `cat << END_OF_FILE >> ${WSC_CONF_FILE}`, terdapat baris penggaris panjang karakter yang lupa diberi tanda komentar `#`:
     ```
     1234567890123456789012345678901234567890123456789012345678901234
     ```
     Baris ini ikut tertulis ke dalam file konfigurasi daemon WPS!
- **Spesifikasi Perbaikan**:
  Pindahkan pemanggilan `write_wps_config_file` ke **sebelum** penulisan `WEP_TX_KEY_IDX`, dan beri tanda `#` pada baris `250`.

---

#### 4.9 `[RT-09]` s/d `[RT-16]` [P2-MEDIUM / P3-LOW] Temuan Tambahan Runtime Shell & Tcl
- **`[RT-09]` [P2-MEDIUM] `chmod 777 /tmp/*` di `mtlk_init_wls_apps.sh:79` Mengikuti Symlink**:
  Mengubah permission target symlink `/tmp/jffs2` (`/mnt/jffs2`), `/tmp/mtlk.ko`, dan `/tmp/var`.
- **`[RT-10]` [P2-MEDIUM] Alias `/etc/profile`, `/bin/flashdump`, dan `/bin/help` Memakai Flag yang Dinonaktifkan di BusyBox 1.8.1**:
  Karena `CONFIG_FEATURE_HUMAN_READABLE` dan `CONFIG_DESKTOP` tidak aktif di BusyBox `.config`:
  - `ll` (`ls -laFh`), `df` (`df -h`), `ps1`, `ps2`, `af` di `rootfs-star/etc/profile:16–24` gagal dengan `invalid option`.
  - `ls -lh` di `rootfs-star/bin/flashdump:80` gagal.
  - `ps aux` di `rootfs-star/bin/help:80` gagal, dan `help` / `flashdump` menyarankan perintah `scp admin@192.168.1.1:...` padahal binary `scp` dan user `admin` tidak ada di sistem (user SSH adalah `root`).
- **`[RT-11]` [P2-MEDIUM] `mt_current_stat.tcl:116–125` Memanggil `vmstat` dan `regsub` yang Tidak Ada**.
- **`[RT-12]` [P2-MEDIUM] `mtlk_wps_cmd.tcl:97–115` Menggunakan `catch [exec ...]` Bukannya `catch {exec ...}`**:
  Dalam Tcl, kurung siku `[...]` dievaluasi *sebelum* dipassing ke `catch`, sehingga jika `exec` gagal, skrip tetap crash sebelum `catch` sempat menangkapnya!
- **`[RT-13]` [P2-MEDIUM] `/bin/json` Gagal pada JSON Multi-Baris & Objek Bersarang**.
- **`[RT-14]` [P2-MEDIUM] `mtlk_init_daemons.sh:24` Menjalankan `upnpd` Sebelum Interface `br0` Dibuat di `mtlk_init_ip.sh`**.
- **`[RT-15]` [P3-LOW] Baris CRLF (`\r\n`) pada `web_config.tcl` & `combined_ver_path.tcl`, serta Permission `0644` pada `mtlk_ledman.sh`, `mtpriv`, dan `wpa_cli`**.
- **`[RT-16]` [P3-LOW] Divergensi Skrip antara `apps/jffs2-etc/networking/VB/` dan `rootfs-star/root/mtlk/etc/`**.

---

### 11.5 🏗️ Domain 4: Build Pipeline, Makefiles & RootFS Packaging (`B-01` s/d `B-18`)

#### 5.1 `[B-01]` [P0-CRITICAL] Syntax Error Kutip (`"'`) & Variabel Host Kosong di `docker/compile.sh`
- **Lokasi File**:
  - `docker/compile.sh` (baris `51`, `86`, `97–99`)
- **Akar Masalah Teknis**:
  Di `docker/compile.sh:51–99`:
  ```bash
  docker run --rm --privileged \
      -v "$(pwd):/home/builder/wap610n" \
      -w /home/builder/wap610n \
      "$IMAGE_NAME" \
      /bin/bash -c '
          ...
          cp kernel/linux-2.6.16-star/arch/arm/boot/bootpImage "output/$TARGET_NAME"
          ...
          chown -R $CURRENT_UID:$CURRENT_GID output/ rootfs/ || true
      "'
  ```
  1. **Unmatched Quote Syntax Error**: Baris `51` membuka argumen `/bin/bash -c` dengan tanda kutip tunggal `'`, tetapi baris `99` menutupnya dengan `"'` (kutip ganda diikuti kutip tunggal). Saat dijalankan, `bash` di dalam container menerima karakter `"` tanpa pasangan di baris terakhir → **`bash: -c: line 48: unexpected EOF while looking for matching '"'` (Exit Code 2)**!
  2. **Variabel `$TARGET_NAME`, `$CURRENT_UID`, `$CURRENT_GID` Kosong di Dalam Container**: Karena blok `/bin/bash -c '...'` diapit oleh tanda kutip tunggal `'`, variabel `$TARGET_NAME`, `$CURRENT_UID`, dan `$CURRENT_GID` **tidak diekspansi oleh shell host**, dan `docker run` juga **tidak meneruskan `-e TARGET_NAME=...`** ke dalam container! Akibatnya di dalam container `$TARGET_NAME` bernilai string kosong `""`, sehingga `cp ... "output/$TARGET_NAME"` menyalin ke `"output/"` dengan nama `bootpImage`, lalu baris berikutnya `ls -lh "output/$TARGET_NAME"` menjadi `ls -lh "output/"`!
- **Spesifikasi Perbaikan**:
  Teruskan `-e TARGET_NAME="$TARGET_NAME" -e CURRENT_UID="$CURRENT_UID" -e CURRENT_GID="$CURRENT_GID"` pada perintah `docker run`, dan tutup baris `99` dengan kutip tunggal `'` saja (tanpa `"`).

---

#### 5.2 `[B-02]` [P0-CRITICAL] `devscripts/net-ver-mkrootfs.sh:130` Menimpa Binary `simpdiff` Hasil Compile Baru dengan Binary Lama 2011
- **Lokasi File**:
  - `devscripts/net-ver-mkrootfs.sh` (baris `129–131`)
  - `make_nv.sh` (baris `405`)
- **Akar Masalah Teknis**:
  Saat proses build berjalan:
  1. `make_nv.sh` mengompilasi kode sumber C `apps/simpdiff/simpdiff.c` (yang telah diperbaiki) dan menginstal binary barunya ke `rootfs/bin/simpdiff`.
  2. Kemudian pada tahap akhir pembuatan image ramdisk, `devscripts/net-ver-mkrootfs.sh:129–131` mengeksekusi:
     ```bash
     if [ -e $ROOTFS_STAR/bin/simpdiff ]; then
         cp -fv $ROOTFS_STAR/bin/simpdiff $ROOTFS_DIR/bin/simpdiff
     fi
     ```
     Perintah ini **menimpa kembali** `rootfs/bin/simpdiff` yang baru saja dikompilasi dengan binary prebuilt tahun 2011 dari `rootfs-star/bin/simpdiff`!
- **Spesifikasi Perbaikan**:
  Ubah kondisi di `devscripts/net-ver-mkrootfs.sh:129` menjadi `if [ ! -e $ROOTFS_DIR/bin/simpdiff ] && [ -e $ROOTFS_STAR/bin/simpdiff ]; then` agar hanya menyalin dari `rootfs-star` jika belum terinstal oleh `make_nv.sh`, dan perbarui juga binary di `rootfs-star/bin/simpdiff`.

---

#### 5.3 `[B-03]` [P0-CRITICAL] `kernel/linux-2.6.16-star/mkbootp.sh` Selalu Exit `0` Saat `/tftpboot` Tidak Ada & Menyimpan Binary 64-Bit `mkcsum`
- **Lokasi File**:
  - `kernel/linux-2.6.16-star/mkbootp.sh` (baris `1–38`)
  - `kernel/linux-2.6.16-star/arch/arm/boot/bootp/mkcsum`
- **Akar Masalah Teknis**:
  1. Di `mkbootp.sh`:
     - Tidak ada `set -e`.
     - Baris `23` dan `25` menelan kegagalan kompilasi dan eksekusi checksum tool dengan `gcc mkcsum.c -o mkcsum 2>/dev/null || true` dan `./mkcsum ... || true`. Jika `mkcsum` gagal menyuntikkan checksum U-Boot ke `bootpImage`, U-Boot atau `ValidateImage()` di WebUI akan menolak image tersebut!
     - Baris terakhir (`32–38`) adalah:
       ```sh
       if [ -e /tftpboot ]; then
           cp arch/arm/boot/bootpImage /tftpboot/bootpImage.$ USERNAME -v
       fi
       ```
       Di dalam container Docker maupun GitHub Actions, direktori `/tftpboot` tidak ada. Dalam shell POSIX, blok `if [ false ]; then ... fi` menghasilkan exit status **`0`**! Akibatnya `mkbootp.sh` **selalu mengembalikan exit code `0`** meskipun `make bootpImage` di baris `31` gagal total!
  2. File binary `kernel/linux-2.6.16-star/arch/arm/boot/bootp/mkcsum` yang di-track di Git berformat **ELF 64-bit x86-64**, yang tidak dapat dijalankan di dalam container Docker Debian Jessie **32-bit (i386)** apabila `gcc mkcsum.c` gagal.
- **Spesifikasi Perbaikan**:
  Tambahkan `set -e` di awal `mkbootp.sh`, hapus `|| true` pada kompilasi dan eksekusi `mkcsum`, dan pastikan kegagalan `make bootpImage` langsung menghentikan skrip dengan exit code non-zero.

---

#### 5.4 `[B-04]` [P1-HIGH] `dropbear-0.52` Dinonaktifkan di `boards/.../appsconfig/.config` & `apps/dropbear-0.52/Makefile` Memiliki Hardcoded Path `/home/satszz/wap610n`
- **Lokasi File**:
  - `boards/star-6.7.2-mtlk-U-Media-vela/appsconfig/.config` (baris `25`)
  - `apps/dropbear-0.52/Makefile` (baris `66–74`)
  - `apps/dropbear-0.52/Makefile.MTLK` (baris `15`)
- **Akar Masalah Teknis**:
  1. Di `boards/star-6.7.2-mtlk-U-Media-vela/appsconfig/.config:25`, baris `#dropbear-0.52 \` masih diberi tanda pagar `#`, sehingga sistem build (`make_nv.sh`) **sama sekali tidak mengompilasi `apps/dropbear-0.52/`**!
  2. File `apps/dropbear-0.52/Makefile` yang di-track di Git berisi path absolut lokal:
     ```makefile
     CC=/home/satszz/wap610n/tools/arm-uclibc-3.4.6/bin/arm-linux-gcc
     ```
     Ini melanggar aturan *Path Portability Invariant* (`AGENTS.md` §1.1) dan akan langsung gagal di dalam Docker (`/home/builder/wap610n` atau `/workspaces/wap610n`) karena `apps/dropbear-0.52/Makefile.MTLK:15` hanya menjalankan `./configure` jika `Makefile` belum ada (`if [ ! -f Makefile ]`)!
- **Spesifikasi Perbaikan**:
  - Ubah `apps/dropbear-0.52/Makefile` agar menggunakan `$(CROSS_COMPILE)gcc` / `$(CROSS_COMPILE)strip` relatif atau hapus `Makefile` yang ter-track agar `./configure` di `Makefile.MTLK` men-generate path yang sesuai saat `reconf`, dan aktifkan `dropbear-0.52` di `appsconfig/.config`.

---

#### 5.5 `[B-05]` [P1-HIGH] `chmod 777 -R` pada `/root/mtlk/` & Kepemilikan UID Host (`1000`/`1001`) Tidak Di-reset ke `0:0` di `devscripts/net-ver-mkrootfs.sh`
- **Lokasi File**:
  - `devscripts/net-ver-mkrootfs.sh` (baris `63`, `143–156`, `253–257`)
- **Akar Masalah Teknis**:
  1. Baris `63` menjalankan `chmod 777 -R $ROOTFS_DIR/root/mtlk/`, membuat seluruh file konfigurasi sensitif (`sys.conf`), modul kernel (`mtlk.ko`), dan binary `webs` menjadi *world-writable* (`0777`).
  2. Banyak file di `rootfs-star/etc/` (`passwd`, `inittab`, `fstab`, `profile`) dan `rootfs-star/lib/*.so` memiliki mode `0666` / `0777` di Git.
  3. Di baris `253–257`, saat menyalin seluruh rootfs ke image ext2 ramdisk (`loop_dir`), skrip hanya menjalankan `sudo chown -R 0:0 loop_dir/dev`! Seluruh direktori lainnya (`/etc`, `/bin`, `/sbin`, `/usr`, `/root`) di dalam firmware tetap dimiliki oleh UID user host/CI (`1000` atau `1001`), bukan `root:root` (`0:0`)!
- **Spesifikasi Perbaikan**:
  - Ubah `sudo chown -R 0:0 loop_dir/dev` menjadi `sudo chown -R 0:0 loop_dir/` agar seluruh file di dalam ext2 ramdisk dimiliki oleh `0:0` (`root:root`).
  - Perbaiki permission `/etc/passwd`, `/etc/shadow` (`0600`), `/etc/inittab`, `/etc/fstab` menjadi `0644`.

---

#### 5.6 `[B-06]` s/d `[B-09]` [P1-HIGH] Temuan Build Pipeline Lainnya
- **`[B-06]` [P1-HIGH] Unchecked `cp -a` ke Loop-Mounted 12 MB Ramdisk & Hilangnya `devscripts/device_table.txt`** (`devscripts/net-ver-mkrootfs.sh:247–267`):
  Jika ukuran `rootfs/` melebihi 12 MB atau `mount -o loop` gagal, `cp -a` gagal sebagian tanpa menghentikan build.
- **`[B-07]` [P1-HIGH] Kegagalan Sub-Make Ditelan di `make_nv.sh:221–241, 276–283` & `apps/scripts/Makefile.MTLK:18–33`**:
  Loop `config:` di `apps/scripts/Makefile.MTLK:19` tidak memiliki `|| exit 1`, dan `reconf` / `make_modules` di `make_nv.sh` tidak mengecek kode keluar `make`.
- **`[B-08]` [P1-HIGH] `apps/expat/Makefile.MTLK:23–36` Menginstal Binary Lama `apps/expat/libexpat.so` (2011) Bukannya `.libs/libexpat.so.1.5.2` Hasil Compile**.
- **`[B-09]` [P1-HIGH] Simbol Integer Kconfig Bernilai `=y` di `boards/.../busybox/.config:299, 500`**:
  `CONFIG_FEATURE_VI_MAX_LEN=y` dan `CONFIG_FEATURE_LESS_MAXLINES=y` (seharusnya angka integer seperti `2048` / `9999999` atau `# ... is not set`).

---

#### 5.7 `[B-10]` s/d `[B-18]` [P2-MEDIUM / P3-LOW] Temuan Tambahan Build & Packaging
- **`[B-10]` [P2-MEDIUM] Step 1 Persiapan `rootfs/` di `compile.sh` / `compile.yml` Dihapus Ulang oleh Step 2 (`reconf`) & Step 3 (`install_`)**.
- **`[B-11]` [P2-MEDIUM] Gate Ukuran `3,801,088` Byte Belum Ada di `make_nv.sh` / `mkbootp.sh`** (hanya ada di `docker/verify-firmware.sh`, dan beberapa check di `verify-firmware.sh:63, 87–91, 138–142` hanya mencetak warning tanpa `ERRORS++`).
- **`[B-12]` [P2-MEDIUM] Hardcoded Path `/home/satszz` & `/home/ricky.cao`** di file tracked `apps/busybox-1.8.1/include/bbconfigopts.h` dan `apps/openssl-0.9.8a/Makefile`.
- **`[B-13]` [P2-MEDIUM] Duplikasi `make_nv.sh` vs `devscripts/make_nv.sh`** & `make clean` menghapus `rootfs/*` serta `kernel/.config`.
- **`[B-14]` [P2-MEDIUM] `apps/WebServer/LINUX/Makefile` Me-link `main.o` Dua Kali**.
- **`[B-15]` [P2-MEDIUM] Device Node `/dev/ptmx` Mode `0644` (Seharusnya `0666`), `/dev/rdflt0` Minor `13` (Seharusnya `3`), & Ekstraksi Ganda `rootfs-star.tgz` di `net-ver-mkrootfs.sh`**.
- **`[B-16]` [P3-LOW] `devscripts/validate-build-prerequisites.sh:6, 34` Langsung Keluar pada Error Pertama** karena kombinasi `set -e` dan `((ERRORS++))` (ketika `ERRORS=0`, ekspresi aritmatika `((ERRORS++))` bernilai `0` yang menghasilkan exit code `1` di bash!).
- **`[B-17]` [P3-LOW] File Statis `libuuid.a` (27 KB) Ikut Terinstal ke `/lib/` Runtime & `vconfig`/`gpio.ko`/`8021q.ko` Belum Di-strip**.
- **`[B-18]` [P3-LOW] Build Mengotori File Tracked `config/.config_WAP610N` & `config/WAP610N_VERSION`**.

---

## 🔩 6. Domain 5: Kernel 2.6.16-star, U-Boot 1.1.4, Drivers & Native C Apps

#### 6.1 `[K-01]` [P0-CRITICAL] `apps/set_env_param/set_env_param.c` Tidak Pernah Bisa Mengupdate atau Menghapus Parameter U-Boot NVRAM yang Sudah Ada (`strcmp("name=val", "name")`)
- **Lokasi File**:
  - `apps/set_env_param/set_env_param.c` (baris `99–145`, `149–191`)
- **Akar Masalah Teknis**:
  Utilitas `/bin/set_env_param` digunakan oleh skrip jaringan (`mtlk_init_ip.sh`, `dhcp.tcl`, dll.) untuk membaca/mengubah variabel U-Boot NVRAM di `/dev/mtdblock3` (seperti `ipaddr`, `netmask`, `gatewayip`).
  Mari kita lihat loop parser environment U-Boot di `set_env_param.c:99–113`:
  ```c
  /* apps/set_env_param/set_env_param.c:99-113 */
  while((*curr_str != 0) && (curr_byte_idx < ENV_DATASIZE)) {
      if(*curr_str++ == '=') {
          //*(curr_str-1) = 0;   /* <-- DIKOMENTARI OLEH DEVELOPER LAMA! */
          params_arr[curr_str_idx].val_ptr = curr_str;
          curr_str += strlen(params_arr[curr_str_idx].val_ptr) + 1;
          curr_byte_idx += strlen(params_arr[curr_str_idx].val_ptr) + 1;

          /* find the parameter asked for */
          if(!strcmp(params_arr[curr_str_idx].name_ptr, param_name)) {
              param_found = 1;
  ```
  - Karena `//*(curr_str-1) = 0;` dikomentari, karakter `'='` tidak diganti dengan `'\0'`.
  - Akibatnya `params_arr[curr_str_idx].name_ptr` menunjuk ke string penuh **`"ipaddr=192.168.1.1"`**, sedangkan `param_name` adalah **`"ipaddr"`**!
  - Pemanggilan `strcmp("ipaddr=192.168.1.1", "ipaddr")` **TIDAK PERNAH BERNILAI `0`**!
  - Dampaknya sangat fatal:
    1. `param_found` **selalu bernilai `0`**!
    2. Setiap kali `set_env_param ipaddr 192.168.1.2` dipanggil, entri lama `ipaddr=192.168.1.1` tidak pernah diupdate; melainkan entri duplikat baru `ipaddr=192.168.1.2` terus ditambahkan di bagian akhir `/dev/mtdblock3`! Sementara U-Boot selalu membaca kemunculan *pertama* di NVRAM!
    3. Fitur hapus parameter (`set_env_param nama_param` dengan `argc == 2`) mati total karena `param_found` selalu `0`!
    4. Di baris `150`, `fclose(f)` menutup file `/dev/mtdblock3` yang otomatis **melepaskan `flock(fd, LOCK_EX)`** sebelum file dibuka ulang untuk ditulis di baris `157` (TOCTOU race condition).
- **Spesifikasi Perbaikan**:
  - Gunakan perbandingan panjang nama:
    ```c
    size_t name_len = (size_t)(params_arr[curr_str_idx].val_ptr - 1 - params_arr[curr_str_idx].name_ptr);
    if (strlen(param_name) == name_len &&
        strncmp(params_arr[curr_str_idx].name_ptr, param_name, name_len) == 0) {
        param_found = 1;
        ...
    ```
  - Pastikan terminator ganda `\0\0` ditulis di akhir tabel environment, dan pertahankan lock `flock` pada satu descriptor `r+b` tanpa menutupnya di tengah jalan.

---

#### 6.2 `[K-02]` [P1-HIGH] Bug `CONFIG_SAFE_RECOVERY_COMMAND` di U-Boot (`star_orion_umedia.h:96`): Tanpa `erase` Flash & Bug Precedence `||` / `&&` di Hush Shell
- **Lokasi File**:
  - `u-boot-1.1.4/include/configs/star_orion_umedia.h` (baris `96`)
- **Akar Masalah Teknis**:
  ```c
  #define CONFIG_SAFE_RECOVERY_COMMAND \
      "cksum $linux_offset || echo Checksum failure! && setenv serverip 2.2.2.2 && setenv ipaddr 2.2.2.1 && tftp 0x100000 $ rescue_image && cp.b 0x100000 $linux_offset 0x39ffff && reset"
  ```
  1. **Tanpa `erase` Flash Sebelum `cp.b`**: `cp.b 0x100000 $linux_offset 0x39ffff` langsung menyalin ke alamat NOR Flash `0x10040000` tanpa menjalankan `erase $linux_offset +0x3a0000` terlebih dahulu!
  2. **Precedence `||` dan `&&` di Hush Shell**: Dalam hush shell U-Boot, `cmd1 || cmd2 && cmd3` dievaluasi dari kiri ke kanan dengan prioritas setara: jika `cksum $linux_offset` **sukses (`0`)**, `cmd2` (`echo`) dilewati, lalu seluruh ekspresi bernilai `0` (sukses), sehingga **`&& setenv serverip 2.2.2.2 && tftp ...` justru dijalankan ketika checksum valid**!
  3. **Spasi pada `$ rescue_image` & Ukuran `0x39ffff`**: Terdapat spasi salah ketik setelah `$` pada `$ rescue_image`, dan `0x39ffff` kurang 1 byte dari `0x3a0000`.
- **Spesifikasi Perbaikan**:
  Gunakan blok `if ! cksum $linux_offset; then echo Checksum failure!; setenv serverip 2.2.2.2; setenv ipaddr 2.2.2.1; if tftp 0x100000 $rescue_image; then erase $linux_offset +0x3a0000; cp.b 0x100000 $linux_offset $filesize; reset; fi; fi`.

---

#### 6.3 `[K-03]` [P1-HIGH] Kernel Heap Disclosure (*Etherleak*) pada Paket Ethernet Pendek (`< 60` Byte) di Driver `star_gsw.c`
- **Lokasi File**:
  - `kernel/linux-2.6.16-star/drivers/net/str9100/star_gsw.c` (baris `2874–2880`, `2972–2978`)
- **Akar Masalah Teknis**:
  Di `star_gsw_send_packet()`:
  ```c
  /* kernel/linux-2.6.16-star/drivers/net/str9100/star_gsw.c:2874-2880 */
  if (skb->len < MIN_PACKET_LEN) {
      if (skb_tailroom(skb) >= (MIN_PACKET_LEN - skb->len)) {
          memset(skb->data + skb->len, 0, MIN_PACKET_LEN - skb->len);
      }
      txdesc_ptr->length = MIN_PACKET_LEN;
  } else {
      txdesc_ptr->length = skb->len;
  }
  ```
  Jika sebuah `sk_buff` berukuran `< 60` byte (misalnya paket ARP atau TCP ACK pendek) tidak memiliki `skb_tailroom` yang cukup, `memset` dilewati (untuk menghindari buffer overflow), **tetapi `txdesc_ptr->length = MIN_PACKET_LEN` (60 byte) tetap diset**! Akibatnya hardware DMA Ethernet STR8132 membaca melewati `skb->end` dan memancarkan hingga 59 byte isi memori kernel (`skb_shared_info` atau objek heap di sebelahnya) ke jaringan Ethernet (**CVE-class Etherleak vulnerability**)!
- **Spesifikasi Perbaikan**:
  Gunakan fungsi standar kernel Linux `skb_padto(skb, MIN_PACKET_LEN)` sebelum memetakan `skb->data` ke DMA:
  ```c
  if (skb->len < MIN_PACKET_LEN) {
      if (skb_padto(skb, MIN_PACKET_LEN)) {
          spin_unlock_irqrestore(&priv->lock, flags);
          return NETDEV_TX_OK;
      }
      txdesc_ptr->length = MIN_PACKET_LEN;
  }
  ```

---

#### 6.4 `[K-04]` [P1-HIGH] Out-of-Bounds Kernel Array Read (`pulse_pattern < 0`) & Timer Use-After-Free di `apps/gpio_driver/gpioDevice.c`
- **Lokasi File**:
  - `apps/gpio_driver/gpioDevice.c` (baris `363–421`, `449–476`, `489–563`)
- **Akar Masalah Teknis**:
  1. Di `gpio_write()` (baris `471`):
     ```c
     pulse_pattern = gpioval - 2;
     if (pulse_pattern >= NUM_PATTERNS) { ... }
     ```
     Variabel `pulse_pattern` bertipe signed `int`. Jika `gpioval` bernilai negatif (misal input string angka yang mengalami signed 32-bit overflow di `string_to_int()`), `pulse_pattern` bernilai negatif, lolos dari pengecekan `pulse_pattern >= NUM_PATTERNS`, dan diakses secara out-of-bounds oleh fungsi timer kernel `led_pulse()` (`ledPatterns[pulse_pattern][...]`) di interrupt context!
  2. Di `gpio_cleanup_module()`, kode memanggil `del_timer(&led_timer)` (bukan `del_timer_sync()`) dan memanggilnya sebelum `unregister_chrdev()`.
- **Spesifikasi Perbaikan**:
  Tambahkan pengecekan `if (pulse_pattern < 0 || pulse_pattern >= NUM_PATTERNS)` di `gpio_write()` dan `led_pulse()`, serta gunakan `del_timer_sync()` di `gpio_cleanup_module()`.

---

#### 6.5 `[K-05]` [P1-HIGH] Bug Macro `GEN_GPIO_WRITE_SCRIPT`, Race Register GPIO Tanpa Spinlock, dan Boot Timer GPIO 2 di `str9100_gpio.c`
- **Lokasi File**:
  - `kernel/linux-2.6.16-star/drivers/char/str9100_gpio.c` (baris `138–171`, `193–202`, `391–420`, `521–599`)
- **Akar Masalah Teknis**:
  1. Macro `GEN_GPIO_WRITE_SCRIPT` menginisialisasi `u32 temp = 0;` tanpa membaca isi register terlebih dahulu, sehingga menulis 1 bit akan menghapus (`0`) 31 bit GPIO lainnya!
  2. `str9100_gpio_out_bit()` melakukan read-modify-write dari `GPIO_DATA_IN_REG` tanpa spinlock, padahal SoC STR8132 menyediakan register atomik hardware `GPIO_DATA_SET_REG` (`offset + 0x10`) dan `GPIO_DATA_CLEAR_REG` (`offset + 0x14`)!
  3. `str9100_gpio_irq_handler()` membaca `intrrawstate` (status mentah semua pin) bukannya `intrmaskedstatus`, dan membersihkan interrupt *setelah* handler selesai.

---

#### 6.6 `[K-06]` s/d `[K-10]` [P2-MEDIUM / P3-LOW] Temuan Native C Apps & Dokumentasi
- **`[K-06]` [P2-MEDIUM] `ping` Tanpa `-c` (Hang Selamanya) & Unbounded `sprintf` di `apps/force_ethpc_renew_ip/force_ethpc_renew_ip.c:47–68, 134–174`**:
  `detect_layer3_link_with_ping()` menjalankan `/bin/ping %s` tanpa `-c 1 -w 2`, yang pada BusyBox `ping` akan terus melakukan ping selamanya!
- **`[K-07]` [P2-MEDIUM] Spawn `popen("cat /tmp/wps_current_status")` & `system("rm ...")` Setiap Detik di `apps/hnap_wps_status_monitor/hnap_wps_status_monitor.c:150–335`**:
  Daemon ini melakukan `fork()` + `exec("/bin/sh -c cat ...")` setiap 1 detik (~120 proses baru per menit) hanya untuk membaca 1 baris dari `/tmp/wps_current_status` dan menghapusnya! Menggantinya dengan `fopen()` + `fgets()` + `unlink()` murni di C akan menghilangkan ribuan fork/jam dan menurunkan suhu CPU secara signifikan.
- **`[K-08]` [P2-MEDIUM] Pembacaan File 1-Byte per Syscall (`read(fd, buf, 1)`) di `apps/simpdiff/simpdiff.c:62–85`**:
  Loop `read_line()` memanggil syscall kernel `read(fd, buf, 1)` untuk setiap karakter (~40.000 syscall per perbandingan file konfigurasi), dan `main()` mengembalikan `-1` (`255`). Menggunakan `fopen()` + `fgets()` buffered C89 akan memangkas jumlah syscall hingga 99%.
- **`[K-09]` [P2-MEDIUM] Override Offset Partisi `mtd4` oleh Parameter U-Boot `config_offset` di `kernel/linux-2.6.16-star/drivers/mtd/maps/str9100.c:215–238`**.
- **`[K-10]` [P3-LOW] Ketidaksesuaian Dokumentasi di `AGENTS.md`**:
  - Tertulis hash `/etc/shadow` adalah `$1$$0U5a6tQh5qE7f7vFjX9P0/` (aktual di `rootfs-star/etc/shadow` adalah `$1$x$zwNyxSLbfNOhQ2tYG39J10`).
  - Tertulis `::respawn:/usr/sbin/dropbear -p 22` ada di `/etc/inittab` (aktualnya dijalankan dari `mtlk_init_start.sh` & `mtlk_init_daemons.sh`).
  - Tertulis `echo green > /dev/led0` di tabel §2 (driver `gpioDevice.c` menerima input angka `echo 1 > /dev/led0`).
  - Contoh `sysmon -d 2` / `sysmon -n 1` (aktual flag saat ini adalah `-l` dan `-c`).

---

### 11.7 ✅ Checklist Eksekusi Perbaikan Terstruktur & Hasil Audit QC Fase 5

Gunakan daftar periksa di bawah ini saat mengeksekusi perbaikan agar tidak ada regresi:

- [x] **Batch 1 — P0 Critical Backend & WebUI (`W-01`..`W-06`, `F-01`..`F-03`)**:
  - [x] `apps/WebServer/webs.h`, `webs.c`, `upldform.c`: Tambah `WEBS_SYS_MALLOC_POST` untuk memisahkan `free()` vs `bfree()`.
  - [x] `apps/WebServer/upldform.c` & `hnap_config_metalink.c`: Reset `locWrite`/`numLeft` di dalam retry loop `MT_BurnAndVerifyImage`, tutup `fp` pada setiap jalur keluar, pasang batas `0x3A0000` di HNAP, dan ganti `"ab"` → `"r+b"`.
  - [x] `apps/WebServer/management.c`: Ganti `vfork()` → `fork()` dan hapus `killchildweb.sh`.
  - [x] `apps/WebServer/device_discover.c` & `asp.c`: Guard `bodyLen > 0`, batas baca `ePtr`, dan guard `NULL` + panjang buffer di `startMultiLangParser`.
  - [x] `rootfs-star/root/mtlk/web/u-media.js` & `network/sta_network.asp`: Hapus tag `<!--#tr-->` yang merusak sintaks JS.
  - [x] `rootfs-star/root/mtlk/web/station/wireless_basic.asp`, `wireless_advanced.asp`, `wmm.asp`, `site_survey.asp`: Perbaiki `selectMode(2)`, `checkWPAPSK`, dan guard `getElementById("wpsMode")`.
- [x] **Batch 2 — P0/P1 Runtime, SSH, Tcl & CLI Applets (`RT-01`..`RT-08`)**:
  - [x] `mtlk_init_start.sh`, `mtlk_init_daemons.sh`, `svr-runopts.c`: Hapus `-B` pada `dropbear` dan tambahkan no-op `'B'` di `svr-runopts.c`.
  - [x] `remote_access.cgi` & `cgi.c`: Perbaiki pembacaan body POST (`dd`), autentikasi, dan hapus duplikasi `HTTP/1.0 200 OK`.
  - [x] `/bin/curl`, `/bin/nettest`, `/bin/flashdump`, `/bin/sysmon`: Hapus `eval`, cegah infinite loop `shift 2`, pipe `nettest` tanpa file `/tmp`, dan perbaiki rumus CPU% `sysmon`.
  - [x] `dhcp.tcl`, `mtlk_wps_msgloop.tcl`, `update_security.tcl`: Ganti `after` → `sleep`, perbaiki `$::argv`, dan perbaiki `run_wpa_passphrase_for_update_security.sh`.
  - [x] `mtlk_init_start.sh` & `mtlk_init_wps.sh`: Hapus `WPS_PBC.sh` tanpa argumen dan perbaiki urutan `write_wps_config_file`.
- [x] **Batch 3 — P0/P1 Firmware Upgrade UI & Flash Dumper (`W-07`..`W-14`, `F-09`)**:
  - [x] `upldform.c`: Perbaiki `Content-Length` `mtd2` (`0x3A0000`) dan `mtd4` (`0x20000`) di `formDumpFirmware`.
  - [x] `upgrade.asp` & `upldform.c` & `webs.c`: Perbaiki redirect `/admin/admin/upgrade.asp`, reset state `precentComplete` saat awal upload, dan hentikan duplikasi halaman di hidden iframe.
- [x] **Batch 4 — P0/P1 Build Pipeline, Kernel, Drivers & Native Apps (`B-01`..`B-09`, `K-01`..`K-08`)**:
  - [x] `docker/compile.sh`, `net-ver-mkrootfs.sh`, `mkbootp.sh`, `appsconfig/.config`, `apps/dropbear-0.52/Makefile`.
  - [x] `apps/set_env_param/set_env_param.c`, `star_gsw.c`, `gpioDevice.c`, `hnap_wps_status_monitor.c`, `simpdiff.c`, `force_ethpc_renew_ip.c`.
  - [x] Sinkronisasi akhir antara `rootfs-star/` dan `rootfs/` serta pembaruan `AGENTS.md`.
