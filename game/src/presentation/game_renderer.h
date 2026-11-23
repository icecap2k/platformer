#pragma once

#include <SDL3/SDL.h>

#include <string>
#include <vector>

#include "application/game_session.h"
#include "infrastructure/sdl/texture.h"
#include "presentation/firefly.h"

namespace presentation {

// Draws the current state of a session. It only reads the game, never changes
// it. The only state it owns is purely visual (loaded images and which way the
// player is facing).
class GameRenderer {
public:
    // `assets_dir` must end with a path separator and contain sprites/rocks.png,
    // sprites/sprites.png, sprites/decor.png, sprites/hazards.png, sprites/enemies.png, sprites/fireflies.png, sprites/checkpoints.png and background/cave.png. Throws std::runtime_error if they cannot be loaded.
    GameRenderer(SDL_Renderer* renderer, const std::string& assets_dir);

    void draw(const application::GameSession& session);

private:
    void drawCave() const;
    void drawTiles(const domain::TileMap& map) const;
    void drawScenery(const domain::TileMap& map, std::uint64_t time_ms) const;
    void drawDrip(const domain::TileMap& map, int column, int row, std::uint64_t time_ms) const;
    void drawCheckpoints(const application::GameSession& session, std::uint64_t time_ms) const;
    void drawFireflies(const application::GameSession& session, std::uint64_t time_ms);
    void drawHazards(const application::GameSession& session) const;
    void drawEnemies(const application::GameSession& session, std::uint64_t time_ms) const;
    void drawFruits(const application::GameSession& session, std::uint64_t time_ms) const;
    void drawCoins(const application::GameSession& session, std::uint64_t time_ms) const;
    void drawPlayer(const domain::Player& player, std::uint64_t time_ms);
    void drawHud(const application::GameSession& session, std::uint64_t time_ms) const;

    SDL_Renderer* renderer_;
    infrastructure::Texture rocks_;
    infrastructure::Texture sprites_;
    infrastructure::Texture decor_;
    infrastructure::Texture hazards_;
    infrastructure::Texture enemies_;
    infrastructure::Texture fireflies_;
    infrastructure::Texture checkpoints_;
    infrastructure::Texture cave_;
    // The fireflies' routes depend only on the room, so they are worked out once per room.
    std::string firefly_room_;
    std::vector<FireflyRoute> firefly_routes_;
    bool facing_left_ = false;
    bool was_on_ground_ = true;
    std::uint64_t landed_at_ms_ = 0;
    bool has_landed_ = false;
};

}  // namespace presentation
