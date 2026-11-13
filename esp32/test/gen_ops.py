"""Write a random typing session (one op per line) to the path in argv[1].
Ops: cX = type character X, t = tab, e = enter, b = backspace."""
import random
import sys

r = random.Random(7)
words = ("the quick brown fox jumps over a lazy dog while typing extraordinarily long "
         "supercalifragilisticexpialidocious_words_without_any_spaces_at_all_that_exceed_the_"
         "limit_of_fifty_two_columns and more").split()
ops = []
for _ in range(900):
    x = r.random()
    if x < 0.80:
        for ch in r.choice(words) + " ":
            ops.append('c' + ch)
        if r.random() < 0.3 and ops[-1] == 'c ':
            ops.pop()
    elif x < 0.88:
        ops.append('e')
    elif x < 0.90:
        ops.append('t')
    elif x < 0.95:
        ops.append(r.choice('BIU'))
    else:
        ops.extend('b' for _ in range(r.randint(1, 4)))
ops += ['c' + c for c in 'x' * 130]
ops += ['B'] + ['c' + c for c in 'bold across a wrapped line ' * 4] + ['B', 'e']  # one long unbroken word
open(sys.argv[1], 'w').write('\n'.join(ops) + '\n')
