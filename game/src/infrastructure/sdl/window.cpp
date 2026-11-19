#include "infrastructure/sdl/window.h"

#include <stdexcept>
#include <string>

namespace infrastructure {

namespace {
[[noreturn]] void fail(const char* what) {
    throw std::runtime_error(std::string(what) + ": " + SDL_GetError());
}
}  // namespace

Window::Window(const char* title, int logical_width, int logical_height, int scale) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fail("SDL_Init failed");
    }

    window_ = SDL_CreateWindow(title, logical_width * scale, logical_height * scale, 0);
    if (window_ == nullptr) {
        SDL_Quit();
        fail("SDL_CreateWindow failed");
    }

    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (renderer_ == nullptr) {
        SDL_DestroyWindow(window_);
        SDL_Quit();
        fail("SDL_CreateRenderer failed");
    }

    // Present in step with the display: without it the loop draws thousands of frames per second
    // and the ones that reach the screen are picked up at uneven moments, which looks like stutter.
    // (If the driver refuses, the game still runs, just without vsync.)
    if (!SDL_SetRenderVSync(renderer_, 1)) {
        SDL_Log("warning: vsync is not available (%s); the game may stutter", SDL_GetError());
    }

    SDL_SetRenderLogicalPresentation(renderer_, logical_width, logical_height,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
}

Window::~Window() {
    SDL_DestroyRenderer(renderer_);
    SDL_DestroyWindow(window_);
    SDL_Quit();
}

}  // namespace infrastructure
