#include "infrastructure/input/gamepad_mapping.h"

namespace infrastructure {

namespace {

bool menuUp(const GamepadSnapshot& pad) {
    return pad.dpad_up || pad.stick_y < -kStickDownThreshold;
}

bool menuDown(const GamepadSnapshot& pad) {
    return pad.dpad_down || pad.stick_y > kStickDownThreshold;
}

}  // namespace

domain::InputState inputFrom(const GamepadSnapshot& pad) {
    domain::InputState input;
    input.left = pad.dpad_left || pad.stick_x < -kStickRunThreshold;
    input.right = pad.dpad_right || pad.stick_x > kStickRunThreshold;
    input.jump = pad.south;
    input.down = menuDown(pad);
    return input;
}

domain::InputState combine(const domain::InputState& first, const domain::InputState& second) {
    domain::InputState input;
    input.left = first.left || second.left;
    input.right = first.right || second.right;
    input.jump = first.jump || second.jump;
    input.down = first.down || second.down;
    return input;
}

GamepadActions actionsFrom(const GamepadSnapshot& before, const GamepadSnapshot& now, bool in_game) {
    using domain::MenuCommand;
    const auto pressed = [](bool was, bool is) { return is && !was; };

    GamepadActions actions;
    if (pressed(menuUp(before), menuUp(now))) {
        actions.menu.push_back(MenuCommand::Up);
    }
    if (pressed(menuDown(before), menuDown(now))) {
        actions.menu.push_back(MenuCommand::Down);
    }
    if (pressed(before.south, now.south)) {
        actions.menu.push_back(MenuCommand::Confirm);
    }
    if (pressed(before.east, now.east) && !in_game) {
        actions.menu.push_back(MenuCommand::Back);
    }
    if (pressed(before.start, now.start)) {
        actions.menu.push_back(MenuCommand::Pause);
    }
    actions.toggle_mute = pressed(before.select, now.select);
    actions.music_steps = (pressed(before.right_shoulder, now.right_shoulder) ? 1 : 0) -
                          (pressed(before.left_shoulder, now.left_shoulder) ? 1 : 0);
    return actions;
}

}  // namespace infrastructure
