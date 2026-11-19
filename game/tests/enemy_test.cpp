#include <doctest/doctest.h>

#include "domain/enemies/enemy_contact.h"
#include "domain/enemies/enemy_patrols.h"
#include "domain/physics/player_physics.h"
#include "test_maps.h"

using namespace domain;
using test_maps::kFloorTop;
using test_maps::makeMap;

namespace {

// An 8x8 enemy standing on the floor at pixel column `x`.
Enemy walkerAt(float x) {
    return Enemy{AABB{x, kFloorTop - 8.0F, 8.0F, 8.0F}};
}

void walk(const Enemy& enemy, EnemyState& state, const TileMap& map, int steps) {
    for (int i = 0; i < steps; ++i) {
        stepEnemy(enemy, state, map, {});
    }
}

}  // namespace

TEST_CASE("an enemy walks along the floor at a steady speed") {
    const TileMap map = makeMap();
    const Enemy enemy = walkerAt(100.0F);
    EnemyState state;
    state.direction = 1;

    walk(enemy, state, map, 20);
    CHECK(state.x_offset == doctest::Approx(20 * kEnemySpeed));
    CHECK(currentBounds(enemy, state).x == doctest::Approx(100.0F + 20 * kEnemySpeed));
    CHECK(currentBounds(enemy, state).y == kFloorTop - 8.0F);  // it never leaves the floor
}

TEST_CASE("an enemy turns around at a wall") {
    // A wall of two blocks standing on the floor at column 20 (x = 160).
    const TileMap map = makeMap({{20, 26}, {20, 25}});
    const Enemy enemy = walkerAt(140.0F);
    EnemyState state;
    state.direction = 1;

    walk(enemy, state, map, 200);  // far more than needed to reach the wall
    CHECK(currentBounds(enemy, state).right() <= 160.0F);  // never inside it
    CHECK(state.direction == -1);                           // and now it is on its way back

    walk(enemy, state, map, 2000);
    const float x = currentBounds(enemy, state).x;
    CHECK(x >= 0.0F);
    CHECK(currentBounds(enemy, state).right() <= 160.0F);   // it keeps pacing, it never gets through
}

TEST_CASE("an enemy turns around at a ledge instead of falling") {
    // The floor has a 2-tile hole at columns 20 and 21 (x = 160..176).
    std::vector<int> tiles(static_cast<std::size_t>(test_maps::kWidth * test_maps::kHeight), 0);
    for (int row = test_maps::kFloorRow; row < test_maps::kHeight; ++row) {
        for (int column = 0; column < test_maps::kWidth; ++column) {
            if (column != 20 && column != 21) {
                tiles[static_cast<std::size_t>(row * test_maps::kWidth + column)] = 1;
            }
        }
    }
    const TileMap map(test_maps::kWidth, test_maps::kHeight, test_maps::kTileSize, tiles);
    const Enemy enemy = walkerAt(140.0F);
    EnemyState state;
    state.direction = 1;

    walk(enemy, state, map, 500);
    CHECK(currentBounds(enemy, state).right() <= 160.0F);  // its front foot never goes over the hole
    CHECK(currentBounds(enemy, state).y == kFloorTop - 8.0F);
}

TEST_CASE("an enemy turns around at the edge of the room") {
    const TileMap map = makeMap();
    const Enemy enemy = walkerAt(4.0F);
    EnemyState state;  // starts going left

    walk(enemy, state, map, 100);
    CHECK(currentBounds(enemy, state).x >= 0.0F);
    CHECK(state.direction == 1);
}

TEST_CASE("an enemy turns around at a ramp") {
    // Ramps are not solid but they are a surface: the enemy treats one in its way as a wall.
    std::vector<int> tiles(static_cast<std::size_t>(test_maps::kWidth * test_maps::kHeight), 0);
    for (int row = test_maps::kFloorRow; row < test_maps::kHeight; ++row) {
        for (int column = 0; column < test_maps::kWidth; ++column) {
            tiles[static_cast<std::size_t>(row * test_maps::kWidth + column)] = 1;
        }
    }
    tiles[static_cast<std::size_t>(26 * test_maps::kWidth + 20)] = kTileSlopeRight;
    const TileMap map(test_maps::kWidth, test_maps::kHeight, test_maps::kTileSize, tiles);
    const Enemy enemy = walkerAt(140.0F);
    EnemyState state;
    state.direction = 1;

    walk(enemy, state, map, 300);
    CHECK(currentBounds(enemy, state).right() <= 160.0F);
}

TEST_CASE("a dead enemy does not move") {
    const TileMap map = makeMap();
    const Enemy enemy = walkerAt(100.0F);
    EnemyState state;
    state.alive = false;
    walk(enemy, state, map, 10);
    CHECK(state.x_offset == 0.0F);
}

TEST_CASE("EnemyPatrols: an enemy nobody touched is alive at its start, and states are per room and index") {
    EnemyPatrols patrols;
    CHECK(patrols.state("a", 0).alive);
    CHECK(patrols.state("a", 0).x_offset == 0.0F);

    patrols.state("a", 0).alive = false;
    CHECK_FALSE(patrols.state("a", 0).alive);
    CHECK(patrols.state("a", 1).alive);
    CHECK(patrols.state("b", 0).alive);
}

TEST_CASE("isStomp: only a falling player whose feet are near the top of the enemy") {
    const AABB enemy{100.0F, kFloorTop - 8.0F, 8.0F, 8.0F};
    Player player;
    player.x = 100.0F;

    SUBCASE("falling onto it from above is a stomp") {
        player.y = enemy.top() - player.height + 3.0F;  // feet 3 px into it
        player.velocity_y = 3.0F;
        CHECK(isStomp(player, enemy));
    }
    SUBCASE("walking into it at the same height is not") {
        player.y = kFloorTop - player.height;  // standing on the same floor
        player.velocity_y = 0.0F;
        CHECK_FALSE(isStomp(player, enemy));
    }
    SUBCASE("falling alongside it (feet already well below its top) is not") {
        player.y = kFloorTop - player.height - 1.0F;  // feet 1 px above the floor
        player.velocity_y = 3.0F;
        CHECK_FALSE(isStomp(player, enemy));
    }
    SUBCASE("jumping up into it from below is not") {
        player.y = enemy.top() - player.height + 3.0F;
        player.velocity_y = -2.0F;
        CHECK_FALSE(isStomp(player, enemy));
    }
}

TEST_CASE("bounceOffEnemy sends the player up and into the air") {
    Player player;
    player.on_ground = true;
    player.coyote_steps = 3;
    player.slide_steps = 5;
    player.velocity_y = 3.0F;

    bounceOffEnemy(player);
    CHECK(player.velocity_y == -kStompBounceSpeed);
    CHECK_FALSE(player.on_ground);
    CHECK(player.coyote_steps == 0);
    CHECK(player.slide_steps == 0);
}

TEST_CASE("an enemy turns around before walking onto spikes") {
    const TileMap map = makeMap();
    const Enemy enemy = walkerAt(100.0F);
    // Spikes on the floor at x = 140..156 (they stand 5 px tall over the floor).
    const std::vector<Hazard> spikes = {Hazard{AABB{140.0F, kFloorTop - 5.0F, 16.0F, 5.0F}}};
    EnemyState state;
    state.direction = 1;

    for (int i = 0; i < 400; ++i) {
        stepEnemy(enemy, state, map, spikes);
    }
    CHECK(currentBounds(enemy, state).right() <= 140.0F);  // never touches them
    CHECK(state.direction == -1);
}

TEST_CASE("an enemy that starts on top of spikes still walks (it does not jitter in place)") {
    const TileMap map = makeMap();
    const Enemy enemy = walkerAt(100.0F);
    const std::vector<Hazard> spikes = {Hazard{AABB{98.0F, kFloorTop - 5.0F, 16.0F, 5.0F}}};
    EnemyState state;
    state.direction = 1;

    for (int i = 0; i < 100; ++i) {
        stepEnemy(enemy, state, map, spikes);
    }
    CHECK(state.x_offset > 10.0F);
}

TEST_CASE("a defeated enemy counts the steps it has lain squashed, then is gone") {
    const TileMap map = makeMap();
    const Enemy enemy = walkerAt(100.0F);
    EnemyState state;
    state.alive = false;

    CHECK(isSquashedBody(state));
    walk(enemy, state, map, kSquashSteps - 1);
    CHECK(isSquashedBody(state));
    walk(enemy, state, map, 1000);
    CHECK(state.dead_steps == kSquashSteps);  // it stops counting: no overflow after a very long time
    state.dead_steps = kSquashSteps - 1;
    walk(enemy, state, map, 1);
    CHECK_FALSE(isSquashedBody(state));  // one second after, it disappears
    CHECK(state.x_offset == 0.0F);

    EnemyState alive;
    CHECK_FALSE(isSquashedBody(alive));
}

TEST_CASE("isStomp at the fastest fall: the deepest penetration a step can produce still counts") {
    const AABB enemy{100.0F, kFloorTop - 8.0F, 8.0F, 8.0F};
    Player player;
    player.x = 100.0F;
    player.velocity_y = kMaxFallSpeed;

    player.y = enemy.top() - player.height + kStompDepth;  // feet exactly kStompDepth below its top
    CHECK(isStomp(player, enemy));
    player.y += 0.5F;                                       // a little deeper: it is a side hit
    CHECK_FALSE(isStomp(player, enemy));
}
