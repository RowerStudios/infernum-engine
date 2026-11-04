#pragma once
#include "engine.h"
#include <string>
#include <vector>
using namespace std;

namespace Json {
class Value;
}

enum class ui_kind {
  panel,
  text,
  image,
  button,
  image_button
};

struct engine_ui_element {
  string name;
  ui_kind kind = ui_kind::panel;
  panel panel_data;
  text text_data;
  image image_data;
  button button_data;
  image_button image_button_data;
  void render(SDL_Renderer *renderer);
};

struct ui_scene {
  vector<engine_ui_element> elements;
  bool load(SDL_Renderer *renderer, const string &file_name = "game_properties.json",
            const string &scene = "");
  bool load_root(SDL_Renderer *renderer, const Json::Value &root,
                 const string &scene = "");
  void render(SDL_Renderer *renderer);
  engine_ui_element *find(const string &name);
};
