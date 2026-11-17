#include <doctest/doctest.h>

#include "domain/input/input_state.h"

using domain::InputState;

TEST_CASE("InputState horizontal axis") {
    InputState input;

    SUBCASE("no input means no movement") {
        CHECK(input.horizontalAxis() == 0);
    }

    SUBCASE("left is negative") {
        input.left = true;
        CHECK(input.horizontalAxis() == -1);
    }

    SUBCASE("right is positive") {
        input.right = true;
        CHECK(input.horizontalAxis() == 1);
    }

    SUBCASE("both directions cancel out") {
        input.left = true;
        input.right = true;
        CHECK(input.horizontalAxis() == 0);
    }
}
