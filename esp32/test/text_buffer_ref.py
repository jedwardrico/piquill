"""Reference: the typing logic from src/main.py, using the Pi's own textfmt."""
import math, os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src'))
import textfmt
COLS = 52; LINES_ON_SCREEN = 14
lines = []; inp = ""; cur = 0
def ins(c):
    global inp, cur
    inp = inp[:cur] + c + inp[cur:]; cur += 1
def new_input_line(finished):
    return textfmt.active_markers(textfmt.format_state(finished))
def push_whole():
    global inp, cur
    lines.append(inp); inp = new_input_line(inp); cur = len(inp)
out = []
for raw in sys.stdin:
    raw = raw.rstrip('\n')
    o = raw[0]; r = 0
    if o == 'c':
        ch = raw[1:] or " "
        ins(ch)
        if ch == ' ':
            if textfmt.vis_len(inp) > COLS:
                push_whole(); r = 1
        elif textfmt.vis_len(inp) > COLS:
            limit = textfmt.raw_index(inp, COLS)
            ls = inp.rfind(' ', 0, limit)
            if ls <= 0:
                sentence = inp[:limit]
                inp = textfmt.active_markers(textfmt.format_state(sentence)) + inp[limit:]
            else:
                sentence = inp[:ls]
                inp = textfmt.active_markers(textfmt.format_state(sentence)) + inp[ls + 1:]
            lines.append(sentence); cur = len(inp); r = 1
    elif o == 't':
        ins(' '); ins(' ')
        if textfmt.vis_len(inp) > COLS:
            push_whole(); r = 1
        cur = len(inp)
    elif o == 'e':
        push_whole(); r = 1
    elif o == 'b':
        if cur > 0: inp = inp[:cur-1] + inp[cur:]; cur -= 1
    elif o in 'BIU':
        inp = textfmt.toggle(inp, {'B': textfmt.BOLD, 'I': textfmt.ITALIC, 'U': textfmt.UNDERLINE}[o]); cur = len(inp)
    out.append(str(r))
h = lambda s: s.encode('latin-1').hex()
print('\n'.join(out))
print("LINES", len(lines))
for l in lines: print("[%s]" % h(l))
print("INPUT [%s] %d" % (h(inp), cur))
words = sum(len(textfmt.strip_markers(l).split()) for l in lines) + len(textfmt.strip_markers(inp).split())
chars = sum(textfmt.vis_len(l) + 1 for l in lines) + textfmt.vis_len(inp)
print("W %d C %d" % (words, chars))
def vis(s):
    n = len(lines); st = max(0, n - LINES_ON_SCREEN*s); return len(lines[st:st+LINES_ON_SCREEN])
print("VIS %d VIS2 %d PAGES %d" % (vis(1), vis(2), max(1, math.ceil(len(lines) / LINES_ON_SCREEN))))
print("DOC %d" % (len(lines) + (1 if textfmt.strip_markers(inp) else 0)))
