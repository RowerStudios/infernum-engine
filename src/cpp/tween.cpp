/*
https://github.com/mobius3/tweeny
https://github.com/vladimirgamalyan/microtween
https://github.com/nicolausYes/easing-functions
https://create.roblox.com/docs/reference/engine/datatypes/TweenInfo
https://create.roblox.com/docs/reference/engine/classes/TweenService
https://create.roblox.com/docs/reference/engine/classes/Tween 

All reference to the sources i used to make the tween.cpp

HEAVILY inspired by roblox tween service
*/

#include "../header/tween.h"
#include <SDL3/SDL.h>
#include <cmath>
#include <cstdio>
#include <memory>
#include <vector>
using namespace std;

struct tween::impl { // all the tween values that can be returned by mentioning the value
  tween_info info;
  tween_values start;
  tween_values goal;
  function<tween_values()> get;
  function<void(const tween_values &)> set;
  vector<tween_property> goals;
  double elapsed = 0.0;
  Uint64 last_tick = 0;
  bool started = false;
  bool playing = false;
  bool completed = false;
};

static vector<weak_ptr<tween::impl>> active_tweens; // detect active tweens

tween_info::tween_info(float time, easing_style style, easing_direction direction, // tween info properties like roblox
                       int repeat_count, bool reverses, float delay_time)
    : time(time), style(style), direction(direction), repeat_count(repeat_count),
      reverses(reverses), delay_time(delay_time) {}

static const float tween_pi = 3.14159265358979323846f; // 22/7 is pi bruh

// ALL The fricking tween functions //
static float ease_in_sine(float t) { return 1.0f - cosf(t * tween_pi / 2.0f); }
static float ease_out_sine(float t) { return sinf(t * tween_pi / 2.0f); }
static float ease_in_out_sine(float t) { return -(cosf(tween_pi * t) - 1.0f) / 2.0f; }
static float ease_in_quad(float t) { return t * t; }
static float ease_out_quad(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }
static float ease_in_out_quad(float t) {
  return t < 0.5f ? 2.0f * t * t : 1.0f - powf(-2.0f * t + 2.0f, 2.0f) / 2.0f;
}
static float ease_in_cubic(float t) { return t * t * t; }
static float ease_out_cubic(float t) { return 1.0f - powf(1.0f - t, 3.0f); }
static float ease_in_out_cubic(float t) {
  return t < 0.5f ? 4.0f * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 3.0f) / 2.0f;
}
static float ease_in_quart(float t) { return t * t * t * t; }
static float ease_out_quart(float t) { return 1.0f - powf(1.0f - t, 4.0f); }
static float ease_in_out_quart(float t) {
  return t < 0.5f ? 8.0f * t * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 4.0f) / 2.0f;
}
static float ease_in_quint(float t) { return t * t * t * t * t; }
static float ease_out_quint(float t) { return 1.0f - powf(1.0f - t, 5.0f); }
static float ease_in_out_quint(float t) {
  return t < 0.5f ? 16.0f * t * t * t * t * t : 1.0f - powf(-2.0f * t + 2.0f, 5.0f) / 2.0f;
}
static float ease_in_expo(float t) {
  return t == 0.0f ? 0.0f : powf(2.0f, 10.0f * t - 10.0f);
}
static float ease_out_expo(float t) {
  return t == 1.0f ? 1.0f : 1.0f - powf(2.0f, -10.0f * t);
}
static float ease_in_out_expo(float t) {
  if (t == 0.0f || t == 1.0f) {
    return t;
  }
  return t < 0.5f ? powf(2.0f, 20.0f * t - 10.0f) / 2.0f
                  : (2.0f - powf(2.0f, -20.0f * t + 10.0f)) / 2.0f;
}
static float ease_in_circ(float t) { return 1.0f - sqrtf(1.0f - t * t); }
static float ease_out_circ(float t) { return sqrtf(1.0f - powf(t - 1.0f, 2.0f)); }
static float ease_in_out_circ(float t) {
  return t < 0.5f ? (1.0f - sqrtf(1.0f - powf(2.0f * t, 2.0f))) / 2.0f
                  : (sqrtf(1.0f - powf(-2.0f * t + 2.0f, 2.0f)) + 1.0f) / 2.0f;
}
static float ease_out_bounce(float t) {
  const float n1 = 7.5625f;
  const float d1 = 2.75f;
  if (t < 1.0f / d1) {
    return n1 * t * t;
  }
  if (t < 2.0f / d1) {
    t -= 1.5f / d1;
    return n1 * t * t + 0.75f;
  }
  if (t < 2.5f / d1) {
    t -= 2.25f / d1;
    return n1 * t * t + 0.9375f;
  }
  t -= 2.625f / d1;
  return n1 * t * t + 0.984375f;
}
static float ease_in_bounce(float t) { return 1.0f - ease_out_bounce(1.0f - t); }
static float ease_in_out_bounce(float t) {
  return t < 0.5f ? (1.0f - ease_out_bounce(1.0f - 2.0f * t)) / 2.0f
                  : (1.0f + ease_out_bounce(2.0f * t - 1.0f)) / 2.0f;
}
static float ease_in_elastic(float t) {
  if (t == 0.0f || t == 1.0f) {
    return t;
  }
  const float c4 = (2.0f * tween_pi) / 3.0f;
  return -powf(2.0f, 10.0f * t - 10.0f) * sinf((t * 10.0f - 10.75f) * c4);
}
static float ease_out_elastic(float t) {
  if (t == 0.0f || t == 1.0f) {
    return t;
  }
  const float c4 = (2.0f * tween_pi) / 3.0f;
  return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
}
static float ease_in_out_elastic(float t) {
  if (t == 0.0f || t == 1.0f) {
    return t;
  }
  const float c5 = (2.0f * tween_pi) / 4.5f;
  return t < 0.5f
             ? -(powf(2.0f, 20.0f * t - 10.0f) *
                 sinf((20.0f * t - 11.125f) * c5)) / 2.0f
             : (powf(2.0f, -20.0f * t + 10.0f) *
                sinf((20.0f * t - 11.125f) * c5)) / 2.0f + 1.0f;
}
static float ease_in_back(float t) {
  const float c1 = 1.70158f;
  const float c3 = c1 + 1.0f;
  return c3 * t * t * t - c1 * t * t;
}
static float ease_out_back(float t) {
  const float c1 = 1.70158f;
  const float c3 = c1 + 1.0f;
  return 1.0f + c3 * powf(t - 1.0f, 3.0f) + c1 * powf(t - 1.0f, 2.0f);
}
static float ease_in_out_back(float t) {
  const float c1 = 1.70158f;
  const float c2 = c1 * 1.525f;
  return t < 0.5f
             ? (powf(2.0f * t, 2.0f) * ((c2 + 1.0f) * 2.0f * t - c2)) / 2.0f
             : (powf(2.0f * t - 2.0f, 2.0f) *
                ((c2 + 1.0f) * (t * 2.0f - 2.0f) + c2) + 2.0f) / 2.0f;
}

// only used ease values that are on roblox
// i dont wanna fucking kill my hands
// merged ease in and out to be a single thing 
// its my own prefered way of making tween easing

static float ease_value(easing_style style, easing_direction direction, float t) {
  if (t < 0.0f) {
    t = 0.0f;
  }
  if (t > 1.0f) {
    t = 1.0f;
  }
  switch (style) {
  case easing_style::linear:
    return t;
  case easing_style::sine:
    if (direction == easing_direction::in) {
      return ease_in_sine(t);
    }
    if (direction == easing_direction::out) {
      return ease_out_sine(t);
    }
    return ease_in_out_sine(t);
  case easing_style::quad:
    if (direction == easing_direction::in) {
      return ease_in_quad(t);
    }
    if (direction == easing_direction::out) {
      return ease_out_quad(t);
    }
    return ease_in_out_quad(t);
  case easing_style::cubic:
    if (direction == easing_direction::in) {
      return ease_in_cubic(t);
    }
    if (direction == easing_direction::out) {
      return ease_out_cubic(t);
    }
    return ease_in_out_cubic(t);
  case easing_style::quart:
    if (direction == easing_direction::in) {
      return ease_in_quart(t);
    }
    if (direction == easing_direction::out) {
      return ease_out_quart(t);
    }
    return ease_in_out_quart(t);
  case easing_style::quint:
    if (direction == easing_direction::in) {
      return ease_in_quint(t);
    }
    if (direction == easing_direction::out) {
      return ease_out_quint(t);
    }
    return ease_in_out_quint(t);
  case easing_style::bounce:
    if (direction == easing_direction::in) {
      return ease_in_bounce(t);
    }
    if (direction == easing_direction::out) {
      return ease_out_bounce(t);
    }
    return ease_in_out_bounce(t);
  case easing_style::elastic:
    if (direction == easing_direction::in) {
      return ease_in_elastic(t);
    }
    if (direction == easing_direction::out) {
      return ease_out_elastic(t);
    }
    return ease_in_out_elastic(t);
  case easing_style::exponential:
    if (direction == easing_direction::in) {
      return ease_in_expo(t);
    }
    if (direction == easing_direction::out) {
      return ease_out_expo(t);
    }
    return ease_in_out_expo(t);
  case easing_style::circular:
    if (direction == easing_direction::in) {
      return ease_in_circ(t);
    }
    if (direction == easing_direction::out) {
      return ease_out_circ(t);
    }
    return ease_in_out_circ(t);
  case easing_style::back:
    if (direction == easing_direction::in) {
      return ease_in_back(t);
    }
    if (direction == easing_direction::out) {
      return ease_out_back(t);
    }
    return ease_in_out_back(t);
  }
  return t;
}

static double tween_delta(Uint64 &last_tick) {
  Uint64 now = SDL_GetPerformanceCounter();
  if (last_tick == 0) {
    last_tick = now;
    return 0.0;
  }
  double delta =
      (double)(now - last_tick) / (double)SDL_GetPerformanceFrequency();
  last_tick = now;
  return delta;
}

static float *tween_field(tween_values &values, const string &name) {
  if (name == "x" || name == "position_x") {
    return &values.x;
  }
  if (name == "y" || name == "position_y") {
    return &values.y;
  }
  if (name == "width" || name == "size_x") {
    return &values.width;
  }
  if (name == "height" || name == "size_y") {
    return &values.height;
  }
  if (name == "scale") {
    return &values.scale;
  }
  if (name == "anchor_x") {
    return &values.anchor_x;
  }
  if (name == "anchor_y") {
    return &values.anchor_y;
  }
  if (name == "background_transparency") {
    return &values.background_transparency;
  }
  if (name == "border_size") {
    return &values.border_size;
  }
  if (name == "border_transparency") {
    return &values.border_transparency;
  }
  if (name == "text_transparency") {
    return &values.text_transparency;
  }
  if (name == "image_transparency") {
    return &values.image_transparency;
  }
  return nullptr;
}

static void apply_tween(tween::impl &tween_data) {
  const tween_info &info = tween_data.info;
  double delay = info.delay_time > 0.0f ? (double)info.delay_time : 0.0;
  if (tween_data.elapsed < delay) {
    return;
  }
  double local_elapsed = tween_data.elapsed - delay;
  double duration = (double)info.time;
  if (duration <= 0.0) {
    tween_data.set(tween_data.goal);
    tween_data.completed = true;
    tween_data.playing = false;
    return;
  }
  int repeat_count = info.repeat_count > 0 ? info.repeat_count : 0;
  double iterations = (double)(repeat_count + 1) * (info.reverses ? 2.0 : 1.0);
  double total = duration * iterations;
  float local = 0.0f;
  if (local_elapsed >= total) {
    tween_data.completed = true;
    tween_data.playing = false;
    if (info.reverses) {
      int last_iteration = (int)iterations - 1;
      local = (last_iteration % 2 == 1) ? 0.0f : 1.0f;
    } else {
      local = 1.0f;
    }
  } else {
    double iteration = floor(local_elapsed / duration);
    local = (float)((local_elapsed - iteration * duration) / duration);
    if (info.reverses && ((int)iteration % 2) == 1) {
      local = 1.0f - local;
    }
  }
  float alpha = ease_value(info.style, info.direction, local);
  tween_values value = tween_data.start;
  for (const tween_property &goal : tween_data.goals) {
    float *start_field = tween_field(tween_data.start, goal.name);
    float *goal_field = tween_field(tween_data.goal, goal.name);
    float *value_field = tween_field(value, goal.name);
    if (start_field && goal_field && value_field) {
      *value_field = *start_field + (*goal_field - *start_field) * alpha;
    }
  }
  tween_data.set(value);
}

static void register_tween(const shared_ptr<tween::impl> &data) {
  for (weak_ptr<tween::impl> &entry : active_tweens) {
    if (entry.lock() == data) {
      return;
    }
  }
  active_tweens.push_back(data);
}

void update_tweens() {
  for (size_t index = 0; index < active_tweens.size();) {
    shared_ptr<tween::impl> data = active_tweens[index].lock();
    if (!data) {
      active_tweens.erase(active_tweens.begin() + index);
      continue;
    }
    if (data->playing) {
      data->elapsed += tween_delta(data->last_tick);
      apply_tween(*data);
    }
    index++;
  }
}

void tween::play() {
  if (!data || data->playing) {
    return;
  }
  if (!data->started || data->completed) {
    data->start = data->get ? data->get() : tween_values();
    data->goal = data->start;
    for (const tween_property &goal : data->goals) {
      if (float *field = tween_field(data->goal, goal.name)) {
        *field = goal.value;
      }
    }
    data->started = true;
    data->completed = false;
    data->elapsed = 0.0;
  }
  data->playing = true;
  data->last_tick = SDL_GetPerformanceCounter();
  register_tween(data);
}

void tween::pause() {
  if (!data || !data->playing) {
    return;
  }
  data->elapsed += tween_delta(data->last_tick);
  apply_tween(*data);
  data->playing = false;
}

void tween::cancel() {
  if (!data) {
    return;
  }
  data->playing = false;
  data->started = false;
  data->completed = false;
  data->elapsed = 0.0;
}

bool tween::completed() const { return data && data->completed; }

bool tween::playing() const { return data && data->playing; }

tween make_tween(function<tween_values()> get,
                 function<void(const tween_values &)> set,
                 const tween_info &info,
                 initializer_list<tween_property> goals) {
  tween result;
  result.data = make_shared<tween::impl>();
  result.data->info = info;
  result.data->get = get;
  result.data->set = set;
  for (const tween_property &goal : goals) {
    if (!tween_field(result.data->goal, goal.name)) {
      printf(">> tween has unknown property: %s .. ERROR\n", goal.name.c_str());
      continue;
    }
    result.data->goals.push_back(goal);
  }
  return result;
}
