"""Battery level from a MAX17048 fuel gauge on the Pi's I2C bus.

The MAX17048 (I2C address 0x36) estimates state of charge from the cell voltage,
so it needs no current-sense wiring. If the gauge or the I2C libraries are missing,
everything here quietly reports "no battery" and PiQuill runs as before.

    gauge = Battery()
    gauge.start()           # polls in a background thread
    gauge.percent           # smoothed 0-100, or None if no gauge was found
    gauge.charging          # True while the cell is charging
"""
import os
import statistics
import threading
import time

try:
    from smbus2 import SMBus
except ImportError:
    try:
        from smbus import SMBus
    except ImportError:
        SMBus = None

ADDRESS = 0x36
REG_VCELL = 0x02   # cell voltage, 78.125 uV per bit
REG_SOC = 0x04     # state of charge, high byte = whole %, low byte = 1/256 %
REG_CRATE = 0x16   # charge/discharge rate, signed, 0.208 %/hour per bit (MAX17048/49 only)

I2C_BUS = int(os.environ.get('ZW_BATTERY_BUS', 1))   # the Pi's normal I2C bus (GPIO2/3)
POLL_SECONDS = int(os.environ.get('ZW_BATTERY_POLL', 30))
SMOOTHING_SAMPLES = 5      # the reported percent is the median of this many readings
CHARGING_RATE = 0.5        # %/hour above which the cell counts as charging


def decode_soc(hi, lo):
    """State of charge in percent from the two SOC register bytes."""
    return min(100.0, max(0.0, hi + lo / 256))


def decode_voltage(hi, lo):
    return ((hi << 8) | lo) * 78.125e-6


def decode_rate(hi, lo):
    """Charge rate in %/hour; positive while charging."""
    raw = (hi << 8) | lo
    if raw >= 0x8000:
        raw -= 0x10000
    return raw * 0.208


class Battery:
    def __init__(self, bus=I2C_BUS, address=ADDRESS):
        self.percent = None    # smoothed whole percent, None = no gauge
        self.voltage = None
        self.charging = False
        self._address = address
        self._samples = []
        self._bus = None
        if SMBus is not None:
            try:
                self._bus = SMBus(bus)
            except OSError:
                self._bus = None   # I2C not enabled
        self.poll()

    def _read(self, reg):
        hi, lo = self._bus.read_i2c_block_data(self._address, reg, 2)
        return hi, lo

    def poll(self):
        """Take one reading and update the smoothed values."""
        if self._bus is None:
            return
        try:
            soc = decode_soc(*self._read(REG_SOC))
            self.voltage = decode_voltage(*self._read(REG_VCELL))
            try:
                self.charging = decode_rate(*self._read(REG_CRATE)) > CHARGING_RATE
            except OSError:
                self.charging = False
        except OSError:
            return   # a failed read keeps the last good value
        self._samples = (self._samples + [soc])[-SMOOTHING_SAMPLES:]
        self.percent = round(statistics.median(self._samples))

    def start(self):
        """Poll in the background so the main loop never waits on I2C."""
        if self._bus is not None:
            threading.Thread(target=self._loop, daemon=True).start()

    def _loop(self):
        while True:
            time.sleep(POLL_SECONDS)
            self.poll()
