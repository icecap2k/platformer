#pragma once

#include "domain/input/input_state.h"

namespace infrastructure {

// Input adapter: translates SDL keyboard events into the domain's abstract
// InputState. A future GamepadInputMapper will produce the same InputState,
// so nothing in the domain has to change when we add controller support.
class KeyboardInputMapper {
public:
    // Pumps the SDL event queue. Call once per frame, before reading state().
    void poll();

    // Current state of the abstract actions (held keys).
    domain::InputState state() const;

    // True once the window was closed or ESC was pressed.
    // Quitting is an application concern, not a game action, so it is not
    // part of InputState.
    bool quitRequested() const { return quit_requested_; }

private:
    bool quit_requested_ = false;
};

}  // namespace infrastructure
