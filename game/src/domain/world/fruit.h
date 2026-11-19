#pragma once

#include "domain/physics/aabb.h"

namespace domain {

// A healing fruit placed in a room. Like a Coin it is level data: it never
// changes, and whether it has been eaten is session state (see PickupCollection).
struct Fruit {
    float x = 0.0F;  // top-left corner
    float y = 0.0F;
    float size = 8.0F;

    AABB bounds() const { return AABB{x, y, size, size}; }
};

}  // namespace domain
