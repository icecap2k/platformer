#include <doctest/doctest.h>

#include <variant>

#include "application/game_session.h"
#include "domain/rules/damage.h"
#include "test_maps.h"

using application::GameSession;
using domain::InputState;
using domain::Room;
using domain::RoomGraph;
using domain::Side;

namespace {

using domain::AABB;

// Two rooms, linked east-west. "a": a checkpoint at x = 100 (on the floor), a coin at x = 40,
// spikes from x = 150 to the east edge (wide enough that each knockback leaves the player on them) that drain the player, and a stalactite trap. "b": a
// checkpoint at x = 60 and a walker. Positions are on the floor (top at 216).
RoomGraph makeWorld() {
    using Links = std::map<Side, domain::RoomId>;
    std::vector<Room> rooms;
    rooms.emplace_back(
        "a", test_maps::makeMap(), Links{{Side::East, "b"}},
        std::vector<domain::Coin>{domain::Coin{40.0F, 208.0F}},
        std::vector<domain::Hazard>{domain::Hazard{AABB{150.0F, 211.0F, 170.0F, 5.0F}}},
        std::vector<domain::FallingStalactite>{}, std::vector<domain::Fruit>{}, std::vector<domain::Enemy>{},
        std::vector<domain::Checkpoint>{domain::Checkpoint{100.0F, 208.0F}});
    rooms.emplace_back(
        "b", test_maps::makeMap(), Links{{Side::West, "a"}}, std::vector<domain::Coin>{},
        std::vector<domain::Hazard>{domain::Hazard{AABB{150.0F, 211.0F, 170.0F, 5.0F}}},
        std::vector<domain::FallingStalactite>{}, std::vector<domain::Fruit>{},
        std::vector<domain::Enemy>{domain::Enemy{AABB{140.0F, test_maps::kFloorTop - 8.0F, 8.0F, 8.0F}}},
        std::vector<domain::Checkpoint>{domain::Checkpoint{60.0F, 208.0F}});
    return RoomGraph(std::move(rooms));
}

template <class Event>
int countOf(const std::vector<application::GameEvent>& events) {
    int count = 0;
    for (const auto& event : events) {
        count += std::holds_alternative<Event>(event) ? 1 : 0;
    }
    return count;
}

// Steps until `done` says so (or 3000 steps), collecting every event.
template <class Done>
std::vector<application::GameEvent> runUntil(GameSession& session, const InputState& input, Done done) {
    std::vector<application::GameEvent> all;
    for (int step = 0; step < 3000 && !done(all); ++step) {
        const auto events = session.update(input);
        all.insert(all.end(), events.begin(), events.end());
    }
    return all;
}

}  // namespace

TEST_CASE("Checkpoint bounds") {
    const domain::Checkpoint checkpoint{16.0F, 24.0F, 8.0F};
    CHECK(checkpoint.bounds().x == 16.0F);
    CHECK(checkpoint.bounds().height == 8.0F);
}

TEST_CASE("reaching a checkpoint is announced once and makes it the active one") {
    GameSession session(makeWorld(), "a", 100.0F, test_maps::kFloorTop - 16.0F);
    CHECK_FALSE(session.lastCheckpoint().has_value());

    const auto events = runUntil(session, InputState{}, [](const auto& all) { return countOf<application::CheckpointReached>(all) > 0; });
    CHECK(countOf<application::CheckpointReached>(events) == 1);
    REQUIRE(session.lastCheckpoint().has_value());
    CHECK(session.lastCheckpoint()->first == "a");

    // Standing on it for a while does not announce it again.
    const auto more = runUntil(session, InputState{}, [](const auto&) { return false; });
    CHECK(countOf<application::CheckpointReached>(more) == 0);
}

TEST_CASE("losing every heart sends the player back to the last checkpoint with full health") {
    // Reaches the checkpoint of room a, then walks right into the spikes until all hearts are gone.
    GameSession session(makeWorld(), "a", 100.0F, test_maps::kFloorTop - 16.0F);
    runUntil(session, InputState{}, [&](const auto&) { return session.lastCheckpoint().has_value(); });
    REQUIRE(session.lastCheckpoint().has_value());

    // Walk onto the spikes (x = 150) and stand there until every heart is gone.
    InputState right;
    right.right = true;
    while (session.player().x < 160.0F) {
        session.update(right);
    }
    const auto events = runUntil(session, InputState{}, [](const auto& all) {
        return countOf<application::PlayerRespawned>(all) > 0 || countOf<application::GameRestarted>(all) > 0;
    });
    CHECK(countOf<application::PlayerRespawned>(events) == 1);
    CHECK(countOf<application::GameRestarted>(events) == 0);
    CHECK(session.currentRoom().id() == "a");
    CHECK(session.player().health == domain::kMaxHealth);
    CHECK(session.player().invulnerable_steps == 0);
    // Back on the checkpoint (centred on it, feet on its base).
    CHECK(session.player().x == doctest::Approx(100.0F));
    CHECK(session.player().y + session.player().height == doctest::Approx(216.0F));
}

TEST_CASE("respawning keeps the coins already taken and sets the room up again") {
    GameSession session(makeWorld(), "a", 36.0F, test_maps::kFloorTop - 16.0F);
    InputState right;
    right.right = true;
    // Take the coin at x = 40, then go on: checkpoint at x = 100, spikes from x = 150.
    const auto events = runUntil(session, right, [](const auto& all) {
        return countOf<application::PlayerRespawned>(all) > 0;
    });
    REQUIRE(countOf<application::PlayerRespawned>(events) == 1);
    CHECK(session.collectedCoins().count() == 1);  // the coin stays taken
}

TEST_CASE("respawning in another room brings the player back there and resets that room's enemies") {
    // Reach room b's checkpoint (x = 60), let the walker be squashed there, then die on b's spikes.
    GameSession session(makeWorld(), "b", 60.0F, test_maps::kFloorTop - 16.0F);
    runUntil(session, InputState{}, [&](const auto&) { return session.lastCheckpoint().has_value(); });
    REQUIRE(session.lastCheckpoint().has_value());
    CHECK(session.lastCheckpoint()->first == "b");

    InputState right;
    right.right = true;
    // The walker is on the way (x = 140): stomping it is not the point; just die by walking into it and the spikes.
    const auto events = runUntil(session, right, [](const auto& all) {
        return countOf<application::PlayerRespawned>(all) > 0;
    });
    REQUIRE(countOf<application::PlayerRespawned>(events) == 1);
    CHECK(session.currentRoom().id() == "b");
    CHECK(session.enemyPatrols().state("b", 0).alive);
    CHECK(session.enemyPatrols().state("b", 0).x_offset == 0.0F);
}

TEST_CASE("without a checkpoint losing every heart still restarts the whole game") {
    // Stands on the spikes (east of the checkpoint at x = 100, which it never touches).
    GameSession session(makeWorld(), "a", 260.0F, test_maps::kFloorTop - 16.0F);
    const auto events = runUntil(session, InputState{}, [](const auto& all) {
        return countOf<application::GameRestarted>(all) > 0;
    });
    CHECK(countOf<application::GameRestarted>(events) == 1);
    CHECK(countOf<application::PlayerRespawned>(events) == 0);
    CHECK_FALSE(session.lastCheckpoint().has_value());
    CHECK(session.currentRoom().id() == "a");
}
