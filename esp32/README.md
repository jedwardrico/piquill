# PiQuill ESP32 (work in progress)

A separate, standalone variant of PiQuill built around an Unexpected Maker
**FeatherS3[D] (ESP32-S3 Feather)** instead of a Raspberry Pi Zero 2 W. It lives in
this folder only. Nothing in `src/`, `docs/` or the top-level README is used or
changed by it, and the Pi version keeps working as before.

**Status:** feature-complete on paper, untested on hardware. The firmware has the
same features as the Pi version (typing, bold/italic/underline, page scrolling,
status panel with battery, save / rename / open / new / delete), except cloud
backup. Everything that does not touch hardware is checked on a PC against the
Python code (see Tests). Nothing has been built with PlatformIO or run on the
ESP32, the panel or the keyboard yet.

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
2. Display driver: init, full refresh, partial refresh (`epd5in76`; needs hardware test)
3. BLE keyboard client (`ble_keyboard`, `hid_keys`; the BLE part has never been
   compiled against the real library or run with a keyboard)
4. Text, formatting and drawing (`text_buffer`, `textfmt`, `gfx`, bitmap fonts from
   `tools/gen_font.py`)
5. Editor, saving and loading (`editor`, `doc_store`, `storage_littlefs` on the
   flash), status panel (`ui`) and battery gauge (`battery`)
6. Optional: Wi-Fi sync, cloud backup

## Keys

Same as the Pi version. Typing, Enter (ends a line and saves), Backspace;
**Up/Left** scroll back a page, **Down/Right** forward (the Pi's arrow keys scroll
pages, they do not move a cursor); **Ctrl+B/I/U** bold, italic, underline;
**Ctrl+S** save (asks for a name the first time), **Ctrl+R** rename, **Ctrl+O** open
(Up/Down choose, Shift = a page, Enter open, Delete then y/n, Esc cancel),
**Ctrl+N** new file, **Ctrl+Esc** power down (save, show a message, deep sleep;
press RESET to start again). Documents are Markdown files in `/documents` on the
board's flash. The board has no clock, so a file's date is only shown once the
time has been set (nothing sets it yet); "most recent first" uses `/recent.txt`.

## Tests

`test/run_tests.sh` runs on a PC (needs g++, python3 and Pillow; no ESP32). Where
the Python code from `src/` exists, the C++ is compared against it directly:

- text buffer wrapping, including style markers, over thousands of random keystrokes
- `textfmt` and Markdown conversion on thousands of random strings
- file-name cleaning, and the battery gauge arithmetic (including its rounding)

The editor (save, rename, open, new, delete, prompts, page scrolling) is checked with
scripted sessions on a real folder, and the HID key decoder (shift, caps lock,
rollover, key repeat) with hand-written cases. `test/render_demo.cpp` draws sample
screens to image files for a visual check. Not covered, because it needs the
hardware: the display driver on the panel, BLE, LittleFS, and the I2C gauge.

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

Classic Bluetooth keyboards (the S3 is BLE only) and the Pi's cloud backup
(Ctrl+E). Setting the clock (so file dates show) would need Wi-Fi time sync.
