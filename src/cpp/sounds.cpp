
// i want gifs on code :( (that one angry bird having big ears gif here!!!)
//
#include "../header/sounds.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_audio.h>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;
namespace fs = std::filesystem;

// vars//

SDL_AudioDeviceID audio_device = 0;
struct sound_dt {
  SDL_AudioSpec spec{};
  Uint8 *buffer = nullptr;
  Uint32 length = 0;
};
struct playing_sound {
  SDL_AudioStream *stream = nullptr;
  bool looped = false;
  string name;
};
static unordered_map<string, sound_dt> sounds;
static vector<playing_sound> playing_sounds;
static fs::path sounds_dir = "sounds";

// funs//

bool verify_sounds() {
  if (!fs::exists(sounds_dir) || !fs::is_directory(sounds_dir)) {
    printf(">> sounds folder was not found.. ERROR\n");
    return false;
  }
  return true;
}

bool init_audio() {
  sounds_dir = string(SDL_GetBasePath()) + "sounds";
  if (!verify_sounds())
    return false;
  if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    printf(">> audio subsystem failed.. ERROR\n");
    return false;
  }
  audio_device =
      SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);

  if (!audio_device) {
    printf(">> audio device was not found.. ERROR\n");
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    return false;
  }
  return true;
}

bool load_sound(const string &name) {
  if (sounds.find(name) != sounds.end())
    return true;
  if (!verify_sounds())
    return false;
  fs::path path = sounds_dir / (name + ".wav");
  sound_dt sound{};
  string filepath = path.string();
  if (!SDL_LoadWAV(filepath.c_str(), &sound.spec, &sound.buffer,
                   &sound.length)) {
    printf(">> cannot load sound: %s .. ERROR\n", name.c_str());
    return false;
  }
  sounds.emplace(name, sound);
  return true;
}

void audio_play_sound(string name, bool looped, int pitch, int volume) {
  if (!audio_device)
    return;
  if (!load_sound(name))
    return;

  sound_dt &sound = sounds.at(name);
  SDL_AudioStream *stream = SDL_CreateAudioStream(&sound.spec, nullptr);
  if (!stream) {
    printf(">> failed to create audiostream.. ERROR\n");
    return;
  }

  pitch = clamp(pitch, 1, 10000);
  volume = clamp(volume, 0, 100);

  if (!SDL_SetAudioStreamFrequencyRatio(stream, pitch / 100.0f)) {
    printf(">> failed to set sound pitch.. ERROR\n");
    SDL_DestroyAudioStream(stream);
    return;
  }
  if (!SDL_SetAudioStreamGain(stream, volume / 100.0f)) {
    printf(">> failed to set sound volume.. ERROR\n");
    SDL_DestroyAudioStream(stream);
    return;
  }
  if (!SDL_BindAudioStream(audio_device, stream)) {
    printf(">> failed to bind audiostream.. ERROR\n");
    SDL_DestroyAudioStream(stream);
    return;
  }
  if (!SDL_PutAudioStreamData(stream, sound.buffer,
                              static_cast<int>(sound.length))) {
    printf(">> failed to put audio data.. ERROR\n");
    SDL_UnbindAudioStream(stream);
    SDL_DestroyAudioStream(stream);
    return;
  }
  playing_sounds.push_back({stream, looped, name});
}

void update_audio() {
  for (auto it = playing_sounds.begin(); it != playing_sounds.end();) {
    if (it->looped && SDL_GetAudioStreamQueued(it->stream) == 0) {
      sound_dt &sound = sounds.at(it->name);
      SDL_PutAudioStreamData(it->stream, sound.buffer,
                             static_cast<int>(sound.length));
      ++it;
      continue;
    }
    if (!it->looped && SDL_GetAudioStreamQueued(it->stream) == 0) {
      SDL_UnbindAudioStream(it->stream);
      SDL_DestroyAudioStream(it->stream);
      it = playing_sounds.erase(it);
    } else {
      ++it;
    }
  }
}

void shutdown_audio() {
  for (auto &sound : playing_sounds) {
    SDL_UnbindAudioStream(sound.stream);
    SDL_DestroyAudioStream(sound.stream);
  }
  playing_sounds.clear();
  for (auto &[name, sound] : sounds) {
    SDL_free(sound.buffer);
  }
  sounds.clear();
  if (audio_device) {
    SDL_CloseAudioDevice(audio_device);
    audio_device = 0;
  }
  SDL_QuitSubSystem(SDL_INIT_AUDIO);
}
