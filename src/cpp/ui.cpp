#include "../header/ui.h"
#include <SDL3/SDL.h>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <json/json.h>
#include <memory>

static string strip_comments(const string &data) {
  string result;
  bool in_string = false;
  bool escaped = false;
  for (size_t i = 0; i < data.size(); ++i) {
    char c = data[i];
    if (in_string) {
      result.push_back(c);
      if (escaped) {
        escaped = false;
      } else if (c == '\\') {
        escaped = true;
      } else if (c == '"') {
        in_string = false;
      }
      continue;
    }
    if (c == '"') {
      in_string = true;
      result.push_back(c);
    } else if (c == '/' && i + 1 < data.size() && data[i + 1] == '/') {
      while (i < data.size() && data[i] != '\n') {
        ++i;
      }
      result.push_back('\n');
    } else if (c == '/' && i + 1 < data.size() && data[i + 1] == '*') {
      i += 2;
      while (i + 1 < data.size() && !(data[i] == '*' && data[i + 1] == '/')) {
        ++i;
      }
      ++i;
    } else {
      result.push_back(c);
    }
  }
  return result;
}

static SDL_Color read_color(const Json::Value &value, SDL_Color fallback) {
  if (!value.isArray() || value.size() < 3) {
    return fallback;
  }
  SDL_Color color;
  color.r = (Uint8)value[0].asInt();
  color.g = (Uint8)value[1].asInt();
  color.b = (Uint8)value[2].asInt();
  color.a = value.size() >= 4 ? (Uint8)value[3].asInt() : 255;
  return color;
}

template <typename properties_type>
static void apply_gui_properties(properties_type &properties, const Json::Value &value) {
  if (value.isMember("position_x")) {
    properties.position_x = value["position_x"].asFloat();
  }
  if (value.isMember("position_y")) {
    properties.position_y = value["position_y"].asFloat();
  }
  if (value.isMember("size_x")) {
    properties.size_x = value["size_x"].asFloat();
  }
  if (value.isMember("size_y")) {
    properties.size_y = value["size_y"].asFloat();
  }
  if (value.isMember("anchor_x")) {
    properties.anchor_x = value["anchor_x"].asFloat();
  }
  if (value.isMember("anchor_y")) {
    properties.anchor_y = value["anchor_y"].asFloat();
  }
  if (value.isMember("scale")) {
    properties.scale = value["scale"].asFloat();
  }
  if (value.isMember("visible")) {
    properties.visible = value["visible"].asBool();
  }
  if (value.isMember("background_color")) {
    properties.background_color = read_color(value["background_color"], properties.background_color);
  }
  if (value.isMember("background_transparency")) {
    properties.background_transparency = value["background_transparency"].asFloat();
  }
  if (value.isMember("border_color")) {
    properties.border_color = read_color(value["border_color"], properties.border_color);
  }
  if (value.isMember("border_transparency")) {
    properties.border_transparency = value["border_transparency"].asFloat();
  }
  if (value.isMember("border_size")) {
    properties.border_size = value["border_size"].asFloat();
  }
}

template <typename properties_type>
static void apply_text_properties(properties_type &properties, const Json::Value &value) {
  if (value.isMember("text")) {
    properties.text = value["text"].asString();
  }
  if (value.isMember("text_size")) {
    properties.text_size = value["text_size"].asFloat();
  }
  if (value.isMember("text_color")) {
    properties.text_color = read_color(value["text_color"], properties.text_color);
  }
  if (value.isMember("text_transparency")) {
    properties.text_transparency = value["text_transparency"].asFloat();
  }
}

template <typename properties_type>
static void apply_image_properties(properties_type &properties, const Json::Value &value) {
  if (value.isMember("image")) {
    properties.image = value["image"].asString();
  }
  if (value.isMember("image_color")) {
    properties.image_color = read_color(value["image_color"], properties.image_color);
  }
  if (value.isMember("image_transparency")) {
    properties.image_transparency = value["image_transparency"].asFloat();
  }
}

static void apply_image_button_properties(image_button_properties &properties, const Json::Value &value) {
  apply_image_properties(properties, value);
  if (value.isMember("auto_button_color")) {
    properties.auto_button_color = value["auto_button_color"].asBool();
  }
  if (value.isMember("hover_color")) {
    properties.hover_color = read_color(value["hover_color"], properties.hover_color);
  }
  if (value.isMember("hover_transparency")) {
    properties.hover_transparency = value["hover_transparency"].asFloat();
  }
  if (value.isMember("hover_size")) {
    properties.hover_size = value["hover_size"].asFloat();
  }
}

static void apply_button_properties(button_properties &properties, const Json::Value &value) {
  apply_text_properties(properties, value);
  if (value.isMember("padding_x")) {
    properties.padding_x = value["padding_x"].asFloat();
  }
  if (value.isMember("padding_y")) {
    properties.padding_y = value["padding_y"].asFloat();
  }
  if (value.isMember("auto_button_color")) {
    properties.auto_button_color = value["auto_button_color"].asBool();
  }
  if (value.isMember("hover_color")) {
    properties.hover_color = read_color(value["hover_color"], properties.hover_color);
  }
  if (value.isMember("hover_transparency")) {
    properties.hover_transparency = value["hover_transparency"].asFloat();
  }
}

static const Json::Value *pick_ui(const Json::Value &root, const string &scene) {
  const Json::Value &scenes = root["scenes"];
  if (!scene.empty() && scenes.isObject()) {
    const Json::Value &named = scenes[scene];
    if (named.isArray()) {
      return &named;
    }
  }
  const Json::Value &legacy = root["ui"];
  if (legacy.isArray()) {
    return &legacy;
  }
  if (scenes.isObject()) {
    const Json::Value &fallback = scenes["main_scene"];
    if (fallback.isArray()) {
      return &fallback;
    }
  }
  return nullptr;
}

static bool build_elements(SDL_Renderer *renderer, const Json::Value &ui,
                           vector<engine_ui_element> &out) {
  for (const Json::Value &entry : ui) {
    if (!entry.isObject()) {
      continue;
    }
    engine_ui_element element;
    element.name = entry["name"].asString();
    string type = entry["type"].asString();

    if (type == "text") {
      element.kind = ui_kind::text;
      string content = entry.get("text", "").asString();
      float size = entry.get("text_size", 24.0f).asFloat();
      element.text_data = render_text(renderer, content, size);
      apply_gui_properties(element.text_data.properties, entry);
      apply_text_properties(element.text_data.properties, entry);
    } else if (type == "image") {
      element.kind = ui_kind::image;
      string name = entry.get("image", "").asString();
      element.image_data = render_image(renderer, name);
      apply_gui_properties(element.image_data.properties, entry);
      apply_image_properties(element.image_data.properties, entry);
    } else if (type == "button") {
      element.kind = ui_kind::button;
      string content = entry.get("text", "").asString();
      float size = entry.get("text_size", 24.0f).asFloat();
      element.button_data = render_button(renderer, content, size);
      apply_gui_properties(element.button_data.properties, entry);
      apply_button_properties(element.button_data.properties, entry);
    } else if (type == "image_button") {
      element.kind = ui_kind::image_button;
      string name = entry.get("image", "").asString();
      element.image_button_data = render_image_button(renderer, name);
      apply_gui_properties(element.image_button_data.properties, entry);
      apply_image_button_properties(element.image_button_data.properties, entry);
    } else {
      element.kind = ui_kind::panel;
      element.panel_data = render_panel(renderer);
      apply_gui_properties(element.panel_data.properties, entry);
    }

    out.push_back(element);
  }

  return true;
}

static bool read_json_file(const string &path, Json::Value &root) {
  ifstream file(path);
  if (!file.is_open()) {
    return false;
  }
  string data((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
  data = strip_comments(data);
  Json::CharReaderBuilder builder;
  string errors;
  unique_ptr<Json::CharReader> reader(builder.newCharReader());
  if (!reader->parse(data.c_str(), data.c_str() + data.size(), &root, &errors)) {
    printf(">> cannot parse %s: %s\n", path.c_str(), errors.c_str());
    return false;
  }
  return true;
}

bool ui_scene::load(SDL_Renderer *renderer, const string &file_name, const string &scene) {
  elements.clear();
  if (!renderer) {
    printf(">> ui loader has no renderer.. ERROR\n");
    return false;
  }
  const char *base_path = SDL_GetBasePath();
  if (!base_path) {
    printf(">> ui loader cannot find base path.. ERROR\n");
    return false;
  }
  string name = scene.empty() ? "main_scene" : scene;
  string scene_path = string(base_path) + "scenes/" + name + "/properties.json";
  Json::Value scene_root;
  if (read_json_file(scene_path, scene_root)) {
    const Json::Value &ui = scene_root["ui"];
    if (ui.isArray()) {
      build_elements(renderer, ui, elements);
      printf(">> loaded %d ui elements for scene '%s' from %s\n",
             (int)elements.size(), name.c_str(), scene_path.c_str());
      return true;
    }
    printf(">> %s has no 'ui' array .. ERROR\n", scene_path.c_str());
    return false;
  }

  string path = string(base_path) + file_name;
  Json::Value root;
  if (!read_json_file(path, root)) {
    printf(">> cannot open %s .. ERROR\n", path.c_str());
    return false;
  }
  bool result = load_root(renderer, root, name);
  if (result) {
    printf(">> loaded %d ui elements for scene '%s' from %s\n",
           (int)elements.size(), name.c_str(), path.c_str());
  }
  return result;
}

bool ui_scene::load_root(SDL_Renderer *renderer, const Json::Value &root, const string &scene) {
  elements.clear();
  if (!renderer) {
    printf(">> ui loader has no renderer.. ERROR\n");
    return false;
  }
  const Json::Value *ui = pick_ui(root, scene);
  if (!ui) {
    printf(">> cannot find ui data for scene '%s' .. ERROR\n",
           scene.empty() ? "(default)" : scene.c_str());
    return false;
  }
  return build_elements(renderer, *ui, elements);
}

void engine_ui_element::render(SDL_Renderer *renderer) {
  switch (kind) {
    case ui_kind::panel:
      panel_data.render(renderer);
      break;
    case ui_kind::text:
      text_data.render(renderer);
      break;
    case ui_kind::image:
      image_data.render(renderer);
      break;
    case ui_kind::button:
      button_data.render(renderer);
      break;
    case ui_kind::image_button:
      image_button_data.render(renderer);
      break;
  }
}

void ui_scene::render(SDL_Renderer *renderer) {
  for (engine_ui_element &element : elements) {
    element.render(renderer);
  }
}

engine_ui_element *ui_scene::find(const string &name) {
  for (engine_ui_element &element : elements) {
    if (element.name == name) {
      return &element;
    }
  }
  return nullptr;
}
