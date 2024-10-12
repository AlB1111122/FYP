#ifndef FILTER_H
#define FILTER_H
#include <stdint.h>
namespace unv {
class Filter {
 private:
  const int n_pixels;

 public:
  Filter();
  static void grayscale(uint8_t *rgb_data, int n_pixels);
};
}  // namespace unv
#endif