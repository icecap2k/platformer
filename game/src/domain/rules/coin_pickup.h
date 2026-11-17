#pragma once

#include <cstddef>
#include <vector>

#include "domain/entities/player.h"
#include "domain/world/coin_collection.h"
#include "domain/world/room.h"

namespace domain {

// Indices (into room.coins()) of the coins the player is touching right now
// and has not collected yet.
std::vector<std::size_t> coinsTouchedBy(const Player& player, const Room& room,
                                        const CoinCollection& collection);

}  // namespace domain
