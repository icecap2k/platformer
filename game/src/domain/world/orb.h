#pragma once

#include "domain/physics/aabb.h"

namespace domain {

// The value is the row of the orb in assets/sprites/orbs.png.
enum class OrbColour { Red = 0, Green, Yellow };

// One of the big orbs hidden around the cave: the exit only opens once every one of them has
// been taken (ADR-0021). Level data, like a Coin; whether it was taken is session state.
struct Orb {
    float x = 0.0F;  // top-left corner of its hitbox
    float y = 0.0F;
    float size = 16.0F;
    OrbColour colour = OrbColour::Red;

    AABB bounds() const { return AABB{x, y, size, size}; }
};

}  // namespace domain
