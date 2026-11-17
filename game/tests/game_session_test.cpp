#include <doctest/doctest.h>

#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

#include "application/game_session.h"
#include "infrastructure/assets/tiled_map_loader.h"
#include "test_maps.h"

using application::GameSession;
using domain::InputState;
using domain::Room;
using domain::RoomGraph;
using domain::Side;

namespace {

// Five rooms in a plus shape, all empty with a floor:
//
//          north
//   west   center   east
//          south
RoomGraph makePlusWorld() {
    using Links = std::map<Side, domain::RoomId>;
    std::vector<Room> rooms;
    rooms.emplace_back("center", test_maps::makeMap(),
                       Links{{Side::North, "north"}, {Side::South, "south"},
                             {Side::East, "east"}, {Side::West, "west"}});
    rooms.emplace_back("north", test_maps::makeMap(), Links{{Side::South, "center"}});
    rooms.emplace_back("south", test_maps::makeMap(), Links{{Side::North, "center"}});
    rooms.emplace_back("east", test_maps::makeMap(), Links{{Side::West, "center"}});
    rooms.emplace_back("west", test_maps::makeMap(), Links{{Side::East, "center"}});
    return RoomGraph(std::move(rooms));
}

void run(GameSession& session, const InputState& input, int steps) {
    for (int i = 0; i < steps; ++i) {
        session.update(input);
    }
}

// Runs until the session leaves `room_id` (or `max_steps` pass).
void runUntilLeaving(GameSession& session, const InputState& input, const std::string& room_id,
                     int max_steps = 600) {
    for (int i = 0; i < max_steps && session.currentRoom().id() == room_id; ++i) {
        session.update(input);
    }
}

}  // namespace

TEST_CASE("GameSession basics") {
    GameSession session(makePlusWorld(), "center", 40.0F, 60.0F);

    SUBCASE("starts in the requested room, in the air") {
        CHECK(session.currentRoom().id() == "center");
        CHECK(session.player().x == 40.0F);
        CHECK(session.player().y == 60.0F);
        CHECK_FALSE(session.player().on_ground);
    }

    SUBCASE("the player settles on the floor") {
        run(session, InputState{}, 200);
        CHECK(session.player().on_ground);
        CHECK(session.currentRoom().id() == "center");
    }

    SUBCASE("input drives the player") {
        InputState input;
        input.right = true;
        run(session, input, 20);
        CHECK(session.player().x > 40.0F);
    }

    SUBCASE("rejects an unknown start room") {
        CHECK_THROWS_AS(GameSession(makePlusWorld(), "nowhere", 0.0F, 0.0F), std::invalid_argument);
    }
}

TEST_CASE("GameSession changes room through any of the four edges") {
    SUBCASE("east") {
        GameSession session(makePlusWorld(), "center", 100.0F, 200.0F);
        InputState right;
        right.right = true;
        runUntilLeaving(session, right, "center");

        CHECK(session.currentRoom().id() == "east");
        CHECK(session.player().x < 20.0F);  // arrived by the west side
        CHECK(session.player().on_ground);
    }

    SUBCASE("west") {
        GameSession session(makePlusWorld(), "center", 100.0F, 200.0F);
        InputState left;
        left.left = true;
        runUntilLeaving(session, left, "center");

        CHECK(session.currentRoom().id() == "west");
        CHECK(session.player().x > 280.0F);  // arrived by the east side
    }

    SUBCASE("north") {
        GameSession session(makePlusWorld(), "center", 100.0F, -12.0F);  // already half out
        session.update(InputState{});

        CHECK(session.currentRoom().id() == "north");
        CHECK(session.player().y > 200.0F);  // arrived at the bottom
    }

    SUBCASE("south") {
        GameSession session(makePlusWorld(), "center", 100.0F, 236.0F);  // already half out
        session.update(InputState{});

        CHECK(session.currentRoom().id() == "south");
        CHECK(session.player().y < 20.0F);  // arrived at the top
    }
}

TEST_CASE("GameSession walls where there is no neighbour") {
    SUBCASE("cannot leave a room through an edge without a neighbour") {
        GameSession session(makePlusWorld(), "east", 100.0F, 200.0F);
        InputState right;
        right.right = true;
        run(session, right, 500);

        CHECK(session.currentRoom().id() == "east");
        CHECK(session.player().x == doctest::Approx(312.0F));
    }

    SUBCASE("can go back and forth between two rooms") {
        GameSession session(makePlusWorld(), "center", 100.0F, 200.0F);
        InputState right;
        right.right = true;
        runUntilLeaving(session, right, "center");
        REQUIRE(session.currentRoom().id() == "east");

        InputState left;
        left.left = true;
        runUntilLeaving(session, left, "east");
        CHECK(session.currentRoom().id() == "center");
    }
}

namespace {

// center (has a coin at the spawn point) <-> east (empty)
RoomGraph makeCoinWorld() {
    using Links = std::map<Side, domain::RoomId>;
    std::vector<Room> rooms;
    rooms.emplace_back("center", test_maps::makeMap(), Links{{Side::East, "east"}},
                       std::vector<domain::Coin>{domain::Coin{100.0F, 204.0F}});
    rooms.emplace_back("east", test_maps::makeMap(), Links{{Side::West, "center"}});
    return RoomGraph(std::move(rooms));
}

int countCoinEvents(const std::vector<application::GameEvent>& events) {
    int count = 0;
    for (const auto& event : events) {
        if (std::holds_alternative<application::CoinCollected>(event)) {
            ++count;
        }
    }
    return count;
}

}  // namespace

TEST_CASE("GameSession coins") {
    GameSession session(makeCoinWorld(), "center", 100.0F, 200.0F);  // overlapping the coin
    CHECK(session.totalCoins() == 1);
    CHECK(session.collectedCoins().count() == 0);

    SUBCASE("touching a coin collects it and reports an event") {
        const auto events = session.update(InputState{});

        REQUIRE(events.size() == 1);
        const auto* collected = std::get_if<application::CoinCollected>(&events[0]);
        REQUIRE(collected != nullptr);
        CHECK(collected->room == "center");
        CHECK(collected->coin_index == 0);
        CHECK(collected->total_collected == 1);
        CHECK(session.collectedCoins().count() == 1);
    }

    SUBCASE("the same coin is never collected twice") {
        session.update(InputState{});
        CHECK(countCoinEvents(session.update(InputState{})) == 0);
        CHECK(session.collectedCoins().count() == 1);
    }

    SUBCASE("a collected coin stays collected after leaving and coming back") {
        session.update(InputState{});
        REQUIRE(session.collectedCoins().count() == 1);

        InputState right;
        right.right = true;
        runUntilLeaving(session, right, "center");
        REQUIRE(session.currentRoom().id() == "east");

        int coin_events = 0;
        InputState left;
        left.left = true;
        for (int i = 0; i < 600; ++i) {
            coin_events += countCoinEvents(session.update(left));
        }

        CHECK(session.currentRoom().id() == "center");
        CHECK(coin_events == 0);
        CHECK(session.collectedCoins().count() == 1);
    }

    SUBCASE("changing room reports a RoomChanged event") {
        InputState right;
        right.right = true;

        bool saw_room_change = false;
        for (int i = 0; i < 600 && !saw_room_change; ++i) {
            for (const auto& event : session.update(right)) {
                if (const auto* changed = std::get_if<application::RoomChanged>(&event)) {
                    CHECK(changed->from == "center");
                    CHECK(changed->to == "east");
                    saw_room_change = true;
                }
            }
        }
        CHECK(saw_room_change);
    }
}

TEST_CASE("GameSession with the shipped rooms") {
    std::vector<Room> rooms;
    for (const char* name : {"room_01", "room_02", "room_03", "room_04"}) {
        rooms.push_back(infrastructure::loadTiledRoom(std::string(PLATFORMER_ASSETS_DIR) +
                                                      "/tiled/" + name + ".tmj"));
    }
    GameSession session(RoomGraph(std::move(rooms)), "room_01", 40.0F, 60.0F);

    SUBCASE("walking east from the start room reaches room_02 standing on the floor") {
        run(session, InputState{}, 120);  // land first
        REQUIRE(session.player().on_ground);

        InputState right;
        right.right = true;
        runUntilLeaving(session, right, "room_01");  // no jumping needed: the floor is continuous

        CHECK(session.currentRoom().id() == "room_02");
        CHECK(session.player().on_ground);
    }
}
