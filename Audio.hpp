#pragma once

#include <string>
#include <unordered_map>

#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>

struct AudioClip {
    Mix_Chunk *chunk = nullptr;
    bool loop = false;
};

class Audio {
public:
    bool init();
    void shutdown();

    bool loadClip(const std::string &name, const std::string &path, bool loop = false);
    void play(const std::string &name, int loops = 0, int channel = -1, int volume = -1);
    void stopChannel(int channel);
    void setChannelVolume(int channel, int volume); // 0-128
    bool isChannelPlaying(int channel) const;

    // Music (streaming) API
    bool loadMusic(const std::string &path);
    void playMusic(int loops = -1, int volume = 128);
    void stopMusic();
    void setMusicVolume(int volume); // 0-128
    bool setMusicPosition(double seconds); // seek within music if supported

private:
    std::unordered_map<std::string, AudioClip> clips_;
    Mix_Music *music_ = nullptr;
};
