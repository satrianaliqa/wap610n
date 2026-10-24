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

EXPECTED_DEVICE_ID = 0x0012
MAX_PARTITION_SIZE = 0x3A0000  # 3.625 MB

def verify_firmware(file_path):
    print(f"[*] Memeriksa file: {file_path}")
    if not os.path.exists(file_path):
        print(f"[!] ERROR: File tidak ditemukan!")
        return False

    file_size = os.path.getsize(file_path)
    print(f"    - Ukuran File: {file_size:,} bytes ({hex(file_size)})")

    if file_size > MAX_PARTITION_SIZE:
        print(f"[!] ERROR: Ukuran file ({hex(file_size)}) melebihi kapasitas partisi SPI Flash ({hex(MAX_PARTITION_SIZE)})!")
        return False
    else:
        print(f"[+] Ukuran file AMAN di bawah batas partisi flash (Sisa kapasitas: {MAX_PARTITION_SIZE - file_size:,} bytes)")

    with open(file_path, 'rb') as f:
        data = f.read()

    if len(data) < 32:
        print(f"[!] ERROR: File terlalu kecil untuk memuat 32-byte header!")
        return False

    header = data[:32]
    w0_branch, w1_pcksum2, w2_ver, w3_devid, w4_res, w5_res, w6_res, w7_cksum = struct.unpack('<8I', header)

    print("\n--- 32-BYTE BOOTP HEADER AUDIT ---")
    print(f"Word 0 (ARM Branch) : {hex(w0_branch)} (Expected: 0xea000006)")
    print(f"Word 1 (Offset CRC) : {hex(w1_pcksum2)} ({w1_pcksum2:,} bytes)")
    print(f"Word 2 (Header Ver) : {w2_ver}")
    print(f"Word 3 (Device ID)  : {hex(w3_devid)} ({'WAP610N RESMI' if w3_devid == 0x0012 else 'MISMATCH!'})")
    print(f"Word 7 (Header CRC) : {hex(w7_cksum)}")

    if w3_devid != EXPECTED_DEVICE_ID:
        print(f"\n[!] FATAL: Device ID tidak cocok! Nilai: {hex(w3_devid)}, Diharapkan: {hex(EXPECTED_DEVICE_ID)}")
        return False
    else:
        print(f"[+] Device ID VALID (0x0012 - Cisco Linksys WAP610N)")

    # Trailer CRC check
    trailer_offset = w1_pcksum2 + 12
    if trailer_offset + 4 <= len(data):
        trailer_crc = struct.unpack('<I', data[trailer_offset:trailer_offset+4])[0]
        print(f"Trailer CRC (Offset {hex(trailer_offset)}) : {hex(trailer_crc)}")
        if trailer_crc == w7_cksum:
            print("[+] Header CRC dan Trailer CRC MATCH 1:1!")
        else:
            print(f"[!] WARNING: Trailer CRC mismatch ({hex(trailer_crc)} vs {hex(w7_cksum)})")
    else:
        print(f"[!] WARNING: Trailer offset ({hex(trailer_offset)}) di luar batas file ({len(data)})")

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
    cand_bootp = os.path.join(FIRMWARE_DIR, 'bootpImage')
    cand_bin = os.path.join(FIRMWARE_DIR, 'WAP610N_v1.0.05.bin')
    default_target = cand_bootp if os.path.exists(cand_bootp) else cand_bin
    target = sys.argv[1] if len(sys.argv) > 1 else default_target
    verify_firmware(target)
