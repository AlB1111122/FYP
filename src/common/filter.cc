#include "../../include/filter.h"

#include <math.h>
#if __STDC_HOSTED__ != 1
#include "../../include/errno.h"
#endif
#ifdef __ARM_NEON
#include <arm_neon.h>
#define USE_NEON_SQRT 1
#else
#define USE_NEON_SQRT 0
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
  int gX[3][3] = {{1, 0, -1}, {2, 0, -2}, {1, 0, -1}};
  int gY[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};

  int maxCols = frameStride / 4 - 1;
  for (int i = 0; i < nPixelBits; i += 4) {
    // deal with top and bottom row
    uint8_t *abv =
        (i < frameStride) ? (&rgbData[i]) : (&rgbData[i - frameStride]);
    uint8_t *blw = (i > nPixelBits - frameStride) ? (&rgbData[i])
                                                  : (&rgbData[i + frameStride]);
    uint8_t *kernalOnRgb[3][3] = {
        {abv - 4, abv, abv + 4},
        {(&rgbData[i]) - 4, (&rgbData[i]), (&rgbData[i]) + 4},
        {blw - 4, blw, blw + 4}};
    // deal with l and r edge

    int col = (i % frameStride) / 4;
    if (col == 0) {
      kernalOnRgb[0][0] = abv;
      kernalOnRgb[1][0] = (&rgbData[i]);
      kernalOnRgb[2][0] = (blw);
    }
    if (col == maxCols) {
      kernalOnRgb[0][2] = abv;
      kernalOnRgb[1][2] = (&rgbData[i]);
      kernalOnRgb[2][2] = (blw);
    }
    int resX = 0;
    int resY = 0;
    float sobelValF = 0;
    for (int j = 0; j < 3; j++) {
      for (int k = 0; k < 3; k++) {
        resX += gX[j][k] * (*kernalOnRgb[j][k]);
        resY += gY[j][k] * (*kernalOnRgb[j][k]);
      }
    }
    float toSqrt = static_cast<float>((resX * resX) + (resY * resY));
    // to make it runnable for testing on non-arm computers
#if USE_NEON_SQRT
    __asm__ volatile("fsqrt %s0, %s1" : "=w"(sobelValF) : "w"(toSqrt));
#else
    sobelValF = sqrtf(toSqrt);
#endif

    int sobelVal = static_cast<int>(roundf(sobelValF));
    if (sobelVal > 255) sobelVal = 255;

    newRgb[i] = sobelVal;
    newRgb[i + 1] = sobelVal;
    newRgb[i + 2] = sobelVal;
    newRgb[i + 3] = 255;  // full opacity
  }
}