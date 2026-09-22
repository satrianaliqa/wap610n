# 📡 WAP610N Project Rules & Invariants

Master operational directives for AI coding agents working on the **Cisco Linksys WAP610N / WET610N Firmware Restoration & BSP Engine**.

---

## 1. 🔒 Strict Workspace & Path Boundaries
- **Workspace Confinement**: Restrict all read, write, search, and execution commands strictly to the project root directory (`./` or `/home/satszz/wap610n`).
- **Forbidden Paths**: NEVER access, inspect, or reference `/home/satszz/sdcard/` or any directory outside the repository root.
- **Path Portability Invariant**: NEVER introduce local host absolute paths (e.g., `/home/satszz/...`, `/root/...`) into Makefiles, `.config`, header files, or shell scripts. Always enforce relative paths (e.g., `CONFIG_PREFIX="../../rootfs-star"`).

---

## 2. ⚡ GCC 3.4.6 & Ancient Kernel (2.6.16) Toolchain Discipline
- **Legacy C Dialect**: Target compiler is **GCC 3.4.6 (ARM uClibc)**.
  - DO NOT use modern C standards (C99/C11 features like variable declarations inside `for` loops, GNU C extensions from modern GCC, etc.).
  - DO NOT replace legacy macros or refactor working legacy syntax to "modern best practices".
- **Binary Compatibility**:
  - The Metalink wireless driver (`mtlk.ko`) and proprietary modules rely on exact kernel struct layouts and exported symbols from Linux 2.6.16-star.
  - DO NOT modify core kernel headers or struct definitions (`include/linux/skbuff.h`, netdevice structs, etc.) that could alter binary alignment or ABI for proprietary `.ko` blobs.

---

## 3. 💾 Physical Flash (4 MB) & RAM (32 MB) Budget Guardrails
- **Hard Flash Partition Limit**:
  - The maximum allowable size for `bootpImage` / `WAP610N_v1.0.08.bin` is **3.625 MB (3,801,088 bytes)**.
  - Any build exceeding this limit **WILL CORRUPT NVRAM / CONFIGFS PARTITIONS (`0x103E0000`) AND BRICK HARDWARE**.
  - Always verify binary size before proposing or committing userspace additions.
- **RAM Footprint (32 MB)**:
  - Do not introduce memory-hungry daemons. All new utilities must be lightweight applets compiled against **uClibc** (strip all debug symbols with `arm-linux-uclibc-strip`).
  - Temporary files and runtime logs must strictly use `/tmp` (`tmpfs`), never writing frequently to physical flash sectors.

---

## 4. 🛠️ CI-First Build Workflow
- **No Heavy Local Compilation**: Do not attempt full local kernel toolchain builds unless explicitly requested by the user.
- **Validation Before Commit**:
  - Check `git diff` carefully before staging changes.
  - Ensure changes do not break the Docker build engine (`docker/Dockerfile`, `docker/compile.sh`, and `.github/workflows/compile.yml`).
- **Clean Commit & Push**: Commit with clear, descriptive commit messages documenting modified subsystem(s), then push to GitHub (`satrianaliqa/wap610n`) to let GitHub Actions CI do the heavy lifting.

---

## 5. 🛡️ OEM Feature & Hardware Integrity
- **Core Functionality Preservation**:
  - Maintain all stock Access Point capabilities: Dual-Band (2.4 GHz / 5 GHz), WPA2-PSK/Enterprise, WPS (GPIO 3), WMM QoS, and Ethernet PHY (`RTL8201CP`).
- **Custom Enhancements Continuity**:
  - Preserve integrated enhancements: **Dropbear SSH (Port 22)**, **Telnet (Port 23)**, **Custom Web GUI MTD Flash Dumper**, and memory sysctl tunings.