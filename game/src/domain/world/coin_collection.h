#pragma once

#include <cstddef>
#include <set>
#include <utility>

#include "domain/world/room.h"

namespace domain {

// Which coins the player has already picked up. A coin is identified by the
// room it lives in and its position in that room's coin list, so the state
// survives leaving and re-entering rooms.
class CoinCollection {
public:
    bool isCollected(const RoomId& room, std::size_t coin_index) const;

    // Returns true only the first time; collecting twice changes nothing.
    bool collect(const RoomId& room, std::size_t coin_index);

    int count() const { return static_cast<int>(collected_.size()); }

private:
    std::set<std::pair<RoomId, std::size_t>> collected_;
};

}  // namespace domain
