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

## Power button (one button, safe on and off)

Power is switched by a [Pololu Mini Pushbutton Power Switch with Reverse Voltage
Protection, LV](https://www.pololu.com/product/2808) (item 2808; 2.2-20 V, about 3 A
continuous, 0.01 uA when off) and one **two-pole momentary push button**. Because
the board cuts power itself, the Pi can be fully off with no standby drain.

How it works:

- **Press to turn on.** The button grounds the board's A pin ("on-only" wiring), so
  the board latches on and the Pi boots. Pressing while running does not cut power.
- **Press to turn off.** The button's second pole pulls a Pi GPIO low. The Pi shuts
  down cleanly (PiQuill saves and shows "PiQuill Powered Down."). As the very last
  step of the shutdown, the Pi raises its OFF pin and the board cuts power.

```
 USB-C --> TP4056 module --B+/B--> 18650 cell
              OUT+  ---------------> Pololu VIN
                                     Pololu VOUT --> 5V boost converter IN+
                                                     boost OUT+ --> Pi 5V (pin 2)
 all grounds common  (TP4056 OUT-, Pololu GND, boost GND, Pi GND)

 Pololu OFF  <-------------- Pi pin 37  GPIO26   (gpio-poweroff)
 Pololu A    -- button pole 1 -- GND              (turn on)
 Pi pin 13 GPIO27 -- button pole 2 -- GND         (shut down)
 Pololu B, ON, CTRL: leave unconnected
```

Add these lines to `config.txt`:

```
dtoverlay=gpio-shutdown,gpio_pin=27
dtoverlay=gpio-poweroff,gpiopin=26
```

- Use a **two-pole** momentary button (for example DPST or DPDT momentary). The
  second pole keeps the Pi's 3.3 V GPIO away from the Pololu's A pin, whose voltage
  the Pololu documentation does not state.
- Do not drive CTRL: it must not be high at the same time as OFF.
- The board latches itself, so the Pi does not need to "hold" power while it boots.
- A press does not need to be long. Pololu warns that very bouncy buttons may not
  work well, so use a decent tactile button.
- The Pololu's own tactile button connects A to B, which is plain push-on/push-off.
  Pressing it while running cuts power at once. Leave it for emergencies if the Pi
  hangs, but remember it does not shut down cleanly.
- If the Pi hangs and does not shut down, the button cannot turn it off. Use the
  board's own button, or disconnect the battery.
- The switch sits between the TP4056's protected output (OUT+/OUT-) and the boost
  converter. The charger is upstream of it, so the cell charges from USB-C even when
  the device is off, and with the switch off nothing downstream draws current.
  Use the **protected** TP4056 version (six pads: IN, OUT and B). Its OUT pads go
  through the cell protection; the B pads connect to the cell.
- The TP4056 is only a charger. It has no 5 V output, so you also need a **5 V boost
  converter** with a comfortable margin: the Pi Zero 2 W can pull about 1 A at 5 V,
  which is well over 1.5 A from the cell. Pick a module rated for 2 A or more that
  works down to about 3 V in. The switch itself handles about 3 A continuous.
- Avoid using the device while charging. The TP4056 has no power-path logic, so a load
  on the cell keeps it from detecting the end of charge, and it can sit at 4.2 V
  longer than is good for the cell. Charge with the device off.
- Pololu's warning applies: do not use this as an emergency cutoff or safety
  disconnect.

Test the whole sequence with a bench supply before trusting it with a battery.

## Battery gauge (MAX17048)

The gauge uses the Pi's normal I2C bus (GPIO2 and GPIO3). The display uses SPI, so no
pins are shared. Enable I2C with `dtparam=i2c_arm=on` in `config.txt` (or
`sudo raspi-config`) and install the library with `sudo apt install python3-smbus`.

```
 MAX17048 breakout      Raspberry Pi (40-pin header)
 -----------------      ----------------------------
 SDA   <------------->  Pin 3   GPIO2 (I2C1 SDA)
 SCL   <------------->  Pin 5   GPIO3 (I2C1 SCL)
 GND   -------------    Pin 9   GND
 VIN   <-------------   Pin 17  3V3
 BAT   <-------------   battery + (the cell side, not the Pi's 5V rail)
```

Pin labels differ between breakouts, so check yours. The gauge's I2C address is
0x36. Check that the Pi sees it with `sudo apt install i2c-tools` then
`i2cdetect -y 1`. Connect the gauge directly to the battery, before the power switch,
so it keeps tracking the cell while the device is off (use the TP4056's OUT+ pad, or B+).

These instructions assume a TP4056 charger module with protection, a single 18650 cell
and a separate 5 V boost converter. They are untested on hardware; check polarity and your boards' pinouts before
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
