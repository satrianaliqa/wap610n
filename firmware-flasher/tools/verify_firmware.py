#!/usr/bin/env python3
"""
WAP610N Firmware Binary Validator
Validates 32-byte header, Device ID (0x0012), CRC32 integrity, and max partition size.
"""
import struct
import sys
import os
import zlib

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FIRMWARE_DIR = os.path.join(BASE_DIR, 'firmware')

EXPECTED_ARM_BRANCH = 0xEA000006
EXPECTED_DEVICE_ID = 0x0012
MAX_PARTITION_SIZE = 0x3A0000  # 3.625 MB


def get_configured_version():
    ver_file = os.path.abspath(os.path.join(BASE_DIR, '..', 'config', 'WAP610N_VERSION'))
    if os.path.isfile(ver_file):
        with open(ver_file, 'r', encoding='utf-8', errors='ignore') as f:
            for line in f:
                if line.startswith('FIRMWARE_VERSION='):
                    return line.split('=', 1)[1].strip().strip('"').strip("'")
    return '1.0.05'


def verify_firmware(file_path):
    print(f"[*] Memeriksa file: {file_path}")
    if not os.path.isfile(file_path):
        print(f"[!] ERROR: File tidak ditemukan!")
        return False

    file_size = os.path.getsize(file_path)
    print(f"    - Ukuran File: {file_size:,} bytes ({hex(file_size)})")

    if file_size > MAX_PARTITION_SIZE:
        print(f"[!] ERROR: Ukuran file ({hex(file_size)}) melebihi kapasitas partisi Parallel NOR Flash (EN29LV320BB) ({hex(MAX_PARTITION_SIZE)})!")
        return False
    else:
        print(f"[+] Ukuran file AMAN di bawah batas partisi Parallel NOR Flash (Sisa kapasitas: {MAX_PARTITION_SIZE - file_size:,} bytes)")

    with open(file_path, 'rb') as f:
        data = f.read()

    if len(data) < 48:
        print(f"[!] ERROR: File terlalu kecil ({len(data)} bytes) untuk memuat 32-byte header + payload + 16-byte trailer!")
        return False

    header = data[:32]
    w0_branch, w1_pcksum2, w2_ver, w3_devid, w4_res, w5_res, w6_res, w7_cksum = struct.unpack('<8I', header)

    print("\n--- 32-BYTE BOOTP HEADER AUDIT ---")
    print(f"Word 0 (ARM Branch) : {hex(w0_branch)} (Expected: 0xea000006)")
    print(f"Word 1 (Offset CRC) : {hex(w1_pcksum2)} ({w1_pcksum2:,} bytes)")
    print(f"Word 2 (Header Ver) : {w2_ver}")
    print(f"Word 3 (Device ID)  : {hex(w3_devid)} ({'WAP610N RESMI' if w3_devid == 0x0012 else 'MISMATCH!'})")
    print(f"Word 7 (Header CRC) : {hex(w7_cksum)}")

    if w0_branch != EXPECTED_ARM_BRANCH:
        print(f"\n[!] FATAL: Word 0 branch magic tidak valid! Nilai: {hex(w0_branch)}, Diharapkan: {hex(EXPECTED_ARM_BRANCH)}")
        return False

    if w3_devid != EXPECTED_DEVICE_ID:
        print(f"\n[!] FATAL: Device ID tidak cocok! Nilai: {hex(w3_devid)}, Diharapkan: {hex(EXPECTED_DEVICE_ID)}")
        return False
    else:
        print(f"[+] Device ID VALID (0x0012 - Cisco Linksys WAP610N)")

    if w1_pcksum2 < 32 or w1_pcksum2 + 16 > len(data):
        print(f"\n[!] FATAL: Offset CRC Word 1 ({hex(w1_pcksum2)}) di luar batas aman payload (len={len(data)})!")
        return False

    if w7_cksum == 0:
        print("\n[!] FATAL: Header CRC Word 7 bernilai 0x00000000 (image belum melalui mkcsum)!")
        return False

    computed_crc = zlib.crc32(data[32:w1_pcksum2]) & 0xFFFFFFFF
    if computed_crc != w7_cksum:
        print(f"\n[!] FATAL: CRC32 payload mismatch! Hitungan: {hex(computed_crc)} vs Header: {hex(w7_cksum)}")
        return False

    # Trailer CRC check
    trailer_offset = w1_pcksum2 + 12
    trailer_crc = struct.unpack('<I', data[trailer_offset:trailer_offset + 4])[0]
    print(f"Trailer CRC (Offset {hex(trailer_offset)}) : {hex(trailer_crc)}")
    if trailer_crc != w7_cksum:
        print(f"[!] FATAL: Trailer CRC mismatch ({hex(trailer_crc)} vs {hex(w7_cksum)})!")
        return False
    print("[+] Payload CRC32, Header CRC, dan Trailer CRC MATCH 1:1!")

    # LZMA Ramdisk signature search (0x5D 0x00 0x00)
    lzma_sig = b'\x5d\x00\x00'
    pos = data.find(lzma_sig, 32)
    if pos != -1:
        print(f"[+] LZMA Ramdisk Signature terdeteksi pada offset {hex(pos)}")
    else:
        print("[!] Warning: Signature LZMA tidak ditemukan di payload")

    print("\n>>> KESIMPULAN: FIRMWARE VALID & SIAP DI-FLASH KE WAP610N <<<")
    return True

if __name__ == '__main__':
    ver = get_configured_version()
    cand_bootp = os.path.join(FIRMWARE_DIR, 'bootpImage')
    cand_bin = os.path.join(FIRMWARE_DIR, f'WAP610N_v{ver}.bin')
    default_target = cand_bootp if os.path.isfile(cand_bootp) else cand_bin
    target = sys.argv[1] if len(sys.argv) > 1 else default_target
    sys.exit(0 if verify_firmware(target) else 1)
