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

struct PlayerHurt {
    domain::RoomId room;
    int health_left;  // hearts left after this hit
};

// A stalactite trap came loose and starts to fall (after its warning shake).
struct StalactiteFell {
    domain::RoomId room;
    std::size_t index;
};

// The player lost its last heart: everything starts over from the first room.
struct GameRestarted {};

using GameEvent = std::variant<RoomChanged, CoinCollected, FruitEaten, PlayerHurt, StalactiteFell, GameRestarted>;

}  // namespace application
