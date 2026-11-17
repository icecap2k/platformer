#pragma once

#include <map>
#include <vector>

#include "domain/world/room.h"

namespace domain {

// The whole world: every room, addressable by id, with consistent links.
class RoomGraph {
public:
    // Throws std::invalid_argument if there are no rooms, two rooms share an
    // id, or a room points to a neighbour that does not exist.
    explicit RoomGraph(std::vector<Room> rooms);

    bool contains(const RoomId& id) const { return rooms_.count(id) != 0; }

    // Throws std::out_of_range for an unknown id.
    const Room& room(const RoomId& id) const;

    std::size_t size() const { return rooms_.size(); }

    // Number of coins in the whole world (collected or not).
    int totalCoins() const;

private:
    std::map<RoomId, Room> rooms_;
};

}  // namespace domain
