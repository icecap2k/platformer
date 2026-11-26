#include "infrastructure/sdl/render_target.h"

#include <stdexcept>
#include <string>

namespace infrastructure {

RenderTarget::RenderTarget(SDL_Renderer* renderer, int logical_width, int logical_height, int scale)
    : renderer_(renderer), logical_width_(logical_width), logical_height_(logical_height) {
    texture_ = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
                                 logical_width * scale, logical_height * scale);
    if (texture_ == nullptr) {
        throw std::runtime_error(std::string("Cannot create a render target: ") + SDL_GetError());
    }
    SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_NEAREST);
    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
}

RenderTarget::~RenderTarget() {
    SDL_DestroyTexture(texture_);
}

void RenderTarget::begin() const {
    SDL_SetRenderTarget(renderer_, texture_);
    SDL_SetRenderLogicalPresentation(renderer_, logical_width_, logical_height_,
                                     SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);
}

void RenderTarget::end() const {
    SDL_SetRenderTarget(renderer_, nullptr);
}

void RenderTarget::draw(const SDL_FRect& destination, float alpha) const {
    SDL_SetTextureAlphaModFloat(texture_, alpha);
    SDL_RenderTexture(renderer_, texture_, nullptr, &destination);
}

}  // namespace infrastructure
