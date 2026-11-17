#include <doctest/doctest.h>

#include <cmath>
#include <set>

#include "presentation/drip.h"

using namespace presentation;

namespace {

constexpr float kDistance = 100.0F;
constexpr std::uint64_t kStepMs = 10;
constexpr std::uint64_t kSeconds = 1000;

}  // namespace

TEST_CASE("dripAt is deterministic") {
    for (std::uint64_t t = 0; t < 60 * kSeconds; t += 137) {
        const Drip a = dripAt(7, 12, kDistance, t);
        const Drip b = dripAt(7, 12, kDistance, t);
        CHECK(a.phase == b.phase);
        CHECK(a.frame == b.frame);
        CHECK(a.fall_offset == b.fall_offset);
    }
}

TEST_CASE("dripAt phases") {
    SUBCASE("a stalactite right above the ground never drips") {
        for (std::uint64_t t = 0; t < 60 * kSeconds; t += kStepMs) {
            CHECK(dripAt(3, 5, kMinFallDistance - 1.0F, t).phase == DripPhase::None);
        }
    }

    SUBCASE("a drop forms, falls and splashes, in that order, with the right frames") {
        DripPhase previous = DripPhase::None;
        std::set<int> frames;
        bool saw_forming = false;
        bool saw_falling = false;
        bool saw_splash = false;
        for (std::uint64_t t = 0; t < 120 * kSeconds; t += kStepMs) {
            const Drip drip = dripAt(3, 5, kDistance, t);
            switch (drip.phase) {
                case DripPhase::None:
                    break;
                case DripPhase::Forming:
                    saw_forming = true;
                    CHECK((drip.frame == kDripFormingSmall || drip.frame == kDripFormingLarge));
                    CHECK(previous != DripPhase::Falling);  // never goes back
                    CHECK(previous != DripPhase::Splash);
                    break;
                case DripPhase::Falling:
                    saw_falling = true;
                    CHECK(drip.frame == kDripFalling);
                    CHECK(drip.fall_offset >= 0.0F);
                    CHECK(drip.fall_offset <= kDistance);
                    CHECK(previous != DripPhase::Splash);
                    CHECK(previous != DripPhase::None);  // it always forms first
                    break;
                case DripPhase::Splash:
                    saw_splash = true;
                    CHECK((drip.frame == kDripSplashA || drip.frame == kDripSplashB));
                    CHECK(previous != DripPhase::None);  // it always falls first
                    break;
            }
            frames.insert(drip.frame);
            previous = drip.phase;
        }
        CHECK(saw_forming);
        CHECK(saw_falling);
        CHECK(saw_splash);
    }

    SUBCASE("the drop accelerates while falling and reaches the ground") {
        // Collect the offsets of one fall, in time order.
        float last = -1.0F;
        float last_step = 0.0F;
        bool accelerating = true;
        float max_offset = 0.0F;
        for (std::uint64_t t = 0; t < 120 * kSeconds; t += 5) {
            const Drip drip = dripAt(3, 5, kDistance, t);
            if (drip.phase != DripPhase::Falling) {
                last = -1.0F;
                last_step = 0.0F;  // a new drop starts from rest
                continue;
            }
            if (last >= 0.0F) {
                const float step = drip.fall_offset - last;
                CHECK(step >= 0.0F);
                if (step + 0.01F < last_step) {
                    accelerating = false;
                }
                last_step = step;
            }
            last = drip.fall_offset;
            max_offset = std::fmax(max_offset, drip.fall_offset);
        }
        CHECK(accelerating);
        CHECK(max_offset > kDistance * 0.9F);
    }
}

TEST_CASE("stalactites do not drip together or all the time") {
    constexpr int kStalactites = 12;
    constexpr std::uint64_t kDuration = 300 * kSeconds;
    std::uint64_t active_ms = 0;
    std::uint64_t all_forming_at_once = 0;
    std::set<std::uint64_t> first_drops;

    for (std::uint64_t t = 0; t < kDuration; t += kStepMs) {
        int dripping = 0;
        for (int column = 0; column < kStalactites; ++column) {
            if (dripAt(column * 3, 11, kDistance, t).phase != DripPhase::None) {
                ++dripping;
                active_ms += kStepMs;
            }
        }
        if (dripping == kStalactites) {
            all_forming_at_once += kStepMs;
        }
    }
    for (int column = 0; column < kStalactites; ++column) {
        for (std::uint64_t t = 0; t < kDuration; t += kStepMs) {
            if (dripAt(column * 3, 11, kDistance, t).phase != DripPhase::None) {
                first_drops.insert(t);
                break;
            }
        }
    }

    // Each one is busy only a small part of the time ("from time to time")...
    const double busy = static_cast<double>(active_ms) / static_cast<double>(kDuration * kStalactites);
    CHECK(busy < 0.25);
    CHECK(busy > 0.02);
    // ...never all together...
    CHECK(all_forming_at_once == 0);
    // ...and they start at different moments.
    CHECK(first_drops.size() >= static_cast<std::size_t>(kStalactites * 2 / 3));
}

TEST_CASE("the gaps between drops vary") {
    std::set<std::uint64_t> gaps;
    std::uint64_t last_start = 0;
    bool was_none = true;
    for (std::uint64_t t = 0; t < 600 * kSeconds; t += kStepMs) {
        const bool none = dripAt(9, 20, kDistance, t).phase == DripPhase::None;
        if (was_none && !none) {
            if (last_start != 0) {
                gaps.insert((t - last_start) / 500);  // buckets of half a second
            }
            last_start = t;
        }
        was_none = none;
    }
    CHECK(gaps.size() >= 4);
}

TEST_CASE("fallTimeMs grows with the distance") {
    CHECK(fallTimeMs(25.0F) < fallTimeMs(100.0F));
    CHECK(fallTimeMs(100.0F) == doctest::Approx(408.0F).epsilon(0.05));
}
