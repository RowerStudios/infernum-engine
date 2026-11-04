// sounds.h api
// // the fuck is api . edit: (its called application programming interface)

#pragma once
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_stdinc.h>
#include <filesystem>
#include <string>
using namespace std;

bool verify_sounds();
bool init_audio();
bool load_sound(const std::string &name);
void audio_play_sound(std::string name, bool looped, int pitch, int volume);
void update_audio();
void shutdown_audio();
