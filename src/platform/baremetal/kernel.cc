#include "../../../include/fb.h"
#include "../../../include/miniuart.h"
#include "../../../include/timer.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/to_string.h"

void printN(uint64_t time, int y) {
  etl::string<32> n_str;
  etl::to_string(time, n_str);
  drawString(100, y, n_str.data(), 0x0f);
}

int main() {
  MiniUart mu = MiniUart();
  Timer t = Timer();
  // uint64_t start = t.now();
  etl::string<15> hello_str = "Hello world!\n";
  char hello_chars[] = "Hello world!\n";

  char b_str[] = "b";
  char c_str[] = "c";
  char dr_str[] = "d";
  mu.init();
  mu.writeText(hello_str);

  fb_init();

  drawString(100, 100, hello_chars, 0x0f);
  while (1) {
    mu.writeText(hello_str);
    uint64_t time = t.now();
    printN(t.to_sec(time), 450);

    etl::string<32> n_str;
    etl::to_string(time, n_str);
    uint64_t duration_since = t.duration_since(time);
    printN(t.to_sec(duration_since), 450);

    drawString(100, 200, n_str.data(), 0x0f);

    etl::string<32> d_str;
    etl::to_string(duration_since, d_str);
    drawString(100, 300, d_str.data(), 0x0f);

    // auto secTime = t.to_sec(time);
    auto secTime = t.get_hertz();
    etl::string<32> sec_str;
    etl::to_string(secTime, sec_str);
    drawString(100, 350, sec_str.data(), 0x0f);
    while (t.duration_since(time) < 10000000) {
      ;
    }
    int b = 4000 / 1000;
    printN(b, 370);
    while (t.duration_since(time) < 15000000) {
      ;
    }
    int c = 54382589 / 1000;
    printN(c, 390);
    while (t.duration_since(time) < 20000000) {
      ;
    }
    int d = 1 / 1000;
    printN(d, 410);
    while (t.duration_since(time) < 25000000) {
      ;
    }
    auto a = t.to_sec(time);
    printN(a, 430);
    while (t.duration_since(time) < 30000000) {
      ;
    }
    auto m = t.to_milli(time);
    printN(m, 450);
    break;
  }
}
