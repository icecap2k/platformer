#include "domain/rules/damage.h"

#include <algorithm>

#include "domain/physics/player_physics.h"

namespace domain {

bool canBeHurt(const Player& player) {
    return player.health > 0 && player.invulnerable_steps == 0;
}

void applyDamage(Player& player, int amount, int direction) {
    player.health = std::max(0, player.health - amount);
    player.invulnerable_steps = kInvulnerableSteps;
    player.hurt_steps = kHurtSteps;
    player.hurt_direction = direction < 0 ? -1 : 1;
    player.velocity_y = -kHurtHopSpeed;  // a small hop, much lower than a jump
    player.on_ground = false;
    player.jump_buffer_steps = 0;  // a jump pressed just before the hit is forgotten
    player.coyote_steps = 0;  // no jumping off the ground it was just thrown from
    player.slide_steps = 0;   // a hit ends any slide
}

int knockbackDirection(const Player& player, const AABB& source) {
    const float player_centre = player.x + player.width * 0.5F;
    const float source_centre = source.x + source.width * 0.5F;
    if (player_centre < source_centre) {
        return -1;
    }
    if (player_centre > source_centre) {
        return 1;
    }
    return player.velocity_x > 0.0F ? -1 : 1;
}

std::optional<AABB> hazardTouching(const Player& player, const Room& room) {
    for (const Hazard& hazard : room.hazards()) {
        if (player.bounds().overlaps(hazard.bounds)) {
            return hazard.bounds;
        }
    }
    return std::nullopt;
}

}  // namespace domain
