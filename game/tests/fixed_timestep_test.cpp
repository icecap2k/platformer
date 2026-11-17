#include <doctest/doctest.h>

#include "domain/time/fixed_timestep.h"

using domain::FixedTimestep;

TEST_CASE("FixedTimestep") {
    FixedTimestep timestep(10);  // 10 ns per step keeps the numbers readable

    SUBCASE("less than one step yields nothing") {
        CHECK(timestep.advance(9) == 0);
    }

    SUBCASE("exactly one step") {
        CHECK(timestep.advance(10) == 1);
    }

    SUBCASE("several steps in one call") {
        CHECK(timestep.advance(35) == 3);
    }

    SUBCASE("leftover time is kept for the next call") {
        CHECK(timestep.advance(7) == 0);
        CHECK(timestep.advance(7) == 1);  // 14 accumulated -> 1 step, 4 left
        CHECK(timestep.advance(6) == 1);  // 4 + 6 = 10 -> 1 step
    }

    SUBCASE("a long stall is capped and the backlog is dropped") {
        CHECK(timestep.advance(10'000) == FixedTimestep::kMaxStepsPerAdvance);
        CHECK(timestep.advance(0) == 0);  // nothing left to catch up on
    }
}
