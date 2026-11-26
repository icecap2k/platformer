#include <doctest/doctest.h>

#include <cstdint>

#include "level_tools/state_set.h"

using level_tools::StateSet;

TEST_CASE("StateSet accepts each key once") {
    StateSet set;
    CHECK(set.size() == 0);
    CHECK(set.insert(42));
    CHECK_FALSE(set.insert(42));
    CHECK(set.insert(0));
    CHECK_FALSE(set.insert(0));
    CHECK(set.size() == 2);
}

TEST_CASE("StateSet keeps every key while it grows") {
    StateSet set;
    constexpr std::uint64_t kCount = 200000;  // far more than the first table holds
    for (std::uint64_t i = 0; i < kCount; ++i) {
        REQUIRE(set.insert(i * 1024));  // keys that differ only in their high bits
    }
    CHECK(set.size() == kCount);
    for (std::uint64_t i = 0; i < kCount; ++i) {
        REQUIRE_FALSE(set.insert(i * 1024));
    }
    CHECK(set.size() == kCount);
}

TEST_CASE("StateSet also holds the value it uses to mark free slots") {
    StateSet set;
    const std::uint64_t all_ones = ~std::uint64_t{0};
    CHECK(set.insert(all_ones));
    CHECK_FALSE(set.insert(all_ones));
    CHECK(set.size() == 1);
}
