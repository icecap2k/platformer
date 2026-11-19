#include "domain/physics/player_physics.h"

#include <algorithm>
#include <cmath>

namespace domain {

namespace {

int tileIndexAt(float pixel, int tile_size) {
    return static_cast<int>(std::floor(pixel / static_cast<float>(tile_size)));
}

// Index of the last tile that a span ending at `end_pixel` actually covers.
// A span that ends exactly on a tile border does NOT cover the next tile.
int lastTileIndexBefore(float end_pixel, int tile_size) {
    return static_cast<int>(std::ceil(end_pixel / static_cast<float>(tile_size))) - 1;
}

bool anySolidInColumn(const TileMap& map, int tile_x, int first_row, int last_row) {
    for (int row = first_row; row <= last_row; ++row) {
        if (map.isSolid(tile_x, row)) {
            return true;
        }
    }
    return false;
}

bool anySolidInRow(const TileMap& map, int tile_y, int first_column, int last_column) {
    for (int column = first_column; column <= last_column; ++column) {
        if (map.isSolid(column, tile_y)) {
            return true;
        }
    }
    return false;
}

// What the player's feet are touching when they are on a ramp.
struct SlopeContact {
    bool found = false;
    float surface_y = 0.0F;  // y of the ramp's surface under the centre of the feet
    int downhill = 0;        // -1 or +1: the direction that goes down the ramp
};

// The ramp under the centre of the player's feet, if any: the one whose surface
// is closest to the feet, with the feet at most `gap_above` pixels above it (they
// can float a little when walking down) or any depth up to a tile inside it.
SlopeContact findSlope(const Player& player, const TileMap& map, float gap_above) {
    const int tile_size = map.tileSize();
    const float foot_x = player.x + player.width * 0.5F;
    const float bottom = player.y + player.height;
    const int column = tileIndexAt(foot_x, tile_size);
    const int bottom_row = lastTileIndexBefore(bottom, tile_size);

    SlopeContact best;
    float best_distance = 0.0F;
    for (int row = bottom_row - 1; row <= bottom_row + 1; ++row) {
        const auto surface = map.slopeSurfaceY(column, row, foot_x);
        if (!surface.has_value()) {
            continue;
        }
        const float depth = bottom - *surface;  // > 0: the feet are inside the ramp
        if (depth < -gap_above || depth > static_cast<float>(tile_size)) {
            continue;
        }
        if (!best.found || std::abs(depth) < best_distance) {
            best.found = true;
            best.surface_y = *surface;
            best.downhill = map.shape(column, row) == TileShape::SlopeRight ? -1 : 1;
            best_distance = std::abs(depth);
        }
    }
    return best;
}

// Puts the player on the ramp surface after it moved. Not while rising (a jump
// leaves the ramp). When it was already on the ground it also follows the surface
// down, so walking down a ramp does not turn into a series of little falls.
void resolveSlope(Player& player, const TileMap& map, bool was_on_ground) {
    if (player.velocity_y < 0.0F) {
        return;
    }
    const SlopeContact contact = findSlope(player, map, was_on_ground ? kSlopeSnap : 0.0F);
    if (contact.found) {
        player.y = contact.surface_y - player.height;
        player.velocity_y = 0.0F;
        player.on_ground = true;
    }
}

// Is there room above a crouching player's head to stand up? (The extra height
// above the box must be free of blocks.)
bool canStandUp(const Player& player, const TileMap& map) {
    const int tile_size = map.tileSize();
    const float extra = kStandingHeight - player.height;
    const int first_row = tileIndexAt(player.y - extra, tile_size);
    const int last_row = lastTileIndexBefore(player.y, tile_size);
    const int first_column = tileIndexAt(player.x, tile_size);
    const int last_column = lastTileIndexBefore(player.x + player.width, tile_size);
    for (int row = first_row; row <= last_row; ++row) {
        if (anySolidInRow(map, row, first_column, last_column)) {
            return false;
        }
    }
    return true;
}

// Crouching halves the box keeping the feet where they are; standing up needs room.
void updateCrouch(Player& player, const TileMap& map, bool wants_to_crouch) {
    if (wants_to_crouch && !player.crouching) {
        player.y += player.height - kCrouchHeight;
        player.height = kCrouchHeight;
        player.crouching = true;
    } else if (!wants_to_crouch && player.crouching && canStandUp(player, map)) {
        player.y -= kStandingHeight - player.height;
        player.height = kStandingHeight;
        player.crouching = false;
    }
}

// Leaving a ramp at its foot, the feet can be a few pixels above the flat floor
// (the ramp's surface was higher than the floor at the last step): put them on
// the floor instead of letting the player fall for a couple of steps.
void snapToFloorBelow(Player& player, const TileMap& map) {
    const int tile_size = map.tileSize();
    const int first_column = tileIndexAt(player.x, tile_size);
    const int last_column = lastTileIndexBefore(player.x + player.width, tile_size);
    const float bottom = player.y + player.height;
    for (int row = lastTileIndexBefore(bottom, tile_size) + 1; row <= tileIndexAt(bottom + kSlopeSnap, tile_size); ++row) {
        if (anySolidInRow(map, row, first_column, last_column)) {
            const float floor_y = static_cast<float>(row * tile_size);
            if (floor_y - bottom <= kSlopeSnap) {
                player.y = floor_y - player.height;
                player.velocity_y = 0.0F;
                player.on_ground = true;
            }
            return;
        }
    }
}

void moveHorizontally(Player& player, const TileMap& map, bool on_slope) {
    const int tile_size = map.tileSize();
    player.x += player.velocity_x;

    // On a ramp the front foot sinks into the ramp by up to half the player's
    // width (45 degrees), and the filler blocks beside the ramp would stop it:
    // so the lowest few pixels of the body do not count for walls there.
    const float feet_inset = on_slope ? player.width * 0.5F + 0.5F : 0.0F;
    const int first_row = tileIndexAt(player.y, tile_size);
    const int last_row = lastTileIndexBefore(player.y + player.height - feet_inset, tile_size);

    if (player.velocity_x > 0.0F) {
        const int column = lastTileIndexBefore(player.x + player.width, tile_size);
        if (anySolidInColumn(map, column, first_row, last_row)) {
            player.x = static_cast<float>(column * tile_size) - player.width;
        }
    } else if (player.velocity_x < 0.0F) {
        const int column = tileIndexAt(player.x, tile_size);
        if (anySolidInColumn(map, column, first_row, last_row)) {
            player.x = static_cast<float>((column + 1) * tile_size);
        }
    }
    // Map edges are NOT handled here: whether leaving the room is possible is
    // a rule of the room graph (see rules/room_transition.h), not of physics.
}

void moveVertically(Player& player, const TileMap& map) {
    const int tile_size = map.tileSize();
    player.y += player.velocity_y;
    player.on_ground = false;

    const int first_column = tileIndexAt(player.x, tile_size);
    const int last_column = lastTileIndexBefore(player.x + player.width, tile_size);

    if (player.velocity_y > 0.0F) {
        const int row = lastTileIndexBefore(player.y + player.height, tile_size);
        if (anySolidInRow(map, row, first_column, last_column)) {
            player.y = static_cast<float>(row * tile_size) - player.height;
            player.velocity_y = 0.0F;
            player.on_ground = true;
        }
    } else if (player.velocity_y < 0.0F) {
        const int row = tileIndexAt(player.y, tile_size);
        if (anySolidInRow(map, row, first_column, last_column)) {
            player.y = static_cast<float>((row + 1) * tile_size);
            player.velocity_y = 0.0F;  // bumped the head: stop rising
        }
    }
}

}  // namespace

StepResult stepPlayer(Player& player, const InputState& input, const TileMap& map) {
    StepResult result;
    if (player.invulnerable_steps > 0) {
        --player.invulnerable_steps;
    }
    const bool hurt = player.hurt_steps > 0;
    const bool was_on_ground = player.on_ground;
    const SlopeContact ramp = was_on_ground ? findSlope(player, map, 1.0F) : SlopeContact{};

    // Crouch: down on firm ground (not on a ramp, where down slides). Standing
    // up waits for room above; being hurt or leaving the ground also stands up.
    updateCrouch(player, map, input.down && !hurt && was_on_ground && !ramp.found);

    // Horizontal movement: direct control, no acceleration. While hurt, the
    // hit decides instead of the player. Down on a ramp slides.
    const int axis = input.horizontalAxis();
    if (hurt) {
        player.velocity_x = static_cast<float>(player.hurt_direction) * kKnockbackSpeed;
        --player.hurt_steps;
        player.slide_steps = 0;
    } else if (ramp.found && input.down) {
        player.slide_direction = ramp.downhill;
        player.slide_steps = kSlideCarrySteps;  // refilled while the key is held on the ramp
        player.velocity_x = static_cast<float>(ramp.downhill) * kSlideSpeed;
    } else if (player.slide_steps > 0 && axis != -player.slide_direction) {
        // Momentum after the ramp: fades to a stop; steering the same way keeps at least a run.
        const float coasting = kSlideSpeed * static_cast<float>(player.slide_steps) / static_cast<float>(kSlideCarrySteps);
        const float speed = axis == player.slide_direction ? std::max(coasting, kRunSpeed) : coasting;
        player.velocity_x = static_cast<float>(player.slide_direction) * speed;
        --player.slide_steps;
    } else {
        player.slide_steps = 0;  // steering against it (or no slide): back to normal control
        player.velocity_x = static_cast<float>(axis) * (player.crouching ? kCrouchSpeed : kRunSpeed);
    }
    const float x_before = player.x;
    moveHorizontally(player, map, ramp.found);
    if (player.slide_steps > 0 && std::abs(player.x - x_before) < std::abs(player.velocity_x) - 0.01F) {
        player.slide_steps = 0;  // it hit a wall
    }

    // Coyote time: standing refills the grace period; each airborne step that
    // does not jump spends one (after the jump check, so the window is exactly
    // kCoyoteSteps airborne steps long).
    if (player.on_ground) {
        player.coyote_steps = kCoyoteSteps;
    }

    // Jump: needs a fresh press (not held since last step) and ground, or the
    // grace period after leaving it. Not while hurt or crouching.
    if (!hurt && !player.crouching && input.jump && !player.jump_held && player.coyote_steps > 0) {
        player.velocity_y = -kJumpSpeed;
        player.coyote_steps = 0;  // spent: no double jump
        result.jumped = true;
    } else if (!player.on_ground && player.coyote_steps > 0) {
        --player.coyote_steps;
    }

    // Variable height: letting go while rising cuts the jump short (but never
    // the small hop of a hit).
    if (!hurt && !input.jump && player.velocity_y < -kJumpCutSpeed) {
        player.velocity_y = -kJumpCutSpeed;
    }
    player.jump_held = input.jump;

    // Gravity, with a terminal velocity so falls stay controllable.
    player.velocity_y = std::min(player.velocity_y + kGravity, kMaxFallSpeed);
    moveVertically(player, map);
    resolveSlope(player, map, was_on_ground);
    if (ramp.found && !player.on_ground && player.velocity_y >= 0.0F && player.velocity_y <= kGravity) {
        snapToFloorBelow(player, map);  // it just stepped off the foot of a ramp
    }
    return result;
}

}  // namespace domain
