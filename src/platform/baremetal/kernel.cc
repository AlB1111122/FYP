#include "../../../include/miniuart.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/string.h"

int main() {
  MiniUart mu = MiniUart();
  etl::string<15> hello_str = "Hello world!";
  mu.init();
  mu.writeText(hello_str);
  while (1) {
    mu.writeText(hello_str);
  }
}