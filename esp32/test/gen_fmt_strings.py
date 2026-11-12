"""Random strings full of markers and Markdown specials, hex-encoded one per line."""
import random, sys
r = random.Random(11)
atoms = ['a', 'b', 'hello', ' ', '  ', '\x01', '\x02', '\x03', '*', '**', '***', '_', '<', '<u>', '</u>', '\\', '`', 'x*y', '\\*', '.', ',']
lines = []
for _ in range(3000):
    n = r.randint(0, 14)
    s = ''.join(r.choice(atoms) for _ in range(n))
    lines.append(s.encode('latin-1').hex())
open(sys.argv[1], 'w').write('\n'.join(lines) + '\n')
