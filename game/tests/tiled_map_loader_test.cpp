#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <set>
#include <string>
#include <vector>

#include "domain/enemies/enemy_patrols.h"
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

const std::string kShippedRooms[] = {"room_01", "room_02", "room_03", "room_04", "room_05",
                                     "room_06", "room_07", "room_08", "room_09"};

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

TEST_CASE("parseTiledRoom reads checkpoints") {
    const std::string text = R"({
      "width": 4, "height": 2, "tilewidth": 8, "tileheight": 8, "infinite": false,
      "layers": [
        { "type": "objectgroup", "name": "objects", "objects": [
            { "id": 1, "class": "checkpoint", "x": 8, "y": 0, "width": 8, "height": 8 },
            { "id": 2, "type": "checkpoint", "x": 24, "y": 0 },
            { "id": 3, "class": "coin", "x": 0, "y": 0, "width": 8, "height": 8 }
        ] },
        { "type": "tilelayer", "name": "ground", "width": 4, "height": 2, "data": [0,0,0,0, 1,1,1,1] }
      ] })";
    const auto room = parseTiledRoom("r", text);
    REQUIRE(room.checkpoints().size() == 2);
    CHECK(room.checkpoints()[0].x == 8.0F);
    CHECK(room.checkpoints()[1].x == 24.0F);
    CHECK(room.checkpoints()[1].size == 8.0F);  // no width: the default size
    CHECK(room.coins().size() == 1);
    CHECK(parseTiledRoom("r", kSmallMap).checkpoints().empty());
}

TEST_CASE("parseTiledRoom reads exits") {
    const std::string text = R"({
      "width": 4, "height": 2, "tilewidth": 8, "tileheight": 8, "infinite": false,
      "layers": [
        { "type": "objectgroup", "name": "objects", "objects": [
            { "id": 1, "class": "exit", "x": 8, "y": 0, "width": 8, "height": 8 },
            { "id": 2, "type": "exit", "x": 24, "y": 0 },
            { "id": 3, "class": "coin", "x": 0, "y": 0, "width": 8, "height": 8 }
        ] },
        { "type": "tilelayer", "name": "ground", "width": 4, "height": 2, "data": [0,0,0,0, 1,1,1,1] }
      ] })";
    const auto room = parseTiledRoom("r", text);
    REQUIRE(room.exits().size() == 2);
    CHECK(room.exits()[0].x == 8.0F);
    CHECK(room.exits()[1].size == 8.0F);  // no width: the default size
    CHECK(parseTiledRoom("r", kSmallMap).exits().empty());
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

    SUBCASE("three orbs, one of each colour, floating in three different rooms") {
        std::set<domain::OrbColour> colours;
        int orbs = 0;
        int rooms_with_orbs = 0;
        for (const std::string& name : kShippedRooms) {
            const auto& room = graph.room(name);
            rooms_with_orbs += room.orbs().empty() ? 0 : 1;
            const float size = static_cast<float>(room.map().tileSize());
            for (const auto& orb : room.orbs()) {
                INFO(name << " orb at " << orb.x << "," << orb.y);
                ++orbs;
                colours.insert(orb.colour);
                // Floating: not even the tile row right under it is ground.
                const int row = static_cast<int>((orb.y + orb.size) / size);
                for (float x = orb.x; x < orb.x + orb.size; x += size * 0.5F) {
                    CHECK_FALSE(room.map().isGround(static_cast<int>(x / size), row));
                }
            }
        }
        CHECK(orbs == 3);
        CHECK(colours.size() == 3);
        CHECK(rooms_with_orbs == 3);
        CHECK(graph.totalOrbs() == 3);
    }

    SUBCASE("the world has nine rooms and exactly one exit, in the last one") {
        CHECK(graph.size() == 9);
        int exits = 0;
        for (const std::string& name : kShippedRooms) {
            exits += static_cast<int>(graph.room(name).exits().size());
        }
        CHECK(exits == 1);
        CHECK(graph.room("room_09").exits().size() == 1);
    }

    SUBCASE("every neighbour link points at a room that exists") {
        for (const std::string& name : kShippedRooms) {
            for (const auto& [side, other] : graph.room(name).neighbors()) {
                INFO(name << " -> " << other);
                CHECK(graph.contains(other));
            }
        }
    }

    SUBCASE("walkers are two tiles big and every one of them patrols") {
        for (const std::string& name : kShippedRooms) {
            const auto& room = graph.room(name);
            for (const auto& enemy : room.enemies()) {
                if (enemy.kind == domain::EnemyKind::Bat) {
                    INFO(name << " bat at " << enemy.bounds.x << "," << enemy.bounds.y << " (it must start in the open)");
                    const float size = static_cast<float>(room.map().tileSize());
                    for (const float fx : {0.05F, 0.5F, 0.95F}) {
                        for (const float fy : {0.05F, 0.5F, 0.95F}) {
                            CHECK_FALSE(room.map().isGround(
                                static_cast<int>((enemy.bounds.x + enemy.bounds.width * fx) / size),
                                static_cast<int>((enemy.bounds.y + enemy.bounds.height * fy) / size)));
                        }
                    }
                    continue;
                }
                INFO(name << " walker at " << enemy.bounds.x << "," << enemy.bounds.y);
                CHECK(enemy.bounds.width == 16.0F);
                CHECK(enemy.bounds.height == 16.0F);
                // A walker in a hole or between two walls would only turn around where it is.
                domain::EnemyState state;
                float left = 0.0F;
                float right = 0.0F;
                for (int step = 0; step < 600; ++step) {
                    domain::stepEnemy(enemy, state, room.map(), room.hazards());
                    left = std::min(left, state.x_offset);
                    right = std::max(right, state.x_offset);
                }
                CHECK(right - left >= 8.0F);
            }
        }
    }

    SUBCASE("everything is on the screen, and nothing to touch is buried in rock") {
        for (const std::string& name : kShippedRooms) {
            const auto& room = graph.room(name);
            const auto check = [&](const char* what, const domain::AABB& box) {
                INFO(name << " " << what << " at " << box.x << "," << box.y);
                CHECK(box.left() >= 0.0F);
                CHECK(box.top() >= 0.0F);
                CHECK(box.right() <= room.map().pixelWidth());
                CHECK(box.bottom() <= room.map().pixelHeight());
                const float size = static_cast<float>(room.map().tileSize());
                CHECK_FALSE(room.map().isSolid(static_cast<int>((box.x + box.width * 0.5F) / size),
                                               static_cast<int>((box.y + box.height * 0.5F) / size)));
            };
            for (const auto& coin : room.coins()) {
                check("coin", coin.bounds());
            }
            for (const auto& fruit : room.fruits()) {
                check("fruit", fruit.bounds());
            }
            for (const auto& orb : room.orbs()) {
                check("orb", orb.bounds());
            }
            for (const auto& checkpoint : room.checkpoints()) {
                check("checkpoint", checkpoint.bounds());
            }
            for (const auto& exit : room.exits()) {
                check("exit", exit.bounds());
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
