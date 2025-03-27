#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include "../../../include/FrameBuffer.h"
#include "../../../include/filter.h"
#include "../../../include/memCtrl.h"
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
  bool wants_to_quit;
  uint64_t last_time;
  uint8_t rgb_data[WIN_WIDTH * WIN_HEIGHT * 4];
  int win_height;
  int win_width;
  uint64_t ttr[400][5];
  int total_frames_completed = 0;
  uint64_t between_update_video_loops[400];
  FrameBuffer *fb_ptr;
  unsigned char *lastBuffer;
};

struct frame_rate_info {
  double frame_ms;
  double fps;
  int total_frames;
  double total_t_exp;
} frame_rate_info;

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
uint8_t f_rgb_data[N_PIXELS];
uint8_t new_rgb_data[N_PIXELS];

extern "C" void full_cache_clean();

void updateFrame(plm_t *mpeg, plm_frame_t *frame, void *user) {
  uint64_t start_time = Timer::now();
  video_app *self = static_cast<video_app *>(user);
  plm_frame_to_rgba(frame, new_rgb_data,
                    self->fb_ptr->getPitch());  // can be hardware accelerated
  uint64_t to_rgb = Timer::now();

  // com::Filter::sobelEdgeDetect(new_rgb_data, N_PIXELS, frame->width * 4,
  //                              self->fb_ptr->getFb());
  // com::Filter::grayscale(new_rgb_data, N_PIXELS, f_rgb_data);
  uint64_t to_filtered = Timer::now();
  memcpy(self->fb_ptr->getFb(), new_rgb_data, N_PIXELS);

  uint64_t to_rendered = Timer::now();
  self->ttr[self->total_frames_completed][0] = self->last_time;
  self->ttr[self->total_frames_completed][1] = start_time;
  self->ttr[self->total_frames_completed][2] = to_rgb;
  self->ttr[self->total_frames_completed][3] = to_filtered;
  self->ttr[self->total_frames_completed][4] = to_rendered;
  self->total_frames_completed++;

  full_cache_clean();  // this works but shouldnt need to clean whole cache
  __asm__ volatile("dmb st" ::: "memory");
}

void updateVideo(video_app *self, Timer &t) {
  auto now = Timer::now();
  uint64_t elapsed_time = t.to_milli(t.duration_since(self->last_time));

  self->between_update_video_loops[self->total_frames_completed] = elapsed_time;
  self->last_time = now;
  plm_decode(self->plm, (frame_rate_info.frame_ms / 1000.0));

  if (plm_has_ended(self->plm)) {
    self->wants_to_quit = true;
  }
}

void make_stat_file(uint64_t start_time, video_app *self, Timer &t,
                    MiniUart &mu, FrameBuffer &fb);

extern "C" int getEl();

plm_t plm_holder;
video_app app;
int main() {
  MiniUart mu = MiniUart();
  Timer t = Timer();
  FrameBuffer fb = FrameBuffer();
  etl::string<15> hello_str = "check\n";
  mu.init();
  app.fb_ptr = &fb;
  auto waiter = Timer::now();
  while (t.duration_since(waiter) < 1500000) {  // wait 1.5 sec
    ;
  }

  printN(getEl(), fb);  // should be 1 not 2 which it boots to automatically
  mu.writeText(hello_str);

  video_app *app_ptr = &app;

  plm_t *plm_ptr = &plm_holder;
  printN(plm_holder.loop, fb);
  mu.writeText(hello_str);

  app_ptr->plm = plm_create_with_memory(soccer, soccer_sz, 0, plm_ptr);

  mu.writeText(hello_str);

  plm_set_video_decode_callback(app_ptr->plm, updateFrame, app_ptr);
  plm_set_loop(app_ptr->plm, FALSE);  // loop video
  plm_set_audio_enabled(app_ptr->plm, FALSE);

  frame_rate_info.fps = plm_get_framerate(app_ptr->plm);
  frame_rate_info.total_t_exp = plm_get_duration(app_ptr->plm);
  frame_rate_info.total_frames =
      frame_rate_info.total_t_exp * frame_rate_info.fps;
  frame_rate_info.frame_ms =
      (1.0 / static_cast<double>(frame_rate_info.fps)) * 1000;

  etl::string<64> frame_stats = "Total frames: ";
  etl::to_string(frame_rate_info.total_frames, frame_stats,
                 etl::format_spec().precision(6), true);
  fb.drawString(400, 10, frame_stats.data(), 0x0f);
  etl::string<64> fps_stats = "FPS: ";
  etl::to_string(frame_rate_info.fps, fps_stats,
                 etl::format_spec().precision(6), true);
  fb.drawString(400, 20, fps_stats.data(), 0x0f);
  etl::string<64> framt = "Max frame time ms: ";
  etl::to_string(frame_rate_info.frame_ms, framt,
                 etl::format_spec().precision(6), true);
  fb.drawString(400, 30, framt.data(), 0x0f);
  etl::string<64> plt = "Correct play time sec: ";
  etl::to_string(frame_rate_info.total_t_exp, plt,
                 etl::format_spec().precision(6), true);
  fb.drawString(400, 40, plt.data(), 0x0f);
  mu.writeText("\n");

  uint64_t start = Timer::now();
  app_ptr->last_time = start;

  etl::string<64> cmpol = "";
  etl::to_string(app_ptr->total_frames_completed, cmpol,
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

  // && (app_ptr->total_frames_completed < 7)
  while ((!app_ptr->wants_to_quit)) {
    updateVideo(app_ptr, t);
  }
  mu.writeText("\n");
  make_stat_file(start, app_ptr, t, mu, fb);
}

void make_stat_file(uint64_t start_time, video_app *self, Timer &t,
                    MiniUart &mu, FrameBuffer &fb) {
  double duration = t.to_sec(t.duration_since(start_time));
  double avg_rgb_t = 0, avg_filter_t = 0, avg_render_t = 0, avg_display_t = 0,
         avg_plm_d_t = 0;

  double durations[400][5];
  int dropped_frames = 0;
  for (int i = 0; i < self->total_frames_completed; i++) {
    double ttplmd = t.to_milli(self->ttr[i][1] - self->ttr[i][0]);
    double ttrgb = t.to_milli(self->ttr[i][2] - self->ttr[i][1]);
    double ttf = t.to_milli(self->ttr[i][3] - self->ttr[i][2]);
    double ttr = t.to_milli(self->ttr[i][4] - self->ttr[i][3]);
    double ttd = t.to_milli(self->ttr[i][4] - self->ttr[i][0]);
    durations[i][0] = ttplmd;
    durations[i][1] = ttrgb;
    durations[i][2] = ttf;
    durations[i][3] = ttr;
    durations[i][4] = ttd;
    avg_plm_d_t += ttplmd / (i + 1);
    avg_rgb_t += ttrgb / (i + 1);
    avg_filter_t += ttf / (i + 1);
    avg_render_t += ttr / (i + 1);
    avg_display_t += ttd / (i + 1);

    if (ttd > frame_rate_info.frame_ms) {
      dropped_frames++;
    }
  }

  mu << "decode(ms),convert_rgb(ms),filter(ms),display(ms)"
        ",time_in_callback(ms),"
     << "avg_decoded(ms),avg_rgb(ms),avg_filtered(ms),avg_rendered(ms),"
     << "avg_total_time_to_display(ms),total_slow_frames,total_callbacks,"
     << "real_play_time(sec),actual_fps,total_video_frames,default_fps,max_"
        "frame_time(ms),correct_play_time(sec)\n";
  for (int i = 0; i < self->total_frames_completed; i++) {
    etl::string<510> uart_str = "";

    etl::to_string(durations[i][0], uart_str, etl::format_spec().precision(6),
                   true);
    uart_str.append(",");

    etl::to_string(durations[i][1], uart_str, etl::format_spec().precision(6),
                   true);
    uart_str.append(",");

    etl::to_string(durations[i][2], uart_str, etl::format_spec().precision(6),
                   true);
    uart_str.append(",");

    etl::to_string(durations[i][3], uart_str, etl::format_spec().precision(6),
                   true);
    uart_str.append(",");

    etl::to_string(durations[i][4], uart_str, etl::format_spec().precision(6),
                   true);
    uart_str.append(",");

    if (i < 1) {
      etl::to_string(avg_plm_d_t, uart_str, etl::format_spec().precision(6),
                     true);
      uart_str.append(",");

      etl::to_string(avg_rgb_t, uart_str, etl::format_spec().precision(6),
                     true);
      uart_str.append(",");

      etl::to_string(avg_filter_t, uart_str, etl::format_spec().precision(6),
                     true);
      uart_str.append(",");

      etl::to_string(avg_render_t, uart_str, etl::format_spec().precision(6),
                     true);
      uart_str.append(",");

      etl::to_string(avg_display_t, uart_str, etl::format_spec().precision(6),
                     true);
      uart_str.append(",");

      etl::to_string(dropped_frames, uart_str, etl::format_spec().precision(6),
                     true);
      uart_str.append(",");

      etl::to_string(self->total_frames_completed, uart_str,
                     etl::format_spec().precision(6), true);
      uart_str.append(",");

      etl::to_string(duration, uart_str, etl::format_spec().precision(6), true);
      uart_str.append(",");

      etl::to_string(self->total_frames_completed / duration, uart_str,
                     etl::format_spec().precision(6), true);
      uart_str.append(",");

      etl::to_string(frame_rate_info.total_frames, uart_str,
                     etl::format_spec().precision(6), true);
      uart_str.append(",");

      etl::to_string(frame_rate_info.fps, uart_str,
                     etl::format_spec().precision(6), true);
      uart_str.append(",");

      etl::to_string(frame_rate_info.frame_ms, uart_str,
                     etl::format_spec().precision(6), true);
      uart_str.append(",");

      etl::to_string(frame_rate_info.total_t_exp, uart_str,
                     etl::format_spec().precision(6), true);
      uart_str.append(",\n");
    } else {
      uart_str.append("\n");
    }
    mu << uart_str.data();
  }
}