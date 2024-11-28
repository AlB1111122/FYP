#include "../../../include/fb.h"
#include "../../../include/miniuart.h"
#include "../../../include/timer.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/to_string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/algorithm.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/vector.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/array.h"
#include "../../../lib/pl_mpeg/pl_mpeg.h"
#include "../../../include/filter.h"

static int nPrints=0;

void print(etl::string<400> string,int x, int y) {
  drawString(x, y, string.data(), 0x0f);
}

#define WIN_HEIGHT 720
#define WIN_WIDTH 1280
#define N_PIXELS (WIN_WIDTH * WIN_HEIGHT * 3)

struct frame_rate_info {
  double frame_ms;
  int fps;
  int total_frames;
  double total_t;
} frame_rate_info;

struct video_app {
  plm_t *plm;
  bool wants_to_quit;
  uint64_t last_time;
  uint8_t rgb_data[WIN_WIDTH * WIN_HEIGHT * 3];
  int win_height;
  int win_width;
};

etl::vector<etl::array<uint64_t, 4>,1000> ttr = {};
unsigned long total_frames_completed;
Timer timer = Timer();

void createApp(video_app *self);
void updateFrame(plm_t *player, plm_frame_t *frame, void *user);
void updateVideo(video_app *self);
void destroy(video_app *self);


int main(int argc, char **argv) {
  if (argc < 2) {
    print("Run with pl_mpeg_player path/<file.mpg>",5,5);
    return 1;
  }
  video_app app;
  video_app *app_ptr = &app;
  etl::fill_n(reinterpret_cast<char *>(app_ptr), sizeof(video_app), 0);

  plm_t plm_holder;
  plm_t *plm_ptr = &plm_holder;
  etl::fill_n(reinterpret_cast<char *>(plm_ptr), sizeof(plm_t), 0);

  //app_ptr->plm = plm_create_with_memory(argv[1], plm_ptr); TODO: replace with memory
  if (!app_ptr->plm) {
    print("Couldn't open video",5,25);
    return 1;
  }

  createApp(app_ptr);
  auto start = timer.now();

  app_ptr->last_time = start;
  while (!app_ptr->wants_to_quit) {
    updateVideo(app_ptr);
  }
  auto finish = timer.now();

  uint64_t duration = finish - start;
  print("render times:\n",5,45);
  double total_rgb_t, total_filter_t, total_render_t, total_display_t = 0;
  int dropped_frames = 0;
  for (auto &times : ttr) {
    uint64_t ttrgb = times[1] - times[0];
    uint64_t ttf = times[2] - times[1];
    uint64_t ttr = times[3] - times[2];
    uint64_t ttd = times[3] - times[0];
    total_rgb_t += ttrgb;
    total_filter_t += ttf;
    total_render_t += ttr;
    total_display_t += ttd;

    if (ttd > frame_rate_info.frame_ms) {
      dropped_frames++;
    }
  }
  etl::string<400> display_times = "average to rgb: ";
  etl::to_string(total_rgb_t / total_frames_completed, display_times,true);
  etl::to_string("ms\naverage to filtered: ", display_times,true);
  etl::to_string(total_filter_t / total_frames_completed, display_times,true);
  etl::to_string("ms\naverage to rendered: ", display_times,true);
  etl::to_string(total_render_t / total_frames_completed, display_times,true);
  etl::to_string("ms\naverage total time to display: ", display_times,true);
  etl::to_string(total_display_t / total_frames_completed, display_times,true);
  print(display_times,5,120);


  etl::string<400> general_stats = "ms\n\ntotal slow frames:";
  etl::to_string(dropped_frames, general_stats,true);
  etl::to_string("\ntotal callbacks to render frames: ", general_stats,true);
  etl::to_string(total_frames_completed, general_stats,true);
  etl::to_string("\ntotal play time: ", general_stats,true);
  etl::to_string(duration, general_stats,true);
  etl::to_string("sec\nFPS: ", general_stats,true);
  etl::to_string(total_frames_completed / duration, general_stats,true);

  destroy(app_ptr);
}

void createApp(video_app *self) {
  int samplerate = plm_get_samplerate(self->plm);
  plm_set_video_decode_callback(self->plm, updateFrame, self);

  plm_set_loop(self->plm, FALSE);  // loop video
  plm_set_audio_enabled(self->plm, FALSE);

  frame_rate_info.fps = plm_get_framerate(self->plm);
  frame_rate_info.total_t = plm_get_duration(self->plm);
  frame_rate_info.total_frames = frame_rate_info.total_t * frame_rate_info.fps;
  // get the amount of time a frame can be displayed in without being dropped in
  // ms
  frame_rate_info.frame_ms =
      (1.0 / static_cast<double>(frame_rate_info.fps)) * 1000;

  // frame number true for intersection video

  etl::string<400> frame_stats = "Total frames: ";
  etl::to_string(frame_rate_info.total_frames, frame_stats,true);
  etl::to_string("\nFPS: ", frame_stats,true);
  etl::to_string(frame_rate_info.fps, frame_stats,true);
  etl::to_string("\nMax frame time ms: ", frame_stats,true);
  etl::to_string(frame_rate_info.frame_ms, frame_stats,true);
  etl::to_string("\nCorrect play time sec: ", frame_stats,true);
  etl::to_string(frame_rate_info.total_t , frame_stats,true);
}


void updateFrame(plm_t *mpeg, plm_frame_t *frame, void *user) {
  auto start_time = timer.now();
  video_app *self = static_cast<video_app *>(user);
  plm_frame_to_rgb(frame, self->rgb_data,
                   frame->width * 3);  // can be hardware accelerated]

  auto to_rgb = timer.now();
  uint8_t new_rgb_data[N_PIXELS];
  /*com::Filter::sobelEdgeDetect(self->rgb_data, N_PIXELS, frame->width * 3,
                               new_rgb_data);*/
  com::Filter::grayscale(self->rgb_data, N_PIXELS, new_rgb_data);

  auto to_filter = timer.now();
  //TODO: display

  auto to_render = timer.now();
  ttr.push_back({start_time, to_rgb, to_filter, to_render});
  total_frames_completed++;
}

void updateVideo(video_app *self) {
  auto now = timer.now();
  uint64_t elapsed_time = timer.duration_since(self->last_time);

  if (elapsed_time >= frame_rate_info.frame_ms) {

      self->last_time = now;
      plm_decode(self->plm, timer.to_sec(elapsed_time));
    /*if (elapsed_time > (frame_rate_info.frame_ms * 2)) {
    if (seek_to != -1) {
      plm_seek(self->plm, seek_to, TRUE);

      self->last_time = now;
      plm_decode(self->plm, (frame_rate_info.frame_ms / 1000.0));
    } else {
      self->last_time = now;
      plm_decode(self->plm, (frame_rate_info.frame_ms / 1000.0));
    }*/
  }

  if (plm_has_ended(self->plm)) {
    self->wants_to_quit = true;
  }
}

void destroy(video_app *self) {
  plm_destroy(self->plm);
}

  /*MiniUart mu = MiniUart();
  Timer t = Timer();
  etl::string<15> hello_str = "Hello world!\n";
  mu.init();
  mu.writeText(hello_str.data());

  fb_init();

  drawString(100, 30, hello_str.data(), 0x0f);
  while (1) {
    mu.writeText(hello_str);
    uint64_t time = t.now();
    printN(t.to_sec(time), 50);

    etl::string<32> n_str;
    etl::to_string(time, n_str);
    uint64_t duration_since = t.duration_since(time);
    printN(t.to_sec(duration_since), 70);

    auto secTime = t.get_hertz();
    etl::string<32> sec_str;
    etl::to_string(secTime, sec_str);
    printN(secTime,130);
    while (t.duration_since(time) < 10000000) {
      ;
    }
    int b = 4000 / 1000;
    printN(b, 150);
    while (t.duration_since(time) < 15000000) {
      ;
    }
    int c = 54382589 / 1000;
    printN(c, 180);
    while (t.duration_since(time) < 20000000) {
      ;
    }
    int d = 1 / 1000;
    printN(d, 200);
    while (t.duration_since(time) < 25000000) {
      ;
    }
    auto a = t.to_sec(time);
    printN(a, 220);
    while (t.duration_since(time) < 30000000) {
      ;
    }
    auto m = t.to_milli(time);
    printN(m, 240);
    uint64_t fullDuration = t.duration_since(time);
    ldiv_t division_result = ldiv(fullDuration, 1000000);
    double res = division_result.quot + ((double)division_result.rem / (double)1000000);
    etl::string<100> text = "The result is ";
    etl::to_string(res, text, etl::format_spec().precision(6),true);
    drawString(100,260, text.data(), 0x0f);
  }*/