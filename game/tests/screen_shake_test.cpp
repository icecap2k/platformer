#include <doctest/doctest.h>

#include <cstdlib>

#include "domain/rules/damage.h"
#include "presentation/screen_shake.h"

using namespace presentation;

TEST_CASE("no shake when the player has not been hit") {
    CHECK(screenShake(0, domain::kInvulnerableSteps) == ShakeOffset{});
}

TEST_CASE("the shake starts strong, fades out and ends well before the invulnerability does") {
    const int total = domain::kInvulnerableSteps;
    int previous_amplitude = 100;
    int shaken_steps = 0;
    for (int since_hit = 0; since_hit < total; ++since_hit) {
        const ShakeOffset offset = screenShake(total - since_hit, total);
        const int amplitude = std::abs(offset.x);
        if (since_hit < kShakeStepsAfterHit) {
            CHECK(amplitude <= previous_amplitude);  // never grows
            previous_amplitude = amplitude;
            shaken_steps += amplitude > 0 ? 1 : 0;
        } else {
            CHECK(offset == ShakeOffset{});
        }
    }
    CHECK(screenShake(total, total).x == kShakeStartAmplitude);  // the first step is the strongest
    CHECK(shaken_steps >= 8);
    CHECK(shaken_steps <= kShakeStepsAfterHit);
}

TEST_CASE("the shake jolts back and forth rather than drifting one way") {
    const int total = domain::kInvulnerableSteps;
    int signs_changed = 0;
    int last = 0;
    for (int since_hit = 0; since_hit < 8; ++since_hit) {
        const int x = screenShake(total - since_hit, total).x;
        if (x != 0 && last != 0 && (x > 0) != (last > 0)) {
            ++signs_changed;
        }
        last = x;
    }
    CHECK(signs_changed >= 5);
}
