#include "../../include/filter.h"

#include <math.h>
#if __STDC_HOSTED__ != 1
#include "../../include/errno.h"
#endif
#include <stdint.h>
#include <sys/types.h>

void com::Filter::grayscale(uint8_t *rgbData, int nPixelBits, uint8_t *newRgb) {
  for (int i = 0; i < nPixelBits; i += 4) {
    // simple avrage to get the grey
    uint8_t gray = static_cast<uint8_t>(
        (rgbData[i] + rgbData[i + 1] + rgbData[i + 2]) / 3);

    newRgb[i] = gray;      // Red
    newRgb[i + 1] = gray;  // Green
    newRgb[i + 2] = gray;  // Blue
  }
}

void com::Filter::sobelEdgeDetect(uint8_t *rgbData, int nPixelBits,
                                  int frameStride, uint8_t *newRgb) {
  // sobel kernel
  int g_x[3][3] = {{1, 0, -1}, {2, 0, -2}, {1, 0, -1}};
  int g_y[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};

  int maxCols = frameStride / 4 - 1;
  for (int i = 0; i < nPixelBits; i += 4) {
    // deal with top and bottom row
    uint8_t *abv =
        (i < frameStride) ? (&rgbData[i]) : (&rgbData[i - frameStride]);
    uint8_t *blw = (i > nPixelBits - frameStride) ? (&rgbData[i])
                                                  : (&rgbData[i + frameStride]);
    uint8_t *kernal_on_rgb[3][3] = {
        {abv - 4, abv, abv + 4},
        {(&rgbData[i]) - 4, (&rgbData[i]), (&rgbData[i]) + 4},
        {blw - 4, blw, blw + 4}};
    // deal with l and r edge

    int col = (i % frameStride) / 4;
    if (col == 0) {
      kernal_on_rgb[0][0] = abv;
      kernal_on_rgb[1][0] = (&rgbData[i]);
      kernal_on_rgb[2][0] = (blw);
    }
    if (col == maxCols) {
      kernal_on_rgb[0][2] = abv;
      kernal_on_rgb[1][2] = (&rgbData[i]);
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

    newRgb[i] = sobel_val;
    newRgb[i + 1] = sobel_val;
    newRgb[i + 2] = sobel_val;
    newRgb[i + 3] = 255;  // full opacity
  }
}