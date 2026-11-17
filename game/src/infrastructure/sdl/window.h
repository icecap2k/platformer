#pragma once

#include <SDL3/SDL.h>

namespace infrastructure {

// RAII wrapper around SDL initialisation, the window and its renderer.
// The game draws on a small logical canvas that SDL scales by an integer
// factor, so pixel art stays sharp.
class Window {
public:
    // Throws std::runtime_error if SDL cannot be initialised or the window
    // cannot be created.
    Window(const char* title, int logical_width, int logical_height, int scale);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    SDL_Renderer* renderer() const { return renderer_; }

private:
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
};

}  // namespace infrastructure
