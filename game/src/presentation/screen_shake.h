#pragma once

namespace presentation {

// The short shake of the screen when the player is hurt. Purely cosmetic (ADR-0008) and
// stateless: it is worked out from how long ago the hit was, which the player already
// knows (`invulnerable_steps` counts down from kInvulnerableSteps after a hit).
inline constexpr int kShakeStepsAfterHit = 14;  // the shake lasts this many steps (about 0.23 s)
inline constexpr int kShakeStartAmplitude = 3;  // pixels at the first step; it fades to nothing

struct ShakeOffset {
    int x = 0;  // whole pixels, so the pixel art stays sharp
    int y = 0;

    bool operator==(const ShakeOffset&) const = default;
};

// Offset to draw the screen with, `invulnerable_steps` being the player's counter (0 when
// not invulnerable) and `invulnerable_total` the value it starts from after a hit.
ShakeOffset screenShake(int invulnerable_steps, int invulnerable_total);

}  // namespace presentation
