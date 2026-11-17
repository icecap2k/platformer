#pragma once

#include "domain/entities/player.h"
#include "domain/input/input_state.h"
#include "domain/world/tile_map.h"

namespace domain {

// Tuning values, in pixels and pixels-per-step (one step = 1/60 s).
inline constexpr float kRunSpeed = 1.5F;
inline constexpr float kGravity = 0.2F;
inline constexpr float kJumpSpeed = 5.0F;
inline constexpr float kMaxFallSpeed = 6.0F;

// Advances the player by exactly one fixed simulation step, colliding with
// the solid tiles of `map`.
//
// Movement is resolved one axis at a time (first X, then Y). Doing both at
// once makes it ambiguous which face of a tile was hit; separating the axes
// makes "landing", "bumping the head" and "hitting a wall" trivial to tell
// apart.
//
// Speeds must stay below one tile per step, otherwise the player could skip
// over a thin tile ("tunnelling").
void stepPlayer(Player& player, const InputState& input, const TileMap& map);

}  // namespace domain
