#include <doctest/doctest.h>

#include "presentation/transition.h"

using namespace presentation;
using domain::Side;

TEST_CASE("easing starts and ends gently") {
    CHECK(easeInOut(0.0F) == 0.0F);
    CHECK(easeInOut(0.5F) == doctest::Approx(0.5F));
    CHECK(easeInOut(1.0F) == 1.0F);
    CHECK(easeInOut(-1.0F) == 0.0F);
    CHECK(easeInOut(2.0F) == 1.0F);
    float previous = 0.0F;
    for (int i = 1; i <= 100; ++i) {
        const float value = easeInOut(static_cast<float>(i) / 100.0F);
        CHECK(value >= previous);  // never goes back
        previous = value;
    }
    // Slow at the ends, quick in the middle.
    CHECK(easeInOut(0.1F) < 0.1F * 0.5F);
    CHECK(easeInOut(0.55F) - easeInOut(0.45F) > 0.2F);
}

TEST_CASE("a transition lasts its time and then it is over") {
    Transition transition;
    CHECK_FALSE(transition.active(0));
    CHECK(transition.progress(0) == 1.0F);

    transition.startFade(1000);
    CHECK(transition.kind() == TransitionKind::Fade);
    CHECK(transition.active(1000));
    CHECK(transition.progress(1000) == 0.0F);
    CHECK(transition.progress(1000 + kFadeMs / 2) == doctest::Approx(0.5F));
    CHECK(transition.active(1000 + kFadeMs - 1));
    CHECK_FALSE(transition.active(1000 + kFadeMs));
    CHECK(transition.progress(1000 + kFadeMs) == 1.0F);

    transition.startSlide(Side::East, 5000);
    CHECK(transition.kind() == TransitionKind::Slide);
    CHECK_FALSE(transition.active(5000 + kSlideMs));
}

TEST_CASE("a slide moves towards the side the player left through, with no gap") {
    Transition transition;
    transition.startSlide(Side::East, 0);
    CHECK(transition.slide(0, 320, 240).old_x == 0.0F);
    CHECK(transition.slide(0, 320, 240).new_x == 320.0F);  // waiting to the east
    CHECK(transition.slide(kSlideMs, 320, 240).old_x == -320.0F);
    CHECK(transition.slide(kSlideMs, 320, 240).new_x == 0.0F);
    for (std::uint64_t t = 0; t <= kSlideMs; t += 15) {
        const SlideOffsets at = transition.slide(t, 320, 240);
        CHECK(at.new_x - at.old_x == doctest::Approx(320.0F));
        CHECK(at.new_y == 0.0F);
    }

    transition.startSlide(Side::West, 0);
    CHECK(transition.slide(0, 320, 240).new_x == -320.0F);
    transition.startSlide(Side::North, 0);
    CHECK(transition.slide(0, 320, 240).new_y == -240.0F);
    CHECK(transition.slide(kSlideMs, 320, 240).old_y == 240.0F);
    transition.startSlide(Side::South, 0);
    CHECK(transition.slide(0, 320, 240).new_y == 240.0F);
}
