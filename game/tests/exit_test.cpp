#include <doctest/doctest.h>

#include <variant>

#include "application/game_session.h"
#include "test_maps.h"

using application::GameSession;
using domain::InputState;
using domain::Room;
using domain::RoomGraph;

namespace {

// One room with a coin at x = 40 and the exit at x = 100, both on the floor.
RoomGraph makeWorld() {
    std::vector<Room> rooms;
    rooms.emplace_back("only", test_maps::makeMap(), std::map<domain::Side, domain::RoomId>{},
                       std::vector<domain::Coin>{domain::Coin{40.0F, 208.0F}, domain::Coin{300.0F, 208.0F}},
                       std::vector<domain::Hazard>{}, std::vector<domain::FallingStalactite>{},
                       std::vector<domain::Fruit>{}, std::vector<domain::Enemy>{},
                       std::vector<domain::Checkpoint>{}, std::vector<domain::Exit>{domain::Exit{100.0F, 208.0F}});
    return RoomGraph(std::move(rooms));
}

}  // namespace

TEST_CASE("Exit bounds") {
    const domain::Exit exit{16.0F, 24.0F, 8.0F};
    CHECK(exit.bounds().x == 16.0F);
    CHECK(exit.bounds().height == 8.0F);
}

TEST_CASE("touching the exit wins the game, once, with the coins collected so far") {
    GameSession session(makeWorld(), "only", 30.0F, test_maps::kFloorTop - 16.0F);
    CHECK_FALSE(session.won());

    InputState right;
    right.right = true;
    int won_events = 0;
    application::GameWon won{};
    for (int step = 0; step < 300; ++step) {
        for (const auto& event : session.update(right)) {
            if (const auto* result = std::get_if<application::GameWon>(&event)) {
                ++won_events;
                won = *result;
            }
        }
    }
    CHECK(session.won());
    CHECK(won_events == 1);
    CHECK(won.coins == 1);        // the coin on the way; the one at x = 300 was never reached
    CHECK(won.total_coins == 2);
}

TEST_CASE("once won, the session no longer advances") {
    GameSession session(makeWorld(), "only", 98.0F, test_maps::kFloorTop - 16.0F);
    InputState right;
    right.right = true;
    for (int step = 0; step < 20 && !session.won(); ++step) {
        session.update(right);
    }
    REQUIRE(session.won());
    const float x = session.player().x;
    const auto events = session.update(right);
    CHECK(events.empty());
    CHECK(session.player().x == x);
}

TEST_CASE("an exit that is not touched does not win") {
    GameSession session(makeWorld(), "only", 200.0F, test_maps::kFloorTop - 16.0F);
    for (int step = 0; step < 120; ++step) {
        session.update(InputState{});
    }
    CHECK_FALSE(session.won());
}
