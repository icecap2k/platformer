#pragma once

#include <cstdint>

#include "domain/world/side.h"

namespace presentation {

// The change from one picture to the next, with easing: it starts gently and arrives gently.
// Between rooms the old one slides away while the new one slides in from the side the player
// left through; between screens (title, game, ending) and on respawn the new picture fades in
// over the old one. The game stands still while it lasts.
//
// Pure logic (no SDL), so it is unit tested.

inline constexpr std::uint64_t kSlideMs = 450;
inline constexpr std::uint64_t kFadeMs = 600;

// Cubic ease in and out: 0 -> 0, 0.5 -> 0.5, 1 -> 1, flat at both ends. `t` is clamped to 0..1.
float easeInOut(float t);

enum class TransitionKind { None, Slide, Fade };

// Where to draw both pictures during a slide, in logical pixels from their resting place.
struct SlideOffsets {
    float old_x = 0.0F;
    float old_y = 0.0F;
    float new_x = 0.0F;
    float new_y = 0.0F;
};

class Transition {
public:
    // `exited_through` is the edge of the old room the player left by.
    void startSlide(domain::Side exited_through, std::uint64_t now_ms);
    void startFade(std::uint64_t now_ms);

    bool active(std::uint64_t now_ms) const;
    TransitionKind kind() const { return kind_; }

    // How far it has got, eased: 0 at the start, 1 once it is over.
    float progress(std::uint64_t now_ms) const;

    // For a slide over a screen of `width` x `height`.
    SlideOffsets slide(std::uint64_t now_ms, float width, float height) const;

private:
    std::uint64_t duration() const;

    TransitionKind kind_ = TransitionKind::None;
    domain::Side side_ = domain::Side::East;
    std::uint64_t started_ms_ = 0;
};

}  // namespace presentation
