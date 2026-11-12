# PiQuill

An e-ink typewriter running on a Raspberry Pi Zero 2W: a 5.76" Good Display e-paper screen, any USB or Bluetooth keyboard, plain-text Markdown files and optional cloud backup.

PiQuill started as a fork of [ZeroWriter](https://github.com/zerowriter/zerowriter1) and has since diverged from it. Credit to ZeroWriter, and before it to https://penkesu.computer/, for the original idea.

----------

An easy, DIY eink typewriter running on a raspberry pi zero. Perfect for beginners.

This project is open-source. Do whatever you want with it. Please note the display drivers and waveshare code belongs to them. But buy their displays, they rock.

Components list: docs/componentslist

pi zero setup steps: docs/how-to-setup-your-pi

----------
 
The src/waveshare_epd directory is a modified waveshare drivers. All waveshare code belongs to them. Great company, buy their gadgets!

Use this modified code at your own risk. The modified driver may cause damage to your display. Don't blame me.

I have included .STL files (hardware/) for the enclosure I have been using. Feel free to use them however you want.

----------

How it works:

The application is in src/main.py, built on top of the example code from waveshare.

The application itself can be modified to do whatever you want (or just leave it be). The basics:

epd.init() clears the screen using slow look up tables -- this prevents artifacting

epd.init_Partial() runs a faster update using modified LUT. (Ben Krasnow: https://hackaday.com/2017/10/31/ben-krasnow-hacks-e-paper-for-fastest-refresh-rate/) -- important to note this only works with the 4.2" v1 waveshare display.

Use a Pi Zero 2W. Don't use an original Zero. The extra power is very useful.

----------

Setup / Getting Started:

docs/how-to-setup-your-pi

- requires pi zero 2w running bookworm, light install recomend (headless/no GUI)
- set up ssh and configure your pi zero remotely via terminal or powershell
- Drop in the src folder provided in this repo and run main.py from ssh
- Set up crontab (from command line: crontab -e) to boot to main.py

Hardware Features:
- 40% keyboard and an eink display
- tons of storage
- bring-your-own-battery-pack: 10,000mah battery will yield around 25-30 hours of usage, a lot more if you cut networking
- or just plug it into something
- portable! stylish! cool! modified from the https://penkesu.computer/ penkesu computer

Program Features:
- light weight python typewriter
- works with any USB keyboard
- KEYMAPS file to edit key maps if you don't want to program your keyboard's firmware
- files save in the /data directory where the program resides, access via SMB
- autosaves the cache every time return is pressed
- CTRL S saves the cache to a txt file
- CTRL N starts a new file
- CTRL ESC turns unit off.
- (NEW, likely buggy) The arrow keys can be used to navigate through and review previous writing
- You could easily add an output to google drive or etc

===

Have fun! Happy writing...

===

Steps to use a Bluetooth Keyboard

1. Run `sudo bluetoothctl`
2. Run `agent on`
3. Run `default-agent`
4. Run `scan on`
5. Wait until you see your device listed (be sure your keyboard is in pairing mode)
6. Run `scan off`
7. Run `devices` to list known Bluetooth devices
8. Run `pair AA:BB:CC:DD:EE:FF` where `AA:BB:CC:DD:EE:FF` is the MAC address of your bluetooth keyboard
9. You may need to reboot your raspberry pi before python is able to register your bluetooth keyboard.

https://www.youtube.com/watch?v=UEmSsscijKE has a video walkthrough of the above steps as well.


----------

Files and shortcuts (GDEH0576T81 version):

Below the text is a panel with a page progress bar, a divider, then session time (resets on a new or opened file), word count and character count, with the file name underneath.
Files are stored as plain text in `src/data/documents/`.

- Ctrl+S: save (asks for a file name the first time; Enter confirms, Esc cancels). Enter also autosaves a named file.
- Ctrl+R: rename / save as
- Ctrl+O: open a saved file (Up/Down to choose (Shift+Up/Down a page at a time), each file shows its date and word count, Enter to open, Del to delete then y to confirm, Esc to cancel)
- Ctrl+N: save the current file and start a new one
- Ctrl+E: copy your saved files to a cloud service (Google Drive, Dropbox, OneDrive, ...) as a backup. Needs one-time setup, see docs/CLOUD_BACKUP.md. It only uploads new/changed files and never deletes anything in the cloud.
- Ctrl+B / Ctrl+I / Ctrl+U: toggle bold / italic / underline for the text you type next (press again to turn off; an open style carries over to the next line). Saved files are Markdown (.md): **bold**, *italic*, and <u>underline</u>, so other editors can read them. Styles are not counted in word/character totals. Since only one font is installed, bold, italic and underline are drawn synthetically.
- Arrow keys: scroll through pages
- Ctrl+Esc: power down
