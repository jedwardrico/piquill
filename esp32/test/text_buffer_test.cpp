#include "text_buffer.h"
#include "text_render.h"
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
int main(int argc, char **argv) {
  TextBuffer b; 
  // ops from stdin: one per line: "c<char>", "t", "e", "b"
  std::string line;
  char buf[256];
  while (fgets(buf, sizeof buf, stdin)) {
    char o = buf[0]; bool r = false;
    if (o == 'c') r = b.typeChar(buf[1]);
    else if (o == 't') r = b.typeTab();
    else if (o == 'e') r = b.enter();
    else if (o == 'b') r = b.backspace();
    printf("%d\n", r ? 1 : 0);
  }
  printf("LINES %zu\n", b.lines().size());
  for (auto &l : b.lines()) printf("[%s]\n", l.c_str());
  printf("INPUT [%s] %zu\n", b.input().c_str(), b.cursor());
  printf("W %zu C %zu\n", b.wordCount(), b.charCount());
  auto v = b.visibleLines(1); printf("VIS %zu\n", v.size());
  v = b.visibleLines(2); printf("VIS2 %zu\n", v.size());
  if (argc > 1) {
    static uint8_t frame[textrender::ROW_BYTES * textrender::SCREEN_H];
    textrender::drawEditor(frame, b, 1);
    FILE *f = fopen(argv[1], "wb"); fprintf(f, "P4\n920 680\n");
    // PBM: 1 = black; our frame 1 = white, so invert
    for (size_t i = 0; i < sizeof frame; i++) fputc(~frame[i], f);
    fclose(f);
  }
}
