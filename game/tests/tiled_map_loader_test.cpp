#include <doctest/doctest.h>

#include <stdexcept>
#include <string>
#include <vector>

#include "domain/world/room_graph.h"
#include "infrastructure/assets/tiled_map_loader.h"

using domain::Side;
using infrastructure::loadTiledRoom;
using infrastructure::parseTiledRoom;

namespace {

// Minimal map: 3x2 tiles of 8 px, one layer. The second tile id carries a
// "flipped horizontally" flag (bit 31) that must be ignored.
const std::string kSmallMap = R"({
  "width": 3, "height": 2, "tilewidth": 8, "tileheight": 8,
  "infinite": false,
  "properties": [
    { "name": "east",  "type": "string", "value": "room_b" },
    { "name": "north", "type": "string", "value": "" },
    { "name": "author", "type": "string", "value": "someone" }
  ],
  "layers": [
    { "type": "objectgroup", "name": "objects", "objects": [
        { "id": 1, "class": "coin", "x": 8, "y": 4, "width": 8, "height": 8 },
        { "id": 2, "type": "coin", "x": 16.5, "y": 0, "width": 0, "height": 0 },
        { "id": 3, "class": "spawn", "x": 1, "y": 1, "width": 8, "height": 8 }
    ] },
    { "type": "tilelayer", "name": "ground", "width": 3, "height": 2,
      "data": [0, 2147483649, 0,
               2, 0, 0] }
  ]
})";

// Two spike rectangles (one uses the old "type" field), a coin, and an object
// of another class that must be ignored.
const std::string kSpikeMap = R"({
  "width": 4, "height": 2, "tilewidth": 8, "tileheight": 8, "infinite": false,
  "layers": [
    { "type": "objectgroup", "name": "objects", "objects": [
        { "id": 1, "class": "spikes", "x": 8, "y": 11, "width": 16, "height": 5 },
        { "id": 2, "type": "spikes", "x": 0, "y": 13, "width": 8, "height": 3 },
        { "id": 3, "class": "coin", "x": 24, "y": 0, "width": 8, "height": 8 },
        { "id": 4, "class": "decoration", "x": 0, "y": 0, "width": 8, "height": 8 },
        { "id": 5, "class": "falling_stalactite", "x": 16, "y": 0, "width": 8, "height": 8 }
    ] },
    { "type": "tilelayer", "name": "ground", "width": 4, "height": 2, "data": [0,0,0,0, 1,1,1,1] }
  ]
})";

// A fruit (also with the old "type" field), a coin and another class, which is ignored.
const std::string kFruitMap = R"({
  "width": 4, "height": 2, "tilewidth": 8, "tileheight": 8, "infinite": false,
  "layers": [
    { "type": "objectgroup", "name": "objects", "objects": [
        { "id": 1, "class": "fruit", "x": 8, "y": 4, "width": 8, "height": 8 },
        { "id": 2, "type": "fruit", "x": 16.5, "y": 0, "width": 0, "height": 0 },
        { "id": 3, "class": "coin", "x": 24, "y": 0, "width": 8, "height": 8 },
        { "id": 4, "class": "decoration", "x": 0, "y": 0, "width": 8, "height": 8 }
    ] },
    { "type": "tilelayer", "name": "ground", "width": 4, "height": 2, "data": [0,0,0,0, 1,1,1,1] }
  ]
})";

// Walkers (also with the old "type" field), a coin and another class, which is ignored.
const std::string kWalkerMap = R"({
  "width": 4, "height": 2, "tilewidth": 8, "tileheight": 8, "infinite": false,
  "layers": [
    { "type": "objectgroup", "name": "objects", "objects": [
        { "id": 1, "class": "walker", "x": 8, "y": 8, "width": 8, "height": 8 },
        { "id": 2, "type": "walker", "x": 16.5, "y": 6, "width": 8, "height": 10 },
        { "id": 3, "class": "coin", "x": 24, "y": 0, "width": 8, "height": 8 }
    ] },
    { "type": "tilelayer", "name": "ground", "width": 4, "height": 2, "data": [0,0,0,0, 1,1,1,1] }
  ]
})";

const std::string kShippedRooms[] = {"room_01", "room_02", "room_03", "room_04"};

std::string roomPath(const std::string& name) {
    return std::string(PLATFORMER_ASSETS_DIR) + "/tiled/" + name + ".tmj";
}

}  // namespace

TEST_CASE("parseTiledRoom") {
    SUBCASE("reads dimensions and tiles from the first tile layer") {
        const auto room = parseTiledRoom("room_a", kSmallMap);
        const auto& map = room.map();

        CHECK(room.id() == "room_a");
        CHECK(map.width() == 3);
        CHECK(map.height() == 2);
        CHECK(map.tileSize() == 8);
        CHECK(map.tileAt(0, 1) == 2);
        CHECK(map.isSolid(1, 0));
    }

    SUBCASE("ignores the flip flags stored in the high bits") {
        const auto room = parseTiledRoom("room_a", kSmallMap);
        CHECK(room.map().tileAt(1, 0) == 1);
    }

    SUBCASE("reads neighbours from custom properties") {
        const auto room = parseTiledRoom("room_a", kSmallMap);

        CHECK(room.neighbor(Side::East) == "room_b");
        CHECK_FALSE(room.neighbor(Side::North).has_value());  // empty value = no neighbour
        CHECK_FALSE(room.neighbor(Side::West).has_value());
        CHECK(room.neighbors().size() == 1);  // unrelated properties are ignored
    }

    SUBCASE("reads coins from objects of class (or legacy type) coin") {
        const auto room = parseTiledRoom("room_a", kSmallMap);

        REQUIRE(room.coins().size() == 2);  // the "spawn" object is ignored
        CHECK(room.coins()[0].x == 8.0F);
        CHECK(room.coins()[0].y == 4.0F);
        CHECK(room.coins()[0].size == 8.0F);
        CHECK(room.coins()[1].x == 16.5F);
        CHECK(room.coins()[1].size == 8.0F);  // width 0 falls back to the default
    }

    SUBCASE("a map without properties has no neighbours") {
        const std::string plain = R"({"width":1,"height":1,"tilewidth":8,"tileheight":8,
                                     "layers":[{"type":"tilelayer","data":[0]}]})";
        CHECK(parseTiledRoom("r", plain).neighbors().empty());
    }

    SUBCASE("rejects text that is not JSON") {
        CHECK_THROWS_AS(parseTiledRoom("r", "not json"), std::runtime_error);
    }

    SUBCASE("rejects a map without a tile layer") {
        const std::string no_layer = R"({"width":1,"height":1,"tilewidth":8,"tileheight":8,
                                        "layers":[{"type":"objectgroup","objects":[]}]})";
        CHECK_THROWS_AS(parseTiledRoom("r", no_layer), std::runtime_error);
    }

    SUBCASE("rejects data whose size does not match the map") {
        const std::string wrong_size = R"({"width":2,"height":2,"tilewidth":8,"tileheight":8,
                                          "layers":[{"type":"tilelayer","data":[0,1,0]}]})";
        CHECK_THROWS_AS(parseTiledRoom("r", wrong_size), std::runtime_error);
    }

    SUBCASE("rejects non-square tiles") {
        const std::string rectangular = R"({"width":1,"height":1,"tilewidth":8,"tileheight":16,
                                           "layers":[{"type":"tilelayer","data":[0]}]})";
        CHECK_THROWS_AS(parseTiledRoom("r", rectangular), std::runtime_error);
    }

    SUBCASE("rejects base64-encoded layer data") {
        const std::string encoded = R"({"width":1,"height":1,"tilewidth":8,"tileheight":8,
                                       "layers":[{"type":"tilelayer","data":"AAAAAA=="}]})";
        CHECK_THROWS_AS(parseTiledRoom("r", encoded), std::runtime_error);
    }
}

TEST_CASE("loadTiledRoom") {
    SUBCASE("reports a missing file") {
        CHECK_THROWS_AS(loadTiledRoom("does/not/exist.tmj"), std::runtime_error);
    }

    SUBCASE("uses the file name as the room id") {
        CHECK(loadTiledRoom(roomPath("room_01")).id() == "room_01");
    }
}

TEST_CASE("parseTiledRoom reads spikes") {
    SUBCASE("every object of class spikes becomes a hazard with the drawn rectangle") {
        const auto room = parseTiledRoom("r", kSpikeMap);

        REQUIRE(room.hazards().size() == 2);
        CHECK(room.hazards()[0].bounds.x == 8.0F);
        CHECK(room.hazards()[0].bounds.y == 11.0F);
        CHECK(room.hazards()[0].bounds.width == 16.0F);
        CHECK(room.hazards()[0].bounds.height == 5.0F);
        CHECK(room.hazards()[1].bounds.y == 13.0F);
        CHECK(room.coins().size() == 1);  // other objects keep working
    }

    SUBCASE("falling_stalactite objects become stalactite traps") {
        const auto room = parseTiledRoom("r", kSpikeMap);

        REQUIRE(room.stalactites().size() == 1);
        CHECK(room.stalactites()[0].bounds.x == 16.0F);
        CHECK(room.stalactites()[0].bounds.width == 8.0F);
        CHECK(room.hazards().size() == 2);  // and they are not spikes
    }

    SUBCASE("a stalactite trap that is not a rectangle is rejected") {
        std::string broken = kSpikeMap;
        const std::string needle = "\"x\": 16, \"y\": 0, \"width\": 8";
        broken.replace(broken.find(needle), needle.size(), "\"x\": 16, \"y\": 0, \"width\": 0");
        CHECK_THROWS_WITH_AS(parseTiledRoom("r", broken), doctest::Contains("falling_stalactite"),
                             std::runtime_error);
    }

    SUBCASE("a room without spikes has no hazards") {
        CHECK(parseTiledRoom("r", kSmallMap).hazards().empty());
    }

    SUBCASE("spikes that are not a rectangle are rejected, naming the problem") {
        std::string broken = kSpikeMap;
        broken.replace(broken.find("\"width\": 16"), 11, "\"width\": 0");
        CHECK_THROWS_WITH_AS(parseTiledRoom("r", broken), doctest::Contains("spikes"),
                             std::runtime_error);
    }
}

TEST_CASE("parseTiledRoom reads enemies") {
    SUBCASE("every object of class walker becomes an enemy whose rectangle is its start and hitbox") {
        const auto room = parseTiledRoom("r", kWalkerMap);
        REQUIRE(room.enemies().size() == 2);
        CHECK(room.enemies()[0].bounds.x == 8.0F);
        CHECK(room.enemies()[0].bounds.y == 8.0F);
        CHECK(room.enemies()[0].bounds.width == 8.0F);
        CHECK(room.enemies()[1].bounds.x == 16.5F);
        CHECK(room.enemies()[1].bounds.height == 10.0F);
        CHECK(room.coins().size() == 1);  // other classes are not enemies
    }
    SUBCASE("a walker without a size is an error") {
        const std::string text = R"({
          "width": 1, "height": 1, "tilewidth": 8, "tileheight": 8, "infinite": false,
          "layers": [ { "type": "objectgroup", "name": "o", "objects": [
            { "id": 1, "class": "walker", "x": 0, "y": 0 } ] },
            { "type": "tilelayer", "name": "g", "width": 1, "height": 1, "data": [0] } ] })";
        CHECK_THROWS_AS(parseTiledRoom("r", text), std::runtime_error);
    }
    SUBCASE("a room without enemies has none") {
        CHECK(parseTiledRoom("r", kSmallMap).enemies().empty());
    }
}

TEST_CASE("parseTiledRoom reads fruits") {
    SUBCASE("every object of class fruit becomes a fruit at its position") {
        const auto room = parseTiledRoom("r", kFruitMap);

        REQUIRE(room.fruits().size() == 2);
        CHECK(room.fruits()[0].x == 8.0F);
        CHECK(room.fruits()[0].y == 4.0F);
        CHECK(room.fruits()[0].size == 8.0F);
        CHECK(room.fruits()[1].x == 16.5F);
        CHECK(room.fruits()[1].size == 8.0F);  // no width: the default size
    }

    SUBCASE("fruits and coins are separate lists") {
        const auto room = parseTiledRoom("r", kFruitMap);
        CHECK(room.coins().size() == 1);
        CHECK(room.hazards().empty());
    }

    SUBCASE("a room without fruits has none") {
        CHECK(parseTiledRoom("r", kSmallMap).fruits().empty());
    }
}

TEST_CASE("the shipped rooms form a consistent world") {
    std::vector<domain::Room> rooms;
    for (const std::string& name : kShippedRooms) {
        rooms.push_back(loadTiledRoom(roomPath(name)));
    }

    // The RoomGraph constructor already rejects links to unknown rooms.
    const domain::RoomGraph graph(std::move(rooms));

    SUBCASE("every room is 40x30 tiles of 8 px with a solid floor") {
        for (const std::string& name : kShippedRooms) {
            const auto& map = graph.room(name).map();
            CHECK(map.width() == 40);
            CHECK(map.height() == 30);
            CHECK(map.tileSize() == 8);
            CHECK(map.isSolid(0, 29));
        }
    }

    SUBCASE("every link is reciprocal") {
        for (const std::string& name : kShippedRooms) {
            for (const auto& [side, neighbor_id] : graph.room(name).neighbors()) {
                INFO(name << " -> " << neighbor_id);
                CHECK(graph.room(neighbor_id).neighbor(domain::opposite(side)) == name);
            }
        }
    }

    SUBCASE("coins: one in some rooms, none in others") {
        CHECK(graph.room("room_01").coins().size() == 1);
        CHECK(graph.room("room_02").coins().empty());
        CHECK(graph.room("room_03").coins().size() == 1);
        CHECK(graph.room("room_04").coins().size() == 1);
        CHECK(graph.totalCoins() == 3);
    }

    SUBCASE("room_01 climbs to its plateau by a 5 tile ramp that rests on solid ground") {
        const auto& map = graph.room("room_01").map();
        for (int i = 0; i < 5; ++i) {
            const int column = 27 + i;
            const int row = 26 - i;
            INFO("ramp tile " << column << "," << row);
            CHECK(map.shape(column, row) == domain::TileShape::SlopeRight);
            CHECK(map.isSolid(column, row + 1));      // it stands on solid ground
            if (i > 0) {
                CHECK(map.isSolid(column, row + 1));  // with filler under it
            }
            CHECK(map.isSolid(column + 1, row));      // and a block at its high side
        }
        CHECK(map.isSolid(32, 22));                   // the plateau
        CHECK_FALSE(map.isGround(32, 21));
    }

    SUBCASE("healing fruits: one in room_02 (past the spikes) and one in room_04") {
        CHECK(graph.room("room_01").fruits().empty());
        CHECK(graph.room("room_02").fruits().size() == 1);
        CHECK(graph.room("room_03").fruits().empty());
        CHECK(graph.room("room_04").fruits().size() == 1);
    }

    SUBCASE("no fruit is buried inside a solid tile, and each rests on something") {
        for (const std::string& name : kShippedRooms) {
            const auto& room = graph.room(name);
            const auto& map = room.map();
            const int ts = map.tileSize();
            for (const auto& fruit : room.fruits()) {
                INFO(name << " fruit at " << fruit.x << "," << fruit.y);
                const int column = static_cast<int>(fruit.x) / ts;
                const int row = static_cast<int>(fruit.y) / ts;
                CHECK_FALSE(map.isGround(column, row));
                CHECK(map.isGround(column, row + 1));  // it is not left floating in the air
            }
        }
    }

    SUBCASE("enemies: one walker in room_03 and one in room_04, each standing on the floor") {
        CHECK(graph.room("room_01").enemies().empty());
        CHECK(graph.room("room_02").enemies().empty());
        CHECK(graph.room("room_03").enemies().size() == 1);
        CHECK(graph.room("room_04").enemies().size() == 1);
        for (const std::string& name : kShippedRooms) {
            const auto& map = graph.room(name).map();
            const int ts = map.tileSize();
            for (const auto& enemy : graph.room(name).enemies()) {
                INFO(name << " enemy at " << enemy.bounds.x << "," << enemy.bounds.y);
                const int column = static_cast<int>(enemy.bounds.x) / ts;
                const int row = static_cast<int>(enemy.bounds.y) / ts;
                CHECK_FALSE(map.isGround(column, row));
                CHECK(map.isGround(column, row + 1));
                CHECK(enemy.bounds.bottom() == static_cast<float>((row + 1) * ts));  // feet on the floor
            }
        }
    }

    SUBCASE("hazards: spikes in rooms 01, 02 and 04, stalactite traps in 01 and 04") {
        CHECK_FALSE(graph.room("room_01").hazards().empty());
        CHECK_FALSE(graph.room("room_02").hazards().empty());
        CHECK(graph.room("room_03").hazards().empty());
        CHECK_FALSE(graph.room("room_04").hazards().empty());
        CHECK(graph.room("room_01").stalactites().size() == 1);
        CHECK(graph.room("room_02").stalactites().empty());
        CHECK(graph.room("room_03").stalactites().empty());
        CHECK(graph.room("room_04").stalactites().size() == 1);
    }

    SUBCASE("hazards and stalactites are not buried in solid tiles, and traps hang from a ceiling") {
        for (const std::string& name : kShippedRooms) {
            const auto& room = graph.room(name);
            const auto& map = room.map();
            const int ts = map.tileSize();
            for (const auto& hazard : room.hazards()) {
                INFO(name << " hazard at " << hazard.bounds.x << "," << hazard.bounds.y);
                for (int column = static_cast<int>(hazard.bounds.left()) / ts;
                     column <= static_cast<int>(hazard.bounds.right() - 1) / ts; ++column) {
                    for (int row = static_cast<int>(hazard.bounds.top()) / ts;
                         row <= static_cast<int>(hazard.bounds.bottom() - 1) / ts; ++row) {
                        CHECK_FALSE(map.isSolid(column, row));
                    }
                }
            }
            for (const auto& stalactite : room.stalactites()) {
                INFO(name << " stalactite at " << stalactite.bounds.x << "," << stalactite.bounds.y);
                const int column = static_cast<int>(stalactite.bounds.x) / ts;
                const int row = static_cast<int>(stalactite.bounds.y) / ts;
                CHECK_FALSE(map.isSolid(column, row));
                CHECK(map.isSolid(column, row - 1));  // it hangs from something solid
            }
        }
    }

    SUBCASE("no coin is buried inside a solid tile") {
        for (const std::string& name : kShippedRooms) {
            const auto& room = graph.room(name);
            for (const auto& coin : room.coins()) {
                INFO(name << " coin at " << coin.x << "," << coin.y);
                const int ts = room.map().tileSize();
                const int first_col = static_cast<int>(coin.x) / ts;
                const int last_col = static_cast<int>(coin.x + coin.size - 1) / ts;
                const int first_row = static_cast<int>(coin.y) / ts;
                const int last_row = static_cast<int>(coin.y + coin.size - 1) / ts;
                for (int row = first_row; row <= last_row; ++row) {
                    for (int col = first_col; col <= last_col; ++col) {
                        CHECK_FALSE(room.map().isSolid(col, row));
                    }
                }
            }
        }
    }

    SUBCASE("all four directions are used somewhere") {
        bool used[4] = {false, false, false, false};
        for (const std::string& name : kShippedRooms) {
            for (const auto& [side, neighbor_id] : graph.room(name).neighbors()) {
                used[static_cast<int>(side)] = true;
            }
        }
        for (bool direction_used : used) {
            CHECK(direction_used);
        }
    }
}
