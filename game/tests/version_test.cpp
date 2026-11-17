#include <doctest/doctest.h>

#include "domain/version.h"

TEST_CASE("the domain compiles and links without SDL") {
    CHECK(domain::gameName() == "platformer");
}
