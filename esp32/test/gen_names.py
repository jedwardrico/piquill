import random, sys
r = random.Random(5)
atoms = ['a', 'B', '9', ' ', '  ', '.', '..', '-', '_', '!', '/', '\\', '.md', '.MD', '.txt', '.Txt', 'doc', 'My Notes', '\t', 'é'.encode('latin-1').decode('latin-1'), '*', '~']
lines = []
for _ in range(2000):
    s = ''.join(r.choice(atoms) for _ in range(r.randint(0, 9)))
    if r.random() < 0.1: s += 'x' * r.randint(20, 40)
    lines.append(s.encode('latin-1').hex())
open(sys.argv[1], 'w').write('\n'.join(lines) + '\n')
