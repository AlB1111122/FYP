#pragma once
#include <stdint.h>

namespace unv {
class Filter {
 private:
  const int n_pixels;

 public:
  Filter();
  static void grayscale(uint8_t *rgb_data, int n_pixels);
  static uint8_t *sobelEdgeDetect(uint8_t *rgb_data, int n_pixels,
                                  int frame_width, uint8_t *sobeld_data);
};
}  // namespace unv