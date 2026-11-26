#pragma once

#include <SDL3/SDL.h>

namespace infrastructure {

// RAII wrapper around a texture that can be drawn INTO: a picture kept off screen, to be put on
// the screen later. It takes the same logical size as the screen, at `scale` real pixels each.
class RenderTarget {
public:
    // Throws std::runtime_error if the texture cannot be created.
    RenderTarget(SDL_Renderer* renderer, int logical_width, int logical_height, int scale);
    ~RenderTarget();

    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    // Everything drawn from now on goes into this picture, in logical coordinates...
    void begin() const;
    // ...until this sends the drawing back to the screen.
    void end() const;

    // Copies the picture to `destination` on whatever is being drawn to, `alpha` 0 = invisible.
    void draw(const SDL_FRect& destination, float alpha = 1.0F) const;

private:
    SDL_Renderer* renderer_;
    SDL_Texture* texture_ = nullptr;
    int logical_width_;
    int logical_height_;
};

}  // namespace infrastructure
