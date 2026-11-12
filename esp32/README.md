# PiQuill ESP32 (work in progress)

A separate, standalone variant of PiQuill built around an Unexpected Maker
**FeatherS3[D] (ESP32-S3 Feather)** instead of a Raspberry Pi Zero 2 W. It lives in
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
- Fewer parts (the Feather has its own LiPo charger and fuel gauge)

## Hardware

- Unexpected Maker FeatherS3[D] (16 MB flash, 8 MB PSRAM)
- Good Display GDEH0576T81 panel + DESPI-C02 adapter (DIP switch P3 = 2.2), same as the Pi build
- Keychron B1 Pro keyboard over Bluetooth LE (seen in a BLE scan; USB host with a
  5 V boost converter is the fallback)
- LiPo cell on the Feather's JST connector

Wiring and open questions: [docs/WIRING.md](docs/WIRING.md).

## Firmware plan

- Toolchain: PlatformIO with the Arduino-ESP32 framework on the pioarduino
  platform (`platformio.ini`).
  CircuitPython is not used: the display driver and the BLE/USB keyboard code
  are more practical in C++, and Arduino-ESP32 has NimBLE and USB host support.
- Display driver: port of `src/waveshare_epd/epd5in76.py` (itself a port of Good
  Display's Arduino demo) to C++ on the ESP32 SPI peripheral.
- Rendering: 920x680 1-bit framebuffer (~78 KB) in PSRAM or internal RAM, bitmap
  font instead of PIL/TrueType.
- Input: BLE HID keyboard (NimBLE client, bonded once), with a US layout first.
  USB host HID is the fallback.
- Storage: LittleFS on the 8 MB flash.

Rough order of work:

1. ~~Blink / serial "hello" to confirm the toolchain and board~~ (replaced by the display test)
2. Display driver: init, full refresh, partial refresh (written, needs hardware test)
3. BLE keyboard client (written: `ble_keyboard`, `hid_keys`; the BLE part has never
   been compiled against the real library or run with a keyboard)
4. Text buffer, wrapping and drawing (written: `text_buffer`, `text_render`, bitmap
   font from `tools/gen_font.py`; bold/italic and the status panel not ported yet)
5. Save/load, deep sleep, battery gauge
6. Optional: Wi-Fi sync

## Tests

`test/run_tests.sh` runs on a PC (needs g++ and python3, no ESP32): it checks that
the text buffer wraps lines exactly like the Pi's typing logic, and that the HID
key decoder handles shift, caps lock, rollover and key repeat correctly. The BLE
connection code itself can only be tested with the board and keyboard.

## Building

```
pio run -e display_test -t upload   # test pattern, no BLE needed
pio run -e typing -t upload         # BLE keyboard -> text on the panel
```

First flash: hold BOOT, tap RESET, release BOOT, then upload. Watch the serial
monitor (`pio device monitor`): the typing app prints each BLE step, including the
HID characteristics it finds and any report sizes it ignores. Put the keyboard in
pairing mode the first time; after that the bond is remembered.

## Not planned (for now)

Classic Bluetooth keyboards (the S3 is BLE only), the cloud backup, and
Markdown-style formatting from `textfmt.py`.
