#pragma once

#include "domain/physics/aabb.h"

namespace domain {

// A beacon placed in a room. Touching it makes it the place where the player comes back
// after losing every heart (ADR-0015). Like a Coin it is level data and never changes;
// which one is active is session state (GameSession::lastCheckpoint).
struct Checkpoint {
    float x = 0.0F;  // top-left corner
    float y = 0.0F;
    float size = 8.0F;

    AABB bounds() const { return AABB{x, y, size, size}; }
};

}  // namespace domain
