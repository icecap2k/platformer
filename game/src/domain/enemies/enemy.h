#pragma once

#include "domain/physics/aabb.h"

namespace domain {

enum class EnemyKind {
    Walker,  // a beetle that paces along the floor
    Bat,     // flies around its spot and goes for the player who comes near
};

// How near (px, centre to centre) the player has to come for a bat to notice, unless the level
// says otherwise.
inline constexpr float kBatDetectRadius = 64.0F;

// An enemy placed in a room. Like a Coin or a Hazard it is level data and never changes:
// `bounds` is where it starts, and also the size of its hitbox. Where it is now and whether it is
// still alive is session state, see enemy_patrols.h.
struct Enemy {
    AABB bounds;
    EnemyKind kind = EnemyKind::Walker;
    float detect_radius = kBatDetectRadius;  // bats only
};

}  // namespace domain
