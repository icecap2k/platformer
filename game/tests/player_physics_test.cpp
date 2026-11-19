#include <doctest/doctest.h>

#include <algorithm>

#include "domain/physics/aabb.h"
#include "domain/physics/player_physics.h"
#include "test_maps.h"

using namespace domain;
using test_maps::kFloorTop;
using test_maps::makeMap;

namespace {

void run(Player& player, const InputState& input, const TileMap& map, int steps) {
    for (int i = 0; i < steps; ++i) {
        stepPlayer(player, input, map);
    }
}

Player standingAt(float x, float feet_y) {
    Player player;
    player.x = x;
    player.y = feet_y - player.height;
    player.on_ground = true;
    return player;
}

}  // namespace

TEST_CASE("AABB overlap") {
    const AABB box{0.0F, 0.0F, 10.0F, 10.0F};

    CHECK(box.overlaps(AABB{5.0F, 5.0F, 10.0F, 10.0F}));
    CHECK_FALSE(box.overlaps(AABB{20.0F, 0.0F, 10.0F, 10.0F}));
    CHECK_FALSE(box.overlaps(AABB{10.0F, 0.0F, 10.0F, 10.0F}));  // touching edges only
}

TEST_CASE("player physics on flat ground") {
    const TileMap map = makeMap();
    const InputState none;

    SUBCASE("a player in the air falls and lands on the floor") {
        Player player;
        player.x = 100.0F;
        player.y = 50.0F;

        run(player, none, map, 200);

        CHECK(player.on_ground);
        CHECK(player.y + player.height == doctest::Approx(kFloorTop));
        CHECK(player.velocity_y == 0.0F);
    }

    SUBCASE("falling speed is capped") {
        Player player;
        player.y = -10000.0F;

        run(player, none, map, 500);

        CHECK(player.velocity_y <= kMaxFallSpeed);
    }

    SUBCASE("running right moves at run speed") {
        Player player = standingAt(100.0F, kFloorTop);
        InputState input;
        input.right = true;

        run(player, input, map, 10);

        CHECK(player.x == doctest::Approx(100.0F + 10 * kRunSpeed));
        CHECK(player.on_ground);
    }

    SUBCASE("running left moves at run speed") {
        Player player = standingAt(100.0F, kFloorTop);
        InputState input;
        input.left = true;

        run(player, input, map, 10);

        CHECK(player.x == doctest::Approx(100.0F - 10 * kRunSpeed));
    }

    SUBCASE("physics alone does not stop the player at the map edge") {
        // Edges are a room-graph rule, tested in room_transition_test.cpp.
        Player player = standingAt(300.0F, kFloorTop);
        InputState input;
        input.right = true;
        run(player, input, map, 100);
        CHECK(player.x > map.pixelWidth());
    }

    SUBCASE("jumping leaves the floor and comes back down") {
        Player player = standingAt(100.0F, kFloorTop);
        const float floor_y = player.y;
        InputState jump;
        jump.jump = true;

        stepPlayer(player, jump, map);
        CHECK_FALSE(player.on_ground);
        CHECK(player.y < floor_y);

        run(player, none, map, 200);
        CHECK(player.on_ground);
        CHECK(player.y == doctest::Approx(floor_y));
    }

    SUBCASE("jump apex is above the floor by the expected amount") {
        Player player = standingAt(100.0F, kFloorTop);
        const float floor_y = player.y;
        InputState jump;
        jump.jump = true;
        stepPlayer(player, jump, map);

        float highest = player.y;
        for (int i = 0; i < 200; ++i) {
            stepPlayer(player, jump, map);  // button held: full-height jump
            highest = std::min(highest, player.y);
        }

        // v^2 / (2g) (about 44 px), give or take the discrete integration.
        const float expected = kJumpSpeed * kJumpSpeed / (2.0F * kGravity);
        CHECK(floor_y - highest == doctest::Approx(expected).epsilon(0.1));
        CHECK(floor_y - highest < 48.0F);  // the jump was lowered to 70% of the original 62 px
    }

    SUBCASE("cannot jump while already in the air") {
        Player player;
        player.x = 100.0F;
        player.y = 50.0F;
        player.on_ground = false;
        InputState jump;
        jump.jump = true;

        stepPlayer(player, jump, map);

        // Only gravity acted on it: no upward kick.
        CHECK(player.velocity_y == doctest::Approx(kGravity));
    }

    SUBCASE("releasing the button early gives a shorter jump") {
        Player player = standingAt(100.0F, kFloorTop);
        const float floor_y = player.y;
        InputState jump;
        jump.jump = true;
        stepPlayer(player, jump, map);

        float highest = player.y;
        for (int i = 0; i < 200; ++i) {
            stepPlayer(player, none, map);  // released right after the first step
            highest = std::min(highest, player.y);
        }

        const float height = floor_y - highest;
        CHECK(height > 0.0F);
        CHECK(height < 12.0F);  // far below the full jump (about 44 px)
    }

    SUBCASE("holding the button does not bounce: a new press is needed") {
        Player player = standingAt(100.0F, kFloorTop);
        InputState jump;
        jump.jump = true;

        int takeoffs = 0;
        bool was_grounded = true;
        for (int i = 0; i < 300; ++i) {
            stepPlayer(player, jump, map);
            if (was_grounded && !player.on_ground) {
                ++takeoffs;
            }
            was_grounded = player.on_ground;
        }
        CHECK(takeoffs == 1);

        // Release for a step, press again: the second jump works.
        stepPlayer(player, none, map);
        stepPlayer(player, jump, map);
        CHECK(player.velocity_y < 0.0F);
    }

    SUBCASE("coyote time lasts exactly kCoyoteSteps airborne steps after a real ledge") {
        // Platform edge at x = 144. Walk off it, wait `idle` steps, then press jump.
        std::vector<std::pair<int, int>> platform;
        for (int column = 10; column <= 17; ++column) {
            platform.emplace_back(column, 20);
        }
        const TileMap ledge_map = makeMap(platform);

        const auto jumpsAfterIdleSteps = [&](int idle) {
            Player walker = standingAt(130.0F, 160.0F);
            InputState right;
            right.right = true;
            for (int i = 0; i < 100 && (walker.on_ground || i == 0); ++i) {
                stepPlayer(walker, right, ledge_map);
            }
            REQUIRE_FALSE(walker.on_ground);  // it has really left the platform
            for (int i = 0; i < idle; ++i) {
                stepPlayer(walker, none, ledge_map);
            }
            InputState jump;
            jump.jump = true;
            const float vy_before = walker.velocity_y;
            stepPlayer(walker, jump, ledge_map);
            return walker.velocity_y < vy_before;  // kicked upwards
        };

        CHECK(jumpsAfterIdleSteps(0));
        CHECK(jumpsAfterIdleSteps(kCoyoteSteps - 1));  // last airborne step that still jumps
        CHECK_FALSE(jumpsAfterIdleSteps(kCoyoteSteps));
    }

    SUBCASE("no double jump with coyote time") {
        Player player = standingAt(100.0F, kFloorTop);
        InputState jump;
        jump.jump = true;
        stepPlayer(player, jump, map);  // takes off
        stepPlayer(player, none, map);  // release
        const float vy = player.velocity_y;
        stepPlayer(player, jump, map);  // press again in the air
        CHECK(player.velocity_y == doctest::Approx(vy + kGravity));
    }
}

TEST_CASE("player physics against tiles") {
    const InputState none;

    SUBCASE("lands on top of a floating platform") {
        // Platform: columns 10..17 at row 20 (top edge at y = 160).
        std::vector<std::pair<int, int>> platform;
        for (int column = 10; column <= 17; ++column) {
            platform.emplace_back(column, 20);
        }
        const TileMap map = makeMap(platform);

        Player player;
        player.x = 100.0F;  // above the platform
        player.y = 50.0F;
        run(player, none, map, 200);

        CHECK(player.on_ground);
        CHECK(player.y + player.height == doctest::Approx(160.0F));
    }

    SUBCASE("walks off a platform and falls to the floor") {
        std::vector<std::pair<int, int>> platform;
        for (int column = 10; column <= 17; ++column) {
            platform.emplace_back(column, 20);
        }
        const TileMap map = makeMap(platform);

        Player player = standingAt(130.0F, 160.0F);
        InputState right;
        right.right = true;
        run(player, right, map, 60);  // walk past the platform's right edge (x = 144)
        run(player, none, map, 200);

        CHECK(player.on_ground);
        CHECK(player.y + player.height == doctest::Approx(kFloorTop));
    }

    SUBCASE("a wall blocks horizontal movement") {
        // A 3-tile-high wall at column 20, standing on the floor.
        const TileMap map = makeMap({{20, 26}, {20, 25}, {20, 24}});

        Player player = standingAt(100.0F, kFloorTop);
        InputState right;
        right.right = true;
        run(player, right, map, 200);

        CHECK(player.x + player.width == doctest::Approx(160.0F));  // wall's left face
        CHECK(player.on_ground);
    }

    SUBCASE("a wall blocks movement when approached from the right") {
        const TileMap map = makeMap({{20, 26}, {20, 25}, {20, 24}});

        Player player = standingAt(250.0F, kFloorTop);
        InputState left;
        left.left = true;
        run(player, left, map, 200);

        CHECK(player.x == doctest::Approx(168.0F));  // wall's right face
    }

    SUBCASE("bumping the head on a ceiling stops the rise") {
        // Low ceiling: row 23 sits 24 px above the floor, a full jump would reach about 44 px.
        std::vector<std::pair<int, int>> ceiling;
        for (int column = 10; column <= 20; ++column) {
            ceiling.emplace_back(column, 23);
        }
        const TileMap map = makeMap(ceiling);

        Player player = standingAt(100.0F, kFloorTop);  // x = 100 is under the ceiling
        InputState jump;
        jump.jump = true;
        stepPlayer(player, jump, map);

        float highest = player.y;
        for (int i = 0; i < 100; ++i) {
            stepPlayer(player, jump, map);
            highest = std::min(highest, player.y);
        }

        CHECK(highest >= 24.0F * 8.0F - 0.001F);  // never above the ceiling's bottom face (y = 192)
        CHECK(player.on_ground);                  // and it comes back down
    }

    SUBCASE("a jump from below a platform does not pass through it") {
        std::vector<std::pair<int, int>> platform;
        for (int column = 10; column <= 17; ++column) {
            platform.emplace_back(column, 22);  // bottom face at y = 184
        }
        const TileMap map = makeMap(platform);

        Player player = standingAt(100.0F, kFloorTop);
        InputState jump;
        jump.jump = true;
        stepPlayer(player, jump, map);

        float highest = player.y;
        for (int i = 0; i < 100; ++i) {
            stepPlayer(player, jump, map);
            highest = std::min(highest, player.y);
        }

        CHECK(highest >= 184.0F - 0.001F);
    }
}
