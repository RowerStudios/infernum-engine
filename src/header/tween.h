#pragma once
#include <functional>
#include <initializer_list>
#include <memory>
#include <string>
using namespace std;

//easing styles
enum class easing_style {
  linear,
  sine,
  quad,
  cubic,
  quart,
  quint,
  bounce,
  elastic,
  exponential,
  circular,
  back
};

enum class easing_direction { in, out, in_out };

struct tween_info {
  float time = 1.0f;
  easing_style style = easing_style::quad;
  easing_direction direction = easing_direction::out;
  int repeat_count = 0;
  bool reverses = false;
  float delay_time = 0.0f;
  tween_info() = default;
  tween_info(float time, easing_style style, easing_direction direction,
             int repeat_count = 0, bool reverses = false,
             float delay_time = 0.0f);
};

// tween property values as of now
struct tween_property {
  string name;
  float value;
};

// we will have more in the future
struct tween_values {
  float x = 0.0f;
  float y = 0.0f;
  float width = 0.0f;
  float height = 0.0f;
  float scale = 1.0f;
  float anchor_x = 0.5f;
  float anchor_y = 0.5f;
  float background_transparency = 1.0f;
  float border_size = 0.0f;
  float border_transparency = 1.0f;
  float text_transparency = 0.0f;
  float image_transparency = 0.0f;
};
// tween functions
struct tween {
  struct impl;
  shared_ptr<impl> data;
  void play();
  void pause();
  void cancel();
  bool completed() const;
  bool playing() const;
};

tween make_tween(function<tween_values()> get,
                 function<void(const tween_values &)> set,
                 const tween_info &info,
                 initializer_list<tween_property> goals);
void update_tweens();
