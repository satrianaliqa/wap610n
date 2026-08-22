# WAP610N Project Rules & Invariants

## 1. Strict Workspace Boundary
- Strictly restrict all file operations, searches, and commands to the project directory `/home/satszz/wap610n` (`./wap610n`).
- NEVER access, search, inspect, or reference `/home/satszz/sdcard/` or any directory outside `./wap610n`.

## 2. CI-First Build Workflow
- When asked to update codebase configuration for GitHub, edit files cleanly, verify git diff, and commit & push to GitHub (`satrianaliqa/wap610n`).
- Do not trigger local compilation runs unless explicitly requested by the user.

## 3. Path Portability Invariant
- Never use local absolute paths (e.g. `/home/satszz/...`) inside `.config`, `Makefile`, or build scripts.
- Always use portable relative paths (e.g. `CONFIG_PREFIX="../../rootfs-star"`).

## 4. OEM Feature Integrity
- Preserve all stock WAP610N Access Point features (Dual-Band 2.4/5GHz, WPA2, WPS, WMM QoS, Dump Flash).
