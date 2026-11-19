#include "domain/world/room.h"

#include <utility>

namespace domain {

Room::Room(RoomId id, TileMap map, std::map<Side, RoomId> neighbors, std::vector<Coin> coins,
           std::vector<Hazard> hazards,
           std::vector<FallingStalactite> stalactites)
    : id_(std::move(id)),
      map_(std::move(map)),
      neighbors_(std::move(neighbors)),
      coins_(std::move(coins)),
      hazards_(std::move(hazards)),
      stalactites_(std::move(stalactites)) {}

std::optional<RoomId> Room::neighbor(Side side) const {
    const auto found = neighbors_.find(side);
    if (found == neighbors_.end()) {
        return std::nullopt;
    }
    return found->second;
}

}  // namespace domain
