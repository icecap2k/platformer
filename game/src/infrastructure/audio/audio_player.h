#pragma once

#include <SDL3/SDL.h>

#include <map>
#include <string>
#include <vector>

namespace infrastructure {

// Plays short sound effects (WAV files) through SDL. It knows nothing about the game: it
// is told a sound NAME and plays <directory><name>.wav. Which sound goes with which
// event is decided by presentation::soundFor, and main wires them together.
//
// Every sound has its own audio stream bound to one device, so different sounds mix
// together and playing a sound again restarts it. If the machine has no usable audio
// output, or a file is missing, the player says so once and then stays silent: the game
// must never fail because of sound.
class AudioPlayer {
public:
    AudioPlayer(const std::string& directory, const std::vector<std::string>& names);
    ~AudioPlayer();

    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;

    // True when there is an output device (even if some sound failed to load).
    bool available() const { return device_ != 0; }
    int loadedCount() const { return static_cast<int>(clips_.size()); }

    // Plays the sound from the start. Unknown names and a muted player do nothing.
    void play(const std::string& name);

    void toggleMute();
    bool muted() const { return muted_; }

private:
    struct Clip {
        Uint8* data = nullptr;
        Uint32 length = 0;
        SDL_AudioStream* stream = nullptr;
    };

    void applyVolume();

    SDL_AudioDeviceID device_ = 0;
    std::map<std::string, Clip> clips_;
    bool muted_ = false;
    float volume_ = 0.5F;
};

}  // namespace infrastructure
