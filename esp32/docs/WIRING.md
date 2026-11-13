# Wiring: Unexpected Maker FeatherS3 -> DESPI-C02 -> GDEH0576T81 (draft, untested)

Board: Unexpected Maker **FeatherS3[D]** (16 MB flash, 8 MB PSRAM). Pins are
taken from its pinout sheet.

The panel side is identical to the Pi build; see `../../docs/WIRING.md` for the
DESPI-C02 header and board settings (P3 = 2.2, power from 3.3 V only, BUSY is
active low).

## Pin names

The number printed on the board by each header pin is the GPIO number. The
`D5`, `D10`, `A0` style names on the pinout sheet are Arduino digital/analog
names and are **not** GPIO numbers (for example `D10` is GPIO3). `src/config.h`
uses raw GPIO numbers.

## Display

| DESPI-C02 pin | Feather header | GPIO | Direction |
|---------------|----------------|------|-----------|
| SCK           | SCK            | 36   | Feather -> display |
| SDI           | MO             | 35   | Feather -> display |
| CS            | 1              | 1    | Feather -> display |
| D/C           | 7              | 7    | Feather -> display |
| RES           | 10             | 10   | Feather -> display |
| BUSY          | 11             | 11   | display -> Feather |
| GND           | GND            | -    | ground |
| 3.3V          | 3V3            | -    | power |

MI (GPIO37) is not connected; the panel is write-only.

Avoided on purpose: GPIO0 and GPIO3 (strapping pins), GPIO19/20 (USB), GPIO43/44
(UART0, kept for debug), GPIO8/9 (battery fuel gauge), and the pins used by
on-board parts (39 LDO2 enable, 40 RGB LED, 41 antenna select, 13 LED,
4 light sensor, 34 5 V detect).

Option: power the display from the board's second regulator (header pin
"3V3" labelled LDO2, output only) and set GPIO39 high before using it. LDO2
shuts off automatically in deep sleep, so the panel draws nothing then. Untested:
make sure the signal pins are not left driving an unpowered panel in sleep.

## Keyboard

**Plan: Bluetooth LE.** The Keychron B1 Pro should pair over BLE, which the
S3 supports, so there is no keyboard wiring at all: no boost converter, no USB
data wires, and the USB-C port stays free for flashing and serial.

**Confirmed:** the B1 Pro shows up in nRF Connect in pairing mode, so it
advertises over BLE. Still to check on the device: that it exposes the "Human
Interface Device" service (0x1812) and which address type and pairing method it
uses. The S3 has no Bluetooth Classic, so a Classic-only keyboard would not work.

Notes for the BLE route:

- Pairing is done once and the bond is stored in flash.
- Reconnecting after sleep takes a second or two, and a keypress cannot wake the
  board from deep sleep, so add a wake button on a spare GPIO.

### Fallback: USB keyboard (host mode)

Only needed if BLE does not work out. The Feather puts no 5 V on any pin when
running from the battery, so a USB keyboard needs its own 5 V:

```
LiPo -> Feather BAT pin --> boost converter IN
                            boost converter OUT (5 V) --> keyboard VBUS
Feather GND ------------->  boost GND -----------------> keyboard GND
Feather GPIO6  --------->   boost enable (SHDN)
```

- Do **not** connect the boost output to the Feather's 5V pin. That pin is for
  USB power in/out and the board only guarantees up to 5 V in.
- The boost enable on GPIO6 lets you switch it off in deep sleep. Check the
  boost converter's minimum input against the LiPo cutoff.
- Add a 100 uF or larger capacitor across VBUS and GND at the keyboard connector.
- Use a USB Type-A female breakout for the keyboard connector.

USB data lines: **GPIO19 (D-) and GPIO20 (D+) are not on the headers.** They
only reach the USB-C connector. Options, from least to most work:

1. Solder fine wires to the D+/D- pins of the USB-C connector (small, 0.5 mm
   pitch, easy to bridge).
2. Look for test points on the PCB (schematic and 3D model are in Unexpected
   Maker's GitHub repo).
3. Use a USB-C OTG adapter and a power source for the keyboard. Whether this
   works depends on how the board presents itself on the USB-C CC pins, which I
   have not checked.

The USB-C connector is also the programming/serial port, so while a keyboard is
using it you cannot use it for flashing or serial output.

## Power button and keyboard switch

One momentary button does all power tasks: press = wake, hold about 2 s = power
down. Diagrams: `wiring-button.svg` (keyboard wired straight to the pack, always
powered) and `wiring-button-keyboard-switch.svg` (keyboard switched off in sleep).

- Button: GPIO `POWER_BUTTON_PIN` (placeholder 5, must be RTC-capable, 0-21) to GND,
  with a 10 k pull-up to 3V3. No capacitor: debounced in software
  (`power_button.h`, tested in `test/power_button_test.cpp`).
- Keyboard (cell removed): BAT+ pad to the pack's positive. BAT- pad to the drain of a
  logic-level N-FET (AO3400, IRLML6344, Si2302), source to GND, gate to
  `KEYBOARD_EN_PIN` (GPIO6) with a 100 k gate-to-GND pull-down. GPIO6 is driven low and
  held through deep sleep. The keyboard's ground floats when off, so give it no other
  wires to the Feather.
- Wake: ext0 (active low). After a wake the button must stay down 300 ms, otherwise
  the board goes back to sleep. Not compiled or tried on hardware yet.

## Flashing

- First flash: hold BOOT, tap RESET, release BOOT (download mode), then upload.
- After flashing, the guide says a silicon bug means you may have to press RESET
  yourself for the new code to start.
- The board ships with CircuitPython and the UF2 bootloader. Flashing Arduino
  code replaces them.

## Battery

- There is a JST PH LiPo connector, a charger, and a MAX17048 fuel gauge on I2C
  (SDA GPIO8, SCL GPIO9, which are also the header pins labelled SDA/SCL).
  Use that for the battery percentage instead of an external gauge.
- GPIO34 reads whether 5 V is present on USB.

## Open questions

- Does the panel work at 10 MHz SPI from this board? The SSD2677 datasheet leaves
  the maximum clock as TBD.
