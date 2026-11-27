#pragma once

#include <vector>

#include "domain/input/input_state.h"
#include "domain/input/menu_command.h"

namespace infrastructure {

// What the buttons and the left stick of a gamepad mean. Pure logic (no SDL), so it is unit
// tested; GamepadInputMapper reads the real device and fills a GamepadSnapshot.
//
// Buttons are named by their place, as SDL does: `south` is A on an Xbox pad and Cross on a
// PlayStation one; `east` is B or Circle.
struct GamepadSnapshot {
    bool south = false;
    bool east = false;
    bool start = false;
    bool select = false;  // Back, Share, "-"
    bool left_shoulder = false;
    bool right_shoulder = false;
    bool dpad_up = false;
    bool dpad_down = false;
    bool dpad_left = false;
    bool dpad_right = false;
    float stick_x = 0.0F;  // -1 left .. +1 right
    float stick_y = 0.0F;  // -1 up .. +1 down
};

// A stick rests a bit off centre and wobbles: it must be pushed this far to count.
inline constexpr float kStickRunThreshold = 0.35F;
// Further to crouch (and to move in a menu), so running with the stick slightly low does not.
inline constexpr float kStickDownThreshold = 0.65F;

// The player's intentions: jump with south; move and crouch with the d-pad or the stick.
domain::InputState inputFrom(const GamepadSnapshot& pad);

// Either device may be used at any moment: an action is on if any of them asks for it.
domain::InputState combine(const domain::InputState& first, const domain::InputState& second);

// What counts once per press, not while held.
struct GamepadActions {
    std::vector<domain::MenuCommand> menu;
    bool toggle_mute = false;  // select
    int music_steps = 0;       // shoulders: -1 left, +1 right
};

// The presses that happened between `before` and `now`. While playing (`in_game`) the east
// button does nothing: only start pauses.
GamepadActions actionsFrom(const GamepadSnapshot& before, const GamepadSnapshot& now, bool in_game);

}  // namespace infrastructure
