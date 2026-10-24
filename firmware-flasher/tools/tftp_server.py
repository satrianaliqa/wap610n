#!/usr/bin/env python3
"""
WAP610N Standalone TFTP Server (Divisi Flasher)
Serves firmware binary files over UDP Port 69 for fast U-Boot network booting/flashing.
"""
import socket
import struct
import os
import sys

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PORT = 69
DEFAULT_SERVE_DIR = os.path.join(BASE_DIR, 'firmware')

def run_tftp_server(serve_dir=None):
    if serve_dir is None:
        serve_dir = DEFAULT_SERVE_DIR
    if not os.path.exists(serve_dir):
        # Fallback to the repository build output if the bundled firmware is absent.
        alt_dir = os.path.abspath(os.path.join(BASE_DIR, '..', 'output'))
        if os.path.exists(alt_dir):
            serve_dir = alt_dir

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        sock.bind(('0.0.0.0', PORT))
    except Exception as e:
        print(f"[ERROR] Gagal bind ke port {PORT}: {e}")
        print("Pastikan script dijalankan dengan sudo atau port 69 tidak sedang dipakai.")
        sys.exit(1)

    print(f"[TFTP SERVER] Berjalan di port {PORT}, melayani folder: {serve_dir}", flush=True)

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

                target_path = os.path.join(serve_dir, os.path.basename(filename))
                if not os.path.exists(target_path):
                    if filename.lower() in ['bootpimage', 'wap610n_v1.0.05.bin']:
                        target_path = os.path.join(serve_dir, 'WAP610N_v1.0.05.bin')
                        if not os.path.exists(target_path):
                            target_path = os.path.join(serve_dir, 'bootpImage')

                if not os.path.exists(target_path):
                    print(f"[TFTP ERROR] File tidak ditemukan: {target_path}", flush=True)
                    err_pkt = struct.pack('!HH', 5, 1) + b"File not found\x00"
                    sock.sendto(err_pkt, client_addr)
                    continue

                file_size = os.path.getsize(target_path)
                print(f"[TFTP] Mengirim file: {target_path} (Ukuran: {file_size} bytes)...", flush=True)

                transfer_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                transfer_sock.settimeout(3.0)

                with open(target_path, 'rb') as f:
                    block_num = 1
                    while True:
                        block_data = f.read(512)
                        data_pkt = struct.pack('!HH', 3, block_num) + block_data

                        for retry in range(5):
                            transfer_sock.sendto(data_pkt, client_addr)
                            try:
                                ack_pkt, ack_addr = transfer_sock.recvfrom(516)
                                if len(ack_pkt) >= 4:
                                    ack_opcode, ack_block = struct.unpack('!HH', ack_pkt[:4])
                                    if ack_opcode == 4 and ack_block == block_num:
                                        break
                            except socket.timeout:
                                pass
                        else:
                            print(f"[TFTP ERROR] Gagal mengirim blok {block_num} setelah 5 percobaan.", flush=True)
                            break

                        if block_num % 500 == 0:
                            print(f"[TFTP] Terkirim blok {block_num} ({block_num * 512} bytes)...", flush=True)

                        if len(block_data) < 512:
                            print(f"[TFTP SUCCESS] Pengiriman file selesai! Total blok: {block_num}", flush=True)
                            break

                        block_num = (block_num + 1) & 0xFFFF

                transfer_sock.close()
        except Exception as e:
            print(f"[TFTP EXCEPTION] {e}", flush=True)

if __name__ == '__main__':
    custom_dir = sys.argv[1] if len(sys.argv) > 1 else None
    run_tftp_server(custom_dir)
