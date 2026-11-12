# Wiring: ESP32-S3 Feather -> DESPI-C02 -> GDEH0576T81 (draft, untested)

The panel side is identical to the Pi build; see `../../docs/WIRING.md` for the
DESPI-C02 header and the board settings (P3 = 2.2, power from 3.3 V only, BUSY is
active low). Only the controller side changes.

**The Feather pin names below are a proposal. Check them against the Adafruit
pinout for your exact board before wiring.**

| DESPI-C02 pin | Feather pin (proposed) | Direction |
|---------------|------------------------|-----------|
| SCK           | SCK                    | Feather -> display |
| SDI           | MOSI                   | Feather -> display |
| CS            | D10                    | Feather -> display |
| D/C           | D9                     | Feather -> display |
| RES           | D6                     | Feather -> display |
| BUSY          | D5                     | display -> Feather |
| GND           | GND                    | ground |
| 3.3V          | 3V                     | power |

## Keyboard power and data

The Feather does not put 5 V on any pin when running from the battery, so a USB
keyboard needs its own 5 V:

```
LiPo -> Feather BAT --> boost converter IN
                        boost converter OUT (5 V) --> keyboard VBUS
Feather GND ----------> boost GND ----------------> keyboard GND
Feather D+/D- (GPIO20/GPIO19) -----------------> keyboard D+/D-
```

- Drive the boost converter's SHDN/enable pin from a spare GPIO (proposed: D11) so
  it is off during deep sleep.
- Check the boost converter's minimum input voltage against the LiPo cutoff.

## Open questions

- Are USB D+/D- (GPIO20/GPIO19) reachable on the Feather headers, or only on the
  USB-C connector? If only the connector, solder to its pads or use a USB-C
  breakout. The same port is used for programming, so plan how to flash.
- Does this Feather revision have a MAX17048 or LC709203F fuel gauge? Both are I2C
  (STEMMA QT / SDA, SCL).
- Does the panel work with a 3.3 V logic ESP32 at the SPI clock the Pi build uses
  (10 MHz)? The SSD2677 datasheet leaves the maximum clock as TBD.
