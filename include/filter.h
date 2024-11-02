#pragma once
#include <stdint.h>

namespace com {
class Filter {
 public:
  Filter();
  static void grayscale(uint8_t *rgb_data, int n_pixels, uint8_t *new_rgb);
  static void sobelEdgeDetect(uint8_t *rgb_data, int n_pixels, int frame_width,
                              uint8_t *new_rgb);
};
}  // namespace com