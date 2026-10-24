#!/usr/bin/env python3
"""
WAP610N Interactive U-Boot & Linux Serial Terminal (Divisi Flasher)
Includes automatic autoboot interrupt and interactive bidirectional terminal.
"""
import serial
import sys
import os
import time
import threading

DEFAULT_PORT = '/dev/ttyUSB0'
DEFAULT_BAUD = 38400

def run_terminal(port=DEFAULT_PORT, baud=DEFAULT_BAUD):
    print(f"[*] Menghubungkan ke port {port} ({baud} baud)...")
    try:
        ser = serial.Serial(port, baud, timeout=0.1)
    except Exception as e:
        print(f"[ERROR] Gagal membuka port {port}: {e}")
        sys.exit(1)

    print("\n=======================================================")
    print("      WAP610N SERIAL TERMINAL (DIVISI FLASHER)         ")
    print("=======================================================")
    print("Tekan Ctrl+C untuk keluar dari terminal.")
    print("Menunggu data serial... (Nyalakan / restart router)")
    print("=======================================================\n")

    running = True

    def serial_reader():
        while running:
            try:
                if ser.in_waiting > 0:
                    data = ser.read(ser.in_waiting)
                    text = data.decode('latin1', errors='replace')
                    sys.stdout.write(text)
                    sys.stdout.flush()
                else:
                    time.sleep(0.02)
            except Exception:
                break

    t = threading.Thread(target=serial_reader, daemon=True)
    t.start()

    try:
        while True:
            line = sys.stdin.readline()
            if not line:
                break
            if line.endswith('\r\n'):
                data_to_send = line
            elif line.endswith('\n'):
                data_to_send = line[:-1] + '\r\n'
            else:
                data_to_send = line + '\r\n'
            ser.write(data_to_send.encode('latin1'))
    except KeyboardInterrupt:
        print("\n[*] Menutup terminal serial...")
    finally:
        running = False
        ser.close()

if __name__ == '__main__':
    port = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_PORT
    baud = int(sys.argv[2]) if len(sys.argv) > 2 else DEFAULT_BAUD
    run_terminal(port, baud)
