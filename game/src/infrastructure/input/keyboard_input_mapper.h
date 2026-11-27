#pragma once

#include <vector>

#include "domain/input/input_state.h"
#include "domain/input/menu_command.h"

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

    // True once the window was closed. (Esc is a menu command now: it pauses, resumes, or
    // leaves from the title.) Quitting is an application concern, not a game action, so it
    // is not part of InputState.
    bool quitRequested() const { return quit_requested_; }

    // True once each time the mute key (M) is pressed. Muting is an application
    // concern, like quitting, so it is not part of InputState.
    bool takeMuteToggle();

    // How many steps the music was turned up (positive) or down (negative) since the last call,
    // with the + and - keys.
    int takeMusicSteps();

    // The menu keys pressed since the last call, in order (Up/W, Down/S, Enter or Space,
    // Esc, P). Each press counts once; holding a key does not repeat it.
    std::vector<domain::MenuCommand> takeMenuCommands();

private:
    bool quit_requested_ = false;
    bool mute_toggled_ = false;
    int music_steps_ = 0;
    std::vector<domain::MenuCommand> menu_commands_;
};

}  // namespace infrastructure
