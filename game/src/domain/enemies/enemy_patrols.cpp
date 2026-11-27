#include "domain/enemies/enemy_patrols.h"

#include <algorithm>
#include <cmath>
#include <iterator>
#include <optional>

namespace domain {

namespace {
// Tile index of a pixel coordinate (floor, so negative values go to negative tiles).
int tileOf(float pixel, int tile_size) {
    return static_cast<int>(std::floor(pixel / static_cast<float>(tile_size)));
}

constexpr float kEdge = 0.001F;  // keeps a box that ends exactly on a tile border out of the next tile

// The y of the ground under the pixel column `x` that is closest to `feet`, looking from `above`
// pixels over them to `below` pixels under them. The surface of a ramp, or the top of a block
// with nothing on it.
std::optional<float> groundNear(const TileMap& map, float x, float feet, float above, float below) {
    const int size = map.tileSize();
    const int column = tileOf(x, size);
    std::optional<float> best;
    for (int row = tileOf(feet - above, size) - 1; row <= tileOf(feet + below, size) + 1; ++row) {
        std::optional<float> surface = map.slopeSurfaceY(column, row, x);
        if (!surface && map.isSolid(column, row) && !map.isGround(column, row - 1)) {
            surface = static_cast<float>(row * size);
        }
        if (surface && *surface >= feet - above - kEdge && *surface <= feet + below + kEdge &&
            (!best || std::abs(*surface - feet) < std::abs(*best - feet))) {
            best = surface;
        }
    }
    return best;
}

// Is there a ramp in or right around the box? Walkers treat walls differently there.
bool rampNear(const TileMap& map, const AABB& box) {
    const int size = map.tileSize();
    for (int row = tileOf(box.bottom(), size) - 1; row <= tileOf(box.bottom(), size); ++row) {
        for (int column = tileOf(box.left(), size); column <= tileOf(box.right() - kEdge, size); ++column) {
            if (map.isSlope(column, row)) {
                return true;
            }
        }
    }
    return false;
}

// Does the box touch anything that is not empty?
bool insideGround(const TileMap& map, const AABB& box) {
    const int size = map.tileSize();
    for (int row = tileOf(box.top(), size); row <= tileOf(box.bottom() - kEdge, size); ++row) {
        for (int column = tileOf(box.left(), size); column <= tileOf(box.right() - kEdge, size); ++column) {
            if (map.isGround(column, row)) {
                return true;
            }
        }
    }
    return false;
}

// One step of free fall; stops on the first ground the feet would cross.
void fall(const Enemy& enemy, EnemyState& state, const TileMap& map) {
    const AABB now = currentBounds(enemy, state);
    state.velocity_y = std::min(state.velocity_y + kEnemyGravity, kEnemyMaxFallSpeed);
    const float centre = now.x + now.width * 0.5F;
    const float next_bottom = std::min(now.bottom() + state.velocity_y, map.pixelHeight());
    const auto ground = groundNear(map, centre, now.bottom(), 0.0F, next_bottom - now.bottom());
    if (ground) {
        state.y_offset += *ground - now.bottom();
        state.velocity_y = 0.0F;
    } else {
        state.y_offset += next_bottom - now.bottom();
    }
}

}  // namespace

AABB currentBounds(const Enemy& enemy, const EnemyState& state) {
    AABB box = enemy.bounds;
    box.x += state.x_offset;
    box.y += state.y_offset;
    return box;
}

void stepEnemy(const Enemy& enemy, EnemyState& state, const TileMap& map,
               const std::vector<Hazard>& hazards) {
    if (!state.alive) {
        state.dead_steps = std::min(state.dead_steps + 1, kSquashSteps);
        return;
    }
    AABB now = currentBounds(enemy, state);
    const float half = now.width * 0.5F;
    const auto under = groundNear(map, now.x + half, now.bottom(), kWalkerStepUp, kWalkerStepDown);
    if (!under) {
        fall(enemy, state, map);
        return;
    }
    state.y_offset += *under - now.bottom();  // feet on the ground (on a ramp it moves up or down)
    state.velocity_y = 0.0F;
    now = currentBounds(enemy, state);

    const float next_left = now.left() + static_cast<float>(state.direction) * kEnemySpeed;
    const float next_right = next_left + now.width;
    const int size = map.tileSize();
    const AABB next_box{next_left, now.y, now.width, now.height};
    const bool on_ramp = rampNear(map, next_box);

    // Walls. Near a ramp the front half sinks into the slope, and the blocks that fill it would
    // stop the walker: the lowest pixels of the body do not count there, as for the player.
    const float feet_inset = on_ramp ? half + 0.5F : 0.0F;
    const int first_column = tileOf(next_left, size);
    const int last_column = tileOf(next_right - kEdge, size);
    bool blocked = next_left < 0.0F || next_right > map.pixelWidth();
    for (int row = tileOf(now.top(), size); row <= tileOf(now.bottom() - feet_inset - kEdge, size); ++row) {
        for (int column = first_column; column <= last_column; ++column) {
            blocked = blocked || map.isSolid(column, row);
        }
    }
    // The ground must go on: a small step up or down under the middle of the body...
    const bool continues =
        groundNear(map, next_left + half, now.bottom(), kWalkerStepDown, kWalkerStepDown).has_value();
    // ...and, on the flat, under the leading foot too, so it never hangs over a drop.
    const int leading_column = state.direction > 0 ? last_column : first_column;
    const bool ledge = !continues || (!on_ramp && !map.isGround(leading_column, tileOf(now.bottom(), size)));

    for (const Hazard& hazard : hazards) {
        blocked = blocked || (next_box.overlaps(hazard.bounds) && !now.overlaps(hazard.bounds));
    }

    if (blocked || ledge) {
        state.direction = -state.direction;
        return;
    }
    state.x_offset += static_cast<float>(state.direction) * kEnemySpeed;
}

void batRetreats(EnemyState& state) {
    state.chasing = false;
    state.retreat_steps = kBatRetreatSteps;
}

void stepBat(const Enemy& enemy, EnemyState& state, const TileMap& map, const Player& player) {
    if (!state.alive) {
        state.dead_steps = std::min(state.dead_steps + 1, kSquashSteps);
        fall(enemy, state, map);
        return;
    }
    const AABB now = currentBounds(enemy, state);
    const float centre_x = now.x + now.width * 0.5F;
    const float centre_y = now.y + now.height * 0.5F;
    const float away_x = centre_x - (player.x + player.width * 0.5F);
    const float away_y = centre_y - (player.y + player.height * 0.5F);
    const float distance = std::hypot(away_x, away_y);

    if (state.retreat_steps > 0) {
        --state.retreat_steps;
    } else if (!state.chasing && player.health > 0 && distance <= enemy.detect_radius) {
        state.chasing = true;
        state.chase_floor_y = player.y + player.height;
    } else if (state.chasing && distance > enemy.detect_radius * kBatGiveUpFactor) {
        state.chasing = false;
    }

    // Where it wants to be (top left corner) and how fast it may get there.
    float goal_x = 0.0F;
    float goal_y = 0.0F;
    float speed = kBatReturnSpeed;
    if (state.retreat_steps > 0) {
        goal_x = now.x + (away_x >= 0.0F ? 16.0F : -16.0F);
        goal_y = now.y - 16.0F;
    } else if (state.chasing) {
        if (player.on_ground) {
            state.chase_floor_y = player.y + player.height;
        }
        goal_x = player.x + player.width * 0.5F - now.width * 0.5F;
        goal_y = state.chase_floor_y - now.height;
        speed = kBatChaseSpeed;
    } else {
        const float angle = static_cast<float>(state.patrol_steps) * kBatPatrolRate;
        ++state.patrol_steps;
        goal_x = enemy.bounds.x + std::sin(angle) * kBatPatrolReach;
        goal_y = enemy.bounds.y + std::sin(angle * 2.0F) * kBatPatrolBob;
    }

    float move_x = goal_x - now.x;
    float move_y = goal_y - now.y;
    const float length = std::hypot(move_x, move_y);
    if (length > speed) {
        move_x *= speed / length;
        move_y *= speed / length;
    }
    // One axis at a time, so that it slides along a wall instead of stopping at it.
    if (!insideGround(map, AABB{now.x + move_x, now.y, now.width, now.height})) {
        state.x_offset += move_x;
        if (std::abs(move_x) > 0.05F) {
            state.direction = move_x > 0.0F ? 1 : -1;
        }
    }
    const AABB moved = currentBounds(enemy, state);
    if (!insideGround(map, AABB{moved.x, moved.y + move_y, moved.width, moved.height})) {
        state.y_offset += move_y;
    }
}

const EnemyState& EnemyPatrols::state(const RoomId& room, std::size_t index) const {
    static const EnemyState kFresh;
    const auto found = states_.find({room, index});
    return found == states_.end() ? kFresh : found->second;
}

EnemyState& EnemyPatrols::state(const RoomId& room, std::size_t index) {
    return states_[{room, index}];
}

void EnemyPatrols::resetRoom(const RoomId& room) {
    for (auto it = states_.begin(); it != states_.end();) {
        it = it->first.first == room ? states_.erase(it) : std::next(it);
    }
}

}  // namespace domain
