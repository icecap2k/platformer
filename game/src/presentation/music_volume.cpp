#include "presentation/music_volume.h"

#include <algorithm>

namespace presentation {

void MusicVolume::change(int steps) {
    step_ = std::clamp(step_ + steps, 0, kMusicSteps);
}

float MusicVolume::gain(bool paused) const {
    const float share = static_cast<float>(step_) / static_cast<float>(kMusicSteps);
    return share * share * (paused ? kPausedMusicShare : 1.0F);
}

std::string MusicVolume::label() const {
    return step_ == 0 ? "MUSIC OFF" : "MUSIC " + std::to_string(step_) + "/" + std::to_string(kMusicSteps);
}

}  // namespace presentation
