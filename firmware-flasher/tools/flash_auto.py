#!/usr/bin/env python3
"""
WAP610N Automated Firmware Flasher & Unbricker
Dual-mode: TFTP Super-Fast (LAN) with automatic fallback to Kermit (UART Serial).
Telah dioptimasi dengan Relaxed Timers & Extended Delays (+50% s.d. +100%) untuk stabilitas SoC lambat.
"""
import argparse
import os
import re
import sys
import time
try:
    import serial
except ImportError:
    serial = None

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
if TOOLS_DIR not in sys.path:
    sys.path.insert(0, TOOLS_DIR)

from verify_firmware import get_configured_version, verify_firmware

BASE_DIR = os.path.dirname(TOOLS_DIR)
FIRMWARE_DIR = os.path.join(BASE_DIR, 'firmware')
LOG_DIR = os.path.join(BASE_DIR, 'logs')

DEFAULT_PORT = '/dev/ttyUSB0'
DEFAULT_BAUD = 38400
RAM_ADDR = 0x100000
FLASH_ADDR = 0x10040000
UBOOT_PROMPT = "MTLK U-MEDIA VB #"

START_CHAR = 0x01
END_CHAR = ord('\r')
import glob

K_ESCAPE = ord('#')


def _newest_glob(patterns):
    matches = []
    for pat in patterns:
        matches.extend(glob.glob(pat))
    files = [p for p in matches if os.path.isfile(p)]
    if not files:
        return None
    files.sort(key=os.path.getmtime, reverse=True)
    return files[0]


def find_firmware_file(explicit_path=None):
    # 1. Explicit CLI argument: fail hard if supplied path is missing (never silently fall back)
    if explicit_path is not None:
        resolved = os.path.abspath(explicit_path)
        if not os.path.isfile(resolved):
            print(f"[ERROR] File firmware yang diminta tidak ditemukan: {explicit_path}", flush=True)
            sys.exit(1)
        return resolved

    ver = get_configured_version()
    ver_clean = ver[1:] if ver.startswith('v') else ver
    ver_bin = f"WAP610N_v{ver_clean}.bin"
    output_dir = os.path.abspath(os.path.join(BASE_DIR, '..', 'output'))

    candidates = [
        os.path.join(FIRMWARE_DIR, 'bootpImage'),
        os.path.join(FIRMWARE_DIR, ver_bin),
        _newest_glob([
            os.path.join(FIRMWARE_DIR, f"WAP610N_v{ver_clean}_*.bin"),
            os.path.join(FIRMWARE_DIR, "WAP610N_v*.bin"),
        ]),
        os.path.join(FIRMWARE_DIR, 'WAP610N_v1.0.05.bin'),
        os.path.join(output_dir, ver_bin),
        _newest_glob([
            os.path.join(output_dir, f"WAP610N_v{ver_clean}_*.bin"),
            os.path.join(output_dir, "WAP610N_v*.bin"),
        ]),
        os.path.join(output_dir, 'bootpImage'),
        os.path.join(output_dir, 'WAP610N_v1.0.05.bin'),
    ]
    for cand in candidates:
        if cand and os.path.isfile(cand):
            return cand

    print(f"[ERROR] Tidak ada file firmware di {FIRMWARE_DIR} atau {output_dir}", flush=True)
    sys.exit(1)

def tochar(x):
    return chr((x + 32) & 0xFF)

def untochar(ch):
    return (ord(ch) - 32) & 0xFF

def chk1(data_bytes):
    s = sum(data_bytes)
    return tochar((s + ((s >> 6) & 0x03)) & 0x3F)

def escape_kermit_data(raw_bytes):
    out = bytearray()
    for b in raw_bytes:
        if b < 32:
            out.append(K_ESCAPE)
            out.append(b ^ 0x40)
        elif b == 127:
            out.append(K_ESCAPE)
            out.append(ord('?'))
        elif b == K_ESCAPE:
            out.append(K_ESCAPE)
            out.append(K_ESCAPE)
        else:
            out.append(b)
    return out

def chunk_firmware(data, max_esc_len=75):
    chunks = []
    curr = bytearray()
    curr_len = 0
    for b in data:
        b_len = 2 if (b < 32 or b == 127 or b == K_ESCAPE) else 1
        if curr_len + b_len > max_esc_len:
            chunks.append(bytes(curr))
            curr = bytearray([b])
            curr_len = b_len
        else:
            curr.append(b)
            curr_len += b_len
    if curr:
        chunks.append(bytes(curr))
    return chunks

def build_packet(seq, ptype, raw_payload=b''):
    if ptype == 'D' or ptype == 'F':
        payload = escape_kermit_data(raw_payload)
    else:
        payload = raw_payload if isinstance(raw_payload, (bytes, bytearray)) else raw_payload.encode('latin1')

    length = len(payload) + 3
    body = tochar(length).encode('latin1') + tochar(seq % 64).encode('latin1') + ptype.encode('latin1') + payload
    chk = chk1(body)
    return bytes([START_CHAR]) + body + chk.encode('latin1') + bytes([END_CHAR])

def send_and_wait_ack(ser, pkt, expected_seq, max_retries=25):
    # Timeout ACK diperpanjang ke 3.0 detik (+100%) agar SoC ARM 200MHz tidak terburu-buru
    for retry in range(max_retries):
        ser.write(pkt)
        start_t = time.time()
        buf = bytearray()
        while time.time() - start_t < 3.0:
            ch = ser.read(1)
            if not ch:
                continue
            if ch[0] == START_CHAR:
                buf = bytearray(ch)
            elif len(buf) > 0:
                buf.extend(ch)
                if ch[0] == END_CHAR or ch[0] == ord('\n'):
                    break

        if len(buf) >= 6 and buf[0] == START_CHAR and buf[-1] in (END_CHAR, ord('\n')):
            if chr(buf[-2]) != chk1(buf[1:-2]):
                time.sleep(0.1)
                continue
            seq_val = untochar(chr(buf[2]))
            type_val = chr(buf[3])
            if type_val == 'Y' and seq_val == (expected_seq % 64):
                return True, buf
            if type_val == 'E':
                err_txt = bytes(buf[4:-2]).decode('latin1', errors='replace')
                print(f"\n[!] Kermit Error dari target: {err_txt}", flush=True)
                return False, buf
            if type_val == 'N':
                time.sleep(0.1)
        time.sleep(0.1)
    return False, None


def wait_for_prompt(ser, timeout=300):
    print("\n=======================================================", flush=True)
    print("[1/5] MENUNGGU ROUTER MENYALA & MENGHENTIKAN AUTOBOOT...", flush=True)
    print(">>> SILAKAN COLOKKAN / NYALAKAN POWER ADAPTOR ROUTER SEKARANG! <<<", flush=True)
    print("=======================================================\n", flush=True)

    ser.reset_input_buffer()
    ser.reset_output_buffer()

    ser.write(b"\r")
    time.sleep(0.5)
    p_check = ser.read(ser.in_waiting or 100).decode('latin1', errors='replace')
    if UBOOT_PROMPT in p_check:
        print("[+] Prompt U-Boot sudah aktif!", flush=True)
        return True

    start_time = time.time()
    buffer = ""
    spam_done = False

    while time.time() - start_time < timeout:
        if ser.in_waiting > 0:
            chunk = ser.read(ser.in_waiting).decode('latin1', errors='replace')
            sys.stdout.write(chunk)
            sys.stdout.flush()
            buffer += chunk

            if not spam_done and ("U-Boot" in buffer or "Clock" in buffer or "RAM" in buffer or "Hit any key" in buffer or "Flash" in buffer):
                for _ in range(30):
                    ser.write(b" \r")
                    time.sleep(0.05)
                spam_done = True

            if UBOOT_PROMPT in buffer:
                time.sleep(1.0)
                ser.write(b"\r")
                time.sleep(0.8)
                print("\n[+] Berhasil masuk ke prompt U-Boot!", flush=True)
                return True
        else:
            time.sleep(0.05)
    return False


def exec_uboot_command(ser, cmd, timeout=180, wait_finish_str=None):
    """
    Eksekusi perintah U-Boot dengan sinkronisasi ketat & timeout santai (+100%).
    Menunggu sampai perintah benar-benar selesai dieksekusi dan prompt MTLK U-MEDIA VB # kembali muncul.
    """
    ser.reset_input_buffer()
    ser.write((cmd + "\r").encode('latin1'))

    out = ""
    start_t = time.time()
    time.sleep(0.8)  # Delay jeda awal untuk echo U-Boot

    while time.time() - start_t < timeout:
        if ser.in_waiting > 0:
            chunk = ser.read(ser.in_waiting).decode('latin1', errors='replace')
            out += chunk
            sys.stdout.write(chunk)
            sys.stdout.flush()

            # Cek ketat prompt U-Boot di akhir buffer (jangan cocokkan '#' progress bar!)
            clean_out = out.rstrip()
            if clean_out.endswith(UBOOT_PROMPT):
                if wait_finish_str:
                    if wait_finish_str in out:
                        time.sleep(0.5)  # Jeda stabilisasi buffer
                        return True, out
                else:
                    time.sleep(0.5)
                    return True, out
        else:
            time.sleep(0.1)

    print(f"\n[!] TIMEOUT ({timeout}s) menunggu penyelesaian perintah: {cmd}", flush=True)
    return False, out


def verify_ram_checksum(ser, ram_addr=RAM_ADDR):
    ok_ck, out_ck = exec_uboot_command(ser, f"cksum {hex(ram_addr)}", timeout=30)
    if (
        not ok_ck
        or "Good checksum" not in out_ck
        or "Bad magic" in out_ck
        or "Bad checksum" in out_ck
    ):
        print(f"\n[!] Verifikasi checksum di RAM ({hex(ram_addr)}) GAGAL:\n{out_ck.strip()}", flush=True)
        return False
    print(f"\n[+] Checksum firmware di RAM ({hex(ram_addr)}) VALID (Good checksum).", flush=True)
    return True


def try_tftp_download(ser, firmware_path, file_size):
    print("\n[*] Menyiapkan parameter jaringan & mencoba koneksi LAN / TFTP...", flush=True)
    for env_cmd in ("setenv ipaddr 192.168.1.1", "setenv serverip 192.168.1.100", "setenv tftpdstp 69"):
        exec_uboot_command(ser, env_cmd, timeout=10)

    ok_ping, ping_resp = exec_uboot_command(ser, "ping 192.168.1.100", timeout=15)
    if ok_ping and "is alive" in ping_resp:
        print("[+] Host TFTP (192.168.1.100) terdeteksi ALIVE via LAN! Menggunakan TFTP super cepat...", flush=True)
        time.sleep(1.0)
        req_filename = os.path.basename(firmware_path)
        ok_tftp, tftp_out = exec_uboot_command(ser, f"tftpboot {hex(RAM_ADDR)} {req_filename}", timeout=60)
        m = re.search(r"Bytes transferred\s*=\s*(\d+)", tftp_out, re.IGNORECASE)
        transferred = int(m.group(1)) if m else -1
        if ok_tftp and transferred == file_size:
            print(f"\n[+] TFTP Download selesai ({transferred:,} bytes == {file_size:,} bytes). Memeriksa checksum RAM...", flush=True)
            if verify_ram_checksum(ser, RAM_ADDR):
                time.sleep(1.0)
                return True
            print("[!] Checksum TFTP di RAM gagal; fallback ke UART Kermit...", flush=True)
        else:
            print(f"\n[!] TFTP transfer tidak lengkap/gagal (terkirim={transferred}, harapan={file_size}).", flush=True)

    print("[*] LAN/TFTP tidak terhubung atau timeout. Melanjutkan dengan UART Kermit transfer...", flush=True)
    ser.write(b"\x03\r")
    time.sleep(1.0)
    ser.reset_input_buffer()
    return False


def _send_via_kermit(ser, firmware_bytes, filename, file_size):
    chunks = chunk_firmware(firmware_bytes, max_esc_len=75)
    total_chunks = len(chunks)
    print(f"[*] Mengirim via Kermit ({total_chunks} chunks)...", flush=True)

    ser.reset_input_buffer()
    ser.write(f"loadb {hex(RAM_ADDR)}\r".encode('latin1'))

    ready_buf = ""
    wait_ready_start = time.time()
    while time.time() - wait_ready_start < 5.0:
        if ser.in_waiting > 0:
            ready_buf += ser.read(ser.in_waiting).decode('latin1', errors='replace')
            if "Ready for binary (kermit) download" in ready_buf:
                break
        time.sleep(0.05)

    print(ready_buf.strip(), flush=True)
    if "Ready for binary (kermit) download" not in ready_buf:
        print("[!] U-Boot tidak siap menerima Kermit ('Ready for binary (kermit) download' tidak muncul)!", flush=True)
        ser.write(b"\x03\r")
        sys.exit(1)

    seq = 0
    s_params = tochar(94) + tochar(5) + tochar(0) + '@' + tochar(13) + '#' + 'N' + '1' + 'N'
    s_pkt = build_packet(seq, 'S', s_params)
    ok, _ = send_and_wait_ack(ser, s_pkt, seq)
    if not ok:
        print("[!] Gagal Send-Init Kermit!", flush=True)
        sys.exit(1)
    seq = (seq + 1) % 64

    f_pkt = build_packet(seq, 'F', filename.encode('latin1'))
    ok, _ = send_and_wait_ack(ser, f_pkt, seq)
    if not ok:
        print("[!] Gagal File Header Kermit!", flush=True)
        sys.exit(1)
    seq = (seq + 1) % 64

    bytes_sent = 0
    start_time = time.time()

    for idx, chunk in enumerate(chunks):
        d_pkt = build_packet(seq, 'D', chunk)
        ok, _ = send_and_wait_ack(ser, d_pkt, seq)
        if not ok:
            print(f"\n[!] Gagal pada pengiriman data chunk {idx+1}/{total_chunks} (seq {seq})!", flush=True)
            sys.exit(1)
        bytes_sent += len(chunk)
        seq = (seq + 1) % 64

        if (idx + 1) % 100 == 0 or (idx + 1) == total_chunks:
            elapsed = time.time() - start_time
            speed = bytes_sent / elapsed if elapsed > 0 else 0
            pct = (bytes_sent / file_size) * 100
            remaining = (file_size - bytes_sent) / speed if speed > 0 else 0
            mins, secs = divmod(int(remaining), 60)
            sys.stdout.write(f"\r[+] Uploading: {bytes_sent:,}/{file_size:,} B ({pct:5.1f}%) | Chunk {idx+1}/{total_chunks} | {speed/1024:4.1f} KB/s | ETA: {mins:02d}:{secs:02d} ")
            sys.stdout.flush()

    print("\n[+] Data transfer selesai!", flush=True)

    z_pkt = build_packet(seq, 'Z', b"")
    ok_z, _ = send_and_wait_ack(ser, z_pkt, seq)
    if not ok_z:
        print("[!] Gagal ACK paket EOF (Z) Kermit!", flush=True)
        sys.exit(1)
    seq = (seq + 1) % 64

    b_pkt = build_packet(seq, 'B', b"")
    ok_b, _ = send_and_wait_ack(ser, b_pkt, seq)
    if not ok_b:
        print("[!] Gagal ACK paket EOT (B) Kermit!", flush=True)
        sys.exit(1)

    # Tunggu sampai U-Boot selesai menutup sesi Kermit dan kembali ke prompt MTLK U-MEDIA VB #
    time.sleep(2.0)
    kermit_summary = ""
    wait_kermit_end = time.time()
    while time.time() - wait_kermit_end < 20:
        if ser.in_waiting > 0:
            chunk = ser.read(ser.in_waiting).decode('latin1', errors='replace')
            kermit_summary += chunk
            sys.stdout.write(chunk)
            sys.stdout.flush()
            if UBOOT_PROMPT in kermit_summary:
                break
        time.sleep(0.2)

    if UBOOT_PROMPT not in kermit_summary:
        print("\n[!] Timeout menunggu U-Boot kembali ke prompt setelah Kermit!", flush=True)
        sys.exit(1)

    m = re.search(r"Total Size\s*=\s*0x[0-9a-fA-F]+\s*=\s*(\d+)\s*Bytes", kermit_summary, re.IGNORECASE)
    if not m or int(m.group(1)) != file_size:
        got_size = m.group(1) if m else "tidak terbaca"
        print(f"\n[!] Ukuran transfer Kermit tidak cocok! Terbaca={got_size}, Harapan={file_size}.", flush=True)
        sys.exit(1)

    if not verify_ram_checksum(ser, RAM_ADDR):
        print("[CRITICAL ERROR] Checksum di RAM gagal setelah transfer Kermit! Membatalkan erase/write flash.", flush=True)
        sys.exit(1)


def run_recovery(port=DEFAULT_PORT, baud=DEFAULT_BAUD, explicit_firmware=None):
    firmware_path = find_firmware_file(explicit_firmware)

    if not verify_firmware(firmware_path):
        print(f"[ERROR] Verifikasi integritas firmware gagal ({firmware_path})! Proses flashing dibatalkan.", flush=True)
        sys.exit(1)

    with open(firmware_path, 'rb') as f:
        firmware_bytes = f.read()

    file_size = len(firmware_bytes)
    filename = os.path.basename(firmware_path)
    print(f"[*] Firmware Target: {firmware_path}", flush=True)
    print(f"[*] Ukuran: {file_size:,} bytes (Hex: {hex(file_size)})", flush=True)

    if serial is None:
        print("[ERROR] Modul Python 'pyserial' belum terpasang. Jalankan: pip3 install pyserial", flush=True)
        sys.exit(1)

    ser = serial.Serial(port, baud, timeout=1)
    try:
        # Step 1: Catch U-Boot
        if not wait_for_prompt(ser, timeout=300):
            print("\n[ERROR] Timeout menunggu U-Boot prompt. Pastikan power router dicabut lalu dicolok kembali.", flush=True)
            sys.exit(1)

        # Step 2: Transfer Firmware ke RAM
        print("\n=======================================================", flush=True)
        print(f"[2/5] MEMULAI TRANSFER FIRMWARE KE RAM {hex(RAM_ADDR)}...", flush=True)
        print("=======================================================\n", flush=True)

        if not try_tftp_download(ser, firmware_path, file_size):
            _send_via_kermit(ser, firmware_bytes, filename, file_size)

        # Jeda relaksasi sebelum Erase Flash
        time.sleep(2.0)

        # Step 3: Erase Flash
        print("\n=======================================================", flush=True)
        print(f"[3/5] MENGHAPUS (ERASE) FLASH MEMORY DARI {hex(FLASH_ADDR)}...", flush=True)
        print("=======================================================\n", flush=True)

        erase_cmd = f"erase {hex(FLASH_ADDR)} +{hex(file_size)}"
        print(f"Executing: {erase_cmd}", flush=True)
        ok_erase, out_erase = exec_uboot_command(ser, erase_cmd, timeout=180, wait_finish_str="Erased")
        if (
            not ok_erase
            or "Erased" not in out_erase
            or "Error" in out_erase
            or "not erased" in out_erase.lower()
        ):
            print("\n[CRITICAL ERROR] Erase flash GAGAL atau timeout! Jangan matikan power; periksa output di atas.", flush=True)
            sys.exit(1)
        print("\n[+] Erase flash SELESAI!", flush=True)

        # Jeda relaksasi sebelum Copy Flash
        time.sleep(2.0)

        # Step 4: Copy from RAM to Flash
        print("\n=======================================================", flush=True)
        print(f"[4/5] MENULIS (FLASHING) FIRMWARE KE FLASH {hex(FLASH_ADDR)}...", flush=True)
        print("=======================================================\n", flush=True)

        cp_cmd = f"cp.b {hex(RAM_ADDR)} {hex(FLASH_ADDR)} {hex(file_size)}"
        print(f"Executing: {cp_cmd}", flush=True)
        print("[*] Menulis biner ke SPI Flash... (Mohon tunggu sampai status 'done')", flush=True)
        ok_cp, out_cp = exec_uboot_command(ser, cp_cmd, timeout=180, wait_finish_str="done")
        if (
            not ok_cp
            or "done" not in out_cp
            or "Error" in out_cp
            or "not equal" in out_cp.lower()
        ):
            print("\n[CRITICAL ERROR] Copy RAM ke Flash GAGAL! Router masih di U-Boot; silakan flash ulang sebelum matikan power.", flush=True)
            sys.exit(1)
        print("\n[+] Penulisan Flash SELESAI (Status: done)!", flush=True)

        # Jeda relaksasi sebelum Checksum
        time.sleep(2.0)

        # Step 5: Verify Checksum & Reboot
        print("\n=======================================================", flush=True)
        print("[5/5] VERIFIKASI CHECKSUM & REBOOT ROUTER...", flush=True)
        print("=======================================================\n", flush=True)

        cksum_cmd = f"cksum {hex(FLASH_ADDR)}"
        print(f"Executing: {cksum_cmd}", flush=True)
        ok_ck, out_ck = exec_uboot_command(ser, cksum_cmd, timeout=30)
        if (
            not ok_ck
            or "Bad magic" in out_ck
            or "Bad checksum" in out_ck
            or "Good checksum" not in out_ck
        ):
            print(f"\n[CRITICAL ERROR] CHECKSUM FLASH DI {hex(FLASH_ADDR)} TIDAK VALID!\n{out_ck.strip()}", flush=True)
            print("[!] Router TIDAK di-reboot demi mencegah brick. Ulangi proses flashing dari U-Boot.", flush=True)
            sys.exit(1)

        print("\n[+] CHECKSUM VALID (Good checksum)! Firmware 100% utuh di Flash.", flush=True)

        # Jeda relaksasi sebelum kirim Reset
        time.sleep(2.0)
        print("\n[+] Me-reboot router sekarang...", flush=True)
        ser.write(b"reset\r")
        time.sleep(1.0)

        print("\n=======================================================", flush=True)
        print(" [MONITOR] LIVE BOOT LOG FIRMWARE BARU (Tekan Ctrl+C untuk selesai):", flush=True)
        print("=======================================================\n", flush=True)

        log_file = os.path.join(LOG_DIR, 'latest_boot.log')
        os.makedirs(LOG_DIR, exist_ok=True)
        with open(log_file, 'w', encoding='utf-8', buffering=1) as f_log:
            start_mon = time.time()
            try:
                # Monitoring log 150 detik (2.5 menit)
                while time.time() - start_mon < 150:
                    if ser.is_open and ser.in_waiting > 0:
                        chunk = ser.read(ser.in_waiting).decode('latin1', errors='replace')
                        sys.stdout.write(chunk)
                        sys.stdout.flush()
                        f_log.write(chunk)
                        f_log.flush()
                    time.sleep(0.1)
            except (KeyboardInterrupt, OSError, Exception) as e:
                print(f"\n[*] Monitoring selesai ({e}).", flush=True)

        print(f"\n[+] Selesai! Log boot tersimpan di: {log_file}", flush=True)
    finally:
        if ser is not None and ser.is_open:
            ser.close()


def main():
    parser = argparse.ArgumentParser(description="WAP610N Automated Firmware Flasher & Unbricker (TFTP / UART Kermit)")
    parser.add_argument("firmware", nargs="?", default=None, help="Path opsional ke file firmware (.bin / bootpImage)")
    parser.add_argument("--port", default=DEFAULT_PORT, help=f"Serial port (default: {DEFAULT_PORT})")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help=f"Baud rate (default: {DEFAULT_BAUD})")
    args = parser.parse_args()
    run_recovery(port=args.port, baud=args.baud, explicit_firmware=args.firmware)


if __name__ == '__main__':
    main()
