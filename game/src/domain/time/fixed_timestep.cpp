#include "domain/time/fixed_timestep.h"

namespace domain {

FixedTimestep::FixedTimestep(std::uint64_t step_ns) : step_ns_(step_ns) {}

int FixedTimestep::advance(std::uint64_t elapsed_ns) {
    accumulator_ns_ += elapsed_ns;

    int steps = 0;
    while (accumulator_ns_ >= step_ns_ && steps < kMaxStepsPerAdvance) {
        accumulator_ns_ -= step_ns_;
        ++steps;
    }

    // If we hit the cap, drop the backlog instead of carrying it forward.
    if (steps == kMaxStepsPerAdvance && accumulator_ns_ >= step_ns_) {
        accumulator_ns_ = 0;
    }
    return steps;
}

}  // namespace domain
