#include "infrastructure/input/gamepad_input_mapper.h"

#include <cmath>
#include <iostream>
#include <set>

namespace infrastructure {

namespace {

float axisValue(SDL_Gamepad* pad, SDL_GamepadAxis axis) {
    return static_cast<float>(SDL_GetGamepadAxis(pad, axis)) / 32767.0F;
}

// Adds one pad to the picture: a button counts if any pad holds it, and each stick direction
// takes the pad pushed furthest.
void read(SDL_Gamepad* pad, GamepadSnapshot& all) {
    const auto down = [pad](SDL_GamepadButton button) { return SDL_GetGamepadButton(pad, button); };
    all.south = all.south || down(SDL_GAMEPAD_BUTTON_SOUTH);
    all.east = all.east || down(SDL_GAMEPAD_BUTTON_EAST);
    all.start = all.start || down(SDL_GAMEPAD_BUTTON_START);
    all.select = all.select || down(SDL_GAMEPAD_BUTTON_BACK);
    all.left_shoulder = all.left_shoulder || down(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    all.right_shoulder = all.right_shoulder || down(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
    all.dpad_up = all.dpad_up || down(SDL_GAMEPAD_BUTTON_DPAD_UP);
    all.dpad_down = all.dpad_down || down(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    all.dpad_left = all.dpad_left || down(SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    all.dpad_right = all.dpad_right || down(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    const float x = axisValue(pad, SDL_GAMEPAD_AXIS_LEFTX);
    const float y = axisValue(pad, SDL_GAMEPAD_AXIS_LEFTY);
    if (std::abs(x) > std::abs(all.stick_x)) {
        all.stick_x = x;
    }
    if (std::abs(y) > std::abs(all.stick_y)) {
        all.stick_y = y;
    }
}

}  // namespace

GamepadInputMapper::GamepadInputMapper() {
    available_ = SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    if (!available_) {
        std::cerr << "[gamepad] unavailable (" << SDL_GetError() << "): keyboard only\n";
    }
}

GamepadInputMapper::~GamepadInputMapper() {
    for (auto& [id, pad] : pads_) {
        SDL_CloseGamepad(pad);
    }
    if (available_) {
        SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    }
}

void GamepadInputMapper::update() {
    change_.reset();
    if (!available_) {
        return;
    }
    const bool had_one = connected();

    // Open the pads that have appeared and close those that are gone.
    std::set<SDL_JoystickID> present;
    int count = 0;
    if (SDL_JoystickID* ids = SDL_GetGamepads(&count)) {
        present.insert(ids, ids + count);
        SDL_free(ids);
    }
    for (const SDL_JoystickID id : present) {
        if (pads_.count(id) == 0) {
            if (SDL_Gamepad* pad = SDL_OpenGamepad(id)) {
                const char* name = SDL_GetGamepadName(pad);
                std::cout << "[gamepad] connected: " << (name != nullptr ? name : "unknown") << std::endl;
                pads_.emplace(id, pad);
            }
        }
    }
    for (auto it = pads_.begin(); it != pads_.end();) {
        if (present.count(it->first) == 0) {
            std::cout << "[gamepad] disconnected" << std::endl;
            SDL_CloseGamepad(it->second);
            it = pads_.erase(it);
        } else {
            ++it;
        }
    }
    if (had_one != connected()) {
        change_ = connected();
    }

    before_ = now_;
    now_ = GamepadSnapshot{};
    for (const auto& [id, pad] : pads_) {
        read(pad, now_);
    }
}

}  // namespace infrastructure
