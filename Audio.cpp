#include "Audio.hpp"
#include <iostream>

bool Audio::init() {
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        std::cerr << "SDL audio init failed: " << SDL_GetError() << std::endl;
        return false;
    }
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        std::cerr << "Mix_OpenAudio failed: " << Mix_GetError() << std::endl;
        return false;
    }
    Mix_AllocateChannels(16);
    return true;
}

void Audio::shutdown() {
    for (auto &kv : clips_) {
        if (kv.second.chunk) {
            Mix_FreeChunk(kv.second.chunk);
        }
    }
    clips_.clear();
    if (music_) {
        Mix_FreeMusic(music_);
        music_ = nullptr;
    }
    Mix_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
}

bool Audio::loadClip(const std::string &name, const std::string &path, bool loop) {
    Mix_Chunk *chunk = Mix_LoadWAV(path.c_str());
    if (!chunk) {
        std::cerr << "Failed to load " << path << ": " << Mix_GetError() << std::endl;
        return false;
    }
    clips_[name] = AudioClip{chunk, loop};
    return true;
}

void Audio::play(const std::string &name, int loops, int channel, int volume) {
    auto it = clips_.find(name);
    if (it == clips_.end() || !it->second.chunk) return;
    if (volume >= 0) {
        Mix_VolumeChunk(it->second.chunk, volume);
    }
    Mix_PlayChannel(channel, it->second.chunk, loops);
}

void Audio::stopChannel(int channel) {
    Mix_HaltChannel(channel);
}

void Audio::setChannelVolume(int channel, int volume) {
    Mix_Volume(channel, volume);
}

bool Audio::isChannelPlaying(int channel) const {
    return Mix_Playing(channel) != 0;
}

bool Audio::loadMusic(const std::string &path) {
    if (music_) {
        Mix_FreeMusic(music_);
        music_ = nullptr;
    }
    music_ = Mix_LoadMUS(path.c_str());
    if (!music_) {
        std::cerr << "Failed to load music " << path << ": " << Mix_GetError() << std::endl;
        return false;
    }
    return true;
}

void Audio::playMusic(int loops, int volume) {
    if (!music_) return;
    Mix_VolumeMusic(volume);
    Mix_PlayMusic(music_, loops);
}

void Audio::stopMusic() {
    Mix_HaltMusic();
}

void Audio::setMusicVolume(int volume) {
    Mix_VolumeMusic(volume);
}

bool Audio::setMusicPosition(double seconds) {
    // Returns 0 on success, -1 on failure
    if (Mix_SetMusicPosition(seconds) == 0) return true;
    return false;
}
