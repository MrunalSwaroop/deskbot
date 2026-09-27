#!/usr/bin/env python3
"""Minimal USB serial console for the Rocky-Wall-E POC.

Install once: python3 -m pip install pyserial
Run:          python3 poc_serial_console.py /dev/ttyACM0
Windows:      python poc_serial_console.py COM5
"""

import sys
import time

try:
    import serial
except ImportError:
    raise SystemExit("Install pyserial first: python3 -m pip install pyserial")

if len(sys.argv) != 2:
    raise SystemExit("Usage: poc_serial_console.py PORT")

port = sys.argv[1]
with serial.Serial(port, 115200, timeout=0.15) as device:
    time.sleep(2)
    print("Connected. Type commands such as EYE happy, SERVO pan 70, MOVE forward, STOP.")
    print("Type HELP for the firmware command list; Ctrl-C exits.")
    try:
        while True:
            incoming = device.readline().decode(errors="replace").strip()
            if incoming:
                print(f"< {incoming}")
            command = input("> ").strip()
            if command:
                device.write((command + "\\n").encode())
    except (KeyboardInterrupt, EOFError):
        print("\\nExiting.")
