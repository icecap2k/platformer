#include <doctest/doctest.h>

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

}  // namespace

TEST_CASE("applyDamage") {
    Player player = standingAt(100.0F);

    SUBCASE("costs hearts, starts invulnerability and a small hop away") {
        REQUIRE(canBeHurt(player));
        applyDamage(player, 1, -1);

        CHECK(player.health == kMaxHealth - 1);
        CHECK(player.invulnerable_steps == kInvulnerableSteps);
        CHECK(player.hurt_steps == kHurtSteps);
        CHECK(player.hurt_direction == -1);
        CHECK(player.velocity_y < 0.0F);
        CHECK(player.velocity_y > -kJumpSpeed / 2.0F);  // much lower than a jump
        CHECK_FALSE(canBeHurt(player));                 // protected for a while
    }

    SUBCASE("health never goes below zero") {
        applyDamage(player, 5, 1);
        CHECK(player.health == 0);
        CHECK_FALSE(canBeHurt(player));
    }

    SUBCASE("any direction is normalised to -1 or +1") {
        applyDamage(player, 1, 7);
        CHECK(player.hurt_direction == 1);
        applyDamage(player, 1, -3);
        CHECK(player.hurt_direction == -1);
    }
}

TEST_CASE("knockbackDirection throws the player away from the source") {
    const Player player = standingAt(100.0F);  // centre x = 104

    CHECK(knockbackDirection(player, AABB{120.0F, 200.0F, 8.0F, 8.0F}) == -1);  // hazard on the right
    CHECK(knockbackDirection(player, AABB{80.0F, 200.0F, 8.0F, 8.0F}) == 1);    // hazard on the left

    SUBCASE("exactly aligned: against the way it was moving") {
        Player moving = player;
        const AABB below{100.0F, 200.0F, 8.0F, 8.0F};  // same centre
        moving.velocity_x = 1.5F;
        CHECK(knockbackDirection(moving, below) == -1);
        moving.velocity_x = -1.5F;
        CHECK(knockbackDirection(moving, below) == 1);
    }
}

TEST_CASE("hazardTouching") {
    const Room room("r", makeMap(), {}, {}, {Hazard{AABB{100.0F, 208.0F, 8.0F, 8.0F}}});

    CHECK(hazardTouching(standingAt(100.0F), room).has_value());
    CHECK_FALSE(hazardTouching(standingAt(200.0F), room).has_value());

    SUBCASE("a room without hazards never hurts") {
        const Room empty("e", makeMap());
        CHECK_FALSE(hazardTouching(standingAt(100.0F), empty).has_value());
    }
}

TEST_CASE("being hurt in the physics") {
    const TileMap map = makeMap();
    const InputState none;
    InputState steer_left;
    steer_left.left = true;
    InputState jump;
    jump.jump = true;

    SUBCASE("the knockback carries the player back a short distance, with a small hop") {
        Player player = standingAt(100.0F);
        const float floor_y = player.y;
        applyDamage(player, 1, 1);

        float highest = player.y;
        for (int i = 0; i < kHurtSteps; ++i) {
            stepPlayer(player, none, map);
            highest = std::min(highest, player.y);
        }
        const float thrown = player.x - 100.0F;
        CHECK(thrown > 5.0F);    // it does go back...
        CHECK(thrown < 12.0F);   // ...but only a little (about 8 px)
        const float hop = floor_y - highest;
        CHECK(hop > 4.0F);   // it does leave the ground...
        CHECK(hop < 10.0F);  // ...but only a few pixels, about 7: far below the 44 px jump
    }

    SUBCASE("the input is ignored while thrown back") {
        Player player = standingAt(100.0F);
        applyDamage(player, 1, 1);  // thrown to the right
        for (int i = 0; i < kHurtSteps; ++i) {
            stepPlayer(player, steer_left, map);  // trying to go left
        }
        CHECK(player.x > 100.0F);
    }

    SUBCASE("it cannot jump while thrown back, but can steer afterwards") {
        Player player = standingAt(100.0F);
        applyDamage(player, 1, 1);
        stepPlayer(player, jump, map);
        CHECK(player.velocity_y > -kJumpSpeed / 2.0F);  // no jump kick on top of the hop

        for (int i = 0; i < 200; ++i) {
            stepPlayer(player, none, map);
        }
        REQUIRE(player.on_ground);
        stepPlayer(player, steer_left, map);
        CHECK(player.velocity_x < 0.0F);  // control is back
    }

    SUBCASE("the invulnerability wears off") {
        Player player = standingAt(100.0F);
        applyDamage(player, 1, 1);
        for (int i = 0; i < kInvulnerableSteps - 1; ++i) {
            stepPlayer(player, none, map);
        }
        CHECK_FALSE(canBeHurt(player));
        stepPlayer(player, none, map);
        CHECK(canBeHurt(player));
    }
}
