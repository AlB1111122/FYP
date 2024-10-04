extern "C" {
#include "../include/pl_mpeg/pl_mpeg.h"
}
#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <iostream>

// change to be dynamic (plm_get_width(self->plm);) if changing video
#define WIN_HEIGHT 720
#define WIN_WIDTH 1280

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

void start_app(video_app *self);
void app_on_video(plm_t *player, plm_frame_t *frame, void *user);
void app_update(video_app *self);
void app_destroy(video_app *self);