#include "domain/world/coin_collection.h"

namespace domain {

bool PickupCollection::isCollected(const RoomId& room, std::size_t index) const {
    return collected_.count({room, index}) != 0;
}

bool PickupCollection::collect(const RoomId& room, std::size_t index) {
    return collected_.emplace(room, index).second;
}

}  // namespace domain
