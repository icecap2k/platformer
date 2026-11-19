#include "domain/rules/healing.h"

#include <algorithm>

namespace domain {

int heal(Player& player, int amount) {
    const int before = player.health;
    player.health = std::min(kMaxHealth, player.health + amount);
    return player.health - before;
}

}  // namespace domain
