# Wiring: Raspberry Pi Zero 2 W -> DESPI-C02 -> GDEH0576T81

Sources: GDEH0576T81 datasheet (FPC pin table, section 1.5) and DESPI-C02
datasheet (header P2 labels, DIP switch P3). Pi pins come from `epdconfig.py`
(BCM numbering).

## DESPI-C02 header P2 (8 pins, top to bottom as printed on the board)

```
 DESPI-C02 P2           Raspberry Pi (40-pin header)
 ------------           ----------------------------
 BUSY   ------------->  Pin 18  GPIO24            (display -> Pi)
 RES    <-------------  Pin 11  GPIO17
 D/C    <-------------  Pin 22  GPIO25
 CS     <-------------  Pin 24  GPIO8  (SPI0 CE0)
 SCK    <-------------  Pin 23  GPIO11 (SPI0 SCLK)
 SDI    <-------------  Pin 19  GPIO10 (SPI0 MOSI)
 GND    -------------   Pin 6   GND
 3.3V   <-------------  Pin 1   3V3
```

| DESPI-C02 pin | Pi BCM | Pi physical pin | Direction |
|---------------|--------|-----------------|-----------|
| BUSY          | GPIO24 | 18              | display -> Pi |
| RES           | GPIO17 | 11              | Pi -> display |
| D/C           | GPIO25 | 22              | Pi -> display |
| CS            | GPIO8 (CE0) | 24         | Pi -> display |
| SCK           | GPIO11 (SCLK) | 23       | Pi -> display |
| SDI           | GPIO10 (MOSI) | 19       | Pi -> display |
| GND           | -      | 6 (or 9, 14, 20, 25) | ground |
| 3.3V          | -      | 1 (or 17)       | power |

## Power button (safe on and off)

A momentary push button on **GPIO3** (pin 5) turns the Pi on and shuts it down cleanly.
GPIO3 is the only pin that can wake a halted Pi, so the button must go there.

```
 push button            Raspberry Pi (40-pin header)
 -----------            ----------------------------
 one side   -------->   Pin 5   GPIO3
 other side -------->   Pin 14  GND  (any GND pin)
```

Add this line to `config.txt`:

```
dtoverlay=gpio-shutdown
```

- Press while running: the Pi shuts down. PiQuill saves your work, shows
  "PiQuill Powered Down." and the Pi halts.
- Press while halted: the Pi boots.
- GPIO3 already has a pull-up on the Pi, so the button needs no resistor.
- GPIO3 is also the hardware I2C clock pin. Do not enable `dtparam=i2c_arm=on`; the
  battery gauge uses a software I2C bus on other pins instead (next section).
- A halted Pi still draws a small standby current from the battery, so it will drain
  slowly. For long storage, unplug the battery or add a hard switch in the battery lead.
  Use a protected LiPo cell, because the Pi cannot cut its own power.

## Battery gauge (MAX17048)

The gauge uses a software ("bit-banged") I2C bus on GPIO5 and GPIO6, because the
hardware I2C pins would clash with the power button. The display uses SPI, so no pins
are shared. Add this line to `config.txt` and install the library with
`sudo apt install python3-smbus`:

```
dtoverlay=i2c-gpio,bus=3,i2c_gpio_sda=5,i2c_gpio_scl=6
```

```
 MAX17048 breakout      Raspberry Pi (40-pin header)
 -----------------      ----------------------------
 SDA   <------------->  Pin 29  GPIO5
 SCL   <------------->  Pin 31  GPIO6
 GND   -------------    Pin 30  GND
 VIN   <-------------   Pin 17  3V3
 BAT   <-------------   battery + (the cell, not the Pi's 5V rail)
```

Pin labels differ between breakouts, so check yours. The bus needs pull-up resistors
to 3.3V; most breakouts include them. The gauge's I2C address is 0x36. Check that the
Pi sees it with `sudo apt install i2c-tools` then `i2cdetect -y 3`.

These instructions assume a single-cell LiPo with a separate charger/boost board.
They are untested on hardware; check polarity and your boards' pinouts before
connecting a battery.

## Board settings

- **DIP switch P3 must be set to 2.2** (the SSD series position). The DESPI-C02
  datasheet says to use 2.2 for SSD (Solomon Systech) driver ICs and 0.47 only
  for UC series. The GDEH0576T81 uses an SSD2677. A wrong setting can stop the
  display refreshing.
- Insert the panel FPC into connector P1 with the display surface facing up,
  then press down the black FPC latch.
- Power from 3.3V only. Do not use 5V.

## Notes

- `epdconfig.py` also toggles GPIO18 (PWR_PIN). The DESPI-C02 has no such pin; leave GPIO18 unconnected.
- Enable SPI (`dtparam=spi=on` in config.txt, see `sampleconfig`).
- Panel BS pin (FPC pin 8): Low = 4-line SPI, High = 3-line (9-bit). The DESPI-C02 header has no BS pin and its datasheet doesn't say how the board sets it. This code needs 4-line SPI (separate D/C). If the display never responds, check the BS setting on the board.
- CS is active low. D/C high = data, low = command. RES is active low.
- BUSY is active low (BUSY_N): the pin reads 0 while the panel is busy and 1 when ready. This matches the SSD2677 datasheet and Good Display's demo code (`lcd_chkstatus` waits for 1). The GDEH0576T81 datasheet note saying "busy = high" is wrong or misleading.
- Driver: `src/waveshare_epd/epd5in76.py`, ported from Good Display's Arduino demo. `main.py` uses it.

## Software notes (GDEH0576T81)

Driver: `src/waveshare_epd/epd5in76.py`.

- Orientation: the demo's scan direction shows the image flipped top to bottom, so the driver flips it back (`flip_y = True`). If your panel is mounted differently, toggle `flip_x` / `flip_y` in the driver.
- Speed: typing redraws only the bottom 240 rows (about 310 ms per update). The whole screen takes about 800 ms. A strip of fewer rows is faster but dimmer; 120 rows is unreadable. Tune with environment variables when starting `main.py`:
  - `ZW_STRIP_ROWS` (default 240, minimum 120, 0 = always update the whole screen)
  - `ZW_STRIP_REPEAT` (default 1; resends each strip update for more contrast)
  - `ZW_FULL_REFRESH_AFTER` (default 60; full-screen refresh after this many partial updates, to clear ghosting)
  - `ZW_TIMING=1` prints the time each update takes.
- SPI clock is 10 MHz (`spi_speed_hz` in the driver). The datasheet leaves the maximum as TBD; lower it if the screen shows garbage.
- Install `python3-numpy` (frame conversion is about 6x faster with it).
- Forcing the waveform temperature in partial mode made refreshes slower (645 ms unforced vs 900+ ms), so the chip's own sensor is used.
