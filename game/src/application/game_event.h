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

using GameEvent = std::variant<RoomChanged, CoinCollected>;

}  // namespace application
