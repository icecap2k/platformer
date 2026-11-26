#pragma once

#include <cstddef>
#include <map>
#include <vector>

#include "domain/world/room_graph.h"

namespace level_tools {

struct ReachabilityOptions {
    domain::RoomId start_room;
    float spawn_x = 40.0F;  // same defaults as the game's main.cpp
    float spawn_y = 60.0F;
    // Safety cap on the search; a report that hits it is marked truncated.
    std::size_t max_states = 20'000'000;
};

struct CoinStatus {
    domain::RoomId room;
    std::size_t index = 0;
    domain::Coin coin;
    bool touchable = false;
};

struct FruitStatus {
    domain::RoomId room;
    std::size_t index = 0;
    domain::Fruit fruit;
    bool touchable = false;
};

struct CheckpointStatus {
    domain::RoomId room;
    std::size_t index = 0;
    domain::Checkpoint checkpoint;
    bool touchable = false;
};

struct ExitStatus {
    domain::RoomId room;
    std::size_t index = 0;
    domain::Exit exit;
    bool touchable = false;
};

struct OrbStatus {
    domain::RoomId room;
    std::size_t index = 0;
    domain::Orb orb;
    bool touchable = false;
};

struct ReachabilityReport {
    std::map<domain::RoomId, bool> room_reachable;  // the player can enter it
    std::map<domain::RoomId, bool> room_standable;  // ... and stand on something in it
    std::vector<CoinStatus> coins;
    std::vector<FruitStatus> fruits;
    std::vector<CheckpointStatus> checkpoints;
    std::vector<ExitStatus> exits;
    std::vector<OrbStatus> orbs;
    std::size_t states_explored = 0;
    bool truncated = false;

    // True when every room can be reached and every coin, fruit, checkpoint, orb and exit touched.
    bool allOk() const;
};

// Exhaustively explores everything a player can do (left/right/none x
// jump/no jump, every simulation step) using the REAL game physics and room
// rules, starting at `start_room` / spawn. Because the physics is
// deterministic, the answer is a proof, not a guess.
//
// Throws std::invalid_argument if `start_room` is not in the graph.
ReachabilityReport analyzeReachability(const domain::RoomGraph& world,
                                       const ReachabilityOptions& options);

}  // namespace level_tools
