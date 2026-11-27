#pragma once

#include <SDL3/SDL.h>

#include <map>
#include <string>
#include <vector>

namespace infrastructure {

// Plays short sound effects and one looping piece of music (WAV files) through SDL. It knows nothing about the game: it
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

    // How loud one sound is compared with the others (1.0 = as recorded).
    void setGain(const std::string& name, float gain);

    // Starts a piece of music that repeats for ever. Returns false (and stays silent) if there
    // is no audio device or the file cannot be loaded.
    bool startMusic(const std::string& path);
    void setMusicGain(float gain);
    // Call it every frame: it queues the next round of the music before the current one ends.
    void update();

    void toggleMute();
    bool muted() const { return muted_; }

private:
    struct Clip {
        Uint8* data = nullptr;
        Uint32 length = 0;
        SDL_AudioStream* stream = nullptr;
    };

    // Loads a WAV file into a clip with its stream bound to the device.
    bool load(const std::string& path, const SDL_AudioSpec& device_spec, Clip& clip);
    void applyVolume();

    SDL_AudioDeviceID device_ = 0;
    std::map<std::string, Clip> clips_;
    Clip music_;  // data == nullptr while there is no music
    bool muted_ = false;
    float volume_ = 0.5F;
};

}  // namespace infrastructure
