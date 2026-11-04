#pragma once
#include <SDL3/SDL.h>
extern "C" {
#ifdef _WIN32
__declspec(dllexport)
#endif
void main_scene(SDL_Renderer *scene_renderer);
}
