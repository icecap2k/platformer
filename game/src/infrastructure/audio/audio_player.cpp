#include "infrastructure/audio/audio_player.h"

#include <iostream>

namespace infrastructure {

AudioPlayer::AudioPlayer(const std::string& directory, const std::vector<std::string>& names) {
    if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        std::cerr << "[audio] unavailable (" << SDL_GetError() << "): the game will be silent\n";
        return;
    }
    device_ = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    SDL_AudioSpec device_spec;
    if (device_ == 0 || !SDL_GetAudioDeviceFormat(device_, &device_spec, nullptr)) {
        std::cerr << "[audio] no output device (" << SDL_GetError() << "): the game will be silent\n";
        if (device_ != 0) {
            SDL_CloseAudioDevice(device_);
            device_ = 0;
        }
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return;
    }

    for (const std::string& name : names) {
        const std::string path = directory + name + ".wav";
        Clip clip;
        SDL_AudioSpec wav_spec;
        if (!SDL_LoadWAV(path.c_str(), &wav_spec, &clip.data, &clip.length)) {
            std::cerr << "[audio] cannot load " << path << ": " << SDL_GetError() << '\n';
            continue;
        }
        clip.stream = SDL_CreateAudioStream(&wav_spec, &device_spec);
        if (clip.stream == nullptr || !SDL_BindAudioStream(device_, clip.stream)) {
            std::cerr << "[audio] cannot play " << path << ": " << SDL_GetError() << '\n';
            if (clip.stream != nullptr) {
                SDL_DestroyAudioStream(clip.stream);
            }
            SDL_free(clip.data);
            continue;
        }
        clips_.emplace(name, clip);
    }
    applyVolume();
    SDL_ResumeAudioDevice(device_);  // make sure it is playing
}

AudioPlayer::~AudioPlayer() {
    for (auto& [name, clip] : clips_) {
        SDL_DestroyAudioStream(clip.stream);
        SDL_free(clip.data);
    }
    clips_.clear();
    if (device_ != 0) {
        SDL_CloseAudioDevice(device_);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }
}

void AudioPlayer::play(const std::string& name) {
    if (muted_) {
        return;
    }
    const auto found = clips_.find(name);
    if (found == clips_.end()) {
        return;
    }
    SDL_ClearAudioStream(found->second.stream);  // start from the beginning
    SDL_PutAudioStreamData(found->second.stream, found->second.data, static_cast<int>(found->second.length));
}

void AudioPlayer::toggleMute() {
    muted_ = !muted_;
    if (muted_) {
        for (auto& [name, clip] : clips_) {
            SDL_ClearAudioStream(clip.stream);  // cut what is playing
        }
    }
    applyVolume();
}

void AudioPlayer::applyVolume() {
    if (device_ != 0) {
        SDL_SetAudioDeviceGain(device_, muted_ ? 0.0F : volume_);
    }
}

}  // namespace infrastructure
