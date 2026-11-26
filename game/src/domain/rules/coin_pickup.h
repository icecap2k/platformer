#pragma once

#include <cstddef>
#include <vector>

#include "domain/entities/player.h"
#include "domain/world/coin_collection.h"
#include "domain/world/room.h"

namespace domain {

// Indices (into `items`) of the pickups the player is touching right now and has
// not taken yet. Works for any item with `bounds()`: coins, fruits...
template <class Item>
std::vector<std::size_t> pickupsTouchedBy(const Player& player, const RoomId& room,
                                          const std::vector<Item>& items,
                                          const PickupCollection& collection) {
    std::vector<std::size_t> touched;
    const AABB body = player.bounds();
    for (std::size_t index = 0; index < items.size(); ++index) {
        if (!collection.isCollected(room, index) && body.overlaps(items[index].bounds())) {
            touched.push_back(index);
        }
    }
    return touched;
}

// Indices (into room.coins()) of the coins the player is touching right now
// and has not collected yet.
std::vector<std::size_t> coinsTouchedBy(const Player& player, const Room& room,
                                        const CoinCollection& collection);

// Same for the fruits of the room (indices into room.fruits()).
std::vector<std::size_t> fruitsTouchedBy(const Player& player, const Room& room,
                                         const PickupCollection& collection);

// Same for the orbs of the room (indices into room.orbs()).
std::vector<std::size_t> orbsTouchedBy(const Player& player, const Room& room,
                                       const PickupCollection& collection);

}  // namespace domain
