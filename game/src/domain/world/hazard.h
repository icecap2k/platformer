#pragma once

#include "domain/physics/aabb.h"

namespace domain {

// Something in a room that hurts the player on contact (today: spikes). Like a
// Coin it is level data: it never changes. Its hitbox is whatever rectangle the
// level author drew, so a level can be forgiving or strict about it.
struct Hazard {
    AABB bounds;
};

}  // namespace domain
