#include "domain/enemies/enemy_patrols.h"

#include <algorithm>
#include <cmath>

namespace domain {

namespace {
// Tile index of a pixel coordinate (floor, so negative values go to negative tiles).
int tileOf(float pixel, int tile_size) {
    return static_cast<int>(std::floor(pixel / static_cast<float>(tile_size)));
}

constexpr float kEdge = 0.001F;  // keeps a box that ends exactly on a tile border out of the next tile
}  // namespace

AABB currentBounds(const Enemy& enemy, const EnemyState& state) {
    AABB box = enemy.bounds;
    box.x += state.x_offset;
    return box;
}

void stepEnemy(const Enemy& enemy, EnemyState& state, const TileMap& map,
               const std::vector<Hazard>& hazards) {
    if (!state.alive) {
        state.dead_steps = std::min(state.dead_steps + 1, kSquashSteps);
        return;
    }
    const AABB now = currentBounds(enemy, state);
    const float next_left = now.left() + static_cast<float>(state.direction) * kEnemySpeed;
    const float next_right = next_left + now.width;
    const int size = map.tileSize();

    const int first_column = tileOf(next_left, size);
    const int last_column = tileOf(next_right - kEdge, size);
    const int first_row = tileOf(now.top(), size);
    const int last_row = tileOf(now.bottom() - kEdge, size);

    bool blocked = false;
    for (int row = first_row; row <= last_row && !blocked; ++row) {
        for (int column = first_column; column <= last_column; ++column) {
            if (map.isGround(column, row)) {
                blocked = true;
                break;
            }
        }
    }
    const AABB next_box{next_left, now.y, now.width, now.height};
    for (const Hazard& hazard : hazards) {
        blocked = blocked || (next_box.overlaps(hazard.bounds) && !now.overlaps(hazard.bounds));
    }
    // The foot that leads needs ground under it, or it would walk off.
    const int leading_column = state.direction > 0 ? last_column : first_column;
    const bool ledge = !map.isGround(leading_column, tileOf(now.bottom(), size));

    if (blocked || ledge) {
        state.direction = -state.direction;
        return;
    }
    state.x_offset += static_cast<float>(state.direction) * kEnemySpeed;
}

const EnemyState& EnemyPatrols::state(const RoomId& room, std::size_t index) const {
    static const EnemyState kFresh;
    const auto found = states_.find({room, index});
    return found == states_.end() ? kFresh : found->second;
}

EnemyState& EnemyPatrols::state(const RoomId& room, std::size_t index) {
    return states_[{room, index}];
}

}  // namespace domain
