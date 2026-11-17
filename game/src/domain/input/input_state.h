#pragma once

namespace domain {

// What the player wants to do this step, expressed as abstract actions.
// The domain never knows whether they came from a keyboard, a gamepad or a
// replay file: that is the job of an input adapter in the infrastructure layer.
struct InputState {
    bool left = false;
    bool right = false;
    bool jump = false;

    // -1 = moving left, 0 = still, +1 = moving right.
    // Pressing both directions cancels out.
    int horizontalAxis() const {
        return (right ? 1 : 0) - (left ? 1 : 0);
    }
};

}  // namespace domain
