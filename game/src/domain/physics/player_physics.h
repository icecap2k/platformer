#pragma once

#include "domain/entities/player.h"
#include "domain/input/input_state.h"
#include "domain/world/tile_map.h"

namespace domain {

// Tuning values, in pixels and pixels-per-step (one step = 1/60 s).
inline constexpr float kRunSpeed = 1.5F;
inline constexpr float kGravity = 0.2F;
// Jump height is v^2 / (2 g): 4.2 gives about 44 px (it was 5.0 = 62 px; now 70% of that).
inline constexpr float kJumpSpeed = 4.2F;
inline constexpr float kMaxFallSpeed = 6.0F;

// Jump feel.
// Coyote time: after walking off a ledge you can still start a jump during this
// many airborne steps.
// (4 steps = about 0.07 s; it was 6.)
inline constexpr int kCoyoteSteps = 4;
// Jump buffering: a fresh jump press that cannot be used yet (still in the air, no
// coyote time left) is remembered for this many steps, and the jump happens on the
// first step the player could jump. (6 steps = 0.1 s.) Hurt or crouching players cannot
// jump, and being hit forgets the press.
inline constexpr int kJumpBufferSteps = 6;
// Being hurt: the player is thrown back at this speed (pixels per step) for a
// few steps, with a small hop (kHurtHopSpeed, applied by rules/damage.h).
// 0.6 px/step for kHurtSteps steps is about 8 px (it was 1.2 = 17 px).
inline constexpr float kKnockbackSpeed = 0.6F;
// Half the height of the first version (2.4 = 14 px): 1.7 gives about 7 px.
inline constexpr float kHurtHopSpeed = 1.7F;

// Variable height: releasing the button while rising caps the upward speed at
// this value, so a tap gives a short hop and holding gives the full jump.
// Scaled with the jump so a tap is still about the same fraction of a full jump.
inline constexpr float kJumpCutSpeed = 1.7F;

// Ramps (45 degrees, tile ids 5 and 6) and sliding. The player stands on a ramp
// by its centre: the feet follow the surface. Walking on a ramp, up or down, is a
// normal walk and standing still does nothing. Pressing DOWN while on a ramp makes
// the player slide down it, whichever way it is pushing: it moves at kSlideSpeed
// and, once it lets go of the key, the ramp ends or it jumps, keeps that speed for
// kSlideCarrySteps steps, fading to a stop. Pressing the opposite way, hitting a
// wall or being hurt ends the slide.
//
// Crouching: pressing DOWN on firm ground (not on a ramp) makes the box half as
// tall, with the feet where they were, and the player walks at kCrouchSpeed and
// cannot jump. It only stands up again when there is room above its head, so a
// low passage keeps it crouched even if the key is released.
inline constexpr float kSlideSpeed = 2.4F;
inline constexpr int kSlideCarrySteps = 12;
// How far above a ramp the feet can be and still be pulled down onto it when
// walking down (less than one tile).
inline constexpr float kSlopeSnap = 8.0F;

inline constexpr float kCrouchSpeed = 0.75F;  // half of the run

// What stepPlayer reports about the step it just did, for the game to announce.
struct StepResult {
    bool jumped = false;  // the player started a jump on this step
};

// Advances the player by exactly one fixed simulation step, colliding with
// the solid tiles of `map`.
//
// Movement is resolved one axis at a time (first X, then Y). Doing both at
// once makes it ambiguous which face of a tile was hit; separating the axes
// makes "landing", "bumping the head" and "hitting a wall" trivial to tell
// apart.
//
// While `hurt_steps` is running the player ignores the input: it is carried
// backwards by the hit. `invulnerable_steps` counts down every step.
//
// Jump rules: a jump needs a fresh press (holding the button does not make
// the player bounce), is allowed up to kCoyoteSteps after leaving the ground,
// and is cut short if the button is released while rising.
//
// Speeds must stay below one tile per step, otherwise the player could skip
// over a thin tile ("tunnelling").
StepResult stepPlayer(Player& player, const InputState& input, const TileMap& map);

}  // namespace domain
