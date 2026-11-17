#pragma once

#include <cstdint>

namespace domain {

// Converts variable real elapsed time into a whole number of fixed-size
// simulation steps. Pure logic: it never reads a clock, the caller feeds it
// the elapsed time. That is what makes it testable without SDL.
class FixedTimestep {
public:
    // Upper bound of steps per advance() call. Prevents the "spiral of death"
    // where a long stall makes the game try to catch up forever.
    static constexpr int kMaxStepsPerAdvance = 5;

    explicit FixedTimestep(std::uint64_t step_ns);

    // Registers `elapsed_ns` of real time and returns how many fixed steps
    // must be simulated now. Leftover time is kept for the next call.
    int advance(std::uint64_t elapsed_ns);

    std::uint64_t stepNs() const { return step_ns_; }

private:
    std::uint64_t step_ns_;
    std::uint64_t accumulator_ns_ = 0;
};

}  // namespace domain
