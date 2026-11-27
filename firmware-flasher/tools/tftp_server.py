#!/usr/bin/env python3
"""
WAP610N Standalone TFTP Server (Divisi Flasher)
Serves firmware binary files over UDP Port 69 for fast U-Boot network booting/flashing.
"""
import socket
import struct
import os
import sys
import time

TOOLS_DIR = os.path.dirname(os.path.abspath(__file__))
if TOOLS_DIR not in sys.path:
    sys.path.insert(0, TOOLS_DIR)

from verify_firmware import get_configured_version

BASE_DIR = os.path.dirname(TOOLS_DIR)
PORT = 69
DEFAULT_SERVE_DIR = os.path.join(BASE_DIR, 'firmware')
ALT_SERVE_DIR = os.path.abspath(os.path.join(BASE_DIR, '..', 'output'))


def _resolve_tftp_target(serve_dir, alt_dir, filename):
    import glob
    req_name = os.path.basename(filename.replace('\\', '/'))
    if not req_name:
        return None

    ver = get_configured_version()
    ver_clean = ver[1:] if ver.startswith('v') else ver
    ver_bin = f"WAP610N_v{ver_clean}.bin"
    aliases = (ver_bin, 'bootpImage', 'WAP610N_v1.0.05.bin')

    candidates = [req_name]
    is_fw_alias = (req_name.lower() in {a.lower() for a in aliases} or
                   (req_name.lower().startswith('wap610n_v') and req_name.lower().endswith('.bin')))
    if is_fw_alias:
        for alias in aliases:
            if alias not in candidates:
                candidates.append(alias)

    search_dirs = [serve_dir]
    if alt_dir and os.path.abspath(alt_dir) != os.path.abspath(serve_dir):
        search_dirs.append(alt_dir)

    for directory in search_dirs:
        if not os.path.isdir(directory):
            continue
        for cand in candidates:
            path = os.path.join(directory, cand)
            if os.path.isfile(path):
                return path
        if is_fw_alias:
            globs = (glob.glob(os.path.join(directory, f'WAP610N_v{ver_clean}_*.bin')) +
                     glob.glob(os.path.join(directory, 'WAP610N_v*.bin')))
            globs = [p for p in globs if os.path.isfile(p)]
            if globs:
                globs.sort(key=os.path.getmtime, reverse=True)
                return globs[0]
    return None

def _send_tftp_file(target_path, client_addr):
    file_size = os.path.getsize(target_path)
    print(f"[TFTP] Mengirim file: {target_path} (Ukuran: {file_size} bytes)...", flush=True)

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as transfer_sock:
        transfer_sock.settimeout(3.0)
        with open(target_path, 'rb') as f:
            block_num = 1
            transfer_aborted = False
            while True:
                block_data = f.read(512)
                data_pkt = struct.pack('!HH', 3, block_num) + block_data

                ack_received = False
                for retry in range(5):
                    transfer_sock.sendto(data_pkt, client_addr)
                    deadline = time.monotonic() + 3.0
                    while True:
                        remaining = deadline - time.monotonic()
                        if remaining <= 0:
                            break
                        transfer_sock.settimeout(remaining)
                        try:
                            ack_pkt, ack_addr = transfer_sock.recvfrom(516)
                        except socket.timeout:
                            break
                        if ack_addr != client_addr:
                            continue
                        if len(ack_pkt) < 4:
                            continue
                        ack_opcode, ack_param = struct.unpack('!HH', ack_pkt[:4])
                        if ack_opcode == 5:
                            err_msg = ack_pkt[4:].split(b'\x00', 1)[0].decode('ascii', errors='replace')
                            print(f"[TFTP ERROR] Client mengirim ERROR untuk blok {block_num} (code={ack_param}): {err_msg}", flush=True)
                            transfer_aborted = True
                            break
                        if ack_opcode == 4 and ack_param == block_num:
                            ack_received = True
                            break
                    if ack_received or transfer_aborted:
                        break
                if transfer_aborted:
                    break
                if not ack_received:
                    print(f"[TFTP ERROR] Gagal mengirim blok {block_num} setelah 5 percobaan.", flush=True)
                    break

                if block_num % 500 == 0:
                    print(f"[TFTP] Terkirim blok {block_num} ({block_num * 512} bytes)...", flush=True)

                if len(block_data) < 512:
                    print(f"[TFTP SUCCESS] Pengiriman file selesai! Total blok: {block_num}", flush=True)
                    break

                block_num = (block_num + 1) & 0xFFFF


def run_tftp_server(serve_dir=None):
    if serve_dir is None:
        serve_dir = DEFAULT_SERVE_DIR
    alt_dir = ALT_SERVE_DIR

    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        try:
            sock.bind(('0.0.0.0', PORT))
        except Exception as e:
            print(f"[ERROR] Gagal bind ke port {PORT}: {e}")
            print("Pastikan script dijalankan dengan sudo atau port 69 tidak sedang dipakai.")
            sys.exit(1)

        print(f"[TFTP SERVER] Berjalan di port {PORT}, melayani folder: {serve_dir} (fallback: {alt_dir})", flush=True)

        while True:
            try:
                data, client_addr = sock.recvfrom(516)
                if len(data) < 4:
                    continue

                opcode = struct.unpack('!H', data[:2])[0]
                if opcode == 1:  # RRQ (Read Request)
                    parts = data[2:].split(b'\x00')
                    filename = parts[0].decode('utf-8', errors='replace').strip()
                    mode = parts[1].decode('utf-8', errors='replace').lower() if len(parts) > 1 else 'octet'
                    print(f"[TFTP] Menerima permintaan file: '{filename}' dari {client_addr} (mode: {mode})", flush=True)

                    target_path = _resolve_tftp_target(serve_dir, alt_dir, filename)
                    if not target_path or not os.path.isfile(target_path):
                        print(f"[TFTP ERROR] File tidak ditemukan: {filename} (checked {serve_dir}, {alt_dir})", flush=True)
                        err_pkt = struct.pack('!HH', 5, 1) + b"File not found\x00"
                        sock.sendto(err_pkt, client_addr)
                        continue

                    _send_tftp_file(target_path, client_addr)
            except KeyboardInterrupt:
                print("\n[TFTP SERVER] Dihentikan.", flush=True)
                break
            except Exception as e:
                print(f"[TFTP EXCEPTION] {e}", flush=True)

if __name__ == '__main__':
    custom_dir = sys.argv[1] if len(sys.argv) > 1 else None
    run_tftp_server(custom_dir)
