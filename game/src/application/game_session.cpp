#include "application/game_session.h"

#include <stdexcept>
#include <utility>

#include "domain/physics/player_physics.h"
#include "domain/rules/coin_pickup.h"
#include "domain/rules/damage.h"
#include "domain/rules/room_transition.h"

namespace application {

GameSession::GameSession(domain::RoomGraph world, const domain::RoomId& start_room, float spawn_x,
                         float spawn_y)
    : world_(std::move(world)),
      start_room_(start_room),
      spawn_x_(spawn_x),
      spawn_y_(spawn_y),
      current_room_(start_room) {
    if (!world_.contains(start_room)) {
        throw std::invalid_argument("Unknown start room: " + start_room);
    }
    player_.x = spawn_x;
    player_.y = spawn_y;
}

void GameSession::restart() {
    current_room_ = start_room_;
    player_ = domain::Player{};
    player_.x = spawn_x_;
    player_.y = spawn_y_;
    collected_coins_ = domain::CoinCollection{};
    traps_ = domain::StalactiteTraps{};
}

bool GameSession::hurtPlayer(std::vector<GameEvent>& events, const domain::AABB& source) {
    domain::applyDamage(player_, 1, domain::knockbackDirection(player_, source));
    events.push_back(PlayerHurt{currentRoom().id(), player_.health});
    if (player_.health > 0) {
        return false;
    }
    restart();
    events.push_back(GameRestarted{});
    return true;
}

void GameSession::updateStalactites(std::vector<GameEvent>& events) {
    const domain::Room& room = currentRoom();
    for (std::size_t index = 0; index < room.stalactites().size(); ++index) {
        const domain::FallingStalactite& stalactite = room.stalactites()[index];
        domain::StalactiteState& state = traps_.state(room.id(), index);
        if (state.phase == domain::StalactitePhase::Gone) {
            continue;
        }
        if (domain::stepStalactite(stalactite, state, room.map(), player_)) {
            events.push_back(StalactiteFell{room.id(), index});
        }
        const domain::AABB box = domain::currentBounds(stalactite, state);
        if (state.phase == domain::StalactitePhase::Falling && domain::canBeHurt(player_) &&
            player_.bounds().overlaps(box)) {
            state.phase = domain::StalactitePhase::Gone;  // it shatters on the player
            if (hurtPlayer(events, box)) {
                return;  // the game restarted: this room is no longer the current one
            }
        }
    }
}

std::vector<GameEvent> GameSession::update(const domain::InputState& input) {
    std::vector<GameEvent> events;
    const domain::Room& room = currentRoom();

    domain::stepPlayer(player_, input, room.map());
    domain::confineToMap(player_, room.map(), domain::openEdgesOf(room));

    // Closed edges were just turned into walls, so only an open edge (which
    // always has a neighbour) can be crossed here.
    if (const auto side = domain::exitedThrough(player_, room.map())) {
        const domain::RoomId previous = current_room_;
        current_room_ = *room.neighbor(*side);
        domain::enterRoom(player_, *side, currentRoom().map());
        events.push_back(RoomChanged{previous, current_room_});
    }

    // Look at the room the player is in NOW (it may have just changed).
    const domain::Room& here = currentRoom();
    for (const std::size_t index : domain::coinsTouchedBy(player_, here, collected_coins_)) {
        collected_coins_.collect(here.id(), index);
        events.push_back(CoinCollected{here.id(), index, collected_coins_.count()});
    }

    // Hazards: a hit costs a heart, throws the player back and gives a moment of
    // invulnerability. Losing the last heart starts the whole game over.
    bool restarted = false;
    if (domain::canBeHurt(player_)) {
        if (const auto source = domain::hazardTouching(player_, here)) {
            restarted = hurtPlayer(events, *source);
        }
    }
    if (!restarted) {
        updateStalactites(events);
    }

    return events;
}

}  // namespace application
