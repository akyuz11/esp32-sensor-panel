"""Karttan seri port uzerinden ekran goruntusu alir (UI_DEBUG_SCREENSHOT=1 gerekir).

Kullanim:
  python tools/screenshot.py COM4 cikti_klasoru  "TILE 0" SHOT:durum "TILE 1" SHOT:veri ...

Her arguman karta gonderilen bir komuttur. "SHOT:<ad>" bir goruntu alir ve
<cikti_klasoru>/<ad>.png olarak kaydeder. Diger komutlar (TILE n, TAP x y, BACK)
oldugu gibi gonderilir.
"""
import os
import sys
import time

import serial
from PIL import Image


def read_until(ser, token, timeout):
    buf = b""
    end = time.time() + timeout
    while time.time() < end:
        buf += ser.read(ser.in_waiting or 1)
        idx = buf.find(token)
        if idx >= 0:
            return buf[idx:]
    raise TimeoutError(f"{token!r} gelmedi")


def take_shot(ser, path, tries=3):
    for attempt in range(tries):
        ser.reset_input_buffer()
        ser.write(b"SHOT\n")
        data = read_until(ser, b"SHOT ", 10)
        while b"\n" not in data:
            data += ser.read(1)
        header, rest = data.split(b"\n", 1)
        _, w, h, checksum = header.decode().split()
        w, h, checksum = int(w), int(h), int(checksum)
        need = w * h * 2
        end = time.time() + 20
        while len(rest) < need and time.time() < end:
            rest += ser.read(need - len(rest))
        pixels = rest[:need]
        if len(pixels) == need and sum(pixels) == checksum:
            img = Image.frombytes("RGB", (w, h), _rgb565_to_rgb888(pixels))
            img.save(path)
            print(f"kaydedildi: {path} ({w}x{h})")
            return
        print(f"bozuk veri, tekrar ({attempt + 1}/{tries})")
    raise RuntimeError("ekran goruntusu alinamadi")


def _rgb565_to_rgb888(data):
    out = bytearray(len(data) // 2 * 3)
    j = 0
    for i in range(0, len(data), 2):
        v = data[i] | (data[i + 1] << 8)
        r = (v >> 11) & 0x1F
        g = (v >> 5) & 0x3F
        b = v & 0x1F
        out[j] = (r << 3) | (r >> 2)
        out[j + 1] = (g << 2) | (g >> 4)
        out[j + 2] = (b << 3) | (b >> 2)
        j += 3
    return bytes(out)


def main():
    port, out_dir, commands = sys.argv[1], sys.argv[2], sys.argv[3:]
    os.makedirs(out_dir, exist_ok=True)
    with serial.Serial(port, 115200, timeout=0.2) as ser:
        time.sleep(0.3)
        for cmd in commands:
            if cmd.startswith("SHOT:"):
                take_shot(ser, os.path.join(out_dir, cmd[5:] + ".png"))
            else:
                ser.write((cmd + "\n").encode())
                time.sleep(0.6)  # gezinme / animasyon bitsin


if __name__ == "__main__":
    main()
