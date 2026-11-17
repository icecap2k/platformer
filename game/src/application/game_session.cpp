#include "application/game_session.h"

#include <stdexcept>
#include <utility>

#include "domain/physics/player_physics.h"
#include "domain/rules/coin_pickup.h"
#include "domain/rules/room_transition.h"

namespace application {

GameSession::GameSession(domain::RoomGraph world, const domain::RoomId& start_room, float spawn_x,
                         float spawn_y)
    : world_(std::move(world)), current_room_(start_room) {
    if (!world_.contains(start_room)) {
        throw std::invalid_argument("Unknown start room: " + start_room);
    }
    player_.x = spawn_x;
    player_.y = spawn_y;
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

    return events;
}

}  // namespace application
