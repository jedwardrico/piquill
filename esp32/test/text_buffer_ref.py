import sys
COLS = 52; LINES_ON_SCREEN = 14
lines = []; inp = ""; cur = 0
def ins(c):
    global inp, cur
    inp = inp[:cur] + c + inp[cur:]; cur += 1
def push_whole():
    global inp, cur
    lines.append(inp); inp = ""; cur = 0
out = []
for raw in sys.stdin:
    raw = raw.rstrip('\n')
    o = raw[0]; r = 0
    if o == 'c':
        ch = raw[1:]
        ch = ch if ch else " "
        ins(ch)
        if ch == ' ':
            if len(inp) > COLS:
                push_whole(); r = 1
        elif len(inp) > COLS:
            limit = COLS
            ls = inp.rfind(' ', 0, limit)
            if ls <= 0:
                sentence = inp[:limit]; inp = inp[limit:]
            else:
                sentence = inp[:ls]; inp = inp[ls+1:]
            lines.append(sentence); cur = len(inp); r = 1
    elif o == 't':
        ins(' '); ins(' ')
        if len(inp) > COLS: push_whole(); r = 1
    elif o == 'e':
        push_whole(); r = 1
    elif o == 'b':
        if cur > 0: inp = inp[:cur-1] + inp[cur:]; cur -= 1
    out.append(str(r))
print('\n'.join(out))
print("LINES", len(lines))
for l in lines: print("[%s]" % l)
print("INPUT [%s] %d" % (inp, cur))
words = sum(len(l.split()) for l in lines) + len(inp.split())
chars = sum(len(l) + 1 for l in lines) + len(inp)
print("W %d C %d" % (words, chars))
def vis(s):
    n = len(lines); st = max(0, n - LINES_ON_SCREEN*s); return len(lines[st:st+LINES_ON_SCREEN])
print("VIS", vis(1)); print("VIS2", vis(2))
