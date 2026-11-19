#pragma once

#include "domain/physics/aabb.h"

namespace domain {

// A stalactite trap placed in a room. Like a Coin or a Hazard it is level data:
// it never changes. What it is doing (waiting, shaking, falling, gone) is session
// state, see stalactite_traps.h.
struct FallingStalactite {
    AABB bounds;  // where it hangs, also its hitbox
};

}  // namespace domain
