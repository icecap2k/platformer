#pragma once

#include <cstdint>

#include "domain/entities/player.h"

namespace presentation {

// Which drawing of the player to show. The value is the column of the frame in
// assets/sprites/sprites.png (left to right); the sheet is drawn by
// tools/art/make_player_sprites.py.
enum class PlayerPose {
    Idle0 = 0, Idle1, Idle2, Idle3,                  // breathing, hood tip swaying
    Run0 = 4, Run1, Run2, Run3, Run4, Run5,          // run cycle
    Rise = 10, Apex = 11, Fall = 12,                 // in the air
    Land = 13,                                       // squashed, just after landing
    Hurt = 14,                                       // thrown back by a hit
    Slide = 15,                                      // sliding down a ramp (and coasting after it)
    Crouch = 16,                                     // crouching on the ground (the box is half as tall)
};

inline constexpr int kPlayerFrameCount = 17;

// `ms_since_landing` value meaning "has not just landed".
inline constexpr std::uint64_t kNotLanding = ~std::uint64_t{0};

// Chooses the pose from what the player is doing. `time_ms` drives the idle and
// run cycles; `ms_since_landing` shows the squashed pose for a moment after
// touching the ground. Pure logic: no SDL, so it is unit tested.
PlayerPose playerPose(const domain::Player& player, std::uint64_t time_ms,
                      std::uint64_t ms_since_landing = kNotLanding);

// While invulnerable after a hit the player blinks: this says whether the sprite
// is shown on a given step. Always visible when not invulnerable.
bool isVisibleWhileInvulnerable(int invulnerable_steps);

// Should the sprite be mirrored? The player keeps looking where it last moved
// while standing still, hence the `was_facing_left` input. A player thrown back
// by a hit keeps looking at what hit it.
bool facesLeft(const domain::Player& player, bool was_facing_left);

// Frame (0..3) of the spinning coin.
int coinFrame(std::uint64_t time_ms);

}  // namespace presentation
