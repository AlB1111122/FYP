#include "../../../include/miniuart.h"
#include "../../../include/timer.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/to_string.h"

int main() {
  MiniUart mu = MiniUart();
  etl::string<15> hello_str = "Hello world!\n";
  mu.init();
  mu.writeText(hello_str);
  while (1) {
    mu.writeText(hello_str);
    uint64_t time = Timer::readTimer();

    etl::string<100> n_str;
    etl::to_string(time, n_str);

    mu.writeText(n_str);
  }
}