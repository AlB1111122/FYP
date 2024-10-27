#include "../../../include/miniuart.h"

void main() {
  uart_init();
  uart_writeText("Hello world!\n");
  while (1)
    ;
}