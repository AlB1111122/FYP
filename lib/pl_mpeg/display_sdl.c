#include "pl_mpeg.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_pixels.h>
#include <SDL2/SDL_render.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
  plm_t *plm;
  int wants_to_quit;
  double last_time;
  uint8_t *rgb_data;
  SDL_Texture *texture_rgb;
  SDL_Window *window;
  SDL_Renderer *renderer;
  int win_height;
  int win_width;
} video_app;

void start_app(video_app *self);
void app_on_video(plm_t *player, plm_frame_t *frame, void *user);
void app_update(video_app *self);
void app_destroy(video_app *self);

int main(int argc, char **argv) {

  if (argc < 2) {
    printf("Run with pl_mpeg_player path/<file.mpg>");
    exit(1);
  }
  video_app app;
  video_app *app_ptr = &app;
  memset(app_ptr, 0, sizeof(video_app));

  plm_t plm_holder;
  plm_t *plm_ptr = &plm_holder;
  memset(plm_ptr, 0, sizeof(plm_t));

  app_ptr->plm = plm_create_with_filename(argv[1], plm_ptr);
  if (!app_ptr->plm) {
    printf("Couldn't open %s", argv[1]);
    exit(1);
  }
  start_app(app_ptr);
  while (!app_ptr->wants_to_quit) {
    app_update(app_ptr);
  }
}

void start_app(video_app *self) {
  int samplerate = plm_get_samplerate(self->plm);
  plm_set_video_decode_callback(self->plm, app_on_video, self);

  plm_set_loop(self->plm, TRUE);
  plm_set_audio_enabled(self->plm, FALSE);

  self->win_width = plm_get_width(self->plm);
  self->win_height = plm_get_height(self->plm);

  self->window = SDL_CreateWindow(
      "pl_mpeg", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      self->win_width, self->win_height, SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);

  self->renderer = SDL_CreateRenderer(self->window, -1, 0);
  if (!self->renderer) {
    printf("Error: Couldn't create renderer. SDL_Error: %s\n", SDL_GetError());
    exit(1);
  }

  self->texture_rgb = SDL_CreateTexture(self->renderer, SDL_PIXELFORMAT_RGB24,
                                        SDL_TEXTUREACCESS_STREAMING,
                                        self->win_width, self->win_height);
  int num_pixels = self->win_width * self->win_height;
  self->rgb_data = (uint8_t *)malloc(num_pixels * 3);
}

void app_on_video(plm_t *mpeg, plm_frame_t *frame, void *user) {
  video_app *self = (video_app *)user;
  plm_frame_to_rgb(frame, self->rgb_data,
                   frame->width * 3); // can be hardware accelerated
  SDL_UpdateTexture(self->texture_rgb, NULL, self->rgb_data, frame->width * 3);
  SDL_RenderClear(self->renderer);
  SDL_RenderCopy(self->renderer, self->texture_rgb, NULL, NULL);
  SDL_RenderPresent(self->renderer);
}

void app_update(video_app *self) {
  double seek_to = -1;

  SDL_Event ev;
  while (SDL_PollEvent(&ev)) {
    if (ev.type == SDL_QUIT ||
        (ev.type == SDL_KEYUP && ev.key.keysym.sym == SDLK_ESCAPE)) {
      self->wants_to_quit = TRUE;
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
    self->wants_to_quit = TRUE;
  }
}

void app_destroy(video_app *self) {
  SDL_DestroyTexture(self->texture_rgb);
  SDL_DestroyRenderer(self->renderer);
  SDL_DestroyWindow(self->window);
  SDL_Quit();
  plm_destroy(self->plm);
  free(self->rgb_data);
}
