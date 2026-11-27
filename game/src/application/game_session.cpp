#include "application/game_session.h"

#include <stdexcept>
#include <utility>

#include "domain/enemies/enemy_contact.h"
#include "domain/physics/player_physics.h"
#include "domain/rules/coin_pickup.h"
#include "domain/rules/damage.h"
#include "domain/rules/healing.h"
#include "domain/rules/room_transition.h"

namespace application {

namespace {
// A fall must be at least this fast (pixels per step) to count as a landing worth announcing:
// it ignores the tiny drops of walking over steps and ramps.
constexpr float kLandingSoundSpeed = 1.5F;
}  // namespace

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
    eaten_fruits_ = domain::PickupCollection{};
    collected_orbs_ = domain::PickupCollection{};
    traps_ = domain::StalactiteTraps{};
    enemies_ = domain::EnemyPatrols{};
    last_checkpoint_.reset();
    gate_room_.clear();
    won_ = false;
}

bool GameSession::hasTakenOrb(domain::OrbColour colour) const {
    for (const domain::RoomId& id : world_.ids()) {
        const auto& orbs = world_.room(id).orbs();
        for (std::size_t index = 0; index < orbs.size(); ++index) {
            if (orbs[index].colour == colour && collected_orbs_.isCollected(id, index)) {
                return true;
            }
        }
    }
    return false;
}

void GameSession::respawnAtCheckpoint() {
    const domain::Room& room = world_.room(last_checkpoint_->first);
    const domain::Checkpoint& checkpoint = room.checkpoints()[last_checkpoint_->second];
    current_room_ = room.id();
    player_ = domain::Player{};
    player_.x = checkpoint.x + (checkpoint.size - player_.width) * 0.5F;
    player_.y = checkpoint.y + checkpoint.size - player_.height;  // feet on the checkpoint's base
    // What was waiting in that room is set up again; the rest of the world keeps its progress.
    traps_.resetRoom(room.id());
    enemies_.resetRoom(room.id());
}

bool GameSession::hurtPlayer(std::vector<GameEvent>& events, const domain::AABB& source) {
    domain::applyDamage(player_, 1, domain::knockbackDirection(player_, source));
    events.push_back(PlayerHurt{currentRoom().id(), player_.health});
    if (player_.health > 0) {
        return false;
    }
    if (last_checkpoint_) {
        respawnAtCheckpoint();
        events.push_back(PlayerRespawned{current_room_});
    } else {
        restart();
        events.push_back(GameRestarted{});
    }
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
        const domain::StalactitePhase before = state.phase;
        if (domain::stepStalactite(stalactite, state, room.map(), player_)) {
            events.push_back(StalactiteFell{room.id(), index});
        }
        if (before == domain::StalactitePhase::Idle && state.phase == domain::StalactitePhase::Shaking) {
            events.push_back(StalactiteWarned{room.id(), index});
        }
        if (before == domain::StalactitePhase::Falling && state.phase == domain::StalactitePhase::Gone) {
            events.push_back(StalactiteShattered{room.id(), index});  // on the floor or a ledge
        }
        const domain::AABB box = domain::currentBounds(stalactite, state);
        if (state.phase == domain::StalactitePhase::Falling && domain::canBeHurt(player_) &&
            player_.bounds().overlaps(box)) {
            state.phase = domain::StalactitePhase::Gone;  // it shatters on the player
            events.push_back(StalactiteShattered{room.id(), index});
            if (hurtPlayer(events, box)) {
                return;  // the game restarted: this room is no longer the current one
            }
        }
    }
}

bool GameSession::updateEnemies(std::vector<GameEvent>& events) {
    const domain::Room& room = currentRoom();
    for (std::size_t index = 0; index < room.enemies().size(); ++index) {
        const domain::Enemy& enemy = room.enemies()[index];
        domain::EnemyState& state = enemies_.state(room.id(), index);
        const bool flies = enemy.kind == domain::EnemyKind::Bat;
        if (flies) {
            domain::stepBat(enemy, state, room.map(), player_);
        } else {
            domain::stepEnemy(enemy, state, room.map(), room.hazards());
        }
        if (!state.alive) {
            continue;  // it only counts the time it has been squashed
        }
        const domain::AABB box = domain::currentBounds(enemy, state);
        if (!player_.bounds().overlaps(box) || player_.health <= 0) {
            continue;
        }
        if (domain::isStomp(player_, box)) {
            // A stomp works even during the invulnerability after a hit.
            state.alive = false;
            domain::bounceOffEnemy(player_);
            events.push_back(EnemyStomped{room.id(), index});
        } else if (domain::canBeHurt(player_)) {
            if (flies) {
                domain::batRetreats(state);
            }
            if (hurtPlayer(events, box)) {
                return true;  // restarted: this room is no longer the current one
            }
        }
    }
    return false;
}

std::vector<GameEvent> GameSession::update(const domain::InputState& input) {
    std::vector<GameEvent> events;
    if (won_) {
        return events;
    }
    const domain::Room& room = currentRoom();

    const bool was_on_ground = player_.on_ground;
    const float speed_before = player_.velocity_y;
    const int slide_before = player_.slide_steps;

    const domain::StepResult step = domain::stepPlayer(player_, input, room.map());
    domain::confineToMap(player_, room.map(), domain::openEdgesOf(room));

    if (step.jumped) {
        events.push_back(PlayerJumped{});
    }
    if (!was_on_ground && player_.on_ground && speed_before >= kLandingSoundSpeed) {
        events.push_back(PlayerLanded{speed_before});
    }
    if (slide_before == 0 && player_.slide_steps > 0) {
        events.push_back(SlideStarted{});
    }

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

    // Healing fruits: eaten on touch, even at full health (then they are wasted).
    for (const std::size_t index : domain::fruitsTouchedBy(player_, here, eaten_fruits_)) {
        eaten_fruits_.collect(here.id(), index);
        const int gained = domain::heal(player_, domain::kFruitHealing);
        events.push_back(FruitEaten{here.id(), index, player_.health, gained});
    }

    for (const std::size_t index : domain::orbsTouchedBy(player_, here, collected_orbs_)) {
        collected_orbs_.collect(here.id(), index);
        events.push_back(OrbCollected{here.id(), index, here.orbs()[index].colour, collected_orbs_.count(),
                                      world_.totalOrbs()});
    }

    // Checkpoints: touching one that is not the active one makes it the new respawn point.
    for (std::size_t index = 0; index < here.checkpoints().size(); ++index) {
        const std::pair<domain::RoomId, std::size_t> id{here.id(), index};
        if (last_checkpoint_ != id && player_.bounds().overlaps(here.checkpoints()[index].bounds())) {
            last_checkpoint_ = id;
            events.push_back(CheckpointReached{here.id(), index});
        }
    }

    // Walking into the room of the open exit raises its portcullis (again on every visit).
    if (!exitOpen() || here.exits().empty()) {
        gate_room_.clear();
    } else if (gate_room_ != here.id()) {
        gate_room_ = here.id();
        events.push_back(GateOpened{here.id()});
    }

    // The exit: once every orb is taken, touching it wins the game, and the session stops there.
    for (const domain::Exit& exit : here.exits()) {
        if (exitOpen() && player_.bounds().overlaps(exit.bounds())) {
            won_ = true;
            events.push_back(GameWon{collected_coins_.count(), world_.totalCoins()});
            return events;
        }
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
        restarted = updateEnemies(events);
    }
    if (!restarted) {
        updateStalactites(events);
    }

    return events;
}

}  // namespace application
