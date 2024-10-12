extern "C" {
#include "../lib/pl_mpeg/pl_mpeg.h"
}
#include <SDL2/SDL.h>

#include <iostream>

#include "../include/filter.h"

// change to be dynamic (plm_get_width(self->plm);) if changing video
#define WIN_HEIGHT 720
#define WIN_WIDTH 1280
#define N_PIXELS (WIN_WIDTH * WIN_HEIGHT * 3)

struct video_app {
  plm_t *plm;
  bool wants_to_quit;
  double last_time;
  uint8_t rgb_data[WIN_WIDTH * WIN_HEIGHT * 3];
  SDL_Texture *texture_rgb;
  SDL_Window *window;
  SDL_Renderer *renderer;
  int win_height;
  int win_width;
};

void createApp(video_app *self);
void updateFrame(plm_t *player, plm_frame_t *frame, void *user);
void updateVideo(video_app *self);
void destroy(video_app *self);

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
  while (!app_ptr->wants_to_quit) {
    updateVideo(app_ptr);
  }

  destroy(app_ptr);
}

void createApp(video_app *self) {
  int samplerate = plm_get_samplerate(self->plm);
  plm_set_video_decode_callback(self->plm, updateFrame, self);

  plm_set_loop(self->plm, TRUE);  // loop video
  plm_set_audio_enabled(self->plm, FALSE);

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
  video_app *self = static_cast<video_app *>(user);
  plm_frame_to_rgb(frame, self->rgb_data,
                   frame->width * 3);  // can be hardware accelerated
  unv::Filter::grayscale(self->rgb_data, N_PIXELS);
  SDL_UpdateTexture(self->texture_rgb, NULL, self->rgb_data, frame->width * 3);
  SDL_RenderClear(self->renderer);
  SDL_RenderCopy(self->renderer, self->texture_rgb, NULL, NULL);
  SDL_RenderPresent(self->renderer);
}

void updateVideo(video_app *self) {
  double seek_to = -1;

  SDL_Event ev;
  while (SDL_PollEvent(&ev)) {
    if (ev.type == SDL_QUIT ||
        (ev.type == SDL_KEYUP && ev.key.keysym.sym == SDLK_ESCAPE)) {
      self->wants_to_quit = true;
    }
  }

  double current_time = (double)SDL_GetTicks() / 1000.0;
  double elapsed_time = current_time - self->last_time;
  if (elapsed_time > 1.0 / 30.0) {
    elapsed_time = 1.0 / 30.0;
  }
  self->last_time = current_time;

  if (seek_to != -1) {
    plm_seek(self->plm, seek_to, FALSE);
  } else {
    plm_decode(self->plm, elapsed_time);
  }

  if (plm_has_ended(self->plm)) {
    self->wants_to_quit = true;
  }
}

void destroy(video_app *self) {
  SDL_DestroyTexture(self->texture_rgb);
  SDL_DestroyRenderer(self->renderer);
  SDL_DestroyWindow(self->window);
  SDL_Quit();
  plm_destroy(self->plm);
}
