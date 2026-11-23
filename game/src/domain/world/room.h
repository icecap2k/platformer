#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "domain/enemies/enemy.h"
#include "domain/world/checkpoint.h"
#include "domain/world/coin.h"
#include "domain/world/falling_stalactite.h"
#include "domain/world/fruit.h"
#include "domain/world/hazard.h"
#include "domain/world/side.h"
#include "domain/world/tile_map.h"

namespace domain {

using RoomId = std::string;

// One screen of the game: its tiles, which room lies beyond each edge, the
// coins placed in it, the hazards that hurt, the stalactite traps and the
// healing fruits and the patrolling enemies. An edge without a neighbour is a wall.
class Room {
public:
    Room(RoomId id, TileMap map, std::map<Side, RoomId> neighbors = {},
         std::vector<Coin> coins = {}, std::vector<Hazard> hazards = {},
         std::vector<FallingStalactite> stalactites = {}, std::vector<Fruit> fruits = {},
         std::vector<Enemy> enemies = {}, std::vector<Checkpoint> checkpoints = {});

    const RoomId& id() const { return id_; }
    const TileMap& map() const { return map_; }
    const std::map<Side, RoomId>& neighbors() const { return neighbors_; }
    const std::vector<Coin>& coins() const { return coins_; }
    const std::vector<Hazard>& hazards() const { return hazards_; }
    const std::vector<FallingStalactite>& stalactites() const { return stalactites_; }
    const std::vector<Fruit>& fruits() const { return fruits_; }
    const std::vector<Enemy>& enemies() const { return enemies_; }
    const std::vector<Checkpoint>& checkpoints() const { return checkpoints_; }

    std::optional<RoomId> neighbor(Side side) const;

private:
    RoomId id_;
    TileMap map_;
    std::map<Side, RoomId> neighbors_;
    std::vector<Coin> coins_;
    std::vector<Hazard> hazards_;
    std::vector<FallingStalactite> stalactites_;
    std::vector<Fruit> fruits_;
    std::vector<Enemy> enemies_;
    std::vector<Checkpoint> checkpoints_;
};

}  // namespace domain
