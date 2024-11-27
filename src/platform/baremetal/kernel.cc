#include "../../../include/fb.h"
#include "../../../include/miniuart.h"
#include "../../../include/timer.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/to_string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/etl_profile.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/algorithm.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/vector.h"
#include <stdlib.h>

static int nPrints=0;

template<typename T>
void printN(T time, int y) {
  etl::string<100> i_str;
  etl::string<100> n_str;
  etl::to_string(nPrints, i_str);
  etl::to_string(time, n_str, etl::format_spec().precision(6),false);
  drawString(100, y, n_str.data(), 0x0f);
  drawString(200, y, i_str.data(), 0x0f);
  nPrints++;
}

int main() {
  MiniUart mu = MiniUart();
  Timer t = Timer();
  etl::string<15> hello_str = "Hello world!\n";
  mu.init();
  mu.writeText(hello_str.data());

  fb_init();

  drawString(100, 30, hello_str.data(), 0x0f);
  while (1) {
    mu.writeText(hello_str);
    uint64_t time = t.now();
    printN(t.to_sec(time), 50);

    etl::string<32> n_str;
    etl::to_string(time, n_str);
    uint64_t duration_since = t.duration_since(time);
    printN(t.to_sec(duration_since), 70);

    auto secTime = t.get_hertz();
    etl::string<32> sec_str;
    etl::to_string(secTime, sec_str);
    printN(secTime,130);
    while (t.duration_since(time) < 10000000) {
      ;
    }
    int b = 4000 / 1000;
    printN(b, 150);
    while (t.duration_since(time) < 15000000) {
      ;
    }
    int c = 54382589 / 1000;
    printN(c, 180);
    while (t.duration_since(time) < 20000000) {
      ;
    }
    int d = 1 / 1000;
    printN(d, 200);
    while (t.duration_since(time) < 25000000) {
      ;
    }
    auto a = t.to_sec(time);
    printN(a, 220);
    while (t.duration_since(time) < 30000000) {
      ;
    }
    auto m = t.to_milli(time);
    printN(m, 240);
    uint64_t fullDuration = t.duration_since(time);
    /*etl::string<100> text = "The result is ";
    etl::to_string(fullDuration/1000000, text, etl::format_spec().precision(6),true);
    drawString(100,260, text.data(), 0x0f);*/
    ldiv_t division_result = ldiv(fullDuration, 1000000);
    double res = division_result.quot + ((double)division_result.rem / (double)1000000);
    etl::string<100> text = "The result is ";
    etl::to_string(res, text, etl::format_spec().precision(6),true);
    drawString(100,260, text.data(), 0x0f);
  }
}
