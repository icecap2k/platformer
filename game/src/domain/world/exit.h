#pragma once

#include "domain/physics/aabb.h"

namespace domain {

// The way out of the cave: touching it wins the game (ADR-0018). Level data, like a Coin.
struct Exit {
    float x = 0.0F;  // top-left corner of its hitbox
    float y = 0.0F;
    float size = 8.0F;

    AABB bounds() const { return AABB{x, y, size, size}; }
};

}  // namespace domain
