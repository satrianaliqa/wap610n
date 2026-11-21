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
import argparse

try:
    import termios
    import tty
except ImportError:
    termios = None
    tty = None

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
    print("Tekan Ctrl+] (atau EOF) untuk keluar dari terminal (Ctrl+C diteruskan ke U-Boot/Shell).")
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

    fd = sys.stdin.fileno() if hasattr(sys.stdin, 'fileno') else None
    old_settings = None
    is_tty = (fd is not None and os.isatty(fd) and termios is not None and tty is not None)

    try:
        if is_tty:
            old_settings = termios.tcgetattr(fd)
            tty.setraw(fd)
            while True:
                ch = os.read(fd, 1)
                if not ch:
                    break
                if ch == b'\x1d':  # Ctrl+]
                    break
                if ch == b'\n':
                    ser.write(b'\r')
                else:
                    ser.write(ch)
        else:
            while True:
                line = sys.stdin.readline()
                if not line:
                    break
                data_to_send = line.rstrip('\r\n') + '\r'
                ser.write(data_to_send.encode('latin1'))
    except KeyboardInterrupt:
        print("\n[*] Menutup terminal serial...")
    finally:
        if old_settings is not None and fd is not None:
            try:
                termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
            except Exception:
                pass
        running = False
        t.join(timeout=1.0)
        ser.close()
        print("\n[*] Terminal serial ditutup.")

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='WAP610N Interactive Serial Terminal')
    parser.add_argument('pos_port', nargs='?', help='Serial port (optional positional)')
    parser.add_argument('pos_baud', nargs='?', type=int, help='Baud rate (optional positional)')
    parser.add_argument('--port', '-p', default=None, help='Serial port device path')
    parser.add_argument('--baud', '-b', type=int, default=None, help='Serial baud rate')
    args = parser.parse_args()
    port = args.port or args.pos_port or DEFAULT_PORT
    baud = args.baud or args.pos_baud or DEFAULT_BAUD
    run_terminal(port, baud)
