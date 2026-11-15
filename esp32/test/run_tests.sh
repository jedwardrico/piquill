#!/bin/sh
# Host-side tests (no ESP32 needed): the C++ text buffer must wrap lines exactly
# like the Python logic taken from src/main.py. Needs g++ and python3.
set -e
cd "$(dirname "$0")"
T=$(mktemp -d)
python3 -I gen_ops.py "$T/ops.txt"
g++ -std=c++17 -Wall -I../src text_buffer_test.cpp ../src/text_buffer.cpp ../src/textfmt.cpp -o "$T/tb"
"$T/tb" < "$T/ops.txt" > "$T/cpp.txt"
python3 -I text_buffer_ref.py < "$T/ops.txt" > "$T/py.txt"
if cmp -s "$T/cpp.txt" "$T/py.txt"; then
  echo "text buffer: OK ($(wc -l < "$T/ops.txt") keystrokes, $(grep -c '^\[' "$T/py.txt") lines)"
else
  echo "text buffer: MISMATCH"; diff "$T/cpp.txt" "$T/py.txt" | head; exit 1
fi
g++ -std=c++17 -Wall -I../src hid_keys_test.cpp ../src/hid_keys.cpp -o "$T/hid"
"$T/hid"
python3 -I gen_fmt_strings.py "$T/fmt.txt"
g++ -std=c++17 -Wall -I../src textfmt_test.cpp ../src/textfmt.cpp -o "$T/fmt"
"$T/fmt" < "$T/fmt.txt" > "$T/fmt_cpp.txt"
python3 -I textfmt_ref.py < "$T/fmt.txt" > "$T/fmt_py.txt"
if cmp -s "$T/fmt_cpp.txt" "$T/fmt_py.txt"; then echo "textfmt: OK ($(wc -l < "$T/fmt.txt") strings)"; else echo "textfmt: MISMATCH"; exit 1; fi
g++ -std=c++17 -Wall -I../src -I. editor_test.cpp ../src/editor.cpp ../src/doc_store.cpp ../src/text_buffer.cpp ../src/textfmt.cpp -o "$T/ed"
TMPDIR="$T" "$T/ed"
python3 -I gen_names.py "$T/names.txt"
g++ -std=c++17 -Wall -I../src sanitize_test.cpp ../src/doc_store.cpp ../src/textfmt.cpp -o "$T/san"
"$T/san" < "$T/names.txt" > "$T/san_cpp.txt"
python3 -I sanitize_ref.py < "$T/names.txt" > "$T/san_py.txt"
if cmp -s "$T/san_cpp.txt" "$T/san_py.txt"; then echo "file names: OK ($(wc -l < "$T/names.txt") names)"; else echo "file names: MISMATCH"; exit 1; fi
printf '2 128\n3 128\n4 128\n5 128\n6 128\n50 128\n51 128\n50 128\n51 128\n50 128\n0 128\n1 128\n' > "$T/bat.txt"
python3 -I -c "import random; r=random.Random(3); print('\\n'.join('%d %d'%(r.randint(0,255),r.randint(0,255)) for _ in range(500)))" >> "$T/bat.txt"
g++ -std=c++17 -Wall -I../src battery_test.cpp -o "$T/bat"
"$T/bat" < "$T/bat.txt" > "$T/bat_cpp.txt"
python3 -I battery_ref.py < "$T/bat.txt" > "$T/bat_py.txt"
if cmp -s "$T/bat_cpp.txt" "$T/bat_py.txt"; then echo "battery: OK"; else echo "battery: MISMATCH"; exit 1; fi
g++ -std=c++17 -Wall -I../src settings_test.cpp ../src/settings.cpp -o "$T/set"
"$T/set"
g++ -std=c++17 -Wall -I../src power_button_test.cpp -o "$T/pb"
"$T/pb"
rm -rf "$T"
