/*#include "../../../include/fb.h"
#include "../../../include/miniuart.h"
#include "../../../include/timer.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/to_string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/algorithm.h"
#include "../../../lib/pl_mpeg/pl_mpeg.h"
#include "../../../include/filter.h"
#include "../../../soccerBytes.h"

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

//etl::vector<etl::array<uint64_t, 4>,1000> ttr = {};
uint64_t ttr[1000][4];
unsigned long total_frames_completed;
Timer timer = Timer();

void createApp(video_app *self);
void updateFrame(plm_t *player, plm_frame_t *frame, void *user);
void updateVideo(video_app *self);
void destroy(video_app *self);


int main() {

  fb_init();
  print("1",5,5);
  video_app app;
  video_app *app_ptr = &app;

  print("2",5,5);
  etl::fill_n(reinterpret_cast<char *>(app_ptr), sizeof(video_app), 0);

  print("3",5,5);
  plm_t plm_holder;
  plm_t *plm_ptr = &plm_holder;
  etl::fill_n(reinterpret_cast<char *>(plm_ptr), sizeof(plm_t), 0);

  print("4",5,5);
  uint8_t *soccer_bytes = &soccer[0];


  app_ptr->plm = plm_create_with_memory(soccer_bytes,soccer_sz,0,plm_ptr);
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
    uint64_t ttrr = times[3] - times[2];
    uint64_t ttd = times[3] - times[0];
    total_rgb_t += ttrgb;
    total_filter_t += ttf;
    total_render_t += ttrr;
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
  //com::Filter::sobelEdgeDetect(self->rgb_data, N_PIXELS, frame->width * 3,
                               //new_rgb_data);
  com::Filter::grayscale(self->rgb_data, N_PIXELS, new_rgb_data);

  auto to_filter = timer.now();
  //TODO: display

  auto to_render = timer.now();
  ttr[total_frames_completed][0] = start_time;
  ttr[total_frames_completed][1] = to_rgb;
  ttr[total_frames_completed][2] = to_filter;
  ttr[total_frames_completed][3] = to_render;
  total_frames_completed++;
}

void updateVideo(video_app *self) {
  auto now = timer.now();
  uint64_t elapsed_time = timer.duration_since(self->last_time);

  if (elapsed_time >= frame_rate_info.frame_ms) {

      self->last_time = now;
      plm_decode(self->plm, timer.to_sec(elapsed_time));
    // if (elapsed_time > (frame_rate_info.frame_ms * 2)) {
    // if (seek_to != -1) {
    //   plm_seek(self->plm, seek_to, TRUE);

    //   self->last_time = now;
    //   plm_decode(self->plm, (frame_rate_info.frame_ms / 1000.0));
    // } else {
    //   self->last_time = now;
    //   plm_decode(self->plm, (frame_rate_info.frame_ms / 1000.0));
    // }
  }

  if (plm_has_ended(self->plm)) {
    self->wants_to_quit = true;
  }
}

void destroy(video_app *self) {
  plm_destroy(self->plm);
}*/

#include "../../../include/fb.h"
#include "../../../include/miniuart.h"
#include "../../../include/timer.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/to_string.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/etl_profile.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/algorithm.h"
#include "/usr/share/etl/etl-20.39.4/include/etl/vector.h"
#include <stdlib.h>
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
};

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

// // int plm_buffer_next_start_code(plm_buffer_t *self) {
// //   printN(61);
// //   plm_buffer_align(self);
// // printN(62);
// //   while (plm_buffer_has(self, (5 << 3))) {
// //     printN(63);
// //     size_t byte_index = (self->bit_index) >> 3;
// //     if (self->bytes[byte_index] == 0x00 &&
// //         self->bytes[byte_index + 1] == 0x00 &&
// //         self->bytes[byte_index + 2] == 0x01) {
// //           printN(64);
// //       self->bit_index = (byte_index + 4) << 3;
// //       printN(65);
// //       printN(sizeof(self->bytes));
// //       printN(byte_index);
// //       printN(self->bytes[byte_index + 3]);
// //       int ret_val = self->bytes[byte_index + 3];
// //       return ret_val;
// //     }
// //     printN(66);
// //     self->bit_index += 8;
// //   }
// //   printN(67);
// //   return -1;
// // }

// int plm_buffer_find_start_code(plm_buffer_t *self, int code) {
// printN(51);
//   int current = 0;
//   while (TRUE) {

// printN(52);
//       plm_buffer_align(self);

//   while (plm_buffer_has(self, (5 << 3))) {
//     size_t byte_index = (self->bit_index) >> 3;
//     if (self->bytes[byte_index] == 0x00 &&
//         self->bytes[byte_index + 1] == 0x00 &&
//         self->bytes[byte_index + 2] == 0x01) {
//       self->bit_index = (byte_index + 4) << 3;
//       current = self->bytes[byte_index + 3];
//     }
//     self->bit_index += 8;
//   }
//   current =  -1;
//     printN(current);

// printN(53);
//     if (current == code || current == -1) {

// printN(54);
//       return current;
//     }
//   }

// printN(55);
//   return -1;
// }

// plm_video_t *plm_video_create_with_buffer(plm_buffer_t *buffer,
//                                           int destroy_when_done) {
//                                             printN(40);
//   plm_video_t *self = &static_video_holder;
//   //memset(self, 0, sizeof(plm_video_t));
// printN(41);
//   self->buffer = buffer;
//   self->destroy_buffer_when_done = destroy_when_done;
// printN(42);
//   // Attempt to decode the sequence header
//   self->start_code =
//       plm_buffer_find_start_code(self->buffer, PLM_START_SEQUENCE);
//       printN(43);
//   if (self->start_code != -1) {
//     printN(44);
//     plm_video_decode_sequence_header(self);
//     printN(45);
//   }
//   printN(46);
//   return self;
// }


// int plm_init_decoders(plm_t *self) {
//   printN(21);
//   if (self->has_decoders) {
//     return TRUE;
//   }
// printN(22);
//   if (!plm_demux_has_headers(self->demux)) {
//     return FALSE;
//   }
//   printN(23);

//   if (plm_demux_get_num_video_streams(self->demux) > 0) {
//     printN(24);
//     if (self->video_enabled) {
//       self->video_packet_type = PLM_DEMUX_PACKET_VIDEO_1;
//     }
//     if (!self->video_decoder) {
//       printN(25);
//       self->video_buffer =
//           plm_buffer_create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);

//                                    printN(26);
//       plm_buffer_set_load_callback(self->video_buffer, plm_read_video_packet,
//                                    self);
//                                    printN(26);
//       self->video_decoder =
//           plm_video_create_with_buffer(self->video_buffer, TRUE);
//           printN(27);
//     }
//   }
//   printN(28);

//   if (plm_demux_get_num_audio_streams(self->demux) > 0) {
//     printN(29);
//     if (self->audio_enabled) {
//       self->audio_packet_type =
//           PLM_DEMUX_PACKET_AUDIO_1 + self->audio_stream_index;
//           printN(30);
//     }
//     printN(31);
//     if (!self->audio_decoder) {
//       printN(32);
//       self->audio_buffer =
//           plm_buffer_create_with_capacity(PLM_BUFFER_DEFAULT_SIZE);
//       printN(33);
//       plm_buffer_set_load_callback(self->audio_buffer, plm_read_audio_packet,
//                                    self);
//                                    printN(34);
//       self->audio_decoder =
//           plm_audio_create_with_buffer(self->audio_buffer, TRUE);
//     }
//     printN(35);
//   }
// printN(36);
//   self->has_decoders = TRUE;
//   printN(37);
//   return TRUE;
// }

// void plm_create_with_buffer(plm_buffer_t *buffer, int destroy_when_done,
//                             plm_t *self) {
//                               printN(10);
//   self->demux = plm_demux_create(buffer, destroy_when_done);
//   printN(11);
//   self->video_enabled = TRUE;
//   self->audio_enabled = TRUE;
//   printN(12);
//   plm_init_decoders(self);
// }

// plm_t *plm_create_with_memory(uint8_t *bytes, size_t length, int free_when_done,
//                               plm_t *self_ptr) {
//   printN(2);//gets here
//   plm_buffer_t *buffer =
//       plm_buffer_create_with_memory(bytes, length, free_when_done);

//   printN(3);
//   plm_create_with_buffer(buffer, TRUE, self_ptr);

//   printN(4);
//   return self_ptr;
// }
//   plm_t plm_holder = {
//     .demux = NULL,
//     .time = 0.0,
//     .has_ended = 0,
//     .loop = 0,
//     .has_decoders = 0,

//     .video_enabled = 0,
//     .video_packet_type = 0,
//     .video_buffer = NULL,
//     .video_decoder = NULL,

//     .audio_enabled = 0,
//     .audio_stream_index = 0,
//     .audio_packet_type = 0,
//     .audio_lead_time = 0.0,
//     .audio_buffer = NULL,
//     .audio_decoder = NULL,

//     .video_decode_callback = NULL,
//     .video_decode_callback_user_data = NULL,

//     .audio_decode_callback = NULL,
//     .audio_decode_callback_user_data = NULL
// };

//   video_app app = {// init lists and memset break why???
//     .plm = NULL,
//     .wants_to_quit = false,
//     .last_time = 0,
//     .rgb_data = {0}, 
//     .win_height = 0,
//     .win_width = 0
//   };

plm_t plm_holder;
video_app app;
int main() {
  MiniUart mu = MiniUart();
  Timer t = Timer();
  etl::string<15> hello_str = "Hello world!\n";
  mu.init();
  mu.writeText(hello_str.data());
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
  printN(5); //last thing working 17:29

  //plm_set_video_decode_callback(app_ptr->plm, updateFrame, self);
  plm_set_loop(app_ptr->plm, FALSE);  // loop video
  plm_set_audio_enabled(app_ptr->plm, FALSE);

  double fps = plm_get_framerate(app_ptr->plm);
  double total_time_per_f  = plm_get_duration(app_ptr->plm);
  int total_frames = total_time_per_f * fps;
  double frame_ms =
      (1.0 / static_cast<double>(fps)) * 1000;
  double play_time = plm_get_duration(app_ptr->plm);

  etl::string<64> frame_stats = "Total frames: ";
  etl::to_string(total_frames, frame_stats,etl::format_spec().precision(6),true);
  drawString(400, 10, frame_stats.data(), 0x0f);
  etl::string<64> fps_stats = "FPS: ";
  etl::to_string(fps, fps_stats,etl::format_spec().precision(6),true);
  drawString(400, 20, fps_stats.data(), 0x0f);
  etl::string<64> framt = "Max frame time ms: ";
  etl::to_string(frame_ms, framt,etl::format_spec().precision(6),true);
  drawString(400, 30, framt.data(), 0x0f);
  etl::string<64> plt = "Correct play time sec: ";
  etl::to_string(play_time, plt,etl::format_spec().precision(6),true);
  drawString(400, 40, plt.data(), 0x0f);
  // while(1){
  //   ;
  // }
  while (1) {
    mu.writeText(hello_str);
    uint64_t time = t.now();
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
    /*etl::string<100> text = "The result is ";
    etl::to_string(fullDuration/1000000, text, etl::format_spec().precision(6),true);
    drawString(100,260, text.data(), 0x0f);*/
    ldiv_t division_result = ldiv(fullDuration, 1000000);
    double res = division_result.quot + ((double)division_result.rem / (double)1000000);
    etl::string<100> text = "The result is ";
    etl::to_string(res, text, etl::format_spec().precision(6),true);
    drawString(100,260, text.data(), 0x0f);
  }
}
