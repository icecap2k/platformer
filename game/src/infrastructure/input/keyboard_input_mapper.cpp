#include "infrastructure/input/keyboard_input_mapper.h"

#include <SDL3/SDL.h>

namespace infrastructure {

void KeyboardInputMapper::poll() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            quit_requested_ = true;
        } else if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
            switch (event.key.scancode) {
                case SDL_SCANCODE_UP:
                case SDL_SCANCODE_W:
                    menu_commands_.push_back(domain::MenuCommand::Up);
                    break;
                case SDL_SCANCODE_DOWN:
                case SDL_SCANCODE_S:
                    menu_commands_.push_back(domain::MenuCommand::Down);
                    break;
                case SDL_SCANCODE_RETURN:
                case SDL_SCANCODE_KP_ENTER:
                case SDL_SCANCODE_SPACE:
                    menu_commands_.push_back(domain::MenuCommand::Confirm);
                    break;
                case SDL_SCANCODE_ESCAPE:
                    menu_commands_.push_back(domain::MenuCommand::Back);
                    break;
                case SDL_SCANCODE_P:
                    menu_commands_.push_back(domain::MenuCommand::Pause);
                    break;
                case SDL_SCANCODE_M:
                    mute_toggled_ = true;
                    break;
                default:
                    break;
            }
        }
    }
}

std::vector<domain::MenuCommand> KeyboardInputMapper::takeMenuCommands() {
    std::vector<domain::MenuCommand> commands;
    commands.swap(menu_commands_);
    return commands;
}

bool KeyboardInputMapper::takeMuteToggle() {
    const bool pressed = mute_toggled_;
    mute_toggled_ = false;
    return pressed;
}

domain::InputState KeyboardInputMapper::state() const {
    const bool* keys = SDL_GetKeyboardState(nullptr);

    domain::InputState input;
    input.left = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
    input.right = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
    input.jump = keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W];
    input.down = keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S];
    return input;
}

}  // namespace infrastructure
