#include "../header/engine.h"
#include "../header/game.h"
#include "../header/sounds.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <json/json.h>
#include <string>
using namespace std;

// vars//
const char *title = "Inferno";
int w = 800; // window width
int h = 600; // window height
bool is_running = true;
SDL_Event event;
bool debug_shown = false;
bool is_fullscreen = false;
bool is_borderless = false;

int main() {
  printf(">> 🔥 Infernum engine initializing.. 🔥\n");
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
    printf(">> sdl cannot be started.. ERROR\n");
    return 1;
  }
  string version;
  const char *base_path = SDL_GetBasePath();
  if (base_path) {
    ifstream ifs(string(base_path) + "VERSION");
    version.assign((std::istreambuf_iterator<char>(ifs)),
                   std::istreambuf_iterator<char>());
  }
  printf("%s\n", version.c_str());
  printf(">> initializing sdl window..\n");
  printf(">> sdl started..\n");
  printf(">> booting up sdl window..\n");
  SDL_Window *window = SDL_CreateWindow(title, w, h, SDL_WINDOW_OPENGL);
  if (!window) {
    printf(">> sdl window cannot be created.. ERROR\n");
    SDL_Quit();
    return 1;
  }
  printf(">> sdl window created..\n");
  printf(">> creating opengl renderer..\n");
  SDL_Renderer *renderer = SDL_CreateRenderer(window, nullptr);
  if (!renderer) {
    printf(">> cannot create a opengl renderer.. ERROR\n");
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  printf(">> fetching renderer to engine..\n");
  fetch_window(window);
  fetch_renderer(renderer);
  printf(">> initializing stats menu..\n");
  init_stats(renderer);
  printf(">> loading main scene..\n");
  load_scene("main_scene");
  printf(">> initializing audio engine..\n");
  if (!init_audio()) {
    printf(">> audio engine failed to initialize.. ERROR\n");
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  printf(">> audio engine initialized..\n");
  printf(">> starting game loop..\n");
  while (is_running) {
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        printf(">> exit signal detected..\n");
        is_running = false;
      }
      if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F3 &&
          !event.key.repeat) {
        debug_shown = !debug_shown;
        show_debug(debug_shown);
      }
      if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F11 &&
          !event.key.repeat) {
        if (event.key.mod & SDL_KMOD_SHIFT) {
          is_borderless = !is_borderless;
          if (is_fullscreen) {
            is_fullscreen = false;
            SDL_SetWindowFullscreen(window, false);
          }
          SDL_SetWindowBordered(window, !is_borderless);
          printf(">> borderless %s..\n", is_borderless ? "on" : "off");
        } else {
          is_fullscreen = !is_fullscreen;
          SDL_SetWindowFullscreen(window, is_fullscreen);
          printf(">> fullscreen %s..\n", is_fullscreen ? "on" : "off");
        }
      }
      if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        fetch_mouse_scroll(event.wheel.x, event.wheel.y);
      }
    }
    // game loop
    run_scene(renderer);
    clear_mouse_scroll();
    update_stats();
    draw_stats(renderer);
    SDL_RenderPresent(renderer);
    update_audio();
  }

  printf(">> shutting down scene..\n");
  unload_scene();
  printf(">> shutting down stats..\n");
  shutdown_stats();
  printf(">> shutting down audio..\n");
  shutdown_audio();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  printf(">> program quitted fully..\n");
  return 0;
}
