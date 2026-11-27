#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include "domain/enemies/enemy.h"
#include "domain/entities/player.h"
#include "domain/world/hazard.h"
#include "domain/world/room.h"
#include "domain/world/tile_map.h"

namespace domain {

inline constexpr float kEnemySpeed = 0.35F;  // pixels per step: slower than the player's run (1.5)
inline constexpr int kSquashSteps = 60;      // a defeated enemy stays squashed on the floor for 1 s
inline constexpr int kSquashBlinkSteps = 20;  // ... and blinks during the last 20 of them

// Walkers fall like the player does, and follow the ground: a surface this far above or below
// their feet is the ground they stand on.
inline constexpr float kEnemyGravity = 0.2F;
inline constexpr float kEnemyMaxFallSpeed = 6.0F;
inline constexpr float kWalkerStepUp = 4.0F;    // it also rescues one placed slightly sunk
inline constexpr float kWalkerStepDown = 2.0F;

// Bats. Undisturbed, one flies a flat figure of eight around its spot. When the player comes
// within its radius it goes for them, LOW: it aims at the height of their feet while they are on
// the ground and does not follow them up when they jump, so it can be stomped.
inline constexpr float kBatPatrolReach = 28.0F;   // px to each side
inline constexpr float kBatPatrolBob = 6.0F;      // px up and down
inline constexpr float kBatPatrolRate = 0.02F;    // radians per step: a loop every 5 s
inline constexpr float kBatReturnSpeed = 0.8F;
inline constexpr float kBatChaseSpeed = 0.7F;     // under half the player's run: it can be outrun
inline constexpr float kBatGiveUpFactor = 1.5F;   // it loses interest this many radii away
inline constexpr int kBatRetreatSteps = 60;       // after biting, it flies off for a second

// What an enemy is doing: how far it is from where the level put it, which way it faces, and
// whether a stomp has not finished it. Session state, like StalactiteState.
struct EnemyState {
    float x_offset = 0.0F;
    float y_offset = 0.0F;
    float velocity_y = 0.0F;  // while falling
    int direction = -1;  // -1 left, +1 right
    bool alive = true;
    int dead_steps = 0;  // steps since it was defeated (it is shown squashed for kSquashSteps)
    // Bats only.
    bool chasing = false;
    float chase_floor_y = 0.0F;  // where the player's feet last stood during the chase
    int retreat_steps = 0;
    int patrol_steps = 0;
};

// Is a defeated enemy still lying there? (Only for drawing: it no longer hurts or moves.)
inline bool isSquashedBody(const EnemyState& state) {
    return !state.alive && state.dead_steps < kSquashSteps;  // dead_steps stops at kSquashSteps
}

// Where it is right now (its starting bounds moved by what it has travelled).
AABB currentBounds(const Enemy& enemy, const EnemyState& state);

// Moves a WALKER one step. Without ground under it, it falls. On the ground it walks, up and down
// ramps too, and turns around instead of moving when the next step would put it inside a wall,
// would leave the ground (a ledge, a drop or the edge of the room) or would walk onto spikes
// (`hazards`; one it already overlaps does not count, or it would never move). Dead enemies stay
// put and just count the steps since they died.
void stepEnemy(const Enemy& enemy, EnemyState& state, const TileMap& map,
               const std::vector<Hazard>& hazards);

// Moves a BAT one step, with the player where it is. It never flies into rock. A defeated bat
// drops to the ground.
void stepBat(const Enemy& enemy, EnemyState& state, const TileMap& map, const Player& player);

// The bat has just hurt the player: it gives up for a moment and flies away.
void batRetreats(EnemyState& state);

// The state of every enemy of the world, identified like coins by room id and position
// in that room's list.
class EnemyPatrols {
public:
    // An enemy nobody has touched is alive at its starting point.
    const EnemyState& state(const RoomId& room, std::size_t index) const;
    EnemyState& state(const RoomId& room, std::size_t index);

    // Puts every enemy of one room back where the level put it, alive.
    void resetRoom(const RoomId& room);

private:
    std::map<std::pair<RoomId, std::size_t>, EnemyState> states_;
};

}  // namespace domain
