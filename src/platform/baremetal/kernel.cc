#include "../../../include/fb.h"
#include "../../../include/miniuart.h"
#include "../../../include/timer.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/to_string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/etl_profile.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/algorithm.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/vector.h"
#include <math.h>

static int nPrints=0;

template<typename T>
void printN(T time, int y) {
  etl::string<100> i_str;
  etl::string<100> n_str;
  etl::to_string(nPrints, i_str);
  etl::to_string(time, n_str,false);
  drawString(100, y, n_str.data(), 0x0f);
  drawString(200, y, i_str.data(), 0x0f);
  nPrints++;
}

int main() {
  etl::vector<int, 4> i[4];
  etl::fill(i->begin(),i->end(),3);
  printN(static_cast<uint64_t>(i->at(2)),2);
  MiniUart mu = MiniUart();
  Timer t = Timer();
  etl::string<15> hello_str = "Hello world!\n";
  char hello_chars[] = "Hello world!\n";

  char b_str[] = "b";
  char c_str[] = "c";
  char dr_str[] = "d";
  mu.init();
  mu.writeText(hello_str);

  fb_init();

  drawString(100, 30, hello_chars, 0x0f);
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
    /*uint64_t fullDuration = t.duration_since(time);
    long fullDL = static_cast<long>(fullDuration);
    long double dur_to_dub= static_cast<long double>(fullDL);
    etl::string<100> text = "The result is ";
    etl::to_string(dur_to_dub/1000000.0, text, etl::format_spec().precision(6),true);
    drawString(100,260, n_str.data(), 0x0f);

    etl::string<10> i_str;
    etl::to_string(nPrints, i_str);
    drawString(200,260, i_str.data(), 0x0f);
    nPrints++;*/

    etl::string<100> text = "The result is ";

    etl::to_string(3.1415, text, etl::format_spec().precision(8), true);
    drawString(100,260, text.data(), 0x0f);
  }
}
