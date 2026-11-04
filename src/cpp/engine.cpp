#include "../header/engine.h"
#include "../header/sounds.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <cstdio>
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

static unordered_map<int, TTF_Font *> fonts;
static bool ttf_ready = false;
static string asset_base_path;

void set_asset_base_path(const string &path) {
  asset_base_path = path;
}

static const char *asset_base() {
  if (!asset_base_path.empty()) {
    return asset_base_path.c_str();
  }
  return SDL_GetBasePath();
}

struct drawable_data {
  SDL_Texture *texture = nullptr;
  float width = 0.0f;
  float height = 0.0f;
  ~drawable_data() {
    if (texture) {
      SDL_DestroyTexture(texture);
    }
  }
};

struct image::impl : drawable_data {
  string name;
};

struct text::impl : drawable_data {
  string content;
  float font_size = 0.0f;
};

struct button_input {
  bool was_inside = false;
  bool was_down = false;
  bool was_down_m2 = false;
  bool was_down_m3 = false;
  bool holding = false;
};

struct button::impl : button_input {
  text label;
};

struct image_button::impl : button_input {
  image picture;
};

struct panel::impl {};

template <typename properties_type>
static SDL_FRect property_rect(SDL_Renderer *renderer, const properties_type &properties, float width, float height) {
  int window_w = 0;
  int window_h = 0;
  SDL_GetCurrentRenderOutputSize(renderer, &window_w, &window_h);
  float scaled_w = width * properties.scale;
  float scaled_h = height * properties.scale;
  SDL_FRect rect = {window_w * properties.position_x / 100.0f - scaled_w * properties.anchor_x,
                    window_h * properties.position_y / 100.0f - scaled_h * properties.anchor_y,
                    scaled_w, scaled_h};
  return rect;
}

static bool mouse_inside(SDL_Renderer *renderer, const SDL_FRect &rect) {
  float window_x = 0.0f;
  float window_y = 0.0f;
  SDL_GetMouseState(&window_x, &window_y);
  float mouse_x = window_x;
  float mouse_y = window_y;
  SDL_RenderCoordinatesFromWindow(renderer, window_x, window_y, &mouse_x, &mouse_y);
  return mouse_x >= rect.x && mouse_x <= rect.x + rect.w &&
         mouse_y >= rect.y && mouse_y <= rect.y + rect.h;
}

static void draw_rect(SDL_Renderer *renderer, const SDL_FRect &rect, SDL_Color color, float transparency) {
  if (transparency >= 1.0f) {
    return;
  }
  Uint8 alpha = (Uint8)(255.0f * (1.0f - transparency));
  if (alpha == 0) {
    return;
  }
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
  SDL_RenderFillRect(renderer, &rect);
}

static void draw_border(SDL_Renderer *renderer, const SDL_FRect &rect, SDL_Color color, float transparency, float size) {
  if (size <= 0.0f || transparency >= 1.0f) {
    return;
  }
  Uint8 alpha = (Uint8)(255.0f * (1.0f - transparency));
  if (alpha == 0) {
    return;
  }
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, alpha);
  SDL_FRect border = rect;
  int thickness = (int)size;
  if (thickness < 1) {
    thickness = 1;
  }
  for (int i = 0; i < thickness; ++i) {
    SDL_RenderRect(renderer, &border);
    border.x += 1.0f;
    border.y += 1.0f;
    border.w -= 2.0f;
    border.h -= 2.0f;
    if (border.w <= 0.0f || border.h <= 0.0f) {
      break;
    }
  }
}

static void apply_texture_color(SDL_Texture *texture, SDL_Color color, float transparency) {
  Uint8 alpha = (Uint8)(255.0f * (1.0f - transparency));
  SDL_SetTextureColorMod(texture, color.r, color.g, color.b);
  SDL_SetTextureAlphaMod(texture, alpha);
}

static void reset_texture_color(SDL_Texture *texture) {
  SDL_SetTextureColorMod(texture, 255, 255, 255);
  SDL_SetTextureAlphaMod(texture, 255);
}

static float scroll_x = 0.0f;
static float scroll_y = 0.0f;
static bool scroll_seen = false;

void fetch_mouse_scroll(float x, float y) {
  scroll_x += x;
  scroll_y += y;
  scroll_seen = true;
}

void clear_mouse_scroll() {
  scroll_x = 0.0f;
  scroll_y = 0.0f;
  scroll_seen = false;
}

static void clear_button_state(button_state &state) {
  state.clicked = false;
  state.click_held = false;
  state.click_held_leave = false;
  state.m_enter = false;
  state.m_leave = false;
  state.m2_clicked = false;
  state.m2_held = false;
  state.m3_clicked = false;
  state.m3_held = false;
  state.m_scroll_up = false;
  state.m_scroll_down = false;
}

static void update_button_state(SDL_Renderer *renderer, const SDL_FRect &rect, button_input &input, button_state &state) {
  clear_button_state(state);
  if (!renderer) {
    return;
  }
  bool inside = mouse_inside(renderer, rect);
  SDL_MouseButtonFlags buttons = SDL_GetMouseState(nullptr, nullptr);
  bool down = (buttons & SDL_BUTTON_LMASK) != 0;
  bool down_m2 = (buttons & SDL_BUTTON_MMASK) != 0;
  bool down_m3 = (buttons & SDL_BUTTON_RMASK) != 0;
  bool released = !down && input.was_down;
  if (down && !input.was_down && inside) {
    input.holding = true;
  }
  state.m_enter = inside && !input.was_inside;
  state.m_leave = !inside && input.was_inside;
  state.clicked = down && !input.was_down && inside;
  state.click_held = down && input.holding;
  state.click_held_leave = released && input.holding;
  if (released) {
    input.holding = false;
  }
  state.m2_clicked = down_m2 && !input.was_down_m2 && inside;
  state.m2_held = down_m2 && inside;
  state.m3_clicked = down_m3 && !input.was_down_m3 && inside;
  state.m3_held = down_m3 && inside;
  state.m_scroll_up = inside && scroll_seen && scroll_y > 0.0f;
  state.m_scroll_down = inside && scroll_seen && scroll_y < 0.0f;
  input.was_inside = inside;
  input.was_down = down;
  input.was_down_m2 = down_m2;
  input.was_down_m3 = down_m3;
}

void play_sound(const string &name, bool looped, int pitch, int volume) {
  audio_play_sound(name, looped, pitch, volume);
}

void render_frame(SDL_Renderer *renderer, float sizex, float sizey, float x, float y, SDL_Color colour) {
  if (!renderer) {
    printf(">> render_frame has no renderer.. ERROR\n");
    return;
  }
  update_tweens();
  int window_w = 0;
  int window_h = 0;
  SDL_GetCurrentRenderOutputSize(renderer, &window_w, &window_h);
  float frame_w = window_w * sizex / 100.0f;
  float frame_h = window_h * sizey / 100.0f;
  SDL_FRect rect = {window_w * x / 100.0f - frame_w / 2.0f,
                    window_h * y / 100.0f - frame_h / 2.0f,
                    frame_w, frame_h};
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(renderer, colour.r, colour.g, colour.b, colour.a);
  SDL_RenderFillRect(renderer, &rect);
}

static TTF_Font *get_font(int size) {
  if (size < 1) {
    size = 1;
  }
  auto found = fonts.find(size);
  if (found != fonts.end()) {
    return found->second;
  }
  if (!ttf_ready) {
    if (!TTF_Init()) {
      printf(">> cannot init ttf.. ERROR\n");
      return nullptr;
    }
    ttf_ready = true;
  }
  const char *base_path = asset_base();
  if (!base_path) {
    printf(">> cannot find base path.. ERROR\n");
    return nullptr;
  }
  string font_path = base_path;
  font_path += "fonts/debug.ttf";
  TTF_Font *font = TTF_OpenFont(font_path.c_str(), (float)size);
  if (!font) {
    printf(">> cannot open engine font.. ERROR\n");
    return nullptr;
  }
  fonts.emplace(size, font);
  return font;
}

static bool ensure_text_texture(SDL_Renderer *renderer, text::impl &data, const text_properties &properties) {
  if (properties.text.empty()) {
    return false;
  }
  if (data.texture && data.content == properties.text && data.font_size == properties.text_size) {
    return true;
  }
  if (data.texture) {
    SDL_DestroyTexture(data.texture);
    data.texture = nullptr;
  }
  TTF_Font *font = get_font((int)properties.text_size);
  if (!font) {
    return false;
  }
  SDL_Color white = {255, 255, 255, 255};
  SDL_Surface *surface = TTF_RenderText_Blended(font, properties.text.c_str(), properties.text.size(), white);
  if (!surface) {
    printf(">> cannot render text surface.. ERROR\n");
    return false;
  }
  data.texture = SDL_CreateTextureFromSurface(renderer, surface);
  if (!data.texture) {
    printf(">> cannot create text texture.. ERROR\n");
    SDL_DestroySurface(surface);
    return false;
  }
  data.width = (float)surface->w;
  data.height = (float)surface->h;
  data.content = properties.text;
  data.font_size = properties.text_size;
  SDL_DestroySurface(surface);
  return true;
}

static bool ensure_image_texture(SDL_Renderer *renderer, image::impl &data, const image_properties &properties) {
  if (properties.image.empty()) {
    return false;
  }
  if (data.texture && data.name == properties.image) {
    return true;
  }
  if (data.texture) {
    SDL_DestroyTexture(data.texture);
    data.texture = nullptr;
  }
  const char *base_path = asset_base();
  if (!base_path) {
    printf(">> cannot find base path.. ERROR\n");
    return false;
  }
  string image_path = base_path;
  image_path += "assets/";
  image_path += properties.image;
  image_path += ".png";
  SDL_Texture *texture = IMG_LoadTexture(renderer, image_path.c_str());
  if (!texture) {
    printf(">> cannot load image: %s .. ERROR\n", properties.image.c_str());
    return false;
  }
  SDL_GetTextureSize(texture, &data.width, &data.height);
  if (data.width <= 0.0f || data.height <= 0.0f) {
    SDL_DestroyTexture(texture);
    return false;
  }
  data.texture = texture;
  data.name = properties.image;
  return true;
}

static void image_display_size(const image::impl &data, const image_properties &properties, float &width, float &height) {
  width = properties.size_x;
  height = properties.size_y;
  if (width <= 0.0f && height <= 0.0f) {
    width = data.width;
    height = data.height;
  } else if (width <= 0.0f) {
    width = data.width * height / data.height;
  } else if (height <= 0.0f) {
    height = data.height * width / data.width;
  }
}

void image::render(SDL_Renderer *renderer) {
  update_tweens();
  if (!renderer || !data || !properties.visible) {
    return;
  }
  if (!ensure_image_texture(renderer, *data, properties)) {
    return;
  }
  float width = 0.0f;
  float height = 0.0f;
  image_display_size(*data, properties, width, height);
  SDL_FRect rect = property_rect(renderer, properties, width, height);
  apply_texture_color(data->texture, properties.image_color, properties.image_transparency);
  SDL_RenderTexture(renderer, data->texture, nullptr, &rect);
  reset_texture_color(data->texture);
}

void image::destroy() {
  data.reset();
}

void text::render(SDL_Renderer *renderer) {
  update_tweens();
  if (!renderer || !data || !properties.visible) {
    return;
  }
  if (!ensure_text_texture(renderer, *data, properties)) {
    return;
  }
  SDL_FRect rect = property_rect(renderer, properties, data->width, data->height);
  apply_texture_color(data->texture, properties.text_color, properties.text_transparency);
  SDL_RenderTexture(renderer, data->texture, nullptr, &rect);
  reset_texture_color(data->texture);
}

void text::destroy() {
  data.reset();
}

void button::render(SDL_Renderer *renderer) {
  update_tweens();
  if (!renderer || !data || !properties.visible) {
    clear_button_state(*this);
    return;
  }
  text &label = data->label;
  if (!label.data) {
    clear_button_state(*this);
    return;
  }
  label.properties.text = properties.text;
  label.properties.text_size = properties.text_size;
  label.properties.text_color = properties.text_color;
  label.properties.text_transparency = properties.text_transparency;
  label.properties.scale = properties.scale;
  label.properties.position_x = properties.position_x;
  label.properties.position_y = properties.position_y;
  label.properties.anchor_x = properties.anchor_x;
  label.properties.anchor_y = properties.anchor_y;
  label.properties.visible = true;
  if (!ensure_text_texture(renderer, *label.data, label.properties)) {
    clear_button_state(*this);
    return;
  }
  float box_w = properties.size_x > 0.0f ? properties.size_x : label.data->width + properties.padding_x * 2.0f;
  float box_h = properties.size_y > 0.0f ? properties.size_y : label.data->height + properties.padding_y * 2.0f;
  SDL_FRect rect = property_rect(renderer, properties, box_w, box_h);
  update_button_state(renderer, rect, *data, *this);
  bool inside = mouse_inside(renderer, rect);
  SDL_Color background = properties.background_color;
  float background_transparency = properties.background_transparency;
  if (properties.auto_button_color && inside) {
    background = properties.hover_color;
    background_transparency = properties.hover_transparency;
  }
  draw_rect(renderer, rect, background, background_transparency);
  draw_border(renderer, rect, properties.border_color, properties.border_transparency, properties.border_size);
  label.render(renderer);
}

void button::destroy() {
  data.reset();
  clear_button_state(*this);
}

void image_button::render(SDL_Renderer *renderer) {
  update_tweens();
  if (!renderer || !data || !properties.visible) {
    clear_button_state(*this);
    return;
  }
  image &picture = data->picture;
  if (!picture.data) {
    clear_button_state(*this);
    return;
  }
  picture.properties.image = properties.image;
  picture.properties.image_color = properties.image_color;
  picture.properties.image_transparency = properties.image_transparency;
  picture.properties.scale = properties.scale;
  picture.properties.position_x = properties.position_x;
  picture.properties.position_y = properties.position_y;
  picture.properties.anchor_x = properties.anchor_x;
  picture.properties.anchor_y = properties.anchor_y;
  picture.properties.size_x = properties.size_x;
  picture.properties.size_y = properties.size_y;
  picture.properties.visible = true;
  if (!ensure_image_texture(renderer, *picture.data, picture.properties)) {
    clear_button_state(*this);
    return;
  }
  float box_w = 0.0f;
  float box_h = 0.0f;
  image_display_size(*picture.data, picture.properties, box_w, box_h);
  SDL_FRect rect = property_rect(renderer, properties, box_w, box_h);
  update_button_state(renderer, rect, *data, *this);
  picture.render(renderer);
  if (properties.auto_button_color && mouse_inside(renderer, rect)) {
    draw_border(renderer, rect, properties.hover_color, properties.hover_transparency, properties.hover_size);
  }
}

void image_button::destroy() {
  data.reset();
  clear_button_state(*this);
}

void panel::render(SDL_Renderer *renderer) {
  update_tweens();
  if (!renderer || !data || !properties.visible) {
    return;
  }
  SDL_FRect rect = property_rect(renderer, properties, properties.size_x, properties.size_y);
  draw_rect(renderer, rect, properties.background_color, properties.background_transparency);
  draw_border(renderer, rect, properties.border_color, properties.border_transparency, properties.border_size);
}

void panel::destroy() {
  data.reset();
}

image render_image(SDL_Renderer *renderer, const string &name) {
  image result;
  if (!renderer) {
    printf(">> render_image has no renderer.. ERROR\n");
    return result;
  }
  result.data = make_shared<image::impl>();
  result.properties.image = name;
  return result;
}

text render_text(SDL_Renderer *renderer, const string &content, float size) {
  text result;
  if (!renderer) {
    printf(">> render_text has no renderer.. ERROR\n");
    return result;
  }
  result.data = make_shared<text::impl>();
  result.properties.text = content;
  result.properties.text_size = size;
  return result;
}

button render_button(SDL_Renderer *renderer, const string &content, float size) {
  button result;
  if (!renderer) {
    printf(">> render_button has no renderer.. ERROR\n");
    return result;
  }
  result.data = make_shared<button::impl>();
  result.data->label = render_text(renderer, content, size);
  result.properties.text = content;
  result.properties.text_size = size;
  return result;
}

image_button render_image_button(SDL_Renderer *renderer, const string &name) {
  image_button result;
  if (!renderer) {
    printf(">> render_image_button has no renderer.. ERROR\n");
    return result;
  }
  result.data = make_shared<image_button::impl>();
  result.data->picture = render_image(renderer, name);
  result.properties.image = name;
  return result;
}

panel render_panel(SDL_Renderer *renderer) {
  panel result;
  if (!renderer) {
    printf(">> render_panel has no renderer.. ERROR\n");
    return result;
  }
  result.data = make_shared<panel::impl>();
  return result;
}

template <typename properties_type>
static tween_values read_gui_properties(const properties_type &properties) {
  tween_values values;
  values.x = properties.position_x;
  values.y = properties.position_y;
  values.width = properties.size_x;
  values.height = properties.size_y;
  values.scale = properties.scale;
  values.anchor_x = properties.anchor_x;
  values.anchor_y = properties.anchor_y;
  values.background_transparency = properties.background_transparency;
  values.border_size = properties.border_size;
  values.border_transparency = properties.border_transparency;
  return values;
}

static bool tween_has_goal(const vector<tween_property> &goals, const string &name) {
  for (const tween_property &goal : goals) {
    if (goal.name == name) {
      return true;
    }
  }
  return false;
}

template <typename properties_type>
static void write_gui_properties(properties_type &properties, const tween_values &values, const vector<tween_property> &goals) {
  if (tween_has_goal(goals, "x") || tween_has_goal(goals, "position_x")) {
    properties.position_x = values.x;
  }
  if (tween_has_goal(goals, "y") || tween_has_goal(goals, "position_y")) {
    properties.position_y = values.y;
  }
  if (tween_has_goal(goals, "width") || tween_has_goal(goals, "size_x")) {
    properties.size_x = values.width;
  }
  if (tween_has_goal(goals, "height") || tween_has_goal(goals, "size_y")) {
    properties.size_y = values.height;
  }
  if (tween_has_goal(goals, "scale")) {
    properties.scale = values.scale;
  }
  if (tween_has_goal(goals, "anchor_x")) {
    properties.anchor_x = values.anchor_x;
  }
  if (tween_has_goal(goals, "anchor_y")) {
    properties.anchor_y = values.anchor_y;
  }
  if (tween_has_goal(goals, "background_transparency")) {
    properties.background_transparency = values.background_transparency;
  }
  if (tween_has_goal(goals, "border_size")) {
    properties.border_size = values.border_size;
  }
  if (tween_has_goal(goals, "border_transparency")) {
    properties.border_transparency = values.border_transparency;
  }
}

template <typename properties_type>
static void write_text_properties(properties_type &properties, const tween_values &values, const vector<tween_property> &goals) {
  write_gui_properties(properties, values, goals);
  if (tween_has_goal(goals, "text_transparency")) {
    properties.text_transparency = values.text_transparency;
  }
}

template <typename properties_type>
static void write_image_properties(properties_type &properties, const tween_values &values, const vector<tween_property> &goals) {
  write_gui_properties(properties, values, goals);
  if (tween_has_goal(goals, "image_transparency")) {
    properties.image_transparency = values.image_transparency;
  }
}

tween create_tween(image target, const tween_info &info, initializer_list<tween_property> goals) {
  shared_ptr<image_properties> properties = target.property_data;
  vector<tween_property> goal_list(goals);
  return make_tween(
      [properties]() { return read_gui_properties(*properties); },
      [properties, goal_list](const tween_values &values) { write_image_properties(*properties, values, goal_list); },
      info, goals);
}

tween create_tween(text target, const tween_info &info, initializer_list<tween_property> goals) {
  shared_ptr<text_properties> properties = target.property_data;
  vector<tween_property> goal_list(goals);
  return make_tween(
      [properties]() {
        tween_values values = read_gui_properties(*properties);
        values.text_transparency = properties->text_transparency;
        return values;
      },
      [properties, goal_list](const tween_values &values) { write_text_properties(*properties, values, goal_list); },
      info, goals);
}

tween create_tween(button target, const tween_info &info, initializer_list<tween_property> goals) {
  shared_ptr<button_properties> properties = target.property_data;
  vector<tween_property> goal_list(goals);
  return make_tween(
      [properties]() {
        tween_values values = read_gui_properties(*properties);
        values.text_transparency = properties->text_transparency;
        return values;
      },
      [properties, goal_list](const tween_values &values) { write_text_properties(*properties, values, goal_list); },
      info, goals);
}

tween create_tween(image_button target, const tween_info &info, initializer_list<tween_property> goals) {
  shared_ptr<image_button_properties> properties = target.property_data;
  vector<tween_property> goal_list(goals);
  return make_tween(
      [properties]() {
        tween_values values = read_gui_properties(*properties);
        values.image_transparency = properties->image_transparency;
        return values;
      },
      [properties, goal_list](const tween_values &values) { write_image_properties(*properties, values, goal_list); },
      info, goals);
}

tween create_tween(panel target, const tween_info &info, initializer_list<tween_property> goals) {
  shared_ptr<panel_properties> properties = target.property_data;
  vector<tween_property> goal_list(goals);
  return make_tween(
      [properties]() { return read_gui_properties(*properties); },
      [properties, goal_list](const tween_values &values) { write_gui_properties(*properties, values, goal_list); },
      info, goals);
}
