#include "../../../include/miniuart.h"

void main() {
  uart_init();
  char* hello_str = "Hello, World!\n";
  uart_writeText(hello_str);
  while (1) {
    uart_writeText(hello_str);
  }
}