#pragma once

#include <cstdint>

#include "domain/entities/player.h"
#include "domain/world/orb.h"

namespace presentation {

// Which drawing of the player to show. The value is the column of the frame in
// assets/sprites/hero.png (left to right); the sheet is drawn by
// tools/art/make_hero_sprites.py.
enum class PlayerPose {
    Idle0 = 0, Idle1, Idle2, Idle3,                  // breathing, hair swaying
    Run0 = 4, Run1, Run2, Run3, Run4, Run5, Run6, Run7,  // run cycle
    Rise = 12, Apex = 13, Fall = 14,                 // in the air
    Land = 15,                                       // squashed, just after landing
    Hurt = 16,                                       // thrown back by a hit
    Slide = 17,                                      // sliding down a ramp (and coasting after it)
    Crouch = 18,                                     // crouching on the ground (the box is half as tall)
};

inline constexpr int kPlayerFrameCount = 19;
inline constexpr int kRunFrames = 8;

// `ms_since_landing` value meaning "has not just landed".
inline constexpr std::uint64_t kNotLanding = ~std::uint64_t{0};

// Chooses the pose from what the player is doing. `time_ms` drives the idle and
// run cycles; `ms_since_landing` shows the squashed pose for a moment after
// touching the ground. Pure logic: no SDL, so it is unit tested.
PlayerPose playerPose(const domain::Player& player, std::uint64_t time_ms,
                      std::uint64_t ms_since_landing = kNotLanding);

// Row (0 .. kHairFlutterFrames - 1) of assets/sprites/hero.png to draw for a pose at
// `time_ms`: each row is the same pose with the long hair at another moment of its wave.
// It waves lazily when standing and faster when moving fast.
inline constexpr int kHairFlutterFrames = 4;

int hairFlutter(PlayerPose pose, std::uint64_t time_ms);

// While invulnerable after a hit the player blinks: this says whether the sprite
// is shown on a given step. Always visible when not invulnerable.
bool isVisibleWhileInvulnerable(int invulnerable_steps);

// Should the sprite be mirrored? The player keeps looking where it last moved
// while standing still, hence the `was_facing_left` input. A player thrown back
// by a hit keeps looking at what hit it.
bool facesLeft(const domain::Player& player, bool was_facing_left);

// Frame (0..3) of the spinning coin.
int coinFrame(std::uint64_t time_ms);

// Frame (0..3) of the glowing healing fruit: its halo breathes (grows and brightens,
// then shrinks).
int fruitFrame(std::uint64_t time_ms);

// How many pixels (-1, 0 or +1) the fruit floats up or down: a slow bob.
int fruitBob(std::uint64_t time_ms);

// A squashed enemy blinks while it has `steps_left` steps or fewer before it disappears
// (the last domain::kSquashBlinkSteps); this says whether it is drawn on that step. Always drawn before that.
bool isVisibleWhileFading(int steps_left);

// Frame (1..2) of an active checkpoint's glow (frame 0 is the unlit one).
int checkpointFrame(std::uint64_t time_ms);

// Frame (0..kOrbFrames - 1) of an orb: it swells and shrinks once while its bubbles rise. Each
// colour is a third of the loop apart, so they do not beat together. The column after the last
// frame in orbs.png is the dashed silhouette left where an orb was taken.
inline constexpr int kOrbFrames = 12;
int orbFrame(domain::OrbColour colour, std::uint64_t time_ms);

// How many pixels (-2..2) an orb floats up or down: a slow, smooth bob.
int orbBob(domain::OrbColour colour, std::uint64_t time_ms);

// Frame (0..1) of the open cave exit: its light brightens and dims.
int exitFrame(std::uint64_t time_ms);

// The exit's portcullis (frames of exit.png). Closed while orbs are missing. Once it is open, the
// first time it is seen after entering its room it stays shut for kGateDelayMs, rises through
// kGateFrames frames and then shows the open exit.
inline constexpr int kExitClosedFrame = 2;
inline constexpr int kGateFirstFrame = 3;
inline constexpr int kGateFrames = 12;
inline constexpr std::uint64_t kGateDelayMs = 350;
inline constexpr std::uint64_t kGateFrameMs = 100;

// Which frame of exit.png to draw. `ms_open` = how long the open exit has been on screen.
int exitSpriteFrame(bool open, std::uint64_t ms_open, std::uint64_t time_ms);

// Frame (0..kEnemyWalkFrames - 1) of the walking enemy: the cycle of its legs. The frame
// after those in the sheet is the squashed one.
inline constexpr int kEnemyWalkFrames = 4;
int enemyFrame(std::uint64_t time_ms);

}  // namespace presentation
