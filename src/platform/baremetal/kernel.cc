#include <etl/etl_profile.h>
#include <etl/string.h>
#include <etl/to_string.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "../../../soccerBytes.h"
#include "filter.h"
#include "frameBuffer.h"
#include "miniuart.h"
#include "pl_mpeg/pl_mpeg.h"
#include "timer.h"

int constexpr WIN_HEIGHT = 720;
int constexpr WIN_WIDTH = 1280;
int constexpr N_PIXELS = (WIN_WIDTH * WIN_HEIGHT * 4);

struct videoApp_t {
  plm_t *plm;
  bool wantsToQuit;
  uint64_t lastTime;
  uint8_t rgb_data[N_PIXELS] __attribute__((aligned(64)));
  int winHeight;
  int winWidth;
  uint64_t ttr[400][5];
  int totalFramesCompleted = 0;
  FrameBuffer *fbPtr;
};

struct FrameRateInfo {
  double frameMs;
  double fps;
  int totalFrames;
  double totalTExp;
} FrameRateInfo;

int xVal = 100;
static int nPrints = 0;

template <typename T>
void printN(T time, FrameBuffer &fb) {
  etl::string<100> i_str;
  etl::string<100> n_str;
  etl::to_string(nPrints, i_str);
  etl::to_string(time, n_str, etl::format_spec().precision(6), false);
  fb.drawString(xVal, (nPrints * 10), n_str.data(), 0x0f);
  fb.drawString(xVal + 50, (nPrints * 10), i_str.data(), 0x0f);
  nPrints++;
  if (nPrints > 106) {
    nPrints = 0;
    xVal += 100;
  }
}

void createApp(videoApp_t *appPtr, plm_t *plmPtr);
void updateFrame(plm_t *mpeg, plm_frame_t *frame, void *user);
void updateVideo(videoApp_t *self);
void showVideoStats(FrameBuffer &fb);
void makeStatFile(uint64_t startTime, videoApp_t *self, Timer &t, MiniUart &mu,
                  FrameBuffer &fb);

// better performance with this just in bss insted of taking up stack
uint8_t fRgbData[N_PIXELS] __attribute__((aligned(64)));
uint8_t newRgbData[N_PIXELS] __attribute__((aligned(64)));

// assembly func defined in boot.s
extern "C" int getEl();

plm_t plmHolder;
videoApp_t app;
int main() {
  MiniUart mu = MiniUart();
  Timer t = Timer();
  FrameBuffer fb = FrameBuffer();
  etl::string<15> helloStr = "check\n";
  app.fbPtr = &fb;

  // let the gpu finish initializing
  auto waiter = Timer::now();
  while (t.durationSince(waiter) < 1500000) {  // wait 1.5 sec
    ;
  }

  // should be 1 not 2 which it boots to automatically
  printN(getEl(), fb);
  mu.writeText(helloStr);
  videoApp_t *appPtr = &app;
  plm_t *plmPtr = &plmHolder;
  mu.writeText(helloStr);

  createApp(appPtr, plmPtr);

  mu.writeText(helloStr);

  showVideoStats(fb);

  mu.writeText("\n");

  uint64_t start = Timer::now();
  appPtr->lastTime = start;

  // fb init results
  etl::string<64> cmpol = "";
  etl::to_string(appPtr->totalFramesCompleted, cmpol,
                 etl::format_spec().precision(6), true);
  uint32_t fbAddress =
      static_cast<uint32_t>(reinterpret_cast<uintptr_t>(fb.getFb()));
  etl::string<64> fbs = "";
  etl::to_string(fbAddress, fbs, etl::format_spec().precision(6), true);
  cmpol.append(" ");
  cmpol.append(fbs);
  cmpol.append("\n");
  mu.writeText(cmpol);
  mu.writeText("check again \n");

  while ((!appPtr->wantsToQuit)) {
    updateVideo(appPtr);
  }
  mu.writeText("\n");
  makeStatFile(start, appPtr, t, mu, fb);
}

void createApp(videoApp_t *appPtr, plm_t *plmPtr) {
  appPtr->plm = plm_create_with_memory(soccer, soccer_sz, 0, plmPtr);

  plm_set_video_decode_callback(appPtr->plm, updateFrame, appPtr);
  plm_set_loop(appPtr->plm, FALSE);  // don't loop video
  plm_set_audio_enabled(appPtr->plm, FALSE);

  FrameRateInfo.fps = plm_get_framerate(appPtr->plm);
  FrameRateInfo.totalTExp = plm_get_duration(appPtr->plm);
  FrameRateInfo.totalFrames = FrameRateInfo.totalTExp * FrameRateInfo.fps;
  FrameRateInfo.frameMs = (1.0 / static_cast<double>(FrameRateInfo.fps)) * 1000;
}

void updateFrame(plm_t *mpeg, plm_frame_t *frame, void *user) {
  uint64_t startTime = Timer::now();
  videoApp_t *self = static_cast<videoApp_t *>(user);
  plm_frame_to_rgba(frame, newRgbData, self->fbPtr->getPitch());
  uint64_t toRgb = Timer::now();

  com::Filter::sobelEdgeDetect(newRgbData, N_PIXELS, frame->width * 4,
                               fRgbData);
  // com::Filter::grayscale(newRgbData, N_PIXELS, fRgbData);
  // com::Filter::fisheyeTransform(newRgbData, N_PIXELS, frame->width * 4,
  //                               fRgbData);
  uint64_t toFiltered = Timer::now();

  self->fbPtr->bufferCpy(fRgbData);
  uint64_t toRendered = Timer::now();

  self->ttr[self->totalFramesCompleted][0] = self->lastTime;
  self->ttr[self->totalFramesCompleted][1] = startTime;
  self->ttr[self->totalFramesCompleted][2] = toRgb;
  self->ttr[self->totalFramesCompleted][3] = toFiltered;
  self->ttr[self->totalFramesCompleted][4] = toRendered;
  self->totalFramesCompleted++;
}

void updateVideo(videoApp_t *self) {
  auto now = Timer::now();
  self->lastTime = now;
  // jump to the next frame every time as not to waste cycles
  plm_decode(self->plm, (FrameRateInfo.frameMs / 1000.0));

  if (plm_has_ended(self->plm)) {
    self->wantsToQuit = true;
  }
}

// debug information
void showVideoStats(FrameBuffer &fb) {
  etl::string<64> frameStats = "Total frames: ";
  etl::to_string(FrameRateInfo.totalFrames, frameStats,
                 etl::format_spec().precision(6), true);
  fb.drawString(400, 10, frameStats.data(), 0x0f);
  etl::string<64> fpsStats = "FPS: ";
  etl::to_string(FrameRateInfo.fps, fpsStats, etl::format_spec().precision(6),
                 true);
  fb.drawString(400, 20, fpsStats.data(), 0x0f);
  etl::string<64> framt = "Max frame time(ms): ";
  etl::to_string(FrameRateInfo.frameMs, framt, etl::format_spec().precision(6),
                 true);
  fb.drawString(400, 30, framt.data(), 0x0f);
  etl::string<64> plt = "Correct play time(sec): ";
  etl::to_string(FrameRateInfo.totalTExp, plt, etl::format_spec().precision(6),
                 true);
  fb.drawString(400, 40, plt.data(), 0x0f);
}

// UART print results in csv format
void makeStatFile(uint64_t startTime, videoApp_t *self, Timer &t, MiniUart &mu,
                  FrameBuffer &fb) {
  double duration = t.toSec(t.durationSince(startTime));
  double avgRgb = 0;
  double avgFilter = 0;
  double avgRender = 0;
  double avgDisplay = 0;
  double avgPlmDec = 0;

  double durations[400][5];
  int droppedFrames = 0;
  for (int i = 0; i < self->totalFramesCompleted; i++) {
    double ttplmd = t.toMilli(self->ttr[i][1] - self->ttr[i][0]);
    double ttrgb = t.toMilli(self->ttr[i][2] - self->ttr[i][1]);
    double ttf = t.toMilli(self->ttr[i][3] - self->ttr[i][2]);
    double ttr = t.toMilli(self->ttr[i][4] - self->ttr[i][3]);
    double ttd = t.toMilli(self->ttr[i][4] - self->ttr[i][0]);
    durations[i][0] = ttplmd;
    durations[i][1] = ttrgb;
    durations[i][2] = ttf;
    durations[i][3] = ttr;
    durations[i][4] = ttd;
    avgPlmDec += (ttplmd - avgPlmDec) / (i + 1);
    avgRgb += (ttrgb - avgRgb) / (i + 1);
    avgFilter += (ttf - avgFilter) / (i + 1);
    avgRender += (ttr - avgRender) / (i + 1);
    avgDisplay += (ttd - avgDisplay) / (i + 1);

    if (ttd > FrameRateInfo.frameMs) {
      droppedFrames++;
    }
  }

  mu << "decode(ms),convert_rgb(ms),filter(ms),display(ms)"
        ",time_in_callback(ms),"
     << "avg_decoded(ms),avg_rgb(ms),avg_filtered(ms),avg_rendered(ms),"
     << "avg_total_time_to_display(ms),total_slow_frames,total_callbacks,"
     << "real_play_time(sec),actual_fps,total_video_frames,default_fps,max_"
        "frame_time(ms),correct_play_time(sec)\n";
  for (int i = 0; i < self->totalFramesCompleted; i++) {
    etl::string<510> uartStr = "";

    etl::to_string(durations[i][0], uartStr, etl::format_spec().precision(6),
                   true);
    uartStr.append(",");

    etl::to_string(durations[i][1], uartStr, etl::format_spec().precision(6),
                   true);
    uartStr.append(",");

    etl::to_string(durations[i][2], uartStr, etl::format_spec().precision(6),
                   true);
    uartStr.append(",");

    etl::to_string(durations[i][3], uartStr, etl::format_spec().precision(6),
                   true);
    uartStr.append(",");

    etl::to_string(durations[i][4], uartStr, etl::format_spec().precision(6),
                   true);
    uartStr.append(",");

    if (i < 1) {
      etl::to_string(avgPlmDec, uartStr, etl::format_spec().precision(6), true);
      uartStr.append(",");

      etl::to_string(avgRgb, uartStr, etl::format_spec().precision(6), true);
      uartStr.append(",");

      etl::to_string(avgFilter, uartStr, etl::format_spec().precision(6), true);
      uartStr.append(",");

      etl::to_string(avgRender, uartStr, etl::format_spec().precision(6), true);
      uartStr.append(",");

      etl::to_string(avgDisplay, uartStr, etl::format_spec().precision(6),
                     true);
      uartStr.append(",");

      etl::to_string(droppedFrames, uartStr, etl::format_spec().precision(6),
                     true);
      uartStr.append(",");

      etl::to_string(self->totalFramesCompleted, uartStr,
                     etl::format_spec().precision(6), true);
      uartStr.append(",");

      etl::to_string(duration, uartStr, etl::format_spec().precision(6), true);
      uartStr.append(",");

      etl::to_string(self->totalFramesCompleted / duration, uartStr,
                     etl::format_spec().precision(6), true);
      uartStr.append(",");

      etl::to_string(FrameRateInfo.totalFrames, uartStr,
                     etl::format_spec().precision(6), true);
      uartStr.append(",");

      etl::to_string(FrameRateInfo.fps, uartStr,
                     etl::format_spec().precision(6), true);
      uartStr.append(",");

      etl::to_string(FrameRateInfo.frameMs, uartStr,
                     etl::format_spec().precision(6), true);
      uartStr.append(",");

      etl::to_string(FrameRateInfo.totalTExp, uartStr,
                     etl::format_spec().precision(6), true);
      uartStr.append(",\n");
    } else {
      uartStr.append("\n");
    }
    mu << uartStr.data();
  }
}
