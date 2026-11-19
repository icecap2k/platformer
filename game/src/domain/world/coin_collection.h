#pragma once

#include <cstddef>
#include <set>
#include <utility>

#include "domain/world/room.h"

namespace domain {

// Which pickups of one kind (coins, fruits...) the player has already taken. A
// pickup is identified by the room it lives in and its position in that room's
// list for its kind, so the state survives leaving and re-entering rooms. Each
// kind has its own collection.
class PickupCollection {
public:
    bool isCollected(const RoomId& room, std::size_t index) const;

    // Returns true only the first time; collecting twice changes nothing.
    bool collect(const RoomId& room, std::size_t index);

    int count() const { return static_cast<int>(collected_.size()); }

private:
    std::set<std::pair<RoomId, std::size_t>> collected_;
};

// Coins were the first pickup; the old name keeps working.
using CoinCollection = PickupCollection;

}  // namespace domain
