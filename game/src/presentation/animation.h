#pragma once

#include <cstdint>

#include "domain/entities/player.h"

namespace presentation {

// Which drawing of the player to show. The order matches the frames in
// assets/sprites/sprites.png (left to right).
enum class PlayerPose { Idle = 0, Walk1 = 1, Walk2 = 2, Jump = 3 };

// Chooses the pose from what the player is doing. `time_ms` drives the walk
// cycle. Pure logic: no SDL, so it is unit tested.
PlayerPose playerPose(const domain::Player& player, std::uint64_t time_ms);

// Should the sprite be mirrored? The player keeps looking where it last moved
// while standing still, hence the `was_facing_left` input.
bool facesLeft(const domain::Player& player, bool was_facing_left);

// Frame (0..3) of the spinning coin.
int coinFrame(std::uint64_t time_ms);

}  // namespace presentation
