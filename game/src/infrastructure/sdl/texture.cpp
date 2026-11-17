#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "infrastructure/sdl/texture.h"

#include <stb/stb_image.h>

#include <stdexcept>

namespace infrastructure {

Texture::Texture(SDL_Renderer* renderer, const std::string& path) {
    int width = 0;
    int height = 0;
    int channels = 0;
    // Force 4 channels (RGBA) whatever the file contains.
    unsigned char* pixels = stbi_load(path.c_str(), &width, &height, &channels, 4);
    if (pixels == nullptr) {
        throw std::runtime_error("Cannot load image '" + path + "': " + stbi_failure_reason());
    }

    SDL_Surface* surface = SDL_CreateSurfaceFrom(width, height, SDL_PIXELFORMAT_RGBA32, pixels,
                                                 width * 4);
    if (surface != nullptr) {
        texture_ = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_DestroySurface(surface);
    }
    stbi_image_free(pixels);

    if (texture_ == nullptr) {
        throw std::runtime_error("Cannot create texture for '" + path + "': " + SDL_GetError());
    }
    SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_NEAREST);
}

Texture::~Texture() {
    SDL_DestroyTexture(texture_);
}

void Texture::draw(SDL_Renderer* renderer, const SDL_FRect& source, const SDL_FRect& destination,
                   bool mirror_horizontally) const {
    if (mirror_horizontally) {
        SDL_RenderTextureRotated(renderer, texture_, &source, &destination, 0.0, nullptr,
                                 SDL_FLIP_HORIZONTAL);
    } else {
        SDL_RenderTexture(renderer, texture_, &source, &destination);
    }
}

}  // namespace infrastructure
