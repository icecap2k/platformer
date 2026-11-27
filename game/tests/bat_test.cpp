#include <doctest/doctest.h>

#include <cmath>
#include <variant>

#include "application/game_session.h"
#include "domain/enemies/enemy_contact.h"
#include "domain/enemies/enemy_patrols.h"
#include "infrastructure/assets/tiled_map_loader.h"
#include "level_tools/text_level.h"
#include "test_maps.h"

using namespace domain;
using test_maps::kFloorTop;
using test_maps::makeMap;

namespace {

// A bat hovering at (x, y) over an empty room with a floor.
Enemy batAt(float x, float y, float radius = 64.0F) {
    return Enemy{AABB{x, y, 16.0F, 12.0F}, EnemyKind::Bat, radius};
}

Player standingAt(float x) {
    Player player;
    player.x = x;
    player.y = kFloorTop - player.height;
    player.on_ground = true;
    return player;
}

void fly(const Enemy& bat, EnemyState& state, const TileMap& map, const Player& player, int steps) {
    for (int i = 0; i < steps; ++i) {
        stepBat(bat, state, map, player);
    }
}

}  // namespace

TEST_CASE("an undisturbed bat flies around its spot") {
    const TileMap map = makeMap();
    const Enemy bat = batAt(150.0F, 100.0F);
    const Player far = standingAt(10.0F);
    EnemyState state;
    float left = 0.0F;
    float right = 0.0F;
    for (int step = 0; step < 700; ++step) {
        stepBat(bat, state, map, far);
        left = std::min(left, state.x_offset);
        right = std::max(right, state.x_offset);
        CHECK_FALSE(state.chasing);
        CHECK(std::abs(state.y_offset) <= kBatPatrolBob + 0.5F);
    }
    CHECK(left < -kBatPatrolReach * 0.8F);
    CHECK(right > kBatPatrolReach * 0.8F);
}

TEST_CASE("it notices the player only within its radius") {
    const TileMap map = makeMap();
    const Player player = standingAt(60.0F);

    EnemyState near_state;
    stepBat(batAt(100.0F, 180.0F, 64.0F), near_state, map, player);
    CHECK(near_state.chasing);

    EnemyState short_sighted;
    stepBat(batAt(100.0F, 180.0F, 30.0F), short_sighted, map, player);
    CHECK_FALSE(short_sighted.chasing);
}

TEST_CASE("chasing, it comes down to the height of the player's feet") {
    const TileMap map = makeMap();
    const Enemy bat = batAt(140.0F, 170.0F);
    const Player player = standingAt(100.0F);
    EnemyState state;

    fly(bat, state, map, player, 150);
    const AABB box = currentBounds(bat, state);
    CHECK(box.bottom() == doctest::Approx(kFloorTop).epsilon(0.01));  // low: feet level
    CHECK(std::abs((box.x + box.width * 0.5F) - (player.x + 4.0F)) < 2.0F);
}

TEST_CASE("it does not follow a jump upwards, so it can be stomped") {
    const TileMap map = makeMap();
    const Enemy bat = batAt(140.0F, 170.0F);
    Player player = standingAt(100.0F);
    EnemyState state;
    fly(bat, state, map, player, 150);

    player.y -= 40.0F;  // up in the air above it
    player.on_ground = false;
    fly(bat, state, map, player, 60);
    CHECK(currentBounds(bat, state).bottom() == doctest::Approx(kFloorTop).epsilon(0.01));

    player.y = currentBounds(bat, state).top() - player.height + 3.0F;  // coming down on it
    player.velocity_y = 3.0F;
    CHECK(isStomp(player, currentBounds(bat, state)));
}

TEST_CASE("it gives up when the player gets far away, and goes back home") {
    const TileMap map = makeMap();
    const Enemy bat = batAt(150.0F, 170.0F);
    EnemyState state;
    fly(bat, state, map, standingAt(110.0F), 60);
    REQUIRE(state.chasing);

    const Player gone = standingAt(300.0F);
    fly(bat, state, map, gone, 1);
    CHECK_FALSE(state.chasing);
    fly(bat, state, map, standingAt(0.0F), 500);
    CHECK(std::abs(state.x_offset) <= kBatPatrolReach + 1.0F);
    CHECK(std::abs(state.y_offset) <= kBatPatrolBob + 1.0F);
}

TEST_CASE("after biting it flies away for a moment") {
    const TileMap map = makeMap();
    const Enemy bat = batAt(104.0F, 200.0F);
    const Player player = standingAt(100.0F);
    EnemyState state;
    stepBat(bat, state, map, player);
    batRetreats(state);
    const float before = currentBounds(bat, state).y;
    fly(bat, state, map, player, 30);
    CHECK(currentBounds(bat, state).y < before - 8.0F);
    CHECK_FALSE(state.chasing);
    fly(bat, state, map, player, kBatRetreatSteps);
    CHECK(state.chasing);  // and then it comes back
}

TEST_CASE("a bat never flies into rock") {
    // A wall between the bat and the player.
    std::vector<std::pair<int, int>> wall;
    for (int row = 18; row <= 26; ++row) wall.emplace_back(15, row);
    const TileMap map = makeMap(wall);
    const Enemy bat = batAt(140.0F, 180.0F, 100.0F);
    EnemyState state;
    for (int step = 0; step < 300; ++step) {
        stepBat(bat, state, map, standingAt(90.0F));
        const AABB box = currentBounds(bat, state);
        CHECK(box.left() >= 128.0F - 0.01F);
        CHECK(box.bottom() <= kFloorTop + 0.01F);
    }
}

TEST_CASE("a defeated bat drops to the ground and lies there") {
    const TileMap map = makeMap();
    const Enemy bat = batAt(150.0F, 100.0F);
    EnemyState state;
    state.alive = false;
    fly(bat, state, map, standingAt(10.0F), 59);
    CHECK(currentBounds(bat, state).bottom() == kFloorTop);
    CHECK(isSquashedBody(state));
    fly(bat, state, map, standingAt(10.0F), 5);
    CHECK_FALSE(isSquashedBody(state));
}

TEST_CASE("in a session a bat can be stomped and its bite costs a heart") {
    const auto world = [](float bat_x) {
        std::vector<Room> rooms;
        rooms.emplace_back("only", makeMap(), std::map<Side, RoomId>{}, std::vector<Coin>{}, std::vector<Hazard>{},
                           std::vector<FallingStalactite>{}, std::vector<Fruit>{},
                           std::vector<Enemy>{batAt(bat_x, kFloorTop - 12.0F)});
        return RoomGraph(std::move(rooms));
    };

    application::GameSession bitten(world(130.0F), "only", 100.0F, kFloorTop - 16.0F);
    int hurts = 0;
    for (int step = 0; step < 60; ++step) {
        for (const auto& event : bitten.update(InputState{})) {
            hurts += std::holds_alternative<application::PlayerHurt>(event) ? 1 : 0;
        }
    }
    CHECK(hurts == 1);
    CHECK(bitten.player().health == kMaxHealth - 1);

    application::GameSession stomping(world(100.0F), "only", 104.0F, kFloorTop - 60.0F);
    bool stomped = false;
    for (int step = 0; step < 60 && !stomped; ++step) {
        for (const auto& event : stomping.update(InputState{})) {
            stomped = stomped || std::holds_alternative<application::EnemyStomped>(event);
        }
    }
    CHECK(stomped);
    CHECK(stomping.player().health == kMaxHealth);
}

TEST_CASE("bats are read from Tiled, with their radius") {
    const std::string map = R"({
      "width": 4, "height": 4, "tilewidth": 8, "tileheight": 8,
      "layers": [
        { "type": "tilelayer", "data": [0,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,1] },
        { "type": "objectgroup", "objects": [
          { "id": 1, "class": "bat", "x": 4, "y": 4, "width": 16, "height": 12 },
          { "id": 2, "class": "bat", "x": 8, "y": 4, "width": 16, "height": 12,
            "properties": [ { "name": "detect_radius", "type": "float", "value": 96 } ] },
          { "id": 3, "class": "walker", "x": 0, "y": 8, "width": 16, "height": 16 } ] }
      ] })";
    const Room room = infrastructure::parseTiledRoom("r", map);
    REQUIRE(room.enemies().size() == 3);
    CHECK(room.enemies()[0].kind == EnemyKind::Bat);
    CHECK(room.enemies()[0].detect_radius == kBatDetectRadius);
    CHECK(room.enemies()[1].detect_radius == 96.0F);
    CHECK(room.enemies()[2].kind == EnemyKind::Walker);

    std::string negative = map;
    negative.replace(negative.find("96"), 2, "-1");
    CHECK_THROWS_WITH_AS(infrastructure::parseTiledRoom("r", negative), doctest::Contains("detect_radius"),
                         std::runtime_error);
}

TEST_CASE("bats (M) survive text -> tmj -> game -> text") {
    const auto level = level_tools::parseTextLevel("---\n........\n..M.....\n########\n");
    REQUIRE(level.enemies.size() == 1);
    CHECK(level.enemies[0].kind == EnemyKind::Bat);
    CHECK(level.enemies[0].bounds.height == 12.0F);
    CHECK(level.enemies[0].bounds.bottom() == 16.0F);
    const Room room = infrastructure::parseTiledRoom("r", level_tools::toTiledJson(level));
    const auto again = level_tools::parseTextLevel(level_tools::toText(room));
    REQUIRE(again.enemies.size() == 1);
    CHECK(again.enemies[0].kind == EnemyKind::Bat);
    CHECK(again.enemies[0].bounds.y == level.enemies[0].bounds.y);
}
