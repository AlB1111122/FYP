#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "../../../include/FrameBuffer.h"
#include "../../../include/filter.h"
#include "../../../include/miniuart.h"
#include "../../../include/timer.h"
#include "../../../lib/pl_mpeg/pl_mpeg.h"
#include "../../../soccerBytes.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/algorithm.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/etl_profile.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/to_string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/vector.h"

#define WIN_HEIGHT 720
#define WIN_WIDTH 1280
#define N_PIXELS (WIN_WIDTH * WIN_HEIGHT * 4)

struct video_app {
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

struct FrameRateInfo_t {
  double frameMs;
  double fps;
  int totalFrames;
  double totalTExp;
} FrameRateInfo_t;

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
uint8_t fRgbData[N_PIXELS] __attribute__((aligned(64)));
uint8_t newRgbData[N_PIXELS] __attribute__((aligned(64)));

void updateFrame(plm_t *mpeg, plm_frame_t *frame, void *user) {
  uint64_t startTime = Timer::now();
  video_app *self = static_cast<video_app *>(user);
  plm_frame_to_rgba(frame, newRgbData,
                    self->fbPtr->getPitch());  // can be hardware accelerated
  uint64_t toRgb = Timer::now();

  com::Filter::sobelEdgeDetect(newRgbData, N_PIXELS, frame->width * 4,
                               fRgbData);
  // com::Filter::grayscale(newRgbData, N_PIXELS, fRgbData);
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

void updateVideo(video_app *self, Timer &t) {
  auto now = Timer::now();
  uint64_t elapsedTime = t.toMilli(t.durationSince(self->lastTime));
  self->lastTime = now;
  plm_decode(self->plm, (FrameRateInfo_t.frameMs / 1000.0));

  if (plm_has_ended(self->plm)) {
    self->wantsToQuit = true;
  }
}

void makeStatFile(uint64_t startTime, video_app *self, Timer &t, MiniUart &mu,
                  FrameBuffer &fb);

extern "C" int getEl();

plm_t plmHolder;
video_app app;
int main() {
  MiniUart mu = MiniUart();
  Timer t = Timer();
  FrameBuffer fb = FrameBuffer();
  etl::string<15> helloStr = "check\n";
  mu.init();
  app.fbPtr = &fb;
  auto waiter = Timer::now();
  while (t.durationSince(waiter) < 1500000) {  // wait 1.5 sec
    ;
  }

  printN(getEl(), fb);  // should be 1 not 2 which it boots to automatically
  mu.writeText(helloStr);

  video_app *appPtr = &app;

  plm_t *plmPtr = &plmHolder;
  printN(plmHolder.loop, fb);
  mu.writeText(helloStr);

  appPtr->plm = plm_create_with_memory(soccer, soccer_sz, 0, plmPtr);

  mu.writeText(helloStr);

  plm_set_video_decode_callback(appPtr->plm, updateFrame, appPtr);
  plm_set_loop(appPtr->plm, FALSE);  // loop video
  plm_set_audio_enabled(appPtr->plm, FALSE);

  FrameRateInfo_t.fps = plm_get_framerate(appPtr->plm);
  FrameRateInfo_t.totalTExp = plm_get_duration(appPtr->plm);
  FrameRateInfo_t.totalFrames = FrameRateInfo_t.totalTExp * FrameRateInfo_t.fps;
  FrameRateInfo_t.frameMs =
      (1.0 / static_cast<double>(FrameRateInfo_t.fps)) * 1000;

  etl::string<64> frameStats = "Total frames: ";
  etl::to_string(FrameRateInfo_t.totalFrames, frameStats,
                 etl::format_spec().precision(6), true);
  fb.drawString(400, 10, frameStats.data(), 0x0f);
  etl::string<64> fpsStats = "FPS: ";
  etl::to_string(FrameRateInfo_t.fps, fpsStats, etl::format_spec().precision(6),
                 true);
  fb.drawString(400, 20, fpsStats.data(), 0x0f);
  etl::string<64> framt = "Max frame time ms: ";
  etl::to_string(FrameRateInfo_t.frameMs, framt,
                 etl::format_spec().precision(6), true);
  fb.drawString(400, 30, framt.data(), 0x0f);
  etl::string<64> plt = "Correct play time sec: ";
  etl::to_string(FrameRateInfo_t.totalTExp, plt,
                 etl::format_spec().precision(6), true);
  fb.drawString(400, 40, plt.data(), 0x0f);
  mu.writeText("\n");

  uint64_t start = Timer::now();
  appPtr->lastTime = start;

  etl::string<64> cmpol = "";
  etl::to_string(appPtr->totalFramesCompleted, cmpol,
                 etl::format_spec().precision(6), true);
  uint32_t fbAddress =
      static_cast<uint32_t>(reinterpret_cast<uintptr_t>(fb.getFb()));
  uint32_t offFbAddress =
      static_cast<uint32_t>(reinterpret_cast<uintptr_t>(fb.getOffFb()));

  etl::string<64> fbs = "";
  etl::to_string(fbAddress, fbs, etl::format_spec().precision(6), true);

  etl::string<64> ofbs = "";
  etl::to_string(offFbAddress, ofbs, etl::format_spec().precision(6), true);
  cmpol.append(" ");
  cmpol.append(fbs);
  cmpol.append(" ");
  cmpol.append(ofbs);
  cmpol.append("\n");
  mu.writeText(cmpol);
  mu.writeText("check again \n");

  // && (appPtr->totalFramesCompleted < 7) && (appPtr->totalFramesCompleted <
  // 30)
  while ((!appPtr->wantsToQuit)) {
    updateVideo(appPtr, t);
  }
  mu.writeText("\n");
  makeStatFile(start, appPtr, t, mu, fb);
}

void makeStatFile(uint64_t startTime, video_app *self, Timer &t, MiniUart &mu,
                  FrameBuffer &fb) {
  double duration = t.toSec(t.durationSince(startTime));
  double avgRgb = 0, avgFilter = 0, avgRender = 0, avgDisplay = 0,
         avgPlmDec = 0;

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

    if (ttd > FrameRateInfo_t.frameMs) {
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

      etl::to_string(FrameRateInfo_t.totalFrames, uartStr,
                     etl::format_spec().precision(6), true);
      uartStr.append(",");

      etl::to_string(FrameRateInfo_t.fps, uartStr,
                     etl::format_spec().precision(6), true);
      uartStr.append(",");

      etl::to_string(FrameRateInfo_t.frameMs, uartStr,
                     etl::format_spec().precision(6), true);
      uartStr.append(",");

      etl::to_string(FrameRateInfo_t.totalTExp, uartStr,
                     etl::format_spec().precision(6), true);
      uartStr.append(",\n");
    } else {
      uartStr.append("\n");
    }
    mu << uartStr.data();
  }
}