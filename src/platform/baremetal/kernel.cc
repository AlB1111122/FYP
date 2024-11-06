#include "../../../include/miniuart.h"

void main() {
  MiniUart mu = MiniUart();
  mu.init();
  mu.writeText("Hello world!\n");
  while (1) {
    mu.writeText("Hello world!\n");
  }
}