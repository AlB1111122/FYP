#include "../../include/filter.h"

#include <stdint.h>

void unv::Filter::grayscale(uint8_t *rgb_data, int n_pixels) {
  for (int i = 0; i < n_pixels; i += 3) {
    // simplae avradge to get the grey
    uint8_t gray = static_cast<uint8_t>(
        (rgb_data[i] + rgb_data[i + 1] + rgb_data[i + 2]) / 3);

    // Set the new RGB values
    rgb_data[i] = gray;      // Red
    rgb_data[i + 1] = gray;  // Green
    rgb_data[i + 2] = gray;  // Blue
  }
}