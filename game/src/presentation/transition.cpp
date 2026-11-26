#include "presentation/transition.h"

#include <algorithm>

namespace presentation {

float easeInOut(float t) {
    t = std::clamp(t, 0.0F, 1.0F);
    if (t < 0.5F) {
        return 4.0F * t * t * t;
    }
    const float rest = 2.0F - 2.0F * t;
    return 1.0F - rest * rest * rest * 0.5F;
}

void Transition::startSlide(domain::Side exited_through, std::uint64_t now_ms) {
    kind_ = TransitionKind::Slide;
    side_ = exited_through;
    started_ms_ = now_ms;
}

void Transition::startFade(std::uint64_t now_ms) {
    kind_ = TransitionKind::Fade;
    started_ms_ = now_ms;
}

std::uint64_t Transition::duration() const {
    return kind_ == TransitionKind::Slide ? kSlideMs : kFadeMs;
}

bool Transition::active(std::uint64_t now_ms) const {
    return kind_ != TransitionKind::None && now_ms - started_ms_ < duration();
}

float Transition::progress(std::uint64_t now_ms) const {
    if (!active(now_ms)) {
        return 1.0F;
    }
    return easeInOut(static_cast<float>(now_ms - started_ms_) / static_cast<float>(duration()));
}

SlideOffsets Transition::slide(std::uint64_t now_ms, float width, float height) const {
    // The direction of the neighbouring room, seen from the old one.
    const float dx = side_ == domain::Side::East ? 1.0F : (side_ == domain::Side::West ? -1.0F : 0.0F);
    const float dy = side_ == domain::Side::South ? 1.0F : (side_ == domain::Side::North ? -1.0F : 0.0F);
    const float done = progress(now_ms);
    // The camera travels one screen towards it: the old picture leaves the opposite way and the
    // new one follows right behind, so there is never a gap between them.
    return SlideOffsets{-dx * width * done, -dy * height * done, dx * width * (1.0F - done),
                        dy * height * (1.0F - done)};
}

}  // namespace presentation
