#include "domain/rules/coin_pickup.h"

namespace domain {

std::vector<std::size_t> coinsTouchedBy(const Player& player, const Room& room,
                                        const CoinCollection& collection) {
    return pickupsTouchedBy(player, room.id(), room.coins(), collection);
}

std::vector<std::size_t> fruitsTouchedBy(const Player& player, const Room& room,
                                         const PickupCollection& collection) {
    return pickupsTouchedBy(player, room.id(), room.fruits(), collection);
}

}  // namespace domain
