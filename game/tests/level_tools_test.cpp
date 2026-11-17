#include <doctest/doctest.h>

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "infrastructure/assets/tiled_map_loader.h"
#include "level_tools/reachability.h"
#include "level_tools/text_level.h"
#include "test_maps.h"

using namespace level_tools;
using domain::Coin;
using domain::Room;
using domain::RoomGraph;
using domain::Side;

namespace {

RoomGraph oneRoom(std::vector<Coin> coins, const std::vector<std::pair<int, int>>& solid = {}) {
    std::vector<Room> rooms;
    rooms.emplace_back("a", test_maps::makeMap(solid), std::map<Side, domain::RoomId>{},
                       std::move(coins));
    return RoomGraph(std::move(rooms));
}

// By default the player starts standing on the floor, so tests measure what
// jumping can do. (The game itself spawns the player in mid-air, y = 60, and
// lets them fall; a platform below the spawn point is then reached for free.)
ReachabilityOptions startAt(const std::string& room, float spawn_y = 200.0F) {
    ReachabilityOptions options;
    options.start_room = room;
    options.spawn_y = spawn_y;
    return options;
}

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    REQUIRE_MESSAGE(file.good(), "cannot open " << path);
    std::ostringstream contents;
    contents << file.rdbuf();
    return contents.str();
}

}  // namespace

TEST_CASE("analyzeReachability") {
    SUBCASE("a coin on the floor can be touched") {
        const auto report = analyzeReachability(oneRoom({Coin{100.0F, 208.0F}}), startAt("a"));

        CHECK(report.room_reachable.at("a"));
        CHECK(report.room_standable.at("a"));
        REQUIRE(report.coins.size() == 1);
        CHECK(report.coins[0].touchable);
        CHECK(report.allOk());
        CHECK_FALSE(report.truncated);
    }

    SUBCASE("a coin within jumping height can be touched") {
        // Floor top is y = 216. A jump rises about 62 px, so y = 170 is reachable.
        const auto report = analyzeReachability(oneRoom({Coin{100.0F, 170.0F}}), startAt("a"));
        CHECK(report.coins[0].touchable);
    }

    SUBCASE("a coin floating too high cannot be touched") {
        const auto report = analyzeReachability(oneRoom({Coin{100.0F, 100.0F}}), startAt("a"));

        REQUIRE(report.coins.size() == 1);
        CHECK_FALSE(report.coins[0].touchable);
        CHECK_FALSE(report.allOk());
    }

    SUBCASE("a high platform reached by a staircase makes its coin reachable") {
        // Platforms 40 px apart, each overlapping the next one horizontally.
        std::vector<std::pair<int, int>> stairs;
        for (int column = 10; column <= 15; ++column) stairs.emplace_back(column, 22);  // top 176
        for (int column = 14; column <= 19; ++column) stairs.emplace_back(column, 17);  // top 136
        for (int column = 18; column <= 23; ++column) stairs.emplace_back(column, 12);  // top 96
        const auto report =
            analyzeReachability(oneRoom({Coin{190.0F, 80.0F}}, stairs), startAt("a"));

        CHECK(report.coins[0].touchable);
    }

    SUBCASE("a platform that is too high to jump onto is not reachable") {
        std::vector<std::pair<int, int>> platform;
        for (int column = 10; column <= 15; ++column) platform.emplace_back(column, 17);  // 80 px up
        const auto report =
            analyzeReachability(oneRoom({Coin{90.0F, 120.0F}}, platform), startAt("a"));

        CHECK_FALSE(report.coins[0].touchable);
    }

    SUBCASE("rooms linked by an edge are reachable, unlinked ones are not") {
        using Links = std::map<Side, domain::RoomId>;
        std::vector<Room> rooms;
        rooms.emplace_back("a", test_maps::makeMap(), Links{{Side::East, "b"}});
        rooms.emplace_back("b", test_maps::makeMap(), Links{{Side::West, "a"}});
        rooms.emplace_back("island", test_maps::makeMap());
        const RoomGraph world(std::move(rooms));

        const auto report = analyzeReachability(world, startAt("a"));

        CHECK(report.room_reachable.at("a"));
        CHECK(report.room_reachable.at("b"));
        CHECK_FALSE(report.room_reachable.at("island"));
        CHECK_FALSE(report.allOk());
    }

    SUBCASE("a wall that is too tall blocks the way to the next room") {
        using Links = std::map<Side, domain::RoomId>;
        // A 10-tile (80 px) wall across the floor at column 20: cannot be jumped.
        std::vector<std::pair<int, int>> wall;
        for (int row = 17; row < 27; ++row) wall.emplace_back(20, row);
        std::vector<Room> rooms;
        rooms.emplace_back("a", test_maps::makeMap(wall), Links{{Side::East, "b"}});
        rooms.emplace_back("b", test_maps::makeMap(), Links{{Side::West, "a"}});
        const RoomGraph world(std::move(rooms));

        const auto report = analyzeReachability(world, startAt("a"));
        CHECK_FALSE(report.room_reachable.at("b"));
    }

    SUBCASE("a platform below the mid-air spawn is reached by simply falling onto it") {
        std::vector<std::pair<int, int>> platform;
        for (int column = 8; column <= 15; ++column) platform.emplace_back(column, 17);  // top 136
        const auto world = oneRoom({Coin{90.0F, 120.0F}}, platform);

        CHECK_FALSE(analyzeReachability(world, startAt("a", 200.0F)).coins[0].touchable);
        CHECK(analyzeReachability(world, startAt("a", 60.0F)).coins[0].touchable);
    }

    SUBCASE("an unknown start room is an error") {
        CHECK_THROWS_AS(analyzeReachability(oneRoom({}), startAt("nowhere")), std::invalid_argument);
    }

    SUBCASE("the state limit stops the search and says so") {
        ReachabilityOptions options = startAt("a");
        options.max_states = 50;
        const auto report = analyzeReachability(oneRoom({}), options);
        CHECK(report.truncated);
    }
}

TEST_CASE("RoomGraph::ids is sorted") {
    std::vector<Room> rooms;
    rooms.emplace_back("b", test_maps::makeMap());
    rooms.emplace_back("a", test_maps::makeMap());
    const RoomGraph world(std::move(rooms));

    CHECK(world.ids() == std::vector<domain::RoomId>{"a", "b"});
}

TEST_CASE("parseTextLevel") {
    const std::string source =
        "# a comment\n"
        "east: room_b\n"
        "west:   room_a  \n"
        "---\n"
        "....\n"
        ".C..\n"
        "#BB#\n";

    SUBCASE("reads size, tiles, coins and neighbours") {
        const TextLevel level = parseTextLevel(source);

        CHECK(level.width == 4);
        CHECK(level.height == 3);
        CHECK(level.tile_size == 8);
        CHECK(level.tiles[8] == kTileGround);  // row 2, column 0
        CHECK(level.tiles[9] == kTileBrick);
        CHECK(level.tiles[5] == 0);  // the coin's own tile stays empty
        REQUIRE(level.coins.size() == 1);
        CHECK(level.coins[0].x == 8.0F);
        CHECK(level.coins[0].y == 8.0F);
        CHECK(level.neighbors.at(Side::East) == "room_b");
        CHECK(level.neighbors.at(Side::West) == "room_a");
    }

    SUBCASE("accepts Windows line endings and a custom tile size") {
        const TextLevel level = parseTextLevel("tile_size: 16\r\n---\r\n..\r\n##\r\n");
        CHECK(level.tile_size == 16);
        CHECK(level.width == 2);
    }

    SUBCASE("builds a Room") {
        const Room room = toRoom("room_x", parseTextLevel(source));
        CHECK(room.id() == "room_x");
        CHECK(room.map().isSolid(0, 2));
        CHECK(room.coins().size() == 1);
    }

    SUBCASE("errors mention the line") {
        CHECK_THROWS_WITH_AS(parseTextLevel("---\n..\n.\n"), doctest::Contains("line 3"),
                             std::runtime_error);
        CHECK_THROWS_WITH_AS(parseTextLevel("---\n.x\n"), doctest::Contains("unknown character 'x'"),
                             std::runtime_error);
        CHECK_THROWS_WITH_AS(parseTextLevel("up: a\n---\n.\n"), doctest::Contains("unknown header key"),
                             std::runtime_error);
        CHECK_THROWS_WITH_AS(parseTextLevel("east\n---\n.\n"), doctest::Contains("line 1"),
                             std::runtime_error);
        CHECK_THROWS_WITH_AS(parseTextLevel("tile_size: abc\n---\n.\n"),
                             doctest::Contains("tile_size"), std::runtime_error);
    }

    SUBCASE("a missing separator or an empty grid is rejected") {
        CHECK_THROWS_AS(parseTextLevel("east: a\n....\n"), std::runtime_error);
        CHECK_THROWS_AS(parseTextLevel("---\n\n\n"), std::runtime_error);
    }
}

TEST_CASE("text <-> Tiled round trips") {
    const TextLevel level = parseTextLevel(
        "north: up\n"
        "east: right\n"
        "---\n"
        "..C.\n"
        ".BB.\n"
        "####\n");

    SUBCASE("toTiledJson produces a map the game loader reads back") {
        const Room room = infrastructure::parseTiledRoom("r", toTiledJson(level));

        CHECK(room.map().width() == 4);
        CHECK(room.map().height() == 3);
        CHECK(room.map().tileAt(1, 1) == kTileBrick);
        CHECK(room.map().tileAt(0, 2) == kTileGround);
        CHECK(room.neighbor(Side::North) == "up");
        CHECK(room.neighbor(Side::East) == "right");
        CHECK_FALSE(room.neighbor(Side::South).has_value());
        REQUIRE(room.coins().size() == 1);
        CHECK(room.coins()[0].x == 16.0F);
        CHECK(room.coins()[0].y == 0.0F);
    }

    SUBCASE("toText gives back the same level") {
        const TextLevel again = parseTextLevel(toText(toRoom("r", level)));

        CHECK(again.tiles == level.tiles);
        CHECK(again.neighbors == level.neighbors);
        REQUIRE(again.coins.size() == 1);
        CHECK(again.coins[0].x == level.coins[0].x);
        CHECK(again.coins[0].y == level.coins[0].y);
    }

    SUBCASE("toText rejects tiles it cannot draw and coins buried in solids") {
        std::vector<Room> unused;
        const Room strange("s", domain::TileMap(1, 1, 8, {7}));
        CHECK_THROWS_AS(toText(strange), std::runtime_error);

        const Room buried("b", domain::TileMap(1, 1, 8, {kTileGround}), {}, {Coin{0.0F, 0.0F}});
        CHECK_THROWS_AS(toText(buried), std::runtime_error);
    }
}

TEST_CASE("the shipped levels") {
    SUBCASE("every room survives tmj -> text -> tmj") {
        for (const char* name : {"room_01", "room_02", "room_03", "room_04"}) {
            INFO(name);
            const Room original = infrastructure::loadTiledRoom(std::string(PLATFORMER_ASSETS_DIR) +
                                                                "/tiled/" + name + ".tmj");
            const TextLevel level = parseTextLevel(toText(original));
            const Room again = infrastructure::parseTiledRoom(name, toTiledJson(level));

            for (int row = 0; row < original.map().height(); ++row) {
                for (int column = 0; column < original.map().width(); ++column) {
                    REQUIRE(again.map().tileAt(column, row) == original.map().tileAt(column, row));
                }
            }
            CHECK(again.neighbors() == original.neighbors());
            CHECK(again.coins().size() == original.coins().size());
        }
    }

    SUBCASE("the example level in game/levels is valid and fully reachable") {
        const TextLevel level =
            parseTextLevel(readFile(std::string(PLATFORMER_LEVELS_DIR) + "/example.txt"));

        std::vector<Room> rooms;
        rooms.push_back(toRoom("example", level));
        const RoomGraph world(std::move(rooms));

        const auto report = analyzeReachability(world, startAt("example"));
        CHECK(report.coins.size() == 3);
        CHECK(report.allOk());
    }
}
