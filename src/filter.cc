#include "../include/filter.h"

#include <stdint.h>

#include <cmath>

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

uint8_t *unv::Filter::sobelEdgeDetect(uint8_t *rgb_data, int n_pixels,
                                      int frame_width, uint8_t *sobeld_data) {
  unv::Filter::grayscale(rgb_data, n_pixels);
  // sobel kernals
  int g_x[3][3] = {{1, 0, -1}, {2, 0, -2}, {1, 0, -1}};
  int g_y[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};
  for (int i = frame_width + 1; i < n_pixels - (frame_width + 1); i += 3) {
    uint8_t *abv = &rgb_data[i - frame_width];
    uint8_t *blw = &rgb_data[i + frame_width];

    uint8_t *kernal_on_rgb[3][3] = {
        {abv - 3, abv, abv + 3},
        {(&rgb_data[i]) - 3, (&rgb_data[i]), (&rgb_data[i]) + 3},
        {blw - 3, blw, blw + 3}};
    int res_x;
    int res_y;
    int sobel_val;
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        res_x += g_x[j][k] * (*kernal_on_rgb[j][k]);
        if (res_x > 255) {
          res_x = 255;
        }
        // res_y += g_y[j][k] * (*kernal_on_rgb[j][k]);
        // sobel_val = std::sqrt((res_x * res_x) + (res_y * res_y));
      }
    }

    sobeld_data[i] = res_x;
    sobeld_data[i + 1] = res_x;
    sobeld_data[i + 2] = res_x;
  }
  return sobeld_data;
}