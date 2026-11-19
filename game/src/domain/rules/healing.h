#pragma once

#include "domain/entities/player.h"

namespace domain {

// Fruits heal this many hearts.
inline constexpr int kFruitHealing = 1;

// Gives hearts back, never above kMaxHealth, and returns how many were really
// gained (0 when already at full health: the fruit is wasted).
int heal(Player& player, int amount);

}  // namespace domain
