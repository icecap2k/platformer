#pragma once

#include <SDL3/SDL.h>

#include <map>
#include <optional>

#include "infrastructure/input/gamepad_mapping.h"

namespace infrastructure {

// Input adapter for gamepads, the sibling of KeyboardInputMapper: it produces the same
// InputState and MenuCommands, so nothing beyond this layer knows which device was used. Pads
// can be plugged in and out at any time; all the connected ones are read together. If SDL has no
// gamepad support the game simply goes on with the keyboard.
class GamepadInputMapper {
public:
    GamepadInputMapper();
    ~GamepadInputMapper();

    GamepadInputMapper(const GamepadInputMapper&) = delete;
    GamepadInputMapper& operator=(const GamepadInputMapper&) = delete;

    // Reads the pads. Call once per frame, after the event queue has been pumped.
    void update();

    // Current state of the abstract actions (held buttons).
    domain::InputState state() const { return inputFrom(now_); }

    // The presses since the previous update.
    GamepadActions actions(bool in_game) const { return actionsFrom(before_, now_, in_game); }

    bool connected() const { return !pads_.empty(); }

    // Set for one update when the first pad arrives (true) or the last one leaves (false).
    std::optional<bool> connectionChange() const { return change_; }

private:
    bool available_ = false;
    std::map<SDL_JoystickID, SDL_Gamepad*> pads_;
    GamepadSnapshot before_;
    GamepadSnapshot now_;
    std::optional<bool> change_;
};

}  // namespace infrastructure
