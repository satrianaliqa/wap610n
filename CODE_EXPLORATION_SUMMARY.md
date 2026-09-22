# 🔍 WAP610N / WET610N Firmware - Code Exploration Report

## 📋 Executive Summary

Ini adalah **master repository restorasi firmware resmi** untuk Access Point Cisco Linksys WAP610N dan Wireless Bridge WET610N. Proyek ini mencakup:

- ✅ Rekonstruksi 100% source code kernel Linux 2.6.16-star (Cavium STR8132 SoC)
- ✅ Restorasi Board Support Package (BSP) star-6.7.2-mtlk-U-Media-vela
- ✅ Kompilasi otomatis via Docker + GitHub Actions CI/CD
- ✅ Firmware image siap flash dengan size budget 3.625 MB
- ✅ Custom enhancements: SSH (Dropbear), Telnet, WPS button, GPIO driver, Web UI

**Repository**: [satrianaliqa/wap610n](https://github.com/satrianaliqa/wap610n)  
**License**: GPLv2  
**Toolchain**: GCC 3.4.6 (ARM uClibc) + Make 3.81  
**Codebase**: ~14.6K baris C/H

---

## 🏗️ Hardware Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    CISCO LINKSYS WAP610N                    │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────────────────────────────────────────────┐    │
│  │  Cavium / Star Semiconductor STR8132 (CNS2132)     │    │
│  │  • ARM922T core @ 200 MHz (ARMv4T)                 │    │
│  │  • 32 MB SDRAM (0x00000000 - 0x02000000)           │    │
│  │  • 4 MB SPI Flash (0x10000000 - 0x10400000)        │    │
│  └─────────────────────────────────────────────────────┘    │
│                          │                                   │
│  ┌────────────────┬──────┴────────┬────────────────┐        │
│  │                │               │                │        │
│  ▼                ▼               ▼                ▼        │
│┌──────────┐  ┌──────────┐   ┌──────────┐  ┌──────────┐     │
││  Flash   │  │Realtek  │   │ Metalink │  │  UART    │     │
││ ROM (4MB)│  │RTL8201CP│   │  MTLK    │  │ Console  │     │
││ EON      │  │10/100   │   │802.11a/n │  │38400 baud│     │
││EN29LV640B│  │PHY      │   │Dual-band │  │8N1,3.3V  │     │
│└──────────┘  └──────────┘   └──────────┘  └──────────┘     │
│                                                              │
│  GPIO-3: WPS Button (Active Low, hold 3s to reset)         │
└─────────────────────────────────────────────────────────────┘
```

---

## 💾 Flash Memory Partitioning

```
┌──────────────────────┬──────────────────────────────────────┐
│ Address Range        │ Partition (MTD)                      │
├──────────────────────┼──────────────────────────────────────┤
│ 0x10000000-0x1003FFFF│ mtdblock1: U-Boot Bootloader (256KB) │
│ [READ-ONLY HW PROT]  │ • Hardware write-protected           │
│                      │ • NEVER overwritten during flash     │
├──────────────────────┼──────────────────────────────────────┤
│ 0x10040000-0x103DFFFF│ mtdblock2: Kernel + Ramdisk (3.6MB) │
│ [RE-WRITABLE]        │ • bootpImage (LZMA compressed)      │
│                      │ • Max size: 3,801,088 bytes (CRITICAL)│
├──────────────────────┼──────────────────────────────────────┤
│ 0x103E0000-0x103EFFFF│ mtdblock3: NVRAM / Environment (64KB)│
│ [RE-WRITABLE]        │ • Parameter table & config           │
├──────────────────────┼──────────────────────────────────────┤
│ 0x103F0000-0x103FFFFF│ mtdblock4: Configfs / Calibration   │
│ [RE-WRITABLE]        │ • Factory calibration data (64KB)    │
└──────────────────────┴──────────────────────────────────────┘

⚠️  WARNING: bootpImage > 3,801,088 bytes = BRICK DEVICE
```

---

## 📁 Codebase Structure

```
/workspaces/wap610n/
│
├── 📄 Makefile                     [Top-level build orchestrator]
├── 📄 make_nv.sh                   [Build script: reconf/build/clean]
├── 📄 net_ver.cm                   [SVN source manifest (historical)]
├── 📄 AGENTS.md                    [Project operational rules & constraints]
│
├── 📁 apps/                        [User-space applications & daemons]
│   ├── busybox-1.8.1/              ├→ Shell, utilities (sh, cp, grep, sed, awk)
│   ├── dropbear-0.52/              ├→ SSH daemon (port 22, secure access)
│   ├── WebServer/                  ├→ HTTP server + ASP web UI
│   ├── gpio_driver/                ├→ GPIO kernel module (WPS button, LEDs)
│   ├── wpa_supplicant-0.5/         ├→ WLAN authentication (WPA2, 802.1X)
│   ├── openssl-0.9.8a/             ├→ SSL/TLS library (HTTPS, SSH)
│   ├── hnap_wps_status_monitor/    ├→ WPS status monitoring daemon
│   ├── Intel-WPS/                  ├→ WPS PushButton implementation
│   ├── mtlk_vb_upnpd/              ├→ UPnP daemon (AV mediaserver)
│   ├── e2fsprogs-1.40.4/           ├→ ext2 filesystem utilities
│   ├── tinytcl/                    ├→ TCL interpreter (scripting)
│   ├── expat/                      ├→ XML parser
│   └── [9 other utility modules]
│
├── 📁 kernel/                      [Linux kernel 2.6.16-star source]
│   └── linux-2.6.16-star/
│       ├── arch/arm/               ├→ ARM922T architecture (Cavium STR8132)
│       ├── drivers/                ├→ Device drivers
│       │   ├── mtlk/               ├→ Metalink wireless driver (binary blob .ko)
│       │   ├── usb/
│       │   └── net/
│       ├── kernel/                 ├→ Core kernel subsystems
│       ├── fs/                     ├→ Filesystem support (JFFS2, ext2)
│       ├── include/                ├→ Kernel headers (critical for ABI)
│       ├── Makefile                ├→ Kernel build system
│       └── .config                 ├→ Kernel config (star-6.7.2-mtlk)
│
├── 📁 boards/                      [Board Support Package (BSP)]
│   └── star-6.7.2-mtlk-U-Media-vela/
│       ├── appsconfig/             ├→ App-level configurations
│       ├── kernel/                 ├→ Kernel config template
│       └── busybox/                ├→ BusyBox config
│
├── 📁 rootfs/                      [Root filesystem base template]
│   ├── bin/, sbin/                 ├→ Binaries
│   ├── lib/                        ├→ Shared libraries (libc, drivers)
│   ├── etc/                        ├→ Configuration files
│   │   ├── inittab
│   │   ├── passwd, shadow
│   │   ├── mtlk_init_start.sh      ├→ Wireless init script
│   │   └── hotplug.d/              ├→ Hotplug scripts
│   ├── usr/                        ├→ User programs
│   ├── dev/                        ├→ Device nodes
│   └── [standard FHS directories]
│
├── 📁 rootfs-star/                 [Pre-built rootfs (extracted from .tgz)]
│   └── [Same as rootfs/ but runtime]
│
├── 📁 u-boot-1.1.4/                [U-Boot 1.1.4 bootloader source]
│   ├── board/                      ├→ Board-specific bootloader config
│   ├── cpu/                        ├→ CPU initialization
│   └── drivers/                    ├→ Bootloader drivers
│
├── 📁 tools/                       [Compiled toolchains & build tools]
│   └── arm-uclibc-3.4.6/           ├→ ARM GCC 3.4.6 + uClibc cross-compiler
│       ├── bin/arm-linux-*         ├→ Compiler, linker, objcopy, etc.
│       └── lib/                    ├→ uClibc libraries
│
├── 📁 docker/                      [Containerized build system]
│   ├── Dockerfile                  ├→ 32-bit Debian 8 + build dependencies
│   ├── compile.sh                  ├→ Docker build orchestration
│   └── verify-firmware.sh          ├→ Firmware integrity audit
│
├── 📁 .github/workflows/           [GitHub Actions CI/CD]
│   └── compile.yml                 ├→ Auto-build on push/tag
│
├── 📁 devscripts/                  [Development utilities]
│   ├── initenv.sh                  ├→ Environment setup
│   ├── select-star.sh              ├→ Platform selection
│   ├── mkrootfs.sh                 ├→ Root filesystem builder
│   ├── release-pack.sh             ├→ Release packaging
│   └── [10+ other dev utilities]
│
└── 📁 config/                      [Configuration & version info]
    ├── WAP610N_VERSION             ├→ Firmware version: 1.0.05
    └── WET610N_VERSION             ├→ WET610N version
```

---

## 🚀 Build System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                        BUILD ENTRY POINT                         │
│                     make / Makefile                              │
├─────────────────────────────────────────────────────────────────┤
│ make configure  →  ./make_nv.sh reconf star-6.7.2-mtlk-U-Media  │
│ make build      →  ./make_nv.sh build wlan.tar.gz WAP610N       │
│ make clean      →  ./make_nv.sh clean WAP610N                   │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    DOCKER BUILD PIPELINE                         │
│              ./docker/compile.sh WAP610N                         │
├─────────────────────────────────────────────────────────────────┤
│ 1. Build container image (32-bit Debian 8)                      │
│    └─ GCC 3.4.6, Make 3.81, build-essential, lzma, etc.         │
│                                                                  │
│ 2. Extract rootfs-star.tgz → rootfs-star/                       │
│    └─ Setup device nodes (/dev/console, /dev/ttyS0)             │
│    └─ Symlink init scripts & binaries                           │
│                                                                  │
│ 3. Reconfigure board platform                                   │
│    └─ Board config: star-6.7.2-mtlk-U-Media-vela                │
│    └─ Kernel .config → boards/.../kernel/.config               │
│    └─ Apps config → boards/.../appsconfig/                      │
│                                                                  │
│ 4. Compile Kernel (Linux 2.6.16-star)                          │
│    └─ Cross-compile: arm-linux-uclibc-gcc (3.4.6)               │
│    └─ Output: vmlinux → bootpImage (after objcopy)              │
│    └─ mkcsum: Calculate CRC32 header/footer                     │
│                                                                  │
│ 5. Compile Applications                                         │
│    └─ BusyBox → /bin/busybox (init, sh, utils)                  │
│    └─ WebServer → /usr/sbin/webs (HTTP daemon)                  │
│    └─ Dropbear → /usr/sbin/dropbear (SSH on port 22)            │
│    └─ GPIO driver → gpio.ko (WPS button control)                │
│    └─ Kernel modules → mtlk.ko (proprietary wireless)           │
│                                                                  │
│ 6. Build Ramdisk (LZMA compressed ext2)                         │
│    └─ Create ramdisk_2.6.16.img.lzma                            │
│    └─ Rootfs packed: /bin, /etc, /lib, /usr, /sys              │
│                                                                  │
│ 7. Create Firmware Image (bootpImage)                           │
│    └─ kernel vmlinux (objcopy binary)                           │
│    └─ + ramdisk LZMA                                            │
│    └─ + custom bootloader header                                │
│    └─ = bootpImage (~2.71 MB, must be < 3.625 MB)              │
│                                                                  │
│ 8. Quality Assurance                                            │
│    └─ Verify file sizes, CRC checksums                          │
│    └─ Validate firmware signature                               │
│    └─ Audit binary compatibility                                │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                      OUTPUT ARTIFACTS                            │
├─────────────────────────────────────────────────────────────────┤
│ output/
│ ├── bootpImage           [Main firmware: kernel + ramdisk]
│ ├── WAP610N_v1.0.05.bin  [Flashable binary for Web UI]
│ ├── ramdisk_2.6.16.img.lzma
│ ├── System.map           [Kernel symbol table]
│ ├── kernel_build.log     [Compilation log]
│ └── [QA reports]
└─────────────────────────────────────────────────────────────────┘
```

---

## 🔄 Startup Sequence

```
┌──────────────────────────────────────────────────────────────┐
│              CISCO LINKSYS WAP610N BOOT FLOW                 │
├──────────────────────────────────────────────────────────────┤
│                                                              │
│ 1. [POWER ON] → U-Boot 1.1.4 (0x10000000)                  │
│    └─ Initialize CPU, memory, UART (38400 baud)            │
│    └─ Load bootpImage from mtdblock2 (0x10040000)          │
│                                                              │
│ 2. [KERNEL LOAD] → Linux 2.6.16-star kernel                │
│    └─ Decompress vmlinux (ARM922T code)                    │
│    └─ Mount ramdisk_2.6.16.img.lzma as rootfs              │
│    └─ Initialize PCI, UART, GPIO, Ethernet (RTL8201)      │
│                                                              │
│ 3. [INIT SCRIPTS] → /sbin/init (→ busybox)                 │
│    └─ Parse /etc/inittab                                   │
│    └─ Mount /proc, /sys, /dev                              │
│    └─ Mount JFFS2 (/mnt/jffs2) if available                │
│                                                              │
│ 4. [DAEMONS] → /etc/mtlk_init_start.sh                     │
│    ├─ telnetd (port 23, /bin/sh)      [Early boot]         │
│    ├─ dropbear SSH (port 22)          [Early boot]         │
│    ├─ WPS PBC monitor (/root/mtlk/etc/WPS_PBC.sh)          │
│    └─ → Execute /root/mtlk/etc/mtlk_init.sh                │
│                                                              │
│ 5. [WIRELESS INIT] → mtlk_init.sh                          │
│    ├─ Load mtlk.ko (Metalink driver)                       │
│    ├─ Configure 802.11a/b/g/n (dual-band)                  │
│    ├─ Start hostapd (Access Point mode)                    │
│    ├─ Assign IP: 192.168.1.1/24                            │
│    └─ Start udhcpd (DHCP server)                           │
│                                                              │
│ 6. [WEB UI] → WebServer (HTTP, port 80)                    │
│    ├─ Listen on 192.168.1.1:80                             │
│    ├─ Serve ASP pages + static content                     │
│    └─ CGI handlers for firmware update, settings           │
│                                                              │
│ 7. [READY] → Access Point fully operational                │
│    ├─ Wireless SSID broadcast (2.4 GHz / 5 GHz)            │
│    ├─ Ethernet WAN + LAN bridging                          │
│    └─ Management via SSH (22), Telnet (23), HTTP (80)      │
│                                                              │
└──────────────────────────────────────────────────────────────┘
```

**Key Daemons**:
- `telnetd` - Telnet (port 23)
- `dropbear` - SSH (port 22)
- `webs` - HTTP server (port 80)
- `hostapd` - Wireless AP mode
- `udhcpd` - DHCP server
- `mtlk` - Metalink wireless driver

---

## 🎯 Key Features & Customizations

| Feature | Status | Location |
|---------|--------|----------|
| **Dual-Band Wireless (2.4/5 GHz)** | ✅ Stock | mtlk driver + hostapd |
| **WPA2-PSK/Enterprise** | ✅ Stock | wpa_supplicant + hostapd |
| **WPS PushButton** | ✅ Stock + Enhanced | GPIO-3 + hnap_wps_status_monitor |
| **SSH Daemon (Dropbear)** | ✅ Custom | apps/dropbear-0.52/ (port 22) |
| **Telnet Daemon** | ✅ Custom | BusyBox telnetd (port 23) |
| **Web GUI** | ✅ Stock | apps/WebServer/ (port 80) |
| **GPIO Driver** | ✅ Custom | apps/gpio_driver/ (kernel module) |
| **Firmware Flasher (Web)** | ✅ Stock | upgrade.asp + CGI handler |
| **Firmware Dumper (MTD)** | ✅ Custom | Web UI flash dump tool |
| **Advanced Busybox Utils** | ✅ Custom | dmesg, tail, head, find, xargs, vi, hexdump |

---

## 💻 Configuration Files

### Kernel Configuration
**File**: `boards/star-6.7.2-mtlk-U-Media-vela/kernel/.config`
- Cavium STR8132 CPU support (ARM922T)
- JFFS2, ext2, vfat filesystems
- Metalink wireless driver (mtlk.ko) support
- Ethernet (RTL8201 PHY)
- GPIO, UART, SPI, USB drivers
- Netfilter/iptables for firewall

### BusyBox Configuration
**File**: `boards/star-6.7.2-mtlk-U-Media-vela/busybox/.config`
- **Applets enabled**: sh, cp, grep, sed, awk, find, xargs, vi, diff, stat, head, tail, dmesg, top, uptime, netstat, traceroute, hexdump, strings, watch, bzip2, readlink, realpath
- **Shell features**: job control, arithmetic expansion, command history
- **Network tools**: ifconfig, route, ping, telnet, wget, nc

### Init Configuration
**File**: `rootfs-star/etc/inittab`
- Runlevel configuration
- Console spawn (ttyS0 at 38400 baud)
- Startup script execution

### Network Configuration
**File**: `rootfs-star/etc/hostname`, `hosts`, `resolv.conf`
- Default hostname
- DNS resolution setup
- Network interface defaults

---

## 📊 Codebase Metrics

```
┌──────────────────────────────────────────┐
│ Codebase Statistics                      │
├──────────────────────────────────────────┤
│ Total C/H code lines:   ~14,657           │
│                                          │
│ Major components:                        │
│  • Kernel (Linux 2.6.16):  ~2.5M LOC     │
│  • BusyBox 1.8.1:          ~100K LOC     │
│  • WebServer:              ~50K LOC      │
│  • Dropbear SSH:           ~70K LOC      │
│  • Applications:           ~15K LOC      │
│  • U-Boot 1.1.4:           ~200K LOC     │
│                                          │
│ Flash budget:             4 MB           │
│ Max kernel+ramdisk:       3.625 MB       │
│ Current firmware size:    ~2.71 MB ✅    │
└──────────────────────────────────────────┘
```

---

## 🔐 Security & Hardening

- **SSH Access**: Dropbear daemon (port 22) with rsa, dss key auth
- **Telnet Access**: BusyBox telnetd (port 23) for remote console
- **Web UI**: SSL/TLS via OpenSSL 0.9.8a (HTTPS capable)
- **Firewall**: Netfilter/iptables kernel support
- **User/Group**: Multi-user support (passwd/shadow files)
- **Root Password**: Default (admin/admin recommended during init)

---

## ⚙️ Build Commands Reference

```bash
# ━━━ Docker-based full compilation (RECOMMENDED) ━━━
./docker/compile.sh WAP610N
./docker/compile.sh WET610N

# ━━━ Local Make-based build (requires toolchain) ━━━
make configure              # Reconfigure platform
make build                  # Build firmware
make clean                  # Clean artifacts

# ━━━ Manual build steps (for debugging) ━━━
./make_nv.sh reconf star-6.7.2-mtlk-U-Media-vela
./make_nv.sh build wlan.tar.gz WAP610N
./make_nv.sh clean WAP610N

# ━━━ Development & debugging ━━━
./devscripts/select-star.sh                    # Select platform
./devscripts/mkrootfs.sh                       # Build rootfs
./devscripts/initenv.sh                        # Setup env
./docker/verify-firmware.sh output/bootpImage  # Audit firmware
```

---

## 📌 Project Rules & Constraints

### ✅ MUST DO
1. All work within `/workspaces/wap610n/` only
2. Use relative paths in Makefiles (NO hardcoded `/home/...`)
3. Verify bootpImage size < 3,801,088 bytes before commit
4. Test Docker build before pushing to GitHub
5. Commit with clear descriptions of modified subsystems
6. Use GCC 3.4.6 compatible C code (NO C99/C11 modern features)

### ❌ NEVER DO
1. Access `/home/satszz/sdcard/` or external directories
2. Modify kernel struct layouts (breaks mtlk.ko binary ABI)
3. Use variable declarations in `for` loops (not C89)
4. Exceed 3.625 MB firmware size (device brick)
5. Change core kernel headers without ABI verification
6. Introduce memory-hungry userspace daemons

### 🛡️ Workflow
- **CI-First**: Push to GitHub, let GitHub Actions validate
- **Check Before Commit**: `git diff` + `docker/compile.sh`
- **Verify Outputs**: Check `output/` artifacts + size audit
- **Clean Staging**: Only commit production-ready code

---

## 🔗 Recent Development (Commit History)

| Commit | Message |
|--------|---------|
| `0f0ca579` | fix(boot): jalankan daemon telnetd, dropbear SSH, WPS safe-shutdown |
| `ba44d648` | feat(wps): jalankan daemon monitoring tombol hardware safe-shutdown |
| `01b5def2` | fix(build): jamin ketersediaan mkcsum di seluruh level build |
| `61050eec` | feat(core): enable system-rw readiness, persistent telnet/dropbear |
| `f0d61a16` | feat(remote): aktifkan auto-start telnetd + dropbear SSH |
| `56aa2051` | fix(kernel): kembalikan target aturan build bootp & mkcsum |
| `4b0b4618` | fix(build): perbaiki pembuatan ramdisk ext2 di container |
| `44888d23` | fix(ci): hilangkan hardcoded version string di workflow |
| `45068dfe` | fix(ci): perbaiki kompilasi mkcsum, normalisasi output |
| `7e046b4b` | fix(busybox): sesuaikan CONFIG_PREFIX ke relative path |
| `ffcc0c61` | feat(busybox): aktifkan applet lengkap (dmesg, tail, find, xargs, vi) |

**Current Branch**: `testing`  
**Last Update**: September 2026  
**Team**: satrianaliqa (Satrian Aliqa)

---

## 📚 Additional Resources

- **README.md**: Detailed technical background, flash map, compilation guide
- **AGENTS.md**: Operational rules, path boundaries, constraints
- **Dockerfile**: 32-bit build environment setup
- **GitHub Actions**: Auto-build on push (`.github/workflows/compile.yml`)
- **devscripts/**: Utility scripts for development & release

---

**Created**: Code Exploration Report  
**Last Updated**: 2026-09-22  
**For**: Cisco Linksys WAP610N / WET610N Firmware Restoration Project
