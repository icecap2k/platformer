#pragma once

#include "domain/physics/aabb.h"

namespace domain {

inline constexpr int kMaxHealth = 3;  // hearts: every hit costs one

// The collision box is 16 px tall standing and half of that crouching (the feet
// stay where they are). See physics/player_physics.h.
inline constexpr float kStandingHeight = 16.0F;
inline constexpr float kCrouchHeight = 8.0F;

// The controllable character. Plain data: the rules that move it live in
// physics/player_physics.h, so this struct stays easy to create in tests.
struct Player {
    float x = 0.0F;  // top-left corner of the collision box
    float y = 0.0F;
    float velocity_x = 0.0F;  // pixels per simulation step
    float velocity_y = 0.0F;
    float width = 8.0F;
    float height = kStandingHeight;
    bool on_ground = false;

    // Jump feel (see physics/player_physics.h).
    int coyote_steps = 0;     // steps left in which a jump is still allowed after leaving the ground
    int jump_buffer_steps = 0;  // steps left in which a jump pressed too early is still remembered
    bool jump_held = false;   // was the jump button down on the previous step?
    bool crouching = false;   // the box is kCrouchHeight tall (it may be forced by a low ceiling)

    // Health and damage (see rules/damage.h).
    int health = kMaxHealth;
    int invulnerable_steps = 0;  // while > 0 the player cannot be hurt again
    int hurt_steps = 0;          // while > 0 the player is thrown back and cannot steer
    int hurt_direction = 1;      // -1 = thrown to the left, +1 = to the right

    // Sliding down a ramp (see physics/player_physics.h): while > 0 the player
    // keeps the speed it picked up, decaying; `slide_direction` is -1 or +1.
    int slide_steps = 0;
    int slide_direction = 0;

    AABB bounds() const { return AABB{x, y, width, height}; }
};

}  // namespace domain
