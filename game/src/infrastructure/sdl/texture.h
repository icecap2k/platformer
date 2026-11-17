#pragma once

#include <SDL3/SDL.h>

#include <string>

namespace infrastructure {

// RAII wrapper around an SDL texture loaded from a PNG file.
// Pixels are drawn with nearest-neighbour scaling so pixel art stays sharp.
class Texture {
public:
    // Throws std::runtime_error if the file cannot be read or decoded.
    Texture(SDL_Renderer* renderer, const std::string& path);
    ~Texture();

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    SDL_Texture* get() const { return texture_; }

    // Draws the `source` rectangle of the texture into `destination`.
    void draw(SDL_Renderer* renderer, const SDL_FRect& source, const SDL_FRect& destination,
              bool mirror_horizontally = false) const;

private:
    SDL_Texture* texture_ = nullptr;
};

}  // namespace infrastructure
