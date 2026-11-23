#pragma once

#include <cstddef>
#include <map>
#include <utility>

#include "domain/entities/player.h"
#include "domain/physics/aabb.h"
#include "domain/world/falling_stalactite.h"
#include "domain/world/room.h"
#include "domain/world/tile_map.h"

namespace domain {

// What a stalactite trap is doing (its level data is FallingStalactite): waiting,
// shaking, falling, gone. This is session state, like CoinCollection.
enum class StalactitePhase {
    Idle,     // hanging, waiting for someone to walk under it
    Shaking,  // a warning: it is about to come loose
    Falling,  // dropping; it hurts only now
    Gone,     // shattered on the floor (or on the player); it does not come back
};

struct StalactiteState {
    StalactitePhase phase = StalactitePhase::Idle;
    int shake_steps = 0;     // steps of warning left
    float y_offset = 0.0F;   // how far it has fallen
    float velocity_y = 0.0F;
};

inline constexpr int kShakeSteps = 30;                 // half a second of warning
inline constexpr float kTriggerMargin = 12.0F;         // how far to the sides it notices the player
inline constexpr float kStalactiteGravity = 0.25F;
inline constexpr float kStalactiteMaxFallSpeed = 6.0F; // under one tile per step: no tunnelling

// Where it is right now (its resting bounds, moved down by what it has fallen).
AABB currentBounds(const FallingStalactite& stalactite, const StalactiteState& state);

// Is the player under the stalactite, within reach of its fall, with nothing
// solid in between?
bool playerUnder(const FallingStalactite& stalactite, const TileMap& map, const Player& player);

// Advances the stalactite one step. Returns true on the step it comes loose
// (starts falling), which is when the game reports it.
bool stepStalactite(const FallingStalactite& stalactite, StalactiteState& state, const TileMap& map,
                    const Player& player);

// The state of every stalactite of the world, identified like coins by room id
// and position in that room's list.
class StalactiteTraps {
public:
    // A stalactite nobody has touched is Idle.
    const StalactiteState& state(const RoomId& room, std::size_t index) const;
    StalactiteState& state(const RoomId& room, std::size_t index);

    // Hangs every stalactite trap of one room again.
    void resetRoom(const RoomId& room);

private:
    std::map<std::pair<RoomId, std::size_t>, StalactiteState> states_;
};

}  // namespace domain
