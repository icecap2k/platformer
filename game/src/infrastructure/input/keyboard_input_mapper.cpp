#include "infrastructure/input/keyboard_input_mapper.h"

#include <SDL3/SDL.h>

namespace infrastructure {

void KeyboardInputMapper::poll() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_EVENT_QUIT) {
            quit_requested_ = true;
        } else if (event.type == SDL_EVENT_KEY_DOWN && event.key.scancode == SDL_SCANCODE_ESCAPE) {
            quit_requested_ = true;
        }
    }
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
