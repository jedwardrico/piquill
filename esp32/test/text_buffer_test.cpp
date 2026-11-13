// Reads typing ops on stdin, prints the buffer state; compared with text_buffer_ref.py.
#include "text_buffer.h"
#include <stdio.h>
#include <string.h>
static void hexp(const std::string &s) { for (unsigned char c : s) printf("%02x", c); }
int main() {
  TextBuffer b;
  char buf[256];
  while (fgets(buf, sizeof buf, stdin)) {
    char o = buf[0]; bool r = false;
    if (o == 'c') r = b.typeChar(buf[1]);
    else if (o == 't') r = b.typeTab();
    else if (o == 'e') r = b.enter();
    else if (o == 'b') r = b.backspace();
    else if (o == 'B') b.toggleFormat('\x01');
    else if (o == 'I') b.toggleFormat('\x02');
    else if (o == 'U') b.toggleFormat('\x03');
    printf("%d\n", r ? 1 : 0);
  }
  printf("LINES %zu\n", b.lines().size());
  for (auto &l : b.lines()) { printf("["); hexp(l); printf("]\n"); }
  printf("INPUT ["); hexp(b.input()); printf("] %zu\n", b.cursor());
  printf("W %zu C %zu\n", b.wordCount(), b.charCount());
  printf("VIS %zu VIS2 %zu PAGES %d\n", b.visibleLines(1).size(), b.visibleLines(2).size(), b.totalPages());
  auto d = b.docLines(); printf("DOC %zu\n", d.size());
}
