#include "domain/world/room_graph.h"

#include <stdexcept>
#include <utility>

namespace domain {

RoomGraph::RoomGraph(std::vector<Room> rooms) {
    if (rooms.empty()) {
        throw std::invalid_argument("RoomGraph needs at least one room");
    }

    for (Room& room : rooms) {
        const RoomId id = room.id();
        if (!rooms_.emplace(id, std::move(room)).second) {
            throw std::invalid_argument("Duplicate room id: " + id);
        }
    }

    for (const auto& [id, room] : rooms_) {
        for (const auto& [side, neighbor_id] : room.neighbors()) {
            if (!contains(neighbor_id)) {
                throw std::invalid_argument("Room '" + id + "' links to unknown room '" +
                                            neighbor_id + "'");
            }
        }
    }
}

const Room& RoomGraph::room(const RoomId& id) const {
    return rooms_.at(id);
}

std::vector<RoomId> RoomGraph::ids() const {
    std::vector<RoomId> result;
    result.reserve(rooms_.size());
    for (const auto& [id, room] : rooms_) {
        result.push_back(id);
    }
    return result;
}

int RoomGraph::totalCoins() const {
    int total = 0;
    for (const auto& [id, room] : rooms_) {
        total += static_cast<int>(room.coins().size());
    }
    return total;
}

}  // namespace domain
