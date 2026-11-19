#include <doctest/doctest.h>

#include "domain/rules/coin_pickup.h"
#include "domain/rules/damage.h"
#include "domain/rules/healing.h"
#include "domain/world/fruit.h"
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

TEST_CASE("Fruit bounds") {
    const Fruit fruit{16.0F, 24.0F, 8.0F};
    const AABB bounds = fruit.bounds();
    CHECK(bounds.x == 16.0F);
    CHECK(bounds.y == 24.0F);
    CHECK(bounds.width == 8.0F);
    CHECK(bounds.height == 8.0F);
}

TEST_CASE("heal") {
    Player player;

    SUBCASE("gives back hearts one at a time") {
        player.health = 1;
        CHECK(heal(player, 1) == 1);
        CHECK(player.health == 2);
    }

    SUBCASE("never goes above the maximum") {
        player.health = kMaxHealth - 1;
        CHECK(heal(player, 5) == 1);  // only one heart was missing
        CHECK(player.health == kMaxHealth);
    }

    SUBCASE("at full health nothing is gained: the fruit is wasted") {
        player.health = kMaxHealth;
        CHECK(heal(player, 1) == 0);
        CHECK(player.health == kMaxHealth);
    }

    SUBCASE("a fruit is worth one heart") {
        CHECK(kFruitHealing == 1);
    }

    SUBCASE("healing does not touch the invulnerability or anything else") {
        applyDamage(player, 1, 1);
        const int invulnerable = player.invulnerable_steps;
        heal(player, 1);
        CHECK(player.invulnerable_steps == invulnerable);
        CHECK(player.hurt_steps > 0);
    }
}

TEST_CASE("fruitsTouchedBy") {
    const Room room("r", makeMap(), {}, {}, {}, {}, {Fruit{100.0F, 208.0F}, Fruit{200.0F, 208.0F}});
    PickupCollection eaten;

    SUBCASE("finds the fruit the player overlaps") {
        const auto touched = fruitsTouchedBy(standingAt(100.0F), room, eaten);
        REQUIRE(touched.size() == 1);
        CHECK(touched[0] == 0);
    }

    SUBCASE("nothing when the player is elsewhere") {
        CHECK(fruitsTouchedBy(standingAt(150.0F), room, eaten).empty());
    }

    SUBCASE("a fruit that was eaten is not touched again") {
        eaten.collect("r", 0);
        CHECK(fruitsTouchedBy(standingAt(100.0F), room, eaten).empty());
        CHECK(fruitsTouchedBy(standingAt(200.0F), room, eaten).size() == 1);  // the other one is still there
    }

    SUBCASE("a room without fruits has nothing to touch") {
        const Room empty("e", makeMap());
        CHECK(fruitsTouchedBy(standingAt(100.0F), empty, eaten).empty());
    }
}

TEST_CASE("coins and fruits are collected independently") {
    const Room room("r", makeMap(), {}, {Coin{100.0F, 208.0F}}, {}, {}, {Fruit{100.0F, 208.0F}});
    CoinCollection coins;
    PickupCollection fruits;

    // Taking the coin does not take the fruit at the same place, and vice versa.
    coins.collect("r", 0);
    CHECK(coinsTouchedBy(standingAt(100.0F), room, coins).empty());
    CHECK(fruitsTouchedBy(standingAt(100.0F), room, fruits).size() == 1);

    fruits.collect("r", 0);
    CHECK(fruitsTouchedBy(standingAt(100.0F), room, fruits).empty());
    CHECK(coins.count() == 1);
    CHECK(fruits.count() == 1);
}
