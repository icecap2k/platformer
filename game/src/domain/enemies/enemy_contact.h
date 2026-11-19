#pragma once

#include "domain/entities/player.h"
#include "domain/physics/aabb.h"

namespace domain {

// How far (px) the player's feet may have sunk below an enemy's top and still count
// as landing on it. A fall moves up to 6 px a step, so a stomp is seen a step late.
inline constexpr float kStompDepth = 6.0F;
// Speed of the bounce after a stomp. Without holding jump the usual jump cut
// (kJumpCutSpeed) trims it to a small hop; holding it gives the full bounce.
inline constexpr float kStompBounceSpeed = 3.2F;

// Is the player, who touches `enemy`, coming down on top of it? It must be falling and
// have its feet near the enemy's top; anything else (walking into it, jumping up
// into it, running past at the same height) is a hit on the player.
bool isStomp(const Player& player, const AABB& enemy);

// Bounces the player off a stomped enemy: up, in the air, no coyote, no slide.
void bounceOffEnemy(Player& player);

}  // namespace domain
