#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

#include "domain/world/coin.h"
#include "domain/world/side.h"
#include "domain/world/tile_map.h"

namespace domain {

using RoomId = std::string;

// One screen of the game: its tiles, which room lies beyond each edge and the
// coins placed in it. An edge without a neighbour is a wall.
class Room {
public:
    Room(RoomId id, TileMap map, std::map<Side, RoomId> neighbors = {},
         std::vector<Coin> coins = {});

    const RoomId& id() const { return id_; }
    const TileMap& map() const { return map_; }
    const std::map<Side, RoomId>& neighbors() const { return neighbors_; }
    const std::vector<Coin>& coins() const { return coins_; }

    std::optional<RoomId> neighbor(Side side) const;

private:
    RoomId id_;
    TileMap map_;
    std::map<Side, RoomId> neighbors_;
    std::vector<Coin> coins_;
};

}  // namespace domain
