#include "domain/world/stalactite_traps.h"

#include <algorithm>
#include <cmath>
#include <iterator>

namespace domain {

namespace {

const StalactiteState kIdle;

int tileIndex(float pixel, int tile_size) {
    return static_cast<int>(std::floor(pixel / static_cast<float>(tile_size)));
}

// Is there something to land on (a block or a ramp) in the columns the box covers, in the given tile row?
bool solidInRow(const TileMap& map, const AABB& box, int row) {
    const int first = tileIndex(box.left(), map.tileSize());
    const int last = tileIndex(box.right() - 0.01F, map.tileSize());
    for (int column = first; column <= last; ++column) {
        if (map.isGround(column, row)) {  // a block or a ramp stops it
            return true;
        }
    }
    return false;
}

}  // namespace

AABB currentBounds(const FallingStalactite& stalactite, const StalactiteState& state) {
    AABB box = stalactite.bounds;
    box.y += state.y_offset;
    return box;
}

bool playerUnder(const FallingStalactite& stalactite, const TileMap& map, const Player& player) {
    const AABB& box = stalactite.bounds;
    const bool in_reach = player.x + player.width > box.left() - kTriggerMargin &&
                          player.x < box.right() + kTriggerMargin;
    if (!in_reach || player.y < box.bottom()) {
        return false;  // beside it or above it
    }
    // Nothing solid between the stalactite and the player's head.
    const int first_row = tileIndex(box.bottom(), map.tileSize());
    const int last_row = tileIndex(player.y, map.tileSize());
    for (int row = first_row; row < last_row; ++row) {
        if (solidInRow(map, box, row)) {
            return false;
        }
    }
    return true;
}

bool stepStalactite(const FallingStalactite& stalactite, StalactiteState& state, const TileMap& map,
                    const Player& player) {
    switch (state.phase) {
        case StalactitePhase::Idle:
            if (playerUnder(stalactite, map, player)) {
                state.phase = StalactitePhase::Shaking;
                state.shake_steps = kShakeSteps;
            }
            return false;
        case StalactitePhase::Shaking:
            if (--state.shake_steps <= 0) {
                state.phase = StalactitePhase::Falling;
                return true;
            }
            return false;
        case StalactitePhase::Falling: {
            state.velocity_y = std::min(state.velocity_y + kStalactiteGravity, kStalactiteMaxFallSpeed);
            state.y_offset += state.velocity_y;
            const AABB box = currentBounds(stalactite, state);
            const int row = tileIndex(box.bottom(), map.tileSize());
            if (box.bottom() >= map.pixelHeight() || solidInRow(map, box, row)) {
                state.phase = StalactitePhase::Gone;  // shattered
            }
            return false;
        }
        case StalactitePhase::Gone:
            return false;
    }
    return false;
}

const StalactiteState& StalactiteTraps::state(const RoomId& room, std::size_t index) const {
    const auto found = states_.find({room, index});
    return found == states_.end() ? kIdle : found->second;
}

StalactiteState& StalactiteTraps::state(const RoomId& room, std::size_t index) {
    return states_[{room, index}];
}

void StalactiteTraps::resetRoom(const RoomId& room) {
    for (auto it = states_.begin(); it != states_.end();) {
        it = it->first.first == room ? states_.erase(it) : std::next(it);
    }
}

}  // namespace domain
