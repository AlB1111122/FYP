#include "../../include/filter.h"
#if __STDC_HOSTED__ == 1
#include <math.h>
#else
#pragma message("compiling on bm")
#include "../../include/newtonSqrt.h"
#endif
#include <stdint.h>
#include <sys/types.h>

void com::Filter::grayscale(uint8_t *rgb_data, int n_pixel_bits,
                            uint8_t *new_rgb) {
  for (int i = 0; i < n_pixel_bits; i += 4) {
    // simple avrage to get the grey
    uint8_t gray = static_cast<uint8_t>(
        (rgb_data[i] + rgb_data[i + 1] + rgb_data[i + 2]) / 3);

    new_rgb[i] = gray;      // Red
    new_rgb[i + 1] = gray;  // Green
    new_rgb[i + 2] = gray;  // Blue
  }
}

void com::Filter::sobelEdgeDetect(uint8_t *rgb_data, int n_pixel_bits,
                                  int frame_stride, uint8_t *new_rgb) {
  // sobel kernel
  int g_x[3][3] = {{1, 0, -1}, {2, 0, -2}, {1, 0, -1}};
  int g_y[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};

  int maxCols = frame_stride / 4 - 1;
  for (int i = 0; i < n_pixel_bits; i += 4) {
    // deal with top and bottom row
    uint8_t *abv =
        (i < frame_stride) ? (&rgb_data[i]) : (&rgb_data[i - frame_stride]);
    uint8_t *blw = (i > n_pixel_bits - frame_stride)
                       ? (&rgb_data[i])
                       : (&rgb_data[i + frame_stride]);
    uint8_t *kernal_on_rgb[3][3] = {
        {abv - 4, abv, abv + 4},
        {(&rgb_data[i]) - 4, (&rgb_data[i]), (&rgb_data[i]) + 4},
        {blw - 4, blw, blw + 4}};
    // deal with l and r edge

    int col = (i % frame_stride) / 4;
    if (col == 0) {
      kernal_on_rgb[0][0] = abv;
      kernal_on_rgb[1][0] = (&rgb_data[i]);
      kernal_on_rgb[2][0] = (blw);
    }
    if (col == maxCols) {
      kernal_on_rgb[0][2] = abv;
      kernal_on_rgb[1][2] = (&rgb_data[i]);
      kernal_on_rgb[2][2] = (blw);
    }
    int res_x = 0;
    int res_y = 0;
    int sobel_val = 0;
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        res_x += g_x[j][k] * (*kernal_on_rgb[j][k]);
        res_y += g_y[j][k] * (*kernal_on_rgb[j][k]);
      }
    }
    sobel_val = sqrt((res_x * res_x) +
                     (res_y * res_y));  // figure out errno later to use regular
    if (sobel_val > 255) {
      sobel_val = 255;
    } else if (sobel_val < 0) {
      sobel_val = 0;
    }

    new_rgb[i] = sobel_val;
    new_rgb[i + 1] = sobel_val;
    new_rgb[i + 2] = sobel_val;
    new_rgb[i + 3] = 255;  // full opacity
  }
}