#include "presentation/screen_shake.h"

namespace presentation {

ShakeOffset screenShake(int invulnerable_steps, int invulnerable_total) {
    const int since_hit = invulnerable_total - invulnerable_steps;
    if (invulnerable_steps <= 0 || since_hit < 0 || since_hit >= kShakeStepsAfterHit) {
        return {};
    }
    // The amplitude fades linearly (3, 3, 3, 2, 2, ... 1, 0); the direction flips every step,
    // and vertically every second step, so it jolts instead of drifting.
    const int amplitude = kShakeStartAmplitude * (kShakeStepsAfterHit - since_hit) / kShakeStepsAfterHit;
    const int sign_x = since_hit % 2 == 0 ? 1 : -1;
    const int sign_y = (since_hit / 2) % 2 == 0 ? 1 : -1;
    return ShakeOffset{sign_x * amplitude, sign_y * (amplitude + 1) / 2};
}

}  // namespace presentation
