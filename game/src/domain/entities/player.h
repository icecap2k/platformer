#pragma once

#include "domain/physics/aabb.h"

namespace domain {

// The controllable character. Plain data: the rules that move it live in
// physics/player_physics.h, so this struct stays easy to create in tests.
struct Player {
    float x = 0.0F;  // top-left corner of the collision box
    float y = 0.0F;
    float velocity_x = 0.0F;  // pixels per simulation step
    float velocity_y = 0.0F;
    float width = 8.0F;
    float height = 16.0F;
    bool on_ground = false;

    AABB bounds() const { return AABB{x, y, width, height}; }
};

}  // namespace domain
