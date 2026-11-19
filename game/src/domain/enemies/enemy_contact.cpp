#include "domain/enemies/enemy_contact.h"

namespace domain {

bool isStomp(const Player& player, const AABB& enemy) {
    return player.velocity_y > 0.0F && player.bounds().bottom() <= enemy.top() + kStompDepth;
}

void bounceOffEnemy(Player& player) {
    player.velocity_y = -kStompBounceSpeed;
    player.on_ground = false;
    player.coyote_steps = 0;
    player.slide_steps = 0;
}

}  // namespace domain
