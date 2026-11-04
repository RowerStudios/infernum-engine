#include <SDL3/SDL.h>
#include <SDL3/SDL_loadso.h>
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdio>
#include <iostream>
#include <string>
using namespace std;

#if defined(_WIN32)
static const char *scene_ext = ".dll";
#elif defined(__APPLE__)
static const char *scene_ext = ".dylib";
#else
static const char *scene_ext = ".so";
#endif

SDL_Renderer *renderer;
SDL_Window *window;
SDL_SharedObject *scene_so = nullptr;
void (*main_scene)(SDL_Renderer *) = nullptr;
bool debug_enabled = false;
TTF_Font *debug_font = nullptr;
int stat_frames = 0;
int stat_fps = 0;
float stat_ms = 0;
Uint64 stat_frame_time = 0;
Uint64 stat_second_time = 0;

void fetch_window(SDL_Window *fetched_window) {
  if (!fetched_window) {
    printf(">> could not fetch window .. ERROR\n");
  }
  window = fetched_window;
}

void fetch_renderer(SDL_Renderer *fetched_renderer) {
  if (!fetched_renderer) {
    printf(">> could not fetch renderer.. ERROR\n");
  }
  renderer = fetched_renderer;
}

void load_scene(string name) {
  string base_path = SDL_GetBasePath();
  if (base_path.empty()) {
    printf(">> cannot find base path.. ERROR\n");
    return;
  }
  string folder_path = base_path + "scenes/" + name + "/" + name + scene_ext;
  scene_so = SDL_LoadObject(folder_path.c_str());
  if (!scene_so) {
    string flat_path = base_path + "scenes/" + name + scene_ext;
    scene_so = SDL_LoadObject(flat_path.c_str());
    if (!scene_so) {
      printf(">> cannot load scene: %s .. WARNING\n", name.c_str());
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      return;
    }
  }
  main_scene =
      (void (*)(SDL_Renderer *))SDL_LoadFunction(scene_so, "main_scene");
  if (!main_scene) {
    printf(">> scene has no main_scene function.. ERROR\n");
    SDL_UnloadObject(scene_so);
    scene_so = nullptr;
  }
}

void run_scene(SDL_Renderer *scene_renderer) {
  if (!scene_renderer) {
    printf(">> scene has no renderer.. ERROR\n");
    return;
  }
  if (main_scene) {
    main_scene(scene_renderer);
  } else {
    SDL_SetRenderDrawColor(scene_renderer, 0, 0, 0, 255);
    SDL_RenderClear(scene_renderer);
  }
}

void unload_scene() {
  if (scene_so) {
    SDL_UnloadObject(scene_so);
    scene_so = nullptr;
  }
  main_scene = nullptr;
}

void show_debug(bool show) {
  debug_enabled = show;
  if (debug_enabled) {
    printf(">> stats toggled on..\n");
  } else {
    printf(">> stats toggled off..\n");
  }
}

void update_stats() {
  Uint64 now = SDL_GetPerformanceCounter();
  if (stat_second_time == 0) {
    stat_second_time = now;
    stat_frame_time = now;
    return;
  }
  float frame_ms = (float)(now - stat_frame_time) * 1000.0f /
                   (float)SDL_GetPerformanceFrequency();
  stat_ms = stat_ms * 0.9f + frame_ms * 0.1f;
  stat_frame_time = now;
  stat_frames++;
  if (now - stat_second_time >= SDL_GetPerformanceFrequency()) {
    stat_fps = stat_frames;
    stat_frames = 0;
    stat_second_time = now;
  }
}

bool init_stats(SDL_Renderer *stats_renderer) {
  if (!stats_renderer) {
    printf(">> stats need a renderer.. ERROR\n");
    return false;
  }
  if (!TTF_Init()) {
    printf(">> cannot init ttf.. ERROR\n");
    return false;
  }
  string font_path = SDL_GetBasePath();
  font_path += "fonts/debug.ttf";
  debug_font = TTF_OpenFont(font_path.c_str(), 16.0f);
  if (!debug_font) {
    printf(">> cannot open debug font.. ERROR\n");
    TTF_Quit();
    return false;
  }
  SDL_SetRenderDrawBlendMode(stats_renderer, SDL_BLENDMODE_BLEND);
  printf(">> stats menu ready..\n");
  return true;
}

void draw_stats(SDL_Renderer *stats_renderer) {
  if (!debug_enabled) {
    return;
  }
  if (!stats_renderer || !debug_font) {
    return;
  }
  int stat_window_w = 0;
  int stat_window_h = 0;
  SDL_GetCurrentRenderOutputSize(stats_renderer, &stat_window_w,
                                 &stat_window_h);
  string stat_lines[] = {
      "inferno debug",
      "fps: " + to_string(stat_fps),
      "frame: " + to_string(stat_ms) + " ms",
      "window: " + to_string(stat_window_w) + "x" + to_string(stat_window_h),
      "gpu: " + string(SDL_GetRendererName(stats_renderer)),
  };
  float stat_line_h = (float)TTF_GetFontHeight(debug_font) + 1.0f;
  SDL_Color stat_color = {255, 255, 255, 255};
  SDL_Texture *stat_textures[5];
  SDL_FRect stat_rects[5];
  float stat_max_w = 0;
  int stat_count = 0;
  for (const string &stat_line : stat_lines) {
    stat_textures[stat_count] = nullptr;
    stat_rects[stat_count] = {0.0f, 0.0f, 0.0f, 0.0f};
    SDL_Surface *stat_surface = TTF_RenderText_Blended(
        debug_font, stat_line.c_str(), stat_line.size(), stat_color);
    if (stat_surface) {
      stat_textures[stat_count] =
          SDL_CreateTextureFromSurface(stats_renderer, stat_surface);
      float stat_w = (float)stat_surface->w;
      float stat_h = (float)stat_surface->h;
      SDL_DestroySurface(stat_surface);
      if (stat_textures[stat_count]) {
        stat_rects[stat_count] = {8.0f, 8.0f + (float)stat_count * stat_line_h,
                                  stat_w, stat_h};
        if (stat_w > stat_max_w) {
          stat_max_w = stat_w;
        }
      }
    }
    stat_count++;
  }
  SDL_FRect stat_bg = {0.0f, 0.0f, stat_max_w + 16.0f,
                       stat_line_h * (float)stat_count + 8.0f};
  SDL_SetRenderDrawColor(stats_renderer, 0, 0, 0, 150);
  SDL_RenderFillRect(stats_renderer, &stat_bg);
  for (int stat_i = 0; stat_i < stat_count; stat_i++) {
    if (stat_textures[stat_i]) {
      SDL_RenderTexture(stats_renderer, stat_textures[stat_i], nullptr,
                        &stat_rects[stat_i]);
      SDL_DestroyTexture(stat_textures[stat_i]);
    }
  }
}

void shutdown_stats() {
  if (debug_font) {
    TTF_CloseFont(debug_font);
    debug_font = nullptr;
  }
  TTF_Quit();
}
