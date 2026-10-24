#!/usr/bin/env python3
"""
WAP610N Automated Firmware Flasher & Unbricker
Dual-mode: TFTP Super-Fast (LAN) with automatic fallback to Kermit (UART Serial).
Telah dioptimasi dengan Relaxed Timers & Extended Delays (+50% s.d. +100%) untuk stabilitas SoC lambat.
"""
import serial
import time
import sys
import os

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FIRMWARE_DIR = os.path.join(BASE_DIR, 'firmware')
LOG_DIR = os.path.join(BASE_DIR, 'logs')

DEFAULT_PORT = '/dev/ttyUSB0'
DEFAULT_BAUD = 38400
RAM_ADDR = 0x100000
FLASH_ADDR = 0x10040000

START_CHAR = 0x01
END_CHAR = ord('\r')
K_ESCAPE = ord('#')

def find_firmware_file():
    # 1. CLI argument
    if len(sys.argv) > 1 and os.path.exists(sys.argv[1]):
        return os.path.abspath(sys.argv[1])

    # 2. Firmware bundled with this flasher (bootpImage / WAP610N_v1.0.05.bin)
    cand_bootp = os.path.join(FIRMWARE_DIR, 'bootpImage')
    if os.path.exists(cand_bootp):
        return cand_bootp

    cand1 = os.path.join(FIRMWARE_DIR, 'WAP610N_v1.0.05.bin')
    if os.path.exists(cand1):
        return cand1

    # 3. Build output in the repository root
    cand2 = os.path.abspath(os.path.join(BASE_DIR, '..', 'output', 'WAP610N_v1.0.05.bin'))
    if os.path.exists(cand2):
        return cand2

    return cand_bootp

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

        if len(buf) >= 5 and buf[0] == START_CHAR:
            seq_val = untochar(chr(buf[2]))
            type_val = chr(buf[3])
            if type_val == 'Y' and seq_val == (expected_seq % 64):
                return True, buf
            elif type_val == 'N':
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

    ser.write(b"\n")
    time.sleep(0.5)
    p_check = ser.read(ser.in_waiting or 100).decode('latin1', errors='replace')
    if "MTLK" in p_check or "#" in p_check:
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
                    ser.write(b" \n")
                    time.sleep(0.05)
                spam_done = True

            if ("MTLK" in buffer or "U-Boot" in buffer) and ("#" in buffer or ">" in buffer):
                time.sleep(1.0)
                ser.write(b"\n")
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
    ser.write((cmd + "\n").encode('latin1'))

    out = ""
    start_t = time.time()
    time.sleep(0.8)  # Delay jeda awal untuk echo U-Boot

    while time.time() - start_t < timeout:
        if ser.in_waiting > 0:
            chunk = ser.read(ser.in_waiting).decode('latin1', errors='replace')
            out += chunk
            sys.stdout.write(chunk)
            sys.stdout.flush()

            # Cek apakah eksekusi selesai berdasarkan prompt U-Boot di akhir buffer
            clean_out = out.rstrip()
            if clean_out.endswith("MTLK U-MEDIA VB #") or clean_out.endswith("#"):
                if wait_finish_str:
                    if wait_finish_str in out:
                        time.sleep(0.5)  # Jeda stabilisasi buffer
                        return True, out
                else:
                    time.sleep(0.5)
                    return True, out
        else:
            time.sleep(0.1)

    print(f"\n[!] TIMEOUT ({timeout}s) menunggu penyelesaian perintah: {cmd}")
    return False, out

def try_tftp_download(ser, filename="bootpImage"):
    print("\n[*] Mencoba koneksi LAN / TFTP terlebih dahulu...")
    ser.reset_input_buffer()
    ser.write(b"ping 192.168.1.100\n")
    time.sleep(4.0)  # Delay pengujian ping diperpanjang
    ping_resp = ser.read(ser.in_waiting or 1024).decode('latin1', errors='replace')
    print(ping_resp.strip())

    if "is alive" in ping_resp:
        print("[+] Host TFTP (192.168.1.100) terdeteksi ALIVE via LAN! Menggunakan TFTP super cepat...", flush=True)
        time.sleep(1.0)
        ok, tftp_out = exec_uboot_command(ser, f"tftpboot {hex(RAM_ADDR)} {filename}", timeout=60, wait_finish_str="Bytes transferred")
        if ok and "Bytes transferred" in tftp_out:
            print("\n[+] TFTP Download Sukses dalam hitungan detik!", flush=True)
            time.sleep(1.0)
            return True

    print("[*] LAN/TFTP tidak terhubung atau timeout. Melanjutkan dengan UART Kermit transfer...", flush=True)
    ser.write(b"\x03\n")
    time.sleep(1.0)
    ser.reset_input_buffer()
    return False

def run_recovery():
    firmware_path = find_firmware_file()
    if not os.path.exists(firmware_path):
        print(f"[ERROR] File firmware tidak ditemukan: {firmware_path}")
        sys.exit(1)

    with open(firmware_path, 'rb') as f:
        firmware_bytes = f.read()

    file_size = len(firmware_bytes)
    filename = "bootpImage"
    print(f"[*] Firmware Target: {firmware_path}")
    print(f"[*] Ukuran: {file_size:,} bytes (Hex: {hex(file_size)})")

    ser = serial.Serial(DEFAULT_PORT, DEFAULT_BAUD, timeout=1)

    # Step 1: Catch U-Boot
    if not wait_for_prompt(ser, timeout=300):
        print("\n[ERROR] Timeout menunggu U-Boot prompt. Pastikan power router dicabut lalu dicolok kembali.")
        ser.close()
        sys.exit(1)

    # Step 2: Transfer Firmware ke RAM
    print("\n=======================================================", flush=True)
    print(f"[2/5] MEMULAI TRANSFER FIRMWARE KE RAM {hex(RAM_ADDR)}...", flush=True)
    print("=======================================================\n", flush=True)

    loaded_via_tftp = try_tftp_download(ser, filename)

    if not loaded_via_tftp:
        chunks = chunk_firmware(firmware_bytes, max_esc_len=75)
        total_chunks = len(chunks)
        print(f"[*] Mengirim via Kermit ({total_chunks} chunks)...")

        ser.reset_input_buffer()
        ser.write(f"loadb {hex(RAM_ADDR)}\n".encode('latin1'))
        time.sleep(1.0)

        resp = ser.read(ser.in_waiting or 1024).decode('latin1', errors='replace')
        print(resp.strip())

        seq = 0
        s_params = tochar(94) + tochar(5) + tochar(0) + '@' + tochar(13) + '#' + 'N' + '1' + 'N'
        s_pkt = build_packet(seq, 'S', s_params)
        ok, _ = send_and_wait_ack(ser, s_pkt, seq)
        if not ok:
            print("[!] Gagal Send-Init Kermit!")
            ser.close()
            sys.exit(1)
        seq = (seq + 1) % 64

        f_pkt = build_packet(seq, 'F', filename.encode('latin1'))
        ok, _ = send_and_wait_ack(ser, f_pkt, seq)
        if not ok:
            print("[!] Gagal File Header Kermit!")
            ser.close()
            sys.exit(1)
        seq = (seq + 1) % 64

        bytes_sent = 0
        start_time = time.time()

        for idx, chunk in enumerate(chunks):
            d_pkt = build_packet(seq, 'D', chunk)
            ok, _ = send_and_wait_ack(ser, d_pkt, seq)
            if not ok:
                print(f"\n[!] Gagal pada pengiriman data chunk {idx+1}/{total_chunks} (seq {seq})!")
                ser.close()
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
        send_and_wait_ack(ser, z_pkt, seq)
        seq = (seq + 1) % 64

        b_pkt = build_packet(seq, 'B', b"")
        send_and_wait_ack(ser, b_pkt, seq)

        # Tunggu sampai U-Boot selesai menutup sesi Kermit dan kembali ke prompt
        time.sleep(2.0)
        kermit_summary = ""
        wait_kermit_end = time.time()
        while time.time() - wait_kermit_end < 20:
            if ser.in_waiting > 0:
                chunk = ser.read(ser.in_waiting).decode('latin1', errors='replace')
                kermit_summary += chunk
                sys.stdout.write(chunk)
                sys.stdout.flush()
                if "MTLK" in kermit_summary and "#" in kermit_summary:
                    break
            time.sleep(0.2)

    # Jeda relaksasi sebelum Erase Flash
    time.sleep(2.0)

    # Step 3: Erase Flash
    print("\n=======================================================", flush=True)
    print(f"[3/5] MENGHAPUS (ERASE) FLASH MEMORY DARI {hex(FLASH_ADDR)}...", flush=True)
    print("=======================================================\n", flush=True)

    erase_cmd = f"erase {hex(FLASH_ADDR)} +{hex(file_size)}"
    print(f"Executing: {erase_cmd}")
    # Timeout erase dinaikkan ke 180 detik (3 menit) agar pasti tuntas
    ok_erase, out_erase = exec_uboot_command(ser, erase_cmd, timeout=180, wait_finish_str="Erased")
    if not ok_erase or "Erased" not in out_erase:
        print("\n[!] WARNING: Erase mungkin belum selesai sempurna!")
    else:
        print("\n[+] Erase flash SELESAI!", flush=True)

    # Jeda relaksasi sebelum Copy Flash
    time.sleep(2.0)

    # Step 4: Copy from RAM to Flash
    print("\n=======================================================", flush=True)
    print(f"[4/5] MENULIS (FLASHING) FIRMWARE KE FLASH {hex(FLASH_ADDR)}...", flush=True)
    print("=======================================================\n", flush=True)

    cp_cmd = f"cp.b {hex(RAM_ADDR)} {hex(FLASH_ADDR)} {hex(file_size)}"
    print(f"Executing: {cp_cmd}")
    print("[*] Menulis biner ke SPI Flash... (Mohon tunggu sampai status 'done')")
    # Timeout copy dinaikkan ke 180 detik (3 menit)
    ok_cp, out_cp = exec_uboot_command(ser, cp_cmd, timeout=180, wait_finish_str="done")
    if not ok_cp or "done" not in out_cp:
        print("\n[!] WARNING: Copy ke flash belum tuntas!")
    else:
        print("\n[+] Penulisan Flash SELESAI (Status: done)!", flush=True)

    # Jeda relaksasi sebelum Checksum
    time.sleep(2.0)

    # Step 5: Verify Checksum & Reboot
    print("\n=======================================================", flush=True)
    print("[5/5] VERIFIKASI CHECKSUM & REBOOT ROUTER...", flush=True)
    print("=======================================================\n", flush=True)

    cksum_cmd = f"cksum {hex(FLASH_ADDR)}"
    print(f"Executing: {cksum_cmd}")
    ok_ck, out_ck = exec_uboot_command(ser, cksum_cmd, timeout=30, wait_finish_str="checksum")
    if "Good checksum" in out_ck:
        print("\n[+] CHECKSUM VALID (Good checksum)! Firmware 100% utuh di Flash.", flush=True)
    else:
        print(f"\n[!] STATUS CHECKSUM: {out_ck.strip()}", flush=True)

    # Jeda relaksasi sebelum kirim Reset
    time.sleep(2.0)
    print("\n[+] Me-reboot router sekarang...", flush=True)
    ser.write(b"reset\n")
    time.sleep(1.0)

    print("\n=======================================================", flush=True)
    print(" [MONITOR] LIVE BOOT LOG FIRMWARE BARU (Tekan Ctrl+C untuk selesai):")
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
            print(f"\n[*] Monitoring selesai ({e}).")

    ser.close()
    print(f"\n[+] Selesai! Log boot tersimpan di: {log_file}")

if __name__ == '__main__':
    run_recovery()
