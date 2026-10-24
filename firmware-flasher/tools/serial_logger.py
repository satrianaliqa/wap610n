#!/usr/bin/env python3
import serial
import sys
import time
import os

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PORT = '/dev/ttyUSB0'
BAUDRATE = 38400
LOG_FILE = os.path.join(BASE_DIR, 'logs', 'router_serial_log.txt')

os.makedirs(os.path.dirname(LOG_FILE), exist_ok=True)

print(f"[SERIAL] Membuka port {PORT} pada baudrate {BAUDRATE}...", flush=True)

try:
    ser = serial.Serial(
        port=PORT,
        baudrate=BAUDRATE,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.1
    )
except Exception as e:
    print(f"[ERROR] Gagal membuka port: {e}", flush=True)
    sys.exit(1)

print(f"\n=======================================================", flush=True)
print(f"  [MONITOR] SERIAL LISTENER AKTIF (38400 baud, 8N1)   ", flush=True)
print(f"  >>> SILAKAN COLOKKAN ADAPTOR POWER ROUTER SEKARANG! <<<", flush=True)
print(f"=======================================================\n", flush=True)
print(f"[SERIAL] Log disimpan di: {LOG_FILE}\n", flush=True)

with open(LOG_FILE, 'w', encoding='utf-8', buffering=1) as f:
    f.write(f"=== Serial Log Started at {time.strftime('%Y-%m-%d %H:%M:%S')} (Baud: {BAUDRATE}) ===\n")
    try:
        while True:
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                text = data.decode('latin1', errors='replace')
                sys.stdout.write(text)
                sys.stdout.flush()
                f.write(text)
                f.flush()
            else:
                time.sleep(0.01)
    except KeyboardInterrupt:
        print("\n[SERIAL] Monitoring dihentikan oleh user.", flush=True)
    finally:
        ser.close()
