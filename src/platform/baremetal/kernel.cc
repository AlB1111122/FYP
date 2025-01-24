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
    uint64_t between_update_video_loops[400];
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
  self->ttr[self->total_frames_completed][0] = self->last_time;
  self->ttr[self->total_frames_completed][1] = start_time;
  self->ttr[self->total_frames_completed][2] = to_rgb;
  self->ttr[self->total_frames_completed][3] = to_filtered;
  self->ttr[self->total_frames_completed][4] = to_rendered;
  self->total_frames_completed++;
}

void updateVideo(video_app *self, Timer& t) {
  auto now = Timer::now();
  uint64_t elapsed_time = t.to_milli(t.duration_since(self->last_time));

  if (elapsed_time >= frame_rate_info.frame_ms) {
    self->between_update_video_loops[self->total_frames_completed] = elapsed_time;
      self->last_time = now;
      plm_decode(self->plm, (frame_rate_info.frame_ms / 1000.0));
      printN(self->total_frames_completed);
  }

  if (plm_has_ended(self->plm) || self->total_frames_completed > 5) {
    self->wants_to_quit = true;
  }
}

void make_stat_file(uint64_t start_time, video_app *self, Timer& t){

  uint64_t duration = t.duration_since(start_time);
  //std::cout << "render times:\n";
  uint64_t total_rgb_t, total_filter_t, total_render_t, total_display_t, plm_d_t = 0;

  //std::vector<std::array<double, 5>> durations = {};
  uint64_t durations[400][5];
  int dropped_frames = 0;
  for (int i = 0;i < self->total_frames_completed;i++) {
    uint64_t ttplmd = self->ttr[i][1] - self->ttr[i][0];
    uint64_t ttrgb = self->ttr[i][2] - self->ttr[i][1];
    uint64_t ttf = self->ttr[i][3] - self->ttr[i][2];
    uint64_t ttr = self->ttr[i][4] - self->ttr[i][3];
    uint64_t ttd = self->ttr[i][4] - self->ttr[i][0];
    self->ttr[i][0] = ttplmd;
    self->ttr[i][1] = ttrgb;
    self->ttr[i][2] = ttf;
    self->ttr[i][3] = ttr;
    self->ttr[i][4] = ttd;
    plm_d_t += ttplmd;
    total_rgb_t += ttrgb;
    total_filter_t += ttf;
    total_render_t += ttr;
    total_display_t += ttd;

    if (ttd > frame_rate_info.frame_ms) {
      dropped_frames++;
    }
    
  }
    drawString(300, 100, "between_loops", 0x0f);
    for(int i =1; i <= self->total_frames_completed; i++){
      etl::string<100> n_str;
      etl::to_string(plm_d_t / self->between_update_video_loops[i], n_str, etl::format_spec().precision(6),false);
      drawString(300, 100 + (i*10),n_str.data(), 0x0f);
    }
    drawString(600, 100, "decode", 0x0f);
    for(int i =1; i <= self->total_frames_completed; i++){
      etl::string<100> n_str;
      etl::to_string(plm_d_t / durations[i][0], n_str, etl::format_spec().precision(6),false);
      drawString(600, 100 + (i*10), n_str.data(), 0x0f);
    }
    drawString(800, 100, "convert_rgb", 0x0f);
    for(int i =1; i <= self->total_frames_completed; i++){
      etl::string<800> n_str;
      etl::to_string(plm_d_t / durations[i][1], n_str, etl::format_spec().precision(6),false);
      drawString(500, 100 + (i*10), n_str.data(), 0x0f);
    }
    drawString(1000, 100, "filter", 0x0f);
    for(int i =1; i <= self->total_frames_completed; i++){
      etl::string<100> n_str;
      etl::to_string(plm_d_t / durations[i][2], n_str, etl::format_spec().precision(6),false);
      drawString(1000, 100 + (i*10), n_str.data(), 0x0f);
    }
    drawString(1200, 100, "display", 0x0f);
    for(int i =1; i <= self->total_frames_completed; i++){
      etl::string<100> n_str;
      etl::to_string(plm_d_t / durations[i][3], n_str, etl::format_spec().precision(6),false);
      drawString(1200, 100 + (i*10), n_str.data(), 0x0f);
    }
    drawString(1400, 800, "callback_time", 0x0f);
    for(int i =1; i <= self->total_frames_completed; i++){
      etl::string<100> n_str;
      etl::to_string(plm_d_t / durations[i][4], n_str, etl::format_spec().precision(6),false);
      drawString(1400, 100 + (i*10), n_str.data(), 0x0f);
    }
    drawString(300, 00, "avg_to_decoded", 0x0f);
    etl::string<100> n_str;
    etl::to_string(plm_d_t / self->total_frames_completed, n_str, etl::format_spec().precision(6),false);
    drawString(300, 810, n_str.data(), 0x0f);
    
    drawString(500, 800, "avg_to_rgb", 0x0f);
    etl::string<100> n_stra;
    etl::to_string(total_rgb_t / self->total_frames_completed, n_stra, etl::format_spec().precision(6),false);
    drawString(500, 810, n_str.data(), 0x0f);
    
    drawString(700, 800, "avg_to_filtered", 0x0f);
    etl::string<100> n_strb;
    etl::to_string(total_filter_t / self->total_frames_completed, n_strb, etl::format_spec().precision(6),false);
    drawString(700, 810, n_str.data(), 0x0f);
    
    drawString(900, 800, "total_render_t", 0x0f);
    etl::string<100> n_strc;
    etl::to_string(total_render_t / self->total_frames_completed, n_strc, etl::format_spec().precision(6),false);
    drawString(900, 810, n_str.data(), 0x0f);
}

plm_t plm_holder;
video_app app;
int main() {
  MiniUart mu = MiniUart();
  Timer t = Timer();
  etl::string<15> hello_str = "Hello world!\n";
  mu.init();
  fb_init();
  
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

  // etl::string<64> frame_stats = "Total frames: ";
  // etl::to_string(frame_rate_info.total_frames, frame_stats,etl::format_spec().precision(6),true);
  // drawString(400, 10, frame_stats.data(), 0x0f);
  // etl::string<64> fps_stats = "FPS: ";
  // etl::to_string(frame_rate_info.fps, fps_stats,etl::format_spec().precision(6),true);
  // drawString(400, 20, fps_stats.data(), 0x0f);
  // etl::string<64> framt = "Max frame time ms: ";
  // etl::to_string(frame_rate_info.frame_ms, framt,etl::format_spec().precision(6),true);
  // drawString(400, 30, framt.data(), 0x0f);
  // etl::string<64> plt = "Correct play time sec: ";
  // etl::to_string(frame_rate_info.total_t_exp, plt,etl::format_spec().precision(6),true);
  // drawString(400, 40, plt.data(), 0x0f);

  //app created

  uint64_t start = Timer::now();
  app_ptr->last_time = start;
  
  while (!app_ptr->wants_to_quit) {
    updateVideo(app_ptr, t);
  }
  make_stat_file(start, app_ptr, t);

//void make_stat_file(uint64_t start_time, video_app *self, Timer& t){
}
