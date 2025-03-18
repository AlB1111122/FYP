#pragma once
#include <stdint.h>

namespace com {
class Filter {
 public:
  Filter();
  static void grayscale(uint8_t *rgb_data, int n_pixel_bits, uint8_t *new_rgb);
  static void sobelEdgeDetect(uint8_t *rgb_data, int n_pixel_bits,
                              int frame_stride, uint8_t *new_rgb);
};
}  // namespace com