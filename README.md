# PiQuill

A distraction-free e-ink typewriter. A Raspberry Pi Zero 2 W drives a 5.76" e-paper
display and reads a regular USB (or Bluetooth) keyboard. There is no desktop, no
browser and no notifications: you turn it on and type.

The whole program is a small Python application in `src/`.

PiQuill began as a fork of [ZeroWriter](https://github.com/zerowriter/zerowriter1) and has since diverged from it.

## Hardware

- Raspberry Pi Zero 2 W (an original Zero is too slow) and a microSD card
- Good Display **GDEH0576T81** 5.76" e-paper panel (920x680, SSD2677 controller)
- Good Display **DESPI-C02** adapter board (DIP switch P3 set to **2.2**)
- Any USB keyboard, plus a USB OTG adapter or cable for the Pi's data port
- Power: a USB power supply or any power bank, or an 18650 cell with a protected TP4056 USB-C charger module and a 5 V boost converter
- Optional: a MAX17048 fuel gauge (shows the battery percentage) and a Pololu push-button power switch (item 2808) with one two-pole push button for safe on/off; see [docs/WIRING.md](docs/WIRING.md)

The GDEH0576T81 is the only panel this code supports. Pin connections and board
settings are in [docs/WIRING.md](docs/WIRING.md).

## Setup

In short:

1. Flash Raspberry Pi OS Lite (Bookworm) with the Raspberry Pi Imager. Set wifi, a
   hostname and a user, and enable SSH.
2. Enable SPI (`dtparam=spi=on` in `config.txt`, or via `sudo raspi-config`).
3. Install the dependencies:

   ```bash
   sudo apt update
   sudo apt install python3-pip python3-spidev python3-rpi.gpio python3-pil python3-numpy python3-smbus git
   sudo pip3 install keyboard --break-system-packages
   ```

4. Clone this repo and run it from `src/`. It needs root to read the keyboard and
   drive the display:

   ```bash
   cd piquill/src
   sudo python main.py
   ```

5. To start on boot, add this line with `crontab -e`:

   ```
   @reboot cd piquill/src/ && sudo python main.py &
   ```

## Using it

Type as you would on any keyboard. The text area shows the last 14 lines, with the
line you are typing at the bottom. Under it, a status panel shows a page progress
bar, session time, word count, character count and the current file name.

| Keys | Action |
|------|--------|
| Ctrl+S | Save. The first time it asks for a file name (Enter confirms, Esc cancels). Pressing Enter at the end of a line also autosaves a named file. |
| Ctrl+R | Rename the file / save as |
| Ctrl+O | Open a saved file. Up/Down chooses (Shift+Up/Down moves a page), Enter opens, Del then `y` deletes, Esc cancels. Each file shows its date and word count. |
| Ctrl+N | Save the current file and start a new one |
| Ctrl+E | Back up saved files to the cloud (see below) |
| Ctrl+B / Ctrl+I / Ctrl+U | Toggle bold / italic / underline for what you type next |
| Arrow keys | Scroll back through earlier pages |
| Tab | Insert two spaces |
| Ctrl+Esc | Save and power down the Pi |
| Power button | Press to turn on. Press again to shut down (saves first), then power is cut completely |

If a MAX17048 is connected, the bottom right of the status panel shows the battery
percentage (`Bat 87%`, with a `+` while charging). Below 15% it reads `LOW`. At 5% or
less, while not charging, PiQuill saves your work and powers off before the battery
runs out. With no gauge connected, nothing is shown and nothing changes. The
percentage is refreshed with the rest of the panel, not on its own timer.

Your work in progress is cached in `src/data/cache.txt` and restored the next time
the program starts, so nothing is lost if the power is pulled.

## Files

Documents are saved in `src/data/documents/` as Markdown (`.md`). Bold, italic and
underline are stored as `**bold**`, `*italic*` and `<u>underline</u>`, so any other
editor can open them. Style markers are not counted in the word and character totals.

Only one font (Courier Prime) is installed, so bold, italic and underline are drawn
synthetically.

To copy files to another computer, use `scp`/SFTP over SSH, or install Samba on the Pi.

## Cloud backup

Ctrl+E uploads new and changed documents with [rclone](https://rclone.org), so it
works with Google Drive, Dropbox, OneDrive, Nextcloud and more. It is a one-way copy
that never deletes anything. It needs one-time setup over SSH, described in
[docs/CLOUD_BACKUP.md](docs/CLOUD_BACKUP.md).

## Bluetooth keyboards

1. Run `sudo bluetoothctl`
2. Run `agent on`, then `default-agent`
3. Run `scan on` and put the keyboard in pairing mode
4. When it appears, run `scan off`, then `pair AA:BB:CC:DD:EE:FF` with its address
5. Reboot the Pi if the keyboard is not picked up

## Keyboard layout

`src/keymaps.py` maps unshifted keys to their shifted characters. Edit it if your
keyboard produces the wrong symbols.

## Tuning the display

E-paper refreshes slowly, so typing only redraws the bottom strip of the screen and
occasionally does a full refresh to clear ghosting. These environment variables
adjust that when starting `main.py`:

| Variable | Default | Meaning |
|----------|---------|---------|
| `ZW_STRIP_ROWS` | 240 | Rows redrawn while typing. Lower is faster but fainter, 0 redraws the whole screen. |
| `ZW_STRIP_REPEAT` | 1 | Repeat each strip update for more contrast |
| `ZW_FULL_REFRESH_AFTER` | 60 | Partial updates before a full-screen refresh |
| `ZW_TIMING` | off | Set to `1` to print how long each update takes |
| `ZW_BATTERY_WARN` | 15 | Percent at or below which the panel shows `LOW` |
| `ZW_BATTERY_SHUTDOWN` | 5 | Percent at or below which PiQuill saves and powers off (not while charging) |
| `ZW_BATTERY_POLL` | 30 | Seconds between battery readings |
| `ZW_BATTERY_BUS` | 1 | I2C bus number |
| `ZW_CLOUD_REMOTE` | unset | rclone destination for backups, e.g. `gdrive:PiQuill` |
| `ZW_CLOUD_TIMEOUT` | 120 | Seconds before a backup is given up on |

Panel orientation and the SPI clock live in `src/waveshare_epd/epd5in76.py`; see
[docs/WIRING.md](docs/WIRING.md) for details.

## Project layout

```
src/main.py             application: key handling, rendering, files
src/textfmt.py          bold/italic/underline handling
src/cloud.py            rclone backup
src/battery.py          MAX17048 battery gauge
src/keymaps.py          shift key map
src/waveshare_epd/      display driver
docs/                   wiring, cloud backup
```

## Credits and license

Inspired by the [penkesu computer](https://penkesu.computer/). The display driver is
adapted from Good Display's and Waveshare's example code, and remains theirs. The
rest is released under the [license](LICENSE) in this repo.

Use this at your own risk. A wrong wiring or driver setting can damage a display.
