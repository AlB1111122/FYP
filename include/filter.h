#pragma once
#include <stdint.h>

namespace com {
class Filter {
 public:
  static void grayscale(uint8_t *rgbData, int nPixelBits, uint8_t *newRgb);
  static void sobelEdgeDetect(uint8_t *rgbData, int nPixelBits, int frameStride,
                              uint8_t *newRgb);
};
}  // namespace com