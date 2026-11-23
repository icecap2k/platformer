#pragma once

#include <cstddef>
#include <variant>

#include "domain/world/room.h"

namespace application {

// Things that happened during a simulation step. The session reports them
// instead of acting on them, so whoever listens decides what to do: print a
// log line today, play a sound or spawn particles tomorrow. The game logic
// never has to know.
struct RoomChanged {
    domain::RoomId from;
    domain::RoomId to;
};

struct CoinCollected {
    domain::RoomId room;
    std::size_t coin_index;
    int total_collected;  // including this one
};

// A healing fruit was eaten. `hearts_gained` is 0 when the player was already at
// full health: the fruit is eaten anyway and wasted.
struct FruitEaten {
    domain::RoomId room;
    std::size_t fruit_index;
    int health_after;
    int hearts_gained;
};

// The player started a jump.
struct PlayerJumped {};

// The player touched the ground after falling at `impact_speed` (pixels per step).
// Only reported for real landings, not for the little steps of walking up ramps.
struct PlayerLanded {
    float impact_speed;
};

// The player started sliding down a ramp.
struct SlideStarted {};

// A stalactite trap began to shake (the warning) and, later, shattered on the
// floor or on the player. (StalactiteFell is the moment in between: it comes loose.)
struct StalactiteWarned {
    domain::RoomId room;
    std::size_t index;
};

struct StalactiteShattered {
    domain::RoomId room;
    std::size_t index;
};

struct PlayerHurt {
    domain::RoomId room;
    int health_left;  // hearts left after this hit
};

// A stalactite trap came loose and starts to fall (after its warning shake).
struct StalactiteFell {
    domain::RoomId room;
    std::size_t index;
};

// The player landed on an enemy and defeated it.
struct EnemyStomped {
    domain::RoomId room;
    std::size_t index;
};

// The player touched a checkpoint that was not the active one: it is now where they come back.
struct CheckpointReached {
    domain::RoomId room;
    std::size_t index;
};

// The player lost its last heart and came back at the last checkpoint with full health.
struct PlayerRespawned {
    domain::RoomId room;
};

// The player lost its last heart with no checkpoint reached: everything starts over from the first room.
struct GameRestarted {};

using GameEvent = std::variant<RoomChanged, CoinCollected, FruitEaten, PlayerJumped, PlayerLanded,
                               SlideStarted, PlayerHurt, StalactiteWarned, StalactiteFell,
                               StalactiteShattered, EnemyStomped, CheckpointReached, PlayerRespawned,
                               GameRestarted>;

}  // namespace application
