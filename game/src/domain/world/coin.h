#pragma once

#include "domain/physics/aabb.h"

namespace domain {

// A collectible coin placed in a room. Whether it has been picked up is NOT
// stored here: a Coin is level data (it never changes), while "collected" is
// session state (see CoinCollection).
struct Coin {
    float x = 0.0F;  // top-left corner
    float y = 0.0F;
    float size = 8.0F;

    AABB bounds() const { return AABB{x, y, size, size}; }
};

}  // namespace domain
