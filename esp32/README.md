# PiQuill ESP32 (work in progress)

A separate, standalone variant of PiQuill built around an Adafruit **ESP32-S3
Feather (8 MB flash, 2 MB PSRAM)** instead of a Raspberry Pi Zero 2 W. It lives in
this folder only. Nothing in `src/`, `docs/` or the top-level README is used or
changed by it, and the Pi version keeps working as before.

**Status:** display driver written (`src/epd5in76.*`) with a bring-up sketch in
`src/main.cpp`. Nothing has been run on the ESP32 or the panel yet. On a PC, the
driver's command/data stream was compared byte for byte against the Python
driver (full, partial, windowed and post-sleep updates, all four flip settings)
and matched; it has not been compiled with the Arduino-ESP32 toolchain.

## Why a separate device

The Pi version is Python on Linux. None of that carries over to a microcontroller,
so this is a fresh firmware project that happens to drive the same panel. The goals:

- Hours-to-weeks battery life (deep sleep between sessions)
- About a second from power-on to typing
- Fewer parts (the Feather has its own LiPo charger)

## Hardware

- Adafruit ESP32-S3 Feather, 8 MB flash / 2 MB PSRAM
- Good Display GDEH0576T81 panel + DESPI-C02 adapter (DIP switch P3 = 2.2), same as the Pi build
- USB keyboard (USB host) powered from a 5 V boost converter (Pololu U3V40F5 or similar)
- LiPo cell on the Feather's JST connector

Wiring and open questions: [docs/WIRING.md](docs/WIRING.md).

## Firmware plan

- Toolchain: PlatformIO with the Arduino-ESP32 framework (`platformio.ini`).
  CircuitPython is not used because the ESP32-S3 USB host support it needs for a
  keyboard is the main risk, and ESP-IDF/Arduino have working USB host HID.
- Display driver: port of `src/waveshare_epd/epd5in76.py` (itself a port of Good
  Display's Arduino demo) to C++ on the ESP32 SPI peripheral.
- Rendering: 920x680 1-bit framebuffer (~78 KB) in PSRAM or internal RAM, bitmap
  font instead of PIL/TrueType.
- Input: USB host HID boot-keyboard reports, with a US layout first.
- Storage: LittleFS on the 8 MB flash.

Rough order of work:

1. ~~Blink / serial "hello" to confirm the toolchain and board~~ (replaced by the display test)
2. Display driver: init, full refresh, partial refresh (written, needs hardware test)
3. USB host keyboard to serial
4. Text buffer, wrapping and drawing
5. Save/load, deep sleep, battery gauge
6. Optional: Wi-Fi sync, BLE keyboards

## Not planned (for now)

Classic Bluetooth keyboards (the S3 is BLE only), the cloud backup, and
Markdown-style formatting from `textfmt.py`.
