#pragma once

#include <optional>
#include <utility>
#include <vector>

#include "application/game_event.h"
#include "domain/enemies/enemy_patrols.h"
#include "domain/entities/player.h"
#include "domain/input/input_state.h"
#include "domain/world/coin_collection.h"
#include "domain/world/room.h"
#include "domain/world/room_graph.h"
#include "domain/world/stalactite_traps.h"

namespace application {

// Owns the state of one play-through and advances it one fixed step at a time.
// It is the single entry point the outer layers use to talk to the game:
// infrastructure feeds it input, presentation reads its state to draw it.
// It has no SDL dependency, so whole sessions can be simulated in tests.
class GameSession {
public:
    // Throws std::invalid_argument if `start_room` is not in the graph.
    GameSession(domain::RoomGraph world, const domain::RoomId& start_room, float spawn_x,
                float spawn_y);

    // Advances one step and returns what happened during it (usually nothing).
    // Once the game is won it no longer advances (and returns nothing).
    std::vector<GameEvent> update(const domain::InputState& input);

    const domain::Player& player() const { return player_; }
    const domain::Room& currentRoom() const { return world_.room(current_room_); }
    const domain::TileMap& map() const { return currentRoom().map(); }

    const domain::CoinCollection& collectedCoins() const { return collected_coins_; }
    const domain::PickupCollection& eatenFruits() const { return eaten_fruits_; }
    int totalCoins() const { return world_.totalCoins(); }
    const domain::StalactiteTraps& stalactiteTraps() const { return traps_; }
    const domain::EnemyPatrols& enemyPatrols() const { return enemies_; }
    bool won() const { return won_; }

    // The checkpoint where the player comes back (room, index in that room's list), if any was reached.
    const std::optional<std::pair<domain::RoomId, std::size_t>>& lastCheckpoint() const {
        return last_checkpoint_;
    }

private:
    // Hurts the player (one heart, knockback). Returns true if that was the last
    // heart and the player was sent back (to the last checkpoint, or to the very start).
    bool hurtPlayer(std::vector<GameEvent>& events, const domain::AABB& source);
    // Wakes up, moves and lets fall the stalactite traps of the current room.
    void updateStalactites(std::vector<GameEvent>& events);
    // Walks the enemies of the current room and resolves contact: a stomp defeats the
    // enemy, any other touch hurts the player. Returns true if the game restarted.
    bool updateEnemies(std::vector<GameEvent>& events);

    // Back at the last checkpoint with full health; coins and fruits already taken stay taken.
    void respawnAtCheckpoint();

    // Back to the start: first room, full health, no coins collected.
    void restart();

    domain::RoomGraph world_;
    domain::RoomId start_room_;
    float spawn_x_;
    float spawn_y_;
    domain::RoomId current_room_;
    domain::Player player_;
    domain::CoinCollection collected_coins_;
    domain::PickupCollection eaten_fruits_;
    domain::StalactiteTraps traps_;
    domain::EnemyPatrols enemies_;
    std::optional<std::pair<domain::RoomId, std::size_t>> last_checkpoint_;
    bool won_ = false;
};

}  // namespace application
