#!/bin/sh
# Host-side tests (no ESP32 needed): the C++ text buffer must wrap lines exactly
# like the Python logic taken from src/main.py. Needs g++ and python3.
set -e
cd "$(dirname "$0")"
T=$(mktemp -d)
python3 -I gen_ops.py "$T/ops.txt"
g++ -std=c++17 -Wall -I../src text_buffer_test.cpp ../src/text_buffer.cpp ../src/text_render.cpp -o "$T/tb"
"$T/tb" < "$T/ops.txt" > "$T/cpp.txt"
python3 -I text_buffer_ref.py < "$T/ops.txt" > "$T/py.txt"
if cmp -s "$T/cpp.txt" "$T/py.txt"; then
  echo "text buffer: OK ($(wc -l < "$T/ops.txt") keystrokes, $(grep -c '^\[' "$T/py.txt") lines)"
else
  echo "text buffer: MISMATCH"; diff "$T/cpp.txt" "$T/py.txt" | head; exit 1
fi
rm -rf "$T"
