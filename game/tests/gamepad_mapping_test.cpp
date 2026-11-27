#include <doctest/doctest.h>

#include <vector>

#include "infrastructure/input/gamepad_mapping.h"

using domain::InputState;
using domain::MenuCommand;
using infrastructure::actionsFrom;
using infrastructure::combine;
using infrastructure::GamepadSnapshot;
using infrastructure::inputFrom;

TEST_CASE("an untouched gamepad asks for nothing") {
    const InputState input = inputFrom(GamepadSnapshot{});
    CHECK_FALSE(input.left);
    CHECK_FALSE(input.right);
    CHECK_FALSE(input.jump);
    CHECK_FALSE(input.down);
}

TEST_CASE("the d-pad and the south button move, crouch and jump") {
    GamepadSnapshot pad;
    pad.dpad_left = true;
    pad.south = true;
    CHECK(inputFrom(pad).left);
    CHECK(inputFrom(pad).jump);
    CHECK_FALSE(inputFrom(pad).right);

    pad = GamepadSnapshot{};
    pad.dpad_right = true;
    pad.dpad_down = true;
    CHECK(inputFrom(pad).right);
    CHECK(inputFrom(pad).down);

    pad = GamepadSnapshot{};
    pad.dpad_up = true;
    CHECK_FALSE(inputFrom(pad).jump);  // only the south button jumps
}

TEST_CASE("the stick ignores small movements and needs more push to crouch than to run") {
    GamepadSnapshot pad;
    pad.stick_x = 0.2F;
    pad.stick_y = 0.2F;
    CHECK_FALSE(inputFrom(pad).right);
    CHECK_FALSE(inputFrom(pad).down);

    pad.stick_x = -0.5F;
    pad.stick_y = 0.5F;  // running with the stick a bit low
    CHECK(inputFrom(pad).left);
    CHECK_FALSE(inputFrom(pad).down);

    pad.stick_x = 0.9F;
    pad.stick_y = 0.9F;
    CHECK(inputFrom(pad).right);
    CHECK(inputFrom(pad).down);
}

TEST_CASE("keyboard and gamepad can be used at the same time") {
    InputState keys;
    keys.left = true;
    InputState pad;
    pad.jump = true;
    const InputState both = combine(keys, pad);
    CHECK(both.left);
    CHECK(both.jump);
    CHECK_FALSE(both.right);
    CHECK_FALSE(both.down);
}

TEST_CASE("menu commands happen once per press") {
    const GamepadSnapshot idle;
    GamepadSnapshot held;
    held.dpad_down = true;
    held.south = true;
    CHECK(actionsFrom(idle, held, false).menu == std::vector<MenuCommand>{MenuCommand::Down, MenuCommand::Confirm});
    CHECK(actionsFrom(held, held, false).menu.empty());
    CHECK(actionsFrom(held, idle, false).menu.empty());  // releasing does nothing

    GamepadSnapshot stick;
    stick.stick_y = -1.0F;
    CHECK(actionsFrom(idle, stick, false).menu == std::vector<MenuCommand>{MenuCommand::Up});
}

TEST_CASE("start pauses; east goes back in menus but does nothing while playing") {
    const GamepadSnapshot idle;
    GamepadSnapshot pad;
    pad.start = true;
    CHECK(actionsFrom(idle, pad, true).menu == std::vector<MenuCommand>{MenuCommand::Pause});

    pad = GamepadSnapshot{};
    pad.east = true;
    CHECK(actionsFrom(idle, pad, false).menu == std::vector<MenuCommand>{MenuCommand::Back});
    CHECK(actionsFrom(idle, pad, true).menu.empty());
}

TEST_CASE("select mutes and the shoulders change the music") {
    const GamepadSnapshot idle;
    GamepadSnapshot pad;
    pad.select = true;
    pad.right_shoulder = true;
    CHECK(actionsFrom(idle, pad, true).toggle_mute);
    CHECK(actionsFrom(idle, pad, true).music_steps == 1);
    CHECK(actionsFrom(pad, pad, true).music_steps == 0);

    pad = GamepadSnapshot{};
    pad.left_shoulder = true;
    CHECK(actionsFrom(idle, pad, true).music_steps == -1);
    CHECK_FALSE(actionsFrom(idle, pad, true).toggle_mute);
}
