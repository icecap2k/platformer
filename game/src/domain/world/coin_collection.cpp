#include "domain/world/coin_collection.h"

namespace domain {

bool CoinCollection::isCollected(const RoomId& room, std::size_t coin_index) const {
    return collected_.count({room, coin_index}) != 0;
}

bool CoinCollection::collect(const RoomId& room, std::size_t coin_index) {
    return collected_.emplace(room, coin_index).second;
}

}  // namespace domain
