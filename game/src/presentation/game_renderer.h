#pragma once

#include <SDL3/SDL.h>

#include <string>

#include "application/game_session.h"
#include "infrastructure/sdl/texture.h"

namespace presentation {

// Draws the current state of a session. It only reads the game, never changes
// it. The only state it owns is purely visual (loaded images and which way the
// player is facing).
class GameRenderer {
public:
    // `assets_dir` must end with a path separator and contain tiled/tiles.png,
    // sprites/sprites.png and sprites/decor.png. Throws std::runtime_error if they cannot be loaded.
    GameRenderer(SDL_Renderer* renderer, const std::string& assets_dir);

    void draw(const application::GameSession& session);

private:
    void drawBackground() const;
    void drawTiles(const domain::TileMap& map) const;
    void drawScenery(const domain::TileMap& map) const;
    void drawCoins(const application::GameSession& session, std::uint64_t time_ms) const;
    void drawPlayer(const domain::Player& player, std::uint64_t time_ms);
    void drawHud(const application::GameSession& session, std::uint64_t time_ms) const;

    SDL_Renderer* renderer_;
    infrastructure::Texture tiles_;
    infrastructure::Texture sprites_;
    infrastructure::Texture decor_;
    bool facing_left_ = false;
    bool was_on_ground_ = true;
    std::uint64_t landed_at_ms_ = 0;
    bool has_landed_ = false;
};

}  // namespace presentation
