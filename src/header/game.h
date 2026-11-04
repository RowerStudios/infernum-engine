#pragma once
#include <string>
using namespace std;
#include <SDL3/SDL.h>
#include <iostream>
void fetch_window(SDL_Window *window);
void fetch_renderer(SDL_Renderer *renderer);
void load_scene(string name);
void run_scene(SDL_Renderer *scene_renderer);
void unload_scene();
void show_debug(bool);
bool init_stats(SDL_Renderer *stats_renderer);
void update_stats();
void draw_stats(SDL_Renderer *stats_renderer);
void shutdown_stats();
