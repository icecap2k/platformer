#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <vector>

#include "domain/physics/player_physics.h"
#include "domain/rules/damage.h"
#include "test_maps.h"

using namespace domain;
using test_maps::kFloorTop;

namespace {

constexpr int kRampBase = 10;  // first ramp tile column
constexpr int kRampTiles = 5;  // it rises 5 * 8 = 40 px, like the one in room_01
constexpr float kPlateauTop = kFloorTop - 40.0F;

// The ramp of room_01 in miniature: a floor, a ramp of 5 tiles rising to the
// right with its filler blocks (below the ramp and to its high side), and a
// plateau that continues to the right. With `rises_right == false` the whole
// scene is mirrored.
TileMap makeRampMap(bool rises_right, bool wall_on_plateau = false) {
    constexpr int kWidth = test_maps::kWidth;
    constexpr int kHeight = test_maps::kHeight;
    std::vector<int> tiles(static_cast<std::size_t>(kWidth * kHeight), 0);
    auto set = [&](int column, int row, int id) { tiles[static_cast<std::size_t>(row * kWidth + column)] = id; };

    for (int row = test_maps::kFloorRow; row < kHeight; ++row) {
        for (int column = 0; column < kWidth; ++column) {
            set(column, row, 1);
        }
    }
    for (int i = 0; i < kRampTiles; ++i) {
        const int column = kRampBase + i;
        const int ramp_row = test_maps::kFloorRow - 1 - i;
        set(column, ramp_row, kTileSlopeRight);
        for (int row = ramp_row + 1; row < test_maps::kFloorRow; ++row) {
            set(column, row, 2);  // filler under the ramp
        }
    }
    for (int column = kRampBase + kRampTiles; column < kRampBase + kRampTiles + 8; ++column) {
        for (int row = test_maps::kFloorRow - kRampTiles; row < test_maps::kFloorRow; ++row) {
            set(column, row, 2);  // the plateau and its filler
        }
    }
    if (wall_on_plateau) {
        for (int row = test_maps::kFloorRow - kRampTiles - 4; row < test_maps::kFloorRow - kRampTiles; ++row) {
            set(kRampBase + kRampTiles + 5, row, 1);
        }
    }

    if (!rises_right) {
        std::vector<int> mirrored(tiles.size());
        for (int row = 0; row < kHeight; ++row) {
            for (int column = 0; column < kWidth; ++column) {
                int id = tiles[static_cast<std::size_t>(row * kWidth + column)];
                id = id == kTileSlopeRight ? kTileSlopeLeft : id;
                mirrored[static_cast<std::size_t>(row * kWidth + (kWidth - 1 - column))] = id;
            }
        }
        tiles = mirrored;
    }
    return TileMap(kWidth, kHeight, test_maps::kTileSize, tiles);
}

// Pixel x of the ramp's low end and of its high end (where the plateau starts).
float lowEndX(bool rises_right) {
    const float left = static_cast<float>(kRampBase * 8);
    return rises_right ? left : test_maps::kWidth * 8.0F - left;
}
float highEndX(bool rises_right) {
    const float right = static_cast<float>((kRampBase + kRampTiles) * 8);
    return rises_right ? right : test_maps::kWidth * 8.0F - right;
}
int uphill(bool rises_right) { return rises_right ? 1 : -1; }

Player standingOnFloorAt(float centre_x) {
    Player player;
    player.x = centre_x - player.width * 0.5F;
    player.y = kFloorTop - player.height;
    player.on_ground = true;
    return player;
}

Player standingOnPlateauAt(float centre_x) {
    Player player = standingOnFloorAt(centre_x);
    player.y = kPlateauTop - player.height;
    return player;
}

InputState walking(int direction) {
    InputState input;
    input.left = direction < 0;
    input.right = direction > 0;
    return input;
}

// Has the centre of the player gone `margin` px past the ramp's low end, onto the floor?
// (The tests stop there: the game itself keeps the player inside the room.)
bool pastLowEnd(const Player& player, bool rises_right, float margin) {
    const float distance = (lowEndX(rises_right) - (player.x + player.width * 0.5F)) *
                           static_cast<float>(uphill(rises_right));
    return distance > margin;
}

float feetX(const Player& player) { return player.x + player.width * 0.5F; }
float bottomOf(const Player& player) { return player.y + player.height; }

// Height of the ground (ramp surface, or plateau/floor top) under the player's centre.
float groundUnder(const TileMap& map, const Player& player) {
    const int column = static_cast<int>(std::floor(feetX(player) / 8.0F));
    for (int row = 0; row < map.height(); ++row) {
        if (const auto surface = map.slopeSurfaceY(column, row, feetX(player))) {
            if (*surface <= bottomOf(player) + 8.0F && *surface >= bottomOf(player) - 8.0F) {
                return *surface;
            }
        }
    }
    for (int row = 0; row < map.height(); ++row) {
        if (map.isSolid(column, row)) {
            return static_cast<float>(row * 8);
        }
    }
    return kFloorTop;
}

}  // namespace

TEST_CASE("walking up a ramp") {
    for (const bool rises_right : {true, false}) {
        INFO("ramp rises to the " << (rises_right ? "right" : "left"));
        const TileMap map = makeRampMap(rises_right);
        const int up = uphill(rises_right);
        Player player = standingOnFloorAt(lowEndX(rises_right) - up * 30.0F);

        float previous_bottom = bottomOf(player);
        int airborne = 0;
        bool reached_top = false;
        for (int step = 0; step < 200 && !reached_top; ++step) {
            stepPlayer(player, walking(up), map);
            airborne += player.on_ground ? 0 : 1;
            CHECK(bottomOf(player) <= previous_bottom + 0.01F);  // never sinks while going up
            previous_bottom = bottomOf(player);
            reached_top = std::abs(feetX(player) - highEndX(rises_right)) > 6.0F &&
                          (feetX(player) - highEndX(rises_right)) * static_cast<float>(up) > 6.0F;
        }
        CHECK(airborne == 0);                       // it walks up, it does not hop
        CHECK(player.on_ground);
        CHECK(bottomOf(player) == doctest::Approx(kPlateauTop));  // and ends on the plateau
        CHECK(player.slide_steps == 0);             // going up is a normal walk
    }
}

TEST_CASE("the feet follow the ramp surface") {
    const TileMap map = makeRampMap(true);
    Player player = standingOnFloorAt(lowEndX(true) - 20.0F);
    for (int step = 0; step < 80; ++step) {
        stepPlayer(player, walking(1), map);
        const float x = feetX(player);
        if (x > lowEndX(true) && x < highEndX(true)) {
            // 45 degrees: one pixel up per pixel across.
            const float expected = kFloorTop - (x - lowEndX(true));
            CHECK(bottomOf(player) == doctest::Approx(expected).epsilon(0.001));
        }
    }
}

TEST_CASE("standing still on a ramp: it stays where it is") {
    for (const bool rises_right : {true, false}) {
        INFO("ramp rises to the " << (rises_right ? "right" : "left"));
        const TileMap map = makeRampMap(rises_right);
        Player player = standingOnFloorAt(lowEndX(rises_right) - uphill(rises_right) * 10.0F);
        for (int step = 0; step < 100; ++step) {
            stepPlayer(player, walking(uphill(rises_right)), map);
            if (std::abs(feetX(player) - (lowEndX(rises_right) + uphill(rises_right) * 20.0F)) < 1.0F) {
                break;  // halfway up the ramp
            }
        }
        REQUIRE(player.on_ground);
        const float x = player.x;
        const float y = player.y;

        for (int step = 0; step < 100; ++step) {
            stepPlayer(player, InputState{}, map);
        }
        CHECK(player.x == doctest::Approx(x));
        CHECK(player.y == doctest::Approx(y));
        CHECK(player.on_ground);
        CHECK(player.slide_steps == 0);
    }
}

TEST_CASE("falling onto a ramp lands on its surface") {
    for (const bool rises_right : {true, false}) {
        INFO("ramp rises to the " << (rises_right ? "right" : "left"));
        const TileMap map = makeRampMap(rises_right);
        Player player;
        player.x = lowEndX(rises_right) + uphill(rises_right) * 20.0F - player.width * 0.5F;
        player.y = 100.0F;
        for (int step = 0; step < 100; ++step) {
            stepPlayer(player, InputState{}, map);
        }
        CHECK(player.on_ground);
        const float expected = kFloorTop - 20.0F;
        CHECK(bottomOf(player) == doctest::Approx(expected));
    }
}

TEST_CASE("jumping from a ramp") {
    const TileMap map = makeRampMap(true);
    Player player = standingOnFloorAt(lowEndX(true) - 10.0F);
    for (int step = 0; step < 100 && feetX(player) < lowEndX(true) + 20.0F; ++step) {
        stepPlayer(player, walking(1), map);
    }
    REQUIRE(player.on_ground);

    InputState jump;
    jump.jump = true;
    stepPlayer(player, jump, map);
    CHECK_FALSE(player.on_ground);
    CHECK(player.velocity_y < 0.0F);

    // It comes back down onto the ramp or the plateau.
    for (int step = 0; step < 200; ++step) {
        stepPlayer(player, InputState{}, map);
    }
    CHECK(player.on_ground);
}

namespace {

// Puts the player on the ramp, a few pixels past its high end, by walking down from the plateau
// (walking is a normal walk: it does not slide).
Player standingOnRamp(const TileMap& map, bool rises_right, float pixels_into_ramp = 10.0F) {
    Player player = standingOnPlateauAt(highEndX(rises_right) + uphill(rises_right) * 12.0F);
    for (int step = 0; step < 100; ++step) {
        const float into_ramp = (highEndX(rises_right) - feetX(player)) * static_cast<float>(uphill(rises_right));
        if (into_ramp >= pixels_into_ramp) {
            break;
        }
        stepPlayer(player, walking(-uphill(rises_right)), map);
    }
    REQUIRE(player.on_ground);
    return player;
}

InputState pressingDown() {
    InputState input;
    input.down = true;
    return input;
}

}  // namespace

TEST_CASE("walking down a ramp is a normal walk: it does not slide") {
    for (const bool rises_right : {true, false}) {
        INFO("ramp rises to the " << (rises_right ? "right" : "left"));
        const TileMap map = makeRampMap(rises_right);
        const int down = -uphill(rises_right);
        Player player = standingOnPlateauAt(highEndX(rises_right) + uphill(rises_right) * 12.0F);

        int airborne = 0;
        bool saw_ramp = false;
        for (int step = 0; step < 200 && !pastLowEnd(player, rises_right, 20.0F); ++step) {
            stepPlayer(player, walking(down), map);
            const float x = feetX(player);
            const bool over_ramp = (x - lowEndX(rises_right)) * static_cast<float>(uphill(rises_right)) > 1.0F &&
                                   (highEndX(rises_right) - x) * static_cast<float>(uphill(rises_right)) > 1.0F;
            if (over_ramp) {
                saw_ramp = true;
                airborne += player.on_ground ? 0 : 1;
                CHECK(std::abs(player.velocity_x) == doctest::Approx(kRunSpeed));  // the usual speed
                CHECK(player.slide_steps == 0);
            }
        }
        CHECK(saw_ramp);
        CHECK(airborne == 0);                              // the feet still follow the surface
        CHECK(bottomOf(player) == doctest::Approx(kFloorTop));
    }
}

TEST_CASE("pressing down on a ramp slides down it") {
    for (const bool rises_right : {true, false}) {
        INFO("ramp rises to the " << (rises_right ? "right" : "left"));
        const TileMap map = makeRampMap(rises_right);
        const int down = -uphill(rises_right);
        Player player = standingOnRamp(map, rises_right);

        int airborne = 0;
        bool slid = false;
        bool at_slide_speed = false;
        for (int step = 0; step < 120 && !pastLowEnd(player, rises_right, 20.0F); ++step) {
            stepPlayer(player, pressingDown(), map);
            const float x = feetX(player);
            const bool over_ramp = (x - lowEndX(rises_right)) * static_cast<float>(uphill(rises_right)) > 1.0F;
            if (over_ramp) {
                airborne += player.on_ground ? 0 : 1;
                slid = slid || player.slide_steps == kSlideCarrySteps;
                at_slide_speed = at_slide_speed || player.velocity_x * static_cast<float>(down) == doctest::Approx(kSlideSpeed);
            }
        }
        CHECK(slid);
        CHECK(at_slide_speed);
        CHECK(airborne == 0);                              // it follows the surface down
        CHECK(player.on_ground);
        CHECK(bottomOf(player) <= kFloorTop + 0.01F);      // never sinks into the floor
        CHECK(bottomOf(player) >= kFloorTop - 0.01F);      // and ends standing on it
    }

    SUBCASE("down slides down the slope whichever way the arrows point") {
        const TileMap map = makeRampMap(true);
        for (const int arrow : {-1, 0, 1}) {
            INFO("arrow " << arrow);
            Player player = standingOnRamp(map, true, 20.0F);
            InputState input = walking(arrow);
            input.down = true;
            stepPlayer(player, input, map);
            CHECK(player.velocity_x == doctest::Approx(-kSlideSpeed));  // always down the slope (left here)
            CHECK(player.slide_steps == kSlideCarrySteps);
        }
    }

    SUBCASE("without pressing down it just stands there") {
        const TileMap map = makeRampMap(true);
        Player player = standingOnRamp(map, true, 20.0F);
        const float x = player.x;
        for (int step = 0; step < 60; ++step) {
            stepPlayer(player, InputState{}, map);
        }
        CHECK(player.x == doctest::Approx(x));
        CHECK(player.slide_steps == 0);
    }

    SUBCASE("down on firm ground is a crouch, not a slide") {
        const TileMap map = makeRampMap(true);
        Player player = standingOnPlateauAt(highEndX(true) + 30.0F);
        stepPlayer(player, pressingDown(), map);
        CHECK(player.crouching);
        CHECK(player.slide_steps == 0);
    }
}

TEST_CASE("stepping off the foot of a ramp onto the floor never leaves the ground") {
    for (const bool rises_right : {true, false}) {
        INFO("ramp rises to the " << (rises_right ? "right" : "left"));
        const TileMap map = makeRampMap(rises_right);

        SUBCASE("walking") {
            Player player = standingOnRamp(map, rises_right, 20.0F);
            int airborne = 0;
            for (int step = 0; step < 80 && !pastLowEnd(player, rises_right, 24.0F); ++step) {
                stepPlayer(player, walking(-uphill(rises_right)), map);
                airborne += player.on_ground ? 0 : 1;
            }
            CHECK(airborne == 0);
            CHECK(bottomOf(player) == doctest::Approx(kFloorTop));
        }

        SUBCASE("sliding at full speed") {
            Player player = standingOnRamp(map, rises_right, 14.0F);
            int airborne = 0;
            for (int step = 0; step < 80 && !pastLowEnd(player, rises_right, 24.0F); ++step) {
                stepPlayer(player, pressingDown(), map);
                airborne += player.on_ground ? 0 : 1;
            }
            CHECK(airborne == 0);  // no flicker of the jump pose at the end of the slide
            CHECK(bottomOf(player) == doctest::Approx(kFloorTop));
        }
    }
}

TEST_CASE("sliding keeps its speed for a while after the ramp, then fades") {
    const TileMap map = makeRampMap(true);
    Player player = standingOnRamp(map, true);
    // Slide down to the floor with down held...
    while (feetX(player) > lowEndX(true) + 1.0F) {
        stepPlayer(player, pressingDown(), map);
    }
    // ...and let go: it carries on, slower each step, then stops.
    float previous_speed = std::abs(player.velocity_x);
    float travelled = 0.0F;
    int coasting_steps = 0;
    for (int step = 0; step < 60; ++step) {
        const float before = player.x;
        stepPlayer(player, InputState{}, map);
        if (player.velocity_x == 0.0F) {
            break;
        }
        ++coasting_steps;
        travelled += std::abs(player.x - before);
        CHECK(std::abs(player.velocity_x) <= previous_speed + 0.001F);  // it never speeds up
        previous_speed = std::abs(player.velocity_x);
        CHECK(player.velocity_x < 0.0F);                                 // still going the same way
    }
    CHECK(coasting_steps >= kSlideCarrySteps - 2);
    CHECK(coasting_steps <= kSlideCarrySteps);
    CHECK(travelled > 8.0F);   // it does glide...
    CHECK(travelled < 24.0F);  // ...but not far
    CHECK(player.slide_steps == 0);
}

TEST_CASE("the slide can be cancelled") {
    const TileMap map = makeRampMap(true);

    SUBCASE("pressing the opposite way after letting go of down") {
        Player player = standingOnRamp(map, true, 12.0F);
        for (int step = 0; step < 6; ++step) {
            stepPlayer(player, pressingDown(), map);
        }
        REQUIRE(player.slide_steps > 0);
        stepPlayer(player, walking(1), map);  // it was sliding to the left
        CHECK(player.slide_steps == 0);
        CHECK(player.velocity_x > 0.0F);
    }

    SUBCASE("being hurt") {
        Player player = standingOnRamp(map, true, 12.0F);
        for (int step = 0; step < 6; ++step) {
            stepPlayer(player, pressingDown(), map);
        }
        REQUIRE(player.slide_steps > 0);
        applyDamage(player, 1, 1);
        CHECK(player.slide_steps == 0);
        stepPlayer(player, pressingDown(), map);
        CHECK(player.velocity_x > 0.0F);  // thrown back, not sliding on
    }
}

TEST_CASE("a slide carries into a jump") {
    const TileMap map = makeRampMap(true);
    Player player = standingOnRamp(map, true, 12.0F);
    for (int step = 0; step < 6; ++step) {
        stepPlayer(player, pressingDown(), map);
    }
    REQUIRE(player.slide_steps > 0);

    InputState jump = pressingDown();
    jump.jump = true;
    stepPlayer(player, jump, map);
    CHECK_FALSE(player.on_ground);
    CHECK(std::abs(player.velocity_x) > kRunSpeed);  // it keeps the extra speed in the air
}

TEST_CASE("walls and ramps") {
    const TileMap map = makeRampMap(true, true);

    SUBCASE("a wall at the top of the ramp still stops the player") {
        Player player = standingOnFloorAt(lowEndX(true) - 20.0F);
        for (int step = 0; step < 300; ++step) {
            stepPlayer(player, walking(1), map);
        }
        const float wall_x = static_cast<float>((kRampBase + kRampTiles + 5) * 8);
        CHECK(player.x + player.width <= wall_x + 0.01F);
        CHECK(player.x + player.width > wall_x - 3.0F);  // it did get to the wall
        CHECK(player.on_ground);
    }

    SUBCASE("sliding into a wall ends the slide") {
        // Slide left down the ramp toward a wall on the floor.
        std::vector<int> tiles(static_cast<std::size_t>(test_maps::kWidth * test_maps::kHeight), 0);
        for (int row = test_maps::kFloorRow; row < test_maps::kHeight; ++row) {
            for (int column = 0; column < test_maps::kWidth; ++column) {
                tiles[static_cast<std::size_t>(row * test_maps::kWidth + column)] = 1;
            }
        }
        for (int row = test_maps::kFloorRow - 4; row < test_maps::kFloorRow; ++row) {
            tiles[static_cast<std::size_t>(row * test_maps::kWidth + 5)] = 1;  // a wall at x = 40..48
        }
        for (int i = 0; i < kRampTiles; ++i) {
            const int column = kRampBase + i;
            const int ramp_row = test_maps::kFloorRow - 1 - i;
            tiles[static_cast<std::size_t>(ramp_row * test_maps::kWidth + column)] = kTileSlopeRight;
            for (int row = ramp_row + 1; row < test_maps::kFloorRow; ++row) {
                tiles[static_cast<std::size_t>(row * test_maps::kWidth + column)] = 2;
            }
        }
        const TileMap wall_map(test_maps::kWidth, test_maps::kHeight, 8, tiles);

        Player player = standingOnFloorAt(lowEndX(true) + 30.0F);
        player.y = kPlateauTop - player.height;  // start high on the ramp...
        player.on_ground = false;
        for (int step = 0; step < 200; ++step) {
            stepPlayer(player, pressingDown(), wall_map);
        }
        CHECK(player.x >= 48.0F - 0.01F);  // stopped by the wall
        CHECK(player.slide_steps == 0);
    }
}

TEST_CASE("sliding at full speed never sinks through the ramp or the floor") {
    const TileMap map = makeRampMap(true);
    Player player = standingOnRamp(map, true);
    for (int step = 0; step < 200 && !pastLowEnd(player, true, 20.0F); ++step) {
        stepPlayer(player, pressingDown(), map);
        CHECK(bottomOf(player) <= kFloorTop + 0.01F);
        CHECK(bottomOf(player) >= groundUnder(map, player) - 8.5F);  // not floating high above it either
    }
}
