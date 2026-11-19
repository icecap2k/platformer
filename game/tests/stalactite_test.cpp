#include <doctest/doctest.h>

#include "domain/rules/damage.h"
#include "domain/world/stalactite_traps.h"
#include "test_maps.h"

using namespace domain;
using test_maps::kFloorTop;
using test_maps::makeMap;

namespace {

// Hangs from a ceiling tile at row 5 (column 12): bounds y = 48..56.
const FallingStalactite kStalactite{AABB{96.0F, 48.0F, 8.0F, 8.0F}};

TileMap mapWithCeiling() { return makeMap({{12, 5}}); }

Player playerAt(float x, float y) {
    Player player;
    player.x = x;
    player.y = y;
    return player;
}

Player standingUnder() { return playerAt(96.0F, kFloorTop - 16.0F); }

}  // namespace

TEST_CASE("playerUnder") {
    const TileMap map = mapWithCeiling();

    CHECK(playerUnder(kStalactite, map, standingUnder()));

    SUBCASE("it notices the player a little to the sides, but not far away") {
        CHECK(playerUnder(kStalactite, map, playerAt(96.0F + 10.0F, kFloorTop - 16.0F)));
        CHECK(playerUnder(kStalactite, map, playerAt(96.0F - 18.0F, kFloorTop - 16.0F)));
        CHECK_FALSE(playerUnder(kStalactite, map, playerAt(96.0F + 40.0F, kFloorTop - 16.0F)));
        CHECK_FALSE(playerUnder(kStalactite, map, playerAt(96.0F - 40.0F, kFloorTop - 16.0F)));
    }

    SUBCASE("not when the player is above it") {
        CHECK_FALSE(playerUnder(kStalactite, map, playerAt(96.0F, 8.0F)));
    }

    SUBCASE("not when a solid tile is in the way") {
        const TileMap blocked = makeMap({{12, 5}, {12, 15}});  // a ledge between them
        CHECK_FALSE(playerUnder(kStalactite, blocked, standingUnder()));
    }
}

TEST_CASE("a stalactite trap goes Idle -> Shaking -> Falling -> Gone") {
    const TileMap map = mapWithCeiling();
    StalactiteState state;

    SUBCASE("it stays put while nobody is under it") {
        for (int i = 0; i < 100; ++i) {
            CHECK_FALSE(stepStalactite(kStalactite, state, map, playerAt(10.0F, kFloorTop - 16.0F)));
        }
        CHECK(state.phase == StalactitePhase::Idle);
        CHECK(state.y_offset == 0.0F);
    }

    SUBCASE("walking under it starts a warning shake, then it comes loose exactly once") {
        const Player player = standingUnder();
        CHECK_FALSE(stepStalactite(kStalactite, state, map, player));
        CHECK(state.phase == StalactitePhase::Shaking);

        int released = 0;
        int shaking_steps = 0;
        for (int i = 0; i < 400 && state.phase != StalactitePhase::Gone; ++i) {
            if (state.phase == StalactitePhase::Shaking) {
                ++shaking_steps;
            }
            released += stepStalactite(kStalactite, state, map, player) ? 1 : 0;
        }
        CHECK(released == 1);
        CHECK(shaking_steps >= kShakeSteps - 1);  // the player had time to react
        CHECK(shaking_steps <= kShakeSteps);
    }

    SUBCASE("leaving does not cancel the warning: once shaking, it falls") {
        stepStalactite(kStalactite, state, map, standingUnder());
        REQUIRE(state.phase == StalactitePhase::Shaking);
        for (int i = 0; i < kShakeSteps; ++i) {
            stepStalactite(kStalactite, state, map, playerAt(10.0F, kFloorTop - 16.0F));
        }
        CHECK(state.phase == StalactitePhase::Falling);
    }

    SUBCASE("it falls with acceleration and shatters when it reaches the floor") {
        state.phase = StalactitePhase::Falling;
        float last_step = 0.0F;
        float previous_offset = 0.0F;
        bool accelerating = true;
        int steps = 0;
        while (state.phase == StalactitePhase::Falling && steps < 500) {
            stepStalactite(kStalactite, state, map, standingUnder());
            const float moved = state.y_offset - previous_offset;
            if (state.phase == StalactitePhase::Falling && moved + 0.001F < last_step) {
                accelerating = false;
            }
            last_step = moved;
            previous_offset = state.y_offset;
            ++steps;
        }
        CHECK(accelerating);
        CHECK(state.phase == StalactitePhase::Gone);
        // It stops at the floor: bounds.bottom() = 56 + y_offset >= floor top (216).
        CHECK(kStalactite.bounds.bottom() + state.y_offset >= kFloorTop);
        CHECK(kStalactite.bounds.bottom() + state.y_offset < kFloorTop + kStalactiteMaxFallSpeed + 0.01F);
    }

    SUBCASE("it shatters on a ledge under it instead of falling through") {
        const TileMap ledge = makeMap({{12, 5}, {12, 15}});
        state.phase = StalactitePhase::Falling;
        for (int i = 0; i < 300 && state.phase == StalactitePhase::Falling; ++i) {
            stepStalactite(kStalactite, state, ledge, playerAt(10.0F, 10.0F));
        }
        CHECK(state.phase == StalactitePhase::Gone);
        CHECK(kStalactite.bounds.bottom() + state.y_offset < 15.0F * 8.0F + kStalactiteMaxFallSpeed + 0.01F);
    }

    SUBCASE("a stalactite that is gone stays gone") {
        state.phase = StalactitePhase::Gone;
        for (int i = 0; i < 50; ++i) {
            CHECK_FALSE(stepStalactite(kStalactite, state, map, standingUnder()));
        }
        CHECK(state.phase == StalactitePhase::Gone);
    }
}

TEST_CASE("currentBounds follows the fall") {
    StalactiteState state;
    CHECK(currentBounds(kStalactite, state).y == 48.0F);
    state.y_offset = 30.0F;
    CHECK(currentBounds(kStalactite, state).y == 78.0F);
    CHECK(currentBounds(kStalactite, state).x == 96.0F);
}

TEST_CASE("StalactiteTraps") {
    StalactiteTraps traps;

    SUBCASE("untouched stalactites are Idle, and each room/index has its own state") {
        CHECK(traps.state("a", 0).phase == StalactitePhase::Idle);
        traps.state("a", 0).phase = StalactitePhase::Gone;
        CHECK(traps.state("a", 0).phase == StalactitePhase::Gone);
        CHECK(traps.state("a", 1).phase == StalactitePhase::Idle);
        CHECK(traps.state("b", 0).phase == StalactitePhase::Idle);
    }
}
