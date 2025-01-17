#include "../../../include/fb.h"
#include "../../../include/miniuart.h"
#include "../../../include/timer.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/to_string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/etl_profile.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/algorithm.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/vector.h"
#include <stdlib.h>
#include <stdint.h>
#include "../../../lib/pl_mpeg/pl_mpeg.h"
#include "../../../soccerBytes.h"

static int nPrints=0;

#define WIN_HEIGHT 720
#define WIN_WIDTH 1280
#define N_PIXELS (WIN_WIDTH * WIN_HEIGHT * 3)

struct video_app {
    plm_t *plm;
    bool wants_to_quit;
    uint64_t last_time;
    uint8_t rgb_data[WIN_WIDTH * WIN_HEIGHT * 3];
    int win_height;
    int win_width;
    uint64_t ttr[400][4];
    int total_frames_completed=0;
};

struct frame_rate_info {
  double frame_ms;
  double fps;
  int total_frames;
  double total_t_exp;
} frame_rate_info;

template<typename T>
void printN(T time) {
  etl::string<100> i_str;
  etl::string<100> n_str;
  etl::to_string(nPrints, i_str);
  etl::to_string(time, n_str, etl::format_spec().precision(6),false);
  drawString(100, (nPrints*10), n_str.data(), 0x0f);
  drawString(200, (nPrints*10), i_str.data(), 0x0f);
  nPrints++;
}

void updateFrame(plm_t *mpeg, plm_frame_t *frame, void *user) {
  uint64_t start_time = Timer::now();
  video_app *self = static_cast<video_app *>(user);
  plm_frame_to_rgb(frame, self->rgb_data,
                   frame->width * 3);  // can be hardware accelerated]
  uint64_t to_rgb = Timer::now();

  uint8_t new_rgb_data[N_PIXELS];
  //com::Filter::sobelEdgeDetect(self->rgb_data, N_PIXELS, frame->width * 3,
                               //new_rgb_data);
  //com::Filter::grayscale(self->rgb_data, N_PIXELS, new_rgb_data);
  uint64_t to_filtered = Timer::now();

  //TODO: display
  uint64_t to_rendered = Timer::now();
  self->ttr[self->total_frames_completed][0] = start_time;
  self->ttr[self->total_frames_completed][1] = to_rgb;
  self->ttr[self->total_frames_completed][2] = to_filtered;
  self->ttr[self->total_frames_completed][3] = to_rendered;
  self->total_frames_completed++;
}

plm_t plm_holder;
video_app app;
int main() {
  MiniUart mu = MiniUart();
  Timer t = Timer();
  etl::string<15> hello_str = "Hello world!\n";
  mu.init();
  fb_init();
  
  //video_app app;
  video_app *app_ptr = &app;
  app_ptr->win_height = 4;
  printN(app_ptr->win_height);

  plm_t *plm_ptr = &plm_holder;
  printN(plm_holder.loop);

  uint8_t *soccer_bytes = &soccer[0];
  printN(1);
  app_ptr->plm = plm_create_with_memory(soccer_bytes,soccer_sz,0,plm_ptr);
  printN(5);

  plm_set_video_decode_callback(app_ptr->plm, updateFrame, app_ptr);
  plm_set_loop(app_ptr->plm, FALSE);  // loop video
  plm_set_audio_enabled(app_ptr->plm, FALSE);

  frame_rate_info.fps = plm_get_framerate(app_ptr->plm);
  frame_rate_info.total_t_exp = plm_get_duration(app_ptr->plm);
  frame_rate_info.total_frames = frame_rate_info.total_t_exp * frame_rate_info.fps;
  frame_rate_info.frame_ms = (1.0 / static_cast<double>(frame_rate_info.fps)) * 1000;

  etl::string<64> frame_stats = "Total frames: ";
  etl::to_string(frame_rate_info.total_frames, frame_stats,etl::format_spec().precision(6),true);
  drawString(400, 10, frame_stats.data(), 0x0f);
  etl::string<64> fps_stats = "FPS: ";
  etl::to_string(frame_rate_info.fps, fps_stats,etl::format_spec().precision(6),true);
  drawString(400, 20, fps_stats.data(), 0x0f);
  etl::string<64> framt = "Max frame time ms: ";
  etl::to_string(frame_rate_info.frame_ms, framt,etl::format_spec().precision(6),true);
  drawString(400, 30, framt.data(), 0x0f);
  etl::string<64> plt = "Correct play time sec: ";
  etl::to_string(frame_rate_info.total_t_exp, plt,etl::format_spec().precision(6),true);
  drawString(400, 40, plt.data(), 0x0f);
  while (1) {
    uint64_t time = Timer::now();
    printN(t.to_sec(time));

    etl::string<32> n_str;
    etl::to_string(time, n_str);
    uint64_t duration_since = t.duration_since(time);
    printN(t.to_sec(duration_since));

    auto secTime = t.get_hertz();
    etl::string<32> sec_str;
    etl::to_string(secTime, sec_str);
    printN(secTime);
    while (t.duration_since(time) < 10000000) {
      ;
    }
    int b = 4000 / 1000;
    printN(b);
    while (t.duration_since(time) < 15000000) {
      ;
    }
    int c = 54382589 / 1000;
    printN(c);
    while (t.duration_since(time) < 20000000) {
      ;
    }
    int d = 1 / 1000;
    printN(d);
    while (t.duration_since(time) < 25000000) {
      ;
    }
    auto a = t.to_sec(time);
    printN(a);
    while (t.duration_since(time) < 30000000) {
      ;
    }
    auto m = t.to_milli(time);
    printN(m);
    uint64_t fullDuration = t.duration_since(time);
    ldiv_t division_result = ldiv(fullDuration, 1000000);
    double res = division_result.quot + ((double)division_result.rem / (double)1000000);
    etl::string<100> text = "The result is ";
    etl::to_string(res, text, etl::format_spec().precision(6),true);
    drawString(100,260, text.data(), 0x0f);
  }
}
