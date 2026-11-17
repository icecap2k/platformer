#include <doctest/doctest.h>

#include <stdexcept>

#include "domain/rules/room_transition.h"
#include "domain/world/room_graph.h"
#include "test_maps.h"

using namespace domain;
using test_maps::makeMap;

namespace {

Player playerAt(float x, float y) {
    Player player;  // 8 x 16
    player.x = x;
    player.y = y;
    return player;
}

}  // namespace

TEST_CASE("Side::opposite") {
    CHECK(opposite(Side::North) == Side::South);
    CHECK(opposite(Side::South) == Side::North);
    CHECK(opposite(Side::East) == Side::West);
    CHECK(opposite(Side::West) == Side::East);
}

TEST_CASE("Room neighbours") {
    const Room room("a", makeMap(), {{Side::East, "b"}});

    CHECK(room.neighbor(Side::East) == "b");
    CHECK_FALSE(room.neighbor(Side::West).has_value());

    const OpenEdges open = openEdgesOf(room);
    CHECK(open.east);
    CHECK_FALSE(open.west);
    CHECK_FALSE(open.north);
    CHECK_FALSE(open.south);
}

TEST_CASE("RoomGraph validation") {
    SUBCASE("finds rooms by id") {
        std::vector<Room> rooms;
        rooms.emplace_back("a", makeMap(), std::map<Side, RoomId>{{Side::East, "b"}});
        rooms.emplace_back("b", makeMap(), std::map<Side, RoomId>{{Side::West, "a"}});
        const RoomGraph graph(std::move(rooms));

        CHECK(graph.size() == 2);
        CHECK(graph.contains("a"));
        CHECK(graph.room("b").neighbor(Side::West) == "a");
        CHECK_THROWS_AS(graph.room("missing"), std::out_of_range);
    }

    SUBCASE("rejects an empty graph") {
        CHECK_THROWS_AS(RoomGraph(std::vector<Room>{}), std::invalid_argument);
    }

    SUBCASE("rejects duplicate ids") {
        std::vector<Room> rooms;
        rooms.emplace_back("a", makeMap());
        rooms.emplace_back("a", makeMap());
        CHECK_THROWS_AS(RoomGraph(std::move(rooms)), std::invalid_argument);
    }

    SUBCASE("rejects a link to a room that does not exist") {
        std::vector<Room> rooms;
        rooms.emplace_back("a", makeMap(), std::map<Side, RoomId>{{Side::East, "ghost"}});
        CHECK_THROWS_AS(RoomGraph(std::move(rooms)), std::invalid_argument);
    }
}

TEST_CASE("confineToMap makes closed edges solid") {
    const TileMap map = makeMap();  // 320 x 240
    const OpenEdges closed;

    SUBCASE("west wall") {
        Player player = playerAt(-5.0F, 100.0F);
        confineToMap(player, map, closed);
        CHECK(player.x == 0.0F);
    }

    SUBCASE("east wall") {
        Player player = playerAt(318.0F, 100.0F);
        confineToMap(player, map, closed);
        CHECK(player.x == doctest::Approx(312.0F));
    }

    SUBCASE("ceiling stops a rising player") {
        Player player = playerAt(100.0F, -4.0F);
        player.velocity_y = -3.0F;
        confineToMap(player, map, closed);
        CHECK(player.y == 0.0F);
        CHECK(player.velocity_y == 0.0F);
    }

    SUBCASE("the bottom edge acts as a floor") {
        Player player = playerAt(100.0F, 235.0F);
        player.velocity_y = 4.0F;
        confineToMap(player, map, closed);
        CHECK(player.y == doctest::Approx(224.0F));
        CHECK(player.on_ground);
    }

    SUBCASE("open edges are left alone") {
        OpenEdges open;
        open.east = true;
        Player player = playerAt(318.0F, 100.0F);
        confineToMap(player, map, open);
        CHECK(player.x == 318.0F);
    }
}

TEST_CASE("exitedThrough uses the player's centre") {
    const TileMap map = makeMap();

    CHECK_FALSE(exitedThrough(playerAt(100.0F, 100.0F), map).has_value());

    // Player is 8 wide, 16 tall: centre = x + 4, y + 8.
    CHECK_FALSE(exitedThrough(playerAt(-3.0F, 100.0F), map).has_value());  // centre at 1
    CHECK(exitedThrough(playerAt(-5.0F, 100.0F), map) == Side::West);      // centre at -1
    CHECK_FALSE(exitedThrough(playerAt(315.0F, 100.0F), map).has_value()); // centre at 319
    CHECK(exitedThrough(playerAt(316.0F, 100.0F), map) == Side::East);     // centre at 320
    CHECK(exitedThrough(playerAt(100.0F, -9.0F), map) == Side::North);
    CHECK(exitedThrough(playerAt(100.0F, 232.0F), map) == Side::South);
}

TEST_CASE("enterRoom puts the player on the opposite edge") {
    const TileMap map = makeMap();

    SUBCASE("leaving east, appears on the west edge keeping height") {
        Player player = playerAt(317.0F, 150.0F);
        enterRoom(player, Side::East, map);
        CHECK(player.x == 0.0F);
        CHECK(player.y == 150.0F);
    }

    SUBCASE("leaving west, appears on the east edge") {
        Player player = playerAt(-6.0F, 150.0F);
        enterRoom(player, Side::West, map);
        CHECK(player.x == doctest::Approx(312.0F));
        CHECK(player.y == 150.0F);
    }

    SUBCASE("leaving north, appears at the bottom and keeps speed and x") {
        Player player = playerAt(200.0F, -10.0F);
        player.velocity_y = -2.0F;
        enterRoom(player, Side::North, map);
        CHECK(player.y == doctest::Approx(224.0F));
        CHECK(player.x == 200.0F);
        CHECK(player.velocity_y == -2.0F);
    }

    SUBCASE("leaving south, appears at the top") {
        Player player = playerAt(200.0F, 233.0F);
        enterRoom(player, Side::South, map);
        CHECK(player.y == 0.0F);
        CHECK(player.x == 200.0F);
    }

    SUBCASE("a freshly entered player is not immediately outside again") {
        for (Side side : {Side::North, Side::South, Side::East, Side::West}) {
            Player player = playerAt(100.0F, 100.0F);
            enterRoom(player, side, map);
            CHECK_FALSE(exitedThrough(player, map).has_value());
        }
    }
}
