extern "C" {
#include "../../../lib/pl_mpeg/pl_mpeg.h"
}
#include <SDL2/SDL.h>
#include <unistd.h>

#include <array>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <vector>
#include <iostream>
#include <fstream>

#include "../../../include/filter.h"

#define WIN_HEIGHT 720
#define WIN_WIDTH 1280
#define N_PIXELS (WIN_WIDTH * WIN_HEIGHT * 3)

struct frame_rate_info {
  double frame_ms;
  int fps;
  int total_frames;
  double total_t;
} frame_rate_info;

std::vector<std::array<std::chrono::system_clock::time_point, 5>> ttr = {};
std::vector<double> between_update_video_loops = {};
unsigned long total_frames_completed;

struct video_app {
  plm_t *plm;
  bool wants_to_quit;
  uint8_t rgb_data[WIN_WIDTH * WIN_HEIGHT * 3];
  SDL_Texture *texture_rgb;
  SDL_Window *window;
  SDL_Renderer *renderer;
  int win_height;
  int win_width;
};

std::chrono::time_point<std::chrono::system_clock> last_time;

void createApp(video_app *self);
void updateFrame(plm_t *player, plm_frame_t *frame, void *user);
void updateVideo(video_app *self);
void destroy(video_app *self);
void make_stat_file(std::chrono::system_clock::time_point start);

int main(int argc, char **argv) {
  if (argc < 2) {
    std::cout << "Run with pl_mpeg_player path/<file.mpg>";
    return 1;
  }
  video_app app;
  video_app *app_ptr = &app;
  std::fill_n(reinterpret_cast<char *>(app_ptr), sizeof(video_app), 0);

  plm_t plm_holder;
  plm_t *plm_ptr = &plm_holder;
  std::fill_n(reinterpret_cast<char *>(plm_ptr), sizeof(plm_t), 0);

  app_ptr->plm = plm_create_with_filename(argv[1], plm_ptr);
  if (!app_ptr->plm) {
    std::cout << "Couldn't open " << argv[1];
    return 1;
  }

  createApp(app_ptr);
  auto start = std::chrono::system_clock::now();

  last_time = start;
  while (!app_ptr->wants_to_quit) {
    updateVideo(app_ptr);
  }
  
  make_stat_file(start);
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
  std::cout << frame_rate_info.total_frames << " total frames, "
            << frame_rate_info.fps
            << "FPS, max frame time ms: " << frame_rate_info.frame_ms << "\n";
  std::cout << "correct play time sec: " << frame_rate_info.total_t << "\n";

  self->window = SDL_CreateWindow("pl_mpeg", SDL_WINDOWPOS_CENTERED,
                                  SDL_WINDOWPOS_CENTERED, WIN_WIDTH, WIN_HEIGHT,
                                  SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

  self->renderer = SDL_CreateRenderer(self->window, -1, 0);
  if (!self->renderer) {
    std::cout << "Error: Couldn't create renderer. SDL_Error: \n"
              << SDL_GetError();
    self->wants_to_quit = true;
  }

  self->texture_rgb =
      SDL_CreateTexture(self->renderer, SDL_PIXELFORMAT_RGB24,
                        SDL_TEXTUREACCESS_STREAMING, WIN_WIDTH, WIN_HEIGHT);
}

void updateFrame(plm_t *mpeg, plm_frame_t *frame, void *user) {
  auto start_time = std::chrono::high_resolution_clock::now();
  video_app *self = static_cast<video_app *>(user);
  plm_frame_to_rgb(frame, self->rgb_data,
                   frame->width * 3);  // can be hardware accelerated]

  auto to_rgb = std::chrono::high_resolution_clock::now();
  uint8_t new_rgb_data[N_PIXELS];
  //com::Filter::sobelEdgeDetect(self->rgb_data, N_PIXELS, frame->width * 3,
                               //new_rgb_data);
  com::Filter::grayscale(self->rgb_data, N_PIXELS, new_rgb_data);

  auto to_filter = std::chrono::high_resolution_clock::now();
  SDL_UpdateTexture(self->texture_rgb, NULL, new_rgb_data, frame->width * 3);
  SDL_RenderClear(self->renderer);
  SDL_RenderCopy(self->renderer, self->texture_rgb, NULL, NULL);
  SDL_RenderPresent(self->renderer);
  auto to_render = std::chrono::high_resolution_clock::now();
  ttr.push_back({last_time,start_time, to_rgb, to_filter, to_render});
  total_frames_completed++;
}

void updateVideo(video_app *self) {
  auto plm_time = plm_get_time(self->plm);
  if (plm_time >= frame_rate_info.total_t) {
    self->wants_to_quit = true;
  }
  auto now = std::chrono::system_clock::now();

  std::chrono::duration<double, std::milli> elapsed_tp = now - last_time;
  double elapsed_time = elapsed_tp.count();

  if (elapsed_time >= frame_rate_info.frame_ms) {
    between_update_video_loops.push_back(elapsed_time);
      last_time = now;
      plm_decode(self->plm, (frame_rate_info.frame_ms / 1000.0));
  }

  if (plm_has_ended(self->plm)) {
    self->wants_to_quit = true;
  }

  SDL_Event ev;
  while (SDL_PollEvent(&ev)) {
    if (ev.type == SDL_QUIT ||
        (ev.type == SDL_KEYUP && ev.key.keysym.sym == SDLK_ESCAPE)) {
      self->wants_to_quit = true;
    }
  }
}

void destroy(video_app *self) {
  SDL_DestroyTexture(self->texture_rgb);
  SDL_DestroyRenderer(self->renderer);
  SDL_DestroyWindow(self->window);
  SDL_Quit();
  plm_destroy(self->plm);
}

void make_stat_file(std::chrono::system_clock::time_point start){
  std::string filename = "results.csv";
  std::ofstream file(filename);

  if (!file.is_open()) {
    std::cerr << "Failed to open file for writing: " << filename << std::endl;
    return;
  }

  auto finish = std::chrono::system_clock::now();

  std::chrono::duration<double> duration = finish - start;
  std::cout << "render times:\n";
  double total_rgb_t, total_filter_t, total_render_t, total_display_t, plm_d_t;

  std::vector<std::array<double, 5>> durations = {};
  int dropped_frames = 0;
  for (auto &times : ttr) {
    std::chrono::duration<double, std::milli> ttplmd = times[1] - times[0];
    std::chrono::duration<double, std::milli> ttrgb = times[2] - times[1];
    std::chrono::duration<double, std::milli> ttf = times[3] - times[2];
    std::chrono::duration<double, std::milli> ttr = times[4] - times[3];
    std::chrono::duration<double, std::milli> ttd = times[4] - times[0];
    durations.push_back({ttplmd.count(),ttrgb.count(),ttf.count(),ttr.count(),ttd.count()});
    plm_d_t += ttplmd.count();
    total_rgb_t += ttrgb.count();
    total_filter_t += ttf.count();
    total_render_t += ttr.count();
    total_display_t += ttd.count();

    if (ttd.count() > frame_rate_info.frame_ms) {
      dropped_frames++;
    }
    
}   //headers
    file << "between_update_video_loops,decode,convert to rgb,filter,display,total time in plm callback,"
          << "avg_to_decoded,avg_to_rgb,avg_to_filtered,avg_to_rendered,"
          << "avg_total_time_to_display,total_slow_frames,total_callbacks,"
          << "total_play_time,actual_fps,total_video_frames,default_fps,max_frame_time(ms),correct_play_time\n";
    
    for (int i =0; i< total_frames_completed; i++) {
        file << between_update_video_loops[i]<<","
            << durations[i][0] << ","
             << durations[i][1] << ","
             << durations[i][2] << ","
             << durations[i][3] << ","
             << durations[i][4];
             if(i < 1){
              file << plm_d_t / total_frames_completed << ","
              << total_rgb_t / total_frames_completed << ","
              << total_filter_t / total_frames_completed << ","
              << total_render_t / total_frames_completed << ","
              << total_display_t / total_frames_completed << ","
              << dropped_frames << ","
              << total_frames_completed << ","
              << duration.count() << ","
              << total_frames_completed / duration.count() << ","
              << frame_rate_info.total_frames<<","
              << frame_rate_info.fps <<","
              << frame_rate_info.frame_ms<<","
              << frame_rate_info.total_t <<"\n";
             }else{
              file << std::endl;
             }
    }
    file.close();
    std::cout << "Data written to " << filename << std::endl;
}
