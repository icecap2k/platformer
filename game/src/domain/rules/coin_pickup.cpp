#include "domain/rules/coin_pickup.h"

namespace domain {

std::vector<std::size_t> coinsTouchedBy(const Player& player, const Room& room,
                                        const CoinCollection& collection) {
    std::vector<std::size_t> touched;
    const AABB body = player.bounds();

    const auto& coins = room.coins();
    for (std::size_t index = 0; index < coins.size(); ++index) {
        if (!collection.isCollected(room.id(), index) && body.overlaps(coins[index].bounds())) {
            touched.push_back(index);
        }
    }
    return touched;
}

}  // namespace domain
