import os, sys
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src'))
import textfmt
for line in sys.stdin:
    s = bytes.fromhex(line.strip()).decode('latin-1')
    h = lambda t: t.encode('latin-1').hex()
    parts = [h(textfmt.to_markdown(s)), h(textfmt.from_markdown(s)),
             h(textfmt.active_markers(textfmt.format_state(s))), h(textfmt.strip_markers(s)), str(textfmt.vis_len(s))]
    parts += [str(textfmt.raw_index(s, n)) for n in range(6)]
    parts += [h(textfmt.toggle(s, textfmt.BOLD)), h(textfmt.toggle(s, textfmt.ITALIC)), h(textfmt.toggle(s, textfmt.UNDERLINE)),
              str(len(textfmt.strip_markers(s).split()))]
    print(' '.join(parts))
