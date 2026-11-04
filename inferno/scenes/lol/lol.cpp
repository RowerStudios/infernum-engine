#include "lol.h"
#include "engine.h"
#include "ui.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <cstdio>

static ui_scene scene_ui;
static bool ui_ready = false;

void main_scene(SDL_Renderer *scene_renderer) {
  if (!scene_renderer) {
    printf(">> scene has no renderer.. ERROR\n");
    return;
  }
  if (!ui_ready) {
    scene_ui.load(scene_renderer, "game_properties.json", "lol");
    ui_ready = true;
  }
  SDL_Color background = {12, 12, 16, 255};
  render_frame(scene_renderer, 100.0f, 100.0f, 50.0f, 50.0f, background);
  scene_ui.render(scene_renderer);
}
