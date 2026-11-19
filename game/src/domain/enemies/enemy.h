#pragma once

#include "domain/physics/aabb.h"

namespace domain {

// A patrolling enemy placed in a room (today there is one kind: it walks back and
// forth along the floor). Like a Coin or a Hazard it is level data and never
// changes: `bounds` is where it starts, and also the size of its hitbox. Where it
// is now and whether it is still alive is session state, see enemy_patrols.h.
struct Enemy {
    AABB bounds;
};

}  // namespace domain
