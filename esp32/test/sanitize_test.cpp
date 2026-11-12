// Reads hex-encoded names on stdin, prints DocStore::sanitizeName for each.
#include "doc_store.h"
#include <stdio.h>
int main() {
  char buf[1024];
  while (fgets(buf, sizeof buf, stdin)) {
    std::string h(buf), s;
    while (!h.empty() && h.back() == '\n') h.pop_back();
    for (size_t i = 0; i + 1 < h.size(); i += 2) s += (char)strtol(h.substr(i, 2).c_str(), nullptr, 16);
    std::string r = DocStore::sanitizeName(s);
    for (unsigned char c : r) printf("%02x", c);
    printf("\n");
  }
}
