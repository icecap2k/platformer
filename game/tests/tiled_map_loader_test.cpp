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
