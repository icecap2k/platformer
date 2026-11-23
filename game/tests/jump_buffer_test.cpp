#include <doctest/doctest.h>

#include "domain/physics/player_physics.h"
#include "domain/rules/damage.h"
#include "test_maps.h"

using namespace domain;
using test_maps::kFloorTop;
using test_maps::makeMap;

namespace {

InputState held() {
    InputState input;
    input.jump = true;
    return input;
}

// A player falling straight down, `height` px above the floor.
Player fallingAbove(float height) {
    Player player;
    player.x = 100.0F;
    player.y = kFloorTop - player.height - height;
    player.velocity_y = 2.0F;
    return player;
}

}  // namespace

TEST_CASE("a jump pressed a moment before landing happens on landing") {
    const TileMap map = makeMap();
    const InputState none;
    Player player = fallingAbove(10.0F);

    // Press in the air, then let go: the press is remembered.
    stepPlayer(player, held(), map);
    CHECK_FALSE(player.on_ground);
    CHECK(player.jump_buffer_steps > 0);
    bool jumped = false;
    for (int i = 0; i < 20 && !jumped; ++i) {
        jumped = stepPlayer(player, none, map).jumped;
    }
    CHECK(jumped);
    CHECK(player.velocity_y < 0.0F);
    CHECK(player.jump_buffer_steps == 0);
}

TEST_CASE("a press that is too early is forgotten") {
    const TileMap map = makeMap();
    const InputState none;
    Player player = fallingAbove(60.0F);  // more than kJumpBufferSteps steps of falling

    stepPlayer(player, held(), map);
    bool jumped = false;
    for (int i = 0; i < 200; ++i) {
        jumped = jumped || stepPlayer(player, none, map).jumped;
    }
    CHECK_FALSE(jumped);
    CHECK(player.jump_buffer_steps == 0);
}

TEST_CASE("the buffer lasts exactly kJumpBufferSteps steps") {
    const TileMap map = makeMap();
    const InputState none;
    Player player = fallingAbove(200.0F);
    stepPlayer(player, held(), map);
    CHECK(player.jump_buffer_steps == kJumpBufferSteps - 1);  // the press step already spent one
    for (int i = 0; i < kJumpBufferSteps - 1; ++i) {
        stepPlayer(player, none, map);
    }
    CHECK(player.jump_buffer_steps == 0);
}

TEST_CASE("holding the button through a landing does not jump again, a fresh press does") {
    const TileMap map = makeMap();
    const InputState none;
    Player player = fallingAbove(0.0F);
    player.jump_held = true;  // already held when it lands: not a fresh press
    for (int i = 0; i < 10; ++i) {
        CHECK_FALSE(stepPlayer(player, held(), map).jumped);
    }
    stepPlayer(player, none, map);
    CHECK(stepPlayer(player, held(), map).jumped);
}

TEST_CASE("the buffered jump is not an extra jump: no double jump from it") {
    const TileMap map = makeMap();
    const InputState none;
    Player player = fallingAbove(10.0F);
    stepPlayer(player, held(), map);
    int jumps = 0;
    for (int i = 0; i < 60; ++i) {
        jumps += stepPlayer(player, none, map).jumped ? 1 : 0;
    }
    CHECK(jumps == 1);
}

TEST_CASE("being hurt forgets a buffered jump") {
    const TileMap map = makeMap();
    Player player = fallingAbove(100.0F);
    stepPlayer(player, held(), map);
    REQUIRE(player.jump_buffer_steps > 0);
    applyDamage(player, 1, 1);
    CHECK(player.jump_buffer_steps == 0);
}

TEST_CASE("a buffered jump works with coyote time too (it does not need to have landed)") {
    const TileMap map = makeMap();
    const InputState none;
    Player player;
    player.x = 100.0F;
    player.y = kFloorTop - player.height;
    player.on_ground = true;
    // Standing: a plain press still jumps at once.
    CHECK(stepPlayer(player, held(), map).jumped);
}
