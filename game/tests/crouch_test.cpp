#include <doctest/doctest.h>

#include <vector>

#include "domain/physics/player_physics.h"
#include "domain/rules/damage.h"
#include "test_maps.h"

using namespace domain;
using test_maps::kFloorTop;
using test_maps::makeMap;

namespace {

Player standingAt(float x) {
    Player player;
    player.x = x;
    player.y = kFloorTop - player.height;
    player.on_ground = true;
    return player;
}

InputState pressing(bool down, int direction = 0, bool jump = false) {
    InputState input;
    input.down = down;
    input.left = direction < 0;
    input.right = direction > 0;
    input.jump = jump;
    return input;
}

float bottomOf(const Player& player) { return player.y + player.height; }

// A ceiling whose underside is 8 px above the floor: a crouching player (8 px
// tall) fits under it exactly, a standing one (16 px) does not.
TileMap makeLowTunnel() {
    std::vector<std::pair<int, int>> ceiling;
    for (int column = 15; column <= 24; ++column) {
        ceiling.emplace_back(column, 25);  // row 25 spans y = 200..208
    }
    return makeMap(ceiling);
}

}  // namespace

TEST_CASE("crouching on firm ground") {
    const TileMap map = makeMap();

    SUBCASE("down halves the box and keeps the feet where they are") {
        Player player = standingAt(100.0F);
        const float feet = bottomOf(player);

        stepPlayer(player, pressing(true), map);

        CHECK(player.crouching);
        CHECK(player.height == kCrouchHeight);
        CHECK(player.height == kStandingHeight / 2.0F);
        CHECK(bottomOf(player) == doctest::Approx(feet));
        CHECK(player.on_ground);  // it does not hop or fall
        CHECK(player.bounds().height == kCrouchHeight);
    }

    SUBCASE("letting go of down stands up again, feet still in place") {
        Player player = standingAt(100.0F);
        const float feet = bottomOf(player);
        stepPlayer(player, pressing(true), map);
        stepPlayer(player, pressing(false), map);

        CHECK_FALSE(player.crouching);
        CHECK(player.height == kStandingHeight);
        CHECK(bottomOf(player) == doctest::Approx(feet));
        CHECK(player.on_ground);
    }

    SUBCASE("it stays crouched while the key is held, for as long as you like") {
        Player player = standingAt(100.0F);
        for (int step = 0; step < 100; ++step) {
            stepPlayer(player, pressing(true), map);
        }
        CHECK(player.crouching);
        CHECK(bottomOf(player) == doctest::Approx(kFloorTop));
    }

    SUBCASE("crouch-walking is slow") {
        Player player = standingAt(100.0F);
        stepPlayer(player, pressing(true), map);
        const float x = player.x;
        stepPlayer(player, pressing(true, 1), map);

        CHECK(player.velocity_x == doctest::Approx(kCrouchSpeed));
        CHECK(player.x - x == doctest::Approx(kCrouchSpeed));
        CHECK(kCrouchSpeed < kRunSpeed);

        stepPlayer(player, pressing(true, -1), map);
        CHECK(player.velocity_x == doctest::Approx(-kCrouchSpeed));
    }

    SUBCASE("it cannot jump while crouched") {
        Player player = standingAt(100.0F);
        stepPlayer(player, pressing(true), map);
        stepPlayer(player, pressing(true, 0, true), map);

        CHECK(player.on_ground);
        CHECK(player.velocity_y >= 0.0F);
        CHECK(bottomOf(player) == doctest::Approx(kFloorTop));
    }

    SUBCASE("standing up and jumping works as before") {
        Player player = standingAt(100.0F);
        stepPlayer(player, pressing(true), map);
        stepPlayer(player, pressing(false), map);  // stands
        stepPlayer(player, pressing(false, 0, true), map);

        CHECK_FALSE(player.on_ground);
        CHECK(player.velocity_y < 0.0F);
    }
}

TEST_CASE("pressing down in the air does not crouch") {
    const TileMap map = makeMap();
    Player player;
    player.x = 100.0F;
    player.y = 50.0F;
    stepPlayer(player, pressing(true), map);

    CHECK_FALSE(player.crouching);
    CHECK(player.height == kStandingHeight);
}

TEST_CASE("a crouching player walking off a ledge stands up in the air") {
    // A platform at row 22 (top y = 176) from column 10 to 17 (x = 80..144).
    std::vector<std::pair<int, int>> platform;
    for (int column = 10; column <= 17; ++column) {
        platform.emplace_back(column, 22);
    }
    const TileMap map = makeMap(platform);

    Player player;
    player.x = 130.0F;
    player.y = 176.0F - player.height;
    player.on_ground = true;
    stepPlayer(player, pressing(true), map);
    REQUIRE(player.crouching);

    for (int step = 0; step < 60 && player.on_ground; ++step) {
        stepPlayer(player, pressing(true, 1), map);  // crawl off the edge, still holding down
    }
    REQUIRE_FALSE(player.on_ground);
    stepPlayer(player, pressing(true, 1), map);
    CHECK_FALSE(player.crouching);  // in the air it stands (down does not crouch there)
    CHECK(player.height == kStandingHeight);
}

TEST_CASE("a low passage forces the crouch") {
    const TileMap map = makeLowTunnel();

    SUBCASE("a standing player cannot walk into it") {
        Player player = standingAt(100.0F);
        for (int step = 0; step < 200; ++step) {
            stepPlayer(player, pressing(false, 1), map);
        }
        CHECK(player.x + player.width <= 15.0F * 8.0F + 0.01F);  // stopped by the tunnel's mouth
    }

    SUBCASE("a crouching player walks under it") {
        Player player = standingAt(100.0F);
        stepPlayer(player, pressing(true), map);
        for (int step = 0; step < 400 && player.x < 200.0F; ++step) {
            stepPlayer(player, pressing(true, 1), map);
        }
        CHECK(player.x >= 200.0F);  // it went through the whole tunnel (x = 120..200)
        CHECK(player.crouching);
    }

    SUBCASE("letting go of down under it does not stand up: there is no room") {
        Player player = standingAt(100.0F);
        stepPlayer(player, pressing(true), map);
        while (player.x < 140.0F) {
            stepPlayer(player, pressing(true, 1), map);  // well inside the tunnel
        }
        for (int step = 0; step < 20; ++step) {
            stepPlayer(player, pressing(false), map);
        }
        CHECK(player.crouching);
        CHECK(player.height == kCrouchHeight);
        CHECK(bottomOf(player) == doctest::Approx(kFloorTop));
    }

    SUBCASE("it can still walk out while forced, and then it stands up") {
        Player player = standingAt(100.0F);
        stepPlayer(player, pressing(true), map);
        while (player.x < 140.0F) {
            stepPlayer(player, pressing(true, 1), map);
        }
        for (int step = 0; step < 300 && player.x < 200.0F; ++step) {
            stepPlayer(player, pressing(false, 1), map);  // down released, still crawling out
        }
        REQUIRE(player.x >= 200.0F);
        stepPlayer(player, pressing(false, 1), map);  // it decides at the start of the next step
        stepPlayer(player, pressing(false, 1), map);
        CHECK(player.crouching == false);  // clear of the ceiling: it stood up
        CHECK(player.height == kStandingHeight);
    }
}

TEST_CASE("a hit makes a crouching player stand up") {
    const TileMap map = makeMap();
    Player player = standingAt(100.0F);
    stepPlayer(player, pressing(true), map);
    REQUIRE(player.crouching);

    applyDamage(player, 1, 1);
    stepPlayer(player, pressing(true), map);  // still holding down

    CHECK_FALSE(player.crouching);
    CHECK(player.height == kStandingHeight);
}

TEST_CASE("a crouching player is a smaller target") {
    Player player;
    player.x = 100.0F;
    player.y = 100.0F;
    const AABB standing = player.bounds();
    player.height = kCrouchHeight;
    player.crouching = true;
    player.y += kStandingHeight - kCrouchHeight;  // the feet stay put

    CHECK(player.bounds().height == standing.height / 2.0F);
    CHECK(player.bounds().bottom() == doctest::Approx(standing.bottom()));
    // Something hanging at head height no longer touches it.
    const AABB at_head{100.0F, standing.top(), 8.0F, 4.0F};
    CHECK_FALSE(player.bounds().overlaps(at_head));
}
