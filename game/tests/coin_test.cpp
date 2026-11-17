#include <doctest/doctest.h>

#include "domain/rules/coin_pickup.h"
#include "domain/world/coin_collection.h"
#include "domain/world/room_graph.h"
#include "test_maps.h"

using namespace domain;

namespace {

Room roomWithCoins(const RoomId& id, std::vector<Coin> coins) {
    return Room(id, test_maps::makeMap(), {}, std::move(coins));
}

Player playerAt(float x, float y) {
    Player player;  // 8 x 16
    player.x = x;
    player.y = y;
    return player;
}

}  // namespace

TEST_CASE("Coin bounds") {
    const Coin coin{10.0F, 20.0F, 8.0F};
    const AABB box = coin.bounds();

    CHECK(box.x == 10.0F);
    CHECK(box.y == 20.0F);
    CHECK(box.width == 8.0F);
    CHECK(box.height == 8.0F);
}

TEST_CASE("CoinCollection") {
    CoinCollection collection;

    SUBCASE("starts empty") {
        CHECK(collection.count() == 0);
        CHECK_FALSE(collection.isCollected("a", 0));
    }

    SUBCASE("collecting is remembered, per room and index") {
        CHECK(collection.collect("a", 0));

        CHECK(collection.isCollected("a", 0));
        CHECK_FALSE(collection.isCollected("a", 1));
        CHECK_FALSE(collection.isCollected("b", 0));
        CHECK(collection.count() == 1);
    }

    SUBCASE("collecting twice only counts once") {
        CHECK(collection.collect("a", 0));
        CHECK_FALSE(collection.collect("a", 0));
        CHECK(collection.count() == 1);
    }
}

TEST_CASE("coinsTouchedBy") {
    const Room room = roomWithCoins("a", {Coin{100.0F, 100.0F}, Coin{200.0F, 100.0F}});
    CoinCollection collection;

    SUBCASE("nothing is touched away from the coins") {
        CHECK(coinsTouchedBy(playerAt(0.0F, 0.0F), room, collection).empty());
    }

    SUBCASE("overlapping a coin touches it") {
        const auto touched = coinsTouchedBy(playerAt(96.0F, 90.0F), room, collection);
        REQUIRE(touched.size() == 1);
        CHECK(touched[0] == 0);
    }

    SUBCASE("only brushing the edge does not count") {
        // Player spans x 92..100 and the coin starts at x = 100.
        CHECK(coinsTouchedBy(playerAt(92.0F, 100.0F), room, collection).empty());
    }

    SUBCASE("collected coins are ignored") {
        collection.collect("a", 0);
        CHECK(coinsTouchedBy(playerAt(96.0F, 90.0F), room, collection).empty());
    }

    SUBCASE("a collected coin in another room does not hide this one") {
        collection.collect("other", 0);
        CHECK(coinsTouchedBy(playerAt(96.0F, 90.0F), room, collection).size() == 1);
    }
}

TEST_CASE("RoomGraph counts coins in the whole world") {
    std::vector<Room> rooms;
    rooms.push_back(roomWithCoins("a", {Coin{}}));
    rooms.push_back(roomWithCoins("b", {}));
    rooms.push_back(roomWithCoins("c", {Coin{}, Coin{}}));

    CHECK(RoomGraph(std::move(rooms)).totalCoins() == 3);
}
