#pragma once

#include <string>

namespace presentation {

// How loud the music is, as the player sets it: steps from 0 (off) to kMusicSteps. Ears hear
// loudness unevenly, so the gain follows the square of the step: each press sounds like an equal
// change. Pure logic (no SDL), so it is unit tested.
inline constexpr int kMusicSteps = 10;
inline constexpr int kDefaultMusicStep = 7;
inline constexpr float kPausedMusicShare = 0.4F;  // quieter behind the pause menu

class MusicVolume {
public:
    // `steps` up (positive) or down (negative), staying between 0 and kMusicSteps.
    void change(int steps);

    int step() const { return step_; }
    // From 0.0 to 1.0, what the audio player should apply.
    float gain(bool paused) const;
    // What to show on screen after a change.
    std::string label() const;

private:
    int step_ = kDefaultMusicStep;
};

}  // namespace presentation
