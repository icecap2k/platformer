#pragma once

#include <cstddef>
#include <map>
#include <utility>

#include "domain/enemies/enemy.h"
#include "domain/entities/player.h"
#include "domain/world/room.h"
#include "domain/world/tile_map.h"

namespace domain {

inline constexpr float kEnemySpeed = 0.35F;  // pixels per step: slower than the player's run (1.5)

// What an enemy is doing: how far it has walked from where the level put it, which
// way it is going, and whether a stomp has not finished it. Session state, like
// StalactiteState.
struct EnemyState {
    float x_offset = 0.0F;
    int direction = -1;  // -1 left, +1 right
    bool alive = true;
};

// Where it is right now (its starting bounds moved sideways by what it has walked).
AABB currentBounds(const Enemy& enemy, const EnemyState& state);

// Walks one step. It turns around instead of moving when the next step would put it
// inside something that is not empty (a wall or a ramp) or when there would be no
// ground under its leading foot (a ledge or the edge of the room). Dead enemies stay put.
void stepEnemy(const Enemy& enemy, EnemyState& state, const TileMap& map);

// The state of every enemy of the world, identified like coins by room id and position
// in that room's list.
class EnemyPatrols {
public:
    // An enemy nobody has touched is alive at its starting point.
    const EnemyState& state(const RoomId& room, std::size_t index) const;
    EnemyState& state(const RoomId& room, std::size_t index);

private:
    std::map<std::pair<RoomId, std::size_t>, EnemyState> states_;
};

}  // namespace domain
