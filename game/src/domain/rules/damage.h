#pragma once

#include <optional>

#include "domain/entities/player.h"
#include "domain/physics/aabb.h"
#include "domain/world/room.h"

namespace domain {

// How a hit works. Everything that hurts (spikes today; enemies and traps
// later) goes through applyDamage, so they all feel the same.
inline constexpr int kHurtSteps = 14;          // thrown back for 14 steps (about 8 px)
inline constexpr int kInvulnerableSteps = 90;  // 1.5 s during which it cannot be hurt again

// A hit can land only if the player is alive and not recovering from another.
bool canBeHurt(const Player& player);

// Takes `amount` hearts (never below 0), starts the invulnerability, and throws
// the player back in `direction` (-1 left, +1 right) with a small hop.
void applyDamage(Player& player, int amount, int direction);

// Which way to throw the player when it touches something at `source`: away
// from it. If they are exactly aligned, against the way the player was moving.
int knockbackDirection(const Player& player, const AABB& source);

// The first hazard of the room the player is touching, if any.
std::optional<AABB> hazardTouching(const Player& player, const Room& room);

}  // namespace domain
