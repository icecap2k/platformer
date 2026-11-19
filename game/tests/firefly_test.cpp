#include <doctest/doctest.h>

#include <algorithm>
#include <cmath>
#include <set>

#include "presentation/firefly.h"
#include "test_maps.h"

using namespace presentation;
using test_maps::makeMap;

TEST_CASE("fireflyRoutes are deterministic and every flight box is free of ground") {
    const auto map = makeMap({{10, 10}, {11, 10}, {20, 5}});
    const auto routes = fireflyRoutes("room_x", map);
    CHECK(routes == fireflyRoutes("room_x", map));
    CHECK(routes.size() <= static_cast<std::size_t>(kFirefliesPerRoom));
    CHECK(routes.size() >= 1);

    for (const FireflyRoute& route : routes) {
        const int a = std::min(route.column, route.column + route.direction * kFireflyLegTilesAcross);
        const int b = std::max(route.column, route.column + route.direction * kFireflyLegTilesAcross);
        for (int row = route.row; row <= route.row + kFireflyLegTilesDown; ++row) {
            for (int column = a; column <= b; ++column) {
                INFO("tile " << column << "," << row);
                CHECK_FALSE(map.isGround(column, row));
            }
        }
    }
}

TEST_CASE("a room with no free air has no fireflies") {
    std::vector<int> tiles(10 * 10, 1);
    const domain::TileMap solid(10, 10, 8, tiles);
    CHECK(fireflyRoutes("r", solid).empty());
}

TEST_CASE("different rooms get different routes") {
    const auto map = makeMap();
    CHECK_FALSE(fireflyRoutes("room_01", map) == fireflyRoutes("room_02", map));
}

namespace {
FireflyRoute routeGoingRight() {
    FireflyRoute route;
    route.column = 10;
    route.row = 5;
    route.direction = 1;
    route.offset_ms = 0;
    return route;
}
}  // namespace

TEST_CASE("a firefly hovers for 3 s, flies a curved leg, hovers again and comes back") {
    const FireflyRoute route = routeGoingRight();
    const float x0 = 80.0F;
    const float y0 = 40.0F;

    SUBCASE("it hovers in place for the first 3 seconds, turned the way it will go") {
        for (std::uint64_t t : {0ULL, 1000ULL, 2999ULL}) {
            const Firefly f = fireflyAt(route, t);
            CHECK(f.hovering);
            CHECK(f.x == x0);
            CHECK(std::abs(f.y - y0) <= 1.01F);  // only a faint bob
            CHECK_FALSE(f.facing_left);
        }
    }
    SUBCASE("the leg goes 5 tiles sideways and 2 down, curving") {
        const Firefly start = fireflyAt(route, kFireflyHoverMs);
        const Firefly middle = fireflyAt(route, kFireflyHoverMs + kFireflyLegMs / 2);
        const Firefly end = fireflyAt(route, kFireflyHoverMs + kFireflyLegMs - 1);
        CHECK_FALSE(start.hovering);
        CHECK(start.x == doctest::Approx(x0).epsilon(0.001));
        CHECK(end.x == doctest::Approx(x0 + 40.0F).epsilon(0.01));
        CHECK(std::abs(end.y - (y0 + 16.0F)) <= 1.01F);  // (a faint bob remains at the ends)
        // A curve, not a diagonal: half way across, but only a quarter of the way down.
        CHECK(middle.x == doctest::Approx(x0 + 20.0F).epsilon(0.01));
        CHECK(middle.y == doctest::Approx(y0 + 4.0F).epsilon(0.01));  // no bob in mid-flight
    }
    SUBCASE("it hovers 3 s at the far end, now facing back, then returns along the same curve") {
        const Firefly f = fireflyAt(route, kFireflyHoverMs + kFireflyLegMs + 500);
        CHECK(f.hovering);
        CHECK(f.facing_left);
        CHECK(f.x == x0 + 40.0F);

        const Firefly back_end = fireflyAt(route, kFireflyCycleMs - 1);
        CHECK(back_end.x == doctest::Approx(x0).epsilon(0.01));
        CHECK(std::abs(back_end.y - y0) <= 1.01F);
        CHECK(back_end.facing_left);
    }
    SUBCASE("the cycle repeats") {
        const Firefly a = fireflyAt(route, 4000);
        const Firefly b = fireflyAt(route, 4000 + kFireflyCycleMs);
        CHECK(a.x == b.x);
        CHECK(a.y == b.y);
    }
}

TEST_CASE("fireflies of a room do not start moving together") {
    const auto routes = fireflyRoutes("room_03", makeMap());
    REQUIRE(routes.size() == static_cast<std::size_t>(kFirefliesPerRoom));

    std::set<std::uint64_t> offsets;
    for (const FireflyRoute& route : routes) {
        offsets.insert(route.offset_ms);
    }
    CHECK(offsets.size() == routes.size());

    int all_flying = 0;
    int some_flying = 0;
    for (std::uint64_t t = 0; t < 4 * kFireflyCycleMs; t += 50) {
        int flying = 0;
        for (const FireflyRoute& route : routes) {
            flying += fireflyAt(route, t).hovering ? 0 : 1;
        }
        all_flying += flying == static_cast<int>(routes.size()) ? 1 : 0;
        some_flying += flying > 0 ? 1 : 0;
    }
    CHECK(some_flying > 0);
    CHECK(all_flying < some_flying / 2);
}

TEST_CASE("the firefly never jumps: its position is continuous over the whole cycle") {
    const FireflyRoute route = routeGoingRight();
    Firefly previous = fireflyAt(route, 0);
    for (std::uint64_t t = 1; t <= 2 * kFireflyCycleMs; ++t) {
        const Firefly now = fireflyAt(route, t);
        CHECK(std::abs(now.x - previous.x) <= 0.2F);
        CHECK(std::abs(now.y - previous.y) <= 0.2F);  // per millisecond
        previous = now;
    }
}

TEST_CASE("the flap and glow frames stay in 0..3 and all four appear") {
    const FireflyRoute route = routeGoingRight();
    std::set<int> frames;
    for (std::uint64_t t = 0; t < 5000; t += 10) {
        const int frame = fireflyAt(route, t).frame;
        CHECK(frame >= 0);
        CHECK(frame <= 3);
        frames.insert(frame);
    }
    CHECK(frames.size() == 4);
}
