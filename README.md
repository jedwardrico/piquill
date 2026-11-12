# ZeroWriter

A distraction-free e-ink typewriter. A Raspberry Pi Zero 2 W drives a 5.76" e-paper
display and reads a regular USB (or Bluetooth) keyboard. There is no desktop, no
browser and no notifications: you turn it on and type.

The whole program is a small Python application in `src/`.

## Hardware

- Raspberry Pi Zero 2 W (an original Zero is too slow) and a microSD card
- Good Display **GDEH0576T81** 5.76" e-paper panel (920x680, SSD2677 controller)
- Good Display **DESPI-C02** adapter board (DIP switch P3 set to **2.2**)
- Any USB keyboard, plus a USB OTG adapter or cable for the Pi's data port
- Power: a USB power supply or any power bank

The GDEH0576T81 is the only panel this code supports. Pin connections and board
settings are in [docs/WIRING.md](docs/WIRING.md). A parts list is in
[docs/componentslist](docs/componentslist).

## Setup

The full walk-through is in [docs/how-to-setup-your-pi](docs/how-to-setup-your-pi).
In short:

1. Flash Raspberry Pi OS Lite (Bookworm) with the Raspberry Pi Imager. Set wifi, a
   hostname and a user, and enable SSH.
2. Enable SPI (`dtparam=spi=on`). [docs/sampleconfig](docs/sampleconfig) is a
   working `config.txt`, including optional overclock settings.
3. Install the dependencies:

   ```bash
   sudo apt update
   sudo apt install python3-pip python3-spidev python3-rpi.gpio python3-pil python3-numpy git
   sudo pip3 install keyboard --break-system-packages
   ```

4. Clone this repo and run it from `src/`. It needs root to read the keyboard and
   drive the display:

   ```bash
   cd zerowriter/src
   sudo python main.py
   ```

5. To start on boot, add this line with `crontab -e`:

   ```
   @reboot cd zerowriter/src/ && sudo python main.py &
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
| Ctrl+Esc | Power down the Pi |

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
| `ZW_CLOUD_REMOTE` | unset | rclone destination for backups, e.g. `gdrive:ZeroWriter` |
| `ZW_CLOUD_TIMEOUT` | 120 | Seconds before a backup is given up on |

Panel orientation and the SPI clock live in `src/waveshare_epd/epd5in76.py`; see
[docs/WIRING.md](docs/WIRING.md) for details.

## Project layout

```
src/main.py             application: key handling, rendering, files
src/textfmt.py          bold/italic/underline handling
src/cloud.py            rclone backup
src/keymaps.py          shift key map
src/waveshare_epd/      display driver
docs/                   wiring, setup, parts, cloud backup
```

## Credits and license

Inspired by the [penkesu computer](https://penkesu.computer/). The display driver is
adapted from Good Display's and Waveshare's example code, and remains theirs. The
rest is released under the [license](LICENSE) in this repo.

Use this at your own risk. A wrong wiring or driver setting can damage a display.
