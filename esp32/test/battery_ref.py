import os, statistics, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src'))
import battery
samples = []
for line in sys.stdin:
    hi, lo = map(int, line.split())
    soc = battery.decode_soc(hi, lo)
    samples = (samples + [soc])[-5:]
    pct = round(statistics.median(samples))   # Python round(): half to even, like battery.py
    print("%.6f %.9f %.6f %d" % (soc, battery.decode_voltage(hi, lo), battery.decode_rate(hi, lo), pct))
