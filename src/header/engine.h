#pragma once
#include "../header/tween.h"
#include <SDL3/SDL.h>
#include <initializer_list>
#include <memory>
#include <string>
using namespace std;

struct gui_properties {
  float position_x = 50.0f;
  float position_y = 50.0f;
  float size_x = 0.0f;
  float size_y = 0.0f;
  float anchor_x = 0.5f;
  float anchor_y = 0.5f;
  float scale = 1.0f;
  bool visible = true;
  SDL_Color background_color = {0, 0, 0, 255};
  float background_transparency = 1.0f;
  SDL_Color border_color = {255, 255, 255, 255};
  float border_transparency = 1.0f;
  float border_size = 0.0f;
};

struct text_properties {
  float position_x = 50.0f;
  float position_y = 50.0f;
  float size_x = 0.0f;
  float size_y = 0.0f;
  float anchor_x = 0.5f;
  float anchor_y = 0.5f;
  float scale = 1.0f;
  bool visible = true;
  SDL_Color background_color = {0, 0, 0, 255};
  float background_transparency = 1.0f;
  SDL_Color border_color = {255, 255, 255, 255};
  float border_transparency = 1.0f;
  float border_size = 0.0f;
  string text = "";
  float text_size = 24.0f;
  SDL_Color text_color = {255, 255, 255, 255};
  float text_transparency = 0.0f;
};

struct image_properties {
  float position_x = 50.0f;
  float position_y = 50.0f;
  float size_x = 0.0f;
  float size_y = 0.0f;
  float anchor_x = 0.5f;
  float anchor_y = 0.5f;
  float scale = 1.0f;
  bool visible = true;
  SDL_Color background_color = {0, 0, 0, 255};
  float background_transparency = 1.0f;
  SDL_Color border_color = {255, 255, 255, 255};
  float border_transparency = 1.0f;
  float border_size = 0.0f;
  string image = "";
  SDL_Color image_color = {255, 255, 255, 255};
  float image_transparency = 0.0f;
};

struct button_properties {
  float position_x = 50.0f;
  float position_y = 50.0f;
  float size_x = 0.0f;
  float size_y = 0.0f;
  float anchor_x = 0.5f;
  float anchor_y = 0.5f;
  float scale = 1.0f;
  bool visible = true;
  SDL_Color background_color = {0, 0, 0, 255};
  float background_transparency = 0.5f;
  SDL_Color border_color = {255, 255, 255, 255};
  float border_transparency = 1.0f;
  float border_size = 0.0f;
  string text = "";
  float text_size = 24.0f;
  SDL_Color text_color = {255, 255, 255, 255};
  float text_transparency = 0.0f;
  float padding_x = 18.0f;
  float padding_y = 10.0f;
  bool auto_button_color = true;
  SDL_Color hover_color = {52, 52, 62, 255};
  float hover_transparency = 0.10f;
};

struct image_button_properties {
  float position_x = 50.0f;
  float position_y = 50.0f;
  float size_x = 0.0f;
  float size_y = 0.0f;
  float anchor_x = 0.5f;
  float anchor_y = 0.5f;
  float scale = 1.0f;
  bool visible = true;
  SDL_Color background_color = {0, 0, 0, 255};
  float background_transparency = 1.0f;
  SDL_Color border_color = {255, 255, 255, 255};
  float border_transparency = 1.0f;
  float border_size = 0.0f;
  string image = "";
  SDL_Color image_color = {255, 255, 255, 255};
  float image_transparency = 0.0f;
  bool auto_button_color = true;
  SDL_Color hover_color = {255, 255, 255, 255};
  float hover_transparency = 0.35f;
  float hover_size = 1.0f;
};

struct panel_properties {
  float position_x = 50.0f;
  float position_y = 50.0f;
  float size_x = 100.0f;
  float size_y = 100.0f;
  float anchor_x = 0.5f;
  float anchor_y = 0.5f;
  float scale = 1.0f;
  bool visible = true;
  SDL_Color background_color = {0, 0, 0, 255};
  float background_transparency = 0.5f;
  SDL_Color border_color = {255, 255, 255, 255};
  float border_transparency = 1.0f;
  float border_size = 0.0f;
};

template <typename properties_type>
struct gui_handle {
  shared_ptr<properties_type> property_data;
  properties_type &properties;
  gui_handle() : property_data(make_shared<properties_type>()), properties(*property_data) {}
  gui_handle(const gui_handle &other) : property_data(other.property_data), properties(*property_data) {}
  gui_handle &operator=(const gui_handle &other) {
    if (this != &other) {
      *property_data = *other.property_data;
    }
    return *this;
  }
};

struct image : gui_handle<image_properties> {
  struct impl;
  shared_ptr<impl> data;
  void render(SDL_Renderer *renderer);
  void destroy();
};

struct text : gui_handle<text_properties> {
  struct impl;
  shared_ptr<impl> data;
  void render(SDL_Renderer *renderer);
  void destroy();
};

struct button_state {
  bool clicked = false;
  bool click_held = false;
  bool click_held_leave = false;
  bool m_enter = false;
  bool m_leave = false;
  bool m2_clicked = false;
  bool m2_held = false;
  bool m3_clicked = false;
  bool m3_held = false;
  bool m_scroll_up = false;
  bool m_scroll_down = false;
};

struct button : gui_handle<button_properties>, button_state {
  struct impl;
  shared_ptr<impl> data;
  void render(SDL_Renderer *renderer);
  void destroy();
};

struct image_button : gui_handle<image_button_properties>, button_state {
  struct impl;
  shared_ptr<impl> data;
  void render(SDL_Renderer *renderer);
  void destroy();
};

struct panel : gui_handle<panel_properties> {
  struct impl;
  shared_ptr<impl> data;
  void render(SDL_Renderer *renderer);
  void destroy();
};

void play_sound(const string &name, bool looped = false, int pitch = 100, int volume = 100);
void set_asset_base_path(const string &path);
void fetch_mouse_scroll(float x, float y);
void clear_mouse_scroll();
void render_frame(SDL_Renderer *renderer, float sizex, float sizey, float x, float y, SDL_Color colour);
image render_image(SDL_Renderer *renderer, const string &name);
text render_text(SDL_Renderer *renderer, const string &content, float size);
button render_button(SDL_Renderer *renderer, const string &content, float size);
image_button render_image_button(SDL_Renderer *renderer, const string &name);
panel render_panel(SDL_Renderer *renderer);
tween create_tween(image target, const tween_info &info, initializer_list<tween_property> goals);
tween create_tween(text target, const tween_info &info, initializer_list<tween_property> goals);
tween create_tween(button target, const tween_info &info, initializer_list<tween_property> goals);
tween create_tween(image_button target, const tween_info &info, initializer_list<tween_property> goals);
tween create_tween(panel target, const tween_info &info, initializer_list<tween_property> goals);
